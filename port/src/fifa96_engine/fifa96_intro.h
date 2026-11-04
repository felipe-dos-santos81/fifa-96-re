#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_vgt_player.h"

/* M1 intro playback: an owned VGT stream player that frames a 320x240
 * indexed surface. The stream bytes are borrowed (feed keeps the pointer),
 * so the caller must keep them alive until playback ends. */
struct fifa96_intro {
  fifa96_vgt_player player;
  uint8_t canvas[320 * 240];
  uint8_t scratch[320 * 240];
  int started;
};

int fifa96_intro_start(struct fifa96_intro *in, struct fifa96_surface *s);
int fifa96_intro_feed(struct fifa96_intro *in, const uint8_t *stream, size_t len);
int fifa96_intro_step(struct fifa96_intro *in, struct fifa96_surface *s);
int fifa96_intro_done(const struct fifa96_intro *in);

/* Marks playback finished (done() reports 1, further steps touch nothing)
 * without writing to the target surface — used for input-skip. */
void fifa96_intro_abort(struct fifa96_intro *in);
