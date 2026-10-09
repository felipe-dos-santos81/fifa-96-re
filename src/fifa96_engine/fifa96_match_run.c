#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_animation.h"
#include "fifa96_loader/fifa96_arm_helpers.h"
#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_record.h"
#include "fifa96_loader/fifa96_scene.h"
#include "fifa96_loader/fifa96_sprite.h"

/* FU-145 S2: the shared phase write (definition in the phase-driver
 * section below; used by begin/reset paths above it). */
static int match_run_write_phase(struct fifa96_match_run *mr, uint8_t phase);

/* FU-70 §1.2 slot tables (flat 0x11064E direction map and the animation chain
 * T1 0x10E1DC -> T2 0x10E1EC / T3 0x10E1F5). */
static const uint8_t match_run_slot_map[16] = {
  0, 5, 10, 0, 6, 4, 2, 0, 9, 1, 8, 0, 0, 0, 0, 0
};
static const uint8_t match_run_anim_a[16] = {
  0, 1, 5, 0, 3, 2, 4, 0, 7, 8, 6, 0, 0, 0, 0, 0
};
static const uint8_t match_run_anim_b[16] = {
  0, 1, 1, 0, 0xFF, 0xFF, 0xFF, 0, 1, 0, 0, 0xFF, 0xFF, 0xFF, 0, 1
};
static const uint8_t match_run_anim_c[16] = {
  0, 0, 0xFF, 0xFF, 0xFF, 0, 1, 1, 1, 0, 0, 0x40, 0, 0, 0, 0
};

/* FU-84 §3.1: the animation row table is static executable data at flat
 * 0x10EF00 (111 rows x 9 bytes; installed into [0x57588] by 0x73D91). Row
 * layout {id, last_frame, flags, next_id, frame_table, sprite_bank}; the
 * frame_table dword is the relocated flat pointer (0x10E2xx..). First-hand
 * /FIFA96.EXE read_memory 0x10EF00: row 0 `00 00 07 00 00 E2 10 00 00`,
 * row 1 `01 0B 07 01 06 E2 10 00 01`, row 0x6E `6E 01 07 00 66 EE 10 00 53`.
 * The render chain consumes only +1 (last frame, the advance wrap) and +8
 * (the FU-85 resolver bank index); the frame tables are the staged FU-84
 * records (Task 2 open leg keeps the executable frame data unported). */
static const uint8_t match_run_anim_rows[FIFA96_ANIM_ROW_COUNT * FIFA96_ANIM_ROW_SIZE] = {
    /* row 0x00 */ 0x00, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x01 */ 0x01, 0x0B, 0x07, 0x01, 0x06, 0xE2, 0x10, 0x00, 0x01,
    /* row 0x02 */ 0x02, 0x0B, 0x07, 0x02, 0x42, 0xE2, 0x10, 0x00, 0x02,
    /* row 0x03 */ 0x03, 0x0B, 0x07, 0x03, 0x7E, 0xE2, 0x10, 0x00, 0x03,
    /* row 0x04 */ 0x04, 0x05, 0x00, 0x00, 0xBA, 0xE2, 0x10, 0x00, 0x04,
    /* row 0x05 */ 0x05, 0x05, 0x02, 0x00, 0xD8, 0xE2, 0x10, 0x00, 0x05,
    /* row 0x06 */ 0x06, 0x03, 0x00, 0x00, 0xF6, 0xE2, 0x10, 0x00, 0x06,
    /* row 0x07 */ 0x07, 0x04, 0x00, 0x00, 0x0A, 0xE3, 0x10, 0x00, 0x06,
    /* row 0x08 */ 0x08, 0x06, 0x00, 0x00, 0x3E, 0xE3, 0x10, 0x00, 0x07,
    /* row 0x09 */ 0x09, 0x04, 0x00, 0x00, 0x62, 0xE3, 0x10, 0x00, 0x08,
    /* row 0x0A */ 0x0A, 0x04, 0x00, 0x00, 0x7C, 0xE3, 0x10, 0x00, 0x09,
    /* row 0x0B */ 0x0B, 0x03, 0x00, 0x00, 0x96, 0xE3, 0x10, 0x00, 0x0A,
    /* row 0x0C */ 0x0C, 0x05, 0x00, 0x00, 0xAA, 0xE3, 0x10, 0x00, 0x0B,
    /* row 0x0D */ 0x0D, 0x05, 0x00, 0x0E, 0xC8, 0xE3, 0x10, 0x00, 0x0C,
    /* row 0x0E */ 0x0E, 0x08, 0x00, 0x0E, 0x04, 0xE4, 0x10, 0x00, 0x0D,
    /* row 0x0F */ 0x0F, 0x04, 0x00, 0x00, 0x32, 0xE4, 0x10, 0x00, 0x0E,
    /* row 0x10 */ 0x10, 0x06, 0x00, 0x00, 0x4C, 0xE4, 0x10, 0x00, 0x0F,
    /* row 0x11 */ 0x11, 0x04, 0x00, 0x00, 0x70, 0xE4, 0x10, 0x00, 0x10,
    /* row 0x12 */ 0x12, 0x05, 0x00, 0x0F, 0x8A, 0xE4, 0x10, 0x00, 0x11,
    /* row 0x13 */ 0x13, 0x03, 0x02, 0x00, 0xA8, 0xE4, 0x10, 0x00, 0x12,
    /* row 0x14 */ 0x14, 0x06, 0x00, 0x7F, 0xBC, 0xE4, 0x10, 0x00, 0x13,
    /* row 0x15 */ 0x15, 0x05, 0x06, 0x15, 0xE0, 0xE4, 0x10, 0x00, 0x14,
    /* row 0x16 */ 0x16, 0x04, 0x00, 0x00, 0xFE, 0xE4, 0x10, 0x00, 0x15,
    /* row 0x17 */ 0x17, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x18 */ 0x18, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x19 */ 0x19, 0x06, 0x00, 0x02, 0x54, 0xE5, 0x10, 0x00, 0x16,
    /* row 0x1A */ 0x1A, 0x08, 0x00, 0x00, 0x92, 0xE5, 0x10, 0x00, 0x17,
    /* row 0x1B */ 0x1B, 0x04, 0x00, 0x00, 0x78, 0xE5, 0x10, 0x00, 0x18,
    /* row 0x1C */ 0x1C, 0x03, 0x04, 0x1C, 0xC0, 0xE5, 0x10, 0x00, 0x1A,
    /* row 0x1D */ 0x1D, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x1E */ 0x1E, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x1F */ 0x1F, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x20 */ 0x20, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x21 */ 0x21, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x22 */ 0x22, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x23 */ 0x23, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x24 */ 0x24, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x25 */ 0x25, 0x04, 0x04, 0x25, 0xD4, 0xE5, 0x10, 0x00, 0x1B,
    /* row 0x26 */ 0x26, 0x00, 0x07, 0x26, 0xEE, 0xE5, 0x10, 0x00, 0x1C,
    /* row 0x27 */ 0x27, 0x00, 0x07, 0x27, 0xF4, 0xE5, 0x10, 0x00, 0x1D,
    /* row 0x28 */ 0x28, 0x0D, 0x07, 0x28, 0xFA, 0xE5, 0x10, 0x00, 0x4C,
    /* row 0x29 */ 0x29, 0x0B, 0x07, 0x29, 0x06, 0xE2, 0x10, 0x00, 0x01,
    /* row 0x2A */ 0x2A, 0x0B, 0x07, 0x2A, 0x7E, 0xE2, 0x10, 0x00, 0x03,
    /* row 0x2B */ 0x2B, 0x02, 0x07, 0x26, 0x40, 0xE6, 0x10, 0x00, 0x1E,
    /* row 0x2C */ 0x2C, 0x03, 0x00, 0x27, 0x50, 0xE6, 0x10, 0x00, 0x1F,
    /* row 0x2D */ 0x2D, 0x03, 0x00, 0x27, 0x64, 0xE6, 0x10, 0x00, 0x20,
    /* row 0x2E */ 0x2E, 0x05, 0x00, 0x27, 0x78, 0xE6, 0x10, 0x00, 0x21,
    /* row 0x2F */ 0x2F, 0x08, 0x00, 0x26, 0x96, 0xE6, 0x10, 0x00, 0x22,
    /* row 0x30 */ 0x30, 0x03, 0x00, 0x26, 0xF6, 0xE2, 0x10, 0x00, 0x06,
    /* row 0x31 */ 0x31, 0x04, 0x00, 0x26, 0x24, 0xE3, 0x10, 0x00, 0x06,
    /* row 0x32 */ 0x32, 0x0A, 0x02, 0x26, 0x8C, 0xE9, 0x10, 0x00, 0x24,
    /* row 0x33 */ 0x33, 0x08, 0x02, 0x26, 0x92, 0xE5, 0x10, 0x00, 0x17,
    /* row 0x34 */ 0x34, 0x07, 0x20, 0x3F, 0xC4, 0xE6, 0x10, 0x00, 0x25,
    /* row 0x35 */ 0x35, 0x07, 0x20, 0x40, 0xEC, 0xE6, 0x10, 0x00, 0x25,
    /* row 0x36 */ 0x36, 0x07, 0x10, 0x41, 0x14, 0xE7, 0x10, 0x00, 0x25,
    /* row 0x37 */ 0x37, 0x07, 0x10, 0x42, 0x3C, 0xE7, 0x10, 0x00, 0x25,
    /* row 0x38 */ 0x38, 0x07, 0x20, 0x3F, 0x64, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x39 */ 0x39, 0x07, 0x20, 0x40, 0x8C, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x3A */ 0x3A, 0x07, 0x10, 0x41, 0xB4, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x3B */ 0x3B, 0x07, 0x10, 0x42, 0xDC, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x3C */ 0x3C, 0x05, 0x00, 0x0F, 0x04, 0xE8, 0x10, 0x00, 0x2B,
    /* row 0x3D */ 0x3D, 0x07, 0x20, 0x3F, 0x22, 0xE8, 0x10, 0x00, 0x2C,
    /* row 0x3E */ 0x3E, 0x07, 0x10, 0x41, 0x4A, 0xE8, 0x10, 0x00, 0x2C,
    /* row 0x3F */ 0x3F, 0x04, 0x20, 0x26, 0x8C, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x40 */ 0x40, 0x04, 0x20, 0x27, 0xC0, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x41 */ 0x41, 0x04, 0x10, 0x26, 0x72, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x42 */ 0x42, 0x04, 0x10, 0x27, 0xA6, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x43 */ 0x43, 0x05, 0x07, 0x43, 0xDA, 0xE8, 0x10, 0x00, 0x30,
    /* row 0x44 */ 0x44, 0x05, 0x02, 0x26, 0xF8, 0xE8, 0x10, 0x00, 0x31,
    /* row 0x45 */ 0x45, 0x06, 0x00, 0x26, 0x16, 0xE9, 0x10, 0x00, 0x32,
    /* row 0x46 */ 0x46, 0x0C, 0x00, 0x41, 0x3A, 0xE9, 0x10, 0x00, 0x33,
    /* row 0x47 */ 0x47, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x48 */ 0x48, 0x03, 0x00, 0x00, 0xE8, 0xE9, 0x10, 0x00, 0x34,
    /* row 0x49 */ 0x49, 0x02, 0x00, 0x00, 0xFC, 0xE9, 0x10, 0x00, 0x35,
    /* row 0x4A */ 0x4A, 0x0A, 0x00, 0x02, 0x0C, 0xEA, 0x10, 0x00, 0x36,
    /* row 0x4B */ 0x4B, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x4C */ 0x4C, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x4D */ 0x4D, 0x08, 0x00, 0x02, 0x44, 0xEA, 0x10, 0x00, 0x38,
    /* row 0x4E */ 0x4E, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x4F */ 0x4F, 0x08, 0x00, 0x1C, 0x72, 0xEA, 0x10, 0x00, 0x39,
    /* row 0x50 */ 0x50, 0x07, 0x00, 0x0F, 0xA0, 0xEA, 0x10, 0x00, 0x3C,
    /* row 0x51 */ 0x51, 0x0B, 0x00, 0x0F, 0xFE, 0xEB, 0x10, 0x00, 0x3D,
    /* row 0x52 */ 0x52, 0x05, 0x00, 0x7E, 0x3A, 0xEC, 0x10, 0x00, 0x3F,
    /* row 0x53 */ 0x53, 0x33, 0x00, 0x41, 0xC8, 0xEA, 0x10, 0x00, 0x3B,
    /* row 0x54 */ 0x54, 0x09, 0x00, 0x0F, 0xCC, 0xEB, 0x10, 0x00, 0x3D,
    /* row 0x55 */ 0x55, 0x07, 0x00, 0x00, 0x2E, 0xEE, 0x10, 0x00, 0x54,
    /* row 0x56 */ 0x56, 0x19, 0x00, 0x0F, 0x58, 0xEC, 0x10, 0x00, 0x40,
    /* row 0x57 */ 0x57, 0x05, 0x00, 0x00, 0xBA, 0xE2, 0x10, 0x00, 0x04,
    /* row 0x58 */ 0x58, 0x09, 0x06, 0x00, 0x0C, 0xED, 0x10, 0x00, 0x42,
    /* row 0x59 */ 0x59, 0x06, 0x00, 0x00, 0x3E, 0xED, 0x10, 0x00, 0x43,
    /* row 0x5A */ 0x5A, 0x0B, 0x07, 0x5A, 0x06, 0xE2, 0x10, 0x00, 0x44,
    /* row 0x5B */ 0x5B, 0x08, 0x06, 0x00, 0x62, 0xED, 0x10, 0x00, 0x46,
    /* row 0x5C */ 0x5C, 0x08, 0x00, 0x27, 0xBE, 0xED, 0x10, 0x00, 0x47,
    /* row 0x5D */ 0x5D, 0x06, 0x00, 0x00, 0xEC, 0xED, 0x10, 0x00, 0x48,
    /* row 0x5E */ 0x5E, 0x05, 0x00, 0x0F, 0xE6, 0xE3, 0x10, 0x00, 0x0C,
    /* row 0x5F */ 0x5F, 0x08, 0x06, 0x00, 0x90, 0xED, 0x10, 0x00, 0x46,
    /* row 0x60 */ 0x60, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x49,
    /* row 0x61 */ 0x61, 0x05, 0x06, 0x61, 0xE0, 0xE4, 0x10, 0x00, 0x14,
    /* row 0x62 */ 0x62, 0x00, 0x07, 0x62, 0x00, 0xE2, 0x10, 0x00, 0x4F,
    /* row 0x63 */ 0x63, 0x00, 0x07, 0x63, 0x00, 0xE2, 0x10, 0x00, 0x50,
    /* row 0x64 */ 0x64, 0x00, 0x07, 0x64, 0x00, 0xE2, 0x10, 0x00, 0x51,
    /* row 0x65 */ 0x65, 0x00, 0x07, 0x65, 0x00, 0xE2, 0x10, 0x00, 0x52,
    /* row 0x66 */ 0x66, 0x00, 0x07, 0x66, 0x60, 0xEE, 0x10, 0x00, 0x53,
    /* row 0x67 */ 0x67, 0x05, 0x06, 0x67, 0x10, 0xEE, 0x10, 0x00, 0x4D,
    /* row 0x68 */ 0x68, 0x03, 0x00, 0x0F, 0x9E, 0xEE, 0x10, 0x00, 0x57,
    /* row 0x69 */ 0x69, 0x04, 0x00, 0x00, 0xB2, 0xEE, 0x10, 0x00, 0x58,
    /* row 0x6A */ 0x6A, 0x08, 0x00, 0x01, 0x70, 0xEE, 0x10, 0x00, 0x55,
    /* row 0x6B */ 0x6B, 0x09, 0x00, 0x01, 0xCC, 0xEE, 0x10, 0x00, 0x59,
    /* row 0x6C */ 0x6C, 0x00, 0x06, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x6D */ 0x6D, 0x01, 0x07, 0x66, 0x56, 0xEE, 0x10, 0x00, 0x53,
    /* row 0x6E */ 0x6E, 0x01, 0x07, 0x00, 0x66, 0xEE, 0x10, 0x00, 0x53,
};

/* FU-89 §6/FUN_000589E0 (first-hand disassemble_function 0x589E0): the
 * near-depth threshold is `max((0x14<<16)/(2*obj[5]), 0x78)`, or the
 * `[0x9088]` fallback when `obj[1] < [0x908C]`; `read_memory 0x109088` gives
 * `[0x9088] = 0x78` and `[0x908C] = 0x140`. obj[1] is the camera y and obj[5]
 * the camera record +0x4C ratio dword, whose static loader default is
 * `0x1500` (first-hand `read_memory 0x107554` = `00 15 00 00`, and the
 * sibling record 0x1075C4 likewise; the writer `0x4D836 MOV [EDX+0x4C],EBX`
 * stores the full dword from the camera-type entry[5]. The FU-96/FU-97 "21"
 * is the **byte at +0x4D**, not the dword.) The camera-type setup
 * (FUN_0004D7E8, values 4608/3048/3464..) is unported (open leg). */
#define MATCH_RUN_NEAR_LIMIT 0x140
#define MATCH_RUN_VIEW_RATIO 0x1500

/* FU-61 §2.3 sampler mapping rows: row 0/1 is the identity pinned by
 * tests/test_input.c (`row_identity`). The view-dependent row index [0x7DEC]
 * (FUN_0004CAE4/FUN_0004CA70) belongs to the match camera (Task 15), so the
 * engine keeps the default identity row here. */
static const uint8_t match_run_input_row[FIFA96_INPUT_MAP_ROW] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

/* Engine keys -> FU-61 §1.2 keyboard-handler codes. Directions: the handler
 * ORs 0x1 (0x46934, flag 0x112BEC = scancode 0x48 keypad 8/up), 0x2 (0x46941,
 * 0x50 keypad 2/down), 0x8 (0x46945, 0x4B keypad 4/left) and 0x4 (0x46952,
 * 0x4D keypad 6/right); the diagonal flags at 0x4695F..0x4698F (keypad
 * 7/9/1/3) OR 0x9/0x5/0xA/0x6, cross-checking those axis assignments.
 * Buttons: KICK 0x10, PASS 0x20 (the 0x40 long-ball and 0x80 keeper arms have
 * no engine key yet). */
static int match_run_map_key(int32_t raw_code, uint8_t *code) {
  switch (raw_code) {
    case FIFA96_ENGINE_KEY_UP:    *code = 0x01u; return 1;
    case FIFA96_ENGINE_KEY_DOWN:  *code = 0x02u; return 1;
    case FIFA96_ENGINE_KEY_LEFT:  *code = 0x08u; return 1;
    case FIFA96_ENGINE_KEY_RIGHT: *code = 0x04u; return 1;
    case FIFA96_ENGINE_KEY_KICK:  *code = 0x10u; return 1;
    case FIFA96_ENGINE_KEY_PASS:  *code = 0x20u; return 1;
    default:                      return 0;
  }
}

int fifa96_match_run_input(struct fifa96_match_run *mr, const fifa96_platform_key *keys,
                           size_t count) {
  uint8_t current[FIFA96_INPUT_PLAYERS] = { 0, 0, 0, 0 };
  uint8_t raw = 0;
  uint8_t mapped = 0;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (keys) {
    for (size_t i = 0; i < count; i++) {
      uint8_t code = 0;
      if (keys[i].state != 1) continue;   /* press events only */
      if (!match_run_map_key(keys[i].raw_code, &code)) continue;
      raw |= code;
    }
  }
  (void)fifa96_input_map(raw, match_run_input_row, &mapped);
  current[0] = mapped;
  (void)fifa96_input_update(&mr->input, current);
  memcpy(mr->input_state, current, sizeof current);   /* sampled slot input */
  return 0;
}

/* M2 interactive Task 1 / FU-77 §1: the shared per-record locomotion mover
 * `FUN_0007BF20` blocks A-E (`0x8E24B..0x8E507`, twin `FUN_0008E244`). The
 * native calls it from both record-machine tails after the action handler
 * (`0x7CD48`/`0x785C5`) for every record; FU-147 S1 removes the
 * controlled-record guard so the driver loop `0x8DB2E..0x8DB5F` runs it for
 * every dispatched record (the pool walk keeps the `+0x9A` skip). The fields
 * the native reads/writes persist on the pool record (`face7d` +0x7D,
 * `speed71` +0x71, `vel73`/`vel75` +0x73/+0x75, `body_timer9c` +0x9C,
 * `timer7b` +0x7B as the stride nibble); the unported inputs are staged as
 * their derived defaults: `+0x6F` stride rate 0, `+0x43` direct-face 0 and
 * the `0x57A73` point (0,0). `move_attr` is the native `dword[rec+0x63]`,
 * whose high word is the block-A distance `+0x65` (the low word falls out of
 * the `>>19` attr extract); it is recomputed here from the post-action target
 * so the stride index sees the fresh distance, exactly as the native
 * recomputes `+0x65` before the table read (`0x8E267..0x8E278`). */
static void match_run_record_mover(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  fifa96_action_locomotion s;
  fifa96_arm_vec from = { r->pos_x, r->pos_y, r->pos_z };
  fifa96_arm_vec to = { r->target_x, r->target_y, r->target_z };
  int32_t distance = 0;
  int32_t lane_unused = 0;
  memset(&s, 0, sizeof s);
  s.pos_x = r->pos_x;
  s.pos_z = r->pos_z;
  s.target_x = r->target_x;
  s.target_z = r->target_z;
  s.delta = r->delta;
  s.facing = r->face7d;
  s.speed = r->speed71;
  s.vel_x = r->vel73;
  s.vel_z = r->vel75;
  (void)fifa96_arm_dist_stage(&from, &to, &distance, &lane_unused);
  s.move_attr = (int32_t)((uint32_t)(uint16_t)distance << 16);
  s.heading = r->type;
  s.has_slot = r->has_slot;
  s.body_timer = r->body_timer9c;
  s.stride = (uint8_t)(r->timer7b & 0x0Fu);
  if (fifa96_action_locomotion_step(&s, fifa96_match_heading_table,
                                    fifa96_match_stride_table) != FIFA96_OK)
    return;
  r->pos_x = s.pos_x;
  r->pos_z = s.pos_z;
  r->face7d = s.facing;
  r->speed71 = s.speed;
  r->vel73 = s.vel_x;
  r->vel75 = s.vel_z;
  r->body_timer9c = s.body_timer;
  r->type = s.heading;
  /* The pool keeps both views of the native velocity bytes: the dword at
   * +0x71 = {word +0x71 speed, word +0x73 vel_x} and the dword at +0x73 =
   * {word +0x73 vel_x, word +0x75 vel_z}. Recompose them from the mover's
   * word writes so the live dword consumers (ball pairing `(int16_t)vel_x`
   * = +0x71, row 04/2A `vel_x >> 16` = +0x73, carrier `+0x71`) read the
   * native bytes, not stale copies. */
  r->vel_x = (int32_t)((uint32_t)(uint16_t)r->speed71 |
                       ((uint32_t)(uint16_t)r->vel73 << 16));
  r->vel_z = (int32_t)((uint32_t)(uint16_t)r->vel73 |
                       ((uint32_t)(uint16_t)r->vel75 << 16));
}

/* FU-141: one pool record -> the FU-138/FU-140 staging record -> the FU-137
 * action dispatch, then the handler's requests back into the pool record. The
 * bound FU-70 slot's animation/direction bytes (native slot +0x20/+0x21, the
 * `slot+0x1D`/`slot+0x1E` high bytes row 00 reads) feed the move target. The
 * row classification is unchanged: an unported row returns
 * -FIFA96_ERR_UNSUPPORTED and the pool chain skips it. */
static int match_run_dispatch_entity(void *ctx, struct fifa96_match_entity *e) {
  struct fifa96_match_run *mr = ctx;
  struct fifa96_match_run_record *r = &mr->record;
  int32_t id = (int32_t)((uint32_t)e->team * FIFA96_MATCH_ENTITY_RECORDS + e->index);
  int rc;
  r->pos_x = e->pos_x;
  r->pos_y = e->pos_y;
  r->pos_z = e->pos_z;
  r->target_x = e->target_x;
  r->target_y = e->target_y;
  r->target_z = e->target_z;
  r->timer89 = e->timer89;
  r->timer81 = e->timer81;
  r->delta = (uint16_t)mr->state.frame_delta;
  r->active = e->active;
  r->has_slot = e->has_slot;
  r->has_ball = e->has_ball;
  r->stage = e->stage;
  r->stage92 = e->stage92;
  r->timer7b = e->timer7b;
  r->lane = e->lane;
  /* FU-142e: reproduce the unported FUN_0008D098 pre-switch walk (`0x8D11E`),
   * which calls 0x8DCD4(pos, target) into `+0x65/+0x67/+0x69` for every free
   * record before the installer arms; the derived staging computes the word
   * row 2A gates on from this call's pos/target. The +0x69 lane stays owned by
   * the rows that consume it (26/27/28/29 recompute it in their prologues). */
  {
    fifa96_arm_vec from = { r->pos_x, r->pos_y, r->pos_z };
    fifa96_arm_vec to = { r->target_x, r->target_y, r->target_z };
    int32_t distance = 0;
    int32_t lane_unused = 0;
    (void)fifa96_arm_dist_stage(&from, &to, &distance, &lane_unused);
    r->distance = distance;
  }
  r->type = e->type;
  r->actor_type = e->actor_type;
  r->code = e->code;   /* native +0x91: the kick/row-04 gates index this byte */
  r->anim_id = e->anim_id;   /* OL-80: byte[[rec+0x28]] live row id */
  r->frame = e->frame;       /* OL-80: native +0x3D live frame index */
  r->entity_id = id;
  r->vel_x = e->vel_x;
  r->vel_z = e->vel_z;
  /* FU-142d: the row-28 pool fields. The team-side/flag830 and the resolved
   * [team+0x831] chosen-record position are this call's inputs; the five
   * process globals come from the run (producers unported, OL-56). */
  r->side = mr->entities.team[e->team].side;
  r->flag830 = mr->entities.team[e->team].flag830;
  r->scratch_a2 = e->scratch_a2;
  r->scratch_a6 = e->scratch_a6;
  r->scratch_aa = e->scratch_aa;
  r->scratch_ae = e->scratch_ae;
  r->scratch_a0 = e->scratch_a0;
  r->scratch_a1 = e->scratch_a1;
  r->global_10f358 = mr->global_10f358;
  r->global_10f35c = mr->global_10f35c;
  r->global_10f364 = mr->global_10f364;
  r->global_10f368 = mr->global_10f368;
  r->global_157ac2 = mr->global_157ac2;
  {
    const struct fifa96_match_team *team = &mr->entities.team[e->team];
    r->chosen_ok = 0;
    r->chosen_x = 0;
    r->chosen_y = 0;
    r->chosen_z = 0;
    if (team->chosen831 >= 0 &&
        (uint32_t)team->chosen831 <
            FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) {
      uint32_t ct = (uint32_t)team->chosen831 / FIFA96_MATCH_ENTITY_RECORDS;
      uint32_t ci = (uint32_t)team->chosen831 % FIFA96_MATCH_ENTITY_RECORDS;
      r->chosen_x = mr->entities.team[ct].records[ci].pos_x;
      r->chosen_y = mr->entities.team[ct].records[ci].pos_y;
      r->chosen_z = mr->entities.team[ct].records[ci].pos_z;
      r->chosen_ok = 1;
    }
  }
  r->player_d = 0;   /* native rec[+4][+0xD]; the roster descriptor is
                      * unmodeled by the pool (FU-142b Appendix C) */
  r->player_e = 0;   /* native rec[+4][+0xE] (same) */
  r->place_offset_x = e->place_offset_x;
  r->place_offset_z = e->place_offset_z;
  r->ran = e->ran;
  r->helper_request = 0;
  r->controlled = 0;
  r->place_valid = 0;
  r->install = 0;
  if (mr->slot.entity == id) {
    r->dir_x = (int8_t)mr->slot.anim_b;   /* native slot +0x20 */
    r->dir_z = (int8_t)mr->slot.anim_c;   /* native slot +0x21 */
  } else {
    r->dir_x = e->dir_x;
    r->dir_z = e->dir_z;
  }
  r->face7d = e->face7d;
  r->speed71 = e->speed71;
  r->vel73 = e->vel73;
  r->vel75 = e->vel75;
  r->body_timer9c = e->body_timer9c;
  rc = fifa96_match_dispatch_action(mr, e->code);
  /* The arm handlers (rows 26/28/2A) write the dword views of the velocity
   * bytes; decompose them back onto the word views the mover reads so the
   * native `+0x71/+0x73/+0x75` bytes stay in lockstep (an arm's `vel_x=0`
   * therefore clears the words too, and a pass-through is identity because the
   * staging invariant holds). */
  r->speed71 = (int16_t)r->vel_x;
  r->vel73 = (int16_t)((uint32_t)r->vel_x >> 16);
  r->vel75 = (int16_t)((uint32_t)r->vel_z >> 16);
  /* FU-147 S1: every dispatched record runs the shared mover (native
   * FUN_0007CA54 tail 0x7CD48 / FUN_000782D0 tail 0x785C5), then the BF20
   * lane block refreshes the +0x6B lane / +0x6D/+0x6F camera deltas / +0x77
   * bound and the per-team camera-nearest tracker. Native
   * `0x7C776..0x7C7CF`, first-hand this slice; the metric call is 0x8DC68 =
   * `fifa96_entity_distance`. The camera focus 0x15774C/0x157754 is the
   * engine render-camera stand-in (FU-147 leg 13). */
  match_run_record_mover(mr);
  {
    int16_t lane = 0;
    int16_t cam_dx = 0;
    int16_t cam_dz = 0;
    if (fifa96_action_locomotion_track(r->pos_x, r->pos_z, mr->render.camera.pos_x,
                                       mr->render.camera.pos_z, &lane, &cam_dx,
                                       &cam_dz) == FIFA96_OK) {
      struct fifa96_match_team *team = &mr->entities.team[e->team];
      int32_t tracker = team->camera_nearest;
      e->bound = e->lane_x;             /* 0x7C77E: +0x77 := old +0x6B */
      e->lane_z = cam_dx;               /* 0x7C78E: +0x6D */
      e->cam_dz6f = cam_dz;             /* 0x7C79A: +0x6F */
      e->lane_x = lane;                 /* 0x7C7AF: +0x6B */
      /* 0x7C7B3..0x7C7CF: the tracker keeps an incumbent with an equal or
       * smaller lane (signed word JGE at 0x7C7C8); a NONE tracker or a
       * strictly smaller fresh lane replaces it. The native `[team+0x7C7]`
       * is a record pointer; the pool stores the team-relative index. */
      if (tracker == FIFA96_MATCH_ENTITY_NONE ||
          (int16_t)e->lane_x < (int16_t)team->records[tracker].lane_x)
        team->camera_nearest = (int32_t)e->index;
    }
  }
  e->pos_x = r->pos_x;
  e->pos_y = r->pos_y;
  e->pos_z = r->pos_z;
  e->target_x = r->target_x;
  e->target_y = r->target_y;
  e->target_z = r->target_z;
  e->timer89 = r->timer89;
  e->stage92 = r->stage92;
  e->timer7b = r->timer7b;
  e->lane = r->lane;
  e->type = r->type;
  e->anim_id = r->anim_id;   /* OL-80: the arm bodies' `anim_sel` write-back */
  e->frame = r->frame;
  e->vel_x = r->vel_x;
  e->vel_z = r->vel_z;
  e->scratch_a2 = r->scratch_a2;
  e->scratch_a6 = r->scratch_a6;
  e->scratch_aa = r->scratch_aa;
  e->scratch_ae = r->scratch_ae;
  e->scratch_a0 = r->scratch_a0;
  e->scratch_a1 = r->scratch_a1;
  e->ran = r->ran;
  e->has_ball = r->has_ball;   /* FU-147 S1: the +0x9B possession-flag
                                * producer (row-1E claim 0x755C0) must persist
                                * on the pool, not only on the staging record */
  e->install = r->install;
  e->helper_request = r->helper_request;
  e->controlled = r->controlled;
  e->place_valid = r->place_valid;
  e->place_x = r->place_x;
  e->place_y = r->place_y;
  e->place_z = r->place_z;
  e->dir_x = r->dir_x;
  e->dir_z = r->dir_z;
  e->face7d = r->face7d;
  e->speed71 = r->speed71;
  e->vel73 = r->vel73;
  e->vel75 = r->vel75;
  e->body_timer9c = r->body_timer9c;
  /* FU-142e: row 2A writes the team `+0x830` flag (arms 0/1) and the
   * `[0x10F358]`/`[0x10F35C]` process globals (arms 0/9/10); the native writes
   * are process-wide, so the staged values land back on the pool/run for the
   * later records of the same frame (the frame staging reads them again). */
  mr->entities.team[e->team].flag830 = r->flag830;
  mr->global_10f358 = r->global_10f358;
  mr->global_10f35c = r->global_10f35c;
  return rc;
}

/* FU-141 frame inputs: the phase/delta plus the FU-71 camera triple used as
 * the three selection vectors (the native 0x157770/0x157788/0x157794 shadow
 * the 0x15774C camera position at init, FU-67 §4.1). The 0x1577FA/0x157800/
 * 0x157806 bucket timers and the 0x10F37C/0x10F388 intercept targets are the
 * unported camera/track block and executable data (FU-67 S3, FU-130), so they
 * stay zero and the derived bucket falls to the third vector. */
static void match_run_entity_frame(const struct fifa96_match_run *mr,
                                   struct fifa96_match_entities_frame *f) {
  memset(f, 0, sizeof *f);
  f->phase = (uint8_t)mr->state.phase;
  f->delta = (uint16_t)mr->state.frame_delta;
  for (int v = 0; v < (int)FIFA96_MATCH_ENTITY_SELECT_VECTORS; v++) {
    f->select_vector[v][0] = mr->render.camera.pos_x;
    f->select_vector[v][1] = mr->render.camera.pos_y;
    f->select_vector[v][2] = mr->render.camera.pos_z;
  }
  f->cam_z = (int16_t)mr->render.camera.pos_z;
}

/* FU-141 drains the pool leaves to the engine: a successful slot merge moves
 * the FU-70 slot binding (and runs the FUN_00078670 clear), and a pending
 * row-1E placement triple resets the FU-71 camera through the existing
 * `fifa96_camera_init` (the FU-71 port of FUN_000700F4's field reset). */
static void match_run_entity_drain(struct fifa96_match_run *mr) {
  int32_t merged = fifa96_match_entities_take_slot_merge(&mr->entities);
  int32_t px = 0;
  int32_t py = 0;
  int32_t pz = 0;
  if (merged >= 0) {
    mr->slot.entity = merged;
    (void)fifa96_control_slot_merge_reset(&mr->slot);
  }
  if (fifa96_match_entities_take_place(&mr->entities, &px, &py, &pz) == 1) {
    (void)fifa96_camera_init(&mr->render.camera, px, py, pz);
  }
}

/* FU-85 §4/FUN_00036C70 (first-hand disassemble_function 0x36C70) stages the
 * 23 render slots per frame body: team 0 records 0..10 (base 0x1588A4) then
 * team 1 records 0..10 (0x1590D9), each from rec+0x59/5D/61 with hidden set by
 * rec+0x9A (staged y = -10000, 0x36D61), then slot 22 from the ball record
 * 0x15880C with the 0x15885A heading. The native record angle is
 * `((0x400 - (rec[+0x7B]>>16)) & 0x3FF) << 6`, i.e. the facing word +0x7D,
 * which the FU-141 pool does not model (FU-77 locomotion unported): the
 * record `stage.heading` stays caller-owned (open leg), while the ball's
 * modeled FU-120 heading is staged.
 *
 * OL-80 (links FU-141 OL-42): the same staging passes `anim_id` from
 * `byte[[rec+0x28]]` (0x36D44) and `frame` from `byte[rec+0x3D]` (0x36D4F).
 * The pool now models both (the FU-84 selector's row id and the frame index),
 * so the staging seeds the slot from the live pool record, runs the derived
 * FU-84 advance and writes the advanced index back into the record for the
 * next frame's dispatch (the arm bodies' `fifa96_arm_anim_select` reads).
 *
 * FU-84 `FUN_0008E008` (first-hand FU-84 §5): each slot's row id (`stage.
 * anim_id`) selects the 0x10EF00 row; the row's +8 byte is the FU-85 resolver
 * sprite bank, and the row's last frame plus the slot's frame record duration
 * (the staged 0x10E200-family records) advance the accumulator once with
 * `delta<<4` (the whole-frame delta `[0x57A64]`, FU-62 §4.3). The row
 * successor/terminal machine and the height-gated 0x5A branch are unported
 * (open leg). */
static void match_run_scene_stage(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r = &mr->render;
  uint32_t slot = 0;
  for (uint32_t team = 0; team < FIFA96_MATCH_ENTITY_TEAMS; team++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++, slot++) {
      struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
      struct fifa96_match_run_entity *re = &r->entities[slot];
      re->stage.pos.x = e->pos_x;
      re->stage.pos.y = e->pos_y;
      re->stage.pos.z = e->pos_z;
      re->stage.anim_id = (int32_t)e->anim_id;   /* 0x36D44 byte[[rec+0x28]] */
      re->stage.frame = (int32_t)e->frame;       /* 0x36D4F byte[rec+0x3D] */
      re->stage.hidden = e->skip_9a ? 1 : 0;
    }
  }
  struct fifa96_match_run_entity *ball = &r->entities[slot];
  ball->stage.pos.x = mr->entities.ball.x;
  ball->stage.pos.y = mr->entities.ball.y;
  ball->stage.pos.z = mr->entities.ball.z;
  ball->stage.heading =
      (int32_t)((uint32_t)(uint16_t)mr->entities.ball.heading << 16);
  ball->stage.hidden = 0;
  r->entity_count = slot + 1;

  if (!r->frames) return;
  for (uint32_t i = 0; i < r->entity_count; i++) {
    struct fifa96_match_run_entity *e = &r->entities[i];
    /* Slots 0..21 are the two teams' records; slot 22 is the ball (no +0x3D
     * frame field), so the frame write-back targets the pool record. */
    struct fifa96_match_entity *rec = NULL;
    if (i < FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) {
      rec = &mr->entities.team[i / FIFA96_MATCH_ENTITY_RECORDS]
                 .records[i % FIFA96_MATCH_ENTITY_RECORDS];
    }
    if (e->stage.anim_id >= 0x6F) continue;   /* the resolver's signed id gate */
    fifa96_anim_row row;
    if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                    (int32_t)e->stage.anim_id, &row) != FIFA96_OK)
      continue;
    e->bank_index = row.sprite_bank;
    fifa96_anim_frame record;
    if (fifa96_animation_frame(r->frames, (int32_t)e->stage.frame, row.last_frame,
                               &record) != FIFA96_OK)
      continue;
    fifa96_anim_advance state;
    state.timer = e->anim_timer;
    state.delta = (uint16_t)mr->state.frame_delta;
    state.frame_index = e->stage.frame;
    state.turn = e->anim_turn;
    fifa96_anim_advance_out out;
    if (fifa96_animation_advance(&state, record.duration, row.last_frame, &out) !=
        FIFA96_OK)
      continue;
    e->anim_timer = state.timer;
    e->stage.frame = out.frame_index;
    if (rec) rec->frame = (uint8_t)out.frame_index;   /* OL-80 +0x3D */
  }
}

/* Stable slot identity: the lifecycle cancels by function pointer, so every
 * run shares this one trampoline and the engine bridge keeps a single match
 * callback in the clock's tick table. The trampoline is the sole driver of the
 * frame body: one call per PIT tick, so the pace stays at 100 Hz even when an
 * engine step spans 0..N ticks (FU-60 drives the pace from the INT-8 ISR). */
static void fifa96_match_run_tick(void *user) {
  struct fifa96_match_run *mr = user;
  if (!mr) return;
  mr->ticks++;
  (void)fifa96_match_run_frame(mr);
}

static int fifa96_match_run_register(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  int slot = fifa96_tick_register(&mr->engine->clock.ticks, fifa96_match_run_tick,
                                  mr, 1u); /* registered at 100 Hz (FU-64 §2.2) */
  return slot < 0 ? slot : 0;
}

static int fifa96_match_run_cancel(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  return fifa96_tick_cancel(&mr->engine->clock.ticks, fifa96_match_run_tick);
}

static void match_run_reset_screen_state(struct fifa96_match_run *mr);

static int fifa96_match_run_teardown(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* The foundation owns no match assets yet; clear the per-match counters,
   * the match clock/period block and the per-side goal words. */
  mr->ticks = 0;
  mr->steps = 0;
  fifa96_match_state_init(&mr->state);
  mr->score[0] = 0;
  mr->score[1] = 0;
  /* C3-OL2: the carried writer state defaults (native producers OL-87). */
  mr->score_last_side = -1;
  mr->score_tracked_side = -1;
  mr->score_max_diff = 0;
  mr->score_last_event = 0;
  mr->global_5882a = 0;                /* drop the kickoff gate with the match */
  /* FU-145 S2: the restart-body clear (`0x84F90`) drops the goal-arm state
   * with the match; the session gate returns to 0 with the session. */
  mr->goal_armed = 0;
  mr->goal_zone = 0;
  mr->goal_snap_x = 0;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = 0;
  mr->situation_id = 0;
  mr->situation_pending = 0;
  mr->session_gate_14c32a = 0;
  match_run_reset_screen_state(mr);    /* FU-146 S3: no installed handler */
  (void)fifa96_match_entities_release(&mr->entities);
  return 0;
}

static int fifa96_match_run_post_exit(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  mr->engine->mode = FIFA96_ENGINE_MODE_FRONTEND;
  return 0;
}

static void fifa96_match_run_install_defaults(struct fifa96_match_run *mr) {
  mr->backend.register_callback = fifa96_match_run_register;
  mr->backend.cancel_callback = fifa96_match_run_cancel;
  mr->backend.teardown = fifa96_match_run_teardown;
  mr->backend.post_exit = fifa96_match_run_post_exit;
  mr->backend.ctx = mr;
}

/* Fresh-match input state: zero the FU-61 edge/held model and bind the
 * controlled player's FU-70 slot (player 0, raw map select 1 per FU-70 §1.4,
 * ordinal 0, side 0). */
static void fifa96_match_run_reset_input(struct fifa96_match_run *mr) {
  fifa96_input_init(&mr->input);
  memset(mr->input_state, 0, sizeof mr->input_state);
  memset(&mr->slot, 0, sizeof mr->slot);
  (void)fifa96_control_slot_init(&mr->slot, 0, 1, 0, 0);
}

/* Fresh-match presentation state (Task 15): zero the FU-71 camera, FU-92
 * window, FU-69 display block and FU-84/85 scene/sprite assets; rendering is
 * opt-in so the Task 12-14 engine fixtures keep their surface untouched. The
 * indexed remap starts as the identity translation with index 0 as the derived
 * colour key (FU-85 §5: the 0x14720 buffer is the installed palette's
 * translation buffer, `FUN_000CE980` copying its source in and 0xCEABC
 * treating 0xFF as transparent; without the per-sprite palette install
 * `FUN_00048DC0` the engine's identity translation is the stand-in -- open
 * leg). FU-84: the selector seeds the driver turn +1 (`FUN_0006E598` 0x6E6D8
 * `[rec+0x46] = 1`). */
static void fifa96_match_run_reset_render(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r = &mr->render;
  memset(r, 0, sizeof *r);
  (void)fifa96_camera_init(&r->camera, 0, 0, 0);
  (void)fifa96_window_init(&r->window, 0, 0);
  (void)fifa96_match_display_init(&r->display, 0);
  r->view_ratio = MATCH_RUN_VIEW_RATIO;
  for (int i = 0; i < 256; i++) r->remap[i] = (uint8_t)(i == 0 ? 0xFF : i);
  for (int i = 0; i < (int)FIFA96_MATCH_RUN_RENDER_SLOTS; i++) r->entities[i].anim_turn = 1;
}

/* FU-146 S3: the fresh-match goal-consumer state. The native image seeds the
 * `FUN_000CBC4C` probe limbs (`0x112E68..0x112E7C`: `56 0e 2d f2 e9 26 31 88
 * 2f dd 24 c6 9c c4 02 07 7d 3f 35 9e 64 3b df 6f`); `screen_leg` restarts as
 * -1 (no installed handler, the image's `[0x15B6D4]==0`) and `begin` installs
 * the derived leg 0 / mode 0 / side 0 defaults. Runs on init, on the fresh
 * match reset in begin and on teardown. */
static void match_run_reset_screen_state(struct fifa96_match_run *mr) {
  static const uint32_t probe_seed[6] = {
      0xF22D0E56u, 0x883126E9u, 0xC624DD2Fu,
      0x0702C49Cu, 0x9E353F7Du, 0x6FDF3B64u,
  };
  mr->screen_leg = -1;
  mr->screen_mode = 0;
  mr->screen_step = 0;
  mr->screen_timer = 0;
  mr->screen_period_frames = 0;
  mr->screen_install_hint = 0;
  mr->screen_actor_age = 0;
  mr->screen_lead_z = 0;
  mr->goal_no_score = 0;
  mr->goal_last_id = 0;
  mr->goal_minute = 0;
  mr->goal_screen_accum = 0;
  mr->goal_log_prev_total = 0;
  mr->goal_total = 0;
  memset(mr->goal_log, 0, sizeof mr->goal_log);
  memcpy(mr->goal_probe_limb, probe_seed, sizeof probe_seed);
}

/* FU-89 §11 / OL-T11-8 (M2 visible-match Task 1) + FU-148 §3 (S4): the
 * resource-loaded formation seed. `FUN_0004A6BC` resolves the `0x14BFC0`
 * table's `.fmt` slots (0..29, one family of six per formation id) — slot
 * `6*formation_id` is the placement file — and the BSS
 * default for an unselected team is formation id 0 (first-hand: `[0x14C1E4]`
 * is BSS 0 and `FUN_0006D9C4` derives `[team+0x7AE] = 0x11033A + id*0x1D`, a
 * 0x1D-byte roster row whose `byte[0]` is the id). S4 replaces the hard-coded
 * 0 with the run's derived `formation[controlled_side]` and the pinned
 * 0x107370 placement names (id 0 `352ko.fmt`, 1 `442ko.fmt`, 2 `swko.fmt`,
 * 3 `424ko.fmt`, 4 `433ko.fmt`; FU-148 §3 leg: the FUN_00011620 team-record
 * producer). Slot 0's file is a BIGF entry of the match art container
 * `/ART/GAMEART0.PVI` (first-hand directory: entry 0; the engine already
 * stages the same file). The seed is a soft failure: no ISO/asset/entry leaves
 * the zero targets the pre-T1 path had. The controlled side is the
 * `[0x157AAC]>>24` mirror (BSS 0). */
#define MATCH_RUN_FORMATION_BANK "/ART/GAMEART0.PVI"

/* FU-144 / OL-T11-6: the native match palette source. `FUN_00048B60`
 * (0x48B6E..0x48B83) reads resource slot 0x32 -- the 0x107370 loader table's
 * slot 50 = 0x101BA4 `"PALsys"`, formatted with `"%s.fsh"` (0x101C90; the
 * `"lfsh"` string starts one byte earlier) -- loads its frame 2
 * (`FUN_000A1920(handle, 2)`) and takes the type-0x22 chunk through
 * `FUN_00047814`. `FUN_00048ED8` then appends the chunk's [0xF0,+0x54) and
 * [0x186,+0x4E) byte ranges over the built palette and `FUN_00048C8C`
 * installs it with the `FUN_000479A0` 8-bit copy (`v << 2`). */
#define MATCH_RUN_PALETTE_BANK "PALsys.fsh"
#define MATCH_RUN_PALETTE_FRAME 2u
#define MATCH_RUN_PALETTE_APPEND_A_OFF 0xF0u
#define MATCH_RUN_PALETTE_APPEND_A_LEN 0x54u
#define MATCH_RUN_PALETTE_APPEND_B_OFF 0x186u
#define MATCH_RUN_PALETTE_APPEND_B_LEN 0x4Eu
#define MATCH_RUN_PALETTE_COUNT 256u

static int match_run_formation_seed(struct fifa96_match_run *mr) {
  struct fifa96_scene_formation formation;
  const char *name;
  uint8_t *bytes = NULL;
  size_t len = 0;
  int seeded = 0;
  if (!mr->engine || !mr->engine->assets) return 0;
  name = fifa96_match_formation_fmt_name(
      mr->formation[mr->phase_machine.side_controlled & 1u]);
  if (!name) return 0;
  if (fifa96_asset_read(mr->engine->assets, MATCH_RUN_FORMATION_BANK, &bytes, &len) !=
      FIFA96_OK)
    return 0;
  if (fifa96_scene_formation_load(bytes, len, name, &formation) == FIFA96_OK)
    seeded = fifa96_match_entities_seed_formation(&mr->entities, &formation,
                                                  mr->phase_machine.side_controlled) ==
             FIFA96_OK;
  fifa96_asset_free(bytes);
  return seeded;
}

/* M2 interactive Task 1 / FU-70 §1.3/§1.4: the derived match-setup slot bind
 * (`FUN_00078824` -> `FUN_000785E0`). The engine models the single human
 * controller (player 0, the `0x4C1DC` map select 1); the native four `0x4C1E0`
 * mode rows are unported, so the derived default is mode 0 = the controlled
 * side (`0x4C1E0[0] == 0` -> `[0x57ABE]`, the `[0x57AAC]>>24` side the engine
 * carries as `side_controlled`). The bind seeds the donor record's `+0x20` and
 * `team+0x828` (slot_pool); the kickoff state-1 arm's `FUN_0007876C` merge
 * then moves the slot onto the taker. */
static void match_run_slot_bind(struct fifa96_match_run *mr) {
  uint32_t side = (uint32_t)(mr->phase_machine.side_controlled & 1);
  struct fifa96_match_team *team = &mr->entities.team[side];
  int32_t id = fifa96_match_entities_bind_slot(&mr->entities, side,
                                               (int16_t)mr->render.camera.pos_x,
                                               (int16_t)mr->render.camera.pos_z);
  if (id == FIFA96_MATCH_ENTITY_NONE) return;
  mr->slot.entity = id;
  mr->slot.player = 0;
  mr->slot.map_select = 1;                /* 0x4C1DC default (FU-70 §1.4) */
  mr->slot.ordinal = (uint8_t)(team->slot_pool - 1u);  /* native +0x1D */
  mr->slot.active = (int8_t)team->side;   /* native +0x22 */
}

/* Release the Task 2 staging arena. Runs only on initialized runs: init must
 * accept uninitialized memory (tests memset 0xAA and re-init), so the owner
 * slot is assigned NULL there and never freed; begin/end/stage call this only
 * after init. A run whose render state was set without staging (owner NULL) is
 * left untouched. */
static void match_run_release_stage(struct fifa96_match_run *mr) {
  if (!mr->stage_owner) return;
  free(mr->stage_owner);
  mr->stage_owner = NULL;
  mr->render.frames = NULL;
  mr->render.banks = NULL;
  mr->render.bank_count = 0;
  mr->render.sprite_data = NULL;
  mr->render.sprite_data_len = 0;
  mr->render.enabled = 0;
  mr->render.palette_ready = 0;
}

void fifa96_match_run_init(struct fifa96_match_run *mr) {
  if (!mr) return;
  fifa96_match_lifecycle_init(&mr->lc);
  fifa96_match_pace_init(&mr->pace);
  fifa96_match_state_init(&mr->state);
  mr->score[0] = 0;
  mr->score[1] = 0;
  mr->score_last_side = -1;       /* C3-OL2 writer state (OL-87 defaults) */
  mr->score_tracked_side = -1;
  mr->score_max_diff = 0;
  mr->score_last_event = 0;
  mr->engine = NULL;
  mr->backend.register_callback = NULL;
  mr->backend.cancel_callback = NULL;
  mr->backend.teardown = NULL;
  mr->backend.post_exit = NULL;
  mr->backend.ctx = NULL;
  mr->ticks = 0;
  mr->steps = 0;
  mr->running = 0;
  mr->stage_owner = NULL;
  memset(&mr->record, 0, sizeof mr->record);
  (void)fifa96_match_entities_init(&mr->entities);
  (void)fifa96_match_phase_machine_init(&mr->phase_machine);
  memset(&mr->rng, 0, sizeof mr->rng);
  mr->global_10f358 = 0;
  mr->global_10f35c = 0;
  mr->global_10f364 = 0;
  mr->global_10f368 = 0;
  mr->global_157ac2 = 0;
  mr->global_5882a = 0;
  mr->pass_parity = 0;
  mr->clock_period_ended = 0;          /* FU-143: no staged clock completion */
  mr->dispatched_ok = 0;
  /* FU-145 S2: fresh goal-arm/queue state (the session gate seeds 0 here and
   * 1 in begin: a live match). */
  mr->goal_armed = 0;
  mr->goal_zone = 0;
  mr->goal_snap_x = 0;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = 0;
  mr->situation_id = 0;
  mr->situation_pending = 0;
  mr->session_gate_14c32a = 0;
  /* FU-149 P1: fresh set-piece dispatcher state (the match-reset seeds:
   * FUN_00073EE0 zeroes 0x157AD4/0x157AD6 and the act-8 replay bytes). */
  mr->sit_side_pending = 0;
  mr->corner_count[0] = 0;
  mr->corner_count[1] = 0;
  mr->side_swap = 0;
  mr->store_15882b = 0;
  mr->store_15882c = 0;
  mr->incident_x = 0;
  mr->incident_z = 0;
  /* FU-148 S4: the native [0x14C1E4]/[0x14C1E5] BSS default is 0. */
  mr->formation[0] = 0;
  mr->formation[1] = 0;
  /* FU-150 P2: fresh referee state, machine dispatch and request slots; the
   * settings config is zero until begin installs the derived default handoff
   * (fouls disabled, offside disabled). */
  memset(&mr->referee, 0, sizeof mr->referee);
  memset(&mr->config, 0, sizeof mr->config);
  mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
  mr->ref_restart_stage = 0;
  mr->ref_whistle = 0;
  mr->ref_speech = 0;
  match_run_reset_screen_state(mr);    /* FU-146 S3: fresh consumer state */
  fifa96_match_run_reset_input(mr);
  fifa96_match_run_reset_render(mr);
}

int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector) {
  if (!mr || !eng) return -FIFA96_ERR_INVALID;
  if (!eng->booted || eng->mode == FIFA96_ENGINE_MODE_QUIT) return -FIFA96_ERR_STATE;
  if (mr->running) return -FIFA96_ERR_STATE;
  if (eng->match && eng->match != mr) return -FIFA96_ERR_STATE;
  if (!mr->backend.register_callback || !mr->backend.cancel_callback ||
      !mr->backend.teardown || !mr->backend.post_exit) {
    fifa96_match_run_install_defaults(mr);
  }
  mr->engine = eng;
  mr->ticks = 0;
  mr->steps = 0;
  fifa96_match_state_init(&mr->state); /* fresh match clock */
  mr->score[0] = 0;                    /* fresh match score pair */
  mr->score[1] = 0;
  mr->score_last_side = -1;            /* C3-OL2 fresh writer state (OL-87) */
  mr->score_tracked_side = -1;
  mr->score_max_diff = 0;
  mr->score_last_event = 0;
  memset(&mr->record, 0, sizeof mr->record); /* fresh FU-138/FU-140 record */
  (void)fifa96_match_entities_init(&mr->entities); /* fresh FU-141 pool */
  (void)fifa96_match_phase_machine_init(&mr->phase_machine); /* fresh FU-142a machine */
  /* FU-142d: a fresh match seeds the RNG (the native FUN_000493A0 match-init
   * seed call 0x493F2) with the derived seed 0 and clears the row-28 process
   * globals (OL-56). */
  (void)fifa96_rng_seed(&mr->rng, 0);
  mr->global_10f358 = 0;
  mr->global_10f35c = 0;
  mr->global_10f364 = 0;
  mr->global_10f368 = 0;
  mr->global_157ac2 = 0;
  mr->global_5882a = 0;                /* fresh kickoff gate (OL-84 residual) */
  mr->pass_parity = 0;                 /* FU-139 §11: fresh [0x157A4F] */
  mr->clock_period_ended = 0;          /* FU-143: fresh clock staging */
  mr->dispatched_ok = 0;               /* Task 15: fresh dispatch observation */
  /* FU-145 S2: a fresh match drops the goal-arm/queue state and seeds the
   * live-session gate 1 ([0x14C32A], FU-146 leg 2) so a fired goal queues
   * ids 5/6 instead of taking the table-2 fallback. */
  mr->goal_armed = 0;
  mr->goal_zone = 0;
  mr->goal_snap_x = 0;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = 0;
  mr->situation_id = 0;
  mr->situation_pending = 0;
  mr->session_gate_14c32a = 1;
  /* FU-149 P1: a fresh match drops the set-piece dispatcher state (the native
   * match reset FUN_00073EE0 zeroes the corner counters 0x157AD4/0x157AD6;
   * the [0x157ABE] swap and the incident triple carry their unported-producer
   * defaults). */
  mr->sit_side_pending = 0;
  mr->corner_count[0] = 0;
  mr->corner_count[1] = 0;
  mr->side_swap = 0;
  mr->store_15882b = 0;
  mr->store_15882c = 0;
  mr->incident_x = 0;
  mr->incident_z = 0;
  /* FU-148 S4: a fresh match restarts at the BSS formation default 0 (the
   * native FUN_00011620 team-record producer is leg). */
  mr->formation[0] = 0;
  mr->formation[1] = 0;
  /* FU-150 P2: a fresh referee machine and the derived FU-68 settings handoff
   * (the menu defaults: foul level 2, offside disabled; the front-end's live
   * settings state is unported). */
  memset(&mr->referee, 0, sizeof mr->referee);
  mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
  mr->ref_restart_stage = 0;
  mr->ref_whistle = 0;
  mr->ref_speech = 0;
  {
    struct fifa96_settings settings;
    fifa96_settings_defaults(&settings);
    (void)fifa96_settings_handoff(&settings, 0u, &mr->config);
  }
  match_run_reset_screen_state(mr);    /* FU-146 S3: fresh consumer state */
  fifa96_match_run_reset_input(mr);    /* fresh input edges/held and slot */
  match_run_release_stage(mr);         /* drop the previous match's staged arena */
  fifa96_match_run_reset_render(mr);   /* fresh camera/window/display/scene */
  /* FU-89 §11 / OL-T11-8: seed the records' targets from the resource-loaded
   * formation before the commit (the native `FUN_0008D098` phase-cell order at
   * `FUN_000740A0`, then `FUN_00073E08`). Soft-fails to zero targets. */
  int formation_seeded = match_run_formation_seed(mr);
  /* M2 interactive Task 1: the derived setup slot bind (`FUN_00078824` ->
   * `FUN_000785E0`) runs before the kickoff state-1 arm, whose `FUN_0007876C`
   * merge moves the bound slot onto the taker (the arm's own `0x8D243` call). */
  match_run_slot_bind(mr);
  /* FU-143 §8/OL-84 (M2 visible-match Task 2): the derived kickoff phase
   * entry, between the native `FUN_000740A0(1, side)` write (0x88E82, act 1 =
   * phase-0x17 handler FUN_00088DC8 stage 0) and the `FUN_00073E08` placement
   * commit begin models below. The run leaves the reset default phase 0 and
   * enters the kickoff-placement phase 1 (class 0: the clock stops). The live
   * phase 2 follows on that chain: the state-1 arm above installs actions 1/2,
   * and action row 01 calls FUN_0008A938 situation 0xB at 0x7DF90 once the
   * derived act-1 producer arms `mr->global_5882a` (M2 playable-match Task 2 /
   * FU-143 §11). */
  (void)match_run_write_phase(mr, FIFA96_MATCH_RUN_KICKOFF_PHASE);
  /* M2 playable-match Task 2 / OL-84 residual: the same setter call runs the
   * FUN_0008D098 state-1 arm per team (native 0x740C8/0x740DB; state 1 arm
   * 0x8D1B1..0x8D243), staging the kickoff actions 1/2 and the team targets.
   * It runs before the placement commit below, matching the native order
   * (0x88E82 FUN_000740A0 precedes 0x88E87 FUN_00073E08; the 0x8D1D1
   * FUN_00079CCC pick reads the placement outputs the formation seed set). */
  (void)fifa96_match_phase_machine_kickoff(mr);
  /* The state-1 arm's `FUN_0007876C` merge (`0x8D243`) recorded the taker as
   * the slot requester; consume it now so the FU-70 slot points at the taker
   * before the first frame body (the native rebinds `[0x57C64]` in place). */
  match_run_entity_drain(mr);
  /* FU-96 leg 5 (M2 interactive T2): the per-record camera place
   * `FUN_00079F3C` runs inside the native `FUN_0008CF60` loop between the
   * phase handler and the `FUN_00079B6C` commit, on the non-controlled team's
   * records only, snapping targets within 0x180 of the camera onto the
   * 0x180 ring. The commit leaves targets untouched, so this whole-pool pass
   * before the commit pass is observationally identical. The kickoff camera is
   * the reset triple (0,0,0) and the live phase is 1 (both gates pass). The
   * resource-less degradation (no formation seed) keeps the documented zero
   * targets: the native phase handler always precedes the place, so a zero
   * target is an engine-only state and must not fabricate a ring position. */
  if (formation_seeded)
    (void)fifa96_match_entities_camera_place(&mr->entities,
                                             mr->phase_machine.side_controlled,
                                             (uint8_t)mr->state.phase,
                                             mr->render.camera.pos_x,
                                             mr->render.camera.pos_z);
  /* FU-89 §kickoff placement / OL-T11-8: the derived kickoff pass after the
   * camera reset (native `FUN_00088DC8` stage 0 order: `FUN_000700F4` camera
   * -> `FUN_00073E08` placement). The act-1 ball spawn (0x1E0/0), the
   * `FUN_00079B6C` per-record commit, the camera-vs-target face and the
   * `0x79C13` selector run over the fresh pool; the kickoff camera is the
   * `[0x10F328/2C/30]` reset triple the engine's fresh camera models. */
  (void)fifa96_match_entities_kickoff_place(&mr->entities, mr->render.camera.pos_x,
                                            mr->render.camera.pos_y,
                                            mr->render.camera.pos_z);
  if (eng->surface) {
    /* FU-92: the derived window setter clamps to the surface; the live match's
     * 160x100 sequence / zoom window selection is an open leg, so the fresh
     * match starts from the full surface window (define_full sets the screen
     * dimensions and applies the window). */
    (void)fifa96_window_define_full(&mr->render.window, eng->surface->width,
                                    eng->surface->height);
  }
  int rc = fifa96_match_lifecycle_begin(&mr->lc, &mr->backend, &mr->pace, selector);
  if (rc != 0) {
    mr->engine = NULL;
    return rc;
  }
  mr->running = 1;
  eng->match = mr;
  eng->mode = FIFA96_ENGINE_MODE_MATCH;
  /* FU-62 §4.6: the state-init zero length would complete a period on the
   * first second, so begin installs the derived lengths for the selector. */
  uint16_t period = selector == 0 ? FIFA96_MATCH_RUN_PERIOD_SECONDS_DEFAULT
                                  : FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET;
  uint16_t extra = selector == 0 ? FIFA96_MATCH_RUN_EXTRA_SECONDS_DEFAULT
                                 : FIFA96_MATCH_RUN_EXTRA_SECONDS_RESET;
  (void)fifa96_match_run_set_period(mr, period, extra);
  /* FU-146 S3 / the match-screen installer callers (FUN_00038630 0x38DCC,
   * FUN_0003BB1C 0x3BF9E): the derived match-screen install. leg 0 / mode 0 /
   * side 0 are the derived front-end defaults (the [0x14AF7C]/[0x14AF74]/
   * [0x14AF60] producers are FU-146 legs 1/3). It arms the goal-screen step
   * machine so the frame scheduler consumes queued goals. */
  (void)fifa96_match_run_screen_install(mr, 0, 0, 0);
  return 0;
}

int fifa96_match_run_set_period(struct fifa96_match_run *mr, uint16_t period_seconds,
                                uint16_t extra_seconds) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->state.period_length = period_seconds;
  mr->state.extra_length = extra_seconds;
  return 0;
}

/* FU-148 §3 (S4): the formation-id writer `FUN_0008EA70(side, id)`:
 * `(&0x14C1E4)[side] = id; FUN_0007412C();`. The consumer (FUN_0007412C ->
 * FUN_0008CE78/FUN_0006D9C4) installs the 0x11033A layout into the team block
 * (a leg: the engine has no team+0x7AE/record+0x90 role fields); the id is
 * observable through `formation[]` and the layout through
 * `fifa96_match_formation_layout`. The native writer does not range-check the
 * side (an OOB write); the port hardens. */
int fifa96_match_run_set_formation(struct fifa96_match_run *mr, uint32_t side,
                                   uint8_t id) {
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->formation[side] = id;
  return 0;
}

/* The FU-72 §2.4 increment only. Kept for the paths whose native writers stay
 * unported (the period-indexed goal-screen handler cluster and its situation
 * queue, OL-77/OL-87): gameplay paths have no derived writer invocation yet.
 * The derived full writer is fifa96_match_run_score_event below. */
int fifa96_match_run_add_goal(struct fifa96_match_run *mr, uint32_t side) {
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->score[side]++;
  return 0;
}

int fifa96_match_run_score_event(struct fifa96_match_run *mr, uint32_t side, uint8_t probe) {
  fifa96_action_score state;
  fifa96_action_score_out out;
  fifa96_err_t rc;
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  state.score[0] = mr->score[0];
  state.score[1] = mr->score[1];
  state.last_side = mr->score_last_side;
  state.tracked_side = mr->score_tracked_side;
  state.max_diff = mr->score_max_diff;
  rc = fifa96_action_score_event(&state, side, probe, &out);
  if (rc != FIFA96_OK) return (int)rc;   /* run untouched on a writer failure */
  mr->score[0] = state.score[0];
  mr->score[1] = state.score[1];
  mr->score_last_side = state.last_side;
  mr->score_tracked_side = state.tracked_side;
  mr->score_max_diff = state.max_diff;
  mr->score_last_event = out.posted ? out.post_id : 0u;
  return 0;
}

/* FU-145 §3 item 3 / 0x740F6 (S2): the shared phase write. Every engine
 * phase write goes through the FU-62 setter and mirrors the FU-142a machine
 * (the native [0x157A4D] switch byte is the [0x157A4A] phase dword's high
 * byte). When the new phase is 2, the native FUN_000740A0 (0x740E0..0x740F6)
 * clears the pan arm [0x15781D]; the port also clears the zone byte (the
 * native leaves [0x15781E] alone, but it is consumed only while armed and
 * the camera reset 0x7026C refreshes it, so the extra clear is
 * observer-clean). Returns 0 or the setter's error. */
static int match_run_write_phase(struct fifa96_match_run *mr, uint8_t phase) {
  int rc = fifa96_match_state_set_phase(&mr->state, phase);
  if (rc != 0) return rc;
  mr->phase_machine.state = phase;
  mr->phase_machine.phase = phase;
  if (phase == 2u) {
    mr->goal_armed = 0;
    mr->goal_zone = 0;
  }
  return 0;
}

/* FU-143 wiring (M2 playability Task 3): the engine-side phase driver. The
 * native funnel is FUN_0008AF38's second rollover: when the phase-class gate is
 * open (class 1 always; class 2 while [0x14C302]==0; class 0 stops the clock;
 * first-hand 0x8AF41..0x8AF80) and the derived completion test
 * `period_seconds == limit + aux_seconds` holds (first-hand 0x8B1EA/0x8B21D;
 * the FU-62 library evaluates it and fifa96_match_run_frame stages the result
 * in `clock_period_ended`), the rollover calls FUN_0008B9CC (first-hand
 * 0x8B574) with the completed period, then zeroes [0x57AB6] and increments
 * [0x157AC2] (0x8B583/0x8B58A). FUN_0008B9CC's derived port is
 * fifa96_action_phase_period_end: under the selector-0/no-extra-time default
 * (extra_time clear, period < 4) it derives phase 0x0C on the controlled side
 * and act 0xB (first-hand 0x8BAA7..0x8BABE, 0x8BADB..0x8BAE7; the native then
 * invokes the act selector, whose handler body is unported). The
 * driver applies the gate to the live phase (fifa96_action_phase_row) and the
 * chooser to the completed period `state.period - 1` (the FU-62 library
 * already incremented the period; the native chooser sees [0x157AC2] before
 * its 0x8B58A increment). Chooser inputs whose native producers are unported
 * are passed as their derived defaults: extra_time 0 ([0x157AC0], the
 * selector-0 default; its writer is a period-1 completion side effect), side
 * 0x157ABE/0x157ABF 0 (OL-75) and the d8/d9 counters 0, plus the zeroed
 * 0x12230/0x12250 probes (OL-74). The post-call act body, the phase-0xC
 * machine FUN_0008BAF0 and the FUN_0004B02C(0)/FUN_00088860 kickoff reset stay
 * unported (FU-143 §4/§5), so the engine lifecycle owns the exit; the
 * 0x0C -> 0 reset is the run_end teardown. */
int fifa96_match_run_phase_drive(struct fifa96_match_run *mr) {
  const fifa96_action_phase_row_desc *row;
  fifa96_action_phase_period_end_out out;
  uint8_t probe[4] = { 0u, 0u, 0u, 0u };
  uint8_t side;
  uint8_t ended;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* One-shot: the staged completion belongs to the latest granted frame. */
  ended = mr->clock_period_ended;
  mr->clock_period_ended = 0;
  row = fifa96_action_phase_row(mr->state.phase);
  if (!row) return 0;   /* native has no bounds check; the port hardens (FU-83) */
  /* Class gate: only class 1 (always) and class 2 (halt gate clear) can
   * complete a period; class 0 stops the clock. */
  if (row->clock_class != 1u && row->clock_class != 2u) return 0;
  if (!ended) return 0;
  if (mr->state.period == 0u) return 0;   /* no completed period to choose */
  side = mr->phase_machine.side_controlled;
  rc = fifa96_action_phase_period_end((uint8_t)(mr->state.period - 1u), 0u,
                                      mr->score[side & 1u],
                                      mr->score[(side ^ 1u) & 1u], side, 0u, 0u, 0u, 0u,
                                      probe, &out);
  if (rc != FIFA96_OK) return rc;
  if (out.phase != FIFA96_ACTION_PHASE_NONE) {
    rc = match_run_write_phase(mr, out.phase);
    if (rc != 0) return rc;
  }
  return 1;
}

/* ===== FU-149 P1 — set pieces & restarts ================================== */

/* The `FUN_00079CCC` pick over the derived pool (FU-149 §1.5, L7; the FU-143
 * §11.1 kickoff precedent): the record of `team` nearest `(from_x, from_z)`
 * over the records' target triples, skipping array index 0 and the
 * `+0x98`/`+0x9A` exclusions with the native strict minimum; the native
 * no-candidate return is the team base (record 0, `0x79CE4`/`0x79D4F`). The
 * native per-record distance source is the record's phase handler
 * `[rec+0x1C]` placement output; the phase handlers are unported, so the
 * target triple stands in (same ruling as the kickoff pick). */
static int32_t match_run_phase_pick(const struct fifa96_match_run *mr, uint32_t team,
                                    int16_t from_x, int16_t from_z) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  int index;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    candidates[i].x = (int16_t)e->target_x;
    candidates[i].y = (int16_t)e->target_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  index = fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                     from_x, from_z, &best);
  return index >= 0 ? (int32_t)index : 0;
}

int fifa96_match_run_phase_arm(struct fifa96_match_run *mr, uint8_t phase) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (phase != 3u && phase != 4u && phase != 6u && phase != 7u &&
      phase != 8u && phase != 9u && phase != 0x0Du)
    return 0;
  /* The native FUN_000740A0 writes [0x157A4A] before FUN_0008D098, and the
   * installs read the latched phase (`FUN_0008CEB8` passes [0x157A4A]>>24). */
  mr->entities.phase = phase;
  if (phase == 0x0Du) {
    /* 0x8D65D: no install-3 prefix (first-hand, unlike the set-piece arms):
     * controlled -> code 0 over records 1..10 (record 0 kept), the other team
     * -> code 0 over records 0..10 (0x8D63E, the shared 0xB/0xE tail). */
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
      uint8_t controlled = (uint8_t)mr->phase_machine.side_controlled ==
                                   mr->entities.team[t].side
                               ? 1u
                               : 0u;
      (void)fifa96_match_arm_install_multi(&mr->entities, t,
                                           controlled != 0u ? 1u : 0u, 10, 0, -1);
    }
    return 0;
  }
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    struct fifa96_match_team *team = &mr->entities.team[t];
    uint8_t controlled =
        (uint8_t)mr->phase_machine.side_controlled == team->side ? 1u : 0u;
    (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, 3, -1);
    if (phase == 7u) {
      /* 0x8D57B: controlled only; probe = the incident triple (the FU-150
       * foul adjudicator is the producer, P2). */
      if (controlled != 0u) {
        int32_t idx = match_run_phase_pick(mr, t, (int16_t)mr->incident_x,
                                           (int16_t)mr->incident_z);
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
        (void)fifa96_match_entities_install(&team->records[idx], phase, 0x12u, 0);
      }
      continue;   /* 0x8D5A2 JNZ -> the non-controlled early return 0x8D814 */
    }
    if (phase == 6u) {
      /* 0x8D4A2: the FUN_00073DC4 penalty spot (0, 0, ±0x8D0) is the probe and
       * the camera reset; the non-controlled team arms its own keeper 0x1F. */
      if (controlled != 0u) {
        int32_t pz = team->side == 1u ? -0x8D0 : 0x8D0;
        int32_t idx;
        (void)fifa96_camera_init(&mr->render.camera, 0, 0, pz);
        idx = match_run_phase_pick(mr, t, 0, (int16_t)pz);
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
        (void)fifa96_match_entities_install(&team->records[idx], phase, 0x13u, 0);
      } else {
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS);
        (void)fifa96_match_entities_install(&team->records[0], phase, 0x1Fu, 0);
      }
      continue;
    }
    if (phase == 8u || phase == 9u) {
      /* 0x8D5D9: the controlled team's team base is record 0 (the keeper);
       * code = (phase == 9) ? 0x1E : 0x1D (0x8D607..0x8D62C). */
      if (controlled != 0u) {
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS);
        (void)fifa96_match_entities_install(&team->records[0], phase,
                                            phase == 9u ? 0x1Eu : 0x1Du, 0);
      }
      continue;
    }
    if (controlled == 0u) continue;   /* phases 3/4: 0x8D814 early return */
    if (phase == 3u) {
      /* 0x8D2A3: camera reset to the goal snapshot triple (0x15777C/80/84),
       * chosen = FUN_00079CCC(0x15777C), install 0x10 (0x8D32A..0x8D349). */
      int32_t idx = match_run_phase_pick(mr, t, (int16_t)mr->goal_snap_x,
                                         (int16_t)mr->goal_snap_z);
      (void)fifa96_camera_init(&mr->render.camera, mr->goal_snap_x,
                               mr->goal_snap_y, mr->goal_snap_z);
      team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
      (void)fifa96_match_entities_install(&team->records[idx], phase, 0x10u, 0);
    } else {
      /* 0x8D35B: probe = FUN_0007D360(snapshot) = (±0x710, 0, ±0xB00) from the
       * snapshot sign bits; camera reset to the probe; the 0x92AC8 & 3 draw is
       * consumed (the 0x1A event sink is dropped, L7); install 0x11. */
      int32_t px = mr->goal_snap_x < 0 ? -0x710 : 0x710;
      int32_t pz = mr->goal_snap_z < 0 ? -0xB00 : 0xB00;
      int32_t idx;
      uint16_t r = 0;
      (void)fifa96_camera_init(&mr->render.camera, px, 0, pz);
      idx = match_run_phase_pick(mr, t, (int16_t)px, (int16_t)pz);
      team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
      (void)fifa96_rng_step(&mr->rng, &r);
      (void)fifa96_match_entities_install(&team->records[idx], phase, 0x11u, 0);
    }
  }
  return 0;
}

/* The FU-149 shared table-2 row runner: the derived `fifa96_action_phase_situation`
 * outcome (phase/act/stage/flags + the sit-3 counter request), the phase write
 * and the FU-149 phase arm. `count_corner` enables the corner-counter
 * increment; only the dispatcher head knows the side (the side-less shared
 * entry passes 0). */
static int match_run_situation_side(struct fifa96_match_run *mr, uint8_t situation,
                                    uint8_t side, int count_corner) {
  fifa96_action_phase_situation_out out;
  fifa96_err_t rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  rc = fifa96_action_phase_situation(situation, &out);
  if (rc != FIFA96_OK) return (int)rc;
  if (count_corner != 0 && out.corner_increment != 0u)
    mr->corner_count[(uint32_t)(side ^ mr->side_swap) & 1u] += 1u; /* 0x8ABFF/0x8AC0F */
  /* The native table-2 arm runs FUN_000740A0 at the phase-writing rows (which
   * also runs the per-team FUN_0008D098 arm); the act-handler invocation
   * (FUN_000888FC, the unported phase-row bodies, OL-13/OL-78) stays
   * unported. */
  if (out.phase != FIFA96_ACTION_PHASE_NONE) {
    rc = (fifa96_err_t)match_run_write_phase(mr, out.phase);
    if (rc != 0) return (int)rc;
    rc = (fifa96_err_t)fifa96_match_run_phase_arm(mr, out.phase);
    if (rc != 0) return (int)rc;
  }
  /* FU-150 P2: the act row's FUN_000888FC invocation (invoke-now). Only act 2
   * (the sit-9/sit-0xA rows) has a ported body: the derived phase-0x18
   * free-kick/penalty hand-off machine, seeded with the row's stage (0 for
   * sit 9, 1 for sit 0xA). Other acts stay unported (OL-13). */
  if (out.act == 2u) {
    mr->ref_machine = FIFA96_MATCH_RUN_REF_RESTART;
    mr->ref_restart_stage = out.stage;
    rc = (fifa96_err_t)fifa96_match_run_referee_step(mr);
    if (rc != 0) return (int)rc;
  }
  return 0;
}

int fifa96_match_run_situation(struct fifa96_match_run *mr, uint8_t situation) {
  return match_run_situation_side(mr, situation, 0u, 0);
}

/* FU-145 §1.3 (first-hand /FIFA96.EXE this slice): the armer's absolute value
 * is stored as a word and re-read sign-extended for the comparison
 * (`0x71354..0x71384`: `abs` in 32 bits, `MOV word[ESP+..],AX`, then
 * `MOV EAX,[ESP+..]; SAR EAX,0x10`). A magnitude whose low word sign-extends
 * negative therefore fails every positive bound — the native trap. */
static int16_t match_run_goal_abs_word(int32_t value) {
  uint32_t magnitude = (uint32_t)value;
  if (value < 0) magnitude = 0u - magnitude;
  return (int16_t)(uint16_t)magnitude;
}

int fifa96_match_goal_zone(int32_t x, int32_t y, int32_t z) {
  uint32_t magnitude = (uint32_t)z;
  int32_t zw;
  int32_t bits = 0;
  int32_t h;
  int32_t d;
  if (z < 0) magnitude = 0u - magnitude;
  zw = (int16_t)(uint16_t)magnitude;       /* 0x70084 MOVSX DX */
  if (zw < 0xB10) bits = 8;                /* 0x70087..0x70093 */
  else if (zw >= 0xB90) bits = 4;          /* 0x70095..0x700A1 */
  if (x < -0xD0) bits |= 1;                /* 0x700A7..0x700B1 */
  else if (x >= 0xD0) bits |= 2;           /* 0x700B3..0x700BB */
  d = (int16_t)(uint16_t)((uint32_t)magnitude - 0xB10u);  /* 0x700BD..0x700C3 */
  if (d <= 0x30) h = 0xA0;                 /* 0x700C6..0x700D9 */
  else h = 0xA0 - (d - 0x30);              /* 0x700CB..0x700D7 */
  if (h < y) bits |= 0x10;                 /* 0x700DE..0x700E3 */
  return bits == 0;                        /* 0x700E8..0x700EB TEST/SETZ */
}

int fifa96_match_goal_arm(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (mr->goal_armed) {
    /* The already-armed reflect arm `0x718A9..0x7190E`: zone != 0 takes the
     * unported angle arm (FU-71 leg 9.6) with no clear; zone == 0 and the
     * reflect input bit set mirror the camera and clear both arm flags. The
     * native clear happens before the mirror body; the ported
     * fifa96_camera_reflect carries the same `0xB10`/`0x720` gate. */
    if (mr->goal_zone == 0) {
      if (fifa96_camera_reflect(&mr->render.camera, mr->render.input_bit0, 0) != 0) {
        mr->goal_armed = 0;
        mr->goal_zone = 0;
        return 1;
      }
    }
    return 0;
  }
  /* 0x71390..0x713A0: phase 2/0x10 only. */
  if (mr->state.phase != 2u && mr->state.phase != 0x10u) return 0;
  /* 0x713A6..0x713C0: |camZ|_w > 0xB20 || |camX|_w > 0x730 (the word
   * truncation trap is reproduced by match_run_goal_abs_word). */
  if (!(match_run_goal_abs_word(mr->render.camera.pos_z) > (int16_t)0xB20 ||
        match_run_goal_abs_word(mr->render.camera.pos_x) > (int16_t)0x730))
    return 0;
  /* 0x713DB..0x713EC: arm, freeze the snapshot triple (y forced 0), then
   * 0x713F2 the goal-mouth classifier over the live camera triple. The
   * native also latches [0x157ACB] at 0x713E6 (leg L7: dropped, its only
   * consumer is the unported 0x8FCC8 read). */
  mr->goal_armed = 1u;
  mr->goal_snap_x = mr->render.camera.pos_x;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = mr->render.camera.pos_z;
  mr->goal_zone = (uint8_t)fifa96_match_goal_zone(mr->render.camera.pos_x,
                                                  mr->render.camera.pos_y,
                                                  mr->render.camera.pos_z);
  return 1;
}

/* FU-145 §1.6 (S2): the scanner's derived possession nearest — the native
 * `FUN_0008DE8C(&0x15777C, team_block + side*0x835, 0, NULL)` call at
 * `0x88ADF`-family (EBX = 0 -> record 0 skipped), over the pool records'
 * (x,z) words with the shared `fifa96_entity_distance` metric (the FU-143
 * §11.1 row-01 precedent). The selected record's `+0x826` team byte is the
 * queued side; it equals the searched block's side, so the result itself
 * only feeds the unported announce/outcome sinks (L2/L3). */
static int32_t match_run_goal_nearest(const struct fifa96_match_run *mr, uint32_t team,
                                      int32_t from_x, int32_t from_z) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  return fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                    (int16_t)from_x, (int16_t)from_z, &best);
}

/* FU-146 §7 item 1 / FU-149 §1.1 (0x8AC28/0x8AC88..0x8ADB4): the situation-6
 * direct fallback = the score increment plus the table-2 row 6 phase-5 write
 * through the shared entry. The native direct arm skips the phase write when
 * [0x157AC2] (the period counter) is 2 or 3 (0x8AD96/0x8AD9F); the score
 * increment still runs. */
static int match_run_goal_fallback(struct fifa96_match_run *mr, uint8_t side) {
  int rc = fifa96_match_run_add_goal(mr, side);
  if (rc != 0) return rc;
  if (mr->global_157ac2 == 2u || mr->global_157ac2 == 3u) return 0;
  return fifa96_match_run_situation(mr, 6);
}

int fifa96_match_run_goal_queue(struct fifa96_match_run *mr, uint8_t side) {
  /* FU-149 P1 freeze reconciliation: the situation-6 entry is the generic
   * dispatcher head (queue ids 5/6, the [0x15B6B8] side latch) with the goal
   * fallback on the direct path — one shared implementation. */
  return fifa96_match_run_set_piece(mr, 6u, side, 0u);
}

int fifa96_match_run_set_piece(struct fifa96_match_run *mr, uint8_t situation,
                               uint8_t side, uint8_t bx) {
  int rc;
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  /* 0x8A944..0x8A96B: situation 0 and 0xB, a closed session gate or a pending
   * situation take the direct path; else the table-1 queue arm. */
  if (situation != 0u && situation != 0x0Bu && mr->session_gate_14c32a &&
      !mr->situation_pending) {
    /* 0x8A971..0x8A996: [0x15B6B8] = (side == 0) then the table-1 id
     * (0x8A8E0) and the pending latch. The id space is the FU-146 S3 screen
     * machinery's (the FU-149 L4 boundary); the head only writes it. */
    mr->sit_side_pending = side == 0u ? 1u : 0u;
    switch (situation) {
      case 2u: case 3u: case 4u:
        mr->situation_id = side == 0u ? 9u : 0u;
        break;
      case 5u: case 7u:
        mr->situation_id = 7u;
        break;
      case 6u:
        mr->situation_id = side == 0u ? 5u : 6u;
        break;
      case 9u: case 10u:
        mr->situation_id = side == 1u ? 1u : 2u;
        break;
      default:   /* sit 1, 8, 0xC and >10: the 0x8AA60 default id 0xA */
        mr->situation_id = 0x0Au;
        break;
    }
    mr->situation_pending = 1u;
    return 0;
  }
  /* 0x8AA7B: the direct path. */
  if (bx != 0u) {
    /* 0x8AA80..0x8AAA3: FUN_000740A0(0, 0) then the act-8 replay bytes, then
     * act 8's tail (0x8A8A5..0x8A8DE): re-dispatch the stored situation/side
     * with BX=0 and leave [0x15882B] = 0xFF. The act-8 (phase-0x1E) timeline
     * stages around the re-dispatch stay unported (FU-149 L6), so the port
     * compresses them to the re-dispatch call. */
    mr->store_15882c = side;
    mr->store_15882b = situation;
    rc = match_run_write_phase(mr, 0u);
    if (rc != 0) return rc;
    rc = fifa96_match_run_set_piece(mr, mr->store_15882b, mr->store_15882c, 0u);
    mr->store_15882b = 0xFFu;
    return rc;
  }
  /* 0x8AAA8..0x8AB7A: the BX==0 table-2 route. The native sit 2..4
   * [0x157B8E]/[0x157B8F] formation-order gate + act-4 arm (0x8AB08..0x8AB62)
   * and the sit-1 arm (0x8AABF..0x8AB06) are unported act-handler machinery
   * (FU-149 L12); the derived route is the table-2 row. Situation 6 keeps
   * its score fallback (0x8AC28); situation 3 counts before the write. */
  if (situation == 6u) return match_run_goal_fallback(mr, side);
  return match_run_situation_side(mr, situation, side, 1);
}

/* FU-145 L4 / FU-149 §1.3: the `[0x1577CA]` ball-record team-byte source is
 * unported; the derived stand-in is the pool controlled actor ([0x157A83],
 * the row-04 convention), then the ball carrier, else side 0. */
static uint8_t match_run_ball_side(const struct fifa96_match_run *mr) {
  int32_t id = mr->entities.controlled;
  if (id < 0) id = mr->entities.ball.carrier;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0u;
  return mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS].side;
}

int fifa96_match_run_goal_scan(struct fifa96_match_run *mr) {
  int32_t abs_z;
  int32_t side;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* 0x8B623..0x8B643: phase 2/0x10 and armed. The native period-4 branch
   * (0x8B60D..0x8B621) joins the tail without the scan only on the
   * extra-time path ([0x157AC0] != 0); the engine carries extra_time 0
   * (FU-143 OL-85), so the skip is unreachable and unmodelled (leg L8). */
  if (mr->state.phase != 2u && mr->state.phase != 0x10u) return 0;
  if (!mr->goal_armed) return 0;
  /* 0x8896B..0x88983: |snapshot z| (full 32-bit magnitude; the native NEG
   * wraps INT_MIN to itself) <= 0xB20 selects the throw-in arm 0x88BCC. */
  {
    uint32_t mag = (uint32_t)mr->goal_snap_z;
    if (mr->goal_snap_z < 0) mag = 0u - mag;
    abs_z = (int32_t)mag;   /* 0x80000000 wraps to INT_MIN like the native */
  }
  if (abs_z <= 0xB20) {
    /* 0x88BCC..0x88C00: the throw-in arm (phase-2 only, 0x88BD4). EDX = the
     * ball record's team ^ 1, BX=1 — the head queues id 9/0 or, when the gate
     * is closed/a situation is pending, takes the phase-0 + act-8 fallback to
     * the table-2 phase 3. The 0x974DC(0x1E) scanner sound is dropped (L7). */
    int rc;
    if (mr->state.phase != 2u) return 0;
    rc = fifa96_match_run_set_piece(mr, 2u,
                                    (uint8_t)(match_run_ball_side(mr) ^ 1u), 1u);
    if (rc != 0) return rc;
    return 1;
  }
  if (mr->goal_zone == 0) {
    /* 0x88989/0x88990 + 0x88B53..0x88BBD: the corner/goal-kick arm (phase-2
     * only, 0x88B5B): situation = 3 + ((snap z < 0) == (ball team == 1)) with
     * EDX = ball team ^ 1 and BX=0. Under the end convention `team 0 defends
     * −z`: cond false -> sit 3 (corner), true -> sit 4 (goal kick). */
    int rc;
    uint8_t ball;
    uint8_t cond;
    if (mr->state.phase != 2u) return 0;
    ball = match_run_ball_side(mr);
    cond = ((mr->goal_snap_z < 0) == (ball == 1u)) ? 1u : 0u;
    rc = fifa96_match_run_set_piece(mr, (uint8_t)(3u + cond),
                                    (uint8_t)(ball ^ 1u), 0u);
    if (rc != 0) return rc;
    return 1;
  }
  /* 0x88996..0x889C5: side select. The native tests the goal-side flag
   * [0x157A4C]; when it is not 1 the side is the snapshot sign (SETL at
   * 0x889BD). The flag's writers/record identity ([0x1587D4]) are unported
   * (leg L4), so the derived side is always the snapshot sign. */
  side = mr->goal_snap_z < 0 ? 1 : 0;
  /* 0x889C7..0x88B31: the possession nearest over the side's team block;
   * the sinks (0x795B4/0x79C50/0x6E598/0x741B4/0x651F0/0x974F0) stay legs. */
  (void)match_run_goal_nearest(mr, (uint32_t)side, mr->goal_snap_x, mr->goal_snap_z);
  /* 0x88B37/0x88B42/0x88B44: MOV EAX,6; XOR EBX,EBX;
   * CALL FUN_0008A938(6, record_side, 0) — the queued side is the selected
   * record's team byte, which is the searched block's side. */
  {
    int rc = fifa96_match_run_goal_queue(mr, (uint8_t)side);
    if (rc != 0) return rc;
  }
  return 1;
}

/* ------------------------------------------------------------------------- *
 * FU-146 S3 — the goal-consumer chain: the FUN_000CBC4C probe, the installed
 * 0x110F78 period handlers, FUN_000948AC scheduler, FUN_000935A0 advance and
 * the FUN_00092D8C/FUN_00092E2C installer.
 * ------------------------------------------------------------------------- */

uint8_t fifa96_match_run_goal_probe(struct fifa96_match_run *mr) {
  uint32_t *limb;
  uint32_t eax;
  uint64_t wide;
  uint32_t carry;
  if (!mr) return 0;
  limb = mr->goal_probe_limb;              /* C0..C5 (0x112E68..0x112E7C) */
  /* 0xCBC4C..0xCBC83: the fold chain EAX = C5; EAX += C4; C4 = EAX; ADC ... */
  wide = (uint64_t)limb[5] + limb[4];
  limb[4] = (uint32_t)wide;
  carry = (uint32_t)(wide >> 32);
  for (int i = 3; i >= 0; i--) {
    wide = (uint64_t)limb[i + 1] + limb[i] + carry;
    limb[i] = (uint32_t)wide;
    carry = (uint32_t)(wide >> 32);
  }
  eax = limb[0];
  /* 0xCBC88..0xCBCB6: C5++, then the wrap carry propagates up through C0 */
  for (int i = 5; i >= 0; i--) {
    limb[i]++;
    if (limb[i] != 0u) return (uint8_t)eax;
  }
  eax++;                                   /* the carry out of C0 */
  return (uint8_t)eax;
}

/* FU-146 §4: the six handler step shapes (the native step tables at 0x93B80,
 * 0x93DE4, 0x94074, 0x94234, 0x944D4, 0x946B4). Every step advance zeroes the
 * handler timer. */
enum {
  MATCH_RUN_SCREEN_SETUP0 = 0,      /* the per-leg first step (falls through) */
  MATCH_RUN_SCREEN_SETUP0_RET,      /* leg 2's first step (native RET 0x9413E) */
  MATCH_RUN_SCREEN_GATE,            /* the [0x15882A] gate (legs 0/1/3) */
  MATCH_RUN_SCREEN_SETUP2,          /* the leg-0/1/3 second setup step */
  MATCH_RUN_SCREEN_ADVANCE,         /* leg 2's bare step advance (0x9413F) */
  MATCH_RUN_SCREEN_PHASE,           /* phase-2 latch clear */
  MATCH_RUN_SCREEN_POST,            /* the latch-gated consume step */
  MATCH_RUN_SCREEN_TAIL             /* timer > 0xB4 -> FUN_000935A0 */
};
static const uint8_t match_run_screen_kinds[6][6] = {
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_GATE, MATCH_RUN_SCREEN_SETUP2,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_GATE, MATCH_RUN_SCREEN_SETUP2,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL},
    {MATCH_RUN_SCREEN_SETUP0_RET, MATCH_RUN_SCREEN_ADVANCE,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL, 0u},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_GATE, MATCH_RUN_SCREEN_SETUP2,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST,
     MATCH_RUN_SCREEN_TAIL, 0u, 0u},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST,
     MATCH_RUN_SCREEN_TAIL, 0u, 0u},
};
static const uint8_t match_run_screen_step_count[6] = {6u, 6u, 5u, 6u, 4u, 4u};

/* FU-146 §4: the per-leg id tables (0x93B98, 0x93DFC, 0x94088, 0x9424C,
 * 0x944E4). Leg 5 has no table and no bound: `0x94845 CMP EDX,5 / JNZ
 * 0x9485C` -> `0x94869 MOV EAX,1`, so id 5 -> side 0 and every other id
 * (including 0 and >= 7) -> side 1, with no no-score path; the post handles
 * leg 5 directly. -1 = the no-score counter (legs 0-4 only). The
 * `id - 1 > count - 1 -> no-score` bound is each table's own dispatch; the
 * `0x94655/0x94677` bound belongs to leg 4's table, not leg 5. */
#define MATCH_RUN_SCREEN_NO_SCORE (-1)
static const int8_t match_run_screen_ids_leg0[9] = {1, 0, 1, -1, 0, -1, -1, -1, 1};
static const int8_t match_run_screen_ids_leg1[9] = {1, 0, 1, -1, 0, -1, -1, -1, 1};
static const int8_t match_run_screen_ids_leg2[7] = {1, 0, 1, 0, 0, 1, 0};
static const int8_t match_run_screen_ids_leg3[9] = {1, 0, 1, -1, 0, 1, -1, -1, 1};
static const int8_t match_run_screen_ids_leg4[6] = {1, 0, -1, -1, 0, 1};
static const int8_t *const match_run_screen_ids[6] = {
    match_run_screen_ids_leg0, match_run_screen_ids_leg1,
    match_run_screen_ids_leg2, match_run_screen_ids_leg3,
    match_run_screen_ids_leg4, NULL,   /* leg 5: no table (see above) */
};
static const uint8_t match_run_screen_id_count[6] = {9u, 9u, 7u, 9u, 6u, 0u};

/* FU-146 §3: the per-mode duration table `0x1110EC[mode*24 + leg]` dwords
 * (first-hand read): modes 0..2 are `{15,15,30,30,60,5}` seconds and mode 3 is
 * `{5,1,5,1,3,2}`; `screen_install` seeds `value * 60` frames (0x9410A..0x94111
 * `*15 *4`). */
static const uint16_t match_run_screen_durations[4][6] = {
    {15u, 15u, 30u, 30u, 60u, 5u},
    {15u, 15u, 30u, 30u, 60u, 5u},
    {15u, 15u, 30u, 30u, 60u, 5u},
    {5u, 1u, 5u, 1u, 3u, 2u},
};

/* The handler post step (the native 0x93D2A/0x94179/0x94219/0x9441B/0x945F9/
 * 0x947EF bodies): consume the pending situation through the per-leg id table.
 * Returns 1 when the step ran (the caller advances), 0 when the latch is clear
 * (native JZ epilogue), or a negative -fifa96_err_t. */
static int match_run_screen_post(struct fifa96_match_run *mr) {
  const int8_t *ids = match_run_screen_ids[mr->screen_leg];
  uint8_t count = match_run_screen_id_count[mr->screen_leg];
  uint8_t id = mr->situation_id;
  int32_t tracked = mr->score_tracked_side;
  int32_t side = MATCH_RUN_SCREEN_NO_SCORE;
  uint8_t probe = 0;
  int rc;
  if (!mr->situation_pending) return 0;       /* 0x93D31 JZ epilogue */
  mr->situation_pending = 1;                  /* 0x93D46 re-latch */
  /* The 0x974DC presentation call (EAX=0x1E, the leg-specific EBX/ECX/EDI
   * args) stays FU-146 §8 leg 8 and is not called. */
  mr->goal_last_id = id;                      /* 0x93D5C [0x15B674] */
  if (mr->screen_leg == 5) {
    /* 0x94824: leg 5 forces the minute 0 and folds 0x400/0x401 into the
     * screen accumulator (0x94834..0x94864). */
    uint32_t base = mr->goal_screen_accum;
    uint32_t plus400 = base + 0x400u;
    uint32_t plus401 = base + 0x401u;
    mr->goal_screen_accum = plus400;
    if ((id == 5u && tracked == 0) || (id != 5u && tracked == 1))
      mr->goal_screen_accum = plus401;
    mr->goal_minute = 0;
  } else {
    mr->goal_minute = (uint16_t)((int32_t)mr->screen_timer / 0x3C);  /* 0x93D66 */
    mr->goal_screen_accum += mr->screen_timer;                       /* 0x93D73 */
  }
  /* The id dispatch. Leg 5 has no table and no bound (0x94845 `CMP EDX,5 /
   * JNZ 0x9485C` -> 0x94869 `MOV EAX,1`): id 5 -> side 0, every other id
   * (including 0 and >= 7) -> side 1. Legs 0-4 use their table with the
   * native `EAX = [0x15B6A8] - 1; CMP EAX,count-1; JA no-score` bound. */
  if (mr->screen_leg == 5) {
    side = (id == 5u) ? 0 : 1;
  } else if ((uint8_t)(id - 1u) < count) {
    side = ids[id - 1u];
  }
  if (side < 0) {
    mr->goal_no_score++;                     /* INC dword [0x15B6A0] */
  } else {
    /* The native writer calls FUN_000CBC4C only on the untracked
     * `score[side] == 1 && score[other] < 3` arm (0x939D1), i.e. with the
     * pre-increment score 0; the probe is computed exactly then. */
    if (tracked != -1 && side != tracked &&
        mr->score[side] == 0u && mr->score[side ^ 1] < 3u)
      probe = fifa96_match_run_goal_probe(mr);
    rc = fifa96_match_run_score_event(mr, (uint32_t)side, probe);
    if (rc < 0) return rc;
  }
  rc = match_run_write_phase(mr, 0);          /* 0x93DB2 FUN_000740A0(0,0) */
  if (rc < 0) return rc;
  return 1;
}

int fifa96_match_run_screen_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (mr->screen_leg < 0 || mr->screen_leg > 5) return 0;   /* no handler */
  /* The handler head (0x93BC4..0x93BD7): [0x15B688] += word[0x157A64]. */
  mr->screen_timer = (uint16_t)(mr->screen_timer + mr->state.frame_delta);
  for (;;) {
    uint8_t kind;
    if (mr->screen_step >= match_run_screen_step_count[mr->screen_leg]) return 0;
    kind = match_run_screen_kinds[mr->screen_leg][mr->screen_step];
    switch (kind) {
      case MATCH_RUN_SCREEN_SETUP0:
        /* The modelled-cell subset of the per-leg first step. Legs 0/1/3 clear
         * the kickoff gate [0x15882A] (0x93C30; legs 1/3 take the clear under
         * the staged [0x15B684]==0 path, FU-146 leg 10); leg 4 clears the pan
         * arm and the snapshot (0x9453C..0x9454E). The presentation/staging
         * remainder (the ±0x720 hint, the 0x10F328/0x15B6C8/0x158897 copies,
         * the situation re-queue, FUN_0004C324) is FU-146 §8 leg 8. */
        if (mr->screen_leg == 0 || mr->screen_leg == 1 || mr->screen_leg == 3) {
          mr->global_5882a = 0;
        } else if (mr->screen_leg == 4) {
          mr->goal_armed = 0;
          mr->goal_snap_x = 0;
          mr->goal_snap_y = 0;
          mr->goal_snap_z = 0;
        }
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_SETUP0_RET:       /* leg 2 0x940D6..0x9413E */
        mr->screen_step++;
        mr->screen_timer = 0;
        return 0;
      case MATCH_RUN_SCREEN_GATE:             /* 0x93C59/0x93EDB/0x9434C */
        if (!mr->global_5882a) return 0;
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_SETUP2:
        /* The leg-0/1/3 re-arm/setup step (0x93C7B/0x93EFB/0x9436E): its
         * staging writes ([0x15781D]=1, the 0x158897 snapshot/camera copies,
         * the clock display words, the situation re-queue) are FU-146 §8
         * leg 8; `screen_install` seeds the duration instead, and the
         * [0x15781D] re-arm is not applied because the 0x158897 snapshot
         * producer is unported (arming with a zero triple would poison the
         * FU-145 camera armer). */
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_ADVANCE:          /* leg 2 0x9413F */
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_PHASE:
        /* 0x93CFE/0x9414D/0x943F1/0x945CF/0x947C3: the phase dword's high byte
         * ([0x157A4A]>>24) is the engine phase. */
        if (mr->state.phase != 2u) return 0;
        mr->situation_pending = 0;            /* the latch clear */
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_POST: {
        int rc = match_run_screen_post(mr);
        if (rc < 0) return rc;
        if (rc == 0) return 0;
        mr->screen_step++;
        mr->screen_timer = 0;
        /* The native falls through into the tail step (0x93DCA); the tail's
         * `timer > 0xB4` test is a guaranteed no-op here because the post zeroed
         * the timer, so returning directly is observationally identical. */
        return 1;
      }
      case MATCH_RUN_SCREEN_TAIL:
        if (mr->screen_timer > 0xB4u)
          return fifa96_match_run_screen_advance(mr);
        return 0;
      default:
        return 0;
    }
  }
}

int fifa96_match_run_screen_advance(struct fifa96_match_run *mr) {
  int32_t total;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  /* 0x935C8..0x935D5: word[score0] + word[score1] (zero-extended words). */
  total = (int32_t)(uint16_t)mr->score[0] + (int32_t)(uint16_t)mr->score[1];
  if (total != mr->goal_log_prev_total) {     /* 0x935E2 CMP; JZ 0x9362B */
    if (total <= 20) {                        /* 0x935E9 JLE: slot = total */
      mr->goal_log[total][0] = mr->score_last_side;
      mr->goal_log[total][1] = (int32_t)mr->goal_last_id;
      mr->goal_log[total][2] = (int32_t)mr->goal_minute;
    } else {
      /* 0x935EB..0x93613: shift slots 0..18 <- 1..19 and write slot 19 */
      for (int32_t i = 0; i < 19; i++) {
        mr->goal_log[i][0] = mr->goal_log[i + 1][0];
        mr->goal_log[i][1] = mr->goal_log[i + 1][1];
        mr->goal_log[i][2] = mr->goal_log[i + 1][2];
      }
      mr->goal_log[19][0] = mr->score_last_side;
      mr->goal_log[19][1] = (int32_t)mr->goal_last_id;
      mr->goal_log[19][2] = (int32_t)mr->goal_minute;
    }
  }
  mr->goal_total = total;                     /* 0x935DC [0x15B69C] */
  mr->goal_log_prev_total = total;            /* 0x93638 [0x15B698] := [0x15B69C] */
  mr->screen_install_hint = 1;                /* 0x936ED [0x15B6C4] */
  mr->screen_timer = 0;                       /* 0x936F3 */
  mr->screen_step = 0;                        /* 0x936FB */
  /* 0x9370A: re-install the same-leg handler and run it once. The 0x9343C /
   * 0x937DC screen installs, the 10/0x14 thresholds and the
   * FUN_0004C324/FUN_00054104 exits stay FU-146 §8 leg 7. */
  return fifa96_match_run_screen_step(mr);
}

int fifa96_match_run_screen_schedule(struct fifa96_match_run *mr) {
  int side1 = 0;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  /* 0x948B2..0x948E0: the screen-over rollover. */
  if (mr->screen_timer > mr->screen_period_frames) {
    mr->situation_id = (uint8_t)(mr->screen_leg != 5 ? 8 : 7);
    mr->situation_pending = 1;
    return fifa96_match_run_screen_step(mr);          /* JMP 0x949E0 */
  }
  if (mr->screen_leg == 2) {                          /* 0x948E5 / 0x9492A */
    if (mr->entities.controlled != FIFA96_MATCH_ENTITY_NONE) {
      uint32_t team =
          (uint32_t)mr->entities.controlled / FIFA96_MATCH_ENTITY_RECORDS;
      side1 = mr->entities.team[team & 1u].side != 0u;
    }
    if (mr->screen_actor_age > 0xF0) {                /* [0x157A97] > 0xF0 */
      mr->situation_id = (uint8_t)(side1 ? 3 : 4);
      mr->situation_pending = 1;
      return fifa96_match_run_screen_step(mr);
    }
  }
  if (mr->screen_leg != 4 && mr->screen_leg != 2 &&   /* 0x9497D..0x949B5 */
      mr->render.camera.pos_z < 0 && mr->screen_lead_z < 0) {
    mr->situation_id = 9;
    mr->situation_pending = 1;
    return fifa96_match_run_screen_step(mr);
  }
  if (mr->screen_leg == 5 && mr->screen_lead_z < 0) { /* 0x949B7..0x949DA */
    mr->situation_id = 7;
    mr->situation_pending = 1;
    return fifa96_match_run_screen_step(mr);
  }
  return fifa96_match_run_screen_step(mr);            /* 0x949E9 CALL */
}

int fifa96_match_run_screen_install(struct fifa96_match_run *mr, int16_t leg,
                                    int16_t mode, int16_t side) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (leg < 0 || leg > 5 || mode < 0 || mode > 3) return -FIFA96_ERR_INVALID;
  (void)side;   /* the tracked-side pick is FU-146 leg 4/10: the carried -1 */
  /* FUN_00092D8C 0x92DC5/0x92DD3: leg/mode, step/timer clear, installer latch.
   * The tracked-side pick ([0x15B684] / [0x1590CC] / [0x159901]) and the
   * 0x10F328 camera copy stay legs. */
  mr->screen_leg = leg;
  mr->screen_mode = mode;
  mr->screen_step = 0;
  mr->screen_timer = 0;
  mr->situation_pending = 1;                  /* 0x92DCD */
  /* FUN_00092E2C: the score pair / writer / goal-log-total resets, the
   * duration seed, the hint clear, then the installed handler runs once
   * (0x92EF7 CALL [0x15B6D4]). The [0x15B6B8] side flag (write-only: fresh
   * xrefs = its two writes), the 0x74034/0x7417C resets (the engine's begin
   * covers the fresh-match camera/slot reset) and the clock display cells stay
   * legs. */
  mr->score[0] = 0;                           /* [0x157AC5] */
  mr->score[1] = 0;                           /* [0x157AC7] */
  mr->score_max_diff = 0;                     /* [0x15B6A4] */
  mr->goal_screen_accum = 0;                  /* [0x15B68C] */
  mr->goal_log_prev_total = 0;                /* [0x15B698] */
  mr->goal_total = 0;                         /* [0x15B69C] */
  mr->screen_period_frames =
      (uint16_t)(match_run_screen_durations[mode][leg] * 60u);
  mr->screen_install_hint = 0;                /* 0x15B6C4 */
  return fifa96_match_run_screen_step(mr);
}

int fifa96_match_run_frame(struct fifa96_match_run *mr) {
  uint32_t pending;
  uint32_t granted;
  int period_ended = 0;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* One 100 Hz pace tick (FU-60), driven from fifa96_match_run_tick:
   * fifa96_match_state_tick consumes the pace grant and, when granted,
   * advances the Q8 clock by one FIFA96_MATCH_STATE_STEP with clock_halt = 0.
   * The pending count is the grant observable, so this reports whether the
   * 30 Hz frame actually ran. */
  pending = fifa96_match_pace_pending(&mr->pace);
  rc = fifa96_match_state_tick(&mr->state, &mr->pace, 0, 0, &period_ended);
  if (rc != 0) return rc;
  granted = fifa96_match_pace_pending(&mr->pace) - pending;
  /* FU-143 wiring (Task 3): stage the FU-62 clock's derived completion
   * (`sec == limit + aux`, fifa96_match_state.c) for the granted frame's
   * phase driver; a completion can only happen on a granted frame. */
  mr->clock_period_ended = (uint8_t)period_ended;
  /* FU-70 §1.1: the slot machine runs from the frame body once per granted
   * 30 Hz frame with the FU-62 §4.3 whole-frame delta (2 at the 0x200 step,
   * i.e. 60 counter units/s). FU-71's FUN_000736AC and the FU-90 display
   * update run on the same frame-body cadence (the original calls the camera
   * from FUN_0004B100, not from the render driver). One pace tick grants at
   * most one frame today; the loop keeps one update per grant if that ever
   * changes. */
  for (uint32_t i = 0; i < granted; i++) {
    /* FU-143 §10/§11 (M2 playable-match Task 2 / OL-84 residual): the act-1
     * (phase-0x17 handler FUN_00088DC8) stage-1 derived remainder. The native
     * body ticks the shared timeline timer [0x58818] by the frame delta and at
     * `[0x58818] >= 0x78` sets [0x5882A]=1 (0x88EF3..0x88F07), the gate action
     * row 01 stage 0 reads (0x7DC6B). begin models the act's stage 0 (phase
     * entry + placement), so only the timer completion is derived here; the
     * engine's `state.tick_total` is the same whole-delta accumulation
     * ([0x58818] += [0x57A64], first-hand FU-81 §1.5) and begin zeroes it. */
    if (mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE &&
        mr->state.tick_total >= 0x78u)
      mr->global_5882a = 1u;
    /* FU-139 §11: the native FUN_0004B100 toggles [0x157A4F] at frame entry
     * (0x4B11A/0x4B129), before the entity chain; row 06's claim arm reads it. */
    mr->pass_parity ^= 1u;
    (void)fifa96_control_slot_update(&mr->slot, mr->input_state[0],
                                     (uint8_t)mr->state.frame_delta, match_run_slot_map,
                                     match_run_anim_a, match_run_anim_b, match_run_anim_c);
    (void)fifa96_camera_update(&mr->render.camera, (int16_t)mr->state.frame_delta,
                               mr->render.view_class, mr->render.input_bit2);
    /* FU-148 §2.1(a)/§6.2 (S4): the FUN_000505D0 pose feed. The native driver
     * FUN_0004D2D4 runs from the draw loop (FUN_000495B0) with the replay/
     * `[0x107DD8]`/pad-idle gates (legs); the engine applies the staged pose
     * here, after the FU-71 update and before the armer, so a fed pose is
     * observable to the same frame's armer (the engine `pos` doubles as the
     * native 0x15774C event target per the FU-71 port mapping). view_mode 0
     * (the fresh-match default) is the unported handler arm: a no-op, so the
     * static tape is unaffected. */
    (void)fifa96_camera_pose_feed(&mr->render.camera, &mr->render.yaw,
                                  &mr->render.pitch, &mr->render.view_ratio,
                                  &mr->render.camera_pose);
    /* FU-145 S2 (0x73B70..0x73B9B): the native camera track FUN_000736AC
     * calls the boundary handler FUN_0007131C only when the camera is out of
     * bounds (|camX| > 0x6C0 || |camZ| > 0xAB0); the armer's own
     * phase/bound gates run inside. Runs after the camera update, before the
     * entity chain (the FUN_000736AC position inside FUN_0004B100). */
    if (fifa96_camera_out_of_bounds(mr->render.camera.pos_x,
                                    mr->render.camera.pos_z))
      (void)fifa96_match_goal_arm(mr);
    (void)fifa96_match_display_update(&mr->render.display, mr->state.frame_delta, 0);
    /* FU-141: the FU-67 entity chain (selection, record walk with the FU-137
     * action dispatch, ball pairing) after the slot/camera updates, matching
     * FUN_0004B100's order. */
    struct fifa96_match_entities_frame ef;
    match_run_entity_frame(mr, &ef);
    rc = fifa96_match_entities_update(&mr->entities, &ef, match_run_dispatch_entity,
                                      mr);
    if (rc != FIFA96_OK) return rc;
    match_run_entity_drain(mr);
    /* FU-150 P2: the referee machine (the native phase-body/act invocation
     * runs inside the entity/phase chain) once per granted frame. Idle when no
     * machine is active, so a fresh run's tape is unchanged. */
    rc = fifa96_match_run_referee_step(mr);
    if (rc < 0) return rc;
    /* FU-142a: the FUN_0008D098 state 0x13/0x14 arm block runs after the
     * FU-141 chain, once per granted frame (the FUN_000740A0 order: the
     * state byte is set, then FUN_0008D098 runs for both teams). */
    if (mr->state.phase == 0x13u || mr->state.phase == 0x14u)
      (void)fifa96_match_phase_machine_step(mr);
    /* FU-146 §7 item 7 / 0x4B198..0x4B1A6: the native session-gated scheduler
     * runs after the camera track (0x4B193 FUN_000736AC, which carries the
     * armer) and before the clock body FUN_0008AF38 (the engine's
     * phase_drive + goal_scan). The native order means a goal queued by this
     * frame's scan is consumed by the next frame's handler. */
    if (mr->session_gate_14c32a) {
      rc = fifa96_match_run_screen_schedule(mr);
      if (rc < 0) return rc;
    }
    /* FU-143 wiring (Task 3): the clock's phase funnel, after the entity chain
     * (the native FUN_0008AF38 runs after the FU-67/entity block at 0x4B1A6
     * and inside it calls FUN_0008B9CC at 0x8B574). A completion staged above
     * runs the derived selector-0 chooser (2 -> 0x0C) on this frame. */
    rc = fifa96_match_run_phase_drive(mr);
    if (rc < 0) return rc;
    /* FU-145 S2 (0x8B63E): the native clock tail FUN_0008AF38 calls the goal
     * scanner FUN_00088940 after the period-end handling; the derived scan
     * carries the phase/armed gate and queues situation 6 (or the wired
     * table-2 fallback). */
    rc = fifa96_match_run_goal_scan(mr);
    if (rc < 0) return rc;
    /* FU-85 §4: FUN_00036C70 stages the render slots after the FU-4B100
     * update chain (0x49523, after the 0x49514 CALL 0x4B100), so the staged
     * positions/animations reflect this frame. Rendering stays opt-in: the
     * Task 12-14 fixtures keep an unstaged scene. */
    if (mr->render.enabled) match_run_scene_stage(mr);
  }
  if (period_ended && !fifa96_match_lifecycle_should_exit(&mr->lc)) {
    /* Ordering guard: the frame body runs in the clock advance before
     * run_step consumes the exit staging, so a period end coinciding with a
     * staged request_exit must not clobber EXIT->OVER (a staged EXIT wins). */
    rc = fifa96_match_lifecycle_mark_over(&mr->lc);
    if (rc != 0) return rc;
  }
  return granted != 0;
}

int fifa96_match_run_resolve(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER) {
    (void)fifa96_match_lifecycle_resolve_over(&mr->lc);   /* OVER -> POST */
  }
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_POST) {
    (void)fifa96_match_lifecycle_request_exit(&mr->lc);   /* POST -> EXIT */
  }
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
  return 0;
}

int fifa96_match_run_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->steps++;
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
  /* G1 live exit path: the frame body runs on the registered 100 Hz trampoline
   * during this engine step's clock advance, so a period end has just marked
   * the FU-64 lifecycle OVER when the step runs. Drive the post-period chain
   * through the single resolve entry (OVER -> POST -> EXIT -> run_end); the
   * one-step compression is deliberate for G1 and POST screen pacing is an
   * open leg for the Task 10 phase driver. Re-entry guard: resolve's run_end
   * clears running and the engine linkage, so a later step returns
   * -FIFA96_ERR_STATE and the engine's MATCH dispatch leaves through its
   * !e->match arm instead of stepping an ended run. */
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER)
    return fifa96_match_run_resolve(mr);
  /* The frame body does NOT run here: the registered 100 Hz trampoline
   * (fifa96_match_run_tick) already consumed this step's PIT ticks from the
   * engine clock, so pace/state advance once per 10 ms regardless of how many
   * ticks one engine step spans. */
  return 0;
}

int fifa96_match_run_end(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  int rc = fifa96_match_lifecycle_end(&mr->lc, &mr->pace);
  struct fifa96_engine *eng = mr->engine;
  match_run_release_stage(mr);
  mr->running = 0;
  if (eng) {
    if (eng->match == mr) eng->match = NULL;
    if (eng->mode == FIFA96_ENGINE_MODE_MATCH) eng->mode = FIFA96_ENGINE_MODE_FRONTEND;
  }
  return rc;
}

/* Scaled indexed span copy: FU-85 §2/FU-88 §5 reduce the resolved sprite to the
 * pivot-placed destination rectangle (fifa96_render_place) and the clipped
 * cover rectangle (fifa96_render_cover_rect); each output row then samples the
 * sprite at the 16.16 source step and writes through the remap, 0xFF
 * transparent (the FU-85 §2 0xCEABC span writer's semantics) into the indexed
 * canvas.
 *
 * `scale` is the signed FU-85 §2 size factor (`size * scale >> 16`); a negative
 * scale mirrors both axes through the library's signed placement/source
 * stepping, and the resolver's out1 flag flips x only. A zero destination size
 * is off-canvas (skipped). */
static int match_run_draw_sprite(struct fifa96_surface *s, const fifa96_sprite_frame *sprite,
                                 const uint8_t *remap, int32_t x, int32_t y, int32_t scale,
                                 int mirrored, const fifa96_render_clip *clip) {
  int32_t dst_w = (int32_t)(((int64_t)sprite->width * scale) >> 16);
  int32_t dst_h = (int32_t)(((int64_t)sprite->height * scale) >> 16);
  if (dst_w == 0 || dst_h == 0) return 0;
  if (mirrored) dst_w = -dst_w;
  int32_t dst_x = 0;
  int32_t dst_y = 0;
  fifa96_err_t err = fifa96_render_place(sprite->width, sprite->height, sprite->pivot_x,
                                         sprite->pivot_y, dst_w, dst_h, x, y, &dst_x, &dst_y);
  if (err != FIFA96_OK) return (int)err;
  fifa96_render_cover cover;
  uint8_t visible = 0;
  err = fifa96_render_cover_rect(dst_x, dst_y, dst_w, dst_h, sprite->width, sprite->height,
                                 clip, &cover, &visible);
  if (err != FIFA96_OK || !visible) return (int)err;
  int32_t cols[FIFA96_SURFACE_MAX_W + 1];
  for (int32_t row = 0; row < cover.dst_h; row++) {
    int32_t src_row =
        (int32_t)((uint32_t)cover.src_y + (uint32_t)row * (uint32_t)cover.src_dy);
    err = fifa96_sprite_columns(cols, (uint32_t)cover.dst_w, cover.src_x, cover.src_dx,
                                src_row, 0, sprite->width);
    if (err != FIFA96_OK) return (int)err;
    uint8_t *dst = s->indexed + (size_t)(cover.dst_y + row) * (size_t)s->width +
                   (size_t)cover.dst_x;
    err = fifa96_sprite_span(dst, sprite->pixels, sprite->pixel_len, cols,
                             (uint32_t)cover.dst_w, remap);
    if (err != FIFA96_OK) return (int)err;
  }
  return 0;
}

/* OL-T11-7 (P0.2 HUD; FU-148 §1.4/§6.1). The native bar blit scales the
 * Frames.fsh frame-13 panel by `h*0xB800>>16` in the zoomed window
 * (FUN_0009BB20/FUN_0009B850) and 1:1 in the full one (FUN_0009AFD0); the
 * layout anchor `[0x14E634]+6` = the Frames.fsh frame-4 height (41) is
 * rounded the same way in FUN_00053240's zoomed branch. */
#define MATCH_HUD_BAR_ZOOM 0xB800

/* The native FUN_000565BC gates (period < 4 via FUN_0004B5F4, not paused via
 * [0x14E688]) plus the asset-readiness mapping of the FUN_0001D940(6) /
 * [0x14E510] / [0x14E538] / [0x14E59C] gates (FU-148 §6.1 leg): the HUD needs
 * its staged bar and the font the window branch selects. The engine has no
 * replay sub-mode, so the [0x109A98]==0 idle gate is vacuous. */
static int match_run_hud_ready(const struct fifa96_match_run_render *r) {
  const struct fifa96_font *font = r->window_zoomed ? &r->hud_font[1] : &r->hud_font[0];
  return r->hud_bar_ready && r->hud_bar.pixels != NULL &&
         (r->window_zoomed ? r->hud_font_ready[1] : r->hud_font_ready[0]) != 0 &&
         font->data != NULL;
}

/* The bar panel blit: zero source pixels stay transparent (the engine's
 * indexed-sprite convention), the destination rect is the native's
 * `size * factor >> 16` and the sampling is the derived nearest-neighbour
 * step `dst * src_w / dst_w` (FU-148 leg: the native FUN_0009B850 span
 * stepper is not decomposed). */
static void match_run_hud_bar(struct fifa96_surface *s,
                              const struct fifa96_match_run_render *r, int bar_x,
                              int bar_y) {
  const fifa96_sprite_frame *bar = &r->hud_bar;
  int dst_w = bar->width;
  int dst_h = bar->height;
  if (r->window_zoomed) {
    dst_w = (bar->width * MATCH_HUD_BAR_ZOOM) >> 16;
    dst_h = (bar->height * MATCH_HUD_BAR_ZOOM) >> 16;
  }
  for (int dy = 0; dy < dst_h; dy++) {
    int src_y = (int)(((int64_t)dy * bar->height) / dst_h);
    int py = bar_y + dy;
    if (py < 0 || py >= s->height) continue;
    for (int dx = 0; dx < dst_w; dx++) {
      int src_x = (int)(((int64_t)dx * bar->width) / dst_w);
      size_t src_off = (size_t)src_y * bar->width + (size_t)src_x;
      if (src_off >= bar->pixel_len) continue;
      uint8_t v = bar->pixels[src_off];
      if (v == 0) continue;
      int px = bar_x + dx;
      if (px < 0 || px >= s->width) continue;
      s->indexed[(size_t)py * (size_t)s->width + (size_t)px] = v;
    }
  }
}

/* Colour-6 outline at (+1,+1) then colour-0 main (FUN_000A06FC's two-pass
 * text, FU-148 §1.5; the native colour ramp is leg). */
static void match_run_hud_text(struct fifa96_surface *s, const fifa96_font *font,
                               const char *str, int x, int y) {
  (void)fifa96_font_blit(s->indexed, s->width, s->height, font, str, x + 1, y + 1, 6);
  (void)fifa96_font_blit(s->indexed, s->width, s->height, font, str, x, y, 0);
}

/* FUN_00055BA8: centred cell text, `x += (cell_w-width)/2` (truncating),
 * `y += max(0,(cell_h-12)/2)`, then the outline/main pair. */
static void match_run_hud_centered(struct fifa96_surface *s, const fifa96_font *font,
                                   const char *str, int cell_x, int cell_y, int cell_w,
                                   int cell_h) {
  int w = fifa96_font_text_width(font, str);
  int x = cell_x + (cell_w - w) / 2;
  int y = cell_y;
  int pad = (cell_h - 12) / 2;
  if (pad > 0) y += pad;
  match_run_hud_text(s, font, str, x, y);
}

/* The HUD pass: bar -> name0 -> name1 -> score0 -> score1 -> period -> clock
 * (FU-148 §1.4). `mr->score[2]`/`state.total_seconds`/`state.period` are the
 * engine seam for the native `0x157AC5/7`, `0x157AB4` and `0x157AC2`. */
static void match_run_draw_hud(const struct fifa96_match_run *mr, struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  if (!match_run_hud_ready(r)) return;
  if (r->display.suspend) return;                 /* [0x14E688] */
  if (mr->state.period >= 4) return;              /* FUN_0004B5F4() < 4 */

  const int zoomed = r->window_zoomed;
  const struct fifa96_font *font = zoomed ? &r->hud_font[1] : &r->hud_font[0];
  const int have_window = r->window.box.w > 0 && r->window.box.h > 0;
  int x0 = have_window ? r->window.box.x0 : 0;
  int y1 = have_window ? r->window.box.y1 : s->height;
  if (y1 > s->height) y1 = s->height;

  const int row_pitch = zoomed ? 9 : 12;
  const int x_left = x0 + (zoomed ? 3 : 6);
  int clock_x = x0 + (zoomed ? 0x23 : 0x32);
  const int bar_x = x0 + (zoomed ? 1 : 2);
  int bar_h = r->hud_bar_height;
  if (zoomed) bar_h = (bar_h * MATCH_HUD_BAR_ZOOM + 0x8000) >> 16;
  const int bar_y = y1 - bar_h - (zoomed ? 1 : 2);
  const int y2 = bar_y + 2;
  const int cells_y = bar_y + 2 * row_pitch + (zoomed ? 2 : 3);

  char clock_buf[16];
  char score_buf[2][8];
  char period_buf[8];
  (void)snprintf(clock_buf, sizeof clock_buf, "%02d:%02d",
                 (int)(mr->state.total_seconds / 60u),
                 (int)(mr->state.total_seconds % 60u));
  (void)snprintf(score_buf[0], sizeof score_buf[0], "%d", (int)mr->score[0]);
  (void)snprintf(score_buf[1], sizeof score_buf[1], "%d", (int)mr->score[1]);
  (void)snprintf(period_buf, sizeof period_buf, "%d", (int)mr->state.period + 1);

  /* Wide-name/score adjustment (0x55D5A..0x55E76): either score >= 100 or a
   * staged name wider than 0x20 shifts the score column and widens the clock
   * cell by 6 (zoomed) / 8 (full). */
  int wide = mr->score[0] >= 100 || mr->score[1] >= 100 ||
             fifa96_font_text_width(font, r->hud_name[0]) > 0x20 ||
             fifa96_font_text_width(font, r->hud_name[1]) > 0x20;
  int clock_cell_w = zoomed ? 0x12 : 0x18;
  if (wide) {
    clock_x += zoomed ? 6 : 8;
    clock_cell_w += zoomed ? 6 : 8;
  }

  match_run_hud_bar(s, r, bar_x, bar_y);
  match_run_hud_text(s, font, r->hud_name[0], x_left, y2);
  match_run_hud_text(s, font, r->hud_name[1], x_left, y2 + row_pitch - 2);
  match_run_hud_text(s, font, score_buf[0],
                     clock_x - fifa96_font_text_width(font, score_buf[0]), y2);
  match_run_hud_text(s, font, score_buf[1],
                     clock_x - fifa96_font_text_width(font, score_buf[1]),
                     y2 + row_pitch - 2);
  match_run_hud_centered(s, font, period_buf, x0, cells_y, row_pitch - 1, row_pitch - 1);
  match_run_hud_centered(s, font, clock_buf, x0 + (zoomed ? 0xE : 0x11), cells_y,
                         clock_cell_w, row_pitch - 1);
}

int fifa96_match_run_render(struct fifa96_match_run *mr, struct fifa96_surface *s) {
  if (!mr || !s) return -FIFA96_ERR_INVALID;
  struct fifa96_match_run_render *r = &mr->render;
  if (!r->enabled) return 0;
  if (!r->frames || !r->banks || r->bank_count == 0 || !r->sprite_data ||
      r->sprite_data_len < 16u)
    return -FIFA96_ERR_STATE;

  /* OL-T11-6: install the staged native match palette on the presented target
   * before the plane conversion (the native match-data load installs the
   * palette before any match frame draws). */
  if (r->palette_ready) (void)fifa96_match_run_palette_install(mr, s);

  /* A zero window box means "not defined": fall back to the full surface (the
   * live match's window A/B selection is an open leg). */
  const int have_window = r->window.box.w > 0 && r->window.box.h > 0;

  /* FU-93 §1: window rect -> 16.16 zoom scale (the overlay/HUD coordinate
   * seam; the FU-88 entity projection has no zoom term, FU-88 §5/§8). */
  int32_t scale_x = 0;
  int32_t scale_y = 0;
  int zoomed = 0;
  if (have_window) {
    (void)fifa96_window_scale(&r->window, &scale_x, &scale_y);
    (void)fifa96_window_zoomed(&r->window, &zoomed);
  }
  r->window_scale_x = scale_x;
  r->window_scale_y = scale_y;
  r->window_zoomed = zoomed;

  fifa96_surface_clear(s, r->background);

  /* FU-92 §1/§5: the 16.16 clip rect is the render window; a zero box falls
   * back to the full surface. The projection centre is the window's
   * src_x + src_w/2. */
  fifa96_render_clip clip;
  fifa96_projection_point center;
  if (have_window) {
    clip.left = r->window.box.x0;
    clip.top = r->window.box.y0;
    clip.right = r->window.box.x1;
    clip.bottom = r->window.box.y1;
    center.x = r->window.cx_fix;
    center.y = r->window.cy_fix;
  } else {
    clip.left = 0;
    clip.top = 0;
    clip.right = s->width;
    clip.bottom = s->height;
    center.x = (s->width >> 1) << 16;
    center.y = (s->height >> 1) << 16;
  }
  /* The window's screen dimensions come from the engine surface at begin, but
   * s is a caller argument: clamp the clip to the target surface so the span
   * blit can never write past its logical width/height (or overflow the
   * column scratch). */
  if (clip.left < 0) clip.left = 0;
  if (clip.top < 0) clip.top = 0;
  if (clip.right > s->width) clip.right = s->width;
  if (clip.bottom > s->height) clip.bottom = s->height;

  int32_t matrix[9];
  int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
  int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];
  fifa96_err_t err = fifa96_projection_matrix(r->yaw, r->pitch, matrix);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_projection_reciprocal(s->width, recip_x);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_projection_reciprocal(s->height, recip_y);
  if (err != FIFA96_OK) return (int)err;

  /* FU-85 §4 camera staging (FUN_00036C70's camera copy/bounds over the FU-71
   * block). */
  const fifa96_render_pos cam_src = { r->camera.pos_x, r->camera.pos_y, r->camera.pos_z };
  fifa96_render_pos cam;
  err = fifa96_render_camera_stage(&cam_src, &cam);
  if (err != FIFA96_OK) return (int)err;

  /* FU-89 §6/FUN_000589E0: the near-depth threshold `[0x54350]` is
   * `max((0x14<<16)/(2*camera +0x4C ratio), 0x78)`, or the 0x78 fallback when
   * the camera y is below 0x140 (see MATCH_RUN_NEAR_LIMIT). The gate below
   * compares it against each entry's jittered depth key. */
  int32_t near_threshold = 0;
  err = fifa96_scene_threshold(r->view_ratio, r->camera.pos_y,
                               FIFA96_SCENE_CLAMP_MIN, MATCH_RUN_NEAR_LIMIT,
                               &near_threshold);
  if (err != FIFA96_OK) return (int)err;

  uint32_t count = r->entity_count;
  if (count > FIFA96_MATCH_RUN_RENDER_SLOTS) count = FIFA96_MATCH_RUN_RENDER_SLOTS;

  fifa96_scene_slot slots[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t staged_x[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t staged_y[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t angles[FIFA96_MATCH_RUN_RENDER_SLOTS];
  uint8_t drawable[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t jitter_z[FIFA96_MATCH_RUN_RENDER_SLOTS * FIFA96_SCENE_POSITION_DWORDS];
  uint32_t list[FIFA96_MATCH_RUN_RENDER_SLOTS];
  for (uint32_t i = 0; i < count; i++) {
    fifa96_render_slot slot;
    err = fifa96_render_slot_stage(&r->entities[i].stage, &slot);
    if (err != FIFA96_OK) return (int)err;
    staged_x[i] = slot.pos.x;
    staged_y[i] = slot.pos.y;
    angles[i] = slot.angle;
    fifa96_projection_vec rel;
    rel.x = (int32_t)((uint32_t)slot.pos.x - (uint32_t)cam.x);
    rel.y = (int32_t)((uint32_t)slot.pos.y - (uint32_t)cam.y);
    rel.z = (int32_t)((uint32_t)slot.pos.z - (uint32_t)cam.z);
    /* FU-89 §7: the jittered array adds `i%6 + 0x70` to y before rotation. */
    fifa96_projection_vec rel_jitter = rel;
    rel_jitter.y = (int32_t)((uint32_t)rel_jitter.y + (uint32_t)(0x70u + (i % 6u)));
    fifa96_projection_vec clean_rot;
    fifa96_projection_vec jitter_rot;
    err = fifa96_projection_transform(matrix, &rel, &clean_rot);
    if (err != FIFA96_OK) return (int)err;
    err = fifa96_projection_transform(matrix, &rel_jitter, &jitter_rot);
    if (err != FIFA96_OK) return (int)err;
    err = fifa96_scene_slot_project(recip_x, recip_y, &center, &clean_rot, &jitter_rot,
                                    &slots[i]);
    if (err != FIFA96_OK) return (int)err;
    jitter_z[i * FIFA96_SCENE_POSITION_DWORDS + 2] = jitter_rot.z;
    list[i] = i;
    drawable[i] = (uint8_t)(slots[i].clean_visible && slots[i].jitter_visible);
  }

  /* FU-89 §2 (corrected first-hand 0x57798..0x577AC): seed keys[k] =
   * z(jittered rotated point of list[k]) and shell-sort the tandem arrays
   * descending. The native list is the fixed 0x18-entry array pre-seeded with
   * the 1-based slot values 0..0x17 by FUN_00056CF4; value 0 is the empty
   * sentinel (skipped at 0x57CD2), so the staged 23 slots occupy the list's
   * values 1..23 and the engine builds the same 24 entries: `values[i] = i+1`
   * for the count slots plus the sentinel `values[count] = 0`. The sentinel's
   * native key is the scratch triple copied to [0x54370] (z of the block-B
   * +0xCB4 triple); the engine uses 0 (that producer is unmodeled). */
  int32_t keys[FIFA96_MATCH_RUN_RENDER_SLOTS + 1];
  uint32_t values[FIFA96_MATCH_RUN_RENDER_SLOTS + 1];
  for (uint32_t i = 0; i < count; i++) values[i] = i + 1;
  values[count] = 0;
  keys[count] = 0;
  err = fifa96_scene_build_keys(count, list, jitter_z, count, keys);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_scene_sort(count + 1, keys, values);
  if (err != FIFA96_OK) return (int)err;

  const fifa96_sprite_bank sprite_bank = {
    .data = r->sprite_data,
    .total_size = r->sprite_data_len,
    .count = 0,
    .tag = { 0, 0, 0, 0 },
  };
  for (uint32_t i = 0; i <= count; i++) {
    uint32_t v = values[i];
    if (v == 0) continue;     /* 0x57CD2: the 1-based empty slot */
    uint32_t slot = v - 1;
    if (slot >= count) continue;
    uint8_t visible = 0;
    /* FU-89 §3.2: the lateral gate reads the staged triple's x (`[0x9A74]` =
     * 0x155AB0, first-hand) and culls it when it is > 0x8E0. */
    err = fifa96_scene_slot_gate(near_threshold, keys[i], staged_y[slot], staged_x[slot],
                                 &visible);
    if (err != FIFA96_OK) return (int)err;
    if (!visible || !drawable[slot]) continue;
    int32_t jitter_y_out = slots[slot].jitter.y;
    uint8_t draw = 0;
    err = fifa96_scene_clip_edges((int32_t)((uint32_t)clip.left << 16),
                                  (int32_t)((uint32_t)clip.top << 16),
                                  (int32_t)((uint32_t)clip.right << 16),
                                  (int32_t)((uint32_t)clip.bottom << 16), slots[slot].clean.y,
                                  slots[slot].jitter.x, slots[slot].jitter.y, &jitter_y_out, &draw);
    if (err != FIFA96_OK) return (int)err;
    if (!draw) continue;

    /* FU-85 §2 direction: `(7 - (((angle - 0x1000) & 0xFFFF) >> 13)) & 7`. The
     * original adds the unported 0xA2A10 view/position angle addend; that
     * addend is an open leg, so the staged FU-84 angle alone drives it. */
    int32_t direction =
        (7 - (int32_t)((uint16_t)((uint32_t)angles[slot] - 0x1000u) >> 13)) & 7;
    fifa96_render_frame frame;
    err = fifa96_render_resolve(r->entities[slot].stage.anim_id, r->entities[slot].stage.frame,
                                direction, r->frames, r->entities[slot].bank_index, r->banks,
                                r->bank_count, r->fixed60, r->fixed61, r->mirror, &frame);
    if (err != FIFA96_OK) return (int)err;
    if (!frame.sprite || (uintptr_t)frame.sprite < (uintptr_t)r->sprite_data) continue;
    fifa96_sprite_frame sprite;
    err = fifa96_sprite_frame_parse(&sprite_bank,
                                    (uint32_t)(frame.sprite - r->sprite_data), &sprite);
    if (err != FIFA96_OK) return (int)err;

    /* FU-85 §2 sprite size: scale = ((clean_y - jitter_y + 0x1000) & ~0xFFFF)
     *                       * 0x5D1 / 0x10000
     *   (the 0x571FD sequence); each frame header size scales by the signed
     *   factor with the 32x32->16 fixup (FUN_00057080's SHRD), i.e.
     *   `size * scale >> 16` (negative scale = mirrored). */
    uint32_t delta = (uint32_t)slots[slot].clean.y - (uint32_t)slots[slot].jitter.y;
    uint32_t size_base = (delta + 0x1000u) & 0xFFFF0000u;
    int32_t scale = (int32_t)(((int64_t)(int32_t)size_base * 0x5D1) >> 16);
    int32_t x = (int32_t)((int64_t)slots[slot].clean.x >> 16);
    int32_t y = (int32_t)((int64_t)slots[slot].clean.y >> 16);
    int rc = match_run_draw_sprite(s, &sprite, r->remap, x, y, scale, frame.mirrored, &clip);
    if (rc != 0) return rc;
    /* FU-85 §2 second pass: composite animator classes resolve an overlay
     * frame (out2) and the original blits it with a second FUN_00057080 call
     * (0x57241) using the same scale/position. Draw it over the main frame;
     * an unparseable overlay is treated as absent, like the blitter's NULL
     * return. */
    if (frame.overlay && (uintptr_t)frame.overlay >= (uintptr_t)r->sprite_data) {
      fifa96_sprite_frame overlay;
      if (fifa96_sprite_frame_parse(&sprite_bank,
                                    (uint32_t)(frame.overlay - r->sprite_data),
                                    &overlay) == FIFA96_OK) {
        rc = match_run_draw_sprite(s, &overlay, r->remap, x, y, scale, frame.mirrored, &clip);
        if (rc != 0) return rc;
      }
    }
  }
  /* OL-T11-7: the overlay presenter runs after the scene (the native
   * FUN_000495B0 -> FUN_00049830 -> FUN_000565BC order). */
  match_run_draw_hud(mr, s);
  return 0;
}

/* OL-T11-6 (M2 playable-match Task 1): the native match palette. See the
 * header contract; the first-hand chain is in FU-144 and the constants above.
 * `FUN_00048B60`/`FUN_00048ED8` read the pre-remap snapshot and write
 * `pal[0x70F2[i]] = snapshot[0x70E8[i]]` (0x48BA9/0x48BBB, 0x48F57/0x48F69),
 * then copy the PALsys chunk's appended ranges over the result, and
 * `FUN_000479A0` converts 6->8 bit with `v << 2`. */
int fifa96_match_palette_from_bank(const uint8_t *bank_data, size_t bank_len,
                                   const uint8_t *base6, uint8_t rgb8[768]) {
  fifa96_sprite_bank bank;
  fifa96_sprite_entry entry;
  fifa96_sprite_chunk chunk;
  const uint8_t *rgb6 = NULL;
  uint16_t count = 0;
  uint8_t pal6[768];
  fifa96_err_t err;
  if (!bank_data || !rgb8) return -FIFA96_ERR_INVALID;
  err = fifa96_sprite_bank_parse(bank_data, bank_len, &bank);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_sprite_bank_entry(&bank, MATCH_RUN_PALETTE_FRAME, &entry);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_sprite_chunk_parse(&bank, entry.offset, &chunk);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_sprite_chunk_palette(&chunk, &rgb6, &count);
  if (err != FIFA96_OK) return (int)err;
  /* The native copies 0x300 bytes from the chunk unconditionally; the port
   * requires the full 256-entry form the retail PALsys.fsh frame 2 carries. */
  if (count != MATCH_RUN_PALETTE_COUNT) return -FIFA96_ERR_UNSUPPORTED;
  if (base6) {
    memcpy(pal6, base6, sizeof pal6);
  } else {
    /* Derived default: the native base buffer 0x14B200 is the previously
     * installed front-end palette and is not statically derivable; using the
     * chunk itself makes the native appends identity (recorded leg). */
    memcpy(pal6, rgb6, sizeof pal6);
  }
  (void)fifa96_sprite_palette_kit_remap(pal6, count);
  memcpy(pal6 + MATCH_RUN_PALETTE_APPEND_A_OFF,
         rgb6 + MATCH_RUN_PALETTE_APPEND_A_OFF, MATCH_RUN_PALETTE_APPEND_A_LEN);
  memcpy(pal6 + MATCH_RUN_PALETTE_APPEND_B_OFF,
         rgb6 + MATCH_RUN_PALETTE_APPEND_B_OFF, MATCH_RUN_PALETTE_APPEND_B_LEN);
  for (size_t i = 0; i < sizeof pal6; i++) rgb8[i] = (uint8_t)(pal6[i] << 2);
  return FIFA96_OK;
}

int fifa96_match_run_palette_install(struct fifa96_match_run *mr,
                                     struct fifa96_surface *s) {
  if (!mr || !s) return -FIFA96_ERR_INVALID;
  if (!mr->render.palette_ready) return -FIFA96_ERR_STATE;
  fifa96_surface_set_palette8(s, mr->render.palette);
  return 0;
}

/* FU-148 §4.2 (S4): the per-entity translation install
 * (FUN_00048DC0 -> FUN_000CE980 -> 0x114720). The native kit path fires when
 * `[0x1068E0]==1` (set by the match load FUN_00048ED8, cleared by the restore
 * FUN_00048FF4) and the entity is 0/0xB; every other entity (or an inactive
 * kit) installs its pool slot verbatim. The engine's analog for the load flag
 * is `render.palette_ready`; the pool is the caller-staged partition. */
int fifa96_match_run_translation_install(struct fifa96_match_run *mr, uint32_t entity) {
  const uint8_t *slot;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (entity >= 23u) return -FIFA96_ERR_INVALID;   /* the native indexes blindly */
  if (!mr->render.palette_pool.base) return -FIFA96_ERR_STATE;  /* pool identity leg 11 */
  slot = mr->render.palette_pool.slots23[entity];
  if (mr->render.palette_ready && (entity == 0u || entity == 0xBu))
    return (int)fifa96_palette_translate_kit(slot, mr->render.remap, entity);
  return (int)fifa96_palette_translate_slot(mr->render.remap, slot);
}

/* Task 2 asset staging (FU-84/85/86). The decode chain mirrors the game's
 * resource path: a container record (`[selector 0xFB BE24 size payload]`) is
 * decoded once, then the payload is a BIGF v2 directory (FU-41/FU-86 §2) whose
 * entries are raw SHPI .fsh slices or nested record chains that decode to SHPI
 * (FU-86 §3). Stage 1 is fed the container tail, not the bounded record slice:
 * the original huff reader is unbounded and 10 PLAYART .qfs entries need the
 * following bytes (FU-86 §3 caveat / leg 9); every other stage is exact. */
#define MATCH_STAGE_DECODE_STAGES 4
#define MATCH_STAGE_FRAME_SIZE 5
#define MATCH_STAGE_FRAME_DURATION 0x50u   /* FU-84 §4 row 1 (walk) */
/* FU-84's frame index is a byte and fifa96_render_resolve accepts the signed
 * non-negative half (0..0x7F, `fi = (int8_t)frame_index` in fifa96_render.c);
 * the staged table covers the whole resolver domain so no accepted index can
 * read past it. */
#define MATCH_STAGE_FRAMES_MAX 128u

/* One BIGF entry after decode: an owned SHPI image, or the unloaded slot the
 * renderer skips (FU-85 §1.2: a NULL bank handle). */
struct match_stage_item {
  uint8_t *data;
  uint32_t len;
  uint32_t count;
};

/* OL-T11-7: the HUD entries the loader table resolves in the GAMEART0
 * container (FU-148 §1.5): resource slot 0x35 "clockfnt.fsh" (full window),
 * 0x36 "playfnt.fsh" (zoomed) and slot 0x2F "Frames.fsh" (the bar bank
 * FUN_00053930 stores frames from at 0x14E624). */
#define MATCH_RUN_HUD_FONT_BANK "clockfnt.fsh"
#define MATCH_RUN_HUD_FONT_ZOOM_BANK "playfnt.fsh"
#define MATCH_RUN_HUD_BAR_BANK "Frames.fsh"
#define MATCH_RUN_HUD_BAR_FRAME 13u   /* the drawn frame ([0x14E658]) */
#define MATCH_RUN_HUD_BAR_LAYOUT_FRAME 4u   /* the 0x14E634 height anchor */

struct match_stage_set {
  struct match_stage_item *items;
  uint32_t count;       /* slots = BIGF entry count (indices stay stable) */
  uint32_t loaded;      /* slots with a decoded SHPI bank */
  int32_t palette_index; /* BIGF entry named PALsys.fsh, -1 when absent */
  int32_t frames_index;  /* BIGF entry named Frames.fsh, -1 when absent */
  int32_t clockfont_index; /* BIGF entry named clockfnt.fsh, -1 when absent */
  int32_t playfont_index;  /* BIGF entry named playfnt.fsh, -1 when absent */
};

/* The native by-name lookup (FUN_000A81A5) lowercases ASCII, so the staged
 * bank name matches are case-insensitive like the original's. */
static int match_run_name_eq(const char *a, const char *b) {
  while (*a && *b) {
    unsigned char ca = (unsigned char)*a;
    unsigned char cb = (unsigned char)*b;
    if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + 0x20);
    if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + 0x20);
    if (ca != cb) return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static void match_stage_set_free(struct match_stage_set *set) {
  if (set->items) {
    for (uint32_t i = 0; i < set->count; i++) free(set->items[i].data);
    free(set->items);
  }
  memset(set, 0, sizeof *set);
}

/* Decode a whole bank container to its BIGF v2 bytes: a raw BIGF file or the
 * refpack record form both FU-86 containers use. Returns 0 with a
 * caller-freeable buffer, -1 when the port cannot decode it. */
static int match_stage_container(const uint8_t *file, size_t file_len, uint8_t **out,
                                 size_t *out_len) {
  *out = NULL;
  *out_len = 0;
  if (file_len >= 4 && memcmp(file, "BIGF", 4) == 0) {
    uint8_t *copy = malloc(file_len);
    if (!copy) return -1;
    memcpy(copy, file, file_len);
    *out = copy;
    *out_len = file_len;
    return 0;
  }
  if (file_len >= 5 && file[1] == 0xFB) {
    size_t declared = ((size_t)file[2] << 16) | ((size_t)file[3] << 8) | (size_t)file[4];
    if (declared == 0) return -1;
    uint8_t *buf = malloc(declared);
    if (!buf) return -1;
    size_t n = 0;
    if (fifa96_record_decode(file, file_len, buf, declared, &n) != 0) {
      free(buf);
      return -1;
    }
    *out = buf;
    *out_len = n;
    return 0;
  }
  return -1;
}

/* Decode one BIGF entry to a SHPI image. `entry_len` is the entry's bounded
 * BIGF record size (used for the raw SHPI form, whose declared total equals it,
 * FU-86 §4) and `raw_len` the container tail from the entry's offset (the
 * stage-1 over-read allowance noted above). Returns 1 with an owned copy and
 * its frame count, 0 when the entry is not a sprite bank (fonts, .dat tables),
 * -1 when it is SHPI/record shaped but the port cannot decode it. */
static int match_stage_entry(const uint8_t *raw, size_t entry_len, size_t raw_len,
                             uint8_t **out, uint32_t *out_len, uint32_t *out_count) {
  *out = NULL;
  *out_len = 0;
  *out_count = 0;
  if (raw_len >= 4 && memcmp(raw, "SHPI", 4) == 0) {
    fifa96_sprite_bank bank;
    if (fifa96_sprite_bank_parse(raw, entry_len, &bank) != FIFA96_OK) return -1;
    uint8_t *copy = malloc(bank.total_size);
    if (!copy) return -1;
    memcpy(copy, raw, bank.total_size);
    *out = copy;
    *out_len = bank.total_size;
    *out_count = bank.count;
    return 1;
  }
  if (raw_len < 5 || raw[1] != 0xFB) return 0;
  const uint8_t *src = raw;
  size_t src_len = raw_len;
  uint8_t *cur = NULL;
  for (int stage = 0; stage < MATCH_STAGE_DECODE_STAGES; stage++) {
    if (src_len >= 4 && memcmp(src, "SHPI", 4) == 0) {
      fifa96_sprite_bank bank;
      if (fifa96_sprite_bank_parse(src, src_len, &bank) != FIFA96_OK) {
        free(cur);
        return -1;
      }
      uint8_t *copy = malloc(bank.total_size);
      if (!copy) {
        free(cur);
        return -1;
      }
      memcpy(copy, src, bank.total_size);
      free(cur);
      *out = copy;
      *out_len = bank.total_size;
      *out_count = bank.count;
      return 1;
    }
    if (src_len < 5 || src[1] != 0xFB) {
      free(cur);
      return -1;
    }
    size_t declared = ((size_t)src[2] << 16) | ((size_t)src[3] << 8) | (size_t)src[4];
    if (declared == 0) {
      free(cur);
      return -1;
    }
    uint8_t *dst = malloc(declared);
    if (!dst) {
      free(cur);
      return -1;
    }
    size_t n = 0;
    if (fifa96_record_decode(src, src_len, dst, declared, &n) != 0) {
      free(dst);
      free(cur);
      return -1;
    }
    free(cur);
    cur = dst;
    src = dst;
    src_len = n;
  }
  free(cur);
  return -1;
}

/* Decode every BIGF entry of one container into a slot-stable set: slot i is
 * container entry i, so the player container's indices are exactly the FU-84
 * row +8 bank indices (observed 0..89). Entries the port cannot decode stay
 * unloaded slots, the renderer's skippable state (FU-85 §1.2). */
static int match_stage_set_build(struct match_stage_set *set, const uint8_t *file,
                                 size_t file_len) {
  memset(set, 0, sizeof *set);
  set->palette_index = -1;
  set->frames_index = -1;
  set->clockfont_index = -1;
  set->playfont_index = -1;
  uint8_t *container = NULL;
  size_t container_len = 0;
  if (match_stage_container(file, file_len, &container, &container_len) != 0) return -1;
  struct fifa96_bigf_info info;
  if (fifa96_bigf_parse(container, container_len, &info) != FIFA96_OK) goto fail;
  if (info.count == 0) goto fail;
  set->items = calloc(info.count, sizeof *set->items);
  if (!set->items) goto fail;
  set->count = (uint32_t)info.count;
  for (size_t i = 0; i < info.count; i++) {
    uint32_t off = 0;
    uint32_t size = 0;
    const char *name = NULL;
    if (fifa96_bigf_record(&info, i, &off, &size, &name) != FIFA96_OK) goto fail;
    if (name && match_run_name_eq(name, MATCH_RUN_PALETTE_BANK))
      set->palette_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_HUD_BAR_BANK))
      set->frames_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_HUD_FONT_BANK))
      set->clockfont_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_HUD_FONT_ZOOM_BANK))
      set->playfont_index = (int32_t)i;
    uint8_t *bank = NULL;
    uint32_t bank_len = 0;
    uint32_t bank_count = 0;
    int got = match_stage_entry(container + off, size, container_len - off, &bank,
                                &bank_len, &bank_count);
    if (got == 1) {
      set->items[i].data = bank;
      set->items[i].len = bank_len;
      set->items[i].count = bank_count;
      set->loaded++;
    } else if (got < 0) {
      set->items[i].data = NULL;   /* shaped like a bank but undecodable */
    } else if ((int32_t)i == set->clockfont_index || (int32_t)i == set->playfont_index) {
      /* OL-T11-7: the FNTI font images are not SHPI banks, so the decode
       * refuses them (FU-148 §1.5); keep the raw entry bytes for the HUD
       * font parse. */
      uint8_t *copy = malloc(size);
      if (!copy) goto fail;
      memcpy(copy, container + off, size);
      set->items[i].data = copy;
      set->items[i].len = size;
      set->items[i].count = 0;
    }
  }
  free(container);
  return 0;
fail:
  free(container);
  match_stage_set_free(set);
  return -1;
}

/* Round an arena cursor up to `align`. SHPI copies need 4 for the int32 offset
 * table at SHPI+0x14 (FU-86 §4: directory entries {name[4], u32 offset}); the
 * bank record array needs its natural alignment for UBSan-clean access. */
static size_t match_stage_pad(size_t n, size_t align) {
  return (n + align - 1u) & ~(align - 1u);
}

int fifa96_match_run_stage(struct fifa96_match_run *mr, const struct fifa96_surface *s,
                           const char *player_bank, const char *pitch_bank) {
  struct match_stage_set player;
  struct match_stage_set pitch;
  struct fifa96_cache *cache;
  const uint8_t *file;
  size_t file_len;
  uint32_t lba = 0;
  uint32_t size = 0;
  size_t frames_len;
  size_t banks_off;
  size_t data_off;
  size_t data_len = 0;
  size_t arena_len;
  uint8_t *arena = NULL;
  struct fifa96_render_bank *banks;
  uint8_t *frames;
  uint32_t total;
  uint8_t palette[768];
  int palette_ready = 0;
  /* OL-T11-7: built locally and committed with the arena swap only on
   * success; the staged names persist (they are not container assets). */
  struct fifa96_font hud_font[2];
  uint8_t hud_font_ready[2] = {0, 0};
  struct fifa96_sprite_frame hud_bar;
  uint16_t hud_bar_height = 0;
  uint8_t hud_bar_ready = 0;
  int rc;

  memset(hud_font, 0, sizeof hud_font);
  memset(&hud_bar, 0, sizeof hud_bar);

  if (!mr || !s || !player_bank || !pitch_bank) return -FIFA96_ERR_INVALID;
  /* A live run guarantees the engine (and its asset table) outlives the arena
   * the run takes ownership of; staging after end would otherwise strand the
   * arena (end releases once) or reach a destroyed engine. */
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!mr->engine || !mr->engine->assets) return -FIFA96_ERR_STATE;

  memset(&player, 0, sizeof player);
  memset(&pitch, 0, sizeof pitch);
  cache = fifa96_cache_create(mr->engine->assets);
  if (!cache) return -FIFA96_ERR_UNSUPPORTED;

  /* Player animation container (FU-86 §1: PLAYART, 91 banks). */
  if (fifa96_asset_lookup(mr->engine->assets, player_bank, &lba, &size) != FIFA96_OK) {
    rc = -FIFA96_ERR_NOT_FOUND;
    goto out_cache;
  }
  file = fifa96_cache_get(cache, player_bank, &file_len);
  if (!file || file_len == 0 || match_stage_set_build(&player, file, file_len) != 0 ||
      player.loaded == 0) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }

  /* Second (pitch/match art) container (FU-86 §1: GAMEART0; identity open). */
  if (fifa96_asset_lookup(mr->engine->assets, pitch_bank, &lba, &size) != FIFA96_OK) {
    rc = -FIFA96_ERR_NOT_FOUND;
    goto out_sets;
  }
  file = fifa96_cache_get(cache, pitch_bank, &file_len);
  if (!file || file_len == 0 || match_stage_set_build(&pitch, file, file_len) != 0 ||
      pitch.loaded == 0) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }

  /* One arena owns the frame table, the bank records (bases/offset tables
   * point into the SHPI copies) and the SHPI images. Nothing touches mr until
   * the whole build succeeds. */
  total = player.count + pitch.count;
  const size_t bank_align = _Alignof(struct fifa96_render_bank);
  frames_len = (size_t)MATCH_STAGE_FRAMES_MAX * MATCH_STAGE_FRAME_SIZE;
  banks_off = match_stage_pad(frames_len, bank_align);
  data_off = match_stage_pad(banks_off + (size_t)total * sizeof *banks, 4u);
  for (uint32_t i = 0; i < player.count; i++)
    if (player.items[i].data) data_len += match_stage_pad(player.items[i].len, 4u);
  for (uint32_t i = 0; i < pitch.count; i++)
    if (pitch.items[i].data) data_len += match_stage_pad(pitch.items[i].len, 4u);
  if (data_off + data_len > 0xFFFFFFFFu) {
    rc = -FIFA96_ERR_UNSUPPORTED;   /* sprite_data_len is 32-bit */
    goto out_sets;
  }
  arena_len = data_off + data_len;
  arena = calloc(1, arena_len);
  if (!arena) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }
  frames = arena;
  banks = (struct fifa96_render_bank *)(void *)(arena + banks_off);

  /* FU-84 §4 frame records: the row-1 walk evidence stores sprite = frame
   * index with duration 0x50, so staging derives that identity table across
   * the resolver's whole 128-index domain; the real per-row tables are
   * executable object-4 data (0x10E200..0x10EF00), not ISO assets (open leg). */
  for (uint32_t i = 0; i < MATCH_STAGE_FRAMES_MAX; i++) {
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 0] = (uint8_t)MATCH_STAGE_FRAME_DURATION;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 1] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 2] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 3] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 4] = (uint8_t)i;
  }

  size_t cursor = data_off;
  uint32_t slot = 0;
  uint32_t base_slot = 0;
  const struct match_stage_set *sets[2] = { &player, &pitch };
  for (int which = 0; which < 2; which++) {
    const struct match_stage_set *set = sets[which];
    for (uint32_t i = 0; i < set->count; i++, slot++) {
      struct fifa96_render_bank *b = &banks[slot];
      if (!set->items[i].data) continue;   /* unloaded slot stays zeroed */
      uint8_t *copy = arena + cursor;
      memcpy(copy, set->items[i].data, set->items[i].len);
      /* OL-T11-6: extract the derived match palette from the pitch container's
       * PALsys.fsh bank while the arena copy is at hand. */
      if (which == 1 && (int32_t)i == pitch.palette_index &&
          fifa96_match_palette_from_bank(copy, set->items[i].len, NULL, palette) ==
              FIFA96_OK)
        palette_ready = 1;
      /* OL-T11-7: parse the HUD assets from the pitch container's arena
       * copies — the FNTI fonts (FUN_000984B4) and the Frames.fsh bar frames
       * (frame 13 drawn, frame 4's height for the layout anchor). */
      if (which == 1) {
        if ((int32_t)i == pitch.clockfont_index) {
          if (fifa96_font_parse(copy, set->items[i].len, &hud_font[0]) == FIFA96_OK)
            hud_font_ready[0] = 1;
        } else if ((int32_t)i == pitch.playfont_index) {
          if (fifa96_font_parse(copy, set->items[i].len, &hud_font[1]) == FIFA96_OK)
            hud_font_ready[1] = 1;
        } else if ((int32_t)i == pitch.frames_index) {
          fifa96_sprite_bank bar_bank;
          fifa96_sprite_entry bar_entry;
          if (fifa96_sprite_bank_parse(copy, set->items[i].len, &bar_bank) == FIFA96_OK) {
            int bar_ok = fifa96_sprite_bank_entry(&bar_bank, MATCH_RUN_HUD_BAR_FRAME,
                                                  &bar_entry) == FIFA96_OK &&
                         fifa96_sprite_frame_parse(&bar_bank, bar_entry.offset, &hud_bar) ==
                             FIFA96_OK;
            int layout_ok =
                fifa96_sprite_bank_entry(&bar_bank, MATCH_RUN_HUD_BAR_LAYOUT_FRAME,
                                         &bar_entry) == FIFA96_OK;
            if (layout_ok) {
              fifa96_sprite_frame layout_frame;
              layout_ok = fifa96_sprite_frame_parse(&bar_bank, bar_entry.offset,
                                                    &layout_frame) == FIFA96_OK;
              if (layout_ok) hud_bar_height = layout_frame.height;
            }
            /* Both the drawn frame and the layout anchor are required (the
             * native pointers are always both set before the HUD can run). */
            hud_bar_ready = (uint8_t)(bar_ok && layout_ok);
          }
        }
      }
      b->base = copy;
      b->offsets = (const int32_t *)(const void *)(copy + 0x14);
      b->count = set->items[i].count;
      /* The FU-86 §4.1 stride switch indexes the animator-record array of the
       * container the bank came from (the player container is the derived
       * 0x57DE8 array; indices 0..90). Appended containers have no derived
       * animator identity (open leg), so their banks use a container-local
       * index instead of leaking into the player switch's classes (the old
       * global slot 0x5B hit /2 by accident). */
      b->step = fifa96_sprite_stride(slot - base_slot, set->items[i].count);
      cursor += match_stage_pad(set->items[i].len, 4u);
    }
    base_slot += set->count;
  }

  free(mr->stage_owner);
  mr->stage_owner = arena;
  struct fifa96_match_run_render *r = &mr->render;
  r->frames = frames;
  r->banks = banks;
  r->bank_count = total;
  r->fixed60 = NULL;   /* runtime-populated records, not ISO assets (FU-85 leg 8) */
  r->fixed61 = NULL;
  r->mirror = NULL;    /* 0x10F2E7 is executable data, not on the ISO (FU-85 §1.3) */
  r->sprite_data = arena;
  r->sprite_data_len = (uint32_t)arena_len;
  r->palette_ready = palette_ready;
  if (palette_ready) memcpy(r->palette, palette, sizeof r->palette);
  r->hud_font[0] = hud_font[0];
  r->hud_font[1] = hud_font[1];
  r->hud_font_ready[0] = hud_font_ready[0];
  r->hud_font_ready[1] = hud_font_ready[1];
  r->hud_bar = hud_bar;
  r->hud_bar_height = hud_bar_height;
  r->hud_bar_ready = hud_bar_ready;
  (void)fifa96_window_init(&r->window, s->width, s->height);
  (void)fifa96_window_define_full(&r->window, s->width, s->height);
  r->enabled = 1;
  arena = NULL;
  rc = 0;

out_sets:
  match_stage_set_free(&player);
  match_stage_set_free(&pitch);
out_cache:
  free(arena);
  fifa96_cache_destroy(cache);
  return rc;
}

/* FU-150 P2 — fouls / referee / offside: the engine seam over the
 * `fifa96_referee` module. The module owns the native cells (registrar,
 * decision, the two sequence machines); the engine owns the machine dispatch,
 * the staged inputs (settings config, the pool record views, the derived
 * nearest queries), the act-2 free-kick/penalty hand-off and the frame
 * cadence. See include/fifa96_engine/fifa96_match_run.h for the contracts and
 * docs/ghidra/FU150_fouls_referee_offside.md for the native evidence. */

/* The engine record view of one encoded entity id. `duel_ok` is staged 1: the
 * native 0x14C360/0x157BD2 compare tables have no ported producer (FU-150 leg
 * 4-adjacent); their producers are unported, so the derived equality holds. */
static int match_run_ref_view(const struct fifa96_match_run *mr, int32_t id,
                              struct fifa96_ref_record *view) {
  const struct fifa96_match_entity *e;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  e = &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
           .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  view->id = id;
  view->side = mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS].side;
  view->player = e->index;
  view->active = e->active;
  view->duel_ok = 1;
  return 1;
}

/* Apply one sequence step's requests: the whistle/speech observation slots,
 * the phase write with its FU-149 arm (the native FUN_000740A0 runs the
 * per-team FUN_0008D098 immediately), the record install on rec_first and the
 * situation dispatch through the shared set-piece entry (BX=1, the native
 * FUN_0008A938 argument; its direct path may start the act-2 machine). The
 * native 0x4C374/0x651F0/0x974F0/0x92040/0x6E724/0x7D388 sink calls and the
 * dropped 0x14C3A0 stats write stay unported (FU-150 legs 3/10). */
static int match_run_ref_apply(struct fifa96_match_run *mr,
                               const struct fifa96_ref_sequence_out *out) {
  int rc;
  if (out->whistle != 0u) mr->ref_whistle = 0x1Eu;
  if (out->speech_code != 0u) mr->ref_speech = out->speech_code;
  /* The native stage-0 order: the phase write runs before the record install
   * (0x8A091 vs 0x8A0A0), and the installer's +0x98 clear reads the new phase
   * cell. FUN_000740A0 stores the side byte at [0x157AAF] (=
   * `[0x157AAC]>>24`, the arm's controlled-side gate 0x8D728) before the
   * per-team FUN_0008D098 arms, so the arm installs on the request's side. */
  if (out->phase_write != 0xFFu) {
    mr->phase_machine.side_controlled = out->phase_side;
    rc = match_run_write_phase(mr, out->phase_write);
    if (rc != 0) return rc;
    rc = fifa96_match_run_phase_arm(mr, out->phase_write);
    if (rc != 0) return rc;
  }
  if (out->install_action != 0u) {
    int32_t id = mr->referee.rec_first;
    if (id >= 0 &&
        id < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
      struct fifa96_match_entity *e =
          &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
      (void)fifa96_match_entities_install(e, mr->state.phase,
                                          out->install_action, 0);
    }
  }
  if (out->situation != 0xFFu) {
    rc = fifa96_match_run_set_piece(mr, out->situation, out->situation_side, 1u);
    if (rc != 0) return rc;
  }
  return 0;
}

/* The derived act-2 (phase-0x18) hand-off, started by the sit-9/sit-0xA table
 * rows with the row's stage (0 for sit 9, 1 for sit 0xA — the native
 * FUN_000888FC(2, stage, 1)):
 *  - stage 0 (0x89318/0x8935A): the soft-foul whistle then phase 0xA on the
 *    fouled side (the native then cascades through the stage 1/2 camera-lead
 *    gates; the port derives one stage per granted frame, FU-150 erratum);
 *  - stage 1 (0x894A2..0x895AC): the decision. The 0x894A2 gate's
 *    [0x158882] has no direct writer in the image (census = 1 read), so the
 *    derived condition is `foul_kind == 3 || session gate` (leg). Phase 7 by
 *    default; phase 6 when the contact kind is not 3, |incident x| < 0x420
 *    and z lies in the fouler-side band [-0xB10,-0x7B0] side 0 /
 *    [0x7B0,0xB10] side 1 (0x894DB..0x89550, verified). Speech 0x23 (6) /
 *    0x2A (7) on rec_second, [0x15882A] cleared (0x8955F) and the FU-149
 *    phase arm installs the taker/keeper. */
static int match_run_ref_step_restart(struct fifa96_match_run *mr) {
  int rc;
  if (mr->ref_restart_stage == 0u) {
    if (mr->referee.foul_kind == 0u) mr->ref_whistle = 0x1Eu;
    /* 0x8935A: FUN_000740A0(0xA, fouled side) — the side lands in [0x157AAF]
     * before the arm. */
    mr->phase_machine.side_controlled = (uint8_t)((mr->referee.rec_first_side ^ 1u) & 1u);
    rc = match_run_write_phase(mr, 0x0Au);
    if (rc != 0) return rc;
    mr->ref_restart_stage = 1u;
    return 1;
  }
  if (!(mr->referee.foul_kind == 3u || mr->session_gate_14c32a != 0u)) {
    mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;   /* 0x894BE -> 0x89615 */
    return 1;
  }
  {
    uint8_t phase = 7u;
    if (mr->referee.foul_kind != 3u) {
      int32_t ax = mr->incident_x;
      if (ax < 0) ax = -ax;
      if (ax < 0x420) {
        int32_t z = mr->incident_z;
        if (mr->referee.rec_first_side == 0u) {
          if (z >= -0xB10 && z <= -0x7B0) phase = 6u;
        } else {
          if (z >= 0x7B0 && z <= 0xB10) phase = 6u;
        }
      }
    }
    /* 0x89590..0x895AC: team byte of [0x15888F] ^ 1 passed to FUN_000740A0
     * (the fouled side) before the phase-7/6 taker arm gates on it. */
    mr->phase_machine.side_controlled = (uint8_t)((mr->referee.rec_first_side ^ 1u) & 1u);
    rc = match_run_write_phase(mr, phase);
    if (rc != 0) return rc;
    rc = fifa96_match_run_phase_arm(mr, phase);
    if (rc != 0) return rc;
    mr->ref_speech = phase == 6u ? 0x23u : 0x2Au;
    mr->global_5882a = 0;                          /* 0x8955F */
  }
  mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
  return 1;
}

int fifa96_match_run_contact(struct fifa96_match_run *mr, uint8_t kind,
                             int32_t rec_a, int32_t rec_b, const int32_t point[3]) {
  struct fifa96_ref_record fouler;
  struct fifa96_ref_record victim;
  struct fifa96_ref_decision_out out;
  uint16_t draw = 0;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!match_run_ref_view(mr, rec_a, &fouler)) return -FIFA96_ERR_INVALID;
  if (!match_run_ref_view(mr, rec_b, &victim)) memset(&victim, 0, sizeof victim);
  rc = fifa96_ref_contact_register(&mr->referee, kind, rec_a,
                                   rec_b >= 0 ? rec_b : 0, point);
  if (rc != 0) return rc;
  mr->referee.phase = mr->state.phase;
  if (mr->config.field_4c306 == 0) {          /* 0x81E8A settings gate */
    mr->referee.recall_consumed = 0;
    return 0;
  }
  (void)fifa96_rng_step(&mr->rng, &draw);     /* 0x81EA6: the 1-in-8 skip */
  if ((draw & 7u) == 0u) {
    mr->referee.recall_consumed = 0;          /* 0x81ECD */
    return 0;
  }
  /* The row-0x0C re-call literal kind 1 (0x81EB4) and its severity draw
   * (FUN_00092AC8; consumed only when severity can run). */
  mr->referee.contact_kind = 1u;
  if (mr->config.field_4c306 > 1) (void)fifa96_rng_step(&mr->rng, &draw);
  memset(&out, 0, sizeof out);
  rc = fifa96_ref_foul_decide(&mr->referee, &mr->config, &fouler, &victim, point,
                              (uint8_t)draw, &out);
  if (rc < 0) return rc;
  mr->referee.recall_consumed = 1;            /* 0x81EC4 */
  mr->incident_x = mr->referee.point[0];      /* [0x158897] mirror */
  mr->incident_z = mr->referee.point[2];      /* [0x15889F] mirror */
  if (out.sequence == FIFA96_REF_SEQ_ACT3) {
    /* FUN_000888FC(3,0,1) invoke-now: stage 0 runs on this call. */
    mr->referee.sequence = FIFA96_REF_SEQ_ACT3;
    mr->referee.stage = 0;
    mr->ref_machine = FIFA96_MATCH_RUN_REF_FOUL;
    return fifa96_match_run_referee_step(mr);
  }
  if (out.restart != 0u) {
    /* 0x8A729: FUN_0008A938(9, fouled side, BX=1) through the P1 shared entry
     * (whose direct path may start the act-2 machine). */
    rc = fifa96_match_run_set_piece(mr, 9u, out.restart_side, 1u);
    if (rc != 0) return rc;
    return 1;
  }
  return 0;
}

int fifa96_match_run_offside_reception(struct fifa96_match_run *mr, int32_t receiver,
                                       const struct fifa96_ref_metric *metric,
                                       int32_t camera_ref, uint8_t mirror,
                                       uint8_t *offside) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  struct fifa96_ref_receiver rec;
  struct fifa96_ref_record event_rec;
  struct fifa96_ref_event_out ev_out;
  const struct fifa96_match_entity *own;
  int32_t rec_team, other_team, own_idx = -1, own_id = -1, last_defender_z = 0;
  int16_t best = 0;
  uint16_t draw = 0;
  int idx, rc;
  uint8_t result = 0;
  if (!mr || !metric || !offside) return -FIFA96_ERR_INVALID;
  *offside = 0;
  if (receiver < 0 ||
      receiver >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  rec_team = receiver / FIFA96_MATCH_ENTITY_RECORDS;
  other_team = rec_team ^ 1;
  /* The native FUN_00079D5C pre-gates (0x79D68/0x79D79/0x79D87) run before
   * the RNG draw; keep the same order so no draw is consumed on a gate. */
  mr->referee.phase = mr->state.phase;
  if (mr->state.phase != 2u) return 0;
  if (mr->referee.offside_suppress != 0) return 0;
  if (mr->config.field_4c2f2 == 0) return 0;
  memset(&rec, 0, sizeof rec);
  rec.z = mr->entities.team[rec_team].records[receiver % FIFA96_MATCH_ENTITY_RECORDS].pos_z;
  rec.side = mr->entities.team[rec_team].side;
  /* The derived FUN_0008DE8C(&0x157770, own team, 0): the own-team nearest to
   * the ball triple (the derived stand-in for 0x157770, FU-150 leg 8), skip
   * index 0. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[rec_team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  idx = fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                   (int16_t)mr->entities.ball.x,
                                   (int16_t)mr->entities.ball.z, &best);
  if (idx >= 0) {
    own = &mr->entities.team[rec_team].records[idx];
    own_idx = idx;
    own_id = (int32_t)(rec_team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
    rec.own_nearest_valid = 1;
    rec.own_nearest_is_receiver = idx == (int)(receiver % FIFA96_MATCH_ENTITY_RECORDS);
    rec.own_nearest_z = own->pos_z;
    rec.own_distance = best;
    /* the 0x79EEF..0x79F1D record-state gate (the [0x158777] identity is
     * staged 0/unported). */
    rec.eligible = (own->type != 0x11u && own->code != 0x10u &&
                    own->code != 0x1Du && own->code != 0x1Eu)
                       ? 1u
                       : 0u;
  }
  /* The derived FUN_0008DE28(0xB10|-0xB10, other team, 0): the opponent
   * nearest to (0, +0xB10) for a side-0 receiver and (0, -0xB10) for side 1. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[other_team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  idx = fifa96_entity_find_nearest(
      candidates, FIFA96_MATCH_ENTITY_RECORDS, 0, 0,
      rec.side == 0u ? (int16_t)0xB10 : (int16_t)-0xB10, &best);
  if (idx >= 0)
    last_defender_z = mr->entities.team[other_team].records[idx].pos_z;
  (void)fifa96_rng_step(&mr->rng, &draw);     /* the tolerance draw */
  rc = fifa96_ref_offside_check(&mr->referee, &mr->config, &rec, metric,
                                last_defender_z, camera_ref, mirror, (uint8_t)draw,
                                &result);
  if (rc != 0) return rc;
  if (result == 0u) return 0;
  if (own_id < 0) return 0;   /* the check requires a valid own-nearest */
  /* The kind-3 event fires on the own-team nearest (first-hand 0x79F29
   * `MOV EDX,ESI`, the slice's receiver reading is the recorded erratum). */
  memset(&event_rec, 0, sizeof event_rec);
  event_rec.id = own_id;
  event_rec.side = mr->entities.team[rec_team].side;
  event_rec.player = mr->entities.team[rec_team].records[own_idx].index;
  event_rec.active = mr->entities.team[rec_team].records[own_idx].active;
  event_rec.duel_ok = 1;
  {
    int32_t point[3];
    point[0] = mr->entities.team[rec_team].records[own_idx].pos_x;
    point[1] = mr->entities.team[rec_team].records[own_idx].pos_y;
    point[2] = mr->entities.team[rec_team].records[own_idx].pos_z;
    rc = fifa96_ref_offside_event(&mr->referee, &mr->config, &event_rec, point,
                                  &ev_out);
    if (rc < 0) return rc;
  }
  mr->incident_x = mr->referee.point[0];
  mr->incident_z = mr->referee.point[2];
  mr->ref_machine = FIFA96_MATCH_RUN_REF_OFFSIDE;
  mr->referee.sequence = FIFA96_REF_SEQ_ACT6;
  mr->referee.stage = 0;
  rc = fifa96_match_run_referee_step(mr);     /* FUN_000888FC(6,..,1) */
  if (rc < 0) return rc;
  /* The native 0x8F188(0x15) request (0x8A743) precedes the act-6 invocation
   * and the step's head clears the request slots, so re-assert it after the
   * synchronous stage-0 call (the observation slot otherwise loses it). */
  mr->ref_speech = ev_out.speech_code;
  *offside = 1;
  return 1;
}

int fifa96_match_run_referee_step(struct fifa96_match_run *mr) {
  struct fifa96_ref_sequence_out out;
  uint8_t machine;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->ref_whistle = 0;
  mr->ref_speech = 0;
  mr->referee.phase = mr->state.phase;
  mr->referee.delta = mr->state.frame_delta;
  /* The [0x157A6A] countdown (0x7438D..0x743A5, `SUB word, AX` with the frame
   * delta; the port saturates instead of wrapping). */
  if (mr->referee.offside_suppress != 0u) {
    uint16_t d = mr->state.frame_delta;
    if (d >= mr->referee.offside_suppress) mr->referee.offside_suppress = 0;
    else mr->referee.offside_suppress =
        (uint16_t)(mr->referee.offside_suppress - d);
  }
  switch (mr->ref_machine) {
    case FIFA96_MATCH_RUN_REF_FOUL:
      machine = mr->ref_machine;
      rc = fifa96_ref_foul_sequence_step(&mr->referee, &out);
      if (rc < 0) return rc;
      rc = match_run_ref_apply(mr, &out);
      if (rc < 0) return rc;
      /* The apply may start the act-2 machine (stage 6's situation 0xA); do
       * not clear it. */
      if (out.done != 0u && mr->ref_machine == machine)
        mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
      return 1;
    case FIFA96_MATCH_RUN_REF_OFFSIDE:
      machine = mr->ref_machine;
      rc = fifa96_ref_offside_sequence_step(&mr->referee, &out);
      if (rc < 0) return rc;
      rc = match_run_ref_apply(mr, &out);
      if (rc < 0) return rc;
      if (out.done != 0u && mr->ref_machine == machine)
        mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
      return 1;
    case FIFA96_MATCH_RUN_REF_RESTART:
      return match_run_ref_step_restart(mr);
    default:
      return 0;
  }
}
