#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_MATCH_DISPLAY_DURATION 0x50

typedef int (*fifa96_match_display_enter_fn)(void *user);
typedef int (*fifa96_match_display_leave_fn)(void *user);
typedef int (*fifa96_match_display_refresh_fn)(void *user);

typedef struct {
  fifa96_match_display_enter_fn enter;
  fifa96_match_display_leave_fn leave;
  fifa96_match_display_refresh_fn refresh;
} fifa96_match_display_backend;

typedef struct {
  int32_t suspend;
  int32_t state;
  int32_t value;
  int32_t timer;
  int32_t duration;
  int32_t selector;
} fifa96_match_display;

int fifa96_match_display_init(fifa96_match_display *display, int32_t phase);
int fifa96_match_display_suspend_enter(fifa96_match_display *display,
                                       const fifa96_match_display_backend *backend, void *user);
int fifa96_match_display_suspend_leave(fifa96_match_display *display,
                                       const fifa96_match_display_backend *backend, void *user);
int fifa96_match_display_update(fifa96_match_display *display, int32_t elapsed, int forced);
