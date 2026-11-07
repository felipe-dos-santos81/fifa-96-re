#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_shootout.h"

/* P14 — derived [D] penalty-shootout model.
 *
 * This is a pure state machine, not a port of native shootout code: FU-237
 * found no shootout entry point on the native end spine (FU237-L4). The
 * front-end owns the presentation; this file owns only the deterministic
 * rules, score, phase and round history.
 *
 * CPU choices are domain-separated hashes of seed + round + phase. They are
 * deliberately recomputable at any time, so a UI can label the outcome and a
 * test can derive the same choice without knowing the hash constants. The
 * model never allocates; all history lives in the fixed rounds[] array.
 *
 * Chosen early-clinch rule: none. Every regulation round takes both kicks and
 * the score is compared after round 5; sudden death also takes a complete
 * round before comparing. That keeps the round display and scoreboard complete
 * at every step. The cap is 20 total rounds.
 */

#define SHOOTOUT_SALT_KEEPER 0x4B1D0000u
#define SHOOTOUT_SALT_KICK 0x4B1D1000u

/* Deterministic 32-bit mixer (splitmix-style finalizer). */
static uint32_t shootout_mix32(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7FEB352DU;
  value ^= value >> 15;
  value *= 0x846CA68BU;
  value ^= value >> 16;
  return value;
}

static uint32_t shootout_hash(uint32_t seed, uint8_t round, uint8_t phase,
                              uint32_t salt) {
  return shootout_mix32(seed ^ ((uint32_t)round * 0x9E3779B9U) ^
                        ((uint32_t)phase * 0x85EBCA6BU) ^ salt);
}

static uint8_t shootout_cpu_choice(uint32_t seed, uint8_t round, uint8_t phase,
                                   uint32_t salt) {
  return (uint8_t)(shootout_hash(seed, round, phase, salt) % 3U);
}

static int shootout_choice_valid(uint8_t choice) {
  return choice <= (uint8_t)FIFA96_SHOOTOUT_CHOICE_RIGHT;
}

static void shootout_clear(struct fifa96_shootout *s) {
  unsigned i;

  memset(s, 0, sizeof(*s));
  s->phase = FIFA96_SHOOTOUT_PHASE_IDLE;
  s->winner = FIFA96_SHOOTOUT_NO_WINNER;
  for (i = 0u; i < FIFA96_SHOOTOUT_MAX_ROUNDS; i++) {
    s->rounds[i].user_kick = FIFA96_SHOOTOUT_NO_CHOICE;
    s->rounds[i].cpu_dive = FIFA96_SHOOTOUT_NO_CHOICE;
    s->rounds[i].cpu_kick = FIFA96_SHOOTOUT_NO_CHOICE;
    s->rounds[i].user_dive = FIFA96_SHOOTOUT_NO_CHOICE;
  }
}

/* Structural validation for caller-owned state. It rejects impossible scores
 * and a last result from the future, but does not attempt to audit every
 * historical choice. */
static int shootout_state_valid(const struct fifa96_shootout *s) {
  if (!s) return 0;
  if (s->user_side > 1u) return 0;
  if (s->phase > FIFA96_SHOOTOUT_PHASE_DONE) return 0;
  if (s->phase == FIFA96_SHOOTOUT_PHASE_IDLE)
    return s->round == 0u && s->user_score == 0u && s->cpu_score == 0u;
  if (s->round < 1u || s->round > FIFA96_SHOOTOUT_MAX_ROUNDS) return 0;
  if (s->user_score > s->round || s->cpu_score > s->round) return 0;
  if (s->phase != FIFA96_SHOOTOUT_PHASE_DONE &&
      s->winner != FIFA96_SHOOTOUT_NO_WINNER)
    return 0;
  if (s->phase == FIFA96_SHOOTOUT_PHASE_DONE &&
      s->winner != FIFA96_SHOOTOUT_NO_WINNER && s->winner > 1u)
    return 0;
  if (s->last.valid) {
    if (s->last.round < 1u || s->last.round > s->round) return 0;
    if (s->last.taker > FIFA96_SHOOTOUT_TAKER_CPU) return 0;
    if (!shootout_choice_valid(s->last.kick)) return 0;
    if (!shootout_choice_valid(s->last.opposite)) return 0;
    if (s->last.scored > 1u) return 0;
  }
  return 1;
}

int fifa96_shootout_init(struct fifa96_shootout *s, uint32_t seed,
                         uint8_t user_side) {
  if (!s) return -1;
  shootout_clear(s);
  if (user_side > 1u) return -1;
  s->seed = seed;
  s->user_side = user_side;
  s->phase = FIFA96_SHOOTOUT_PHASE_USER_KICK;
  s->round = 1u;
  return 0;
}

int fifa96_shootout_select_kick(struct fifa96_shootout *s, uint8_t choice) {
  struct fifa96_shootout_round *r;
  uint8_t dive;

  if (!shootout_state_valid(s)) return -1;
  if (s->phase != FIFA96_SHOOTOUT_PHASE_USER_KICK) return -1;
  if (!shootout_choice_valid(choice)) return -1;

  dive = shootout_cpu_choice(s->seed, s->round,
                             FIFA96_SHOOTOUT_PHASE_USER_KICK,
                             SHOOTOUT_SALT_KEEPER);
  r = &s->rounds[s->round - 1u];
  r->user_kick = choice;
  r->cpu_dive = dive;
  r->user_scored = choice != dive ? 1u : 0u;
  if (r->user_scored) s->user_score++;

  s->last.valid = 1u;
  s->last.taker = FIFA96_SHOOTOUT_TAKER_USER;
  s->last.side = s->user_side;
  s->last.round = s->round;
  s->last.kick = choice;
  s->last.opposite = dive;
  s->last.scored = r->user_scored;

  s->phase = FIFA96_SHOOTOUT_PHASE_USER_DIVE;
  return 0;
}

int fifa96_shootout_select_dive(struct fifa96_shootout *s, uint8_t choice) {
  struct fifa96_shootout_round *r;
  uint8_t kick;

  if (!shootout_state_valid(s)) return -1;
  if (s->phase != FIFA96_SHOOTOUT_PHASE_USER_DIVE) return -1;
  if (!shootout_choice_valid(choice)) return -1;

  kick = shootout_cpu_choice(s->seed, s->round,
                             FIFA96_SHOOTOUT_PHASE_USER_DIVE,
                             SHOOTOUT_SALT_KICK);
  r = &s->rounds[s->round - 1u];
  r->user_dive = choice;
  r->cpu_kick = kick;
  r->cpu_scored = kick != choice ? 1u : 0u;
  if (r->cpu_scored) s->cpu_score++;
  r->complete = 1u;

  s->last.valid = 1u;
  s->last.taker = FIFA96_SHOOTOUT_TAKER_CPU;
  s->last.side = (uint8_t)(1u - s->user_side);
  s->last.round = s->round;
  s->last.kick = kick;
  s->last.opposite = choice;
  s->last.scored = r->cpu_scored;

  if (s->round >= FIFA96_SHOOTOUT_REGULATION_ROUNDS &&
      s->user_score != s->cpu_score) {
    s->winner = s->user_score > s->cpu_score
                    ? s->user_side
                    : (uint8_t)(1u - s->user_side);
    s->phase = FIFA96_SHOOTOUT_PHASE_DONE;
  } else if (s->round >= FIFA96_SHOOTOUT_MAX_ROUNDS) {
    s->winner = FIFA96_SHOOTOUT_NO_WINNER;
    s->phase = FIFA96_SHOOTOUT_PHASE_DONE;
  } else {
    s->round++;
    s->phase = FIFA96_SHOOTOUT_PHASE_USER_KICK;
  }
  return 0;
}

uint8_t fifa96_shootout_phase(const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s)) return FIFA96_SHOOTOUT_PHASE_IDLE;
  return s->phase;
}

uint8_t fifa96_shootout_round(const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s)) return 0u;
  return s->round;
}

uint8_t fifa96_shootout_user_score(const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s)) return 0u;
  return s->user_score;
}

uint8_t fifa96_shootout_cpu_score(const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s)) return 0u;
  return s->cpu_score;
}

uint8_t fifa96_shootout_winner(const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s)) return FIFA96_SHOOTOUT_NO_WINNER;
  return s->winner;
}

const struct fifa96_shootout_last *fifa96_shootout_last_result(
    const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s) || !s->last.valid) return NULL;
  return &s->last;
}

const struct fifa96_shootout_round *fifa96_shootout_round_result(
    const struct fifa96_shootout *s, uint8_t round) {
  if (!shootout_state_valid(s)) return NULL;
  if (round < 1u || round > s->round) return NULL;
  return &s->rounds[round - 1u];
}

uint8_t fifa96_shootout_cpu_keeper_dive(const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s)) return FIFA96_SHOOTOUT_CHOICE_NONE;
  if (s->phase != FIFA96_SHOOTOUT_PHASE_USER_KICK &&
      s->phase != FIFA96_SHOOTOUT_PHASE_USER_DIVE)
    return FIFA96_SHOOTOUT_CHOICE_NONE;
  return shootout_cpu_choice(s->seed, s->round,
                             FIFA96_SHOOTOUT_PHASE_USER_KICK,
                             SHOOTOUT_SALT_KEEPER);
}

uint8_t fifa96_shootout_cpu_kick(const struct fifa96_shootout *s) {
  if (!shootout_state_valid(s)) return FIFA96_SHOOTOUT_CHOICE_NONE;
  if (s->phase != FIFA96_SHOOTOUT_PHASE_USER_KICK &&
      s->phase != FIFA96_SHOOTOUT_PHASE_USER_DIVE)
    return FIFA96_SHOOTOUT_CHOICE_NONE;
  return shootout_cpu_choice(s->seed, s->round,
                             FIFA96_SHOOTOUT_PHASE_USER_DIVE,
                             SHOOTOUT_SALT_KICK);
}

const char *fifa96_shootout_choice_name(uint8_t choice) {
  switch (choice) {
    case FIFA96_SHOOTOUT_CHOICE_LEFT:
      return "Left";
    case FIFA96_SHOOTOUT_CHOICE_CENTRE:
      return "Centre";
    case FIFA96_SHOOTOUT_CHOICE_RIGHT:
      return "Right";
    default:
      return "None";
  }
}

static int shootout_string_done(char *buf, size_t cap, int written) {
  if (written < 0 || (size_t)written >= cap) {
    if (cap > 0u) buf[0] = '\0';
    return -1;
  }
  return 0;
}

int fifa96_shootout_score_string(const struct fifa96_shootout *s, char *buf,
                                 size_t cap) {
  int written;

  if (!buf || cap == 0u) return -1;
  buf[0] = '\0';
  if (!shootout_state_valid(s)) return -1;

  written = snprintf(buf, cap, "%u-%u", (unsigned)s->user_score,
                     (unsigned)s->cpu_score);
  return shootout_string_done(buf, cap, written);
}

int fifa96_shootout_summary(const struct fifa96_shootout *s, char *buf,
                            size_t cap) {
  int written = -1;

  if (!buf || cap == 0u) return -1;
  buf[0] = '\0';
  if (!shootout_state_valid(s)) return -1;

  switch (s->phase) {
    case FIFA96_SHOOTOUT_PHASE_IDLE:
      written = snprintf(buf, cap, "Pens: idle");
      break;
    case FIFA96_SHOOTOUT_PHASE_USER_KICK:
    case FIFA96_SHOOTOUT_PHASE_USER_DIVE:
      if (s->round <= FIFA96_SHOOTOUT_REGULATION_ROUNDS) {
        written = snprintf(buf, cap, "Pens: %u-%u, round %u/%u",
                           (unsigned)s->user_score, (unsigned)s->cpu_score,
                           (unsigned)s->round,
                           (unsigned)FIFA96_SHOOTOUT_REGULATION_ROUNDS);
      } else {
        written = snprintf(buf, cap, "Pens: %u-%u, sudden death %u",
                           (unsigned)s->user_score, (unsigned)s->cpu_score,
                           (unsigned)(s->round -
                                      FIFA96_SHOOTOUT_REGULATION_ROUNDS));
      }
      break;
    case FIFA96_SHOOTOUT_PHASE_DONE:
      if (s->winner == s->user_side) {
        written = snprintf(buf, cap, "Pens: %u-%u, user wins after round %u",
                           (unsigned)s->user_score, (unsigned)s->cpu_score,
                           (unsigned)s->round);
      } else if (s->winner == (uint8_t)(1u - s->user_side)) {
        written = snprintf(buf, cap, "Pens: %u-%u, cpu wins after round %u",
                           (unsigned)s->user_score, (unsigned)s->cpu_score,
                           (unsigned)s->round);
      } else {
        written = snprintf(buf, cap,
                           "Pens: %u-%u, tied after %u rounds (cap)",
                           (unsigned)s->user_score, (unsigned)s->cpu_score,
                           (unsigned)s->round);
      }
      break;
    default:
      return -1;
  }
  return shootout_string_done(buf, cap, written);
}
