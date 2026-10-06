// tests/test_render.c — FU-85 match renderer model
// (docs/ghidra/FU85_match_renderer.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_render.h"

_Static_assert(offsetof(fifa96_render_entity, pos) == 0, "pos");
_Static_assert(offsetof(fifa96_render_entity, heading) == 12, "heading");
_Static_assert(offsetof(fifa96_render_entity, anim_id) == 16, "anim_id");
_Static_assert(offsetof(fifa96_render_slot, angle) == 12, "angle");
_Static_assert(offsetof(fifa96_render_slot, anim_id) == 16, "slot anim_id");
_Static_assert(offsetof(fifa96_render_bank, count) > offsetof(fifa96_render_bank, offsets), "count");
_Static_assert(offsetof(fifa96_render_bank, step) > offsetof(fifa96_render_bank, count), "step");
_Static_assert(offsetof(fifa96_render_frame, overlay) > offsetof(fifa96_render_frame, sprite),
               "overlay");
_Static_assert(offsetof(fifa96_render_frame, offset) > offsetof(fifa96_render_frame, overlay),
               "offset");
_Static_assert(offsetof(fifa96_render_frame, mirrored) > offsetof(fifa96_render_frame, overlay_offset),
               "mirrored");
_Static_assert(offsetof(fifa96_render_cover, src_x) == 16, "src_x");
_Static_assert(offsetof(fifa96_render_cover, mirrored_y) == 33, "mirrored_y");

#define R_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static const uint8_t bank_store[512];
static int32_t bank_offsets[512];
static const uint8_t frames[20] = {0, 0, 0, 0, 3, 0, 0, 0, 0, 9,
                                   0, 0, 0, 0, 5, 0, 0, 0, 0, 7};
static uint8_t mirror[512];

static fifa96_render_bank bank_a(void) {
  fifa96_render_bank b;
  b.base = bank_store;
  b.offsets = bank_offsets;
  b.count = 256;
  b.step = 3;
  return b;
}

static void test_camera_stage(void) {
  fifa96_render_pos src;
  fifa96_render_pos cam;
  src.x = 100;
  src.y = 50;
  src.z = -200;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.x == 100 && cam.y == 50 && cam.z == -200);
  src.x = FIFA96_RENDER_CAM_BOUND;
  src.z = -FIFA96_RENDER_CAM_BOUND;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.x == FIFA96_RENDER_CAM_BOUND && cam.z == -FIFA96_RENDER_CAM_BOUND);
  src.x = FIFA96_RENDER_CAM_BOUND + 1;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.x == FIFA96_RENDER_CAM_RESET && cam.y == 0 && cam.z == FIFA96_RENDER_CAM_RESET);
  src.x = 0;
  src.z = -FIFA96_RENDER_CAM_BOUND - 1;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.x == FIFA96_RENDER_CAM_RESET && cam.y == 0 && cam.z == FIFA96_RENDER_CAM_RESET);
  src.z = 0;
  src.y = -1;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.y == 0);
  src.y = FIFA96_RENDER_CAM_Y_MAX;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.y == FIFA96_RENDER_CAM_Y_MAX);
  src.y = FIFA96_RENDER_CAM_Y_MAX + 1;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.y == 0);
  src.x = INT32_MIN;
  assert(fifa96_render_camera_stage(&src, &cam) == FIFA96_OK);
  assert(cam.x == FIFA96_RENDER_CAM_RESET && cam.y == 0 && cam.z == FIFA96_RENDER_CAM_RESET);
  assert(fifa96_render_camera_stage(NULL, &cam) == R_INVALID);
  assert(fifa96_render_camera_stage(&src, NULL) == R_INVALID);
}

static void test_slot_stage(void) {
  fifa96_render_entity e;
  fifa96_render_slot s;
  e.pos.x = 11;
  e.pos.y = 22;
  e.pos.z = 33;
  e.heading = 0;
  e.anim_id = 0x2A;
  e.frame = 5;
  e.hidden = 0;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.pos.x == 11 && s.pos.y == 22 && s.pos.z == 33);
  assert(s.anim_id == 0x2A && s.frame == 5 && s.hidden == 0);
  assert(s.angle == 0);
  e.heading = 1 << 16;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.angle == 0x3FF << 6);
  e.heading = 0x401 << 16;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.angle == 0x3FF * 64);
  e.heading = 0x400 << 16;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.angle == 0);
  e.heading = 0x7FF << 16;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.angle == 64);
  e.heading = -1;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.angle == 0x40);
  e.heading = (int32_t)0xFFFF0000;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.angle == 64);
  e.hidden = 1;
  assert(fifa96_render_slot_stage(&e, &s) == FIFA96_OK);
  assert(s.pos.x == 11 && s.pos.y == FIFA96_RENDER_HIDDEN_Y && s.pos.z == 33);
  assert(s.hidden == 1);
  assert(fifa96_render_slot_stage(NULL, &s) == R_INVALID);
  assert(fifa96_render_slot_stage(&e, NULL) == R_INVALID);
}

static void test_bank_at(void) {
  fifa96_render_bank b = bank_a();
  assert(fifa96_render_bank_at(&b, 0) == bank_store + bank_offsets[0]);
  assert(fifa96_render_bank_at(&b, 3) == bank_store + bank_offsets[6]);
  assert(fifa96_render_bank_at(&b, -1) == NULL);
  b.count = 4;
  assert(fifa96_render_bank_at(&b, 3) == bank_store + bank_offsets[6]);
  assert(fifa96_render_bank_at(&b, 4) == NULL);
  assert(fifa96_render_bank_at(&b, 0x7FFFFFFF) == NULL);
  b.base = NULL;
  assert(fifa96_render_bank_at(&b, 0) == NULL);
  assert(fifa96_render_bank_at(NULL, 0) == NULL);
}

static void test_resolve_errors(void) {
  fifa96_render_bank banks[3];
  fifa96_render_frame out;
  banks[0] = bank_a();
  banks[1] = bank_a();
  banks[2] = bank_a();
  memset(&out, 0xAA, sizeof out);
  assert(fifa96_render_resolve(0x6F, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.sprite == NULL && out.overlay == NULL && out.mirrored == 0);
  assert(fifa96_render_resolve(0x7F, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.sprite == NULL);
  assert(fifa96_render_resolve(0x10, 0, 0, NULL, 0, banks, 3, NULL, NULL, mirror, &out) ==
         R_INVALID);
  assert(fifa96_render_resolve(0x10, 0, 0, frames, 0, NULL, 3, NULL, NULL, mirror, &out) ==
         R_INVALID);
  assert(fifa96_render_resolve(0x10, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, NULL) ==
         R_INVALID);
  assert(fifa96_render_resolve(0x10, 0, 0, frames, 3, banks, 3, NULL, NULL, mirror, &out) ==
         R_INVALID);
  assert(fifa96_render_resolve(0x10, -1, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         R_INVALID);
  assert(fifa96_render_resolve(0x10, 0xFF, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         R_INVALID);
}

static void test_resolve_default(void) {
  fifa96_render_bank banks[3];
  fifa96_render_bank fix;
  fifa96_render_frame out;
  banks[0] = bank_a();
  banks[1] = bank_a();
  banks[1].step = 5;
  banks[2] = bank_a();
  banks[2].step = 7;
  fix = bank_a();
  fix.step = 11;
  out.sprite = (const uint8_t *)1;
  assert(fifa96_render_resolve(0x10, 3, 2, frames, 1, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 5 * 2 + 7);
  assert(out.sprite == bank_store + bank_offsets[out.offset * 2]);
  assert(out.overlay == NULL && out.overlay_offset == 0 && out.mirrored == 0);
  assert(fifa96_render_resolve(0x10, 0x102, 0, frames, 0, banks, 3, NULL, NULL, mirror,
                               &out) == FIFA96_OK);
  assert(out.offset == 5);
  assert(fifa96_render_resolve(0x10, 0, 5, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 3 + 3 && out.mirrored == 1);
  assert(fifa96_render_resolve(0x10, 0, 7, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 1 + 3 && out.mirrored == 1);
  assert(fifa96_render_resolve(0x1A, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 && out.mirrored == 1);
  assert(fifa96_render_resolve(0x1A, 0, 5, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 3 + 3 && out.mirrored == 0);
  assert(fifa96_render_resolve(0x60, 0, 1, frames, 0, banks, 3, &fix, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 1 + 3);
  assert(out.overlay_offset == 11 * 1 + 3);
  assert(out.overlay == bank_store + bank_offsets[out.overlay_offset * 2]);
  assert(fifa96_render_resolve(0x61, 0, 1, frames, 0, banks, 3, NULL, &fix, mirror, &out) ==
         FIFA96_OK);
  assert(out.overlay_offset == 11 * 1 + 3);
  assert(fifa96_render_resolve(0x60, 0, 1, frames, 0, banks, 3, NULL, &fix, mirror, &out) ==
         FIFA96_OK);
  assert(out.overlay == NULL && out.overlay_offset == 0);
  assert(fifa96_render_resolve(0x80, 0, 1, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 1 + 3);
}

static void test_resolve_flat(void) {
  fifa96_render_bank banks[3];
  fifa96_render_frame out;
  uint8_t id;
  banks[0] = bank_a();
  banks[1] = bank_a();
  banks[2] = bank_a();
  banks[1].step = 5;
  for (id = 0; id < 8; id++) {
    int32_t dir = id;
    assert(fifa96_render_resolve(0x4A, 0, dir, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
           FIFA96_OK);
    if (dir == 0 || dir == 1 || dir == 2) {
      assert(out.offset == 3 * dir + 3 && out.mirrored == 0);
    } else if (dir == 3 || dir == 4) {
      assert(out.offset == 5 * (dir - 3) + 3 && out.mirrored == 0);
    } else if (dir == 5) {
      assert(out.offset == 5 * 0 + 3 && out.mirrored == 1);
    } else if (dir == 6) {
      assert(out.offset == 3 * 2 + 3 && out.mirrored == 1);
    } else {
      assert(out.offset == 3 * 1 + 3 && out.mirrored == 1);
    }
  }
  assert(fifa96_render_resolve(0x2F, 0, 3, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 && out.mirrored == 0);
  assert(fifa96_render_resolve(0x00, 0, 3, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 3 + 3 && out.mirrored == 0);
}

static void test_resolve_pair_classes(void) {
  fifa96_render_bank banks[3];
  fifa96_render_frame out;
  banks[0] = bank_a();
  banks[1] = bank_a();
  banks[2] = bank_a();
  banks[1].step = 5;
  banks[2].step = 7;
  mirror[3] = 9;
  assert(fifa96_render_resolve(0x34, 0, 4, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 5 * 0 + 3);
  assert(fifa96_render_resolve(0x36, 0, 2, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 5 * 2 + 3 && out.mirrored == 1);
  assert(fifa96_render_resolve(0x36, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 && out.mirrored == 1);
  banks[2].count = 4;
  assert(fifa96_render_resolve(0x35, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.overlay_offset == 9 - 1 && out.overlay == NULL);
  banks[2].count = 256;
  assert(fifa96_render_resolve(0x35, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.overlay == bank_store + bank_offsets[out.overlay_offset * 2]);
  mirror[3] = 0;
  assert(fifa96_render_resolve(0x35, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.overlay == NULL && out.overlay_offset == 7 * 0 + 3);
  mirror[3] = 9;
  banks[2].count = 256;
  assert(fifa96_render_resolve(0x39, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.overlay_offset == 9 - 1);
  assert(fifa96_render_resolve(0x3A, 0, 7, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 1 + 3 && out.mirrored == 1);
  assert(fifa96_render_resolve(0x3C, 0, 0, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 && out.mirrored == 0);
  assert(fifa96_render_resolve(0x3D, 0, 4, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 5 * 0 + 3 && out.mirrored == 0);
  assert(fifa96_render_resolve(0x3E, 0, 6, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 2 + 3 && out.mirrored == 1);
  assert(fifa96_render_resolve(0x3F, 0, 1, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 1 + 3 && out.overlay == NULL);
  assert(fifa96_render_resolve(0x40, 0, 1, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 1 + 3 && out.overlay_offset == 5 * 1 + 3);
  assert(fifa96_render_resolve(0x41, 0, 7, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 1 + 3 && out.mirrored == 1 && out.overlay == NULL);
  assert(fifa96_render_resolve(0x42, 0, 6, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 3 * 2 + 3 && out.mirrored == 1);
  assert(out.overlay_offset == 5 * 2 + 3);
  assert(fifa96_render_resolve(0x1B, 0, 4, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 5 * 0 + 3 && out.mirrored == 0);
  assert(fifa96_render_resolve(0x1B, 0, 7, frames, 0, banks, 3, NULL, NULL, mirror, &out) ==
         FIFA96_OK);
  assert(out.offset == 5 * 3 + 3 && out.mirrored == 0);
}

static void test_place(void) {
  int32_t x;
  int32_t y;
  assert(fifa96_render_place(10, 20, 5, 10, 20, 40, 100, 200, &x, &y) == FIFA96_OK);
  assert(x == 90 && y == 180);
  assert(fifa96_render_place(10, 20, 2, 3, -20, -40, 100, 200, &x, &y) == FIFA96_OK);
  assert(x == 100 + (-0x20000 * 8 >> 16));
  assert(y == 200 + (-0x20000 * 17 >> 16));
  assert(fifa96_render_place(10, 20, -2, 0, 0, 0, 100, 200, &x, &y) == FIFA96_OK);
  assert(x == 100 && y == 200);
  assert(fifa96_render_place(0, 20, 0, 0, 20, 40, 0, 0, &x, &y) == R_INVALID);
  assert(fifa96_render_place(10, 0, 0, 0, 20, 40, 0, 0, &x, &y) == R_INVALID);
  assert(fifa96_render_place(10, 20, 0, 0, 20, 40, 0, 0, NULL, &y) == R_INVALID);
  assert(fifa96_render_place(10, 20, 0, 0, 20, 40, 0, 0, &x, NULL) == R_INVALID);
}

static const fifa96_render_clip clip = {10, 10, 100, 100};

static void test_cover_inside(void) {
  fifa96_render_cover c;
  uint8_t visible = 0xAA;
  assert(fifa96_render_cover_rect(20, 20, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1);
  assert(c.dst_x == 20 && c.dst_y == 20 && c.dst_w == 32 && c.dst_h == 16);
  assert(c.src_x == 0x10000 && c.src_y == 0x10000);
  assert(c.src_dx == 0x20000 && c.src_dy == 0x20000);
  assert(c.mirrored_x == 0 && c.mirrored_y == 0);
}

static void test_cover_clip(void) {
  fifa96_render_cover c;
  uint8_t visible = 0;
  assert(fifa96_render_cover_rect(0, 20, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1);
  assert(c.dst_x == 10 && c.dst_w == 22);
  assert(c.src_x == 0x10000 + 10 * 0x20000);
  assert(fifa96_render_cover_rect(20, 5, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1);
  assert(c.dst_y == 10 && c.dst_h == 11);
  assert(c.src_y == 0x10000 + 5 * 0x20000);
  assert(fifa96_render_cover_rect(90, 20, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1 && c.dst_w == 10);
  assert(fifa96_render_cover_rect(20, 95, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1 && c.dst_h == 5);
  assert(fifa96_render_cover_rect(0, 5, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1 && c.dst_x == 10 && c.dst_y == 10 && c.dst_w == 22 && c.dst_h == 11);
}

static void test_cover_mirror(void) {
  fifa96_render_cover c;
  uint8_t visible = 0;
  assert(fifa96_render_cover_rect(20, 20, -32, -16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1);
  assert(c.dst_w == 32 && c.dst_h == 16);
  assert(c.mirrored_x == 1 && c.mirrored_y == 1);
  assert(c.src_dx == -0x20000 && c.src_dy == -0x20000);
  assert(c.src_x == (64 << 16) - 0x10000);
  assert(c.src_y == (32 << 16) - 0x10000);
}

static void test_cover_invisible(void) {
  fifa96_render_cover c;
  uint8_t visible = 0xAA;
  assert(fifa96_render_cover_rect(100, 20, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 0 && c.dst_w == 0 && c.dst_x == 0);
  visible = 0xAA;
  assert(fifa96_render_cover_rect(20, 100, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 0 && c.dst_h == 0);
  visible = 0xAA;
  assert(fifa96_render_cover_rect(-32, 20, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 0);
  visible = 0xAA;
  assert(fifa96_render_cover_rect(20, -16, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 0);
  visible = 0xAA;
  assert(fifa96_render_cover_rect(-21, 20, 32, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 1 && c.dst_x == 10 && c.dst_w == 1);
  assert(c.src_x == 0x10000 + 31 * 0x20000);
  visible = 0xAA;
  assert(fifa96_render_cover_rect(20, 20, 0, 16, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 0);
  visible = 0xAA;
  assert(fifa96_render_cover_rect(20, 20, 32, 0, 64, 32, &clip, &c, &visible) == FIFA96_OK);
  assert(visible == 0);
}

static void test_cover_errors(void) {
  fifa96_render_cover c;
  uint8_t visible;
  assert(fifa96_render_cover_rect(20, 20, 32, 16, 64, 32, NULL, &c, &visible) == R_INVALID);
  assert(fifa96_render_cover_rect(20, 20, 32, 16, 64, 32, &clip, NULL, &visible) == R_INVALID);
  assert(fifa96_render_cover_rect(20, 20, 32, 16, 64, 32, &clip, &c, NULL) == R_INVALID);
  assert(fifa96_render_cover_rect(20, 20, 32, 16, 0, 32, &clip, &c, &visible) == R_INVALID);
  assert(fifa96_render_cover_rect(20, 20, 32, 16, 64, 0, &clip, &c, &visible) == R_INVALID);
}

int main(void) {
  test_camera_stage();
  test_slot_stage();
  test_bank_at();
  test_resolve_errors();
  test_resolve_default();
  test_resolve_flat();
  test_resolve_pair_classes();
  test_place();
  test_cover_inside();
  test_cover_clip();
  test_cover_mirror();
  test_cover_invisible();
  test_cover_errors();
  puts("test_render: all assertions passed");
  return 0;
}
