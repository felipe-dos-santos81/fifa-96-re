#pragma once
#include <stddef.h>
#include <stdint.h>

/* Derived [D] penalty-shootout model.
 *
 * The native game has no proven shootout entry (FU-237 / FU237-L4); this is a
 * pure, player-facing model driven by the front-end after a cup draw. It is
 * not a port of native shootout code or data.
 *
 * Rules:
 *   - Three choices: LEFT, CENTRE, RIGHT. The user kicks first, then dives
 *     against the CPU kick. This order is always user-kick -> user-dive,
 *     independent of `user_side`; `user_side` only labels the game side.
 *   - A user kick scores when it differs from the CPU keeper dive. A CPU kick
 *     scores when it differs from the user's dive.
 *   - Five regulation rounds. Every round takes both kicks (no early clinch),
 *     so the round/scoreboard display is always complete. After round 5 the
 *     winner is the higher score; a tie starts sudden death.
 *   - Sudden death takes one complete round at a time and decides after each
 *     round if the scores differ. The hard cap is 20 total rounds; a tie after
 *     round 20 ends DONE with `FIFA96_SHOOTOUT_NO_WINNER`.
 *
 * The CPU choices are deterministic functions of seed + round + phase, so the
 * same seed and the same user choices reproduce the same shootout. The model
 * uses fixed arrays and never allocates.
 */

#define FIFA96_SHOOTOUT_REGULATION_ROUNDS 5u
#define FIFA96_SHOOTOUT_MAX_ROUNDS 20u
#define FIFA96_SHOOTOUT_NO_WINNER 0xFFu
#define FIFA96_SHOOTOUT_NO_CHOICE 0xFFu

enum fifa96_shootout_choice {
  FIFA96_SHOOTOUT_CHOICE_LEFT = 0,
  FIFA96_SHOOTOUT_CHOICE_CENTRE = 1,
  FIFA96_SHOOTOUT_CHOICE_RIGHT = 2,
  FIFA96_SHOOTOUT_CHOICE_NONE = FIFA96_SHOOTOUT_NO_CHOICE
};

enum fifa96_shootout_phase {
  FIFA96_SHOOTOUT_PHASE_IDLE = 0,
  FIFA96_SHOOTOUT_PHASE_USER_KICK = 1,
  FIFA96_SHOOTOUT_PHASE_USER_DIVE = 2,
  FIFA96_SHOOTOUT_PHASE_DONE = 3
};

enum fifa96_shootout_taker {
  FIFA96_SHOOTOUT_TAKER_USER = 0,
  FIFA96_SHOOTOUT_TAKER_CPU = 1
};

/* One taken kick, as exposed to the UI. `kick` is the taker's choice and
 * `opposite` is the keeper dive (user kick) or the user dive (CPU kick). */
struct fifa96_shootout_last {
  uint8_t valid;
  uint8_t taker;  /* enum fifa96_shootout_taker */
  uint8_t side;   /* game side 0/1 of the taker */
  uint8_t round;  /* 1-based */
  uint8_t kick;
  uint8_t opposite;
  uint8_t scored; /* 1 = goal, 0 = saved */
};

/* One round of the scoreboard. Choice fields are `NO_CHOICE` until taken. */
struct fifa96_shootout_round {
  uint8_t user_kick;
  uint8_t cpu_dive;
  uint8_t cpu_kick;
  uint8_t user_dive;
  uint8_t user_scored;
  uint8_t cpu_scored;
  uint8_t complete;
};

struct fifa96_shootout {
  uint32_t seed;
  uint8_t user_side; /* 0 or 1 */
  uint8_t phase;     /* enum fifa96_shootout_phase */
  uint8_t round;     /* 1-based current/last round; 0 in IDLE */
  uint8_t user_score;
  uint8_t cpu_score;
  uint8_t winner;    /* game side 0/1, or FIFA96_SHOOTOUT_NO_WINNER */
  struct fifa96_shootout_last last;
  struct fifa96_shootout_round rounds[FIFA96_SHOOTOUT_MAX_ROUNDS];
};

/* Start a shootout at USER_KICK, round 1. `user_side` is 0 or 1. Calling this
 * again fully resets the model (reinit/reset). On an invalid `user_side` the
 * object is reset to IDLE and -1 is returned. */
int fifa96_shootout_init(struct fifa96_shootout *s, uint32_t seed,
                         uint8_t user_side);

/* Take the user's kick. Valid only in USER_KICK with a LEFT/CENTRE/RIGHT
 * choice. Returns 0, -1 on NULL/out-of-phase/invalid choice. On success the
 * CPU keeper dive is recorded and the phase becomes USER_DIVE. */
int fifa96_shootout_select_kick(struct fifa96_shootout *s, uint8_t choice);

/* Take the user's dive against the CPU kick. Valid only in USER_DIVE with a
 * LEFT/CENTRE/RIGHT choice. Returns 0, -1 on NULL/out-of-phase/invalid choice.
 * On success the round completes, the score is updated, and the model either
 * advances to the next round or becomes DONE. */
int fifa96_shootout_select_dive(struct fifa96_shootout *s, uint8_t choice);

/* Queries; safe on NULL. IDLE is returned for an invalid/uninitialised
 * state. */
uint8_t fifa96_shootout_phase(const struct fifa96_shootout *s);
uint8_t fifa96_shootout_round(const struct fifa96_shootout *s);
uint8_t fifa96_shootout_user_score(const struct fifa96_shootout *s);
uint8_t fifa96_shootout_cpu_score(const struct fifa96_shootout *s);

/* Winning game side 0/1, or FIFA96_SHOOTOUT_NO_WINNER while in progress or
 * after a tied round-20 cap. */
uint8_t fifa96_shootout_winner(const struct fifa96_shootout *s);

/* Last taken kick, or NULL when none has been taken / on invalid state. The
 * pointer aliases the model and stays valid until the next selection/init. */
const struct fifa96_shootout_last *fifa96_shootout_last_result(
    const struct fifa96_shootout *s);

/* 1-based round record, including the in-progress round. Returns NULL for an
 * invalid/future round. The pointer aliases the model. */
const struct fifa96_shootout_round *fifa96_shootout_round_result(
    const struct fifa96_shootout *s, uint8_t round);

/* Deterministic CPU choice for the current round, derived from seed + round +
 * phase. The keeper dive opposes the user's kick; the CPU kick opposes the
 * user's dive. Both return FIFA96_SHOOTOUT_CHOICE_NONE outside active play. */
uint8_t fifa96_shootout_cpu_keeper_dive(const struct fifa96_shootout *s);
uint8_t fifa96_shootout_cpu_kick(const struct fifa96_shootout *s);

/* UI labels: "Left", "Centre", "Right", or "None". */
const char *fifa96_shootout_choice_name(uint8_t choice);

/* Compact score string, e.g. "3-2". Returns 0 on success, -1 on invalid
 * arguments/state or a buffer too small (the buffer is emptied on failure). */
int fifa96_shootout_score_string(const struct fifa96_shootout *s, char *buf,
                                 size_t cap);

/* One-line hub/screen summary, never containing a newline. Formats:
 *   "Pens: idle"
 *   "Pens: 2-1, round 3/5"
 *   "Pens: 5-5, sudden death 1"
 *   "Pens: 4-2, user wins after round 5"
 *   "Pens: 2-4, cpu wins after round 6"
 *   "Pens: 5-5, tied after 20 rounds (cap)"
 * Returns 0 on success, -1 on invalid arguments/state or a short buffer (the
 * buffer is emptied on failure). */
int fifa96_shootout_summary(const struct fifa96_shootout *s, char *buf,
                            size_t cap);
