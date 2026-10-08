#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_OUTFIELD_CODE_COUNT 5

typedef struct fifa96_outfield_rule {
  uint16_t mask;
  uint16_t want;
  uint32_t handler;
} fifa96_outfield_rule;

typedef int (*fifa96_outfield_rule_fn)(uint32_t handler, void *context);

const fifa96_outfield_rule *fifa96_outfield_pressed_rules(uint8_t code);
const fifa96_outfield_rule *fifa96_outfield_released_rules(uint8_t code);
int fifa96_outfield_rule_match(const fifa96_outfield_rule *rule, uint16_t word);
int fifa96_outfield_rules_run(const fifa96_outfield_rule *rules, uint16_t word,
                              fifa96_outfield_rule_fn call, void *context,
                              uint32_t *handler);

typedef struct fifa96_outfield_edge {
  uint16_t pressed;
  uint16_t released;
  uint8_t type;
  uint8_t high_577ee_ge_50;
  uint8_t tracked;
  uint8_t user_absent_or_self;
  uint8_t chaser;
} fifa96_outfield_edge;

int fifa96_outfield_dispatch_code(const fifa96_outfield_edge *edge, uint8_t *code);

typedef struct fifa96_outfield_forced_state {
  uint8_t is_team_controlled;
  uint8_t is_team_second;
  uint8_t type_5;
  uint8_t opponent_has_ball;
  uint8_t controlled_has_ball;
} fifa96_outfield_forced_state;

int fifa96_outfield_forced_action(const fifa96_outfield_forced_state *state, uint8_t current,
                                  uint8_t *next);

typedef struct fifa96_outfield_chase_state {
  uint8_t phase;
  uint8_t type_gate;
  uint8_t not_team_controlled;
  uint8_t not_team_second;
  uint16_t distance;
  uint16_t camera;
  uint8_t user_present;
  uint8_t sides_differ;
  uint8_t unbound;
  uint16_t timer;
  uint8_t third_zero;
} fifa96_outfield_chase_state;

int fifa96_outfield_chase_action(const fifa96_outfield_chase_state *state, uint8_t current,
                                 uint8_t *next);

/* ===== FU-75 §1.2/§1.3/§1.7 + FU-137 §4.1 (M2 Task 14 / OL-38): the outfield
 * record machine's per-record decision `FUN_0007CA54` subset. The native
 * machine (`0x7CA54..0x7CD53`, FU-75 §1) runs, for one record:
 *   - the input-row dispatch (`0x7CB0D..0x7CC11`): when the record owns a
 *     control slot (`+0x20`, `0x7CABA`), the pre-gate `0x7CAC4..0x7CB08`
 *     (byte `[0x157AB0]`, `rec == [0x1587AC]`, type 3, the released bit
 *     `0x20`) may call the `0x7D1D4` selection handler directly; otherwise the
 *     raw slot words `[+4]`/`[+6]` select a dispatch code 0..4 (`0x7CB22`,
 *     `fifa96_outfield_dispatch_code`) and either the pressed word
 *     (`&0xFF0`, pressed table) or — only when the pressed word is zero — the
 *     released word (released table) runs the row scan. The scan stops (to the
 *     tail) on a matching row with handler 0 or a handler returning non-zero
 *     (`0x7CBAC..0x7CBB0`, `0x7CBC6`); it never runs both tables;
 *   - the no-edge arm (`0x7CC13..0x7CC7D`): both raw words zero, `(slot[+0x10]
 *     & 0xF0) != 0`, phase 2, and either `0x30 < (int16)(lane>>16) < 0x90` or
 *     the side-filtered `slot[+0x10] & 0xC0` path; fires the camera-triple copy
 *     into `+0x4D..+0x55` and `FUN_00079B58` (`receiver_timer`);
 *   - the phase-2 tail (`0x7CC82..0x7CD24`): the per-type gate flat
 *     `0x110680[type]&1`, then `FUN_0007C990` (`fifa96_outfield_forced_action`)
 *     and the code-8 chase gate (`fifa96_outfield_chase_gate`).
 * The machine's final `CALL [rec+0x18]` + `FUN_0006E8E8`/`FUN_00079B1C`/
 * `FUN_0007BF20` tail is the engine dispatch itself and is not re-modelled
 * here. */
typedef struct fifa96_outfield_input_state {
  uint8_t has_slot;      /* record +0x20 != 0 */
  uint8_t phase;         /* [0x157A4A]>>24 */
  uint16_t pressed;      /* slot[+4] raw word */
  uint16_t released;     /* slot[+6] raw word */
  uint16_t slot_word10;  /* slot[+0x10] word */
  int32_t lane;          /* dword record[+0x69] (>>16 = the lane word) */
  uint8_t user_present;  /* [0x157A83] != 0 */
  uint8_t user_side;     /* [[0x157A83]]+0x826 */
  uint8_t side;          /* record's team +0x826 */
  uint8_t type;          /* record +0x8E>>24 */
  uint8_t high_577ee_ge_50;
  uint8_t tracked;
  uint8_t user_absent_or_self;
  uint8_t chaser;
  uint8_t flag_157ab0;   /* byte [0x157AB0] != 0 (0x7CAC4) */
  uint8_t is_1578ac;     /* record == [0x1587AC] (0x7CACD) */
  fifa96_outfield_forced_state forced;
  uint8_t current_code;  /* record +0x91 */
  fifa96_outfield_chase_state chase;
} fifa96_outfield_input_state;

typedef struct fifa96_outfield_input_out {
  uint8_t direct_arm;    /* the 0x7D1D4 pre-gate fired (0x7CB03) */
  uint8_t no_edge;       /* both slot words zero */
  uint8_t scan_ran;      /* a pressed/released row scan ran */
  uint8_t scan_code;     /* the selected dispatch code 0..4 */
  uint8_t scan_stopped;  /* a row handler accepted (returned non-zero) */
  uint8_t no_edge_arm;   /* the camera copy + FUN_00079B58 request fired */
  uint8_t forced;        /* FUN_0007C990 install request */
  uint8_t forced_code;
  uint8_t chase;         /* the code-8 chase gate install request */
} fifa96_outfield_input_out;

int fifa96_outfield_input_row(const fifa96_outfield_input_state *state,
                              fifa96_outfield_rule_fn call, void *context,
                              fifa96_outfield_input_out *out);

/* The code-8 chase gate: the per-type gate flat `0x110680[type]&1` composed
 * with `fifa96_outfield_chase_action` (FU-75 §1.6). Returns 1 when code 8 is
 * requested, 0 when gated out or already current, -FIFA96_ERR_INVALID on NULL.
 * `type` beyond 0x19 returns 0 (the native reads adjacent image bytes there;
 * no real record type reaches it). */
int fifa96_outfield_chase_gate(const fifa96_outfield_chase_state *state, uint8_t type,
                               uint8_t current, uint8_t *next);
