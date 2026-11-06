#include "fifa96_engine/fifa96_team_data.h"

#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_loader/fifa96_record.h"

#define TEAM_DATA_PCCD_PATH "/FEDATA/PCCD.POG"
#define TEAM_DATA_ARENA_MAX (4u * 1024u * 1024u)

/* P6-team-identity §4: the port's 44-name Team Select list mapped to the
 * mode-4 ("World") table slots. The native cells are (mode,index) pairs, not
 * team ids; there is no static 44-id table in the image, so this mapping is a
 * port construct. `name` is the LANG000 name for diagnostics/tests. */
static const struct team_lang_map {
  uint32_t lang_id;
  const char *name;
  uint8_t mode4_index;
} team_lang_map[FIFA96_TEAM_DATA_LANG_COUNT] = {
    {0x023u, "Brazil",       6},
    {0x0A6u, "Italy",        28},
    {0x013u, "Argentina",    1},
    {0x06Eu, "England",      18},
    {0x088u, "Germany",      21},
    {0x07Eu, "France",       20},
    {0x126u, "Spain",        50},
    {0x0C9u, "Netherlands",  35},
    {0x0E7u, "Portugal",     41},
    {0x01Fu, "Belgium",      4},
    {0x134u, "Sweden",       51},
    {0x0CFu, "Norway",       38},
    {0x058u, "Denmark",      16},
    {0x10Eu, "Scotland",     46},
    {0x0A2u, "Ireland",      42},
    {0x153u, "Wales",        57},
    {0x0BDu, "Mexico",       32},
    {0x149u, "USA",          56},
    {0x0A9u, "Japan",        30},
    {0x02Au, "Cameroon",     8},
    {0x0CBu, "Nigeria",      37},
    {0x0FFu, "Romania",      43},
    {0x024u, "Bulgaria",     7},
    {0x137u, "Switzerland",  52},
    {0x017u, "Austria",      3},
    {0x04Fu, "Czech Rep.",   15},
    {0x102u, "Russia",       44},
    {0x08Cu, "Greece",       23},
    {0x145u, "Turkey",       53},
    {0x037u, "Colombia",     12},
    {0x022u, "Bolivia",      5},
    {0x02Eu, "Chile",        10},
    {0x148u, "Uruguay",      55},
    {0x016u, "Australia",    2},
    {0x02Bu, "Canada",       9},
    {0x02Fu, "China",        11},
    {0x104u, "Saudi Arabia", 45},
    {0x0C5u, "Morocco",      33},
    {0x06Cu, "Egypt",        17},
    {0x124u, "South Africa", 48},
    {0x079u, "Finland",      19},
    {0x0E6u, "Poland",       40},
    {0x09Bu, "Hungary",      25},
    {0x0A4u, "Israel",       27},
};

static size_t team_data_declared(const uint8_t *raw, size_t len) {
  if (!raw || len < 5) return 0;
  return ((size_t)raw[2] << 16) | ((size_t)raw[3] << 8) | (size_t)raw[4];
}

static void team_data_code(char dst[FIFA96_TEAM_CODE_MAX], const char *src) {
  memset(dst, 0, FIFA96_TEAM_CODE_MAX);
  if (!src) return;
  size_t n = strlen(src);
  if (n > FIFA96_TEAM_CODE_MAX - 1u) n = FIFA96_TEAM_CODE_MAX - 1u;
  for (size_t i = 0; i < n; i++) {
    char c = src[i];
    if (c >= 'a' && c <= 'z') c = (char)(c - ('a' - 'A'));
    dst[i] = c;
  }
}

int fifa96_team_data_init(struct fifa96_team_data *td,
                          const struct fifa96_asset_table *assets) {
  if (!td) return -FIFA96_ERR_INVALID;
  memset(td, 0, sizeof *td);
  if (!assets) return -FIFA96_ERR_NOT_FOUND;

  uint8_t *raw = NULL;
  size_t raw_len = 0;
  fifa96_err_t err = fifa96_asset_read(assets, TEAM_DATA_PCCD_PATH, &raw, &raw_len);
  if (err != FIFA96_OK || !raw) {
    fifa96_asset_free(raw);
    return -FIFA96_ERR_NOT_FOUND;
  }
  size_t declared = team_data_declared(raw, raw_len);
  if (declared == 0 || declared > TEAM_DATA_ARENA_MAX) {
    fifa96_asset_free(raw);
    return -FIFA96_ERR_UNSUPPORTED;
  }
  uint8_t *arena = malloc(declared);
  if (!arena) {
    fifa96_asset_free(raw);
    return -FIFA96_ERR_IO;
  }
  size_t decoded = 0;
  int rc = fifa96_record_decode(raw, raw_len, arena, declared, &decoded);
  fifa96_asset_free(raw);
  if (rc != 0 || decoded != declared) {
    free(arena);
    return -FIFA96_ERR_UNSUPPORTED;
  }
  err = fifa96_pccd_parse(arena, decoded, &td->db);
  if (err != FIFA96_OK) {
    free(arena);
    memset(td, 0, sizeof *td);
    return -(int)err;
  }
  td->arena = arena;
  td->arena_len = decoded;
  td->ready = 1;
  return 0;
}

void fifa96_team_data_shutdown(struct fifa96_team_data *td) {
  if (!td) return;
  free(td->arena);
  memset(td, 0, sizeof *td);
}

static const struct team_lang_map *team_lang_find(uint32_t lang_id) {
  for (int i = 0; i < FIFA96_TEAM_DATA_LANG_COUNT; i++)
    if (team_lang_map[i].lang_id == lang_id) return &team_lang_map[i];
  return NULL;
}

int fifa96_team_data_lang_index(uint32_t lang_id) {
  const struct team_lang_map *entry = team_lang_find(lang_id);
  return entry ? (int)entry->mode4_index : -1;
}

int fifa96_team_data_lang_count(void) { return FIFA96_TEAM_DATA_LANG_COUNT; }

int fifa96_team_data_lookup(const struct fifa96_team_data *td, uint32_t lang_id,
                            struct fifa96_team_identity *identity) {
  if (!td || !identity) return -FIFA96_ERR_INVALID;
  memset(identity, 0, sizeof *identity);
  if (!td->ready) return -FIFA96_ERR_STATE;

  const struct team_lang_map *entry = team_lang_find(lang_id);
  if (!entry) return -FIFA96_ERR_NOT_FOUND;

  struct fifa96_pccd_team team;
  fifa96_err_t err = fifa96_pccd_team(&td->db, FIFA96_PCCD_MODE_WORLD,
                                      entry->mode4_index, &team);
  if (err != FIFA96_OK) return -(int)err;

  identity->valid = 1;
  identity->lang_id = lang_id;
  identity->pccd_index = entry->mode4_index;
  identity->pccd_record = (uint8_t)team.record;
  identity->pccd_id = team.id;
  identity->name_long = team.name_long;
  identity->name_short = team.name_short;
  identity->name_medium = team.name_medium;
  team_data_code(identity->code, team.name_short);
  identity->formation = team.formation;
  memcpy(identity->kit, team.kit, sizeof identity->kit);
  memcpy(identity->print, team.print, sizeof identity->print);
  memcpy(identity->roster, team.roster, sizeof identity->roster);
  return 0;
}
