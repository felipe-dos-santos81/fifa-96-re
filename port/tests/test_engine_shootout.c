/* test_engine_shootout.c — P14 derived [D] penalty-shootout model.
 *
 * Standalone unit test (no ISO, no engine): phase/round progression, scoring
 * both ways, regulation and sudden-death winners, the 20-round cap, invalid
 * choices, deterministic same-seed replay and reinitialisation. The model is
 * derived; this test pins the port contract, not native behaviour.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_shootout.h"

static int failures;

#define CHECK(cond)                                                      \
  do {                                                                   \
    if (!(cond)) {                                                       \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
      failures++;                                                        \
      return;                                                            \
    }                                                                    \
  } while (0)

static uint8_t choice_other(uint8_t choice) {
  return (uint8_t)((choice + 1u) % 3u);
}

/* Force a round result: `user_goal` and `cpu_goal` are 0 (saved) or 1. */
static int play_round(struct fifa96_shootout *s, int user_goal, int cpu_goal) {
  uint8_t keeper;
  uint8_t kick;
  uint8_t cpu_kick;
  uint8_t dive;

  keeper = fifa96_shootout_cpu_keeper_dive(s);
  if (keeper > FIFA96_SHOOTOUT_CHOICE_RIGHT) return -1;
  kick = user_goal ? choice_other(keeper) : keeper;
  if (fifa96_shootout_select_kick(s, kick) != 0) return -1;

  cpu_kick = fifa96_shootout_cpu_kick(s);
  if (cpu_kick > FIFA96_SHOOTOUT_CHOICE_RIGHT) return -1;
  dive = cpu_goal ? choice_other(cpu_kick) : cpu_kick;
  if (fifa96_shootout_select_dive(s, dive) != 0) return -1;
  return 0;
}

static int shootouts_equal(const struct fifa96_shootout *a,
                           const struct fifa96_shootout *b) {
  unsigned i;

  if (a->seed != b->seed) return 0;
  if (a->user_side != b->user_side) return 0;
  if (a->phase != b->phase) return 0;
  if (a->round != b->round) return 0;
  if (a->user_score != b->user_score) return 0;
  if (a->cpu_score != b->cpu_score) return 0;
  if (a->winner != b->winner) return 0;
  if (a->last.valid != b->last.valid) return 0;
  if (a->last.taker != b->last.taker) return 0;
  if (a->last.side != b->last.side) return 0;
  if (a->last.round != b->last.round) return 0;
  if (a->last.kick != b->last.kick) return 0;
  if (a->last.opposite != b->last.opposite) return 0;
  if (a->last.scored != b->last.scored) return 0;
  for (i = 0u; i < FIFA96_SHOOTOUT_MAX_ROUNDS; i++) {
    if (a->rounds[i].user_kick != b->rounds[i].user_kick) return 0;
    if (a->rounds[i].cpu_dive != b->rounds[i].cpu_dive) return 0;
    if (a->rounds[i].cpu_kick != b->rounds[i].cpu_kick) return 0;
    if (a->rounds[i].user_dive != b->rounds[i].user_dive) return 0;
    if (a->rounds[i].user_scored != b->rounds[i].user_scored) return 0;
    if (a->rounds[i].cpu_scored != b->rounds[i].cpu_scored) return 0;
    if (a->rounds[i].complete != b->rounds[i].complete) return 0;
  }
  return 1;
}

static int run_script(uint32_t seed, uint8_t user_side,
                      struct fifa96_shootout *out) {
  static const uint8_t kicks[FIFA96_SHOOTOUT_MAX_ROUNDS] = {
      0u, 1u, 2u, 0u, 1u, 2u, 0u, 1u, 2u, 0u,
      1u, 2u, 0u, 1u, 2u, 0u, 1u, 2u, 0u};
  static const uint8_t dives[FIFA96_SHOOTOUT_MAX_ROUNDS] = {
      2u, 1u, 0u, 2u, 1u, 0u, 2u, 1u, 0u, 2u,
      1u, 0u, 2u, 1u, 0u, 2u, 1u, 0u, 2u, 1u};
  unsigned i;

  if (fifa96_shootout_init(out, seed, user_side) != 0) return -1;
  for (i = 0u; i < FIFA96_SHOOTOUT_MAX_ROUNDS; i++) {
    uint8_t phase = fifa96_shootout_phase(out);
    if (phase == FIFA96_SHOOTOUT_PHASE_DONE) break;
    if (phase != FIFA96_SHOOTOUT_PHASE_USER_KICK) return -1;
    if (fifa96_shootout_select_kick(out, kicks[i]) != 0) return -1;
    if (fifa96_shootout_select_dive(out, dives[i]) != 0) return -1;
  }
  return fifa96_shootout_phase(out) == FIFA96_SHOOTOUT_PHASE_DONE ? 0 : -1;
}

static void test_init_validation(void) {
  struct fifa96_shootout s;
  char buf[64];

  memset(&s, 0xA5, sizeof(s));
  CHECK(fifa96_shootout_init(NULL, 1u, 0u) == -1);
  CHECK(fifa96_shootout_init(&s, 1u, 2u) == -1);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_IDLE);
  CHECK(fifa96_shootout_round(&s) == 0u);
  CHECK(fifa96_shootout_user_score(&s) == 0u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_winner(&s) == FIFA96_SHOOTOUT_NO_WINNER);
  CHECK(fifa96_shootout_last_result(&s) == NULL);
  CHECK(fifa96_shootout_round_result(&s, 1u) == NULL);
  CHECK(fifa96_shootout_cpu_keeper_dive(&s) == FIFA96_SHOOTOUT_CHOICE_NONE);
  CHECK(fifa96_shootout_cpu_kick(&s) == FIFA96_SHOOTOUT_CHOICE_NONE);
  CHECK(fifa96_shootout_select_kick(&s, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_select_dive(&s, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: idle") == 0);
  CHECK(fifa96_shootout_score_string(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "0-0") == 0);

  CHECK(fifa96_shootout_init(&s, 0x12345678u, 0u) == 0);
  CHECK(s.seed == 0x12345678u);
  CHECK(s.user_side == 0u);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_KICK);
  CHECK(fifa96_shootout_round(&s) == 1u);
  CHECK(fifa96_shootout_user_score(&s) == 0u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_winner(&s) == FIFA96_SHOOTOUT_NO_WINNER);
  CHECK(fifa96_shootout_last_result(&s) == NULL);
  CHECK(fifa96_shootout_round_result(&s, 1u) != NULL);
  CHECK(fifa96_shootout_round_result(&s, 1u)->user_kick ==
        FIFA96_SHOOTOUT_NO_CHOICE);
  CHECK(fifa96_shootout_round_result(&s, 1u)->cpu_dive ==
        FIFA96_SHOOTOUT_NO_CHOICE);
  CHECK(fifa96_shootout_round_result(&s, 1u)->complete == 0u);
  CHECK(fifa96_shootout_round_result(&s, 2u) == NULL);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 0-0, round 1/5") == 0);
  CHECK(strchr(buf, '\n') == NULL);

  CHECK(fifa96_shootout_select_kick(&s, 3u) == -1);
  CHECK(fifa96_shootout_select_kick(&s, FIFA96_SHOOTOUT_NO_CHOICE) == -1);
  CHECK(fifa96_shootout_select_dive(&s, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_select_kick(NULL, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_select_dive(NULL, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_KICK);
  CHECK(fifa96_shootout_round(&s) == 1u);
  CHECK(fifa96_shootout_user_score(&s) == 0u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_last_result(&s) == NULL);

  CHECK(fifa96_shootout_phase(NULL) == FIFA96_SHOOTOUT_PHASE_IDLE);
  CHECK(fifa96_shootout_round(NULL) == 0u);
  CHECK(fifa96_shootout_user_score(NULL) == 0u);
  CHECK(fifa96_shootout_cpu_score(NULL) == 0u);
  CHECK(fifa96_shootout_winner(NULL) == FIFA96_SHOOTOUT_NO_WINNER);
  CHECK(fifa96_shootout_last_result(NULL) == NULL);
  CHECK(fifa96_shootout_round_result(NULL, 1u) == NULL);
  CHECK(fifa96_shootout_cpu_keeper_dive(NULL) ==
        FIFA96_SHOOTOUT_CHOICE_NONE);
  CHECK(fifa96_shootout_cpu_kick(NULL) == FIFA96_SHOOTOUT_CHOICE_NONE);

  CHECK(strcmp(fifa96_shootout_choice_name(FIFA96_SHOOTOUT_CHOICE_LEFT),
               "Left") == 0);
  CHECK(strcmp(fifa96_shootout_choice_name(FIFA96_SHOOTOUT_CHOICE_CENTRE),
               "Centre") == 0);
  CHECK(strcmp(fifa96_shootout_choice_name(FIFA96_SHOOTOUT_CHOICE_RIGHT),
               "Right") == 0);
  CHECK(strcmp(fifa96_shootout_choice_name(FIFA96_SHOOTOUT_CHOICE_NONE),
               "None") == 0);
  CHECK(strcmp(fifa96_shootout_choice_name(7u), "None") == 0);
}

static void test_scoring_both_ways(void) {
  struct fifa96_shootout s;
  const struct fifa96_shootout_last *last;
  const struct fifa96_shootout_round *round;
  uint8_t keeper;
  uint8_t kick;
  uint8_t cpu_kick;
  uint8_t dive;

  CHECK(fifa96_shootout_init(&s, 0x00C0FFEEu, 0u) == 0);
  keeper = fifa96_shootout_cpu_keeper_dive(&s);
  CHECK(keeper <= FIFA96_SHOOTOUT_CHOICE_RIGHT);
  kick = choice_other(keeper);
  CHECK(fifa96_shootout_select_kick(&s, kick) == 0);
  CHECK(fifa96_shootout_user_score(&s) == 1u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_DIVE);
  last = fifa96_shootout_last_result(&s);
  CHECK(last != NULL);
  CHECK(last->valid == 1u);
  CHECK(last->taker == FIFA96_SHOOTOUT_TAKER_USER);
  CHECK(last->side == 0u);
  CHECK(last->round == 1u);
  CHECK(last->kick == kick);
  CHECK(last->opposite == keeper);
  CHECK(last->scored == 1u);
  round = fifa96_shootout_round_result(&s, 1u);
  CHECK(round != NULL);
  CHECK(round->user_kick == kick);
  CHECK(round->cpu_dive == keeper);
  CHECK(round->user_scored == 1u);
  CHECK(round->complete == 0u);

  cpu_kick = fifa96_shootout_cpu_kick(&s);
  CHECK(cpu_kick <= FIFA96_SHOOTOUT_CHOICE_RIGHT);
  dive = choice_other(cpu_kick);
  CHECK(fifa96_shootout_select_dive(&s, dive) == 0);
  CHECK(fifa96_shootout_user_score(&s) == 1u);
  CHECK(fifa96_shootout_cpu_score(&s) == 1u);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_KICK);
  CHECK(fifa96_shootout_round(&s) == 2u);
  CHECK(fifa96_shootout_winner(&s) == FIFA96_SHOOTOUT_NO_WINNER);
  last = fifa96_shootout_last_result(&s);
  CHECK(last != NULL);
  CHECK(last->taker == FIFA96_SHOOTOUT_TAKER_CPU);
  CHECK(last->side == 1u);
  CHECK(last->round == 1u);
  CHECK(last->kick == cpu_kick);
  CHECK(last->opposite == dive);
  CHECK(last->scored == 1u);
  round = fifa96_shootout_round_result(&s, 1u);
  CHECK(round != NULL);
  CHECK(round->cpu_kick == cpu_kick);
  CHECK(round->user_dive == dive);
  CHECK(round->cpu_scored == 1u);
  CHECK(round->complete == 1u);
  CHECK(fifa96_shootout_round_result(&s, 2u) != NULL);
  CHECK(fifa96_shootout_round_result(&s, 3u) == NULL);

  /* Saves both ways, with user_side 1 to exercise side mapping. */
  CHECK(fifa96_shootout_init(&s, 0x00C0FFEEu, 1u) == 0);
  keeper = fifa96_shootout_cpu_keeper_dive(&s);
  CHECK(fifa96_shootout_select_kick(&s, keeper) == 0);
  CHECK(fifa96_shootout_user_score(&s) == 0u);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_DIVE);
  last = fifa96_shootout_last_result(&s);
  CHECK(last != NULL);
  CHECK(last->taker == FIFA96_SHOOTOUT_TAKER_USER);
  CHECK(last->side == 1u);
  CHECK(last->scored == 0u);
  CHECK(last->kick == keeper);
  CHECK(last->opposite == keeper);

  cpu_kick = fifa96_shootout_cpu_kick(&s);
  CHECK(fifa96_shootout_select_dive(&s, cpu_kick) == 0);
  CHECK(fifa96_shootout_user_score(&s) == 0u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_KICK);
  CHECK(fifa96_shootout_round(&s) == 2u);
  last = fifa96_shootout_last_result(&s);
  CHECK(last != NULL);
  CHECK(last->taker == FIFA96_SHOOTOUT_TAKER_CPU);
  CHECK(last->side == 0u);
  CHECK(last->scored == 0u);
  CHECK(last->kick == cpu_kick);
  CHECK(last->opposite == cpu_kick);
}

static void test_regulation_and_no_early_clinch(void) {
  struct fifa96_shootout s;
  char buf[96];
  unsigned i;

  CHECK(fifa96_shootout_init(&s, 0x0BADF00Du, 0u) == 0);
  CHECK(play_round(&s, 1, 0) == 0);
  CHECK(fifa96_shootout_user_score(&s) == 1u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_KICK);
  CHECK(fifa96_shootout_round(&s) == 2u);
  CHECK(fifa96_shootout_winner(&s) == FIFA96_SHOOTOUT_NO_WINNER);

  for (i = 1u; i < FIFA96_SHOOTOUT_REGULATION_ROUNDS; i++)
    CHECK(play_round(&s, 1, 0) == 0);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_DONE);
  CHECK(fifa96_shootout_round(&s) == 5u);
  CHECK(fifa96_shootout_user_score(&s) == 5u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_winner(&s) == 0u);
  CHECK(fifa96_shootout_select_kick(&s, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_select_dive(&s, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 5-0, user wins after round 5") == 0);
  CHECK(strchr(buf, '\n') == NULL);
  CHECK(fifa96_shootout_score_string(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "5-0") == 0);

  /* CPU win with user_side 1: the CPU is game side 0. */
  CHECK(fifa96_shootout_init(&s, 0x0BADF00Du, 1u) == 0);
  for (i = 0u; i < FIFA96_SHOOTOUT_REGULATION_ROUNDS; i++)
    CHECK(play_round(&s, 0, 1) == 0);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_DONE);
  CHECK(fifa96_shootout_round(&s) == 5u);
  CHECK(fifa96_shootout_user_score(&s) == 0u);
  CHECK(fifa96_shootout_cpu_score(&s) == 5u);
  CHECK(fifa96_shootout_winner(&s) == 0u);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 0-5, cpu wins after round 5") == 0);
}

static void test_sudden_death(void) {
  struct fifa96_shootout s;
  char buf[96];
  unsigned i;

  CHECK(fifa96_shootout_init(&s, 0x5EED5EEDu, 1u) == 0);
  for (i = 0u; i < FIFA96_SHOOTOUT_REGULATION_ROUNDS; i++)
    CHECK(play_round(&s, 1, 1) == 0);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_KICK);
  CHECK(fifa96_shootout_round(&s) == 6u);
  CHECK(fifa96_shootout_user_score(&s) == 5u);
  CHECK(fifa96_shootout_cpu_score(&s) == 5u);
  CHECK(fifa96_shootout_winner(&s) == FIFA96_SHOOTOUT_NO_WINNER);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 5-5, sudden death 1") == 0);
  CHECK(fifa96_shootout_round_result(&s, 6u) != NULL);
  CHECK(fifa96_shootout_round_result(&s, 7u) == NULL);

  CHECK(play_round(&s, 1, 0) == 0);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_DONE);
  CHECK(fifa96_shootout_round(&s) == 6u);
  CHECK(fifa96_shootout_user_score(&s) == 6u);
  CHECK(fifa96_shootout_cpu_score(&s) == 5u);
  CHECK(fifa96_shootout_winner(&s) == 1u);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 6-5, user wins after round 6") == 0);
  CHECK(fifa96_shootout_last_result(&s) != NULL);
  CHECK(fifa96_shootout_last_result(&s)->taker ==
        FIFA96_SHOOTOUT_TAKER_CPU);
  CHECK(fifa96_shootout_last_result(&s)->side == 0u);
  CHECK(fifa96_shootout_last_result(&s)->scored == 0u);

  /* CPU wins the first sudden-death round. */
  CHECK(fifa96_shootout_init(&s, 0x5EED5EEDu, 0u) == 0);
  for (i = 0u; i < FIFA96_SHOOTOUT_REGULATION_ROUNDS; i++)
    CHECK(play_round(&s, 1, 1) == 0);
  CHECK(play_round(&s, 0, 1) == 0);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_DONE);
  CHECK(fifa96_shootout_round(&s) == 6u);
  CHECK(fifa96_shootout_user_score(&s) == 5u);
  CHECK(fifa96_shootout_cpu_score(&s) == 6u);
  CHECK(fifa96_shootout_winner(&s) == 1u);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 5-6, cpu wins after round 6") == 0);
}

static void test_cap(void) {
  struct fifa96_shootout s;
  char buf[96];
  unsigned i;

  CHECK(fifa96_shootout_init(&s, 0xCAFEBABEu, 1u) == 0);
  for (i = 0u; i < FIFA96_SHOOTOUT_MAX_ROUNDS; i++)
    CHECK(play_round(&s, 1, 1) == 0);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_DONE);
  CHECK(fifa96_shootout_round(&s) == FIFA96_SHOOTOUT_MAX_ROUNDS);
  CHECK(fifa96_shootout_user_score(&s) == 20u);
  CHECK(fifa96_shootout_cpu_score(&s) == 20u);
  CHECK(fifa96_shootout_winner(&s) == FIFA96_SHOOTOUT_NO_WINNER);
  CHECK(fifa96_shootout_last_result(&s) != NULL);
  CHECK(fifa96_shootout_last_result(&s)->round == 20u);
  CHECK(fifa96_shootout_round_result(&s, 20u) != NULL);
  CHECK(fifa96_shootout_round_result(&s, 20u)->complete == 1u);
  CHECK(fifa96_shootout_round_result(&s, 21u) == NULL);
  CHECK(fifa96_shootout_select_kick(&s, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_select_dive(&s, FIFA96_SHOOTOUT_CHOICE_LEFT) == -1);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 20-20, tied after 20 rounds (cap)") == 0);
  CHECK(fifa96_shootout_score_string(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "20-20") == 0);
}

static void test_deterministic_same_seed(void) {
  struct fifa96_shootout a;
  struct fifa96_shootout b;
  struct fifa96_shootout c;
  struct fifa96_shootout d;

  memset(&a, 0, sizeof(a));
  memset(&b, 0, sizeof(b));
  CHECK(run_script(0x13579BDFu, 0u, &a) == 0);
  CHECK(run_script(0x13579BDFu, 0u, &b) == 0);
  CHECK(fifa96_shootout_phase(&a) == FIFA96_SHOOTOUT_PHASE_DONE);
  CHECK(shootouts_equal(&a, &b));

  CHECK(fifa96_shootout_init(&c, 0x13579BDFu, 0u) == 0);
  CHECK(fifa96_shootout_init(&d, 0x13579BDFu, 0u) == 0);
  CHECK(fifa96_shootout_cpu_keeper_dive(&c) ==
        fifa96_shootout_cpu_keeper_dive(&d));
  CHECK(fifa96_shootout_cpu_kick(&c) == fifa96_shootout_cpu_kick(&d));

  /* Different seeds do not produce the same round-1 keeper dive here. */
  CHECK(fifa96_shootout_init(&c, 1u, 0u) == 0);
  CHECK(fifa96_shootout_init(&d, 2u, 0u) == 0);
  CHECK(fifa96_shootout_cpu_keeper_dive(&c) !=
        fifa96_shootout_cpu_keeper_dive(&d));
}

static void test_reinit(void) {
  struct fifa96_shootout s;
  struct fifa96_shootout fresh;
  uint8_t keeper;

  CHECK(fifa96_shootout_init(&s, 0xABCDEF01u, 0u) == 0);
  keeper = fifa96_shootout_cpu_keeper_dive(&s);
  CHECK(fifa96_shootout_select_kick(&s, choice_other(keeper)) == 0);
  CHECK(fifa96_shootout_select_dive(&s, fifa96_shootout_cpu_kick(&s)) == 0);
  CHECK(fifa96_shootout_round(&s) == 2u);
  CHECK(fifa96_shootout_user_score(&s) == 1u);

  CHECK(fifa96_shootout_init(&s, 0x0F1E2D3Cu, 1u) == 0);
  CHECK(fifa96_shootout_phase(&s) == FIFA96_SHOOTOUT_PHASE_USER_KICK);
  CHECK(fifa96_shootout_round(&s) == 1u);
  CHECK(fifa96_shootout_user_score(&s) == 0u);
  CHECK(fifa96_shootout_cpu_score(&s) == 0u);
  CHECK(fifa96_shootout_winner(&s) == FIFA96_SHOOTOUT_NO_WINNER);
  CHECK(fifa96_shootout_last_result(&s) == NULL);
  CHECK(fifa96_shootout_round_result(&s, 1u)->user_kick ==
        FIFA96_SHOOTOUT_NO_CHOICE);
  CHECK(fifa96_shootout_round_result(&s, 1u)->cpu_dive ==
        FIFA96_SHOOTOUT_NO_CHOICE);
  CHECK(fifa96_shootout_round_result(&s, 1u)->cpu_kick ==
        FIFA96_SHOOTOUT_NO_CHOICE);
  CHECK(fifa96_shootout_round_result(&s, 1u)->user_dive ==
        FIFA96_SHOOTOUT_NO_CHOICE);
  CHECK(fifa96_shootout_round_result(&s, 1u)->complete == 0u);

  CHECK(fifa96_shootout_init(&fresh, 0x0F1E2D3Cu, 1u) == 0);
  CHECK(fifa96_shootout_cpu_keeper_dive(&s) ==
        fifa96_shootout_cpu_keeper_dive(&fresh));
  CHECK(fifa96_shootout_cpu_kick(&s) == fifa96_shootout_cpu_kick(&fresh));
  CHECK(fifa96_shootout_init(&s, 0xABCDEF01u, 0u) == 0);
  CHECK(fifa96_shootout_cpu_keeper_dive(&s) == keeper);
}

static void test_strings(void) {
  struct fifa96_shootout s;
  char buf[96];
  char tiny[3];

  CHECK(fifa96_shootout_init(&s, 9u, 0u) == 0);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 0-0, round 1/5") == 0);
  CHECK(strchr(buf, '\n') == NULL);
  CHECK(fifa96_shootout_score_string(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "0-0") == 0);

  CHECK(fifa96_shootout_summary(NULL, buf, sizeof(buf)) == -1);
  CHECK(fifa96_shootout_summary(&s, NULL, sizeof(buf)) == -1);
  CHECK(fifa96_shootout_summary(&s, buf, 0u) == -1);
  CHECK(fifa96_shootout_score_string(NULL, buf, sizeof(buf)) == -1);
  CHECK(fifa96_shootout_score_string(&s, NULL, sizeof(buf)) == -1);
  CHECK(fifa96_shootout_score_string(&s, buf, 0u) == -1);

  CHECK(fifa96_shootout_summary(&s, tiny, sizeof(tiny)) == -1);
  CHECK(tiny[0] == '\0');
  CHECK(fifa96_shootout_score_string(&s, tiny, sizeof(tiny)) == -1);
  CHECK(tiny[0] == '\0');

  CHECK(play_round(&s, 1, 1) == 0);
  CHECK(fifa96_shootout_summary(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "Pens: 1-1, round 2/5") == 0);
  CHECK(fifa96_shootout_score_string(&s, buf, sizeof(buf)) == 0);
  CHECK(strcmp(buf, "1-1") == 0);
}

int main(void) {
  test_init_validation();
  test_scoring_both_ways();
  test_regulation_and_no_early_clinch();
  test_sudden_death();
  test_cap();
  test_deterministic_same_seed();
  test_reinit();
  test_strings();

  if (failures != 0) {
    printf("test_engine_shootout: %d failure(s)\n", failures);
    return 1;
  }
  printf("test_engine_shootout: all assertions passed\n");
  return 0;
}
