#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_lang.h"
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_record.h"

#define FIFA96_LANG_DECODE_MAX (64u * 1024u * 1024u)

static int lang_char_fold(int c) {
  return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

static int lang_ci_equal(const char *a, const char *b) {
  while (*a && *b) {
    if (lang_char_fold((unsigned char)*a) != lang_char_fold((unsigned char)*b)) return 0;
    a++;
    b++;
  }
  return *a == 0 && *b == 0;
}

static const char *lang_find_asset(const struct fifa96_asset_table *t, const char *name) {
  if (t == NULL || t->entries == NULL) return NULL;
  for (size_t i = 0; i < t->count; i++) {
    const char *p = t->entries[i].path;
    const char *base = p;
    for (const char *q = p; *q; q++) {
      if (*q == '/' || *q == '\\') base = q + 1;
    }
    if (lang_ci_equal(base, name)) return p;
  }
  return NULL;
}

/* A POG file is a raw record (`selector, 0xFB, BE24 decoded length`). The
 * record decoder rejects already-decoded containers, so pass those through
 * untouched (the unit fixtures use this shape). */
static int lang_decode_record(const uint8_t *src, size_t src_len, uint8_t **out,
                              size_t *out_len) {
  if (src == NULL || out == NULL || out_len == NULL || src_len < 5) return -1;
  *out = NULL;
  *out_len = 0;
  if (src_len >= 4 && memcmp(src, "SHPI", 4) == 0) {
    *out_len = src_len;
    return 0;   /* borrowed by the caller; no allocation */
  }
  if (src[1] != 0xFB) return -1;
  uint32_t declared = ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 8) | (uint32_t)src[4];
  if (declared == 0 || declared > FIFA96_LANG_DECODE_MAX) return -1;
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

void fifa96_lang_shutdown(struct fifa96_lang *la) {
  if (la == NULL) return;
  free(la->lang_owner);
  free(la->font_owner);
  memset(la, 0, sizeof *la);
}

int fifa96_lang_init_memory(struct fifa96_lang *la, const uint8_t *lang, size_t lang_len,
                            const uint8_t *font, size_t font_len) {
  if (la == NULL) return -1;
  fifa96_lang_shutdown(la);
  if (lang == NULL || lang_len < 0x10 || memcmp(lang, "LANGO", 5) != 0) return -1;
  uint32_t base = fifa96_read_u32le(lang + 8);
  if (base < 0x10 || (size_t)base > lang_len || ((base - 0x10u) & 3u) != 0) return -1;
  uint32_t count = (base - 0x10u) / 4u;
  if (count == 0 || count > 0xFFFFu) return -1;
  size_t table_len = (size_t)count * 4u;
  if ((size_t)base - 0x10u != table_len) return -1;

  /* Validate every entry before publishing the image: an id outside the image
   * or a string without a terminator is a malformed table (the native indexes
   * blindly). */
  for (uint32_t i = 0; i < count; i++) {
    uint32_t off = fifa96_read_u32le(lang + 0x10 + (size_t)i * 4u);
    if ((size_t)off >= lang_len - (size_t)base) return -1;
    const uint8_t *s = lang + base + off;
    if (memchr(s, 0, lang_len - (size_t)base - off) == NULL) return -1;
  }

  uint8_t *lang_copy = malloc(lang_len);
  if (lang_copy == NULL) return -1;
  memcpy(lang_copy, lang, lang_len);
  la->lang_owner = lang_copy;
  la->lang_len = lang_len;
  la->base = base;
  la->count = count;
  la->ready = 1;

  if (font != NULL && font_len != 0) {
    uint8_t *font_copy = malloc(font_len);
    if (font_copy != NULL) {
      memcpy(font_copy, font, font_len);
      fifa96_font parsed;
      if (fifa96_font_parse(font_copy, font_len, &parsed) == FIFA96_OK) {
        la->font_owner = font_copy;
        la->font_len = font_len;
        la->font = parsed;
        la->font_ready = 1;
      } else {
        free(font_copy);
      }
    }
  }
  return 0;
}

int fifa96_lang_init(struct fifa96_lang *la, struct fifa96_asset_table *assets) {
  if (la == NULL) return -1;
  fifa96_lang_shutdown(la);
  const char *lang_path = lang_find_asset(assets, "LANG000.POG");
  const char *font_path = lang_find_asset(assets, "HELV12.POG");
  if (lang_path == NULL || font_path == NULL) return -1;

  uint8_t *lang_bytes = NULL;
  size_t lang_bytes_len = 0;
  uint8_t *font_bytes = NULL;
  size_t font_bytes_len = 0;
  if (fifa96_asset_read(assets, lang_path, &lang_bytes, &lang_bytes_len) != FIFA96_OK ||
      lang_bytes == NULL || lang_bytes_len == 0)
    return -1;
  if (fifa96_asset_read(assets, font_path, &font_bytes, &font_bytes_len) != FIFA96_OK ||
      font_bytes == NULL || font_bytes_len == 0) {
    fifa96_asset_free(lang_bytes);
    return -1;
  }

  uint8_t *lang_decoded = NULL;
  size_t lang_decoded_len = 0;
  uint8_t *font_decoded = NULL;
  size_t font_decoded_len = 0;
  const uint8_t *lang_src = lang_bytes;
  const uint8_t *font_src = font_bytes;
  size_t lang_src_len = lang_bytes_len;
  size_t font_src_len = font_bytes_len;
  if (lang_decode_record(lang_bytes, lang_bytes_len, &lang_decoded, &lang_decoded_len) != 0)
    goto fail;
  if (lang_decoded != NULL) {
    lang_src = lang_decoded;
    lang_src_len = lang_decoded_len;
  }
  if (lang_decode_record(font_bytes, font_bytes_len, &font_decoded, &font_decoded_len) != 0)
    goto fail;
  if (font_decoded != NULL) {
    font_src = font_decoded;
    font_src_len = font_decoded_len;
  }

  if (fifa96_lang_init_memory(la, lang_src, lang_src_len, font_src, font_src_len) == 0) {
    fifa96_asset_free(lang_bytes);
    fifa96_asset_free(font_bytes);
    free(lang_decoded);
    free(font_decoded);
    return 0;
  }

fail:
  free(lang_decoded);
  free(font_decoded);
  fifa96_asset_free(lang_bytes);
  fifa96_asset_free(font_bytes);
  fifa96_lang_shutdown(la);
  return -1;
}

const char *fifa96_lang_string(const struct fifa96_lang *la, uint32_t id) {
  if (la == NULL || !la->ready || la->lang_owner == NULL || id >= la->count) return NULL;
  uint32_t off = fifa96_read_u32le(la->lang_owner + 0x10 + (size_t)id * 4u);
  return (const char *)(la->lang_owner + la->base + off);
}

const fifa96_font *fifa96_lang_font(const struct fifa96_lang *la) {
  if (la == NULL || !la->font_ready) return NULL;
  return &la->font;
}

int fifa96_lang_ready(const struct fifa96_lang *la) {
  return la != NULL && la->ready != 0;
}
