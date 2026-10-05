// tests/test_settings.c — FU-49 settings table. Expected behavior is derived
// in docs/ghidra/FU49_sfx_events_settings.md: FUN_0001da58 set/cycle/
// decrement, FUN_0001ddc8 + FUN_0001de94 defaults, FUN_0001d940 getter.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_settings.h"

static void test_init(void) {
  struct fifa96_settings s;
  memset(&s, 0xAA, sizeof s);
  fifa96_settings_init(&s);
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++) {
    assert(s.value[i] == 0);
    assert(s.max[i] == 0);
  }
}

static void test_defaults(void) {
  struct fifa96_settings s;
  memset(&s, 0xAA, sizeof s);
  fifa96_settings_defaults(&s);
  int32_t exp[FIFA96_SETTINGS_COUNT] = {0};
  exp[0x00] = 1;
  exp[0x01] = 1;
  exp[0x02] = 1;
  exp[0x04] = 0;
  exp[0x06] = 1;
  exp[0x07] = 0;
  exp[0x08] = 2;
  exp[0x09] = 0;
  exp[0x0A] = 2;
  exp[0x0B] = 1;
  exp[0x0C] = 4;
  exp[0x0D] = 0;
  exp[0x0E] = 0;
  exp[0x0F] = 0;
  exp[0x10] = 0;
  exp[0x11] = 0x5B;
  exp[0x12] = 0x5D;
  exp[0x13] = 0x62;
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++) {
    assert(s.value[i] == exp[i]);
    assert(s.max[i] == 0);
  }
}

static void test_invalid(void) {
  struct fifa96_settings s;
  fifa96_settings_init(&s);
  int32_t out = -1;
  assert(fifa96_settings_set(&s, FIFA96_SETTINGS_COUNT, 1) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_settings_set(NULL, 0, 1) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_settings_get(&s, FIFA96_SETTINGS_COUNT, &out) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_settings_get(NULL, 0, &out) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_settings_get(&s, 0, NULL) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_settings_get(&s, 0, &out) == FIFA96_OK && out == 0);
}

static void test_store(void) {
  struct fifa96_settings s;
  fifa96_settings_init(&s);
  s.max[5] = 3;
  s.value[5] = 9;
  assert(fifa96_settings_set(&s, 5, 0) == FIFA96_OK && s.value[5] == 0);
  assert(fifa96_settings_set(&s, 5, 2) == FIFA96_OK && s.value[5] == 2);
  assert(fifa96_settings_set(&s, 5, 3) == FIFA96_OK && s.value[5] == 2);
  assert(fifa96_settings_set(&s, 5, -3) == FIFA96_OK && s.value[5] == 2);
  s.max[6] = 0;
  assert(fifa96_settings_set(&s, 6, 12345) == FIFA96_OK && s.value[6] == 12345);
  int32_t out = 0;
  assert(fifa96_settings_get(&s, 6, &out) == FIFA96_OK && out == 12345);
}

static void test_decrement(void) {
  struct fifa96_settings s;
  fifa96_settings_init(&s);
  s.max[5] = 3;
  assert(fifa96_settings_set(&s, 5, -1) == FIFA96_OK && s.value[5] == 2);
  assert(fifa96_settings_set(&s, 5, -1) == FIFA96_OK && s.value[5] == 1);
  assert(fifa96_settings_set(&s, 5, -1) == FIFA96_OK && s.value[5] == 0);
  assert(fifa96_settings_set(&s, 5, -1) == FIFA96_OK && s.value[5] == 2);
  s.value[6] = 5;
  s.max[6] = 0;
  assert(fifa96_settings_set(&s, 6, -1) == FIFA96_OK && s.value[6] == 4);
  s.value[7] = 0;
  s.max[7] = 0;
  assert(fifa96_settings_set(&s, 7, -1) == FIFA96_OK && s.value[7] == -1);
}

static void test_cycle(void) {
  struct fifa96_settings s;
  fifa96_settings_init(&s);
  s.max[5] = 3;
  s.value[5] = 2;
  assert(fifa96_settings_set(&s, 5, -2) == FIFA96_OK && s.value[5] == 0);
  s.value[5] = 0;
  assert(fifa96_settings_set(&s, 5, -2) == FIFA96_OK && s.value[5] == 1);
  s.value[5] = 1;
  assert(fifa96_settings_set(&s, 5, -2) == FIFA96_OK && s.value[5] == 2);
  s.max[6] = 0;
  s.value[6] = 4;
  assert(fifa96_settings_set(&s, 6, -2) == -FIFA96_ERR_UNSUPPORTED);
  assert(s.value[6] == 4);
}

static void test_sliders(void) {
  struct fifa96_settings s;
  fifa96_settings_init(&s);
  s.max[0x11] = 100;
  s.max[0x12] = 100;
  s.max[0x13] = 100;

  assert(fifa96_settings_set(&s, 0x11, -1) == FIFA96_OK && s.value[0x11] == 0);
  s.value[0x11] = 95;
  assert(fifa96_settings_set(&s, 0x11, -1) == FIFA96_OK && s.value[0x11] == 90);
  s.value[0x12] = 91;
  assert(fifa96_settings_set(&s, 0x12, -2) == FIFA96_OK && s.value[0x12] == 96);
  s.value[0x13] = 97;
  assert(fifa96_settings_set(&s, 0x13, -2) == FIFA96_OK && s.value[0x13] == 99);
  s.value[0x13] = 94;
  assert(fifa96_settings_set(&s, 0x13, -2) == FIFA96_OK && s.value[0x13] == 99);
  s.value[0x13] = 95;
  assert(fifa96_settings_set(&s, 0x13, -2) == FIFA96_OK && s.value[0x13] == 99);
  s.value[0x13] = 99;
  assert(fifa96_settings_set(&s, 0x13, -2) == FIFA96_OK && s.value[0x13] == 99);
  s.value[0x11] = 50;
  assert(fifa96_settings_set(&s, 0x11, 60) == FIFA96_OK && s.value[0x11] == 60);
  assert(fifa96_settings_set(&s, 0x11, 100) == FIFA96_OK &&
         s.value[0x11] == 60);

  struct fifa96_settings z;
  fifa96_settings_init(&z);
  z.value[0x11] = 3;
  z.max[0x11] = 0;
  assert(fifa96_settings_set(&z, 0x11, -2) == FIFA96_OK && z.value[0x11] == -1);
  z.value[0x12] = 5;
  z.max[0x12] = 0;
  assert(fifa96_settings_set(&z, 0x12, -1) == FIFA96_OK && z.value[0x12] == 0);
}

int main(void) {
  test_init();
  test_defaults();
  test_invalid();
  test_store();
  test_decrement();
  test_cycle();
  test_sliders();
  printf("test_settings OK\n");
  return 0;
}
