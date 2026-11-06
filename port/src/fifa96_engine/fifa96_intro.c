#include "fifa96_engine/fifa96_intro.h"

#include <string.h>

#define FIFA96_INTRO_W 320u
#define FIFA96_INTRO_H 240u

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
  uint32_t w = frame.width;
  uint32_t h = frame.height;
  if (w == 0 || h == 0 || w > (uint32_t)s->width || h > (uint32_t)s->height)
    return -(int)FIFA96_ERR_TRUNCATED;
  size_t need = (size_t)w * (size_t)h;
  if (!frame.pixels || frame.pixels_len < need) return -(int)FIFA96_ERR_TRUNCATED;
  if (s->width == 320 && s->height == 240) {
    /* M1 tape path: preserve the original 1:1 top-left blit exactly (the
     * region outside the frame stays untouched), so the 320x240 intro frames
     * keep their pinned hashes. */
    for (uint32_t y = 0; y < h; y++) {
      memcpy(s->indexed + (size_t)y * (size_t)s->width,
             frame.pixels + (size_t)y * w, w);
    }
  } else if ((uint32_t)s->width == w && (uint32_t)s->height == h) {
    memcpy(s->indexed, frame.pixels, need);
  } else {
    /* Windowed 640x480 shell: the intro stream is still 320x240. Scale the
     * frame to fill the surface (nearest); VGA 320x200 footage therefore
     * gets the native 4:3 stretch instead of sitting in the top-left. */
    for (int y = 0; y < s->height; y++) {
      uint32_t sy = (uint32_t)(((int64_t)y * h) / s->height);
      if (sy >= h) sy = h - 1;
      const uint8_t *src = frame.pixels + (size_t)sy * w;
      uint8_t *dst = s->indexed + (size_t)y * (size_t)s->width;
      for (int x = 0; x < s->width; x++) {
        uint32_t sx = (uint32_t)(((int64_t)x * w) / s->width);
        if (sx >= w) sx = w - 1;
        dst[x] = src[sx];
      }
    }
  }
  if (frame.palette_changed) fifa96_surface_set_palette8(s, frame.palette);
  return 0;
}

int fifa96_intro_done(const struct fifa96_intro *in) {
  if (!in) return 1;
  return fifa96_vgt_player_ended(&in->player);
}

void fifa96_intro_abort(struct fifa96_intro *in) {
  if (!in) return;
  in->started = 0;
  in->player.ended = 1;
}
