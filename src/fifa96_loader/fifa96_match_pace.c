#include <stddef.h>
#include "fifa96_loader/fifa96_match_pace.h"

void fifa96_match_pace_init(struct fifa96_match_pace *p) {
  if (!p) return;
  p->acc = 0;
  p->pending = 0;
  p->hold = 0;
}

int fifa96_match_pace_hold(struct fifa96_match_pace *p) {
  if (!p) return -FIFA96_ERR_INVALID;
  p->hold = 1;
  return 0;
}

int fifa96_match_pace_resume(struct fifa96_match_pace *p) {
  if (!p) return -FIFA96_ERR_INVALID;
  p->hold = 0;
  return 0;
}

int fifa96_match_pace_tick(struct fifa96_match_pace *p, int blocked, int *granted) {
  if (!p || !granted) return -FIFA96_ERR_INVALID;
  *granted = 0;
  if (p->hold || blocked) return 0;
  p->acc += FIFA96_MATCH_PACE_STEP;
  if (p->acc >= FIFA96_MATCH_PACE_FRAME) {
    p->acc -= FIFA96_MATCH_PACE_FRAME;
    p->pending++;
    *granted = 1;
  }
  return 0;
}

uint32_t fifa96_match_pace_pending(const struct fifa96_match_pace *p) {
  if (!p) return 0;
  return p->pending;
}
