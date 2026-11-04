#include "fifa96_loader/fifa96_music.h"
#include <string.h>

static int32_t music_abs32(int32_t value) {
  uint32_t bits = (uint32_t)value;
  return value < 0 ? (int32_t)(0u - bits) : value;
}

static int32_t music_sar16(int32_t value) {
  if (value < 0) return (int32_t)(~((~(uint32_t)value) >> 16));
  return (int32_t)((uint32_t)value >> 16);
}

static int32_t music_clamp_volume(int32_t value) {
  if (value < 0) return 0;
  if (value > (int32_t)FIFA96_MUSIC_VOLUME_MAX)
    return (int32_t)FIFA96_MUSIC_VOLUME_MAX;
  return value;
}

static int32_t music_rand(const struct fifa96_music *music) {
  if (music->backend && music->backend->rand)
    return music->backend->rand(music->backend->ctx);
  return 0;
}

static void music_play(const struct fifa96_music *music, int32_t id) {
  if (id > 0 && music->backend && music->backend->play)
    music->backend->play(music->backend->ctx, id);
}

static const struct fifa96_music_event *music_event(
    const struct fifa96_music *music, int32_t event) {
  if (!music->config.events) return NULL;
  if (event < 0 || (uint32_t)event >= music->config.event_count) return NULL;
  return &music->config.events[event];
}

static const struct fifa96_music_tempo *music_tempo(
    const struct fifa96_music *music, int32_t index) {
  if (!music->config.tempo) return NULL;
  if (index < 0 || (uint32_t)index >= FIFA96_MUSIC_TEMPO_MAX) return NULL;
  return &music->config.tempo[index];
}

static int32_t music_period(int32_t numerator, int32_t scale) {
  int32_t period = music_sar16((int32_t)((uint32_t)scale * 100u));
  if (period < 1) period = 1;
  return music_abs32(numerator / period);
}

void fifa96_music_init(struct fifa96_music *music,
                       const struct fifa96_music_config *config,
                       const struct fifa96_music_backend *backend) {
  if (!music) return;
  memset(music, 0, sizeof *music);
  if (config) {
    music->config = *config;
    if (music->config.track_count > FIFA96_MUSIC_TRACK_MAX)
      music->config.track_count = FIFA96_MUSIC_TRACK_MAX;
    if (music->config.event_count > FIFA96_MUSIC_EVENT_MAX)
      music->config.event_count = FIFA96_MUSIC_EVENT_MAX;
  }
  music->backend = backend;
  music->volume = (int32_t)FIFA96_MUSIC_VOLUME_MAX;
  music->state = FIFA96_MUSIC_STATE_RANDOM;
  for (uint32_t i = 0; i < FIFA96_MUSIC_TRACK_MAX; i++)
    music->track[i].handle = FIFA96_MUSIC_NO_HANDLE;
}

int fifa96_music_reset(struct fifa96_music *music, int32_t volume) {
  if (!music) return -(int)FIFA96_ERR_TRUNCATED;
  if (!music->enabled) {
    music->active = FIFA96_MUSIC_ACTIVE_DISABLED;
    return FIFA96_OK;
  }
  music->volume = music_clamp_volume(volume);
  for (uint32_t i = 0; i < music->config.track_count; i++) {
    if (music->backend && music->backend->arm)
      music->track[i].handle = music->backend->arm(music->backend->ctx, i);
    else
      music->track[i].handle = FIFA96_MUSIC_NO_HANDLE;
    music->track[i].last_volume = 0;
    music->track[i].last_pan = 0;
  }
  music->active = FIFA96_MUSIC_ACTIVE_STARTED;
  if (!music->paused) {
    music->pos = 0;
    music->limit = 0;
    music->state = FIFA96_MUSIC_STATE_RANDOM;
    fifa96_music_tempo_calc(music);
    fifa96_music_start_event(music, music->config.initial_event);
  }
  music->paused = 0;
  return FIFA96_OK;
}

int fifa96_music_start_event(struct fifa96_music *music, int32_t event) {
  if (!music) return -(int)FIFA96_ERR_TRUNCATED;
  if (music->paused) return FIFA96_OK;
  if (event < 0 || event >= (int32_t)FIFA96_MUSIC_EVENT_MAX)
    return -(int)FIFA96_ERR_TRUNCATED;
  if ((uint32_t)event >= music->config.event_count) return FIFA96_OK;
  const struct fifa96_music_event *record = music_event(music, event);
  if (!record || record->valid == 0) return FIFA96_OK;
  int32_t end = (int32_t)((uint32_t)record->time << 16) - 1;
  int32_t rate = (int32_t)((uint32_t)record->rate << 16);
  if (rate > 0 && end < music->pos) {
    music_play(music, record->id);
    return FIFA96_OK;
  }
  int32_t limit = (int32_t)((uint32_t)rate + (uint32_t)music->pos);
  if (end < limit) limit = end;
  music->limit = limit;
  music->state = FIFA96_MUSIC_STATE_EVENT;
  music->current_event = event;
  fifa96_music_tempo_calc(music);
  music_play(music, record->id);
  return FIFA96_OK;
}

int fifa96_music_tempo_calc(struct fifa96_music *music) {
  if (!music) return -(int)FIFA96_ERR_TRUNCATED;
  if (music->state == FIFA96_MUSIC_STATE_EVENT ||
      music->state == FIFA96_MUSIC_STATE_TEMPO) {
    const struct fifa96_music_event *record =
        music_event(music, music->current_event);
    if (music->state == FIFA96_MUSIC_STATE_TEMPO)
      music->limit = (int32_t)((uint32_t)(music_rand(music) & 0xFFFF) *
                               (uint32_t)music->config.limit_scale);
    const struct fifa96_music_tempo *entry =
        record ? music_tempo(music, record->tempo) : NULL;
    int32_t scale = 0;
    if (entry)
      scale = music->state == FIFA96_MUSIC_STATE_TEMPO ? entry->b : entry->a;
    int32_t numerator =
        (int32_t)((uint32_t)(record ? record->rate : 0) << 16);
    music->period = music_period(numerator, scale);
  } else if (music->state == FIFA96_MUSIC_STATE_RANDOM) {
    int32_t limit = (int32_t)((uint32_t)(music_rand(music) & 0xFFFF) *
                              (uint32_t)music->config.limit_scale);
    int32_t scale =
        limit > music->pos ? music->config.period_up : music->config.period_down;
    int32_t period = music_sar16((int32_t)((uint32_t)scale * 100u));
    if (period < 1) period = 1;
    int32_t diff = (int32_t)((uint32_t)limit - (uint32_t)music->pos);
    music->limit = limit;
    music->period = music_abs32(diff) / period;
  }
  return FIFA96_OK;
}

int fifa96_music_tick(struct fifa96_music *music) {
  if (!music) return -(int)FIFA96_ERR_TRUNCATED;
  if (!music->enabled || !music->active || music->paused) return FIFA96_OK;
  if ((int)music->state > (int)FIFA96_MUSIC_STATE_RANDOM) return FIFA96_OK;
  int32_t up = (int32_t)((uint32_t)music->pos + (uint32_t)music->period);
  int32_t down = (int32_t)((uint32_t)music->pos - (uint32_t)music->period);
  if (music->pos < music->limit) {
    music->pos = up;
    if (up < music->limit) return FIFA96_OK;
  } else {
    music->pos = down;
    if (down > music->limit) return FIFA96_OK;
  }
  if (music->state == FIFA96_MUSIC_STATE_EVENT)
    music->state = FIFA96_MUSIC_STATE_TEMPO;
  else if (music->state == FIFA96_MUSIC_STATE_TEMPO)
    music->state = FIFA96_MUSIC_STATE_RANDOM;
  fifa96_music_tempo_calc(music);
  return FIFA96_OK;
}
