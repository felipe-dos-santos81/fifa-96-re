#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_engine/fifa96_match_phase_machine.h"
#include "fifa96_engine/fifa96_match_act.h"
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_control.h"
#include "fifa96_loader/fifa96_font.h"
#include "fifa96_loader/fifa96_input.h"
#include "fifa96_loader/fifa96_match_display.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"
#include "fifa96_loader/fifa96_match_pace.h"
#include "fifa96_loader/fifa96_match_state.h"
#include "fifa96_loader/fifa96_palette.h"
#include "fifa96_loader/fifa96_referee.h"
#include "fifa96_loader/fifa96_render.h"
#include "fifa96_loader/fifa96_rng.h"
#include "fifa96_loader/fifa96_settings.h"
#include "fifa96_loader/fifa96_sprite.h"
#include "fifa96_loader/fifa96_window.h"

/* Engine-level match driver (M2 foundation): owns the FU-64 lifecycle and the
 * FU-60 pace, the FU-61 input model and the controlled player's FU-70 control
 * slot, and binds them to an engine.
 *
 * Lifecycle: always call fifa96_match_run_init before first use — begin reads
 * the struct, so a non-initialized run (including one with a stale function
 * pointer in `backend`) is undefined behavior. begin is the only start and
 * end (or the exit-step implicit end) the only stop; the begin/end balance is
 * tracked in `running`.
 *
 * Ownership: the run passed to begin stays caller-owned and must remain alive
 * until fifa96_match_run_end returns or fifa96_engine_destroy runs, whichever
 * comes first. The engine ends a live run at destroy before freeing itself,
 * so a running run must outlive the engine call.
 *
 * Backend injection: `backend` doubles as the seam for headless tests. begin
 * installs the engine-backed callbacks when all four are zeroed, uses a
 * complete caller-supplied backend verbatim, and replaces an incomplete one
 * (any of the four NULL) with the engine defaults; a custom backend must
 * therefore provide all four callbacks. */
struct fifa96_engine;
struct fifa96_surface;

/* Match presentation slots: FU-85 §4 stages 23 entities (11 + 11 + ball) into
 * the render arrays; the engine keeps the same staging size. */
#define FIFA96_MATCH_RUN_RENDER_SLOTS 23

/* FU-158 §8.1 (phase-13 T2): the dedicated ball draw's bank capture. The
 * native presenter FUN_000572E8 draws the GAMEART0 `ball.fsh` bank (BIGF entry
 * 37; engine bank index = the PLAYART 91-slot offset + 37 = 128) with the
 * frame from the composer roll byte. `ball_bank` is `FIFA96_MATCH_RUN_BALL_
 * BANK_NONE` when the staged pitch container carries no `ball.fsh` (the
 * no-ISO/synthetic-fixture path keeps the FU-84 resolve). */
#define FIFA96_MATCH_RUN_BALL_BANK_NONE 0xFFu

/* OL-T11-7 HUD: the staged team-name seam's buffer (the native FUN_00017748
 * filters into a 10-byte local; the engine keeps a little headroom). */
#define FIFA96_MATCH_HUD_NAME_MAX 16

/* Derived period lengths in whole seconds (FU-62 §4.6: the clock compares
 * `period_seconds == [0x5881A]` for periods 0/1 and `[0x5881C]` for periods
 * 2/3, both = `[0x4C1D1]` minutes × 60/20). The default settings half-length
 * index 0 selects 2 minutes (FU-68 §4.1: table flat 0x37170 = {2,4,6,...};
 * `fifa96_settings_defaults` value[0x0E] = 0), which is what begin installs
 * for selector 0 (the menu/boot path, FU-64 §1.1). A non-zero selector takes
 * the `FUN_0004A228 -> FUN_0004B508` reset override 0x3C/0x1E (60 s / 30 s). */
#define FIFA96_MATCH_RUN_PERIOD_SECONDS_DEFAULT 120u
#define FIFA96_MATCH_RUN_EXTRA_SECONDS_DEFAULT 40u
#define FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET 60u
#define FIFA96_MATCH_RUN_EXTRA_SECONDS_RESET 30u

/* FU-143 §8/OL-84 (M2 visible-match Task 2): the derived kickoff phase entry.
 * The native selector-0 match starts at the reset phase 0 and enters the
 * kickoff-placement phase through act 1 = the phase-0x17 handler FUN_00088DC8
 * (invoked by the FUN_0008A938 situation-1 table-2 arm 0x8ABAB): its stage 0
 * (0x88E4B..0x88E87) runs FUN_000700F4 and FUN_00073E28 (phase reset) and then
 * FUN_000740A0(AL=1, side=[0x157AAC]>>24) at 0x88E82 -> [0x157A4D]=1,
 * followed by FUN_00073E08 -> FUN_0008CF60 (the placement commit that begin
 * models through fifa96_match_entities_kickoff_place). begin installs this
 * entry from the reset default: the run's phase becomes 1 (the kickoff
 * placement phase, class 0, so the FU-62 clock stops) with prev_phase 0. The
 * live in-play phase 2 is written only by the FUN_0008A938 situation-0xB arm
 * (0x8AEF6 -> 0x8AF02 FUN_000740A0(EAX=2, side)); `get_xrefs_to 0x8A938` = 39
 * and the EBX=0 situation-0xB producers are the phase-1/action bodies 0x7DF90
 * (row 01), 0x85D38 (row 0x10), 0x863F9 (row 0x11), 0x84495 (row 0x12),
 * 0x84E8F (row 0x13), the keeper/restart bodies 0x7546E/0x75B58/0x76072 and
 * the unresolved computed-situation act-8 call 0x8A8CE. The transition is
 * real on the kickoff chain: the same 0x88E82 setter call runs FUN_0008D098
 * per team (0x740C8/0x740DB), whose state-1 arm (0x8D1B1, table 0x8D040[1])
 * installs action 1 (0x8D1F1/0x8D200 CALL 0x7D9A4) / action 2 (0x8D238), and
 * action row 01 (0x7DBC0, the `phase == 1` gate at 0x7DBCB..0x7DBD6) calls
 * situation 0xB at 0x7DF90.
 *
 * M2 playable-match Task 2 landed the residual: begin runs the state-1 arm via
 * `fifa96_match_phase_machine_kickoff` (between this entry and the placement
 * commit, the native order), the wired action row 01 supplies the situation
 * 0xB call, and the derived act-1 stage-1 producer sets `mr->global_5882a` at
 * `state.tick_total >= 0x78` so a begun run reaches the live phase 2 on its
 * own (see fifa96_match_run_frame and FU-143 §11).
 *
 * M2 phase-18 T4 (FU-203): the native act-1 route is now live. `begin` no
 * longer calls this entry directly — it enters through
 * `fifa96_match_act_dispatch(mr, 1, 0, 1)` (which runs the phase-0x17 body
 * stage 0 = this entry), and the phase-`0x20` cutscene terminal re-dispatches
 * situation 1 -> act 1 -> phase `0x17` after the `0x3E8` wait. `0x110794[0x17]
 * == 0x1107EC[1] == 0x88DC8` (the phase/act dual identity; `get_xrefs_to
 * 0x88DC8` = 1). The frame-body `global_5882a` arm is retained as the derived
 * stage-1 remainder (idempotent with the phase-`0x17` stage-1 arm; leg
 * FU203-L2). */
#define FIFA96_MATCH_RUN_KICKOFF_PHASE 1u

/* Engine-side staging record: the FU-85 §4 entity triple/anim/frame/hidden plus
 * the FU-84 row +8 sprite-bank (animator) index the FU-85 resolver consumes.
 * `anim_timer`/`anim_turn` are the FU-84 `FUN_0008E008` per-frame driver state
 * (native record +0x32 accumulator and +0x46 turn sign, default +1 from the
 * selector); `stage.anim_id` holds the row id (`[rec+0x28]` byte 0) and
 * `stage.frame` the frame index (`rec+0x3D`). The scene staging advances both
 * once per granted frame and derives `bank_index` from the row's +8 byte. */
struct fifa96_match_run_entity {
  fifa96_render_entity stage;
  uint8_t bank_index;
  uint16_t anim_timer;
  int8_t anim_turn;
};

/* Task 15 presentation state, all caller-owned and reset by init/begin:
 *  - camera    FU-71 follow core (position/velocity/timer), advanced once per
 *              granted 30 Hz frame with the frame delta;
 *  - yaw/pitch FU-88 view record angles (obj[3]/obj[4]); the view matrix is
 *              `fifa96_projection_matrix(yaw, pitch)`;
 *  - window    FU-92 render window/clip + FU-93 window scale (HUD seam);
 *  - display   FU-90 display/suspend block (held; overlays not wired yet);
 *  - entities  the FU-85/89 scene staging and the FU-84/85 sprite assets the
 *              resolver reads (frame table, render banks, fixed 0x60/0x61
 *              banks, composite mirror table), plus the indexed blit remap;
 *  - enabled   opt-in flag: the Task 12-14 engine fixtures never stage a
 *              scene, so rendering stays off until a match sets it. */
/* FU-152 §4.1 (P4): the presentation residual state behind the presenter
 * rows R1/R2/R3 (the HUD row R4 is FU-148, the ball row R5 is dormant). */

enum {
  FIFA96_MATCH_REPLAY_LIVE = 0x00,      /* [0x109A98] == 0 */
  FIFA96_MATCH_REPLAY_ARMED = 0x81,     /* the armed/intro state */
  FIFA96_MATCH_REPLAY_PLAY = 0x82,
  FIFA96_MATCH_REPLAY_PAUSE = 0x83,
  FIFA96_MATCH_REPLAY_STEP_BACK = 0x84,
  FIFA96_MATCH_REPLAY_STEP_FWD = 0x85,
  FIFA96_MATCH_REPLAY_SLOW = 0x86,
};

enum {
  FIFA96_MATCH_REPLAY_ROW_NONE = 0,
  FIFA96_MATCH_REPLAY_ROW_HUD,    /* FUN_00064C34 (caption/progress/buttons) */
  FIFA96_MATCH_REPLAY_ROW_BLINK,  /* FUN_000564A0 (caption id 0x18C) */
};

/* The replay row state ([0x109A98] family). All zero at rest = live, so a
 * fresh run draws no replay row. The caption strings and the progress bar
 * assets are runtime-filled legs (FU-152 legs 1/2/3), so the engine stages
 * final text and positions and derives only the gates/step machine. */
struct fifa96_match_run_replay {
  uint8_t state;          /* [0x109A98] */
  uint8_t hud_armed;      /* [0x109AC4] */
  uint8_t camera_index;   /* [0x109A8C] 0..5 */
  uint8_t camera_kind;    /* FUN_0004D134 selected record (FIFA96_CAMERA_REPLAY_*) */
  uint8_t camera_sub;     /* the FUN_0004DDA8 sub index 5..8 (view modes) */
  uint32_t record_index;  /* [0x107DDC]: the last FUN_0004D134 index (any value) */
  uint32_t cursor;        /* [0x109A94] */
  uint32_t cursor_limit;  /* [0x109AC0] */
  int32_t phase_counter;  /* [0x14E58C] (FUN_00053D7C) */
  int32_t blink;          /* [0x108FC4] */
  uint32_t wait_timer;    /* [0x109A9C] */
  uint32_t wait_reload;   /* [0x109AA0] */
  int32_t advance;        /* [0x109AA8] */
  int32_t caption_x;      /* staged engine caption cell (native FUN_00012FE4 leg) */
  int32_t caption_y;
  char captions[6][FIFA96_MATCH_HUD_NAME_MAX];  /* [0x109AD0..0x109AD8] leg 1 */
};

/* The R2 row gate (fresh 0x565BC decompile + the predicate bodies):
 * family ([0x109A98] & 0x80), not the armed state (0x81), outside the ramp
 * window ([0x14E58C]-0xF1 < 0x78 with [0x14E58C] != 0), HUD arm for the
 * FUN_00064C34 row. */
int fifa96_match_run_replay_row(const struct fifa96_match_run *mr);

int fifa96_match_run_replay_family(const struct fifa96_match_run_replay *replay);
int fifa96_match_run_replay_live(const struct fifa96_match_run_replay *replay);
int fifa96_match_run_replay_armed(const struct fifa96_match_run_replay *replay);
int fifa96_match_run_replay_playing(const struct fifa96_match_run_replay *replay);
int fifa96_match_run_replay_ramp_window(const struct fifa96_match_run_replay *replay);

/* FUN_000642B0: `cursor * 100 / cursor_limit` (0 when the limit is 0). */
uint32_t fifa96_match_run_replay_progress(const struct fifa96_match_run_replay *replay);

/* FUN_000564A0: advance the blink counter by `tick`; returns 1 when the
 * caption draws (counts 0..9, or the >= 0x14 reset frame), 0 in 10..19. */
int fifa96_match_run_replay_blink_step(struct fifa96_match_run_replay *replay,
                                       int32_t tick);

/* The reachable FUN_000642FC subset: the 0x80 -> 0x81 promotion, the 0x81 arm
 * (reset cursor/camera + selector 0), the button mapping (1 toggles
 * play/pause, 0x20 cycles the camera mod 6, 8/4/2 -> step modes), the
 * single-frame step-mode reset and the exit arm (bit 0x80 -> state 0, HUD
 * arm cleared). The replay-ring advance (FUN_00063D34/CBC/6428C), the held
 * pad probe FUN_000451F1 and the pan keys FUN_0004CA08 stay legs. */
int fifa96_match_run_replay_step(struct fifa96_match_run *mr, uint32_t buttons,
                                 int32_t delta);

/* The exit chain FUN_00064E8C's reachable state: [0x109A98] = 0 and the HUD
 * arm freed. The hero/selection release and audio sinks stay legs. */
void fifa96_match_run_replay_exit(struct fifa96_match_run *mr);

/* FUN_0004D134: select replay camera `index` (0..6; the native default only
 * records the index). The engine always stores the `[0x107DDC]` cell
 * (`record_index`) and keeps the current `{kind, sub}` selection on the
 * default. Returns 1 on a selected camera, 0 for the default,
 * -FIFA96_ERR_INVALID (NULL). */
int fifa96_match_run_replay_camera_set(struct fifa96_match_run *mr, uint32_t index);

/* The R1 substitution strip (FU-152 §2.6/§3.2). `record` is the byte block
 * behind the native [0x1587D4] pointer (the field meanings stay FU-145 L4);
 * `cursor_raw` is [0x1587E7] (k = % 5), `flags` the [0x1587DA + i*2 + side]
 * cells, `record_side` the (*record + 0x826) byte. Marks come from
 * FUN_0004BD38, the two displayed numbers from FUN_0004BDF8. The mark/name
 * blit inputs and the name clamp cells are staged (legs 13/14). */
struct fifa96_match_run_sub {
  uint8_t active;
  uint8_t cursor_raw;      /* [0x1587E7] */
  uint8_t current_side;    /* [0x1587E3] >> 24 */
  uint8_t record_side;     /* (*[0x1587D4] + 0x826) */
  uint8_t mode;            /* [0x157A4A] >> 24 */
  uint8_t special;         /* [0x1587D4] == 0x157A9F */
  uint8_t wide;            /* FUN_00044BE0 (settings 4) */
  uint8_t flags[10];       /* [0x1587DA + i*2 + side] */
  uint8_t record[0x12];    /* the [0x1587D4..0x1587E5] bytes FUN_0004BDF8 reads */
  uint16_t frame5_height;  /* Frames.fsh frame-5 height ([0x14E63A] family) */
  uint16_t name_width;     /* the staged measured name width ([0x14E63A]) */
  int32_t x_clamp_lo, x_clamp_hi;  /* [0x108DE4]/[0x108DE8] name clamp */
  int32_t mark_pitch;      /* staged glyph pitch (leg 13) */
  int32_t mark_step;       /* staged state glyph step (leg 13) */
  const struct fifa96_sprite_frame *mark_sprite;  /* staged strip glyph */
  char name[2][FIFA96_MATCH_HUD_NAME_MAX];
};

int fifa96_match_run_sub_mark(const struct fifa96_match_run_sub *sub, int side,
                              int slot);
int fifa96_match_run_sub_numbers(const struct fifa96_match_run_sub *sub, int *p1,
                                 int *p2);

/* The R3 overlay screen (FU-152 §2.7/§3.2). The case table maps the id to the
 * draw path FUN_000550E4's switch takes; the line strings/positions are staged
 * (the per-case helpers are legs). `extra_time` is the native extra-time mode
 * gate (its producer is unported), `second` the [0x14E5C8] overlay. */
enum {
  FIFA96_OVERLAY_ROW_NONE = 0,
  FIFA96_OVERLAY_ROW_PERIOD,         /* case 0 */
  FIFA96_OVERLAY_ROW_PLAYER_LIST,    /* 1,4 */
  FIFA96_OVERLAY_ROW_SUBSTITUTION,   /* 5,0xB,0xE */
  FIFA96_OVERLAY_ROW_RECORD_INFO,    /* 6 */
  FIFA96_OVERLAY_ROW_PERIOD_STRING,  /* 7,0xC */
  FIFA96_OVERLAY_ROW_LIST,           /* 8 */
  FIFA96_OVERLAY_ROW_MESSAGE,        /* 9 */
  FIFA96_OVERLAY_ROW_LIST_MESSAGE,   /* 0xA */
  FIFA96_OVERLAY_ROW_STATS,          /* 0xD */
  FIFA96_OVERLAY_ROW_PLAYER_NAME,    /* 0xF */
  FIFA96_OVERLAY_ROW_EXTRA_TIME,     /* 0x11 */
};

struct fifa96_match_run_overlay {
  uint16_t id;             /* [0x14E674] & 0xFF */
  uint8_t armed;           /* [0x14E674] & 0x8000 */
  uint8_t second;          /* [0x14E5C8] */
  uint8_t extra_time;      /* native extra-time mode (producer unported) */
  int32_t timer;           /* [0x14E684] */
  int32_t timeout;         /* [0x14E680] */
  int32_t direction;       /* overlay +0x24 (FUN_00053E08) */
  int32_t rate;            /* overlay +0x28 */
  int32_t second_timer;    /* [0x14E5D8] */
  int32_t second_timeout;  /* [0x14E5D4] */
  int32_t second_direction;
  int32_t second_rate;
  int32_t line_x[4];       /* staged layout (per-case helpers are legs) */
  int32_t line_y[4];
  char lines[4][FIFA96_MATCH_HUD_NAME_MAX];
};

int fifa96_match_run_overlay_row(uint16_t id);
int fifa96_match_run_overlay_arm(struct fifa96_match_run_overlay *overlay, uint16_t id);
int fifa96_match_run_overlay_timeout_step(struct fifa96_match_run_overlay *overlay,
                                          int32_t tick);
/* The R3 row gate: armed, not suspended, no replay family, and the extra-time
 * id exclusion (only 0x11 draws in extra time). */
int fifa96_match_run_overlay_visible(const struct fifa96_match_run *mr);

/* FU-152 §2.5 leg 6: the R5 ball row is dormant (the only writer FUN_00056690
 * is always called with 0); the engine keeps `render.ball_row` NULL and the
 * predicate reports the reachability of the staged producer. */
int fifa96_match_run_ball_row_reachable(const struct fifa96_match_run *mr);

struct fifa96_match_run_render {
  int enabled;
  struct fifa96_camera camera;
  int32_t yaw, pitch;
  int view_class;
  int input_bit2;
  /* FU-145 S2 (0x718FF): the boundary reflect arm's input bit 0 — the native
   * tests `(word[0x14C1D4] | word[0x14C1D6]) & 1` (the per-side range words
   * FU-139 reads; image-zero). The runtime producer path is landed (phase-10
   * T2: the front-end options editor FUN_0001C728 -> the eight config cells ->
   * `fifa96_input_range_words`; the engine derives this bit in
   * `fifa96_match_run_set_input_options`). No front-end staging runs on the
   * tape, so the cell keeps the caller-staged default 0, mirroring
   * `input_bit2` (the sibling bit 1 the FU-71 update reads). */
  int input_bit0;
  struct fifa96_window window;
  fifa96_match_display display;
  int32_t window_scale_x, window_scale_y;   /* FU-93 16.16 zoom scale */
  int window_zoomed;
  /* FU-89 §6: the near-depth threshold `[0x54350]` divides by the camera
   * record's +0x4C ratio dword (first-hand read_memory 0x107554 = `00 15 00
   * 00` = 0x1500; the FU-96/FU-97 "21" is the +0x4D byte; the writer 0x4D836
   * stores the full dword from the camera-type entry[5]). The camera-type
   * setup (FUN_0004D7E8) is unported, so the engine carries the static
   * default (open leg). */
  int32_t view_ratio;
  /* FU-148 §2.1(a)/§6.2 (S4): the FUN_000505D0 pose-feed staging. view_mode
   * is the native [0x14E57C] (getter FUN_00053D50; the 0x51xxx view-handler
   * writers are unported, FU-148 leg 8) and zero selects the native default
   * arm (the handler call — leg). The other args mirror the camera-record /
   * replay cells (FUN_0004D2D4's gate, the [0x109A70] replay record and the
   * FUN_0004B818 side mirror). Zero-initialized: a fresh match applies no
   * pose until a producer lands (the tape stays byte-identical). */
  fifa96_camera_pose_args camera_pose;
  /* FU-148 §4.2 (S4): the translation-pool seam. The native pool (FUN_00049138
   * -> FUN_00046F80) is a distinct allocation whose content producer is leg 11;
   * a caller may stage one by partitioning a buffer through
   * fifa96_palette_pool_partition. Until then the render keeps the identity
   * remap. */
  fifa96_palette_pool palette_pool;
  uint8_t background;
  struct fifa96_match_run_entity entities[FIFA96_MATCH_RUN_RENDER_SLOTS];
  uint32_t entity_count;
  const uint8_t *frames;                    /* FU-84 frame records (5 B each) */
  const struct fifa96_render_bank *banks;
  uint32_t bank_count;
  /* FU-158 §8.1/§8.2 (phase-13 T2): the ball slot's dedicated art state. The
   * native composer maintains the byte [0x155C4E]: the low nibble is the
   * GAMEART0 `ball.fsh` frame index, advanced +1 per frame while the camera
   * rate words 0x1577C0/0x1577C2 are non-zero (the engine stand-in is ball
   * motion — FU-158 L158.2), and the presenter sanitizes a frame >= the bank
   * count to 0 (0x5739D), clearing the low nibble. `ball_bank` is captured by
   * BIGF name at staging (0xFF = absent). */
  uint8_t ball_bank;
  uint8_t ball_roll;
  const struct fifa96_render_bank *fixed60;
  const struct fifa96_render_bank *fixed61;
  const uint8_t *mirror;                    /* FU-85 §1.3 0x10F2E7 table */
  const uint8_t *sprite_data;               /* backing blob for frame parse */
  uint32_t sprite_data_len;
  uint8_t remap[256];                       /* indexed translation + 0xFF key */
  /* OL-T11-6 (M2 playable-match Task 1): the derived native match palette in
   * the engine's 8-bit RGB form. Staged from the pitch container's PALsys.fsh
   * entry (frame 2 type-0x22 chunk, kit remap + appends, native `v << 2`);
   * installed onto the target surface by fifa96_match_run_palette_install,
   * which fifa96_match_run_render runs before the plane conversion. */
  uint8_t palette[768];
  int palette_ready;                        /* a match palette is staged */
  /* OL-T11-7 (FU-148 §1.5/§6.1): the HUD assets. `hud_font[0]` stages
   * clockfnt.fsh (native resource slot 0x35, the full window's font) and
   * `hud_font[1]` playfnt.fsh (slot 0x36, the zoomed one), both by BIGF name
   * from the pitch container (the native FUN_0004AFB8(0x35/0x36) selection
   * keyed on `[0x108DDC] < 0x10000`). `hud_bar` is Frames.fsh frame 13 (the
   * drawn background panel: FUN_00053930 copies the bank's frames to
   * 0x14E624 and the HUD reads frame 13) and `hud_bar_height` frame 4's
   * height (the layout's 0x14E634 anchor). `hud_name` is the team-name stage
   * seam: the native name source (FUN_00011BEC over the team block) is
   * unported (leg OL-T11-72), so a begun run carries empty names and the HUD
   * draws the name pass only when a caller stages one. */
  struct fifa96_font hud_font[2];
  uint8_t hud_font_ready[2];
  struct fifa96_sprite_frame hud_bar;
  uint16_t hud_bar_height;
  uint8_t hud_bar_ready;
  char hud_name[2][FIFA96_MATCH_HUD_NAME_MAX];
  /* FU-152 §4.1 (P4): the residual presenter rows. All zero at rest, so a
   * fresh/fixtured run draws exactly the FU-148 HUD (the M2 tape is
   * unchanged). `ball_row` is the dormant R5 seam (FU-152 §2.5). */
  struct fifa96_match_run_replay replay;
  struct fifa96_match_run_sub sub;
  struct fifa96_match_run_overlay overlay;
  const struct fifa96_sprite_frame *ball_row;
};

/* Minimal derived match record (M2 Task 5 / FU-138 §4, extended by M2 Task 7 /
 * FU-140 §4 and M2 Task 8 / FU-141): the native record is a 0xB2-strided block
 * (FU-137 §3) whose fields the FU-76 §3.1 action-00 body and the FU-79 §7
 * keeper row 1E read and write. The engine binds these fields to the entity
 * pool (`mr->entities`, FU-141): the FU-67 update chain copies one pool record
 * into this staging record, dispatches the record's action code, then drains
 * the requests back into the pool. Offsets are the native record fields: pos
 * +0x59/+0x5D/+0x61, target +0x4D/+0x51/+0x55, timer +0x89/+0x81, active
 * +0x8D, ran +0x9E, stage +0x8F, ball flag +0x9B, control-slot pointer +0x20,
 * slot direction +0x1D/+0x1E -> +0x20/+0x21. `install` is the derived seam
 * request for the FU-137 §2 `FUN_0007D9A4` install, consumed by
 * `fifa96_match_entities_update`; `ran` is set by action 00 and cleared by the
 * same installer drain. `place_x/y/z` + `place_valid` are the row-1E
 * `FUN_000700F4` placement request (`0x15774C/50/54`), consumed by the frame
 * body as the FU-71 `fifa96_camera_init` reset; `helper_request` is the
 * `FUN_0007876C` slot-merge request, consumed by the FU-141 pool merge. FU-142b
 * adds the row-26 fields `stage92` (native +0x92, the 0/1/2 latch), `timer7b`
 * (+0x7B), `lane` (+0x69 dz word) and the `player_d`/`player_e` stand-ins for
 * the native `rec[+4]` descriptor bytes +0xD/+0xE (the roster descriptor is
 * unmodeled; the pool path stages 0). FU-142e adds `distance` (+0x65): the
 * unported FUN_0008D098 pre-switch walk (`0x8D11E`) writes the 0x8DCD4 out
 * triple for every free record before the installer arms, so the frame staging
 * recomputes the word from this dispatch's pos/target and row 2A gates on it. */
struct fifa96_match_run_record {
  int32_t pos_x;
  int32_t pos_y;       /* native +0x5D, FU-140 row 1E placement height */
  int32_t pos_z;
  int32_t target_x;
  int32_t target_y;     /* native +0x51 (row-28 target = pos copies) */
  int32_t target_z;
  /* FU-153 / M2 phase-12 T1 (15.a): the FUN_0007BF20 jump state — word +0x85
   * (the arm-staged ramp time = the arc peak) and word +0x87 (the running
   * time). The 0x7C057 `+0x85 != 0` gate guards the `0x7C072..0x7C113` arc
   * block staged right after the mover; the landing edge zeroes both and
   * copies the position into the target. */
  int16_t jump_base;   /* native +0x85 */
  int16_t jump_timer;  /* native +0x87 */
  /* FU302 (phase-28 T7): the FUN_00076B28 steering extent `word[+0x83]`
   * (`0x76B3C..0x76B6A`). The same native cell the jump arms write; in this
   * ported path it is written and read only inside the steering call (the
   * unported FUN_0006E8E8 event gates are its other consumers, FU180-L1),
   * so it is reset at every record staging. */
  int16_t steer83;
  int32_t place_x;     /* FU-140 row 1E: native 0x15774C */
  int32_t place_y;     /* native 0x157750 */
  int32_t place_z;     /* native 0x157754 */
  int32_t timer89;
  uint16_t timer81;
  uint16_t delta;      /* FU-62 frame delta, native [0x157A64] */
  uint8_t active;
  uint8_t has_slot;
  uint8_t has_ball;         /* native +0x9B (FU-140 row 1E) */
  uint8_t helper_request;   /* FU-140 row 1E: FUN_0007876C slot-merge request */
  uint8_t controlled;       /* FU-140 row 1E: native [0x157A83] = rec */
  uint8_t stage;            /* native +0x8F stage byte (FU-140 row 1E) */
  uint8_t stage92;          /* native +0x92 stage latch (FU-142b row 26) */
  uint16_t timer7b;         /* native +0x7B (FU-142b row 26) */
  int32_t lane;             /* native +0x69 dz word, sign-extended (FU-142b) */
  /* FU311-L5 (phase-30 T2): the high word of the native packed `dword[+0x69]`
   * — the `+0x6B` lane word. The native reads it as `dword[+0x69] >> 16`
   * (`0x78032`, `0x81519`, `0x7DCCC`, ...); the port keeps it as its own staged
   * word (`lane` is the low dz word), so handlers reading the high word use
   * this cell. Staged from the pool record and never written back (the native
   * `+0x6B` has no writer in the ported engine path). */
  int32_t lane_x;           /* native word +0x6B (the +0x69 dword's high word) */
  int32_t distance;         /* native +0x65 0x8DCD4 out[0] word (FU-142e row 2A) */
  int8_t player_d;          /* rec[+4][+0xD] table index (FU-142b row 26) */
  int8_t player_e;          /* rec[+4][+0xE] stage-0 gate (FU-142b row 26) */
  /* FU301 (phase-28 T6): the roster descriptor `[rec+4]` bytes the row-0x1B
   * wide path reads — `desc_c` = `byte[desc+0xC]` (its `SAR 0x19` ratio seeds
   * the extents and Block A's `ECX += ratio`; the whole int8 via `SAR 0x18` is
   * Block A's `0x10-x` difficulty-0 adjust), `desc_f` = `byte[desc+0xF]` (the
   * Block E reduce threshold, loaded as `dword[desc+0xC]` and `SAR 0x18`),
   * `desc_1b` = `int8 byte[desc+0x1B]` (the Block E `C0 ± b` rate jitter).
   * The `[rec+4]` binding is the FU167/FU168/FU176/FU204 leg class and the
   * producer is unlocated (FU301-L3), so staging resets them to zero (the
   * engine's honest default). */
  int8_t desc_c;            /* rec[+4][+0xC] */
  int8_t desc_f;            /* rec[+4][+0xF] */
  int8_t desc_1b;           /* rec[+4][+0x1B] */
  int8_t dir_x;
  int8_t dir_z;
  /* FU-77 `FUN_0007BF20` shared-mover state (M2 interactive Task 1): the
   * native record fields the per-frame locomotion integrator reads and writes
   * (`face7d` = +0x7D facing, `speed71` = +0x71 speed metric, `vel73`/`vel75` =
   * +0x73/+0x75 velocity words, `body_timer9c` = +0x9C stride accumulator).
   * The engine runs the mover for the slot-bound (controlled) record first;
   * the AI-side integration stays a numbered leg. */
  int16_t face7d;
  int16_t speed71;
  int16_t vel73, vel75;
  uint8_t body_timer9c;
  int8_t place_offset_x;    /* caller-supplied 0x10F334[type8] (FU-140) */
  int8_t place_offset_z;    /* caller-supplied 0x10F33C[type8] (FU-140) */
  uint8_t place_valid;      /* row 1E: the placement triple is live (FU-141) */
  uint8_t ran;         /* native +0x9E, set by the action-00 body */
  /* FU-139 §9 (Task 11): the current pool record identity, staged so rows
   * 07/0F can resolve the team candidates and the kick direction table. */
  int32_t entity_id;   /* team*11 + index, or NONE */
  uint8_t row44;       /* native +0x44 animation/event ack byte (FU-151: the
                        * row-1E stage 0/2/8 and row-1D stage 0/4 gates read
                        * it; FU-159/T3: produced from the current anim row's
                        * flags bit 0 at the dispatch staging) */
  uint16_t aux_word;   /* native +0x36: the resolved frame's aux word (the
                        * FUN_0006E8E8 one-shot model, FU-161 §5.4) */
  uint8_t frame_freeze;/* aux-0x2B frame-timer freeze request (FU-161 §5.2) */
  uint8_t code;        /* native +0x91 (the byte the installer writes and the
                        * 0x110680/0x7E600/0x7C990 gates index; staged from the
                        * pool entity so handlers never use the face octant) */
  uint8_t install;     /* derived install request of the last dispatch, 0 = none */
  uint8_t anim_id;     /* OL-80 live animation row id: native byte[[rec+0x28]],
                        * the value the FU-84 selector stores and the arm bodies
                        * pass as `anim_sel` to `fifa96_arm_anim_select`; staged
                        * from the pool and written back after the dispatch. */
  uint8_t frame;       /* native +0x3D animation frame index (row-08 scan gate;
                        * OL-80: staged live from the pool record, whose scene
                        * staging advances it through the FU-84 driver model) */
  /* FU-142d (row 28, Appendix G) staging: the native record +0x8E facing byte
   * and +0x71/+0x73 velocity pair the body writes, the derived scratch gates
   * (native +0xA0..+0xAE, carried by the pool), the team +0x830 flag, the
   * resolved [team+0x831] chosen-record position and the five process globals
   * the body reads (their native producers are unported, OL-56). */
  int32_t vel_x, vel_z;     /* native +0x71/+0x73 (arm-2 zero writes) */
  uint8_t type;             /* native +0x8E byte: the one face octant the
                             * 0x79C50 rows (28/2A/0F/08) write and every
                             * +0x8E reader consumes (OL-83 unified) */
  uint8_t side;             /* team +0x826 side (arm-0 negation gate) */
  uint8_t flag830;          /* team +0x830 (arm-1 gate) */
  uint8_t chosen_ok;        /* derived: team+0x831 resolved to a pool record */
  int32_t chosen_x;         /* resolved [team+0x831]+0x59 x */
  int32_t chosen_y;         /* resolved [team+0x831]+0x5D y */
  int32_t chosen_z;         /* resolved [team+0x831]+0x61 z */
  int32_t scratch_a2;       /* native +0xA2 derived gate A */
  int32_t scratch_a6;       /* native +0xA6 derived gate B */
  int32_t scratch_aa;       /* native +0xAA approach timer A */
  int32_t scratch_ae;       /* native +0xAE approach timer B */
  uint8_t scratch_a0;       /* native +0xA0 RNG bit */
  uint8_t scratch_a1;       /* native +0xA1 RNG bit */
  int32_t global_10f358;    /* native [0x10F358] arm-2 re-roll gate */
  int32_t global_10f35c;    /* native [0x10F35C] arm-2 chase flag */
  int32_t global_10f364;    /* native [0x10F364] arm-0 set-piece x */
  int32_t global_10f368;    /* native [0x10F368] arm-0 set-piece z */
  uint8_t global_157ac2;    /* native [0x157AC2] arm-0 mode byte */
};

/* FU-184 §4 (phase-16 T2): the row-0x14 celebration scatter state — the native
 * `[0x158720]` scatter index and the 6-point scatter buffer `[0x1586D8..]`
 * (the five `fifa96_action_sequence_scatter_celebration` points plus the
 * native's pre-clamp points[4] copy at `[0x158714]`, FU184-L1). It persists
 * across the stage-0 -> stage-1 frames of the row machine; init and begin zero
 * it, and the row's stage-2 resets `idx` (native `0x87C43`). */
struct fifa96_match_run_celebration {
  int32_t idx;                    /* native [0x158720] */
  fifa96_action_vec3 points[6];    /* native [0x1586D8..0x158714] */
};

struct fifa96_match_run {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct fifa96_match_state state;               /* match clock/period block */
  uint16_t score[2];                             /* per-side goal words (FU-72 §2.4) */
  /* C3-OL2 (M2 playability Task 4): the FUN_00093944 writer's state cells —
   * native [0x15B670] last scoring side, [0x15B6B4] tracked goal-difference
   * side (-1 = the plain-increment sentinel) and [0x15B6A4] max goal
   * difference — plus the id the latest score event posted to FUN_0009252C
   * (0 = none), the engine's observability seam. The native producers
   * (FUN_00092D8C's
   * tracked-side pick from the unported team+0x828 flags at 0x1590CC/0x159901
   * and FUN_00092E2C's resets) are unported (OL-87), so init/begin install the
   * carried defaults: last_side/tracked_side -1, max_diff 0. */
  int32_t score_last_side;
  int32_t score_tracked_side;
  int32_t score_max_diff;
  uint8_t score_last_event;
  /* M2 phase-9 T3 (FU-146 §3 / FU-148 §12.2): the tracked-side pick and the
   * input/range words. FUN_00092D8C's head FUN_00078824 zeroes the team flags
   * [0x1590CC]/[0x159901] (the sole writers; no other exists) before the pick
   * reads them, and the [0x15B684] game-mode dword switches the pick to the
   * installer's side argument (its producers are the unported front-end
   * FUN_00038630 arms; the engine seeds 0). With the flags zero the pick is 1
   * (image default), so the natural goal's untracked arm posts the probe id
   * (0xD3). FUN_0001C9BC (match init FUN_00011B7C 0x11BA6) builds the
   * per-side range words [0x14C1D4]/[0x14C1D6] from the eight config dwords
   * 0x105278/7C/88/8C/90/9C/A0/A4; the engine stages those cells (BSS 0 by
   * default) and derives `(w0|w1) & 4` as the pan-step walk gate. M2 phase-10
   * T2: the producer path `fifa96_match_run_set_input_options` stages the
   * eight cells from the native 13-entry front-end options block
   * (FUN_0001C728 / FUN_0001CAEC) and rebuilds the words. */
  uint8_t tracked_flag[2];
  uint8_t screen_record_mode;
  int32_t input_cfg[8];
  uint16_t input_range[2];
  /* T4 (OL-89): the FUN_0009252C display boundary. `score_sound_device` and
   * `score_sound_midi` are the native gate cells [0x115FCC]/[0x114A98]
   * (image 0/0; their FUN_000A7FD4/FUN_000A8172 and FUN_000A8103/A810B
   * producers are the unported sound-device legs), and `score_display_event`
   * is the id the latest score event dispatched through the gate (0 = none;
   * the FUN_00066724 text/audio sink stays a leg). */
  uint8_t score_sound_device;
  uint8_t score_sound_midi;
  uint8_t score_display_event;
  /* FU-143 wiring (M2 playability Task 3): the FU-62 second-rollover
   * completion (`period_seconds == limit + aux_seconds`) for the latest tick,
   * staged by fifa96_match_run_frame from fifa96_match_state_tick's
   * `period_ended` output and consumed by fifa96_match_run_phase_drive. The
   * derived test itself lives in the FU-62 library
   * (src/fifa96_loader/fifa96_match_state.c completion block); the plan's
   * driver signature takes the run only, so the clock's result flows through
   * this field. Reset by init/begin, cleared by the driver (one-shot). */
  uint8_t clock_period_ended;
  struct fifa96_engine *engine;                  /* engine holding this run */
  struct fifa96_match_lifecycle_backend backend; /* engine callbacks or stub */
  uint32_t ticks;                                /* 100 Hz match callback hits */
  uint32_t steps;                                /* run steps since begin */
  /* FU313/FU309-L5 (phase-30 T1): the default-off camera-cell trace. The sink
   * is opened by `begin` only when FIFA96_CAMERA_TRACE names a path (empty or
   * unset = NULL, the inert default); one line per granted frame is written at
   * the frame-body loop tail. The frame index is run-relative and resets at
   * begin. Closed idempotently by end/teardown. */
  FILE *cam_trace;                               /* the FIFA96_CAMERA_TRACE sink (NULL = off) */
  uint32_t cam_trace_frame;                      /* the run-relative granted-frame index */
  int running;                                   /* begin/end balance */
  struct fifa96_input input;                     /* FU-61 player-0 edge/held model */
  uint8_t input_state[FIFA96_INPUT_PLAYERS];     /* last sampled FU-61 state (slot input) */
  fifa96_control_slot slot;                      /* FU-70 slot bound to player 0 */
  struct fifa96_match_run_record record;         /* FU-141 dispatch staging record */
  struct fifa96_match_run_celebration celebration; /* FU-184 row-0x14 scatter state */
  struct fifa96_match_entities entities;         /* FU-141 entity/ball pool */
  struct fifa96_match_phase_machine phase_machine; /* FU-142a installer-arms machine */
  /* FU-142d: the match RNG (seeded by begin, zerod by init; the native seeds at
   * match init FUN_000493A0/0x493F2 with settings[0x18] via FUN_0001D940, whose
   * value/producer is unported — the derived seed is 0, OL-56) and the five
   * row-28 process globals, staged to zero until their native producers (the
   * FUN_0008D098 entry block/row 2A/installer clear) are modelled (OL-56). */
  struct fifa96_rng rng;
  int32_t global_10f358;
  int32_t global_10f35c;
  int32_t global_10f364;
  int32_t global_10f368;
  uint8_t global_157ac2;
  /* FU-213 §2.4/§2.5 (M2 phase-19 T1): the OL-85 extra-time flag, the native
   * `[0x157AC0]` boolean byte (FU-207 §1). The live clock's period completion
   * raises it when the completed regulation (W2 0x8B2AC) or extra-time-2 (W4
   * 0x8B3A2) period ends with DIFFERING scores, and per-frame mid-extra-time
   * while the period index is 2 or 3 (W5 0x8B5CD). The chooser
   * `fifa96_action_phase_period_end` reads it to select the extra-time arm
   * (act 0xC / phase 0x13-0x14) over the no-extra-time arm (act 0xB / phase
   * 0x0C). Staged 0 (the `[0x157AC0]` BSS default); the front-end writer W6
   * (0x389A4) and the equal-score override W3 (`[0x14C2EE]`, no engine cell)
   * stay unported legs (FU213-L1/L7). This is T1's exposed flag for T2's
   * situation-8 gate / the act-0xC-0xB route. */
  uint8_t global_157ac0;
  /* FU-231 §5 / FU-236 §1.1 (M2 phase-22 T2): the native end word
   * `[0x158822]` (a 16-bit word; FU234-L2: the harness return `0x8B678 MOV
   * AX,word[0x158822]`). Set by the ported `FUN_0008BAF0` end machine to `1`
   * (regulation-over, the flag-clear phase-0xC terminal `0x8BCD9`) or `2`
   * (extra-time-over, the phase-0x13/0x14 dwell `0x8BD8A`/`0x8BDD4`); `0` =
   * none. The native match reset `FUN_000886D4` clears it (`0x8871B`), so
   * init/begin stage `0`. The no-ET screen arm `0x37F54` (`[0x105FFC]=5`) has
   * no engine cell (FU-231 §5 / leg FU231-L2). */
  uint16_t end_word_158822;
  /* FU-210 §6.1/§8 L3 + FU-217 §3 (M2 phase-19 T3 → phase-20 T3): the native
   * actor-record pointer `[0x1587D4]` the phase-0x1D arm-0 coupled installer
   * (`0x899A2`) targets — `FUN_0007D9A4(rec = [0x1587D4], code = 0x20, stage =
   * 0, invoke = 0)`. The native producer is the sit-8 arm's `[0x1587CC +
   * side*4] -> [0x1587D4]` bind (`0x8AE4B`), which now LANDS LIVE (T3: the
   * `match_run_free_record` selector + the `set_piece` sit-8 re-init; FU210-L3 /
   * FU217-L1 resolve). It holds the engine pool id (`team*RECORDS + index`) or
   * -1 (FIFA96_MATCH_ENTITY_NONE); init/begin stage it NONE. On the live path
   * the sit-8 re-init binds a real free outfield record, so the arm-0 install
   * fires and action row 0x20 is COUNTED. */
  int32_t actor_1587d4;
  /* FU-217 §3.2/§8 (M2 phase-20 T3): the per-side source table
   * `[0x1587CC]` (side 0) / `[0x1587D0]` (side 1) the sit-8 arm's bind
   * (`0x8AE30`/`0x8AE3F` `FUN_000897F4(side, 0xA)`) writes, and from which
   * `[0x1587D4]` is copied (`0x8AE4B`/`0x8AE54`). Each holds the pool id
   * (`team*RECORDS + index`) or FIFA96_MATCH_ENTITY_NONE. Write-only until the
   * arm-8 no-winner rebind (`0x89DCF`, leg FU217-L2) lands. */
  int32_t actor_1587cc[2];
  /* FU-151 P3 (keeper machines): the process cells the row-1E/1D machines
   * carry across frames — the 0x15774C/50/54 camera focus (the FU-147 leg-13
   * render-camera stand-in for X/Z: the engine keeps its own derived cells so
   * the machine's hold-follow writes do not move the real FU-71 camera X/Z),
   * the 0x157A77 reset triple (the [0x10F328] constant = (0,0,0), FU-140), the
   * 0x157C30 `{band,dx,dz}` staging vector, the 0x157C36 saved point, the
   * 0x157C42 travelled gauge, the [0x157AB2] latch and the stage-6
   * [0x157820]/[0x157822] animation flags (their native consumers outside
   * row 1E stay FU-151 leg 15). Init/begin zero them.
   *
   * FU307-L4 (phase-29 T3): `dword[0x157750]` has NO stand-in here — it is one
   * native cell and the engine keeps exactly one: `render.camera.pos_y`
   * (`fifa96_match_handlers.c` stages it into the keeper machines and applies
   * the row-1E raw-store cuts back to it). */
  int32_t keeper_cam_x, keeper_cam_z;
  int32_t keeper_reset_x, keeper_reset_y, keeper_reset_z;
  int16_t keeper_vec_band, keeper_vec_dx, keeper_vec_dz;
  int16_t keeper_saved_x, keeper_saved_z;
  int16_t keeper_gauge;
  uint8_t keeper_latch_157ab2;
  uint8_t flag_157820;
  uint8_t flag_157822;
  /* M2 phase-15 T2 (FU-176/FU-179/FU-180): the keeper-cluster process cells.
   * `global_157a4c` = native `[0x157A4C]` — the row-0x1F stage-3 reset latch
   * (aliased `[0x157A49]>>24`, the byte the `FUN_00076130` human-phase gate
   * reads); `global_157c5c` = native `[0x157C5C]` (row-0x1F prologue clear and
   * the input handler's `clear_c5c`); `global_157c5d` = native `[0x157C5D]`
   * (row-0x19 CPU-keeper latch, write-only); `keeper_latch[8]` = native
   * `[0x157C54..0x157C5B]` (row-0x1B stage-0 clears, stage-2 `[0x157C55]`
   * read, FU180-L7); `predict_c44` = `[0x157C44]` and `predict_c48_x/y/z` =
   * `[0x157C48..0x157C53]` (the row-0x19 predictor latches). The other
   * keeper-cluster cells' native producers are unported (the
   * FU176-L2/FU179-L1/FU180 L-class legs); init and begin stage them zero.
   * `global_157a4c` is the exception: its live producer is the sit-8 re-init
   * (`mr->global_157a4c = 1`; M2 phase-20 T3 / FU-217,
   * src/fifa96_engine/fifa96_match_run.c:3166). */
  uint8_t global_157a4c;
  uint8_t global_157c5c;
  uint8_t global_157c5d;
  uint8_t keeper_latch[8];
  /* FU301 (phase-28 T6): the native difficulty/option-9 dword `[0x14C2F6]`
   * the row-0x1B wide seed and Block A/C read (`==1` / `==0` / `2 - x`). The
   * producers are `FUN_0003749C`/`FUN_00038630`, both `= FUN_0001D940(9)` =
   * `dword[0x149278+0x24]`, and the static table is all-zero (fresh
   * `read_memory 0x149278`), so the image/live default is **0**. The option-9
   * front-end writer family is unported (FU301-L2), so this is a staged input
   * (tests write it; init/begin reset it to 0). */
  int32_t difficulty_14c2f6;
  /* FU300 (phase-28 T4): the row-0x1A process cells the full body consumes.
   * `global_157c2c` = the native dword `[0x157C2C]` direction stamp — the
   * keeper ladder arms write 0x2C/0x2D/0x2E in-frame (the seam publishes
   * `stamp_157c2c`, G2) and the row's direction machine rewrites it 0x5C/0x54;
   * every native write is a dword immediate with a zero high word, so the
   * modeled domain is one byte.
   * The direction-machine gate words are the LIVE camera cells (fix wave):
   * `[0x1577EE]>>16` = `camera.event_param`, `[0x1577F8]>>16` =
   * `camera.timer`, `[0x1577BC]>>16` = `camera.speed`, and the lead words
   * `word[0x1577C0]/[0x1577C2]` = `camera.vel_x`/`vel_z`. No run-level
   * duplicate is kept — row 0x1A and the goal-screen selection read the camera
   * fields directly. `bounce_15759f`/`bounce_1575cc`/`bounce_1575d0` = the
   * goal-bounce cells `[0x15759F]`/`word[0x1575CC]`/`word[0x1575D0]` the id-1
   * ladder arm reads (producer FUN_00072AC4 unported, FU-145 / FU300-L3;
   * staged zero). */
  uint8_t global_157c2c;
  int32_t bounce_15759f;
  int16_t bounce_1575cc;
  int16_t bounce_1575d0;
  /* FU296 Seam A (phase-27 T3) + FU299 (phase-28 T2, Option B): the record-0
   * machine `FUN_000782D0` view the FU-141 walk steps in its `i == 0` branch.
   * `fifa96_match_run_init`/`_begin` reset it; `fifa96_match_run_frame` hands
   * it to the walk. Its `side_xor` cell binds to the run's `[0x157ABE]` process
   * cell per frame; its `interlock_157c5d`/`latch_157c5c`/`force_install` cells
   * bind/publish around each record-0 step through the walk's cell pointers
   * (read cell -> step -> write cell — the machine is a view, not a second
   * copy; the native gate -> ladder -> clear -> dispatch order). */
  fifa96_keeper_cpu keeper_cpu;
  /* M2 phase-15 T3 / FU-178: the row-09 stage an engine dispatch reached but
   * did not implement (arms 1/2/3 = FU178-L1). 0 = none; 1/2/3 = the unported
   * arm the handler parked at (the native advances/acts there). Written by
   * `fifa96_match_action_09`, cleared by init; observability for the explicit
   * unported path (never a silent no-op). */
  uint8_t match09_leg_stage;
  /* FU302 (phase-28 T7): the row-0x1B wide-path completion — the seed + Blocks
   * A-G through the `0x71C94` seat (`0x77423`) and the install tree
   * (`0x77428..0x774D2`). The values the port computes but whose sinks are
   * unported (the OL-27/OL-82 family) are recorded here, never faked:
   *  - `match1b_hypot` = the `0xCD514` fold result stored to `word[0x1577BE]`
   *    at `0x77275`; the seat's setter re-derives the live cell, so this
   *    records the fold itself. **0 means "the zero guard skipped the fold"**
   *    (the live `[0x1577BE]`/`camera.speed` value is not mirrored here), not
   *    a computed zero;
   *  - `match1b_pan_x/z` = `word[0x1577C0]/[0x1577C2]` after Block E's negate,
   *    `int8[desc+0x1B]` jitter and `0x8DC50` reduce (`0x77312`);
   *  - `match1b_vec_mag/x/z` = the native 6-byte vector `{mag, step_x,
   *    step_z}` at `ESP+8` (`0x7733F..0x77385`);
   *  - `match1b_sign_match` = the `EDI` flag (`0x77313..0x7733A`);
   *  - `match1b_event_id` = the `0x8F188` request (6/3/0; 0xFF none),
   *    `match1b_stat_id` = the `0x8ED40(rec, …)` request (0x64/0x19/5; 0xFF
   *    none), `match1b_stat_tracked` = the `0x8ED40(tracked, 5)` companion
   *    (after the seat `[0x1577CA]` is the record), and `match1b_sound_id` =
   *    the terminal `0x974DC` request (4);
   *  - `match1b_sit_code`/`match1b_sit_emit` = the sit dispatch's
   *    `0x92820(rec, 0x1A)` / `0x651F0(5|0xE)` requests. The native request is
   *    **unconditional** (`0x773DC`, after the `[0x157823]` gate); only the
   *    latch clear and the counters are gated. 0 = not set (an early wide
   *    gate), 0x1A/5|0xE on every pan dispatch.
   * Cleared by init/begin and at the top of every row-0x1B dispatch (so a
   * head-diverted dispatch reports nothing, not a stale previous result). */
  int16_t match1b_hypot;
  int16_t match1b_pan_x, match1b_pan_z;
  int16_t match1b_vec_mag, match1b_vec_x, match1b_vec_z;
  uint8_t match1b_sign_match;
  uint8_t match1b_event_id;
  uint8_t match1b_stat_id;
  uint8_t match1b_stat_tracked;
  uint8_t match1b_sound_id;
  uint8_t match1b_sit_code;
  uint8_t match1b_sit_emit;
  /* FU302 (phase-28 T7): the `[0x157823]` one-shot sit-dispatch latch and the
   * `[0x157ACC]/[0x157AD0]` per-side sit counters the `0x7738C..0x773D8` block
   * increments (`index = (side ^ [0x157ABE]) & 1` through `0x741B4`; the run's
   * `side_swap` is the `[0x157ABE]` cell). The latch's native producer is
   * unported (FU302-L5), so it is a staged input that defaults to zero; it
   * gates only the counter increments and its own clear, **not** the sit
   * request (`0x77398 JZ 0x773DC`). The counters are live native state.
   * Init/begin clear them. */
  uint8_t flag_157823;
  uint16_t counter_157acc[2];
  uint16_t counter_157ad0[2];
  /* FU300 T4 (phase-28): the row-0x1A sinks' computed arguments. The sinks
   * themselves (`0x8ED40` actor registrar, `0x8F188` event ring, `0x651F0` sit
   * dispatch, `0x974DC`/`0x974F0` audio) are unported (FU300-L3), so the row
   * records what it would hand them: `match1a_event_id` = the event-ladder id
   * {5,4,9,1,7,0} and `match1a_anim` = the **last** `0x651F0` argument — 6 from
   * the direction arms, then 0xF/0xE/1 from the event block's select. The
   * direction-arm 6 is observable on **both** `0x54` exits before that select
   * (the `0x767E7` lane arm and the `0x76868` anim arm), not only the latter.
   * Observability only — never a silent no-op (the row-reference precedent).
   * Cleared by init/begin. */
  uint8_t match1a_event_id;
  uint8_t match1a_anim;
  int32_t predict_c44;
  int32_t predict_c48_x;
  int32_t predict_c48_y;
  int32_t predict_c48_z;
  /* M2 playable-match Task 2 / OL-84 residual: the native `[0x5882A]` kickoff
   * gate, the flag the act-1 (phase-0x17 handler FUN_00088DC8) stage 1 sets at
   * the shared timeline timer `[0x58818] >= 0x78` (`0x88EF3..0x88F07`) and
   * action row 01 stage 0 gates on (`0x7DC6B`). M2 phase-18 T4 (FU-203): the
   * phase-0x17 row is now registered, so its stage-1 arm is the live producer;
   * `fifa96_match_run_frame` still derives it from `state.tick_total` (the same
   * whole-delta accumulation, `[0x58818] += [0x57A64]`) while the phase is 1 —
   * the retained derived stage-1 remainder (idempotent; leg FU203-L2).
   * init/begin clear it. */
  uint8_t global_5882a;
  /* M2 phase-15 T1 / FU-175: the native singleton act slot `FUN_000888FC`
   * installs (`id = byte[0x158828]`, `stage = byte[0x158829]`, `timer =
   * word[0x158818]`). The act machine (`fifa96_match_act_dispatch` /
   * `fifa96_match_act_tick`) owns it; the clock-tail tick runs after the ball
   * machine (the native `0x8B672`). This absorbs the FU-170 §7 ad-hoc
   * `act9_armed`/`act9_timer` pair: act 9 is now `dispatch(mr, 9, 0, invoke)`
   * and its handler is the phase-0x1F row (`0x88F4C`) reached through
   * `fifa96_match_dispatch_phase`. init/begin clear it. */
  struct fifa96_match_act act;
  /* FU-191 §6/§9 (M2 phase-17 T1): the per-record phase-walk cursor. The native
   * walk `FUN_0008D098` threads the record through EAX and the third argument
   * through EBX=-1; the engine's `fifa96_match_handler_fn` carries only `mr`, so
   * `fifa96_match_run_record_phase_walk` stages the current record + the native
   * EBX here before `fifa96_match_dispatch_phase_record`, and a ported
   * per-record body (`fifa96_match_phase_02`) reads it. `phase_walk_active` is
   * 0 outside the walk. `phase_dir_default[2]` is the native per-side default
   * direction index `word[0x1577DE + side*4] >> 16` the `0x6DCC8` third-argument
   * `< 0` path selects (FU-198 §5.1); its producer is unported (a runtime cell),
   * so init/begin stage it 0 (FU-177 §7 dispatched-but-gated). */
  struct fifa96_match_entity *phase_walk_rec;
  int32_t phase_walk_edge;
  uint8_t phase_walk_active;
  int32_t phase_dir_default[2];
  /* The derived act-9 → GATE handshake marker: within the restart window the
   * native `[0x15882A]` gate has exactly one reachable producer. Fresh xrefs
   * (24 touches): the leg-0 SETUP0 clears it unconditionally (`0x93C30`;
   * legs 1/3 set `0x93E84`/`0x942F5` when `[0x15B684] != 0`
   * (`0x93E74 CMP`/`0x93E7B JZ`, `0x942EA TEST`/`0x942EC JZ`) and clear
   * `0x93EB6`/`0x94327` on the staged (`== 0`) path), the
   * reset clears it (`0x886F4`), act-1's kickoff handler clears at `0x88E56`
   * and sets at `0x88F07`, act-2's tail sets at `0x895EC`, and act-9 stage 0
   * clears it again (`0x88F9A`) while it writes phase 0x15. After that write
   * the screen machine waits at step 1 with the singleton act slot holding
   * act 9 (the sit-0xC dispatch), so the only producer that can open *that
   * window's* gate is the act-9 stage-3 completion (`0x890C4`); the kickoff
   * gate (act 1), act-2's value, the reset and staged/forced values (the
   * M2 v13 tape's m 42 compression) are the refused non-act-9 values.
   * `act9_gate` marks the completion; SETUP2's sit-3 arm consumes it (the
   * native step-2 queue runs on the produced gate). A staged gate therefore
   * advances without the corner arm — the disclosed hardening that keeps the
   * v13 strata; see the FU-170 T3/T4 status. Cleared by SETUP0 (fresh cycle)
   * and init/begin. */
  uint8_t act9_gate;
  /* FU-145 S2 (goal arming) run state, native cells:
   *  - `goal_armed` = [0x15781D] the pan-arm flag (armed by the armer
   *    0x713DB; cleared by a phase-2 write 0x740F6, the reflect arm 0x71908,
   *    the restart body 0x84F90 and the S3 goal handler 0x9453C);
   *  - `goal_zone` = [0x15781E] the FUN_00070074 goal-mouth classifier's
   *    return (1 = the camera sits inside the mouth band);
   *  - `goal_snap_x/y/z` = the frozen 0x15777C/80/84 pan-time snapshot triple
   *    (y is forced 0 by the armer; the scanner reads z);
   *  - `situation_id` = [0x15B6A8] the table-1 queue cell (0 = none, 5 =
   *    side-0 goal, 6 = side-1 goal) and `situation_pending` = [0x15B6C0] its
   *    latch; written by `fifa96_match_run_goal_queue` (S2), consumed by the
   *    S3 scheduler/handlers;
   *  - `session_gate_14c32a` = [0x14C32A]: begin seeds 0 (FU-166/R14: the
   *    settled value of every ordinary played match — Friendly/Leagues/
   *    Tournaments/Playoffs write mode 0; byte 1 is the Practise session
   *    only, `FUN_00032DE0`). With the gate 0 the per-frame scheduler is off
   *    and the queue route falls back to the native table-2 situation-6 arm
   *    (the native byte-0 direct goal path). */
  uint8_t goal_armed;
  uint8_t goal_zone;
  int32_t goal_snap_x;
  int32_t goal_snap_y;
  int32_t goal_snap_z;
  /* FU-173 §4.2 (phase-14 T3): the goal-record cell the byte-0 phase-5
   * celebration arm reads — the native `[0x157A9F]` pointer (the goal
   * scanner's selected possession record). The engine store is the pool id
   * (`team * RECORDS + index`) plus the block team's side byte (the arm's
   * comparison operand); written by `fifa96_match_run_goal_scan`'s goal arm
   * (the only scanner arm whose record selection the port derives — FU173-L1
   * bounds the throw-in/corner writes), NONE until a scan, cleared by
   * init/begin; consumed by `fifa96_match_run_phase_arm(5)`. */
  int32_t goal_record;
  uint8_t goal_record_side;
  uint8_t situation_id;
  uint8_t situation_pending;
  uint8_t session_gate_14c32a;
  /* FU-149 P1 (set pieces & restarts) dispatcher state, native cells:
   *  - `sit_side_pending` = [0x15B6B8]: the queue head latches `(side == 0)`
   *    of the queued situation (0x8A977/0x8A982) before the table-1 id write;
   *  - `corner_count[2]` = the `word[0x157AD4]`/`word[0x157AD6]` corner
   *    counters, indexed by `side ^ side_swap` (0x8ABFF/0x8AC0F); reset by
   *    the match reset FUN_00073EE0 (0x73F6A/0x73F9C) and begin; no reader
   *    (FU-149 L10: accumulated count is write-only statically);
   *  - `side_swap` = byte [0x157ABE], the display/score slot swap (the
   *    producer is unported, FU-143 OL-75 / FU-149 L1-adjacent; seeded 0);
   *  - `store_15882b`/`store_15882c` = the act-8 replay bytes [0x15882B]
   *    (situation) / [0x15882C] (side) written by the BX!=0 fallback
   *    (0x8AA93/0x8AA9D) and re-read by act 8's tail (0x8A8BD..0x8ACD); the
   *    tail leaves `store_15882b = 0xFF` (0x8A8D3);
   *  - `incident_x`/`incident_y`/`incident_z` = the incident position dwords
   *    [0x158897]/[0x15889B]/[0x15889F] the phase-7 arm probes (0x8D5A8) and
   *    the row-0x12 place copies (0x83E7C..0x83E7E, FU310-L1); the producer is
   *    the FU-150 foul adjudicator (P2), seeded 0. (FU-149 §3 lists 0x15889B as
   *    the z cell; first-hand the 12-byte triple is x@897/y@89B/z@89F and
   *    `FUN_00079CCC` reads +8 = 0x15889F — slice erratum.) */
  uint8_t sit_side_pending;
  uint16_t corner_count[2];
  uint8_t side_swap;
  uint8_t store_15882b;
  uint8_t store_15882c;
  /* FU-194 §4.1/§4.2 (M2 phase-17 T5): the act-8 (phase-0x1E) restart-sweep
   * cursor. The native walk indices are `byte[0x15882E]` (team, `act8_team`)
   * and `byte[0x15882F]` (record, `act8_record`); `[0x1587F0]`
   * (`act8_selected`, NULL = none) is the selected code-0xC record the stage-1
   * sweep stores (0x8A831). Three distinct cells from the stored situation/side
   * (`store_15882b`/`store_15882c`, §4.2). The native producer of the walk is
   * the unported act-8 timeline; the engine zeroes them (FU194-L2). Read only by
   * the unwired `fifa96_match_action_phase_1E` body. */
  uint8_t act8_team;
  uint8_t act8_record;
  struct fifa96_match_entity *act8_selected;
  int32_t incident_x;
  int32_t incident_y;
  int32_t incident_z;
  /* FU-149 L13 (T1 taker rows): the throw/corner staging cells the 0x10/0x11/
   * 0x12/0x13 stage machines read and write across frames:
   *  - `sp_flag_158784` = `word[0x158784]`, the throw-staging gate the derived
   *    `FUN_000832A8` stand-in clears (`0x832BC`) and the row stages test
   *    (`0x85838`/`0x86033`/`0x840D2`); the other writer (`FUN_00078824`
   *    0x78911, the slot machinery) is unported (L13 leg);
   *  - `sp_delivery` = `byte[0x1587B4]`, the throw/corner delivery/event row
   *    index (1..7) `FUN_00085498` writes (`0x8549x`) and the kick stages index
   *    their event sub-tables with (`0x861DD`/`0x84289`, `[x]-1`); the image
   *    default is 0 and the input-driven `FUN_00083428` rows 1..6 stay legs. */
  uint8_t sp_flag_158784;
  uint8_t sp_delivery;
  /* FU274 §1.1: `byte[0x157821]` -- row 0x10's far-ball re-queue gate
   * (`0x85CE4`) and the keeper predictor's pan gate (`0x739BB`/`0x7489C`) --
   * is one native cell owned by the camera block: `render.camera.pan_counter`
   * clears it on reset (`FUN_000700F4`) and increments per pan frame
   * (`FUN_000709D0`). The former separate `sp_157821` shadow is retired. */
  /* FU-196 §3/§4 (M2 phase-17 T2) + FU-202 §6 rec (b) (M2 phase-18 T3): the
   * situation-producer machine. The native match-init
   * `FUN_000493A0 -> FUN_0004B02C(1)` (0x49415) and the period-end
   * `FUN_0004B02C(0)` (0x4950A) + `FUN_00088860` (0x88860) are the sole native
   * producers of situations 0/8 (FU-196 §3.1/§3.2); the engine had none.
   * `fifa96_match_run_period_setup` ports `FUN_0004B02C`. Its live `begin`
   * wiring is gated by `period_setup_live`: T3 sets it **1** on `begin` and runs
   * the mode-1 producer *after* the begin-tail act-slot reset, so the situation-0
   * act 0xA survives into the first granted frame (phase 0x20 installs code 0x24;
   * FU-202 §6 rec (b)). `fifa96_match_run_init` leaves it 0 (the standalone
   * default). Situation 8 fires act 7 (phase 0x1D), whose body/slot stay NULL
   * (the coupled row-0x20 installer needs the unported actor-record producer;
   * FU205-L2). */
  uint8_t period_setup_live;
  /* FU-197 §2/§5 (M2 phase-17 T2): the act-4 sit-2..4 formation-order arm's
   * state. `formation_order[side][i]` mirrors the desired line-up matrix
   * `0x14C360 + side*0x7B + i*4`; `formation_current[side][i]` mirrors the
   * current order `0x157BD2 + side*0x2C + i*4` (the `FUN_00073EE0` reset copies
   * the desired order into it, FU-197 §5.3); `formation_mismatch[side]` is the
   * `FUN_0008C974` residual `[0x157B8E + side]`; `formation_order_ok` gates the
   * whole arm. FU-201 §3/§6 (R49) pins the verdict: the native desired-order
   * producer is the **front-end menu** roster builder
   * `FUN_00011620`/`FUN_00011978`/`FUN_00012010` (five front-end callers,
   * fresh), and the match-init reset `FUN_00073EE0` copies desired→current. The
   * arm's gate `[0x157B8E]/[0x157B8F]` is a **permutation residual** from
   * `FUN_0008C974`, so immediately after the reset `current == desired` and the
   * residual is 0: the fill is **necessary but not sufficient** to trip the arm.
   * Un-gating `formation_order_ok` on the fill alone would be either a **no-op**
   * (current copied) or a **fake producer** (current left zeroed — sorting the
   * zero stand-in manufactures a residual). The genuine trigger is a runtime
   * **non-permutation** of current vs desired (FU201-L1 / FU197-L1) that no
   * native or engine writer emits on the tape. So `formation_order_ok` stays 0
   * and the matrices are a **staged leg** (FU201-L4: the engine has no
   * match-begin consumer of the desired matrix — its begin formation consumer
   * `match_run_formation_seed` reads the `.fmt` layout blob, not this
   * descriptor-pointer order matrix). Init/begin zero the state and clear the
   * gate. */
  int32_t formation_order[FIFA96_MATCH_ENTITY_TEAMS][FIFA96_MATCH_ENTITY_RECORDS];
  int32_t formation_current[FIFA96_MATCH_ENTITY_TEAMS][FIFA96_MATCH_ENTITY_RECORDS];
  uint8_t formation_mismatch[FIFA96_MATCH_ENTITY_TEAMS];
  uint8_t formation_order_ok;
  /* FU-197 §2: the arm's stored re-dispatch cells `[0x158820]` (side, word) and
   * `[0x15881E]` (situation, word) set at `0x8AB45`/`0x8AB50` and read by act-4
   * stage 1 (`0x897AE`/`0x897B4`). Only the gated act-4 arm writes them. */
  uint8_t store_158820;
  uint8_t store_15881e;
  /* FU-150 P2 (fouls/referee/offside): the caller-owned `fifa96_referee_state`
   * (registrar/decision/sequence cells), the staged `fifa96_match_config`
   * (settings gates `[0x14C306]` field_4c306 / `[0x14C2F2]` field_4c2f2; init
   * zeroes, begin installs the FU-68 default-settings handoff), the derived
   * machine dispatcher (`ref_machine`, FIFA96_MATCH_RUN_REF_*), the derived
   * act-2 hand-off stage (a compression of the native stage 1..3 camera-lead
   * gates) and the whistle/speech request observation slots (the FU-63
   * event-queue sinks stay unported, FU-150 leg 3). */
  struct fifa96_referee_state referee;
  struct fifa96_match_config config;
  uint8_t ref_machine;
  uint8_t ref_restart_stage;
  uint8_t ref_whistle;
  uint8_t ref_speech;
  /* FU-148 §3 (S4): the per-side formation id ([0x14C1E4]/[0x14C1E5]). The
   * native image default is 0 (BSS) and the match-init producer FUN_00011620
   * copies the team record +0x12 byte; the engine has no team record (leg), so
   * begin seeds 0 and `fifa96_match_run_set_formation` (the FUN_0008EA70
   * writer) is the derived producer. The kickoff formation seed reads
   * `formation[controlled_side]` for the 0x14BFC0 `6*id` placement name. */
  uint8_t formation[2];
  /* FU-146 S3 (goal consumers) run state, native cells:
   *  - `screen_leg` = [0x15B680] the installed `0x110F78` handler index
   *    (0..5; -1 = no handler, the image's [0x15B6D4]==0);
   *  - `screen_mode` = [0x15B6BC] the installer's mode word (duration row);
   *  - `screen_step` = [0x15B6B0] the handler step counter;
   *  - `screen_timer` = [0x15B688] the handler's frame-delta accumulator
   *    (`word[0x157A64]` per granted frame, 60 units/s);
   *  - `screen_period_frames` = [0x15B694] the per-leg/mode duration
   *    (`0x1110EC[mode*24+leg] * 60`, seeded by screen_install);
   *  - `screen_install_hint` = [0x15B6C4] (1 after FUN_000935A0);
   *  - `screen_actor_age` = [0x157A97] the camera-track actor age (producer
   *    FUN_00072AC4 unported, staged 0);
   *  - the native lead words `word[0x1577C0]`/`word[0x1577C2]` are NOT
   *    duplicated here: they are `camera.vel_x`/`vel_z` (the one native pan
   *    pair; the row-0x1A `'T'` arm and row-0x1B Block E write them, and the
   *    goal-screen legs below read `camera.vel_z`);
   *  - `goal_no_score` = [0x15B6A0] the handler's no-score counter;
   *  - `goal_last_id`/`goal_minute` = [0x15B674]/[0x15B678] the last consumed
   *    id and its minute (the goal-log triple components);
   *  - `goal_screen_accum` = [0x15B68C] the per-leg screen-time accumulator;
   *  - `goal_log_prev_total`/`goal_total` = [0x15B698]/[0x15B69C] the
   *    score-total bookkeeping (the ring append gate);
   *  - `goal_log` = the 0x15B6D8 goal-log ring (12-byte triples, 21 slots;
   *    the native shift-and-append ring);
   *  - `goal_probe_limb` = the FUN_000CBC4C six 32-bit limbs
   *    (0x112E68..0x112E7C, seeded from the image). */
  int16_t screen_leg;
  int16_t screen_mode;
  uint8_t screen_step;
  uint16_t screen_timer;
  uint16_t screen_period_frames;
  uint8_t screen_install_hint;
  int32_t screen_actor_age;
  uint32_t goal_no_score;
  uint8_t goal_last_id;
  uint16_t goal_minute;
  uint32_t goal_screen_accum;
  int32_t goal_log_prev_total;
  int32_t goal_total;
  int32_t goal_log[21][3];
  uint32_t goal_probe_limb[6];
  /* FU-139 §11 (Task 13): the native [0x157A4F] frame toggle
   * (FUN_0004B100 0x4B11A `XOR AH,1` / 0x4B129 store; cleared by the match
   * reset FUN_0004B02C 0x4B038). Row 06's claim arm and the second-half
   * searches run only on the odd frames; the derived frame body toggles it
   * once per granted 30 Hz frame before the entity chain. */
  uint8_t pass_parity;
  /* Task 15 / M2-B observability: bit `c` is set when action code `c`
   * dispatched FIFA96_OK through fifa96_match_dispatch_action during this run
   * (the acceptance tape reads the wired-row dispatch set from it). Pure
   * bookkeeping — no handler behavior reads or depends on it. Reset by
   * init/begin like the other per-match counters. */
  uint64_t dispatched_ok;
  struct fifa96_match_run_render render;         /* Task 15 presentation state */
  void *stage_owner;                             /* Task 2 staging arena (owned) */
  /* FU-199 §7.2 (M2 phase-18 T1): the owned decoded `.fmt` family blobs the
   * OL-13 metadata bind keeps alive for the run — the record `phase_meta`/
   * `phase_meta10`/`phase_meta14` pointers index into it. Freed by end (and by
   * a re-bind); assigned NULL by init (init accepts uninitialized memory). */
  void *meta_owner;
};

/* Zero-init a run: lifecycle, pace, match state (clock and score pair), input
 * model, control slot, the dispatch staging record (FU-141), the entity/ball
 * pool (init seeds the native record reset), the FU-142a installer-arms
 * machine (zeroed, chosen831 seeded NONE), presentation state
 * (camera/window/display/scene, rendering disabled), the staging-arena holder
 * (assigned NULL, never freed: init accepts uninitialized memory, so it cannot
 * trust the holder), backend, counters and engine linkage. Must be called
 * before the first begin on a run. A staged run must be released by end (or the
 * next begin) before re-initialization; a direct re-init leaves the arena
 * unreachable and leaks it. NULL is a no-op. */
void fifa96_match_run_init(struct fifa96_match_run *mr);

/* One match input poll: fold the engine key presses in `keys` (codes 1..9;
 * state == 1 only) into the FU-61 keyboard-handler code byte (directions
 * 0x1/0x2/0x4/0x8 = up/down/right/left, buttons 0x10 kick / 0x20 pass), pass
 * it through the FU-61 identity mapping row into the run's fifa96_input
 * edge/held model, and latch the mapped per-player state for the frame body.
 * A key absent from a later call reads as released through the input model's
 * previous-state array. The FU-70 control slot is NOT touched here: its update
 * runs once per granted frame inside fifa96_match_run_frame (FU-70 §1.1).
 * Returns 0, or -FIFA96_ERR_INVALID when `mr` is NULL; NULL `keys` and
 * `count == 0` are tolerated as a no-input poll. */
int fifa96_match_run_input(struct fifa96_match_run *mr, const fifa96_platform_key *keys,
                           size_t count);

/* M2 phase-10 T2 (FU-148 §12/§13): stage the native front-end options block
 * (13 dwords 0x105274..0x1052A4, edited by FUN_0001C728 / queried by
 * FUN_0001CAEC) and rebuild the run's eight consumed config cells
 * (0x105278/7C/88/8C/90/9C/A0/A4), the per-side range words [0x14C1D4]/D6
 * (FUN_0001C9BC) and the derived `render.input_bit0` (reflect) /
 * `render.input_bit2` (camera interpolation) bits. The pan-step walk gate
 * `(w0|w1) & 4` is read by the frame body. Returns FIFA96_OK,
 * -FIFA96_ERR_INVALID on NULL. */
int fifa96_match_run_set_input_options(
    struct fifa96_match_run *mr,
    const int32_t options[FIFA96_INPUT_OPTION_COUNT]);

/* Wire the run to a booted, non-quitting engine (MATCH mode), reset the match
 * clock (including the score pair), counters, input model, control slot and
 * presentation state for a fresh match (rendering disabled; the full-surface
 * FU-92 window is sized from the engine surface), install the derived FU-62
 * period lengths for the selector (see the FIFA96_MATCH_RUN_*_SECONDS_*
 * constants), run the derived FU-89 kickoff placement (formation seed +
 * record commit) and install the derived FU-143 §8/OL-84 kickoff phase entry
 * (`FIFA96_MATCH_RUN_KICKOFF_PHASE`: phase 1, class 0) together with the
 * derived `FUN_0008D098` state-1 arm (`fifa96_match_phase_machine_kickoff`,
 * installed between the phase entry and the placement commit, the native
 * order), and start the
 * lifecycle. The derived act-1 producer (`global_5882a`) and the wired action
 * row 01 then carry the run from phase 1 to the live phase 2 on its own
 * (M2 playable-match Task 2 / FU-143 §11). Returns 0,
 * -FIFA96_ERR_INVALID (NULL arguments), -FIFA96_ERR_STATE (unbooted/QUIT
 * engine, run already live, or another run live on the engine), or the
 * lifecycle's register failure. */
int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector);

/* Write the FU-62 §4.6 period lengths the clock's period-end check reads
 * ([0x5881A] regular periods 0/1, [0x5881C] extra periods 2/3), in whole
 * seconds. Values are written verbatim (zero reproduces the state-init
 * end-on-first-second behavior); a live match normally keeps begin's derived
 * default. Returns 0, -FIFA96_ERR_INVALID (NULL) or -FIFA96_ERR_STATE (run
 * not live). */
int fifa96_match_run_set_period(struct fifa96_match_run *mr, uint16_t period_seconds,
                                uint16_t extra_seconds);

/* FU-72 §2.4 plain increment: one side's goal word (`INC word [side*2 +
 * 0x57AC5]`). Since FU-146 S3 the eleven FUN_00093944 call sites (the
 * period-indexed goal-screen handler cluster, FU-142 App. I.10) land in
 * `fifa96_match_run_screen_step` -> `fifa96_match_run_score_event`; this plain
 * increment stays as the `fifa96_match_run_goal_queue` direct fallback
 * (`0x8AC88`/`0x8AC94`). begin and teardown reset the pair. Returns 0,
 * -FIFA96_ERR_INVALID (NULL or side > 1), or -FIFA96_ERR_STATE (run not
 * live). */
int fifa96_match_run_add_goal(struct fifa96_match_run *mr, uint32_t side);

/* C3-OL2 (M2 playability Task 4): the derived `FUN_00093944` score-event
 * source (`fifa96_action_score_event`, FU-142 App. I.10 / FU-72 §2.4 errata)
 * run over the match's live score pair and state cells. Increments one side's
 * goal word, records the side and, when `score_tracked_side` is not the
 * carried -1, updates the tracked-side goal difference and posts the native
 * threshold event id; the id the latest call posted lands in
 * `score_last_event` (0 = none). `probe` is the `FUN_000CBC4C` byte
 * (`fifa96_match_run_goal_probe`); the writer reads it only on the
 * `score[side] == 1 && score[other] < 3` non-tracked arm, and the goal-screen
 * post step computes it exactly then (else 0).
 *
 * The native invokers are the eleven goal-screen handler sites
 * (`0x93D98..0x9486E`, FU-142 App. I.10), driven by the `FUN_0008A938`
 * situation queue and the period-indexed handler table `0x110F78`; since FU-146
 * S3 they are reached through `fifa96_match_run_screen_step` (installer ->
 * frame scheduler -> post step, with the `FUN_0009252C` display gate carrying
 * only `score_last_event`). `fifa96_match_run_add_goal` stays the FU-72 plain
 * increment for the `fifa96_match_run_goal_queue` direct fallback. Returns 0,
 * -FIFA96_ERR_INVALID (NULL `mr` or `side > 1`), or -FIFA96_ERR_STATE (run not
 * live). A writer failure (invalid staged tracked side) leaves the run
 * untouched. */
int fifa96_match_run_score_event(struct fifa96_match_run *mr, uint32_t side, uint8_t probe);

/* Exactly one 10 ms/100 Hz pace tick of the match frame body. Feeds the FU-60
 * pace (blocked = 0, clock_halt = 0): 1 = the pace granted a 30 Hz frame, the
 * match state advanced one 0x200 step, the controlled player's FU-70 slot was
 * updated once with the FU-62 §4.3 whole frame delta
 * (`state.frame_delta`, 2 at the 0x200 step = 60 counter units/s), the
 * FU-71 camera/display blocks advanced with the same delta (FU-71's
 * FUN_000736AC runs from the frame body FUN_0004B100, not the render driver),
 * and the FU-141 entity/ball pool ran the FU-67 chain (team 0, team 1, the
 * ball pairing) with the FU-137 action dispatch bound to the pool records,
 * after the derived act-1 kickoff producer armed `global_5882a` while the
 * phase is 1 (FU-143 §11; the wired row 01 gates on it);
 * then, when the phase is 0x13/0x14, the FU-142a
 * `fifa96_match_phase_machine_step` ran the FUN_0008D098 installer arms once
 * (the FUN_000740A0 order);
 * then, when `session_gate_14c32a` holds, the FU-146 S3 goal-screen scheduler
 * ran (`fifa96_match_run_screen_schedule`, the native `0x4B1A1` before the
 * clock body; a queued goal id is consumed here through the installed period
 * handler into `fifa96_match_run_score_event`);
 * and the derived FU-143 phase driver ran (`fifa96_match_run_phase_drive`),
 * staging the FU-62 clock's `period_ended` completion and writing the derived
 * post-period phase (2 -> 0x0C under the selector-0 default);
 * 0 = no frame was due; or a -fifa96_err_t. A period end marks the lifecycle
 * over, except when an exit is already staged: the staged EXIT wins because
 * the frame body runs in the clock advance before the exit step consumes it.
 *
 * Sole driver: the registered 100 Hz tick trampoline (fifa96_match_run_tick),
 * so one call happens per PIT tick. fifa96_match_run_step must NOT call it. */
int fifa96_match_run_frame(struct fifa96_match_run *mr);

/* FU-159 §5.4 / FU-160 §1.4 (T3): the `FUN_0006E598` store core over a pool
 * record — the row install the unported action bodies (row `0x0E` ->
 * `0x827D0`) and the tests use. First-hand `/FIFA96.EXE`
 * `disassemble_bytes 0x6E598..0x6E713`:
 *  - the id clamp `(int16)id < 0 || >= 0x6F -> 0` (`0x6E68E..0x6E69B`);
 *  - the row pointer `[0x157588] + id*9` and the flags folds
 *    `+0x45 = flags & 4`, `+0x43 = flags & 2`, `+0x44 = flags & 1`
 *    (`0x6E6B4..0x6E6CC`) — the engine's `row44` producer;
 *  - `+0x46 = 1` (`0x6E6D8`) and the `+0x3F` lean 0/2/-2 from flags bits
 *    4/5 (`0x6E6E0..0x6E6FB`);
 *  - the frame resolve `FUN_0006E490` (`0x6E6FE..0x6E706`: `frame < 0 ->
 *    last`, `frame > last -> 0`) and the frame-timer zero `word[+0x32] = 0`
 *    (`0x6E70B`).
 * The derived surface stores `anim_id`/`frame`/`row44`, forces the aux
 * re-arm (`aux_seen = 0xFFFF`) and zeroes the render slot's `anim_timer`
 * (the +0x32 twin; the slot is `team*11 + index`). NULL `mr`/`e` or an
 * out-of-range slot -> -FIFA96_ERR_INVALID. Returns FIFA96_OK. */
int fifa96_match_run_record_anim_install(struct fifa96_match_run *mr,
                                         struct fifa96_match_entity *e,
                                         int32_t id, uint8_t frame);

/* FU-142b (phase-12 T1; exposed phase-28 T4): the post-handler velocity
 * decomposition the FU-141 dispatch runs after every action body and before the
 * mover — the arm rows write the dword views `vel_x`/`vel_z` (+0x71/+0x73 as a
 * 16.16 pair), and this pass rebuilds the word views the mover reads
 * (`speed71` +0x71, `vel73` +0x73, `vel75` +0x75) from them. A body that writes
 * a native word directly (row 0x1A's `'T'` metric at 0x768CE) must mirror it
 * into the dword view's low word or this pass discards it (the T4 review
 * finding). Exported so the row-level pins exercise the exact dispatch pass;
 * NULL is a no-op. */
void fifa96_match_run_decompose_velocity_words(struct fifa96_match_run_record *r);

/* FU-159 §3.1/§5.2 (T3): the `FUN_0008E008` frame-wrap row successor over a
 * pool record (the pool twin `FUN_0008DF84` install core). First-hand
 * `decompile_function 0x8E008`: on the frame-timer expiry of a record whose
 * current frame is its row's last, the successor is the row's `next_id`
 * (`+3`) when `byte[+0x44] == 0` — `0x6F` keeps the current row, `< 0x70`
 * installs it, `>= 0x70` mirrors the face (`FUN_00079C50(-dir[oct])`) and
 * installs `next_id - 0x70`; the install frame argument is
 * `byte[+0x46] + byte[+0x3D]` (the just-zero-resolved last frame plus the
 * `+0x46 = 1` install default, so a same-size-or-smaller next row starts at
 * frame 0). Terminal rows (`flags & 1`) take the movement/steering tables
 * (`0x11052A`/`0x1105AA`, FU159-L5/FU161-L3) and are this helper's no-op.
 * The mirror octant is `(type + 4) & 7` (the 0x10F334/0x10F33C vectors
 * negate exactly; `FUN_00079C50` recomputes the same octant). Stores
 * `anim_id`/`frame`/`row44`. NULL -> -FIFA96_ERR_INVALID. */
int fifa96_match_run_record_anim_successor(struct fifa96_match_entity *e);

/* M2 phase-9 T3 (FU-152 §2.9 / FU-145 §1.2): the every-frame FUN_00071DF4
 * call site (FUN_000736AC 0x739C6..0x73B6B). Runs the ported first arm when
 * the call-site gates hold — `pan_counter == 0`, the bearing word
 * `[0x1577BE] != 0`, `[0x1577EE].hi > 0x10`, both rate bytes
 * `[0x157816]/[0x157817] == 0` — and `render.camera.tracked` ([0x1577CA], set
 * by the row-event wiring through `fifa96_camera_set_tracked`) names a pool
 * record with a slot ([rec+0x20]): `fifa96_camera_rate_event` follows the
 * slot dir bytes into the camera velocity when the event height
 * `[0x1577EE].hi` exceeds 0xF0 and clamps the pair to +-15. Returns 1 when the
 * arm ran, 0 when a gate refused, or -FIFA96_ERR_INVALID on NULL. The
 * record-class gate `[[rec]+4][0] == 0x18D8` reduces to the pool-record
 * identity (all three [0x1577CA] writers store pool records or 0 — 0x71D27
 * the store, 0x70258/0x70054 the clears; the +4
 * descriptor is built at runtime). The FU303/FU294-L2 second arm
 * (`0x71ef0..0x7200f`, phase-28 T8) is `fifa96_camera_rate_table`: the
 * `[0x157815]` dir byte + the tracked record's dir pair resolve the
 * 0x10E169/0x11042B rate pair into `camera.rate_x/rate_z` for the next
 * frame's pan-push; the keeper event sink (the
 * `FUN_00092998(1,2,-1)` ring) stays a leg (OL-27), so the arm is called
 * with `event_byte` 0. Returns 1 when either arm ran. */
int fifa96_match_run_camera_follow(struct fifa96_match_run *mr);

/* M2 phase-10 T1 (FU-148 §13.5 / FU-142 OL-88): the FUN_0007A084 receive arm,
 * the staging tail's 0x7A8C7/0x7A8EF call (the code-keyed `receive` flag
 * `fifa96_ball_pair_stage_tail` reports; the native gate at 0x7A8BE/0x7A8D1
 * proves every receive code has `[0x1104BB+code] == 0`, so the 0x7A0A4 entry
 * gate is a pass-through on this path).
 *
 * First-hand `/FIFA96.EXE` `disassemble_function 0x7A084` (269 insns):
 *  - 0x7A207..0x7A219 `FUN_00071C94(actor [0x158730], 0x158738, EBX =
 *    (int16)[0x15873E] = traj, ECX = 0)`: the camera event with the staged
 *    vector copied into 0x1577B8..0x1577BD (FUN_00070544 seeds the fast path
 *    from 0x1577BA/BC = the vector's middle (height) and z words), and the
 *    actor stored as [0x1577CA] (tracked). The 0x71C99 [0x157A6C] bail
 *    returns 1 from `fifa96_camera_event_set` without the reset/tracked
 *    store, but this arm *continues* (ack/scan/release still run); only a
 *    negative setter error aborts;
 *  - 0x7A227 `[0x157A83] = 0` (the controlled actor is released);
 *  - 0x7A2F0 `[0x158746] = 1` (the staged event is acknowledged);
 *  - 0x7A2F6 phase `[0x157A49]>>24 == 1` exits before the scan (the camera
 *    event has already run);
 *  - 0x7A2FF..0x7A331 the [0x157770] base plus the camera velocity words
 *    ([0x1577BE]/[0x1577C0] high halves << 5) leads, via the ported
 *    `fifa96_ball_pair_receive_target` (the engine's 0x157770 stand-in is its
 *    camera triple, FU-67 §4.1);
 *  - 0x7A335..0x7A376 the actor's own-team scan (FUN_0008DE8C: 11 records,
 *    word x/z, +0x98/+0x9A skips) with the skip word sign_extend8(+0x8D) when
 *    the +0x91 action is 1 or 0x10/0x11/0x12 — the native's
 *    `MOV EAX,dword[EBP+0x8E]; SAR EAX,0x18` reads the dword alias's high
 *    byte, i.e. +0x91 (the same idiom as dword[+0x69]>>16 = +0x6B); the
 *    +0x8E facing octant is not read by this gate — via the ported
 *    `fifa96_ball_pair_receive`;
 *  - 0x7A37B..0x7A3A2 `[0x158734] = receiver`; the actor team's +0x7B2 =
 *    receiver / +0x7B6 = 0; the opponent block's +0x7B2/+0x7B6 = 0;
 *  - 0x7A40A..0x7A412 the actor is `[0x158724]` (the possession carrier) ->
 *    `[0x15872D] = 0x14`.
 *
 * `actor`/`vector`/`traj` are the native staged block at the 0x7A8EF call
 * point ([0x158730]/0x158738/0x15873E) — the kick path passes
 * `fifa96_ball_kick_out`'s receive snapshot so the arm runs before the
 * 0x7A934 inactive clear's effects are re-applied by the caller.
 *
 * Legs (sinks/unmodeled cells, numbered in FU-148 §14): the
 * 0x7A0D6..0x7A201 0x18E2-class vector rebuild (the pool does not model the
 * record class); the 0x7A21E..0x7A2E1 animation-id sound selection and its
 * 0x974DC stoppage-push sink; the 0x7A3C0..0x7A408 timer81 loop (the +0x9D
 * byte and [0x157A3C] descriptor table are unmodeled); the code-3
 * [team+0x7E7] clear (unmodeled cell, OL-72); the 0x7A419..0x7A448
 * FUN_00079D5C foul/offside body (FU-150 E-chain).
 *
 * Returns FIFA96_OK (the arm ran), -FIFA96_ERR_INVALID (NULL mr/vector or an
 * actor outside the pool), or -FIFA96_ERR_STATE (the run is not live). */
int fifa96_match_run_ball_receive(struct fifa96_match_run *mr, int32_t actor,
                                  const fifa96_ball_pair_vector *vector,
                                  int16_t traj);

/* Step the derived FU-143 phase drivers for one granted 30 Hz frame (M2
 * playability Task 3). The frame body calls this once per granted frame, after
 * the FU-141 entity chain and before the FU-85 scene staging (the native clock
 * `FUN_0008AF38` runs after the entity chain at `0x4B1A6`, and its
 * `FUN_0008B9CC` phase write lands inside it).
 *
 * Behavior (first-hand `/FIFA96.EXE`; FU-143 docs/ghidra/FU143_phase_rows.md):
 *  - reads the live phase's row and applies the class-1/class-2 gate
 *    (`0x8AF41..0x8AF80`: class 1 always runs, class 2 runs while
 *    `[0x14C302]==0`, class 0 stops the clock);
 *  - consumes the staged FU-62 clock completion (`sec == limit + aux`,
 *    `0x8B1EA`/`0x8B21D`; see `clock_period_ended`) and runs the derived
 *    `FUN_0008B9CC` chooser `fifa96_action_phase_period_end` for the completed
 *    period (`state.period - 1`: the FU-62 library already incremented the
 *    period at completion, while the native calls the chooser before
 *    `[0x57AC2]++` at `0x8B58A`);
 *  - writes the derived phase through `fifa96_match_state_set_phase` and
 *    mirrors `phase_machine.state`/`phase` (the native `[0x157A4D]` switch
 *    byte is the high byte of the `[0x157A4A]` phase dword).
 *
 * With the extra-time flag clear (period < 4) the chooser derives phase 0x0C on
 * the controlled side and act 0xB (`0x8BAA7..0x8BABE`, `0x8BADB..0x8BAE7`);
 * with the flag set it derives act 0xC / phase 0x13-0x14 (FU-213 §3). The
 * native then invokes the act selector. Since T1 (FU-213) the flag
 * `[0x157AC0]` producer (`mr->global_157ac0`) is LIVE — the score tie-break on a
 * completion plus the W5 mid-extra-time set — so the chooser receives it; the
 * act handler body and the phase-0xC machine `FUN_0008BAF0` stay unported
 * (FU-143 §4/§5 legs). The `FUN_0004B02C(0)` period-end producer
 * (`fifa96_match_run_period_setup(mr, 0)`, native `FUN_00088860`) now runs at
 * the TOP of every `phase_drive` — the native every-frame cadence (M2 phase-21
 * T4, FU225-L5/D10); its `state.phase == 0x0C` and `global_157ac2 == 4`
 * self-gates select sit-1/sit-8 and may write a phase outside the chooser path.
 * The engine lifecycle owns the exit and the 0x0C -> 0 reset is the run_end
 * teardown (`fifa96_match_state_init`).
 *
 * The chooser path writes no phase for a class-0 phase, without a staged
 * completion, or when the chooser derives `FIFA96_ACTION_PHASE_NONE`. Returns 1
 * when the derived chooser ran (a phase is written only when its output is not
 * `FIFA96_ACTION_PHASE_NONE`), 0 otherwise, -FIFA96_ERR_INVALID (NULL), or a
 * -fifa96_err_t from the drivers. */
int fifa96_match_run_phase_drive(struct fifa96_match_run *mr);

/* The derived `FUN_0008A938` situation dispatcher's table-2 row (M2
 * playable-match Task 2 / OL-84 residual; FU-143 §3.2/§5; FU-149 P1 shared
 * entry). Runs the derived row for `situation` 0x00..0x0C
 * (`fifa96_action_phase_situation`), applies its phase outcome through
 * `fifa96_match_state_set_phase` (the native `FUN_000740A0` write inside the
 * table-2 arms, e.g. situation 0xB -> 0x8AEF6 -> 0x8AF02 phase 2), mirrors
 * `phase_machine.state`/`phase`, and runs the FU-149 phase arm for the new
 * phase (`fifa96_match_run_phase_arm`: phases 3/4/6/7/8/9/0xD install their
 * taker/keeper codes — the native per-team `FUN_0008D098` call inside
 * `FUN_000740A0`). This is the **side-less shared entry**: row 01's
 * situation-0xB producer (`fifa96_match_handlers.c`), the goal fallback
 * `fifa96_match_run_goal_queue` (situation 6) and the FU-149
 * `fifa96_match_run_set_piece` direct path all route here; the dispatcher
 * head gates live only in `_set_piece`. Without a dispatcher side the sit-3
 * counter increment is not applied (the phase outcome still is). The
 * act-handler invocation (`FUN_000888FC`, the unported phase-row bodies)
 * stays unported (OL-73/OL-78); situations whose row carries no phase run
 * their ported act body only (sit 9/0xA start the FU-150 P2 act-2
 * free-kick/penalty hand-off; the act-only rows 1/8/0xC with unported bodies
 * are a no-op on the run). Returns 0,
 * -FIFA96_ERR_INVALID (NULL `mr` or situation >= 0x0D), or a -fifa96_err_t
 * from the phase setter. */
int fifa96_match_run_situation(struct fifa96_match_run *mr, uint8_t situation);

/* FU-149 §1.1/§3 item 2 (P1): the full `FUN_0008A938` dispatcher head
 * (`0x8A938..0x8AC27`) for one situation/side/BX triple, native semantics
 * first-hand this slice:
 *  - head gates (`0x8A944..0x8A96B`): situation 0 and 0xB, a closed session
 *    gate (`!session_gate_14c32a`) or a pending situation take the direct
 *    path; otherwise the table-1 queue arm jumps (`0x8A8E0`) and RETs:
 *    `sit_side_pending = (side == 0)` (0x8A982), the id from the queue table
 *    (2/3/4 -> 9 side 0 / 0 side 1; 5/7 -> 7; 6 -> 5/6; 9/10 -> 1 side 1 /
 *    2 side 0; 8, 1, 0xC and >10 -> 0xA via the `SUB ECX,2`/`CMP CX,8`
 *    default) and `situation_pending = 1`;
 *  - direct path BX != 0 (`0x8AA80..0x8AAA3` + act 8's tail
 *    `0x8A8A5..0x8A8DE`): `store_15882c = side`, `store_15882b = situation`,
 *    phase 0 (`FUN_000740A0(0, 0)`), then the stored situation/side are
 *    re-dispatched with BX=0 and `store_15882b = 0xFF`. The act-8
 *    (phase-0x1E) timeline stages around the re-dispatch stay unported
 *    (FU-149 L6), so the port compresses them to the re-dispatch call;
 *  - direct path BX == 0 (`0x8AAA8..0x8AB7A`): the table-2 route through the
 *    shared `fifa96_match_run_situation` entry; situation 6 keeps its score
 *    fallback (`fifa96_match_run_goal_queue`), situation 3 increments the
 *    corner counter `side ^ side_swap` (0x8ABFF/0x8AC0F) before the write.
 *    The native sit 2..4 `[0x157B8E]`/`[0x157B8F]` formation-order gate and
 *    its act-4 arm (`0x8AB08..0x8AB62`), and the sit-1 arm
 *    (`0x8AABF..0x8AB06`), are unported act-handler machinery (FU-149 L12);
 *    the derived route is always the table-2 row.
 * The dispatcher's queue ids are consumed by the FU-146 S3 screen machinery
 * (the L4 boundary); the head only writes them. Returns 0,
 * -FIFA96_ERR_INVALID (NULL `mr` or side > 1), or a -fifa96_err_t (the direct
 * path for situations >= 0x0D keeps the hardened table rejection). */
int fifa96_match_run_set_piece(struct fifa96_match_run *mr, uint8_t situation,
                               uint8_t side, uint8_t bx);

/* FU-149 §1.5/§3 item 4 (P1): the `FUN_0008D098` phase-arm subset for the
 * set-piece phases 3/4/6/7/8/9/0xD (first-hand arm table `0x8D040` and the
 * decompiled switch; `0x8D2A3`/`0x8D35B`/`0x8D4A2`/`0x8D57B`/`0x8D5D9`/
 * `0x8D65D` re-read this slice). Runs per team (`[team+0x826] ==
 * phase_machine.side_controlled` is the controlled team):
 *  - both teams: action 3 over records 0..10 through the `FUN_0008CEB8`
 *    multi-install (`fifa96_match_arm_install_multi`; index-0 3 -> 0x19
 *    pre-coercion);
 *  - phase 3: camera reset to the goal snapshot triple (`0x15777C`), the
 *    controlled team's nearest record to that triple (`FUN_00079CCC` derived
 *    as the pool nearest over the records' targets with skip 0, fallback
 *    record 0) becomes `team->target` and gets action 0x10;
 *  - phase 4: probe = `FUN_0007D360(snapshot)` = `(±0x710, 0, ±0xB00)` from
 *    the snapshot sign bits, camera reset to it, the nearest record gets
 *    action 0x11; `(FUN_00092AC8 & 3) != 0` fires the dropped event sink
 *    (L7) but the RNG draw is still consumed;
 *  - phase 6: probe = `FUN_00073DC4(team side)` = `(0, 0, ±0x8D0)`, camera
 *    reset to it, the controlled team's nearest gets action 0x13; the
 *    non-controlled team installs action 0x1F on its record 0 and
 *    `[team+0x7B2] = record 0`;
 *  - phase 7: probe = the incident triple (`incident_x`/`incident_z`, the
 *    FU-150 producer — P2), the nearest gets action 0x12;
 *  - phases 8/9: `team->target = record 0` and action 0x1D (8) / 0x1E (9)
 *    on it;
 *  - phase 0xD: controlled team installs action 0 over records 1..10
 *    (record 0 kept), the other team over 0..10;
 *  - the per-event sinks `FUN_0008F188(0x17/0x1A, …)` stay dropped (FU-149
 *    L7) and the non-controlled team early-returns for phases 3/4/7/8/9.
 * Phases outside the subset are a no-op (the native `FUN_0008D098` cases
 * 0/1/2/5/0xB/0xC/E/F/0x10/0x13/0x14/0x15 stay their own ports). The pool
 * phase is latched to the arm phase before the installs (the native
 * `[0x157A4A]` write precedes `FUN_0008D098`). Returns 0 or
 * -FIFA96_ERR_INVALID (NULL). */
int fifa96_match_run_phase_arm(struct fifa96_match_run *mr, uint8_t phase);

/* FU-191 §2/§9 (M2 phase-17 T1): the native per-record phase walk
 * `FUN_0008D098`'s loop (`0x8D0F2..0x8D166`) — the second caller of the phase
 * table (FU175-L1). For `team`'s 11 records (stride `0xB2`), skip the
 * occupied-record exclusion `[rec+0x9A] != 0` (`0x8D0F7`), then call the phase
 * row's handler through `fifa96_match_dispatch_phase_record(mr, phase, rec, -1)`
 * (the native `0x8D107 CALL 0x6D920` stages `[rec+0x1C] = 0x110794[phase]` and
 * `0x8D110 CALL dword[ECX+0x1C]` invokes it with the EAX=rec/EDX=&rec[0x4D]/
 * EBX=-1 ABI). `match_run_write_phase` calls it for both teams after every phase
 * write (the native `FUN_000740A0` `0x740C8`/`0x740DB`), **gated on
 * `fifa96_match_phase_table[phase].fn != NULL`** so it is a structural no-op
 * until a per-record body lands (FU-191 §10; the gap vs the native
 * unconditional `FUN_0008DCD4` distance write is FU191-L2). `phase` out of the
 * table, or `team >= 2`, -> -FIFA96_ERR_INVALID; otherwise FIFA96_OK (per-record
 * handler errors are absorbed: the native walk ignores the handler return). */
int fifa96_match_run_record_phase_walk(struct fifa96_match_run *mr, uint8_t phase,
                                       uint32_t team);

/* FU-196 §3/§4 T2 rec (M2 phase-17 T2): the native `FUN_0004B02C` situation
 * producer — the match-init / period-end machine the engine lacked. `mode == 1`
 * is the `FUN_000493A0` match-init arm (0x49415): after the derived restart
 * sinks, dispatch situation 0 (`fifa96_match_run_set_piece(mr, 0, 0, 0)` — the
 * 0x4B0A5 mirror → table-2 sit 0 = phase 0x11 + act 0xA). `mode == 0` is the
 * `FUN_00088860` period-end arm (0x88860): gated on `state.phase == 0xC`, when
 * `global_157ac2 == 4` (extra time) dispatch situation 8 with the RNG side
 * (`fifa96_rng_step` & 1, the 0x888C8 mirror), else situation 1 on the
 * controlled side (the 0x888F2 mirror). Both modes then apply the extra-time
 * gate (`(input_range[0] | input_range[1]) & 0x0100`, the 0x4B0CB `TEST AH,1`):
 * on set, dispatch situation 8 (side 0) and `global_157ac2 = 4` (0x4B0D9).
 * The `FUN_00074034`/`FUN_0007412C`/`FUN_0007417C`/`FUN_0008CBB0`/`FUN_00078824`
 * restart/refresh sinks, the `[0x157A5C]`/word[0x157A62] prefix clears and the
 * `FUN_00068822`/`FUN_000CB2B6` tails stay numbered legs (FU196-L2/L6); the
 * extra-time bit 8 semantics are FU196-L1. This is producer-only: it sets no
 * table slot and installs no row (R45/R9). Returns 0, or -FIFA96_ERR_INVALID on
 * NULL. The live `begin` caller is gated by `mr->period_setup_live`. */
int fifa96_match_run_period_setup(struct fifa96_match_run *mr, int mode);

/* FU-239 §1/§3 (M2 phase-22 T1): the period-over re-arm dispatcher — the native
 * `FUN_0004B02C(mode 0)` -> `FUN_00088860` chain that re-arms the next half out
 * of the class-0 period-end park. `FUN_00088860`'s gate is the current phase
 * `0x0C` (`0x8886B`); the discriminator `0x888B0 CMP [0x157AC2],4` selects the
 * extra-time arm (sit-8, act 7, row `0x1D`, `0x888C8`) when the period index is
 * 4, else the half-time arm (sit-1, act 1, row `0x17`, `0x888F2`). Both act
 * bodies write a class-0 kickoff staging phase (`1` / `0x10`) and the raw
 * kickoff restart then reaches the class-1 **play phase `2`** (`0x8AEFA MOV
 * EAX,2`; the only class-1 phase with any static writer, FU-239 §3). The engine
 * collapses the staging frames to the play phase; the exact `0x13/0x14 -> 2`
 * route is the unprovable leg **FU239-L1** (never faked). Sets `phase = 2` and
 * `period_seconds = 0`; NULL is a no-op. */
void fifa96_match_run_period_over_rearm(struct fifa96_match_run *mr);

/* FU-231 §3/§6.1 (M2 phase-22 T2): the minimal port of the `FUN_0008BAF0`
 * end-machine terminal decision, over the live cells. Returns the end word
 * `[0x158822]` as a pure function of the run's phase/flag/timer:
 *  - `1` = regulation-over: the flag-clear phase-`0xC` terminal (`0x8BCBF CMP
 *    byte[0x157AC0],0` / `0x8BCD9 MOV word[0x158822],1`);
 *  - `2` = extra-time-over: the phase-`0x13`/`0x14` arm's dwell with the end
 *    timer `word[0x158818] > 0x4B0` (the arm's `CMP EAX,0x4B0` at
 *    `0x8BD52`/`0x8BDA4`; the `word[0x158822]=2` write is
 *    `0x8BD8A`/`0x8BDD4`, latched at `0x8BDDD`). The
 *    native `CMP EAX,0x4B0 ; JG` is a SIGNED compare of the sign-extended word
 *    (`MOV EAX,[0x158816] ; SAR EAX,0x10`), so `mr->act.timer` is compared
 *    signed;
 *  - `0` = none.
 * The phase-`0xC` flag-set arm (`0x8BCC8` -> phase `0x13`) and the no-ET
 * `[0x105FFC]=5` screen (`0x37F54`) are not end-word values; the latter has no
 * engine cell (FU-231 §5 / leg FU231-L2). NULL -> 0. The frame body stages the
 * cell once, gated on the past-half-time index and the FU-235 dwell (FU-234-L7),
 * never the raw `period_ended`. */
uint16_t fifa96_match_run_end_word(const struct fifa96_match_run *mr);

/* FU-232 §4.1/§4.3 (M2 phase-22 T3): the native frame-loop over-condition of
 * the end-word consumer `FUN_0004B100`. `mark_over` (`[0x5FFC]=2`, the
 * `0x4B244 MOV EAX,2 ; CALL 0x36BC0` site) fires iff the end word
 * `word[0x158822]` is `1` (regulation terminal) AND the period index
 * `byte[0x157AC2]` is `>= 2`, except at index `4` with the extra-time flag
 * `byte[0x157AC0] != 0` (`0x4B237 CMP byte[0x157AC0],0 ; JNZ` -> no write). Word
 * `2` (the ET-over arm at `0x4B257`) is NOT a match-over — FU-232 §3: it writes
 * no `[0x5FFC]` screen and clears the word (`0x4B297 MOV word[0x158822],SI`), so
 * this returns 0 for it and the frame loop consumes the word instead. A pure
 * reader over the live cells; NULL -> 0. This is the single source of truth for
 * the `fifa96_match_run_frame` tail. */
int fifa96_match_run_should_mark_over(const struct fifa96_match_run *mr);

/* FU-197 §4/§9 item 2 (M2 phase-17 T2): the `FUN_0008C974` formation-order
 * sorter + residual counter for one team. Computes the display side index
 * `(team.side ^ side_swap) & 1` (the `0x741B4` 0x8C98B..0x8C998), selection-
 * sorts `formation_current[side]` to match `formation_order[side]` (the
 * `0x8C9A6..0x8CAF8` loop), and stores the post-sort residual into
 * `formation_mismatch[side]` (the `0x8CB1C..0x8CB5E` count). Gated on
 * `mr->formation_order_ok`: 0 (the live default) is a no-op, because the
 * desired matrix's front-end producer plus the reset copy leave
 * `current == desired` (residual 0) and sorting the zero stand-in is a fake
 * (FU201 §4/§6, R49). The companion `0x157B90` bytes / `0x157BA6` words, the
 * `0x8C758` entity re-link and the `0x4728C` placement-commit sink stay
 * FU197-L2 legs. Returns 0, or -FIFA96_ERR_INVALID (NULL or team > 1). */
int fifa96_match_run_formation_sort(struct fifa96_match_run *mr, uint32_t team);

/* FU-184 §5/§7 T2-2 (phase-16 T2): the shared kickoff phase entry, factored
 * out of `fifa96_match_run_begin` (`:2213-2224`). Runs the native `0x740A0`
 * sequence for the kickoff phase 1: the phase-1 latch
 * (`match_run_write_phase`), the per-team `FUN_0008D098` state-1 arm
 * (`fifa96_match_phase_machine_kickoff`) and the entity drain that consumes
 * the arm's slot-merge request. begin calls it between the formation seed/slot
 * bind and the `FUN_00073E08` placement commit, matching the native order; the
 * row-0x14 celebration release calls it to continue past a goal (native
 * `0x87C71`). M2 phase-18 T4 (FU-203): `begin` no longer calls it directly — it
 * enters through `fifa96_match_act_dispatch(mr, 1, 0, 1)` (the phase-0x17 body
 * stage 0 runs this entry); the row-0x14 release still calls it directly (the
 * derived release is not an act-1 dispatch). Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID (NULL). */
int fifa96_match_run_kickoff_entry(struct fifa96_match_run *mr);

/* ===== FU-150 P2 — fouls / referee / offside =============================== */

/* The derived machine dispatcher slots (`mr->ref_machine`). NONE = idle; the
 * surviving module sequence maps to its FIFA96_REF_SEQ_* value. The former
 * RESTART slot (the act-2 phase-0x18 free-kick/penalty hand-off) is RETIRED
 * (FU-183 §5 / phase-16 T1): the phase-0x18 row `fifa96_match_action_phase_18`
 * is owned by the act machine (`fifa96_match_act_dispatch`/`_tick`), so there
 * is no second driver (the retired FU175-L7 double-run). The former FOUL slot
 * (the act-3 phase-0x19 foul sequence) is RETIRED by the same rule (FU-187 §5 /
 * phase-16 T4): the phase-0x19 row `fifa96_match_action_phase_19` is owned by
 * the act machine, dispatched invoke-now from `fifa96_match_run_contact`. */
#define FIFA96_MATCH_RUN_REF_NONE 0u
#define FIFA96_MATCH_RUN_REF_OFFSIDE FIFA96_REF_SEQ_ACT6

/* FU-150 §Port contract: the derived contact entry. The native contact
 * registrar FUN_0008A3FC is called from the action-0x0C body (unported,
 * FU-137/OL-9), so a contact is a caller-staged input (the deepest-reachable
 * honesty rule): `rec_a`/`rec_b` are encoded entity ids and `point` the
 * 12-byte contact triple (NULL -> the module's zero triple).
 *
 * Runs the registrar, then the row-0x0C re-call path (0x81E8A..0x81ED5,
 * first-hand): behind `config.field_4c306 != 0`, a valid rec_a and a first RNG
 * draw with `(AL & 7) != 0`, calls `fifa96_ref_foul_decide` with the row's
 * literal entry kind 1 (the staged `referee.contact_kind`) and a second draw
 * supplied as the severity `rng_bits` (only consumed when field_4c306 > 1),
 * then writes the [0x15888E] re-call flag (`recall_consumed`). Outcome: an
 * ACT3 decision dispatches act 3 invoke-now through the act seam
 * (`fifa96_match_act_dispatch(mr, 3, 0, 1)`; the native `FUN_000888FC(3,0,BX=1)`
 * at `0x8A6FE`), which runs the phase-0x19 stage 0 immediately (FU-187 §5 /
 * phase-16 T4 — the former ad-hoc FOUL machine is retired); a severity-0
 * decision routes situation 9 (fouled side, BX=1) through the shared
 * `fifa96_match_run_set_piece` entry (which, on the direct path, starts the
 * act-2 phase-0x18 machine). Mirrors the clamped incident triple into
 * `incident_x`/`incident_z`. Returns 1 when the decision ran, 0 when a gate
 * skipped it, or -FIFA96_ERR_INVALID (NULL mr / invalid rec_a). */
int fifa96_match_run_contact(struct fifa96_match_run *mr, uint8_t kind,
                             int32_t rec_a, int32_t rec_b, const int32_t point[3]);

/* FU-150 §Port contract: the derived reception-time offside check. The native
 * caller is the reception handler FUN_0007A084 0x7A448, ported live into
 * `fifa96_match_run_ball_receive` (FU276-L2, phase-26b Task 5) behind the
 * [0x14C2F2] `field_4c2f2` enable; this entry stays callable for the direct
 * geometry tests. `receiver` is an encoded entity id, `metric` the staged
 * 0x158738 block (the arm passes the receive vector's +0/+4 words) and
 * `camera_ref`/`mirror` the [0x157754]/[0x157823] staged camera inputs
 * (FU-150 risk note). The engine
 * resolves the own-team nearest (FUN_0008DE8C derived as the pool nearest to
 * the ball triple, skip index 0) and the opponent last defender
 * (FUN_0008DE28 derived as the opponent nearest to (0, ±0xB10)) and stages the
 * record-state gate (code != 0x11, code not in {0x10,0x1D,0x1E}).
 *
 * Behind the phase/suppression/settings pre-gates it draws one RNG value (the
 * native FUN_00092AC8 tolerance draw; the native draws it after its geometry
 * gates, so the derived stream may differ in gate-failure cases, leg 2) and
 * runs `fifa96_ref_offside_check`; on a fired check it stores the kind-3 event
 * on the own-nearest record (first-hand 0x79F29 `EDX = ESI`; the slice's
 * "receiver record" reading is the recorded erratum) via
 * `fifa96_ref_offside_event`, mirrors the incident triple and starts the
 * phase-0x1C machine with stage 0 run immediately. Sets `*offside`. Returns 1
 * when the event fired, 0 otherwise (gates or no offside), or
 * -FIFA96_ERR_INVALID (NULL mr/offside or invalid receiver). */
int fifa96_match_run_offside_reception(struct fifa96_match_run *mr, int32_t receiver,
                                       const struct fifa96_ref_metric *metric,
                                       int32_t camera_ref, uint8_t mirror,
                                       uint8_t *offside);

/* Run the active referee machine once (one granted 30 Hz frame; the frame body
 * calls this after the FU-141 entity chain). NONE -> 0. OFFSIDE runs the module
 * step and applies its requests: the whistle/speech observation slots
 * (`ref_whistle`/`ref_speech`), the phase write through
 * `match_run_write_phase` + the FU-149 arm, the record install on rec_first,
 * and the situation dispatch through the shared
 * `fifa96_match_run_set_piece(..., BX=1)` (whose direct path may start the
 * act-2 phase-0x18 machine, run invoke-now). Also drives the [0x157A6A]
 * suppression countdown (0x7438D..0x743A5, saturated at 0) and stages the live
 * phase into the referee state. Returns 1 when a step ran, 0 when idle, or a
 * -fifa96_err_t (including -FIFA96_ERR_INVALID for NULL). (The former RESTART
 * arm is retired — FU-183 §5 / phase-16 T1: the act machine owns the
 * phase-0x18 hand-off `fifa96_match_action_phase_18`; the former FOUL arm is
 * retired — FU-187 §5 / phase-16 T4: the act machine owns the phase-0x19 foul
 * sequence `fifa96_match_action_phase_19`.) */
int fifa96_match_run_referee_step(struct fifa96_match_run *mr);

/* FU-183 §5/§6 rank 2 (phase-16 T1): the phase-0x18 / act-2 row body. Runs the
 * derived free-kick/penalty hand-off (`match_run_ref_step_restart`): stage 0
 * writes phase 0xA on the fouled side (whistle when the foul kind is 0), the
 * stage 1/2 gates run the FK/penalty decision (phase 7 default, phase 6 in the
 * fouler-side spot band) and arm the FU-149 taker; the terminal clears the act
 * slot. The act machine owns the row (the sit-9/sit-0xA producer dispatches act
 * 2 invoke-now; the clock tail ticks it). Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for NULL. */
int fifa96_match_action_phase_18(struct fifa96_match_run *mr);

/* FU-187 §2/§5 rank 7 (phase-16 T4): the phase-0x19 / act-3 row body. The
 * native handler `0x89FA4` is the 7-stage foul sequence (`fifa96_ref_foul_
 * sequence_step`); the derived body aliases `mr->act.stage/timer` <-> the
 * sequence (`referee.stage/timer`) so the native head add (`[0x158818] +=
 * [0x157A64]`) is performed exactly once (FU187-L3), applies the requests
 * through `match_run_ref_apply` (stage 0 writes phase 0xF + installs action
 * 0x16 on rec_first; stage 6 the situation-0xA re-dispatch) and clears the act
 * slot before the terminal situation (the native 0x8A3D0/0x8A3D6/0x8A3EA
 * order). The act machine owns the row (the foul decision dispatches act 3
 * invoke-now; the clock tail ticks it). Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for NULL. */
int fifa96_match_action_phase_19(struct fifa96_match_run *mr);

/* FU-188 §2/§6 (phase-16 T4): the phase-0x20 / act-0xA row body (the code-0x24
 * timeline). Its phase-table slot (`fifa96_match_phase_table[0x20]`) is **NULL**
 * — the row is not counted — because its only producer is situation 0, which no
 * engine source/test dispatches (FU-188 §6). The body is derived/ported and
 * exported so it is tested directly and ready for the situation-0 producer
 * (FU188-L1). Derived stages: head `mr->act.timer += frame_delta`; stage 0
 * installs code 0x24 on both teams' 11 records (the native `0x8B750`/`0x8B78B`
 * invoke-now — the generic invoke has no engine port, FU188-L4, so only the
 * install lands) and advances; stage 1 the `0x974D8(0x3E8)` wait; stage 2
 * clears the act slot and re-dispatches situation 1 (`0x8B85D`). The timeline
 * cells (`0x15880C..0x158852`) and the `word[0x15881A]`/`0x45001` gates are
 * FU188-L2 legs. Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL. */
int fifa96_match_action_phase_20(struct fifa96_match_run *mr);

/* FU-193 §2/§6.1 (M2 phase-17 T4) + FU-203 (M2 phase-18 T4): the phase-0x17 /
 * act-1 row body — the kickoff / phase-1 entry (native `0x88DC8`, ends
 * `0x88F39`). Its phase-table slot (`fifa96_match_phase_table[0x17]`) is
 * **WIRED and counted** (M2 phase-18 T4): the producer is live — the phase-0x20
 * body's stage-2 terminal re-dispatches situation 1 (`0x8B85D`), the loader row
 * arms act 1, and the clock tail runs this body. The begin kickoff is
 * re-expressed through the same route (`fifa96_match_act_dispatch(mr, 1, 0, 1)`),
 * so the ad-hoc direct call is retired. `0x110794[0x17] == 0x1107EC[1] ==
 * 0x88DC8` (the phase/act dual identity; the sit-1 producer `0x8ABCF`). NOTE:
 * this PHASE row 0x17 is distinct from ACTION row 0x17 (the act-4 formation-arm
 * installer, NULL/leg) — FU-203 §2.4. Derived stages: head
 * `mr->act.timer += frame_delta` (0x88DD7); stage 0 clears `[0x15882A]`
 * (0x88E56) and runs the derived kickoff entry (the `FUN_000740A0(1)` phase-1
 * write), timer=0, stage->1; stage 1 hands off to act 4 (0x88EE8) when the
 * goal-side flag `[0x157A4C] != 1` and a residual formation mismatch exists,
 * else the shared timeline gate `word[0x15881A] >= 0x78` (derived
 * `state.tick_total`) sets `[0x15882A] = 1` and clears the act slot. The head
 * guard (`0x88DFC` goal-log words `[0x157AC5]`/`[0x157AC7]` + `[0x157AC2]`)
 * and the `0x4C374`/`0x700F4`/`0x73E28`/`0x73E08`/`0x4C324`/`0x4C31C` sinks are
 * FU193-L7 legs. Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_17(struct fifa96_match_run *mr);

/* FU-193 §3/§6.2 + FU-197 §6/§9.4 (M2 phase-17 T4): the phase-0x1A / act-4 row
 * body — the formation-order / line-up walk (native `0x89620`, ends `0x897F1`).
 * Its phase-table slot (`fifa96_match_phase_table[0x1A]`) is **NULL** — the row
 * is not counted — because the engine has no act-4 producer: the T2 sit-2..4 arm
 * is gated dormant on the desired-order matrix and the act-1 stage-1 hand-off is
 * itself unwired (FU193-L3). FU-201 §6 (R49) proves the arm is not live even
 * under a faithful matrix fill: the fill + the `FUN_00073EE0` reset copy leave
 * `current == desired` (a permutation residual of 0), and the genuine trigger is
 * a runtime non-permutation (FU201-L1) with no writer; un-gating on the fill
 * alone would be a no-op or a fake producer. So the row stays a **staged leg**
 * (FU201-L4).
 * Derived stages: head `mr->act.timer += frame_delta` (0x89629); stage 0 walks
 * the records for the first `formation_current != formation_order` mismatch
 * (occupied `skip_9a` records accept the current order, 0x896D7), and on an
 * active record writes phase 0xB (0x8972F) and installs action row 0x17
 * (0x89740), timer=0, stage->1; the end block (0x897AE) clears the act slot and
 * re-dispatches the stored situation/side (`store_15881e`/`store_158820`, BX=1).
 * Stage 1's gate (`[rec+0x8E]` -> `[0x110680]`, the `word[0x15881A] > 0xF0`
 * timeline) is derived on `state.tick_total`; the `FUN_0007D9A4` invoke-now and
 * the `0x8F188`/`0x4C374`/`0x4C31C` sinks stay FU197-L6 legs. The walk is gated
 * on `mr->formation_order_ok` (0 live = dormant no-op; a fake install is
 * forbidden). Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_1A(struct fifa96_match_run *mr);

/* FU-193 §4/§6.3 (M2 phase-17 T4): the phase-0x1C / act-6 row body — the
 * pre-restart timeline (native `0x89110`, ends `0x89213`). Its phase-table slot
 * (`fifa96_match_phase_table[0x1C]`) is **NULL** — the row is not counted —
 * because no engine source dispatches act 6 (FU193-L4); the existing driver is
 * the offside machine (`fifa96_match_run_offside_reception` ->
 * `fifa96_ref_offside_sequence_step`), so wiring the row would double-run it.
 * The native handler is already ported as `fifa96_ref_offside_sequence_step`
 * (FU-150/FU-165); this body aliases the act slot's stage/timer to the referee
 * state (the phase-0x19 FU187-L3 pattern), runs the 3-stage step (stage 0 writes
 * phase 0xA + whistle, stages 1/2 wait on `0x4BEC8`, stage 2 re-dispatches
 * situation 9 on the opponent), clears the act slot on the terminal and applies
 * the requests through `match_run_ref_apply`. Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_1C(struct fifa96_match_run *mr);

/* FU-194 §3/§6.1 (M2 phase-17 T5): the phase-0x1D / act-7 row body — the native
 * 10-arm set-piece timeline `0x89868`. Its phase-table slot
 * (`fifa96_match_phase_table[0x1D]`) is **NULL** — the row is not counted — and
 * it forms one coupled unit with action row 0x20 (FU-194 §6.1): arm 0 carries
 * the row-0x20 installer `0x899A2` (`MOV EDX,0x20; CALL 0x7D9A4`, deferred),
 * but the route needs the act-7 producer (situation 8), which the engine reaches
 * only through the T2 `fifa96_match_run_period_setup` (+ `period_setup_live`),
 * which is **gated off** (R9 / FU196-L8) — so neither the phase-0x1D nor the
 * row-0x20 slot is producer-live and both stay NULL (leg FU194-L1/L8). Derived
 * stages: head `mr->act.timer += frame_delta` (0x89882/0x89899); stage 0
 * (0x898B4) clears `[0x15882A]` (0x898B4), writes phase 0x10 when
 * `state.phase != 0x10` (0x898B6..0x898DD), installs action row 0x19 on the
 * **team base** (`records[0]`, once per team — the native `0x8990A` CALL sits
 * outside the 11-record ESI loop), timer=0, stage->1; stage 1 (0x899EE) fires
 * the whistle `0x974DC(0x1E)` and sets `[0x15882A] = 1` once the shared timeline
 * reaches 0x78 (`state.tick_total`); stage 9 (0x89E6E) is the terminal (no
 * situation re-dispatch): clear the act slot and `[0x157A4C]`. The coupled
 * row-0x20 installer, the ball row-0x1F install (0x899CD) and the `0x7A028`
 * reset are FU194-L8/L7 legs (the actor `[0x1587D4]` / ball-record producers are
 * unported; a fake install is forbidden); arms 2..8 (the
 * `0x651F0`/`0x974F0`/`0x45001`/`0x4BEC8`/`0x4C374` sinks and the arm-8
 * selection block `0x89BB6..0x89E6D`) stay FU194-L3/L7 legs. Returns FIFA96_OK,
 * or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_1D(struct fifa96_match_run *mr);

/* FU-241 §6.3 (M2 phase-23 T4; formerly FU-194 §4/§6.2): the phase-0x1E / act-8
 * row body — the native 2-team x 11-record restart sweep `0x8A798`. Its
 * phase-table slot (`fifa96_match_phase_table[0x1E]`) is now REGISTERED/COUNTED
 * (the de-compression): `fifa96_match_run_set_piece` BX!=0 dispatches act 8
 * invoke-now instead of the former compressed in-frame re-dispatch, and the
 * clock-tail tick drives this row. FU-241 §6.1/§6.2 proves the sweep
 * render-invisible + dormant on M2 (no state=3), so no presented frame moves.
 * Head: **no** timer add (the native head
 * reads the stage directly, 0x8A79B). Stage 0 (0x8A7B2) resets the walk cursor
 * (`act8_team`/`act8_record`) and falls into stage 1; stage 1 (0x8A7D6) sweeps
 * one record per call — team 0 records 0..10 then team 1 — clears
 * `act8_selected` before every gate test (0x8A814), and when a record's code
 * `[rec+0x91] == 0xC` with `byte[rec+0x99] != 0` and `byte[rec+0x9A] == 0`
 * stores it in `act8_selected` (0x8A831; the `0x4C374` `[0x157A68]` clear is the
 * FU194-L3 erratum leg), advancing to stage 2 after both teams; stage 2
 * (0x8A895) clears the act slot and re-dispatches the stored situation/side
 * (`store_15882b`/`store_15882c`, BX=0) then clears `store_15882b`. The
 * `0x4BEC8` wait gates are derived ready (the referee leg-10 precedent).
 * Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_1E(struct fifa96_match_run *mr);

/* FU-195 §2/§6.1 (M2 phase-17 T6): the phase-0x21 / act-0xB row body — the
 * native extra-time-CLEAR period-end timer (`0x8B874`, act slot `0x1107EC[0xB]`,
 * the FU165-L9 dual identity). This is the ONE counted row of T6: its producer
 * `FUN_0008B9CC` (`0x8BAE7 CALL 0x000888FC(0xB,0,BX=1)`) is mirrored live by
 * `fifa96_match_run_phase_drive` -> `fifa96_action_phase_period_end` (which
 * yields `out.act = 0x0B` when the extra-time flag is clear; the flag is live
 * since T1/FU-213) -> the act-selector
 * dispatch, so wiring `fifa96_match_phase_table[0x21]` is an honest +1. Derived
 * stages (raw disasm; the body is a table-only target with no Ghidra function):
 * head `mr->act.timer += frame_delta` (`0x8B884`, `word[0x158818] +=
 * word[0x157A64]`); stage 0 (`0x8B89E`) fires the `0x974DC(0x1E)` audio event,
 * resets the timer and advances to stage 1; stage 1 (`0x8B8C3`) waits on the
 * `word[0x15881A] >= 0x3C` gate (derived: the act timer, FU195-L4), then fires
 * the event and clears the act slot (handler/id/stage/timer); stage > 1 is the
 * native `0x8B89A` POP/RET no-op. The body touches only the act slot and the
 * `0x1E` sink (staged as `mr->ref_whistle`, the sibling phase-1C/1D convention;
 * the ring-push/audio-handle remainder is FU195-L3) — no record, no phase write,
 * so it is render-invisible. Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL
 * `mr`. */
int fifa96_match_action_phase_21(struct fifa96_match_run *mr);

/* FU-195 §3/§6.1/§7 (M2 phase-17 T6; updated by M2 phase-19 T1/FU-213): the
 * phase-0x22 / act-0xC row body — the
 * native extra-time-SET analogue (`0x8B900`, act slot `0x1107EC[0xC]`). Its
 * phase-table slot (`fifa96_match_phase_table[0x22]`) is still **NULL** — the
 * row is NOT counted — but the reason is now the slot, not the producer: the
 * OL-85 extra-time flag `[0x157AC0]` producer LANDED in T1 (`mr->global_157ac0`;
 * `fifa96_match_run_phase_drive` passes the live flag, so the chooser can yield
 * `out.act = 0x0C`), resolving the former producer-dormancy leg **FU195-L2**;
 * the body is not dispatched because the phase-0x22 table slot is registered by
 * a later task (T4). The body is the
 * three-stage form of `phase_21` (same head; stages 0/1/2 each gated on the
 * derived timer `>= 0x3C` after a reset; the terminal stage 2 clears the act
 * slot), exported for direct unit testing (the FU-185/FU-188/FU-193
 * "ported body, no live installer" precedent). Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_22(struct fifa96_match_run *mr);

/* FU-145 §1.4 (S2): the goal-mouth classifier `FUN_00070074`
 * (`0x70074..0x700F1`, 47 insns; first-hand this slice). Returns 1 iff the
 * triple sits inside the mouth band: `0xB10 <= |z|_w < 0xB90`,
 * `-0xD0 <= x < 0xD0` and `y <= h(z)` where `h = 0xA0` while
 * `|z|_w - 0xB10 <= 0x30`, else `0xA0 - (|z|_w - 0xB40)`. The native reads
 * the z comparison through the sign-extended low word of the 32-bit magnitude
 * (`MOVSX DX` at `0x70084`/`0x700C3`) and x/y as full dwords. Pure function. */
int fifa96_match_goal_zone(int32_t x, int32_t y, int32_t z);

/* FU-145 §3 item 1 (S2): the boundary pan armer `FUN_0007131C`
 * (`0x71390..0x713F7` arm body; the already-armed reflect arm
 * `0x718A9..0x7190E`). Called once per granted frame by
 * `fifa96_match_run_frame` when `fifa96_camera_out_of_bounds` holds (the
 * native track call gate `0x73B70..0x73B9B`). When disarmed and the phase is
 * 2/0x10 and `|camera.pos_z|_w > 0xB20 || |camera.pos_x|_w > 0x730` (the
 * native stores the absolute values as words and compares the sign-extended
 * low words), it sets `goal_armed`, freezes `goal_snap_*` := the camera triple
 * with y zeroed, and stores `goal_zone` = `fifa96_match_goal_zone`.
 * When already armed it runs the reflect arm: with `goal_zone == 0` and
 * `render.input_bit0` set it calls `fifa96_camera_reflect` and clears
 * `goal_armed`/`goal_zone` when the mirror fires (the native clears before
 * the mirror; zone != 0 or a clear input takes the unported angle arm, FU-71
 * leg 9.6, with no clear). The pan itself is the camera track (FU-71; the
 * pan producer is leg L1/S4); the fixture seam is the camera velocity pair.
 * Returns 1 when it armed or cleared, 0 for a no-op, -FIFA96_ERR_INVALID
 * (NULL). */
int fifa96_match_goal_arm(struct fifa96_match_run *mr);

/* FU-146 §7 item 1, landed here because FU-145's scanner calls it: the
 * `FUN_0008A938` situation-6 entry (`0x8A9E8` queue arm + the `0x8AC28`/
 * `0x8AC88` fallback). Since FU-149 P1 it delegates to the generic head
 * `fifa96_match_run_set_piece(mr, 6, side, 0)` — the single shared entry
 * (no parallel mechanism): with the session gate open (`session_gate_14c32a`)
 * and no pending situation the queue arm sets `situation_id` := 5 (side 0) /
 * 6 (side 1), `sit_side_pending` and `situation_pending`. Otherwise the
 * native fallback: the direct score increment (`fifa96_match_run_add_goal`)
 * plus the table-2 situation-6 phase-5 write through the shared
 * `fifa96_match_run_situation` entry. Returns 0, -FIFA96_ERR_INVALID
 * (NULL or side > 1), or a -fifa96_err_t from the fallback writers. */
int fifa96_match_run_goal_queue(struct fifa96_match_run *mr, uint8_t side);

/* FU-145 §3 item 2 (S2) + FU-149 §1.3 (P1): the clock-tail scan
 * (`FUN_0008AF38` `0x8B623..0x8B643` -> `FUN_00088940 0x88940..0x88C0E`).
 * Runs after `fifa96_match_run_phase_drive` in the frame loop. Gate: phase 2
 * or 0x10 and `goal_armed` (the native period-4 extra-time branch
 * `0x8B60D..0x8B621` skips the scan, but the engine's extra-time flag is
 * carried 0 per FU-143 OL-85, so the skip is unreachable and unmodelled —
 * leg L8). Scanner body:
 *  - `|goal_snap_z| <= 0xB20` -> the throw-in arm (`0x88BCC`, phase-2 only,
 *    `0x88BD4`): situation 2 with `EDX = ball_team ^ 1` and BX=1 through
 *    `fifa96_match_run_set_piece` (queue or the phase-0 + act-8 fallback);
 *  - `goal_zone == 0` -> the corner/goal-kick arm (`0x88B53`, phase-2 only,
 *    `0x88B5B`): situation `3 + ((snap z < 0) == (ball_team == 1))` with
 *    `EDX = ball_team ^ 1` and BX=0 (under the end convention `team 0 defends
 *    −z`: cond false -> sit 3 corner, true -> sit 4 goal kick);
 *  - else the goal arm: side from the snapshot sign (`0x889B6 SETL`; the
 *    `[0x157A4C]` goal-side flag / `[0x1587D4]` record arm is leg L4), the
 *    derived possession nearest search over that side's team block from the
 *    snapshot triple (`FUN_0008DE8C` with skip 0; the selected record feeds
 *    the unported announce/outcome sinks, leg L2/L3), then
 *    `fifa96_match_run_goal_queue(mr, side)`.
 * The `ball_team` source `[0x1577CA]` is the FU-145 L4 stand-in: the pool
 * controlled actor, then the ball carrier, else side 0 (the native record
 * identity is unported). The scanner sound `0x974DC(0x1E)` stays dropped
 * (FU-149 L7). Returns 1 when a situation arm ran, 0 for a gate no-op,
 * -FIFA96_ERR_INVALID (NULL) or a propagated -fifa96_err_t. */
int fifa96_match_run_goal_scan(struct fifa96_match_run *mr);

/* FU-146 §7 item 2 (S3): the goal-screen installer `FUN_00092D8C` +
 * `FUN_00092E2C` (`0x92D8C..0x92EFF`). Native register ABI: EAX = leg (the
 * `0x14AF7C` word), EDX = mode (`0x14AF74`), BX = side (`0x14AF60`); the
 * native callers are the front-end match-screen arms (FUN_00038630 0x38DCC,
 * FUN_0003BB1C 0x3BF9E) and `begin` installs the derived defaults
 * (leg 0 / mode 0 / side 0: FU-146 legs 1/3).
 *
 * Ported effects: `screen_leg`/`screen_mode` set, `screen_step` and
 * `screen_timer` zeroed, the installer latch `situation_pending = 1`
 * (0x92DCD), the score pair and `score_max_diff` zeroed (0x92E2C
 * [0x157AC5]/[0x157AC7]/[0x15B6A4]), the goal-log totals zeroed
 * ([0x15B698]/[0x15B69C]), `screen_period_frames` seeded from the per-mode
 * duration table `0x1110EC[mode*24+leg]` dwords × 60 (modes 0..3; the values
 * are `{15,15,30,30,60,5}` for modes 0..2 and `{5,1,5,1,3,2}` for mode 3),
 * `screen_install_hint = 0`, then the installed handler runs once (the native
 * `0x92EF7 CALL [0x15B6D4]`). The native tracked-side pick ([0x15B684] plus
 * the [0x1590CC]/[0x159901] flags) is leg 4/10: the carried -1 stands
 * untouched. The camera reset `FUN_000700F4`, the slot bind `FUN_00078824`
 * (covered by begin's `match_run_slot_bind`) and the score/log display cells
 * stay legs (FU-146 §8 leg 8).
 *
 * The native installer indexes `0x110F78` blindly; the port hardens (leg must
 * be 0..5, mode 0..3). Returns 0, -FIFA96_ERR_INVALID (NULL or out-of-range
 * leg/mode) or -FIFA96_ERR_STATE (run not live). */
int fifa96_match_run_screen_install(struct fifa96_match_run *mr, int16_t leg,
                                    int16_t mode, int16_t side);

/* FU-146 §7 item 3 (S3): the session-gated scheduler `FUN_000948AC`
 * (`0x948AC..0x949F7`), called once per granted frame from the frame body when
 * `session_gate_14c32a` holds — the native `0x4B198 CMP byte [0x14C32A],0 /
 * JZ / 0x4B1A1 CALL`, before the clock body (`0x4B1A6 CALL 0x8AF38`, the
 * engine's phase_drive + goal_scan). Arms, in native order:
 *  - `screen_timer > screen_period_frames` -> `situation_id = (leg != 5) ? 8 : 7`
 *    (+ latch);
 *  - leg 2 and the controlled record's team side != 0 and
 *    `screen_actor_age > 0xF0` -> id 3; the complement -> id 4;
 *  - leg not 2/4 and `camera.pos_z < 0` and `camera.vel_z < 0` -> id 9;
 *  - leg 5 and `camera.vel_z < 0` -> id 7;
 *  - tail: run the installed handler (`0x949E9 CALL [0x15B6D4]`).
 * The engine hardens the slotless run; the `[0x157A83]` reader resolves the
 * controlled id's team side (the native `byte[[[0x157A83]]+0x826]`). Returns 0,
 * -FIFA96_ERR_INVALID (NULL), -FIFA96_ERR_STATE (run not live), or a
 * propagated handler error. */
int fifa96_match_run_screen_schedule(struct fifa96_match_run *mr);

/* FU-146 §7 item 4 (S3): the installed period handler step machine — the six
 * `0x110F78` handlers (`0x93BBC`, `0x93E20`, `0x940A4`, `0x94270`, `0x944FC`,
 * `0x946C4`), table-driven per leg. The head adds the frame delta to
 * `screen_timer` and dispatches on `screen_step`; the per-leg kind table is the
 * native step table. Ported step kinds: the setup step (the `[0x15882A]` clear
 * via `global_5882a` for legs 0/1/3 — the `[0x15B684]==0` staged path — and the
 * leg-4 arm/snapshot clear), the `[0x15882A]` gate (legs 0/1/3), the phase-2
 * latch-clear step, the post step and the `> 0xB4` advance call. The post
 * consumes the pending situation: `goal_last_id`/`goal_minute`
 * ([0x15B674]/[0x15B678], leg 5 forces the minute 0), the per-leg id table maps
 * the queued id to a score side (or the no-score counter `goal_no_score`,
 * extended with `leg 5`: id 5 -> side 0, ids 1..6 else -> side 1) and runs
 * `fifa96_match_run_score_event` with the lazily-computed `FUN_000CBC4C` probe
 * (called exactly on the native `score[side] == 1 && score[other] < 3`
 * untracked arm), then the `FUN_000740A0(0,0)` equivalent (phase 0). The native
 * setup bodies' presentation/staging writes (the ±0x720 hint, the
 * 0x10F328/0x15B6C8/0x158897 copies, the `[0x15781D]` re-arm + snapshot, the
 * situation re-queues 0xC/3/0xA/4/2/1/0, `FUN_000974DC`, `FUN_0004C324`) stay
 * FU-146 §8 leg 8 (the duration is seeded by screen_install instead).
 * Returns 1 when the post consumed an id, 0 for a gate/step no-op, a negative
 * -fifa96_err_t. */
int fifa96_match_run_screen_step(struct fifa96_match_run *mr);

/* FU-146 §7 item 6 (S3): the `FUN_000935A0` advance/restart subset
 * (`0x9362B..0x9370A`). Appends the `{score_last_side, goal_last_id,
 * goal_minute}` triple to `goal_log` when `word[score0]+word[score1]` differs
 * from `goal_log_prev_total` (the native slot index is the new total; beyond
 * 20 the ring shifts entries 1..19 down and writes slot 19), records the
 * totals, zeroes `screen_timer`/`screen_step`, sets `screen_install_hint = 1`
 * and re-runs the installed handler (the native `0x93701`/`0x9370A` re-install
 * and call). The `0x9343C`/`0x937DC` screen installs, the 10/0x14 score
 * thresholds (`FUN_00037EC4`/`FUN_00037F0C`) and the
 * `FUN_0004C324`/`FUN_00054104` screen exits stay legs (FU-146 §8 leg 7).
 * Returns 0 or a negative -fifa96_err_t. */
int fifa96_match_run_screen_advance(struct fifa96_match_run *mr);

/* FU-146 §7 item 5 (S3): the deterministic six-limb probe `FUN_000CBC4C`
 * (`0xCBC4C..0xCBCB7`) over `goal_probe_limb` (C0..C5 at 0x112E68..0x112E7C).
 * Each call folds the limb chain into itself (`C5+C4` -> C4, then the ADC
 * cascade through C0), increments C5 and propagates the wrap carry up through
 * C0, returning the native EAX low byte (the writer tests `AL & 3`). Seed
 * bytes first-hand: `56 0e 2d f2 e9 26 31 88 2f dd 24 c6 9c c4 02 07 7d 3f
 * 35 9e 64 3b df 6f`. NULL returns 0. */
uint8_t fifa96_match_run_goal_probe(struct fifa96_match_run *mr);

/* One match presentation pass into the engine's indexed surface (Task 15):
 * clears the canvas to `render.background`, then recomposes the scene per the
 * FU-85/88/89 chain — FU-88 view matrix from `yaw`/`pitch`, reciprocal divide
 * (surface width/height), per-entity FU-85 §4 staging plus the FU-89 §7
 * jitter, FU-89 depth key seeding/sort and window clip, FU-85 resolver over
 * the caller-supplied FU-84 frame table/banks, the second overlay-frame pass
 * for the composite classes (FU-85 §2 second `FUN_00057080`), signed-scale
 * `fifa96_render_place` pivot placement and `fifa96_render_cover_rect`
 * clipping, and an indexed span blit through `render.remap` (0xFF
 * transparent) into `s->indexed`; the clip is clamped to `s` and presentation
 * stays `fifa96_surface_plane`'s job. Window scale (FU-93) is recomputed into
 * `window_scale_x`/`window_scale_y`/`window_zoomed` for the future HUD seam.
 *
 * Pure recomposition: the camera/display advance once per granted 30 Hz frame
 * in fifa96_match_run_frame, so calling this once per presented engine frame
 * (the MATCH dispatch does) is idempotent and hash-stable.
 *
 * Returns 0 (including the `enabled == 0` no-op), -FIFA96_ERR_INVALID (NULL),
 * or -FIFA96_ERR_STATE when enabled without the sprite assets (frames/banks/
 * sprite_data); no partial canvas is written on validation failure. */
int fifa96_match_run_render(struct fifa96_match_run *mr, struct fifa96_surface *s);

/* Stage the match sprite assets for rendering (M2 Task 2). Reads the two
 * caller-supplied ISO bank container paths through the run engine's mounted
 * asset table (`fifa96_asset_lookup` + `fifa96_cache_get`), decodes each to
 * its SHPI sprite banks (FU-86: a raw .fsh slice or a nested
 * huff(0x31)->refpack(0x10)->tree(0x46) .qfs chain; the first stage is fed the
 * container tail because the original huff reader is unbounded, FU-86 §3
 * caveat/leg 9), then fills `render.frames` (FU-84 §4 5-byte records: duration
 * 0x50, aux 0, sprite = frame index — the row-1 walk evidence; the table spans
 * the resolver's whole 128-index signed-byte domain so no accepted frame index
 * can read past it; the real per-row tables live in the executable's object-4
 * data, FU-84 §3.1/§4, not on the ISO, open leg), `render.banks` (one record
 * per BIGF entry, in container order; the FU-84 row +8 index selects the
 * player container's slots; an entry the port cannot decode stays an unloaded
 * NULL-handle slot, FU-85 §1.2; the FU-86 §4.1 stride switch is indexed
 * container-locally because only the player container's animator indices are
 * derived) and `render.sprite_data` (one owned arena backing frames/banks), and
 * sizes the FU-92 window to the full surface `s`. `render.enabled` becomes 1
 * only after the whole stage succeeds.
 *
 * Bank identity: FU-86 names art/playart.pvi as the player animation container
 * (91 banks, its data cross-check quotes /ART/PLAYART.PVI from the retail ISO)
 * and art/gameart0.pvi as the match art container (its cross-check quotes
 * /ART/GAMEART0.PVI); which of the second container's banks serve the pitch
 * stage is not derivable from the FU docs (numbered open leg), so the caller
 * supplies both paths, as the interface requires.
 *
 * Idempotent: a re-stage builds the replacement arena first and swaps it in
 * only on success. A failure leaves the run bit-identical (the previous stage,
 * if any, stays live). The arena is owned by the run and released by the next
 * successful stage, by begin (which resets presentation) and by end.
 *
 * Returns 0, -FIFA96_ERR_INVALID (NULL mr/s/path), -FIFA96_ERR_STATE (the run
 * is not live, or is not bound to a booted engine asset table),
 * -FIFA96_ERR_NOT_FOUND (a bank path is absent from the table), or
 * -FIFA96_ERR_UNSUPPORTED (a container is present but the port cannot reach
 * sprite banks in it). */
int fifa96_match_run_stage(struct fifa96_match_run *mr, const struct fifa96_surface *s,
                           const char *player_bank, const char *pitch_bank);

/* OL-T11-6 (M2 playable-match Task 1): the derived native match palette.
 *
 * The native match-data load FUN_00048ED8 (single caller FUN_0003BB1C 0x3BB45)
 * sources resource slot 0x32 = "PALsys.fsh" frame 2 (FUN_00048B60 0x48B6E..
 * 0x48B83 loads `FUN_0004AFB8(0x32)`, `FUN_000A1920(handle, 2)` and
 * `FUN_00047814`; slot 50 of the 0x107370 loader table is 0x101BA4 "PALsys"
 * with the 0x101C90 "%s.fsh" format) and builds the palette from the current
 * palette buffer 0x14B200 plus that chunk: the FU-98 kit remap
 * `pal[0x70F2[i]] = snapshot[0x70E8[i]]` over the 10 static table pairs, then
 * the chunk's [0xF0,+0x54) and [0x186,+0x4E) byte ranges copied over the base
 * (0x48F86/0x48F9F). FUN_00048C8C installs the result into 0x14B200 and
 * rebuilds the 8-bit copy at 0x14B800 through FUN_000479A0, whose conversion is
 * `v << 2` (0x479A8..0x479B9) -- not the v*255/63 sprite-palette scaling.
 *
 * `fifa96_match_palette_from_bank` extracts the frame-2 chunk from a
 * PALsys.fsh-shaped SHPI bank, applies the native transforms to `base6` (a
 * 768-byte 6-bit base palette; NULL selects the derived engine default
 * `base := the chunk`, which makes the native appends identity) and writes the
 * 8-bit RGB `v << 2` result to `rgb8`. The native base buffer 0x14B200 is the
 * previously installed front-end palette and is not statically derivable;
 * that substitution is the recorded derivation leg. The type-0x22 chunk must
 * hold exactly 256 entries (the native copies 0x300 bytes unconditionally; the
 * port hardens). Returns FIFA96_OK, -FIFA96_ERR_INVALID (NULL arguments or a
 * bank without frame 2), a negative fifa96_err_t from the SHPI/frame/chunk
 * parse, or -FIFA96_ERR_UNSUPPORTED when the chunk count is not 256.
 *
 * `fifa96_match_run_palette_install` applies the staged `render.palette` to
 * `s` (the native install target is the current palette buffer + DAC). Returns
 * FIFA96_OK, -FIFA96_ERR_INVALID (NULL), or -FIFA96_ERR_STATE when
 * `render.palette_ready` is 0. `fifa96_match_run_render` calls it before the
 * plane conversion whenever a palette is staged, so a presented match frame
 * carries the RGB palette; the pre-palette behavior (unstaged run) is
 * unchanged. */
int fifa96_match_palette_from_bank(const uint8_t *bank_data, size_t bank_len,
                                   const uint8_t *base6, uint8_t rgb8[768]);
int fifa96_match_run_palette_install(struct fifa96_match_run *mr,
                                     struct fifa96_surface *s);

/* FU-148 §4.2 (S4): the per-entity translation install. The native per-draw
 * consumer FUN_00048DC0(entity) runs when `[0x1068E0]==1` (the match-data
 * load) and entity is 0/0xB: it memmoves the entity's pool slot
 * (`0x14BF60[entity]` = FUN_00046F80's partition) and rewrites the kit band
 * members, then FUN_000CE980 copies the 0x100-byte result into 0x114720 (the
 * engine's `render.remap`). Every other entity copies its pool slot verbatim.
 * The engine's `[0x1068E0]` analog is `render.palette_ready`; the pool is the
 * caller-staged `render.palette_pool`. Returns 0, -FIFA96_ERR_INVALID (NULL
 * `mr` or entity > 22; the native indexes the partition blindly), or
 * -FIFA96_ERR_STATE (run not live, or no pool staged: the pool resource
 * identity is FU-148 leg 11, so the identity remap stays the stand-in). */
int fifa96_match_run_translation_install(struct fifa96_match_run *mr, uint32_t entity);

/* FU-148 §3 (S4): the formation-id writer `FUN_0008EA70(side, id)`:
 * `(&0x14C1E4)[side] = id` followed by the FUN_0007412C consumer (the
 * 0x11033A layout install; the engine models the id and exposes the layout
 * through `fifa96_match_formation_layout`). The native writer does not clamp;
 * the port hardens (side 0/1). Returns 0, -FIFA96_ERR_INVALID (NULL or side >
 * 1), or -FIFA96_ERR_STATE (run not live). */
int fifa96_match_run_set_formation(struct fifa96_match_run *mr, uint32_t side,
                                   uint8_t id);

/* FU-199 §7.2 / FU-200 §1/§2 (M2 phase-18 T1; native placement + per-field
 * corrected by the two-axis review): the match-begin OL-13 metadata producer —
 * the engine mirror of the native `FUN_0006D920` bind
 * (`FUN_0007412C -> FUN_0008CE78 -> FUN_0006D9C4 -> FUN_0006D920`). It loads
 * the formation id's `.fmt` family (`352pl`/`352pk`/`freekick` for id 0) from
 * the GAMEART0 bank into the run-owned `mr->meta_owner` and stages every
 * record's `phase_meta`/`phase_meta10`/`phase_meta14` with the native strides
 * (`idx*0x256` bytes, `idx*2` dwords, `idx*0x256` dwords; `idx = active`).
 * `fifa96_match_run_begin` calls it at the native position — after the
 * formation seed and before the derived slot bind and the screen-install
 * handler (native `FUN_00092e2c`: begin `FUN_00074034` -> bind `FUN_0007417c`
 * -> handler `0x92EF7 CALL [0x15B6D4]`), so the phase-0x15 write legitimately
 * sees bound metadata. **Per-field, per-side:** each side's three blobs copy
 * independently; a missing/short `.fmt` entry (or an out-of-range id) clears
 * only that side's own field's pointer, never the other side's or a sibling's —
 * so each row's `phase_meta*` NULL gate reflects exactly its own field. It
 * first clears the pool and any prior owner, then retains the owner iff at least
 * one pointer was staged, so no pointer ever dangles into freed memory.
 * Soft-fails (a pointer stays NULL) with no assets/ISO. Returns 1 when any
 * pointer was bound, 0 otherwise, or -FIFA96_ERR_INVALID when `mr` is NULL.
 * Exposed for direct unit testing of the error path. */
int fifa96_match_run_record_meta_bind(struct fifa96_match_run *mr);

/* Resolve the post-period screen chain (FU-64 §6): OVER -> POST
 * (`resolve_over`, the `[0x5FFC]=3` period resolution), POST -> EXIT (the
 * leave staging `[0x5FFC]=4`), then, when EXIT is current, run the existing
 * run_end path (pace hold + register cancel + teardown + post-exit) so the
 * engine returns to FRONTEND. A screen that is still ACTIVE is a no-op
 * (nothing has ended). Returns the run_end result (1 = post-exit ran, 0 =
 * plain teardown), 0 when there was no OVER/POST/EXIT screen to resolve,
 * -FIFA96_ERR_INVALID (NULL) or -FIFA96_ERR_STATE (run not live). */
int fifa96_match_run_resolve(struct fifa96_match_run *mr);

/* One engine step of the run: drives the lifecycle only (the frame body runs
 * on the registered 100 Hz tick hook). 0 while live; the end result
 * (1 = post-exit) when the step exits, either from a staged EXIT or from a
 * period-end OVER, which this step drives through fifa96_match_run_resolve
 * (OVER -> POST -> EXIT -> end, compressed into this step; POST screen pacing
 * is an open leg); or a -fifa96_err_t. After the step has ended the run,
 * further steps return -FIFA96_ERR_STATE. */
int fifa96_match_run_step(struct fifa96_match_run *mr);

/* Tear the match down through the lifecycle and clear the engine linkage.
 * 0 = plain teardown, 1 = post-exit ran, or a -fifa96_err_t. */
int fifa96_match_run_end(struct fifa96_match_run *mr);
