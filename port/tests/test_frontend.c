#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_frontend.h"

static void test_init_zeroes(void) {
  struct fifa96_frontend frontend;
  frontend.confirm = 7;
  frontend.in_match = 7;
  frontend.phase = 7;
  fifa96_frontend_init(&frontend);
  assert(frontend.confirm == 0);
  assert(frontend.in_match == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
}

static void test_driver_confirm_gate(void) {
  struct fifa96_frontend frontend;
  fifa96_frontend_init(&frontend);
  assert(fifa96_frontend_driver(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  frontend.phase = FIFA96_FRONTEND_PHASE_PANEL;
  assert(fifa96_frontend_driver(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);
  frontend.confirm = 1;
  assert(fifa96_frontend_driver(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_EXIT);
  assert(fifa96_frontend_driver(NULL) == -FIFA96_ERR_INVALID);
}

static void test_frontend_result_mapping(void) {
  struct fifa96_frontend frontend;
  fifa96_frontend_init(&frontend);
  assert(fifa96_frontend_frontend_result(&frontend, 8) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);
  frontend.phase = FIFA96_FRONTEND_PHASE_FRONTEND;
  assert(fifa96_frontend_frontend_result(&frontend, 10) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_EXIT);
  frontend.phase = FIFA96_FRONTEND_PHASE_FRONTEND;
  assert(fifa96_frontend_frontend_result(&frontend, 11) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_EXIT);
  frontend.phase = FIFA96_FRONTEND_PHASE_PANEL;
  assert(fifa96_frontend_frontend_result(&frontend, 9) ==
         -FIFA96_ERR_UNSUPPORTED);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);
  assert(fifa96_frontend_frontend_result(&frontend, -1) ==
         -FIFA96_ERR_UNSUPPORTED);
  assert(fifa96_frontend_frontend_result(NULL, 8) == -FIFA96_ERR_INVALID);
}

static void test_panel_result_returns_to_frontend(void) {
  struct fifa96_frontend frontend;
  fifa96_frontend_init(&frontend);
  frontend.phase = FIFA96_FRONTEND_PHASE_PANEL;
  assert(fifa96_frontend_panel_result(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  frontend.phase = FIFA96_FRONTEND_PHASE_PANEL;
  frontend.confirm = 1;
  assert(fifa96_frontend_panel_result(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  assert(fifa96_frontend_panel_result(NULL) == -FIFA96_ERR_INVALID);
}

static void test_entry_state(void) {
  struct fifa96_frontend frontend;
  uint32_t state = 0xdeadbeefu;
  fifa96_frontend_init(&frontend);
  assert(fifa96_frontend_entry_state(&frontend, &state) == 1);
  assert(state == 17);
  frontend.in_match = 1;
  state = 0xdeadbeefu;
  assert(fifa96_frontend_entry_state(&frontend, &state) == 0);
  assert(state == 0xdeadbeefu);
  assert(fifa96_frontend_entry_state(NULL, &state) == -FIFA96_ERR_INVALID);
  assert(fifa96_frontend_entry_state(&frontend, NULL) == -FIFA96_ERR_INVALID);
}

static void test_frontend_event_classification(void) {
  struct fifa96_frontend frontend;
  enum fifa96_frontend_event event = FIFA96_FRONTEND_EVENT_EXIT;
  int32_t mapped = 0;
  fifa96_frontend_init(&frontend);
  assert(fifa96_frontend_event(&frontend, 0, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_MENU && mapped == 0);
  assert(fifa96_frontend_event(&frontend, 7, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_MENU && mapped == 7);
  assert(fifa96_frontend_event(&frontend, 8, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_EXIT && mapped == 8);
  assert(fifa96_frontend_event(&frontend, 9, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_SELECT && mapped == 9);
  assert(fifa96_frontend_event(&frontend, -11, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_JUMP && mapped == -11);
  assert(fifa96_frontend_event(&frontend, -1, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_NONE && mapped == -1);
  assert(fifa96_frontend_event(&frontend, 12, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_NONE && mapped == 12);
  assert(fifa96_frontend_event(&frontend, 10, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_SETTINGS && mapped == 10);
  assert(fifa96_frontend_event(&frontend, 11, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_SETTINGS_ALT && mapped == 11);
}

static void test_frontend_event_confirm_gate(void) {
  struct fifa96_frontend frontend;
  enum fifa96_frontend_event event = FIFA96_FRONTEND_EVENT_NONE;
  int32_t mapped = 0;
  fifa96_frontend_init(&frontend);
  assert(fifa96_frontend_event(&frontend, -10, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_DECLINE && mapped == -10);
  assert(frontend.confirm == 0);
  assert(fifa96_frontend_event(&frontend, -10, 1, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_EVENT_CONFIRM && mapped == 10);
  assert(frontend.confirm == 1);
  assert(fifa96_frontend_event(NULL, 0, 0, &event, &mapped) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_frontend_event(&frontend, 0, 0, NULL, &mapped) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_frontend_event(&frontend, 0, 0, &event, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_panel_event_classification(void) {
  struct fifa96_frontend frontend;
  enum fifa96_frontend_panel_event event = FIFA96_FRONTEND_PANEL_NONE;
  int32_t mapped = 0;
  fifa96_frontend_init(&frontend);
  assert(fifa96_frontend_panel_event(&frontend, 4, 0, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_PANEL_EXIT && mapped == 4);
  assert(fifa96_frontend_panel_event(&frontend, 5, 0, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_PANEL_EXIT && mapped == 5);
  assert(fifa96_frontend_panel_event(&frontend, 0, 1, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_PANEL_SELECT && mapped == 0);
  assert(fifa96_frontend_panel_event(&frontend, 1, 1, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_PANEL_SELECT && mapped == 1);
  assert(fifa96_frontend_panel_event(&frontend, 0, 0, 0, &event, &mapped) == 0);
  assert(event == FIFA96_FRONTEND_PANEL_NONE && mapped == 0);
  assert(fifa96_frontend_panel_event(&frontend, -10, 0, 0, &event, &mapped) ==
         0);
  assert(event == FIFA96_FRONTEND_PANEL_NONE && mapped == -10);
  assert(frontend.confirm == 0);
  assert(fifa96_frontend_panel_event(&frontend, -10, 0, 1, &event, &mapped) ==
         0);
  assert(event == FIFA96_FRONTEND_PANEL_CONFIRM && mapped == 5);
  assert(frontend.confirm == 1);
  assert(fifa96_frontend_panel_event(&frontend, -2, 0, 0, &event, &mapped) ==
         0);
  assert(event == FIFA96_FRONTEND_PANEL_NONE && mapped == -2);
  assert(fifa96_frontend_panel_event(NULL, 4, 0, 0, &event, &mapped) ==
         -FIFA96_ERR_INVALID);
}

static void test_exit_classification(void) {
  struct fifa96_frontend frontend;
  enum fifa96_frontend_exit exit = FIFA96_FRONTEND_EXIT_SETTINGS;
  uint32_t state = 0xdeadbeefu;
  fifa96_frontend_init(&frontend);
  frontend.in_match = 1;
  assert(fifa96_frontend_exit(&frontend, 8, &exit, &state) == 0);
  assert(exit == FIFA96_FRONTEND_EXIT_STATE16);
  assert(state == 16);
  assert(frontend.in_match == 0);
  state = 0xdeadbeefu;
  assert(fifa96_frontend_exit(&frontend, 10, &exit, &state) == 0);
  assert(exit == FIFA96_FRONTEND_EXIT_SETTINGS);
  assert(state == 0xdeadbeefu);
  assert(fifa96_frontend_exit(&frontend, 11, &exit, &state) == 0);
  assert(exit == FIFA96_FRONTEND_EXIT_SETTINGS_ALT);
  assert(fifa96_frontend_exit(&frontend, 9, &exit, &state) ==
         -FIFA96_ERR_UNSUPPORTED);
  assert(fifa96_frontend_exit(NULL, 8, &exit, &state) == -FIFA96_ERR_INVALID);
  assert(fifa96_frontend_exit(&frontend, 8, NULL, &state) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_frontend_exit(&frontend, 8, &exit, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_dispatch_resolve(void) {
  uint32_t handlers[FIFA96_FRONTEND_STATE_COUNT];
  uint32_t handler = 0xdeadbeefu;
  for (uint32_t i = 0; i < FIFA96_FRONTEND_STATE_COUNT; i++) {
    handlers[i] = 0x14451u + i * 4u;
  }
  assert(fifa96_frontend_dispatch_resolve(handlers, 0, &handler) == 0);
  assert(handler == 0x14451u);
  assert(fifa96_frontend_dispatch_resolve(handlers, 19, &handler) == 0);
  assert(handler == 0x14451u + 19u * 4u);
  handler = 0xdeadbeefu;
  assert(fifa96_frontend_dispatch_resolve(handlers, 20, &handler) == 0);
  assert(handler == 0xdeadbeefu);
  assert(fifa96_frontend_dispatch_resolve(handlers, 0xFFFFFFFFu, &handler) ==
         0);
  assert(handler == 0xdeadbeefu);
  handlers[3] = 0;
  assert(fifa96_frontend_dispatch_resolve(handlers, 3, &handler) ==
         -FIFA96_ERR_NOT_FOUND);
  assert(handler == 0xdeadbeefu);
  assert(fifa96_frontend_dispatch_resolve(NULL, 0, &handler) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_frontend_dispatch_resolve(handlers, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_driver_sequence(void) {
  struct fifa96_frontend frontend;
  enum fifa96_frontend_exit exit;
  uint32_t state = 0;
  fifa96_frontend_init(&frontend);
  assert(fifa96_frontend_driver(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  assert(fifa96_frontend_frontend_result(&frontend, 8) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);
  assert(fifa96_frontend_driver(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);
  assert(fifa96_frontend_panel_result(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  assert(fifa96_frontend_driver(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  assert(fifa96_frontend_frontend_result(&frontend, 11) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_EXIT);
  assert(fifa96_frontend_driver(&frontend) == 0);
  assert(frontend.phase == FIFA96_FRONTEND_PHASE_EXIT);
  assert(fifa96_frontend_exit(&frontend, 11, &exit, &state) == 0);
  assert(exit == FIFA96_FRONTEND_EXIT_SETTINGS_ALT);
}

int main(void) {
  test_init_zeroes();
  test_driver_confirm_gate();
  test_frontend_result_mapping();
  test_panel_result_returns_to_frontend();
  test_entry_state();
  test_frontend_event_classification();
  test_frontend_event_confirm_gate();
  test_panel_event_classification();
  test_exit_classification();
  test_dispatch_resolve();
  test_driver_sequence();
  puts("test_frontend: ok");
  return 0;
}
