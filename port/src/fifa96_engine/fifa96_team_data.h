#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_pccd.h"

struct fifa96_asset_table;

/* P6: the engine team-identity source.
 *
 * `fifa96_team_data_init` reads `/FEDATA/PCCD.POG` through the engine asset
 * table, RefPack-decodes it (declared 199,265 B) into an owned arena, parses
 * the PCDB image with `fifa96_pccd_parse`, and keeps the decoded bytes alive
 * for the borrowed name pointers. It soft-fails (a negative error, `ready`
 * left 0) when the ISO/asset is absent or the decode/parse fails; no caller
 * needs to branch on availability before rendering.
 *
 * The mode-4 ("World") table has 62 slots. The port's 44-name Team Select
 * list is a menu construct (P6-team-identity §4): the table below maps each
 * LANG000 id to its mode-4 index. Fifteen mode-4 teams are not exposed
 * (P6-L1/P6-L2 in the P6 register). The loader resolves the index through
 * the mode table to the team record; there is no native team-id resolver. */

#define FIFA96_TEAM_DATA_LANG_COUNT 44
#define FIFA96_TEAM_CODE_MAX 10

struct fifa96_team_data {
  struct fifa96_pccd_db db;  /* borrowed view into `arena` */
  uint8_t *arena;            /* owned decoded PCDB image */
  size_t arena_len;
  int ready;                 /* 1 = db is parsed and usable */
};

struct fifa96_team_identity {
  int valid;                 /* 1 = the fields below are filled */
  uint32_t lang_id;          /* the menu's LANG000 id */
  uint8_t pccd_index;        /* mode-4 slot index 0..61 */
  uint8_t pccd_record;       /* team record index 0..241 (the `rid`) */
  uint32_t pccd_id;          /* record +0x00 (CTEAM/TC key) */
  const char *name_long;     /* record +0x04 pool string */
  const char *name_short;    /* record +0x08 pool string */
  const char *name_medium;   /* record +0x0C pool string */
  char code[FIFA96_TEAM_CODE_MAX];  /* record +0x08, uppercased, max 9 */
  uint8_t formation;         /* record +0x12 */
  uint8_t kit[4][3];         /* record +0x25..+0x30 */
  uint8_t print[4];          /* record +0x31..+0x34 */
  uint32_t roster[FIFA96_PCCD_ROSTER_COUNT];  /* record +0x49.. */
};

/* Read/decode/parse `/FEDATA/PCCD.POG`. `td` must be zeroed or previously
 * shut down; init overwrites it (call shutdown first to re-init). Returns 0 on
 * success, -FIFA96_ERR_INVALID (NULL td), or a negative loader/decode error
 * (with td->ready == 0 and no allocation retained). */
int fifa96_team_data_init(struct fifa96_team_data *td,
                          const struct fifa96_asset_table *assets);

/* Free the owned arena and clear `td`. NULL is a no-op. */
void fifa96_team_data_shutdown(struct fifa96_team_data *td);

/* Resolve a LANG000 team id through the 44-entry map into the mode-4 table.
 * Fills `identity` (name pointers borrow `td`'s arena) and returns 0, or
 * -FIFA96_ERR_INVALID (NULL), -FIFA96_ERR_STATE (not initialised), or
 * -FIFA96_ERR_NOT_FOUND (lang_id not in the exposed 44). Unknown/absent data
 * leaves `identity` zeroed. */
int fifa96_team_data_lookup(const struct fifa96_team_data *td, uint32_t lang_id,
                            struct fifa96_team_identity *identity);

/* Resolve one roster slot (0..15) of `lang_id`'s exposed team: the slot's
 * `identity.roster[slot]` player-table index is passed to
 * `fifa96_pccd_player`, and the player's name is copied into `name` with a
 * terminating NUL (truncated to `cap - 1` bytes when needed). The PCCD player
 * record has no shirt-number field, so `number` is the stable 1-based roster
 * slot (`slot + 1`), independent of the player id. Like `lookup`, the name is
 * copied out of `td`'s borrowed arena; parse results borrow `td`'s database.
 * Returns 0, -FIFA96_ERR_INVALID (NULL `td`/`name`/`number`, `cap == 0`, or
 * `slot > 15`), -FIFA96_ERR_STATE (not initialised), -FIFA96_ERR_NOT_FOUND
 * (lang_id not exposed or the roster slot names a missing player), or another
 * negated PCCD parser error. */
int fifa96_team_data_player(const struct fifa96_team_data *td, uint32_t lang_id,
                            uint8_t slot, char *name, size_t cap,
                            uint8_t *number);

/* Fill all 16 roster names/numbers for `lang_id` into `names`/`numbers` and set
 * `*count` to 16. A missing player or an empty name becomes "PLAYER n" with
 * number n, where n is the 1-based slot (`slot + 1`). Returns 0,
 * -FIFA96_ERR_INVALID (any NULL argument), -FIFA96_ERR_STATE (not
 * initialised), -FIFA96_ERR_NOT_FOUND (lang_id not exposed), or another
 * negated PCCD parser error; on error `*count` is left 0. */
int fifa96_team_data_roster(const struct fifa96_team_data *td, uint32_t lang_id,
                            char names[16][24], uint8_t numbers[16],
                            int *count);

/* The 44-entry map's mode-4 index for a LANG id, or -1 when the id is not one
 * of the exposed teams. Pure: works without an initialised team data. */
int fifa96_team_data_lang_index(uint32_t lang_id);

/* Number of exposed teams (44). */
int fifa96_team_data_lang_count(void);
