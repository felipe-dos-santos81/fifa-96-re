#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_SETTINGS_COUNT 26u
#define FIFA96_SETTINGS_SLIDER_FIRST 0x11u
#define FIFA96_SETTINGS_SLIDER_LAST 0x13u
#define FIFA96_SETTINGS_SLIDER_STEP 5

struct fifa96_settings {
  int32_t value[FIFA96_SETTINGS_COUNT];
  int32_t max[FIFA96_SETTINGS_COUNT];
};

void fifa96_settings_init(struct fifa96_settings *settings);

void fifa96_settings_defaults(struct fifa96_settings *settings);

int fifa96_settings_set(struct fifa96_settings *settings, uint32_t setting,
                        int32_t value);

int fifa96_settings_get(const struct fifa96_settings *settings,
                        uint32_t setting, int32_t *out);
