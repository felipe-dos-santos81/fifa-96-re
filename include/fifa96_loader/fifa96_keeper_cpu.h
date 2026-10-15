#pragma once
#include <stdint.h>

#include "fifa96_loader/fifa96_err.h"

/* FU282 — the R11-L4 record-0 machine `FUN_000782D0` (`0x782D0..0x785D0`,
 * 224 insns; sole caller `0x8DB35` in `FUN_0008D8EC`). This port is the
 * bounded SKELETON FU282 rules GO: the per-actor counter head (§1.1), the
 * deferred action install (§1.3), and the ladder call gate (§1.4). The ladder
 * `FUN_00077EAC` itself is a no-op seam here (FU282-L1, Task 8). Evidence:
 * `docs/ghidra/FU282_r11l4_record0_machine.md`. */

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
} fifa96_keeper_cpu_record;

/* The machine's products for one record. */
typedef struct fifa96_keeper_cpu_out {
  uint8_t install;       /* a deferred install fired (FUN_0007D9A4, EBX=0/ECX=0) */
  uint8_t install_code;  /* the target code (0x0 / 0x19 / 0x4) */
  uint8_t force_install; /* `[0x157AB2] := 1` (the force-install latch) */
  uint8_t ladder;        /* the `0x78599` gate fired (the ladder seam was called) */
} fifa96_keeper_cpu_out;

/* `FUN_00077EAC` (`0x78599 CALL`) — the CPU ladder seam. Unported (FU282-L1);
 * the default seam is NULL, so the skeleton neither installs the
 * `0x1A`/`0x1B`/`0x1C` rows nor fabricates a stage. */
typedef void (*fifa96_keeper_cpu_ladder_fn)(void *ctx);

typedef struct fifa96_keeper_cpu {
  uint8_t head_flag;        /* [0x157823] — the one-shot actor flag */
  uint8_t side_xor;         /* [0x157ABE] — the XOR-side cell */
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
