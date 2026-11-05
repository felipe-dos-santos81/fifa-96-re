#include <stddef.h>
#include "fifa96_engine/fifa96_menu_model.h"

/* A3 §1.1: FUN_0001FC50's nine FUN_00015960 widgets, in the native creation
 * order. `code` is the packed widget id's low byte (the screen return value),
 * `label_id` the FUN_0001771C argument. */
static const struct fifa96_menu_item_desc menu_items[FIFA96_MENU_ITEM_COUNT] = {
    {0x081u, "Friendly",    4u, 1, 0, FIFA96_MENU_ACTION_FRIENDLY},
    {0x1BFu, "Leagues",     5u, 1, 1, FIFA96_MENU_ACTION_NONE},
    {0x1BBu, "Tournaments", 6u, 1, 2, FIFA96_MENU_ACTION_NONE},
    {0x1BDu, "Playoffs",    7u, 1, 3, FIFA96_MENU_ACTION_NONE},
    {0x0EAu, "Practise",    8u, 1, 4, FIFA96_MENU_ACTION_NONE},
    {0x0B3u, "Load Game",   9u, 0, 0, FIFA96_MENU_ACTION_NONE},
    {0x0D8u, "Options",     1u, 0, 1, FIFA96_MENU_ACTION_OPTIONS},
    {0x0C2u, "Modem Setup", 2u, 0, 2, FIFA96_MENU_ACTION_NONE},
    {0x0F3u, "Quit to DOS", 3u, 0, 3, FIFA96_MENU_ACTION_QUIT},
};

/* A3 §2.3: the 44 national-team names as plain LANG000 entries (language-
 * stable ids). The codes are the derived three-letter HUD stand-ins (the
 * native PCCD/PCDB short name is not decoded, P2-L4). */
static const struct {
  uint32_t lang_id;
  const char *name;
  const char *code;
} menu_teams[FIFA96_MENU_TEAM_COUNT] = {
    {0x023u, "Brazil",       "BRA"},
    {0x0A6u, "Italy",        "ITA"},
    {0x013u, "Argentina",    "ARG"},
    {0x06Eu, "England",      "ENG"},
    {0x088u, "Germany",      "GER"},
    {0x07Eu, "France",       "FRA"},
    {0x126u, "Spain",        "ESP"},
    {0x0C9u, "Netherlands",  "NED"},
    {0x0E7u, "Portugal",     "POR"},
    {0x01Fu, "Belgium",      "BEL"},
    {0x134u, "Sweden",       "SWE"},
    {0x0CFu, "Norway",       "NOR"},
    {0x058u, "Denmark",      "DEN"},
    {0x10Eu, "Scotland",     "SCO"},
    {0x0A2u, "Ireland",      "IRL"},
    {0x153u, "Wales",        "WAL"},
    {0x0BDu, "Mexico",       "MEX"},
    {0x149u, "USA",          "USA"},
    {0x0A9u, "Japan",        "JPN"},
    {0x02Au, "Cameroon",     "CMR"},
    {0x0CBu, "Nigeria",      "NGA"},
    {0x0FFu, "Romania",      "ROU"},
    {0x024u, "Bulgaria",     "BUL"},
    {0x137u, "Switzerland",  "SUI"},
    {0x017u, "Austria",      "AUT"},
    {0x04Fu, "Czech Rep.",   "CZE"},
    {0x102u, "Russia",       "RUS"},
    {0x08Cu, "Greece",       "GRE"},
    {0x145u, "Turkey",       "TUR"},
    {0x037u, "Colombia",     "COL"},
    {0x022u, "Bolivia",      "BOL"},
    {0x02Eu, "Chile",        "CHI"},
    {0x148u, "Uruguay",      "URU"},
    {0x016u, "Australia",    "AUS"},
    {0x02Bu, "Canada",       "CAN"},
    {0x02Fu, "China",        "CHN"},
    {0x104u, "Saudi Arabia", "KSA"},
    {0x0C5u, "Morocco",      "MAR"},
    {0x06Cu, "Egypt",        "EGY"},
    {0x124u, "South Africa", "RSA"},
    {0x079u, "Finland",      "FIN"},
    {0x0E6u, "Poland",       "POL"},
    {0x09Bu, "Hungary",      "HUN"},
    {0x0A4u, "Israel",       "ISR"},
};

static int menu_item_at(int col, int row) {
  for (int i = 0; i < FIFA96_MENU_ITEM_COUNT; i++) {
    if (menu_items[i].col == col && menu_items[i].row == row) return i;
  }
  return -1;
}

static int menu_col_rows(int col) {
  int rows = 0;
  for (int i = 0; i < FIFA96_MENU_ITEM_COUNT; i++) {
    if (menu_items[i].col == col && menu_items[i].row + 1 > rows) rows = menu_items[i].row + 1;
  }
  return rows;
}

void fifa96_menu_model_init(struct fifa96_menu_model *model) {
  if (model == NULL) return;
  model->screen = FIFA96_MENU_SCREEN_GAME_SELECT;
  model->item = 0;          /* Friendly, right column row 0 */
  /* A3 §2.1: the native entry defaults are team cells 0 and 4. */
  model->team[0] = 0;       /* Brazil */
  model->team[1] = 4;       /* Germany */
  model->team_col = 0;
}

int fifa96_menu_model_nav(struct fifa96_menu_model *model, int32_t code) {
  if (model == NULL || code < 0 || code > 3) return 0;
  switch (model->screen) {
    case FIFA96_MENU_SCREEN_GAME_SELECT: {
      const struct fifa96_menu_item_desc *cur = &menu_items[model->item];
      if (code == 0 || code == 1) {
        int rows = menu_col_rows(cur->col);
        int row = (cur->row + (code == 0 ? rows - 1 : 1)) % rows;
        int next = menu_item_at(cur->col, row);
        if (next >= 0) model->item = next;
      } else {
        int col = cur->col ^ 1;   /* the two columns toggle */
        int rows = menu_col_rows(col);
        int row = cur->row < rows ? cur->row : rows - 1;
        int next = menu_item_at(col, row);
        if (next >= 0) model->item = next;
      }
      return 1;
    }
    case FIFA96_MENU_SCREEN_TEAM_SELECT:
      if (code == 0 || code == 1) {
        model->team[model->team_col] =
            (model->team[model->team_col] + (code == 0 ? FIFA96_MENU_TEAM_COUNT - 1 : 1)) %
            FIFA96_MENU_TEAM_COUNT;
      } else {
        model->team_col ^= 1;
      }
      return 1;
    case FIFA96_MENU_SCREEN_OPTIONS:
      return 1;   /* no rows; the caller still redraws */
    default:
      return 0;
  }
}

enum fifa96_menu_action fifa96_menu_model_activate(struct fifa96_menu_model *model) {
  if (model == NULL) return FIFA96_MENU_ACTION_NONE;
  switch (model->screen) {
    case FIFA96_MENU_SCREEN_GAME_SELECT: {
      enum fifa96_menu_action action = menu_items[model->item].action;
      if (action == FIFA96_MENU_ACTION_FRIENDLY) {
        model->screen = FIFA96_MENU_SCREEN_TEAM_SELECT;
        model->team_col = 0;
        return FIFA96_MENU_ACTION_NONE;
      }
      if (action == FIFA96_MENU_ACTION_OPTIONS) {
        model->screen = FIFA96_MENU_SCREEN_OPTIONS;
        return FIFA96_MENU_ACTION_NONE;
      }
      return action;   /* QUIT is live; the six disabled items are NONE */
    }
    case FIFA96_MENU_SCREEN_TEAM_SELECT:
      return FIFA96_MENU_ACTION_START_MATCH;
    case FIFA96_MENU_SCREEN_OPTIONS:
      model->screen = FIFA96_MENU_SCREEN_GAME_SELECT;
      return FIFA96_MENU_ACTION_NONE;
    default:
      return FIFA96_MENU_ACTION_NONE;
  }
}

int fifa96_menu_model_back(struct fifa96_menu_model *model) {
  if (model == NULL) return 0;
  if (model->screen == FIFA96_MENU_SCREEN_GAME_SELECT ||
      model->screen == FIFA96_MENU_SCREEN_LIBRARY)
    return 0;
  model->screen = FIFA96_MENU_SCREEN_GAME_SELECT;
  return 1;
}

const struct fifa96_menu_item_desc *fifa96_menu_model_items(void) {
  return menu_items;
}

const char *fifa96_menu_model_item_fallback(int index) {
  if (index < 0 || index >= FIFA96_MENU_ITEM_COUNT) return NULL;
  return menu_items[index].fallback;
}

uint32_t fifa96_menu_team_id(int index) {
  if (index < 0 || index >= FIFA96_MENU_TEAM_COUNT) return 0;
  return menu_teams[index].lang_id;
}

const char *fifa96_menu_team_fallback(int index) {
  if (index < 0 || index >= FIFA96_MENU_TEAM_COUNT) return NULL;
  return menu_teams[index].name;
}

const char *fifa96_menu_team_code(int index) {
  if (index < 0 || index >= FIFA96_MENU_TEAM_COUNT) return NULL;
  return menu_teams[index].code;
}
