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
 *          recip_x[0x200] = (320<<16)/0x200 = 40960
 *          recip_y[0x200] = (240<<16)/0x200 = 30720
 *          (fifa96_projection_reciprocal divides by max(i,10), so the index
 *          itself is the divisor at 0x200).
 *          clean  = centre + (0, 0) = (160<<16, 120<<16) = (160.0, 120.0)
 *   jitter py = 0x70 * 30720 = 3440640
 *          jitter = (160<<16, (120<<16) - 3440640) = (160.0, 67.5)
 *
 *   FU-85 §2 sprite scale (clean_y - jitter_y as the size reference):
 *          d = 3440640; (d + 0x1000) & 0xFFFF0000 = 0x340000 (52.5 -> 52 px);
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
  assert(mr.render.view_ratio == 21);   /* FU-97 static +0x4C default */
  assert(mr.render.entities[0].anim_turn == 1);   /* FU-84 selector default */
  assert(mr.render.entities[0].anim_timer == 0);
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

/* Composite classes resolve an overlay frame (FU-85 §2 out2) and the original
 * blits it in a second FUN_00057080 pass at the same scale. Fixture: anim id
 * 0x40 -> the overlay comes from banks[bank_index+1]; a 16x16 solid-0x22
 * overlay over the 32x32 solid-0x11 main frame gets its own destination size
 * 16*77428 >> 16 = 18 and pivot placement (18<<16)/16 = 73728,
 * (73728*8)>>16 = 9 -> (160-9, 120-9) = (151,111) 18x18. */
static void test_composite_overlay_is_blitted(void) {
  struct render_fixture f;
  fixture_init(&f);
  uint8_t data[BLOB_LEN + 16 + 16 * 16];
  memcpy(data, f.blob, BLOB_LEN);
  uint8_t *overlay = data + BLOB_LEN;
  memset(overlay, 0x00, 16);
  memset(overlay + 16, 0x22, 16 * 16);
  overlay[4] = 16;
  overlay[6] = 16;
  overlay[8] = 8;
  overlay[10] = 8;
  int32_t offsets0[2] = { 0, 0 };
  int32_t offsets1[2] = { (int32_t)BLOB_LEN, 0 };
  struct fifa96_render_bank banks[2];
  banks[0].base = data;
  banks[0].offsets = offsets0;
  banks[0].count = 2;
  banks[0].step = 1;
  banks[1].base = data;
  banks[1].offsets = offsets1;
  banks[1].count = 2;
  banks[1].step = 1;
  f.mr.render.sprite_data = data;
  f.mr.render.sprite_data_len = (uint32_t)sizeof data;
  f.mr.render.banks = banks;
  f.mr.render.bank_count = 2;
  f.mr.render.entities[0].stage.anim_id = 0x40;

  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[102 * 320 + 142] == 0x11);   /* main frame corner */
  assert(f.s->indexed[138 * 320 + 178] == 0x11);
  assert(f.s->indexed[111 * 320 + 151] == 0x22);   /* overlay rect */
  assert(f.s->indexed[128 * 320 + 168] == 0x22);
  assert(f.s->indexed[111 * 320 + 150] == 0x11);   /* around the overlay */
  assert(f.s->indexed[111 * 320 + 169] == 0x11);
  assert(f.s->indexed[110 * 320 + 151] == 0x11);
  assert(f.s->indexed[129 * 320 + 151] == 0x11);
  fifa96_surface_destroy(f.s);
}

/* A mirrored resolver frame passes a negative destination width through the
 * signed `fifa96_render_place`/`fifa96_render_cover_rect` path (the negative
 * scale branch). Fixture: anim id 0x36 always mirrors; the source's right half
 * (x 16..31) is 0x33, the left half 0x11. dst_w = -37 places the mirrored
 * rect at (141,102): output column 0 samples source x=31 (0x33) and the last
 * column samples source x=0 (0x11). */
static void test_mirrored_frame_uses_signed_size(void) {
  struct render_fixture f;
  fixture_init(&f);
  for (int y = 0; y < 32; y++)
    for (int x = 16; x < 32; x++) f.blob[16 + y * 32 + x] = 0x33;
  f.mr.render.entities[0].stage.anim_id = 0x36;

  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[102 * 320 + 141] == 0x33);
  assert(f.s->indexed[102 * 320 + 177] == 0x11);
  assert(f.s->indexed[138 * 320 + 141] == 0x33);
  assert(f.s->indexed[138 * 320 + 177] == 0x11);
  fifa96_surface_destroy(f.s);
}

/* The window's clip can outgrow the surface handed to render (the window was
 * sized from the engine surface at begin). The clip must clamp to the target:
 * the fixture's (142,102) rect is fully outside a 16x12 surface and nothing
 * past width*height may be written. */
static void test_clip_clamped_to_smaller_surface(void) {
  struct render_fixture f;
  fixture_init(&f);
  fifa96_surface_destroy(f.s);
  f.s = fifa96_surface_create(16, 12);
  assert(f.s != NULL);
  fifa96_surface_clear(f.s, 0);

  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  for (int i = 16 * 12; i < FIFA96_SURFACE_MAX_W * FIFA96_SURFACE_MAX_H; i++)
    assert(f.s->indexed[i] == 0);
  fifa96_surface_destroy(f.s);
}

/* ---- Task 11: scene staging, animation banks, depth/lateral gates --------
 *
 * Native chain (first-hand on /FIFA96.EXE):
 *  - FUN_00036C70 stages 23 slots from the FU-141 pool: team 0 records 0..10
 *    (base 0x1588A4), team 1 records 0..10 (base 0x1590D9), then slot 22 =
 *    the ball (0x15880C); position from rec+0x59/5D/61, hidden from rec+0x9A
 *    (staged y = -10000, 0x36D61), ball heading from 0x15885A;
 *  - the FU-84 row table at flat 0x10EF00 (111 rows x 9 B, first-hand
 *    read_memory) gives row+8 = the sprite bank the resolver consumes;
 *  - FUN_000589E0 computes the near-depth threshold `[0x54350] =
 *    max((0x14<<16)/(2*obj[5]), 0x78)`, or `[0x9088] = 0x78` when
 *    `obj[1] < [0x908C] = 0x140` (obj[5] = camera record +0x4C ratio angle,
 *    static loader default 0x15; obj[1] = camera y);
 *  - the 0x57CF6 walk culls a slot when its staged x (`[0x9A74] = 0x155AB0`,
 *    first-hand) is > 0x8E0;
 *  - FUN_00056CF4 (first-hand) makes the draw list a fixed 0x18-entry array
 *    with the 0-sentinel entry: the staged world is 23 slots, 24 - sentinel. */

#define SCENE_BLOB0_LEN (16u + 32u * 32u)
#define SCENE_BLOB_LEN (SCENE_BLOB0_LEN + 16u + 16u * 16u)

/* Pinned from the first verified run: the staged frame-0 32x32 solid-0x22
 * sprite covers the derived (142,102) 37x37 rect (the existing fixture's
 * derivation), and frame 1 the 16x16 solid-0x33 at (151,111); the geometry
 * asserts above are independent of the pins. */
#define SCENE_FRAME0_HASH 0x3ea0f266a78482a7ull
#define SCENE_FRAME1_HASH 0x6c75ec5c4595e961ull

struct scene_fixture {
  struct fifa96_match_run mr;
  struct fifa96_surface *s;
  uint8_t blob[SCENE_BLOB_LEN];
  int32_t off0[2];
  int32_t off1[4];
  struct fifa96_render_bank banks[2];
  uint8_t frames[16 * 5];
};

/* Two derived banks over one blob: bank 0 entry 0 is a 32x32 solid 0x22,
 * bank 1 entry 0 the same, entry 1 a 16x16 solid 0x33 (FU-84 row 1's frame
 * table stores sprite = frame index, so frame 0/1 select entry 0/1). Frames
 * are the Task 2 identity records {duration 0x50, aux 0, sprite = index}. */
static void scene_fixture_init(struct scene_fixture *f) {
  memset(f, 0, sizeof *f);
  fifa96_match_run_init(&f->mr);
  f->s = fifa96_surface_create(320, 240);
  assert(f->s != NULL);
  fifa96_surface_clear(f->s, 0);

  uint8_t *e0 = f->blob;
  uint8_t *e1 = f->blob + SCENE_BLOB0_LEN;
  memset(e0, 0x22, SCENE_BLOB0_LEN);
  memset(e0, 0x00, 16);
  e0[4] = 32;
  e0[6] = 32;
  e0[8] = 16;
  e0[10] = 16;
  memset(e1, 0x33, 16u + 16u * 16u);
  memset(e1, 0x00, 16);
  e1[4] = 16;
  e1[6] = 16;
  e1[8] = 8;
  e1[10] = 8;

  f->off0[0] = 0;
  f->off0[1] = 0;
  f->off1[0] = 0;
  f->off1[1] = 0;
  f->off1[2] = (int32_t)SCENE_BLOB0_LEN;
  f->off1[3] = 0;
  f->banks[0].base = f->blob;
  f->banks[0].offsets = f->off0;
  f->banks[0].count = 1;
  f->banks[0].step = 1;
  f->banks[1].base = f->blob;
  f->banks[1].offsets = f->off1;
  f->banks[1].count = 2;
  f->banks[1].step = 1;

  for (unsigned i = 0; i < 16; i++) {
    f->frames[i * 5 + 0] = 0x50;
    f->frames[i * 5 + 1] = 0;
    f->frames[i * 5 + 2] = 0;
    f->frames[i * 5 + 3] = 0;
    f->frames[i * 5 + 4] = (uint8_t)i;
  }

  struct fifa96_match_run_render *r = &f->mr.render;
  r->frames = f->frames;
  r->banks = f->banks;
  r->bank_count = 2;
  r->sprite_data = f->blob;
  r->sprite_data_len = (uint32_t)sizeof f->blob;
  r->enabled = 1;
  (void)fifa96_window_init(&r->window, 320, 240);
  (void)fifa96_window_define_full(&r->window, 320, 240);
  /* phase 0 is class 0 (the clock never ends a period, FU-143 §3); the
   * explicit length keeps direct frame-body drives clear of the state-init
   * zero-length period end. */
  f->mr.state.period_length = 90;
}

/* One 100 Hz frame-body tick needs ~3.33 pace ticks to grant a frame. */
static void drive_granted(struct fifa96_match_run *mr, int grants) {
  int got = 0;
  for (int i = 0; i < grants * 110 + 10 && got < grants; i++) {
    int rc = fifa96_match_run_frame(mr);
    assert(rc >= 0);
    if (rc == 1) got++;
  }
  assert(got == grants);
}

static uint32_t drawn_count(const struct fifa96_surface *s, uint8_t background) {
  uint32_t drawn = 0;
  for (int i = 0; i < s->width * s->height; i++)
    if (s->indexed[i] != background) drawn++;
  return drawn;
}

/* FUN_00036C70/FUN_00056CF4: the pool stages 23 render slots (11 + 11 + ball)
 * with positions, hidden from +0x9A and the FU-84 row+8 bank index derived
 * from the entity's animation id. */
static void test_scene_stages_pool_entities(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_x = 0;
  f.mr.entities.team[0].records[0].pos_y = 0;
  f.mr.entities.team[0].records[0].pos_z = 0x200;
  f.mr.entities.team[0].records[3].pos_x = 0x30;
  f.mr.entities.team[1].records[0].pos_x = 0x40;
  f.mr.entities.team[1].records[0].pos_y = 0x10;
  f.mr.entities.team[1].records[0].pos_z = 0x280;
  f.mr.entities.team[1].records[0].skip_9a = 1;
  f.mr.entities.ball.x = 0x20;
  f.mr.entities.ball.y = 0x30;
  f.mr.entities.ball.z = 0x400;
  f.mr.entities.ball.heading = (int16_t)0x123;
  f.mr.render.entities[0].stage.anim_id = 1;   /* FU-84 row 1 (bank 1) */

  assert(f.mr.render.entity_count == 0);
  drive_granted(&f.mr, 1);
  assert(f.mr.render.entity_count == FIFA96_MATCH_RUN_RENDER_SLOTS);
  assert(FIFA96_MATCH_RUN_RENDER_SLOTS == 23);
  assert(f.mr.render.entities[0].stage.pos.x == 0);
  assert(f.mr.render.entities[0].stage.pos.y == 0);
  assert(f.mr.render.entities[0].stage.pos.z == 0x200);
  assert(f.mr.render.entities[3].stage.pos.x == 0x30);
  assert(f.mr.render.entities[11].stage.pos.x == 0x40);
  assert(f.mr.render.entities[11].stage.pos.y == 0x10);
  assert(f.mr.render.entities[11].stage.pos.z == 0x280);
  assert(f.mr.render.entities[11].stage.hidden == 1);
  assert(f.mr.render.entities[22].stage.pos.x == 0x20);
  assert(f.mr.render.entities[22].stage.pos.y == 0x30);
  assert(f.mr.render.entities[22].stage.pos.z == 0x400);
  assert(f.mr.render.entities[22].stage.heading == ((int32_t)0x123 << 16));
  /* The 0x15885A heading is a signed word: the staging sign-extends the high
   * half so fifa96_render_slot_stage's `>> 16` recovers it. */
  f.mr.entities.ball.heading = (int16_t)-2;
  drive_granted(&f.mr, 1);
  assert(f.mr.render.entities[22].stage.heading == -131072);   /* 0xFFFE << 16 */
  /* FU-84 §3.1 row 1 = `01 0b 07 01 06 e2 00 00 01`: +8 sprite bank 1. */
  assert(f.mr.render.entities[0].bank_index == 1);
  fifa96_surface_destroy(f.s);
}

/* The fixture entity's frame index advances on the FU-84 `FUN_0008E008`
 * accumulator: duration 0x50, delta 2 -> delta<<4 = 0x20 per granted frame,
 * so the fourth grant advances frame 0 -> 1. The canvas follows through the
 * row-1 bank (frame 0 = 32x32 0x22 at (142,102); frame 1 = 16x16 0x33 at
 * (151,111), the existing fixture's scale derivation). */
static void test_entity_animates_over_frames(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_z = 0x200;
  f.mr.render.entities[0].stage.anim_id = 1;

  drive_granted(&f.mr, 1);
  assert(f.mr.render.entities[0].bank_index == 1);
  assert(f.mr.render.entities[0].stage.frame == 0);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[102 * 320 + 142] == 0x22);
  assert(f.s->indexed[138 * 320 + 178] == 0x22);
  assert(f.s->indexed[120 * 320 + 160] == 0x22);
  uint64_t frame0 = fifa96_surface_hash(f.s);
  printf("test_engine_match_render scene frame0 hash = 0x%016llx\n",
         (unsigned long long)frame0);

  drive_granted(&f.mr, 3);   /* grants 2..4: 0x20,0x40 accumulate, then 0x60 */
  assert(f.mr.render.entities[0].stage.frame == 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[111 * 320 + 151] == 0x33);
  assert(f.s->indexed[128 * 320 + 168] == 0x33);
  assert(f.s->indexed[102 * 320 + 142] == 0x00);   /* frame-0 rect gone */
  assert(f.s->indexed[138 * 320 + 178] == 0x00);
  uint64_t frame1 = fifa96_surface_hash(f.s);
  printf("test_engine_match_render scene frame1 hash = 0x%016llx\n",
         (unsigned long long)frame1);
  assert(frame1 != frame0);
  assert(frame0 == SCENE_FRAME0_HASH);
  assert(frame1 == SCENE_FRAME1_HASH);
  fifa96_surface_destroy(f.s);
}

/* FUN_000589E0/FUN_00036C70 gates: with the camera at y = 0x140 and the
 * static ratio 0x15 the computed threshold is `(0x14<<16)/(2*0x15) = 0x79E7`,
 * so a key (jitter z) of 0x1000 is culled; at ratio 0x800 the same depth
 * computes 0x140 and draws. At y = 0x13F the `[0x908C]` limit is not reached,
 * so `[0x9088] = 0x78` is the threshold and 0x1000 draws. */
static void test_near_depth_threshold_gate(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_y = 0x140;
  f.mr.entities.team[0].records[0].pos_z = 0x1000;

  assert(fifa96_camera_init(&f.mr.render.camera, 0, 0x140, 0) == 0);
  drive_granted(&f.mr, 1);
  assert(f.mr.render.entities[0].stage.pos.z == 0x1000);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(drawn_count(f.s, 0) == 0);   /* 0x79E7 > 0x1000: culled */

  f.mr.render.view_ratio = 0x800;
  drive_granted(&f.mr, 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(drawn_count(f.s, 0) > 0);    /* 0x140 <= 0x1000: drawn */

  f.mr.render.view_ratio = 0x15;
  assert(fifa96_camera_init(&f.mr.render.camera, 0, 0x13F, 0) == 0);
  f.mr.entities.team[0].records[0].pos_y = 0x13F;
  drive_granted(&f.mr, 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(drawn_count(f.s, 0) > 0);    /* y < 0x140: [0x9088] = 0x78 */
  fifa96_surface_destroy(f.s);
}

/* The 0x57CF6/FUN_00056FA4 lateral gate culls a staged x > 0x8E0 (signed):
 * the same relative position (camera tracks the record) draws at 0x8E0 and
 * is culled at 0x8E1. */
static void test_lateral_cull_at_8e0(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_x = 0x8E0;
  f.mr.entities.team[0].records[0].pos_z = 0x200;
  assert(fifa96_camera_init(&f.mr.render.camera, 0x8E0, 0, 0) == 0);
  drive_granted(&f.mr, 1);
  assert(f.mr.render.entities[0].stage.pos.x == 0x8E0);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(drawn_count(f.s, 0) > 0);    /* lateral == 0x8E0: visible */

  f.mr.entities.team[0].records[0].pos_x = 0x8E1;
  assert(fifa96_camera_init(&f.mr.render.camera, 0x8E1, 0, 0) == 0);
  drive_granted(&f.mr, 1);
  assert(f.mr.render.entities[0].stage.pos.x == 0x8E1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(drawn_count(f.s, 0) == 0);   /* lateral 0x8E1 > 0x8E0: culled */
  fifa96_surface_destroy(f.s);
}

/* FU-85 §5 `0x14720`/`fifa96_sprite_span`: the staged remap is the identity
 * translation with index 0 as the transparent key -- source pixel 0 writes
 * nothing, 0x80 passes through unchanged. */
static void test_remap_identity_and_color_key(void) {
  struct render_fixture f;
  fixture_init(&f);
  f.blob[16 + 0 * 32 + 0] = 0x00;
  f.blob[16 + 0 * 32 + 1] = 0x80;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[102 * 320 + 142] == 0x00);   /* source x=0: transparent */
  assert(f.s->indexed[102 * 320 + 143] == 0x80);   /* identity pass-through */
  assert(f.s->indexed[102 * 320 + 144] == 0x11);
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
  /* The MATCH dispatch's granted frames ran the FU-85 §4 scene staging over
   * the FU-141 pool (the live/tape path); the pool's zero positions leave the
   * canvas at the background, but the 23 slots are staged. */
  assert(mr_on.render.entity_count == FIFA96_MATCH_RUN_RENDER_SLOTS);
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
  test_composite_overlay_is_blitted();
  test_mirrored_frame_uses_signed_size();
  test_clip_clamped_to_smaller_surface();
  test_scene_stages_pool_entities();
  test_entity_animates_over_frames();
  test_near_depth_threshold_gate();
  test_lateral_cull_at_8e0();
  test_remap_identity_and_color_key();
  test_engine_match_step_renders();
  puts("test_engine_match_render OK");
  return 0;
}
