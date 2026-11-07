#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_settings.h"

/* P2 (A3 §1.1/§2.3): the engine-side Game Select / Team Select model.
 *
 * Game Select mirrors the native `FUN_0001FC50` widget order: right column
 * (`-2` arg) Friendly/Leagues/Tournaments/Playoffs/Practise and left column
 * (`-3`) Load Game/Options/Modem Setup/Quit to DOS. Each item keeps its native
 * label id (the `FUN_0001771C(id)` argument) and action code (the packed widget
 * id's low byte, the value the screen returns). P2 originally shipped only
 * Friendly/Options/Quit live; P13 enables every entry: Leagues/Tournaments/
 * Playoffs/Practise open the single-team Team Select, Load Game opens the
 * derived slot list, and Modem Setup opens the derived offline notice.
 *
 * Team Select uses the 44 national names A3 verified as plain LANG000 entries.
 * The ids are language-stable; the three-letter codes are fallbacks. P6 maps
 * each row to its PCDB mode-4 slot and replaces the code with the record +0x08
 * short name when `fifa96_team_data` is available (Brazil -> BRA, Italy ->
 * ITALY; `fifa96_menu_team_pccd_index`). */

#define FIFA96_MENU_ITEM_COUNT 9
#define FIFA96_MENU_TEAM_COUNT 44
#define FIFA96_MENU_TEAM_VISIBLE 7

/* P13 derived screens: the hub, Load/Save/Modem and the read-only table/
 * bracket all share one eight-row list window. Ten save slots overflow it, so
 * the window scrolls. */
#define FIFA96_MENU_LIST_VISIBLE 8
#define FIFA96_MENU_SAVE_SLOTS 10

/* P6 (P6-options §1/§3): the native Options table is 26 nine-byte records at
 * 0x105340 = {u32 label LANG id; u32 choice LANG-id array; u8 max}, indexed by
 * the setting number. The screen order is the 20-byte table at 0x10542A:
 * 0,18,1,17,2,19,3,4,6,7,21,9,10,11,12,13,14,15,16,66. Slot 19 is the dead
 * descriptor 66: `FUN_0001D948(66)` reads the dword at `0x105340 + 66*9 =
 * 0x105592`, which is `0x001C0000`, not a descriptor; the model never selects
 * it. Rows 0..18 are therefore the 19 real native rows, and two
 * port additions follow: the Controls page and Reset Defaults. Both are
 * labelled in comments and the docs as port additions (there is no native
 * options row for either). */
#define FIFA96_MENU_OPTION_DESC_COUNT 26
#define FIFA96_MENU_OPTION_ORDER_COUNT 20
#define FIFA96_MENU_OPTION_NATIVE_ROWS 19
#define FIFA96_MENU_OPTION_VISIBLE 8
#define FIFA96_MENU_OPTION_ROW_CONTROLS 19   /* port addition */
#define FIFA96_MENU_OPTION_ROW_RESET 20      /* port addition */
#define FIFA96_MENU_OPTION_ROW_COUNT 21
#define FIFA96_MENU_OPTION_PAGE_LIST 0
#define FIFA96_MENU_OPTION_PAGE_CONTROLS 1
#define FIFA96_MENU_OPTION_HELP_MAX 16

struct fifa96_menu_option_desc {
  uint8_t setting;          /* index into fifa96_settings.value[] */
  uint8_t max;              /* descriptor +8 (0..max-1 are valid values) */
  uint8_t is_slider;        /* 0x11..0x13: 0..99, step 5, native bar */
  uint8_t live;             /* P6: the port has a consumer for this setting */
  uint32_t label_id;        /* FUN_0001771C id; 0 for an empty descriptor */
  const char *label;        /* compiled-in ASCII fallback */
  const uint32_t *choices;  /* LANG ids, `max` entries; NULL for sliders */
  const char *const *choice_text;  /* ASCII fallbacks, `max` entries */
};

enum fifa96_menu_screen {
  /* 0 is the legacy FU-135 library placeholder: zero-initialised callers and
   * the deterministic M1/M2 tapes keep the old render byte-for-byte. */
  FIFA96_MENU_SCREEN_LIBRARY = 0,
  FIFA96_MENU_SCREEN_GAME_SELECT = 1,
  FIFA96_MENU_SCREEN_TEAM_SELECT = 2,
  FIFA96_MENU_SCREEN_OPTIONS = 3,
  /* P13: appended so the historical values never move. */
  FIFA96_MENU_SCREEN_COMPETITION_HUB = 4,
  FIFA96_MENU_SCREEN_LOAD_GAME = 5,
  FIFA96_MENU_SCREEN_SAVE_GAME = 6,
  FIFA96_MENU_SCREEN_MODEM = 7,
  FIFA96_MENU_SCREEN_LIST = 8
};

enum fifa96_menu_action {
  FIFA96_MENU_ACTION_NONE = 0,
  FIFA96_MENU_ACTION_FRIENDLY = 1,
  FIFA96_MENU_ACTION_OPTIONS = 2,
  FIFA96_MENU_ACTION_QUIT = 3,
  FIFA96_MENU_ACTION_START_MATCH = 4,
  /* P13 single-player flows. LEAGUE..PRACTISE are also the native mode bytes
   * (1..4) and are stored in `pending_kind` while Team Select is open. */
  FIFA96_MENU_ACTION_LEAGUE = 5,
  FIFA96_MENU_ACTION_TOURNAMENT = 6,
  FIFA96_MENU_ACTION_PLAYOFF = 7,
  FIFA96_MENU_ACTION_PRACTISE = 8,
  FIFA96_MENU_ACTION_LOAD_GAME = 9,
  FIFA96_MENU_ACTION_HUB_PLAY = 10,
  FIFA96_MENU_ACTION_HUB_SIMULATE = 11,
  FIFA96_MENU_ACTION_HUB_TABLE = 12,
  FIFA96_MENU_ACTION_HUB_SAVE = 13,
  FIFA96_MENU_ACTION_HUB_EXIT = 14,
  FIFA96_MENU_ACTION_SAVE_SLOT = 15,
  FIFA96_MENU_ACTION_LOAD_SLOT = 16,
  FIFA96_MENU_ACTION_MODEM_BACK = 17
};

struct fifa96_menu_item_desc {
  uint32_t label_id;        /* FUN_0001771C(id) */
  const char *fallback;     /* compiled-in ASCII, used without LANG000 */
  uint8_t code;             /* native screen return code */
  int8_t col;               /* 0 = left (-3), 1 = right (-2) */
  int8_t row;
  enum fifa96_menu_action action;
};

struct fifa96_menu_model {
  enum fifa96_menu_screen screen;
  int item;                 /* index into fifa96_menu_model_items() */
  int team[2];              /* home/visitor index into the 44-name table */
  int team_col;             /* active team-select column */
  /* P6: the retail options block (0x149278 / options.dat shape). `max` is
   * copied from fifa96_settings_descriptor_max so fifa96_settings_set works;
   * fifa96_menu_model_reset keeps these values across a post-match return. */
  struct fifa96_settings settings;
  int option_row;           /* absolute model row 0..20 */
  int option_scroll;        /* first visible native row, 0..13 */
  int option_page;          /* 0 list, 1 Controls (port addition) */
  /* P13: 0 = Friendly's two-column Team Select; 1..4 = the native competition
   * mode (League/Tournament/Playoff/Practise) whose single-team Team Select is
   * open. The hub and list screens share `list_row`/`list_scroll`; the front-end
   * sets `list_count` when it builds a read-only table/bracket. */
  uint8_t pending_kind;
  int list_row;
  int list_scroll;
  int list_count;
};

void fifa96_menu_model_init(struct fifa96_menu_model *model);

/* P6: re-enter Game Select after a match without touching `settings`, so the
 * Options chosen before kickoff survive the return (the native driver keeps
 * the 0x68-byte block; only an explicit cancel restores the backup). */
void fifa96_menu_model_reset(struct fifa96_menu_model *model);

/* Apply a front-end direction code (0 up, 1 down, 2 left, 3 right). Returns 1
 * when the model is a navigable screen (the caller then redraws). */
int fifa96_menu_model_nav(struct fifa96_menu_model *model, int32_t code);

/* Consume the confirm code. Friendly/Options switch screens and return NONE;
 * Quit returns QUIT; team-select Play returns START_MATCH; a disabled Game
 * Select item is a no-op. */
enum fifa96_menu_action fifa96_menu_model_activate(struct fifa96_menu_model *model);

/* Back/decline: team-select and options return to Game Select (1); Game Select
 * has no back arm (0). P6: the Options Controls page backs to the list first. */
int fifa96_menu_model_back(struct fifa96_menu_model *model);

const struct fifa96_menu_item_desc *fifa96_menu_model_items(void);
const char *fifa96_menu_model_item_fallback(int index);

/* P6 native option metadata (P6-options §1). `fifa96_menu_option_descriptors`
 * returns the 26 records (index == setting); the order returns the 20 native
 * slots (last = 66, the dead descriptor). */
const struct fifa96_menu_option_desc *fifa96_menu_option_descriptors(void);
int fifa96_menu_option_descriptor_count(void);
const uint8_t *fifa96_menu_option_order(void);
const struct fifa96_menu_option_desc *fifa96_menu_option_descriptor(int setting);

/* Choice helpers: the LANG id of a valid value (0 when the value has no
 * string, e.g. half-length index 7, the native 0-minute table entry) and the
 * compiled-in ASCII fallback. */
uint32_t fifa96_menu_option_choice_id(int setting, int32_t value);
const char *fifa96_menu_option_choice_text(int setting, int32_t value);

/* Model row helpers. Rows 0..18 map through the native order table; rows 19
 * (Controls) and 20 (Reset Defaults) are labelled port additions. */
int fifa96_menu_model_option_rows(void);
int fifa96_menu_model_option_setting(int row);       /* -1 for port rows */
const char *fifa96_menu_model_option_label(int row);
uint32_t fifa96_menu_model_option_label_id(int row);
int fifa96_menu_model_option_live(int row);
int fifa96_menu_model_option_slider(int row);

/* Format a row's display value into `buf` (1 on success, 0 otherwise). Half
 * length appends the port-added "min" suffix and understands the native
 * 8-entry {2,4,6,8,10,20,45,0} table, including index 7. */
int fifa96_menu_model_option_value(const struct fifa96_menu_model *model,
                                   int row, char *buf, size_t cap);

/* The Controls page text block (port addition; real current mappings). The
 * returned array is static; *count receives its length. */
const char *const *fifa96_menu_model_controls_text(size_t *count);

uint32_t fifa96_menu_team_id(int index);
const char *fifa96_menu_team_fallback(int index);
const char *fifa96_menu_team_code(int index);

/* P6: the row's PCDB mode-4 ("World") slot index (0..61), or -1 when `index`
 * is outside the 44-row table. The mapping is the single table in
 * fifa96_team_data.c keyed by the same LANG id. */
int fifa96_menu_team_pccd_index(int index);
