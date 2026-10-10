#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_INPUT_PLAYERS 4
#define FIFA96_INPUT_CODES 0x12
#define FIFA96_INPUT_MAP_ROW 16
#define FIFA96_INPUT_MAX_DEVICES 8

struct fifa96_input {
  uint8_t prev[FIFA96_INPUT_PLAYERS];
  uint8_t edge[FIFA96_INPUT_PLAYERS];
  uint8_t held;
  uint8_t fresh;
  uint8_t held_latch;
  uint8_t fresh_latch;
};

void fifa96_input_init(struct fifa96_input *in);
int fifa96_input_map(uint8_t raw, const uint8_t row[FIFA96_INPUT_MAP_ROW], uint8_t *mapped);
int fifa96_input_pack(const uint8_t *states, int count, uint32_t *packed);
int fifa96_input_update(struct fifa96_input *in, const uint8_t current[FIFA96_INPUT_PLAYERS]);
int fifa96_input_event(const uint8_t flags[FIFA96_INPUT_CODES], int code, int players);
int fifa96_input_code_player(const uint32_t bitmap[FIFA96_INPUT_CODES], int code, int players);
int fifa96_input_lockout(uint8_t *state, int *frames);

/* M2 phase-9 T3 (FU-148 §12.2 / FU-145 §1.8): FUN_0001C9BC's match-init
 * builder for the per-side input/range words [0x14C1D4] (player 0) and
 * [0x14C1D6] (player 1). The consumer bits are `(words[0] | words[1]) & 1`
 * (the FU-145 boundary reflect input), `& 2` (the FUN_000736AC interpolation
 * bit) and `& 4` (the FUN_000709D0 pan-step random walk gate). `cells` is the
 * eight config dwords 0x105278/0x10527C/0x105288/0x10528C/0x105290/0x10529C/
 * 0x1052A0/0x1052A4 (fresh get_xrefs_to = reads only in FUN_0001C9BC, 0x105278
 * has no static writer; the image is zero, so the derived words are zero until
 * a front-end config producer is staged). `gate` is the native [0x15753C]
 * entry test (`FUN_0006D1B2() == 1`, i.e. the cell equal to 0; the image/BSS
 * default): a nonzero gate leaves both words untouched. Returns 1 when the
 * words were built, 0 for the refused gate, -FIFA96_ERR_INVALID with both
 * words untouched on NULL. */
int fifa96_input_range_words(const int32_t cells[8], int gate, uint16_t words[2]);
