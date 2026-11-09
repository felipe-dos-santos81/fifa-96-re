#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

/* P6: the PCCD/PCDB team database (clean-room layout).
 *
 * `/FEDATA/PCCD.POG` is a RefPack record (selector 0x10, declared
 * 0x030A61 = 199,265 B); the decoded image starts with "PCDB" and holds the
 * team/player tables. Offsets are PCDB image offsets, first-hand from the
 * retail decode (P6-team-identity §1/§2):
 *
 *   +0x00 "PCDB"
 *   +0x04 0x14     header/table base byte count
 *   +0x08 0x4B4    team-table block
 *   +0x0C 0x8A06   player-table block
 *   +0x10 0x2866A  string pool (NUL-terminated ASCII)
 *   +0x14 0x0C     mode-table base offset
 *   +0x18 0x0D     mode count (13)
 *   +0x1C          13 dword mode offsets, relative to +0x14's base
 *   +0x4B4 0x49    team fixed-record size (the roster follows)
 *   +0x4B8 0xF2    team record count (242)
 *   +0x4BC          242 dword record offsets
 *   +0x8A06 0x24   player stride
 *   +0x8A0A 0xE1F  player count (3615)
 *   +0x8A0E         3615 player records
 *
 * A team record is 0x89 B: the 0x49 fixed fields plus 16 roster dwords.
 * Mode 4 ("World") is the port's team-select mode; its slot table is
 * 0x1BC..0x2B3 (62 slots, 59 distinct teams + three duplicate tails).
 *
 * This is a pure byte parser: no file I/O, no allocation, no writes to the
 * source. Every table and record access is bounds-checked; malformed input is
 * an error return, never a crash.
 *
 * P6 erratum vs P6-team-identity §2/§3: the retail team kit triples are at
 * +0x25/+0x28/+0x2B/+0x2E (3 bytes each) and the player overall rating is at
 * +0x1B (Bebeto 95, Dunga 90); those two report offsets are off by one field
 * size. The values are the report's stated measured values. */

#define FIFA96_PCCD_MODE_WORLD 4u
#define FIFA96_PCCD_MODE_COUNT_MAX 13u
#define FIFA96_PCCD_TEAM_COUNT_MAX 242u
#define FIFA96_PCCD_TEAM_FIXED_SIZE 0x49u
#define FIFA96_PCCD_TEAM_RECORD_SIZE 0x89u
#define FIFA96_PCCD_PLAYER_STRIDE 0x24u
#define FIFA96_PCCD_PLAYER_COUNT_MAX 3615u
#define FIFA96_PCCD_ROSTER_COUNT 16u

struct fifa96_pccd_db {
  const uint8_t *bytes;       /* borrowed decoded PCDB image */
  size_t len;
  uint32_t header_size;       /* +0x04 */
  uint32_t team_block;        /* +0x08 */
  uint32_t team_size;         /* +0x4B4 (fixed record size, 0x49) */
  uint32_t team_count;        /* +0x4B8 */
  uint32_t team_offsets;      /* +0x4BC */
  uint32_t player_block;      /* +0x0C */
  uint32_t player_stride;     /* +0x8A06 */
  uint32_t player_count;      /* +0x8A0A */
  uint32_t player_records;    /* +0x8A0E */
  uint32_t string_pool;       /* +0x10 */
  uint32_t mode_base;         /* +0x14 */
  uint32_t mode_count;        /* +0x18 */
  uint32_t mode_table;        /* +0x1C */
  uint32_t mode_start[FIFA96_PCCD_MODE_COUNT_MAX];  /* absolute slot tables */
  uint32_t mode_slots[FIFA96_PCCD_MODE_COUNT_MAX];  /* dword slots per mode */
};

struct fifa96_pccd_team {
  uint32_t record;            /* mode-table record index (the `rid`) */
  uint32_t id;                /* +0x00 PCCD team id (CTEAM/TC key) */
  const char *name_long;      /* +0x04 pool offset */
  const char *name_short;     /* +0x08 pool offset (the HUD pick) */
  const char *name_medium;    /* +0x0C pool offset */
  uint8_t trait;              /* +0x10 */
  uint8_t formation;          /* +0x12 formation id 0..4 */
  uint8_t kit[4][3];          /* +0x25/+0x28/+0x2B/+0x2E shirt/shorts/socks */
  uint8_t print[4];           /* +0x31 number colours (ASCII 'A'..'Y') */
  int8_t ai[4];               /* +0x1E signed AI bounds */
  uint32_t roster[FIFA96_PCCD_ROSTER_COUNT];  /* +0x49 player record indices */
};

struct fifa96_pccd_player {
  uint32_t record;            /* player-table index (the roster value) */
  uint32_t id;                /* +0x00 player id */
  const char *name;           /* +0x05 u24 pool offset */
  uint8_t byte8;              /* +0x08 */
  char position;              /* +0x09 ASCII 'G'/'D'/'M'/'F' */
  uint8_t rating;             /* +0x1B overall (measured; see the erratum) */
  uint8_t skills[17];         /* +0x0A..+0x1A */
  /* Native player-record appearance digit codes. FUN_0004C00C reads the
   * hair digit at +0x1D and FUN_0004C098 the skin digit at +0x1E of the
   * runtime player descriptor; the PCCD record carries the same pair at
   * +0x1C/+0x1D (the runtime descriptor base is one byte below the PCCD
   * record in the native lineup matrix). Values are ASCII '0'..'4'. */
  uint8_t hair_code;          /* PCCD +0x1C */
  uint8_t skin_code;          /* PCCD +0x1D */
};

/* Validate `bytes` as a PCDB image. Returns FIFA96_OK with `db` filled, or
 * FIFA96_ERR_INVALID (NULL), FIFA96_ERR_BAD_MAGIC, or FIFA96_ERR_TRUNCATED
 * (short header/table, a bad offset or count, an out-of-range record, or a
 * missing NUL in the pool). The source is borrowed and must outlive `db`. */
fifa96_err_t fifa96_pccd_parse(const uint8_t *bytes, size_t len,
                               struct fifa96_pccd_db *db);

/* Resolve `(mode,index)` to a team record and fill `team`:
 *   modeoff = dword[0x1C + 4*mode]; rid = dword[mode_base + modeoff + 4*index];
 *   rec = dword[team_offsets + 4*rid].
 * Returns FIFA96_OK, FIFA96_ERR_INVALID (NULL), or FIFA96_ERR_NOT_FOUND
 * (mode/index outside the parsed tables) / FIFA96_ERR_TRUNCATED (a record or
 * pool string outside the image). `team->name_*` borrow `db->bytes`. */
fifa96_err_t fifa96_pccd_team(const struct fifa96_pccd_db *db, uint32_t mode,
                              uint32_t index, struct fifa96_pccd_team *team);

/* Resolve a player-table index (0..3614, the values stored in the team
 * roster) and fill `player`. Returns FIFA96_OK, FIFA96_ERR_INVALID (NULL), or
 * FIFA96_ERR_NOT_FOUND (index >= player_count). `player->name` borrows
 * `db->bytes`. */
fifa96_err_t fifa96_pccd_player(const struct fifa96_pccd_db *db, uint32_t index,
                                struct fifa96_pccd_player *player);

/* Number of dword slots in `mode`'s table, or 0 for a NULL db / bad mode. */
uint32_t fifa96_pccd_mode_slots(const struct fifa96_pccd_db *db, uint32_t mode);
