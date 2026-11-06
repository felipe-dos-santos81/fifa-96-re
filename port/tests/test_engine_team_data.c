/* port/tests/test_engine_team_data.c — P6: engine team identity from PCCD.
 *
 * The 44 LANG-id -> mode-4 map and the menu handoff are pure/static and run
 * everywhere; the retail identity fields (Brazil 6 / Italy 28, ids 59/47,
 * formations 1/2, kit triples, codes BRA/ITALY) are ISO-gated. No-ISO lookup
 * must soft-fail.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_menu_model.h"
#include "fifa96_engine/fifa96_team_data.h"
#include "fifa96_loader/fifa96_err.h"

#define TEAM_DATA_ISO_PATH "data/game/FIFAPCCD96.iso"

static int file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static void test_mapping(void) {
  assert(fifa96_team_data_lang_count() == 44);
  assert(fifa96_team_data_lang_index(0x023u) == 6);   /* Brazil */
  assert(fifa96_team_data_lang_index(0x0A6u) == 28);  /* Italy */
  assert(fifa96_team_data_lang_index(0x013u) == 1);   /* Argentina */
  assert(fifa96_team_data_lang_index(0x06Eu) == 18);  /* England */
  assert(fifa96_team_data_lang_index(0x0A4u) == 27);  /* Israel (last row) */
  assert(fifa96_team_data_lang_index(0x000u) == -1);
  assert(fifa96_team_data_lang_index(0xFFFFu) == -1);

  /* The menu's row -> mode-4 slot is the same single map keyed by LANG id. */
  for (int i = 0; i < FIFA96_MENU_TEAM_COUNT; i++) {
    int slot = fifa96_menu_team_pccd_index(i);
    assert(slot >= 0);
    assert(slot == fifa96_team_data_lang_index(fifa96_menu_team_id(i)));
  }
  assert(fifa96_menu_team_pccd_index(-1) == -1);
  assert(fifa96_menu_team_pccd_index(FIFA96_MENU_TEAM_COUNT) == -1);
}

static void test_soft_failure(void) {
  struct fifa96_team_data td;
  memset(&td, 0, sizeof td);
  struct fifa96_team_identity identity;
  assert(fifa96_team_data_lookup(&td, 0x023u, &identity) == -FIFA96_ERR_STATE);
  assert(fifa96_team_data_lookup(NULL, 0x023u, &identity) == -FIFA96_ERR_INVALID);
  assert(fifa96_team_data_lookup(&td, 0x023u, NULL) == -FIFA96_ERR_INVALID);
  assert(identity.valid == 0);

  assert(fifa96_team_data_init(NULL, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_team_data_init(&td, NULL) == -FIFA96_ERR_NOT_FOUND);
  struct fifa96_asset_table empty;
  memset(&empty, 0, sizeof empty);
  assert(fifa96_team_data_init(&td, &empty) == -FIFA96_ERR_NOT_FOUND);
  assert(td.ready == 0 && td.arena == NULL);
  fifa96_team_data_shutdown(&td);
  fifa96_team_data_shutdown(NULL);
}

static void test_retail_identity(void) {
  if (!file_exists(TEAM_DATA_ISO_PATH)) {
    fprintf(stderr, "SKIP retail team data case (no %s)\n", TEAM_DATA_ISO_PATH);
    return;
  }
  struct fifa96_asset_table *assets = NULL;
  assert(fifa96_asset_mount_file(TEAM_DATA_ISO_PATH, &assets) == FIFA96_OK);
  struct fifa96_team_data td;
  assert(fifa96_team_data_init(&td, assets) == 0);
  assert(td.ready == 1 && td.arena != NULL && td.arena_len == 199265u);

  struct fifa96_team_identity identity;
  assert(fifa96_team_data_lookup(&td, 0x023u, &identity) == 0);
  assert(identity.valid == 1);
  assert(identity.pccd_index == 6u);
  assert(identity.pccd_record == 0x2Bu);
  assert(identity.pccd_id == 59u);
  assert(strcmp(identity.code, "BRA") == 0);
  assert(strcmp(identity.name_long, "Brazil") == 0);
  assert(strcmp(identity.name_short, "Bra") == 0);
  assert(identity.formation == 1u);
  assert(identity.kit[0][0] == 26u && identity.kit[0][1] == 4u &&
         identity.kit[0][2] == 7u);
  assert(identity.print[0] == 'D' && identity.print[1] == 'D' &&
         identity.print[2] == 14u && identity.print[3] == 15u);
  assert(identity.roster[0] == 2047u && identity.roster[15] == 106u);

  assert(fifa96_team_data_lookup(&td, 0x0A6u, &identity) == 0);
  assert(identity.pccd_index == 28u);
  assert(identity.pccd_record == 0x20u);
  assert(identity.pccd_id == 47u);
  assert(strcmp(identity.code, "ITALY") == 0);
  assert(identity.formation == 2u);
  assert(identity.kit[0][0] == 3u && identity.kit[0][1] == 30u &&
         identity.kit[0][2] == 3u);
  assert(identity.roster[0] == 806u && identity.roster[15] == 168u);

  assert(fifa96_team_data_lookup(&td, 0x000u, &identity) == -FIFA96_ERR_NOT_FOUND);
  assert(identity.valid == 0);

  /* Every exposed name resolves, has a non-empty uppercased code, and a valid
   * formation id. */
  for (int i = 0; i < FIFA96_MENU_TEAM_COUNT; i++) {
    uint32_t lang = fifa96_menu_team_id(i);
    assert(fifa96_team_data_lookup(&td, lang, &identity) == 0);
    assert(identity.valid == 1);
    assert(identity.code[0] != '\0');
    assert(identity.formation <= 4u);
    for (size_t c = 0; c < strlen(identity.code); c++)
      assert(!(identity.code[c] >= 'a' && identity.code[c] <= 'z'));
  }

  fifa96_team_data_shutdown(&td);
  assert(td.ready == 0 && td.arena == NULL);
  assert(fifa96_team_data_lookup(&td, 0x023u, &identity) == -FIFA96_ERR_STATE);
  fifa96_asset_unmount(assets);
}

int main(void) {
  test_mapping();
  test_soft_failure();
  test_retail_identity();
  puts("test_engine_team_data OK");
  return 0;
}
