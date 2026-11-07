#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_competition.h"

/* P13 — derived single-player competition model.
 *
 * This is the pure functional shell: an eight-team league (circle schedule),
 * an eight-team cup (Tournament/Playoff share the bracket), a Practise
 * descriptor, deterministic simulated scores, a bounded cup replay rule and a
 * deterministic shootout. It is derived [D], not a port of the native league /
 * cup tables or result rules.
 *
 * Layout assumptions used by the rest of this file:
 *   - League fixtures are stored round-major in groups of four, with the user
 *     fixture first inside each round. `current` is its index.
 *   - Cup rounds are created on demand: 4 fixtures (round 0), 2 (round 1), 1
 *     (round 2). The user fixture is first inside each round. `current` points
 *     at it while the user is alive.
 *   - The user is always presented as home (`fixtures[i].home`), so the
 *     engine's home/away score maps directly.
 *   - Practise is an ACTIVE competition with one team and no fixtures; it has
 *     nothing to play, so it stays ACTIVE and all fixture helpers report none.
 */

#define COMPETITION_SERIAL_MAGIC0 'F'
#define COMPETITION_SERIAL_MAGIC1 '9'
#define COMPETITION_SERIAL_MAGIC2 '6'
#define COMPETITION_SERIAL_MAGIC3 'C'
#define COMPETITION_SERIAL_VERSION 1u
#define COMPETITION_SERIAL_HEADER 29u
#define COMPETITION_SERIAL_FIXTURE 10u

static int competition_is_cup(uint8_t kind) {
  return kind == FIFA96_COMPETITION_TOURNAMENT ||
         kind == FIFA96_COMPETITION_PLAYOFF;
}

static uint8_t competition_cup_round_start(uint8_t round) {
  if (round == 0u) return 0u;
  if (round == 1u) return 4u;
  return 6u;
}

static uint8_t competition_cup_round_count(uint8_t round) {
  if (round == 0u) return 4u;
  if (round == 1u) return 2u;
  return 1u;
}

/* Deterministic 32-bit mixer (splitmix-style finalizer). */
static uint32_t competition_mix32(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7FEB352DU;
  value ^= value >> 15;
  value *= 0x846CA68BU;
  value ^= value >> 16;
  return value;
}

static uint32_t competition_hash(uint32_t seed, uint32_t salt) {
  return competition_mix32(seed ^ (salt * 0x9E3779B9U) ^ 0x85EBCA6BU);
}

/* Derived [D]: simulated scores are a deterministic function of the
 * competition seed and the fixture index. They are independent of call order,
 * so a save/load continues with the same results. */
static void competition_simulated_score(const struct fifa96_competition *comp,
                                        uint8_t fixture_index,
                                        uint8_t *home_goals,
                                        uint8_t *away_goals) {
  uint32_t h = competition_hash(comp->seed,
                                (uint32_t)fixture_index + 0x100u);
  *home_goals = (uint8_t)(h % 4u);
  *away_goals = (uint8_t)((h >> 8) % 4u);
}

/* Derived [D]: the same deterministic shootout decides a user third draw and
 * every simulated cup tie. The match score is deliberately left as the draw;
 * only the winner changes. */
static uint8_t competition_shootout_winner(
    const struct fifa96_competition *comp, uint8_t fixture_index) {
  uint32_t h = competition_hash(comp->seed,
                                (uint32_t)fixture_index + 0x200u);
  return (h & 1u) ? comp->fixtures[fixture_index].away
                  : comp->fixtures[fixture_index].home;
}

static void competition_store_league_result(struct fifa96_competition *comp,
                                            uint8_t fixture_index,
                                            uint8_t home_goals,
                                            uint8_t away_goals) {
  struct fifa96_competition_fixture *f = &comp->fixtures[fixture_index];
  f->played = 1u;
  f->home_goals = home_goals;
  f->away_goals = away_goals;
  if (home_goals > away_goals)
    f->winner = f->home;
  else if (away_goals > home_goals)
    f->winner = f->away;
  else
    f->winner = FIFA96_COMPETITION_NO_TEAM;
}

/* Simulated cup ties never draw: a level score is decided by the shootout. */
static void competition_store_cup_simulated(struct fifa96_competition *comp,
                                            uint8_t fixture_index,
                                            uint8_t home_goals,
                                            uint8_t away_goals) {
  struct fifa96_competition_fixture *f = &comp->fixtures[fixture_index];
  f->played = 1u;
  f->home_goals = home_goals;
  f->away_goals = away_goals;
  if (home_goals == away_goals) {
    f->shootout = 1u;
    f->winner = competition_shootout_winner(comp, fixture_index);
  } else {
    f->shootout = 0u;
    f->winner = home_goals > away_goals ? f->home : f->away;
  }
}

static void competition_simulate_league_rest(struct fifa96_competition *comp,
                                             uint8_t round) {
  uint8_t start = (uint8_t)(round * 4u);
  uint8_t i;
  for (i = start; i < (uint8_t)(start + 4u); i++) {
    if (comp->fixtures[i].played) continue;
    uint8_t home_goals = 0u;
    uint8_t away_goals = 0u;
    competition_simulated_score(comp, i, &home_goals, &away_goals);
    competition_store_league_result(comp, i, home_goals, away_goals);
  }
}

static void competition_league_advance(struct fifa96_competition *comp) {
  comp->round++;
  if (comp->round >= comp->rounds) {
    uint8_t order[FIFA96_COMPETITION_TEAMS];
    int n = fifa96_competition_standings(comp, order);
    comp->champion = n > 0 ? order[0] : FIFA96_COMPETITION_NO_TEAM;
    comp->state = FIFA96_COMPETITION_COMPLETE;
    comp->current = comp->fixture_count;
  } else {
    comp->current = (uint8_t)(comp->round * 4u);
  }
}

static void competition_league_finish_round(struct fifa96_competition *comp,
                                            uint8_t fixture_index) {
  (void)fixture_index;
  competition_simulate_league_rest(comp, comp->round);
  competition_league_advance(comp);
}

static void competition_league_finish_all(struct fifa96_competition *comp) {
  uint8_t i;
  for (i = 0u; i < comp->fixture_count; i++) {
    if (comp->fixtures[i].played) continue;
    uint8_t home_goals = 0u;
    uint8_t away_goals = 0u;
    competition_simulated_score(comp, i, &home_goals, &away_goals);
    competition_store_league_result(comp, i, home_goals, away_goals);
  }
  uint8_t order[FIFA96_COMPETITION_TEAMS];
  int n = fifa96_competition_standings(comp, order);
  comp->champion = n > 0 ? order[0] : FIFA96_COMPETITION_NO_TEAM;
  comp->state = FIFA96_COMPETITION_COMPLETE;
  comp->round = comp->rounds;
  comp->current = comp->fixture_count;
}

static int competition_cup_build_next(struct fifa96_competition *comp);

static void competition_cup_complete(struct fifa96_competition *comp) {
  uint8_t final_round = (uint8_t)(comp->rounds - 1u);
  uint8_t final_index = competition_cup_round_start(final_round);
  comp->champion = comp->fixtures[final_index].winner;
  comp->state = FIFA96_COMPETITION_COMPLETE;
  comp->round = comp->rounds;
  comp->current = comp->fixture_count;
}

/* Simulate every unplayed cup fixture in the current round, then create and
 * simulate the following rounds until the final. This is the "simulate all"
 * path and the "user lost, finish the bracket" path. A pending user replay
 * (`played` but no winner) is replayed here with no further replay: a draw is
 * decided by the shootout. */
static int competition_cup_finish(struct fifa96_competition *comp) {
  while (comp->state == FIFA96_COMPETITION_ACTIVE) {
    uint8_t start = competition_cup_round_start(comp->round);
    uint8_t count = competition_cup_round_count(comp->round);
    uint8_t i;
    for (i = start; i < (uint8_t)(start + count); i++) {
      struct fifa96_competition_fixture *f = &comp->fixtures[i];
      if (f->played && f->winner != FIFA96_COMPETITION_NO_TEAM) continue;
      uint8_t home_goals = 0u;
      uint8_t away_goals = 0u;
      competition_simulated_score(comp, i, &home_goals, &away_goals);
      competition_store_cup_simulated(comp, i, home_goals, away_goals);
      if (f->user && f->winner != comp->user_team) comp->eliminated = 1u;
    }
    if (comp->round + 1u >= comp->rounds) {
      competition_cup_complete(comp);
      return 0;
    }
    if (competition_cup_build_next(comp) != 0) return -1;
  }
  return 0;
}

/* Resolve the user's cup tie and advance the bracket. The caller has already
 * stored a decisive user result (or a third-draw shootout). */
static int competition_cup_advance(struct fifa96_competition *comp) {
  uint8_t start = competition_cup_round_start(comp->round);
  uint8_t count = competition_cup_round_count(comp->round);
  uint8_t i;
  for (i = start; i < (uint8_t)(start + count); i++) {
    struct fifa96_competition_fixture *f = &comp->fixtures[i];
    if (f->played && f->winner != FIFA96_COMPETITION_NO_TEAM) continue;
    uint8_t home_goals = 0u;
    uint8_t away_goals = 0u;
    competition_simulated_score(comp, i, &home_goals, &away_goals);
    competition_store_cup_simulated(comp, i, home_goals, away_goals);
    if (f->user && f->winner != comp->user_team) comp->eliminated = 1u;
  }
  if (comp->eliminated) return competition_cup_finish(comp);
  if (comp->round + 1u >= comp->rounds) {
    competition_cup_complete(comp);
    return 0;
  }
  return competition_cup_build_next(comp);
}

/* Build the next cup round from the current round's winners. The user fixture
 * is moved to the front of the new round so `current` stays a simple index. */
static int competition_cup_build_next(struct fifa96_competition *comp) {
  uint8_t next_round = (uint8_t)(comp->round + 1u);
  uint8_t start = competition_cup_round_start(comp->round);
  uint8_t count = competition_cup_round_count(comp->round);
  uint8_t next_start = competition_cup_round_start(next_round);
  uint8_t next_count = (uint8_t)(count / 2u);
  uint8_t i;

  if (next_round >= comp->rounds) return -1;
  if (next_start != comp->fixture_count) return -1;

  /* Validate every winner before writing any new fixture. */
  for (i = 0u; i < next_count; i++) {
    if (comp->fixtures[(uint8_t)(start + 2u * i)].winner ==
            FIFA96_COMPETITION_NO_TEAM ||
        comp->fixtures[(uint8_t)(start + 2u * i + 1u)].winner ==
            FIFA96_COMPETITION_NO_TEAM)
      return -1;
  }

  for (i = 0u; i < next_count; i++) {
    uint8_t first = comp->fixtures[(uint8_t)(start + 2u * i)].winner;
    uint8_t second = comp->fixtures[(uint8_t)(start + 2u * i + 1u)].winner;
    struct fifa96_competition_fixture *f =
        &comp->fixtures[(uint8_t)(next_start + i)];
    memset(f, 0, sizeof(*f));
    f->round = next_round;
    f->home = first;
    f->away = second;
    f->winner = FIFA96_COMPETITION_NO_TEAM;
    if (first == comp->user_team || second == comp->user_team) {
      f->home = comp->user_team;
      f->away = first == comp->user_team ? second : first;
      f->user = 1u;
    }
    comp->fixture_count++;
  }

  /* The user fixture is always the first fixture of its round. */
  if (next_count > 1u) {
    uint8_t user_offset = next_count;
    for (i = 0u; i < next_count; i++) {
      if (comp->fixtures[(uint8_t)(next_start + i)].user) {
        user_offset = i;
        break;
      }
    }
    if (user_offset > 0u && user_offset < next_count) {
      struct fifa96_competition_fixture tmp =
          comp->fixtures[next_start];
      comp->fixtures[next_start] =
          comp->fixtures[(uint8_t)(next_start + user_offset)];
      comp->fixtures[(uint8_t)(next_start + user_offset)] = tmp;
    }
  }

  comp->round = next_round;
  comp->current = next_start;
  return 0;
}

static int competition_model_valid(const struct fifa96_competition *comp) {
  uint8_t i;

  if (comp->kind > FIFA96_COMPETITION_PRACTISE) return 0;
  if (comp->state > FIFA96_COMPETITION_COMPLETE) return 0;
  if (comp->eliminated > 1u) return 0;
  if (comp->team_count > FIFA96_COMPETITION_TEAMS) return 0;
  if (comp->fixture_count > FIFA96_COMPETITION_FIXTURES) return 0;
  if (comp->current > comp->fixture_count) return 0;
  if (comp->rounds > FIFA96_COMPETITION_ROUNDS) return 0;
  if (comp->round > comp->rounds) return 0;
  if (comp->champion != FIFA96_COMPETITION_NO_TEAM &&
      comp->champion >= comp->team_count)
    return 0;

  switch (comp->kind) {
    case FIFA96_COMPETITION_NONE:
      if (comp->state != FIFA96_COMPETITION_IDLE) return 0;
      if (comp->user_team != 0u || comp->team_count != 0u ||
          comp->rounds != 0u || comp->fixture_count != 0u ||
          comp->current != 0u ||
          comp->champion != FIFA96_COMPETITION_NO_TEAM ||
          comp->eliminated != 0u)
        return 0;
      break;
    case FIFA96_COMPETITION_PRACTISE:
      if (comp->state == FIFA96_COMPETITION_IDLE) return 0;
      if (comp->user_team != 0u || comp->team_count != 1u ||
          comp->rounds != 0u || comp->fixture_count != 0u ||
          comp->current != 0u ||
          comp->champion != FIFA96_COMPETITION_NO_TEAM ||
          comp->eliminated != 0u)
        return 0;
      break;
    case FIFA96_COMPETITION_LEAGUE:
      if (comp->state == FIFA96_COMPETITION_IDLE) return 0;
      if (comp->team_count != FIFA96_COMPETITION_TEAMS ||
          comp->rounds != FIFA96_COMPETITION_ROUNDS ||
          comp->fixture_count != 28u ||
          comp->user_team >= FIFA96_COMPETITION_TEAMS ||
          comp->eliminated != 0u)
        return 0;
      if (comp->state == FIFA96_COMPETITION_ACTIVE) {
        if (comp->round >= FIFA96_COMPETITION_ROUNDS ||
            comp->champion != FIFA96_COMPETITION_NO_TEAM)
          return 0;
        if (comp->current != (uint8_t)(comp->round * 4u)) return 0;
        if (comp->current >= comp->fixture_count ||
            !comp->fixtures[comp->current].user ||
            comp->fixtures[comp->current].played)
          return 0;
      } else {
        if (comp->round != FIFA96_COMPETITION_ROUNDS ||
            comp->current != 28u ||
            comp->champion == FIFA96_COMPETITION_NO_TEAM)
          return 0;
      }
      for (i = 0u; i < comp->fixture_count; i++) {
        if (comp->fixtures[i].round != (uint8_t)(i / 4u)) return 0;
      }
      break;
    case FIFA96_COMPETITION_TOURNAMENT:
    case FIFA96_COMPETITION_PLAYOFF:
      if (comp->state == FIFA96_COMPETITION_IDLE) return 0;
      if (comp->team_count != FIFA96_COMPETITION_TEAMS ||
          comp->rounds != 3u ||
          comp->user_team >= FIFA96_COMPETITION_TEAMS)
        return 0;
      if (comp->state == FIFA96_COMPETITION_ACTIVE) {
        if (comp->round >= comp->rounds ||
            comp->champion != FIFA96_COMPETITION_NO_TEAM ||
            comp->eliminated != 0u)
          return 0;
        if (comp->fixture_count !=
            (uint8_t)(competition_cup_round_start(comp->round) +
                      competition_cup_round_count(comp->round)))
          return 0;
        if (comp->current != competition_cup_round_start(comp->round))
          return 0;
        if (comp->current >= comp->fixture_count ||
            !comp->fixtures[comp->current].user)
          return 0;
      } else {
        if (comp->fixture_count != 7u || comp->round != 3u ||
            comp->current != 7u ||
            comp->champion == FIFA96_COMPETITION_NO_TEAM)
          return 0;
        if (comp->fixtures[6].winner != comp->champion) return 0;
      }
      {
        uint8_t r;
        for (r = 0u; r < comp->rounds; r++) {
          uint8_t start = competition_cup_round_start(r);
          uint8_t count = competition_cup_round_count(r);
          uint8_t j;
          for (j = 0u; j < count; j++) {
            uint8_t index = (uint8_t)(start + j);
            if (index >= comp->fixture_count) break;
            if (comp->fixtures[index].round != r) return 0;
          }
        }
      }
      break;
    default:
      return 0;
  }

  for (i = 0u; i < comp->fixture_count; i++) {
    const struct fifa96_competition_fixture *f = &comp->fixtures[i];
    if (f->round >= comp->rounds) return 0;
    if (f->home >= comp->team_count || f->away >= comp->team_count) return 0;
    if (f->home == f->away) return 0;
    if (f->played > 1u || f->replay > 2u || f->shootout > 1u ||
        f->user > 1u)
      return 0;
    if (f->winner != FIFA96_COMPETITION_NO_TEAM &&
        f->winner != f->home && f->winner != f->away)
      return 0;
    if (f->shootout && f->winner == FIFA96_COMPETITION_NO_TEAM) return 0;
    if (f->user && f->home != comp->user_team) return 0;
  }
  return 1;
}

void fifa96_competition_init(struct fifa96_competition *comp) {
  if (!comp) return;
  memset(comp, 0, sizeof(*comp));
  comp->champion = FIFA96_COMPETITION_NO_TEAM;
}

int fifa96_competition_start(struct fifa96_competition *comp, uint8_t kind,
                             uint8_t user_team, const uint8_t *pool,
                             uint8_t pool_count, uint32_t seed) {
  uint8_t i;

  if (!comp || !pool || pool_count == 0u) return -1;
  if (kind < FIFA96_COMPETITION_LEAGUE ||
      kind > FIFA96_COMPETITION_PRACTISE)
    return -1;
  if (user_team >= pool_count) return -1;
  if (kind == FIFA96_COMPETITION_PRACTISE) {
    if (user_team != 0u) return -1;
  } else if (user_team >= FIFA96_COMPETITION_TEAMS) {
    return -1;
  }

  fifa96_competition_init(comp);
  comp->kind = kind;
  comp->state = FIFA96_COMPETITION_ACTIVE;
  comp->user_team = user_team;
  comp->seed = seed;

  if (kind == FIFA96_COMPETITION_PRACTISE) {
    comp->team_count = 1u;
    comp->teams[0] = pool[user_team];
    return 0;
  }

  comp->team_count = FIFA96_COMPETITION_TEAMS;
  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++)
    comp->teams[i] = pool[i % pool_count];

  if (kind == FIFA96_COMPETITION_LEAGUE) {
    /* Circle schedule: fix ring[0], rotate ring[1..7] each round. Every pair
     * meets exactly once. The user fixture is swapped to user-home and moved
     * to the front of its round. */
    uint8_t ring[FIFA96_COMPETITION_TEAMS] = {0u, 1u, 2u, 3u,
                                              4u, 5u, 6u, 7u};
    uint8_t round;
    uint8_t index = 0u;
    comp->rounds = FIFA96_COMPETITION_ROUNDS;
    for (round = 0u; round < FIFA96_COMPETITION_ROUNDS; round++) {
      uint8_t round_start = index;
      uint8_t pair;
      for (pair = 0u; pair < 4u; pair++) {
        struct fifa96_competition_fixture *f = &comp->fixtures[index];
        uint8_t home = ring[pair];
        uint8_t away = ring[FIFA96_COMPETITION_TEAMS - 1u - pair];
        memset(f, 0, sizeof(*f));
        f->round = round;
        f->home = home;
        f->away = away;
        f->winner = FIFA96_COMPETITION_NO_TEAM;
        if (home == user_team || away == user_team) {
          f->user = 1u;
          if (away == user_team) {
            f->home = away;
            f->away = home;
          }
        }
        index++;
      }
      /* Put the user fixture first inside this round. */
      for (pair = 0u; pair < 4u; pair++) {
        uint8_t at = (uint8_t)(round_start + pair);
        if (comp->fixtures[at].user && pair != 0u) {
          struct fifa96_competition_fixture tmp =
              comp->fixtures[round_start];
          comp->fixtures[round_start] = comp->fixtures[at];
          comp->fixtures[at] = tmp;
          break;
        }
      }
      {
        uint8_t rotate = ring[1];
        for (pair = 1u; pair < FIFA96_COMPETITION_TEAMS - 1u; pair++)
          ring[pair] = ring[(uint8_t)(pair + 1u)];
        ring[FIFA96_COMPETITION_TEAMS - 1u] = rotate;
      }
    }
    comp->fixture_count = 28u;
    comp->current = 0u;
    return 0;
  }

  /* Tournament / Playoff: the same derived eight-team bracket. */
  comp->rounds = 3u;
  {
    uint8_t pair;
    for (pair = 0u; pair < 4u; pair++) {
      struct fifa96_competition_fixture *f = &comp->fixtures[pair];
      uint8_t home = (uint8_t)(2u * pair);
      uint8_t away = (uint8_t)(2u * pair + 1u);
      memset(f, 0, sizeof(*f));
      f->round = 0u;
      f->home = home;
      f->away = away;
      f->winner = FIFA96_COMPETITION_NO_TEAM;
      if (home == user_team || away == user_team) {
        f->user = 1u;
        if (away == user_team) {
          f->home = away;
          f->away = home;
        }
      }
    }
    for (pair = 1u; pair < 4u; pair++) {
      if (comp->fixtures[pair].user) {
        struct fifa96_competition_fixture tmp = comp->fixtures[0];
        comp->fixtures[0] = comp->fixtures[pair];
        comp->fixtures[pair] = tmp;
        break;
      }
    }
  }
  comp->fixture_count = 4u;
  comp->current = 0u;
  return 0;
}

int fifa96_competition_next_fixture(struct fifa96_competition *comp,
                                    uint8_t *fixture_index, uint8_t *home,
                                    uint8_t *away) {
  uint8_t i;

  if (!comp || !fixture_index || !home || !away) return 0;
  if (comp->state != FIFA96_COMPETITION_ACTIVE) return 0;
  if (comp->kind != FIFA96_COMPETITION_LEAGUE &&
      !competition_is_cup(comp->kind))
    return 0;
  if (!competition_model_valid(comp)) return 0;
  if (comp->current >= comp->fixture_count) return 0;

  for (i = comp->current; i < comp->fixture_count; i++) {
    const struct fifa96_competition_fixture *f = &comp->fixtures[i];
    if (!f->user) continue;
    if (comp->kind == FIFA96_COMPETITION_LEAGUE) {
      if (f->played) continue;
    } else if (f->played && f->winner != FIFA96_COMPETITION_NO_TEAM) {
      continue;
    }
    comp->current = i;
    *fixture_index = i;
    *home = f->home;
    *away = f->away;
    return 1;
  }
  return 0;
}

int fifa96_competition_apply_result(struct fifa96_competition *comp,
                                    uint8_t fixture_index, uint8_t home_goals,
                                    uint8_t away_goals) {
  struct fifa96_competition_fixture *f;

  if (!comp) return -1;
  if (comp->state != FIFA96_COMPETITION_ACTIVE) return -1;
  if (comp->kind != FIFA96_COMPETITION_LEAGUE &&
      !competition_is_cup(comp->kind))
    return -1;
  if (!competition_model_valid(comp)) return -1;
  if (fixture_index >= comp->fixture_count) return -1;
  if (fixture_index != comp->current) return -1;
  f = &comp->fixtures[fixture_index];
  if (!f->user) return -1;
  if (f->round != comp->round) return -1;
  if (comp->kind == FIFA96_COMPETITION_LEAGUE) {
    if (f->played) return -1;
  } else if (f->played &&
             (f->winner != FIFA96_COMPETITION_NO_TEAM || f->shootout != 0u)) {
    return -1;
  }

  if (comp->kind == FIFA96_COMPETITION_LEAGUE) {
    competition_store_league_result(comp, fixture_index, home_goals,
                                    away_goals);
    competition_league_finish_round(comp, fixture_index);
    return 0;
  }

  f->played = 1u;
  f->home_goals = home_goals;
  f->away_goals = away_goals;
  if (home_goals == away_goals) {
    if (f->replay < 2u) {
      /* A cup draw is a replay: the same fixture stays next. */
      f->replay++;
      f->winner = FIFA96_COMPETITION_NO_TEAM;
      f->shootout = 0u;
      /* The rest of the round is played while the replay is pending. */
      {
        uint8_t start = competition_cup_round_start(comp->round);
        uint8_t count = competition_cup_round_count(comp->round);
        uint8_t i;
        for (i = start; i < (uint8_t)(start + count); i++) {
          struct fifa96_competition_fixture *other = &comp->fixtures[i];
          if (other->played &&
              other->winner != FIFA96_COMPETITION_NO_TEAM)
            continue;
          if (i == fixture_index) continue;
          uint8_t sim_home = 0u;
          uint8_t sim_away = 0u;
          competition_simulated_score(comp, i, &sim_home, &sim_away);
          competition_store_cup_simulated(comp, i, sim_home, sim_away);
        }
      }
      return 0;
    }
    /* Third draw: the derived shootout decides; the score stays a draw. */
    f->shootout = 1u;
    f->winner = competition_shootout_winner(comp, fixture_index);
  } else if (home_goals > away_goals) {
    f->winner = f->home;
  } else {
    f->winner = f->away;
  }

  if (f->winner != comp->user_team) comp->eliminated = 1u;
  if (competition_cup_advance(comp) != 0) return -1;
  return 0;
}

int fifa96_competition_simulate_next(struct fifa96_competition *comp) {
  uint8_t fixture_index = 0u;
  uint8_t home = 0u;
  uint8_t away = 0u;
  uint8_t home_goals = 0u;
  uint8_t away_goals = 0u;

  if (!comp) return -1;
  if (comp->state != FIFA96_COMPETITION_ACTIVE) return 0;
  if (comp->kind == FIFA96_COMPETITION_PRACTISE) return 0;
  if (comp->kind != FIFA96_COMPETITION_LEAGUE &&
      !competition_is_cup(comp->kind))
    return -1;
  if (!competition_model_valid(comp)) return -1;
  if (!fifa96_competition_next_fixture(comp, &fixture_index, &home, &away))
    return 0;
  (void)home;
  (void)away;

  competition_simulated_score(comp, fixture_index, &home_goals, &away_goals);
  if (comp->kind == FIFA96_COMPETITION_LEAGUE) {
    competition_store_league_result(comp, fixture_index, home_goals,
                                    away_goals);
    competition_league_finish_round(comp, fixture_index);
    return 1;
  }

  {
    struct fifa96_competition_fixture *f = &comp->fixtures[fixture_index];
    /* A user simulate is decisive immediately: no replay counter. */
    f->played = 1u;
    f->home_goals = home_goals;
    f->away_goals = away_goals;
    if (home_goals == away_goals) {
      f->shootout = 1u;
      f->winner = competition_shootout_winner(comp, fixture_index);
    } else {
      f->shootout = 0u;
      f->winner = home_goals > away_goals ? f->home : f->away;
    }
    if (f->winner != comp->user_team) comp->eliminated = 1u;
  }
  if (competition_cup_advance(comp) != 0) return -1;
  return 1;
}

int fifa96_competition_simulate_all(struct fifa96_competition *comp) {
  if (!comp) return -1;
  if (comp->kind == FIFA96_COMPETITION_NONE) return -1;
  if (comp->kind == FIFA96_COMPETITION_PRACTISE) return 0;
  if (comp->kind != FIFA96_COMPETITION_LEAGUE &&
      !competition_is_cup(comp->kind))
    return -1;
  if (comp->state == FIFA96_COMPETITION_COMPLETE) return 0;
  if (comp->state != FIFA96_COMPETITION_ACTIVE) return -1;
  if (!competition_model_valid(comp)) return -1;

  if (comp->kind == FIFA96_COMPETITION_LEAGUE) {
    competition_league_finish_all(comp);
    return 0;
  }
  return competition_cup_finish(comp);
}

int fifa96_competition_standings(const struct fifa96_competition *comp,
                                 uint8_t *order) {
  int points[FIFA96_COMPETITION_TEAMS];
  int goals_for[FIFA96_COMPETITION_TEAMS];
  int goals_against[FIFA96_COMPETITION_TEAMS];
  uint8_t n;
  uint8_t i;

  if (!comp || !order || comp->kind != FIFA96_COMPETITION_LEAGUE) return 0;
  if (comp->state == FIFA96_COMPETITION_IDLE) return 0;
  if (comp->fixture_count > FIFA96_COMPETITION_FIXTURES) return 0;
  n = comp->team_count;
  if (n > FIFA96_COMPETITION_TEAMS) return 0;

  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++) {
    points[i] = 0;
    goals_for[i] = 0;
    goals_against[i] = 0;
  }

  for (i = 0u; i < comp->fixture_count; i++) {
    const struct fifa96_competition_fixture *f = &comp->fixtures[i];
    if (!f->played) continue;
    if (f->home >= n || f->away >= n) continue;
    goals_for[f->home] += (int)f->home_goals;
    goals_against[f->home] += (int)f->away_goals;
    goals_for[f->away] += (int)f->away_goals;
    goals_against[f->away] += (int)f->home_goals;
    if (f->home_goals > f->away_goals) {
      points[f->home] += 3;
    } else if (f->away_goals > f->home_goals) {
      points[f->away] += 3;
    } else {
      points[f->home] += 1;
      points[f->away] += 1;
    }
  }

  for (i = 0u; i < n; i++) order[i] = i;

  /* Selection sort: points, goal difference, goals for, team id, then the
   * teams[] index as the final total-order fallback. */
  for (i = 0u; i < n; i++) {
    uint8_t best = i;
    uint8_t j;
    for (j = (uint8_t)(i + 1u); j < n; j++) {
      uint8_t current_team = order[best];
      uint8_t candidate_team = order[j];
      int best_gd = goals_for[current_team] - goals_against[current_team];
      int j_gd = goals_for[candidate_team] - goals_against[candidate_team];
      int better = 0;
      if (points[candidate_team] != points[current_team])
        better = points[candidate_team] > points[current_team];
      else if (j_gd != best_gd)
        better = j_gd > best_gd;
      else if (goals_for[candidate_team] != goals_for[current_team])
        better = goals_for[candidate_team] > goals_for[current_team];
      else if (comp->teams[candidate_team] != comp->teams[current_team])
        better = comp->teams[candidate_team] < comp->teams[current_team];
      else
        better = candidate_team < current_team;
      if (better) best = j;
    }
    if (best != i) {
      uint8_t tmp = order[i];
      order[i] = order[best];
      order[best] = tmp;
    }
  }
  return (int)n;
}

int fifa96_competition_summary(const struct fifa96_competition *comp,
                               char *buf, size_t cap) {
  int written = -1;

  if (!comp || !buf || cap == 0u) return -1;
  if (!competition_model_valid(comp)) return -1;

  switch (comp->kind) {
    case FIFA96_COMPETITION_NONE:
      written = snprintf(buf, cap, "None");
      break;
    case FIFA96_COMPETITION_PRACTISE:
      written = snprintf(buf, cap, "Practise: team %u",
                         (unsigned)comp->teams[0]);
      break;
    case FIFA96_COMPETITION_LEAGUE:
      if (comp->state == FIFA96_COMPETITION_COMPLETE) {
        written = snprintf(buf, cap,
                           "League: team %u, champion team %u, %d pts",
                           (unsigned)comp->teams[comp->user_team],
                           (unsigned)comp->teams[comp->champion],
                           fifa96_competition_user_points(comp));
      } else {
        written = snprintf(buf, cap, "League: team %u, round %u/%u, %d pts",
                           (unsigned)comp->teams[comp->user_team],
                           (unsigned)(comp->round + 1u),
                           (unsigned)comp->rounds,
                           fifa96_competition_user_points(comp));
      }
      break;
    case FIFA96_COMPETITION_TOURNAMENT:
    case FIFA96_COMPETITION_PLAYOFF:
      if (comp->state == FIFA96_COMPETITION_COMPLETE) {
        written = snprintf(buf, cap, "%s: team %u, %s, champion team %u",
                           fifa96_competition_kind_name(comp->kind),
                           (unsigned)comp->teams[comp->user_team],
                           comp->eliminated ? "eliminated" : "winner",
                           (unsigned)comp->teams[comp->champion]);
      } else {
        written = snprintf(buf, cap, "%s: team %u, round %u/%u",
                           fifa96_competition_kind_name(comp->kind),
                           (unsigned)comp->teams[comp->user_team],
                           (unsigned)(comp->round + 1u),
                           (unsigned)comp->rounds);
      }
      break;
    default:
      return -1;
  }
  if (written < 0 || (size_t)written >= cap) return -1;
  return 0;
}

int fifa96_competition_serialize(const struct fifa96_competition *comp,
                                 uint8_t *buf, size_t cap, size_t *len) {
  size_t total;
  size_t off = 0u;
  uint8_t i;

  if (!comp || !buf || !len) return -1;
  *len = 0u;
  if (!competition_model_valid(comp)) return -1;
  total = COMPETITION_SERIAL_HEADER +
          (size_t)comp->fixture_count * COMPETITION_SERIAL_FIXTURE;
  if (total > FIFA96_COMPETITION_SERIAL_MAX) return -2;
  if (cap < total) return -2;

  buf[off++] = (uint8_t)COMPETITION_SERIAL_MAGIC0;
  buf[off++] = (uint8_t)COMPETITION_SERIAL_MAGIC1;
  buf[off++] = (uint8_t)COMPETITION_SERIAL_MAGIC2;
  buf[off++] = (uint8_t)COMPETITION_SERIAL_MAGIC3;
  buf[off++] = (uint8_t)COMPETITION_SERIAL_VERSION;
  buf[off++] = (uint8_t)(total & 0xFFu);
  buf[off++] = (uint8_t)((total >> 8) & 0xFFu);
  buf[off++] = comp->kind;
  buf[off++] = comp->state;
  buf[off++] = comp->user_team;
  buf[off++] = comp->team_count;
  buf[off++] = comp->round;
  buf[off++] = comp->rounds;
  buf[off++] = comp->fixture_count;
  buf[off++] = comp->current;
  buf[off++] = comp->champion;
  buf[off++] = comp->eliminated;
  buf[off++] = (uint8_t)(comp->seed & 0xFFu);
  buf[off++] = (uint8_t)((comp->seed >> 8) & 0xFFu);
  buf[off++] = (uint8_t)((comp->seed >> 16) & 0xFFu);
  buf[off++] = (uint8_t)((comp->seed >> 24) & 0xFFu);
  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++)
    buf[off++] = comp->teams[i];
  for (i = 0u; i < comp->fixture_count; i++) {
    const struct fifa96_competition_fixture *f = &comp->fixtures[i];
    buf[off++] = f->round;
    buf[off++] = f->home;
    buf[off++] = f->away;
    buf[off++] = f->played;
    buf[off++] = f->home_goals;
    buf[off++] = f->away_goals;
    buf[off++] = f->replay;
    buf[off++] = f->shootout;
    buf[off++] = f->winner;
    buf[off++] = f->user;
  }

  if (off != total) return -1;
  *len = off;
  return 0;
}

int fifa96_competition_deserialize(struct fifa96_competition *comp,
                                   const uint8_t *buf, size_t len) {
  struct fifa96_competition tmp;
  uint16_t declared;
  uint8_t fixture_count;
  size_t expected;
  size_t off;
  uint8_t i;

  if (!comp || !buf) return -1;
  if (len < 7u) return -2;
  if (buf[0] != (uint8_t)COMPETITION_SERIAL_MAGIC0 ||
      buf[1] != (uint8_t)COMPETITION_SERIAL_MAGIC1 ||
      buf[2] != (uint8_t)COMPETITION_SERIAL_MAGIC2 ||
      buf[3] != (uint8_t)COMPETITION_SERIAL_MAGIC3)
    return -2;
  if (buf[4] != (uint8_t)COMPETITION_SERIAL_VERSION) return -2;
  declared = (uint16_t)((uint16_t)buf[5] | ((uint16_t)buf[6] << 8));
  if ((size_t)declared != len) return -2;
  if (len < COMPETITION_SERIAL_HEADER) return -2;
  fixture_count = buf[13];
  if (fixture_count > FIFA96_COMPETITION_FIXTURES) return -2;
  expected = COMPETITION_SERIAL_HEADER +
             (size_t)fixture_count * COMPETITION_SERIAL_FIXTURE;
  if (len != expected) return -2;

  memset(&tmp, 0, sizeof(tmp));
  off = 7u;
  tmp.kind = buf[off++];
  tmp.state = buf[off++];
  tmp.user_team = buf[off++];
  tmp.team_count = buf[off++];
  tmp.round = buf[off++];
  tmp.rounds = buf[off++];
  tmp.fixture_count = buf[off++];
  tmp.current = buf[off++];
  tmp.champion = buf[off++];
  tmp.eliminated = buf[off++];
  tmp.seed = (uint32_t)buf[off] | ((uint32_t)buf[off + 1u] << 8) |
             ((uint32_t)buf[off + 2u] << 16) |
             ((uint32_t)buf[off + 3u] << 24);
  off += 4u;
  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++) tmp.teams[i] = buf[off++];
  for (i = 0u; i < fixture_count; i++) {
    struct fifa96_competition_fixture *f = &tmp.fixtures[i];
    f->round = buf[off++];
    f->home = buf[off++];
    f->away = buf[off++];
    f->played = buf[off++];
    f->home_goals = buf[off++];
    f->away_goals = buf[off++];
    f->replay = buf[off++];
    f->shootout = buf[off++];
    f->winner = buf[off++];
    f->user = buf[off++];
  }
  if (off != len) return -2;
  if (!competition_model_valid(&tmp)) return -2;
  *comp = tmp;
  return 0;
}

const struct fifa96_competition_fixture *fifa96_competition_fixture(
    const struct fifa96_competition *comp, uint8_t index) {
  if (!comp || index >= comp->fixture_count ||
      index >= FIFA96_COMPETITION_FIXTURES)
    return NULL;
  return &comp->fixtures[index];
}

int fifa96_competition_user_points(const struct fifa96_competition *comp) {
  int points = 0;
  uint8_t i;

  if (!comp || comp->kind != FIFA96_COMPETITION_LEAGUE) return 0;
  if (comp->state == FIFA96_COMPETITION_IDLE) return 0;
  if (comp->fixture_count > FIFA96_COMPETITION_FIXTURES) return 0;
  for (i = 0u; i < comp->fixture_count; i++) {
    const struct fifa96_competition_fixture *f = &comp->fixtures[i];
    uint8_t user_goals;
    uint8_t other_goals;
    if (!f->played) continue;
    if (f->home != comp->user_team && f->away != comp->user_team) continue;
    if (f->home == comp->user_team) {
      user_goals = f->home_goals;
      other_goals = f->away_goals;
    } else {
      user_goals = f->away_goals;
      other_goals = f->home_goals;
    }
    if (user_goals > other_goals)
      points += 3;
    else if (user_goals == other_goals)
      points += 1;
  }
  return points;
}

const char *fifa96_competition_kind_name(uint8_t kind) {
  switch (kind) {
    case FIFA96_COMPETITION_NONE:
      return "None";
    case FIFA96_COMPETITION_LEAGUE:
      return "League";
    case FIFA96_COMPETITION_TOURNAMENT:
      return "Tournament";
    case FIFA96_COMPETITION_PLAYOFF:
      return "Playoff";
    case FIFA96_COMPETITION_PRACTISE:
      return "Practise";
    default:
      return "Unknown";
  }
}
