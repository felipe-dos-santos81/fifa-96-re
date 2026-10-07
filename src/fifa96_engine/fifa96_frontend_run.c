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

int fifa96_frontend_run_init(struct fifa96_frontend_run *fr, struct fifa96_surface *s) {
  if (!fr || !s) return -1;
  memset(fr, 0, sizeof *fr);
  fifa96_frontend_init(&fr->frontend);
  fifa96_menu_art_init(NULL);
  fr->surface = s;
  fr->phase = fr->frontend.phase;
  uint32_t state = 0;
  if (fifa96_frontend_entry_state(&fr->frontend, &state) == 1) fr->entry_state = state;
  return 0;
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
         * starts from. The bridge consumes this flag. */
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

  struct fifa96_menu_state menu = {
      .entry_state = fr->entry_state,
      .selected_row = fr->selected_row,
      .cursor_on = (fr->frames & 1) != 0,
  };
  fifa96_menu_art_draw(s, &menu);
  fr->frames++;
  return 0;
}
