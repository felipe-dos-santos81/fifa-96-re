#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_SFX_EVENT_MAX 23u
#define FIFA96_SFX_EVENT_ID_MAX 0x17
#define FIFA96_SFX_EVENT_VOLUME_MAX 0x7Fu

enum fifa96_sfx_event_state {
  FIFA96_SFX_EVENT_NONE = 0,
  FIFA96_SFX_EVENT_ALLOCATED = 1,
  FIFA96_SFX_EVENT_PLAYING = 2,
  FIFA96_SFX_EVENT_STOPPED = 3
};

struct fifa96_sfx_event_backend {
  int32_t (*play)(void *ctx, int32_t id);
  int (*ready)(void *ctx, int32_t handle);
  int32_t (*volume)(void *ctx, int32_t handle, uint32_t volume);
  int32_t (*param)(void *ctx, int32_t handle, int32_t param);
  int32_t (*stop)(void *ctx, int32_t handle, uint32_t param);
  int32_t (*release)(void *ctx, int32_t handle);
  void *ctx;
};

struct fifa96_sfx_event_slot {
  int32_t id;
  int32_t handle;
  enum fifa96_sfx_event_state state;
};

struct fifa96_sfx_events {
  uint32_t count;
  int enabled;
  int armed;
  int audio;
  struct fifa96_sfx_event_slot slot[FIFA96_SFX_EVENT_MAX];
  const struct fifa96_sfx_event_backend *backend;
};

void fifa96_sfx_event_init(struct fifa96_sfx_events *events,
                           const struct fifa96_sfx_event_backend *backend);

int fifa96_sfx_event_register(struct fifa96_sfx_events *events, uint32_t event,
                              int32_t id);

int fifa96_sfx_event_register_all(struct fifa96_sfx_events *events,
                                  const int32_t *ids, uint32_t count);

int fifa96_sfx_event_play(struct fifa96_sfx_events *events, uint32_t event);

int fifa96_sfx_event_stop(struct fifa96_sfx_events *events, uint32_t event,
                          int32_t param);

int fifa96_sfx_event_set_volume(struct fifa96_sfx_events *events,
                                uint32_t event, int32_t volume);

int fifa96_sfx_event_set_param(struct fifa96_sfx_events *events,
                               uint32_t event, int32_t param);
