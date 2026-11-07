#pragma once
#include <stdint.h>
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_menu_model.h"
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_font.h"

#define FIFA96_MENU_VISIBLE_ROWS 8
#define FIFA96_MENU_ENTRY_COUNT 20
#define FIFA96_MENU_LIST_TEXT_MAX 96

/* FU135 geometry: the original front-end menu is a 20-entry list showing 8
 * rows at UI x=0x85, y=0xB8+0x21*k, width=0x1BC in a 0x280x0x1E0 canvas. */
struct fifa96_menu_state {
  uint32_t entry_state;
  int selected_row;
  int cursor_on;
  /* P2: 0 (FIFA96_MENU_SCREEN_LIBRARY) keeps the legacy render above exactly;
   * the native screens are drawn from the draw-ready snapshot below. */
  enum fifa96_menu_screen screen;
  const fifa96_font *font;      /* HELV12 from fifa96_lang; NULL = no labels */
  const char *title;
  const char *footer;
  struct {
    const char *label;
    int col;                    /* 0 = left, 1 = right */
    int row;
    int enabled;
  } items[FIFA96_MENU_ITEM_COUNT];
  int item_count;
  int selected_item;
  const char *column_label[2];  /* Home / Visitor */
  const char *teams[2][FIFA96_MENU_TEAM_VISIBLE];
  int team_rows;
  int team_selected_row[2];     /* visible row, -1 when off-window */
  int team_col;
  /* P6 Options (P6-options §3): the draw-ready snapshot of the native 8-row
   * window, the Controls page text block, and the value storage the front-end
   * formats into. `slider_pct` is -1 for non-sliders. */
  struct {
    const char *label;
    const char *value;
    int enabled;
    int slider_pct;
  } options[FIFA96_MENU_OPTION_VISIBLE];
  char option_value_text[FIFA96_MENU_OPTION_VISIBLE][40];
  int option_count;             /* total model rows (21) */
  int option_selected;          /* absolute row index */
  int option_scroll;            /* first visible row */
  int option_page;              /* 0 list, 1 Controls */
  const char *help[FIFA96_MENU_OPTION_HELP_MAX];
  int help_count;
  int team_locked;              /* P13: single-team Team Select (visitor hidden) */
  /* P13 derived list screens: the hub, Load/Save/Modem and the read-only
   * table/bracket snapshot. `list_rows` is the eight-row visible window;
   * `list_row_count`/`list_selected`/`list_scroll` are the absolute model. */
  const char *list_title;
  const char *list_footer;
  struct {
    const char *label;
    int enabled;
  } list_rows[FIFA96_MENU_LIST_VISIBLE];
  char list_text[FIFA96_MENU_LIST_VISIBLE][FIFA96_MENU_LIST_TEXT_MAX];
  int list_row_count;
  int list_selected;
  int list_scroll;
  const char *list_help;
  char list_help_text[FIFA96_MENU_LIST_TEXT_MAX];
};

/* Legacy loader: tries OPTIONS.INV only (the FU-135 placeholder path; the
 * deterministic M1/M2 tapes call this). NULL resets to the built-in fallback
 * renderer. */
int fifa96_menu_art_init(struct fifa96_asset_table *assets_or_null);

/* P2 native loader: tries /FEGFX/MOD2.POG frame 0 first (the real Game Select
 * background), then the legacy OPTIONS.INV. Falls back exactly like
 * fifa96_menu_art_init when neither decodes. */
int fifa96_menu_art_init_native(struct fifa96_asset_table *assets_or_null);

void fifa96_menu_art_draw(struct fifa96_surface *s, const struct fifa96_menu_state *st);

#define FIFA96_MENU_FALLBACK_HASH 0x1be5e1aaceefaf09ull
