#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_FRONTEND_STATE_COUNT 20

enum fifa96_frontend_phase {
  FIFA96_FRONTEND_PHASE_FRONTEND = 0,
  FIFA96_FRONTEND_PHASE_PANEL = 1,
  FIFA96_FRONTEND_PHASE_EXIT = 2
};

enum fifa96_frontend_event {
  FIFA96_FRONTEND_EVENT_NONE = 0,
  FIFA96_FRONTEND_EVENT_MENU = 1,
  FIFA96_FRONTEND_EVENT_SELECT = 2,
  FIFA96_FRONTEND_EVENT_JUMP = 3,
  FIFA96_FRONTEND_EVENT_CONFIRM = 4,
  FIFA96_FRONTEND_EVENT_DECLINE = 5,
  FIFA96_FRONTEND_EVENT_EXIT = 6,
  FIFA96_FRONTEND_EVENT_SETTINGS = 7,
  FIFA96_FRONTEND_EVENT_SETTINGS_ALT = 8
};

enum fifa96_frontend_panel_event {
  FIFA96_FRONTEND_PANEL_NONE = 0,
  FIFA96_FRONTEND_PANEL_SELECT = 1,
  FIFA96_FRONTEND_PANEL_EXIT = 2,
  FIFA96_FRONTEND_PANEL_CONFIRM = 3
};

enum fifa96_frontend_exit {
  FIFA96_FRONTEND_EXIT_STATE16 = 0,
  FIFA96_FRONTEND_EXIT_SETTINGS = 1,
  FIFA96_FRONTEND_EXIT_SETTINGS_ALT = 2
};

struct fifa96_frontend {
  uint32_t confirm;
  uint32_t in_match;
  uint32_t phase;
};

void fifa96_frontend_init(struct fifa96_frontend *frontend);
int fifa96_frontend_driver(struct fifa96_frontend *frontend);
int fifa96_frontend_frontend_result(struct fifa96_frontend *frontend,
                                    int32_t code);
int fifa96_frontend_panel_result(struct fifa96_frontend *frontend);
int fifa96_frontend_entry_state(const struct fifa96_frontend *frontend,
                                uint32_t *state);
int fifa96_frontend_event(struct fifa96_frontend *frontend, int32_t code,
                          int confirm_gate,
                          enum fifa96_frontend_event *event, int32_t *mapped);
int fifa96_frontend_panel_event(struct fifa96_frontend *frontend, int32_t code,
                                int select_gate, int confirm_gate,
                                enum fifa96_frontend_panel_event *event,
                                int32_t *mapped);
int fifa96_frontend_exit(struct fifa96_frontend *frontend, int32_t code,
                         enum fifa96_frontend_exit *exit, uint32_t *state);
int fifa96_frontend_dispatch_resolve(const uint32_t *handlers, uint32_t state,
                                     uint32_t *handler);
