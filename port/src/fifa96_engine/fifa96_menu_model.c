#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_engine/fifa96_competition.h"
#include "fifa96_engine/fifa96_menu_model.h"
#include "fifa96_engine/fifa96_team_data.h"

/* A3 §1.1: FUN_0001FC50's nine FUN_00015960 widgets, in the native creation
 * order. `code` is the packed widget id's low byte (the screen return value),
 * `label_id` the FUN_0001771C argument. */
static const struct fifa96_menu_item_desc menu_items[FIFA96_MENU_ITEM_COUNT] = {
    {0x081u, "Friendly",    4u, 1, 0, FIFA96_MENU_ACTION_FRIENDLY},
    {0x1BFu, "Leagues",     5u, 1, 1, FIFA96_MENU_ACTION_LEAGUE},
    {0x1BBu, "Tournaments", 6u, 1, 2, FIFA96_MENU_ACTION_TOURNAMENT},
    {0x1BDu, "Playoffs",    7u, 1, 3, FIFA96_MENU_ACTION_PLAYOFF},
    {0x0EAu, "Practise",    8u, 1, 4, FIFA96_MENU_ACTION_PRACTISE},
    {0x0B3u, "Load Game",   9u, 0, 0, FIFA96_MENU_ACTION_LOAD_GAME},
    {0x0D8u, "Options",     1u, 0, 1, FIFA96_MENU_ACTION_OPTIONS},
    /* Modem Setup opens the offline notice; the same action leaves it. */
    {0x0C2u, "Modem Setup", 2u, 0, 2, FIFA96_MENU_ACTION_MODEM_BACK},
    {0x0F3u, "Quit to DOS", 3u, 0, 3, FIFA96_MENU_ACTION_QUIT},
};

/* A3 §2.3: the 44 national-team names as plain LANG000 entries (language-
 * stable ids). The codes are the fallback three-letter HUD stand-ins; P6
 * replaces them with the PCCD record +0x08 short name when team data is
 * available (`fifa96_menu_team_pccd_index` gives the mode-4 slot). */
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

/* P13: the Game Select action values start at 5, but `pending_kind` is the
 * native mode byte (1..4). */
static uint8_t menu_action_mode(enum fifa96_menu_action action) {
  switch (action) {
    case FIFA96_MENU_ACTION_LEAGUE: return FIFA96_COMPETITION_LEAGUE;
    case FIFA96_MENU_ACTION_TOURNAMENT: return FIFA96_COMPETITION_TOURNAMENT;
    case FIFA96_MENU_ACTION_PLAYOFF: return FIFA96_COMPETITION_PLAYOFF;
    case FIFA96_MENU_ACTION_PRACTISE: return FIFA96_COMPETITION_PRACTISE;
    default: return 0;
  }
}

static int menu_item_at(int col, int row) {
  for (int i = 0; i < FIFA96_MENU_ITEM_COUNT; i++) {
    if (menu_items[i].col == col && menu_items[i].row == row) return i;
  }
  return -1;
}

/* P6: the choice LANG-id arrays from the descriptor records at 0x105340
 * (read_memory 0x1052B4..0x105340, the range the records point into). Off/On
 * is {0xD2,0xD7}; the language list is {0x6F,0x59,0x7D,0x72,0x135,0xA5};
 * Player Names is {0xD2,0x1C8,0x1C9}; half length is 0x248..0x24E. */
static const uint32_t option_off_on[] = {0xD2u, 0xD7u};
static const uint32_t option_language[] = {0x6Fu, 0x59u, 0x7Du, 0x72u, 0x135u, 0xA5u};
static const uint32_t option_names[] = {0xD2u, 0x1C8u, 0x1C9u};
static const uint32_t option_replay[] = {0xD2u, 0xD7u, 0xF5u};
static const uint32_t option_game_type[] = {0x09u, 0x11Au};
static const uint32_t option_fouls[] = {0xD2u, 0xCDu, 0xD7u};
static const uint32_t option_pitch[] = {0x181u, 0x69u, 0x51u, 0x67u, 0xF5u};
static const uint32_t option_clock[] = {0x42u, 0xD9u};
static const uint32_t option_half[] = {0x248u, 0x249u, 0x24Au, 0x24Bu,
                                       0x24Cu, 0x24Du, 0x24Eu};
static const uint32_t option_skill[] = {0x110u, 0xECu};

static const char *const text_off_on[] = {"Off", "On"};
static const char *const text_language[] = {"English", "German", "French",
                                            "Spanish", "Swedish", "Italian"};
/* The retail LANG000 entries 0x1C8/0x1C9 decode to "Names"/"Numbers". */
static const char *const text_names[] = {"Off", "Names", "Numbers"};
static const char *const text_replay[] = {"Off", "On", "Random"};
static const char *const text_game_type[] = {"Action", "Simulation"};
static const char *const text_fouls[] = {"Off", "No Bookings", "On"};
static const char *const text_pitch[] = {"Hot", "Dry", "Damp", "Drenched", "Random"};
static const char *const text_clock[] = {"Continuous", "Out of Play"};
static const char *const text_half[] = {"2", "4", "6", "8", "10", "20", "45"};
static const char *const text_skill[] = {"Semi Pro", "Pro"};

/* 0x105340: 26 x {u32 label; u32 choices; u8 max}, decoded field by field.
 * `live` is the P6 port-consumer flag, not a native byte: only Sound Effects
 * (0x01), Fouls (0x0A), Injuries (0x0B), Half Length (0x0E), Offside (0x10)
 * and SFX Volume (0x11) reach a consumer in this slice. Everything else is
 * visible but dim (stored, no consumer). */
static const struct fifa96_menu_option_desc option_descriptors
    [FIFA96_MENU_OPTION_DESC_COUNT] = {
    {0x00u,   2u, 0u, 0u, 0x0C7u, "Music",            option_off_on,    text_off_on},
    {0x01u,   2u, 0u, 1u, 0x113u, "Sound Effects",    option_off_on,    text_off_on},
    {0x02u,   2u, 0u, 0u, 0x0DFu, "Play by Play",     option_off_on,    text_off_on},
    {0x03u,   6u, 0u, 0u, 0x0AFu, "Language",         option_language,  text_language},
    {0x04u,   2u, 0u, 0u, 0x131u, "Super VGA",        option_off_on,    text_off_on},
    {0x05u,   2u, 0u, 0u, 0x131u, "Super VGA",        option_off_on,    text_off_on},
    {0x06u,   2u, 0u, 0u, 0x142u, "Time Display",     option_off_on,    text_off_on},
    {0x07u,   3u, 0u, 0u, 0x18Bu, "Player Names",     option_names,     text_names},
    {0x08u,   3u, 0u, 0u, 0x018u, "Auto Replay",      option_replay,    text_replay},
    {0x09u,   2u, 0u, 0u, 0x087u, "Game Type",        option_game_type, text_game_type},
    {0x0Au,   3u, 0u, 1u, 0x07Cu, "Fouls",            option_fouls,     text_fouls},
    {0x0Bu,   2u, 0u, 1u, 0x09Fu, "Injuries",         option_off_on,    text_off_on},
    {0x0Cu,   5u, 0u, 0u, 0x0DDu, "Pitch Condition",  option_pitch,     text_pitch},
    {0x0Du,   2u, 0u, 0u, 0x036u, "Clock",            option_clock,     text_clock},
    {0x0Eu,   7u, 0u, 1u, 0x093u, "Half Length",      option_half,      text_half},
    {0x0Fu,   2u, 0u, 0u, 0x11Du, "Skill Level",      option_skill,     text_skill},
    {0x10u,   2u, 0u, 1u, 0x0D5u, "Offside",          option_off_on,    text_off_on},
    {0x11u, 100u, 1u, 1u, 0x191u, "SFX Volume",       NULL,             NULL},
    {0x12u, 100u, 1u, 0u, 0x184u, "Music Volume",     NULL,             NULL},
    {0x13u, 100u, 1u, 0u, 0x185u, "Speech Volume",    NULL,             NULL},
    {0x14u,   0u, 0u, 0u, 0u,     NULL,               NULL,             NULL},
    {0x15u,   2u, 0u, 0u, 0x21Bu, "4-Button Joypad",  option_off_on,    text_off_on},
    {0x16u,   0u, 0u, 0u, 0u,     NULL,               NULL,             NULL},
    {0x17u,   0u, 0u, 0u, 0u,     NULL,               NULL,             NULL},
    {0x18u,   0u, 0u, 0u, 0u,     NULL,               NULL,             NULL},
    {0x19u,   0u, 0u, 0u, 0u,     NULL,               NULL,             NULL},
};

/* 0x10542A, 20 bytes. Index 19 is descriptor 66 (dead); the model's native
 * rows stop at index 18, matching the screen's last slot 0x13 clamp. */
static const uint8_t option_order[FIFA96_MENU_OPTION_ORDER_COUNT] = {
    0u, 18u, 1u, 17u, 2u, 19u, 3u, 4u, 6u, 7u,
    21u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 66u};

/* Port addition: the Controls page. These are the real current mappings
 * (platform_sdl3.c map_scancode/map_button, fifa96_match_run.c input bits).
 * P8/FU-346: the pad's South/East buttons and the keyboard Enter/X/Backspace
 * share the same engine CONFIRM/DECLINE codes, so the dual role is real in
 * both directions. In menus they select/confirm and back/decline; in live play
 * they kick and pass. Z/C/V keep their dedicated kick/pass/long-ball codes, and
 * the half-time sit-1 kickoff only releases on a slot input (C/PASS is the
 * documented start-second-half key). P9/FU-347 (P9-fix2 axis): holding Up or
 * Down with Z aims the shot to ±0x80 along the goal-mouth width (world x);
 * an unaimed (centre) shot is the keeper's. */
static const char *const option_controls_text[] = {
    "CONTROLS",
    "A/SOUTH or ENTER",
    "  MENU: SELECT / CONFIRM",
    "  PLAY: KICK",
    "B/EAST or X/BACKSPACE",
    "  MENU: BACK / DECLINE",
    "  PLAY: PASS",
    "Z KICK   C PASS   V LONG BALL",
    "UP/DOWN + Z  AIM SHOT",
    "CENTRE SHOT = KEEPER'S",
    "HALF-TIME: C/PASS STARTS 2ND HALF",
    "ARROWS / PAD DPAD  MOVE",
    "ESC  MENU: QUIT TO DOS",
    "     MATCH: PAUSE",
    "PAD WEST  LONG BALL",
};

static void menu_model_settings_defaults(struct fifa96_menu_model *model) {
  fifa96_settings_defaults(&model->settings);
  const int32_t *max = fifa96_settings_descriptor_max();
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++)
    model->settings.max[i] = max[i];
}

static void menu_model_reset_screen(struct fifa96_menu_model *model) {
  model->screen = FIFA96_MENU_SCREEN_GAME_SELECT;
  model->item = 0;          /* Friendly, right column row 0 */
  /* A3 §2.1: the native entry defaults are team cells 0 and 4. */
  model->team[0] = 0;       /* Brazil */
  model->team[1] = 4;       /* Germany */
  model->team_col = 0;
  model->option_row = 0;
  model->option_scroll = 0;
  model->option_page = FIFA96_MENU_OPTION_PAGE_LIST;
  model->pending_kind = 0;
  model->list_row = 0;
  model->list_scroll = 0;
  model->list_count = 0;
}

/* P13: the list screens' row counts. `.list_count` is set by the front-end for
 * the read-only table/bracket (its content is built there); every other screen
 * has a fixed layout. */
static int menu_model_screen_rows(const struct fifa96_menu_model *model) {
  switch (model->screen) {
    case FIFA96_MENU_SCREEN_COMPETITION_HUB: return 5;
    case FIFA96_MENU_SCREEN_LOAD_GAME:
    case FIFA96_MENU_SCREEN_SAVE_GAME: return FIFA96_MENU_SAVE_SLOTS;
    case FIFA96_MENU_SCREEN_MODEM: return 2;
    case FIFA96_MENU_SCREEN_LIST:
      return model->list_count > 0 ? model->list_count : 1;
    default: return 0;
  }
}

static void menu_model_list_scroll(struct fifa96_menu_model *model, int rows) {
  if (rows < 1) rows = 1;
  if (model->list_row < 0) model->list_row = 0;
  if (model->list_row >= rows) model->list_row = rows - 1;
  if (model->list_row < model->list_scroll)
    model->list_scroll = model->list_row;
  if (model->list_row >= model->list_scroll + FIFA96_MENU_LIST_VISIBLE)
    model->list_scroll = model->list_row - FIFA96_MENU_LIST_VISIBLE + 1;
  int max_scroll = rows - FIFA96_MENU_LIST_VISIBLE;
  if (max_scroll < 0) max_scroll = 0;
  if (model->list_scroll > max_scroll) model->list_scroll = max_scroll;
  if (model->list_scroll < 0) model->list_scroll = 0;
}

static void menu_model_option_scroll(struct fifa96_menu_model *model) {
  if (model->option_row < model->option_scroll)
    model->option_scroll = model->option_row;
  if (model->option_row >= model->option_scroll + FIFA96_MENU_OPTION_VISIBLE)
    model->option_scroll = model->option_row - FIFA96_MENU_OPTION_VISIBLE + 1;
  const int max_scroll = FIFA96_MENU_OPTION_ROW_COUNT - FIFA96_MENU_OPTION_VISIBLE;
  if (model->option_scroll > max_scroll) model->option_scroll = max_scroll;
  if (model->option_scroll < 0) model->option_scroll = 0;
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
  menu_model_settings_defaults(model);
  menu_model_reset_screen(model);
}

void fifa96_menu_model_reset(struct fifa96_menu_model *model) {
  if (model == NULL) return;
  /* Keep `settings`: the native options block survives a match return (the
   * front-end reset path); only an explicit cancel would restore a backup. */
  menu_model_reset_screen(model);
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
      if (model->pending_kind != 0) {
        /* P13: competition Team Select is single-team; the visitor cell is
         * locked to the derived opponent and only the home cell moves. */
        model->team_col = 0;
        if (code == 0 || code == 1) {
          model->team[0] =
              (model->team[0] + (code == 0 ? FIFA96_MENU_TEAM_COUNT - 1 : 1)) %
              FIFA96_MENU_TEAM_COUNT;
        }
        return 1;
      }
      if (code == 0 || code == 1) {
        model->team[model->team_col] =
            (model->team[model->team_col] + (code == 0 ? FIFA96_MENU_TEAM_COUNT - 1 : 1)) %
            FIFA96_MENU_TEAM_COUNT;
      } else {
        model->team_col ^= 1;
      }
      return 1;
    case FIFA96_MENU_SCREEN_OPTIONS: {
      if (model->option_page != FIFA96_MENU_OPTION_PAGE_LIST)
        return 1;   /* Controls: no selectable rows; the caller redraws */
      if (code == 0) {
        if (model->option_row > 0) model->option_row--;
      } else if (code == 1) {
        if (model->option_row < FIFA96_MENU_OPTION_ROW_COUNT - 1) model->option_row++;
      } else {
        int setting = fifa96_menu_model_option_setting(model->option_row);
        if (setting >= 0 && option_descriptors[setting].live) {
          /* FUN_0001DA58: horizontal -1 = decrement, -2 = step/increment. */
          (void)fifa96_settings_set(&model->settings, (uint32_t)setting,
                                    code == 2 ? -1 : -2);
        }
      }
      menu_model_option_scroll(model);
      return 1;
    }
    case FIFA96_MENU_SCREEN_COMPETITION_HUB:
    case FIFA96_MENU_SCREEN_LOAD_GAME:
    case FIFA96_MENU_SCREEN_SAVE_GAME:
    case FIFA96_MENU_SCREEN_MODEM:
    case FIFA96_MENU_SCREEN_LIST: {
      int rows = menu_model_screen_rows(model);
      if (code == 0 && model->list_row > 0) model->list_row--;
      else if (code == 1 && model->list_row < rows - 1) model->list_row++;
      menu_model_list_scroll(model, rows);
      return 1;
    }
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
        model->pending_kind = 0;
        return FIFA96_MENU_ACTION_NONE;
      }
      if (action == FIFA96_MENU_ACTION_OPTIONS) {
        model->screen = FIFA96_MENU_SCREEN_OPTIONS;
        model->option_row = 0;
        model->option_scroll = 0;
        model->option_page = FIFA96_MENU_OPTION_PAGE_LIST;
        return FIFA96_MENU_ACTION_NONE;
      }
      if (action == FIFA96_MENU_ACTION_LEAGUE ||
          action == FIFA96_MENU_ACTION_TOURNAMENT ||
          action == FIFA96_MENU_ACTION_PLAYOFF ||
          action == FIFA96_MENU_ACTION_PRACTISE) {
        model->screen = FIFA96_MENU_SCREEN_TEAM_SELECT;
        model->team_col = 0;
        model->pending_kind = menu_action_mode(action);   /* native byte 1..4 */
        return action;
      }
      if (action == FIFA96_MENU_ACTION_LOAD_GAME) {
        model->screen = FIFA96_MENU_SCREEN_LOAD_GAME;
        model->list_row = 0;
        model->list_scroll = 0;
        model->list_count = FIFA96_MENU_SAVE_SLOTS;
        return action;
      }
      if (action == FIFA96_MENU_ACTION_MODEM_BACK) {
        model->screen = FIFA96_MENU_SCREEN_MODEM;
        model->list_row = 0;
        model->list_scroll = 0;
        model->list_count = 2;
        return action;
      }
      return action;   /* QUIT remains live; Friendly/Options handled above */
    }
    case FIFA96_MENU_SCREEN_TEAM_SELECT:
      return FIFA96_MENU_ACTION_START_MATCH;
    case FIFA96_MENU_SCREEN_COMPETITION_HUB:
      switch (model->list_row) {
        case 0: return FIFA96_MENU_ACTION_HUB_PLAY;
        case 1: return FIFA96_MENU_ACTION_HUB_SIMULATE;
        case 2:
          model->screen = FIFA96_MENU_SCREEN_LIST;
          model->list_row = 0;
          model->list_scroll = 0;
          return FIFA96_MENU_ACTION_HUB_TABLE;
        case 3:
          model->screen = FIFA96_MENU_SCREEN_SAVE_GAME;
          model->list_row = 0;
          model->list_scroll = 0;
          model->list_count = FIFA96_MENU_SAVE_SLOTS;
          return FIFA96_MENU_ACTION_HUB_SAVE;
        case 4:
          model->screen = FIFA96_MENU_SCREEN_GAME_SELECT;
          model->list_row = 0;
          model->list_scroll = 0;
          return FIFA96_MENU_ACTION_HUB_EXIT;
        default:
          return FIFA96_MENU_ACTION_NONE;
      }
    case FIFA96_MENU_SCREEN_SAVE_GAME:
      return FIFA96_MENU_ACTION_SAVE_SLOT;
    case FIFA96_MENU_SCREEN_LOAD_GAME:
      return FIFA96_MENU_ACTION_LOAD_SLOT;
    case FIFA96_MENU_SCREEN_MODEM:
      if (model->list_row == 1) {
        model->screen = FIFA96_MENU_SCREEN_GAME_SELECT;
        model->list_row = 0;
        model->list_scroll = 0;
        return FIFA96_MENU_ACTION_MODEM_BACK;
      }
      return FIFA96_MENU_ACTION_NONE;   /* notice row is not actionable */
    case FIFA96_MENU_SCREEN_LIST:
      return FIFA96_MENU_ACTION_NONE;   /* read-only */
    case FIFA96_MENU_SCREEN_OPTIONS: {
      if (model->option_page != FIFA96_MENU_OPTION_PAGE_LIST) {
        model->option_page = FIFA96_MENU_OPTION_PAGE_LIST;
        return FIFA96_MENU_ACTION_NONE;
      }
      if (model->option_row == FIFA96_MENU_OPTION_ROW_CONTROLS) {
        model->option_page = FIFA96_MENU_OPTION_PAGE_CONTROLS;
        return FIFA96_MENU_ACTION_NONE;
      }
      if (model->option_row == FIFA96_MENU_OPTION_ROW_RESET) {
        /* Port action: re-seed the retail defaults + descriptor max. */
        menu_model_settings_defaults(model);
        return FIFA96_MENU_ACTION_NONE;
      }
      int setting = fifa96_menu_model_option_setting(model->option_row);
      if (setting >= 0 && option_descriptors[setting].live) {
        /* Gamepad convenience: Enter cycles the row (sliders step +5). */
        (void)fifa96_settings_set(&model->settings, (uint32_t)setting, -2);
      }
      return FIFA96_MENU_ACTION_NONE;
    }
    default:
      return FIFA96_MENU_ACTION_NONE;
  }
}

int fifa96_menu_model_back(struct fifa96_menu_model *model) {
  if (model == NULL) return 0;
  if (model->screen == FIFA96_MENU_SCREEN_OPTIONS &&
      model->option_page != FIFA96_MENU_OPTION_PAGE_LIST) {
    model->option_page = FIFA96_MENU_OPTION_PAGE_LIST;
    return 1;
  }
  if (model->screen == FIFA96_MENU_SCREEN_GAME_SELECT ||
      model->screen == FIFA96_MENU_SCREEN_LIBRARY)
    return 0;
  if (model->screen == FIFA96_MENU_SCREEN_SAVE_GAME ||
      model->screen == FIFA96_MENU_SCREEN_LIST) {
    model->screen = FIFA96_MENU_SCREEN_COMPETITION_HUB;
    model->list_row = 0;
    model->list_scroll = 0;
    return 1;
  }
  model->screen = FIFA96_MENU_SCREEN_GAME_SELECT;
  model->pending_kind = 0;
  model->list_row = 0;
  model->list_scroll = 0;
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

/* P6: the menu row's mode-4 ("World") index. The single 44-entry mapping lives
 * in fifa96_team_data.c; the menu table's LANG ids are the keys, so the two
 * tables cannot drift. Returns -1 for an invalid row. */
int fifa96_menu_team_pccd_index(int index) {
  if (index < 0 || index >= FIFA96_MENU_TEAM_COUNT) return -1;
  return fifa96_team_data_lang_index(menu_teams[index].lang_id);
}

/* ------------------------------------------------------------------------ *
 * P6 Options metadata and row helpers.
 * ------------------------------------------------------------------------ */

const struct fifa96_menu_option_desc *fifa96_menu_option_descriptors(void) {
  return option_descriptors;
}

int fifa96_menu_option_descriptor_count(void) {
  return FIFA96_MENU_OPTION_DESC_COUNT;
}

const uint8_t *fifa96_menu_option_order(void) {
  return option_order;
}

const struct fifa96_menu_option_desc *fifa96_menu_option_descriptor(int setting) {
  if (setting < 0 || setting >= FIFA96_MENU_OPTION_DESC_COUNT) return NULL;
  return &option_descriptors[setting];
}

uint32_t fifa96_menu_option_choice_id(int setting, int32_t value) {
  const struct fifa96_menu_option_desc *d = fifa96_menu_option_descriptor(setting);
  if (d == NULL || d->is_slider || d->choices == NULL) return 0;
  /* Half length's descriptor max is 7, but the apply table 0x37170 has eight
   * entries; index 7 (0 minutes) has no LANG choice string. */
  if (setting == 0x0E) {
    if (value < 0 || value >= 7) return 0;
    return d->choices[value];
  }
  if (value < 0 || value >= d->max) return 0;
  return d->choices[value];
}

const char *fifa96_menu_option_choice_text(int setting, int32_t value) {
  const struct fifa96_menu_option_desc *d = fifa96_menu_option_descriptor(setting);
  if (d == NULL || d->is_slider || d->choice_text == NULL) return NULL;
  if (setting == 0x0E) {
    if (value < 0 || value >= (int32_t)FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT) return NULL;
    if (value == 7) return "0";   /* native 8th minute value, shown honestly */
    return d->choice_text[value];
  }
  if (value < 0 || value >= d->max) return NULL;
  return d->choice_text[value];
}

int fifa96_menu_model_option_rows(void) {
  return FIFA96_MENU_OPTION_ROW_COUNT;
}

int fifa96_menu_model_option_setting(int row) {
  if (row < 0 || row >= FIFA96_MENU_OPTION_NATIVE_ROWS) return -1;
  return (int)option_order[row];
}

const char *fifa96_menu_model_option_label(int row) {
  if (row == FIFA96_MENU_OPTION_ROW_CONTROLS) return "Controls";
  if (row == FIFA96_MENU_OPTION_ROW_RESET) return "Reset Defaults";
  int setting = fifa96_menu_model_option_setting(row);
  if (setting < 0) return NULL;
  return option_descriptors[setting].label;
}

uint32_t fifa96_menu_model_option_label_id(int row) {
  int setting = fifa96_menu_model_option_setting(row);
  if (setting < 0) return 0;
  return option_descriptors[setting].label_id;
}

int fifa96_menu_model_option_live(int row) {
  int setting = fifa96_menu_model_option_setting(row);
  if (setting < 0) return 0;
  return option_descriptors[setting].live != 0;
}

int fifa96_menu_model_option_slider(int row) {
  int setting = fifa96_menu_model_option_setting(row);
  if (setting < 0) return 0;
  return option_descriptors[setting].is_slider != 0;
}

int fifa96_menu_model_option_value(const struct fifa96_menu_model *model,
                                   int row, char *buf, size_t cap) {
  if (model == NULL || buf == NULL || cap == 0) return 0;
  buf[0] = '\0';
  int setting = fifa96_menu_model_option_setting(row);
  if (setting < 0) return 0;
  const struct fifa96_menu_option_desc *d = &option_descriptors[setting];
  int32_t value = model->settings.value[setting];
  if (d->is_slider) {
    snprintf(buf, cap, "%d", (int)value);
    return 1;
  }
  if (setting == 0x0E) {
    if (value >= 0 && value < (int32_t)FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT) {
      snprintf(buf, cap, "%d min", (int)fifa96_match_config_half_lengths[value]);
      return 1;
    }
    snprintf(buf, cap, "?");
    return 1;
  }
  const char *text = fifa96_menu_option_choice_text(setting, value);
  snprintf(buf, cap, "%s", text != NULL ? text : "?");
  return 1;
}

const char *const *fifa96_menu_model_controls_text(size_t *count) {
  if (count != NULL) *count = sizeof option_controls_text / sizeof option_controls_text[0];
  return option_controls_text;
}
