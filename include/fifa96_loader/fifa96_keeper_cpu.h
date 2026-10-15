#pragma once
#include <stdint.h>

#include "fifa96_loader/fifa96_err.h"

/* FU282 — the R11-L4 record-0 machine `FUN_000782D0` (`0x782D0..0x785D0`,
 * 224 insns; sole caller `0x8DB35` in `FUN_0008D8EC`). This port is the
 * bounded SKELETON FU282 rules GO: the per-actor counter head (§1.1), the
 * deferred action install (§1.3), and the ladder call gate (§1.4). The ladder
 * `FUN_00077EAC` itself was the FU282-L1 no-op seam; FU283 (Task 8) ports the
 * ladder body into it (`fifa96_keeper_cpu_ladder_run`).
 * Evidence: `docs/ghidra/FU282_r11l4_record0_machine.md`,
 * `docs/ghidra/FU283_r11l4_ladder.md`. */

/* The machine's per-frame inputs for one record (`rec`). The install tree's
 * possession chain (`[[rec]+0x7B2]`, `[[[rec]]+0x7A6]`) and the `0x110680`
 * install-permission table are unported producers (FU282-L3/L5), so the
 * caller supplies their resolved booleans; the native comparison results they
 * stand in for are named per field. */
typedef struct fifa96_keeper_cpu_record {
  uint8_t side;         /* byte +0x826 — the XOR-side source */
  uint8_t code;         /* byte +0x91 — the current action code */
  uint8_t phase;        /* `[0x157A4A] >> 0x18` */
  uint8_t is_actor;     /* `rec == [0x157A83]` */
  uint8_t permit;       /* `[0x110680][code] & 1` (table unported, FU282-L5) */
  uint8_t ball_owner;   /* `rec == [[rec]+0x7B2]` (possession, FU282-L3) */
  uint8_t owner_rel_5;  /* owner-related record `[+0x91] == 5` (FU282-L3) */
  uint16_t timer81;     /* word +0x81 (non-zero => target 0, `0x78507`) */
  uint32_t queued20;    /* dword +0x20 (the ladder-gate queued-action cell) */

  /* FU283 (Task 8) — the ladder `FUN_00077EAC` inputs (`0x77EB7..0x77F51`). */
  uint8_t side_swap;         /* `[0x157ABE]` — the side-swap cell; its producer
                              * (`FUN_0007417c`, `(byte[0x157AC2]^byte[0x157AC3])&1`)
                              * is UNPORTED (FU291-L1), the caller supplies the
                              * available match-state value */
  uint16_t input_range[2];   /* `[0x14C1D4]` — the per-side word table
                              * (`fifa96_input_range_words`); the FU291 gate reads
                              * bit 9 of `input_range[(side ^ side_swap) & 1]` */
  uint8_t gate_9a;           /* byte +0x9A — the skip state (0x77EF6) */
  uint8_t is_T;              /* `*(param_1[10]) == 'T'` (0x77F03) */
  uint8_t actor_present;     /* `[0x157A83] != 0` (0x77F3C) */
  uint8_t actor_same_side;   /* `[0x157A83]` `+0x826` == rec `+0x826` (0x77F45) */
  uint8_t stage;             /* `CX` ∈ 1..5 — the stage-select index. UNPORTED
                              * producer (FU283-L6; the caller supplies it; 0 =
                              * no stage, the honest live value) */
  uint8_t tracked_present;   /* `[0x1577CA] != 0` — `camera.tracked` (0x765C7) */
  uint8_t tracked_same_side; /* the tracked record's `+0x826` == rec `+0x826` */
  uint8_t tracked_code;      /* the tracked record's byte +0x91 (0x765E3) */
  uint8_t global_158740;     /* `[0x158740] >> 0x18` — UNPORTED gate (0x765FA) */
} fifa96_keeper_cpu_record;

/* FU283 §4/§5 — the ladder's products. Every installer is
 * `FUN_0007D9A4(rec, code, 0, ECX=1)`: **invoke-now**, unlike the machine's
 * deferred `ECX=0` install. */
typedef struct fifa96_keeper_cpu_ladder_out {
  uint8_t ran;            /* the head gates + actor/side skip passed (reached the selector) */
  uint8_t copy_pos;       /* the `+0x81` path: `pos +0x59 -> +0x4D`, no install (0x77F20) */
  uint8_t install;        /* an install fired */
  uint8_t install_code;   /* 0x1A / 0x1B / 0x1C / 0x4 */
  uint8_t invoke_now;     /* `ECX=1` (always set with `install`; the FU283 contrast) */
  uint8_t stamp_157c2c;   /* the `0x1A` arms stamp `[0x157C2C]` 0x2C/0x2D/0x2E, else 0 */
} fifa96_keeper_cpu_ladder_out;

/* The machine's products for one record. */
typedef struct fifa96_keeper_cpu_out {
  uint8_t install;       /* a deferred install fired (FUN_0007D9A4, EBX=0/ECX=0) */
  uint8_t install_code;  /* the target code (0x0 / 0x19 / 0x4) */
  uint8_t force_install; /* `[0x157AB2] := 1` (the force-install latch) */
  uint8_t ladder;        /* the `0x78599` gate fired (the ladder seam was called) */
  fifa96_keeper_cpu_ladder_out ladder_out; /* the ladder's products (FU283) */
} fifa96_keeper_cpu_out;

/* `FUN_00077EAC` (`0x78599 CALL`) — the CPU ladder seam. The default is the
 * ported `fifa96_keeper_cpu_ladder_run`; the ladder cannot raise a live stage
 * index (its selector inputs are unported, FU283-L6), so with the caller's
 * `stage == 0` it returns at `0x781E1` and installs nothing. */
typedef int (*fifa96_keeper_cpu_ladder_fn)(void *ctx,
                                           const fifa96_keeper_cpu_record *rec,
                                           fifa96_keeper_cpu_ladder_out *out);

typedef struct fifa96_keeper_cpu {
  uint8_t head_flag;        /* [0x157823] — the one-shot actor flag */
  uint8_t side_xor;         /* [0x157ABE] — the XOR-side cell */
  uint8_t permit;           /* `[0x110680][code] & 1` (P1, FU282-L5) — the
                             * install-permission gate. The per-action-code table
                             * is UNPORTED, so this is a caller-supplied producer
                             * INPUT (default 0 = the honest live value; the
                             * staged reach test drives it). */
  uint16_t counter_ad0[2];  /* [0x157AD0 + idx*2] — the actor-side frame counter */
  uint16_t counter_acc[2];  /* [0x157ACC + idx*2] — the complement-side counter */
  uint8_t force_install;    /* [0x157AB2] — the force-install latch */
  uint8_t interlock_157c5d; /* [0x157C5D] — the ladder GATE (one-shot interlock) */
  uint8_t latch_157c5c;     /* [0x157C5C] — the ladder-internal LATCH (a DISTINCT
                             * cell; this machine never reads or writes it) */
  fifa96_keeper_cpu_ladder_fn ladder; /* the `0x78599` seam (NULL = stub) */
  void *ladder_ctx;
} fifa96_keeper_cpu;

void fifa96_keeper_cpu_init(fifa96_keeper_cpu *cpu);

/* One `FUN_000782D0` step for one record. Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID on a NULL argument. */
int fifa96_keeper_cpu_step(fifa96_keeper_cpu *cpu,
                           const fifa96_keeper_cpu_record *rec,
                           fifa96_keeper_cpu_out *out);

/* FU283 §4/§5 — the ported `FUN_00077EAC` body: the head gates, the actor/side
 * skip, the `FUN_000765C4` code-4 fall-through, and the five install arms
 * (CX-1 through `0x77E98`). The stage index `rec->stage` and the unported
 * `global_158740` selector input are caller-supplied — the selector machine
 * itself stays an open leg (FU283-L6). The `[0x14C1D4]` bit-9 gate is now
 * computed live from `rec->side ^ rec->side_swap` + `rec->input_range`
 * (FU291, phase-27 T2). */
int fifa96_keeper_cpu_ladder_run(void *ctx, const fifa96_keeper_cpu_record *rec,
                                 fifa96_keeper_cpu_ladder_out *out);

/* FU283 §6 — `FUN_000765C4` (`0x765C4`), the tracked-teammate predicate: 1 iff
 * the tracked record is present, on the same side, and (its `byte +0x91 == 5`
 * or `[0x158740]>>24 != 4`). Exported for direct unit testing (the rows-27/29/2C
 * precedent). */
int fifa96_keeper_cpu_tracked_predicate(uint8_t tracked_present,
                                        uint8_t tracked_same_side,
                                        uint8_t tracked_code,
                                        uint8_t global_158740);
