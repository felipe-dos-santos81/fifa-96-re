#pragma once
#include <stdint.h>
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_surface.h"

#define FIFA96_MENU_VISIBLE_ROWS 8
#define FIFA96_MENU_ENTRY_COUNT 20

/* FU135 geometry: the original front-end menu is a 20-entry list showing 8
 * rows at UI x=0x85, y=0xB8+0x21*k, width=0x1BC in a 0x280x0x1E0 canvas. */
struct fifa96_menu_state {
  uint32_t entry_state;
  int selected_row;
  int cursor_on;
};

/* (Re)loads menu art; NULL resets to the built-in fallback renderer.
 * Initialization order: fifa96_frontend_run_init() resets the renderer to the
 * fallback, so callers wanting asset-backed menus must call
 * fifa96_menu_art_init(assets) after it (fifa96_engine_boot does). */
int fifa96_menu_art_init(struct fifa96_asset_table *assets_or_null);
void fifa96_menu_art_draw(struct fifa96_surface *s, const struct fifa96_menu_state *st);

#define FIFA96_MENU_FALLBACK_HASH 0x1be5e1aaceefaf09ull
