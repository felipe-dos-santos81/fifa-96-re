#include <stddef.h>
#include <string.h>
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_loader/fifa96_projection.h"
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
  /* The foundation owns no match assets yet; clear the per-match counters and
   * the match clock/period block. */
  mr->ticks = 0;
  mr->steps = 0;
  fifa96_match_state_init(&mr->state);
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

void fifa96_match_run_init(struct fifa96_match_run *mr) {
  if (!mr) return;
  fifa96_match_lifecycle_init(&mr->lc);
  fifa96_match_pace_init(&mr->pace);
  fifa96_match_state_init(&mr->state);
  mr->engine = NULL;
  mr->backend.register_callback = NULL;
  mr->backend.cancel_callback = NULL;
  mr->backend.teardown = NULL;
  mr->backend.post_exit = NULL;
  mr->backend.ctx = NULL;
  mr->ticks = 0;
  mr->steps = 0;
  mr->running = 0;
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
  fifa96_match_run_reset_input(mr);    /* fresh input edges/held and slot */
  fifa96_match_run_reset_render(mr);   /* fresh camera/window/display/scene */
  if (eng->surface) {
    /* FU-92: the derived window setter clamps to the surface; the live match's
     * 160x100 sequence / zoom window selection is an open leg, so the fresh
     * match starts from the full surface window. */
    (void)fifa96_window_init(&mr->render.window, eng->surface->width,
                             eng->surface->height);
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
  if (period_ended) {
    rc = fifa96_match_lifecycle_mark_over(&mr->lc);
    if (rc != 0) return rc;
  }
  return granted != 0;
}

int fifa96_match_run_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->steps++;
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
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
 * canvas. `dst_w` may be negative to mirror (the resolver's out1 flag). */
static int match_run_draw_sprite(struct fifa96_surface *s, const fifa96_sprite_frame *sprite,
                                 const uint8_t *remap, int32_t x, int32_t y, int32_t dst_w,
                                 int32_t dst_h, const fifa96_render_clip *clip) {
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
   * are written from list[0..count-1], so the index-0 gate is inert). The
   * near-depth threshold [0x54350] is not derived in this task, so the gate
   * runs with threshold 0 (the FU-88 near plane still culls z < 5). */
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
     *   (the 0x571FD sequence); the frame's header size scales by it with the
     *   32x32->16 fixup (FUN_00057080's SHRD), i.e. `size * scale >> 16`. */
    uint32_t delta = (uint32_t)slots[v].clean.y - (uint32_t)slots[v].jitter.y;
    uint32_t size_base = (delta + 0x1000u) & 0xFFFF0000u;
    int32_t scale = (int32_t)(((int64_t)(int32_t)size_base * 0x5D1) >> 16);
    int32_t dst_w = (int32_t)(((int64_t)sprite.width * scale) >> 16);
    int32_t dst_h = (int32_t)(((int64_t)sprite.height * scale) >> 16);
    if (dst_w <= 0 || dst_h <= 0) continue;
    if (frame.mirrored) dst_w = -dst_w;
    int32_t x = (int32_t)((int64_t)slots[v].clean.x >> 16);
    int32_t y = (int32_t)((int64_t)slots[v].clean.y >> 16);
    int rc = match_run_draw_sprite(s, &sprite, r->remap, x, y, dst_w, dst_h, &clip);
    if (rc != 0) return rc;
  }
  return 0;
}
