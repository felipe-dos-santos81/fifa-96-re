#include <string.h>
#include "fifa96_engine/fifa96_frontend_run.h"

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

  if (fifa96_frontend_driver(&fr->frontend) != 0) return -1;
  for (int i = 0; i < fr->queue_len; i++) {
    int32_t code = fr->queue[i];
    if (fr->frontend.phase == FIFA96_FRONTEND_PHASE_PANEL) {
      enum fifa96_frontend_panel_event event = FIFA96_FRONTEND_PANEL_NONE;
      int32_t mapped = code;
      if (fifa96_frontend_panel_event(&fr->frontend, code, 1, 1, &event, &mapped) != 0) continue;
      if (event == FIFA96_FRONTEND_PANEL_EXIT) fifa96_frontend_panel_result(&fr->frontend);
    } else if (fr->frontend.phase == FIFA96_FRONTEND_PHASE_EXIT) {
      enum fifa96_frontend_exit exit_state = FIFA96_FRONTEND_EXIT_STATE16;
      uint32_t state = fr->entry_state;
      if (fifa96_frontend_exit(&fr->frontend, code, &exit_state, &state) == 0) {
        fr->entry_state = state;
      }
    } else {
      enum fifa96_frontend_event event = FIFA96_FRONTEND_EVENT_NONE;
      int32_t mapped = code;
      if (fifa96_frontend_event(&fr->frontend, code, 0, &event, &mapped) != 0) continue;
      if (event == FIFA96_FRONTEND_EVENT_EXIT ||
          event == FIFA96_FRONTEND_EVENT_SETTINGS ||
          event == FIFA96_FRONTEND_EVENT_SETTINGS_ALT) {
        fifa96_frontend_frontend_result(&fr->frontend, mapped);
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
  if (fr->quit_requested || fr->frontend.phase == FIFA96_FRONTEND_PHASE_EXIT) *quit = 1;

  fifa96_surface_clear(s, 0);   /* Task 9 replaces this stub with the menu art */
  fr->frames++;
  return 0;
}
