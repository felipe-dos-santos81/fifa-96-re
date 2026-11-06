#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_menu_art.h"
#include "fifa96_loader/fifa96_record.h"
#include "fifa96_loader/fifa96_sprite.h"

#define MENU_UI_W 0x280
#define MENU_UI_H 0x1E0
#define MENU_UI_X 0x85
#define MENU_UI_Y 0xB8
#define MENU_UI_STEP 0x21
#define MENU_UI_ROW_W 0x1BC

#define MENU_ART_DECODE_MAX (64u * 1024u * 1024u)

/* P2 native Game Select geometry (A3 §4.3, measured on MOD2 frame 0 and
 * divided by two): left column 32..143, right 172..292, row pitch 22, height
 * 14, first row y 78; the fifth right slot is at 166. These are 320x240
 * coordinates scaled to the engine surface. */
#define MENU_GS_LEFT_X 32
#define MENU_GS_RIGHT_X 172
#define MENU_GS_TOP 78
#define MENU_GS_PITCH 22
#define MENU_GS_ROW_H 14
#define MENU_GS_LEFT_W 112
#define MENU_GS_RIGHT_W 120

/* P6 Options geometry: the native 640x480 column (x 0x85, width 0x1BC, first
 * row y 0xB8, pitch 0x21, row height 14) divided by two, in the engine's
 * 320x240 canvas like MENU_GS_* above. */
#define MENU_OPT_X 66
#define MENU_OPT_W 222
#define MENU_OPT_TOP 92
#define MENU_OPT_PITCH 16
#define MENU_OPT_ROW_H 14
#define MENU_OPT_HELP_TOP 28
#define MENU_OPT_HELP_PITCH 14

enum {
  MENU_IDX_BG = 1,
  MENU_IDX_PANEL = 2,
  MENU_IDX_ROW = 3,
  MENU_IDX_SEL = 4,
  MENU_IDX_CURSOR = 5,
  MENU_IDX_BAR = 6,
  MENU_PALETTE_COUNT = 7
};

static struct {
  int background_ready;
  int palette_ready;
  uint8_t background[FIFA96_SURFACE_MAX_W * FIFA96_SURFACE_MAX_H];
  uint8_t palette[768];
} menu_art;

static const uint8_t menu_fallback_palette[MENU_PALETTE_COUNT][3] = {
  {0x00, 0x00, 0x00},
  {0x00, 0x00, 0xA8},
  {0xA8, 0xA8, 0xA8},
  {0x54, 0x54, 0x54},
  {0xFC, 0xFC, 0x54},
  {0xFC, 0x00, 0x00},
  {0xFC, 0xFC, 0xFC}
};

struct menu_colors {
  uint8_t main;
  uint8_t shadow;
  uint8_t sel_fill;
  uint8_t sel_text;
  uint8_t dim;
};

static int menu_char_fold(int c) {
  return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

static int menu_ci_equal(const char *a, const char *b) {
  while (*a && *b) {
    if (menu_char_fold((unsigned char)*a) != menu_char_fold((unsigned char)*b)) return 0;
    a++;
    b++;
  }
  return *a == 0 && *b == 0;
}

static const char *menu_find_asset(const struct fifa96_asset_table *t, const char *name) {
  if (t == NULL || t->entries == NULL) return NULL;
  for (size_t i = 0; i < t->count; i++) {
    const char *p = t->entries[i].path;
    const char *base = p;
    for (const char *q = p; *q; q++) {
      if (*q == '/' || *q == '\\') base = q + 1;
    }
    if (menu_ci_equal(base, name)) return p;
  }
  return NULL;
}

/* POG records are `selector, 0xFB, BE24 decoded length`; already-decoded SHPI
 * test fixtures pass through borrowed. */
static int menu_decode_record(const uint8_t *src, size_t src_len, uint8_t **out,
                              size_t *out_len) {
  if (src == NULL || out == NULL || out_len == NULL || src_len < 5) return -1;
  *out = NULL;
  *out_len = 0;
  if (src_len >= 4 && memcmp(src, "SHPI", 4) == 0) {
    *out_len = src_len;
    return 0;
  }
  if (src[1] != 0xFB) return -1;
  uint32_t declared = ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 8) | (uint32_t)src[4];
  if (declared == 0 || declared > MENU_ART_DECODE_MAX) return -1;
  uint8_t *dst = malloc((size_t)declared);
  if (dst == NULL) return -1;
  size_t got = 0;
  if (fifa96_record_decode(src, src_len, dst, (size_t)declared, &got) != FIFA96_OK ||
      got != (size_t)declared) {
    free(dst);
    return -1;
  }
  *out = dst;
  *out_len = got;
  return 0;
}

static void menu_art_reset(void) {
  menu_art.background_ready = 0;
  menu_art.palette_ready = 0;
}

/* Load one sprite-bank file as the menu background. Returns 1 on success. */
static int menu_art_try_load(struct fifa96_asset_table *assets, const char *name) {
  const char *path = menu_find_asset(assets, name);
  if (path == NULL) return 0;

  uint8_t *bytes = NULL;
  size_t len = 0;
  if (fifa96_asset_read(assets, path, &bytes, &len) != FIFA96_OK || bytes == NULL)
    return 0;
  if (len == 0) {
    fifa96_asset_free(bytes);
    return 0;
  }

  uint8_t *decoded = NULL;
  size_t decoded_len = 0;
  const uint8_t *src = bytes;
  size_t src_len = len;
  if (menu_decode_record(bytes, len, &decoded, &decoded_len) != 0) {
    fifa96_asset_free(bytes);
    return 0;
  }
  if (decoded != NULL) {
    src = decoded;
    src_len = decoded_len;
  }

  fifa96_sprite_bank bank;
  fifa96_sprite_entry entry;
  fifa96_sprite_frame frame;
  int loaded = 0;
  if (fifa96_sprite_bank_parse(src, src_len, &bank) == FIFA96_OK &&
      fifa96_sprite_bank_entry(&bank, 0, &entry) == FIFA96_OK &&
      fifa96_sprite_frame_parse(&bank, entry.offset, &frame) == FIFA96_OK &&
      frame.width > 0 && frame.height > 0 &&
      (uint64_t)frame.width * frame.height <= frame.pixel_len) {
    for (int y = 0; y < FIFA96_SURFACE_MAX_H; y++) {
      uint32_t sy = (uint32_t)y * frame.height / FIFA96_SURFACE_MAX_H;
      for (int x = 0; x < FIFA96_SURFACE_MAX_W; x++) {
        uint32_t sx = (uint32_t)x * frame.width / FIFA96_SURFACE_MAX_W;
        menu_art.background[(size_t)y * FIFA96_SURFACE_MAX_W + x] =
            frame.pixels[(size_t)sy * frame.width + sx];
      }
    }
    menu_art.background_ready = 1;
    loaded = 1;

    fifa96_sprite_chunk chunk;
    const uint8_t *rgb6 = NULL;
    uint16_t count = 0;
    if (fifa96_sprite_chunk_parse(&bank, entry.offset, &chunk) == FIFA96_OK &&
        fifa96_sprite_chunk_palette(&chunk, &rgb6, &count) == FIFA96_OK &&
        count <= 256) {
      memset(menu_art.palette, 0, sizeof menu_art.palette);
      fifa96_sprite_palette_to_rgb(rgb6, count, menu_art.palette);
      menu_art.palette_ready = 1;
    }
  }

  free(decoded);
  fifa96_asset_free(bytes);
  return loaded;
}

int fifa96_menu_art_init(struct fifa96_asset_table *assets_or_null) {
  menu_art_reset();
  if (assets_or_null == NULL || assets_or_null->entries == NULL ||
      assets_or_null->count == 0) {
    return 0;
  }
  (void)menu_art_try_load(assets_or_null, "OPTIONS.INV");
  return 0;
}

int fifa96_menu_art_init_native(struct fifa96_asset_table *assets_or_null) {
  menu_art_reset();
  if (assets_or_null == NULL || assets_or_null->entries == NULL ||
      assets_or_null->count == 0) {
    return 0;
  }
  if (menu_art_try_load(assets_or_null, "MOD2.POG")) return 0;
  (void)menu_art_try_load(assets_or_null, "OPTIONS.INV");
  return 0;
}

static void menu_fill(struct fifa96_surface *s, int x, int y, int w, int h, uint8_t idx) {
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > s->width) w = s->width - x;
  if (y + h > s->height) h = s->height - y;
  if (w <= 0 || h <= 0) return;
  for (int r = 0; r < h; r++) {
    memset(s->indexed + (size_t)(y + r) * (size_t)s->width + (size_t)x, idx, (size_t)w);
  }
}

static void menu_draw_background(struct fifa96_surface *s) {
  if (menu_art.background_ready) {
    for (int y = 0; y < s->height; y++) {
      uint32_t sy = (uint32_t)y * FIFA96_SURFACE_MAX_H / (uint32_t)s->height;
      for (int x = 0; x < s->width; x++) {
        uint32_t sx = (uint32_t)x * FIFA96_SURFACE_MAX_W / (uint32_t)s->width;
        s->indexed[(size_t)y * (size_t)s->width + (size_t)x] =
            menu_art.background[(size_t)sy * FIFA96_SURFACE_MAX_W + sx];
      }
    }
  } else {
    fifa96_surface_clear(s, MENU_IDX_BG);
  }
}

static void menu_install_palette(struct fifa96_surface *s) {
  uint8_t palette[768];
  memset(palette, 0, sizeof palette);
  if (menu_art.palette_ready) {
    memcpy(palette, menu_art.palette, sizeof palette);
  } else {
    for (int i = 0; i < MENU_PALETTE_COUNT; i++) {
      palette[i * 3] = menu_fallback_palette[i][0];
      palette[i * 3 + 1] = menu_fallback_palette[i][1];
      palette[i * 3 + 2] = menu_fallback_palette[i][2];
    }
  }
  fifa96_surface_set_palette8(s, palette);
}

static struct menu_colors menu_colors_for_art(void) {
  /* MOD2's UI ramp: 0 black, 1 near-white, 3 light yellow, 9 amber. The
   * compiled-in fallback palette uses its own 4/6/2 entries. */
  if (menu_art.palette_ready) {
    struct menu_colors c = {1, 0, 3, 0, 9};
    return c;
  }
  struct menu_colors c = {4, 0, 6, 0, 2};
  return c;
}

static void menu_text(struct fifa96_surface *s, const fifa96_font *font, const char *str,
                      int x, int y, uint8_t color) {
  if (font == NULL || str == NULL || s == NULL) return;
  (void)fifa96_font_blit(s->indexed, s->width, s->height, font, str, x, y, color);
}

static void menu_text_shadow(struct fifa96_surface *s, const fifa96_font *font,
                             const char *str, int x, int y, uint8_t color) {
  if (font == NULL || str == NULL || s == NULL) return;
  (void)fifa96_font_blit(s->indexed, s->width, s->height, font, str, x + 1, y + 1, 0);
  (void)fifa96_font_blit(s->indexed, s->width, s->height, font, str, x, y, color);
}

/* Native P2 geometry is expressed in the engine's 320x240 canvas (A3 §4.3),
 * unlike the legacy FU-135 0x280x0x1E0 coordinates above. */
static int menu_sx(const struct fifa96_surface *s, int v) {
  return v * s->width / 320;
}

static int menu_sy(const struct fifa96_surface *s, int v) {
  return v * s->height / 240;
}

static void menu_draw_library(struct fifa96_surface *s, const struct fifa96_menu_state *st) {
  const int x = MENU_UI_X * s->width / MENU_UI_W;
  const int y = MENU_UI_Y * s->height / MENU_UI_H;
  const int w = MENU_UI_ROW_W * s->width / MENU_UI_W;
  const int rh = MENU_UI_STEP * s->height / MENU_UI_H;
  int sel = st->selected_row;
  if (sel < 0) sel = 0;
  if (sel >= FIFA96_MENU_VISIBLE_ROWS) sel = FIFA96_MENU_VISIBLE_ROWS - 1;

  menu_fill(s, 0, 0, s->width, rh / 2 > 0 ? rh / 2 : 1, MENU_IDX_BAR);
  menu_fill(s, x - 2, y - 2, w + 4, rh * FIFA96_MENU_VISIBLE_ROWS + 4, MENU_IDX_PANEL);
  for (int k = 0; k < FIFA96_MENU_VISIBLE_ROWS; k++) {
    menu_fill(s, x, y + k * rh, w, rh - 2, k == sel ? MENU_IDX_SEL : MENU_IDX_ROW);
  }
  if (st->cursor_on) {
    menu_fill(s, x - 6, y + sel * rh + (rh - 2 - 8) / 2, 4, 8, MENU_IDX_CURSOR);
  }
}

static void menu_draw_game_select(struct fifa96_surface *s, const struct fifa96_menu_state *st,
                                  const struct menu_colors *c) {
  const int row_h = menu_sy(s, MENU_GS_ROW_H);
  for (int i = 0; i < st->item_count && i < FIFA96_MENU_ITEM_COUNT; i++) {
    int col = st->items[i].col;
    int x = menu_sx(s, col ? MENU_GS_RIGHT_X : MENU_GS_LEFT_X);
    int y = menu_sy(s, MENU_GS_TOP + st->items[i].row * MENU_GS_PITCH);
    int w = menu_sx(s, col ? MENU_GS_RIGHT_W : MENU_GS_LEFT_W);
    int text_x = x + menu_sx(s, 6);
    int text_y = y + (row_h - 13) / 2;
    if (i == st->selected_item) {
      menu_fill(s, x, y, w, row_h, c->sel_fill);
      menu_text(s, st->font, st->items[i].label, text_x, text_y, c->sel_text);
    } else {
      menu_text_shadow(s, st->font, st->items[i].label, text_x, text_y,
                       st->items[i].enabled ? c->main : c->dim);
    }
  }
  menu_text_shadow(s, st->font, st->title, menu_sx(s, 12), menu_sy(s, 8), c->main);
  menu_text_shadow(s, st->font, st->footer, menu_sx(s, 8), menu_sy(s, 225), c->main);
}

static void menu_draw_team_select(struct fifa96_surface *s, const struct fifa96_menu_state *st,
                                  const struct menu_colors *c) {
  const int row_h = menu_sy(s, 16);
  const int first_y = menu_sy(s, 64);
  const int pitch = menu_sy(s, 20);
  const int left_x = menu_sx(s, 40);
  const int right_x = menu_sx(s, 184);
  const int bar_x[2] = {menu_sx(s, 32), menu_sx(s, 176)};
  const int bar_w = menu_sx(s, 132);

  /* The 44-name list needs a readable panel over the photo background. */
  menu_fill(s, menu_sx(s, 12), menu_sy(s, 32), menu_sx(s, 296), menu_sy(s, 190), 0);
  menu_fill(s, menu_sx(s, 12), menu_sy(s, 32), menu_sx(s, 296), 1, c->dim);
  menu_fill(s, menu_sx(s, 12), menu_sy(s, 221), menu_sx(s, 296), 1, c->dim);

  menu_text_shadow(s, st->font, st->title, menu_sx(s, 12), menu_sy(s, 8), c->main);
  for (int side = 0; side < 2; side++) {
    int hx = side == 0 ? left_x : right_x;
    if (st->team_col == side)
      menu_text_shadow(s, st->font, ">", hx - menu_sx(s, 10), menu_sy(s, 44), c->main);
    menu_text_shadow(s, st->font, st->column_label[side], hx, menu_sy(s, 44), c->main);
    for (int r = 0; r < st->team_rows; r++) {
      int y = first_y + r * pitch;
      int tx = side == 0 ? left_x : right_x;
      int ty = y + (row_h - 13) / 2;
      if (r == st->team_selected_row[side]) {
        menu_fill(s, bar_x[side], y, bar_w, row_h, c->sel_fill);
        menu_text(s, st->font, st->teams[side][r], tx, ty, c->sel_text);
      } else {
        menu_text_shadow(s, st->font, st->teams[side][r], tx, ty, c->main);
      }
    }
  }
  menu_text_shadow(s, st->font, st->footer, menu_sx(s, 8), menu_sy(s, 225), c->main);
}

static void menu_draw_options(struct fifa96_surface *s, const struct fifa96_menu_state *st,
                              const struct menu_colors *c) {
  menu_text_shadow(s, st->font, st->title, menu_sx(s, 12), menu_sy(s, 8), c->main);

  if (st->option_page != FIFA96_MENU_OPTION_PAGE_LIST) {
    /* Port addition: the Controls text block, plain HELV12. */
    int x = menu_sx(s, MENU_OPT_X);
    int y = menu_sy(s, MENU_OPT_HELP_TOP);
    int pitch = menu_sy(s, MENU_OPT_HELP_PITCH);
    for (int i = 0; i < st->help_count && i < FIFA96_MENU_OPTION_HELP_MAX; i++) {
      menu_text_shadow(s, st->font, st->help[i], x, y + i * pitch,
                       i == 0 ? c->sel_fill : c->main);
    }
    menu_text_shadow(s, st->font, st->footer, menu_sx(s, 8), menu_sy(s, 225), c->main);
    return;
  }

  const int x = menu_sx(s, MENU_OPT_X);
  const int w = menu_sx(s, MENU_OPT_W);
  const int top = menu_sy(s, MENU_OPT_TOP);
  const int pitch = menu_sy(s, MENU_OPT_PITCH);
  const int row_h = menu_sy(s, MENU_OPT_ROW_H);
  const int label_x = x + menu_sx(s, 6);
  const int value_right = x + w - menu_sx(s, 6);

  for (int k = 0; k < FIFA96_MENU_OPTION_VISIBLE; k++) {
    int row = st->option_scroll + k;
    if (row >= st->option_count) break;
    int y = top + k * pitch;
    int text_y = y + (row_h - 13) / 2;
    int selected = row == st->option_selected;
    uint8_t color = selected ? c->sel_text
                             : (st->options[k].enabled ? c->main : c->dim);
    if (selected) menu_fill(s, x, y, w, row_h, c->sel_fill);
    if (st->options[k].label != NULL) {
      if (selected)
        menu_text(s, st->font, st->options[k].label, label_x, text_y, color);
      else
        menu_text_shadow(s, st->font, st->options[k].label, label_x, text_y, color);
    }
    if (st->options[k].value != NULL && st->options[k].value[0] != '\0') {
      int vw = fifa96_font_text_width(st->font, st->options[k].value);
      if (vw > value_right - label_x) vw = value_right - label_x;
      int vx = value_right - vw;
      if (vx < label_x) vx = label_x;
      if (selected)
        menu_text(s, st->font, st->options[k].value, vx, text_y, color);
      else
        menu_text_shadow(s, st->font, st->options[k].value, vx, text_y, color);
    }
  }

  /* Scroll indicator: only when the 21-row list overflows the 8-row window. */
  if (st->option_count > FIFA96_MENU_OPTION_VISIBLE) {
    const int track_x = x + w + menu_sx(s, 4);
    const int track_h = FIFA96_MENU_OPTION_VISIBLE * pitch - (pitch - row_h);
    int thumb_h = track_h * FIFA96_MENU_OPTION_VISIBLE / st->option_count;
    if (thumb_h < menu_sy(s, 8)) thumb_h = menu_sy(s, 8);
    int span = st->option_count - FIFA96_MENU_OPTION_VISIBLE;
    int thumb_y = top;
    if (span > 0) thumb_y += (track_h - thumb_h) * st->option_scroll / span;
    menu_fill(s, track_x, top, menu_sx(s, 2), track_h, c->dim);
    menu_fill(s, track_x, thumb_y, menu_sx(s, 2), thumb_h, c->main);
  }

  menu_text_shadow(s, st->font, st->footer, menu_sx(s, 8), menu_sy(s, 225), c->main);
}

void fifa96_menu_art_draw(struct fifa96_surface *s, const struct fifa96_menu_state *st) {
  if (s == NULL || st == NULL) return;

  menu_draw_background(s);
  menu_install_palette(s);

  if (st->screen == FIFA96_MENU_SCREEN_LIBRARY) {
    menu_draw_library(s, st);
    return;
  }

  struct menu_colors colors = menu_colors_for_art();
  switch (st->screen) {
    case FIFA96_MENU_SCREEN_GAME_SELECT:
      menu_draw_game_select(s, st, &colors);
      break;
    case FIFA96_MENU_SCREEN_TEAM_SELECT:
      menu_draw_team_select(s, st, &colors);
      break;
    case FIFA96_MENU_SCREEN_OPTIONS:
      menu_draw_options(s, st, &colors);
      break;
    default:
      menu_draw_library(s, st);
      break;
  }
}
