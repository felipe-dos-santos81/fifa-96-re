#pragma once
#include <stdint.h>
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_menu_model.h"
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_font.h"

#define FIFA96_MENU_VISIBLE_ROWS 8
#define FIFA96_MENU_ENTRY_COUNT 20

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
  const char *message;          /* Options stand-in body */
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
