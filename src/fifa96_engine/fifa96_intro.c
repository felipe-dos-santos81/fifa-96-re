#include "fifa96_engine/fifa96_intro.h"

#include <string.h>

#define FIFA96_INTRO_W 320u
#define FIFA96_INTRO_H 240u
#define FIFA96_INTRO_PIXELS (FIFA96_INTRO_W * FIFA96_INTRO_H)

/* Companion 1SNh/1SNd audio is not mixed in M1; the player still consumes
 * the chunks so the video stream keeps advancing. Open leg: FU-37 §B. */
static void intro_on_audio(void *user, const uint8_t *chunk, size_t chunk_len) {
  (void)user;
  (void)chunk;
  (void)chunk_len;
}

int fifa96_intro_start(struct fifa96_intro *in, struct fifa96_surface *s) {
  if (!in || !s) return -1;
  int r = fifa96_vgt_player_init(&in->player, FIFA96_INTRO_W, FIFA96_INTRO_H,
                                 in->canvas, sizeof in->canvas,
                                 in->scratch, sizeof in->scratch,
                                 intro_on_audio, in);
  if (r != 0) return r;
  in->started = 1;
  return 0;
}

int fifa96_intro_feed(struct fifa96_intro *in, const uint8_t *stream, size_t len) {
  if (!in || !in->started || !stream || len == 0) return -1;
  fifa96_vgt_player_feed(&in->player, stream, len);
  return 0;
}

int fifa96_intro_step(struct fifa96_intro *in, struct fifa96_surface *s) {
  if (!in || !s) return -1;
  fifa96_vgt_frame frame;
  int r = fifa96_vgt_player_step(&in->player, &frame);
  if (r < 0) return r;
  if (r == 0) return 0;  /* stream ended (or already ended): surface untouched */
  if (!frame.pixels || frame.pixels_len < FIFA96_INTRO_PIXELS)
    return -(int)FIFA96_ERR_TRUNCATED;
  memcpy(s->indexed, frame.pixels, FIFA96_INTRO_PIXELS);
  if (frame.palette_changed) fifa96_surface_set_palette8(s, frame.palette);
  return 0;
}

int fifa96_intro_done(const struct fifa96_intro *in) {
  if (!in) return 1;
  return fifa96_vgt_player_ended(&in->player);
}
