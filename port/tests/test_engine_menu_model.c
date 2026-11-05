/* port/tests/test_engine_menu_model.c — P2 native Game Select / Team Select.
 *
 * Unit cases pin the A3 §1.1 item table/navigation and the 44-name table; the
 * fixture drives the front-end wrapper through a full keyboard path
 * (Return -> Team Select -> Right/Up -> Return) and then runs the real bridge,
 * asserting the chosen 3-letter names and formation reach the match run. No
 * ISO is needed: the LANGO/FNTI fixtures are synthetic, so the model and the
 * handoff are exercised on every machine. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_match_bridge.h"
#include "fifa96_engine/fifa96_menu_model.h"
#include "fifa96_engine/fifa96_platform_null.h"

#define MODEL_TEST_COUNT 0x150u

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void put16le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static const char *fixture_string(uint32_t id) {
  switch (id) {
    case 0x023u: return "Brazil";
    case 0x081u: return "Friendly";
    case 0x084u: return "Game Select";
    case 0x098u: return "Home";
    case 0x0A6u: return "Italy";
    case 0x0D8u: return "Options";
    case 0x0F3u: return "Quit to DOS";
    case 0x13Du: return "Team Select";
    case 0x14Au: return "Visitor";
    default: return "";
  }
}

static size_t build_lango(uint8_t *buf, size_t cap) {
  const size_t base = 0x10 + MODEL_TEST_COUNT * 4u;
  size_t pos = base;
  uint8_t *offsets = calloc(MODEL_TEST_COUNT, 4);
  assert(offsets != NULL);
  for (uint32_t i = 0; i < MODEL_TEST_COUNT; i++) {
    const char *s = fixture_string(i);
    size_t len = strlen(s) + 1u;
    assert(pos + len <= cap);
    memcpy(buf + pos, s, len);
    put32le(offsets + (size_t)i * 4u, (uint32_t)(pos - base));
    pos += len;
  }
  memset(buf, 0, 0x10);
  memcpy(buf, "LANGO", 5);
  buf[5] = 2;
  put32le(buf + 8, (uint32_t)base);
  memcpy(buf + 0x10, offsets, MODEL_TEST_COUNT * 4u);
  free(offsets);
  return pos;
}

static size_t build_font(uint8_t *buf, size_t cap) {
  const size_t block = 0x20u + 96u * 4u;
  const size_t pixels = block + 0x10u;
  const size_t bitmap_len = 16u * 8u;
  const size_t total = pixels + bitmap_len;
  assert(total <= cap);
  memset(buf, 0, total);
  memcpy(buf, "FNTI", 4);
  buf[4] = 32;
  buf[5] = 127;
  buf[6] = 4;
  buf[7] = 6;
  buf[8] = 1;
  put32le(buf + 0x1C, (uint32_t)block);
  buf[block] = 0x7A;
  put16le(buf + block + 4, 32);
  put16le(buf + block + 6, 8);
  memset(buf + pixels, 0xFF, bitmap_len);
  return total;
}

static void test_item_table(void) {
  const struct fifa96_menu_item_desc *items = fifa96_menu_model_items();
  assert(items != NULL);
  assert(items[0].label_id == 0x81u && items[0].code == 4u);
  assert(items[0].action == FIFA96_MENU_ACTION_FRIENDLY);
  assert(items[0].col == 1 && items[0].row == 0);
  assert(items[5].label_id == 0xB3u && items[5].code == 9u && items[5].col == 0);
  assert(items[6].label_id == 0xD8u && items[6].action == FIFA96_MENU_ACTION_OPTIONS);
  assert(items[8].label_id == 0xF3u && items[8].action == FIFA96_MENU_ACTION_QUIT);
  for (int i = 0; i < FIFA96_MENU_ITEM_COUNT; i++)
    assert(fifa96_menu_model_item_fallback(i) != NULL);
}

static void test_game_select_navigation(void) {
  struct fifa96_menu_model m;
  fifa96_menu_model_init(&m);
  assert(m.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(m.item == 0);

  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.item == 1);   /* Leagues */
  assert(fifa96_menu_model_nav(&m, 0) == 1);
  assert(m.item == 0);
  assert(fifa96_menu_model_nav(&m, 0) == 1);
  assert(m.item == 4);   /* Practise: right column wraps to row 4 */
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.item == 0);
  assert(fifa96_menu_model_nav(&m, 2) == 1);
  assert(m.item == 5);   /* Load Game, left row 0 */
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.item == 6);   /* Options */
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.screen == FIFA96_MENU_SCREEN_OPTIONS);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.item == 7);   /* Modem Setup */
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.item == 8);   /* Quit to DOS */
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_QUIT);

  fifa96_menu_model_init(&m);
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.item == 1);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);   /* disabled */
  assert(m.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fifa96_menu_model_nav(&m, 2) == 1);
  assert(m.item == 6);
  assert(fifa96_menu_model_nav(&m, 3) == 1);
  assert(m.item == 1);
  assert(fifa96_menu_model_back(&m) == 0);
  assert(fifa96_menu_model_nav(&m, 9) == 0);
}

static void test_team_select(void) {
  struct fifa96_menu_model m;
  fifa96_menu_model_init(&m);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.screen == FIFA96_MENU_SCREEN_TEAM_SELECT);
  assert(m.team[0] == 0 && m.team[1] == 4 && m.team_col == 0);

  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.team[0] == 1);
  assert(fifa96_menu_model_nav(&m, 0) == 1);
  assert(m.team[0] == 0);
  assert(fifa96_menu_model_nav(&m, 0) == 1);
  assert(m.team[0] == FIFA96_MENU_TEAM_COUNT - 1);   /* wrap */
  assert(fifa96_menu_model_nav(&m, 3) == 1);
  assert(m.team_col == 1);
  assert(fifa96_menu_model_nav(&m, 0) == 1);
  assert(m.team[1] == 3);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_START_MATCH);
  assert(fifa96_menu_model_back(&m) == 1);
  assert(m.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
}

static void test_team_table(void) {
  assert(FIFA96_MENU_TEAM_COUNT == 44);
  assert(fifa96_menu_team_id(0) == 0x23u);
  assert(strcmp(fifa96_menu_team_fallback(0), "Brazil") == 0);
  assert(strcmp(fifa96_menu_team_code(0), "BRA") == 0);
  assert(fifa96_menu_team_id(1) == 0xA6u);
  assert(strcmp(fifa96_menu_team_fallback(1), "Italy") == 0);
  assert(strcmp(fifa96_menu_team_code(1), "ITA") == 0);
  assert(fifa96_menu_team_id(43) == 0xA4u);
  assert(strcmp(fifa96_menu_team_code(43), "ISR") == 0);
  assert(fifa96_menu_team_id(-1) == 0);
  assert(fifa96_menu_team_fallback(FIFA96_MENU_TEAM_COUNT) == NULL);
  assert(fifa96_menu_team_code(99) == NULL);
}

static void press(struct fifa96_frontend_run *fr, int32_t code) {
  fifa96_platform_key k = {.raw_code = code, .state = 1};
  assert(fifa96_frontend_run_input(fr, &k, 1) == 0);
}

static void step(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  int quit = 0;
  assert(fifa96_frontend_run_step(fr, s, &quit) == 0);
  assert(quit == 0);
}

static void test_native_fixture_reaches_handoff(void) {
  uint8_t lang[8192];
  uint8_t font[2048];
  size_t lang_len = build_lango(lang, sizeof lang);
  size_t font_len = build_font(font, sizeof font);

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
  assert(fifa96_lang_init_memory(&e->frontend.lang, lang, lang_len, font, font_len) == 0);
  fifa96_frontend_run_enable_native(&e->frontend, 1);

  struct fifa96_surface *s = e->surface;
  step(&e->frontend, s);
  assert(e->frontend.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  uint64_t game_select_hash = fifa96_surface_hash(s);

  press(&e->frontend, FIFA96_ENGINE_KEY_CONFIRM);
  step(&e->frontend, s);
  assert(e->frontend.menu.screen == FIFA96_MENU_SCREEN_TEAM_SELECT);
  assert(fifa96_surface_hash(s) != game_select_hash);

  /* Visitor column, then Germany (cell 4) -> Italy (cell 1). */
  press(&e->frontend, FIFA96_ENGINE_KEY_RIGHT);
  step(&e->frontend, s);
  assert(e->frontend.menu.team_col == 1);
  for (int i = 0; i < 3; i++) {
    press(&e->frontend, FIFA96_ENGINE_KEY_UP);
    step(&e->frontend, s);
  }
  assert(e->frontend.menu.team[1] == 1);
  assert(e->frontend.team_id[1] == 0xA6u);

  press(&e->frontend, FIFA96_ENGINE_KEY_CONFIRM);
  step(&e->frontend, s);
  assert(e->frontend.match_start == 1);
  assert(e->frontend.team_selected == 1);
  assert(strcmp(e->frontend.team_name[0], "Brazil") == 0);
  assert(strcmp(e->frontend.team_name[1], "Italy") == 0);
  assert(strcmp(e->frontend.team_code[0], "BRA") == 0);
  assert(strcmp(e->frontend.team_code[1], "ITA") == 0);
  assert(e->frontend.formation[0] == 0 && e->frontend.formation[1] == 0);

  assert(fifa96_match_bridge_from_frontend(&e->match_run, e) == 0);
  assert(e->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(e->match_run.running == 1);
  assert(strcmp(e->match_run.render.hud_name[0], "BRA") == 0);
  assert(strcmp(e->match_run.render.hud_name[1], "ITA") == 0);
  assert(e->match_run.formation[0] == 0 && e->match_run.formation[1] == 0);

  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

int main(void) {
  test_item_table();
  test_game_select_navigation();
  test_team_select();
  test_team_table();
  test_native_fixture_reaches_handoff();
  puts("test_engine_menu_model OK");
  return 0;
}
