#include "fifa96_loader/fifa96_pacing.h"

uint32_t fifa96_pacing_frames_due(uint32_t ticks) { return ticks * 15u / 100u; }

int fifa96_pacing_catch_up(uint32_t frames_before, uint32_t progress) {
  return progress >= frames_before + 2u;
}
