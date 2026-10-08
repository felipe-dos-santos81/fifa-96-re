#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_match_run.h"
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
  rc = fifa96_match_dispatch_action(mr, e->code);
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
  e->install = r->install;
  e->helper_request = r->helper_request;
  e->controlled = r->controlled;
  e->place_valid = r->place_valid;
  e->place_x = r->place_x;
  e->place_y = r->place_y;
  e->place_z = r->place_z;
  e->dir_x = r->dir_x;
  e->dir_z = r->dir_z;
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
  mr->pass_parity = 0;
  mr->clock_period_ended = 0;          /* FU-143: no staged clock completion */
  mr->dispatched_ok = 0;
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
  /* FU-89 §kickoff placement / OL-T11-9: the derived kickoff pass (the act-1
   * ball spawn 0x1E0/0 plus the `FUN_0008CF60` per-record commit). */
  (void)fifa96_match_entities_kickoff_place(&mr->entities);
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
  mr->pass_parity = 0;                 /* FU-139 §11: fresh [0x157A4F] */
  mr->clock_period_ended = 0;          /* FU-143: fresh clock staging */
  mr->dispatched_ok = 0;               /* Task 15: fresh dispatch observation */
  fifa96_match_run_reset_input(mr);    /* fresh input edges/held and slot */
  match_run_release_stage(mr);         /* drop the previous match's staged arena */
  fifa96_match_run_reset_render(mr);   /* fresh camera/window/display/scene */
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
    rc = fifa96_match_state_set_phase(&mr->state, out.phase);
    if (rc != 0) return rc;
    /* The native [0x157A4D] switch byte is the phase dword's high byte; the
     * FU-142a machine keeps its own view, so both mirrors follow. */
    mr->phase_machine.state = out.phase;
    mr->phase_machine.phase = out.phase;
  }
  return 1;
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
    /* FU-139 §11: the native FUN_0004B100 toggles [0x157A4F] at frame entry
     * (0x4B11A/0x4B129), before the entity chain; row 06's claim arm reads it. */
    mr->pass_parity ^= 1u;
    (void)fifa96_control_slot_update(&mr->slot, mr->input_state[0],
                                     (uint8_t)mr->state.frame_delta, match_run_slot_map,
                                     match_run_anim_a, match_run_anim_b, match_run_anim_c);
    (void)fifa96_camera_update(&mr->render.camera, (int16_t)mr->state.frame_delta,
                               mr->render.view_class, mr->render.input_bit2);
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
    /* FU-142a: the FUN_0008D098 state 0x13/0x14 arm block runs after the
     * FU-141 chain, once per granted frame (the FUN_000740A0 order: the
     * state byte is set, then FUN_0008D098 runs for both teams). */
    if (mr->state.phase == 0x13u || mr->state.phase == 0x14u)
      (void)fifa96_match_phase_machine_step(mr);
    /* FU-143 wiring (Task 3): the clock's phase funnel, after the entity chain
     * (the native FUN_0008AF38 runs after the FU-67/entity block at 0x4B1A6
     * and inside it calls FUN_0008B9CC at 0x8B574). A completion staged above
     * runs the derived selector-0 chooser (2 -> 0x0C) on this frame. */
    rc = fifa96_match_run_phase_drive(mr);
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

int fifa96_match_run_render(struct fifa96_match_run *mr, struct fifa96_surface *s) {
  if (!mr || !s) return -FIFA96_ERR_INVALID;
  struct fifa96_match_run_render *r = &mr->render;
  if (!r->enabled) return 0;
  if (!r->frames || !r->banks || r->bank_count == 0 || !r->sprite_data ||
      r->sprite_data_len < 16u)
    return -FIFA96_ERR_STATE;

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
  return 0;
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

struct match_stage_set {
  struct match_stage_item *items;
  uint32_t count;       /* slots = BIGF entry count (indices stay stable) */
  uint32_t loaded;      /* slots with a decoded SHPI bank */
};

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
    if (fifa96_bigf_record(&info, i, &off, &size, NULL) != FIFA96_OK) goto fail;
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
  int rc;

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
