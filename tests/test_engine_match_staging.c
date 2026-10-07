/* tests/test_engine_match_staging.c — M2 Task 2: match asset staging.
 *
 * FU chain under test (docs/ghidra/):
 *  - FU-86 §1/§2: art/playart.pvi loads through FUN_0004AAE0 into resource slot
 *    0x47 and is a refpack stream whose decoded payload is a BIGF v2 container
 *    of 91 entries; each entry is a raw SHPI .fsh bank or a nested .qfs chain
 *    huff(0x31) -> refpack(0x10) -> tree(0x46) -> SHPI. Its data cross-check
 *    (FU-86 "Method") reads both containers from the retail ISO:
 *    /ART/PLAYART.PVI (807380 B) and /ART/GAMEART0.PVI (154387 B). FU-86 §1
 *    also cites art/gameart0.pvi as the match art container (resource slots
 *    0..0x3E), so the derived ISO bank pair for this test is those two paths.
 *  - FU-84 §3.1/§4: an animation row carries a sprite-bank index at +8
 *    (observed 0..89, i.e. the PLAYART container order) and points at 5-byte
 *    frame records {duration:u16, aux:u16, sprite:u8}; row 1 (walk) stores
 *    `50 00 00 00 00 / 50 00 00 00 01 / ...`, i.e. sprite = frame index.
 *  - FU-85 §1.2: a bank is the SHPI container handle; a missing handle is the
 *    renderer's "not loaded" state (the port's NULL bank).
 *  - FU-92/§3: the render window is a surface-relative box the render clips to.
 *
 * The second container's bank identity (which GAMEART0 banks serve the pitch
 * stage) is not derivable from FU-84/85/86/89; it is recorded as an open leg
 * and passed by the caller, as the interface requires.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_sprite.h"

/* The FU-84 frame index is a signed byte; fifa96_render_resolve accepts the
 * non-negative half (0..0x7F, fifa96_render.c) with no table bound. */
#define MATCH_RESOLVER_FRAMES_MAX 128u

#define STAGE_ISO_PATH "game/FIFAPCCD96.iso"
/* FU-86 data cross-check paths (ISO holds the upper-case 8.3 names). */
#define STAGE_PLAYER_BANK "/ART/PLAYART.PVI"
#define STAGE_PITCH_BANK "/ART/GAMEART0.PVI"
/* A second, smaller FU-129-listed pitch container: 2 SHPI banks + a .dat. */
#define STAGE_PITCH_FIELD "/ART/GAMEFLD1.PVI"
#define STAGE_ABSENT_BANK "/ART/NOSTAGE.PVI"
#define STAGE_NOT_A_BANK "/VIDEO/VID_INTR.TGV"

static int file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

struct stage_fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
  struct fifa96_match_run mr;
  struct fifa96_surface *s;
};

/* The fixture is caller-owned and the run is begun at its final address: a
 * by-value return would memcpy the struct and leave `engine->match` pointing
 * into the released frame, so the engine destroy would end a dead run. */
static void fixture_make(struct stage_fixture *f, const char *iso_path) {
  memset(f, 0, sizeof *f);
  f->plat = fifa96_platform_null_create(NULL);
  assert(f->plat != NULL);
  struct fifa96_engine_config cfg;
  memset(&cfg, 0, sizeof cfg);
  cfg.iso_path = iso_path;
  cfg.width = 320;
  cfg.height = 240;
  cfg.headless = 1;
  f->engine = fifa96_engine_create(&cfg, f->plat);
  assert(f->engine != NULL);
  assert(fifa96_engine_boot(f->engine) == 0);
  /* Zero the run first so the struct has no undefined padding for the
   * no-mutation memcmp checks. */
  memset(&f->mr, 0, sizeof f->mr);
  fifa96_match_run_init(&f->mr);
  assert(fifa96_match_run_begin(&f->mr, f->engine, 0) == 0);
  assert(f->engine->match == &f->mr);   /* begun on the stable object */
  assert(f->mr.render.enabled == 0);
  f->s = fifa96_surface_create(320, 240);
  assert(f->s != NULL);
}

static void fixture_drop(struct stage_fixture *f) {
  assert(fifa96_match_run_end(&f->mr) == 0);
  assert(f->engine->match == NULL);   /* no stale pointer for engine destroy */
  assert(fifa96_match_run_end(&f->mr) == -FIFA96_ERR_STATE);   /* second end no-op */
  /* Supported reset order: end released the staged arena, so re-initializing
   * is leak-free (ASan). init itself never frees a holder it cannot trust. */
  fifa96_match_run_init(&f->mr);
  fifa96_engine_destroy(f->engine);
  fifa96_platform_destroy(f->plat);
  fifa96_surface_destroy(f->s);
}

/* NULL arguments and an unbound run (no engine assets) are contract errors
 * that never touch the asset table. */
static void test_argument_contract(void) {
  struct fifa96_match_run mr;
  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(fifa96_match_run_stage(NULL, s, STAGE_PLAYER_BANK, STAGE_PITCH_BANK) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_stage(&mr, NULL, STAGE_PLAYER_BANK, STAGE_PITCH_BANK) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_stage(&mr, s, NULL, STAGE_PITCH_BANK) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_stage(&mr, s, STAGE_PLAYER_BANK, NULL) == -FIFA96_ERR_INVALID);
  /* no engine bound: no asset table to read. */
  assert(fifa96_match_run_stage(&mr, s, STAGE_PLAYER_BANK, STAGE_PITCH_BANK) ==
         -FIFA96_ERR_STATE);
  assert(mr.render.enabled == 0);
  fifa96_surface_destroy(s);
}

/* Staging requires a live run: after end the guard refuses before touching the
 * engine, so an arena cannot outlive the end that releases it. */
static void test_stage_requires_live_run(void) {
  struct stage_fixture f;
  fixture_make(&f, NULL);
  assert(fifa96_match_run_end(&f.mr) == 0);
  assert(f.engine->match == NULL);
  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_PLAYER_BANK, STAGE_PITCH_BANK) ==
         -FIFA96_ERR_STATE);
  assert(f.mr.render.enabled == 0);
  fifa96_engine_destroy(f.engine);
  fifa96_platform_destroy(f.plat);
  fifa96_surface_destroy(f.s);
}

/* No ISO (the boot smoke mode mounts an empty table): the absent bank is
 * NOT_FOUND and the run is bit-identical afterwards. */
static void test_absent_assets_no_iso(void) {
  struct stage_fixture f;
  fixture_make(&f, NULL);
  struct fifa96_match_run before;
  memcpy(&before, &f.mr, sizeof before);
  fifa96_surface_clear(f.s, 0x11);
  uint64_t canvas = fifa96_surface_hash(f.s);

  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_PLAYER_BANK, STAGE_PITCH_BANK) ==
         -FIFA96_ERR_NOT_FOUND);
  assert(f.mr.render.enabled == 0);
  assert(f.mr.render.frames == NULL && f.mr.render.banks == NULL);
  assert(f.mr.render.sprite_data == NULL);
  assert(memcmp(&f.mr, &before, sizeof before) == 0);
  assert(fifa96_surface_hash(f.s) == canvas);
  fixture_drop(&f);
}

/* Absent paths on a mounted ISO are NOT_FOUND with no run mutation, for either
 * side of the pair. */
static void test_absent_paths_with_iso(void) {
  struct stage_fixture f;
  fixture_make(&f, STAGE_ISO_PATH);
  struct fifa96_match_run before;
  memcpy(&before, &f.mr, sizeof before);

  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_ABSENT_BANK, STAGE_PITCH_BANK) ==
         -FIFA96_ERR_NOT_FOUND);
  assert(memcmp(&f.mr, &before, sizeof before) == 0);
  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_PLAYER_BANK, STAGE_ABSENT_BANK) ==
         -FIFA96_ERR_NOT_FOUND);
  assert(memcmp(&f.mr, &before, sizeof before) == 0);
  assert(f.mr.render.enabled == 0);
  fixture_drop(&f);
}

/* A present file the port cannot decode as a sprite-bank container is
 * UNSUPPORTED, again with no run mutation. */
static void test_undecodable_bank_with_iso(void) {
  struct stage_fixture f;
  fixture_make(&f, STAGE_ISO_PATH);
  struct fifa96_match_run before;
  memcpy(&before, &f.mr, sizeof before);

  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_NOT_A_BANK, STAGE_PITCH_BANK) ==
         -FIFA96_ERR_UNSUPPORTED);
  assert(memcmp(&f.mr, &before, sizeof before) == 0);
  assert(f.mr.render.enabled == 0);
  fixture_drop(&f);
}

/* The full staging contract on the derived ISO pair: real banks land in
 * render.banks (player container first, FU-84 row +8 order), frames and the
 * arena are non-NULL, the FU-92 window is the surface box, render.enabled is 1
 * only after success, and the staged render runs on a cleared surface. */
static void test_stage_derived_pair_with_iso(void) {
  struct stage_fixture f;
  fixture_make(&f, STAGE_ISO_PATH);

  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_PLAYER_BANK, STAGE_PITCH_BANK) == 0);
  const struct fifa96_match_run_render *r = &f.mr.render;
  assert(r->enabled == 1);
  assert(r->frames != NULL);
  assert(r->banks != NULL);
  assert(r->bank_count > 91u);   /* 91 PLAYART slots + the pitch container's */
  assert(r->sprite_data != NULL && r->sprite_data_len >= 16u);

  /* FU-92/FU-93: the window is the full surface box (non-degenerate). */
  assert(r->window.box.w == f.s->width && r->window.box.h == f.s->height);
  assert(r->window.box.x0 == 0 && r->window.box.y0 == 0);
  assert(r->window.box.x1 == f.s->width && r->window.box.y1 == f.s->height);

  /* FU-86 §2/§4.2: PLAYART entry 0 (xstandd.fsh) is a raw SHPI bank at slot 0
   * with its five standing poses; bank index 0 falls to the 0x78B5C switch
   * default, so the derived stride is count/5 = 1 (not the value under test). */
  assert(r->banks[0].base != NULL && r->banks[0].count == 5u);
  assert(r->banks[0].step == 1);
  size_t bank0_len = (size_t)(r->banks[0].base - r->sprite_data);
  assert(bank0_len < r->sprite_data_len);
  fifa96_sprite_bank bank0;
  assert(fifa96_sprite_bank_parse(r->banks[0].base,
                                  r->sprite_data_len - bank0_len, &bank0) == FIFA96_OK);
  assert(bank0.count == r->banks[0].count);
  fifa96_sprite_entry entry0;
  assert(fifa96_sprite_bank_entry(&bank0, 0, &entry0) == FIFA96_OK);
  fifa96_sprite_frame frame0;
  assert(fifa96_sprite_frame_parse(&bank0, entry0.offset, &frame0) == FIFA96_OK);
  assert(frame0.width != 0 && frame0.height != 0);
  assert(frame0.pixels >= r->sprite_data &&
         frame0.pixels + frame0.pixel_len <= r->sprite_data + r->sprite_data_len);

  /* The supplied pitch container contributes appended sprite banks beyond the
   * 91 player slots (FU-86 §1: GAMEART0 fills match art slots 0..0x3E). The
   * ISO's entry 35 (Npost.fsh) has 4 frames and lands at slot 91+35; its
   * stride index is container-local (0x23 -> the 0x78B5C /2 class -> 2), where
   * the old global slot 0x7E fell to the switch default /5 -> 0. */
  uint32_t pitch_banks = 0;
  for (uint32_t i = 91; i < r->bank_count; i++)
    if (r->banks[i].base != NULL) pitch_banks++;
  assert(pitch_banks > 0);
  assert(r->banks[91u + 35u].count == 4u);
  assert(r->banks[91u + 35u].step == 2);

  /* FU-84 §4/§5: frame_index is a signed byte, so the resolver accepts
   * 0..0x7F; the staged table covers all 128 records and every accepted index
   * resolves inside the arena. */
  assert((size_t)(r->frames - r->sprite_data) + MATCH_RESOLVER_FRAMES_MAX * 5u <=
         r->sprite_data_len);
  assert(r->frames[127 * 5 + 4] == 127);
  for (int fi = 0; fi < (int)MATCH_RESOLVER_FRAMES_MAX; fi++) {
    fifa96_render_frame resolved;
    assert(fifa96_render_resolve(0, fi, 0, r->frames, 0, r->banks, r->bank_count,
                                 r->fixed60, r->fixed61, r->mirror, &resolved) == FIFA96_OK);
  }

  /* The staged render consumes the staged frame table/bank/sprite data: stage
   * one entity (test_engine_match_render geometry) and require a drawn pixel,
   * not just the background clear. Recomposition stays hash-stable. */
  f.mr.render.entities[0].stage.pos.x = 0;
  f.mr.render.entities[0].stage.pos.y = 0;
  f.mr.render.entities[0].stage.pos.z = 0x200;
  f.mr.render.entities[0].stage.heading = 0;
  f.mr.render.entities[0].stage.anim_id = 0;
  f.mr.render.entities[0].stage.frame = 0;
  f.mr.render.entities[0].stage.hidden = 0;
  f.mr.render.entities[0].bank_index = 0;
  f.mr.render.entity_count = 1;

  fifa96_surface_clear(f.s, 0x5A);
  uint64_t before = fifa96_surface_hash(f.s);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  uint64_t staged = fifa96_surface_hash(f.s);
  assert(staged != before);
  uint32_t drawn = 0;
  for (int i = 0; i < f.s->width * f.s->height; i++)
    if (f.s->indexed[i] != f.mr.render.background) drawn++;
  assert(drawn > 0);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);
  assert(fifa96_surface_hash(f.s) == staged);

  /* A failed re-stage leaves the live staging bit-identical. */
  struct fifa96_match_run live;
  memcpy(&live, &f.mr, sizeof live);
  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_PLAYER_BANK, STAGE_ABSENT_BANK) ==
         -FIFA96_ERR_NOT_FOUND);
  assert(memcmp(&f.mr, &live, sizeof live) == 0);
  assert(f.mr.render.enabled == 1);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);

  /* Idempotent re-stage swaps the arena without leaking (ASan) and a different
   * pitch container reshapes the appended slots. */
  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_PLAYER_BANK, STAGE_PITCH_BANK) == 0);
  assert(f.mr.render.enabled == 1 && f.mr.render.frames != NULL);
  uint32_t gameart_slots = f.mr.render.bank_count;
  assert(fifa96_match_run_stage(&f.mr, f.s, STAGE_PLAYER_BANK, STAGE_PITCH_FIELD) == 0);
  assert(f.mr.render.enabled == 1);
  assert(f.mr.render.bank_count > 91u && f.mr.render.bank_count < gameart_slots);
  assert(f.mr.render.banks[0].base != NULL);
  assert(fifa96_match_run_render(&f.mr, f.s) == 0);

  fixture_drop(&f);
}

int main(void) {
  test_argument_contract();
  test_stage_requires_live_run();
  test_absent_assets_no_iso();
  if (file_exists(STAGE_ISO_PATH)) {
    test_absent_paths_with_iso();
    test_undecodable_bank_with_iso();
    test_stage_derived_pair_with_iso();
  } else {
    fprintf(stderr, "SKIP ISO staging cases (no %s)\n", STAGE_ISO_PATH);
  }
  puts("test_engine_match_staging OK");
  return 0;
}
