#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_record.h"
#include "fifa96_loader/fifa96_scene.h"
#include "fifa96_loader/fifa96_sprite.h"

/* FU-70 §1.2 slot tables (flat 0x11064E direction map and the animation chain
 * T1 0x10E1DC -> T2 0x10E1EC / T3 0x10E1F5). */
static const uint8_t match_run_slot_map[16] = {
  0, 5, 10, 0, 6, 4, 2, 0, 9, 1, 8, 0, 0, 0, 0, 0
};
static const uint8_t match_run_anim_a[16] = {
  0, 1, 5, 0, 3, 2, 4, 0, 7, 8, 6, 0, 0, 0, 0, 0
};
static const uint8_t match_run_anim_b[16] = {
  0, 1, 1, 0, 0xFF, 0xFF, 0xFF, 0, 1, 0, 0, 0xFF, 0xFF, 0xFF, 0, 1
};
static const uint8_t match_run_anim_c[16] = {
  0, 0, 0xFF, 0xFF, 0xFF, 0, 1, 1, 1, 0, 0, 0x40, 0, 0, 0, 0
};

/* FU-61 §2.3 sampler mapping rows: row 0/1 is the identity pinned by
 * tests/test_input.c (`row_identity`). The view-dependent row index [0x7DEC]
 * (FUN_0004CAE4/FUN_0004CA70) belongs to the match camera (Task 15), so the
 * engine keeps the default identity row here. */
static const uint8_t match_run_input_row[FIFA96_INPUT_MAP_ROW] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

/* Engine keys -> FU-61 §1.2 keyboard-handler codes. Directions: the handler
 * ORs 0x1 (0x46934, flag 0x112BEC = scancode 0x48 keypad 8/up), 0x2 (0x46941,
 * 0x50 keypad 2/down), 0x8 (0x46945, 0x4B keypad 4/left) and 0x4 (0x46952,
 * 0x4D keypad 6/right); the diagonal flags at 0x4695F..0x4698F (keypad
 * 7/9/1/3) OR 0x9/0x5/0xA/0x6, cross-checking those axis assignments.
 * Buttons: KICK 0x10, PASS 0x20 (the 0x40 long-ball and 0x80 keeper arms have
 * no engine key yet). */
static int match_run_map_key(int32_t raw_code, uint8_t *code) {
  switch (raw_code) {
    case FIFA96_ENGINE_KEY_UP:    *code = 0x01u; return 1;
    case FIFA96_ENGINE_KEY_DOWN:  *code = 0x02u; return 1;
    case FIFA96_ENGINE_KEY_LEFT:  *code = 0x08u; return 1;
    case FIFA96_ENGINE_KEY_RIGHT: *code = 0x04u; return 1;
    case FIFA96_ENGINE_KEY_KICK:  *code = 0x10u; return 1;
    case FIFA96_ENGINE_KEY_PASS:  *code = 0x20u; return 1;
    default:                      return 0;
  }
}

int fifa96_match_run_input(struct fifa96_match_run *mr, const fifa96_platform_key *keys,
                           size_t count) {
  uint8_t current[FIFA96_INPUT_PLAYERS] = { 0, 0, 0, 0 };
  uint8_t raw = 0;
  uint8_t mapped = 0;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (keys) {
    for (size_t i = 0; i < count; i++) {
      uint8_t code = 0;
      if (keys[i].state != 1) continue;   /* press events only */
      if (!match_run_map_key(keys[i].raw_code, &code)) continue;
      raw |= code;
    }
  }
  (void)fifa96_input_map(raw, match_run_input_row, &mapped);
  current[0] = mapped;
  (void)fifa96_input_update(&mr->input, current);
  memcpy(mr->input_state, current, sizeof current);   /* sampled slot input */
  return 0;
}

/* Stable slot identity: the lifecycle cancels by function pointer, so every
 * run shares this one trampoline and the engine bridge keeps a single match
 * callback in the clock's tick table. The trampoline is the sole driver of the
 * frame body: one call per PIT tick, so the pace stays at 100 Hz even when an
 * engine step spans 0..N ticks (FU-60 drives the pace from the INT-8 ISR). */
static void fifa96_match_run_tick(void *user) {
  struct fifa96_match_run *mr = user;
  if (!mr) return;
  mr->ticks++;
  (void)fifa96_match_run_frame(mr);
}

static int fifa96_match_run_register(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  int slot = fifa96_tick_register(&mr->engine->clock.ticks, fifa96_match_run_tick,
                                  mr, 1u); /* registered at 100 Hz (FU-64 §2.2) */
  return slot < 0 ? slot : 0;
}

static int fifa96_match_run_cancel(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  return fifa96_tick_cancel(&mr->engine->clock.ticks, fifa96_match_run_tick);
}

static int fifa96_match_run_teardown(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* The foundation owns no match assets yet; clear the per-match counters,
   * the match clock/period block and the per-side goal words. */
  mr->ticks = 0;
  mr->steps = 0;
  fifa96_match_state_init(&mr->state);
  mr->score[0] = 0;
  mr->score[1] = 0;
  return 0;
}

static int fifa96_match_run_post_exit(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  mr->engine->mode = FIFA96_ENGINE_MODE_FRONTEND;
  return 0;
}

static void fifa96_match_run_install_defaults(struct fifa96_match_run *mr) {
  mr->backend.register_callback = fifa96_match_run_register;
  mr->backend.cancel_callback = fifa96_match_run_cancel;
  mr->backend.teardown = fifa96_match_run_teardown;
  mr->backend.post_exit = fifa96_match_run_post_exit;
  mr->backend.ctx = mr;
}

/* Fresh-match input state: zero the FU-61 edge/held model and bind the
 * controlled player's FU-70 slot (player 0, raw map select 1 per FU-70 §1.4,
 * ordinal 0, side 0). */
static void fifa96_match_run_reset_input(struct fifa96_match_run *mr) {
  fifa96_input_init(&mr->input);
  memset(mr->input_state, 0, sizeof mr->input_state);
  memset(&mr->slot, 0, sizeof mr->slot);
  (void)fifa96_control_slot_init(&mr->slot, 0, 1, 0, 0);
}

/* Fresh-match presentation state (Task 15): zero the FU-71 camera, FU-92
 * window, FU-90 display block and FU-84/85 scene/sprite assets; rendering is
 * opt-in so the Task 12-14 engine fixtures keep their surface untouched. The
 * indexed remap starts as the identity translation with index 0 as the derived
 * colour key (FU-85 §2/§5: the 0x14720 remap writes 0xFF for transparent
 * pixels; the palette translation itself is an open leg). */
static void fifa96_match_run_reset_render(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r = &mr->render;
  memset(r, 0, sizeof *r);
  (void)fifa96_camera_init(&r->camera, 0, 0, 0);
  (void)fifa96_window_init(&r->window, 0, 0);
  (void)fifa96_match_display_init(&r->display, 0);
  for (int i = 0; i < 256; i++) r->remap[i] = (uint8_t)(i == 0 ? 0xFF : i);
}

/* Release the Task 2 staging arena. Runs only on initialized runs: init must
 * accept uninitialized memory (tests memset 0xAA and re-init), so the owner
 * slot is assigned NULL there and never freed; begin/end/stage call this only
 * after init. A run whose render state was set without staging (owner NULL) is
 * left untouched. */
static void match_run_release_stage(struct fifa96_match_run *mr) {
  if (!mr->stage_owner) return;
  free(mr->stage_owner);
  mr->stage_owner = NULL;
  mr->render.frames = NULL;
  mr->render.banks = NULL;
  mr->render.bank_count = 0;
  mr->render.sprite_data = NULL;
  mr->render.sprite_data_len = 0;
  mr->render.enabled = 0;
}

void fifa96_match_run_init(struct fifa96_match_run *mr) {
  if (!mr) return;
  fifa96_match_lifecycle_init(&mr->lc);
  fifa96_match_pace_init(&mr->pace);
  fifa96_match_state_init(&mr->state);
  mr->score[0] = 0;
  mr->score[1] = 0;
  mr->engine = NULL;
  mr->backend.register_callback = NULL;
  mr->backend.cancel_callback = NULL;
  mr->backend.teardown = NULL;
  mr->backend.post_exit = NULL;
  mr->backend.ctx = NULL;
  mr->ticks = 0;
  mr->steps = 0;
  mr->running = 0;
  mr->stage_owner = NULL;
  memset(&mr->record, 0, sizeof mr->record);
  fifa96_match_run_reset_input(mr);
  fifa96_match_run_reset_render(mr);
}

int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector) {
  if (!mr || !eng) return -FIFA96_ERR_INVALID;
  if (!eng->booted || eng->mode == FIFA96_ENGINE_MODE_QUIT) return -FIFA96_ERR_STATE;
  if (mr->running) return -FIFA96_ERR_STATE;
  if (eng->match && eng->match != mr) return -FIFA96_ERR_STATE;
  if (!mr->backend.register_callback || !mr->backend.cancel_callback ||
      !mr->backend.teardown || !mr->backend.post_exit) {
    fifa96_match_run_install_defaults(mr);
  }
  mr->engine = eng;
  mr->ticks = 0;
  mr->steps = 0;
  fifa96_match_state_init(&mr->state); /* fresh match clock */
  mr->score[0] = 0;                    /* fresh match score pair */
  mr->score[1] = 0;
  memset(&mr->record, 0, sizeof mr->record); /* fresh FU-138 action record */
  fifa96_match_run_reset_input(mr);    /* fresh input edges/held and slot */
  match_run_release_stage(mr);         /* drop the previous match's staged arena */
  fifa96_match_run_reset_render(mr);   /* fresh camera/window/display/scene */
  if (eng->surface) {
    /* FU-92: the derived window setter clamps to the surface; the live match's
     * 160x100 sequence / zoom window selection is an open leg, so the fresh
     * match starts from the full surface window (define_full sets the screen
     * dimensions and applies the window). */
    (void)fifa96_window_define_full(&mr->render.window, eng->surface->width,
                                    eng->surface->height);
  }
  int rc = fifa96_match_lifecycle_begin(&mr->lc, &mr->backend, &mr->pace, selector);
  if (rc != 0) {
    mr->engine = NULL;
    return rc;
  }
  mr->running = 1;
  eng->match = mr;
  eng->mode = FIFA96_ENGINE_MODE_MATCH;
  /* FU-62 §4.6: the state-init zero length would complete a period on the
   * first second, so begin installs the derived lengths for the selector. */
  uint16_t period = selector == 0 ? FIFA96_MATCH_RUN_PERIOD_SECONDS_DEFAULT
                                  : FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET;
  uint16_t extra = selector == 0 ? FIFA96_MATCH_RUN_EXTRA_SECONDS_DEFAULT
                                 : FIFA96_MATCH_RUN_EXTRA_SECONDS_RESET;
  (void)fifa96_match_run_set_period(mr, period, extra);
  return 0;
}

int fifa96_match_run_set_period(struct fifa96_match_run *mr, uint16_t period_seconds,
                                uint16_t extra_seconds) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->state.period_length = period_seconds;
  mr->state.extra_length = extra_seconds;
  return 0;
}

int fifa96_match_run_add_goal(struct fifa96_match_run *mr, uint32_t side) {
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->score[side]++;
  return 0;
}

int fifa96_match_run_frame(struct fifa96_match_run *mr) {
  uint32_t pending;
  uint32_t granted;
  int period_ended = 0;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* One 100 Hz pace tick (FU-60), driven from fifa96_match_run_tick:
   * fifa96_match_state_tick consumes the pace grant and, when granted,
   * advances the Q8 clock by one FIFA96_MATCH_STATE_STEP with clock_halt = 0.
   * The pending count is the grant observable, so this reports whether the
   * 30 Hz frame actually ran. */
  pending = fifa96_match_pace_pending(&mr->pace);
  rc = fifa96_match_state_tick(&mr->state, &mr->pace, 0, 0, &period_ended);
  if (rc != 0) return rc;
  granted = fifa96_match_pace_pending(&mr->pace) - pending;
  /* FU-70 §1.1: the slot machine runs from the frame body once per granted
   * 30 Hz frame with the FU-62 §4.3 whole-frame delta (2 at the 0x200 step,
   * i.e. 60 counter units/s). FU-71's FUN_000736AC and the FU-90 display
   * update run on the same frame-body cadence (the original calls the camera
   * from FUN_0004B100, not from the render driver). One pace tick grants at
   * most one frame today; the loop keeps one update per grant if that ever
   * changes. */
  for (uint32_t i = 0; i < granted; i++) {
    (void)fifa96_control_slot_update(&mr->slot, mr->input_state[0],
                                     (uint8_t)mr->state.frame_delta, match_run_slot_map,
                                     match_run_anim_a, match_run_anim_b, match_run_anim_c);
    (void)fifa96_camera_update(&mr->render.camera, (int16_t)mr->state.frame_delta,
                               mr->render.view_class, mr->render.input_bit2);
    (void)fifa96_match_display_update(&mr->render.display, mr->state.frame_delta, 0);
  }
  if (period_ended && !fifa96_match_lifecycle_should_exit(&mr->lc)) {
    /* Ordering guard: the frame body runs in the clock advance before
     * run_step consumes the exit staging, so a period end coinciding with a
     * staged request_exit must not clobber EXIT->OVER (a staged EXIT wins). */
    rc = fifa96_match_lifecycle_mark_over(&mr->lc);
    if (rc != 0) return rc;
  }
  return granted != 0;
}

int fifa96_match_run_resolve(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER) {
    (void)fifa96_match_lifecycle_resolve_over(&mr->lc);   /* OVER -> POST */
  }
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_POST) {
    (void)fifa96_match_lifecycle_request_exit(&mr->lc);   /* POST -> EXIT */
  }
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
  return 0;
}

int fifa96_match_run_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->steps++;
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
  /* G1 live exit path: the frame body runs on the registered 100 Hz trampoline
   * during this engine step's clock advance, so a period end has just marked
   * the FU-64 lifecycle OVER when the step runs. Drive the post-period chain
   * through the single resolve entry (OVER -> POST -> EXIT -> run_end); the
   * one-step compression is deliberate for G1 and POST screen pacing is an
   * open leg for the Task 10 phase driver. Re-entry guard: resolve's run_end
   * clears running and the engine linkage, so a later step returns
   * -FIFA96_ERR_STATE and the engine's MATCH dispatch leaves through its
   * !e->match arm instead of stepping an ended run. */
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER)
    return fifa96_match_run_resolve(mr);
  /* The frame body does NOT run here: the registered 100 Hz trampoline
   * (fifa96_match_run_tick) already consumed this step's PIT ticks from the
   * engine clock, so pace/state advance once per 10 ms regardless of how many
   * ticks one engine step spans. */
  return 0;
}

int fifa96_match_run_end(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  int rc = fifa96_match_lifecycle_end(&mr->lc, &mr->pace);
  struct fifa96_engine *eng = mr->engine;
  match_run_release_stage(mr);
  mr->running = 0;
  if (eng) {
    if (eng->match == mr) eng->match = NULL;
    if (eng->mode == FIFA96_ENGINE_MODE_MATCH) eng->mode = FIFA96_ENGINE_MODE_FRONTEND;
  }
  return rc;
}

/* Scaled indexed span copy: FU-85 §2/FU-88 §5 reduce the resolved sprite to the
 * pivot-placed destination rectangle (fifa96_render_place) and the clipped
 * cover rectangle (fifa96_render_cover_rect); each output row then samples the
 * sprite at the 16.16 source step and writes through the remap, 0xFF
 * transparent (the FU-85 §2 0xCEABC span writer's semantics) into the indexed
 * canvas.
 *
 * `scale` is the signed FU-85 §2 size factor (`size * scale >> 16`); a negative
 * scale mirrors both axes through the library's signed placement/source
 * stepping, and the resolver's out1 flag flips x only. A zero destination size
 * is off-canvas (skipped). */
static int match_run_draw_sprite(struct fifa96_surface *s, const fifa96_sprite_frame *sprite,
                                 const uint8_t *remap, int32_t x, int32_t y, int32_t scale,
                                 int mirrored, const fifa96_render_clip *clip) {
  int32_t dst_w = (int32_t)(((int64_t)sprite->width * scale) >> 16);
  int32_t dst_h = (int32_t)(((int64_t)sprite->height * scale) >> 16);
  if (dst_w == 0 || dst_h == 0) return 0;
  if (mirrored) dst_w = -dst_w;
  int32_t dst_x = 0;
  int32_t dst_y = 0;
  fifa96_err_t err = fifa96_render_place(sprite->width, sprite->height, sprite->pivot_x,
                                         sprite->pivot_y, dst_w, dst_h, x, y, &dst_x, &dst_y);
  if (err != FIFA96_OK) return (int)err;
  fifa96_render_cover cover;
  uint8_t visible = 0;
  err = fifa96_render_cover_rect(dst_x, dst_y, dst_w, dst_h, sprite->width, sprite->height,
                                 clip, &cover, &visible);
  if (err != FIFA96_OK || !visible) return (int)err;
  int32_t cols[FIFA96_SURFACE_MAX_W + 1];
  for (int32_t row = 0; row < cover.dst_h; row++) {
    int32_t src_row =
        (int32_t)((uint32_t)cover.src_y + (uint32_t)row * (uint32_t)cover.src_dy);
    err = fifa96_sprite_columns(cols, (uint32_t)cover.dst_w, cover.src_x, cover.src_dx,
                                src_row, 0, sprite->width);
    if (err != FIFA96_OK) return (int)err;
    uint8_t *dst = s->indexed + (size_t)(cover.dst_y + row) * (size_t)s->width +
                   (size_t)cover.dst_x;
    err = fifa96_sprite_span(dst, sprite->pixels, sprite->pixel_len, cols,
                             (uint32_t)cover.dst_w, remap);
    if (err != FIFA96_OK) return (int)err;
  }
  return 0;
}

int fifa96_match_run_render(struct fifa96_match_run *mr, struct fifa96_surface *s) {
  if (!mr || !s) return -FIFA96_ERR_INVALID;
  struct fifa96_match_run_render *r = &mr->render;
  if (!r->enabled) return 0;
  if (!r->frames || !r->banks || r->bank_count == 0 || !r->sprite_data ||
      r->sprite_data_len < 16u)
    return -FIFA96_ERR_STATE;

  /* A zero window box means "not defined": fall back to the full surface (the
   * live match's window A/B selection is an open leg). */
  const int have_window = r->window.box.w > 0 && r->window.box.h > 0;

  /* FU-93 §1: window rect -> 16.16 zoom scale (the overlay/HUD coordinate
   * seam; the FU-88 entity projection has no zoom term, FU-88 §5/§8). */
  int32_t scale_x = 0;
  int32_t scale_y = 0;
  int zoomed = 0;
  if (have_window) {
    (void)fifa96_window_scale(&r->window, &scale_x, &scale_y);
    (void)fifa96_window_zoomed(&r->window, &zoomed);
  }
  r->window_scale_x = scale_x;
  r->window_scale_y = scale_y;
  r->window_zoomed = zoomed;

  fifa96_surface_clear(s, r->background);

  /* FU-92 §1/§5: the 16.16 clip rect is the render window; a zero box falls
   * back to the full surface. The projection centre is the window's
   * src_x + src_w/2. */
  fifa96_render_clip clip;
  fifa96_projection_point center;
  if (have_window) {
    clip.left = r->window.box.x0;
    clip.top = r->window.box.y0;
    clip.right = r->window.box.x1;
    clip.bottom = r->window.box.y1;
    center.x = r->window.cx_fix;
    center.y = r->window.cy_fix;
  } else {
    clip.left = 0;
    clip.top = 0;
    clip.right = s->width;
    clip.bottom = s->height;
    center.x = (s->width >> 1) << 16;
    center.y = (s->height >> 1) << 16;
  }
  /* The window's screen dimensions come from the engine surface at begin, but
   * s is a caller argument: clamp the clip to the target surface so the span
   * blit can never write past its logical width/height (or overflow the
   * column scratch). */
  if (clip.left < 0) clip.left = 0;
  if (clip.top < 0) clip.top = 0;
  if (clip.right > s->width) clip.right = s->width;
  if (clip.bottom > s->height) clip.bottom = s->height;

  int32_t matrix[9];
  int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
  int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];
  fifa96_err_t err = fifa96_projection_matrix(r->yaw, r->pitch, matrix);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_projection_reciprocal(s->width, recip_x);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_projection_reciprocal(s->height, recip_y);
  if (err != FIFA96_OK) return (int)err;

  /* FU-85 §4 camera staging (FUN_00036C70's camera copy/bounds over the FU-71
   * block). */
  const fifa96_render_pos cam_src = { r->camera.pos_x, r->camera.pos_y, r->camera.pos_z };
  fifa96_render_pos cam;
  err = fifa96_render_camera_stage(&cam_src, &cam);
  if (err != FIFA96_OK) return (int)err;

  uint32_t count = r->entity_count;
  if (count > FIFA96_MATCH_RUN_RENDER_SLOTS) count = FIFA96_MATCH_RUN_RENDER_SLOTS;

  fifa96_scene_slot slots[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t staged_y[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t angles[FIFA96_MATCH_RUN_RENDER_SLOTS];
  uint8_t drawable[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t jitter_z[FIFA96_MATCH_RUN_RENDER_SLOTS * FIFA96_SCENE_POSITION_DWORDS];
  uint32_t list[FIFA96_MATCH_RUN_RENDER_SLOTS];
  for (uint32_t i = 0; i < count; i++) {
    fifa96_render_slot slot;
    err = fifa96_render_slot_stage(&r->entities[i].stage, &slot);
    if (err != FIFA96_OK) return (int)err;
    staged_y[i] = slot.pos.y;
    angles[i] = slot.angle;
    fifa96_projection_vec rel;
    rel.x = (int32_t)((uint32_t)slot.pos.x - (uint32_t)cam.x);
    rel.y = (int32_t)((uint32_t)slot.pos.y - (uint32_t)cam.y);
    rel.z = (int32_t)((uint32_t)slot.pos.z - (uint32_t)cam.z);
    /* FU-89 §7: the jittered array adds `i%6 + 0x70` to y before rotation. */
    fifa96_projection_vec rel_jitter = rel;
    rel_jitter.y = (int32_t)((uint32_t)rel_jitter.y + (uint32_t)(0x70u + (i % 6u)));
    fifa96_projection_vec clean_rot;
    fifa96_projection_vec jitter_rot;
    err = fifa96_projection_transform(matrix, &rel, &clean_rot);
    if (err != FIFA96_OK) return (int)err;
    err = fifa96_projection_transform(matrix, &rel_jitter, &jitter_rot);
    if (err != FIFA96_OK) return (int)err;
    err = fifa96_scene_slot_project(recip_x, recip_y, &center, &clean_rot, &jitter_rot,
                                    &slots[i]);
    if (err != FIFA96_OK) return (int)err;
    jitter_z[i * FIFA96_SCENE_POSITION_DWORDS + 2] = jitter_rot.z;
    list[i] = i;
    drawable[i] = (uint8_t)(slots[i].clean_visible && slots[i].jitter_visible);
  }

  /* FU-89 §2: seed the depth keys from the jittered z and shell-sort
   * descending. keys[0] stays 0 (the original's seeding quirk: keys[1..count]
   * are written from list[0..count-1], so the index-0 gate is inert). Open
   * legs (no derived source for the engine's scene yet): the near-depth
   * threshold `[0x54350]` (FU-89 §6) and the per-slot lateral `[0x9A74]`
   * `> 0x8E0` cull (FU-89 §3.2); the gate therefore runs with threshold and
   * lateral 0 (the FU-88 near plane still culls z < 5). */
  int32_t keys[FIFA96_MATCH_RUN_RENDER_SLOTS + 1];
  uint32_t values[FIFA96_MATCH_RUN_RENDER_SLOTS];
  keys[0] = 0;
  for (uint32_t i = 0; i < count; i++) values[i] = i;
  err = fifa96_scene_build_keys(count, list, jitter_z, count, keys);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_scene_sort(count, keys, values);
  if (err != FIFA96_OK) return (int)err;

  const fifa96_sprite_bank sprite_bank = {
    .data = r->sprite_data,
    .total_size = r->sprite_data_len,
    .count = 0,
    .tag = { 0, 0, 0, 0 },
  };
  for (uint32_t i = 0; i < count; i++) {
    uint32_t v = values[i];
    if (v >= count) continue;
    uint8_t visible = 0;
    err = fifa96_scene_slot_gate(0, keys[i], staged_y[v], 0, &visible);
    if (err != FIFA96_OK) return (int)err;
    if (!visible || !drawable[v]) continue;
    int32_t jitter_y_out = slots[v].jitter.y;
    uint8_t draw = 0;
    err = fifa96_scene_clip_edges((int32_t)((uint32_t)clip.left << 16),
                                  (int32_t)((uint32_t)clip.top << 16),
                                  (int32_t)((uint32_t)clip.right << 16),
                                  (int32_t)((uint32_t)clip.bottom << 16), slots[v].clean.y,
                                  slots[v].jitter.x, slots[v].jitter.y, &jitter_y_out, &draw);
    if (err != FIFA96_OK) return (int)err;
    if (!draw) continue;

    /* FU-85 §2 direction: `(7 - (((angle - 0x1000) & 0xFFFF) >> 13)) & 7`. The
     * original adds the unported 0xA2A10 view/position angle addend; that
     * addend is an open leg, so the staged FU-84 angle alone drives it. */
    int32_t direction =
        (7 - (int32_t)((uint16_t)((uint32_t)angles[v] - 0x1000u) >> 13)) & 7;
    fifa96_render_frame frame;
    err = fifa96_render_resolve(r->entities[v].stage.anim_id, r->entities[v].stage.frame,
                                direction, r->frames, r->entities[v].bank_index, r->banks,
                                r->bank_count, r->fixed60, r->fixed61, r->mirror, &frame);
    if (err != FIFA96_OK) return (int)err;
    if (!frame.sprite || (uintptr_t)frame.sprite < (uintptr_t)r->sprite_data) continue;
    fifa96_sprite_frame sprite;
    err = fifa96_sprite_frame_parse(&sprite_bank,
                                    (uint32_t)(frame.sprite - r->sprite_data), &sprite);
    if (err != FIFA96_OK) return (int)err;

    /* FU-85 §2 sprite size: scale = ((clean_y - jitter_y + 0x1000) & ~0xFFFF)
     *                       * 0x5D1 / 0x10000
     *   (the 0x571FD sequence); each frame header size scales by the signed
     *   factor with the 32x32->16 fixup (FUN_00057080's SHRD), i.e.
     *   `size * scale >> 16` (negative scale = mirrored). */
    uint32_t delta = (uint32_t)slots[v].clean.y - (uint32_t)slots[v].jitter.y;
    uint32_t size_base = (delta + 0x1000u) & 0xFFFF0000u;
    int32_t scale = (int32_t)(((int64_t)(int32_t)size_base * 0x5D1) >> 16);
    int32_t x = (int32_t)((int64_t)slots[v].clean.x >> 16);
    int32_t y = (int32_t)((int64_t)slots[v].clean.y >> 16);
    int rc = match_run_draw_sprite(s, &sprite, r->remap, x, y, scale, frame.mirrored, &clip);
    if (rc != 0) return rc;
    /* FU-85 §2 second pass: composite animator classes resolve an overlay
     * frame (out2) and the original blits it with a second FUN_00057080 call
     * (0x57241) using the same scale/position. Draw it over the main frame;
     * an unparseable overlay is treated as absent, like the blitter's NULL
     * return. */
    if (frame.overlay && (uintptr_t)frame.overlay >= (uintptr_t)r->sprite_data) {
      fifa96_sprite_frame overlay;
      if (fifa96_sprite_frame_parse(&sprite_bank,
                                    (uint32_t)(frame.overlay - r->sprite_data),
                                    &overlay) == FIFA96_OK) {
        rc = match_run_draw_sprite(s, &overlay, r->remap, x, y, scale, frame.mirrored, &clip);
        if (rc != 0) return rc;
      }
    }
  }
  return 0;
}

/* Task 2 asset staging (FU-84/85/86). The decode chain mirrors the game's
 * resource path: a container record (`[selector 0xFB BE24 size payload]`) is
 * decoded once, then the payload is a BIGF v2 directory (FU-41/FU-86 §2) whose
 * entries are raw SHPI .fsh slices or nested record chains that decode to SHPI
 * (FU-86 §3). Stage 1 is fed the container tail, not the bounded record slice:
 * the original huff reader is unbounded and 10 PLAYART .qfs entries need the
 * following bytes (FU-86 §3 caveat / leg 9); every other stage is exact. */
#define MATCH_STAGE_DECODE_STAGES 4
#define MATCH_STAGE_FRAME_SIZE 5
#define MATCH_STAGE_FRAME_DURATION 0x50u   /* FU-84 §4 row 1 (walk) */
/* FU-84's frame index is a byte and fifa96_render_resolve accepts the signed
 * non-negative half (0..0x7F, `fi = (int8_t)frame_index` in fifa96_render.c);
 * the staged table covers the whole resolver domain so no accepted index can
 * read past it. */
#define MATCH_STAGE_FRAMES_MAX 128u

/* One BIGF entry after decode: an owned SHPI image, or the unloaded slot the
 * renderer skips (FU-85 §1.2: a NULL bank handle). */
struct match_stage_item {
  uint8_t *data;
  uint32_t len;
  uint32_t count;
};

struct match_stage_set {
  struct match_stage_item *items;
  uint32_t count;       /* slots = BIGF entry count (indices stay stable) */
  uint32_t loaded;      /* slots with a decoded SHPI bank */
};

static void match_stage_set_free(struct match_stage_set *set) {
  if (set->items) {
    for (uint32_t i = 0; i < set->count; i++) free(set->items[i].data);
    free(set->items);
  }
  memset(set, 0, sizeof *set);
}

/* Decode a whole bank container to its BIGF v2 bytes: a raw BIGF file or the
 * refpack record form both FU-86 containers use. Returns 0 with a
 * caller-freeable buffer, -1 when the port cannot decode it. */
static int match_stage_container(const uint8_t *file, size_t file_len, uint8_t **out,
                                 size_t *out_len) {
  *out = NULL;
  *out_len = 0;
  if (file_len >= 4 && memcmp(file, "BIGF", 4) == 0) {
    uint8_t *copy = malloc(file_len);
    if (!copy) return -1;
    memcpy(copy, file, file_len);
    *out = copy;
    *out_len = file_len;
    return 0;
  }
  if (file_len >= 5 && file[1] == 0xFB) {
    size_t declared = ((size_t)file[2] << 16) | ((size_t)file[3] << 8) | (size_t)file[4];
    if (declared == 0) return -1;
    uint8_t *buf = malloc(declared);
    if (!buf) return -1;
    size_t n = 0;
    if (fifa96_record_decode(file, file_len, buf, declared, &n) != 0) {
      free(buf);
      return -1;
    }
    *out = buf;
    *out_len = n;
    return 0;
  }
  return -1;
}

/* Decode one BIGF entry to a SHPI image. `entry_len` is the entry's bounded
 * BIGF record size (used for the raw SHPI form, whose declared total equals it,
 * FU-86 §4) and `raw_len` the container tail from the entry's offset (the
 * stage-1 over-read allowance noted above). Returns 1 with an owned copy and
 * its frame count, 0 when the entry is not a sprite bank (fonts, .dat tables),
 * -1 when it is SHPI/record shaped but the port cannot decode it. */
static int match_stage_entry(const uint8_t *raw, size_t entry_len, size_t raw_len,
                             uint8_t **out, uint32_t *out_len, uint32_t *out_count) {
  *out = NULL;
  *out_len = 0;
  *out_count = 0;
  if (raw_len >= 4 && memcmp(raw, "SHPI", 4) == 0) {
    fifa96_sprite_bank bank;
    if (fifa96_sprite_bank_parse(raw, entry_len, &bank) != FIFA96_OK) return -1;
    uint8_t *copy = malloc(bank.total_size);
    if (!copy) return -1;
    memcpy(copy, raw, bank.total_size);
    *out = copy;
    *out_len = bank.total_size;
    *out_count = bank.count;
    return 1;
  }
  if (raw_len < 5 || raw[1] != 0xFB) return 0;
  const uint8_t *src = raw;
  size_t src_len = raw_len;
  uint8_t *cur = NULL;
  for (int stage = 0; stage < MATCH_STAGE_DECODE_STAGES; stage++) {
    if (src_len >= 4 && memcmp(src, "SHPI", 4) == 0) {
      fifa96_sprite_bank bank;
      if (fifa96_sprite_bank_parse(src, src_len, &bank) != FIFA96_OK) {
        free(cur);
        return -1;
      }
      uint8_t *copy = malloc(bank.total_size);
      if (!copy) {
        free(cur);
        return -1;
      }
      memcpy(copy, src, bank.total_size);
      free(cur);
      *out = copy;
      *out_len = bank.total_size;
      *out_count = bank.count;
      return 1;
    }
    if (src_len < 5 || src[1] != 0xFB) {
      free(cur);
      return -1;
    }
    size_t declared = ((size_t)src[2] << 16) | ((size_t)src[3] << 8) | (size_t)src[4];
    if (declared == 0) {
      free(cur);
      return -1;
    }
    uint8_t *dst = malloc(declared);
    if (!dst) {
      free(cur);
      return -1;
    }
    size_t n = 0;
    if (fifa96_record_decode(src, src_len, dst, declared, &n) != 0) {
      free(dst);
      free(cur);
      return -1;
    }
    free(cur);
    cur = dst;
    src = dst;
    src_len = n;
  }
  free(cur);
  return -1;
}

/* Decode every BIGF entry of one container into a slot-stable set: slot i is
 * container entry i, so the player container's indices are exactly the FU-84
 * row +8 bank indices (observed 0..89). Entries the port cannot decode stay
 * unloaded slots, the renderer's skippable state (FU-85 §1.2). */
static int match_stage_set_build(struct match_stage_set *set, const uint8_t *file,
                                 size_t file_len) {
  memset(set, 0, sizeof *set);
  uint8_t *container = NULL;
  size_t container_len = 0;
  if (match_stage_container(file, file_len, &container, &container_len) != 0) return -1;
  struct fifa96_bigf_info info;
  if (fifa96_bigf_parse(container, container_len, &info) != FIFA96_OK) goto fail;
  if (info.count == 0) goto fail;
  set->items = calloc(info.count, sizeof *set->items);
  if (!set->items) goto fail;
  set->count = (uint32_t)info.count;
  for (size_t i = 0; i < info.count; i++) {
    uint32_t off = 0;
    uint32_t size = 0;
    if (fifa96_bigf_record(&info, i, &off, &size, NULL) != FIFA96_OK) goto fail;
    uint8_t *bank = NULL;
    uint32_t bank_len = 0;
    uint32_t bank_count = 0;
    int got = match_stage_entry(container + off, size, container_len - off, &bank,
                                &bank_len, &bank_count);
    if (got == 1) {
      set->items[i].data = bank;
      set->items[i].len = bank_len;
      set->items[i].count = bank_count;
      set->loaded++;
    } else if (got < 0) {
      set->items[i].data = NULL;   /* shaped like a bank but undecodable */
    }
  }
  free(container);
  return 0;
fail:
  free(container);
  match_stage_set_free(set);
  return -1;
}

/* Round an arena cursor up to `align`. SHPI copies need 4 for the int32 offset
 * table at SHPI+0x14 (FU-86 §4: directory entries {name[4], u32 offset}); the
 * bank record array needs its natural alignment for UBSan-clean access. */
static size_t match_stage_pad(size_t n, size_t align) {
  return (n + align - 1u) & ~(align - 1u);
}

int fifa96_match_run_stage(struct fifa96_match_run *mr, const struct fifa96_surface *s,
                           const char *player_bank, const char *pitch_bank) {
  struct match_stage_set player;
  struct match_stage_set pitch;
  struct fifa96_cache *cache;
  const uint8_t *file;
  size_t file_len;
  uint32_t lba = 0;
  uint32_t size = 0;
  size_t frames_len;
  size_t banks_off;
  size_t data_off;
  size_t data_len = 0;
  size_t arena_len;
  uint8_t *arena = NULL;
  struct fifa96_render_bank *banks;
  uint8_t *frames;
  uint32_t total;
  int rc;

  if (!mr || !s || !player_bank || !pitch_bank) return -FIFA96_ERR_INVALID;
  /* A live run guarantees the engine (and its asset table) outlives the arena
   * the run takes ownership of; staging after end would otherwise strand the
   * arena (end releases once) or reach a destroyed engine. */
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!mr->engine || !mr->engine->assets) return -FIFA96_ERR_STATE;

  memset(&player, 0, sizeof player);
  memset(&pitch, 0, sizeof pitch);
  cache = fifa96_cache_create(mr->engine->assets);
  if (!cache) return -FIFA96_ERR_UNSUPPORTED;

  /* Player animation container (FU-86 §1: PLAYART, 91 banks). */
  if (fifa96_asset_lookup(mr->engine->assets, player_bank, &lba, &size) != FIFA96_OK) {
    rc = -FIFA96_ERR_NOT_FOUND;
    goto out_cache;
  }
  file = fifa96_cache_get(cache, player_bank, &file_len);
  if (!file || file_len == 0 || match_stage_set_build(&player, file, file_len) != 0 ||
      player.loaded == 0) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }

  /* Second (pitch/match art) container (FU-86 §1: GAMEART0; identity open). */
  if (fifa96_asset_lookup(mr->engine->assets, pitch_bank, &lba, &size) != FIFA96_OK) {
    rc = -FIFA96_ERR_NOT_FOUND;
    goto out_sets;
  }
  file = fifa96_cache_get(cache, pitch_bank, &file_len);
  if (!file || file_len == 0 || match_stage_set_build(&pitch, file, file_len) != 0 ||
      pitch.loaded == 0) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }

  /* One arena owns the frame table, the bank records (bases/offset tables
   * point into the SHPI copies) and the SHPI images. Nothing touches mr until
   * the whole build succeeds. */
  total = player.count + pitch.count;
  const size_t bank_align = _Alignof(struct fifa96_render_bank);
  frames_len = (size_t)MATCH_STAGE_FRAMES_MAX * MATCH_STAGE_FRAME_SIZE;
  banks_off = match_stage_pad(frames_len, bank_align);
  data_off = match_stage_pad(banks_off + (size_t)total * sizeof *banks, 4u);
  for (uint32_t i = 0; i < player.count; i++)
    if (player.items[i].data) data_len += match_stage_pad(player.items[i].len, 4u);
  for (uint32_t i = 0; i < pitch.count; i++)
    if (pitch.items[i].data) data_len += match_stage_pad(pitch.items[i].len, 4u);
  if (data_off + data_len > 0xFFFFFFFFu) {
    rc = -FIFA96_ERR_UNSUPPORTED;   /* sprite_data_len is 32-bit */
    goto out_sets;
  }
  arena_len = data_off + data_len;
  arena = calloc(1, arena_len);
  if (!arena) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }
  frames = arena;
  banks = (struct fifa96_render_bank *)(void *)(arena + banks_off);

  /* FU-84 §4 frame records: the row-1 walk evidence stores sprite = frame
   * index with duration 0x50, so staging derives that identity table across
   * the resolver's whole 128-index domain; the real per-row tables are
   * executable object-4 data (0x10E200..0x10EF00), not ISO assets (open leg). */
  for (uint32_t i = 0; i < MATCH_STAGE_FRAMES_MAX; i++) {
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 0] = (uint8_t)MATCH_STAGE_FRAME_DURATION;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 1] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 2] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 3] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 4] = (uint8_t)i;
  }

  size_t cursor = data_off;
  uint32_t slot = 0;
  uint32_t base_slot = 0;
  const struct match_stage_set *sets[2] = { &player, &pitch };
  for (int which = 0; which < 2; which++) {
    const struct match_stage_set *set = sets[which];
    for (uint32_t i = 0; i < set->count; i++, slot++) {
      struct fifa96_render_bank *b = &banks[slot];
      if (!set->items[i].data) continue;   /* unloaded slot stays zeroed */
      uint8_t *copy = arena + cursor;
      memcpy(copy, set->items[i].data, set->items[i].len);
      b->base = copy;
      b->offsets = (const int32_t *)(const void *)(copy + 0x14);
      b->count = set->items[i].count;
      /* The FU-86 §4.1 stride switch indexes the animator-record array of the
       * container the bank came from (the player container is the derived
       * 0x57DE8 array; indices 0..90). Appended containers have no derived
       * animator identity (open leg), so their banks use a container-local
       * index instead of leaking into the player switch's classes (the old
       * global slot 0x5B hit /2 by accident). */
      b->step = fifa96_sprite_stride(slot - base_slot, set->items[i].count);
      cursor += match_stage_pad(set->items[i].len, 4u);
    }
    base_slot += set->count;
  }

  free(mr->stage_owner);
  mr->stage_owner = arena;
  struct fifa96_match_run_render *r = &mr->render;
  r->frames = frames;
  r->banks = banks;
  r->bank_count = total;
  r->fixed60 = NULL;   /* runtime-populated records, not ISO assets (FU-85 leg 8) */
  r->fixed61 = NULL;
  r->mirror = NULL;    /* 0x10F2E7 is executable data, not on the ISO (FU-85 §1.3) */
  r->sprite_data = arena;
  r->sprite_data_len = (uint32_t)arena_len;
  (void)fifa96_window_init(&r->window, s->width, s->height);
  (void)fifa96_window_define_full(&r->window, s->width, s->height);
  r->enabled = 1;
  arena = NULL;
  rc = 0;

out_sets:
  match_stage_set_free(&player);
  match_stage_set_free(&pitch);
out_cache:
  free(arena);
  fifa96_cache_destroy(cache);
  return rc;
}
