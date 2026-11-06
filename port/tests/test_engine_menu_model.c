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
#include "fifa96_loader/fifa96_err.h"

#define MODEL_TEST_COUNT 0x150u
#define MENU_ISO_PATH "data/game/FIFAPCCD96.iso"

static int menu_file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

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
    case 0x009u: return "Action";
    case 0x023u: return "Brazil";
    case 0x036u: return "Clock";
    case 0x042u: return "Continuous";
    case 0x06Fu: return "English";
    case 0x07Cu: return "Fouls";
    case 0x081u: return "Friendly";
    case 0x084u: return "Game Select";
    case 0x087u: return "Game Type";
    case 0x093u: return "Half Length";
    case 0x098u: return "Home";
    case 0x09Fu: return "Injuries";
    case 0x0A6u: return "Italy";
    case 0x0AFu: return "Language";
    case 0x0C7u: return "Music";
    case 0x0D2u: return "Off";
    case 0x0D5u: return "Offside";
    case 0x0D7u: return "On";
    case 0x0D8u: return "Options";
    case 0x0DDu: return "Pitch Condition";
    case 0x0DFu: return "Play by Play";
    case 0x0F3u: return "Quit to DOS";
    case 0x0F5u: return "Random";
    case 0x113u: return "Sound Effects";
    case 0x11Du: return "Skill Level";
    case 0x131u: return "Super VGA";
    case 0x13Du: return "Team Select";
    case 0x142u: return "Time Display";
    case 0x14Au: return "Visitor";
    case 0x184u: return "Music Volume";
    case 0x185u: return "Speech Volume";
    case 0x18Bu: return "Player Names";
    case 0x191u: return "SFX Volume";
    case 0x1C8u: return "Names";
    case 0x1C9u: return "Numbers";
    case 0x21Bu: return "4-Button Joypad";
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
  assert(m.option_row == 0 && m.option_scroll == 0);
  assert(m.option_page == FIFA96_MENU_OPTION_PAGE_LIST);
  assert(fifa96_menu_model_back(&m) == 1);
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
  /* P6: every row has a mode-4 slot; Brazil 6, Italy 28, Israel 27. */
  assert(fifa96_menu_team_pccd_index(0) == 6);
  assert(fifa96_menu_team_pccd_index(1) == 28);
  assert(fifa96_menu_team_pccd_index(43) == 27);
  assert(fifa96_menu_team_pccd_index(-1) == -1);
  assert(fifa96_menu_team_pccd_index(FIFA96_MENU_TEAM_COUNT) == -1);
}

static void test_option_descriptors(void) {
  static const int32_t expected_max[FIFA96_MENU_OPTION_DESC_COUNT] = {
      2, 2, 2, 6, 2, 2, 2, 3, 3, 2, 3, 2, 5,
      2, 7, 2, 2, 100, 100, 100, 0, 2, 0, 0, 0, 0};
  static const uint8_t expected_order[FIFA96_MENU_OPTION_ORDER_COUNT] = {
      0, 18, 1, 17, 2, 19, 3, 4, 6, 7, 21, 9, 10, 11, 12, 13, 14, 15, 16, 66};
  const struct fifa96_menu_option_desc *d = fifa96_menu_option_descriptors();
  assert(d != NULL);
  assert(fifa96_menu_option_descriptor_count() == FIFA96_MENU_OPTION_DESC_COUNT);
  assert(fifa96_menu_option_descriptor_count() == 26);
  for (int i = 0; i < FIFA96_MENU_OPTION_DESC_COUNT; i++) {
    assert(d[i].setting == (uint8_t)i);
    assert(d[i].max == expected_max[i]);
    assert(d[i].label != NULL || i == 20 || i >= 22);
    /* The P6 live set: Sound Effects, Fouls, Injuries, Half Length, Offside,
     * SFX Volume. Everything else is visible but dim. */
    int live = (i == 0x01 || i == 0x0A || i == 0x0B ||
                i == 0x0E || i == 0x10 || i == 0x11);
    assert(d[i].live == live);
    assert(d[i].is_slider == (i >= 0x11 && i <= 0x13));
  }
  assert(d[0].label_id == 0xC7u && strcmp(d[0].label, "Music") == 0);
  assert(d[0].choices[0] == 0xD2u && d[0].choices[1] == 0xD7u);
  assert(d[1].label_id == 0x113u && strcmp(d[1].label, "Sound Effects") == 0);
  assert(d[3].label_id == 0xAFu && d[3].max == 6);
  assert(d[3].choices[0] == 0x6Fu && d[3].choices[4] == 0x135u);
  assert(d[7].choices[0] == 0xD2u && d[7].choices[1] == 0x1C8u &&
         d[7].choices[2] == 0x1C9u);
  assert(strcmp(d[7].choice_text[1], "Names") == 0);
  assert(strcmp(d[7].choice_text[2], "Numbers") == 0);
  assert(d[10].label_id == 0x7Cu && d[10].choices[1] == 0xCDu);
  assert(d[14].label_id == 0x93u && d[14].max == 7);
  assert(d[14].choices[0] == 0x248u && d[14].choices[6] == 0x24Eu);
  assert(d[17].label_id == 0x191u && d[17].max == 100u);
  assert(d[17].choices == NULL && d[17].choice_text == NULL);
  assert(d[20].max == 0 && d[20].label == NULL && d[20].label_id == 0);
  assert(d[21].label_id == 0x21Bu && d[21].max == 2u);
  for (int i = 22; i < FIFA96_MENU_OPTION_DESC_COUNT; i++)
    assert(d[i].max == 0 && d[i].label == NULL);

  const uint8_t *order = fifa96_menu_option_order();
  assert(order != NULL);
  assert(memcmp(order, expected_order, sizeof expected_order) == 0);
  assert(order[19] == 66u);   /* dead descriptor, never selectable */

  assert(fifa96_menu_model_option_setting(0) == 0);
  assert(fifa96_menu_model_option_setting(1) == 18);
  assert(fifa96_menu_model_option_setting(18) == 16);
  assert(fifa96_menu_model_option_setting(19) == -1);
  assert(fifa96_menu_model_option_setting(-1) == -1);
  assert(strcmp(fifa96_menu_model_option_label(16), "Half Length") == 0);
  assert(fifa96_menu_model_option_label_id(16) == 0x93u);
  assert(strcmp(fifa96_menu_model_option_label(FIFA96_MENU_OPTION_ROW_CONTROLS),
                "Controls") == 0);
  assert(strcmp(fifa96_menu_model_option_label(FIFA96_MENU_OPTION_ROW_RESET),
                "Reset Defaults") == 0);
  assert(fifa96_menu_model_option_rows() == 21);
  size_t help_count = 0;
  const char *const *help = fifa96_menu_model_controls_text(&help_count);
  assert(help != NULL && help_count == 14);
  assert(strcmp(help[0], "CONTROLS") == 0);
  /* The real dual roles: the shared CONFIRM/DECLINE codes are pad A/South and
   * keyboard Enter, and pad B/East and keyboard X/Backspace. */
  assert(strcmp(help[1], "A/SOUTH or ENTER") == 0);
  assert(strcmp(help[2], "  MENU: SELECT / CONFIRM") == 0);
  assert(strcmp(help[3], "  PLAY: KICK") == 0);
  assert(strcmp(help[4], "B/EAST or X/BACKSPACE") == 0);
  assert(strcmp(help[5], "  MENU: BACK / DECLINE") == 0);
  assert(strcmp(help[6], "  PLAY: PASS") == 0);
  assert(strcmp(help[7], "Z KICK   C PASS   V LONG BALL") == 0);
  assert(strcmp(help[8], "L/R + Z  AIM   CENTRE = KEEPER") == 0);
  assert(strcmp(help[9], "HALF-TIME: C/PASS STARTS 2ND HALF") == 0);
  assert(strcmp(help[help_count - 1], "PAD WEST  LONG BALL") == 0);
}

static void test_option_defaults_and_values(void) {
  struct fifa96_menu_model m;
  fifa96_menu_model_init(&m);
  assert(m.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(m.option_row == 0 && m.option_scroll == 0);
  assert(m.option_page == FIFA96_MENU_OPTION_PAGE_LIST);
  assert(m.settings.value[0x00] == 1);
  assert(m.settings.value[0x01] == 1);
  assert(m.settings.value[0x0A] == 2);
  assert(m.settings.value[0x0B] == 1);
  assert(m.settings.value[0x0E] == 0);
  assert(m.settings.value[0x10] == 0);
  assert(m.settings.value[0x11] == 0x5B);
  assert(m.settings.value[0x12] == 0x5D);
  assert(m.settings.value[0x13] == 0x62);
  const int32_t *max = fifa96_settings_descriptor_max();
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++)
    assert(m.settings.max[i] == max[i]);
  assert(m.settings.max[0x0E] == 7);

  char buf[40];
  assert(fifa96_menu_model_option_value(&m, 16, buf, sizeof buf) == 1);
  assert(strcmp(buf, "2 min") == 0);
  m.settings.value[0x0E] = 6;
  assert(fifa96_menu_model_option_value(&m, 16, buf, sizeof buf) == 1);
  assert(strcmp(buf, "45 min") == 0);
  /* Native apply table index 7 is the honest 0-minute entry (no LANG choice). */
  m.settings.value[0x0E] = 7;
  assert(fifa96_menu_option_choice_id(0x0E, 7) == 0);
  assert(strcmp(fifa96_menu_option_choice_text(0x0E, 7), "0") == 0);
  assert(fifa96_menu_model_option_value(&m, 16, buf, sizeof buf) == 1);
  assert(strcmp(buf, "0 min") == 0);

  m.settings.value[0x0A] = 1;
  assert(fifa96_menu_model_option_value(&m, 12, buf, sizeof buf) == 1);
  assert(strcmp(buf, "No Bookings") == 0);
  m.settings.value[0x0A] = 2;
  assert(fifa96_menu_model_option_value(&m, 12, buf, sizeof buf) == 1);
  assert(strcmp(buf, "On") == 0);
  assert(fifa96_menu_model_option_value(&m, 3, buf, sizeof buf) == 1);
  assert(strcmp(buf, "91") == 0);
  assert(fifa96_menu_model_option_value(&m, 0, buf, sizeof buf) == 1);
  assert(strcmp(buf, "On") == 0);
  /* Port rows carry no value. */
  assert(fifa96_menu_model_option_value(&m, FIFA96_MENU_OPTION_ROW_CONTROLS,
                                        buf, sizeof buf) == 0);
  assert(fifa96_menu_model_option_value(NULL, 0, buf, sizeof buf) == 0);
  assert(fifa96_menu_model_option_value(&m, 0, NULL, sizeof buf) == 0);
}

static void test_option_navigation(void) {
  struct fifa96_menu_model m;
  fifa96_menu_model_init(&m);
  /* Game Select: right to the left column, down to Options, Enter. */
  assert(fifa96_menu_model_nav(&m, 3) == 1);
  assert(m.item == 5);
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.item == 6);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.screen == FIFA96_MENU_SCREEN_OPTIONS);

  /* Up at the top clamps (no wrap). */
  assert(fifa96_menu_model_nav(&m, 0) == 1);
  assert(m.option_row == 0 && m.option_scroll == 0);
  /* Row 0 Music is stored-only: left/right and Enter do nothing. */
  int32_t music = m.settings.value[0x00];
  assert(fifa96_menu_model_nav(&m, 2) == 1);
  assert(fifa96_menu_model_nav(&m, 3) == 1);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x00] == music);

  for (int i = 0; i < 16; i++) assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.option_row == 16);            /* Half Length */
  assert(m.option_scroll == 9);
  assert(m.settings.value[0x0E] == 0);
  assert(fifa96_menu_model_nav(&m, 3) == 1);
  assert(m.settings.value[0x0E] == 1);   /* 4 min */
  assert(fifa96_menu_model_nav(&m, 2) == 1);
  assert(m.settings.value[0x0E] == 0);
  /* Enter cycles (gamepad convenience): 0 -> 1. */
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x0E] == 1);

  /* Down to the last native row, then the two port rows; scroll follows. */
  for (int i = 0; i < 2; i++) assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.option_row == 18 && m.option_scroll == 11);
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.option_row == FIFA96_MENU_OPTION_ROW_CONTROLS && m.option_scroll == 12);
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.option_row == FIFA96_MENU_OPTION_ROW_RESET && m.option_scroll == 13);
  assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.option_row == FIFA96_MENU_OPTION_ROW_RESET);   /* clamped */
  assert(m.option_scroll == 13);

  /* Reset Defaults re-seeds the retail defaults + max. */
  m.settings.value[0x0E] = 4;
  m.settings.value[0x0A] = 0;
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x0E] == 0);
  assert(m.settings.value[0x0A] == 2);
  assert(m.settings.max[0x0E] == 7);

  /* Controls page: port addition; back returns to the list, then Game Select. */
  m.option_row = FIFA96_MENU_OPTION_ROW_CONTROLS;
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.screen == FIFA96_MENU_SCREEN_OPTIONS);
  assert(m.option_page == FIFA96_MENU_OPTION_PAGE_CONTROLS);
  assert(fifa96_menu_model_nav(&m, 1) == 1);   /* no selectable rows */
  assert(m.option_page == FIFA96_MENU_OPTION_PAGE_CONTROLS);
  assert(fifa96_menu_model_back(&m) == 1);
  assert(m.screen == FIFA96_MENU_SCREEN_OPTIONS);
  assert(m.option_page == FIFA96_MENU_OPTION_PAGE_LIST);
  assert(fifa96_menu_model_back(&m) == 1);
  assert(m.screen == FIFA96_MENU_SCREEN_GAME_SELECT);

  /* Sliders step by 5 and stop at 99 (descriptor max 100). Re-enter Options
   * (the Game Select item is still Options) and walk to SFX Volume row 3. */
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.option_row == 0 && m.option_scroll == 0);
  for (int i = 0; i < 3; i++) assert(fifa96_menu_model_nav(&m, 1) == 1);
  assert(m.option_row == 3);
  assert(m.settings.value[0x11] == 0x5B);
  assert(fifa96_menu_model_nav(&m, 3) == 1);
  assert(m.settings.value[0x11] == 0x60);   /* 91 + 5 = 96 */
  assert(fifa96_menu_model_nav(&m, 2) == 1);
  assert(m.settings.value[0x11] == 0x5B);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x11] == 0x60);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x11] == 0x63);   /* 96 + 5 clamps to max-1 */
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x11] == 0x63);

  /* Sound Effects row 2: Off/On cycles as a two-choice row. */
  m.option_row = 2;
  m.settings.value[0x01] = 1;
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x01] == 0);
  assert(fifa96_menu_model_activate(&m) == FIFA96_MENU_ACTION_NONE);
  assert(m.settings.value[0x01] == 1);
}

/* P6: the bridge applies the Options block only inside the Team Select
 * handoff. The direct-begin path (team_selected = 0) keeps begin's default
 * 120/40 handoff even when the menu settings were edited. */
static struct fifa96_engine *option_engine_make(fifa96_platform **plat) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;
  *plat = fifa96_platform_null_create(&pcfg);
  assert(*plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, *plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  return e;
}

static void test_bridge_applies_options(void) {
  fifa96_platform *plat = NULL;
  struct fifa96_engine *e = option_engine_make(&plat);
  e->frontend.menu.settings.value[0x0E] = 1;   /* 4 minutes */
  e->frontend.menu.settings.value[0x0A] = 1;   /* No Bookings */
  e->frontend.menu.settings.value[0x0B] = 0;   /* Injuries off */
  e->frontend.menu.settings.value[0x10] = 1;   /* Offside on */
  e->frontend.menu.settings.value[0x09] = 1;   /* Simulation */
  e->frontend.menu.settings.value[0x03] = 1;   /* German */
  e->frontend.menu.settings.value[0x06] = 0;   /* Time display off */
  e->frontend.menu.settings.value[0x0D] = 1;   /* Clock out of play */
  e->frontend.menu.settings.value[0x11] = 50;  /* audio-only setting */
  e->frontend.team_selected = 1;
  e->frontend.match_start = 1;

  assert(fifa96_match_bridge_from_frontend(&e->match_run, e) == 0);
  assert(e->match_run.running == 1);
  assert(e->match_run.config.field_4c2f6 == 1);
  assert(e->match_run.config.field_4c2e6 == 1);
  assert(e->match_run.config.field_4c30a == 0);
  assert(e->match_run.config.field_4c306 == 1);
  assert(e->match_run.config.field_4c2f2 == 1);
  assert(e->match_run.config.field_4c312 == 1);   /* Play by Play default On */
  assert(e->match_run.config.field_4c316 == 0);
  assert(e->match_run.config.clock_halt == 1);
  assert(e->match_run.config.half_length_minutes == 4);
  assert(e->match_run.config.period_length == 4 * 60);
  assert(e->match_run.config.extra_length == 4 * 20);
  assert(e->match_run.state.period_length == 4 * 60);
  assert(e->match_run.state.extra_length == 4 * 20);
  assert(fifa96_match_run_set_config(NULL, &e->match_run.config) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_set_config(&e->match_run, NULL) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_end(&e->match_run) == 0);
  struct fifa96_match_config cfg;
  memset(&cfg, 0, sizeof cfg);
  assert(fifa96_match_run_set_config(&e->match_run, &cfg) == -FIFA96_ERR_STATE);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

static void test_bridge_direct_begin_keeps_defaults(void) {
  fifa96_platform *plat = NULL;
  struct fifa96_engine *e = option_engine_make(&plat);
  /* Edit the options to prove the gate, then start without Team Select. */
  e->frontend.menu.settings.value[0x0E] = 6;
  e->frontend.menu.settings.value[0x0A] = 0;
  e->frontend.menu.settings.value[0x0B] = 0;
  e->frontend.menu.settings.value[0x10] = 1;
  e->frontend.team_selected = 0;
  e->frontend.match_start = 1;

  assert(fifa96_match_bridge_from_frontend(&e->match_run, e) == 0);
  assert(e->match_run.running == 1);
  assert(e->match_run.config.field_4c306 == 2);
  assert(e->match_run.config.field_4c30a == 1);
  assert(e->match_run.config.field_4c2f2 == 0);
  assert(e->match_run.config.half_length_minutes == 2);
  assert(e->match_run.config.period_length == 120);
  assert(e->match_run.config.extra_length == 40);
  assert(e->match_run.state.period_length == 120);
  assert(e->match_run.state.extra_length == 40);
  assert(fifa96_match_run_end(&e->match_run) == 0);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

static void key_event(struct fifa96_frontend_run *fr, int32_t code, int state) {
  fifa96_platform_key k = {.raw_code = code, .state = state};
  assert(fifa96_frontend_run_input(fr, &k, 1) == 0);
}

static void idle(struct fifa96_frontend_run *fr) {
  assert(fifa96_frontend_run_input(fr, NULL, 0) == 0);
}

/* A discrete tap: press and release in one poll, then the run step consumes
 * the queue. The F1 edge latch clears key_down on the release, so the same
 * key can be pressed again on the next poll. */
static void press(struct fifa96_frontend_run *fr, int32_t code) {
  fifa96_platform_key k[2] = {
    {.raw_code = code, .state = 1},
    {.raw_code = code, .state = 0},
  };
  assert(fifa96_frontend_run_input(fr, k, 2) == 0);
}

static void step(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  int quit = 0;
  assert(fifa96_frontend_run_step(fr, s, &quit) == 0);
  assert(quit == 0);
}

/* P6 (T3 review F3): the Options screen was only pinned at the model layer and
 * the art test fabricated its snapshot. This case drives the real native step
 * with the decoded LANGO/FNTI fixtures (the existing font fixture: the FNTI
 * image is parsed by fifa96_lang_init_memory and the same step draws it), then
 * asserts the draw-ready fifa96_menu_state: rows/labels/values, the selected
 * row and scroll window, the Controls page fill, and Back -> Game Select with
 * the changed setting preserved. */
static void test_native_options_state_fill(void) {
  uint8_t lang[8192];
  uint8_t font[2048];
  size_t lang_len = build_lango(lang, sizeof lang);
  size_t font_len = build_font(font, sizeof font);

  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  assert(fifa96_lang_init_memory(&fr.lang, lang, lang_len, font, font_len) == 0);
  fifa96_frontend_run_enable_native(&fr, 1);
  idle(&fr);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);

  struct fifa96_menu_state st;
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(st.font != NULL);
  assert(st.item_count == FIFA96_MENU_ITEM_COUNT && st.selected_item == 0);
  assert(strcmp(st.title, "Game Select") == 0);
  assert(strcmp(st.items[0].label, "Friendly") == 0);
  assert(st.items[0].enabled == 1 && st.items[1].enabled == 0);

  /* Game Select -> left column -> Options -> Enter. */
  press(&fr, FIFA96_ENGINE_KEY_RIGHT);
  step(&fr, s);
  assert(fr.menu.item == 5);
  press(&fr, FIFA96_ENGINE_KEY_DOWN);
  step(&fr, s);
  assert(fr.menu.item == 6);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_OPTIONS);

  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.screen == FIFA96_MENU_SCREEN_OPTIONS);
  assert(st.font != NULL);
  assert(strcmp(st.title, "Options") == 0);
  assert(st.option_page == FIFA96_MENU_OPTION_PAGE_LIST);
  assert(st.option_count == FIFA96_MENU_OPTION_ROW_COUNT);
  assert(st.option_selected == 0 && st.option_scroll == 0);
  static const char *const window_labels[FIFA96_MENU_OPTION_VISIBLE] = {
      "Music", "Music Volume", "Sound Effects", "SFX Volume",
      "Play by Play", "Speech Volume", "Language", "Super VGA"};
  static const char *const window_values[FIFA96_MENU_OPTION_VISIBLE] = {
      "On", "93", "On", "91", "On", "98", "English", "Off"};
  static const int window_enabled[FIFA96_MENU_OPTION_VISIBLE] = {
      0, 0, 1, 1, 0, 0, 0, 0};
  static const int window_slider[FIFA96_MENU_OPTION_VISIBLE] = {
      -1, 93, -1, 91, -1, 98, -1, -1};
  for (int k = 0; k < FIFA96_MENU_OPTION_VISIBLE; k++) {
    assert(strcmp(st.options[k].label, window_labels[k]) == 0);
    assert(strcmp(st.options[k].value, window_values[k]) == 0);
    assert(st.options[k].enabled == window_enabled[k]);
    assert(st.options[k].slider_pct == window_slider[k]);
  }

  /* Scroll to Half Length (row 16 -> scroll 9) and check the scrolled fill. */
  for (int i = 0; i < 16; i++) {
    press(&fr, FIFA96_ENGINE_KEY_DOWN);
    step(&fr, s);
  }
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.option_selected == 16 && st.option_scroll == 9);
  assert(st.option_count == FIFA96_MENU_OPTION_ROW_COUNT);
  static const char *const scroll_labels[FIFA96_MENU_OPTION_VISIBLE] = {
      "Player Names", "4-Button Joypad", "Game Type", "Fouls",
      "Injuries", "Pitch Condition", "Clock", "Half Length"};
  static const char *const scroll_values[FIFA96_MENU_OPTION_VISIBLE] = {
      "Off", "Off", "Action", "On", "On", "Random", "Continuous", "2 min"};
  static const int scroll_enabled[FIFA96_MENU_OPTION_VISIBLE] = {
      0, 0, 0, 1, 1, 0, 0, 1};
  for (int k = 0; k < FIFA96_MENU_OPTION_VISIBLE; k++) {
    assert(strcmp(st.options[k].label, scroll_labels[k]) == 0);
    assert(strcmp(st.options[k].value, scroll_values[k]) == 0);
    assert(st.options[k].enabled == scroll_enabled[k]);
    assert(st.options[k].slider_pct == -1);
  }

  /* 2 min -> 4 min; the next snapshot reflects it. */
  press(&fr, FIFA96_ENGINE_KEY_RIGHT);
  step(&fr, s);
  assert(fr.menu.settings.value[0x0E] == 1);
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(strcmp(st.options[7].value, "4 min") == 0);

  /* Controls: row 19, Enter -> the help page fill. */
  for (int i = 0; i < 3; i++) {
    press(&fr, FIFA96_ENGINE_KEY_DOWN);
    step(&fr, s);
  }
  assert(fr.menu.option_row == FIFA96_MENU_OPTION_ROW_CONTROLS);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.option_page == FIFA96_MENU_OPTION_PAGE_CONTROLS);
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.option_page == FIFA96_MENU_OPTION_PAGE_CONTROLS);
  size_t help_count = 0;
  const char *const *help = fifa96_menu_model_controls_text(&help_count);
  assert(st.help_count == (int)help_count);
  for (size_t i = 0; i < help_count; i++) assert(strcmp(st.help[i], help[i]) == 0);
  assert(strcmp(st.help[0], "CONTROLS") == 0);
  assert(strcmp(st.help[st.help_count - 1], "PAD WEST  LONG BALL") == 0);
  assert(strcmp(st.help[st.help_count - 2], "     MATCH: PAUSE") == 0);
  assert(strcmp(st.help[st.help_count - 3], "ESC  MENU: QUIT TO DOS") == 0);

  /* Back -> list (window kept), Back -> Game Select; the setting survives. */
  press(&fr, FIFA96_ENGINE_KEY_DECLINE);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_OPTIONS);
  assert(fr.menu.option_page == FIFA96_MENU_OPTION_PAGE_LIST);
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.option_page == FIFA96_MENU_OPTION_PAGE_LIST);
  assert(st.option_selected == FIFA96_MENU_OPTION_ROW_CONTROLS);
  assert(st.option_scroll == 12);
  assert(strcmp(st.options[0].label, "Fouls") == 0);
  press(&fr, FIFA96_ENGINE_KEY_DECLINE);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(st.selected_item == 6);   /* still the Options item */
  assert(fr.menu.settings.value[0x0E] == 1);

  /* Re-enter: the Options window is re-seeded at the top, the value is kept. */
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_OPTIONS);
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(st.option_selected == 0 && st.option_scroll == 0);
  for (int i = 0; i < 16; i++) {
    press(&fr, FIFA96_ENGINE_KEY_DOWN);
    step(&fr, s);
  }
  fifa96_frontend_run_menu_state(&fr, &st);
  assert(strcmp(st.options[7].value, "4 min") == 0);

  fifa96_lang_shutdown(&fr.lang);
  fifa96_surface_destroy(s);
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
  idle(&e->frontend);   /* F1: one neutral poll arms the native edge latch */

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
  /* P6 no-ISO fallback: the static mode-4 slots are known, but the record
   * identity stays zero and the 3-letter menu code is unchanged. */
  assert(e->frontend.pccd_index[0] == 6 && e->frontend.pccd_index[1] == 28);
  assert(e->frontend.pccd_record[0] == 0 && e->frontend.pccd_record[1] == 0);
  assert(e->frontend.pccd_id[0] == 0 && e->frontend.pccd_id[1] == 0);
  assert(e->frontend.pccd_ready[0] == 0 && e->frontend.pccd_ready[1] == 0);

  assert(fifa96_match_bridge_from_frontend(&e->match_run, e) == 0);
  assert(e->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(e->match_run.running == 1);
  assert(strcmp(e->match_run.render.hud_name[0], "BRA") == 0);
  assert(strcmp(e->match_run.render.hud_name[1], "ITA") == 0);
  assert(e->match_run.formation[0] == 0 && e->match_run.formation[1] == 0);
  /* P6: the Options apply is a no-op with the default block — begin already
   * installed the same cells and the same 120/40 period lengths. */
  assert(e->match_run.config.field_4c306 == 2);
  assert(e->match_run.config.field_4c30a == 1);
  assert(e->match_run.config.field_4c2f2 == 0);
  assert(e->match_run.config.half_length_minutes == 2);
  assert(e->match_run.config.period_length == 120);
  assert(e->match_run.config.extra_length == 40);
  assert(e->match_run.state.period_length == 120);
  assert(e->match_run.state.extra_length == 40);

  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

/* P6 ISO-gated: the same keyboard path now resolves Brazil/Italy through the
 * decoded PCCD mode-4 table, so the handoff carries the record +0x08 codes
 * (BRA/ITALY), ids, formations and kit-0 triples into the match begin. */
static void test_native_pccd_handoff(void) {
  if (!menu_file_exists(MENU_ISO_PATH)) {
    fprintf(stderr, "SKIP PCCD handoff case (no %s)\n", MENU_ISO_PATH);
    return;
  }
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
  ecfg.iso_path = MENU_ISO_PATH;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(e->team_data.ready == 1);
  assert(fifa96_lang_init_memory(&e->frontend.lang, lang, lang_len, font, font_len) == 0);
  fifa96_frontend_run_enable_native(&e->frontend, 1);
  idle(&e->frontend);
  step(&e->frontend, e->surface);
  assert(e->frontend.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);

  press(&e->frontend, FIFA96_ENGINE_KEY_CONFIRM);
  step(&e->frontend, e->surface);
  assert(e->frontend.menu.screen == FIFA96_MENU_SCREEN_TEAM_SELECT);
  press(&e->frontend, FIFA96_ENGINE_KEY_RIGHT);
  step(&e->frontend, e->surface);
  for (int i = 0; i < 3; i++) {
    press(&e->frontend, FIFA96_ENGINE_KEY_UP);
    step(&e->frontend, e->surface);
  }
  assert(e->frontend.menu.team[1] == 1);   /* Italy */

  press(&e->frontend, FIFA96_ENGINE_KEY_CONFIRM);
  step(&e->frontend, e->surface);
  assert(e->frontend.match_start == 1);
  assert(e->frontend.team_selected == 1);
  assert(strcmp(e->frontend.team_code[0], "BRA") == 0);
  assert(strcmp(e->frontend.team_code[1], "ITALY") == 0);
  assert(e->frontend.pccd_index[0] == 6 && e->frontend.pccd_index[1] == 28);
  assert(e->frontend.pccd_record[0] == 0x2B && e->frontend.pccd_record[1] == 0x20);
  assert(e->frontend.pccd_id[0] == 59u && e->frontend.pccd_id[1] == 47u);
  assert(e->frontend.formation[0] == 1 && e->frontend.formation[1] == 2);
  assert(e->frontend.pccd_ready[0] == 1 && e->frontend.pccd_ready[1] == 1);
  assert(e->frontend.kit[0][0][0] == 26 && e->frontend.kit[0][0][1] == 4 &&
         e->frontend.kit[0][0][2] == 7);
  assert(e->frontend.kit[1][0][0] == 3 && e->frontend.kit[1][0][1] == 30 &&
         e->frontend.kit[1][0][2] == 3);
  assert(e->frontend.roster[0][0] == 2047u && e->frontend.roster[1][0] == 806u);

  assert(fifa96_match_bridge_from_frontend(&e->match_run, e) == 0);
  assert(e->match_run.formation[0] == 1 && e->match_run.formation[1] == 2);
  assert(strcmp(e->match_run.render.hud_name[0], "BRA") == 0);
  assert(strcmp(e->match_run.render.hud_name[1], "ITALY") == 0);
  assert(e->match_run.matchup_active == 1);
  assert(e->match_run.matchup.kit_ready == 1);
  assert(fifa96_match_run_end(&e->match_run) == 0);

  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

/* F1: the SDL3 backend re-presents every physically held key with state == 1
 * on every 10 ms poll. Two consecutive held CONFIRM polls on Game Select must
 * enter Team Select once, never activate START_MATCH; only a release followed
 * by a fresh press starts the match. */
static void test_native_held_confirm_latch(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  fifa96_frontend_run_enable_native(&fr, 1);   /* armed after one neutral poll */
  idle(&fr);
  assert(fr.input_armed == 1);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);

  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_TEAM_SELECT);
  assert(fr.match_start == 0);

  /* The same held key on the next poll is not a fresh press. */
  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_TEAM_SELECT);
  assert(fr.match_start == 0);

  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 0);
  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  step(&fr, s);
  assert(fr.match_start == 1);

  fifa96_surface_destroy(s);
}

/* F1: reset happens while the result key is still down. The held CONFIRM/ESC
 * must be ignored until a neutral (released) poll and a fresh press; a held
 * ESC must not set quit_requested. */
static void test_native_held_input_after_reset(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  fifa96_frontend_run_enable_native(&fr, 1);
  idle(&fr);
  step(&fr, s);

  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  assert(fr.input_armed == 0);
  assert(fr.key_down[FIFA96_ENGINE_KEY_CONFIRM] == 0);

  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fr.match_start == 0);
  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_GAME_SELECT);
  assert(fr.match_start == 0);

  /* Release arms the latch; the next press is fresh. */
  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 0);
  assert(fr.input_armed == 1);
  key_event(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  step(&fr, s);
  assert(fr.menu.screen == FIFA96_MENU_SCREEN_TEAM_SELECT);

  /* Held ESC after reset: no quit_requested, no process exit. */
  assert(fifa96_frontend_run_reset(&fr, s) == 0);
  key_event(&fr, FIFA96_ENGINE_KEY_QUIT, 1);
  step(&fr, s);
  assert(fr.quit_requested == 0);
  key_event(&fr, FIFA96_ENGINE_KEY_QUIT, 1);
  step(&fr, s);
  assert(fr.quit_requested == 0);

  /* Release + fresh ESC is still a deliberate quit. */
  key_event(&fr, FIFA96_ENGINE_KEY_QUIT, 0);
  key_event(&fr, FIFA96_ENGINE_KEY_QUIT, 1);
  int quit = 0;
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(quit == 1);

  fifa96_surface_destroy(s);
}

int main(void) {
  test_item_table();
  test_game_select_navigation();
  test_team_select();
  test_team_table();
  test_option_descriptors();
  test_option_defaults_and_values();
  test_option_navigation();
  test_native_options_state_fill();
  test_native_held_confirm_latch();
  test_native_held_input_after_reset();
  test_native_fixture_reaches_handoff();
  test_native_pccd_handoff();
  test_bridge_applies_options();
  test_bridge_direct_begin_keeps_defaults();
  puts("test_engine_menu_model OK");
  return 0;
}
