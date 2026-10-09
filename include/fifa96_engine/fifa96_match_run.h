#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_engine/fifa96_match_phase_machine.h"
#include "fifa96_engine/fifa96_platform.h"
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
 * own (see fifa96_match_run_frame and FU-143 §11). */
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
 * records the index and the engine keeps the current selection). Returns 1 on
 * a selected camera, 0 for the default, -FIFA96_ERR_INVALID (NULL). */
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
   * FU-139 reads; image-zero, runtime producer unported, leg L1). The engine
   * carries a caller-staged bit (default 0), mirroring `input_bit2` (the
   * sibling bit 1 the FU-71 update reads). */
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
  int32_t distance;         /* native +0x65 0x8DCD4 out[0] word (FU-142e row 2A) */
  int8_t player_d;          /* rec[+4][+0xD] table index (FU-142b row 26) */
  int8_t player_e;          /* rec[+4][+0xE] stage-0 gate (FU-142b row 26) */
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
  /* FU-139 §9 (Task 11): the current pool record identity and its +0x8B
   * actor-type byte, staged so rows 07/0F can resolve the team candidates and
   * the kick direction table. */
  int32_t entity_id;   /* team*11 + index, or NONE */
  uint8_t actor_type;  /* native +0x8B>>24 = the +0x8E byte; staged from the
                        * pool, which never writes it (OL-83: rows 04/06/07/18
                        * read it while rows 28/2A/0F/07/08 persist `type`) */
  uint8_t row44;       /* native +0x44 animation/event ack byte (FU-151: the
                        * row-1E stage 0/2/8 and row-1D stage 0/4 gates read
                        * it; the producer is the unported animation/event
                        * pipeline, a numbered leg) */
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
  uint8_t type;             /* native +0x8E byte = `[rec+0x8B] >> 24` (the
                             * 0x79C50 face octant rows 28/0F/08 write; the
                             * row-04/06/07/18 readers use `actor_type`, OL-83) */
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
  int running;                                   /* begin/end balance */
  struct fifa96_input input;                     /* FU-61 player-0 edge/held model */
  uint8_t input_state[FIFA96_INPUT_PLAYERS];     /* last sampled FU-61 state (slot input) */
  fifa96_control_slot slot;                      /* FU-70 slot bound to player 0 */
  struct fifa96_match_run_record record;         /* FU-141 dispatch staging record */
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
  /* FU-151 P3 (keeper machines): the process cells the row-1E/1D machines
   * carry across frames — the 0x15774C/50/54 camera focus (the FU-147 leg-13
   * render-camera stand-in: the engine keeps its own derived cell so the
   * machine's hold-follow writes do not move the real FU-71 camera), the
   * 0x157A77 reset triple (the [0x10F328] constant = (0,0,0), FU-140), the
   * 0x157C30 `{band,dx,dz}` staging vector, the 0x157C36 saved point, the
   * 0x157C42 travelled gauge, the [0x157AB2] latch and the stage-6
   * [0x157820]/[0x157822] animation flags (their native consumers outside
   * row 1E stay FU-151 leg 15). Init/begin zero them. */
  int32_t keeper_cam_x, keeper_cam_y, keeper_cam_z;
  int32_t keeper_reset_x, keeper_reset_y, keeper_reset_z;
  int16_t keeper_vec_band, keeper_vec_dx, keeper_vec_dz;
  int16_t keeper_saved_x, keeper_saved_z;
  int16_t keeper_gauge;
  uint8_t keeper_latch_157ab2;
  uint8_t flag_157820;
  uint8_t flag_157822;
  /* M2 playable-match Task 2 / OL-84 residual: the native `[0x5882A]` kickoff
   * gate, the flag the act-1 (phase-0x17 handler FUN_00088DC8) stage 1 sets at
   * the shared timeline timer `[0x58818] >= 0x78` (`0x88EF3..0x88F07`) and
   * action row 01 stage 0 gates on (`0x7DC6B`). `fifa96_match_run_frame`
   * derives the producer from `state.tick_total` (the same whole-delta
   * accumulation, `[0x58818] += [0x57A64]`) while the phase is 1; init/begin
   * clear it. */
  uint8_t global_5882a;
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
   *  - `session_gate_14c32a` = [0x14C32A]: begin seeds 1 (a live match
   *    session; the native writers are front-end, FU-146 leg 2). With the
   *    gate 0 or a pending situation the queue route falls back to the
   *    native table-2 situation-6 arm. */
  uint8_t goal_armed;
  uint8_t goal_zone;
  int32_t goal_snap_x;
  int32_t goal_snap_y;
  int32_t goal_snap_z;
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
   *  - `incident_x`/`incident_z` = the incident position dwords [0x158897]/
   *    [0x15889F] the phase-7 arm probes (0x8D5A8); the producer is the
   *    FU-150 foul adjudicator (P2), seeded 0. (FU-149 §3 lists 0x15889B as
   *    the z cell; first-hand the 12-byte triple is x@897/y@89B/z@89F and
   *    `FUN_00079CCC` reads +8 = 0x15889F — slice erratum.) */
  uint8_t sit_side_pending;
  uint16_t corner_count[2];
  uint8_t side_swap;
  uint8_t store_15882b;
  uint8_t store_15882c;
  int32_t incident_x;
  int32_t incident_z;
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
   *  - `screen_lead_z` = `word[0x1577C2]` the FU-71 camera lead word
   *    (producer unported, staged 0: OL-72);
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
  int16_t screen_lead_z;
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
 * Under the selector-0/no-extra-time default (`extra_time` clear, period < 4)
 * the chooser derives phase 0x0C on the controlled side and act 0xB
 * (`0x8BAA7..0x8BABE`, `0x8BADB..0x8BAE7`); the native then invokes the act
 * selector, but the act handler body, the phase-0xC machine `FUN_0008BAF0`,
 * the extra-time flag `[0x157AC0]` producer and the
 * `FUN_0004B02C(0)`/`FUN_00088860` reset path stay unported (FU-143 §4/§5
 * legs), so the engine lifecycle owns the exit and the 0x0C -> 0 reset is the
 * run_end teardown (`fifa96_match_state_init`).
 *
 * No phase write happens for a class-0 phase, without a staged completion, or
 * when the chooser derives `FIFA96_ACTION_PHASE_NONE`. Returns 1 when the
 * derived chooser ran (a phase is written only when its output is not
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

/* ===== FU-150 P2 — fouls / referee / offside =============================== */

/* The derived machine dispatcher slots (`mr->ref_machine`). NONE = idle; the
 * module sequences map to their FIFA96_REF_SEQ_* value; RESTART is the derived
 * act-2 (phase-0x18) free-kick/penalty hand-off machine the sit-9/0xA rows
 * start (a compression of the native stage 1..3 camera-lead gates, FU-150
 * erratum). */
#define FIFA96_MATCH_RUN_REF_NONE 0u
#define FIFA96_MATCH_RUN_REF_FOUL FIFA96_REF_SEQ_ACT3
#define FIFA96_MATCH_RUN_REF_OFFSIDE FIFA96_REF_SEQ_ACT6
#define FIFA96_MATCH_RUN_REF_RESTART 3u

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
 * ACT3 decision starts the phase-0x19 machine and runs stage 0 immediately
 * (the native FUN_000888FC invoke-now); a severity-0 decision routes
 * situation 9 (fouled side, BX=1) through the shared
 * `fifa96_match_run_set_piece` entry (which, on the direct path, starts the
 * RESTART machine). Mirrors the clamped incident triple into
 * `incident_x`/`incident_z`. Returns 1 when the decision ran, 0 when a gate
 * skipped it, or -FIFA96_ERR_INVALID (NULL mr / invalid rec_a). */
int fifa96_match_run_contact(struct fifa96_match_run *mr, uint8_t kind,
                             int32_t rec_a, int32_t rec_b, const int32_t point[3]);

/* FU-150 §Port contract: the derived reception-time offside check. The native
 * caller is the reception handler FUN_0007A084 0x7A448 (unported), so this is
 * the staged entry: `receiver` is an encoded entity id, `metric` the staged
 * 0x158738 block (producer unported, leg 8) and `camera_ref`/`mirror` the
 * [0x157754]/[0x157823] staged camera inputs (FU-150 risk note). The engine
 * resolves the own-team nearest (FUN_0008DE8C derived as the pool nearest to
 * the ball triple, skip index 0) and the opponent last defender
 * (FUN_0008DE28 derived as the opponent nearest to (0, ±0xB10)) and stages the
 * record-state gate (type != 0x11, code not in {0x10,0x1D,0x1E}).
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
 * calls this after the FU-141 entity chain). NONE -> 0. FOUL/OFFSIDE run the
 * module step and apply its requests: the whistle/speech observation slots
 * (`ref_whistle`/`ref_speech`), the phase write through
 * `match_run_write_phase` + the FU-149 arm, the record install on rec_first,
 * and the situation dispatch through the shared
 * `fifa96_match_run_set_piece(..., BX=1)` (whose direct path may start the
 * RESTART machine and run its step, the native invoke-now order). RESTART runs
 * the derived act-2 hand-off: stage 0 writes phase 0xA on the fouled side
 * (whistle when the foul kind is 0), stage 1 runs the FK/penalty decision
 * (first-hand 0x894A2..0x895AC: `contact_kind == 3` or the session gate
 * proceeds; phase 7 default, phase 6 when |incident x| < 0x420 and z lies in
 * the fouler-side band [-0xB10,-0x7B0] side 0 / [0x7B0,0xB10] side 1) and arms
 * the FU-149 phase 7/6 taker. Also drives the [0x157A6A] suppression countdown
 * (0x7438D..0x743A5, saturated at 0) and stages the live phase into the
 * referee state. Returns 1 when a step ran, 0 when idle, or a -fifa96_err_t
 * (including -FIFA96_ERR_INVALID for NULL). */
int fifa96_match_run_referee_step(struct fifa96_match_run *mr);

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
 *  - leg not 2/4 and `camera.pos_z < 0` and `screen_lead_z < 0` -> id 9;
 *  - leg 5 and `screen_lead_z < 0` -> id 7;
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
