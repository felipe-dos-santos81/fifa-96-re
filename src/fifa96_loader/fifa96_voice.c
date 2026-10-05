#include "fifa96_loader/fifa96_voice.h"
#include <string.h>

static int voice_handle_ok(int handle) {
  return handle >= 0 && handle < FIFA96_VOICE_COUNT;
}

static int voice_gate_open(const struct fifa96_voice_registry *registry) {
  return registry->sound_state >= 1 && registry->sound_state <= 5;
}

void fifa96_voice_registry_init(struct fifa96_voice_registry *registry) {
  if (!registry) return;
  memset(registry, 0, sizeof *registry);
  registry->master = 0x7F;
}

int fifa96_voice_active(const struct fifa96_voice_registry *registry,
                        int handle) {
  if (!registry) return -(int)FIFA96_ERR_TRUNCATED;
  if (!voice_handle_ok(handle)) return -(int)FIFA96_ERR_VOICE_HANDLE;
  if (!voice_gate_open(registry)) return -(int)FIFA96_ERR_TRUNCATED;
  return registry->voice[handle].state == 1 ? 0 : 1;
}

uint8_t fifa96_voice_gain(uint8_t global_gain, uint8_t scale, uint8_t pan) {
  int32_t product = (int32_t)(int8_t)global_gain * (int32_t)(int8_t)scale *
                    (int32_t)(int8_t)pan;
  return (uint8_t)(product / 0x3F01);
}

void fifa96_voice_gain_packed(uint8_t pan, uint8_t gain, uint32_t *left,
                              uint32_t *right) {
  uint32_t packed = fifa96_mixer_pan_packed(pan, gain);
  if (left) *left = (uint32_t)(uint16_t)packed << 10;
  if (right) *right = (packed >> 16) << 10;
}

int fifa96_voice_apply_gains(struct fifa96_mixer *mixer, int handle,
                             uint8_t pan, uint8_t gain) {
  if (!mixer || !voice_handle_ok(handle)) return -(int)FIFA96_ERR_TRUNCATED;
  struct fifa96_mixer_voice *ch = &mixer->voices[handle];
  if (!ch->active) return FIFA96_OK;
  uint32_t packed = fifa96_mixer_pan_packed(pan, gain);
  ch->gain_l = (uint32_t)(uint16_t)packed;
  ch->gain_r = packed >> 16;
  return FIFA96_OK;
}

int fifa96_voice_set_volume(struct fifa96_voice_registry *registry,
                            struct fifa96_mixer *mixer, int handle,
                            int32_t volume) {
  if (!registry || !mixer) return -(int)FIFA96_ERR_TRUNCATED;
  if (!voice_handle_ok(handle)) return -(int)FIFA96_ERR_VOICE_HANDLE;
  if (!voice_gate_open(registry)) return -(int)FIFA96_ERR_TRUNCATED;
  if (volume < 0 || volume > (int32_t)FIFA96_VOICE_VOLUME_MAX)
    return -(int)FIFA96_ERR_VOICE_VOLUME;
  struct fifa96_voice *v = &registry->voice[handle];
  if (v->state == 0) return -(int)FIFA96_ERR_VOICE_STATE;
  v->pan = (uint8_t)volume;
  v->gain = fifa96_voice_gain(registry->master, v->volume, v->caller);
  return fifa96_voice_apply_gains(mixer, handle, v->pan, v->gain);
}

int fifa96_voice_set_pan(struct fifa96_voice_registry *registry,
                         struct fifa96_mixer *mixer, int handle, int32_t pan) {
  if (!registry || !mixer) return -(int)FIFA96_ERR_TRUNCATED;
  if (!voice_handle_ok(handle)) return -(int)FIFA96_ERR_VOICE_HANDLE;
  if (!voice_gate_open(registry)) return -(int)FIFA96_ERR_TRUNCATED;
  if (pan < 0 || pan > (int32_t)FIFA96_VOICE_PAN_MAX)
    return -(int)FIFA96_ERR_VOICE_PAN;
  struct fifa96_voice *v = &registry->voice[handle];
  if (v->state == 0) return -(int)FIFA96_ERR_VOICE_STATE;
  v->caller = (uint8_t)pan;
  v->gain = fifa96_voice_gain(registry->master, v->volume, v->caller);
  return fifa96_voice_apply_gains(mixer, handle, v->pan, v->gain);
}

int fifa96_voice_start_slide(struct fifa96_voice_registry *registry, int handle,
                             int32_t target, int32_t frames) {
  if (!registry) return -(int)FIFA96_ERR_TRUNCATED;
  if (!voice_handle_ok(handle)) return -(int)FIFA96_ERR_VOICE_HANDLE;
  if (!voice_gate_open(registry)) return -(int)FIFA96_ERR_TRUNCATED;
  struct fifa96_voice *v = &registry->voice[handle];
  if (v->state != 1) return -(int)FIFA96_ERR_VOICE_SLIDE;
  if (frames <= 0) frames = 1;
  int32_t caller = (int32_t)(int8_t)v->caller;
  v->slide_target = (int32_t)((uint32_t)target << 16);
  v->slide_start = (int32_t)((uint32_t)caller << 16);
  uint32_t delta = (uint32_t)target - (uint32_t)caller;
  int32_t numerator = (int32_t)(delta << 16);
  v->slide_step = numerator / frames;
  return FIFA96_OK;
}

int fifa96_voice_stop(struct fifa96_voice_registry *registry,
                      struct fifa96_mixer *mixer, int handle) {
  if (!registry || !mixer) return -(int)FIFA96_ERR_TRUNCATED;
  if (!voice_handle_ok(handle)) return -(int)FIFA96_ERR_VOICE_HANDLE;
  if (!voice_gate_open(registry)) return -(int)FIFA96_ERR_TRUNCATED;
  struct fifa96_voice *v = &registry->voice[handle];
  if (v->state == 0) return -(int)FIFA96_ERR_VOICE_STATE;
  fifa96_mixer_stop(mixer, handle);
  if (v->type == 2 && registry->cleanup)
    registry->cleanup(registry->cleanup_ctx, handle);
  return FIFA96_OK;
}
