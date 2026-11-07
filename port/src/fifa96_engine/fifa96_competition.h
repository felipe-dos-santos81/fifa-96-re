#pragma once
#include <stddef.h>
#include <stdint.h>

/* P13 — derived single-player competition state.
 *
 * The native league/tournament/playoff modules (FUN_00028960 / FUN_0002F9DC /
 * FUN_00031AA4 and the shared result recorder FUN_00028588) are not ported as
 * data structures. This module implements the functional shell with the native
 * mode bytes (League 1, Tournament 2, Playoff 3, Practise 4), a deterministic
 * eight-team league and cup, and a serializable record. Native fixture/table
 * layouts and the native result rules remain documented legs.
 *
 * Team ids are opaque menu indices (0..43 in the current front-end). The
 * caller owns the pool; the model never resolves names. The user's fixture is
 * always presented as home (side 0) to the engine, so the match score maps
 * directly to home_goals/away_goals.
 */

#define FIFA96_COMPETITION_TEAMS 8
#define FIFA96_COMPETITION_FIXTURES 32
#define FIFA96_COMPETITION_ROUNDS 7
#define FIFA96_COMPETITION_NO_TEAM 0xFFu

enum fifa96_competition_kind {
  FIFA96_COMPETITION_NONE = 0,
  FIFA96_COMPETITION_LEAGUE = 1,
  FIFA96_COMPETITION_TOURNAMENT = 2,
  FIFA96_COMPETITION_PLAYOFF = 3,
  FIFA96_COMPETITION_PRACTISE = 4
};

enum fifa96_competition_state {
  FIFA96_COMPETITION_IDLE = 0,
  FIFA96_COMPETITION_ACTIVE = 1,
  FIFA96_COMPETITION_COMPLETE = 2
};

struct fifa96_competition_fixture {
  uint8_t round;
  uint8_t home;              /* index into teams[] */
  uint8_t away;              /* index into teams[] */
  uint8_t played;
  uint8_t home_goals;
  uint8_t away_goals;
  uint8_t replay;            /* cup draw replays used */
  uint8_t shootout;          /* 1 = decided by the derived shootout */
  uint8_t winner;            /* index into teams[], NO_TEAM if undecided */
  uint8_t user;              /* 1 = the user's fixture */
};

struct fifa96_competition {
  uint8_t kind;              /* enum fifa96_competition_kind */
  uint8_t state;             /* enum fifa96_competition_state */
  uint8_t user_team;         /* index into teams[] */
  uint8_t team_count;
  uint8_t teams[FIFA96_COMPETITION_TEAMS]; /* opaque menu team ids */
  uint8_t round;
  uint8_t rounds;
  uint8_t fixture_count;
  uint8_t current;           /* fixture index of the next user fixture */
  uint8_t champion;          /* teams[] index, NO_TEAM when none */
  uint8_t eliminated;        /* user lost a cup tie */
  uint32_t seed;
  struct fifa96_competition_fixture fixtures[FIFA96_COMPETITION_FIXTURES];
};

/* Reset to IDLE. */
void fifa96_competition_init(struct fifa96_competition *comp);

/* Build a competition.
 *
 * `kind` is LEAGUE/TOURNAMENT/PLAYOFF/PRACTISE. `user_team` is an index into
 * `pool` (0..pool_count-1); the model stores `pool[user_team]` as
 * `teams[user_team]`. League/cup need 8 teams; Practise needs 1. Missing pool
 * entries are filled by cycling the supplied entries, so a short synthetic
 * pool still yields a valid field. `seed` drives simulated scores and the
 * derived shootout. Returns 0, -1 on invalid arguments. */
int fifa96_competition_start(struct fifa96_competition *comp, uint8_t kind,
                             uint8_t user_team, const uint8_t *pool,
                             uint8_t pool_count, uint32_t seed);

/* The next user fixture. Fills teams[] indices and the fixture index; returns
 * 1 when one exists, 0 when the competition is complete or the user has no
 * remaining tie. `current` is updated. */
int fifa96_competition_next_fixture(struct fifa96_competition *comp,
                                    uint8_t *fixture_index, uint8_t *home,
                                    uint8_t *away);

/* Record the played user fixture (home/away goals). Cup draws increment the
 * replay counter and leave the same fixture next; after two replays the
 * derived shootout decides. Simulates the other fixtures in the round and
 * advances. Returns 0, -1 on invalid arguments/index. */
int fifa96_competition_apply_result(struct fifa96_competition *comp,
                                    uint8_t fixture_index, uint8_t home_goals,
                                    uint8_t away_goals);

/* Simulate the next user fixture and the rest of the round. Returns 1 when a
 * fixture was simulated, 0 when none is available, -1 on invalid. */
int fifa96_competition_simulate_next(struct fifa96_competition *comp);

/* Simulate every remaining fixture to completion (a user cup loss or an
 * explicit "skip season"). Returns 0, -1 on invalid. */
int fifa96_competition_simulate_all(struct fifa96_competition *comp);

/* Standings order for a league. Fills `order[team_count]` with teams[] indices
 * sorted by points, goal difference, goals for, then team id; returns the
 * number written (0 for a cup or a NULL). */
int fifa96_competition_standings(const struct fifa96_competition *comp,
                                 uint8_t *order);

/* Human-readable one-line summary for the hub/load list. Returns 0 on success,
 * -1 on invalid. The string is deterministic and never contains a newline. */
int fifa96_competition_summary(const struct fifa96_competition *comp,
                               char *buf, size_t cap);

/* Serialize/deserialize the fixed competition record. The caller owns the
 * buffer; the encoding is little-endian fields with a version byte. Returns 0
 * on success, -1 on invalid arguments, -2 on a short/unsupported buffer. */
#define FIFA96_COMPETITION_SERIAL_MAX 1024u
int fifa96_competition_serialize(const struct fifa96_competition *comp,
                                 uint8_t *buf, size_t cap, size_t *len);
int fifa96_competition_deserialize(struct fifa96_competition *comp,
                                   const uint8_t *buf, size_t len);

/* Fixture/table helpers for the hub. */
const struct fifa96_competition_fixture *fifa96_competition_fixture(
    const struct fifa96_competition *comp, uint8_t index);
int fifa96_competition_user_points(const struct fifa96_competition *comp);
const char *fifa96_competition_kind_name(uint8_t kind);
