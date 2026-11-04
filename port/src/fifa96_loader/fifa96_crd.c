#include "fifa96_loader/fifa96_crd.h"
#include <string.h>

static uint32_t crd_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static int32_t crd_i32(const uint8_t *p) {
  return (int32_t)crd_u32(p);
}

static int crd_err(fifa96_err_t err) {
  return -(int)err;
}

static void crd_header_parse(const uint8_t *src, struct fifa96_crd_header *h) {
  h->version = crd_u32(src + 0x04);
  h->coeff_pos = crd_i32(src + 0x10);
  h->coeff_ramp = crd_i32(src + 0x14);
  h->position = crd_i32(src + 0x18);
  h->intensity = crd_i32(src + 0x1C);
  h->limit = crd_i32(src + 0x20);
  h->track_count = crd_u32(src + 0x24);
  h->event_count = crd_u32(src + 0x28);
  h->limit_scale = crd_i32(src + 0x2C);
  h->period_up = crd_i32(src + 0x30);
  h->period_down = crd_i32(src + 0x34);
  h->initial_event = crd_i32(src + 0x38);
}

static void crd_tempo_parse(const uint8_t *src, uint32_t i,
                            struct fifa96_crd_tempo *t) {
  const uint8_t *p = src + FIFA96_CRD_TEMPO_OFF + i * FIFA96_CRD_TEMPO_STRIDE;
  t->a = crd_i32(p);
  t->b = crd_i32(p + 4);
}

static void crd_event_parse(const uint8_t *src, uint32_t i,
                            struct fifa96_crd_event *e) {
  const uint8_t *p = src + FIFA96_CRD_EVENT_OFF + i * FIFA96_CRD_EVENT_STRIDE;
  e->valid = crd_i32(p + 0x00);
  e->rate = crd_i32(p + 0x04);
  e->time = crd_i32(p + 0x08);
  e->tempo = crd_i32(p + 0x0C);
  e->id = crd_i32(p + 0x10);
}

static int crd_track_parse(const uint8_t *src, size_t src_len, uint32_t i,
                           struct fifa96_crd_track *t) {
  const uint8_t *p = src + FIFA96_CRD_TRACK_OFF + i * FIFA96_CRD_TRACK_STRIDE;
  const uint8_t *e = p + 0x28;
  if (memcmp(e, "EACS", 4) != 0) return crd_err(FIFA96_ERR_BAD_MAGIC);
  t->eacs_off = FIFA96_CRD_TRACK_OFF + i * FIFA96_CRD_TRACK_STRIDE + 0x28;
  t->rate = crd_u32(e + 0x04);
  t->f8 = e[0x08];
  t->f9 = e[0x09];
  t->f10 = e[0x0A];
  t->voice = (int8_t)e[0x0B];
  t->sample_count = crd_u32(e + 0x0C);
  t->loop_start = crd_i32(e + 0x10);
  t->loop_len = crd_u32(e + 0x14);
  t->data_off = crd_u32(e + 0x18);
  t->volume = e[0x1D];
  t->data_len = 0;
  if (t->data_off < FIFA96_CRD_HEADER_SIZE || t->data_off > src_len)
    return crd_err(FIFA96_ERR_TRUNCATED);
  if (t->sample_count > src_len - t->data_off)
    return crd_err(FIFA96_ERR_TRUNCATED);
  return FIFA96_OK;
}

int fifa96_crd_parse(const uint8_t *src, size_t src_len, struct fifa96_crd *crd) {
  if (!src || !crd) return crd_err(FIFA96_ERR_TRUNCATED);
  if (src_len > 0xFFFFFFFFu) return crd_err(FIFA96_ERR_UNSUPPORTED);
  if (src_len < 4) return crd_err(FIFA96_ERR_TRUNCATED);
  if (crd_u32(src) != FIFA96_CRD_MAGIC) return crd_err(FIFA96_ERR_BAD_MAGIC);
  if (src_len < FIFA96_CRD_HEADER_SIZE) return crd_err(FIFA96_ERR_TRUNCATED);

  struct fifa96_crd tmp;
  memset(&tmp, 0, sizeof tmp);
  tmp.src = src;
  tmp.src_len = src_len;
  crd_header_parse(src, &tmp.header);
  if (tmp.header.version != FIFA96_CRD_VERSION)
    return crd_err(FIFA96_ERR_UNSUPPORTED);
  if (tmp.header.track_count > FIFA96_CRD_TRACK_MAX ||
      tmp.header.event_count > FIFA96_CRD_EVENT_MAX)
    return crd_err(FIFA96_ERR_TRUNCATED);

  for (uint32_t i = 0; i < FIFA96_CRD_TEMPO_MAX; i++)
    crd_tempo_parse(src, i, &tmp.tempo[i]);
  for (uint32_t i = 0; i < FIFA96_CRD_EVENT_MAX; i++)
    crd_event_parse(src, i, &tmp.event[i]);
  for (uint32_t i = 0; i < FIFA96_CRD_TRACK_MAX; i++) {
    int rc = crd_track_parse(src, src_len, i, &tmp.track[i]);
    if (rc != FIFA96_OK) return rc;
    if (i > 0 && tmp.track[i].data_off < tmp.track[i - 1].data_off)
      return crd_err(FIFA96_ERR_TRUNCATED);
  }
  for (uint32_t i = 0; i < FIFA96_CRD_TRACK_MAX; i++) {
    uint32_t end = (i + 1 < FIFA96_CRD_TRACK_MAX)
                       ? tmp.track[i + 1].data_off
                       : (uint32_t)src_len;
    tmp.track[i].data_len = end - tmp.track[i].data_off;
  }

  *crd = tmp;
  return FIFA96_OK;
}

uint32_t fifa96_crd_track_count(const struct fifa96_crd *crd) {
  return crd ? crd->header.track_count : 0;
}

uint32_t fifa96_crd_event_count(const struct fifa96_crd *crd) {
  return crd ? crd->header.event_count : 0;
}

int fifa96_crd_track(const struct fifa96_crd *crd, uint32_t index,
                     struct fifa96_crd_track *out) {
  if (!crd || !out) return crd_err(FIFA96_ERR_TRUNCATED);
  if (index >= FIFA96_CRD_TRACK_MAX) return crd_err(FIFA96_ERR_TRUNCATED);
  *out = crd->track[index];
  return FIFA96_OK;
}

int fifa96_crd_event(const struct fifa96_crd *crd, uint32_t index,
                     struct fifa96_crd_event *out) {
  if (!crd || !out) return crd_err(FIFA96_ERR_TRUNCATED);
  if (index >= FIFA96_CRD_EVENT_MAX) return crd_err(FIFA96_ERR_TRUNCATED);
  *out = crd->event[index];
  return FIFA96_OK;
}

int fifa96_crd_tempo(const struct fifa96_crd *crd, uint32_t index,
                     struct fifa96_crd_tempo *out) {
  if (!crd || !out) return crd_err(FIFA96_ERR_TRUNCATED);
  if (index >= FIFA96_CRD_TEMPO_MAX) return crd_err(FIFA96_ERR_TRUNCATED);
  *out = crd->tempo[index];
  return FIFA96_OK;
}
