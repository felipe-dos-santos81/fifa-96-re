/* port/tests/test_engine_frontend_modes.c — P13 derived single-player flows.
 *
 * Synthetic, no-ISO coverage for every newly live Game Select entry: Leagues,
 * Tournaments, Playoffs, Practise, Load Game and the Modem offline notice,
 * plus the competition hub, save/load round-trip, cup replay/shootout path and
 * the zero-default reset contract. The competition/save modules are the
 * derived P13 implementations; no native art or SGAME body is claimed. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fifa96_engine/fifa96_competition.h"
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_menu_art.h"
#include "fifa96_engine/fifa96_menu_model.h"
#include "fifa96_engine/fifa96_save.h"
#include "fifa96_engine/fifa96_surface.h"

static void press(struct fifa96_frontend_run *fr, int32_t code) {
  fifa96_platform_key k[2] = {
      {.raw_code = code, .state = 1},
      {.raw_code = code, .state = 0},
  };
  assert(fifa96_frontend_run_input(fr, k, 2) == 0);
}

static void idle(struct fifa96_frontend_run *fr) {
  assert(fifa96_frontend_run_input(fr, NULL, 0) == 0);
}

static void step(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  int quit = 0;
  assert(fifa96_frontend_run_step(fr, s, &quit) == 0);
  assert(quit == 0);
}

static void run_start(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  assert(fifa96_frontend_run_init(fr, s) == 0);
  fifa96_frontend_run_enable_native(fr, 1);
  idle(fr);
  step(fr, s);
  assert(fr->menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
}

/* Walk the real keyboard model from the current Game Select item to `target`
 * (the two-column widget order). */
static void game_select_item(struct fifa96_frontend_run *fr,
                             struct fifa96_surface *s, int target) {
  const struct fifa96_menu_item_desc *items = fifa96_menu_model_items();
  assert(fr->menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  for (int guard = 0; guard < 32 && fr->menu.item != target; guard++) {
    int current = fr->menu.item;
    if (items[current].col != items[target].col) press(fr, FIFA96_ENGINE_KEY_RIGHT);
    else press(fr, FIFA96_ENGINE_KEY_DOWN);
    step(fr, s);
  }
  assert(fr->menu.item == target);
}

static void open_team_select(struct fifa96_frontend_run *fr,
                             struct fifa96_surface *s, int item) {
  game_select_item(fr, s, item);
  press(fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(fr, s);
  assert(fr->menu.screen == FIFA96_MENU_SCREEN_TEAM_SELECT);
}

static char *temp_dir(char *buf, size_t cap) {
  if (snprintf(buf, cap, "%s", "/tmp/fifa96_modes_XXXXXX") >= (int)cap) return NULL;
  return mkdtemp(buf);
}

static void cleanup_dir(const char *dir) {
  char path[FIFA96_SAVE_PATH_MAX];
  for (int slot = 0; slot < FIFA96_SAVE_SLOT_COUNT; slot++) {
    if (fifa96_save_path(path, sizeof path, dir, slot) == 0) (void)remove(path);
  }
  (void)rmdir(dir);
}

static void test_public_values_and_items(void) {
  /* The historical enum values must not move. */
  assert(FIFA96_MENU_SCREEN_LIBRARY == 0);
  assert(FIFA96_MENU_SCREEN_GAME_SELECT == 1);
  assert(FIFA96_MENU_SCREEN_TEAM_SELECT == 2);
  assert(FIFA96_MENU_SCREEN_OPTIONS == 3);
  assert(FIFA96_MENU_SCREEN_COMPETITION_HUB == 4);
  assert(FIFA96_MENU_SCREEN_LOAD_GAME == 5);
  assert(FIFA96_MENU_SCREEN_SAVE_GAME == 6);
  assert(FIFA96_MENU_SCREEN_MODEM == 7);
  assert(FIFA96_MENU_SCREEN_LIST == 8);
  assert(FIFA96_MENU_ACTION_LEAGUE == 5);
  assert(FIFA96_MENU_ACTION_TOURNAMENT == 6);
  assert(FIFA96_MENU_ACTION_PLAYOFF == 7);
  assert(FIFA96_MENU_ACTION_PRACTISE == 8);
  assert(FIFA96_MENU_ACTION_LOAD_GAME == 9);
  assert(FIFA96_MENU_ACTION_HUB_PLAY == 10);
  assert(FIFA96_MENU_ACTION_HUB_SIMULATE == 11);
  assert(FIFA96_MENU_ACTION_HUB_TABLE == 12);
  assert(FIFA96_MENU_ACTION_HUB_SAVE == 13);
  assert(FIFA96_MENU_ACTION_HUB_EXIT == 14);
  assert(FIFA96_MENU_ACTION_SAVE_SLOT == 15);
  assert(FIFA96_MENU_ACTION_LOAD_SLOT == 16);
  assert(FIFA96_MENU_ACTION_MODEM_BACK == 17);

  const struct fifa96_menu_item_desc *items = fifa96_menu_model_items();
  static const enum fifa96_menu_action expected[FIFA96_MENU_ITEM_COUNT] = {
      FIFA96_MENU_ACTION_FRIENDLY, FIFA96_MENU_ACTION_LEAGUE,
      FIFA96_MENU_ACTION_TOURNAMENT, FIFA96_MENU_ACTION_PLAYOFF,
      FIFA96_MENU_ACTION_PRACTISE, FIFA96_MENU_ACTION_LOAD_GAME,
      FIFA96_MENU_ACTION_OPTIONS, FIFA96_MENU_ACTION_MODEM_BACK,
      FIFA96_MENU_ACTION_QUIT};
  for (int i = 0; i < FIFA96_MENU_ITEM_COUNT; i++) {
    assert(items[i].action == expected[i]);
    assert(items[i].action != FIFA96_MENU_ACTION_NONE);
  }

  /* Model-level screen transitions for every new entry. */
  struct fifa96_menu_model m;
  fifa96_menu_model_init(&m);
  static const int open_items[6] = {1, 2, 3, 4, 5, 7};
  static const enum fifa96_menu_screen screens[6] = {
      FIFA96_MENU_SCREEN_TEAM_SELECT, FIFA96_MENU_SCREEN_TEAM_SELECT,
      FIFA96_MENU_SCREEN_TEAM_SELECT, FIFA96_MENU_SCREEN_TEAM_SELECT,
      FIFA96_MENU_SCREEN_LOAD_GAME, FIFA96_MENU_SCREEN_MODEM};
  static const uint8_t kinds[6] = {
      FIFA96_COMPETITION_LEAGUE, FIFA96_COMPETITION_TOURNAMENT,
      FIFA96_COMPETITION_PLAYOFF, FIFA96_COMPETITION_PRACTISE, 0, 0};
  for (int i = 0; i < 6; i++) {
    fifa96_menu_model_init(&m);
    m.item = open_items[i];
    (void)fifa96_menu_model_activate(&m);
    assert(m.screen == screens[i]);
    if (screens[i] == FIFA96_MENU_SCREEN_TEAM_SELECT)
      assert(m.pending_kind == kinds[i]);
    assert(fifa96_menu_model_back(&m) == 1);
    assert(m.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  }

  assert(fifa96_menu_model_activate(NULL) == FIFA96_MENU_ACTION_NONE);
  assert(fifa96_menu_model_back(NULL) == 0);
  assert(fifa96_frontend_run_match_result(NULL, 0, 0) == -1);
}

static void test_game_select_state(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  run_start(&fr, s);
  struct fifa96_menu_state st;
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(st.item_count == FIFA96_MENU_ITEM_COUNT);
  assert(strcmp(st.title, "Game Select") == 0);
  for (int i = 0; i < FIFA96_MENU_ITEM_COUNT; i++) {
    assert(st.items[i].label != NULL && st.items[i].label[0] != '\0');
    assert(st.items[i].enabled == 1);
  }
  /* None of the derived list state leaks into Game Select. */
  assert(st.list_title == NULL && st.list_row_count == 0);
  fifa96_surface_destroy(s);
}

static void test_league_hub_and_save_load(void) {
  char dir[64];
  if (temp_dir(dir, sizeof dir) == NULL) {
    puts("SKIP league save/load (mkdtemp failed)");
    return;
  }
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  run_start(&fr, s);
  fifa96_frontend_run_set_save_dir(&fr, dir);
  fr.menu.settings.value[0x0E] = 1;   /* 4 min */

  open_team_select(&fr, s, 1);   /* Leagues */
  struct fifa96_menu_state st;
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.team_locked == 1);
  assert(st.column_label[1] == NULL);
  /* Home only: Right must not move the locked visitor column. */
  press(&fr, FIFA96_ENGINE_KEY_RIGHT);
  step(&fr, s);
  assert(fr.menu.team_col == 0);
  press(&fr, FIFA96_ENGINE_KEY_DOWN);
  step(&fr, s);
  int selected = fr.menu.team[0];
  assert(selected == 1);

  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.match_start == 1);
  assert(fr.mode == FIFA96_COMPETITION_LEAGUE);
  assert(fr.competition.kind == FIFA96_COMPETITION_LEAGUE);
  assert(fr.competition.state == FIFA96_COMPETITION_ACTIVE);
  assert(fr.competition.user_team < fr.competition.team_count);
  assert(fr.competition.teams[fr.competition.user_team] == selected);
  int fixture_index = fr.competition.current;
  const struct fifa96_competition_fixture *fixture =
      fifa96_competition_fixture(&fr.competition, (uint8_t)fixture_index);
  assert(fixture != NULL && fixture->user == 1);
  assert(fr.team_selected == 1);
  assert(fr.team_id[0] ==
         fifa96_menu_team_id(fr.competition.teams[fixture->home]));
  assert(fr.team_id[1] ==
         fifa96_menu_team_id(fr.competition.teams[fixture->away]));

  assert(fifa96_frontend_run_match_result(&fr, 2, 1) == 0);
  assert(fr.match_start == 0);
  assert(fr.team_selected == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  fixture = fifa96_competition_fixture(&fr.competition, (uint8_t)fixture_index);
  assert(fixture != NULL && fixture->played == 1);
  assert(fixture->home_goals == 2 && fixture->away_goals == 1);

  struct fifa96_save_info info;
  assert(fifa96_save_inspect(dir, 0, &info) == 0 && info.valid == 1);

  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  assert(st.list_row_count == 5);
  assert(st.list_rows[0].label != NULL);
  assert(st.list_rows[0].enabled == 1);
  assert(st.list_rows[2].enabled == 1);
  assert(st.list_help != NULL && st.list_help[0] != '\0');

  /* Hub -> Save game (row 3) -> slot 1. */
  for (int i = 0; i < 3; i++) {
    press(&fr, FIFA96_ENGINE_KEY_DOWN);
    step(&fr, s);
  }
  assert(fr.menu.list_row == 3);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_SAVE_GAME);
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.list_row_count == FIFA96_SAVE_SLOT_COUNT);
  assert(st.list_rows[0].label != NULL);
  assert(strstr(st.list_rows[0].label, "Empty") == NULL);
  press(&fr, FIFA96_ENGINE_KEY_DOWN);
  step(&fr, s);
  assert(fr.menu.list_row == 1);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  assert(fifa96_save_inspect(dir, 1, &info) == 0 && info.valid == 1);

  /* Hub -> Game Select -> Load Game -> slot 1 -> restored hub. */
  press(&fr, FIFA96_ENGINE_KEY_DECLINE);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  game_select_item(&fr, s, 5);   /* Load Game */
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_LOAD_GAME);
  press(&fr, FIFA96_ENGINE_KEY_DOWN);
  step(&fr, s);
  assert(fr.menu.list_row == 1);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  assert(fr.match_start == 0);
  assert(fr.competition.kind == FIFA96_COMPETITION_LEAGUE);
  assert(fr.competition.state == FIFA96_COMPETITION_ACTIVE);
  assert(fr.menu.settings.value[0x0E] == 1);

  /* Hub Play next uses the loaded current fixture and starts the handoff. */
  assert(fr.menu.list_row == 0);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.match_start == 1);
  assert(fr.mode == FIFA96_COMPETITION_LEAGUE);
  assert(fr.team_selected == 1);
  assert(fifa96_frontend_run_match_result(&fr, 0, 0) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);

  /* Hub Simulate next advances the league and auto-saves slot 0. */
  uint8_t round_before = fr.competition.round;
  press(&fr, FIFA96_ENGINE_KEY_DOWN);
  step(&fr, s);
  assert(fr.menu.list_row == 1);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  assert(fr.competition.round != round_before ||
         fr.competition.state != FIFA96_COMPETITION_ACTIVE);
  assert(fifa96_save_inspect(dir, 0, &info) == 0 && info.valid == 1);

  cleanup_dir(dir);
  fifa96_surface_destroy(s);
}

static void test_cup_replay_and_bracket(int item, uint8_t kind) {
  char dir[64];
  if (temp_dir(dir, sizeof dir) == NULL) {
    puts("SKIP cup save/load (mkdtemp failed)");
    return;
  }
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  run_start(&fr, s);
  fifa96_frontend_run_set_save_dir(&fr, dir);

  open_team_select(&fr, s, item);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.match_start == 1);
  assert(fr.mode == kind);
  assert(fr.competition.kind == kind);
  assert(fr.competition.state == FIFA96_COMPETITION_ACTIVE);
  int fixture_index = fr.competition.current;
  uint8_t round = fr.competition.round;
  assert(fifa96_competition_fixture(&fr.competition, (uint8_t)fixture_index) != NULL);

  /* First draw: the cup keeps the same fixture for the replay. */
  assert(fifa96_frontend_run_match_result(&fr, 1, 1) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  int advanced = fr.competition.current != fixture_index ||
                 fr.competition.round != round ||
                 fr.competition.state != FIFA96_COMPETITION_ACTIVE;
  const struct fifa96_competition_fixture *replayed =
      fifa96_competition_fixture(&fr.competition, (uint8_t)fixture_index);
  assert(replayed != NULL);
  assert(replayed->replay >= 1 || advanced);

  /* Keep drawing until the derived shootout advances or eliminates the user. */
  for (int attempt = 0; attempt < 5 && !advanced; attempt++) {
    uint8_t before_current = fr.competition.current;
    uint8_t before_round = fr.competition.round;
    assert(fifa96_frontend_run_match_result(&fr, 1, 1) == 0);
    advanced = fr.competition.current != before_current ||
               fr.competition.round != before_round ||
               fr.competition.state != FIFA96_COMPETITION_ACTIVE;
  }
  assert(advanced);
  int shootout = 0;
  for (uint8_t i = 0; i < fr.competition.fixture_count; i++) {
    const struct fifa96_competition_fixture *f =
        fifa96_competition_fixture(&fr.competition, i);
    if (f != NULL && f->shootout != 0) shootout = 1;
  }
  assert(shootout || fr.competition.eliminated != 0);

  /* Table/Bracket is a read-only list screen and backs to the hub. */
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  fr.menu.list_row = 2;
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_LIST);
  struct fifa96_menu_state st;
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.screen == FIFA96_MENU_SCREEN_LIST);
  assert(st.list_row_count >= 1);
  assert(st.list_rows[0].label != NULL && st.list_rows[0].label[0] != '\0');
  press(&fr, FIFA96_ENGINE_KEY_DECLINE);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);

  cleanup_dir(dir);
  fifa96_surface_destroy(s);
}

static void test_practise(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  run_start(&fr, s);
  open_team_select(&fr, s, 4);   /* Practise */
  press(&fr, FIFA96_ENGINE_KEY_DOWN);
  step(&fr, s);
  int selected = fr.menu.team[0];
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.match_start == 1);
  assert(fr.mode == FIFA96_COMPETITION_PRACTISE);
  assert(fr.competition.kind == FIFA96_COMPETITION_PRACTISE);
  assert(fr.team_selected == 1);
  assert(fr.team_id[0] == fifa96_menu_team_id(selected));
  assert(fr.team_id[1] == fifa96_menu_team_id(4));   /* fixed Germany */
  assert(strcmp(fr.team_name[1], fifa96_menu_team_fallback(4)) == 0);

  assert(fifa96_frontend_run_match_result(&fr, 0, 0) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fr.match_start == 0);
  assert(fr.competition.state == FIFA96_COMPETITION_IDLE);
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  fifa96_surface_destroy(s);
}

static void test_load_missing_and_valid(void) {
  char dir[64];
  if (temp_dir(dir, sizeof dir) == NULL) {
    puts("SKIP load slot case (mkdtemp failed)");
    return;
  }
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  run_start(&fr, s);
  fifa96_frontend_run_set_save_dir(&fr, dir);

  game_select_item(&fr, s, 5);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_LOAD_GAME);
  struct fifa96_menu_state st;
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.list_row_count == FIFA96_SAVE_SLOT_COUNT);
  assert(strcmp(st.list_rows[0].label, "Empty") == 0);
  assert(st.list_rows[0].enabled == 0);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_LOAD_GAME);
  assert(fr.match_start == 0);

  /* A valid slot restores the competition and settings into the hub. */
  struct fifa96_competition comp;
  uint8_t pool[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  assert(fifa96_competition_start(&comp, FIFA96_COMPETITION_LEAGUE, 0, pool, 8,
                                  0x96u) == 0);
  fr.menu.settings.value[0x0E] = 6;
  assert(fifa96_save_write(dir, 0, &comp, &fr.menu.settings) == 0);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  assert(fr.match_start == 0);
  assert(fr.competition.kind == FIFA96_COMPETITION_LEAGUE);
  assert(fr.competition.state == FIFA96_COMPETITION_ACTIVE);
  assert(fr.mode == FIFA96_COMPETITION_LEAGUE);
  assert(fr.menu.settings.value[0x0E] == 6);

  cleanup_dir(dir);
  fifa96_surface_destroy(s);
}

static void test_modem_notice(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  run_start(&fr, s);
  game_select_item(&fr, s, 7);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_MODEM);
  assert(fr.match_start == 0);

  struct fifa96_menu_state st;
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.screen == FIFA96_MENU_SCREEN_MODEM);
  assert(strcmp(st.title, "Modem Setup") == 0);
  assert(st.list_row_count == 2);
  assert(strcmp(st.list_rows[0].label,
                "Modem/network play is not available in this SDL port") == 0);
  assert(strcmp(st.list_rows[1].label, "Back") == 0);
  /* The notice row is not actionable. */
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_MODEM);
  assert(fr.match_start == 0);
  press(&fr, FIFA96_ENGINE_KEY_DOWN);
  step(&fr, s);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fr.match_start == 0);

  /* X also backs straight out. */
  game_select_item(&fr, s, 7);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_MODEM);
  press(&fr, FIFA96_ENGINE_KEY_DECLINE);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  fifa96_surface_destroy(s);
}

static void test_zero_default_reset_preserves(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  run_start(&fr, s);

  /* Zero/IDLE competition keeps the historical Game Select reset, settings
   * still survive. */
  assert(fr.competition.state == FIFA96_COMPETITION_IDLE);
  fr.menu.settings.value[0x0E] = 4;
  fr.menu.settings.value[0x0A] = 0;
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fr.menu.settings.value[0x0E] == 4);
  assert(fr.menu.settings.value[0x0A] == 0);

  /* An active league re-enters the hub and keeps competition/mode/save_dir. */
  uint8_t pool[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  assert(fifa96_competition_start(&fr.competition, FIFA96_COMPETITION_LEAGUE, 0,
                                  pool, 8, 0x96u) == 0);
  fr.mode = FIFA96_COMPETITION_LEAGUE;
  fifa96_frontend_run_set_save_dir(&fr, "/tmp/fifa96-reset-keep");
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  assert(fr.competition.kind == FIFA96_COMPETITION_LEAGUE);
  assert(fr.competition.state == FIFA96_COMPETITION_ACTIVE);
  assert(fr.mode == FIFA96_COMPETITION_LEAGUE);
  assert(strcmp(fr.save_dir, "/tmp/fifa96-reset-keep") == 0);
  assert(fr.match_start == 0 && fr.team_selected == 0);

  /* Complete also stays in the hub; IDLE returns to Game Select. */
  fr.competition.state = FIFA96_COMPETITION_COMPLETE;
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_COMPETITION_HUB);
  fr.competition.state = FIFA96_COMPETITION_IDLE;
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);

  /* A zeroed save_dir is repaired to the documented default. */
  fr.save_dir[0] = '\0';
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(strcmp(fr.save_dir, ".") == 0);
  fifa96_frontend_run_set_save_dir(&fr, NULL);
  assert(strcmp(fr.save_dir, ".") == 0);

  fifa96_surface_destroy(s);
}

int main(void) {
  test_public_values_and_items();
  test_game_select_state();
  test_league_hub_and_save_load();
  test_cup_replay_and_bracket(2, FIFA96_COMPETITION_TOURNAMENT);
  test_cup_replay_and_bracket(3, FIFA96_COMPETITION_PLAYOFF);
  test_practise();
  test_load_missing_and_valid();
  test_modem_notice();
  test_zero_default_reset_preserves();
  puts("test_engine_frontend_modes OK");
  return 0;
}
