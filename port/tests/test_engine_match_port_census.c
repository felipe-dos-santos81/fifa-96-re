/* port/tests/test_engine_match_port_census.c — phase-36: the port-coverage census.
 *
 * Two different questions, two tables:
 *   - the dispatch tables (`fifa96_match_action_table` / `_phase_table`) answer
 *     "does a live engine path reach this slot?" (the wired metric);
 *   - `fifa96_match_port_table` answers "does every native slot that HAS a body
 *     have a C implementation?" (the port-coverage metric).
 *
 * Native census: 45 action slots + 35 phase slots = 80. Two slots have no body
 * by design: action 0x2B is the shared row-29 epilogue RET at 0x87738 and phase
 * 0x16 is the loader's zero/INT3 sentinel. That leaves 78 native-body slots.
 * The completion percentage in the README is floor(100 * bodies / 78), and the
 * target is 78/78 = 100%. The wired count stays a separate, stricter metric.
 *
 * The test also refuses a silent dispatch/port mismatch: every non-NULL
 * dispatch handler must be the same function the port table names. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_handlers.h"

static const struct fifa96_match_handler *dispatch_row(
    const struct fifa96_match_port_row *p) {
  return p->domain == FIFA96_MATCH_PORT_ACTION
             ? &fifa96_match_action_table[p->code]
             : &fifa96_match_phase_table[p->code];
}

/* The derived numbers must be the ones the docs carry (AGENTS.md: never
 * hand-tuned). Run from the repo root (CMake WORKING_DIRECTORY). */
static int file_contains(const char *path, const char *needle) {
  FILE *fp = fopen(path, "r");
  char line[1024];
  int found = 0;
  if (!fp) return 0;
  while (fgets(line, sizeof line, fp) != NULL) {
    if (strstr(line, needle) != NULL) {
      found = 1;
      break;
    }
  }
  fclose(fp);
  return found;
}

int main(void) {
  unsigned action_install = 0;
  unsigned phase_install = 0;
  unsigned bodies = 0;
  unsigned no_body = 0;
  unsigned none = 0;
  unsigned i;

  assert(FIFA96_MATCH_PORT_SLOTS == 80u);
  assert(FIFA96_MATCH_PORT_NATIVE_ROWS == 78u);
  assert(FIFA96_MATCH_ACTION_ROWS == 45u);
  assert(FIFA96_MATCH_PHASE_ROWS == 35u);

  for (i = 0; i < FIFA96_MATCH_PORT_SLOTS; i++) {
    const struct fifa96_match_port_row *p = &fifa96_match_port_table[i];
    const struct fifa96_match_handler *d = dispatch_row(p);
    if (i < FIFA96_MATCH_ACTION_ROWS) {
      assert(p->domain == FIFA96_MATCH_PORT_ACTION);
      assert(p->code == (uint8_t)i);
    } else {
      assert(p->domain == FIFA96_MATCH_PORT_PHASE);
      assert(p->code == (uint8_t)(i - FIFA96_MATCH_ACTION_ROWS));
    }
    if (p->kind == FIFA96_MATCH_PORT_BODY) {
      assert(p->body != NULL && p->body[0] != '\0');
      /* At least one typed body pointer must name the implementation: a stale
       * string alone is not a port. */
      assert(p->fn != NULL || p->arm_fn != NULL || p->body_0d_fn != NULL);
      bodies++;
    } else if (p->kind == FIFA96_MATCH_PORT_NO_BODY) {
      assert(p->fn == NULL && p->body == NULL);
      no_body++;
    } else {
      assert(p->kind == FIFA96_MATCH_PORT_NONE);
      assert(p->fn == NULL && p->body == NULL);
      none++;
    }
    /* A dispatch handler must be the same C body the port table names. */
    if (d->fn != NULL) {
      assert(p->kind == FIFA96_MATCH_PORT_BODY);
      assert(p->fn == d->fn);
      if (p->domain == FIFA96_MATCH_PORT_ACTION) action_install++;
      else phase_install++;
    }
  }

  assert(none == 0u);          /* every native-body slot is ported */
  assert(bodies == 78u);
  assert(no_body == 2u);
  assert(bodies + no_body == FIFA96_MATCH_PORT_SLOTS);
  assert(fifa96_match_port_bodies() == bodies);
  assert(fifa96_match_port_no_body() == no_body);
  assert(fifa96_match_port_install_live() == action_install + phase_install);
  assert(fifa96_match_port_blocked_count() == 5u);
  assert(fifa96_match_port_wired() == 57u);

  /* The two body-less slots, at their native positions. */
  assert(fifa96_match_port_table[0x2B].kind == FIFA96_MATCH_PORT_NO_BODY);
  assert(fifa96_match_port_table[FIFA96_MATCH_ACTION_ROWS + 0x16].kind ==
         FIFA96_MATCH_PORT_NO_BODY);

  /* The install-live census (the natural count subtracts the blocked set). */
  assert(action_install == 37u);
  assert(phase_install == 25u);
  assert(fifa96_match_port_install_live() == 62u);

  /* The five producer-blocked rows are install-live but not naturally
   * reachable (FU-338 §4); the natural count subtracts them. */
  for (i = 0; i < FIFA96_MATCH_PORT_BLOCKED_ROWS; i++) {
    const struct fifa96_match_blocked_row *b = &fifa96_match_port_blocked[i];
    const struct fifa96_match_handler *d =
        b->domain == FIFA96_MATCH_PORT_ACTION
            ? &fifa96_match_action_table[b->code]
            : &fifa96_match_phase_table[b->code];
    assert(d->fn != NULL);
  }

  assert(fifa96_match_port_bodies() == FIFA96_MATCH_PORT_NATIVE_ROWS);
  assert(file_contains("README.md", "Reverse Engineering (100%)"));
  assert(file_contains("README.md", "57/80 dispatch slots naturally wired"));
  assert(file_contains("port/ENGINE.md",
                       "78/78 native dispatch bodies ported (100%)"));
  assert(file_contains("port/ENGINE.md", "57/80 dispatch slots wired"));
  assert(file_contains("port/ENGINE.md", "62/80 install-live"));
  printf("port-census: bodies %u/%u (%u%%) no-body %u wired %u/%u "
         "install-live %u/%u action %u/45 phase %u/35\n",
         bodies, (unsigned)FIFA96_MATCH_PORT_NATIVE_ROWS,
         (100u * bodies) / (unsigned)FIFA96_MATCH_PORT_NATIVE_ROWS, no_body,
         fifa96_match_port_wired(), (unsigned)FIFA96_MATCH_PORT_SLOTS,
         fifa96_match_port_install_live(),
         (unsigned)FIFA96_MATCH_PORT_SLOTS, action_install, phase_install);
  puts("test_engine_match_port_census OK");
  return 0;
}
