#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_match_bridge.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"

static void test_mode_latch_and_final_score(void) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  struct fifa96_match_run *mr = &e->match_run;
  assert(fifa96_match_run_set_live_start(mr, 1) == 0);
  assert(fifa96_match_run_begin(mr, e, 0) == 0);
  assert(fifa96_match_run_set_mode(mr, FIFA96_COMPETITION_PRACTISE) == 0);
  assert(mr->match_mode == FIFA96_COMPETITION_PRACTISE);
  assert(mr->session_gate_14c32a == 1u);
  assert(fifa96_match_run_set_mode(mr, FIFA96_COMPETITION_LEAGUE) == 0);
  assert(mr->match_mode == FIFA96_COMPETITION_LEAGUE);
  assert(mr->session_gate_14c32a == 0u);
  assert(fifa96_match_run_set_mode(mr, 5) == -FIFA96_ERR_INVALID);

  mr->score[0] = 2;
  mr->score[1] = 1;
  mr->lc.screen = FIFA96_MATCH_SCREEN_POST;
  assert(fifa96_match_run_end(mr) == 0);
  assert(mr->final_score_valid == 1u);
  assert(mr->final_score[0] == 2u && mr->final_score[1] == 1u);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

static void test_frontend_result_advances_league(void) {
  char dir[] = "/tmp/f96modeXXXXXX";
  assert(mkdtemp(dir) != NULL);
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  fifa96_frontend_run_set_save_dir(&fr, dir);
  fr.mode = FIFA96_COMPETITION_LEAGUE;
  const uint8_t pool[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  assert(fifa96_competition_start(&fr.competition, FIFA96_COMPETITION_LEAGUE,
                                  0, pool, 8, 0xCAFEBABEu) == 0);
  uint8_t fix = 0, home = 0, away = 0;
  assert(fifa96_competition_next_fixture(&fr.competition, &fix, &home, &away) == 1);
  assert(fr.competition.teams[home] == 0u);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fifa96_frontend_run_match_result(&fr, 2, 1) == 0);
  assert(fr.competition.fixtures[fix].played == 1u);
  assert(fr.competition.fixtures[fix].home_goals == 2u);
  assert(fr.competition.fixtures[fix].away_goals == 1u);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  char path[FIFA96_SAVE_PATH_MAX];
  assert(fifa96_save_path(path, sizeof path, dir, 0) == 0);
  assert(access(path, F_OK) == 0);
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(fr.competition.kind == FIFA96_COMPETITION_LEAGUE);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  remove(path);
  fifa96_surface_destroy(s);
  rmdir(dir);
}

/* The engine glue: a completed competition match must apply its preserved
 * result and land on the hub, not Game Select, with the fixture recorded. */
static void test_engine_fulltime_applies_result(void) {
  char dir[] = "/tmp/f96glueXXXXXX";
  assert(mkdtemp(dir) != NULL);
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  fifa96_frontend_run_set_save_dir(&e->frontend, dir);
  e->frontend.mode = FIFA96_COMPETITION_LEAGUE;
  const uint8_t pool[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  assert(fifa96_competition_start(&e->frontend.competition,
                                  FIFA96_COMPETITION_LEAGUE, 0, pool, 8,
                                  0x1234u) == 0);
  uint8_t fix = 0, home = 0, away = 0;
  assert(fifa96_competition_next_fixture(&e->frontend.competition, &fix, &home,
                                         &away) == 1);
  e->frontend.match_start = 1;
  assert(fifa96_match_bridge_from_frontend(&e->match_run, e) == 0);
  assert(e->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(e->match_run.match_mode == FIFA96_COMPETITION_LEAGUE);
  assert(e->match_run.session_gate_14c32a == 0u);
  e->match_run.score[0] = 3;
  e->match_run.score[1] = 0;
  /* The live POST branch sets result_pending and requests EXIT in one step;
   * stage that already-requested state so the test remains a one-step glue
   * check rather than a two-minute clock run. */
  e->match_run.lc.screen = FIFA96_MATCH_SCREEN_EXIT;
  e->match_run.result_pending = 1;
  assert(fifa96_engine_step(e) == 0);
  assert(e->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(e->match == NULL);
  assert(e->frontend.competition.fixtures[fix].played == 1u);
  assert(e->frontend.competition.fixtures[fix].home_goals == 3u);
  assert(e->frontend.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
  char path[FIFA96_SAVE_PATH_MAX];
  assert(fifa96_save_path(path, sizeof path, dir, 0) == 0);
  remove(path);
  rmdir(dir);
}

int main(void) {
  test_mode_latch_and_final_score();
  test_frontend_result_advances_league();
  test_engine_fulltime_applies_result();
  puts("test_engine_mode_flow: ok");
  return 0;
}
