/* tests/test_engine_m1.c — M1 headless acceptance tape.
 *
 * Drives boot -> intro -> front-end -> navigate -> quit with the null
 * platform backend and a scripted key tape, recording one `frame=<n>
 * hash=<hex>` line per presented frame. With game/FIFAPCCD96.iso present the
 * transcript is compared byte-for-byte against tests/golden/engine/m1-frames.txt;
 * without the ISO the no-assets smoke mode runs, the test checks
 * self-consistency instead, and the golden comparison is skipped so CI
 * without the asset still passes.
 *
 * Step cadence: the null backend advances now_ns() by one step_ns per present
 * and this test sets step_ns = 10 ms, so every engine step is exactly one
 * 100 Hz PIT tick. Intro video plays at 15 frames per 100 ticks (FU-37) --
 * frame 1 lands on step 7 (7 * 15 / 100 = 1) and each next frame ~6.7 steps
 * later -- and the tape is consumed one entry per step (the engine polls once
 * per step). The leading M1_INTRO_PAD_STEPS release-only entries are ignored
 * during playback, so the skip CONFIRM reaches the engine only after 102
 * real VIDEO/VID_INTR.TGV frames (the last at step 680); it aborts the rest
 * of the stream and the navigation entries then drive the front-end. The
 * golden therefore pins the real intro frames (distinct present-hash chain
 * links, one per presented step) before the menu frames.
 *
 * Regenerate the golden transcript (with the ISO present):
 *   ./build/test_engine_m1 > tests/golden/engine/m1-frames.txt
 */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

#define M1_ISO_PATH "game/FIFAPCCD96.iso"
#define M1_GOLDEN_PATH "tests/golden/engine/m1-frames.txt"
#define M1_STEP_CAP 5000
#define M1_TRANSCRIPT_CAP (1u << 20)

/* Skip-intro CONFIRM, menu DOWN x2, CONFIRM (select), DECLINE, then QUIT.
 * The null backend releases one entry per poll and the engine polls once per
 * step, so each press/release is observed on its own step; the intro padding
 * delays the skip CONFIRM until after real intro playback. */
static const fifa96_platform_key M1_NAV_KEYS[] = {
    {FIFA96_ENGINE_KEY_CONFIRM, 1}, {FIFA96_ENGINE_KEY_CONFIRM, 0},
    {FIFA96_ENGINE_KEY_DOWN, 1},    {FIFA96_ENGINE_KEY_DOWN, 0},
    {FIFA96_ENGINE_KEY_DOWN, 1},    {FIFA96_ENGINE_KEY_DOWN, 0},
    {FIFA96_ENGINE_KEY_CONFIRM, 1}, {FIFA96_ENGINE_KEY_CONFIRM, 0},
    {FIFA96_ENGINE_KEY_DECLINE, 1}, {FIFA96_ENGINE_KEY_DECLINE, 0},
    {FIFA96_ENGINE_KEY_QUIT, 1},    {FIFA96_ENGINE_KEY_QUIT, 0},
};

/* Release-only padding consumed while the intro plays: 680 ignored entries at
 * 10 ms per step carry playback through 102 decoded VID_INTR frames, after
 * which the navigation CONFIRM aborts the intro instead of skipping it on
 * step 1. */
#define M1_INTRO_PAD_STEPS 680u
#define M1_NAV_KEYS_LEN (sizeof M1_NAV_KEYS / sizeof M1_NAV_KEYS[0])
#define M1_TAPE_LEN (M1_INTRO_PAD_STEPS + M1_NAV_KEYS_LEN)

/* Every tape entry except the trailing QUIT pair. The engine must survive all
 * of them (intro + navigation spread across steps) and quit on the QUIT
 * press, so the pre-quit steps present the intro and at least three
 * front-end frames. */
#define M1_PRE_QUIT_ENTRIES (M1_TAPE_LEN - 2)
_Static_assert(M1_PRE_QUIT_ENTRIES >= 4, "tape must present >= 3 front-end frames before quit");

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

/* Boots the engine on the null backend, steps until QUIT (or the cap), and
 * appends one `frame=N hash=H` line per presented frame. The per-step
 * invariants (one present, nonzero hash, frame ordinals +1) hold in both the
 * ISO and no-assets modes. */
static void run_tape(int with_iso, char *transcript, size_t cap, size_t *out_len,
                     struct fifa96_platform_null_stats *out_stats) {
  fifa96_platform_key tape[M1_TAPE_LEN];
  for (size_t i = 0; i < M1_INTRO_PAD_STEPS; i++)
    tape[i] = (fifa96_platform_key){FIFA96_ENGINE_KEY_UP, 0};
  memcpy(tape + M1_INTRO_PAD_STEPS, M1_NAV_KEYS, sizeof M1_NAV_KEYS);

  struct fifa96_platform_null_config pcfg = {0};
  pcfg.tape = tape;
  pcfg.tape_len = M1_TAPE_LEN;
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per step, deterministic */
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);

  struct fifa96_engine_config ecfg = {0};
  ecfg.iso_path = with_iso ? M1_ISO_PATH : NULL;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(fifa96_engine_should_quit(e) == 0);

  size_t used = 0;
  uint64_t last_presents = 0;
  int steps = 0;
  while (!fifa96_engine_should_quit(e) && steps < M1_STEP_CAP) {
    assert(fifa96_engine_step(e) == 0);
    steps++;
    struct fifa96_platform_null_stats st;
    fifa96_platform_null_stats(plat, &st);
    assert(st.presents == last_presents + 1u);   /* one present per step */
    last_presents = st.presents;
    assert(st.present_hash != 0u);
    /* Only the QUIT key may end the run: the engine must still be alive after
     * every pre-QUIT entry has been consumed one step at a time. */
    if ((size_t)steps <= M1_PRE_QUIT_ENTRIES)
      assert(fifa96_engine_should_quit(e) == 0);
    int n = snprintf(transcript + used, cap - used,
                     "frame=%" PRIu64 " hash=%016" PRIx64 "\n",
                     st.presents, st.present_hash);
    assert(n > 0 && used + (size_t)n < cap);
    used += (size_t)n;
  }
  assert(fifa96_engine_should_quit(e) == 1);     /* QUIT must arrive within the cap */
  assert(steps == (int)M1_PRE_QUIT_ENTRIES + 1); /* quit exactly on the QUIT press */
  {   /* one entry per step: >= 3 of the presents are front-end frames */
    struct fifa96_platform_null_stats end_st;
    fifa96_platform_null_stats(plat, &end_st);
    assert(end_st.presents == (uint64_t)steps);
    assert(end_st.presents - 1u >= 3u);
  }

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
  fprintf(stderr, "test_engine_m1: transcript differs at line %zu (byte %zu):\n", line, at);
  fprintf(stderr, "  golden: %.*s\n", (int)(we - ws), want + ws);
  fprintf(stderr, "  actual: %.*s\n", (int)(ge - gs), got + gs);
  if (got_len != want_len)
    fprintf(stderr, "  lengths differ: golden %zu, actual %zu bytes\n", want_len, got_len);
}

int main(void) {
  int with_iso = file_exists(M1_ISO_PATH);
  char *transcript = malloc(M1_TRANSCRIPT_CAP);
  assert(transcript != NULL);

  size_t tlen = 0;
  struct fifa96_platform_null_stats st;
  run_tape(with_iso, transcript, M1_TRANSCRIPT_CAP, &tlen, &st);
  assert(st.presents >= 1u);
  assert(st.present_hash != 0u);
  assert(fwrite(transcript, 1, tlen, stdout) == tlen);

  if (!with_iso) {
    fprintf(stderr, "SKIP golden comparison (no ISO)\n");
    fprintf(stderr, "test_engine_m1 OK\n");
    free(transcript);
    return 0;
  }

  size_t glen = 0;
  char *golden = slurp(M1_GOLDEN_PATH, &glen);
  if (!golden) {
    fprintf(stderr, "test_engine_m1: cannot read golden %s\n", M1_GOLDEN_PATH);
    free(transcript);
    return 1;
  }
  if (tlen != glen || memcmp(transcript, golden, tlen) != 0) {
    size_t at = 0;
    while (at < tlen && at < glen && transcript[at] == golden[at]) at++;
    report_diff(transcript, tlen, golden, glen, at);
    fprintf(stderr, "test_engine_m1 FAIL: transcript differs from %s\n", M1_GOLDEN_PATH);
    free(golden);
    free(transcript);
    return 1;
  }
  free(golden);
  free(transcript);
  /* Status goes to stderr: stdout is the transcript, so the documented
   * regeneration redirect stays byte-exact. */
  fprintf(stderr, "test_engine_m1 OK\n");
  return 0;
}
