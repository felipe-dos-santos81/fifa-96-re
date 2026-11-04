#include "fifa96_loader/fifa96_match_display.h"

int fifa96_match_display_init(fifa96_match_display *display, int32_t phase) {
  if (display == NULL)
    return -FIFA96_ERR_INVALID;
  display->suspend = 0;
  display->state = 0;
  display->value = 0;
  display->timer = 0;
  display->duration = FIFA96_MATCH_DISPLAY_DURATION;
  display->selector = phase;
  if (phase == 0x10) {
    display->selector = 0x0C;
    display->suspend = 1;
    display->value = 0x10;
  }
  return FIFA96_OK;
}

int fifa96_match_display_suspend_enter(fifa96_match_display *display,
                                       const fifa96_match_display_backend *backend, void *user) {
  if (display == NULL)
    return -FIFA96_ERR_INVALID;
  if (display->suspend)
    return FIFA96_OK;
  if (backend != NULL && backend->enter != NULL) {
    int rc = backend->enter(user);
    if (rc < 0)
      return rc;
  }
  display->suspend = 1;
  return FIFA96_OK;
}

int fifa96_match_display_suspend_leave(fifa96_match_display *display,
                                       const fifa96_match_display_backend *backend, void *user) {
  int rc = FIFA96_OK;
  if (display == NULL)
    return -FIFA96_ERR_INVALID;
  if (!display->suspend)
    return FIFA96_OK;
  if (backend != NULL && backend->leave != NULL) {
    int result = backend->leave(user);
    if (result < 0)
      rc = result;
  }
  display->suspend = 0;
  if (backend != NULL && backend->refresh != NULL) {
    int result = backend->refresh(user);
    if (result < 0 && rc == FIFA96_OK)
      rc = result;
  }
  return rc;
}

int fifa96_match_display_update(fifa96_match_display *display, int32_t elapsed, int forced) {
  if (display == NULL)
    return -FIFA96_ERR_INVALID;
  if (display->state != 1)
    return 0;
  display->timer += elapsed;
  if (forced || display->duration <= display->timer) {
    display->timer = 0;
    display->duration = 0;
    return 1;
  }
  return 0;
}
