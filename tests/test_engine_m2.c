/* tests/test_engine_m2.c — M2-B headless acceptance tape (spec §5).
 *
 * Drives the spec §5 sequence with the null backend and a scripted key tape:
 * boot -> skip intro -> front-end -> start match (selector 0) -> kickoff ->
 * move -> kick -> score -> period end -> exit to front-end. Every presented
 * frame records one `frame=<n> hash=<hex>` line; while a match is live the
 * line additionally carries `state=<phase>/<home>-<away>` (the FU-62 match
 * clock phase, native [0x157A4A]>>24, and the FU-72 §2.4 score pair). With
 * game/FIFAPCCD96.iso present the transcript is compared byte-for-byte against
 * tests/golden/engine/m2-frames.txt; without the ISO the no-assets smoke mode
 * runs, the test checks self-consistency instead, and the golden comparison is
 * skipped so CI without the asset still passes (the M1 convention).
 *
 * Regenerate the golden transcript (with the ISO present):
 *   ./build/test_engine_m2 > tests/golden/engine/m2-frames.txt
 *
 * --- The golden's provenance and what it pins (the M2-B baseline) -----------
 *
 * The transcript is the deterministic null-backend replay of the engine-owned
 * run started by the real front-end -> match bridge (selector 0, FU-64 §1.1).
 * It pins, in order:
 *   1. the boot/intro-skip frame, the front-end frames of the panel-open and
 *      panel-confirm navigation, and the match-start frame (frame 5);
 *   2. the forced kickoff segments: phase 0x13 (first half, frame 6 onward,
 *      `state=19/...`) and phase 0x14 (second half, frame 26 onward,
 *      `state=20/...`), during which the FU-142a installer arms fire and stage
 *      codes 0x26 (uncontrolled side) and 0x28 + 0x2A (controlled side) into
 *      the FU-141 pool records;
 *   3. the mechanics segment (phase 2): the six Gate-G3 rows and row 1E are
 *      staged into dedicated pool records and dispatched through the engine's
 *      per-record chain (00 and the arm-staged 26/28/2A already dispatched);
 *      the test asserts the observed OK set;
 *   4. the score step: `fifa96_match_run_add_goal(mr, 0)` (no wired body
 *      contains the native FUN_00093944 score writer — see the report / the
 *      FU-142f carried leg), pinned by `state=2/1-0` lines;
 *   5. the forced 1 s period end (`state=2/1-0` on the resolving frame), the
 *      OVER -> POST -> EXIT -> run_end compression returning the engine to
 *      FRONTEND, and 20 post-exit front-end frames.
 *
 * Forced-phase declaration (parent G1 carry-forward): the selector-0/phase-0
 * default never reaches a live period end, so the tape declares its phase
 * assumption explicitly: at match-relative step 1 it forces
 * `mr.state.phase = 0x13` and `mr.phase_machine.state = 0x13` (kickoff), at
 * step 21 `0x14` (second-half arms), and at step 41 phase 2 (class 1: the
 * clock runs, so the shortened 1 s period completes) — mirroring the G1 fix
 * wave's `test_live_period_end_exits_to_frontend` convention.
 *
 * FU-143 phase-driver wiring (playability Task 3): the run frame body now
 * steps the derived `fifa96_match_run_phase_drive` each granted frame, so the
 * phase-2 period end writes the derived post-period phase 0x0C (the selector-0
 * no-extra-time chooser) instead of leaving phase 2 until the teardown reset.
 * The transcript stays BYTE-IDENTICAL: the driver runs inside the exit step
 * (after the state tick), while the `state=` sample is taken before each step,
 * so frame 145 still records the pre-step `state=2/1-0`; no re-pin is needed.
 * The 0x13/0x14/2 forcing stays declared because the derived kickoff entry
 * (`FUN_0008A938` situation 0xB -> phase 2, OL-79) and the extra-time flag
 * `[0x157AC0]` producer are unported, so the tape cannot drop the forcing.
 * This test's golden byte-comparison is the tape-mode record the task asks
 * for: byte-identical = forcing kept, no re-pin.
 *
 * Wired-row dispatch set: the tape stages the Gate-G3 rows (06/07/0F/18/21/23)
 * and keeper row 1E into pool record 1..7 of team 0 at the mechanics step; row
 * 00 dispatches from the pool's reset-installed code before the arms, and the
 * arms stage 26/28/2A organically. The observed set is read from
 * `mr.dispatched_ok` (bit c set when action code c dispatched FIFA96_OK), the
 * Task 15 observability seam, and must equal M2_WIRED_MASK.
 *
 * Step cadence: step_ns = 10 ms, so the null backend advances the engine clock
 * exactly one 100 Hz PIT tick per step; the engine polls once per step, so the
 * 12-entry key tape lands one entry per step (entry index = engine step - 1).
 * Both the ISO and no-assets runs start the match on step 5 (the no-assets
 * boot enters the front-end directly, so the leading CONFIRM wraps through the
 * front-end confirm/panel path instead of skipping the intro), which keeps the
 * match-relative directives aligned without a second tape.
 */
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_loader/fifa96_match_state.h"

/* The engine's mode enum and engine-owned match run are engine-internal state;
 * this acceptance tape reads/forces the run's phase and reads the dispatch
 * observation mask, so it includes the internal header (the bridge tests'
 * convention). */
#include "fifa96_engine/fifa96_engine_internal.h"

#define M2_ISO_PATH "game/FIFAPCCD96.iso"
#define M2_GOLDEN_PATH "tests/golden/engine/m2-frames.txt"
#define M2_STEP_CAP 2000
#define M2_TRANSCRIPT_CAP (1u << 20)

/* Match-relative directive steps (1 = the first match frame, engine step 6):
 *   m 1  first-half kickoff forcing (phase 0x13 on run + machine);
 *   m 21 second-half kickoff forcing (phase 0x14);
 *   m 41 mechanics: assert the arms' record codes, force phase 2, shorten the
 *        period to 1 s (class-1 phase: the clock runs) and stage the G3 rows;
 *   m 62 score: assert the full wired-row dispatch set, add the goal. */
#define M2_M_KICKOFF 1
#define M2_M_HALF 21
#define M2_M_MECH 41
#define M2_M_GOAL 62

/* Post-exit front-end frames recorded before the tape stops. */
#define M2_POST_EXIT_FRAMES 20

/* The wired action rows at the Task 15 baseline (FU-137 §7: 11/80): the rows
 * whose installer arm + body + pool binding are all bounded. 00 and 1E wire
 * first; cluster B/D rows 06/07/0F/18/21/23 close Gate G3; the FU-142a arms
 * stage 26/28/2A. */
#define M2_WIRED_MASK                                                        \
  ((1ull << 0x00) | (1ull << 0x06) | (1ull << 0x07) | (1ull << 0x0F) |       \
   (1ull << 0x18) | (1ull << 0x1E) | (1ull << 0x21) | (1ull << 0x23) |       \
   (1ull << 0x26) | (1ull << 0x28) | (1ull << 0x2A))

/* The scripted key tape: intro skip (with the ISO), panel DECLINE/CONFIRM
 * navigation, then the match input: move (RIGHT, UP) and kick. One entry is
 * consumed per engine step; indices 0..4 land on the front-end steps 1..5
 * (index 4's panel confirm starts the match), index 5 is the first match poll
 * (step 6, the navigation confirm's release, ignored), the move keys land on
 * steps 7/9 and the KICK press on step 11. */
static const fifa96_platform_key M2_KEYS[] = {
    {FIFA96_ENGINE_KEY_CONFIRM, 1}, {FIFA96_ENGINE_KEY_CONFIRM, 0},
    {FIFA96_ENGINE_KEY_DECLINE, 1}, {FIFA96_ENGINE_KEY_DECLINE, 0},
    {FIFA96_ENGINE_KEY_CONFIRM, 1}, {FIFA96_ENGINE_KEY_CONFIRM, 0},
    {FIFA96_ENGINE_KEY_RIGHT, 1},   {FIFA96_ENGINE_KEY_RIGHT, 0},
    {FIFA96_ENGINE_KEY_UP, 1},      {FIFA96_ENGINE_KEY_UP, 0},
    {FIFA96_ENGINE_KEY_KICK, 1},    {FIFA96_ENGINE_KEY_KICK, 0},
};
#define M2_KEYS_LEN (sizeof M2_KEYS / sizeof M2_KEYS[0])

/* Rows staged into team-0 records 1..7 at the mechanics step. */
static const uint8_t M2_STAGE_ROWS[] = {0x06, 0x07, 0x0F, 0x18, 0x21, 0x23, 0x1E};

struct m2_result {
  int steps;                 /* total engine steps */
  int match_start_step;      /* engine step that entered MATCH (5 on both modes) */
  int exit_step;             /* engine step whose resolve returned to FRONTEND */
  int move_step_seen;        /* RIGHT press observed in input_state[0] (step 7) */
  int up_step_seen;          /* UP press observed in input_state[0] (step 9) */
  int kick_step_seen;        /* KICK press observed in input_state[0] (step 11) */
  int armed_26;              /* team 1 held code 0x26 at the mechanics step */
  int armed_28;              /* team 0 held code 0x28 */
  int armed_2a;              /* team 0 record 1 held code 0x2A */
  uint64_t mask_mech;        /* dispatched_ok observed at the mechanics step */
  uint64_t mask_final;       /* dispatched_ok at the exit step */
  uint8_t phase_before_exit; /* sampled on the resolving frame */
  uint16_t score_before_exit[2];
};

static int file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static char *slurp(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
  long n = ftell(f);
  if (n < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
  char *buf = malloc((size_t)n + 1u);
  if (!buf) { fclose(f); return NULL; }
  if (n != 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);
  buf[n] = '\0';
  *len = (size_t)n;
  return buf;
}

static void append(char *transcript, size_t cap, size_t *used, const char *fmt,
                   uint64_t frame, uint64_t hash) {
  int n = snprintf(transcript + *used, cap - *used, fmt, frame, hash);
  assert(n > 0 && *used + (size_t)n < cap);
  *used += (size_t)n;
}

static void append_state(char *transcript, size_t cap, size_t *used, uint8_t phase,
                         uint16_t home, uint16_t away) {
  int n = snprintf(transcript + *used, cap - *used, " state=%u/%u-%u", (unsigned)phase,
                   (unsigned)home, (unsigned)away);
  assert(n > 0 && *used + (size_t)n < cap);
  *used += (size_t)n;
}

/* Applies the match-relative directives at the top of engine step `next`. The
 * match run is live whenever these run (m > 0 implies the bridge has started
 * it). */
static void m2_directives(struct fifa96_engine *e, int next, int match_start_step,
                          struct m2_result *res) {
  struct fifa96_match_run *mr = &e->match_run;
  int m = next - match_start_step;
  if (m == M2_M_KICKOFF) {
    assert(mr->state.phase == 0);                       /* pre-kickoff default */
    assert(fifa96_match_state_set_phase(&mr->state, 0x13) == 0);
    mr->phase_machine.state = 0x13;                     /* kickoff: forced live phase */
  } else if (m == M2_M_HALF) {
    assert(fifa96_match_state_set_phase(&mr->state, 0x14) == 0);
    mr->phase_machine.state = 0x14;
  } else if (m == M2_M_MECH) {
    /* The FU-142a arms fired during the 0x13/0x14 frames: the uncontrolled
     * side holds 0x26, the controlled side 0x28 with its first free record
     * overwritten by 0x2A. */
    res->armed_26 = mr->entities.team[1].records[1].code == 0x26;
    res->armed_28 = mr->entities.team[0].records[2].code == 0x28;
    res->armed_2a = mr->entities.team[0].records[1].code == 0x2A;
    assert(res->armed_26 && res->armed_28 && res->armed_2a);
    res->mask_mech = mr->dispatched_ok;
    assert((res->mask_mech & ((1ull << 0x26) | (1ull << 0x28) | (1ull << 0x2A))) ==
           ((1ull << 0x26) | (1ull << 0x28) | (1ull << 0x2A)));
    assert(fifa96_match_state_set_phase(&mr->state, 2) == 0);   /* class 1: clock runs */
    assert(fifa96_match_run_set_period(mr, 1, 1) == 0);
    for (size_t i = 0; i < sizeof M2_STAGE_ROWS / sizeof M2_STAGE_ROWS[0]; i++)
      assert(fifa96_match_entities_install(&mr->entities.team[0].records[1 + i], 2,
                                           M2_STAGE_ROWS[i], 0) == 1);
  } else if (m == M2_M_GOAL) {
    /* Every wired row dispatched FIFA96_OK during the replay. */
    res->mask_final = mr->dispatched_ok;
    assert(res->mask_final == M2_WIRED_MASK);
    assert(fifa96_match_run_add_goal(mr, 0) == 0);      /* the tape's score step */
  }
}

/* Boots the engine, replays the tape with the match directives, and appends
 * one transcript line per presented frame. The per-step invariants (one
 * present, nonzero hash, frame ordinals +1) hold in both modes. */
static void run_tape(int with_iso, char *transcript, size_t cap, size_t *out_len,
                     struct fifa96_platform_null_stats *out_stats,
                     struct m2_result *res) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.tape = M2_KEYS;
  pcfg.tape_len = M2_KEYS_LEN;
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per step, deterministic */
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);

  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.iso_path = with_iso ? M2_ISO_PATH : NULL;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(fifa96_engine_should_quit(e) == 0);

  memset(res, 0, sizeof *res);
  res->match_start_step = -1;
  res->exit_step = -1;

  size_t used = 0;
  int steps = 0;
  while (steps < M2_STEP_CAP) {
    int next = steps + 1;
    if (res->match_start_step > 0)
      m2_directives(e, next, res->match_start_step, res);
    /* The state line records the state that produced this frame's render, so
     * it is sampled before the step (the resolving frame then carries the
     * period-end phase/score, not the post-teardown zeros). */
    int live = (e->mode == FIFA96_ENGINE_MODE_MATCH && e->match != NULL);
    uint8_t s_phase = 0;
    uint16_t s_home = 0;
    uint16_t s_away = 0;
    if (live) {
      s_phase = e->match_run.state.phase;
      s_home = e->match_run.score[0];
      s_away = e->match_run.score[1];
    }
    assert(fifa96_engine_step(e) == 0);
    steps++;
    struct fifa96_platform_null_stats st;
    fifa96_platform_null_stats(plat, &st);
    assert(st.presents == (uint64_t)steps);   /* one present per step */
    assert(st.present_hash != 0u);
    append(transcript, cap, &used, "frame=%" PRIu64 " hash=%016" PRIx64, st.presents,
           st.present_hash);
    if (live) append_state(transcript, cap, &used, s_phase, s_home, s_away);
    assert(used + 1u < cap);
    transcript[used++] = '\n';
    if (res->match_start_step < 0 && e->mode == FIFA96_ENGINE_MODE_MATCH) {
      res->match_start_step = steps;
      /* The bridge starts the FU-64 §1.1 selector-0 run and stages the
       * derived FU-86 art pair when the ISO is mounted; without it rendering
       * stays disabled (the M1 soft-failure path). */
      assert(e->match_run.lc.selector == 0);
      assert(e->match_run.render.enabled == (with_iso ? 1 : 0));
    }
    if (steps == 7 || steps == 9 || steps == 11) {
      /* The scripted move (RIGHT 0x04 / UP 0x01) and kick (0x10) presses
       * reached the match input model. */
      assert(live);
      uint8_t in = e->match_run.input_state[0];
      if (steps == 7 && in == 0x04) res->move_step_seen = steps;
      if (steps == 9 && in == 0x01) res->up_step_seen = steps;
      if (steps == 11 && in == 0x10) res->kick_step_seen = steps;
    }
    if (res->match_start_step > 0 && res->exit_step < 0 &&
        e->mode != FIFA96_ENGINE_MODE_MATCH) {
      res->exit_step = steps;
      res->phase_before_exit = s_phase;
      res->score_before_exit[0] = s_home;
      res->score_before_exit[1] = s_away;
      res->mask_final = e->match_run.dispatched_ok;
    }
    if (res->exit_step > 0 && steps - res->exit_step >= M2_POST_EXIT_FRAMES) break;
  }
  assert(steps < M2_STEP_CAP);               /* the sequence ends by itself */
  res->steps = steps;
  fifa96_platform_null_stats(plat, out_stats);
  *out_len = used;
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

/* First-difference report so a golden drift points at the offending line. */
static void report_diff(const char *got, size_t got_len, const char *want, size_t want_len,
                        size_t at) {
  size_t gs = at, ws = at;
  while (gs > 0 && got[gs - 1] != '\n') gs--;
  while (ws > 0 && want[ws - 1] != '\n') ws--;
  size_t ge = at, we = at;
  while (ge < got_len && got[ge] != '\n') ge++;
  while (we < want_len && want[we] != '\n') we++;
  size_t line = 1;
  for (size_t i = 0; i < at && i < got_len; i++)
    if (got[i] == '\n') line++;
  fprintf(stderr, "test_engine_m2: transcript differs at line %zu (byte %zu):\n", line, at);
  fprintf(stderr, "  golden: %.*s\n", (int)(we - ws), want + ws);
  fprintf(stderr, "  actual: %.*s\n", (int)(ge - gs), got + gs);
  if (got_len != want_len)
    fprintf(stderr, "  lengths differ: golden %zu, actual %zu bytes\n", want_len, got_len);
}

int main(void) {
  int with_iso = file_exists(M2_ISO_PATH);
  char *transcript = malloc(M2_TRANSCRIPT_CAP);
  assert(transcript != NULL);

  size_t tlen = 0;
  struct fifa96_platform_null_stats st;
  struct m2_result res;
  run_tape(with_iso, transcript, M2_TRANSCRIPT_CAP, &tlen, &st, &res);

  /* Structural acceptance (both modes): the spec §5 sequence replayed. */
  assert(res.match_start_step == 5);
  /* The golden pins frames 6..145 as the match (exit at frame 145), i.e. the
   * forced mechanics window (m 41) plus the 1 s phase-2 clock (100 ticks). */
  assert(res.exit_step - res.match_start_step == 140);
  assert(res.armed_26 && res.armed_28 && res.armed_2a);  /* installer arms fired */
  assert(res.move_step_seen == 7);                       /* move input reached the run */
  assert(res.up_step_seen == 9);
  assert(res.kick_step_seen == 11);                      /* kick input reached the run */
  assert(res.mask_final == M2_WIRED_MASK);               /* wired-row dispatch set */
  assert(res.phase_before_exit == 2);                    /* forced class-1 period end */
  assert(res.score_before_exit[0] == 1 && res.score_before_exit[1] == 0);
  assert(st.presents == (uint64_t)res.steps);
  assert(st.present_hash != 0u);
  /* The state lines pin the forced kickoff phases, the scoreless first frame
   * and the goal. */
  assert(strstr(transcript, " state=19/") != NULL);
  assert(strstr(transcript, " state=20/") != NULL);
  assert(strstr(transcript, " state=2/0-0\n") != NULL);
  assert(strstr(transcript, " state=2/1-0\n") != NULL);
  assert(fwrite(transcript, 1, tlen, stdout) == tlen);

  if (!with_iso) {
    fprintf(stderr, "SKIP golden comparison (no ISO)\n");
    fprintf(stderr, "test_engine_m2 OK\n");
    free(transcript);
    return 0;
  }

  size_t glen = 0;
  char *golden = slurp(M2_GOLDEN_PATH, &glen);
  if (!golden) {
    fprintf(stderr, "test_engine_m2: cannot read golden %s\n", M2_GOLDEN_PATH);
    free(transcript);
    return 1;
  }
  if (tlen != glen || memcmp(transcript, golden, tlen) != 0) {
    size_t at = 0;
    while (at < tlen && at < glen && transcript[at] == golden[at]) at++;
    report_diff(transcript, tlen, golden, glen, at);
    fprintf(stderr, "test_engine_m2 FAIL: transcript differs from %s\n", M2_GOLDEN_PATH);
    free(golden);
    free(transcript);
    return 1;
  }
  free(golden);
  free(transcript);
  /* Status goes to stderr: stdout is the transcript, so the documented
   * regeneration redirect stays byte-exact. */
  fprintf(stderr, "test_engine_m2 OK\n");
  return 0;
}
