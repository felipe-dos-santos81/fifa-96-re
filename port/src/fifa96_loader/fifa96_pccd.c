#include "fifa96_loader/fifa96_pccd.h"

#include <string.h>

#define PCCD_HEADER_MIN 0x1Cu
#define PCCD_MODES_OFF 0x1Cu
#define PCCD_TEAM_HEADER 0x08u
#define PCCD_PLAYER_HEADER 0x08u

static uint32_t pccd_rd32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

/* Bounds-checked view of a NUL-terminated pool string. `off` is pool-relative
 * (the record stores the pool offset, not a file offset). */
static fifa96_err_t pccd_string(const struct fifa96_pccd_db *db, uint32_t off,
                                const char **out) {
  if (!db || !out || !db->bytes) return FIFA96_ERR_INVALID;
  if (db->string_pool >= db->len) return FIFA96_ERR_TRUNCATED;
  size_t avail = db->len - db->string_pool;
  if (off >= avail) return FIFA96_ERR_TRUNCATED;
  const uint8_t *p = db->bytes + db->string_pool + off;
  if (memchr(p, 0, avail - off) == NULL) return FIFA96_ERR_TRUNCATED;
  *out = (const char *)p;
  return FIFA96_OK;
}

/* True when `[off, off+need)` fits `len`, without overflowing off+need. */
static int pccd_range(size_t len, uint32_t off, size_t need) {
  return (size_t)off <= len && need <= len - (size_t)off;
}

fifa96_err_t fifa96_pccd_parse(const uint8_t *bytes, size_t len,
                               struct fifa96_pccd_db *db) {
  if (!bytes || !db) return FIFA96_ERR_INVALID;
  memset(db, 0, sizeof *db);
  if (len < PCCD_HEADER_MIN) return FIFA96_ERR_TRUNCATED;
  if (memcmp(bytes, "PCDB", 4) != 0) return FIFA96_ERR_BAD_MAGIC;

  uint32_t header_size = pccd_rd32(bytes + 0x04);
  uint32_t team_block = pccd_rd32(bytes + 0x08);
  uint32_t player_block = pccd_rd32(bytes + 0x0C);
  uint32_t string_pool = pccd_rd32(bytes + 0x10);
  uint32_t mode_base = pccd_rd32(bytes + 0x14);
  uint32_t mode_count = pccd_rd32(bytes + 0x18);
  if (header_size < 0x14u || header_size > len) return FIFA96_ERR_TRUNCATED;
  if (mode_count == 0 || mode_count > FIFA96_PCCD_MODE_COUNT_MAX)
    return FIFA96_ERR_TRUNCATED;
  if (mode_base > len) return FIFA96_ERR_TRUNCATED;
  if (!pccd_range(len, PCCD_MODES_OFF, (size_t)mode_count * 4u))
    return FIFA96_ERR_TRUNCATED;

  /* Team block: fixed-record size + count + the offset table. */
  if (!pccd_range(len, team_block, PCCD_TEAM_HEADER)) return FIFA96_ERR_TRUNCATED;
  uint32_t team_size = pccd_rd32(bytes + team_block);
  uint32_t team_count = pccd_rd32(bytes + team_block + 4);
  if (team_size != FIFA96_PCCD_TEAM_FIXED_SIZE) return FIFA96_ERR_UNSUPPORTED;
  if (team_count == 0 || team_count > FIFA96_PCCD_TEAM_COUNT_MAX)
    return FIFA96_ERR_TRUNCATED;
  uint32_t team_offsets = team_block + PCCD_TEAM_HEADER;
  if (!pccd_range(len, team_offsets, (size_t)team_count * 4u))
    return FIFA96_ERR_TRUNCATED;

  /* Player block: fixed stride + count + the records. */
  if (!pccd_range(len, player_block, PCCD_PLAYER_HEADER)) return FIFA96_ERR_TRUNCATED;
  uint32_t player_stride = pccd_rd32(bytes + player_block);
  uint32_t player_count = pccd_rd32(bytes + player_block + 4);
  if (player_stride != FIFA96_PCCD_PLAYER_STRIDE) return FIFA96_ERR_UNSUPPORTED;
  if (player_count == 0 || player_count > FIFA96_PCCD_PLAYER_COUNT_MAX)
    return FIFA96_ERR_TRUNCATED;
  uint32_t player_records = player_block + PCCD_PLAYER_HEADER;
  if (!pccd_range(len, player_records,
                  (size_t)player_count * FIFA96_PCCD_PLAYER_STRIDE))
    return FIFA96_ERR_TRUNCATED;

  /* String pool: must be inside the image and may not overlap the players. */
  size_t player_end =
      (size_t)player_records + (size_t)player_count * FIFA96_PCCD_PLAYER_STRIDE;
  if ((size_t)string_pool < player_end || (size_t)string_pool >= len)
    return FIFA96_ERR_TRUNCATED;

  uint32_t mode_start[FIFA96_PCCD_MODE_COUNT_MAX] = {0};
  uint32_t mode_slots[FIFA96_PCCD_MODE_COUNT_MAX] = {0};
  for (uint32_t m = 0; m < mode_count; m++) {
    uint32_t rel = pccd_rd32(bytes + PCCD_MODES_OFF + m * 4u);
    if (!pccd_range(len, mode_base, rel)) return FIFA96_ERR_TRUNCATED;
    uint32_t abs = mode_base + rel;
    uint32_t next;
    if (m + 1u < mode_count) {
      uint32_t next_rel = pccd_rd32(bytes + PCCD_MODES_OFF + (m + 1u) * 4u);
      if (!pccd_range(len, mode_base, next_rel)) return FIFA96_ERR_TRUNCATED;
      next = mode_base + next_rel;
    } else {
      next = team_block;
    }
    if (next <= abs || (next - abs) % 4u != 0) return FIFA96_ERR_TRUNCATED;
    uint32_t slots = (next - abs) / 4u;
    if (slots == 0 || !pccd_range(len, abs, (size_t)slots * 4u))
      return FIFA96_ERR_TRUNCATED;
    for (uint32_t i = 0; i < slots; i++)
      if (pccd_rd32(bytes + abs + i * 4u) >= team_count)
        return FIFA96_ERR_TRUNCATED;
    mode_start[m] = abs;
    mode_slots[m] = slots;
  }

  /* Every team record offset is a dword inside the team block and the full
   * 0x89-byte record fits before the player block. */
  uint32_t team_table_end = team_offsets + team_count * 4u;
  for (uint32_t r = 0; r < team_count; r++) {
    uint32_t rec = pccd_rd32(bytes + team_offsets + r * 4u);
    if (rec < team_table_end || rec > player_block ||
        player_block - rec < FIFA96_PCCD_TEAM_RECORD_SIZE)
      return FIFA96_ERR_TRUNCATED;
  }

  db->bytes = bytes;
  db->len = len;
  db->header_size = header_size;
  db->team_block = team_block;
  db->team_size = team_size;
  db->team_count = team_count;
  db->team_offsets = team_offsets;
  db->player_block = player_block;
  db->player_stride = player_stride;
  db->player_count = player_count;
  db->player_records = player_records;
  db->string_pool = string_pool;
  db->mode_base = mode_base;
  db->mode_count = mode_count;
  db->mode_table = PCCD_MODES_OFF;
  memcpy(db->mode_start, mode_start, sizeof mode_start);
  memcpy(db->mode_slots, mode_slots, sizeof mode_slots);
  return FIFA96_OK;
}

uint32_t fifa96_pccd_mode_slots(const struct fifa96_pccd_db *db, uint32_t mode) {
  if (!db || !db->bytes || mode >= db->mode_count) return 0;
  return db->mode_slots[mode];
}

fifa96_err_t fifa96_pccd_team(const struct fifa96_pccd_db *db, uint32_t mode,
                              uint32_t index, struct fifa96_pccd_team *team) {
  if (!db || !db->bytes || !team) return FIFA96_ERR_INVALID;
  if (mode >= db->mode_count || index >= db->mode_slots[mode])
    return FIFA96_ERR_NOT_FOUND;
  uint32_t rid = pccd_rd32(db->bytes + db->mode_start[mode] + index * 4u);
  if (rid >= db->team_count) return FIFA96_ERR_TRUNCATED;
  uint32_t rec = pccd_rd32(db->bytes + db->team_offsets + rid * 4u);
  if (!pccd_range(db->len, rec, FIFA96_PCCD_TEAM_RECORD_SIZE))
    return FIFA96_ERR_TRUNCATED;
  const uint8_t *p = db->bytes + rec;
  memset(team, 0, sizeof *team);
  team->record = rid;
  team->id = pccd_rd32(p);
  fifa96_err_t err = pccd_string(db, pccd_rd32(p + 0x04), &team->name_long);
  if (err != FIFA96_OK) return err;
  err = pccd_string(db, pccd_rd32(p + 0x08), &team->name_short);
  if (err != FIFA96_OK) return err;
  err = pccd_string(db, pccd_rd32(p + 0x0C), &team->name_medium);
  if (err != FIFA96_OK) return err;
  team->trait = p[0x10];
  team->formation = p[0x12];
  for (uint32_t k = 0; k < 4; k++)
    for (uint32_t c = 0; c < 3; c++)
      team->kit[k][c] = p[0x25 + k * 3u + c];
  memcpy(team->print, p + 0x31, sizeof team->print);
  for (uint32_t i = 0; i < FIFA96_PCCD_ROSTER_COUNT; i++)
    team->order_key[i] = (int8_t)p[0x35 + i];
  for (uint32_t i = 0; i < 4; i++) team->ai[i] = (int8_t)p[0x1E + i];
  for (uint32_t i = 0; i < FIFA96_PCCD_ROSTER_COUNT; i++)
    team->roster[i] = pccd_rd32(p + FIFA96_PCCD_TEAM_FIXED_SIZE + i * 4u);
  return FIFA96_OK;
}

fifa96_err_t fifa96_pccd_player(const struct fifa96_pccd_db *db, uint32_t index,
                                struct fifa96_pccd_player *player) {
  if (!db || !db->bytes || !player) return FIFA96_ERR_INVALID;
  if (index >= db->player_count) return FIFA96_ERR_NOT_FOUND;
  const uint8_t *p = db->bytes + db->player_records + (size_t)index * db->player_stride;
  memset(player, 0, sizeof *player);
  player->record = index;
  player->id = pccd_rd32(p);
  uint32_t name_off = (uint32_t)p[0x05] | ((uint32_t)p[0x06] << 8) |
                      ((uint32_t)p[0x07] << 16);
  fifa96_err_t err = pccd_string(db, name_off, &player->name);
  if (err != FIFA96_OK) return err;
  player->byte8 = p[0x08];
  player->position = (char)p[0x09];
  memcpy(player->skills, p + 0x0A, sizeof player->skills);
  player->sort_key = (int8_t)p[0x0B];
  player->rating = p[0x1B];
  player->hair_code = p[0x1C];
  player->skin_code = p[0x1D];
  return FIFA96_OK;
}

fifa96_err_t fifa96_pccd_lineup_order(
    const struct fifa96_pccd_db *db,
    const uint32_t roster[FIFA96_PCCD_ROSTER_COUNT],
    const int8_t order_key[FIFA96_PCCD_ROSTER_COUNT],
    uint8_t order[FIFA96_PCCD_ROSTER_COUNT]) {
  if (!db || !db->bytes || !roster || !order_key || !order)
    return FIFA96_ERR_INVALID;
  int8_t position[FIFA96_PCCD_ROSTER_COUNT];
  int8_t sort_key[FIFA96_PCCD_ROSTER_COUNT];
  for (uint32_t slot = 0; slot < FIFA96_PCCD_ROSTER_COUNT; slot++) {
    struct fifa96_pccd_player player;
    fifa96_err_t err = fifa96_pccd_player(db, roster[slot], &player);
    if (err != FIFA96_OK) return err;
    position[slot] = (int8_t)player.position;
    sort_key[slot] = player.sort_key;
  }
  /* The native `FUN_00012010` fallback template at 0x10010. -1 is
   * "any position"; it must stay signed (a plain `char` is unsigned on
   * some targets and would turn 0xFF into 255). */
  static const int8_t fallback[FIFA96_PCCD_ROSTER_COUNT] = {
      'G', 'D', 'D', 'D', 'D', 'M', 'M', 'M', 'M', 'F', 'F',
      -1, -1, -1, -1, -1};
  uint8_t used[FIFA96_PCCD_ROSTER_COUNT] = {0};
  for (uint32_t i = 0; i < FIFA96_PCCD_ROSTER_COUNT; i++) {
    int best = -1;
    int best_key = 128;
    for (uint32_t slot = 0; slot < FIFA96_PCCD_ROSTER_COUNT; slot++) {
      if (used[slot]) continue;
      int key = order_key[slot];
      if (key != -1 && key < best_key) {
        best_key = key;
        best = (int)slot;
      }
    }
    if (best < 0) {
      int8_t key = fallback[i];
      best_key = 128;
      for (uint32_t slot = 0; slot < FIFA96_PCCD_ROSTER_COUNT; slot++) {
        if (used[slot]) continue;
        if (key != -1 && position[slot] != key) continue;
        if (best < 0 || sort_key[slot] < best_key) {
          best = (int)slot;
          best_key = sort_key[slot];
        }
      }
    }
    if (best < 0) {
      /* Malformed roster (no slot matches the position): the native
       * FUN_00011E48 falls back to the first unused slot. */
      for (uint32_t slot = 0; slot < FIFA96_PCCD_ROSTER_COUNT; slot++) {
        if (!used[slot]) {
          best = (int)slot;
          break;
        }
      }
    }
    if (best < 0) return FIFA96_ERR_NOT_FOUND;
    order[i] = (uint8_t)best;
    used[best] = 1;
  }
  return FIFA96_OK;
}
