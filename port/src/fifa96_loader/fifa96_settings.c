#include "fifa96_loader/fifa96_settings.h"
#include <string.h>

static int settings_valid(const struct fifa96_settings *settings,
                          uint32_t setting) {
  return settings != NULL && setting < FIFA96_SETTINGS_COUNT;
}

void fifa96_settings_init(struct fifa96_settings *settings) {
  if (!settings) return;
  memset(settings, 0, sizeof *settings);
}

void fifa96_settings_defaults(struct fifa96_settings *settings) {
  if (!settings) return;
  fifa96_settings_init(settings);
  static const struct {
    uint32_t setting;
    int32_t value;
  } defaults[] = {
      {0x00, 1}, {0x01, 1}, {0x02, 1}, {0x04, 0}, {0x06, 1},
      {0x07, 0}, {0x08, 2}, {0x09, 0}, {0x0A, 2}, {0x0B, 1},
      {0x0C, 4}, {0x0D, 0}, {0x0E, 0}, {0x0F, 0}, {0x10, 0},
  };
  for (size_t i = 0; i < sizeof defaults / sizeof defaults[0]; i++)
    settings->value[defaults[i].setting] = defaults[i].value;
  settings->value[0x11] = 0x5B;
  settings->value[0x12] = 0x5D;
  settings->value[0x13] = 0x62;
}

int fifa96_settings_set(struct fifa96_settings *settings, uint32_t setting,
                        int32_t value) {
  if (!settings_valid(settings, setting)) return -(int)FIFA96_ERR_TRUNCATED;
  int32_t max = settings->max[setting];
  if (setting >= FIFA96_SETTINGS_SLIDER_FIRST &&
      setting <= FIFA96_SETTINGS_SLIDER_LAST && value < 0) {
    int32_t current = settings->value[setting];
    if (value == -1) {
      current -= FIFA96_SETTINGS_SLIDER_STEP;
      if (current < 0) current = 0;
    } else {
      current += FIFA96_SETTINGS_SLIDER_STEP;
      if (max <= current) current = max - 1;
    }
    settings->value[setting] = current;
    return FIFA96_OK;
  }
  if (value == -1) {
    int32_t current = settings->value[setting];
    settings->value[setting] = current == 0 ? max - 1 : current - 1;
  } else if (value == -2) {
    if (max == 0) return -(int)FIFA96_ERR_UNSUPPORTED;
    settings->value[setting] = (settings->value[setting] + 1) % max;
  } else if ((value >= 0 && value < max) || max == 0) {
    settings->value[setting] = value;
  }
  return FIFA96_OK;
}

int fifa96_settings_get(const struct fifa96_settings *settings,
                        uint32_t setting, int32_t *out) {
  if (!settings_valid(settings, setting) || !out)
    return -(int)FIFA96_ERR_TRUNCATED;
  *out = settings->value[setting];
  return FIFA96_OK;
}

const int32_t fifa96_match_config_half_lengths
    [FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT] = {2, 4, 6, 8, 10, 20, 45, 0};

int fifa96_settings_handoff(const struct fifa96_settings *settings,
                            uint32_t match_type,
                            struct fifa96_match_config *config) {
  if (!settings || !config) return -(int)FIFA96_ERR_TRUNCATED;
  int32_t half_index = settings->value[0x0E];
  if (half_index < 0 ||
      (uint32_t)half_index >= FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT)
    return -(int)FIFA96_ERR_TRUNCATED;
  int competition = match_type == 4u;
  config->field_4c2f6 = settings->value[0x09];
  config->field_4c326 = settings->value[0x0F];
  config->field_4c2e6 = settings->value[0x03];
  config->field_4c30a = competition ? 0 : settings->value[0x0B];
  config->field_4c306 = competition ? 0 : settings->value[0x0A];
  config->field_4c2f2 = competition ? 0 : settings->value[0x10];
  config->field_4c312 = settings->value[0x02];
  config->field_4c316 = competition ? 1 : settings->value[0x06];
  config->clock_halt = competition ? 1 : settings->value[0x0D];
  config->flag_4c2ee = (match_type == 0u || match_type == 3u) ? 0 : 1;
  config->zero_4c2fe = 0;
  config->zero_4c30e = 0;
  config->zero_4c31a = 0;
  config->zero_4c31e = 0;
  int32_t half = competition ? 1
                             : fifa96_match_config_half_lengths[half_index];
  config->half_length_minutes = half;
  config->period_length = half * 0x3C;
  config->extra_length = half * 0x14;
  return FIFA96_OK;
}
