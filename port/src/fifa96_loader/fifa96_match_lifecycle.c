#include <stddef.h>
#include "fifa96_loader/fifa96_match_lifecycle.h"

void fifa96_match_lifecycle_init(struct fifa96_match_lifecycle *lc) {
  if (!lc) return;
  lc->selector = 0;
  lc->active = 0;
  lc->screen = 0;
  lc->target = 0;
  lc->prev = 0;
  lc->cadence = 0;
  lc->flag = 0;
  lc->unload = 0;
  lc->registered = 0;
  lc->backend = NULL;
}

int fifa96_match_lifecycle_begin(struct fifa96_match_lifecycle *lc,
                                 const struct fifa96_match_lifecycle_backend *backend,
                                 struct fifa96_match_pace *pace,
                                 uint32_t selector) {
  int rc;
  if (!lc || !backend || !pace || !backend->register_callback ||
      !backend->cancel_callback || !backend->teardown) {
    return -FIFA96_ERR_INVALID;
  }
  if (lc->active || lc->registered) return -FIFA96_ERR_STATE;
  lc->selector = selector;
  lc->active = 1;
  lc->screen = FIFA96_MATCH_SCREEN_ACTIVE;
  lc->target = 0;
  lc->prev = 0;
  lc->cadence = 0;
  lc->flag = 0;
  lc->backend = backend;
  fifa96_match_pace_init(pace);
  fifa96_match_pace_resume(pace);
  rc = backend->register_callback(backend->ctx);
  if (rc != 0) {
    lc->active = 0;
    lc->registered = 0;
    lc->backend = NULL;
    fifa96_match_pace_hold(pace);
    return rc;
  }
  lc->registered = 1;
  return 0;
}

int fifa96_match_lifecycle_mark_over(struct fifa96_match_lifecycle *lc) {
  if (!lc) return -FIFA96_ERR_INVALID;
  lc->screen = FIFA96_MATCH_SCREEN_OVER;
  return 0;
}

int fifa96_match_lifecycle_resolve_over(struct fifa96_match_lifecycle *lc) {
  if (!lc) return -FIFA96_ERR_INVALID;
  if (lc->screen != FIFA96_MATCH_SCREEN_OVER) return 0;
  lc->cadence = 0;
  lc->screen = FIFA96_MATCH_SCREEN_POST;
  return 1;
}

int fifa96_match_lifecycle_request_exit(struct fifa96_match_lifecycle *lc) {
  if (!lc) return -FIFA96_ERR_INVALID;
  lc->screen = FIFA96_MATCH_SCREEN_EXIT;
  lc->target = FIFA96_MATCH_LIFECYCLE_LEAVE;
  return 0;
}

int fifa96_match_lifecycle_should_exit(const struct fifa96_match_lifecycle *lc) {
  return lc != NULL && lc->screen == FIFA96_MATCH_SCREEN_EXIT;
}

int fifa96_match_lifecycle_end(struct fifa96_match_lifecycle *lc,
                               struct fifa96_match_pace *pace) {
  int rc = 0;
  if (!lc || !pace) return -FIFA96_ERR_INVALID;
  if (!lc->active) return -FIFA96_ERR_STATE;
  fifa96_match_pace_hold(pace);
  lc->active = 0;
  lc->unload = 0;
  if (lc->registered) {
    if (lc->backend && lc->backend->cancel_callback) {
      int cancel_rc = lc->backend->cancel_callback(lc->backend->ctx);
      if (cancel_rc != 0 && rc == 0) rc = cancel_rc;
    }
    lc->registered = 0;
  }
  if (lc->backend && lc->backend->teardown) {
    int teardown_rc = lc->backend->teardown(lc->backend->ctx);
    if (teardown_rc != 0 && rc == 0) rc = teardown_rc;
  }
  if (lc->target == FIFA96_MATCH_LIFECYCLE_LEAVE) {
    lc->target = FIFA96_MATCH_LIFECYCLE_RESUME;
    if (lc->backend && lc->backend->post_exit) {
      int post_rc = lc->backend->post_exit(lc->backend->ctx);
      if (post_rc != 0 && rc == 0) rc = post_rc;
    }
  }
  if (rc != 0) return rc;
  return lc->target == FIFA96_MATCH_LIFECYCLE_RESUME ? 1 : 0;
}
