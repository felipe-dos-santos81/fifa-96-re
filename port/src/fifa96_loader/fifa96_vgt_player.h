#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_tgv_stream.h"

#define FIFA96_VGT_PALETTE_SIZE 768u

typedef void (*fifa96_vgt_audio_fn)(void *user, const uint8_t *chunk, size_t chunk_len);

typedef struct {
  const uint8_t *pixels;
  size_t pixels_len;
  uint32_t width;
  uint32_t height;
  const uint8_t *palette;
  int palette_changed;
} fifa96_vgt_frame;

typedef struct {
  fifa96_tgv_walk walk;
  uint8_t *front;
  uint8_t *back;
  size_t front_cap;
  size_t back_cap;
  uint32_t width;
  uint32_t height;
  uint8_t palette[FIFA96_VGT_PALETTE_SIZE];
  fifa96_vgt_audio_fn on_audio;
  void *audio_user;
  int ended;
} fifa96_vgt_player;

int fifa96_vgt_player_init(fifa96_vgt_player *player, uint32_t width, uint32_t height,
                           uint8_t *canvas, size_t canvas_cap,
                           uint8_t *scratch, size_t scratch_cap,
                           fifa96_vgt_audio_fn on_audio, void *audio_user);
void fifa96_vgt_player_feed(fifa96_vgt_player *player, const uint8_t *data, size_t size);
int fifa96_vgt_player_step(fifa96_vgt_player *player, fifa96_vgt_frame *frame);
int fifa96_vgt_player_ended(const fifa96_vgt_player *player);
