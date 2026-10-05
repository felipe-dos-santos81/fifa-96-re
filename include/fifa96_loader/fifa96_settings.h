#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_SETTINGS_COUNT 26u
#define FIFA96_SETTINGS_SLIDER_FIRST 0x11u
#define FIFA96_SETTINGS_SLIDER_LAST 0x13u
#define FIFA96_SETTINGS_SLIDER_STEP 5

#define FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT 8u

extern const int32_t fifa96_match_config_half_lengths
    [FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT];

struct fifa96_settings {
  int32_t value[FIFA96_SETTINGS_COUNT];
  int32_t max[FIFA96_SETTINGS_COUNT];
};

struct fifa96_match_config {
  int32_t field_4c2f6;
  int32_t field_4c326;
  int32_t field_4c2e6;
  int32_t field_4c30a;
  int32_t field_4c306;
  int32_t field_4c2f2;
  int32_t field_4c312;
  int32_t field_4c316;
  int32_t clock_halt;
  int32_t flag_4c2ee;
  int32_t zero_4c2fe;
  int32_t zero_4c30e;
  int32_t zero_4c31a;
  int32_t zero_4c31e;
  int32_t half_length_minutes;
  int32_t period_length;
  int32_t extra_length;
};

void fifa96_settings_init(struct fifa96_settings *settings);

void fifa96_settings_defaults(struct fifa96_settings *settings);

int fifa96_settings_set(struct fifa96_settings *settings, uint32_t setting,
                        int32_t value);

int fifa96_settings_get(const struct fifa96_settings *settings,
                        uint32_t setting, int32_t *out);

int fifa96_settings_handoff(const struct fifa96_settings *settings,
                            uint32_t match_type,
                            struct fifa96_match_config *config);
