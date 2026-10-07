#include <string.h>
#include "fifa96_engine/fifa96_menu_art.h"
#include "fifa96_loader/fifa96_sprite.h"

#define MENU_UI_W 0x280
#define MENU_UI_H 0x1E0
#define MENU_UI_X 0x85
#define MENU_UI_Y 0xB8
#define MENU_UI_STEP 0x21
#define MENU_UI_ROW_W 0x1BC

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

static const char *menu_find_options_inv(const struct fifa96_asset_table *t) {
  static const char name[] = "OPTIONS.INV";
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

int fifa96_menu_art_init(struct fifa96_asset_table *assets_or_null) {
  menu_art.background_ready = 0;
  menu_art.palette_ready = 0;
  if (assets_or_null == NULL || assets_or_null->entries == NULL ||
      assets_or_null->count == 0) {
    return 0;
  }
  const char *path = menu_find_options_inv(assets_or_null);
  if (path == NULL) return 0;

  uint8_t *bytes = NULL;
  size_t len = 0;
  if (fifa96_asset_read(assets_or_null, path, &bytes, &len) != FIFA96_OK ||
      bytes == NULL || len == 0) {
    return 0;
  }

  fifa96_sprite_bank bank;
  fifa96_sprite_entry entry;
  fifa96_sprite_frame frame;
  if (fifa96_sprite_bank_parse(bytes, len, &bank) == FIFA96_OK &&
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
  fifa96_asset_free(bytes);
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

void fifa96_menu_art_draw(struct fifa96_surface *s, const struct fifa96_menu_state *st) {
  if (s == NULL || st == NULL) return;

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
