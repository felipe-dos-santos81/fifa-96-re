/* port/tests/test_engine_pccd.c — P6: the PCCD/PCDB team database parser.
 *
 * The retail case is ISO-gated: /FEDATA/PCCD.POG is read through the engine
 * asset table and RefPack-decoded (declared 199,265 B) before the pure parser
 * sees it. The synthetic cases pin the bad-input contract (magic, truncation,
 * counts, stride, dead record offsets, out-of-range mode slots).
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_pccd.h"
#include "fifa96_loader/fifa96_record.h"

#define PCCD_ISO_PATH "data/game/FIFAPCCD96.iso"

static int file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

/* Decode the retail PCDB into an owned arena. Returns 0 when the ISO is
 * absent (the caller skips). */
static int load_retail_pcdb(uint8_t **out, size_t *out_len) {
  *out = NULL;
  *out_len = 0;
  if (!file_exists(PCCD_ISO_PATH)) return 0;
  struct fifa96_asset_table *assets = NULL;
  assert(fifa96_asset_mount_file(PCCD_ISO_PATH, &assets) == FIFA96_OK);
  uint8_t *raw = NULL;
  size_t raw_len = 0;
  fifa96_err_t err = fifa96_asset_read(assets, "/FEDATA/PCCD.POG", &raw, &raw_len);
  fifa96_asset_unmount(assets);
  assert(err == FIFA96_OK && raw != NULL);
  size_t declared = ((size_t)raw[2] << 16) | ((size_t)raw[3] << 8) | (size_t)raw[4];
  assert(declared == 199265u);
  uint8_t *arena = malloc(declared);
  assert(arena != NULL);
  size_t got = 0;
  assert(fifa96_record_decode(raw, raw_len, arena, declared, &got) == 0);
  assert(got == declared);
  fifa96_asset_free(raw);
  *out = arena;
  *out_len = got;
  return 1;
}

static void test_bad_input(void) {
  struct fifa96_pccd_db db;
  uint8_t buf[0x40];
  memset(buf, 0, sizeof buf);
  assert(fifa96_pccd_parse(NULL, 0, &db) == FIFA96_ERR_INVALID);
  assert(fifa96_pccd_parse(buf, sizeof buf, NULL) == FIFA96_ERR_INVALID);
  memset(buf, 'X', sizeof buf);
  assert(fifa96_pccd_parse(buf, sizeof buf, &db) == FIFA96_ERR_BAD_MAGIC);
  memset(buf, 0, sizeof buf);
  memcpy(buf, "PCDB", 4);
  assert(fifa96_pccd_parse(buf, sizeof buf, &db) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_pccd_team(NULL, 0, 0, NULL) == FIFA96_ERR_INVALID);
  assert(fifa96_pccd_player(NULL, 0, NULL) == FIFA96_ERR_INVALID);
}

static void test_retail_pcdb(void) {
  uint8_t *arena = NULL;
  size_t len = 0;
  if (!load_retail_pcdb(&arena, &len)) {
    fprintf(stderr, "SKIP retail PCDB case (no %s)\n", PCCD_ISO_PATH);
    return;
  }
  struct fifa96_pccd_db db;
  assert(fifa96_pccd_parse(arena, len, &db) == FIFA96_OK);
  assert(db.team_block == 0x4B4u);
  assert(db.team_size == 0x49u && db.team_count == 242u);
  assert(db.player_block == 0x8A06u);
  assert(db.player_stride == 0x24u && db.player_count == 3615u);
  assert(db.string_pool == 0x2866Au);
  assert(db.mode_base == 0x0Cu && db.mode_count == 13u);
  assert(fifa96_pccd_mode_slots(&db, 4) == 62u);
  uint32_t mode4_start = db.mode_start[4];

  struct fifa96_pccd_team team;
  assert(fifa96_pccd_team(&db, 4, 6, &team) == FIFA96_OK);
  assert(team.record == 0x2Bu);
  assert(team.id == 59u);
  assert(strcmp(team.name_long, "Brazil") == 0);
  assert(strcmp(team.name_short, "Bra") == 0);
  assert(strcmp(team.name_medium, "Brazil") == 0);
  assert(team.formation == 1u);
  assert(team.kit[0][0] == 26u && team.kit[0][1] == 4u && team.kit[0][2] == 7u);
  assert(team.kit[1][0] == 3u && team.kit[1][1] == 30u && team.kit[1][2] == 30u);
  assert(team.print[0] == 'D' && team.print[1] == 'D');
  assert(team.roster[0] == 2047u && team.roster[15] == 106u);

  assert(fifa96_pccd_team(&db, 4, 28, &team) == FIFA96_OK);
  assert(team.record == 0x20u && team.id == 47u);
  assert(strcmp(team.name_short, "Italy") == 0);
  assert(team.formation == 2u);
  assert(team.kit[0][0] == 3u && team.kit[0][1] == 30u && team.kit[0][2] == 3u);
  assert(team.roster[0] == 806u && team.roster[15] == 168u);

  struct fifa96_pccd_player player;
  assert(fifa96_pccd_player(&db, 2047, &player) == FIFA96_OK);
  assert(player.id == 4543u);
  assert(strcmp(player.name, "Bebeto") == 0);
  assert(player.position == 'F');
  assert(player.rating == 95u);
  assert(fifa96_pccd_player(&db, 1034, &player) == FIFA96_OK);
  assert(strcmp(player.name, "Dunga") == 0);
  assert(player.rating == 90u);

  /* Every exposed mode-4 slot resolves to a record with valid names and the
   * 0..4 formation id. */
  for (uint32_t i = 0; i < 62u; i++) {
    assert(fifa96_pccd_team(&db, 4, i, &team) == FIFA96_OK);
    assert(team.id != 0u);
    assert(team.name_long != NULL && team.name_short != NULL && team.name_medium != NULL);
    assert(team.formation <= 4u);
  }
  assert(fifa96_pccd_team(&db, 4, 62, &team) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_pccd_team(&db, 13, 0, &team) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_pccd_player(&db, 3615, &player) == FIFA96_ERR_NOT_FOUND);

  /* Corruptions of the real image are errors, not crashes. */
  uint8_t *copy = malloc(len);
  assert(copy != NULL);
  memcpy(copy, arena, len);
  copy[0] = 'X';
  assert(fifa96_pccd_parse(copy, len, &db) == FIFA96_ERR_BAD_MAGIC);
  memcpy(copy, arena, len);
  put32le(copy + 0x4B8, 0u);
  assert(fifa96_pccd_parse(copy, len, &db) == FIFA96_ERR_TRUNCATED);
  memcpy(copy, arena, len);
  put32le(copy + 0x8A06, 0x25u);
  assert(fifa96_pccd_parse(copy, len, &db) == FIFA96_ERR_UNSUPPORTED);
  memcpy(copy, arena, len);
  put32le(copy + 0x4BC, 0u);
  assert(fifa96_pccd_parse(copy, len, &db) == FIFA96_ERR_TRUNCATED);
  memcpy(copy, arena, len);
  put32le(copy + mode4_start, 0xFFFFu);
  assert(fifa96_pccd_parse(copy, len, &db) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_pccd_parse(arena, 0x100u, &db) == FIFA96_ERR_TRUNCATED);
  free(copy);
  free(arena);
}

int main(void) {
  test_bad_input();
  test_retail_pcdb();
  puts("test_engine_pccd OK");
  return 0;
}
