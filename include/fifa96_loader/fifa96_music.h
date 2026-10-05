#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_MUSIC_TRACK_MAX 4u
#define FIFA96_MUSIC_EVENT_MAX 16u
#define FIFA96_MUSIC_TEMPO_MAX 8u
#define FIFA96_MUSIC_VOLUME_MAX 0x7Fu
#define FIFA96_MUSIC_ACTIVE_STARTED 1
#define FIFA96_MUSIC_ACTIVE_DISABLED 0xD0
#define FIFA96_MUSIC_NO_HANDLE (-1)

enum fifa96_music_state {
  FIFA96_MUSIC_STATE_EVENT = 0,
  FIFA96_MUSIC_STATE_TEMPO = 1,
  FIFA96_MUSIC_STATE_RANDOM = 2
};

struct fifa96_music_event {
  int32_t valid;
  int32_t rate;
  int32_t time;
  int32_t tempo;
  int32_t id;
};

struct fifa96_music_tempo {
  int32_t a;
  int32_t b;
};

struct fifa96_music_config {
  uint32_t track_count;
  uint32_t event_count;
  const struct fifa96_music_event *events;
  const struct fifa96_music_tempo *tempo;
  int32_t limit_scale;
  int32_t period_up;
  int32_t period_down;
  int32_t initial_event;
};

struct fifa96_music_backend {
  int32_t (*rand)(void *ctx);
  int32_t (*play)(void *ctx, int32_t id);
  int32_t (*arm)(void *ctx, uint32_t track);
  void *ctx;
};

struct fifa96_music_track {
  int32_t handle;
  int32_t last_volume;
  int32_t last_pan;
};

struct fifa96_music {
  struct fifa96_music_config config;
  const struct fifa96_music_backend *backend;
  int enabled;
  int paused;
  int active;
  int32_t volume;
  int32_t pos;
  int32_t limit;
  int32_t period;
  int32_t current_event;
  enum fifa96_music_state state;
  struct fifa96_music_track track[FIFA96_MUSIC_TRACK_MAX];
};

void fifa96_music_init(struct fifa96_music *music,
                       const struct fifa96_music_config *config,
                       const struct fifa96_music_backend *backend);

int fifa96_music_reset(struct fifa96_music *music, int32_t volume);

int fifa96_music_start_event(struct fifa96_music *music, int32_t event);

int fifa96_music_tempo_calc(struct fifa96_music *music);

int fifa96_music_tick(struct fifa96_music *music);
