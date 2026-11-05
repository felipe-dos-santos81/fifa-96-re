#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_font.h"

struct fifa96_asset_table;

/* P2 (A3 §1.2): the native front-end text resources.
 *
 * `/FEDATA/LANG000.POG` is a RefPack record (`10 FB 00 27 54`) whose decoded
 * image is the `LANGO` string table: `"LANGO"` at +0, a u32 `base` at +8 and a
 * u32 offset table at +0x10 (`count = (base - 0x10) / 4` = 591 entries, each
 * offset relative to `base`) of NUL-terminated strings. `FUN_0001771C(id)` is
 * the native lookup (the runtime-filled pointer table `0x1472F0`).
 *
 * `/FEGFX/HELV12.POG` is a RefPack record (`10 FB 00 13 10`) whose decoded
 * image is FNTI; `fifa96_font_parse` reads it here. Both decodes reuse the
 * ported record/refpack and font modules.
 *
 * A module instance owns the decoded images (the `fifa96_font` pointers are
 * into the owned font copy), so no global state crosses engine instances. */
struct fifa96_lang {
  uint8_t *lang_owner;       /* owned decoded LANGO image */
  size_t lang_len;
  uint32_t base;             /* string-base offset within lang_owner */
  uint32_t count;            /* offset-table entries */
  uint8_t *font_owner;       /* owned decoded FNTI image */
  size_t font_len;
  fifa96_font font;
  int font_ready;
  int ready;                 /* LANGO validated */
};

/* Decode LANG000.POG + HELV12.POG from `assets` by basename (case-insensitive)
 * and initialise `la`. Missing/unreadable assets, a bad record or a malformed
 * LANGO table soft-fails: `la` is left empty and the front-end stays on its
 * legacy placeholder path. Returns 0 on success, -1 otherwise. */
int fifa96_lang_init(struct fifa96_lang *la, struct fifa96_asset_table *assets);

/* Test/engine seam: initialise from already decoded LANGO/FNTI images. The
 * images are copied (the font keeps pointers into the copy). `font` may be
 * NULL; lookups then work but the front-end has no text renderer. Returns 0 on
 * success, -1 on NULL/invalid input (including any offset that leaves the
 * image or has no NUL terminator). */
int fifa96_lang_init_memory(struct fifa96_lang *la, const uint8_t *lang, size_t lang_len,
                            const uint8_t *font, size_t font_len);

/* Native `FUN_0001771C(id)`: the NUL-terminated string, or NULL for an
 * out-of-range id / an uninitialised module. */
const char *fifa96_lang_string(const struct fifa96_lang *la, uint32_t id);

/* The parsed HELV12 font, or NULL when the font asset was absent/invalid. */
const fifa96_font *fifa96_lang_font(const struct fifa96_lang *la);

int fifa96_lang_ready(const struct fifa96_lang *la);

/* Free the owned images and clear the module (safe on a zeroed instance and
 * on NULL). */
void fifa96_lang_shutdown(struct fifa96_lang *la);
