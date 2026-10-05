#include <stddef.h>
#include "fifa96_loader/fifa96_frontend.h"

void fifa96_frontend_init(struct fifa96_frontend *frontend) {
  if (frontend == NULL) {
    return;
  }
  frontend->confirm = 0;
  frontend->in_match = 0;
  frontend->phase = FIFA96_FRONTEND_PHASE_FRONTEND;
}

int fifa96_frontend_driver(struct fifa96_frontend *frontend) {
  if (frontend == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  if (frontend->confirm != 0) {
    frontend->phase = FIFA96_FRONTEND_PHASE_EXIT;
  }
  return 0;
}

int fifa96_frontend_frontend_result(struct fifa96_frontend *frontend,
                                    int32_t code) {
  if (frontend == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  if (code == 8) {
    frontend->phase = FIFA96_FRONTEND_PHASE_PANEL;
    return 0;
  }
  if (code == 10 || code == 11) {
    frontend->phase = FIFA96_FRONTEND_PHASE_EXIT;
    return 0;
  }
  return -FIFA96_ERR_UNSUPPORTED;
}

int fifa96_frontend_panel_result(struct fifa96_frontend *frontend) {
  if (frontend == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  frontend->phase = FIFA96_FRONTEND_PHASE_FRONTEND;
  return 0;
}

int fifa96_frontend_entry_state(const struct fifa96_frontend *frontend,
                                uint32_t *state) {
  if (frontend == NULL || state == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  if (frontend->in_match != 0) {
    return 0;
  }
  *state = 17;
  return 1;
}

int fifa96_frontend_event(struct fifa96_frontend *frontend, int32_t code,
                          int confirm_gate,
                          enum fifa96_frontend_event *event, int32_t *mapped) {
  if (frontend == NULL || event == NULL || mapped == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  *mapped = code;
  if (code >= 0 && code < 8) {
    *event = FIFA96_FRONTEND_EVENT_MENU;
  } else if (code == 8) {
    *event = FIFA96_FRONTEND_EVENT_EXIT;
  } else if (code == 9) {
    *event = FIFA96_FRONTEND_EVENT_SELECT;
  } else if (code == 10) {
    *event = FIFA96_FRONTEND_EVENT_SETTINGS;
  } else if (code == 11) {
    *event = FIFA96_FRONTEND_EVENT_SETTINGS_ALT;
  } else if (code == -10) {
    if (confirm_gate != 0) {
      frontend->confirm = 1;
      *mapped = 10;
      *event = FIFA96_FRONTEND_EVENT_CONFIRM;
    } else {
      *event = FIFA96_FRONTEND_EVENT_DECLINE;
    }
  } else if (code == -11) {
    *event = FIFA96_FRONTEND_EVENT_JUMP;
  } else {
    *event = FIFA96_FRONTEND_EVENT_NONE;
  }
  return 0;
}

int fifa96_frontend_panel_event(struct fifa96_frontend *frontend, int32_t code,
                                int select_gate, int confirm_gate,
                                enum fifa96_frontend_panel_event *event,
                                int32_t *mapped) {
  if (frontend == NULL || event == NULL || mapped == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  *mapped = code;
  if (code == 4 || code == 5) {
    *event = FIFA96_FRONTEND_PANEL_EXIT;
  } else if (code == -10) {
    if (confirm_gate != 0) {
      frontend->confirm = 1;
      *mapped = 5;
      *event = FIFA96_FRONTEND_PANEL_CONFIRM;
    } else {
      *event = FIFA96_FRONTEND_PANEL_NONE;
    }
  } else if (code == 0 || code == 1) {
    if (select_gate != 0) {
      *event = FIFA96_FRONTEND_PANEL_SELECT;
    } else {
      *event = FIFA96_FRONTEND_PANEL_NONE;
    }
  } else {
    *event = FIFA96_FRONTEND_PANEL_NONE;
  }
  return 0;
}

int fifa96_frontend_exit(struct fifa96_frontend *frontend, int32_t code,
                         enum fifa96_frontend_exit *exit, uint32_t *state) {
  if (frontend == NULL || exit == NULL || state == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  if (code == 8) {
    frontend->in_match = 0;
    *exit = FIFA96_FRONTEND_EXIT_STATE16;
    *state = 16;
    return 0;
  }
  if (code == 10) {
    *exit = FIFA96_FRONTEND_EXIT_SETTINGS;
    return 0;
  }
  if (code == 11) {
    *exit = FIFA96_FRONTEND_EXIT_SETTINGS_ALT;
    return 0;
  }
  return -FIFA96_ERR_UNSUPPORTED;
}

int fifa96_frontend_dispatch_resolve(const uint32_t *handlers, uint32_t state,
                                     uint32_t *handler) {
  if (handlers == NULL || handler == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  if (state > 0x13u) {
    return 0;
  }
  if (handlers[state] == 0) {
    return -FIFA96_ERR_NOT_FOUND;
  }
  *handler = handlers[state];
  return 0;
}
