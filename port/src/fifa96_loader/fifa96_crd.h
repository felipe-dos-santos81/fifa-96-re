#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_CRD_MAGIC 0x46445243u
#define FIFA96_CRD_VERSION 2u
#define FIFA96_CRD_HEADER_SIZE 0x38Cu
#define FIFA96_CRD_TEMPO_OFF 0x3Cu
#define FIFA96_CRD_TEMPO_STRIDE 8u
#define FIFA96_CRD_TRACK_OFF 0x7Cu
#define FIFA96_CRD_TRACK_STRIDE 0x74u
#define FIFA96_CRD_EVENT_OFF 0x24Cu
#define FIFA96_CRD_EVENT_STRIDE 0x14u
#define FIFA96_CRD_TRACK_MAX 4u
#define FIFA96_CRD_EVENT_MAX 16u
#define FIFA96_CRD_TEMPO_MAX 8u

struct fifa96_crd_header {
  uint32_t version;
  int32_t coeff_pos;
  int32_t coeff_ramp;
  int32_t position;
  int32_t intensity;
  int32_t limit;
  uint32_t track_count;
  uint32_t event_count;
  int32_t limit_scale;
  int32_t period_up;
  int32_t period_down;
  int32_t initial_event;
};

struct fifa96_crd_tempo {
  int32_t a;
  int32_t b;
};

struct fifa96_crd_event {
  int32_t valid;
  int32_t rate;
  int32_t time;
  int32_t tempo;
  int32_t id;
};

struct fifa96_crd_track {
  uint32_t eacs_off;
  uint32_t rate;
  uint8_t f8, f9, f10;
  int8_t voice;
  uint32_t sample_count;
  int32_t loop_start;
  uint32_t loop_len;
  uint32_t data_off;
  uint32_t data_len;
  uint8_t volume;
};

struct fifa96_crd {
  struct fifa96_crd_header header;
  struct fifa96_crd_tempo tempo[FIFA96_CRD_TEMPO_MAX];
  struct fifa96_crd_track track[FIFA96_CRD_TRACK_MAX];
  struct fifa96_crd_event event[FIFA96_CRD_EVENT_MAX];
  const uint8_t *src;
  size_t src_len;
};

int fifa96_crd_parse(const uint8_t *src, size_t src_len, struct fifa96_crd *crd);
uint32_t fifa96_crd_track_count(const struct fifa96_crd *crd);
uint32_t fifa96_crd_event_count(const struct fifa96_crd *crd);
int fifa96_crd_track(const struct fifa96_crd *crd, uint32_t index, struct fifa96_crd_track *out);
int fifa96_crd_event(const struct fifa96_crd *crd, uint32_t index, struct fifa96_crd_event *out);
int fifa96_crd_tempo(const struct fifa96_crd *crd, uint32_t index, struct fifa96_crd_tempo *out);
