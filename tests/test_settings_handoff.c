// tests/test_settings_handoff.c — FU-68 settings -> match-config hand-off.
// Expected behavior is derived in docs/ghidra/FU68_settings_handoff.md:
// FUN_0003749C translation over caller-owned data.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_settings.h"

#define HALF_COUNT FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT

static void test_half_length_table(void) {
  static const int32_t expected[HALF_COUNT] = {2, 4, 6, 8, 10, 20, 45, 0};
  for (uint32_t i = 0; i < HALF_COUNT; i++)
    assert(fifa96_match_config_half_lengths[i] == expected[i]);
}

static void fill(struct fifa96_settings *s) {
  fifa96_settings_init(s);
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++)
    s->value[i] = (int32_t)(i * 3 + 1);
}

static void test_normal_mapping(void) {
  struct fifa96_settings s;
  fill(&s);
  s.value[0x0E] = 4;
  struct fifa96_match_config c;
  memset(&c, 0xAA, sizeof c);
  assert(fifa96_settings_handoff(&s, 2, &c) == FIFA96_OK);
  assert(c.field_4c2f6 == s.value[0x09]);
  assert(c.field_4c326 == s.value[0x0F]);
  assert(c.field_4c2e6 == s.value[0x03]);
  assert(c.field_4c30a == s.value[0x0B]);
  assert(c.field_4c306 == s.value[0x0A]);
  assert(c.field_4c2f2 == s.value[0x10]);
  assert(c.field_4c312 == s.value[0x02]);
  assert(c.field_4c316 == s.value[0x06]);
  assert(c.clock_halt == s.value[0x0D]);
  assert(c.flag_4c2ee == 1);
  assert(c.zero_4c2fe == 0);
  assert(c.zero_4c30e == 0);
  assert(c.zero_4c31a == 0);
  assert(c.zero_4c31e == 0);
  assert(c.half_length_minutes == 10);
  assert(c.period_length == 10 * 60);
  assert(c.extra_length == 10 * 20);
}

static void test_competition_override(void) {
  struct fifa96_settings s;
  fill(&s);
  s.value[0x0E] = 6;
  struct fifa96_match_config c;
  memset(&c, 0xAA, sizeof c);
  assert(fifa96_settings_handoff(&s, 4, &c) == FIFA96_OK);
  assert(c.field_4c2f6 == s.value[0x09]);
  assert(c.field_4c326 == s.value[0x0F]);
  assert(c.field_4c2e6 == s.value[0x03]);
  assert(c.field_4c312 == s.value[0x02]);
  assert(c.field_4c30a == 0);
  assert(c.field_4c306 == 0);
  assert(c.field_4c2f2 == 0);
  assert(c.field_4c316 == 1);
  assert(c.clock_halt == 1);
  assert(c.flag_4c2ee == 1);
  assert(c.half_length_minutes == 1);
  assert(c.period_length == 60);
  assert(c.extra_length == 20);
}

static void test_type_flag(void) {
  struct fifa96_settings s;
  fill(&s);
  s.value[0x0E] = 0;
  struct fifa96_match_config c;
  assert(fifa96_settings_handoff(&s, 0, &c) == FIFA96_OK);
  assert(c.flag_4c2ee == 0);
  assert(fifa96_settings_handoff(&s, 3, &c) == FIFA96_OK);
  assert(c.flag_4c2ee == 0);
  assert(fifa96_settings_handoff(&s, 1, &c) == FIFA96_OK);
  assert(c.flag_4c2ee == 1);
}

static void test_half_lengths(void) {
  struct fifa96_settings s;
  fill(&s);
  struct fifa96_match_config c;
  for (int32_t i = 0; i < (int32_t)HALF_COUNT; i++) {
    s.value[0x0E] = i;
    assert(fifa96_settings_handoff(&s, 2, &c) == FIFA96_OK);
    assert(c.half_length_minutes == fifa96_match_config_half_lengths[i]);
    assert(c.period_length == fifa96_match_config_half_lengths[i] * 60);
    assert(c.extra_length == fifa96_match_config_half_lengths[i] * 20);
  }
}

static void test_raw_values(void) {
  struct fifa96_settings s;
  fill(&s);
  s.value[0x0E] = 0;
  s.value[0x02] = -7;
  s.value[0x06] = -1;
  s.value[0x0D] = 123;
  struct fifa96_match_config c;
  assert(fifa96_settings_handoff(&s, 2, &c) == FIFA96_OK);
  assert(c.field_4c312 == -7);
  assert(c.field_4c316 == -1);
  assert(c.clock_halt == 123);
}

static void test_range(void) {
  struct fifa96_settings s;
  fill(&s);
  struct fifa96_match_config c;
  memset(&c, 0x5A, sizeof c);
  struct fifa96_match_config before;
  memcpy(&before, &c, sizeof c);
  s.value[0x0E] = (int32_t)HALF_COUNT;
  assert(fifa96_settings_handoff(&s, 2, &c) == -FIFA96_ERR_TRUNCATED);
  assert(memcmp(&c, &before, sizeof c) == 0);
  s.value[0x0E] = -1;
  assert(fifa96_settings_handoff(&s, 2, &c) == -FIFA96_ERR_TRUNCATED);
  assert(memcmp(&c, &before, sizeof c) == 0);
}

static void test_nulls(void) {
  struct fifa96_settings s;
  fill(&s);
  struct fifa96_match_config c;
  assert(fifa96_settings_handoff(NULL, 2, &c) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_settings_handoff(&s, 2, NULL) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_settings_handoff(NULL, 2, NULL) == -FIFA96_ERR_TRUNCATED);
}

int main(void) {
  test_half_length_table();
  test_normal_mapping();
  test_competition_override();
  test_type_flag();
  test_half_lengths();
  test_raw_values();
  test_range();
  test_nulls();
  printf("test_settings_handoff OK\n");
  return 0;
}
