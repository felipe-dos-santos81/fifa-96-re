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
