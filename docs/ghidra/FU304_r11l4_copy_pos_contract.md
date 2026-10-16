# FU304 — R11-L4: the `copy_pos` apply + the ladder→seam output contract

**Phase/Provenance:** M2 phase-28 recon-ahead, Task 7 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive; confirmed this session by
`ghidra_get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay,
2706 functions). Read-only: no rename/comment/label/function-create/script/save; the binary,
the ISO and git untouched. Every native address/byte below is a **fresh** `disassemble_bytes` /
`get_xrefs_to` / `get_function_by_address` of *this* session unless marked *carried* (source
named). Engine-source facts cite the built tree by line (read-only). No file is modified except
this draft. Closes (as a derivation) **FU290-L10** — the report-but-unapplied `out->copy_pos`.

**One line.** The ladder's `+0x81` arm (`0x77F16..0x77F29`, fresh) is a 3-dword
`pos→target` copy (`+0x59/+0x5D/+0x61 → +0x4D/+0x51/+0x55`) that exits at the shared
epilogue `0x782C4` with **no install, no stage machine, no latch write**; its port
(`out->copy_pos`, `src/fifa96_loader/fifa96_keeper_cpu.c:240-243`) is correct but the
FU296 Seam A never applies it. The seam must write `e->target_x/y/z = e->pos_x/y/z`
before the single record dispatch. The full step-output contract the seam must
consume/apply is §3; the `latch_written`-class publish defects (`[0x157AB2]`,
`[0x157C5D]`) and the never-published `[0x157C2C]` stamp are named as legs.

---

## 1. The native copy — fresh bytes

Fresh `disassemble_bytes 0x77EB7` (140 B) gives the exact `0x77F16..0x77F29` window:

```
0x77F16  CMP word[EBP+0x81], 0        ; 6683bd8100000000   the +0x81 countdown word
0x77F1E  JZ  0x77F2E                  ; 740e   ==0 -> the actor/side skip -> stage machine
0x77F20  LEA EDI, [EBP+0x4D]          ; 8d7d4d   dest = target triple (+0x4D)
0x77F23  LEA ESI, [EBP+0x59]          ; 8d7559   src  = position triple (+0x59)
0x77F26  MOVSD ES:EDI, ESI            ; a5
0x77F27  MOVSD ES:EDI, ESI            ; a5
0x77F28  MOVSD ES:EDI, ESI            ; a5       12 bytes = 3 dwords
0x77F29  JMP 0x782C4                  ; e996030000
```

Fresh `disassemble_bytes 0x782C4` (16 B) confirms `0x782C4` is `FUN_00077EAC`'s shared
**epilogue** — `ADD ESP,0x14; POP EBP; POP EDI; POP ESI; POP EDX; POP ECX; POP EBX; RET`
(`83c414 5d 5f 5e 5a 59 5b c3`) — i.e. the copy arm returns from the ladder with the record
untouched except the triple, and the caller `FUN_000782D0` proceeds to its single record
dispatch (`0x785A8 CALL dword[EBP+0x18]`, fresh §3-S2). No `FUN_0007D9A4` install, no
`[0x157C5C]` write (`0x7802D`/`0x780D7` are inside the stage-select block this arm never
reaches), no `[0x157C2C]` stamp.

**Engine mapping.** `pos_x/pos_y/pos_z` = native `+0x59/+0x5D/+0x61`;
`target_x/target_y/target_z` = native `+0x4D/+0x51/+0x55`
(`include/fifa96_engine/fifa96_match_entities.h:123` / `:139`). So the exact apply is
`e->target_x = e->pos_x; e->target_y = e->pos_y; e->target_z = e->pos_z;`.

## 2. The copy's preconditions (all fresh-verified this session unless marked)

The chain in execution order; each closed gate jumps to the shared epilogue `0x782C4`:

| # | gate | fresh site / bytes | native test | port | live source / status |
|---|---|---|---|---|---|
| 1 | permit | `0x784D3..0x784E9` (`8a8080061100 2401 … 0f84af000000`) | `[0x110680][code] & 1`; `JZ 0x7859E` skips the install tree **and** the ladder call | `if (rec->permit != 0)` (`fifa96_keeper_cpu.c:50`) | `mr->keeper_cpu.permit` — P1 unported, **zero live** (FU282-L5/FU296-L2; no `src/` producer, only tests set it, fresh grep) |
| 2a | ladder gate phase | `0x7857B..0x78586` (`a14a7a1500 … 7516`) | `[0x157A4A]>>0x18 == 2` | `rec->phase == 2` (`:65`) | `mr->state.phase`, live |
| 2b | no queued action | `0x78588` (`837d2000`) | `dword[rec+0x20] == 0` | `rec->queued20 == 0` | `e->has_slot`, live |
| 2c | interlock | `0x7858E..0x78595` (`803d5d7c150000`) | `byte[0x157C5D] != 0` | `cpu->interlock_157c5d != 0` | always 0 live (the co-blocker, FU304-L5) |
| 2d | the call | `0x78597/0x78599` (`89e8 e80ef9ffff`) | `CALL 0x77EAC` | `cpu->ladder(...)` | Seam A (entities.c:780), linked (`CMakeLists.txt:445`) |
| 3 | keeper row | `0x77EB7..0x77EC5` (`8b808e000000 c1f818 83f819 0f85f9030000`) | `byte[rec+0x91] == 0x19` | `rec->code != 0x19u` return (`:230`) | `e->code`, live |
| 4 | FU291 bit-9 gate | `0x77ECB..0x77EF0` (`668b0445d4c11400 30c0 80e402 … 0f85ce030000`) | `word[0x14C1D4+idx*2]`, `AND AH,2` must be **0** | `((rec->input_range[idx] >> 9) & 1u) != 0` return (`:234-236`) | `rec->input_range` — caller-supplied, **the seam leaves it zero** (FU304-L4) |
| 5 | skip state | `0x77EF6..0x77EFD` (`80bd9a00000000`) | `byte[rec+0x9A] == 0` | `rec->gate_9a != 0` return (`:238`) | `e->skip_9a`, live |
| 6 | 'T' descriptor | `0x77F03..0x77F10` (`8b4528 8a00 … 83f854 0f84ae030000`) | `byte[[rec+0x28]] != 0x54` | `rec->is_T != 0` return (`:239`) | caller-supplied 0 (the `+0x28` descriptor unmodelled; FU296 §3.2 P8) |
| 7 | **the trigger** | `0x77F16..0x77F1E` (`6683bd8100000000 740e`) | `word[rec+0x81] != 0` | `rec->timer81 != 0` → `out->copy_pos = 1` (`:240-243`) | `e->timer81`, live |

Same-frame install-tree interaction (fresh `0x784EF..0x78505`, `668b9581000000 … 31c0`):
`word[+0x81] != 0` **also forces the deferred install target to 0** (`XOR EAX,EAX` at
`0x78503`), so with `code = 0x19` a live timer both installs code 0 (deferred,
`0x78576`) and takes the copy arm — the record then dispatches row 0 with the freshly
copied target. The copy arm and the stage-select `copy_valid` arm are mutually exclusive
by construction (the copy arm returns before `fifa96_keeper_cpu_stage_select`); the port
already keeps `ran = 0` on this arm (`tests/test_keeper_cpu.c:556-561`), which correctly
suppresses the `latch_written` publish (`fifa96_keeper_cpu.c:73-76`) — the native reaches
neither `0x7802D` nor `0x780D7`.

**Live reach today.** The copy is **double-locked dead live**: (1) `permit == 0`
(FU296-L2/FU282-L5), and (2) even with a permit, the interlock `[0x157C5D]` is always 0
on the ported path (FU304-L5). So applying `copy_pos` in the seam is a **zero-golden-delta**
change (M1/M2 stay byte-identical) until P1 lands.

## 3. The exact step-output contract (what the seam must consume/apply)

The seam = the `i == 0` branch of `fifa96_match_entities_team_update`
(`src/fifa96_engine/fifa96_match_entities.c:741-791`, entity applies before the one
`action(ctx, e)`) plus the frame bind/publish sites in `fifa96_match_run.c`.
Every step output, its native owner, and its required apply/publish:

### (A) Entity applies (must land before the single dispatch)

| output | native owner (fresh) | required apply | engine status |
|---|---|---|---|
| `out.install` + `install_code` | `0x78576 CALL 0x7D9A4` with `EAX=EBP, EDX=target, EBX=0, ECX=0` (`0x78567/0x7856A/0x78574/0x78576`, fresh) — **deferred** | `e->code = install_code` | **applied** (entities.c:781) |
| `ladder_out.install` + `install_code` + `invoke_now` | the five arms `0x78227`/`0x7824F`/`0x78277` (0x1A, stamps `0x78230/58/7E`), `0x78299` (0x1B), `0x782B6`/`0x782BB` (0x1C/4) — `FUN_0007D9A4(rec, code, 0, ECX=1)` **invoke-now** (carried FU290 §dispatch; arm bytes re-pinned FU186 §1) | `e->code = install_code` | **applied** (entities.c:782-783); `invoke_now` itself unconsumed (FU296-L8, FU304-L6) |
| `ladder_out.copy_valid` + `copy_x/y/z` | the `+0x4D` copy sites `0x7805B` (CX=1), `0x780A5` (CX=4), `0x781D4` (CX=A) (carried FU290/FU296 T5 closure) | `e->target_x/y/z = copy_x/y/z` | **applied** (entities.c:786-790) |
| **`ladder_out.copy_pos`** | **`0x77F20..0x77F28`** (fresh, §1) — `pos(+0x59) → target(+0x4D)`, 3 dwords, then epilogue `0x782C4`: no install | **`e->target_x/y/z = e->pos_x/y/z`** | **NOT applied** — the seam consumes only `copy_valid` (fresh read); this task's gap, **FU290-L10** |
| `out.force_install` (`cpu->force_install`) | `0x7856C MOV byte[0x157AB2],BH` (`BH=1` set at `0x78565`), before the deferred install | publish to the run cell `mr->keeper_latch_157ab2` | **frame-level only** (run.c:4251 read / :4259 write-back) — ordering leg FU304-L2 |

### (B) `latch_written`-class publishes (native single process cells with same-frame readers)

| publish | native owner (fresh) | same-frame reader | engine status |
|---|---|---|---|
| `cpu->latch_157c5c` + `latch_written` | `[0x157C5C]`: clear `0x7802D`, set `0x780D7` (carried FU290/FU292; the port emits `ladder_out.latch_157c5c`) | row-0x1B body `0x76FF4` (carried FU296) | **in-frame published** (run.c:1415-1418) — the working model |
| `cpu->interlock_157c5d = 0` | `0x785A2 MOV byte[0x157C5D],CH` (fresh; `CH` zeroed at `0x7859E`) — unconditional, reached via the `0x7859E` join | next frame's gate `0x7858E` (fresh `get_xrefs_to 0x157C5D` = 3: read `0x7858E`, clear `0x785A2`, set `0x74707`); setter = the row-0x19 body (`fifa96_match_handlers.c:4097`) | **frame-level write-back** (run.c:4257) that erases the same-walk row-0x19 set — the phase-27 co-blocker; re-pinned FU304-L5 |
| `ladder_out.stamp_157c2c` | `[0x157C2C]`: writes `0x78230`/`0x78258`/`0x7827E` (fresh `get_xrefs_to 0x157C2C` = 10) | the row-0x1A direction machine `0x766FC`/`0x7674C`/`0x76757`/`0x767A9`/`0x767E7`/`0x76960`/`0x76A12` (fresh, same xref list) | **not published** — no `[0x157C2C]` cell exists in `src/`/`include/` (fresh grep: only the output field + unit tests); consumer unported (FU186-L1) — FU304-L3 |

### (C) Internal / diagnostics (no entity apply; must not be treated as actions)

`ladder_out.ran` (the `0x77FF5` AX gate; drives the `latch_written` publish inside
`fifa96_keeper_cpu_step`), `ladder_out.stage` (the computed `CX`; the T8 erratum stands —
no `kc_rec.stage`, FU290/FU296 T8), `kc_out.ladder` (the `0x78599` gate fired, a
diagnostic). `ladder_out.install_code` is only meaningful with `install != 0`.

**Frozen apply shape (the seam promotion's target):**

```c
if (kc_out.ladder_out.install)                 /* existing */
  e->code = kc_out.ladder_out.install_code;
if (kc_out.ladder_out.copy_valid) {            /* existing (FU290) */
  e->target_x = kc_out.ladder_out.copy_x;
  e->target_y = kc_out.ladder_out.copy_y;
  e->target_z = kc_out.ladder_out.copy_z;
}
if (kc_out.ladder_out.copy_pos) {              /* NEW — 0x77F20, no install */
  e->target_x = e->pos_x;
  e->target_y = e->pos_y;
  e->target_z = e->pos_z;
}
```

Both target writers sit before `action(ctx, e)` — the native `0x78599`-before-`0x785A8`
order (fresh 0x78597..0x785A8). `copy_pos` and `copy_valid` are mutually exclusive, so
their relative order is immaterial; both are single-assignment applies.

## 4. Evidence table (fresh this session)

| claim | fresh evidence |
|---|---|
| the copy `0x77F16..0x77F29`: `CMP word[+0x81],0; JZ 0x77F2E; LEA EDI,[EBP+0x4D]; LEA ESI,[EBP+0x59]; MOVSD x3; JMP 0x782C4` | `disassemble_bytes 0x77EB7` (140 B) |
| `0x782C4` = `ADD ESP,0x14` + 6 pops + `RET` (the shared epilogue) | `disassemble_bytes 0x782C4` (16 B) |
| the permit gate `[0x110680][code]&1; JZ 0x7859E` (skips install tree + ladder) | `disassemble_bytes 0x784C0` (96 B) |
| install tree: `word[+0x81]!=0 -> target 0` (`0x784FE..0x78505`); `0x78565 BH=1`; `0x7856C [0x157AB2]=BH`; `0x78567 MOVSX EDX,AX`; `0x7856A XOR ECX,ECX`; `0x78576 CALL 0x7D9A4` | `disassemble_bytes 0x784C0` (96 B) + `disassemble_bytes 0x78540` (120 B) |
| ladder gate order: `0x7857B` phase, `0x78588 dword[rec+0x20]`, `0x7858E [0x157C5D]`, `0x78599 CALL 0x77EAC`, `0x785A2 [0x157C5D]=0`, `0x785A8 CALL [EBP+0x18]` | `disassemble_bytes 0x78540` (120 B) |
| `get_xrefs_to 0x157C5D` = 3: read `0x7858E`, clear `0x785A2`, set `0x74707` | `get_xrefs_to 0x157C5D` |
| `get_xrefs_to 0x157AB2` = 17 (writers incl. `0x74101`, `0x7856C`, `0x7501F..`, `0x76077`; readers `0x7514A`, `0x751FB`, `0x754BC`, `0x75826`, `0x758CD`, `0x75AB7`, `0x75BAD`, `0x760DF`) | `get_xrefs_to 0x157AB2` |
| `get_xrefs_to 0x157C2C` = 10 (writers `0x78230/58/7E`, `0x7674C/767A9`; readers `0x766FC/76757/767E7/76960/76A12`) | `get_xrefs_to 0x157C2C` |
| `FUN_000782D0` body `0x782D0..0x785D0` (the machine owns the `0x7856C`/`0x78576` writes) | `get_function_by_address 0x7856C` |
| the port reports but the seam drops `copy_pos` (`out->copy_pos=1` at `fifa96_keeper_cpu.c:241`; seam consumes `install`/`install_code`/`copy_valid`, entities.c:741-791) | engine read; `grep copy_pos` (loader-only + tests) |
| `mr->keeper_cpu.permit` never raised live (init 0 `fifa96_keeper_cpu.c:14`; no `src/` writer; tests assert "never raised live") | `grep` src/tests; `test_engine_match_frame.c:3590/3612` |
| `keeper_latch_157ab2` live readers = rows 0x1E/0x1D staging (handlers.c:449/610), write-back 522/652; frame bind run.c:4251/4259 | engine read |
| `[0x157C2C]` has no engine cell | `grep 157c2c` (loader + tests only) |
| `fifa96_keeper_cpu.c` is in the engine link (`CMakeLists.txt:445`) — FU296-L1 closed | `grep keeper_cpu CMakeLists.txt` |
| seam binds (interlock/latch/force/side_xor, read->walk->write) and the in-frame `latch_written` publish | `src/fifa96_match_run.c:4249-4259`, `:1415-1418` |

**Carried (named, not re-derived):** FU290 §dispatch (the arm map / `ECX=1` invoke-now),
FU290/FU296 T5 closures (the `copy_valid` sites `0x7805B/0x780A5/0x781D4`; the
`[0x157C5C]` writes `0x7802D/0x780D7`; the row-0x1B reader `0x76FF4`), FU186 §1.1 (the
`0x77F16` shorthand; arm table), FU291 (the `[0x14C1D4]` table port), FU296 §3 (P1..P13).

## 5. Numbered legs

* **FU304-L1 — the `copy_pos` apply at the seam (CLOSES FU290-L10 when landed).**
  Owner: the phase-28 plan (Seam A promotion); native owner `0x77F20`. The apply is
  exactly specified (§3-A, §3 shape) and **zero-δ live** (double-locked: P1 and the
  interlock). The machine-level unit already pins the output
  (`tests/test_keeper_cpu.c:556-561`: `copy_pos==1 && ran==0 && install==0`); the
  seam-level RED-first test (stage `permit=1`, `timer81!=0`, `code=0x19`, the three
  clear gates; assert the target triple follows the position) is the plan's item.
* **FU304-L2 (new) — the `[0x157AB2]` force-latch publish ordering.**
  Native `0x7856C` (fresh, mid-machine write; readers the 8 sites in `0x75xxx`, fresh
  xref list) vs the engine's frame-level write-back (run.c:4259). Net: a same-walk row
  body (rows 0x1E/0x1D, handlers.c:449/610) sees the pre-frame value, and the write-back
  overwrites the bodies' own same-walk writes (handlers.c:522/652). Zero live
  (`permit=0`); P1 landing makes it matter. Owner: the seam promotion / FU296.
* **FU304-L3 — the `[0x157C2C]` stamp publish.** Fresh: the three arm writers
  `0x78230/0x78258/0x7827E`; the same-frame reader is the row-0x1A direction machine
  (`0x766FC/0x76757/0x767E7/0x76960/0x76A12`). No engine cell; the consumer is
  FU186-L1-unported and the row-0x1A slot stays NULL (FU296-L7). `out->stamp_157c2c`
  stays report-only. Owner: FU186-L1 / the row-0x1A wiring.
* **FU304-L4 (new) — the `input_range` producer wiring at the seam.**
  The FU291 bit-9 gate reads `[0x14C1D4+idx*2]` (fresh `0x77EDE` bytes); the engine has
  `mr->input_range` live (`fifa96_match_run.c:484/2296`, carried FU291) but the Seam A's
  `kc_rec` leaves `input_range[]` zero (fresh read, entities.c:741-779), so the gate's
  honest live value is unproven (zeros open the gate). Owner: the seam promotion
  (with FU291-L2's prepass glue).
* **FU304-L5 — the interlock arming order (re-pinned).** Fresh `get_xrefs_to 0x157C5D`
  = 3: read `0x7858E`, clear `0x785A2` (the record-0 step), set `0x74707` (the row-0x19
  body, `fifa96_match_handlers.c:4097`). The runtime set runs inside the same walk; the
  run.c:4257 write-back of the machine's cleared view erases it before the next frame's
  bind (run.c:4249) reads it. The phase-27 co-blocker; owner FU296-L3 / the seam.
* **FU304-L6 — the invoke-now reconciliation (carried FU296-L8).** The ladder's `ECX=1`
  install runs the row at install time and again at `0x785A8`; the engine's single
  dispatch runs it once. Only carried here — no new evidence; owner FU296-L8.

## 6. GO / NO-GO

**The `copy_pos` apply: GO.** The copy is exactly specified (fresh bytes), the port's
output is already correct, and the seam apply is a two-line, zero-golden-delta change
(double-locked dead live by P1 and the interlock). **The end-to-end live install:
unchanged NO-GO** — P1 `[0x110680]` (FU296-L2) **and** the interlock ordering
(FU304-L5) remain the blockers; this task adds the `[0x157AB2]` publish ordering
(FU304-L2) and the `input_range` wiring (FU304-L4) to the same promotion's checklist.
Census unmoved; no producer faked.
