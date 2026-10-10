#include <stddef.h>
#include "fifa96_loader/fifa96_input.h"

void fifa96_input_init(struct fifa96_input *in) {
  int i;
  if (!in) return;
  for (i = 0; i < FIFA96_INPUT_PLAYERS; i++) {
    in->prev[i] = 0;
    in->edge[i] = 0;
  }
  in->held = 0;
  in->fresh = 0;
  in->held_latch = 0;
  in->fresh_latch = 0;
}

int fifa96_input_map(uint8_t raw, const uint8_t row[FIFA96_INPUT_MAP_ROW], uint8_t *mapped) {
  if (!row || !mapped) return -FIFA96_ERR_INVALID;
  *mapped = (uint8_t)((raw & 0xF0u) | row[raw & 0x0Fu]);
  return 0;
}

int fifa96_input_pack(const uint8_t *states, int count, uint32_t *packed) {
  int i;
  uint32_t out = 0;
  if (!states || !packed) return -FIFA96_ERR_INVALID;
  if (count < 0 || count > FIFA96_INPUT_MAX_DEVICES) return -FIFA96_ERR_INVALID;
  for (i = 0; i < count; i++) {
    uint32_t v = (uint32_t)((states[i] & 0x0Fu) >> 2);
    if (states[i] & 0xF0u) v |= 4u;
    out |= v << (i * 4);
  }
  *packed = out;
  return 0;
}

int fifa96_input_update(struct fifa96_input *in, const uint8_t current[FIFA96_INPUT_PLAYERS]) {
  int i;
  uint8_t held = 0;
  uint8_t fresh = 0;
  if (!in || !current) return -FIFA96_ERR_INVALID;
  for (i = 0; i < FIFA96_INPUT_PLAYERS; i++) {
    uint8_t e = current[i];
    if (in->prev[i] != 0 && e != 0) e = 0;
    in->edge[i] = e;
    in->prev[i] = current[i];
    held |= current[i];
    fresh |= e;
  }
  in->held = held;
  in->fresh = fresh;
  in->held_latch |= held;
  in->fresh_latch |= fresh;
  return 0;
}

int fifa96_input_event(const uint8_t flags[FIFA96_INPUT_CODES], int code, int players) {
  if (!flags) return -FIFA96_ERR_INVALID;
  if (code < 0 || code >= FIFA96_INPUT_CODES) return 0;
  if (code == 1 && players != 2) return 0;
  if (code == 3 && players == 1) return 0;
  return flags[code];
}

int fifa96_input_code_player(const uint32_t bitmap[FIFA96_INPUT_CODES], int code, int players) {
  int p;
  if (!bitmap) return -FIFA96_ERR_INVALID;
  if (code < 0 || code >= FIFA96_INPUT_CODES) return 0;
  if (players < 0) return 0;
  if (players > 32) players = 32;
  for (p = 0; p < players; p++) {
    if (bitmap[code] & (1u << p)) return p;
  }
  return 0;
}

int fifa96_input_lockout(uint8_t *state, int *frames) {
  if (!state || !frames) return -FIFA96_ERR_INVALID;
  if (*frames > 0) {
    *state = 0;
    (*frames)--;
  }
  return 0;
}

int fifa96_input_range_words(const int32_t cells[8], int gate, uint16_t words[2]) {
  uint16_t w0, w1;
  if (!cells || !words) return -FIFA96_ERR_INVALID;
  if (gate != 0) return 0;                      /* 0x1C9C2 CMP EAX,1 / JNZ RET */
  /* First-hand 0x1C9BC (the match-init FUN_00011B7C -> FUN_0001C9BC call):
   * both words zeroed first, then the per-side config arms (byte ORs). */
  w0 = 0;
  w1 = 0;
  if (cells[0] == 1) w0 = 2;                    /* 0x105278 == 1 */
  if (cells[0] == 2) w0 = (uint16_t)(w0 | 4);   /* == 2 (the walk-gate bit) */
  if (cells[1] != 0) w0 = (uint16_t)(w0 | 1);   /* 0x10527C != 0 */
  if (cells[2] == 1) w0 = (uint16_t)(w0 | 0x200);  /* 0x14C1D5 |= 2 */
  if (cells[5] == 1) w1 = (uint16_t)(w1 | 0x200);  /* 0x14C1D7 |= 2 */
  /* 0x10528C 1/2/3/4 -> byte [0x14C1D4] |= 0x20/0x80/0x40/0x8 */
  switch (cells[3]) {
    case 1: w0 = (uint16_t)(w0 | 0x20); break;
    case 2: w0 = (uint16_t)(w0 | 0x80); break;
    case 3: w0 = (uint16_t)(w0 | 0x40); break;
    case 4: w0 = (uint16_t)(w0 | 0x8); break;
    default: break;
  }
  /* 0x1052A0 1/2/3/4 -> byte [0x14C1D6] |= 0x20/0x80/0x40/0x8 */
  switch (cells[6]) {
    case 1: w1 = (uint16_t)(w1 | 0x20); break;
    case 2: w1 = (uint16_t)(w1 | 0x80); break;
    case 3: w1 = (uint16_t)(w1 | 0x40); break;
    case 4: w1 = (uint16_t)(w1 | 0x8); break;
    default: break;
  }
  if (cells[4] == 1) w0 = (uint16_t)(w0 | 0x10);   /* 0x105290 == 1 */
  if (cells[7] == 1) w1 = (uint16_t)(w1 | 0x10);   /* 0x1052A4 == 1 */
  words[0] = w0;
  words[1] = w1;
  return 1;
}
