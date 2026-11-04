#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_mixer.h"

#define FIFA96_VOICE_COUNT 16
#define FIFA96_VOICE_VOLUME_MAX 0xFFu
#define FIFA96_VOICE_PAN_MAX 0x7Fu

struct fifa96_voice {
  uint8_t state;
  uint8_t type;
  uint8_t caller;
  uint8_t volume;
  uint8_t gain;
  uint8_t pan;
  int32_t slide_step;
  int32_t slide_start;
  int32_t slide_target;
};

typedef void (*fifa96_voice_cleanup_fn)(void *ctx, int handle);

struct fifa96_voice_registry {
  int sound_state;
  uint8_t master;
  fifa96_voice_cleanup_fn cleanup;
  void *cleanup_ctx;
  struct fifa96_voice voice[FIFA96_VOICE_COUNT];
};

void fifa96_voice_registry_init(struct fifa96_voice_registry *registry);

int fifa96_voice_active(const struct fifa96_voice_registry *registry,
                        int handle);

int fifa96_voice_set_volume(struct fifa96_voice_registry *registry,
                            struct fifa96_mixer *mixer, int handle,
                            int32_t volume);

int fifa96_voice_set_pan(struct fifa96_voice_registry *registry,
                         struct fifa96_mixer *mixer, int handle, int32_t pan);

int fifa96_voice_start_slide(struct fifa96_voice_registry *registry, int handle,
                             int32_t target, int32_t frames);

int fifa96_voice_stop(struct fifa96_voice_registry *registry,
                      struct fifa96_mixer *mixer, int handle);

uint8_t fifa96_voice_gain(uint8_t global_gain, uint8_t scale, uint8_t pan);

void fifa96_voice_gain_packed(uint8_t pan, uint8_t gain, uint32_t *left,
                              uint32_t *right);

int fifa96_voice_apply_gains(struct fifa96_mixer *mixer, int handle,
                             uint8_t pan, uint8_t gain);
