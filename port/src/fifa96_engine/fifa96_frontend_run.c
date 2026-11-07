#include <stdio.h>
#include <string.h>
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_menu_art.h"

#define FRONTEND_RUN_QUEUE_CAP 8

/* M1 approximation of the FU-66 action-code space (FUN_00016350 is not
 * decomposed): directions are menu-navigation codes 0..3, CONFIRM is the
 * -10 action the confirm gate upgrades to CONFIRM (else DECLINE), and
 * DECLINE leaves the front-end loop (code 8 -> panel). QUIT never reaches
 * the library. */
static int frontend_run_map_key(int32_t raw_code, int *code) {
  switch (raw_code) {
    case FIFA96_ENGINE_KEY_UP:      *code = 0;   return 1;
    case FIFA96_ENGINE_KEY_DOWN:    *code = 1;   return 1;
    case FIFA96_ENGINE_KEY_LEFT:    *code = 2;   return 1;
    case FIFA96_ENGINE_KEY_RIGHT:   *code = 3;   return 1;
    case FIFA96_ENGINE_KEY_CONFIRM: *code = -10; return 1;
    case FIFA96_ENGINE_KEY_DECLINE: *code = 8;   return 1;
    default:                        return 0;
  }
}

static const char *frontend_native_string(const struct fifa96_frontend_run *fr, uint32_t id,
                                          const char *fallback) {
  const char *s = fifa96_lang_string(&fr->lang, id);
  return s != NULL ? s : fallback;
}

static void frontend_copy_name(char dst[FIFA96_FRONTEND_TEAM_NAME_MAX], const char *src) {
  size_t len = src != NULL ? strlen(src) : 0;
  if (len >= FIFA96_FRONTEND_TEAM_NAME_MAX) len = FIFA96_FRONTEND_TEAM_NAME_MAX - 1;
  if (len != 0) memcpy(dst, src, len);
  dst[len] = '\0';
}

/* Uppercase ASCII and truncate to FIFA96_TEAM_CODE_MAX-1 (the native
 * FUN_00017748 filter writes a 10-byte local). */
static void frontend_copy_code(char dst[FIFA96_TEAM_CODE_MAX], const char *src) {
  memset(dst, 0, FIFA96_TEAM_CODE_MAX);
  if (src == NULL) return;
  size_t len = strlen(src);
  if (len > FIFA96_TEAM_CODE_MAX - 1u) len = FIFA96_TEAM_CODE_MAX - 1u;
  for (size_t i = 0; i < len; i++) {
    char c = src[i];
    if (c >= 'a' && c <= 'z') c = (char)(c - ('a' - 'A'));
    dst[i] = c;
  }
}

/* P13: which 44-name menu row feeds each match side. Friendly uses the two
 * Team Select cells. League/cup use the current user fixture's home/away
 * (the user is always presented as home); Practise uses the chosen home side
 * and the fixed Germany row (4). */
static int frontend_native_team_index(const struct fifa96_frontend_run *fr, int side) {
  if (fr->mode == FIFA96_COMPETITION_PRACTISE)
    return side == 0 ? fr->menu.team[0] : 4;
  if (fr->mode >= FIFA96_COMPETITION_LEAGUE &&
      fr->mode <= FIFA96_COMPETITION_PLAYOFF &&
      fr->competition.state != FIFA96_COMPETITION_IDLE) {
    const struct fifa96_competition_fixture *fixture =
        fifa96_competition_fixture(&fr->competition, fr->competition.current);
    if (fixture != NULL && fixture->user != 0 &&
        fixture->home < fr->competition.team_count &&
        fixture->away < fr->competition.team_count)
      return (int)fr->competition.teams[side == 0 ? fixture->home : fixture->away];
  }
  return fr->menu.team[side];
}

/* Snapshot the model's current teams into the bridge payload. The full name is
 * kept for display. P6: when the engine's team data is ready the handoff is the
 * selected PCCD mode-4 record — id/record/index, short-name code (+0x08),
 * formation (+0x12), kit triples (+0x25..+0x30), print colours and roster; the
 * menu's 3-letter code and zeroed identity stay the no-ISO fallback. */
static void frontend_native_sync_teams(struct fifa96_frontend_run *fr) {
  for (int side = 0; side < 2; side++) {
    int index = frontend_native_team_index(fr, side);
    fr->team_id[side] = fifa96_menu_team_id(index);
    frontend_copy_name(fr->team_name[side],
                       frontend_native_string(fr, fr->team_id[side],
                                              fifa96_menu_team_fallback(index)));
    int pccd_index = fifa96_menu_team_pccd_index(index);
    fr->pccd_index[side] = pccd_index >= 0 ? (uint8_t)pccd_index : 0;
    fr->pccd_record[side] = 0;
    fr->pccd_id[side] = 0;
    fr->formation[side] = 0;
    memset(fr->kit[side], 0, sizeof fr->kit[side]);
    memset(fr->kit_print[side], 0, sizeof fr->kit_print[side]);
    memset(fr->roster[side], 0, sizeof fr->roster[side]);
    fr->pccd_ready[side] = 0;
    frontend_copy_code(fr->team_code[side], fifa96_menu_team_code(index));

    struct fifa96_team_identity identity;
    if (fr->team_data != NULL &&
        fifa96_team_data_lookup(fr->team_data, fr->team_id[side], &identity) == 0) {
      fr->pccd_index[side] = identity.pccd_index;
      fr->pccd_record[side] = identity.pccd_record;
      fr->pccd_id[side] = identity.pccd_id;
      fr->formation[side] = identity.formation;
      memcpy(fr->kit[side], identity.kit, sizeof fr->kit[side]);
      memcpy(fr->kit_print[side], identity.print, sizeof fr->kit_print[side]);
      memcpy(fr->roster[side], identity.roster, sizeof fr->roster[side]);
      frontend_copy_code(fr->team_code[side], identity.code);
      fr->pccd_ready[side] = 1;
    }
  }
  fr->team_selected = 1;
}

static int frontend_native_team_scroll(int index) {
  int scroll = index - FIFA96_MENU_TEAM_VISIBLE / 2;
  if (scroll < 0) scroll = 0;
  if (scroll > FIFA96_MENU_TEAM_COUNT - FIFA96_MENU_TEAM_VISIBLE)
    scroll = FIFA96_MENU_TEAM_COUNT - FIFA96_MENU_TEAM_VISIBLE;
  return scroll;
}

/* P6: an option row's value, preferring the native choice LANG string and
 * falling back to the compiled-in table. Half length uses the native
 * {2,4,6,8,10,20,45,0} minute table and the port-added "min" suffix. */
static void frontend_native_option_value(const struct fifa96_frontend_run *fr, int row,
                                         char *buf, size_t cap) {
  if (buf == NULL || cap == 0) return;
  buf[0] = '\0';
  int setting = fifa96_menu_model_option_setting(row);
  if (setting < 0) return;
  const struct fifa96_menu_option_desc *desc = fifa96_menu_option_descriptor(setting);
  if (desc == NULL) return;
  int32_t value = fr->menu.settings.value[setting];
  if (desc->is_slider) {
    snprintf(buf, cap, "%d", (int)value);
    return;
  }
  if (setting == 0x0E) {
    if (value >= 0 && value < (int32_t)FIFA96_MATCH_CONFIG_HALF_LENGTH_COUNT)
      snprintf(buf, cap, "%d min", (int)fifa96_match_config_half_lengths[value]);
    else
      snprintf(buf, cap, "?");
    return;
  }
  uint32_t choice = fifa96_menu_option_choice_id(setting, value);
  const char *text = choice != 0 ? fifa96_lang_string(&fr->lang, choice) : NULL;
  if (text == NULL) text = fifa96_menu_option_choice_text(setting, value);
  snprintf(buf, cap, "%s", text != NULL ? text : "?");
}

static const char *frontend_native_option_label(const struct fifa96_frontend_run *fr, int row) {
  const char *fallback = fifa96_menu_model_option_label(row);
  uint32_t id = fifa96_menu_model_option_label_id(row);
  return id != 0 ? frontend_native_string(fr, id, fallback) : fallback;
}

static const char *frontend_native_team_label(const struct fifa96_frontend_run *fr,
                                              int index) {
  if (index < 0 || index >= FIFA96_MENU_TEAM_COUNT) return "?";
  return frontend_native_string(fr, fifa96_menu_team_id(index),
                                fifa96_menu_team_fallback(index));
}

static const char *frontend_native_competition_team(
    const struct fifa96_frontend_run *fr, uint8_t slot) {
  if (slot >= fr->competition.team_count) return "?";
  return frontend_native_team_label(fr, fr->competition.teams[slot]);
}

/* Derived 3/1/0 points for the table view; the competition module owns the
 * authoritative standings order, this only formats a row. */
static int frontend_native_competition_points(const struct fifa96_competition *comp,
                                              uint8_t slot) {
  int points = 0;
  for (uint8_t i = 0; i < comp->fixture_count; i++) {
    const struct fifa96_competition_fixture *f = fifa96_competition_fixture(comp, i);
    if (f == NULL || f->played == 0) continue;
    if (f->home == slot) {
      if (f->home_goals > f->away_goals) points += 3;
      else if (f->home_goals == f->away_goals) points += 1;
    } else if (f->away == slot) {
      if (f->away_goals > f->home_goals) points += 3;
      else if (f->away_goals == f->home_goals) points += 1;
    }
  }
  return points;
}

/* Number of rows the read-only table/bracket screen will contain. */
static int frontend_native_list_total(const struct fifa96_frontend_run *fr) {
  const struct fifa96_competition *comp = &fr->competition;
  if (comp->kind == FIFA96_COMPETITION_LEAGUE) return comp->team_count;
  if (comp->kind == FIFA96_COMPETITION_TOURNAMENT ||
      comp->kind == FIFA96_COMPETITION_PLAYOFF)
    return comp->fixture_count;
  return 0;
}

static void frontend_native_competition_help(const struct fifa96_frontend_run *fr,
                                             struct fifa96_menu_state *st) {
  const struct fifa96_competition *comp = &fr->competition;
  st->list_help_text[0] = '\0';
  if (fifa96_competition_summary(comp, st->list_help_text,
                                 sizeof st->list_help_text) != 0)
    snprintf(st->list_help_text, sizeof st->list_help_text, "%s",
             fifa96_competition_kind_name(comp->kind));
  if (comp->state == FIFA96_COMPETITION_COMPLETE &&
      comp->champion != FIFA96_COMPETITION_NO_TEAM &&
      comp->champion < comp->team_count) {
    size_t len = strlen(st->list_help_text);
    if (len + 16u < sizeof st->list_help_text)
      snprintf(st->list_help_text + len, sizeof st->list_help_text - len,
               "  Champion: %s", frontend_native_competition_team(fr, comp->champion));
  } else if (comp->eliminated != 0) {
    size_t len = strlen(st->list_help_text);
    if (len + 12u < sizeof st->list_help_text)
      snprintf(st->list_help_text + len, sizeof st->list_help_text - len,
               "  Eliminated");
  }
  st->list_help = st->list_help_text;
}

static void frontend_native_list_scroll(const struct fifa96_menu_model *model,
                                        int rows, int *scroll) {
  int first = model->list_scroll;
  if (first < 0) first = 0;
  int max_scroll = rows - FIFA96_MENU_LIST_VISIBLE;
  if (max_scroll < 0) max_scroll = 0;
  if (first > max_scroll) first = max_scroll;
  *scroll = first;
}

static void frontend_native_menu_state_fill(const struct fifa96_frontend_run *fr,
                                            struct fifa96_menu_state *st) {
  memset(st, 0, sizeof *st);
  st->screen = fr->menu.screen;
  st->font = fifa96_lang_font(&fr->lang);
  st->cursor_on = (fr->frames & 1) != 0;
  st->selected_item = fr->menu.item;
  st->team_col = fr->menu.team_col;

  switch (fr->menu.screen) {
    case FIFA96_MENU_SCREEN_GAME_SELECT: {
      const struct fifa96_menu_item_desc *items = fifa96_menu_model_items();
      st->title = frontend_native_string(fr, 0x84u, "Game Select");
      st->footer = "Enter: select";
      st->item_count = FIFA96_MENU_ITEM_COUNT;
      for (int i = 0; i < FIFA96_MENU_ITEM_COUNT; i++) {
        st->items[i].label =
            frontend_native_string(fr, items[i].label_id, items[i].fallback);
        st->items[i].col = items[i].col;
        st->items[i].row = items[i].row;
        /* P13: every Game Select entry now opens a derived flow (Modem Setup
         * opens the honest offline notice), so none is dimmed. */
        st->items[i].enabled = 1;
      }
      break;
    }
    case FIFA96_MENU_SCREEN_TEAM_SELECT:
      switch (fr->menu.pending_kind) {
        case FIFA96_COMPETITION_LEAGUE: st->title = "LEAGUE SELECT"; break;
        case FIFA96_COMPETITION_TOURNAMENT: st->title = "TOURNAMENT"; break;
        case FIFA96_COMPETITION_PLAYOFF: st->title = "PLAYOFFS"; break;
        case FIFA96_COMPETITION_PRACTISE: st->title = "PRACTISE"; break;
        default:
          /* The native Team Select screen is titled FRIENDLY for the friendly
           * flow; the LANG id 0x13D is the generic screen label. */
          st->title = "FRIENDLY";
          break;
      }
      st->footer = "Enter: play   X: back";
      st->column_label[0] = frontend_native_string(fr, 0x98u, "Home");
      st->team_locked = fr->menu.pending_kind != 0;
      if (!st->team_locked)
        st->column_label[1] = frontend_native_string(fr, 0x14Au, "Visitor");
      st->team_rows = FIFA96_MENU_TEAM_VISIBLE;
      st->team_selected_row[0] = -1;
      st->team_selected_row[1] = -1;
      for (int side = 0; side < 2; side++) {
        if (st->team_locked && side == 1) continue;
        int scroll = frontend_native_team_scroll(fr->menu.team[side]);
        for (int row = 0; row < FIFA96_MENU_TEAM_VISIBLE; row++) {
          int index = scroll + row;
          st->teams[side][row] =
              frontend_native_string(fr, fifa96_menu_team_id(index),
                                     fifa96_menu_team_fallback(index));
          if (index == fr->menu.team[side]) st->team_selected_row[side] = row;
        }
      }
      break;
    case FIFA96_MENU_SCREEN_OPTIONS:
      st->title = frontend_native_string(fr, 0xD8u, "Options");
      st->option_page = fr->menu.option_page;
      if (fr->menu.option_page != FIFA96_MENU_OPTION_PAGE_LIST) {
        size_t count = 0;
        const char *const *help = fifa96_menu_model_controls_text(&count);
        if (count > FIFA96_MENU_OPTION_HELP_MAX) count = FIFA96_MENU_OPTION_HELP_MAX;
        st->help_count = (int)count;
        for (size_t i = 0; i < count; i++) st->help[i] = help[i];
        st->footer = "X: back";
      } else {
        st->option_count = fifa96_menu_model_option_rows();
        st->option_selected = fr->menu.option_row;
        st->option_scroll = fr->menu.option_scroll;
        st->footer = "Arrows: change  Enter: select  X: back";
        for (int k = 0; k < FIFA96_MENU_OPTION_VISIBLE; k++) {
          int row = st->option_scroll + k;
          if (row >= st->option_count) break;
          st->options[k].label = frontend_native_option_label(fr, row);
          frontend_native_option_value(fr, row, st->option_value_text[k],
                                       sizeof st->option_value_text[k]);
          st->options[k].value = st->option_value_text[k];
          st->options[k].enabled =
              fifa96_menu_model_option_live(row) ||
              row == FIFA96_MENU_OPTION_ROW_CONTROLS ||
              row == FIFA96_MENU_OPTION_ROW_RESET;
          int setting = fifa96_menu_model_option_setting(row);
          st->options[k].slider_pct =
              (setting >= 0 && fifa96_menu_model_option_slider(row))
                  ? (int)fr->menu.settings.value[setting]
                  : -1;
        }
      }
      break;
    case FIFA96_MENU_SCREEN_COMPETITION_HUB: {
      const struct fifa96_competition *comp = &fr->competition;
      const char *kind = fifa96_competition_kind_name(comp->kind);
      st->title = kind;
      st->list_title = kind;
      st->list_footer = "Enter: select   X: back";
      st->footer = st->list_footer;
      st->list_row_count = 5;
      st->list_selected = fr->menu.list_row;
      st->list_scroll = 0;
      int active = comp->state == FIFA96_COMPETITION_ACTIVE;
      static const char *const labels[5] = {
          "Play next", "Simulate next", "Table / Bracket", "Save game", "Exit"};
      for (int i = 0; i < 5; i++) {
        st->list_rows[i].label = labels[i];
        if (i == 0 || i == 1) st->list_rows[i].enabled = active;
        else if (i == 2)
          st->list_rows[i].enabled =
              comp->kind == FIFA96_COMPETITION_LEAGUE ||
              comp->kind == FIFA96_COMPETITION_TOURNAMENT ||
              comp->kind == FIFA96_COMPETITION_PLAYOFF;
        else st->list_rows[i].enabled = 1;
      }
      frontend_native_competition_help(fr, st);
      break;
    }
    case FIFA96_MENU_SCREEN_LOAD_GAME:
    case FIFA96_MENU_SCREEN_SAVE_GAME: {
      int save = fr->menu.screen == FIFA96_MENU_SCREEN_SAVE_GAME;
      st->title = save ? "Save Game" : "Load Game";
      st->list_title = st->title;
      st->list_footer = save ? "Enter: save   X: back" : "Enter: load   X: back";
      st->footer = st->list_footer;
      st->list_row_count = FIFA96_MENU_SAVE_SLOTS;
      st->list_selected = fr->menu.list_row;
      int scroll = 0;
      frontend_native_list_scroll(&fr->menu, FIFA96_MENU_SAVE_SLOTS, &scroll);
      st->list_scroll = scroll;
      for (int k = 0; k < FIFA96_MENU_LIST_VISIBLE; k++) {
        int slot = scroll + k;
        if (slot >= FIFA96_MENU_SAVE_SLOTS) break;
        char label[FIFA96_SAVE_SUMMARY_MAX];
        (void)fifa96_save_slot_label(fr->save_dir, slot, label, sizeof label);
        snprintf(st->list_text[k], sizeof st->list_text[k], "%s", label);
        st->list_rows[k].label = st->list_text[k];
        st->list_rows[k].enabled =
            save || strcmp(label, "Empty") != 0;
      }
      snprintf(st->list_help_text, sizeof st->list_help_text, "Slot %d of %d",
               fr->menu.list_row + 1, FIFA96_MENU_SAVE_SLOTS);
      st->list_help = st->list_help_text;
      break;
    }
    case FIFA96_MENU_SCREEN_MODEM:
      st->title = "Modem Setup";
      st->list_title = "Modem Setup";
      st->list_footer = "X: back";
      st->footer = st->list_footer;
      st->list_row_count = 2;
      st->list_selected = fr->menu.list_row;
      st->list_scroll = 0;
      st->list_text[0][0] = '\0';
      snprintf(st->list_text[0], sizeof st->list_text[0], "%s",
               "Modem/network play is not available in this SDL port");
      st->list_rows[0].label = st->list_text[0];
      st->list_rows[0].enabled = 0;
      snprintf(st->list_text[1], sizeof st->list_text[1], "%s", "Back");
      st->list_rows[1].label = st->list_text[1];
      st->list_rows[1].enabled = 1;
      snprintf(st->list_help_text, sizeof st->list_help_text, "%s",
               "Derived offline notice");
      st->list_help = st->list_help_text;
      break;
    case FIFA96_MENU_SCREEN_LIST: {
      const struct fifa96_competition *comp = &fr->competition;
      int total = frontend_native_list_total(fr);
      if (total < 1) total = 1;
      int scroll = 0;
      frontend_native_list_scroll(&fr->menu, total, &scroll);
      st->title = comp->kind == FIFA96_COMPETITION_LEAGUE ? "League Table"
                                                          : "Bracket";
      st->list_title = st->title;
      st->list_footer = "X: back";
      st->footer = st->list_footer;
      st->list_row_count = total;
      st->list_selected = fr->menu.list_row;
      st->list_scroll = scroll;
      if (comp->kind == FIFA96_COMPETITION_LEAGUE) {
        uint8_t order[FIFA96_COMPETITION_TEAMS];
        int count = fifa96_competition_standings(comp, order);
        if (count < 1) count = total;
        for (int k = 0; k < FIFA96_MENU_LIST_VISIBLE; k++) {
          int row = scroll + k;
          if (row >= count) break;
          uint8_t slot = order[row];
          snprintf(st->list_text[k], sizeof st->list_text[k], "%d. %s  %d pts",
                   row + 1, frontend_native_competition_team(fr, slot),
                   frontend_native_competition_points(comp, slot));
          st->list_rows[k].label = st->list_text[k];
          st->list_rows[k].enabled = 1;
        }
      } else {
        for (int k = 0; k < FIFA96_MENU_LIST_VISIBLE; k++) {
          int row = scroll + k;
          if (row >= total) break;
          const struct fifa96_competition_fixture *f =
              fifa96_competition_fixture(comp, (uint8_t)row);
          if (f == NULL) {
            snprintf(st->list_text[k], sizeof st->list_text[k], "%s",
                     "No fixture");
          } else if (f->played != 0) {
            snprintf(st->list_text[k], sizeof st->list_text[k],
                     "R%d %s %d-%d %s%s%s", (int)f->round + 1,
                     frontend_native_competition_team(fr, f->home),
                     (int)f->home_goals, (int)f->away_goals,
                     frontend_native_competition_team(fr, f->away),
                     f->shootout != 0 ? " SO" : "",
                     f->replay != 0 ? " R" : "");
          } else {
            snprintf(st->list_text[k], sizeof st->list_text[k], "R%d %s v %s",
                     (int)f->round + 1,
                     frontend_native_competition_team(fr, f->home),
                     frontend_native_competition_team(fr, f->away));
          }
          st->list_rows[k].label = st->list_text[k];
          st->list_rows[k].enabled = 1;
        }
      }
      frontend_native_competition_help(fr, st);
      break;
    }
    default:
      break;
  }
}

void fifa96_frontend_run_menu_state(const struct fifa96_frontend_run *fr,
                                    struct fifa96_menu_state *st) {
  if (fr == NULL || st == NULL) return;
  frontend_native_menu_state_fill(fr, st);
}

/* P13: clear the bridge handoff without touching the menu model's selected
 * teams or the preserved competition/settings blocks. */
static void frontend_native_clear_handoff(struct fifa96_frontend_run *fr) {
  fr->match_start = 0;
  fr->team_selected = 0;
  fr->team_id[0] = 0;
  fr->team_id[1] = 0;
  memset(fr->team_name, 0, sizeof fr->team_name);
  memset(fr->team_code, 0, sizeof fr->team_code);
  memset(fr->pccd_index, 0, sizeof fr->pccd_index);
  memset(fr->pccd_record, 0, sizeof fr->pccd_record);
  memset(fr->pccd_id, 0, sizeof fr->pccd_id);
  memset(fr->formation, 0, sizeof fr->formation);
  memset(fr->kit, 0, sizeof fr->kit);
  memset(fr->kit_print, 0, sizeof fr->kit_print);
  memset(fr->roster, 0, sizeof fr->roster);
  memset(fr->pccd_ready, 0, sizeof fr->pccd_ready);
}

static void frontend_native_auto_save(struct fifa96_frontend_run *fr) {
  /* Soft failure: a read-only/default directory must never trap the hub. */
  (void)fifa96_save_write(fr->save_dir, 0, &fr->competition, &fr->menu.settings);
}

/* Keep the model's two cells aligned with the derived fixture so a state
 * snapshot (and any non-fixture fallback) shows the same opponent. */
static void frontend_native_menu_teams_from_fixture(struct fifa96_frontend_run *fr) {
  if (fr->mode == FIFA96_COMPETITION_PRACTISE) {
    fr->menu.team[1] = 4;
    return;
  }
  if (fr->mode < FIFA96_COMPETITION_LEAGUE ||
      fr->mode > FIFA96_COMPETITION_PLAYOFF)
    return;
  const struct fifa96_competition_fixture *fixture =
      fifa96_competition_fixture(&fr->competition, fr->competition.current);
  if (fixture == NULL || fixture->user == 0 ||
      fixture->home >= fr->competition.team_count ||
      fixture->away >= fr->competition.team_count)
    return;
  fr->menu.team[0] = fr->competition.teams[fixture->home];
  fr->menu.team[1] = fr->competition.teams[fixture->away];
}

static int frontend_native_start_competition(struct fifa96_frontend_run *fr) {
  uint8_t kind = fr->menu.pending_kind;
  if (kind < FIFA96_COMPETITION_LEAGUE || kind > FIFA96_COMPETITION_PRACTISE)
    return 0;
  uint8_t pool[FIFA96_COMPETITION_TEAMS] = {0, 1, 2, 3, 4, 5, 6, 7};
  uint8_t count = kind == FIFA96_COMPETITION_PRACTISE ? 1u
                                                      : FIFA96_COMPETITION_TEAMS;
  int selected = fr->menu.team[0];
  if (selected < 0 || selected >= FIFA96_MENU_TEAM_COUNT) selected = 0;
  /* The model stores pool[user_team]; keep the user in its selected slot (and
   * start out as home) by placing the chosen team into the default pool. */
  uint8_t user_slot = (uint8_t)(selected % count);
  pool[user_slot] = (uint8_t)selected;
  if (fifa96_competition_start(&fr->competition, kind, user_slot, pool, count,
                               0x96u) != 0)
    return 0;
  fr->mode = kind;
  fr->menu.pending_kind = kind;
  if (kind == FIFA96_COMPETITION_PRACTISE) {
    fr->menu.team[1] = 4;   /* fixed derived Germany opponent */
  } else {
    uint8_t fixture_index = 0, home = 0, away = 0;
    if (fifa96_competition_next_fixture(&fr->competition, &fixture_index, &home,
                                        &away) != 1)
      return 0;
  }
  frontend_native_menu_teams_from_fixture(fr);
  frontend_native_clear_handoff(fr);
  fr->match_start = 1;
  return 1;
}

static void frontend_native_hub_play(struct fifa96_frontend_run *fr) {
  if (fr->competition.state != FIFA96_COMPETITION_ACTIVE) return;
  uint8_t fixture_index = 0, home = 0, away = 0;
  if (fifa96_competition_next_fixture(&fr->competition, &fixture_index, &home,
                                      &away) != 1)
    return;
  fr->mode = fr->competition.kind;
  fr->menu.pending_kind = fr->mode;
  frontend_native_menu_teams_from_fixture(fr);
  frontend_native_clear_handoff(fr);
  fr->match_start = 1;
}

static void frontend_native_handle_action(struct fifa96_frontend_run *fr,
                                          enum fifa96_menu_action action) {
  switch (action) {
    case FIFA96_MENU_ACTION_START_MATCH:
      if (fr->menu.pending_kind != 0) {
        (void)frontend_native_start_competition(fr);
      } else {
        fr->mode = 0;   /* Friendly: exactly the pre-P13 handoff */
        fr->match_start = 1;
      }
      break;
    case FIFA96_MENU_ACTION_HUB_PLAY:
      frontend_native_hub_play(fr);
      break;
    case FIFA96_MENU_ACTION_HUB_SIMULATE:
      if (fr->competition.state == FIFA96_COMPETITION_ACTIVE &&
          fifa96_competition_simulate_next(&fr->competition) == 1)
        frontend_native_auto_save(fr);
      break;
    case FIFA96_MENU_ACTION_HUB_TABLE:
      fr->menu.list_count = frontend_native_list_total(fr);
      if (fr->menu.list_count < 1) fr->menu.list_count = 1;
      break;
    case FIFA96_MENU_ACTION_SAVE_SLOT:
      if (fr->menu.list_row >= 0 &&
          fr->menu.list_row < FIFA96_MENU_SAVE_SLOTS &&
          fr->competition.kind != FIFA96_COMPETITION_NONE)
        (void)fifa96_save_write(fr->save_dir, fr->menu.list_row,
                                &fr->competition, &fr->menu.settings);
      fr->menu.screen = FIFA96_MENU_SCREEN_COMPETITION_HUB;
      fr->menu.list_row = 0;
      fr->menu.list_scroll = 0;
      break;
    case FIFA96_MENU_ACTION_LOAD_SLOT: {
      struct fifa96_competition loaded;
      struct fifa96_settings settings;
      if (fr->menu.list_row < 0 ||
          fr->menu.list_row >= FIFA96_MENU_SAVE_SLOTS ||
          fifa96_save_read(fr->save_dir, fr->menu.list_row, &loaded,
                           &settings) != 0)
        break;
      fr->competition = loaded;
      fr->menu.settings = settings;
      fr->mode = loaded.kind;
      fr->menu.pending_kind = loaded.kind;
      frontend_native_menu_teams_from_fixture(fr);
      frontend_native_clear_handoff(fr);
      fr->menu.list_row = 0;
      fr->menu.list_scroll = 0;
      if (loaded.kind == FIFA96_COMPETITION_PRACTISE ||
          loaded.state == FIFA96_COMPETITION_COMPLETE)
        fr->menu.screen = FIFA96_MENU_SCREEN_GAME_SELECT;
      else
        fr->menu.screen = FIFA96_MENU_SCREEN_COMPETITION_HUB;
      break;
    }
    case FIFA96_MENU_ACTION_HUB_EXIT:
      fr->menu.pending_kind = 0;
      break;
    default:
      break;
  }
}

/* P2 native input path: the same mapped queue drives the model. The library
 * state machine is not stepped, so its phase stays FRONTEND; the engine's
 * bridge consumes `match_start` exactly as it does for the panel path. */
static int frontend_native_step(struct fifa96_frontend_run *fr, struct fifa96_surface *s,
                                int *quit) {
  for (int i = 0; i < fr->queue_len; i++) {
    int32_t code = fr->queue[i];
    if (code >= 0 && code <= 3) {
      (void)fifa96_menu_model_nav(&fr->menu, code);
    } else if (code == -10) {
      enum fifa96_menu_action action = fifa96_menu_model_activate(&fr->menu);
      if (action == FIFA96_MENU_ACTION_QUIT) fr->quit_requested = 1;
      else frontend_native_handle_action(fr, action);
    } else if (code == 8) {
      (void)fifa96_menu_model_back(&fr->menu);
    }
  }
  fr->queue_len = 0;

  frontend_native_sync_teams(fr);
  fr->phase = fr->frontend.phase;
  struct fifa96_menu_state menu = {0};
  fifa96_frontend_run_menu_state(fr, &menu);
  fifa96_menu_art_draw(s, &menu);
  fr->frames++;
  *quit = fr->quit_requested ? 1 : 0;
  return 0;
}

int fifa96_frontend_run_init(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  if (!fr || !s) return -1;
  memset(fr, 0, sizeof *fr);
  fifa96_frontend_init(&fr->frontend);
  fifa96_menu_model_init(&fr->menu);
  fifa96_lang_shutdown(&fr->lang);
  fifa96_menu_art_init(NULL);
  fr->surface = s;
  fr->phase = fr->frontend.phase;
  snprintf(fr->save_dir, sizeof fr->save_dir, "%s", ".");
  fifa96_competition_init(&fr->competition);
  /* F1: a fresh run is armed for normal first use. The engine's native boot
   * disarms again when it enables the native menu; the legacy path ignores
   * the latch entirely. */
  fr->input_armed = 1;
  uint32_t state = 0;
  if (fifa96_frontend_entry_state(&fr->frontend, &state) == 1) fr->entry_state = state;
  return 0;
}

int fifa96_frontend_run_reset(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  if (!fr || !s) return -1;
  fifa96_frontend_init(&fr->frontend);
  /* P6: keep the Options settings across the post-match return (the native
   * driver keeps the 0x149278 block); only the screen is re-seeded. */
  fifa96_menu_model_reset(&fr->menu);
  fr->surface = s;
  fr->phase = fr->frontend.phase;
  fr->queue_len = 0;
  memset(fr->queue, 0, sizeof fr->queue);
  /* F1: a match ends with the result key possibly still held; ignore it (and
   * any other held key) until a neutral poll and a fresh press. */
  fr->input_armed = 0;
  memset(fr->key_down, 0, sizeof fr->key_down);
  fr->frames = 0;
  fr->quit_requested = 0;
  fr->match_start = 0;
  fr->team_id[0] = 0;
  fr->team_id[1] = 0;
  memset(fr->team_name, 0, sizeof fr->team_name);
  memset(fr->team_code, 0, sizeof fr->team_code);
  memset(fr->pccd_index, 0, sizeof fr->pccd_index);
  memset(fr->pccd_record, 0, sizeof fr->pccd_record);
  memset(fr->pccd_id, 0, sizeof fr->pccd_id);
  memset(fr->formation, 0, sizeof fr->formation);
  memset(fr->kit, 0, sizeof fr->kit);
  memset(fr->kit_print, 0, sizeof fr->kit_print);
  memset(fr->roster, 0, sizeof fr->roster);
  memset(fr->pccd_ready, 0, sizeof fr->pccd_ready);
  fr->team_selected = 0;
  fr->selected_row = 0;
  fr->entry_state = 0;
  uint32_t state = 0;
  if (fifa96_frontend_entry_state(&fr->frontend, &state) == 1) fr->entry_state = state;
  /* P13: competition, mode and save_dir survive the post-match return (like
   * the Options block). A live/completed league or cup re-enters the hub;
   * Practise and a zero/IDLE competition keep the historical Game Select. */
  if (fr->save_dir[0] == '\0')
    snprintf(fr->save_dir, sizeof fr->save_dir, "%s", ".");
  if (fr->mode >= FIFA96_COMPETITION_LEAGUE &&
      fr->mode <= FIFA96_COMPETITION_PLAYOFF &&
      (fr->competition.state == FIFA96_COMPETITION_ACTIVE ||
       fr->competition.state == FIFA96_COMPETITION_COMPLETE)) {
    fr->menu.screen = FIFA96_MENU_SCREEN_COMPETITION_HUB;
    fr->menu.pending_kind = fr->mode;
    fr->menu.list_row = 0;
    fr->menu.list_scroll = 0;
    fr->menu.list_count = 5;
  }
  return 0;
}

void fifa96_frontend_run_set_save_dir(struct fifa96_frontend_run *fr, const char *dir) {
  if (fr == NULL) return;
  if (dir == NULL || dir[0] == '\0') dir = ".";
  snprintf(fr->save_dir, sizeof fr->save_dir, "%s", dir);
}

int fifa96_frontend_run_match_result(struct fifa96_frontend_run *fr,
                                     uint16_t home_goals, uint16_t away_goals) {
  if (fr == NULL) return -1;
  if (fr->mode >= FIFA96_COMPETITION_LEAGUE &&
      fr->mode <= FIFA96_COMPETITION_PLAYOFF &&
      (fr->competition.state == FIFA96_COMPETITION_ACTIVE ||
       fr->competition.state == FIFA96_COMPETITION_COMPLETE)) {
    if (fr->competition.state == FIFA96_COMPETITION_ACTIVE) {
      uint8_t home = home_goals > 255u ? 255u : (uint8_t)home_goals;
      uint8_t away = away_goals > 255u ? 255u : (uint8_t)away_goals;
      if (fifa96_competition_apply_result(&fr->competition,
                                          fr->competition.current, home,
                                          away) != 0)
        return -1;
      frontend_native_auto_save(fr);
    }
    frontend_native_clear_handoff(fr);
    fr->menu.screen = FIFA96_MENU_SCREEN_COMPETITION_HUB;
    fr->menu.pending_kind = fr->mode;
    fr->menu.list_row = 0;
    fr->menu.list_scroll = 0;
    fr->menu.list_count = 5;
    return 0;
  }
  /* Friendly and Practise are no-stakes: do not touch a saved competition for
   * Friendly, and drop the Practise descriptor so reset returns to Game
   * Select. */
  if (fr->mode == FIFA96_COMPETITION_PRACTISE)
    fifa96_competition_init(&fr->competition);
  frontend_native_clear_handoff(fr);
  fr->menu.screen = FIFA96_MENU_SCREEN_GAME_SELECT;
  fr->menu.pending_kind = 0;
  fr->menu.list_row = 0;
  fr->menu.list_scroll = 0;
  fr->menu.list_count = 0;
  return 0;
}

void fifa96_frontend_run_enable_native(struct fifa96_frontend_run *fr, int enable) {
  if (!fr) return;
  fr->native_menu = enable != 0;
  if (fr->native_menu) {
    fr->input_armed = 0;
    memset(fr->key_down, 0, sizeof fr->key_down);
  }
}

void fifa96_frontend_run_disarm_input(struct fifa96_frontend_run *fr) {
  if (!fr) return;
  fr->input_armed = 0;
  memset(fr->key_down, 0, sizeof fr->key_down);
}

/* F1 native input edge latch. The SDL3 backend re-presents every physically
 * held key with state == 1 on every 10 ms poll, so a raw press event is not an
 * edge by itself. While disarmed, held keys are only recorded and a poll with
 * nothing down re-arms the latch; once armed, a key queues only on its 0 -> 1
 * transition and a held key never repeats. The legacy path below keeps the
 * original press-event behavior so the M1/M2 tapes stay byte-identical. */
static int frontend_run_native_input(struct fifa96_frontend_run *fr,
                                     const fifa96_platform_key *keys, size_t count) {
  if (!fr->input_armed) {
    for (size_t i = 0; i < count; i++) {
      int32_t raw = keys[i].raw_code;
      if (raw <= 0 || (size_t)raw >= sizeof fr->key_down) continue;
      if (keys[i].state == 1) fr->key_down[raw] = 1;
      else if (keys[i].state == 0) fr->key_down[raw] = 0;
    }
    int any_down = 0;
    for (size_t k = 0; k < sizeof fr->key_down; k++) {
      if (fr->key_down[k] != 0) {
        any_down = 1;
        break;
      }
    }
    if (!any_down) fr->input_armed = 1;
    return 0;
  }
  for (size_t i = 0; i < count; i++) {
    int32_t raw = keys[i].raw_code;
    if (raw <= 0 || (size_t)raw >= sizeof fr->key_down) continue;
    if (keys[i].state == 0) {
      fr->key_down[raw] = 0;
      continue;
    }
    if (keys[i].state != 1 || fr->key_down[raw] != 0) continue;
    fr->key_down[raw] = 1;
    if (raw == FIFA96_ENGINE_KEY_QUIT) {
      fr->quit_requested = 1;
      continue;
    }
    int code = 0;
    if (!frontend_run_map_key(raw, &code)) continue;
    if (fr->queue_len < FRONTEND_RUN_QUEUE_CAP) fr->queue[fr->queue_len++] = code;
  }
  return 0;
}

int fifa96_frontend_run_input(struct fifa96_frontend_run *fr, const fifa96_platform_key *keys,
                              size_t count) {
  if (!fr || (keys == NULL && count != 0)) return -1;
  if (fr->native_menu) return frontend_run_native_input(fr, keys, count);
  for (size_t i = 0; i < count; i++) {
    if (keys[i].state != 1) continue;   /* press events only for now */
    if (keys[i].raw_code == FIFA96_ENGINE_KEY_QUIT) {
      fr->quit_requested = 1;
      continue;
    }
    int code = 0;
    if (!frontend_run_map_key(keys[i].raw_code, &code)) continue;
    if (fr->queue_len < FRONTEND_RUN_QUEUE_CAP) fr->queue[fr->queue_len++] = code;
  }
  return 0;
}

int fifa96_frontend_run_step(struct fifa96_frontend_run *fr, struct fifa96_surface *s, int *quit) {
  if (!fr || !s || !quit) return -1;
  *quit = 0;
  if (fr->native_menu) return frontend_native_step(fr, s, quit);

  int library_exit = 0;

  /* A front-end accepted CONFIRM (the gated -10 path) parks the library in
   * EXIT and only fifa96_frontend_init clears the confirm flag. The M1
   * wrapping consumes it and returns the front-end to its navigable phase on
   * the following step; the M1 golden pins that behavior for the front-end
   * menu, so the FU-66 front-end confirm route stays an open leg until the
   * boot mode selector is modeled. A panel confirm is consumed here on the
   * step after its match-start classification was recorded below. */
  if (fr->frontend.confirm != 0) {
    fr->frontend.confirm = 0;
    fifa96_frontend_panel_result(&fr->frontend);
  }

  if (fifa96_frontend_driver(&fr->frontend) != 0) return -1;
  for (int i = 0; i < fr->queue_len; i++) {
    int32_t code = fr->queue[i];
    if (fr->frontend.phase == FIFA96_FRONTEND_PHASE_PANEL) {
      enum fifa96_frontend_panel_event event = FIFA96_FRONTEND_PANEL_NONE;
      int32_t mapped = code;
      if (fifa96_frontend_panel_event(&fr->frontend, code, 1, 1, &event, &mapped) != 0) continue;
      if (event == FIFA96_FRONTEND_PANEL_EXIT) {
        fifa96_frontend_panel_result(&fr->frontend);
      } else if (event == FIFA96_FRONTEND_PANEL_CONFIRM) {
        /* FU-66 §5: the panel's gated -10 (0x1F994 -> r=5) sets [0x5094],
         * and FU-66 §2 turns the confirm flag into the driver exit. Classify
         * that transition through the FU-66 §4 code-8 exit tail (0x1EDFD,
         * `MOV EAX,0x10`): the port's exit state for it is STATE16, the
         * classification the FU-64 §1.1 menu-driven start-match family
         * starts from. The bridge consumes this flag. The original confirm
         * path exits without a state dispatch (state 16 on the panel path is
         * the non-confirm 4/5 tail), so this is the port's startable
         * stand-in per C1-OL3. */
        enum fifa96_frontend_exit exit_state = FIFA96_FRONTEND_EXIT_STATE16;
        uint32_t state = fr->entry_state;
        if (fifa96_frontend_exit(&fr->frontend, 8, &exit_state, &state) == 0 &&
            exit_state == FIFA96_FRONTEND_EXIT_STATE16) {
          fr->match_start = 1;
        }
      }
    } else if (fr->frontend.phase == FIFA96_FRONTEND_PHASE_EXIT) {
      enum fifa96_frontend_exit exit_state = FIFA96_FRONTEND_EXIT_STATE16;
      uint32_t state = fr->entry_state;
      if (fifa96_frontend_exit(&fr->frontend, code, &exit_state, &state) == 0) {
        fr->entry_state = state;
        if (exit_state == FIFA96_FRONTEND_EXIT_SETTINGS ||
            exit_state == FIFA96_FRONTEND_EXIT_SETTINGS_ALT) {
          library_exit = 1;
        }
      }
    } else {
      enum fifa96_frontend_event event = FIFA96_FRONTEND_EVENT_NONE;
      int32_t mapped = code;
      /* M1 gate: the per-state prompt is an open leg, so the constant gate
       * lets -10 (CONFIRM's action code) be accepted as a real CONFIRM. */
      if (fifa96_frontend_event(&fr->frontend, code, 1, &event, &mapped) != 0) continue;
      if (event == FIFA96_FRONTEND_EVENT_MENU) {
        if (mapped == 0) {
          fr->selected_row =
              (fr->selected_row + FIFA96_MENU_VISIBLE_ROWS - 1) % FIFA96_MENU_VISIBLE_ROWS;
        } else if (mapped == 1) {
          fr->selected_row = (fr->selected_row + 1) % FIFA96_MENU_VISIBLE_ROWS;
        }
      }
      if (event == FIFA96_FRONTEND_EVENT_EXIT ||
          event == FIFA96_FRONTEND_EVENT_SETTINGS ||
          event == FIFA96_FRONTEND_EVENT_SETTINGS_ALT) {
        fifa96_frontend_frontend_result(&fr->frontend, mapped);
      }
      if (event == FIFA96_FRONTEND_EVENT_SETTINGS ||
          event == FIFA96_FRONTEND_EVENT_SETTINGS_ALT) {
        library_exit = 1;
      }
    }
  }
  fr->queue_len = 0;

  /* A confirm accepted by a panel event moves the driver to EXIT this step. */
  if (fifa96_frontend_driver(&fr->frontend) != 0) return -1;

  fr->phase = fr->frontend.phase;
  if (fr->frontend.phase != FIFA96_FRONTEND_PHASE_EXIT) {
    uint32_t state = fr->entry_state;
    if (fifa96_frontend_entry_state(&fr->frontend, &state) == 1) fr->entry_state = state;
  }
  /* Only the QUIT key or a settings exit leaves the engine; an accepted
   * CONFIRM is consumed at the top of the following step (see above). */
  if (fr->quit_requested || library_exit) *quit = 1;

  struct fifa96_menu_state menu = {0};
  menu.entry_state = fr->entry_state;
  menu.selected_row = fr->selected_row;
  menu.cursor_on = (fr->frames & 1) != 0;
  fifa96_menu_art_draw(s, &menu);
  fr->frames++;
  return 0;
}
