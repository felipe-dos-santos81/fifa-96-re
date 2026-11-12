/* port/src/fifa96_engine/fifa96_match_replay_ring.c — the native packer/
 * applier pair (`FUN_0006408C` / `FUN_00063B80`) over the `0x155AA4` view
 * block and the `0x118` record.
 *
 * Evidence: spec `2026-11-12-recorded-replay-ring-design.md` §5.1 — a fresh
 * `disassemble_function 0x6408C` (packer) and `0x63B80` (applier) on
 * `/FIFA96.EXE`, both 23-iteration loops with pre-incremented byte pointers
 * (the `+0xC4` appearance / `+0xDB` frame lanes). The header carries the full
 * record -> view map; the offsets below are that map.
 *
 * The host is little-endian and the native block is a little-endian byte
 * image, so every multi-byte lane goes through `memcpy` (never a cast through
 * an unaligned pointer). */
#include <string.h>

#include "fifa96_engine/fifa96_match_replay_ring.h"

/* The map's fixed lane offsets (header). */
enum {
  FIFA96_REPLAY_FOCUS = 0x00,        /* focus triple: +0x00/+0x04/+0x08 */
  FIFA96_REPLAY_SLOTS = 0x0C,        /* 23 slots x 0x0C bytes (x, y, z) */
  FIFA96_REPLAY_SLOT_STRIDE = 0x0C,
  FIFA96_REPLAY_HEADINGS = 0x120,    /* packed heading dwords, 4 bytes each */
  FIFA96_REPLAY_APPEARANCE = 0x17C,  /* 23 appearance bytes */
  FIFA96_REPLAY_FRAMES = 0x193,      /* 23 frame bytes */
  FIFA96_REPLAY_CAMERA_BYTES = 0x1AA,/* roll / queue pop / band / flag */
  FIFA96_REPLAY_MATRIX = 0x1AE,      /* six camera dwords */
  FIFA96_REPLAY_QUEUE6 = 0x1C6,      /* six bytes */
  FIFA96_REPLAY_WORD_1CC = 0x1CC,    /* `word_55C70` */
  FIFA96_REPLAY_WORD_1CE = 0x1CE,    /* `word_55C72`: the stamp on apply */

  FIFA96_REPLAY_REC_FOCUS = 0x00,
  FIFA96_REPLAY_REC_X = 0x0C,        /* 23 words */
  FIFA96_REPLAY_REC_Z = 0x3A,        /* 23 words */
  FIFA96_REPLAY_REC_Y = 0x68,        /* 23 words */
  FIFA96_REPLAY_REC_HEADINGS = 0x96, /* 23 words */
  FIFA96_REPLAY_REC_APPEARANCE = 0xC4,
  FIFA96_REPLAY_REC_FRAMES = 0xDB,
  FIFA96_REPLAY_REC_CAMERA_BYTES = 0xF2,
  FIFA96_REPLAY_REC_MATRIX = 0xF6,
  FIFA96_REPLAY_REC_QUEUE6 = 0x10E,
  FIFA96_REPLAY_REC_WORD_1CC = 0x114,
  FIFA96_REPLAY_REC_STAMP = 0x116,
};

static uint16_t replay_get_word(const uint8_t *block, size_t off) {
  uint16_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

static uint32_t replay_get_dword(const uint8_t *block, size_t off) {
  uint32_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

static void replay_put_word(uint8_t *block, size_t off, uint16_t value) {
  memcpy(block + off, &value, sizeof value);
}

static void replay_put_dword(uint8_t *block, size_t off, uint32_t value) {
  memcpy(block + off, &value, sizeof value);
}

/* The applier's MOVSX: a native position word becomes a signed dword. */
static int32_t replay_sign_extend(uint16_t word) {
  return (word & 0x8000u) ? (int32_t)word - 0x10000 : (int32_t)word;
}

void fifa96_match_replay_pack(const uint8_t *view, uint8_t *record,
                              uint16_t stamp) {
  size_t i;

  if (!view || !record) return;

  /* The focus triple copies verbatim. */
  for (i = 0; i < 3; i++)
    replay_put_dword(record, FIFA96_REPLAY_REC_FOCUS + 4 * i,
                     replay_get_dword(view, FIFA96_REPLAY_FOCUS + 4 * i));

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = FIFA96_REPLAY_SLOTS + FIFA96_REPLAY_SLOT_STRIDE * i;
    /* The three position arrays are stored as words in the native order
     * x, z, y (`+0x0C`, `+0x3A`, `+0x68`). */
    replay_put_word(record, FIFA96_REPLAY_REC_X + 2 * i,
                    replay_get_word(view, slot + 0x0));
    replay_put_word(record, FIFA96_REPLAY_REC_Z + 2 * i,
                    replay_get_word(view, slot + 0x8));
    replay_put_word(record, FIFA96_REPLAY_REC_Y + 2 * i,
                    replay_get_word(view, slot + 0x4));
    replay_put_word(record, FIFA96_REPLAY_REC_HEADINGS + 2 * i,
                    replay_get_word(view, FIFA96_REPLAY_HEADINGS + 4 * i));
    record[FIFA96_REPLAY_REC_APPEARANCE + i] =
        view[FIFA96_REPLAY_APPEARANCE + i];
    record[FIFA96_REPLAY_REC_FRAMES + i] = view[FIFA96_REPLAY_FRAMES + i];
  }

  memcpy(record + FIFA96_REPLAY_REC_CAMERA_BYTES,
         view + FIFA96_REPLAY_CAMERA_BYTES, 4);
  memcpy(record + FIFA96_REPLAY_REC_MATRIX, view + FIFA96_REPLAY_MATRIX, 0x18);
  memcpy(record + FIFA96_REPLAY_REC_QUEUE6, view + FIFA96_REPLAY_QUEUE6, 6);
  memcpy(record + FIFA96_REPLAY_REC_WORD_1CC, view + FIFA96_REPLAY_WORD_1CC, 2);
  /* The asymmetric lane: the packer stamps the record's `+0x116` word. */
  replay_put_word(record, FIFA96_REPLAY_REC_STAMP, stamp);
}

void fifa96_match_replay_unpack(const uint8_t *record, uint8_t *view) {
  size_t i;

  if (!record || !view) return;

  for (i = 0; i < 3; i++)
    replay_put_dword(view, FIFA96_REPLAY_FOCUS + 4 * i,
                     replay_get_dword(record, FIFA96_REPLAY_REC_FOCUS + 4 * i));

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = FIFA96_REPLAY_SLOTS + FIFA96_REPLAY_SLOT_STRIDE * i;
    /* The applier sign-extends each position word into the slot's dword. */
    replay_put_dword(
        view, slot + 0x0,
        (uint32_t)replay_sign_extend(
            replay_get_word(record, FIFA96_REPLAY_REC_X + 2 * i)));
    replay_put_dword(
        view, slot + 0x4,
        (uint32_t)replay_sign_extend(
            replay_get_word(record, FIFA96_REPLAY_REC_Y + 2 * i)));
    replay_put_dword(
        view, slot + 0x8,
        (uint32_t)replay_sign_extend(
            replay_get_word(record, FIFA96_REPLAY_REC_Z + 2 * i)));
    /* The heading word; the dword's high word is left for the composer. */
    replay_put_word(view, FIFA96_REPLAY_HEADINGS + 4 * i,
                    replay_get_word(record, FIFA96_REPLAY_REC_HEADINGS + 2 * i));
    view[FIFA96_REPLAY_APPEARANCE + i] = record[FIFA96_REPLAY_REC_APPEARANCE + i];
    view[FIFA96_REPLAY_FRAMES + i] = record[FIFA96_REPLAY_REC_FRAMES + i];
  }

  memcpy(view + FIFA96_REPLAY_CAMERA_BYTES,
         record + FIFA96_REPLAY_REC_CAMERA_BYTES, 4);
  memcpy(view + FIFA96_REPLAY_MATRIX, record + FIFA96_REPLAY_REC_MATRIX, 0x18);
  memcpy(view + FIFA96_REPLAY_QUEUE6, record + FIFA96_REPLAY_REC_QUEUE6, 6);
  memcpy(view + FIFA96_REPLAY_WORD_1CC, record + FIFA96_REPLAY_REC_WORD_1CC, 2);
  /* The stamp lands in the view's `+0x1CE` (`word_55C72`), not `+0x1CC`. */
  replay_put_word(view, FIFA96_REPLAY_WORD_1CE,
                  replay_get_word(record, FIFA96_REPLAY_REC_STAMP));
}
