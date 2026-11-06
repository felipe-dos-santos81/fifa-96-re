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

/* Native shell canvas. MOD1/MOD2/VS/TITLE are 640x480/80x80 assets; the
 * windowed engine draws them at their native size and downsamples once to the
 * engine surface. The 320x240 tape path keeps the legacy composition below, so
 * M1/M2 and the hash-pinned menu tests are untouched. */
#define MENU_HIRES_W 640
#define MENU_HIRES_H 480
#define MENU_VS_FRAMES 13
#define MENU_VS_SIZE 80
#define MENU_BAR_W 300
#define MENU_BAR_H 48
#define MENU_LOGO_W 120
#define MENU_LOGO_H 91
#define MENU_LOGO_MAX (MENU_LOGO_W * MENU_LOGO_H)
#define MENU_HIRES_PIXELS (MENU_HIRES_W * MENU_HIRES_H)

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
/* P9: 15 help lines (the dual pad/keyboard roles and the half-time hint) must
 * clear the y=225 footer; 13 keeps the last line above it at 320x240. */
#define MENU_OPT_HELP_PITCH 13

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
  /* Native 640x480 shell (windowed path). Loaded only when the assets exist;
   * `hires_ready` gates the composition, so an ISO-less boot keeps the legacy
   * fallback. */
  int hires_ready;
  uint8_t game_bg[MENU_HIRES_PIXELS];
  uint8_t team_bg[MENU_HIRES_PIXELS];
  uint8_t bar_sel[MENU_BAR_W * MENU_BAR_H];
  uint8_t fifa_logo[MENU_LOGO_MAX];
  uint8_t vs[MENU_VS_FRAMES][MENU_VS_SIZE * MENU_VS_SIZE];
  uint8_t hires_canvas[MENU_HIRES_PIXELS];
  uint8_t title_layer[1024 * 80];
  uint8_t hires_pal_game[768];
  uint8_t hires_pal_team[768];
  uint8_t hires_pal_flags[768];
  fifa96_font title_font;
  fifa96_font menu_font;
  uint8_t *title_font_data;
  size_t title_font_len;
  uint8_t *menu_font_data;
  size_t menu_font_len;
  uint32_t vs_frame;
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
  menu_art.hires_ready = 0;
  free(menu_art.title_font_data);
  free(menu_art.menu_font_data);
  menu_art.title_font_data = NULL;
  menu_art.menu_font_data = NULL;
  menu_art.title_font_len = 0;
  menu_art.menu_font_len = 0;
}

/* Load one frame (index) from a POG/SHPI bank into an owned pixel buffer. The
 * palette is taken from entry 0 (the shared bank palette) when requested. */
static int menu_hires_frame(struct fifa96_asset_table *assets, const char *name,
                            uint32_t index, uint8_t *out, int out_w, int out_h,
                            uint8_t palette[768]) {
  const char *path = menu_find_asset(assets, name);
  if (path == NULL || out == NULL) return 0;
  uint8_t *bytes = NULL;
  size_t len = 0;
  if (fifa96_asset_read(assets, path, &bytes, &len) != FIFA96_OK || bytes == NULL)
    return 0;
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
  int ok = 0;
  if (fifa96_sprite_bank_parse(src, src_len, &bank) == FIFA96_OK &&
      fifa96_sprite_bank_entry(&bank, index, &entry) == FIFA96_OK &&
      fifa96_sprite_frame_parse(&bank, entry.offset, &frame) == FIFA96_OK &&
      frame.width == (uint32_t)out_w && frame.height == (uint32_t)out_h &&
      frame.pixel_len >= (uint32_t)(out_w * out_h) && frame.pixels != NULL) {
    memcpy(out, frame.pixels, (size_t)out_w * (size_t)out_h);
    ok = 1;
    if (palette != NULL) {
      fifa96_sprite_chunk chunk;
      const uint8_t *rgb6 = NULL;
      uint16_t count = 0;
      int have_palette = 0;
      memset(palette, 0, 768);
      /* The frame's own chunk carries the palette for flag/VS/logo banks; the
       * background and bar frames share entry 0's palette. */
      if (fifa96_sprite_chunk_parse(&bank, entry.offset, &chunk) == FIFA96_OK &&
          fifa96_sprite_chunk_palette(&chunk, &rgb6, &count) == FIFA96_OK &&
          count <= 256)
        have_palette = 1;
      if (!have_palette) {
        fifa96_sprite_entry pal_entry;
        if (fifa96_sprite_bank_entry(&bank, 0, &pal_entry) == FIFA96_OK &&
            fifa96_sprite_chunk_parse(&bank, pal_entry.offset, &chunk) == FIFA96_OK &&
            fifa96_sprite_chunk_palette(&chunk, &rgb6, &count) == FIFA96_OK &&
            count <= 256)
          have_palette = 1;
      }
      if (have_palette)
        fifa96_sprite_palette_to_rgb(rgb6, count, palette);
    }
  }
  free(decoded);
  fifa96_asset_free(bytes);
  return ok;
}

static int menu_hires_font(struct fifa96_asset_table *assets, const char *name,
                           fifa96_font *font, uint8_t **owned, size_t *owned_len) {
  const char *path = menu_find_asset(assets, name);
  if (path == NULL) return 0;
  uint8_t *bytes = NULL;
  size_t len = 0;
  if (fifa96_asset_read(assets, path, &bytes, &len) != FIFA96_OK || bytes == NULL)
    return 0;
  uint8_t *decoded = NULL;
  size_t decoded_len = 0;
  if (menu_decode_record(bytes, len, &decoded, &decoded_len) != 0) {
    fifa96_asset_free(bytes);
    return 0;
  }
  fifa96_asset_free(bytes);
  const uint8_t *src = decoded != NULL ? decoded : NULL;
  size_t src_len = decoded_len;
  if (src == NULL) return 0;
  if (fifa96_font_parse(src, src_len, font) != 0) {
    free(decoded);
    return 0;
  }
  free(*owned);
  *owned = decoded;
  *owned_len = src_len;
  return 1;
}

/* Remap an indexed frame from its own bank palette onto the target screen
 * palette (nearest RGB). The transparent key is left untouched so the draw
 * blit still sees it. */
static void menu_remap_indexed(uint8_t *pixels, size_t count, const uint8_t *src,
                               const uint8_t *dst, uint8_t key) {
  if (pixels == NULL || src == NULL || dst == NULL) return;
  for (size_t p = 0; p < count; p++) {
    uint8_t idx = pixels[p];
    if (idx == key) continue;
    int32_t sr = src[(size_t)idx * 3 + 0];
    int32_t sg = src[(size_t)idx * 3 + 1];
    int32_t sb = src[(size_t)idx * 3 + 2];
    uint32_t best = 0;
    uint32_t best_d = UINT32_MAX;
    for (uint32_t i = 0; i < 256u; i++) {
      int32_t dr = sr - dst[i * 3 + 0];
      int32_t dg = sg - dst[i * 3 + 1];
      int32_t db = sb - dst[i * 3 + 2];
      uint32_t d = (uint32_t)(dr * dr + dg * dg + db * db);
      if (d < best_d) { best_d = d; best = i; }
    }
    pixels[p] = (uint8_t)best;
  }
}

/* The windowed shell assets. MOD2 is the Game Select composition (background +
 * selected-row bar), MOD1 the Team Select background/FIFA logo, VS the animated
 * lower-left logo (13 frames), and SLICK64/SLICK18 the native title/menu fonts.
 * A missing optional piece is simply not drawn. */
static void menu_hires_load(struct fifa96_asset_table *assets) {
  menu_art.hires_ready = 0;
  if (!menu_hires_frame(assets, "MOD2.POG", 0, menu_art.game_bg, MENU_HIRES_W,
                        MENU_HIRES_H, menu_art.hires_pal_game))
    return;
  if (!menu_hires_frame(assets, "MOD2.POG", 1, menu_art.bar_sel, MENU_BAR_W,
                        MENU_BAR_H, NULL))
    return;
  if (!menu_hires_frame(assets, "MOD1.POG", 0, menu_art.team_bg, MENU_HIRES_W,
                        MENU_HIRES_H, menu_art.hires_pal_team))
    return;
  {
    /* MOD4A carries the flags/league art on its own palette; the Team Select
     * screen uses the MOD1 palette, so remap the FIFA mark once at load. */
    uint8_t flags_pal[768];
    uint8_t logo_pal[768];
    (void)menu_hires_frame(assets, "MOD4A.POG", 0, menu_art.hires_canvas,
                           MENU_HIRES_W, MENU_HIRES_H, flags_pal);
    memcpy(menu_art.hires_pal_flags, flags_pal, sizeof flags_pal);
    if (menu_hires_frame(assets, "MOD4A.POG", 1, menu_art.fifa_logo, MENU_LOGO_W,
                         MENU_LOGO_H, logo_pal))
      menu_remap_indexed(menu_art.fifa_logo, MENU_LOGO_W * MENU_LOGO_H, logo_pal,
                         menu_art.hires_pal_team, 255);
  }
  {
    uint8_t vs_pal[768];
    for (int i = 0; i < MENU_VS_FRAMES; i++) {
      if (!menu_hires_frame(assets, "VS.POG", (uint32_t)i, menu_art.vs[i],
                            MENU_VS_SIZE, MENU_VS_SIZE, i == 0 ? vs_pal : NULL))
        break;
      menu_remap_indexed(menu_art.vs[i], MENU_VS_SIZE * MENU_VS_SIZE, vs_pal,
                         menu_art.hires_pal_game, 255);
    }
  }
  if (!menu_hires_font(assets, "SLICK64.POG", &menu_art.title_font,
                       &menu_art.title_font_data, &menu_art.title_font_len))
    return;
  if (!menu_hires_font(assets, "SLICK18.POG", &menu_art.menu_font,
                       &menu_art.menu_font_data, &menu_art.menu_font_len))
    return;
  menu_art.hires_ready = 1;
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
  menu_hires_load(assets_or_null);
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

/* ===== Native 640x480 shell ============================================ */

/* Native Game Select geometry, measured from MOD2 frame 0 (640x480). The
 * bars are part of the background; the text/selection bar are composed here. */
#define HIRES_GS_LEFT_X 64
#define HIRES_GS_RIGHT_X 344
#define HIRES_GS_TOP 172
#define HIRES_GS_PITCH 52
#define HIRES_GS_ROW_H 34
#define HIRES_GS_LEFT_W 222
#define HIRES_GS_RIGHT_W 240
#define HIRES_TEAM_LEFT_X 110
#define HIRES_TEAM_RIGHT_X 370
#define HIRES_TEAM_TOP 300
#define HIRES_TEAM_PITCH 18
#define HIRES_TEAM_W 180
#define HIRES_TEAM_ROW_H 18

static void hires_fill(int x, int y, int w, int h, uint8_t color) {
  uint8_t *c = menu_art.hires_canvas;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > MENU_HIRES_W) w = MENU_HIRES_W - x;
  if (y + h > MENU_HIRES_H) h = MENU_HIRES_H - y;
  if (w <= 0 || h <= 0) return;
  for (int r = 0; r < h; r++)
    memset(c + (size_t)(y + r) * MENU_HIRES_W + (size_t)x, color, (size_t)w);
}

/* Nearest-neighbour blit with a transparent palette index, clipped to the
 * 640x480 canvas. Used for the native bar/logo/VS sprites. */
static void hires_blit(const uint8_t *src, int sw, int sh, int x, int y,
                       int dw, int dh, uint8_t transparent) {
  uint8_t *c = menu_art.hires_canvas;
  if (src == NULL || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
  for (int dy = 0; dy < dh; dy++) {
    int sy = dy * sh / dh;
    int ty = y + dy;
    if (ty < 0 || ty >= MENU_HIRES_H) continue;
    for (int dx = 0; dx < dw; dx++) {
      int sx = dx * sw / dw;
      int tx = x + dx;
      if (tx < 0 || tx >= MENU_HIRES_W) continue;
      uint8_t p = src[(size_t)sy * (size_t)sw + (size_t)sx];
      if (p == transparent) continue;
      c[(size_t)ty * MENU_HIRES_W + (size_t)tx] = p;
    }
  }
}

static void hires_text(const fifa96_font *font, const char *str, int x, int y,
                       uint8_t color) {
  if (font == NULL || str == NULL) return;
  (void)fifa96_font_blit(menu_art.hires_canvas, MENU_HIRES_W, MENU_HIRES_H, font,
                         str, x, y, color);
}

static void hires_text_shadow(const fifa96_font *font, const char *str, int x, int y,
                              uint8_t color) {
  hires_text(font, str, x + 2, y + 2, 0);
  hires_text(font, str, x, y, color);
}

/* Scaled mask blit for the big title: the SLICK64 string is rendered into a
 * sentinel-filled layer at native size, then sampled down to `dw`/`dh`. This
 * keeps the native letterforms while letting "GAME SELECT" fit 640 px. */
static void hires_mask_blit(const uint8_t *src, int stride, int sw, int sh, int x,
                            int y, int dw, int dh, uint8_t sentinel, uint8_t color) {
  if (src == NULL || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
  for (int dy = 0; dy < dh; dy++) {
    int sy = dy * sh / dh;
    int ty = y + dy;
    if (ty < 0 || ty >= MENU_HIRES_H) continue;
    for (int dx = 0; dx < dw; dx++) {
      int sx = dx * sw / dw;
      int tx = x + dx;
      if (tx < 0 || tx >= MENU_HIRES_W) continue;
      if (src[(size_t)sy * (size_t)stride + (size_t)sx] == sentinel) continue;
      menu_art.hires_canvas[(size_t)ty * MENU_HIRES_W + (size_t)tx] = color;
    }
  }
}

static void hires_title(const char *str, int x, int y, int max_w, uint8_t color) {
  int w = fifa96_font_text_width(&menu_art.title_font, str);
  if (w <= 0) return;
  if (w > 1024) w = 1024;
  memset(menu_art.title_layer, 0xFF, sizeof menu_art.title_layer);
  (void)fifa96_font_blit(menu_art.title_layer, 1024, 80, &menu_art.title_font,
                         str, 0, 0, 1);
  int dw = w;
  int dh = menu_art.title_font.default_height;
  if (dw > max_w) {
    dh = dh * max_w / dw;
    dw = max_w;
  }
  hires_mask_blit(menu_art.title_layer, 1024, w,
                  menu_art.title_font.default_height, x + 3, y + 3, dw, dh, 0xFF, 0);
  hires_mask_blit(menu_art.title_layer, 1024, w,
                  menu_art.title_font.default_height, x, y, dw, dh, 0xFF, color);
}

/* Downsample the 640x480 composition to the engine surface. Indexed pixels
 * cannot be averaged directly; sample the block's centre. The windowed shell
 * is composed at 640 and this path keeps the 320 tape surface correct. */
static void hires_present(struct fifa96_surface *s, const uint8_t *palette) {
  if (s == NULL || palette == NULL) return;
  fifa96_surface_set_palette8(s, palette);
  if (s->width == MENU_HIRES_W && s->height == MENU_HIRES_H) {
    memcpy(s->indexed, menu_art.hires_canvas, MENU_HIRES_PIXELS);
    return;
  }
  for (int y = 0; y < s->height; y++) {
    int sy = (y * MENU_HIRES_H + MENU_HIRES_H / 2) / s->height;
    if (sy >= MENU_HIRES_H) sy = MENU_HIRES_H - 1;
    const uint8_t *src = menu_art.hires_canvas + (size_t)sy * MENU_HIRES_W;
    for (int x = 0; x < s->width; x++) {
      int sx = (x * MENU_HIRES_W + MENU_HIRES_W / 2) / s->width;
      if (sx >= MENU_HIRES_W) sx = MENU_HIRES_W - 1;
      s->indexed[(size_t)y * (size_t)s->width + (size_t)x] = src[sx];
    }
  }
}

static void hires_game_select(const struct fifa96_menu_state *st) {
  memcpy(menu_art.hires_canvas, menu_art.game_bg, MENU_HIRES_PIXELS);
  hires_title("GAME SELECT", 30, 26, 590, 6);
  for (int i = 0; i < st->item_count && i < FIFA96_MENU_ITEM_COUNT; i++) {
    const char *label = st->items[i].label;
    if (label == NULL) continue;
    int col = st->items[i].col;
    int row = st->items[i].row;
    int x = col ? HIRES_GS_RIGHT_X : HIRES_GS_LEFT_X;
    int w = col ? HIRES_GS_RIGHT_W : HIRES_GS_LEFT_W;
    int y = HIRES_GS_TOP + row * HIRES_GS_PITCH;
    int ty = y + (HIRES_GS_ROW_H - menu_art.menu_font.default_height) / 2;
    if (i == st->selected_item) {
      /* The native selection bar (MOD2 frame 1) is wider than the row and
       * carries its own yellow border; centre it on the row. */
      hires_blit(menu_art.bar_sel, MENU_BAR_W, MENU_BAR_H, x - (MENU_BAR_W - w) / 2,
                 y - (MENU_BAR_H - HIRES_GS_ROW_H) / 2, MENU_BAR_W, MENU_BAR_H, 255);
      hires_text(&menu_art.menu_font, label, x + 18, ty, 25);
    } else {
      hires_text_shadow(&menu_art.menu_font, label, x + 18, ty, 1);
    }
  }
  /* The animated VS mark at the lower-left (13 native frames). */
  if (menu_art.vs_frame < UINT32_MAX) menu_art.vs_frame++;
  {
    uint32_t frame = (menu_art.vs_frame / 4u) % MENU_VS_FRAMES;
    hires_blit(menu_art.vs[frame], MENU_VS_SIZE, MENU_VS_SIZE, 16, 386,
               MENU_VS_SIZE, MENU_VS_SIZE, 255);
  }
}

static void hires_team_select(const struct fifa96_menu_state *st) {
  memcpy(menu_art.hires_canvas, menu_art.team_bg, MENU_HIRES_PIXELS);
  hires_title("FRIENDLY", 30, 18, 400, 6);
  /* Vertical LEAGUE SELECT strip (native 46x163 art is a text column; draw the
   * letters stacked with the menu font). */
  for (int i = 0; i < 13; i++) {
    char ch[2] = { 0, 0 };
    ch[0] = "LEAGUE SELECT"[i];
    hires_text_shadow(&menu_art.menu_font, ch, 22, 150 + i * 18, 6);
  }
  /* FIFA globe mark above each team column. */
  hires_blit(menu_art.fifa_logo, MENU_LOGO_W, MENU_LOGO_H, 150, 150, 120, 114, 255);
  hires_blit(menu_art.fifa_logo, MENU_LOGO_W, MENU_LOGO_H, 410, 150, 120, 114, 255);
  hires_text_shadow(&menu_art.menu_font, st->column_label[0] ? st->column_label[0] : "Home",
                    170, 124, 1);
  hires_text_shadow(&menu_art.menu_font, st->column_label[1] ? st->column_label[1] : "Visitor",
                    430, 124, 1);
  for (int side = 0; side < 2; side++) {
    int x = side == 0 ? HIRES_TEAM_LEFT_X : HIRES_TEAM_RIGHT_X;
    for (int r = 0; r < st->team_rows; r++) {
      const char *name = st->teams[side][r];
      if (name == NULL) continue;
      int y = HIRES_TEAM_TOP + r * HIRES_TEAM_PITCH;
      if (r == st->team_selected_row[side]) {
        hires_fill(x - 8, y - 2, HIRES_TEAM_W, HIRES_TEAM_ROW_H, 0);
        hires_fill(x - 8, y - 2, HIRES_TEAM_W, 1, 6);
        hires_fill(x - 8, y + HIRES_TEAM_ROW_H - 3, HIRES_TEAM_W, 1, 6);
        hires_text(&menu_art.menu_font, name, x + 4, y, 25);
      } else {
        hires_text_shadow(&menu_art.menu_font, name, x + 4, y, 1);
      }
    }
  }
  if (st->footer != NULL)
    hires_text_shadow(&menu_art.menu_font, st->footer, 180, 452, 1);
}

/* Native Options/Controls composition over the MOD1 shell. Rows use the same
 * 640x480 window as the native table (x 132, width 444, top 184, pitch 32). */
static void hires_options(const struct fifa96_menu_state *st) {
  memcpy(menu_art.hires_canvas, menu_art.team_bg, MENU_HIRES_PIXELS);
  if (st->option_page != FIFA96_MENU_OPTION_PAGE_LIST) {
    hires_title("CONTROLS", 30, 16, 400, 6);
    int x = 132;
    int y = 78;
    int pitch = 25;
    for (int i = 0; i < st->help_count && i < FIFA96_MENU_OPTION_HELP_MAX; i++) {
      if (st->help[i] == NULL) continue;
      if (i == 0)
        hires_text(&menu_art.menu_font, st->help[i], x, y + i * pitch, 25);
      else
        hires_text_shadow(&menu_art.menu_font, st->help[i], x, y + i * pitch, 1);
    }
    if (st->footer != NULL)
      hires_text_shadow(&menu_art.menu_font, st->footer, 170, 452, 1);
    return;
  }
  hires_title("OPTIONS", 30, 16, 400, 6);
  const int x = 132;
  const int w = 444;
  const int top = 184;
  const int pitch = 32;
  const int row_h = 28;
  for (int k = 0; k < FIFA96_MENU_OPTION_VISIBLE; k++) {
    int row = st->option_scroll + k;
    if (row >= st->option_count) break;
    int y = top + k * pitch;
    int selected = row == st->option_selected;
    if (selected) {
      hires_fill(x, y, w, row_h, 0);
      hires_fill(x, y, w, 2, 6);
      hires_fill(x, y + row_h - 2, w, 2, 6);
    }
    if (st->options[k].label != NULL) {
      if (selected)
        hires_text(&menu_art.menu_font, st->options[k].label, x + 12, y + 5, 25);
      else
        hires_text_shadow(&menu_art.menu_font, st->options[k].label, x + 12, y + 5, 1);
    }
    if (st->options[k].value != NULL && st->options[k].value[0] != '\0') {
      int vw = fifa96_font_text_width(&menu_art.menu_font, st->options[k].value);
      int vx = x + w - 12 - vw;
      if (vx < x + 12) vx = x + 12;
      if (selected)
        hires_text(&menu_art.menu_font, st->options[k].value, vx, y + 5, 25);
      else
        hires_text_shadow(&menu_art.menu_font, st->options[k].value, vx, y + 5, 1);
    }
  }
  if (st->option_count > FIFA96_MENU_OPTION_VISIBLE) {
    int track_x = x + w + 8;
    int track_h = FIFA96_MENU_OPTION_VISIBLE * pitch - (pitch - row_h);
    int thumb_h = track_h * FIFA96_MENU_OPTION_VISIBLE / st->option_count;
    if (thumb_h < 16) thumb_h = 16;
    int span = st->option_count - FIFA96_MENU_OPTION_VISIBLE;
    int thumb_y = top;
    if (span > 0) thumb_y += (track_h - thumb_h) * st->option_scroll / span;
    hires_fill(track_x, top, 4, track_h, 9);
    hires_fill(track_x, thumb_y, 4, thumb_h, 1);
  }
  if (st->footer != NULL)
    hires_text_shadow(&menu_art.menu_font, st->footer, 170, 452, 1);
}

/* Returns 1 when the native hires shell drew the frame. The legacy library
 * path keeps the old composition. */
static int menu_art_draw_hires(struct fifa96_surface *s,
                               const struct fifa96_menu_state *st) {
  if (!menu_art.hires_ready || s == NULL || st == NULL) return 0;
  if (st->screen == FIFA96_MENU_SCREEN_GAME_SELECT) {
    hires_game_select(st);
    hires_present(s, menu_art.hires_pal_game);
    return 1;
  }
  if (st->screen == FIFA96_MENU_SCREEN_TEAM_SELECT) {
    hires_team_select(st);
    hires_present(s, menu_art.hires_pal_team);
    return 1;
  }
  if (st->screen == FIFA96_MENU_SCREEN_OPTIONS) {
    hires_options(st);
    hires_present(s, menu_art.hires_pal_team);
    return 1;
  }
  return 0;
}

void fifa96_menu_art_draw(struct fifa96_surface *s, const struct fifa96_menu_state *st) {
  if (s == NULL || st == NULL) return;

  if (menu_art_draw_hires(s, st)) return;

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
