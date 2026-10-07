#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_platform.h"

struct fifa96_platform_null_config {
  const fifa96_platform_key *tape;  /* scripted input; poll returns one entry per call */
  size_t tape_len;
  uint64_t step_ns;                 /* virtual clock step per now_ns() call */
  int audio_ring_frames;            /* default 4096 when 0 */
};

struct fifa96_platform_null_stats {
  uint64_t presents;
  uint64_t frames;        /* frames presented in total */
  uint64_t audio_frames;  /* PCM frames submitted */
  uint64_t present_hash;  /* FNV-1a 64 over planes + palette */
  uint64_t audio_hash;    /* FNV-1a 64 over PCM (little-endian bytes) */
};

fifa96_platform *fifa96_platform_null_create(const struct fifa96_platform_null_config *cfg);
void fifa96_platform_null_stats(const fifa96_platform *p, struct fifa96_platform_null_stats *out);
