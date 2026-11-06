#pragma once
#include <stdint.h>

/* P2 (A3 §1.1/§2.3): the engine-side Game Select / Team Select model.
 *
 * Game Select mirrors the native `FUN_0001FC50` widget order: right column
 * (`-2` arg) Friendly/Leagues/Tournaments/Playoffs/Practise and left column
 * (`-3`) Load Game/Options/Modem Setup/Quit to DOS. Each item keeps its native
 * label id (the `FUN_0001771C(id)` argument) and action code (the packed widget
 * id's low byte, the value the screen returns). Only Friendly/Options/Quit are
 * live in P2; the other six are drawn disabled (A3-L1/A3-L2 legs).
 *
 * Team Select uses the 44 national names A3 verified as plain LANG000 entries.
 * The ids are language-stable; the three-letter codes are fallbacks. P6 maps
 * each row to its PCDB mode-4 slot and replaces the code with the record +0x08
 * short name when `fifa96_team_data` is available (Brazil -> BRA, Italy ->
 * ITALY; `fifa96_menu_team_pccd_index`). */

#define FIFA96_MENU_ITEM_COUNT 9
#define FIFA96_MENU_TEAM_COUNT 44
#define FIFA96_MENU_TEAM_VISIBLE 7

enum fifa96_menu_screen {
  /* 0 is the legacy FU-135 library placeholder: zero-initialised callers and
   * the deterministic M1/M2 tapes keep the old render byte-for-byte. */
  FIFA96_MENU_SCREEN_LIBRARY = 0,
  FIFA96_MENU_SCREEN_GAME_SELECT = 1,
  FIFA96_MENU_SCREEN_TEAM_SELECT = 2,
  FIFA96_MENU_SCREEN_OPTIONS = 3
};

enum fifa96_menu_action {
  FIFA96_MENU_ACTION_NONE = 0,
  FIFA96_MENU_ACTION_FRIENDLY = 1,
  FIFA96_MENU_ACTION_OPTIONS = 2,
  FIFA96_MENU_ACTION_QUIT = 3,
  FIFA96_MENU_ACTION_START_MATCH = 4
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
};

void fifa96_menu_model_init(struct fifa96_menu_model *model);

/* Apply a front-end direction code (0 up, 1 down, 2 left, 3 right). Returns 1
 * when the model is a navigable screen (the caller then redraws). */
int fifa96_menu_model_nav(struct fifa96_menu_model *model, int32_t code);

/* Consume the confirm code. Friendly/Options switch screens and return NONE;
 * Quit returns QUIT; team-select Play returns START_MATCH; a disabled Game
 * Select item is a no-op. */
enum fifa96_menu_action fifa96_menu_model_activate(struct fifa96_menu_model *model);

/* Back/decline: team-select and options return to Game Select (1); Game Select
 * has no back arm (0). */
int fifa96_menu_model_back(struct fifa96_menu_model *model);

const struct fifa96_menu_item_desc *fifa96_menu_model_items(void);
const char *fifa96_menu_model_item_fallback(int index);

uint32_t fifa96_menu_team_id(int index);
const char *fifa96_menu_team_fallback(int index);
const char *fifa96_menu_team_code(int index);

/* P6: the row's PCDB mode-4 ("World") slot index (0..61), or -1 when `index`
 * is outside the 44-row table. The mapping is the single table in
 * fifa96_team_data.c keyed by the same LANG id. */
int fifa96_menu_team_pccd_index(int index);
