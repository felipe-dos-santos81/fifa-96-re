/* test_engine_competition.c — P13 derived single-player competition model.
 *
 * Standalone unit test (no ISO, no engine): league circle schedule and table,
 * cup bracket with replays and the derived shootout, Practise, simulation,
 * serialization round-trips and malformed/truncated buffers, and the summary
 * line. The model is derived [D]; this test pins the port contract, not native
 * competition behaviour. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_competition.h"

static const uint8_t test_pool[FIFA96_COMPETITION_TEAMS] = {
    10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u};

static unsigned played_count(const struct fifa96_competition *comp) {
  unsigned count = 0;
  uint8_t i;
  for (i = 0u; i < comp->fixture_count; i++)
    if (comp->fixtures[i].played) count++;
  return count;
}

static int has_newline(const char *text) {
  return strchr(text, '\n') != NULL;
}

struct test_stats {
  int points;
  int goals_for;
  int goals_against;
};

static void compute_stats(const struct fifa96_competition *comp,
                          struct test_stats stats[FIFA96_COMPETITION_TEAMS]) {
  uint8_t i;
  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++) {
    stats[i].points = 0;
    stats[i].goals_for = 0;
    stats[i].goals_against = 0;
  }
  for (i = 0u; i < comp->fixture_count; i++) {
    const struct fifa96_competition_fixture *f = &comp->fixtures[i];
    if (!f->played) continue;
    stats[f->home].goals_for += (int)f->home_goals;
    stats[f->home].goals_against += (int)f->away_goals;
    stats[f->away].goals_for += (int)f->away_goals;
    stats[f->away].goals_against += (int)f->home_goals;
    if (f->home_goals > f->away_goals)
      stats[f->home].points += 3;
    else if (f->away_goals > f->home_goals)
      stats[f->away].points += 3;
    else {
      stats[f->home].points += 1;
      stats[f->away].points += 1;
    }
  }
}

/* Mirrors the header's standings order: points, GD, GF, opaque team id, then
 * teams[] index as the total-order fallback. */
static int stats_rank_before(const struct fifa96_competition *comp,
                             const struct test_stats *stats, uint8_t a,
                             uint8_t b) {
  int a_gd = stats[a].goals_for - stats[a].goals_against;
  int b_gd = stats[b].goals_for - stats[b].goals_against;
  if (stats[a].points != stats[b].points)
    return stats[a].points > stats[b].points;
  if (a_gd != b_gd) return a_gd > b_gd;
  if (stats[a].goals_for != stats[b].goals_for)
    return stats[a].goals_for > stats[b].goals_for;
  if (comp->teams[a] != comp->teams[b]) return comp->teams[a] < comp->teams[b];
  return a < b;
}

static void compare_competitions(const struct fifa96_competition *a,
                                 const struct fifa96_competition *b) {
  uint8_t i;
  assert(a->kind == b->kind);
  assert(a->state == b->state);
  assert(a->user_team == b->user_team);
  assert(a->team_count == b->team_count);
  assert(a->round == b->round);
  assert(a->rounds == b->rounds);
  assert(a->fixture_count == b->fixture_count);
  assert(a->current == b->current);
  assert(a->champion == b->champion);
  assert(a->eliminated == b->eliminated);
  assert(a->seed == b->seed);
  assert(memcmp(a->teams, b->teams, sizeof(a->teams)) == 0);
  for (i = 0u; i < a->fixture_count; i++) {
    assert(a->fixtures[i].round == b->fixtures[i].round);
    assert(a->fixtures[i].home == b->fixtures[i].home);
    assert(a->fixtures[i].away == b->fixtures[i].away);
    assert(a->fixtures[i].played == b->fixtures[i].played);
    assert(a->fixtures[i].home_goals == b->fixtures[i].home_goals);
    assert(a->fixtures[i].away_goals == b->fixtures[i].away_goals);
    assert(a->fixtures[i].replay == b->fixtures[i].replay);
    assert(a->fixtures[i].shootout == b->fixtures[i].shootout);
    assert(a->fixtures[i].winner == b->fixtures[i].winner);
    assert(a->fixtures[i].user == b->fixtures[i].user);
  }
}

static void test_start_validation(void) {
  struct fifa96_competition comp;
  uint8_t pool[FIFA96_COMPETITION_TEAMS] = {0u, 1u, 2u, 3u,
                                            4u, 5u, 6u, 7u};
  uint8_t fixture_index = 99u;
  uint8_t home = 99u;
  uint8_t away = 99u;
  uint8_t i;

  fifa96_competition_init(&comp);
  assert(comp.kind == FIFA96_COMPETITION_NONE);
  assert(comp.state == FIFA96_COMPETITION_IDLE);
  assert(comp.champion == FIFA96_COMPETITION_NO_TEAM);
  assert(comp.team_count == 0u);
  assert(comp.fixture_count == 0u);
  assert(comp.rounds == 0u);
  fifa96_competition_init(NULL);

  assert(fifa96_competition_next_fixture(NULL, &fixture_index, &home,
                                         &away) == 0);
  assert(fifa96_competition_next_fixture(&comp, NULL, &home, &away) == 0);
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, NULL,
                                         &away) == 0);
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         NULL) == 0);
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 0);

  assert(fifa96_competition_start(NULL, FIFA96_COMPETITION_LEAGUE, 0u, pool,
                                  8u, 1u) == -1);
  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_NONE, 0u, pool,
                                  8u, 1u) == -1);
  assert(fifa96_competition_start(&comp, 5u, 0u, pool, 8u, 1u) == -1);
  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 0u, NULL,
                                  8u, 1u) == -1);
  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 0u, pool,
                                  0u, 1u) == -1);
  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 8u, pool,
                                  8u, 1u) == -1);

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 0u, pool,
                                  8u, 1u) == 0);
  assert(comp.kind == FIFA96_COMPETITION_LEAGUE);
  assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  assert(comp.user_team == 0u);
  assert(comp.team_count == FIFA96_COMPETITION_TEAMS);
  assert(comp.rounds == FIFA96_COMPETITION_ROUNDS);
  assert(comp.fixture_count == 28u);
  assert(comp.current == 0u);
  assert(comp.champion == FIFA96_COMPETITION_NO_TEAM);
  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++)
    assert(comp.teams[i] == pool[i]);
  assert(comp.fixtures[0].user == 1u);
  assert(comp.fixtures[0].home == 0u);

  assert(fifa96_competition_apply_result(NULL, 0u, 1u, 0u) == -1);
  assert(fifa96_competition_apply_result(&comp, 1u, 1u, 0u) == -1);
  assert(fifa96_competition_apply_result(&comp, 28u, 1u, 0u) == -1);
  assert(fifa96_competition_apply_result(&comp, 0u, 1u, 0u) == 0);
  assert(fifa96_competition_apply_result(&comp, 0u, 1u, 0u) == -1);

  /* A short pool is cycled into the eight-team field. */
  {
    uint8_t short_pool[3] = {40u, 41u, 42u};
    assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 1u,
                                    short_pool, 3u, 7u) == 0);
    assert(comp.team_count == FIFA96_COMPETITION_TEAMS);
    assert(comp.teams[0] == 40u);
    assert(comp.teams[1] == 41u);
    assert(comp.teams[2] == 42u);
    assert(comp.teams[3] == 40u);
    assert(comp.teams[4] == 41u);
    assert(comp.teams[5] == 42u);
    assert(comp.teams[6] == 40u);
    assert(comp.teams[7] == 41u);
    assert(comp.user_team == 1u);
  }

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_PRACTISE, 1u,
                                  pool, 3u, 1u) == -1);
  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_PRACTISE, 0u,
                                  pool, 3u, 1u) == 0);
  assert(comp.kind == FIFA96_COMPETITION_PRACTISE);
  assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  assert(comp.team_count == 1u);
  assert(comp.teams[0] == pool[0]);
  assert(comp.rounds == 0u);
  assert(comp.fixture_count == 0u);
  assert(comp.current == 0u);
  assert(comp.champion == FIFA96_COMPETITION_NO_TEAM);

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_TOURNAMENT, 0u,
                                  pool, 8u, 1u) == 0);
  assert(comp.kind == FIFA96_COMPETITION_TOURNAMENT);
  assert(comp.rounds == 3u);
  assert(comp.fixture_count == 4u);
  assert(comp.round == 0u);
  assert(comp.current == 0u);

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_PLAYOFF, 7u, pool,
                                  8u, 1u) == 0);
  assert(comp.kind == FIFA96_COMPETITION_PLAYOFF);
  assert(comp.fixture_count == 4u);
  assert(comp.fixtures[0].user == 1u);
  assert(comp.fixtures[0].home == 7u);
}

static void test_league_round_robin(void) {
  struct fifa96_competition comp;
  struct test_stats stats[FIFA96_COMPETITION_TEAMS];
  uint8_t opponents[7];
  uint8_t opponent_count = 0u;
  uint8_t pairs[FIFA96_COMPETITION_TEAMS][FIFA96_COMPETITION_TEAMS];
  uint8_t order[FIFA96_COMPETITION_TEAMS];
  uint8_t round;
  uint8_t i;
  uint8_t j;
  uint8_t out_index = 0u;
  uint8_t out_home = 0u;
  uint8_t out_away = 0u;
  unsigned pair_count = 0u;

  memset(pairs, 0, sizeof(pairs));
  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 3u,
                                  test_pool, 8u, 0x12345678u) == 0);

  for (round = 0u; round < FIFA96_COMPETITION_ROUNDS; round++) {
    uint8_t fixture_index = 0u;
    uint8_t home = 0u;
    uint8_t away = 0u;
    assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                           &away) == 1);
    assert(fixture_index == (uint8_t)(round * 4u));
    assert(home == 3u);
    assert(away < FIFA96_COMPETITION_TEAMS);
    assert(away != 3u);
    for (j = 0u; j < opponent_count; j++) assert(opponents[j] != away);
    opponents[opponent_count++] = away;
    assert(comp.fixtures[fixture_index].user == 1u);
    assert(comp.fixtures[fixture_index].home == 3u);

    for (j = 0u; j < 4u; j++) {
      uint8_t at = (uint8_t)(round * 4u + j);
      const struct fifa96_competition_fixture *f =
          fifa96_competition_fixture(&comp, at);
      assert(f != NULL);
      assert(f->round == round);
      assert(f->played == 0u);
      assert(f->home != f->away);
      pairs[f->home][f->away] = 1u;
      pairs[f->away][f->home] = 1u;
    }

    assert(fifa96_competition_apply_result(&comp, fixture_index, 2u, 0u) == 0);
    assert(played_count(&comp) == (unsigned)((round + 1u) * 4u));
    for (j = 0u; j < 4u; j++)
      assert(comp.fixtures[(uint8_t)(round * 4u + j)].played == 1u);
    if (round < FIFA96_COMPETITION_ROUNDS - 1u) {
      assert(comp.state == FIFA96_COMPETITION_ACTIVE);
      assert(comp.round == (uint8_t)(round + 1u));
      assert(comp.current == (uint8_t)((round + 1u) * 4u));
    }
  }

  assert(opponent_count == 7u);
  for (i = 0u; i < 7u; i++) {
    for (j = (uint8_t)(i + 1u); j < 7u; j++)
      assert(opponents[i] != opponents[j]);
  }

  assert(comp.state == FIFA96_COMPETITION_COMPLETE);
  assert(comp.round == FIFA96_COMPETITION_ROUNDS);
  assert(comp.current == 28u);
  assert(comp.fixture_count == 28u);
  assert(comp.eliminated == 0u);
  assert(comp.champion == 3u);
  assert(fifa96_competition_user_points(&comp) == 21);

  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++) {
    for (j = (uint8_t)(i + 1u); j < FIFA96_COMPETITION_TEAMS; j++) {
      assert(pairs[i][j] == 1u);
      pair_count++;
    }
  }
  assert(pair_count == 28u);

  memset(order, 0xFF, sizeof(order));
  assert(fifa96_competition_standings(&comp, order) == 8);
  compute_stats(&comp, stats);
  for (i = 0u; i < 8u; i++) {
    assert(order[i] < FIFA96_COMPETITION_TEAMS);
    for (j = (uint8_t)(i + 1u); j < 8u; j++) assert(order[i] != order[j]);
  }
  for (i = 0u; i < 7u; i++)
    assert(stats_rank_before(&comp, stats, order[i], order[i + 1u]));
  assert(order[0] == comp.champion);
  assert(stats[3].points == 21);

  assert(fifa96_competition_fixture(&comp, 28u) == NULL);
  assert(fifa96_competition_fixture(&comp, 200u) == NULL);
  assert(fifa96_competition_next_fixture(&comp, &out_index, &out_home,
                                         &out_away) == 0);
  assert(fifa96_competition_apply_result(&comp, 0u, 1u, 0u) == -1);
}

static void test_standings_team_id_tiebreak(void) {
  struct fifa96_competition comp;
  uint8_t descending_pool[FIFA96_COMPETITION_TEAMS] = {80u, 70u, 60u, 50u,
                                                        40u, 30u, 20u, 10u};
  uint8_t order[FIFA96_COMPETITION_TEAMS];
  uint8_t i;

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 0u,
                                  descending_pool, 8u, 0x1111u) == 0);
  /* Every fixture a 1-1 draw: all teams have 7 points, GD 0 and GF 7, so the
   * opaque team id is the decisive tie-break. The pool is descending, making
   * the expected order the reverse of the teams[] indices. */
  for (i = 0u; i < comp.fixture_count; i++) {
    comp.fixtures[i].played = 1u;
    comp.fixtures[i].home_goals = 1u;
    comp.fixtures[i].away_goals = 1u;
    comp.fixtures[i].winner = FIFA96_COMPETITION_NO_TEAM;
  }
  assert(fifa96_competition_standings(&comp, order) == 8);
  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++)
    assert(order[i] == (uint8_t)(7u - i));
  assert(fifa96_competition_user_points(&comp) == 7);
}

static void test_league_simulate(void) {
  struct fifa96_competition comp;
  struct fifa96_competition twin;
  struct test_stats stats[FIFA96_COMPETITION_TEAMS];
  uint8_t order[FIFA96_COMPETITION_TEAMS];
  uint8_t round;

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 2u,
                                  test_pool, 8u, 0x0BADF00Du) == 0);
  assert(fifa96_competition_start(&twin, FIFA96_COMPETITION_LEAGUE, 2u,
                                  test_pool, 8u, 0x0BADF00Du) == 0);

  for (round = 0u; round < FIFA96_COMPETITION_ROUNDS; round++) {
    uint8_t fixture_index = 0u;
    uint8_t home = 0u;
    uint8_t away = 0u;
    assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                           &away) == 1);
    assert(fixture_index == (uint8_t)(round * 4u));
    assert(home == 2u);
    assert(fifa96_competition_simulate_next(&comp) == 1);
    assert(played_count(&comp) == (unsigned)((round + 1u) * 4u));
    if (round < FIFA96_COMPETITION_ROUNDS - 1u)
      assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  }

  assert(comp.state == FIFA96_COMPETITION_COMPLETE);
  assert(played_count(&comp) == 28u);
  assert(comp.round == FIFA96_COMPETITION_ROUNDS);
  assert(comp.current == 28u);
  assert(comp.champion != FIFA96_COMPETITION_NO_TEAM);
  assert(fifa96_competition_standings(&comp, order) == 8);
  assert(comp.champion == order[0]);
  compute_stats(&comp, stats);
  assert(fifa96_competition_user_points(&comp) == stats[2].points);

  assert(fifa96_competition_simulate_next(&comp) == 0);
  assert(fifa96_competition_simulate_all(&comp) == 0);
  assert(fifa96_competition_simulate_all(&twin) == 0);
  assert(memcmp(comp.fixtures, twin.fixtures, sizeof(comp.fixtures)) == 0);
  assert(comp.champion == twin.champion);
  assert(comp.round == twin.round);
  assert(comp.current == twin.current);

  assert(fifa96_competition_simulate_next(NULL) == -1);
  assert(fifa96_competition_simulate_all(NULL) == -1);
}

static void test_practise(void) {
  struct fifa96_competition comp;
  uint8_t fixture_index = 77u;
  uint8_t home = 77u;
  uint8_t away = 77u;
  uint8_t order[FIFA96_COMPETITION_TEAMS];
  char summary[128];

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_PRACTISE, 0u,
                                  test_pool, 8u, 0x5EEDu) == 0);
  assert(comp.kind == FIFA96_COMPETITION_PRACTISE);
  assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  assert(comp.team_count == 1u);
  assert(comp.teams[0] == test_pool[0]);
  assert(comp.round == 0u);
  assert(comp.rounds == 0u);
  assert(comp.fixture_count == 0u);
  assert(comp.current == 0u);
  assert(comp.champion == FIFA96_COMPETITION_NO_TEAM);
  assert(comp.eliminated == 0u);
  assert(fifa96_competition_fixture(&comp, 0u) == NULL);
  assert(fifa96_competition_standings(&comp, order) == 0);
  assert(fifa96_competition_user_points(&comp) == 0);
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 0);
  assert(fifa96_competition_simulate_next(&comp) == 0);
  assert(fifa96_competition_simulate_all(&comp) == 0);
  assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  assert(fifa96_competition_apply_result(&comp, 0u, 0u, 0u) == -1);
  assert(fifa96_competition_summary(&comp, summary, sizeof(summary)) == 0);
  assert(strstr(summary, "Practise") != NULL);
  assert(!has_newline(summary));
}

static void test_tournament_bracket(void) {
  struct fifa96_competition comp;
  uint8_t seen[FIFA96_COMPETITION_TEAMS];
  uint8_t fixture_index = 0u;
  uint8_t home = 0u;
  uint8_t away = 0u;
  uint8_t i;
  uint8_t order[FIFA96_COMPETITION_TEAMS];

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_TOURNAMENT, 2u,
                                  test_pool, 8u, 0x51EDu) == 0);
  assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  assert(comp.round == 0u);
  assert(comp.rounds == 3u);
  assert(comp.fixture_count == 4u);
  assert(comp.current == 0u);

  memset(seen, 0, sizeof(seen));
  for (i = 0u; i < 4u; i++) {
    const struct fifa96_competition_fixture *f =
        fifa96_competition_fixture(&comp, i);
    assert(f != NULL);
    assert(f->round == 0u);
    assert(f->played == 0u);
    assert(f->home != f->away);
    seen[f->home]++;
    seen[f->away]++;
  }
  for (i = 0u; i < FIFA96_COMPETITION_TEAMS; i++) assert(seen[i] == 1u);

  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 1);
  assert(fixture_index == 0u);
  assert(home == 2u);
  assert(away == comp.fixtures[0].away);
  assert(comp.fixtures[0].user == 1u);

  assert(fifa96_competition_apply_result(&comp, 0u, 2u, 0u) == 0);
  assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  assert(comp.round == 1u);
  assert(comp.fixture_count == 6u);
  assert(comp.current == 4u);
  for (i = 0u; i < 4u; i++) {
    assert(comp.fixtures[i].played == 1u);
    assert(comp.fixtures[i].winner != FIFA96_COMPETITION_NO_TEAM);
  }
  assert(comp.fixtures[4].user == 1u);
  assert(comp.fixtures[4].home == 2u);
  assert(comp.fixtures[4].away == comp.fixtures[1].winner);
  assert(comp.fixtures[5].home == comp.fixtures[2].winner);
  assert(comp.fixtures[5].away == comp.fixtures[3].winner);
  assert(comp.fixtures[4].round == 1u);
  assert(comp.fixtures[5].round == 1u);

  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 1);
  assert(fixture_index == 4u);
  assert(home == 2u);
  assert(fifa96_competition_apply_result(&comp, 4u, 3u, 1u) == 0);
  assert(comp.state == FIFA96_COMPETITION_ACTIVE);
  assert(comp.round == 2u);
  assert(comp.fixture_count == 7u);
  assert(comp.current == 6u);
  assert(comp.fixtures[6].user == 1u);
  assert(comp.fixtures[6].home == 2u);
  assert(comp.fixtures[6].away == comp.fixtures[5].winner);
  assert(comp.fixtures[6].round == 2u);

  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 1);
  assert(fixture_index == 6u);
  assert(home == 2u);
  assert(fifa96_competition_apply_result(&comp, 6u, 1u, 0u) == 0);
  assert(comp.state == FIFA96_COMPETITION_COMPLETE);
  assert(comp.round == 3u);
  assert(comp.current == 7u);
  assert(comp.eliminated == 0u);
  assert(comp.champion == 2u);
  assert(comp.fixtures[6].winner == 2u);
  for (i = 0u; i < 7u; i++) {
    assert(comp.fixtures[i].played == 1u);
    assert(comp.fixtures[i].winner != FIFA96_COMPETITION_NO_TEAM);
  }
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 0);
  assert(fifa96_competition_simulate_next(&comp) == 0);
  assert(fifa96_competition_apply_result(&comp, 6u, 1u, 0u) == -1);
  assert(fifa96_competition_standings(&comp, order) == 0);
}

static void test_cup_user_loss(void) {
  struct fifa96_competition comp;
  uint8_t fixture_index = 0u;
  uint8_t home = 0u;
  uint8_t away = 0u;
  uint8_t i;

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_TOURNAMENT, 0u,
                                  test_pool, 8u, 0xC0FFEEu) == 0);
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 1);
  assert(fixture_index == 0u);
  assert(home == 0u);
  assert(away == comp.fixtures[0].away);
  assert(fifa96_competition_apply_result(&comp, 0u, 0u, 2u) == 0);
  assert(comp.eliminated == 1u);
  assert(comp.state == FIFA96_COMPETITION_COMPLETE);
  assert(comp.fixture_count == 7u);
  assert(comp.round == 3u);
  assert(comp.current == 7u);
  assert(comp.fixtures[0].winner == comp.fixtures[0].away);
  assert(comp.champion != FIFA96_COMPETITION_NO_TEAM);
  assert(comp.champion != 0u);
  assert(comp.champion == comp.fixtures[6].winner);
  assert(played_count(&comp) == 7u);
  for (i = 0u; i < 7u; i++)
    assert(comp.fixtures[i].winner != FIFA96_COMPETITION_NO_TEAM);
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 0);
  assert(fifa96_competition_simulate_next(&comp) == 0);
  assert(fifa96_competition_simulate_all(&comp) == 0);
}

static void test_cup_replay_shootout(void) {
  int saw_user_win = 0;
  int saw_user_loss = 0;
  uint32_t seed;

  for (seed = 0u; seed < 128u; seed++) {
    struct fifa96_competition comp;
    uint8_t attempt;
    assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_TOURNAMENT, 1u,
                                    test_pool, 8u, seed) == 0);
    for (attempt = 0u; attempt < 3u; attempt++) {
      uint8_t fixture_index = 0u;
      uint8_t home = 0u;
      uint8_t away = 0u;
      assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                             &away) == 1);
      assert(fixture_index == 0u);
      assert(home == 1u);
      assert(fifa96_competition_apply_result(&comp, 0u, 1u, 1u) == 0);
      assert(comp.fixtures[0].played == 1u);
      assert(comp.fixtures[0].home_goals == 1u);
      assert(comp.fixtures[0].away_goals == 1u);
      if (attempt < 2u) {
        assert(comp.fixtures[0].replay == (uint8_t)(attempt + 1u));
        assert(comp.fixtures[0].shootout == 0u);
        assert(comp.fixtures[0].winner == FIFA96_COMPETITION_NO_TEAM);
        assert(comp.state == FIFA96_COMPETITION_ACTIVE);
        assert(comp.round == 0u);
        assert(comp.fixture_count == 4u);
        assert(comp.current == 0u);
      } else {
        uint8_t winner = comp.fixtures[0].winner;
        assert(comp.fixtures[0].replay == 2u);
        assert(comp.fixtures[0].shootout == 1u);
        assert(winner == comp.fixtures[0].home ||
               winner == comp.fixtures[0].away);
        if (winner == 1u) {
          saw_user_win = 1;
          assert(comp.eliminated == 0u);
          assert(comp.state == FIFA96_COMPETITION_ACTIVE);
          assert(comp.round == 1u);
          assert(comp.fixture_count == 6u);
          assert(comp.current == 4u);
          assert(fifa96_competition_simulate_all(&comp) == 0);
          assert(comp.state == FIFA96_COMPETITION_COMPLETE);
          assert(comp.champion != FIFA96_COMPETITION_NO_TEAM);
        } else {
          saw_user_loss = 1;
          assert(comp.eliminated == 1u);
          assert(comp.state == FIFA96_COMPETITION_COMPLETE);
          assert(comp.fixture_count == 7u);
          assert(comp.current == 7u);
          assert(played_count(&comp) == 7u);
          assert(comp.champion != FIFA96_COMPETITION_NO_TEAM);
          assert(comp.champion != 1u);
        }
      }
    }
  }
  assert(saw_user_win);
  assert(saw_user_loss);
}

static void test_cup_simulated_ties(void) {
  struct fifa96_competition comp;
  uint8_t i;

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_PLAYOFF, 5u,
                                  test_pool, 8u, 0xABCDEF01u) == 0);
  assert(fifa96_competition_simulate_all(&comp) == 0);
  assert(comp.state == FIFA96_COMPETITION_COMPLETE);
  assert(comp.fixture_count == 7u);
  assert(comp.round == 3u);
  assert(comp.current == 7u);
  assert(played_count(&comp) == 7u);
  for (i = 0u; i < 7u; i++) {
    const struct fifa96_competition_fixture *f = &comp.fixtures[i];
    assert(f->played == 1u);
    assert(f->winner != FIFA96_COMPETITION_NO_TEAM);
    if (f->home_goals == f->away_goals)
      assert(f->shootout == 1u);
    else
      assert(f->shootout == 0u);
  }
  assert(comp.champion == comp.fixtures[6].winner);
  if (comp.champion == 5u)
    assert(comp.eliminated == 0u);
  else
    assert(comp.eliminated == 1u);
}

static void test_cup_simulate_next_decides(void) {
  int saw_tie = 0;
  uint32_t seed;

  for (seed = 0u; seed < 128u; seed++) {
    struct fifa96_competition comp;
    const struct fifa96_competition_fixture *f;
    uint8_t fixture_index = 0u;
    uint8_t home = 0u;
    uint8_t away = 0u;
    assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_TOURNAMENT, 0u,
                                    test_pool, 8u, seed) == 0);
    assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                           &away) == 1);
    assert(fifa96_competition_simulate_next(&comp) == 1);
    f = &comp.fixtures[0];
    assert(f->played == 1u);
    assert(f->replay == 0u);
    if (f->home_goals == f->away_goals) {
      saw_tie = 1;
      assert(f->shootout == 1u);
      assert(f->winner != FIFA96_COMPETITION_NO_TEAM);
    } else {
      assert(f->shootout == 0u);
      assert(f->winner == (f->home_goals > f->away_goals ? f->home : f->away));
    }
    if (f->winner == 0u) {
      assert(comp.eliminated == 0u);
      assert(comp.state == FIFA96_COMPETITION_ACTIVE);
      assert(comp.round == 1u);
      assert(comp.fixture_count == 6u);
      assert(comp.current == 4u);
    } else {
      assert(comp.eliminated == 1u);
      assert(comp.state == FIFA96_COMPETITION_COMPLETE);
      assert(comp.fixture_count == 7u);
      assert(comp.current == 7u);
      assert(played_count(&comp) == 7u);
    }
  }
  assert(saw_tie);
}

static void test_serialization(void) {
  struct fifa96_competition comp;
  struct fifa96_competition restored;
  uint8_t buf[FIFA96_COMPETITION_SERIAL_MAX];
  uint8_t buf2[FIFA96_COMPETITION_SERIAL_MAX];
  size_t len = 0u;
  size_t len2 = 0u;
  uint8_t round;

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 4u,
                                  test_pool, 8u, 0xDEADBEEFu) == 0);
  for (round = 0u; round < 3u; round++) {
    uint8_t fixture_index = 0u;
    uint8_t home = 0u;
    uint8_t away = 0u;
    assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                           &away) == 1);
    assert(fifa96_competition_apply_result(&comp, fixture_index, 1u, 0u) == 0);
  }

  assert(fifa96_competition_serialize(&comp, buf, sizeof(buf), &len) == 0);
  assert(len == (size_t)(29u + 28u * 10u));
  assert(len <= FIFA96_COMPETITION_SERIAL_MAX);
  assert(buf[0] == (uint8_t)'F');
  assert(buf[1] == (uint8_t)'9');
  assert(buf[2] == (uint8_t)'6');
  assert(buf[3] == (uint8_t)'C');
  assert(buf[4] == 1u);
  assert(buf[5] == (uint8_t)(len & 0xFFu));
  assert(buf[6] == (uint8_t)((len >> 8) & 0xFFu));
  assert(buf[17] == 0xEFu);
  assert(buf[18] == 0xBEu);
  assert(buf[19] == 0xADu);
  assert(buf[20] == 0xDEu);

  memset(&restored, 0, sizeof(restored));
  assert(fifa96_competition_deserialize(&restored, buf, len) == 0);
  compare_competitions(&comp, &restored);
  assert(restored.seed == 0xDEADBEEFu);
  assert(restored.kind == FIFA96_COMPETITION_LEAGUE);
  assert(restored.fixture_count == 28u);
  assert(restored.fixtures[0].user == 1u);
  assert(restored.fixtures[0].home == 4u);
  assert(restored.round == 3u);
  assert(restored.current == 12u);

  assert(fifa96_competition_serialize(&restored, buf2, sizeof(buf2), &len2) ==
         0);
  assert(len2 == len);
  assert(memcmp(buf, buf2, len) == 0);

  /* An init record serializes too. */
  {
    struct fifa96_competition empty;
    struct fifa96_competition loaded;
    fifa96_competition_init(&empty);
    len = 0u;
    assert(fifa96_competition_serialize(&empty, buf, sizeof(buf), &len) == 0);
    assert(len == 29u);
    memset(&loaded, 0, sizeof(loaded));
    assert(fifa96_competition_deserialize(&loaded, buf, len) == 0);
    assert(loaded.kind == FIFA96_COMPETITION_NONE);
    assert(loaded.state == FIFA96_COMPETITION_IDLE);
    assert(loaded.champion == FIFA96_COMPETITION_NO_TEAM);
    assert(loaded.fixture_count == 0u);
  }

  /* A pending cup replay round-trips and continues identically. */
  {
    struct fifa96_competition cup;
    struct fifa96_competition copy;
    uint8_t fixture_index = 0u;
    uint8_t home = 0u;
    uint8_t away = 0u;
    assert(fifa96_competition_start(&cup, FIFA96_COMPETITION_TOURNAMENT, 3u,
                                    test_pool, 8u, 0x55AA55AAu) == 0);
    assert(fifa96_competition_next_fixture(&cup, &fixture_index, &home,
                                           &away) == 1);
    assert(fifa96_competition_apply_result(&cup, 0u, 2u, 2u) == 0);
    assert(cup.fixtures[0].replay == 1u);
    assert(cup.fixtures[0].winner == FIFA96_COMPETITION_NO_TEAM);
    len = 0u;
    assert(fifa96_competition_serialize(&cup, buf, sizeof(buf), &len) == 0);
    assert(len == (size_t)(29u + 4u * 10u));
    memset(&copy, 0, sizeof(copy));
    assert(fifa96_competition_deserialize(&copy, buf, len) == 0);
    compare_competitions(&cup, &copy);
    assert(copy.fixtures[0].replay == 1u);
    assert(copy.fixtures[0].played == 1u);
    assert(copy.fixtures[0].winner == FIFA96_COMPETITION_NO_TEAM);
    assert(fifa96_competition_apply_result(&cup, 0u, 3u, 1u) == 0);
    assert(fifa96_competition_apply_result(&copy, 0u, 3u, 1u) == 0);
    compare_competitions(&cup, &copy);
  }

  /* A completed cup carries shootout/eliminated/champion fields. */
  {
    struct fifa96_competition cup;
    struct fifa96_competition copy;
    assert(fifa96_competition_start(&cup, FIFA96_COMPETITION_PLAYOFF, 5u,
                                    test_pool, 8u, 0xABCDEF01u) == 0);
    assert(fifa96_competition_simulate_all(&cup) == 0);
    assert(cup.state == FIFA96_COMPETITION_COMPLETE);
    assert(cup.fixture_count == 7u);
    len = 0u;
    assert(fifa96_competition_serialize(&cup, buf, sizeof(buf), &len) == 0);
    assert(len == (size_t)(29u + 7u * 10u));
    memset(&copy, 0, sizeof(copy));
    assert(fifa96_competition_deserialize(&copy, buf, len) == 0);
    compare_competitions(&cup, &copy);
    assert(copy.round == 3u);
    assert(copy.current == 7u);
    assert(copy.champion == cup.champion);
    assert(copy.champion != FIFA96_COMPETITION_NO_TEAM);
  }

  /* Practise round-trips with no fixtures. */
  {
    struct fifa96_competition practice;
    struct fifa96_competition copy;
    assert(fifa96_competition_start(&practice, FIFA96_COMPETITION_PRACTISE,
                                    0u, test_pool, 8u, 0x5EEDu) == 0);
    len = 0u;
    assert(fifa96_competition_serialize(&practice, buf, sizeof(buf), &len) ==
           0);
    assert(len == 29u);
    memset(&copy, 0, sizeof(copy));
    assert(fifa96_competition_deserialize(&copy, buf, len) == 0);
    compare_competitions(&practice, &copy);
    assert(copy.kind == FIFA96_COMPETITION_PRACTISE);
    assert(copy.team_count == 1u);
    assert(copy.fixture_count == 0u);
  }

  /* Malformed / truncated buffers. */
  len = 0u;
  assert(fifa96_competition_serialize(&comp, buf, sizeof(buf), &len) == 0);
  assert(len > 0u);
  {
    struct fifa96_competition target;
    struct fifa96_competition before;
    uint8_t bad[FIFA96_COMPETITION_SERIAL_MAX];

    memset(&target, 0, sizeof(target));
    assert(fifa96_competition_deserialize(NULL, buf, len) == -1);
    assert(fifa96_competition_deserialize(&target, NULL, len) == -1);
    assert(fifa96_competition_deserialize(&target, buf, 0u) == -2);
    assert(fifa96_competition_deserialize(&target, buf, 3u) == -2);
    assert(fifa96_competition_deserialize(&target, buf, 6u) == -2);
    assert(fifa96_competition_deserialize(&target, buf, len - 1u) == -2);
    {
      /* A crafted header that declares its own tiny length must not make the
       * deserializer read past the header fields. */
      uint8_t tiny_record[7] = {(uint8_t)'F', (uint8_t)'9', (uint8_t)'6',
                                (uint8_t)'C', 1u, 7u, 0u};
      uint8_t short_record[28];
      assert(fifa96_competition_deserialize(&target, tiny_record,
                                            sizeof(tiny_record)) == -2);
      memset(short_record, 0, sizeof(short_record));
      memcpy(short_record, tiny_record, 4u);
      short_record[4] = 1u;
      short_record[5] = 28u;
      short_record[6] = 0u;
      assert(fifa96_competition_deserialize(&target, short_record,
                                            sizeof(short_record)) == -2);
    }

    memcpy(bad, buf, len);
    bad[0] = (uint8_t)'X';
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[4] = 2u;
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[5] ^= 0xFFu;
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[7] = 9u;
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[8] = 9u;
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[13] = 33u;
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[15] = 8u;
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[29] = 0xFFu;
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memcpy(bad, buf, len);
    bad[31] = bad[30];
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);

    memset(&target, 0xA5, sizeof(target));
    memcpy(&before, &target, sizeof(target));
    memcpy(bad, buf, len);
    bad[0] = (uint8_t)'X';
    assert(fifa96_competition_deserialize(&target, bad, len) == -2);
    assert(memcmp(&target, &before, sizeof(target)) == 0);
  }

  assert(fifa96_competition_serialize(NULL, buf, sizeof(buf), &len) == -1);
  assert(fifa96_competition_serialize(&comp, NULL, sizeof(buf), &len) == -1);
  assert(fifa96_competition_serialize(&comp, buf, sizeof(buf), NULL) == -1);
  len = 0u;
  assert(fifa96_competition_serialize(&comp, buf, 1u, &len) == -2);
  assert(len == 0u);
  len = 0u;
  {
    struct fifa96_competition invalid;
    fifa96_competition_init(&invalid);
    invalid.kind = FIFA96_COMPETITION_LEAGUE;
    assert(fifa96_competition_serialize(&invalid, buf, sizeof(buf), &len) ==
           -1);
  }

  /* Fixture fields survive: spot-check each serialized byte position. */
  assert(fifa96_competition_serialize(&comp, buf, sizeof(buf), &len) == 0);
  assert(buf[29] == comp.fixtures[0].round);
  assert(buf[30] == comp.fixtures[0].home);
  assert(buf[31] == comp.fixtures[0].away);
  assert(buf[32] == comp.fixtures[0].played);
  assert(buf[33] == comp.fixtures[0].home_goals);
  assert(buf[34] == comp.fixtures[0].away_goals);
  assert(buf[35] == comp.fixtures[0].replay);
  assert(buf[36] == comp.fixtures[0].shootout);
  assert(buf[37] == comp.fixtures[0].winner);
  assert(buf[38] == comp.fixtures[0].user);
}

static void test_summary(void) {
  struct fifa96_competition comp;
  char summary[128];
  char tiny[5];

  fifa96_competition_init(&comp);
  assert(fifa96_competition_summary(&comp, summary, sizeof(summary)) == 0);
  assert(strcmp(summary, "None") == 0);
  assert(!has_newline(summary));
  assert(fifa96_competition_summary(NULL, summary, sizeof(summary)) == -1);
  assert(fifa96_competition_summary(&comp, NULL, sizeof(summary)) == -1);
  assert(fifa96_competition_summary(&comp, summary, 0u) == -1);
  assert(fifa96_competition_summary(&comp, tiny, sizeof(tiny)) == 0);
  assert(strcmp(tiny, "None") == 0);
  assert(fifa96_competition_summary(&comp, tiny, 4u) == -1);

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 6u,
                                  test_pool, 8u, 1u) == 0);
  assert(fifa96_competition_summary(&comp, summary, sizeof(summary)) == 0);
  assert(strstr(summary, "League") != NULL);
  assert(strstr(summary, "round") != NULL);
  assert(!has_newline(summary));
  assert(fifa96_competition_simulate_all(&comp) == 0);
  assert(fifa96_competition_summary(&comp, summary, sizeof(summary)) == 0);
  assert(strstr(summary, "League") != NULL);
  assert(strstr(summary, "champion") != NULL);
  assert(!has_newline(summary));

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_PRACTISE, 0u,
                                  test_pool, 8u, 1u) == 0);
  assert(fifa96_competition_summary(&comp, summary, sizeof(summary)) == 0);
  assert(strstr(summary, "Practise") != NULL);
  assert(!has_newline(summary));

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_TOURNAMENT, 2u,
                                  test_pool, 8u, 0x51EDu) == 0);
  assert(fifa96_competition_summary(&comp, summary, sizeof(summary)) == 0);
  assert(strstr(summary, "Tournament") != NULL);
  assert(!has_newline(summary));
  assert(fifa96_competition_apply_result(&comp, 0u, 2u, 0u) == 0);
  assert(fifa96_competition_apply_result(&comp, 4u, 3u, 1u) == 0);
  assert(fifa96_competition_apply_result(&comp, 6u, 1u, 0u) == 0);
  assert(fifa96_competition_summary(&comp, summary, sizeof(summary)) == 0);
  assert(strstr(summary, "Tournament") != NULL);
  assert(strstr(summary, "champion") != NULL);
  assert(!has_newline(summary));

  assert(strcmp(fifa96_competition_kind_name(FIFA96_COMPETITION_NONE),
                "None") == 0);
  assert(strcmp(fifa96_competition_kind_name(FIFA96_COMPETITION_LEAGUE),
                "League") == 0);
  assert(strcmp(fifa96_competition_kind_name(FIFA96_COMPETITION_TOURNAMENT),
                "Tournament") == 0);
  assert(strcmp(fifa96_competition_kind_name(FIFA96_COMPETITION_PLAYOFF),
                "Playoff") == 0);
  assert(strcmp(fifa96_competition_kind_name(FIFA96_COMPETITION_PRACTISE),
                "Practise") == 0);
  assert(strcmp(fifa96_competition_kind_name(200u), "Unknown") == 0);
}

static void test_helpers(void) {
  struct fifa96_competition comp;
  struct fifa96_competition empty;
  uint8_t order[FIFA96_COMPETITION_TEAMS];
  uint8_t fixture_index = 0u;
  uint8_t home = 0u;
  uint8_t away = 0u;

  fifa96_competition_init(&empty);
  assert(fifa96_competition_fixture(&empty, 0u) == NULL);
  assert(fifa96_competition_fixture(NULL, 0u) == NULL);
  assert(fifa96_competition_user_points(&empty) == 0);
  assert(fifa96_competition_user_points(NULL) == 0);
  assert(fifa96_competition_standings(&empty, order) == 0);
  assert(fifa96_competition_standings(NULL, order) == 0);
  assert(fifa96_competition_standings(&empty, NULL) == 0);
  assert(fifa96_competition_simulate_next(&empty) == 0);
  assert(fifa96_competition_simulate_all(&empty) == -1);

  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_TOURNAMENT, 0u,
                                  test_pool, 8u, 1u) == 0);
  assert(fifa96_competition_standings(&comp, order) == 0);
  assert(fifa96_competition_user_points(&comp) == 0);
  assert(fifa96_competition_fixture(&comp, 0u) != NULL);
  assert(fifa96_competition_fixture(&comp, 4u) == NULL);
  assert(fifa96_competition_next_fixture(&comp, &fixture_index, &home,
                                         &away) == 1);
  assert(fixture_index == 0u);
  assert(home == 0u);
}

int main(void) {
  test_start_validation();
  test_league_round_robin();
  test_standings_team_id_tiebreak();
  test_league_simulate();
  test_practise();
  test_tournament_bracket();
  test_cup_user_loss();
  test_cup_replay_shootout();
  test_cup_simulated_ties();
  test_cup_simulate_next_decides();
  test_serialization();
  test_summary();
  test_helpers();
  printf("test_engine_competition: all assertions passed\n");
  return 0;
}
