/* tests/test_engine_match_render.c — Task 15: match presentation.
 *
 * FU chain under test (docs/ghidra/):
 *  - FU-71 camera follow state (fifa96_camera), FU-88 view matrix + reciprocal
 *    perspective divide (fifa96_projection), FU-89 scene assembly/render list
 *    (fifa96_scene), FU-85 match renderer resolver + pivot placement + clipped
 *    scaled copy (fifa96_render), FU-84 sprite-frame records (flat 0x10E200
 *    family; the resolver reads the frame table's +4 sprite byte through the
 *    bank index at row +8), FU-92 render window/clip, FU-93 window->zoom.
 *
 * Fixture geometry (surface 320x240, camera (0,0,0), yaw = pitch = 0, so the
 * FU-88 view matrix is the identity; positions are plain integers, FU-88 §5):
 *
 *   entity 0 world position (0, 0, 0x200). FU-89 §7 adds the per-slot jitter
 *   `i%6 + 0x70` to y before rotation, so slot 0's jittered relative point is
 *   (0, 0x70, 0x200).
 *
 *   clean  z = 0x200: z < 0x400 so no z normalisation shift; index 0x200.
 *          recip_x[0x200] = (320<<16)/0x201 = 40880
 *          recip_y[0x200] = (240<<16)/0x201 = 30660
 *          clean  = centre + (0, 0) = (160<<16, 120<<16) = (160.0, 120.0)
 *   jitter py = 0x70 * 30660 = 3433920
 *          jitter = (160<<16, (120<<16) - 3433920) = (160.0, 67.6)
 *
 *   FU-85 §2 sprite scale (clean_y - jitter_y as the size reference):
 *          d = 3433920; (d + 0x1000) & 0xFFFF0000 = 0x340000 (52 px);
 *          scale = 0x340000 * 0x5D1 / 0x10000 = 77428.
 *   sprite 32x32 -> dest = (32 * 77428) >> 16 = 37 px (both axes).
 *   pivot (16,16), placed at the clean point (160,120):
 *          sx = (37<<16)/32 = 75776; x' = 160 - ((75776*16)>>16) = 142;
 *          y' = 120 - 18 = 102. Rect = (142,102) 37x37, fully inside the
 *          320x240 clip (no cover adjustment).
 *
 * So the solid 0x11 sprite covers x 142..178, y 102..138, and the projected
 * pivot pixel (160,120) is inside it; a re-render must repeat the same canvas.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_err.h"

/* FNV-1a over the 320x240 indexed canvas + 768-byte palette (fifa96_surface_hash):
 * pinned from the first verified run (all-zero palette, background 0, the
 * derived (142,102) 37x37 solid-0x11 rect; the geometry assertions below pin
 * the canvas shape independently). */
#define FIXTURE_HASH 0xfec6b5f3bb9aeee0ull

#define BLOB_LEN (16u + 32u * 32u)

/* A synthetic runtime sprite frame: the FU-85 §2 header read by
 * FUN_00057080/FUN_000A2E24 (`[+2]>>16` dword aliases = the u16 fields at
 * +4/+6/+8/+10 parsed by fifa96_sprite_frame_parse): w=32, h=32, pivot=(16,16),
 * pixels at +16 (all 0x11 so the covered rect is non-background). */
static void blob_init(uint8_t *blob) {
  memset(blob, 0x11, BLOB_LEN);
  memset(blob, 0x00, 16);
  blob[4] = 32;
  blob[6] = 32;
  blob[8] = 16;
  blob[10] = 16;
}

/* One frame record (FU-84 §4): duration 0x40, aux 0, sprite byte 0. */
static void setup_render(struct fifa96_match_run_render *r, uint8_t *blob, int32_t *offsets,
                         struct fifa96_render_bank *bank, uint8_t *frames) {
  blob_init(blob);
  offsets[0] = 0;
  offsets[1] = 0;
  bank->base = blob;
  bank->offsets = offsets;
  bank->count = 2;
  bank->step = 1;
  frames[0] = 0x40;
  frames[1] = 0;
  frames[2] = 0;
  frames[3] = 0;
  frames[4] = 0;

  (void)fifa96_camera_init(&r->camera, 0, 0, 0);
  r->yaw = 0;
  r->pitch = 0;
  (void)fifa96_window_init(&r->window, 320, 240);
  (void)fifa96_window_define_full(&r->window, 320, 240);
  r->frames = frames;
  r->banks = bank;
  r->bank_count = 1;
  r->sprite_data = blob;
  r->sprite_data_len = BLOB_LEN;
  r->entities[0].stage.pos.x = 0;
  r->entities[0].stage.pos.y = 0;
  r->entities[0].stage.pos.z = 0x200;
  r->entities[0].stage.heading = 0;
  r->entities[0].stage.anim_id = 0;
  r->entities[0].stage.frame = 0;
  r->entities[0].stage.hidden = 0;
  r->entities[0].bank_index = 0;
  r->entity_count = 1;
  r->background = 0;
  r->enabled = 1;
}

struct render_fixture {
  struct fifa96_match_run mr;
  struct fifa96_surface *s;
  uint8_t blob[BLOB_LEN];
  int32_t offsets[2];
  struct fifa96_render_bank bank;
  uint8_t frames[5];
};

static void fixture_init(struct render_fixture *f) {
  memset(f, 0, sizeof *f);
  fifa96_match_run_init(&f->mr);
  f->s = fifa96_surface_create(320, 240);
  assert(f->s != NULL);
  fifa96_surface_clear(f->s, 0);
  setup_render(&f->mr.render, f->blob, f->offsets, &f->bank, f->frames);
}

static void test_init_resets_render_state(void) {
  struct fifa96_match_run mr;
  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);

  assert(mr.render.enabled == 0);
  assert(mr.render.camera.pos_x == 0 && mr.render.camera.pos_y == 0);
  assert(mr.render.camera.pos_z == 0);
  assert(mr.render.camera.vel_x == 0 && mr.render.camera.vel_z == 0);
  assert(mr.render.camera.speed == 0 && mr.render.camera.paused == 0);
  assert(mr.render.yaw == 0 && mr.render.pitch == 0);
  assert(mr.render.view_class == 0 && mr.render.input_bit2 == 0);
  assert(mr.render.window.box.w == 0 && mr.render.window.box.h == 0);
  assert(mr.render.display.suspend == 0 && mr.render.display.state == 0);
  assert(mr.render.display.selector == 0);
  assert(mr.render.display.duration == FIFA96_MATCH_DISPLAY_DURATION);
  assert(mr.render.window_scale_x == 0 && mr.render.window_scale_y == 0);
  assert(mr.render.window_zoomed == 0);
  assert(mr.render.entity_count == 0);
  assert(mr.render.frames == NULL && mr.render.banks == NULL);
  assert(mr.render.bank_count == 0 && mr.render.fixed60 == NULL);
  assert(mr.render.fixed61 == NULL && mr.render.mirror == NULL);
  assert(mr.render.sprite_data == NULL && mr.render.sprite_data_len == 0);
  assert(mr.render.background == 0);
  /* Indexed remap: identity translation, index 0 = derived colour key. */
  assert(mr.render.remap[0] == 0xFF);
  assert(mr.render.remap[1] == 1);
  assert(mr.render.remap[0x80] == 0x80);
  assert(mr.render.remap[255] == 255);
}

static void test_null_and_disabled(void) {
  struct render_fixture f;
  fixture_init(&f);

  assert(fifa96_match_run_render(NULL, f.s) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_render(&f.mr, NULL) == -FIFA96_ERR_INVALID);

  /* disabled (the init/begin default) is a strict no-op so the Task 12-14
   * engine fixtures keep their surface untouched. */
  f.mr.render.enabled = 0;
  fifa96_surface_clear(f.s, 0x5A);
  uint64_t before = fifa96_surface_hash(f.s);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(fifa96_surface_hash(f.s) == before);

  /* enabled without assets is a state error and writes nothing. */
  f.mr.render.enabled = 1;
  f.mr.render.banks = NULL;
  assert(fifa96_match_run_render(&f.mr, f.s) == -FIFA96_ERR_STATE);
  assert(fifa96_surface_hash(f.s) == before);

  fifa96_surface_destroy(f.s);
}

static void test_fixture_pins_hash_and_geometry(void) {
  struct render_fixture f;
  fixture_init(&f);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);

  /* The derived destination rect (142,102)-(178,138) and its surroundings. */
  assert(f.s->indexed[102 * 320 + 142] == 0x11);
  assert(f.s->indexed[138 * 320 + 178] == 0x11);
  assert(f.s->indexed[120 * 320 + 160] == 0x11);   /* projected pivot pixel */
  assert(f.s->indexed[102 * 320 + 141] == 0x00);
  assert(f.s->indexed[102 * 320 + 179] == 0x00);
  assert(f.s->indexed[101 * 320 + 142] == 0x00);
  assert(f.s->indexed[139 * 320 + 142] == 0x00);
  assert(f.s->indexed[0] == 0x00);

  /* The solid sprite covers exactly the 37x37 rect and nothing else. */
  for (int y = 0; y < 240; y++) {
    for (int x = 0; x < 320; x++) {
      int inside = x >= 142 && x <= 178 && y >= 102 && y <= 138;
      assert(f.s->indexed[y * 320 + x] == (inside ? 0x11 : 0x00));
    }
  }

  uint64_t hash = fifa96_surface_hash(f.s);
  printf("test_engine_match_render fixture hash = 0x%016llx\n", (unsigned long long)hash);
  assert(hash == FIXTURE_HASH);

  /* Idempotent: rendering the same fixture again is hash-stable. */
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(fifa96_surface_hash(f.s) == hash);
  fifa96_surface_destroy(f.s);

  /* A fresh run/surface with the same fixture pins the same canvas. */
  struct render_fixture g;
  fixture_init(&g);
  assert(fifa96_match_run_render(&g.mr, g.s) == 0);
  uint64_t again = fifa96_surface_hash(g.s);
  printf("test_engine_match_render second fixture hash = 0x%016llx\n", (unsigned long long)again);
  assert(again == hash);
  fifa96_surface_destroy(g.s);
}

/* A zeroed window falls back to the full surface: the same centre and clip as
 * the explicit full-surface window, so the canvas is identical. */
static void test_zero_window_uses_surface(void) {
  struct render_fixture f;
  fixture_init(&f);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  uint64_t full = fifa96_surface_hash(f.s);

  (void)fifa96_window_init(&f.mr.render.window, 320, 240);   /* box zeroed */
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(fifa96_surface_hash(f.s) == full);
  fifa96_surface_destroy(f.s);
}

struct engine_fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
};

static struct engine_fixture make_engine_fixture(void) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per engine step */
  struct engine_fixture f;
  f.plat = fifa96_platform_null_create(&pcfg);
  assert(f.plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  f.engine = fifa96_engine_create(&ecfg, f.plat);
  assert(f.engine != NULL);
  assert(fifa96_engine_boot(f.engine) == 0);
  return f;
}

static void drop_engine_fixture(struct engine_fixture f) {
  fifa96_engine_destroy(f.engine);
  fifa96_platform_destroy(f.plat);
}

/* MATCH mode renders once per presented frame. The disabled twin must present
 * the untouched boot surface (the Task 12-14 contract); the enabled twin must
 * present a different canvas after its first step. */
static void test_engine_match_step_renders(void) {
  struct engine_fixture off = make_engine_fixture();
  struct fifa96_match_run mr_off;
  fifa96_match_run_init(&mr_off);
  assert(fifa96_match_run_begin(&mr_off, off.engine, 0) == 0);
  assert(mr_off.render.enabled == 0);   /* default: Tasks 12-14 fixtures safe */
  for (int i = 0; i < 4; i++) assert(fifa96_engine_step(off.engine) == 0);
  struct fifa96_platform_null_stats stats_off;
  fifa96_platform_null_stats(off.plat, &stats_off);
  assert(stats_off.presents == 4);
  assert(fifa96_match_run_end(&mr_off) == 0);
  drop_engine_fixture(off);

  struct engine_fixture on = make_engine_fixture();
  struct fifa96_match_run mr_on;
  fifa96_match_run_init(&mr_on);
  assert(fifa96_match_run_begin(&mr_on, on.engine, 0) == 0);
  uint8_t blob[BLOB_LEN];
  int32_t offsets[2];
  struct fifa96_render_bank bank;
  uint8_t frames[5];
  setup_render(&mr_on.render, blob, offsets, &bank, frames);   /* begin reset it */
  for (int i = 0; i < 4; i++) assert(fifa96_engine_step(on.engine) == 0);
  struct fifa96_platform_null_stats stats_on;
  fifa96_platform_null_stats(on.plat, &stats_on);
  assert(stats_on.presents == 4);
  assert(stats_on.present_hash != stats_off.present_hash);
  assert(fifa96_match_run_end(&mr_on) == 0);
  drop_engine_fixture(on);
}

int main(void) {
  test_init_resets_render_state();
  test_null_and_disabled();
  test_fixture_pins_hash_and_geometry();
  test_zero_window_uses_surface();
  test_engine_match_step_renders();
  puts("test_engine_match_render OK");
  return 0;
}
