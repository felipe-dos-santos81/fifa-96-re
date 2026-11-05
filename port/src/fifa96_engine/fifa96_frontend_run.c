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

/* Snapshot the model's current teams into the bridge payload. The full name is
 * kept for display; the HUD takes the derived 3-letter code (A3 §2.3/P2-L4). */
static void frontend_native_commit_teams(struct fifa96_frontend_run *fr) {
  for (int side = 0; side < 2; side++) {
    int index = fr->menu.team[side];
    fr->team_id[side] = fifa96_menu_team_id(index);
    frontend_copy_name(fr->team_name[side],
                       frontend_native_string(fr, fr->team_id[side],
                                              fifa96_menu_team_fallback(index)));
    const char *code = fifa96_menu_team_code(index);
    memset(fr->team_code[side], 0, sizeof fr->team_code[side]);
    if (code != NULL) {
      size_t len = strlen(code);
      if (len > 3) len = 3;
      memcpy(fr->team_code[side], code, len);
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

static void frontend_native_menu_state(const struct fifa96_frontend_run *fr,
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
        st->items[i].enabled = items[i].action != FIFA96_MENU_ACTION_NONE;
      }
      break;
    }
    case FIFA96_MENU_SCREEN_TEAM_SELECT:
      st->title = frontend_native_string(fr, 0x13Du, "Team Select");
      st->footer = "Enter: play   X: back";
      st->column_label[0] = frontend_native_string(fr, 0x98u, "Home");
      st->column_label[1] = frontend_native_string(fr, 0x14Au, "Visitor");
      st->team_rows = FIFA96_MENU_TEAM_VISIBLE;
      st->team_selected_row[0] = -1;
      st->team_selected_row[1] = -1;
      for (int side = 0; side < 2; side++) {
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
      st->message = "Not implemented in this port.";
      st->footer = "X: back";
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
      if (action == FIFA96_MENU_ACTION_QUIT) {
        fr->quit_requested = 1;
      } else if (action == FIFA96_MENU_ACTION_START_MATCH) {
        frontend_native_commit_teams(fr);
        fr->match_start = 1;
      }
    } else if (code == 8) {
      (void)fifa96_menu_model_back(&fr->menu);
    }
  }
  fr->queue_len = 0;

  frontend_native_commit_teams(fr);
  fr->phase = fr->frontend.phase;
  struct fifa96_menu_state menu = {0};
  frontend_native_menu_state(fr, &menu);
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
  uint32_t state = 0;
  if (fifa96_frontend_entry_state(&fr->frontend, &state) == 1) fr->entry_state = state;
  return 0;
}

int fifa96_frontend_run_reset(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  if (!fr || !s) return -1;
  fifa96_frontend_init(&fr->frontend);
  fifa96_menu_model_init(&fr->menu);
  fr->surface = s;
  fr->phase = fr->frontend.phase;
  fr->queue_len = 0;
  memset(fr->queue, 0, sizeof fr->queue);
  fr->frames = 0;
  fr->quit_requested = 0;
  fr->match_start = 0;
  fr->team_id[0] = 0;
  fr->team_id[1] = 0;
  memset(fr->team_name, 0, sizeof fr->team_name);
  memset(fr->team_code, 0, sizeof fr->team_code);
  memset(fr->formation, 0, sizeof fr->formation);
  fr->team_selected = 0;
  fr->selected_row = 0;
  fr->entry_state = 0;
  uint32_t state = 0;
  if (fifa96_frontend_entry_state(&fr->frontend, &state) == 1) fr->entry_state = state;
  return 0;
}

void fifa96_frontend_run_enable_native(struct fifa96_frontend_run *fr, int enable) {
  if (!fr) return;
  fr->native_menu = enable != 0;
}

int fifa96_frontend_run_input(struct fifa96_frontend_run *fr, const fifa96_platform_key *keys,
                              size_t count) {
  if (!fr || (keys == NULL && count != 0)) return -1;
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
