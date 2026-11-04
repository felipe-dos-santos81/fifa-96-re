#include "fifa96_loader/fifa96_sfx_event.h"
#include <string.h>

static int sfx_event_valid(const struct fifa96_sfx_events *events,
                           uint32_t event) {
  return events != NULL && event < FIFA96_SFX_EVENT_MAX;
}

void fifa96_sfx_event_init(struct fifa96_sfx_events *events,
                           const struct fifa96_sfx_event_backend *backend) {
  if (!events) return;
  memset(events, 0, sizeof *events);
  for (uint32_t i = 0; i < 8; i++)
    events->slot[i].handle = -1;
  events->backend = backend;
}

int fifa96_sfx_event_register(struct fifa96_sfx_events *events, uint32_t event,
                              int32_t id) {
  if (!sfx_event_valid(events, event)) return -(int)FIFA96_ERR_TRUNCATED;
  struct fifa96_sfx_event_slot *slot = &events->slot[event];
  if (slot->state != FIFA96_SFX_EVENT_NONE) return FIFA96_OK;
  slot->id = id;
  slot->state = FIFA96_SFX_EVENT_ALLOCATED;
  return FIFA96_OK;
}

int fifa96_sfx_event_register_all(struct fifa96_sfx_events *events,
                                  const int32_t *ids, uint32_t count) {
  if (!events) return -(int)FIFA96_ERR_TRUNCATED;
  if (!events->enabled) return FIFA96_OK;
  if (!ids) return -(int)FIFA96_ERR_TRUNCATED;
  if (count > FIFA96_SFX_EVENT_MAX) return -(int)FIFA96_ERR_TRUNCATED;
  for (uint32_t i = 0; i < count; i++) {
    if (ids[i] >= FIFA96_SFX_EVENT_ID_MAX) continue;
    fifa96_sfx_event_register(events, i, ids[i]);
  }
  events->count = count;
  events->armed = 1;
  return FIFA96_OK;
}

int fifa96_sfx_event_play(struct fifa96_sfx_events *events, uint32_t event) {
  if (!sfx_event_valid(events, event)) return -(int)FIFA96_ERR_TRUNCATED;
  if (!events->enabled || !events->armed) return FIFA96_OK;
  if (event >= events->count) return FIFA96_OK;
  if (events->audio <= 0) return FIFA96_OK;
  struct fifa96_sfx_event_slot *slot = &events->slot[event];
  if (slot->state == FIFA96_SFX_EVENT_NONE) return FIFA96_OK;
  const struct fifa96_sfx_event_backend *backend = events->backend;
  if (backend && backend->ready && !backend->ready(backend->ctx, slot->handle))
    return FIFA96_OK;
  if (!backend || !backend->play) return -(int)FIFA96_ERR_UNSUPPORTED;
  int32_t handle = backend->play(backend->ctx, slot->id);
  slot->handle = handle;
  slot->state =
      handle >= 0 ? FIFA96_SFX_EVENT_PLAYING : FIFA96_SFX_EVENT_STOPPED;
  return handle >= 0 ? FIFA96_OK : handle;
}

int fifa96_sfx_event_set_volume(struct fifa96_sfx_events *events,
                                uint32_t event, int32_t volume) {
  if (!sfx_event_valid(events, event)) return -(int)FIFA96_ERR_TRUNCATED;
  if (!events->enabled || !events->armed) return FIFA96_OK;
  if (volume < 0) volume = 0;
  if (volume > (int32_t)FIFA96_SFX_EVENT_VOLUME_MAX)
    volume = (int32_t)FIFA96_SFX_EVENT_VOLUME_MAX;
  struct fifa96_sfx_event_slot *slot = &events->slot[event];
  if (slot->state == FIFA96_SFX_EVENT_NONE) return FIFA96_OK;
  if (slot->handle <= 0) return FIFA96_OK;
  const struct fifa96_sfx_event_backend *backend = events->backend;
  if (!backend || !backend->volume) return -(int)FIFA96_ERR_UNSUPPORTED;
  backend->volume(backend->ctx, slot->handle, (uint32_t)volume);
  return FIFA96_OK;
}

int fifa96_sfx_event_set_param(struct fifa96_sfx_events *events,
                               uint32_t event, int32_t param) {
  if (!sfx_event_valid(events, event)) return -(int)FIFA96_ERR_TRUNCATED;
  if (!events->enabled || !events->armed) return FIFA96_OK;
  struct fifa96_sfx_event_slot *slot = &events->slot[event];
  if (slot->state == FIFA96_SFX_EVENT_NONE) return FIFA96_OK;
  if (slot->handle <= 0) return FIFA96_OK;
  const struct fifa96_sfx_event_backend *backend = events->backend;
  if (!backend || !backend->param) return -(int)FIFA96_ERR_UNSUPPORTED;
  backend->param(backend->ctx, slot->handle, param);
  return FIFA96_OK;
}

int fifa96_sfx_event_stop(struct fifa96_sfx_events *events, uint32_t event,
                          int32_t param) {
  if (!sfx_event_valid(events, event)) return -(int)FIFA96_ERR_TRUNCATED;
  if (!events->enabled || !events->armed) return FIFA96_OK;
  if (param < 0) param = 0;
  if (param > (int32_t)FIFA96_SFX_EVENT_VOLUME_MAX)
    param = (int32_t)FIFA96_SFX_EVENT_VOLUME_MAX;
  struct fifa96_sfx_event_slot *slot = &events->slot[event];
  const struct fifa96_sfx_event_backend *backend = events->backend;
  if (slot->state != FIFA96_SFX_EVENT_NONE && slot->handle > 0) {
    if (!backend || !backend->stop) return -(int)FIFA96_ERR_UNSUPPORTED;
    backend->stop(backend->ctx, slot->handle, (uint32_t)param);
    if (param < 1) {
      if (!backend->release) return -(int)FIFA96_ERR_UNSUPPORTED;
      backend->release(backend->ctx, slot->handle);
    }
  }
  if (param < 1) {
    slot->handle = -1;
    if (slot->state != FIFA96_SFX_EVENT_NONE)
      slot->state = FIFA96_SFX_EVENT_STOPPED;
  }
  return FIFA96_OK;
}
