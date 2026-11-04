#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_MATCH_PACE_STEP 0x102u
#define FIFA96_MATCH_PACE_FRAME 0x35Cu

struct fifa96_match_pace {
  uint32_t acc;
  uint32_t pending;
  int hold;
};

void fifa96_match_pace_init(struct fifa96_match_pace *p);
int fifa96_match_pace_hold(struct fifa96_match_pace *p);
int fifa96_match_pace_resume(struct fifa96_match_pace *p);
int fifa96_match_pace_tick(struct fifa96_match_pace *p, int blocked, int *granted);
uint32_t fifa96_match_pace_pending(const struct fifa96_match_pace *p);
