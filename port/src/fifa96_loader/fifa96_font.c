#include "fifa96_loader/fifa96_font.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"

/* OL-T11-7 / FU-148 §1.5: the FNTI font images the match HUD draws with.
 *
 * The native chain is FUN_000984B4 (header -> text-renderer state),
 * FUN_0009FFF0 (glyph blit) and FUN_00098630/FUN_000986B4 (advance-sum text
 * width). FU-148 derived the field layout first-hand; the 4bpp glyph decode
 * was verified against the retail clockfnt.fsh/playfnt.fsh bitmaps (the
 * block's own `0x7A` mode byte is the FUN_000AFBFC depth the native uses; the
 * structural test decodes the real '0' raster). */

/* FUN_000AFBFC: mode byte -> bits/pixel (0x79 = 1, 0x7A = 4, 0x7B = 8; the
 * other cases are the >8-bit modes whose glyph path returns 0). */
static int font_depth_from_mode(uint8_t mode) {
  switch (mode & 0x7Fu) {
    case 0x79: return 1;
    case 0x7A: return 4;
    case 0x7B: return 8;
    default: return 0;
  }
}

static uint32_t font_bswap32(uint32_t v) {
  return (v >> 24) | ((v & 0x00FF0000u) >> 8) | ((v & 0x0000FF00u) << 8) | (v << 24);
}

/* FUN_000984B4's FNTI-family predicate: the byte-swapped magic is
 * "FNTI"/"FNTM"/"FNTP". The fourth accepted native variant (a LE "MTNF"
 * magic) takes the other parser arm with a different depth source
 * (`arg[3] - '0'`) and none is known in the retail containers, so the port
 * reports it BAD_MAGIC rather than misparsing it. */
static int font_magic_ok(uint32_t dword) {
  uint32_t swapped = font_bswap32(dword);
  return swapped == 0x464E5449u || swapped == 0x464E544Du || swapped == 0x464E5450u;
}

int fifa96_font_parse(const uint8_t *data, size_t len, fifa96_font *out) {
  if (!data || !out) return -FIFA96_ERR_INVALID;
  if (len < 0x20) return -FIFA96_ERR_TRUNCATED;
  if (!font_magic_ok(fifa96_read_u32le(data))) return -FIFA96_ERR_BAD_MAGIC;

  uint16_t count = 0;
  if (data[5] >= data[4]) count = (uint16_t)((unsigned)data[5] - data[4] + 1u);

  uint32_t widths_off = fifa96_read_u32le(data + 0x10) >> 16;
  uint32_t heights_off = fifa96_read_u32le(data + 0x14) & 0xFFFFu;
  uint32_t advances_off = fifa96_read_u32le(data + 0x14) >> 16;
  uint32_t x_offsets_off = fifa96_read_u32le(data + 0x18) & 0xFFFFu;
  uint32_t y_offsets_off = fifa96_read_u32le(data + 0x18) >> 16;
  uint32_t bitmap_off = fifa96_read_u32le(data + 0x1C);

  /* The per-char descriptor table starts at +0x20 and must hold every glyph
   * the range names (the native indexes it unconditionally). */
  if ((size_t)0x20 + (size_t)count * 4u > len) return -FIFA96_ERR_TRUNCATED;
  /* Every present table must hold `count` bytes. */
  uint32_t tables[5] = {widths_off, heights_off, advances_off, x_offsets_off, y_offsets_off};
  for (int i = 0; i < 5; i++) {
    if (tables[i] == 0) continue;
    if ((size_t)tables[i] + count > len) return -FIFA96_ERR_TRUNCATED;
  }
  /* Bitmap block: {mode, ., ., ., w:u16, h:u16} then the pixel area at +0x10
   * (the native glyph base is DAT_00112810 + 0x10 where DAT_00112810 =
   * data + +0x1C). */
  if (bitmap_off > len || len - bitmap_off < 0x10) return -FIFA96_ERR_TRUNCATED;
  const uint8_t *block = data + bitmap_off;
  int depth = font_depth_from_mode(block[0]);
  if (depth != 1 && depth != 4 && depth != 8) return -FIFA96_ERR_UNSUPPORTED;
  uint32_t bitmap_width = fifa96_read_u16le(block + 4);
  uint32_t bitmap_height = fifa96_read_u16le(block + 6);
  if (bitmap_width == 0 || bitmap_height == 0) return -FIFA96_ERR_UNSUPPORTED;
  size_t stride = ((size_t)bitmap_width * (size_t)depth + 7u) >> 3;
  size_t bitmap_len = stride * bitmap_height;
  if (bitmap_len > len - bitmap_off - 0x10u) return -FIFA96_ERR_TRUNCATED;

  out->data = data;
  out->len = (uint32_t)len;
  out->first = data[4];
  out->last = data[5];
  out->glyph_count = count;
  out->default_width = data[6];
  out->default_height = data[7];
  out->advance_base = data[8];
  out->widths = widths_off ? data + widths_off : NULL;
  out->heights = heights_off ? data + heights_off : NULL;
  out->advances = advances_off ? (const int8_t *)(const void *)(data + advances_off) : NULL;
  out->x_offsets = x_offsets_off ? (const int8_t *)(const void *)(data + x_offsets_off) : NULL;
  out->y_offsets = y_offsets_off ? (const int8_t *)(const void *)(data + y_offsets_off) : NULL;
  out->glyphs = data + 0x20;
  out->bitmap = block + 0x10;
  out->bitmap_len = (uint32_t)bitmap_len;
  out->bitmap_width = (uint16_t)bitmap_width;
  out->bitmap_height = (uint16_t)bitmap_height;
  out->depth = (uint8_t)depth;
  return FIFA96_OK;
}

/* FUN_00098630: the advance sum; a char outside [first,last] draws nothing
 * and adds nothing. */
int fifa96_font_text_width(const fifa96_font *font, const char *str) {
  if (!font || !font->data || !str) return 0;
  int total = 0;
  for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
    unsigned idx = (unsigned)(*p) - font->first;
    if (idx >= font->glyph_count) continue;
    unsigned glyph_w = font->widths ? font->widths[idx] : font->default_width;
    total += font->advances ? (int)font->advances[idx]
                            : (int)font->advance_base + (int)glyph_w;
  }
  return total;
}

int fifa96_font_blit(uint8_t *canvas, int canvas_w, int canvas_h, const fifa96_font *font,
                     const char *str, int x, int y, uint8_t color) {
  if (!canvas || canvas_w <= 0 || canvas_h <= 0 || !font || !font->data || !str)
    return -FIFA96_ERR_INVALID;
  size_t stride = ((size_t)font->bitmap_width * (size_t)font->depth + 7u) >> 3;
  unsigned mask = (1u << font->depth) - 1u;
  int cursor_x = x;
  int cursor_y = y;
  for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
    unsigned idx = (unsigned)(*p) - font->first;
    if (idx >= font->glyph_count) continue;
    unsigned glyph_w = font->widths ? font->widths[idx] : font->default_width;
    unsigned glyph_h = font->heights ? font->heights[idx] : font->default_height;
    int advance = font->advances ? (int)font->advances[idx]
                                 : (int)font->advance_base + (int)glyph_w;
    int off_x = font->x_offsets ? (int)font->x_offsets[idx] : 0;
    int off_y = font->y_offsets ? (int)font->y_offsets[idx] : 0;
    uint32_t desc = fifa96_read_u32le(font->glyphs + (size_t)idx * 4u);
    unsigned bit = desc & 0xFFFFu;
    unsigned row = desc >> 16;
    int glyph_x = cursor_x + off_x;
    int glyph_y = cursor_y + off_y;
    for (unsigned gy = 0; gy < glyph_h; gy++) {
      size_t row_off = ((size_t)row + gy) * stride;
      if (row_off > font->bitmap_len) break;
      const uint8_t *row_src = font->bitmap + row_off;
      int py = glyph_y + (int)gy;
      for (unsigned gx = 0; gx < glyph_w; gx++) {
        size_t bit_pos = (size_t)bit + gx;
        size_t byte_off = (bit_pos * font->depth) >> 3;
        if (byte_off >= font->bitmap_len - row_off) break;
        unsigned shift = 8u - (unsigned)font->depth - (unsigned)((bit_pos * font->depth) & 7u);
        unsigned pixel = (unsigned)(row_src[byte_off] >> shift) & mask;
        if (pixel == 0) continue;
        int px = glyph_x + (int)gx;
        if (px < 0 || px >= canvas_w || py < 0 || py >= canvas_h) continue;
        canvas[(size_t)py * (size_t)canvas_w + (size_t)px] = color;
      }
    }
    cursor_x += advance;
  }
  return FIFA96_OK;
}

int fifa96_font_blit_outlined(uint8_t *canvas, int canvas_w, int canvas_h,
                              const fifa96_font *font, const char *str, int x, int y) {
  if (!canvas || canvas_w <= 0 || canvas_h <= 0 || !font || !font->data || !str)
    return -FIFA96_ERR_INVALID;
  (void)fifa96_font_blit(canvas, canvas_w, canvas_h, font, str, x + 1, y + 1, 6);
  return fifa96_font_blit(canvas, canvas_w, canvas_h, font, str, x, y, 0);
}

int fifa96_font_draw_centered(uint8_t *canvas, int canvas_w, int canvas_h,
                              const fifa96_font *font, const char *str, int y,
                              int scale, int wide) {
  int width, half, x;
  if (!canvas || canvas_w <= 0 || canvas_h <= 0 || !font || !font->data || !str)
    return -FIFA96_ERR_INVALID;
  width = fifa96_font_text_width(font, str);
  /* FUN_00054640: narrow halves the measure, wide quarters it. */
  half = wide ? (width >> 2) : (width >> 1);
  x = (wide ? 0x140 : 0xA0) - (int)(((int64_t)half * scale + 0x8000) >> 16);
  return fifa96_font_blit_outlined(canvas, canvas_w, canvas_h, font, str, x, y);
}
