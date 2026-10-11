#pragma once
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_cache.h"
#include "fifa96_engine/fifa96_clock.h"
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_intro.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_surface.h"

enum fifa96_engine_mode {
  FIFA96_ENGINE_MODE_INTRO = 0,
  FIFA96_ENGINE_MODE_FRONTEND = 1,
  FIFA96_ENGINE_MODE_QUIT = 2,
  FIFA96_ENGINE_MODE_MATCH = 3
};

struct fifa96_engine {
  struct fifa96_engine_config cfg;
  fifa96_platform *plat;
  int booted;
  int quit;
  uint64_t frames;
  struct fifa96_surface *surface;
  struct fifa96_asset_table *assets;
  struct fifa96_cache *cache;   /* asset bytes backing a live intro */
  struct fifa96_intro intro;
  int intro_active;             /* intro stream loaded and still stepping */
  uint32_t intro_frames;        /* video frames advanced at 15 fps */
  struct fifa96_frontend_run frontend;
  struct fifa96_match_run match_run; /* engine-owned run the bridge begins */
  struct fifa96_match_run *match; /* active match driver, NULL when none */
  enum fifa96_engine_mode mode;
  struct fifa96_engine_clock clock;
  uint64_t last_ns;      /* now_ns() at the previous step; 0 before the first */
  uint32_t step_ticks;   /* PIT ticks fired by the most recent step */
};

/* M2 interactive Task 1: the FU-77 shared-mover tables pinned first-hand from
 * /FIFA96.EXE (see fifa96_match_locomotion_tables.c). The heading flat
 * 0x1104D2 (32 bytes) and the 0x10F680 stride-velocity row (0x500 words). */
extern const uint8_t fifa96_match_heading_table[32];
extern const int16_t fifa96_match_stride_table[0x500];

/* M2 phase-11 T2 fix round 2 (FU-148 §16): the FUN_0008BAF0 head-error max
 * clamp shift count, `0x8BF41..47` = `MOV ECX,[EBP+0x76]; SAR ECX,6;
 * SAR AX,CL`, with the CL & 0x1F x86 mask (the 8086 uses the full CL; the
 * 186+ and this 32-bit image's 386+ mask to 5 bits). Exposed so the mask
 * itself is unit-pinned: the machine build carries no sanitizers and every
 * hardware shift masks the count, so an expression-level pin is the only
 * discriminating test for the `& 0x1F`. The native cell 0x158862 is the
 * low word of the dword the shift reads (`[EBP+0x76]` with the machine base
 * 0x1587EC); its high word (+0x78, 0x158864) is unmodeled but provably
 * irrelevant here — `>> 6` keeps bits 6..10 only, so bits >= 16 vanish
 * before `& 0x1F` (final review). Returns `(head_error >> 6) & 0x1F`. */
int32_t fifa96_match_run_ball_head_error_shift(int32_t head_error);
