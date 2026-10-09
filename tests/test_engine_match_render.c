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
#include "fifa96_loader/fifa96_font.h"

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
  /* FU-96/FU-97's "21" is the +0x4D byte; the +0x4C dword is `00 15 00 00`
   * (first-hand 0x107554/0x1075C4). */
  assert(mr.render.view_ratio == 0x1500);
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
  /* OL-T11-7 HUD staging: no assets, no staged names. */
  assert(mr.render.hud_font[0].data == NULL && mr.render.hud_font[1].data == NULL);
  assert(mr.render.hud_font_ready[0] == 0 && mr.render.hud_font_ready[1] == 0);
  assert(mr.render.hud_bar.pixels == NULL && mr.render.hud_bar_ready == 0);
  assert(mr.render.hud_bar_height == 0);
  assert(mr.render.hud_name[0][0] == '\0' && mr.render.hud_name[1][0] == '\0');
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
 * from the entity's animation id (OL-80: the staged `anim_id` is the pool
 * record's `byte[[rec+0x28]]` stand-in, so it is set on the pool, not the
 * render slot). */
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
  f.mr.entities.team[0].records[0].anim_id = 1;   /* FU-84 row 1 (bank 1) */

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
 * (151,111), the existing fixture's scale derivation). The frame is the pool
 * record's native +0x3D byte (OL-80): the staging seeds from the pool and
 * writes the advanced index back. */
static void test_entity_animates_over_frames(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_z = 0x200;
  f.mr.entities.team[0].records[0].anim_id = 1;
  assert(f.mr.entities.team[0].records[0].frame == 0);

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
  assert(f.mr.entities.team[0].records[0].frame == 1);   /* +0x3D live write */
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

/* OL-80 live-animation fixture: the pool record's `anim_id` (native
 * byte[[rec+0x28]], staged at 0x36D44) and `frame` (native byte[rec+0x3D],
 * 0x36D4F) drive the FU-84 row+8 bank selection live, and a pool-side frame
 * change is what the staging consumes. Row 0x28's +8 byte is 0x4C (first-hand
 * table at flat 0x10EF00), so switching the pool `anim_id` 1 -> 0x28 moves the
 * staged bank index 1 -> 0x4C on the next granted frame; a pool `frame` of 1
 * is staged as-is (the accumulator only advances past the 0x50 duration after
 * two more 0x20 grants). */
static void test_live_anim_inputs_drive_bank_row(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_z = 0x200;
  f.mr.entities.team[0].records[0].anim_id = 1;
  drive_granted(&f.mr, 1);
  assert(f.mr.render.entities[0].stage.anim_id == 1);
  assert(f.mr.render.entities[0].bank_index == 1);
  assert(f.mr.render.entities[0].stage.frame == 0);

  f.mr.entities.team[0].records[0].anim_id = 0x28;
  f.mr.entities.team[0].records[0].frame = 1;
  drive_granted(&f.mr, 1);
  assert(f.mr.render.entities[0].stage.anim_id == 0x28);
  assert(f.mr.render.entities[0].bank_index == 0x4C);   /* row 0x28 +8 */
  assert(f.mr.render.entities[0].stage.frame == 1);
  assert(f.mr.entities.team[0].records[0].frame == 1);
  fifa96_surface_destroy(f.s);
}

/* OL-80 staging round-trip: the pool record's `anim_id` is staged into
 * `mr->record` on every dispatch and written back; a row-00 dispatch never
 * touches it, so the pool value must survive. This is the input half the
 * constant-kind arm assertions cannot discriminate (deleting
 * `r->anim_id = e->anim_id` clobbers the pool value to the zeroed staging
 * record); the handler tests pin the write-back half. The staged bank row
 * follows the survived id. */
static void test_anim_id_staging_round_trip(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_z = 0x200;
  f.mr.entities.team[0].records[0].anim_id = 0x28;
  drive_granted(&f.mr, 1);
  assert(f.mr.entities.team[0].records[0].anim_id == 0x28);
  assert(f.mr.render.entities[0].stage.anim_id == 0x28);
  assert(f.mr.render.entities[0].bank_index == 0x4C);   /* row 0x28 +8 */
  fifa96_surface_destroy(f.s);
}

/* FUN_000589E0/FUN_00036C70 gates (the ratio is set explicitly per phase: the
 * reset default 0x1500 computes 121 and is not exercised here):
 *  - camera y = 0x140, ratio 0x15 -> threshold `(0x14<<16)/(2*0x15) = 0x79E7`,
 *    so a key (jitter z) of 0x1000 is culled;
 *  - ratio 0x800 -> the same depth computes 0x140 and draws;
 *  - camera y = 0x13F -> the `[0x908C]` limit is not reached, so
 *    `[0x9088] = 0x78` is the threshold and 0x1000 draws. */
static void test_near_depth_threshold_gate(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  f.mr.entities.team[0].records[0].pos_y = 0x140;
  f.mr.entities.team[0].records[0].pos_z = 0x1000;

  f.mr.render.view_ratio = 0x15;
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

/* FU-96 leg 5 / FUN_00079F3C (M2 interactive T2): the per-record camera place
 * moves a non-controlled record inside 0x180 of the camera out to the ring.
 * Fixture: camera (0,0,0); the controlled side (0) record 0 sits at
 * (0, -0x60) and is NOT placed (the native side gate), so it stays behind the
 * camera and never draws; the non-controlled side (1) record 0 sits at
 * (8, 0x40) (fast length 0x42) at depth 0x40 < the 0x78 near threshold, so it
 * is culled before the place; the place snaps its target to (0x2F, 0x17D) and
 * the kickoff commit lands it, after which its sprite covers (199,120). The
 * far record 1 (0, 0x400) is beyond the ring and keeps drawing at the centre.
 * The kickoff instant frames the positive-depth side in the native too
 * (`fifa96_projection_screen` requires z >= NEAR), so the fixture asserts the
 * framed side plus the native non-placement of the controlled side — no fake
 * both-sides framing. */
static void test_camera_place_moves_near_record_into_frame(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  struct fifa96_match_entities *pool = &f.mr.entities;
  pool->team[0].records[0].target_z = -0x60;
  pool->team[1].records[0].target_x = 8;
  pool->team[1].records[0].target_z = 0x40;
  pool->team[1].records[1].target_z = 0x400;

  /* Pre-place: the near record is below the near gate (0x78) and culled. The
   * fixture commits with `FUN_00079B6C`'s commit block (the selector's row
   * 0x26 needs the full retail bank set, not this 2-bank fixture). */
  assert(fifa96_match_entities_place(&pool->team[0].records[0]) == FIFA96_OK);
  assert(fifa96_match_entities_place(&pool->team[1].records[0]) == FIFA96_OK);
  assert(fifa96_match_entities_place(&pool->team[1].records[1]) == FIFA96_OK);
  assert(pool->team[1].records[0].pos_x == 8);
  assert(pool->team[1].records[0].pos_z == 0x40);
  drive_granted(&f.mr, 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[120 * 320 + 199] == 0x00);   /* near record culled */
  assert(f.s->indexed[120 * 320 + 160] == 0x22);   /* far record draws */
  assert(f.s->indexed[120 * 320 + 80] == 0x00);    /* controlled record: behind */

  /* Place + commit: the near record lands on the 0x180 ring and draws. */
  assert(fifa96_match_entities_camera_place(pool, 0, 1, 0, 0) == FIFA96_OK);
  assert(pool->team[1].records[0].target_x == 0x2F);
  assert(pool->team[1].records[0].target_z == 0x17D);
  assert(pool->team[0].records[0].target_z == -0x60);   /* controlled untouched */
  assert(fifa96_match_entities_place(&pool->team[1].records[0]) == FIFA96_OK);
  assert(pool->team[1].records[0].pos_z == 0x17D);
  drive_granted(&f.mr, 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[120 * 320 + 199] == 0x22);   /* moved into frame */
  assert(f.s->indexed[120 * 320 + 160] == 0x22);   /* far record still draws */
  fifa96_surface_destroy(f.s);
}

/* FU-89 §11 / OL-T11-8 (M2 visible-match Task 1): a formation-seeded pool
 * draws. The camera is at the origin, so the near-depth gate (threshold 0x78
 * against the jittered z) admits the positive-depth team-1 records and culls
 * the negative-depth team-0 ones. The first render with zero targets is the
 * background (the pre-T1 state); after `fifa96_match_entities_seed_formation`
 * (+2 own pair for the controlled side 0, +0 opp pair negated for side 1) and
 * the `FUN_00079B6C` commit, team 1's record 0 at (0, 0, +0x18C) projects to
 * the window centre and its 32x32 frame-0 sprite covers the centre pixel. */
static void test_formation_seeded_entities_draw(void) {
  struct scene_fixture f;
  scene_fixture_init(&f);
  fifa96_scene_formation formation;
  memset(&formation, 0, sizeof formation);
  formation.loaded = 1;
  for (unsigned i = 0; i < FIFA96_SCENE_FORMATION_RECORDS; i++) {
    int8_t depth = (int8_t)-(12 + (int)i);
    formation.bytes[i * 4 + 0] = 0;
    formation.bytes[i * 4 + 1] = (uint8_t)depth;
    formation.bytes[i * 4 + 2] = 0;
    formation.bytes[i * 4 + 3] = (uint8_t)depth;
  }
  drive_granted(&f.mr, 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(drawn_count(f.s, 0) == 0);   /* zero targets: nothing passes the gate */

  assert(fifa96_match_entities_seed_formation(&f.mr.entities, &formation, 0) == FIFA96_OK);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      assert(fifa96_match_entities_place(&f.mr.entities.team[t].records[i]) == FIFA96_OK);
  /* team 1 record 0: opp pair -12 -> z = +396 (side 1 negates) */
  assert(f.mr.entities.team[1].records[0].pos_z == 396);
  assert(f.mr.entities.team[0].records[0].pos_z == -396);
  drive_granted(&f.mr, 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(drawn_count(f.s, 0) > 0);    /* the positive-depth records now draw */
  assert(f.s->indexed[120 * 320 + 160] == 0x22);   /* record 0's frame-0 pixel */
  assert(f.s->indexed[90 * 320 + 160] == 0x00);    /* above the sprite rect */
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

/* ------------------------------------------------------------------------ *
 * OL-T11-6 (M2 playable-match Task 1): the derived native match palette.
 *
 * Synthetic PALsys.fsh-shaped bank (FU-91 §2 / FU-144): SHPI, 3 frames; frame
 * 2 is 1x1 with its type-0x22 256-entry chunk at second_offset 17 (16-byte
 * frame header + the 1 pixel), mirroring the retail PALsys.fsh frame 2 layout.
 * ------------------------------------------------------------------------ */
#define PAL_BANK_FRAME2 72u
#define PAL_BANK_CHUNK (PAL_BANK_FRAME2 + 17u)
#define PAL_BANK_RGB6 (PAL_BANK_CHUNK + 16u)
#define PAL_BANK_LEN (PAL_BANK_RGB6 + 768u)

static void pal_bank_put32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void pal_bank_init(uint8_t *bank) {
  memset(bank, 0, PAL_BANK_LEN);
  memcpy(bank, "SHPI", 4);
  pal_bank_put32(bank + 4, PAL_BANK_LEN);
  pal_bank_put32(bank + 8, 3);
  memcpy(bank + 12, "GIMX", 4);
  pal_bank_put32(bank + 16 + 0 * 8 + 4, 40);
  pal_bank_put32(bank + 16 + 1 * 8 + 4, 56);
  pal_bank_put32(bank + 16 + 2 * 8 + 4, PAL_BANK_FRAME2);
  bank[40 + 4] = 1;   /* frame 0: w = h = 1, second_offset 0 */
  bank[40 + 6] = 1;
  bank[56 + 4] = 1;   /* frame 1 */
  bank[56 + 6] = 1;
  bank[PAL_BANK_FRAME2 + 1] = 17;   /* second_offset (LE24 at +1..+3) */
  bank[PAL_BANK_FRAME2 + 4] = 1;
  bank[PAL_BANK_FRAME2 + 6] = 1;
  bank[PAL_BANK_CHUNK] = 0x22;
  bank[PAL_BANK_CHUNK + 4] = 0x00;  /* entry count 256 (LE16) */
  bank[PAL_BANK_CHUNK + 5] = 0x01;
}

static void pal_bank_set6(uint8_t *bank, unsigned index, uint8_t r, uint8_t g, uint8_t b) {
  bank[PAL_BANK_RGB6 + index * 3 + 0] = r;
  bank[PAL_BANK_RGB6 + index * 3 + 1] = g;
  bank[PAL_BANK_RGB6 + index * 3 + 2] = b;
}

static void test_match_palette_from_bank(void) {
  uint8_t bank[PAL_BANK_LEN];
  pal_bank_init(bank);
  pal_bank_set6(bank, 0, 1, 2, 3);
  pal_bank_set6(bank, 63, 63, 63, 63);
  static const uint8_t src[10] = {132, 135, 140, 143, 146, 150, 153, 158, 161, 164};
  static const uint8_t dst[10] = {156, 157, 158, 159, 160, 161, 162, 163, 164, 165};
  for (unsigned i = 0; i < 10; i++) {
    pal_bank_set6(bank, src[i], (uint8_t)(1 + i), 0, 0);
    pal_bank_set6(bank, dst[i], (uint8_t)(20 + i), 0, 0);
  }
  for (unsigned e = 80; e <= 107; e++) pal_bank_set6(bank, e, 0x38, 0x11, 0x28);
  for (unsigned e = 130; e <= 155; e++) pal_bank_set6(bank, e, 0x38, 0x11, 0x28);

  uint8_t base[768];
  memset(base, 0, sizeof base);
  for (unsigned i = 0; i < 10; i++) base[dst[i] * 3] = (uint8_t)(20 + i);
  for (unsigned i = 0; i < 10; i++) base[src[i] * 3] = (uint8_t)(1 + i);
  base[80 * 3] = 20;
  base[130 * 3] = 30;
  base[200 * 3] = 50;

  uint8_t rgb8[768];
  assert(fifa96_match_palette_from_bank(bank, PAL_BANK_LEN, base, rgb8) == FIFA96_OK);
  /* FU-144 erratum (FU-98 §1): the kit remap reads the base snapshot through
   * the 0x70E8 table and writes the 0x70F2 table. */
  assert(rgb8[156 * 3] == 4 && rgb8[157 * 3] == 8 && rgb8[158 * 3] == 12 &&
         rgb8[159 * 3] == 16 && rgb8[160 * 3] == 20 && rgb8[161 * 3] == 24 &&
         rgb8[162 * 3] == 28 && rgb8[163 * 3] == 32 && rgb8[164 * 3] == 36 &&
         rgb8[165 * 3] == 40);
  assert(rgb8[163 * 3] == 32);  /* snapshot: base[158] read before i=2 write */
  assert(rgb8[164 * 3] == 36);  /* snapshot: base[161] read before i=5 write */
  /* Native appends (FUN_00048ED8 0x48F86/0x48F9F): the PALsys chunk's
   * 0xF0/0x54 and 0x186/0x4E ranges overwrite the base (the 130..155 range
   * covers the low remap sources 132..153). */
  assert(rgb8[80 * 3] == 0xE0 && rgb8[80 * 3 + 1] == 0x44 && rgb8[80 * 3 + 2] == 0xA0);
  assert(rgb8[130 * 3] == 0xE0);
  assert(rgb8[132 * 3] == 0xE0);  /* source inside the appended range B */
  assert(rgb8[200 * 3] == 200);   /* untouched base entry */
  assert(rgb8[0] == 0);           /* base entry 0, not the chunk's */

  /* NULL base: the derived engine default is base := the chunk itself. */
  uint8_t rgb8b[768];
  assert(fifa96_match_palette_from_bank(bank, PAL_BANK_LEN, NULL, rgb8b) == FIFA96_OK);
  assert(memcmp(rgb8b, rgb8, sizeof rgb8) != 0);
  /* The native 6->8 conversion is `v << 2` (FUN_000479A0 0x479A8..0x479B9),
   * not the sprite-palette v*255/63 scaling. */
  assert(rgb8b[0] == 4 && rgb8b[1] == 8 && rgb8b[2] == 12);
  assert(rgb8b[63 * 3] == 0xFC && rgb8b[63 * 3 + 1] == 0xFC && rgb8b[63 * 3 + 2] == 0xFC);
  assert(rgb8b[80 * 3] == 0xE0);
  assert(rgb8b[156 * 3] == 0xE0);   /* self-remap from the appended chunk[132] */
  assert(rgb8b[163 * 3] == 32);     /* self-remap snapshot: chunk[158] = 8 */
  assert(rgb8b[200 * 3] == 0);

  /* Contract errors. */
  assert(fifa96_match_palette_from_bank(NULL, PAL_BANK_LEN, NULL, rgb8) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_palette_from_bank(bank, PAL_BANK_LEN, NULL, NULL) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_palette_from_bank(bank, 8, NULL, rgb8) == -FIFA96_ERR_TRUNCATED);
  bank[8] = 2;   /* frame 2 absent */
  assert(fifa96_match_palette_from_bank(bank, PAL_BANK_LEN, NULL, rgb8) ==
         -FIFA96_ERR_INVALID);
  bank[8] = 3;
  bank[PAL_BANK_CHUNK] = 0x23;
  assert(fifa96_match_palette_from_bank(bank, PAL_BANK_LEN, NULL, rgb8) ==
         -FIFA96_ERR_BAD_MAGIC);
  bank[PAL_BANK_CHUNK] = 0x22;
  bank[PAL_BANK_CHUNK + 4] = 0x10;   /* entry count 16, not 256 */
  bank[PAL_BANK_CHUNK + 5] = 0x00;
  assert(fifa96_match_palette_from_bank(bank, PAL_BANK_LEN, NULL, rgb8) ==
         -FIFA96_ERR_UNSUPPORTED);
}

static void test_match_palette_install(void) {
  struct render_fixture f;
  fixture_init(&f);

  uint64_t before = fifa96_surface_hash(f.s);
  assert(fifa96_match_run_palette_install(&f.mr, f.s) == -FIFA96_ERR_STATE);
  assert(fifa96_surface_hash(f.s) == before);
  assert(fifa96_match_run_palette_install(NULL, f.s) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_palette_install(&f.mr, NULL) == -FIFA96_ERR_INVALID);

  for (int i = 0; i < 768; i++) f.mr.render.palette[i] = (uint8_t)(0x20 + (i % 0x20));
  f.mr.render.palette_ready = 1;
  assert(fifa96_match_run_palette_install(&f.mr, f.s) == 0);
  assert(memcmp(f.s->palette, f.mr.render.palette, sizeof f.s->palette) == 0);
  assert(f.s->palette[0] == 0x20 && f.s->palette[31] == 0x3F && f.s->palette[32] == 0x20);

  /* The render pass installs the staged palette and stays deterministic. */
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(memcmp(f.s->palette, f.mr.render.palette, sizeof f.s->palette) == 0);
  uint64_t staged = fifa96_surface_hash(f.s);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(fifa96_surface_hash(f.s) == staged);

  /* Without a staged palette the render leaves the surface palette alone. */
  f.mr.render.palette_ready = 0;
  fifa96_surface_clear(f.s, 0);
  memset(f.s->palette, 0xAB, sizeof f.s->palette);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->palette[0] == 0xAB && f.s->palette[767] == 0xAB);
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
  /* begin ran the derived kickoff pass: the act-1 ball spawn (0x1E0, 0, 0)
   * and the `0x79C13` selector (inactive records -> row 0x26). */
  assert(mr_on.entities.ball.x == 0x1E0);
  assert(mr_on.entities.ball.y == 0);
  assert(mr_on.entities.ball.z == 0);
  assert(mr_on.entities.team[0].records[0].anim_id == 0x26);
  assert(mr_on.entities.team[1].records[10].anim_id == 0x26);
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

/* ------------------------------------------------------------------------ *
 * OL-T11-7 (M2 full-gameplay P0.2): the match HUD (FU-148 §1/§6.1).
 *
 * First-hand native chain: FUN_000565BC's gates (period < 4 via FUN_0004B5F4,
 * not paused via [0x14E688], settings/asset gate FUN_0001D940(6)) select
 * FUN_00055C24, which draws with the full window's clockfnt.fsh and the
 * zoomed playfnt.fsh: bar (Frames.fsh frame 13 via FUN_0009BB20/0x9AFD0) ->
 * name0 -> name1 -> score0 -> score1 (right-aligned at clock_x) -> period ->
 * clock (FUN_00055BA8 centred cells), each text run as colour-6 outline at
 * (+1,+1) then colour-0 main. Layout (FU-148 §1.3/§1.4, full window):
 * row_pitch 0xC, x_left x0+6, clock_x x0+0x32, bar_x x0+2,
 * bar_y y1 - bar_h - 2, y2 bar_y+2, period cell (x0, 2*row_pitch+y2+3,
 * row_pitch-1, row_pitch-1), clock cell (x0+0x11, same y, 0x18, ...).
 *
 * Fixture: a synthetic FNTI font with one 1x1 solid glyph per char '0'..':'
 * and advance 2 (so text widths are exactly 2 per char), the Frames.fsh
 * frame-13 stand-in {0x11, 0, 0, 0x22} with the native layout height 41, and
 * background 0x7F so the main-pass colour 0 is distinguishable from "not
 * drawn". State: score 2-1, total_seconds 83 -> "01:23", period 0 -> "1".
 * ------------------------------------------------------------------------ */

#define HUD_FONT_LEN 0x100

static void hud_put32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

/* Synthetic FNTI image: chars '0'..':' (11 glyphs), default 1x1, width table
 * 1 each, height table 1 each, advance table 2 each, no offsets; the 4bpp
 * bitmap is 32x1 and glyph i sits at bit column 2*i (high nibble of byte i). */
static void hud_font_init(uint8_t *f) {
  memset(f, 0, HUD_FONT_LEN);
  memcpy(f, "FNTI", 4);
  f[4] = 0x30;   /* '0' */
  f[5] = 0x3A;   /* ':' */
  f[6] = 1;
  f[7] = 1;
  f[8] = 0;
  hud_put32(f + 0x10, 0x004C0000u);          /* widths at 0x4C */
  hud_put32(f + 0x14, (0x62u << 16) | 0x57); /* heights 0x57, advances 0x62 */
  hud_put32(f + 0x18, (0x78u << 16) | 0x6D); /* xoff 0x6D, yoff 0x78 */
  hud_put32(f + 0x1C, 0xA0u);                /* bitmap block at 0xA0 */
  for (unsigned i = 0; i < 11; i++) {
    hud_put32(f + 0x20 + i * 4u, i * 2u);    /* bit column 2*i, row 0 */
    f[0x4C + i] = 1;                         /* width */
    f[0x57 + i] = 1;                         /* height */
    f[0x62 + i] = 2;                         /* advance */
    f[0x6D + i] = 0;                         /* x offset */
    f[0x78 + i] = 0;                         /* y offset */
    f[0xB0 + i] = 0xF0;                      /* pixel at bit 2*i: high nibble */
  }
  f[0xA0] = 0x7A;                            /* 4bpp block */
  f[0xA4] = 32;                              /* bitmap width (pixels) */
  f[0xA6] = 1;                               /* bitmap height (rows) */
}

struct hud_fixture {
  struct fifa96_match_run mr;
  struct fifa96_surface *s;
  uint8_t blob[BLOB_LEN];
  int32_t offsets[2];
  struct fifa96_render_bank bank;
  uint8_t frames[5];
  uint8_t font_data[HUD_FONT_LEN];
  uint8_t bar_pixels[4];
};

static void hud_fixture_init(struct hud_fixture *f) {
  memset(f, 0, sizeof *f);
  fifa96_match_run_init(&f->mr);
  f->s = fifa96_surface_create(320, 240);
  assert(f->s != NULL);
  setup_render(&f->mr.render, f->blob, f->offsets, &f->bank, f->frames);
  /* HUD-only canvas: no staged entities, background 0x7F so index-0 text is
   * visible. */
  f->mr.render.entity_count = 0;
  f->mr.render.background = 0x7F;
  fifa96_surface_clear(f->s, 0x7F);

  hud_font_init(f->font_data);
  assert(fifa96_font_parse(f->font_data, sizeof f->font_data,
                           &f->mr.render.hud_font[0]) == FIFA96_OK);
  f->mr.render.hud_font_ready[0] = 1;
  f->bar_pixels[0] = 0x11;
  f->bar_pixels[1] = 0x00;
  f->bar_pixels[2] = 0x00;
  f->bar_pixels[3] = 0x22;
  f->mr.render.hud_bar.width = 2;
  f->mr.render.hud_bar.height = 2;
  f->mr.render.hud_bar.pixels = f->bar_pixels;
  f->mr.render.hud_bar.pixel_len = 4;
  f->mr.render.hud_bar_height = 41;   /* the Frames.fsh frame-4 layout height */
  f->mr.render.hud_bar_ready = 1;

  f->mr.state.period = 0;             /* displayed 1 */
  f->mr.state.total_seconds = 83;     /* 01:23 */
  f->mr.score[0] = 2;
  f->mr.score[1] = 1;
}

/* Full 320x240 window: the derived layout is
 *   row_pitch 12, x_left 6, clock_x 50, bar_x 2, bar_y 240-41-2 = 197,
 *   y2 199, cells_y bar_y+2*12+3 = 224, period cell (0, 224, 11, 11),
 *   clock cell (17, 224, 24, 11). */
static void test_hud_draws_bar_text_cells(void) {
  struct hud_fixture f;
  hud_fixture_init(&f);
  f.mr.render.hud_name[0][0] = '1';
  f.mr.render.hud_name[0][1] = '2';
  f.mr.render.hud_name[0][2] = '\0';
  f.mr.render.hud_name[1][0] = '3';
  f.mr.render.hud_name[1][1] = '4';
  f.mr.render.hud_name[1][2] = '\0';

  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  /* Bar panel at (bar_x, bar_y) = (2,197); pixel 0 transparent. */
  assert(f.s->indexed[197 * 320 + 2] == 0x11);
  assert(f.s->indexed[197 * 320 + 3] == 0x7F);
  assert(f.s->indexed[198 * 320 + 2] == 0x7F);
  assert(f.s->indexed[198 * 320 + 3] == 0x22);
  /* name0 "12" at (x_left, y2) = (6,199): 1x1 glyphs with advance 2, main
   * colour 0, outline +1/+1. */
  assert(f.s->indexed[199 * 320 + 6] == 0x00);
  assert(f.s->indexed[199 * 320 + 7] == 0x7F);
  assert(f.s->indexed[199 * 320 + 8] == 0x00);
  assert(f.s->indexed[200 * 320 + 7] == 0x06);
  assert(f.s->indexed[200 * 320 + 9] == 0x06);
  /* name1 "34" at (6, y2+row_pitch-2) = (6,209). */
  assert(f.s->indexed[209 * 320 + 6] == 0x00);
  assert(f.s->indexed[210 * 320 + 7] == 0x06);
  /* score0 "2" right-aligned at clock_x=50: x = 50-2 = 48; score1 below. */
  assert(f.s->indexed[199 * 320 + 48] == 0x00);
  assert(f.s->indexed[199 * 320 + 49] == 0x7F);
  assert(f.s->indexed[200 * 320 + 49] == 0x06);
  assert(f.s->indexed[200 * 320 + 50] == 0x7F);
  assert(f.s->indexed[209 * 320 + 48] == 0x00);
  assert(f.s->indexed[210 * 320 + 49] == 0x06);
  /* period "1" centred in (0,224,11,11): px = (11-2)/2 = 4. */
  assert(f.s->indexed[224 * 320 + 4] == 0x00);
  assert(f.s->indexed[224 * 320 + 5] == 0x7F);
  assert(f.s->indexed[225 * 320 + 5] == 0x06);
  /* clock "01:23" (width 10) centred in (17,224,24,11): px = 17+7 = 24,
   * glyphs every 2 px -> 24,26,28,30,32. */
  assert(f.s->indexed[224 * 320 + 24] == 0x00);
  assert(f.s->indexed[224 * 320 + 32] == 0x00);
  assert(f.s->indexed[224 * 320 + 33] == 0x7F);
  assert(f.s->indexed[225 * 320 + 25] == 0x06);
  assert(f.s->indexed[225 * 320 + 33] == 0x06);
  assert(f.s->indexed[225 * 320 + 34] == 0x7F);
  /* Outside the HUD block the background is untouched. */
  assert(f.s->indexed[0] == 0x7F);
  assert(f.s->indexed[239 * 320 + 319] == 0x7F);
  assert(f.s->indexed[196 * 320 + 2] == 0x7F);

  uint64_t hud = fifa96_surface_hash(f.s);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(fifa96_surface_hash(f.s) == hud);
  fifa96_surface_destroy(f.s);
}

/* With no staged names the name pass draws nothing (leg OL-T11-72); the
 * scores/period/clock still draw, and a goal score >= 100 shifts the score
 * column and widens the clock cell by the native +8 (full window). */
static void test_hud_names_absent_and_wide_scores(void) {
  struct hud_fixture f;
  hud_fixture_init(&f);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[199 * 320 + 6] == 0x7F);   /* no name0 */
  assert(f.s->indexed[209 * 320 + 6] == 0x7F);   /* no name1 */
  assert(f.s->indexed[199 * 320 + 48] == 0x00);  /* score0 still there */

  f.mr.score[1] = 100;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  /* wide: clock_x 50+8 = 58; score1 "100" (width 6) at 58-6 = 52, glyphs
   * every 2 px; the clock cell widens 24+8 -> px = 17 + (32-10)/2 = 28. */
  assert(f.s->indexed[209 * 320 + 52] == 0x00);
  assert(f.s->indexed[209 * 320 + 53] == 0x7F);
  assert(f.s->indexed[209 * 320 + 54] == 0x00);
  assert(f.s->indexed[210 * 320 + 53] == 0x06);
  assert(f.s->indexed[224 * 320 + 28] == 0x00);
  fifa96_surface_destroy(f.s);
}

/* The derived gates: suspend ([0x14E688]), period >= 4, missing bar and
 * missing font each suppress the whole pass. */
static void test_hud_gates(void) {
  struct hud_fixture f;
  hud_fixture_init(&f);

  f.mr.render.display.suspend = 1;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[197 * 320 + 2] == 0x7F);
  assert(f.s->indexed[224 * 320 + 24] == 0x7F);
  f.mr.render.display.suspend = 0;

  f.mr.state.period = 4;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[197 * 320 + 2] == 0x7F);
  assert(f.s->indexed[224 * 320 + 24] == 0x7F);
  f.mr.state.period = 0;

  f.mr.render.hud_bar_ready = 0;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[197 * 320 + 2] == 0x7F);
  f.mr.render.hud_bar_ready = 1;

  f.mr.render.hud_font_ready[0] = 0;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[197 * 320 + 2] == 0x7F);
  assert(f.s->indexed[224 * 320 + 24] == 0x7F);
  f.mr.render.hud_font_ready[0] = 1;

  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[197 * 320 + 2] == 0x11);
  fifa96_surface_destroy(f.s);
}

/* A 160x200 window is the zoomed branch (FU-93 §1): playfnt is the font,
 * row_pitch 9, and the Frames bar scales by 0xB800. Zoomed layout:
 * bar_x 1, bar_y 200 - ((41*0xB800+0x8000)>>16=29) - 1 = 170, y2 172, cells_y 190,
 * clock cell (0xE=14, 170+18+2=190, 0x12=18, 8); the 2x2 bar frame scales
 * to 1x1. */
static void test_hud_zoomed_uses_playfnt_layout(void) {
  struct hud_fixture f;
  hud_fixture_init(&f);
  assert(fifa96_window_set(&f.mr.render.window, 0, 0, 160, 200) == 0);
  assert(fifa96_font_parse(f.font_data, sizeof f.font_data,
                           &f.mr.render.hud_font[1]) == FIFA96_OK);
  f.mr.render.hud_font_ready[1] = 1;
  /* The zoomed branch must select slot 1, not slot 0. */
  f.mr.render.hud_font_ready[0] = 0;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[170 * 320 + 1] == 0x11);   /* scaled bar pixel */
  /* clock "01:23" centred in (14,192,18,8): px = 14 + 4 = 18. */
  assert(f.s->indexed[190 * 320 + 18] == 0x00);
  assert(f.s->indexed[191 * 320 + 19] == 0x06);
  /* period "1" centred in (0,192,8,8): px = 3. */
  assert(f.s->indexed[190 * 320 + 3] == 0x00);
  /* score0 "2" at clock_x = 0x23 = 35: x = 33. */
  assert(f.s->indexed[172 * 320 + 33] == 0x00);

  /* ...and absent when the zoomed font is missing. */
  f.mr.render.hud_font_ready[1] = 0;
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(f.s->indexed[170 * 320 + 1] == 0x7F);
  fifa96_surface_destroy(f.s);
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
  test_live_anim_inputs_drive_bank_row();
  test_anim_id_staging_round_trip();
  test_near_depth_threshold_gate();
  test_lateral_cull_at_8e0();
  test_formation_seeded_entities_draw();
  test_camera_place_moves_near_record_into_frame();
  test_remap_identity_and_color_key();
  test_match_palette_from_bank();
  test_match_palette_install();
  test_hud_draws_bar_text_cells();
  test_hud_names_absent_and_wide_scores();
  test_hud_gates();
  test_hud_zoomed_uses_playfnt_layout();
  test_engine_match_step_renders();
  puts("test_engine_match_render OK");
  return 0;
}
