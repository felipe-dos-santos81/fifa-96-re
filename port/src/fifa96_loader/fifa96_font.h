#pragma once
#include <stddef.h>
#include <stdint.h>

/* FIFA96 FNTI bitmap font images (OL-T11-7 / FU-148 §1.5).
 *
 * First-hand chain on /FIFA96.EXE:
 *  - FUN_000984B4 parses the header into the text-renderer state: magic dword
 *    (the FNTI/FNTM/FNTP byte-swapped family; the LE "MTNF" variant takes a
 *    different native parser arm and is rejected), first/last char codes
 *    +4/+5, default width/height +6/+7, a base advance byte +8, three dwords
 *    at +0x10/+0x14/+0x18 whose high/low words are relative table offsets
 *    (or 0 = absent) and +0x1C = the bitmap block offset;
 *  - FUN_0009FFF0 blits a string: per char (index = ch - first) the width,
 *    height, signed advance, signed x/y offset come from those tables (or the
 *    defaults), the per-char u32 at +0x20 carries {low16 = bit column,
 *    high16 = bitmap row}, and the glyph nibbles are read from the shared
 *    bitmap at +0x1C + 0x10 with stride `(bitmap_width*depth + 7) >> 3`,
 *    row-major over the char's w x h box; a zero nibble is transparent. The
 *    depth comes from the bitmap block: the native pushes the block pointer
 *    into FUN_000AFBFC (`0xA00A2 PUSH ESI` with ESI = block base), whose
 *    mode-byte table yields 1/4/8 bpp — the retail fonts' block says
 *    0x7A = 4bpp (verified against the real glyph rasters);
 *  - FUN_00098630/FUN_000986B4 sum the per-char advances for the text width
 *    (chars outside [first,last] contribute nothing and draw nothing).
 *
 * Legs (FU-148 §7): the native 4bpp plot path maps each nibble through the
 * runtime-filled `0x15BBAC` shade table (`FUN_00019E9C`); the port draws a
 * flat colour index for every non-zero nibble (documented divergence). */

typedef struct fifa96_font {
  const uint8_t *data;          /* borrowed FNTI image */
  uint32_t len;
  uint8_t first;                /* +4 first char code */
  uint8_t last;                 /* +5 last char code */
  uint16_t glyph_count;         /* last - first + 1 */
  uint8_t default_width;        /* +6 */
  uint8_t default_height;       /* +7 */
  uint8_t advance_base;         /* +8, added to the width when no advance table */
  const uint8_t *widths;        /* glyph_count entries, or NULL */
  const uint8_t *heights;       /* glyph_count entries, or NULL */
  const int8_t *advances;       /* signed, or NULL */
  const int8_t *x_offsets;      /* signed, or NULL */
  const int8_t *y_offsets;      /* signed, or NULL */
  const uint8_t *glyphs;        /* glyph_count u32 {bit column, row} at +0x20 */
  const uint8_t *bitmap;        /* shared bitmap pixels (block + 0x10) */
  uint32_t bitmap_len;          /* validated pixel-area byte length */
  uint16_t bitmap_width;        /* block word +4, pixels */
  uint16_t bitmap_height;       /* block word +6, rows */
  uint8_t depth;                /* bits/pixel from the block's mode byte */
} fifa96_font;

/* Parse an FNTI font image. Returns FIFA96_ERR_OK, -FIFA96_ERR_INVALID
 * (NULL args), -FIFA96_ERR_TRUNCATED (short image or a table/bitmap out of
 * range), -FIFA96_ERR_BAD_MAGIC (not the FNTI family) or
 * -FIFA96_ERR_UNSUPPORTED (non-4/8/1-bit block depth). *out is written only
 * on success. */
int fifa96_font_parse(const uint8_t *data, size_t len, fifa96_font *out);

/* Sum of the per-char advances (FUN_00098630); 0 for NULL inputs and for
 * chars outside [first,last]. */
int fifa96_font_text_width(const fifa96_font *font, const char *str);

/* Draw `str` at (x, y) with the flat colour index `color` into a
 * `canvas_w` x `canvas_h` 8-bit canvas: per char, pixels start at
 * (x + x_offset, y + y_offset) and the cursor advances by the char's advance;
 * non-zero glyph nibbles write `color`, zero nibbles are transparent, and
 * everything outside the canvas is clipped. Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID (NULL font/canvas/str, non-positive canvas size). */
int fifa96_font_blit(uint8_t *canvas, int canvas_w, int canvas_h, const fifa96_font *font,
                     const char *str, int x, int y, uint8_t color);

/* FU-152 §4.3 (P4): FUN_000544B4's two-pass text — colour 6 at (+1,+1) then
 * colour 0 at (x,y) (the native outline/main pair; the shade ramp stays
 * FU-148's recorded leg). */
int fifa96_font_blit_outlined(uint8_t *canvas, int canvas_w, int canvas_h,
                              const fifa96_font *font, const char *str, int x, int y);

/* FU-152 §4.3 (P4): FUN_00054640's centred draw. The centre is 0xA0 (narrow)
 * or 0x140 (wide); the offset is the halved (narrow: `w>>1`) or quartered
 * (wide: `w>>2`) measure scaled `(v*scale + 0x8000) >> 16`. Draws the
 * outline/main pair at (centre - offset, y). Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID. */
int fifa96_font_draw_centered(uint8_t *canvas, int canvas_w, int canvas_h,
                              const fifa96_font *font, const char *str, int y,
                              int scale, int wide);
