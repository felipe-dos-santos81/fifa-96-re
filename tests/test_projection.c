// tests/test_projection.c — FU-88 matrix/projection model
// (docs/ghidra/FU88_projection.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_projection.h"

_Static_assert(offsetof(fifa96_projection_vec, y) == 4, "vec y");
_Static_assert(offsetof(fifa96_projection_vec, z) == 8, "vec z");
_Static_assert(offsetof(fifa96_projection_point, y) == 4, "screen y");
_Static_assert(FIFA96_PROJECTION_RECIP_COUNT == 1024, "recip count");
_Static_assert(FIFA96_PROJECTION_NEAR == 5, "near");

#define P_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
static int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];

static void test_sincos(void) {
  int32_t s, c;
  assert(fifa96_projection_sincos(0x0000, &s, &c) == FIFA96_OK);
  assert(s == 0 && c == 0x10000);
  assert(fifa96_projection_sincos(0x0001, &s, &c) == FIFA96_OK);
  assert(s == 6 && c == 0x10000);
  assert(fifa96_projection_sincos(0x1000, &s, &c) == FIFA96_OK);
  assert(s == 25079 && c == 60547);
  assert(fifa96_projection_sincos(0x2000, &s, &c) == FIFA96_OK);
  assert(s == 46340 && c == 46340);
  assert(fifa96_projection_sincos(0x4000, &s, &c) == FIFA96_OK);
  assert(s == 0x10000 && c == 0);
  assert(fifa96_projection_sincos(0x8001, &s, &c) == FIFA96_OK);
  assert(s == -7 && c == -0x10000);
  assert(fifa96_projection_sincos(0xC000, &s, &c) == FIFA96_OK);
  assert(s == -0x10000 && c == 0);
  assert(fifa96_projection_sincos(-0x4000, &s, &c) == FIFA96_OK);
  assert(s == -0x10000 && c == 0);
  assert(fifa96_projection_sincos(0x3456, &s, &c) == FIFA96_OK);
  assert(s == 62868 && c == 18506);
  assert(fifa96_projection_sincos(0, NULL, &c) == P_INVALID);
  assert(fifa96_projection_sincos(0, &s, NULL) == P_INVALID);
}

static void test_matrix(void) {
  int32_t m[9];
  assert(fifa96_projection_matrix(0, 0, m) == FIFA96_OK);
  const int32_t ident[9] = {0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x10000};
  assert(memcmp(m, ident, sizeof(m)) == 0);
  assert(fifa96_projection_matrix(0x2000, 0, m) == FIFA96_OK);
  const int32_t yaw45[9] = {46340, 0, -46340, 0, 0x10000, 0, 46340, 0, 46340};
  assert(memcmp(m, yaw45, sizeof(m)) == 0);
  assert(fifa96_projection_matrix(0, 0x4000, m) == FIFA96_OK);
  const int32_t pitch90[9] = {0x10000, 0, 0, 0, 0, -0x10000, 0, 0x10000, 0};
  assert(memcmp(m, pitch90, sizeof(m)) == 0);
  assert(fifa96_projection_matrix(0x1000, 0x2000, m) == FIFA96_OK);
  const int32_t both[9] = {60547, -17734, -17734, 0, 46340, -46340, 25079, 42812, 42812};
  assert(memcmp(m, both, sizeof(m)) == 0);
  assert(fifa96_projection_matrix(0, 0, NULL) == P_INVALID);
}

static void test_transform(void) {
  int32_t m[9];
  fifa96_projection_vec in;
  fifa96_projection_vec out;
  assert(fifa96_projection_matrix(0, 0, m) == FIFA96_OK);
  in.x = 1;
  in.y = 2;
  in.z = 3;
  assert(fifa96_projection_transform(m, &in, &out) == FIFA96_OK);
  assert(out.x == 1 && out.y == 2 && out.z == 3);
  assert(fifa96_projection_matrix(0x2000, 0, m) == FIFA96_OK);
  in.x = 0x10000;
  in.y = 0x10000;
  in.z = 0;
  assert(fifa96_projection_transform(m, &in, &out) == FIFA96_OK);
  assert(out.x == 46340 && out.y == 0x10000 && out.z == -46340);
  in.x = -1;
  in.y = -2;
  in.z = -3;
  assert(fifa96_projection_transform(m, &in, &out) == FIFA96_OK);
  assert(out.x == -3 && out.y == -2 && out.z == -2);
  assert(fifa96_projection_transform(NULL, &in, &out) == P_INVALID);
  assert(fifa96_projection_transform(m, NULL, &out) == P_INVALID);
  assert(fifa96_projection_transform(m, &in, NULL) == P_INVALID);
}

static void test_reciprocal(void) {
  assert(fifa96_projection_reciprocal(320, recip_x) == FIFA96_OK);
  assert(recip_x[0] == 0x200000 && recip_x[9] == 0x200000);
  assert(recip_x[10] == 1906501);
  assert(recip_x[255] == 81920);
  assert(recip_x[256] == 81601);
  assert(recip_x[512] == 40880);
  assert(recip_x[1023] == 20480);
  assert(fifa96_projection_reciprocal(200, recip_y) == FIFA96_OK);
  assert(recip_y[0] == 0x140000 && recip_y[255] == 51200);
  assert(recip_y[1023] == 12800);
  assert(fifa96_projection_reciprocal(0, recip_x) == P_INVALID);
  assert(fifa96_projection_reciprocal(-1, recip_x) == P_INVALID);
  assert(fifa96_projection_reciprocal(0x8000, recip_x) == P_INVALID);
  assert(fifa96_projection_reciprocal(320, NULL) == P_INVALID);
}

static void test_screen(void) {
  fifa96_projection_point center;
  fifa96_projection_point out;
  fifa96_projection_vec v;
  uint8_t visible;
  center.x = 160 << 16;
  center.y = 100 << 16;
  out.x = 12345;
  out.y = -12345;
  v.x = 513;
  v.y = 0;
  v.z = 0x2000;
  visible = 7;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(visible == 1 && out.x == 11796475 && out.y == 6553600);
  v.x = -513;
  v.y = 257;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(out.x == 9175045 && out.y == 6143204);
  v.x = 100;
  v.y = -50;
  v.z = 0x10000;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(out.x == 10517635 && out.y == 6563561);
  v.x = 1;
  v.y = 1;
  v.z = 0x3FF;
  visible = 7;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(visible == 1 && out.x == 10506240 && out.y == 6540800);
  v.z = 0x400;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(out.x == 10506160 && out.y == 6540850);
  v.z = 0x1000000;
  visible = 7;
  out.x = 42;
  out.y = 43;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(visible == 0 && out.x == 42 && out.y == 43);
  v.z = FIFA96_PROJECTION_NEAR - 1;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(visible == 0);
  v.z = FIFA96_PROJECTION_NEAR;
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, &visible) == FIFA96_OK);
  assert(visible == 1 && out.x == 12582912 && out.y == 5242880);
  assert(fifa96_projection_screen(NULL, recip_y, &center, &v, &out, &visible) == P_INVALID);
  assert(fifa96_projection_screen(recip_x, NULL, &center, &v, &out, &visible) == P_INVALID);
  assert(fifa96_projection_screen(recip_x, recip_y, NULL, &v, &out, &visible) == P_INVALID);
  assert(fifa96_projection_screen(recip_x, recip_y, &center, NULL, &out, &visible) == P_INVALID);
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, NULL, &visible) == P_INVALID);
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &v, &out, NULL) == P_INVALID);
}

static void test_project(void) {
  int32_t m[9];
  fifa96_projection_vec cam;
  fifa96_projection_vec world;
  fifa96_projection_point center;
  fifa96_projection_point out;
  uint8_t visible;
  center.x = 160 << 16;
  center.y = 100 << 16;
  assert(fifa96_projection_matrix(0, 0, m) == FIFA96_OK);
  cam.x = 0;
  cam.y = 0;
  cam.z = 0;
  world.x = 513;
  world.y = 0;
  world.z = 0x2000;
  visible = 7;
  assert(fifa96_projection_project(m, &cam, recip_x, recip_y, &center, &world, &out, &visible) ==
         FIFA96_OK);
  assert(visible == 1 && out.x == 11796475 && out.y == 6553600);
  cam.x = 513;
  cam.z = 0x2000;
  world.x = 1026;
  world.z = 0x4000;
  assert(fifa96_projection_project(m, &cam, recip_x, recip_y, &center, &world, &out, &visible) ==
         FIFA96_OK);
  assert(out.x == 11796475 && out.y == 6553600);
  world.z = cam.z + 4;
  visible = 7;
  assert(fifa96_projection_project(m, &cam, recip_x, recip_y, &center, &world, &out, &visible) ==
         FIFA96_OK);
  assert(visible == 0);
  assert(fifa96_projection_project(NULL, &cam, recip_x, recip_y, &center, &world, &out, &visible) ==
         P_INVALID);
}

int main(void) {
  test_sincos();
  test_matrix();
  test_transform();
  test_reciprocal();
  test_screen();
  test_project();
  puts("test_projection: all assertions passed");
  return 0;
}
