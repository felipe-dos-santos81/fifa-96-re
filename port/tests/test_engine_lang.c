/* port/tests/test_engine_lang.c — P2 LANG000/HELV12 decode + font smoke.
 *
 * The synthetic LANGO fixture pins the A3 §1.2 layout (`"LANGO"`, base at +8,
 * u32 offsets relative to base) and the `FUN_0001771C(id)` lookup; the
 * synthetic FNTI pins the reused `fifa96_font_parse`/`fifa96_font_blit` path.
 * The retail case is ISO-gated: it decodes `/FEDATA/LANG000.POG` and
 * `/FEGFX/HELV12.POG` through `fifa96_record_decode` and checks the known
 * label/team ids, so an ISO-less run skips cleanly. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_lang.h"

#define LANG_TEST_ISO_PATH "data/game/FIFAPCCD96.iso"
#define LANG_TEST_COUNT 0x150u

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void put16le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static const char *fixture_string(uint32_t id) {
  switch (id) {
    case 0x000u: return "NOTRANS";
    case 0x023u: return "Brazil";
    case 0x081u: return "Friendly";
    case 0x084u: return "Game Select";
    case 0x098u: return "Home";
    case 0x0A6u: return "Italy";
    case 0x0D8u: return "Options";
    case 0x0F3u: return "Quit to DOS";
    case 0x13Du: return "Team Select";
    case 0x14Au: return "Visitor";
    default: return "";
  }
}

/* Decoded LANGO image: 256-entry offset table (empty strings included). */
static size_t build_lango(uint8_t *buf, size_t cap) {
  const size_t base = 0x10 + LANG_TEST_COUNT * 4u;
  size_t pos = base;
  uint8_t *offsets = calloc(LANG_TEST_COUNT, 4);
  assert(offsets != NULL);
  for (uint32_t i = 0; i < LANG_TEST_COUNT; i++) {
    const char *s = fixture_string(i);
    size_t len = strlen(s) + 1u;
    assert(pos + len <= cap);
    memcpy(buf + pos, s, len);
    put32le(offsets + (size_t)i * 4u, (uint32_t)(pos - base));
    pos += len;
  }
  memset(buf, 0, 0x10);
  memcpy(buf, "LANGO", 5);
  buf[5] = 2;
  put32le(buf + 8, (uint32_t)base);
  memcpy(buf + 0x10, offsets, LANG_TEST_COUNT * 4u);
  free(offsets);
  return pos;
}

/* Synthetic ASCII FNTI: ' '..'~' with default 4x6 glyphs, 4bpp bitmap block
 * at 0x1A0. The shared zero glyph descriptor makes every char draw the same
 * block, which is enough for the render smoke. */
static size_t build_font(uint8_t *buf, size_t cap) {
  const size_t block = 0x20u + 96u * 4u;
  const size_t pixels = block + 0x10u;
  const size_t bitmap_len = 16u * 8u;
  const size_t total = pixels + bitmap_len;
  assert(total <= cap);
  memset(buf, 0, total);
  memcpy(buf, "FNTI", 4);
  buf[4] = 32;      /* first ' ' */
  buf[5] = 127;     /* last '~' */
  buf[6] = 4;       /* default width */
  buf[7] = 6;       /* default height */
  buf[8] = 1;       /* advance = base + width */
  put32le(buf + 0x1C, (uint32_t)block);
  buf[block] = 0x7A;              /* 4bpp */
  put16le(buf + block + 4, 32);   /* bitmap width */
  put16le(buf + block + 6, 8);    /* bitmap rows */
  memset(buf + pixels, 0xFF, bitmap_len);
  return total;
}

static void test_memory_lookup_and_font(void) {
  uint8_t lang[4096];
  uint8_t font[1024];
  size_t lang_len = build_lango(lang, sizeof lang);
  size_t font_len = build_font(font, sizeof font);

  struct fifa96_lang la;
  memset(&la, 0, sizeof la);
  assert(fifa96_lang_init_memory(&la, lang, lang_len, font, font_len) == 0);
  assert(fifa96_lang_ready(&la) == 1);
  assert(strcmp(fifa96_lang_string(&la, 0x81u), "Friendly") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0xD8u), "Options") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0xF3u), "Quit to DOS") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0x23u), "Brazil") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0x13Du), "Team Select") == 0);
  assert(fifa96_lang_string(&la, LANG_TEST_COUNT) == NULL);
  assert(fifa96_lang_string(NULL, 0x81u) == NULL);

  const fifa96_font *f = fifa96_lang_font(&la);
  assert(f != NULL);
  assert(f->first == 32 && f->last == 127 && f->glyph_count == 96);
  assert(f->default_width == 4 && f->default_height == 6);
  assert(f->depth == 4 && f->bitmap_width == 32 && f->bitmap_height == 8);
  assert(fifa96_font_text_width(f, "Friendly") == 40);

  uint8_t canvas[320 * 240];
  memset(canvas, 0, sizeof canvas);
  assert(fifa96_font_blit(canvas, 320, 240, f, "Friendly", 10, 10, 5) == FIFA96_OK);
  size_t painted = 0;
  for (size_t i = 0; i < sizeof canvas; i++)
    if (canvas[i] == 5) painted++;
  assert(painted > 0);

  fifa96_lang_shutdown(&la);
  assert(fifa96_lang_ready(&la) == 0);
  assert(fifa96_lang_string(&la, 0x81u) == NULL);
  assert(fifa96_lang_font(&la) == NULL);
  fifa96_lang_shutdown(NULL);   /* tolerant */
}

static void test_malformed_tables(void) {
  uint8_t lang[4096];
  uint8_t font[1024];
  size_t lang_len = build_lango(lang, sizeof lang);
  size_t font_len = build_font(font, sizeof font);
  struct fifa96_lang la;
  memset(&la, 0, sizeof la);

  uint8_t bad[4096];
  assert(lang_len <= sizeof bad);
  memcpy(bad, lang, lang_len);
  bad[0] = 'X';
  assert(fifa96_lang_init_memory(&la, bad, lang_len, font, font_len) != 0);
  assert(fifa96_lang_ready(&la) == 0);

  memcpy(bad, lang, lang_len);
  put32le(bad + 8, 8);   /* base below the header */
  assert(fifa96_lang_init_memory(&la, bad, lang_len, font, font_len) != 0);

  memcpy(bad, lang, lang_len);
  put32le(bad + 8, (uint32_t)lang_len + 8u);   /* base outside the image */
  assert(fifa96_lang_init_memory(&la, bad, lang_len, font, font_len) != 0);

  memcpy(bad, lang, lang_len);
  put32le(bad + 0x10, 0x7FFFFu);   /* string offset outside the image */
  assert(fifa96_lang_init_memory(&la, bad, lang_len, font, font_len) != 0);

  /* A valid table with no font still resolves strings. */
  assert(fifa96_lang_init_memory(&la, lang, lang_len, NULL, 0) == 0);
  assert(fifa96_lang_ready(&la) == 1);
  assert(fifa96_lang_font(&la) == NULL);
  assert(strcmp(fifa96_lang_string(&la, 0x81u), "Friendly") == 0);
  fifa96_lang_shutdown(&la);

  assert(fifa96_lang_init_memory(NULL, lang, lang_len, font, font_len) != 0);
  assert(fifa96_lang_init(NULL, NULL) != 0);
}

static int file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static void test_retail_assets(void) {
  if (!file_exists(LANG_TEST_ISO_PATH)) {
    puts("SKIP test_engine_lang retail assets (no ISO)");
    return;
  }
  struct fifa96_asset_table *assets = NULL;
  assert(fifa96_asset_mount_file(LANG_TEST_ISO_PATH, &assets) == FIFA96_OK);
  struct fifa96_lang la;
  memset(&la, 0, sizeof la);
  assert(fifa96_lang_init(&la, assets) == 0);
  assert(fifa96_lang_ready(&la) == 1);
  assert(strcmp(fifa96_lang_string(&la, 0x81u), "Friendly") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0xD8u), "Options") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0xF3u), "Quit to DOS") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0x84u), "Game Select") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0x13Du), "Team Select") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0x98u), "Home") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0x14Au), "Visitor") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0x23u), "Brazil") == 0);
  assert(strcmp(fifa96_lang_string(&la, 0xA6u), "Italy") == 0);

  const fifa96_font *f = fifa96_lang_font(&la);
  assert(f != NULL);
  assert(f->first == 32 && f->last == 127 && f->glyph_count == 96);
  assert(f->default_width == 12 && f->default_height == 13);
  assert(f->depth == 4 && f->bitmap_width == 256 && f->bitmap_height == 31);
  uint8_t *canvas = calloc(320u * 240u, 1);
  assert(canvas != NULL);
  assert(fifa96_font_blit(canvas, 320, 240, f, "Friendly", 10, 10, 5) == FIFA96_OK);
  size_t painted = 0;
  for (size_t i = 0; i < 320u * 240u; i++)
    if (canvas[i] == 5) painted++;
  assert(painted > 50);
  free(canvas);

  fifa96_lang_shutdown(&la);
  fifa96_asset_unmount(assets);
}

int main(void) {
  test_memory_lookup_and_font();
  test_malformed_tables();
  test_retail_assets();
  puts("test_engine_lang OK");
  return 0;
}
