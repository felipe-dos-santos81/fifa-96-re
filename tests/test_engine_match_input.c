/* tests/test_engine_match_input.c — Task 14: match input mapping and control
 * slots (docs/ghidra/FU61_match_input.md, FU70_control_slots.md).
 *
 * FU-61 §1.2 keyboard-handler key-code rows (handler at flat 0x468B7; the
 * INT-9 make/break handler writes [scancode + 0x112BA4] at 0xCB973 / 0xCB964):
 *   direction 0x1 = up    (flag 0x112BEC = scancode 0x48 keypad 8, OR at 0x46934)
 *   direction 0x2 = down  (flag 0x112BF4 = scancode 0x50 keypad 2, OR at 0x46941)
 *   direction 0x8 = left  (flag 0x112BEF = scancode 0x4B keypad 4, OR at 0x46945)
 *   direction 0x4 = right (flag 0x112BF1 = scancode 0x4D keypad 6, OR at 0x46952)
 *   the diagonal flags OR 0x9 (keypad 7, 0x4695F), 0x5 (keypad 9, 0x4696C),
 *   0xA (keypad 1, 0x46979) and 0x6 (keypad 3, 0x46986), cross-checking the
 *   up/left and down/right axis assignment.
 *   button 0x10 = kick (Ctrl/A flags 0x112BC1/0x112BC2, OR at 0x468DE)
 *   button 0x20 = pass (Alt/S flags 0x112BDC/0x112BC3, OR at 0x468F4)
 *   buttons 0x40 (Space/D/Enter) and 0x80 (W) have no engine key in this task.
 *
 * FU-70 §1.1/§1.2 slot tables (flat): direction map 0x11064E, animation chain
 * T1 0x10E1DC -> T2 0x10E1EC / T3 0x10E1F5. The slot machine runs from the
 * frame body once per granted 30 Hz frame (not per input poll); FU-62 §4.3
 * makes the delta the whole frame-clock word = 2 at the 0x200 step
 * (60 counter units/s). A fifa96_match_run_input poll only latches the sample. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

/* FU-70 §1.2 tables, read from the flat binary (0x11064E, 0x10E1DC,
 * 0x10E1EC, 0x10E1F5); pinned here so the engine wire-in is observable. */
static const uint8_t fu70_map[16] = {0, 5, 10, 0, 6, 4, 2, 0, 9, 1, 8, 0, 0, 0, 0, 0};
static const uint8_t fu70_t1[16] = {0, 1, 5, 0, 3, 2, 4, 0, 7, 8, 6, 0, 0, 0, 0, 0};
static const uint8_t fu70_t2[16] = {0, 1, 1, 0, 255, 255, 255, 0, 1, 0, 0, 255, 255, 255, 0, 1};
static const uint8_t fu70_t3[16] = {0, 0, 255, 255, 255, 0, 1, 1, 1, 0, 0, 64, 0, 0, 0, 0};

static void press_key(struct fifa96_match_run *mr, int32_t raw_code) {
  fifa96_platform_key key = {.raw_code = raw_code, .state = 1};
  assert(fifa96_match_run_input(mr, &key, 1) == 0);
}

static void no_input(struct fifa96_match_run *mr) {
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
}

/* One 100 Hz pace tick; returns 1 when it granted a 30 Hz match frame. */
static int frame_tick(struct fifa96_match_run *mr) {
  int rc = fifa96_match_run_frame(mr);
  assert(rc >= 0);
  return rc;
}

/* Advance the pace until exactly one 30 Hz frame is granted (at most 4 ticks
 * from a zero accumulator: 0x102 + 0x102 + 0x102 + 0x102 >= 0x35C). */
static void grant_frame(struct fifa96_match_run *mr) {
  for (int i = 0; i < 8; i++) {
    if (frame_tick(mr)) return;
  }
  assert(0);
}

static void test_init_initializes_input_and_slot(void) {
  struct fifa96_match_run mr;
  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);

  assert(mr.input.prev[0] == 0 && mr.input.prev[1] == 0);
  assert(mr.input.prev[2] == 0 && mr.input.prev[3] == 0);
  assert(mr.input.edge[0] == 0 && mr.input.edge[1] == 0);
  assert(mr.input.edge[2] == 0 && mr.input.edge[3] == 0);
  assert(mr.input.held == 0 && mr.input.fresh == 0);
  assert(mr.input.held_latch == 0 && mr.input.fresh_latch == 0);
  assert(mr.input_state[0] == 0 && mr.input_state[1] == 0);
  assert(mr.input_state[2] == 0 && mr.input_state[3] == 0);

  assert(mr.slot.entity == -1);
  assert(mr.slot.player == 0);
  assert(mr.slot.ordinal == 0);
  assert(mr.slot.map_select == 1);   /* FU-70 §1.4: 0x4C1DC defaults to 1 */
  assert(mr.slot.active == 0);
  assert(mr.slot.pressed == 0 && mr.slot.released == 0);
  assert(mr.slot.held == 0 && mr.slot.held_prev == 0);
  assert(mr.slot.prev_mapped == 0 && mr.slot.raw == 0);
  assert(mr.slot.anim_a == 0 && mr.slot.anim_b == 0 && mr.slot.anim_c == 0);
  assert(mr.slot.counter == 0);
}

static void test_null_guards(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_input(NULL, NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_input(NULL, NULL, 4) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_input(&mr, NULL, 5) == 0);   /* tolerated no-input poll */
  assert(mr.input.held == 0 && mr.input.fresh == 0);
  assert(mr.input_state[0] == 0 && mr.slot.counter == 0);
}

/* Each direction key lands on its exact FU-61 handler code and drives the
 * FU-70 slot animation chain (T1 -> T2/T3). */
static void test_direction_codes_and_animation(void) {
  static const struct {
    int32_t key;
    uint8_t code;
    uint8_t anim_a, anim_b, anim_c;
  } cases[] = {
    {FIFA96_ENGINE_KEY_UP,    0x01, 1, 1, 0},
    {FIFA96_ENGINE_KEY_DOWN,  0x02, 5, 255, 0},
    {FIFA96_ENGINE_KEY_LEFT,  0x08, 7, 0, 1},
    {FIFA96_ENGINE_KEY_RIGHT, 0x04, 3, 0, 255},
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    press_key(&mr, cases[i].key);
    assert(mr.input.prev[0] == cases[i].code);
    assert(mr.input.edge[0] == cases[i].code);
    assert(mr.input.held == cases[i].code);
    assert(mr.input.fresh == cases[i].code);
    assert(mr.input_state[0] == cases[i].code);
    assert(mr.slot.counter == 0 && mr.slot.raw == 0);   /* poll only latches */
    grant_frame(&mr);
    assert(mr.slot.raw == cases[i].code);
    assert(mr.slot.prev_mapped == cases[i].code);
    assert(mr.slot.pressed == cases[i].code);
    assert(mr.slot.released == 0);
    assert(mr.slot.anim_a == cases[i].anim_a);
    assert(mr.slot.anim_b == cases[i].anim_b);
    assert(mr.slot.anim_c == cases[i].anim_c);
    assert(mr.slot.counter == 2);   /* one update per frame, delta 2 */
  }
}

/* KICK/PASS button bits and their OR with a direction, plus the diagonal
 * raw-nibble OR the keyboard handler performs. */
static void test_button_and_diagonal_codes(void) {
  struct fifa96_match_run mr;

  fifa96_match_run_init(&mr);
  press_key(&mr, FIFA96_ENGINE_KEY_KICK);
  assert(mr.input.prev[0] == 0x10);
  assert(mr.input.edge[0] == 0x10);
  assert(mr.input.held == 0x10 && mr.input.fresh == 0x10);
  assert(mr.slot.counter == 0);              /* the poll does not touch the slot */
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x10);
  assert(mr.slot.anim_a == 0 && mr.slot.anim_b == 0 && mr.slot.anim_c == 0);

  fifa96_match_run_init(&mr);
  press_key(&mr, FIFA96_ENGINE_KEY_PASS);
  assert(mr.input.prev[0] == 0x20);
  assert(mr.input.edge[0] == 0x20);
  assert(mr.input.held == 0x20 && mr.input.fresh == 0x20);
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x20);

  fifa96_match_run_init(&mr);
  {
    fifa96_platform_key keys[2] = {
      {.raw_code = FIFA96_ENGINE_KEY_KICK, .state = 1},
      {.raw_code = FIFA96_ENGINE_KEY_UP, .state = 1},
    };
    assert(fifa96_match_run_input(&mr, keys, 2) == 0);
  }
  assert(mr.input.prev[0] == 0x11);
  assert(mr.input.edge[0] == 0x11);
  assert(mr.input.held == 0x11 && mr.input.fresh == 0x11);
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x11);
  assert(mr.slot.anim_a == fu70_t1[0x1]);   /* 1 */
  assert(mr.slot.anim_b == fu70_t2[1]);     /* 1 */
  assert(mr.slot.anim_c == fu70_t3[1]);     /* 0 */

  fifa96_match_run_init(&mr);
  {
    fifa96_platform_key keys[2] = {
      {.raw_code = FIFA96_ENGINE_KEY_UP, .state = 1},
      {.raw_code = FIFA96_ENGINE_KEY_RIGHT, .state = 1},
    };
    assert(fifa96_match_run_input(&mr, keys, 2) == 0);
  }
  assert(mr.input.prev[0] == 0x05);   /* handler diagonal OR 0x1|0x4 */
  assert(mr.input.edge[0] == 0x05);
  assert(mr.input.held == 0x05 && mr.input.fresh == 0x05);
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x05);
  assert(mr.slot.anim_a == fu70_t1[0x5]);   /* 2 */
  assert(mr.slot.anim_b == fu70_t2[2]);     /* 1 */
  assert(mr.slot.anim_c == fu70_t3[2]);     /* 255 */
}

/* The FU-61 edge model: an edge fires once per press, nonzero->nonzero is
 * suppressed and releases are derived from the previous-state array. */
static void test_edges_fire_once_per_press(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  press_key(&mr, FIFA96_ENGINE_KEY_RIGHT);
  assert(mr.input.edge[0] == 0x04 && mr.input.fresh == 0x04);
  assert(mr.input.held == 0x04 && mr.input.held_latch == 0x04);

  press_key(&mr, FIFA96_ENGINE_KEY_RIGHT);   /* still held: no new edge */
  assert(mr.input.prev[0] == 0x04);
  assert(mr.input.edge[0] == 0 && mr.input.fresh == 0);
  assert(mr.input.held == 0x04);

  /* An explicit release event is ignored for edges ... */
  {
    fifa96_platform_key key = {.raw_code = FIFA96_ENGINE_KEY_RIGHT, .state = 0};
    assert(fifa96_match_run_input(&mr, &key, 1) == 0);
  }
  /* ... the release is derived from the missing press. */
  assert(mr.input.prev[0] == 0);
  assert(mr.input.edge[0] == 0 && mr.input.held == 0);
  assert(mr.input.fresh_latch == 0x04);      /* latch stays sticky */

  press_key(&mr, FIFA96_ENGINE_KEY_RIGHT);   /* release-then-repress */
  assert(mr.input.edge[0] == 0x04);

  /* Nonzero -> nonzero (a different key while one is held) is suppressed. */
  fifa96_match_run_init(&mr);
  press_key(&mr, FIFA96_ENGINE_KEY_LEFT);
  assert(mr.input.edge[0] == 0x08);
  press_key(&mr, FIFA96_ENGINE_KEY_UP);
  assert(mr.input.prev[0] == 0x01);
  assert(mr.input.edge[0] == 0);
  assert(mr.input.held == 0x01);
  assert(mr.input.edge[1] == 0 && mr.input.edge[2] == 0 && mr.input.edge[3] == 0);
}

/* FU-70 hold machine / released reporting / counter, through the engine entry
 * (same semantics tests/test_control.c pins with synthetic tables). */
static void test_slot_hold_machine_and_counter(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  press_key(&mr, FIFA96_ENGINE_KEY_KICK);            /* 0x10 */
  assert(mr.slot.counter == 0);                      /* poll only */
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x10 && mr.slot.held == 0);
  assert(mr.slot.counter == 2);

  press_key(&mr, FIFA96_ENGINE_KEY_PASS);            /* 0x20: 0x10 falls */
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x20);
  assert(mr.slot.held == 0x20);      /* mask armed by the falling 0x10 */
  assert(mr.slot.held_prev == 0x10);
  assert(mr.slot.released == 0);
  assert(mr.slot.counter == 4);

  {
    fifa96_platform_key keys[2] = {
      {.raw_code = FIFA96_ENGINE_KEY_KICK, .state = 1},
      {.raw_code = FIFA96_ENGINE_KEY_PASS, .state = 1},
    };
    assert(fifa96_match_run_input(&mr, keys, 2) == 0);   /* 0x30 */
  }
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x10);   /* adding KICK is a rising edge */
  assert(mr.slot.held == 0x20);      /* the mask keeps PASS */
  assert(mr.slot.counter == 6);

  press_key(&mr, FIFA96_ENGINE_KEY_PASS);            /* 0x20: 0x10 falls */
  grant_frame(&mr);
  assert(mr.slot.pressed == 0);
  assert(mr.slot.released == 0);
  assert(mr.slot.held == 0x20);      /* residual mask held */
  assert(mr.slot.counter == 8);

  press_key(&mr, FIFA96_ENGINE_KEY_KICK);            /* 0x10: 0x20 falls */
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x10);   /* KICK rises again */
  assert(mr.slot.held == 0);         /* mask empties ... */
  assert(mr.slot.released == 0x10);  /* ... releasing the held_prev save */
  assert(mr.slot.counter == 10);

  no_input(&mr);
  grant_frame(&mr);
  assert(mr.slot.held == 0);
  assert(mr.slot.released == 0x10);
  assert(mr.slot.counter == 10);                     /* release does not reset */

  no_input(&mr);
  grant_frame(&mr);
  assert(mr.slot.released == 0);
  assert(mr.slot.counter == 0);                      /* idle reset */
}

/* A direction-only release reports no released word (FU-70 releases only the
 * held button word) and resets the counter. */
static void test_direction_release_reports_no_button(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  press_key(&mr, FIFA96_ENGINE_KEY_DOWN);   /* 0x02 */
  grant_frame(&mr);
  assert(mr.slot.pressed == 0x02 && mr.slot.counter == 2);
  no_input(&mr);
  grant_frame(&mr);
  assert(mr.slot.pressed == 0 && mr.slot.held == 0);
  assert(mr.slot.released == 0);
  assert(mr.slot.counter == 0);
}

/* The FU-70 direction map 0x11064E is only applied when map_select == 0; the
 * default raw path (map_select == 1) is pinned above. */
static void test_map_select_zero_remaps_direction(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.slot.map_select = 0;
  press_key(&mr, FIFA96_ENGINE_KEY_RIGHT);   /* 0x04 -> map row 6 */
  grant_frame(&mr);
  assert(mr.input.prev[0] == 0x04);          /* input model stays raw */
  assert(mr.slot.raw == 0x04);
  assert(mr.slot.prev_mapped == fu70_map[0x4]);   /* 0x06 */
  assert(mr.slot.pressed == fu70_map[0x4]);
  assert(mr.slot.anim_a == fu70_t1[0x6]);    /* 4 */
  assert(mr.slot.anim_b == fu70_t2[4]);      /* 255 */
  assert(mr.slot.anim_c == fu70_t3[4]);      /* 255 */
}

/* Slot cadence: one update per granted 30 Hz frame with the FU-62 §4.3 whole
 * frame delta (2); polls and no-grant pace ticks never touch the slot. */
static void test_slot_updates_once_per_granted_frame(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;             /* class 1: the clock always runs */
  mr.state.period_length = 90;    /* no period end inside the test window */

  press_key(&mr, FIFA96_ENGINE_KEY_KICK);
  assert(mr.slot.counter == 0 && mr.slot.pressed == 0);

  /* The first 3 pace ticks accumulate without a grant. */
  assert(frame_tick(&mr) == 0);
  assert(frame_tick(&mr) == 0);
  assert(frame_tick(&mr) == 0);
  assert(mr.slot.counter == 0 && mr.slot.pressed == 0);

  /* The 4th tick grants: exactly one update, delta = frame_delta = 2. */
  assert(frame_tick(&mr) == 1);
  assert(mr.state.frame_delta == 2);
  assert(mr.slot.raw == 0x10 && mr.slot.pressed == 0x10);
  assert(mr.slot.counter == 2);

  /* No-grant ticks between frames do not touch the slot. */
  assert(frame_tick(&mr) == 0);
  assert(frame_tick(&mr) == 0);
  assert(mr.slot.counter == 2);
  assert(frame_tick(&mr) == 1);
  assert(mr.slot.counter == 4);

  /* A poll between grants changes the sample but not the slot. */
  press_key(&mr, FIFA96_ENGINE_KEY_PASS);
  assert(mr.slot.counter == 4 && mr.slot.raw == 0x10);
  grant_frame(&mr);
  assert(mr.slot.raw == 0x20 && mr.slot.pressed == 0x20);
  assert(mr.slot.counter == 6);
}

/* 1000 pace ticks (10 s) grant exactly 300 frames; with delta 2 the FU-70
 * counter advances 60 units/s up to its 0xFA cap. */
static void test_300_grants_advance_slot_by_frame_delta(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  press_key(&mr, FIFA96_ENGINE_KEY_KICK);

  int grants = 0;
  for (int i = 0; i < 1000; i++) grants += frame_tick(&mr);
  assert(grants == 300);
  assert(mr.state.frame_delta == 2);
  assert(mr.slot.counter == 0xFA);   /* +2 per frame until the FU-70 cap */
  assert(mr.slot.raw == 0x10);
}

struct fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
};

static struct fixture make_fixture(const fifa96_platform_key *tape, size_t tape_len) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.tape = tape;
  pcfg.tape_len = tape_len;
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per engine step */
  struct fixture f;
  f.plat = fifa96_platform_null_create(&pcfg);
  assert(f.plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  f.engine = fifa96_engine_create(&ecfg, f.plat);
  assert(f.engine != NULL);
  assert(fifa96_engine_boot(f.engine) == 0);
  return f;
}

static void drop_fixture(struct fixture f) {
  fifa96_engine_destroy(f.engine);
  fifa96_platform_destroy(f.plat);
}

/* MATCH mode polls once per engine step and the slot only advances on the
 * 100 Hz clock ticks that grant a 30 Hz frame. The tape holds RIGHT across the
 * 8 steps so the frame-body sample at the grant ticks stays live. One step is
 * exactly one PIT tick here, so the grants land on ticks 4, 7 and 10. */
static void test_engine_step_polls_match_input(void) {
  fifa96_platform_key tape[8];
  for (int i = 0; i < 8; i++) {
    tape[i].raw_code = FIFA96_ENGINE_KEY_RIGHT;
    tape[i].state = 1;
  }
  struct fixture f = make_fixture(tape, 8);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);

  /* Step 1 samples RIGHT; the step's tick does not grant, so the slot is
   * untouched (poll != frame update). */
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.input.prev[0] == 0x04);
  assert(mr.input.edge[0] == 0x04);
  assert(mr.input_state[0] == 0x04);
  assert(mr.slot.counter == 0 && mr.slot.raw == 0);

  /* Steps 2-3: still no grant. */
  assert(fifa96_engine_step(f.engine) == 0);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.slot.counter == 0);

  /* Step 4: the 4th tick grants, one slot update with delta 2. */
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.slot.raw == 0x04);
  assert(mr.slot.pressed == 0x04);
  assert(mr.slot.counter == 2);
  assert(mr.slot.anim_a == fu70_t1[0x4]);

  /* Steps 5-6: no grant; step 7 grants a second update (same sample). */
  assert(fifa96_engine_step(f.engine) == 0);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.slot.counter == 2);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.slot.counter == 4);
  assert(mr.slot.pressed == 0);    /* unchanged sample: no new edge */

  /* Steps 8-10: the tape runs out (release derived); the step-10 grant
   * reports the direction-only release and resets the counter. */
  assert(fifa96_engine_step(f.engine) == 0);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.input.prev[0] == 0);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.slot.raw == 0 && mr.slot.released == 0);
  assert(mr.slot.counter == 0);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* QUIT is an engine key: MATCH mode quits the engine without feeding it to the
 * match input model. */
static void test_engine_step_quit_in_match(void) {
  const fifa96_platform_key tape[1] = {{FIFA96_ENGINE_KEY_QUIT, 1}};
  struct fixture f = make_fixture(tape, 1);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(fifa96_engine_should_quit(f.engine) == 1);
  assert(mr.input.prev[0] == 0 && mr.slot.raw == 0);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

int main(void) {
  test_init_initializes_input_and_slot();
  test_null_guards();
  test_direction_codes_and_animation();
  test_button_and_diagonal_codes();
  test_edges_fire_once_per_press();
  test_slot_hold_machine_and_counter();
  test_direction_release_reports_no_button();
  test_map_select_zero_remaps_direction();
  test_slot_updates_once_per_granted_frame();
  test_300_grants_advance_slot_by_frame_delta();
  test_engine_step_polls_match_input();
  test_engine_step_quit_in_match();
  puts("test_engine_match_input OK");
  return 0;
}
