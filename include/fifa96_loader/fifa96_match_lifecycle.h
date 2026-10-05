#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_match_pace.h"

enum fifa96_match_screen {
  FIFA96_MATCH_SCREEN_ACTIVE = 0,
  FIFA96_MATCH_SCREEN_OVER = 2,
  FIFA96_MATCH_SCREEN_POST = 3,
  FIFA96_MATCH_SCREEN_EXIT = 4
};

#define FIFA96_MATCH_LIFECYCLE_LEAVE 3u
#define FIFA96_MATCH_LIFECYCLE_RESUME 2u

struct fifa96_match_lifecycle_backend {
  int (*register_callback)(void *ctx);
  int (*cancel_callback)(void *ctx);
  int (*teardown)(void *ctx);
  int (*post_exit)(void *ctx);
  void *ctx;
};

struct fifa96_match_lifecycle {
  uint32_t selector;
  uint32_t active;
  uint32_t screen;
  uint32_t target;
  uint32_t prev;
  uint32_t cadence;
  uint32_t flag;
  uint32_t unload;
  int registered;
  const struct fifa96_match_lifecycle_backend *backend;
};

void fifa96_match_lifecycle_init(struct fifa96_match_lifecycle *lc);
int fifa96_match_lifecycle_begin(struct fifa96_match_lifecycle *lc,
                                 const struct fifa96_match_lifecycle_backend *backend,
                                 struct fifa96_match_pace *pace,
                                 uint32_t selector);
int fifa96_match_lifecycle_mark_over(struct fifa96_match_lifecycle *lc);
int fifa96_match_lifecycle_resolve_over(struct fifa96_match_lifecycle *lc);
int fifa96_match_lifecycle_request_exit(struct fifa96_match_lifecycle *lc);
int fifa96_match_lifecycle_should_exit(const struct fifa96_match_lifecycle *lc);
int fifa96_match_lifecycle_end(struct fifa96_match_lifecycle *lc,
                               struct fifa96_match_pace *pace);
