/* tests/test_font.c — OL-T11-7 (M2 full-gameplay P0.2): the FNTI font images
 * the match HUD draws with (`clockfnt.fsh` slot 0x35 / `playfnt.fsh` slot
 * 0x36, GAMEART0 entries 53/54).
 *
 * FU chain under test (docs/ghidra/FU148_presentation_hud_camera.md §1.5):
 *  - FUN_000984B4 header parse: magic +4 first/+5 last/+6 default width/+7
 *    default height/+8 advance base, +0x10 high word = width-table offset,
 *    +0x14 low/high = height/advance table offsets, +0x18 low/high =
 *    x/y-offset table offsets, +0x1C = bitmap block offset;
 *  - FUN_0009FFF0 blit: per-char u32 {bit column, row} at +0x20, 4bpp glyph
 *    nibbles from `bitmap + 0x10` with the block's `(width*depth+7)>>3` row
 *    stride, zero transparent;
 *  - FUN_00098630 text width = advance sum, chars outside [first,last]
 *    skipped without an advance.
 *
 * First case: a synthetic FNTI image with the derived geometry, so every
 * assertion is independent of the retail container. Second case: the retail
 * `clockfnt.fsh`/`playfnt.fsh` extracted from the checked-in
 * `tests/golden/gameart0.pvi` (the same file FU-148 used), pinning the real
 * header values and the '0' glyph raster decoded first-hand (FU-148 §1.5
 * provenance). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_font.h"
#include "fifa96_loader/fifa96_record.h"

static void put32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

/* Synthetic FNTI image, chars '0'..'2' (3 glyphs):
 *   widths  {2,1,2} heights {2,2,2} advances {3,2,3} xoff {0,-1,0} yoff {0,1,0}
 *   bitmap block at 0x40: mode 0x7A (4bpp), 16x2 pixels, 8 bytes per row;
 *   '0' bit column 0 (both nibbles of byte 0), '1' bit column 4 (high nibble
 *   of byte 2), '2' bit column 8 (both nibbles of byte 4), both rows solid. */
#define SYN_FONT_LEN 0x60
static void syn_font_init(uint8_t *f) {
  memset(f, 0, SYN_FONT_LEN);
  memcpy(f, "FNTI", 4);
  f[4] = 0x30;   /* first '0' */
  f[5] = 0x32;   /* last '2' */
  f[6] = 2;
  f[7] = 2;
  f[8] = 0;
  put32(f + 0x10, 0x002C0000u);          /* width table at 0x2C */
  put32(f + 0x14, (0x33u << 16) | 0x30); /* heights 0x30, advances 0x33 */
  put32(f + 0x18, (0x39u << 16) | 0x36); /* xoff 0x36, yoff 0x39 */
  put32(f + 0x1C, 0x40u);                /* bitmap block at 0x40 */
  put32(f + 0x20 + 0, 0x00000000u);      /* '0': bit 0, row 0 */
  put32(f + 0x20 + 4, 0x00000004u);      /* '1': bit 4, row 0 */
  put32(f + 0x20 + 8, 0x00000008u);      /* '2': bit 8, row 0 */
  f[0x2C] = 2; f[0x2D] = 1; f[0x2E] = 2;             /* widths */
  f[0x30] = 2; f[0x31] = 2; f[0x32] = 2;             /* heights */
  f[0x33] = 3; f[0x34] = 2; f[0x35] = 3;             /* advances */
  f[0x36] = 0; f[0x37] = (uint8_t)-1; f[0x38] = 0;   /* x offsets */
  f[0x39] = 0; f[0x3A] = 1; f[0x3B] = 0;             /* y offsets */
  f[0x40] = 0x7A;                                    /* block mode: 4bpp */
  f[0x44] = 16; f[0x46] = 2;                         /* 16x2 bitmap */
  f[0x50] = 0xFF; f[0x52] = 0xF0; f[0x54] = 0xFF;    /* row 0 pixels */
  f[0x58] = 0xFF; f[0x5A] = 0xF0; f[0x5C] = 0xFF;    /* row 1 pixels */
}

static void test_synthetic_parse_and_metrics(void) {
  uint8_t data[SYN_FONT_LEN];
  syn_font_init(data);
  fifa96_font font;
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == FIFA96_OK);
  assert(font.data == data && font.len == SYN_FONT_LEN);
  assert(font.first == 0x30 && font.last == 0x32 && font.glyph_count == 3);
  assert(font.default_width == 2 && font.default_height == 2);
  assert(font.widths[0] == 2 && font.widths[1] == 1 && font.widths[2] == 2);
  assert(font.heights[0] == 2 && font.heights[1] == 2 && font.heights[2] == 2);
  assert(font.advances[0] == 3 && font.advances[1] == 2 && font.advances[2] == 3);
  assert(font.x_offsets[1] == -1 && font.y_offsets[1] == 1);
  assert(font.bitmap_width == 16 && font.bitmap_height == 2 && font.depth == 4);
  assert(font.bitmap == data + 0x50);

  /* FUN_00098630: the advance sum; an out-of-range char is skipped. */
  assert(fifa96_font_text_width(&font, "012") == 8);
  assert(fifa96_font_text_width(&font, "0a0") == 6);
  assert(fifa96_font_text_width(&font, "") == 0);
  assert(fifa96_font_text_width(&font, "9") == 0);
  assert(fifa96_font_text_width(NULL, "0") == 0);
  assert(fifa96_font_text_width(&font, NULL) == 0);
}

static void test_synthetic_blit(void) {
  uint8_t data[SYN_FONT_LEN];
  syn_font_init(data);
  fifa96_font font;
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == FIFA96_OK);

  uint8_t canvas[16 * 8];
  memset(canvas, 0, sizeof canvas);
  assert(fifa96_font_blit(canvas, 16, 8, &font, "012", 0, 0, 5) == FIFA96_OK);
  /* '0' at the cursor: both rows of its 2x2 box. */
  assert(canvas[0 * 16 + 0] == 5 && canvas[0 * 16 + 1] == 5);
  assert(canvas[1 * 16 + 0] == 5 && canvas[1 * 16 + 1] == 5);
  /* '1': advance 3 -> cursor 3, x offset -1 -> x = 2, y offset 1. */
  assert(canvas[1 * 16 + 2] == 5 && canvas[2 * 16 + 2] == 5);
  assert(canvas[0 * 16 + 2] == 0);
  /* '2': cursor 3 + 2 = 5, both rows of its 2x2 box. */
  assert(canvas[0 * 16 + 5] == 5 && canvas[0 * 16 + 6] == 5);
  assert(canvas[1 * 16 + 5] == 5 && canvas[1 * 16 + 6] == 5);
  /* Zero nibbles and past-the-box cells stay untouched. */
  assert(canvas[0 * 16 + 3] == 0 && canvas[0 * 16 + 4] == 0);
  assert(canvas[0 * 16 + 7] == 0 && canvas[7 * 16 + 15] == 0);

  /* Out-of-range chars draw nothing and add no advance ('0a0' = two '0's
   * at cursor 0 and 3). */
  memset(canvas, 0, sizeof canvas);
  assert(fifa96_font_blit(canvas, 16, 8, &font, "0a0", 0, 0, 7) == FIFA96_OK);
  assert(canvas[0] == 7 && canvas[1] == 7);
  assert(canvas[3] == 7 && canvas[4] == 7);

  /* Clipping: a negative origin drops the off-canvas pixels only. */
  memset(canvas, 0, sizeof canvas);
  assert(fifa96_font_blit(canvas, 16, 8, &font, "012", -1, -2, 9) == FIFA96_OK);
  assert(canvas[0 * 16 + 1] == 9);   /* '1' second row at y=0 */
  assert(canvas[0 * 16 + 0] == 0);
  assert(canvas[7 * 16 + 7] == 0);

  /* Contract errors. */
  assert(fifa96_font_blit(NULL, 16, 8, &font, "0", 0, 0, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_font_blit(canvas, 16, 8, NULL, "0", 0, 0, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_font_blit(canvas, 16, 8, &font, NULL, 0, 0, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_font_blit(canvas, 0, 8, &font, "0", 0, 0, 1) == -FIFA96_ERR_INVALID);
}

/* Without the optional tables the defaults apply: width/height defaults and
 * advance = advance_base + width (FUN_0009FFF0's DAT_00112804 == 0 arm). */
static void test_defaults_when_tables_absent(void) {
  uint8_t data[SYN_FONT_LEN];
  syn_font_init(data);
  put32(data + 0x10, 0);
  put32(data + 0x14, 0);
  put32(data + 0x18, 0);
  data[6] = 2;
  data[7] = 1;
  data[8] = 1;
  fifa96_font font;
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == FIFA96_OK);
  assert(font.widths == NULL && font.heights == NULL && font.advances == NULL);
  assert(font.x_offsets == NULL && font.y_offsets == NULL);
  /* width = default 2, advance = base 1 + width 2 = 3 per glyph. */
  assert(fifa96_font_text_width(&font, "012") == 9);
  uint8_t canvas[16 * 4];
  memset(canvas, 0, sizeof canvas);
  assert(fifa96_font_blit(canvas, 16, 4, &font, "012", 0, 0, 3) == FIFA96_OK);
  assert(canvas[0] == 3 && canvas[1] == 3);       /* '0' default 2x1 */
  /* '1' at cursor 3 with default width 2 reads bitmap pixels 4 and 5; only
   * pixel 4 is set (0xF0). */
  assert(canvas[3] == 3 && canvas[4] == 0);
  assert(canvas[6] == 3 && canvas[7] == 3);       /* '2' at cursor 6 */
  assert(canvas[2] == 0);
}

static void test_parse_errors(void) {
  uint8_t data[SYN_FONT_LEN];
  fifa96_font font;
  syn_font_init(data);

  assert(fifa96_font_parse(NULL, SYN_FONT_LEN, &font) == -FIFA96_ERR_INVALID);
  assert(fifa96_font_parse(data, SYN_FONT_LEN, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_font_parse(data, 8, &font) == -FIFA96_ERR_TRUNCATED);

  syn_font_init(data);
  data[0] = 'X';
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == -FIFA96_ERR_BAD_MAGIC);

  /* A width-table offset past the image is truncated. */
  syn_font_init(data);
  put32(data + 0x10, 0x00FF0000u);
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == -FIFA96_ERR_TRUNCATED);

  /* A glyph table that overruns the image is truncated. */
  syn_font_init(data);
  data[5] = 0x40;   /* 17 glyphs > the 3-entry table at 0x20..0x2C */
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == -FIFA96_ERR_TRUNCATED);

  /* A bitmap block that runs past the image is truncated. */
  syn_font_init(data);
  put32(data + 0x1C, 0x58u);   /* 0x58 + 0x10 + 2*8 > 0x60 */
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == -FIFA96_ERR_TRUNCATED);

  /* A block mode the port does not decode is unsupported. */
  syn_font_init(data);
  data[0x40] = 0x01;
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == -FIFA96_ERR_UNSUPPORTED);
}

/* ---- retail fonts from the checked-in GAMEART0 container ----------------- */

#define FONT_GOLDEN_PATH "tests/golden/gameart0.pvi"

struct retail_container {
  uint8_t *container;
  size_t len;
};

/* Decode the 0x10FB envelope (the same container form fifa96_match_run_stage
 * consumes) and hand back the BIGF bytes. */
static int retail_open(struct retail_container *out) {
  FILE *f = fopen(FONT_GOLDEN_PATH, "rb");
  if (!f) return 0;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
  long n = ftell(f);
  if (n < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
  uint8_t *file = malloc((size_t)n);
  if (!file) { fclose(f); return 0; }
  if (fread(file, 1, (size_t)n, f) != (size_t)n) { free(file); fclose(f); return 0; }
  fclose(f);
  if ((size_t)n < 5 || file[1] != 0xFB) { free(file); return 0; }
  size_t declared = ((size_t)file[2] << 16) | ((size_t)file[3] << 8) | file[4];
  uint8_t *container = malloc(declared);
  if (!container) { free(file); return 0; }
  size_t got = 0;
  if (fifa96_record_decode(file, (size_t)n, container, declared, &got) != 0) {
    free(file);
    free(container);
    return 0;
  }
  free(file);
  out->container = container;
  out->len = got;
  return 1;
}

static const uint8_t *retail_entry(struct retail_container *c, const char *name) {
  struct fifa96_bigf_info info;
  if (fifa96_bigf_parse(c->container, c->len, &info) != FIFA96_OK) return NULL;
  for (size_t i = 0; i < info.count; i++) {
    uint32_t off = 0, size = 0;
    const char *entry_name = NULL;
    if (fifa96_bigf_record(&info, i, &off, &size, &entry_name) != FIFA96_OK) return NULL;
    if (entry_name && strcmp(entry_name, name) == 0) return c->container + off;
  }
  return NULL;
}

static void test_retail_fonts(void) {
  struct retail_container c;
  if (!retail_open(&c)) {
    fprintf(stderr, "SKIP retail font cases (no %s)\n", FONT_GOLDEN_PATH);
    return;
  }
  const uint8_t *clock = retail_entry(&c, "clockfnt.fsh");
  const uint8_t *play = retail_entry(&c, "playfnt.fsh");
  assert(clock != NULL && play != NULL);

  fifa96_font clockfont;
  assert(fifa96_font_parse(clock, 6497u, &clockfont) == FIFA96_OK);
  assert(clockfont.first == 0x20 && clockfont.last == 0xFF);
  assert(clockfont.glyph_count == 224);
  assert(clockfont.default_width == 9 && clockfont.default_height == 13);
  assert(clockfont.bitmap_width == 682 && clockfont.bitmap_height == 13);
  assert(clockfont.depth == 4);
  /* First-hand FU-148 §1.5 run: '0' = width 5, height 8, advance 6, offsets
   * (1, 2) at bit column 72 row 0. */
  assert(clockfont.widths[0x30 - 0x20] == 5);
  assert(clockfont.heights[0x30 - 0x20] == 8);
  assert(clockfont.advances[0x30 - 0x20] == 6);
  assert(clockfont.x_offsets[0x30 - 0x20] == 1);
  assert(clockfont.y_offsets[0x30 - 0x20] == 2);
  /* The '0' raster (4bpp nibbles; zero transparent): the first-hand decode
   * has row 0 blank, rows 1/2 full, rows 3-5 with a one-column centre gap,
   * rows 6/7 full -> 5+5+4+4+4+5+5 = 32 set pixels. */
  uint8_t canvas[16 * 16];
  memset(canvas, 0, sizeof canvas);
  assert(fifa96_font_blit(canvas, 16, 16, &clockfont, "0", 0, 0, 7) == FIFA96_OK);
  int set = 0;
  for (int i = 0; i < 16 * 16; i++) set += canvas[i] != 0;
  assert(set == 32);
  assert(canvas[(2 + 1) * 16 + 1 + 0] == 7);   /* row 1, col 0: set */
  assert(canvas[(2 + 1) * 16 + 1 + 4] == 7);   /* row 1, col 4: set */
  assert(canvas[(2 + 3) * 16 + 1 + 2] == 0);   /* row 3, col 2: gap */
  assert(canvas[(2 + 7) * 16 + 1 + 4] == 7);   /* row 7, col 4: set */
  assert(fifa96_read_u32le(clockfont.glyphs + (0x30 - 0x20) * 4) == 0x48u);

  fifa96_font playfont;
  assert(fifa96_font_parse(play, 3352u, &playfont) == FIFA96_OK);
  assert(playfont.first == 0x20 && playfont.last == 0xFF);
  assert(playfont.default_width == 5 && playfont.default_height == 7);
  assert(playfont.bitmap_width == 368 && playfont.bitmap_height == 7);
  assert(playfont.depth == 4);
  /* "00:00" is the HUD's clock string: advance sum over the retail table. */
  assert(fifa96_font_text_width(&clockfont, "00:00") == 28);
  assert(fifa96_font_text_width(&playfont, "00:00") == 18);

  free(c.container);
}

/* FU-152 §4.3 (P4): the FUN_00054640 centred-draw helper and the FUN_000544B4
 * two-pass (colour 6 outline then colour 0) text. The centre is 0xA0 on the
 * 320-axis and 0x140 on the wide (settings-4) axis, minus the halved (or
 * quartered when wide) measured width scaled `(v*scale + 0x8000) >> 16`.
 * FUN_00054640 first-hand: narrow `0xA0 - ((w>>1)*scale+0x8000)>>16`, wide
 * `0x140 - ((w>>2)*scale+0x8000)>>16`. */
static void test_outlined_and_centered(void) {
  uint8_t data[SYN_FONT_LEN];
  syn_font_init(data);
  fifa96_font font;
  assert(fifa96_font_parse(data, SYN_FONT_LEN, &font) == FIFA96_OK);

  /* Two-pass: the outline colour 6 at (+1,+1), the main colour 0 at (x,y). */
  uint8_t canvas[16 * 8];
  memset(canvas, 0, sizeof canvas);
  assert(fifa96_font_blit_outlined(canvas, 16, 8, &font, "0", 2, 3) == FIFA96_OK);
  assert(canvas[3 * 16 + 2] == 0 && canvas[3 * 16 + 3] == 0);
  assert(canvas[4 * 16 + 2] == 0 && canvas[4 * 16 + 3] == 0);   /* main box */
  assert(canvas[4 * 16 + 4] == 6);   /* outline of the second column */
  assert(canvas[5 * 16 + 3] == 6 && canvas[5 * 16 + 4] == 6);   /* outline row */

  /* "012" width 8. Narrow scale 1.0: four -> x = 0xA0 - 4 = 156; '0' at
   * (156,0) main and (157,1) outline. */
  uint8_t canvas320[320 * 8];
  memset(canvas320, 0, sizeof canvas320);
  assert(fifa96_font_draw_centered(canvas320, 320, 8, &font, "012", 0, 0x10000, 0) ==
         FIFA96_OK);
  assert(canvas320[0 * 320 + 156] == 0);
  assert(canvas320[1 * 320 + 156] == 0);
  assert(canvas320[2 * 320 + 157] == 6);   /* '0' outline below the main box */

  /* The wide flag centres on 0x140 with the quartered width: w>>2 = 2 ->
   * x = 0x140 - 2 = 318. */
  memset(canvas320, 0, sizeof canvas320);
  assert(fifa96_font_draw_centered(canvas320, 320, 8, &font, "012", 0, 0x10000, 1) ==
         FIFA96_OK);
  assert(canvas320[0 * 320 + 318] == 0);
  assert(canvas320[0 * 320 + 319] == 0);

  /* Zoomed scale 0x8000: (4*0x8000+0x8000)>>16 = 2 -> x = 0xA0 - 2 = 158. */
  memset(canvas320, 0, sizeof canvas320);
  assert(fifa96_font_draw_centered(canvas320, 320, 8, &font, "012", 0, 0x8000, 0) ==
         FIFA96_OK);
  assert(canvas320[0 * 320 + 158] == 0);
  assert(canvas320[0 * 320 + 159] == 0);

  assert(fifa96_font_blit_outlined(NULL, 16, 8, &font, "0", 0, 0) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_font_draw_centered(NULL, 16, 8, &font, "0", 0, 0x10000, 0) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_font_draw_centered(canvas, 16, 8, NULL, "0", 0, 0x10000, 0) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_font_draw_centered(canvas, 16, 8, &font, NULL, 0, 0x10000, 0) ==
         -FIFA96_ERR_INVALID);
}

int main(void) {
  test_synthetic_parse_and_metrics();
  test_synthetic_blit();
  test_defaults_when_tables_absent();
  test_parse_errors();
  test_outlined_and_centered();
  test_retail_fonts();
  puts("test_font OK");
  return 0;
}
