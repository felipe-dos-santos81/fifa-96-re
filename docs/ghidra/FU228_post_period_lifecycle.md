# FU-228 — the post-period lifecycle (OVER→POST→EXIT) and the end-of-match word (phase-21 wave W21-6)

Provenance: M2 phase-21 Stage-1 recon, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, `memory_size 2757479`, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `get_function_by_address` / `get_xrefs_to`
of **this** session unless marked *cross-cited* (then the frozen FU doc is
named). The alias traps are applied (`dword[x]>>16` = the word at `x+2`,
`>>24` = the byte at `x+3`); LE globals sit at `+0x100000` from the file offset;
addresses are `/FIFA96.EXE` runtime addresses. Engine/loader source read
first-hand. **No writes**: no rename, comment, label, function, script or
project save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git
commands.

Scope (W21-6 brief): map the **post-period lifecycle** the phase-21 tape's reach
needs, and the **end-of-match word** `[0x158822]` / `FUN_0008BAF0`
(**FU213-L3**). Steps: (1) read the engine lifecycle `fifa96_match_run`
OVER→POST→EXIT (`resolve`), and **where a period completion currently sends the
run**; (2) read the native end word and its writers/callers (fresh xrefs);
(3) verdict: the minimal mapping the tape (stopping at ET entry) needs, and
whether the end machine is required at all for this phase. This extends FU-64,
FU-143, FU-62, FU-196, FU-213.

---

## Answer in one line

**The end machine is not required for this phase — provided the extra-time flag
`[0x157AC0]` is set at ET entry (see FU228-L6) — but the engine's lifecycle
trigger is wrong for it.** Natively, a period completion does **not** end the
match: the end word `[0x158822]` stays `0` unless `FUN_0008BAF0` (the phase-`0xC`
end machine, the **sole** caller `0x8B664` in the per-frame clock `FUN_0008AF38`)
sets it, and the frame loop `FUN_0004B100` enters the match-over screen
(`[0x5FFC]=2` via `MOV EAX,2; CALL 0x36BC0` at `0x4B244`) **only** when
`[0x158822]==1` *and* the period index is past half-time. The engine instead
marks its lifecycle `OVER` on **every** FU-62 completion
(`fifa96_match_run.c:4161`), so today the **first** period completion exits the
run. The tape's reach (half-time, then a tied regulation end → ET entry) never
sets the end word (at ET entry the extra-time flag is set, so `FUN_0008BAF0`
writes phase `0x13` at `0x8BCC8`, not `[0x158822]=1` at `0x8BCD9`); therefore the
minimal mapping is to **re-point the lifecycle `mark_over` at the native
end-of-match condition** (period-aware, end-word-driven) instead of porting
`FUN_0008BAF0`. No new table row; the census does not move.

---

## 1. The engine post-period lifecycle (fresh, first-hand source)

### 1.1 The three-phase screen pair `OVER(2) → POST(3) → EXIT(4)`

The engine mirrors the native screen machine (`[0x5FFC]`; FU-64 §5) as
`struct fifa96_match_lifecycle` over five screens
(`include/fifa96_loader/fifa96_match_lifecycle.h:6..11`):

```
FIFA96_MATCH_SCREEN_ACTIVE = 0,   /* native [0x5FFC]=0 */
FIFA96_MATCH_SCREEN_OVER   = 2,   /* native [0x5FFC]=2 (mark_over)   */
FIFA96_MATCH_SCREEN_POST   = 3,   /* native [0x5FFC]=3 (resolve_over) */
FIFA96_MATCH_SCREEN_EXIT   = 4;   /* native [0x5FFC]=4 (request_exit) */
```

with leave/resume targets `FIFA96_MATCH_LIFECYCLE_LEAVE 3u` /
`FIFA96_MATCH_LIFECYCLE_RESUME 2u` (`:13..14`). The transitions are pure
(`src/fifa96_loader/fifa96_match_lifecycle.c`):

* `fifa96_match_lifecycle_init` (`:4`) — screen `0`.
* `fifa96_match_lifecycle_mark_over` (`:50`) — `screen := OVER` (native
  `[0x5FFC]=2`).
* `fifa96_match_lifecycle_resolve_over` (`:56`) — iff `screen == OVER`, zero
  `cadence`, `screen := POST`, return `1` (native `[0x5FFC]=3`,
  `0x494CC..0x49532`).
* `fifa96_match_lifecycle_request_exit` (`:64`) — `screen := EXIT`,
  `target := LEAVE` (native `[0x5FFC]=4`, `0x497D3`).
* `fifa96_match_lifecycle_should_exit` (`:71`) — `screen == EXIT`.
* `fifa96_match_lifecycle_end` (`:75`) — hold pace, cancel callback, teardown,
  post-exit when `target == LEAVE`, return `1` iff `target == RESUME`.

### 1.2 The run-level lifecycle chain

`src/fifa96_engine/fifa96_match_run.c`:

* **`fifa96_match_run_frame` (`:4014`)** — the per-frame body (driven by the
  registered 100 Hz trampoline `fifa96_match_run_tick` → `:1723`). It:
  * calls `fifa96_match_state_tick(&mr->state, &mr->pace, 0, 0, &period_ended)`
    (`:4026`) — the FU-62 clock; `period_ended` is set inside
    `fifa96_match_state.c:84..88` (`state.period++` on `period_seconds == limit
    + aux_seconds`, `:77..79`);
  * stages it: `mr->clock_period_ended = (uint8_t)period_ended` (`:4032`);
  * runs the frame body, including `fifa96_match_run_phase_drive` (`:4131`);
  * **at the tail (`:4161..4167`)**:
    ```c
    if (period_ended && !fifa96_match_lifecycle_should_exit(&mr->lc)) {
      rc = fifa96_match_lifecycle_mark_over(&mr->lc);   /* screen := OVER */
    }
    ```
    This is the **only** `mark_over` call in the engine
    (fresh `grep mark_over src include` = `:4165` plus the definition and the
    header decl) and it is gated **only** on the raw FU-62 completion
    (`period_ended`) and `!should_exit` — **period-agnostic**.
* **`fifa96_match_run_step` (`:4184`)** — the engine's MATCH dispatch
  (`fifa96_engine.c:232` calls it once per `fifa96_engine_step`). When the
  screen is `OVER` it drives the whole compression:
  ```c
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER)
    return fifa96_match_run_resolve(mr);      /* :4198..4199 */
  ```
* **`fifa96_match_run_resolve` (`:4171`)**:
  ```c
  if (screen == OVER) resolve_over();          /* OVER -> POST   :4175 */
  if (screen == POST) request_exit();          /* POST -> EXIT   :4178 */
  if (should_exit()) return fifa96_match_run_end(mr);   /* :4180 */
  ```
* **`fifa96_match_run_end` (`:4207`)** — `fifa96_match_lifecycle_end` (`:4210`),
  release stage/meta, `mr->running = 0` (`:4214`), `eng->match = NULL` and
  `eng->mode = FIFA96_ENGINE_MODE_FRONTEND` (`:4217`).

So the engine's post-period lifecycle is **structurally the native screen
machine** (`OVER=2/POST=3/EXIT=4`), but it is **compressed into one step**
(`step` sees `OVER` and calls `resolve`, which runs `OVER→POST→EXIT→end`), and it
is **entered by the raw period completion**, not by the native end word.

### 1.3 Where a period completion currently sends the run (the finding)

On any granted frame where the FU-62 clock completes a period:

```text
fifa96_match_state_frame  -> state.period++ , *period_ended = 1
fifa96_match_run_frame    -> clock_period_ended = 1
                           -> phase_drive(mr)         (chooser: phase 0x0C / 0x13)
                           -> if (period_ended) mark_over()   /* :4161 */
next fifa96_engine_step   -> fifa96_match_run_step -> resolve -> run_end -> FRONTEND
```

The completion itself is **period-index-blind** in the engine: `period 0 -> 1`
(the **half-time** completion) fires the identical `mark_over`/`resolve` path as
the regulation end. The current contract is pinned by
`tests/test_engine_match_completion.c:121`
`test_period_end_resolve_returns_to_frontend` — a 1 s period ending at
`state.period == 1` (i.e. after the first completion) asserts
`mr.lc.screen == FIFA96_MATCH_SCREEN_OVER` (`:134`) and resolve → engine
`FRONTEND` (`:137..146`). The v20 M2 tape does **not** exercise it
(`tests/test_engine_m2.c:3859` — *"no period-end exit exists on this path, so
`res.exit_step` stays `-1`"*), because the tape's staged run never completes a
period; the phase-21 tape extension (FU230) will.

---

## 2. The native end word `[0x158822]` (fresh)

### 2.1 The cell and its complete writer/reader set

`[0x158822]` is a **word**; fresh `read_memory 0x158818` (24 B) is all zero at
file-load (runtime word). Fresh `get_xrefs_to 0x158822` = **10**:

| # | site | function | kind | instruction (fresh) | effect |
|---|---|---|---|---|---|
| R-a | `0x4B106` | `FUN_0004B100` | READ | `CMP word[0x158822],0` | head guard: `!=0` → `EAX=1`, `JMP 0x4B2E1` (frame body skipped) |
| W-a | `0x4B0E7` | `FUN_0004B02C` | WRITE | `MOV word[0x158822],BX` (BX=0) | reset |
| W-b | `0x4B29B` | `FUN_0004B100` | WRITE | `MOV word[0x158822],SI` (SI=0) | the status-`2` arm clears it |
| W-c | `0x8871B` | `FUN_000886D4` | WRITE | `MOV word[0x158822],BX` (BX=0) | reset (alongside `[0x15881A]`/`[0x15881C]`) |
| W-d | `0x8BCD9` | `FUN_0008BAF0` | WRITE | `MOV word[0x158822],1` | **phase-0xC, flag clear → MATCH OVER (1)** |
| W-e | `0x8BD8A` | `FUN_0008BAF0` | WRITE | `MOV word[0x158822],2` | phase `0x14` → ET over (2) |
| W-f | `0x8BDD4` | `FUN_0008BAF0` | WRITE | `MOV word[0x158822],2` | phase `0x13` → ET over (2) |
| W-g | `0x74342` | *(no function)* | WRITE | `MOV word[0x158822],CX` (CX=0) | reset |
| W-h | `0x88D42` | *(no function)* | WRITE | `MOV word[0x158822],SI` | reset block |
| R-b | `0x8B678` | `FUN_0008AF38` | READ | `MOV AX,word[0x158822]` | the clock harness **returns** the word |

Fresh `read_memory`/`disassemble_bytes` evidence: `0x4B0E0` (40 B) gives W-a
(`0x4B0E7`, bytes `66 89 1d 22 88 15 00`) then `FUN_0008B100`'s head `0x4B100 …
0x4B106 CMP word[0x158822],0` (bytes `66 83 3d 22 88 15 00 00`);
`0x4B0FC` (28 B) the head guard `0x4B106..0x4B115`; `0x4B180` (330 B) the
`0x4B29B MOV word[0x158822],SI` clear (bytes `66 89 35 22 88 15 00`);
`0x88700` (36 B) W-c; `0x74330` (32 B) W-g (bytes `66 89 0d 22 88 15 00`);
`0x88D30` (32 B) W-h (bytes `66 89 35 22 88 15 00`).

So the word has **5 clears** (`0x4B0E7`, `0x4B29B`, `0x8871B`, `0x74342`,
`0x88D42`) and **3 setters**, all inside `FUN_0008BAF0` (`0x8BCD9 = 1`,
`0x8BD8A = 2`, `0x8BDD4 = 2`); the two readers are `FUN_0004B100` (guard +
branch) and `FUN_0008AF38` (return). This matches FU-213 §5's census
(*cross-cited*, count 10, same sites).

### 2.2 `FUN_0008BAF0` — the end machine (fresh)

Fresh `get_function_by_address 0x8BAF0` = `FUN_0008baf0`, body
`0x8BAF0..0x8C065`. Fresh `get_xrefs_to 0x8BAF0` = **1** — a single
`UNCONDITIONAL_CALL` from `0x8B664` inside `FUN_0008AF38` (the per-frame clock).
The phase dispatch (fresh `disassemble_bytes 0x8BAF0` 64 B + `0x8BB50` 170 B +
`0x8BBF0` 120 B + `0x8BC62` 140 B + `0x8BCE7` 260 B):

```
0x8BAFB EAX = [0x157A4A] >> 24        ; = byte[0x157A4D] (CURRENT phase)
0x8BB0E CMP EAX,6  ; JNZ 0x8BB55      ; phase 6 block -> tail
0x8BB55 CMP EAX,2  ; JNZ 0x8BC01      ; phase 2 block -> tail
0x8BC01 CMP EAX,0xC ; JNZ 0x8BCE7     ; phase 0x0C block
0x8BCE7 CMP EAX,0x13 ; JZ 0x8BCF5
0x8BCEC CMP EAX,0x14 ; JNZ tail       ; phase 0x13/0x14 block
```

So it handles phases `{6, 2, 0x0C, 0x13, 0x14}` and is a no-op otherwise.

**The phase-`0xC` block (the match-over decision), fresh `0x8BC01..0x8BCE2`:**

```
0x8BC0A CMP dword[0x158808],0 ; JNZ 0x8BC20   ; act installed? don't tick
0x8BC13 MOV AX,[0x157A64] ; ADD word[0x158818],AX   ; the act/end timer += delta
0x8BC20..0x8BC52  the 0x960 camera triple + 0x36200(3) + 0x8DCD4 sinks
0x8BC57 MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x78 ; JLE 0x8BDE7
        ; timeline gate on word[0x158818] > 0x78
0x8BC68 CALL 0x45001 ; TEST AL,AL ; JNZ 0x8BC80
0x8BC71 MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0xB4 ; JLE 0x8BCA3
0x8BC80 CMP dword[0x10F378],0 ; JZ 0x8BCA3      ; the no-ET latch [0x10F378]
0x8BC89 CALL 0x37F54 ; [0x10F378]=0 ; word[0x158818]=0 ; JMP tail
0x8BCA3 CALL 0x45001 ; TEST AL,AL ; JNZ 0x8BCBF
0x8BCAC MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x168 ; JLE tail
0x8BCBF CMP byte[0x157AC0],0 ; JZ 0x8BCD9       ; <<< the EXTRA-TIME FLAG gate
0x8BCC8 MOV EAX,0x13 ; XOR EDX,EDX ; CALL 0x740A0 ; flag SET  -> phase 0x13
0x8BCD9 MOV word[0x158822],1                     ; flag CLEAR -> MATCH OVER (1)
```

**The phase-`0x13`/`0x14` block, fresh `0x8BCE7..0x8BDDD`:** `0x8BCF5 CMP
dword[0x158808],0 ; JNZ 0x8BD0B`; `0x8BCFE ADD word[0x158818],AX`;
`0x8BD0B MOV ESI,0x780 ; MOV EDX,[0x158814] …` (the camera hold +
`[0x158830]=0x780`); then the two `[0x158816]>>16` timeline gates
(`0x8BD52 CMP EAX,0x4B0` / `0x8BD5A`; `0x8BDAC/0x8BDC4`) and

```
0x8BD45 EAX = phase ; CMP EAX,0x14 ; JNZ 0x8BD95
0x8BD81 CMP dword[0x10F360],0 ; JNZ 0x8BDDD    ; the ET-over latch [0x10F360]
0x8BD8A MOV word[0x158822],2                    ; phase 0x14 -> ET OVER (2)
0x8BD95 MOV AX,[0x157AC5] ; CMP AX,[0x157AC7] ; JNZ tail   ; scores differ -> hold
0x8BDBB..0x8BDD4 MOV word[0x158822],2           ; phase 0x13 + equal -> ET OVER (2)
0x8BDDD MOV dword[0x10F360],1                   ; latch
```

So `FUN_0008BAF0` writes the end word **only** on a *gated terminal*: the
phase-`0xC` screen with the **extra-time flag clear** (`0x8BCD9`) or an
**extra-time half terminal** (`0x8BD8A`/`0x8BDD4`, also `[0x10F360]`-latched).

### 2.3 The caller contract and the frame-loop consumer (fresh)

The harness tail (fresh `disassemble_bytes 0x8B58A` 280 B):

```
0x8B65B CMP byte[0x14C32A],0 ; JNZ 0x8B669   ; the session gate
0x8B664 CALL 0x8BAF0                        ; <<< THE END MACHINE (only call)
0x8B669 CMP dword[0x158808],0 ; JZ 0x8B678
0x8B672 CALL dword[0x158808]                ; the installed-act tick
0x8B678 MOV AX,[0x158822]                   ; <<< RETURN the end word
0x8B687 RET
```

The consumer `FUN_0004B100` (the per-frame match loop; fresh
`get_function_by_address 0x4B100` body `0x4B100..0x4B2E7`; fresh
`disassemble_bytes 0x4B180` 330 B) — the fresh bytes of the match-state branch:

```
0x4B106 CMP word[0x158822],0 ; JZ 0x4B11A ; EAX=1 ; JMP 0x4B2E1   ; head guard
0x4B1A6 CALL 0x8AF38                         ; AX = the end word
0x4B1AB CMP AX,0x1 ; JC 0x4B2A9              ; AX == 0 -> ongoing
0x4B1B5 JBE 0x4B1C6                          ; AX == 1 -> regulation-side screen
0x4B1B7 CMP AX,0x2 ; JZ 0x4B257              ; AX == 2 -> ET-over screen
0x4B1C1 JMP 0x4B2A9
0x4B1C6 XOR EAX,EAX ; MOV AL,[0x157AC2] ; CMP EAX,2 ; JGE 0x4B220
0x4B1D2..0x4B21B  (period index < 2) the HALF-TIME block (0x513EC/0x53DC4/0x4B678…)
0x4B220 JL 0x4B227 ; CMP EAX,4 ; JL 0x4B244
0x4B244 MOV EAX,0x2 ; CALL 0x36BC0           ; <<< [0x5FFC]=2  ==  mark_over
0x4B24E XOR EAX,EAX ; … RET
0x4B297 MOV word[0x158822],SI                ; the AX==2 arm clears the word
```

Fresh `get_function_by_address 0x36BC0` is not needed: FU-64 §2.1 (*cross-cited*)
establishes `FUN_00036BC0(x)` writes `[0x5FFC]=x`; the fresh bytes
`0x4B244 MOV EAX,0x2 / 0x4B249 CALL 0x36BC0` are the native
`[0x5FFC]=2` (**mark_over**) site, exactly what FU-64 §6 maps to
`fifa96_match_lifecycle_mark_over`.

**The decisive structure:** the native match-over screen is entered **only**
when the frame loop sees `[0x158822]==1` (AX==1) *and* the period index is past
half-time (`0x4B1C6 CMP [0x157AC2],2`). When the period index is `< 2` (the
half-time completion) the AX==1 path takes the **half-time block**
(`0x4B1D2`), not `0x4B244`. So **a period completion alone does not end the
native match** — the end word and the period index gate it.

---

## 3. The alias trap: `[0x158822]` is **not** `[0x157822]`

The engine has `mr->flag_157820` / `mr->flag_157822`
(`include/fifa96_engine/fifa96_match_run.h:656..657`). These are `[0x157820]`
and **`[0x157822]`** — the **keeper-AI pause pair**, one dword *earlier* than
the end word (`[0x158822]`). The distinction is confirmed by the writers:
`src/fifa96_loader/fifa96_keeper.c:732..738` (row-1E stage 6 sets/clears them)
and the docs FU-151 §6 / FU-157 §3. The engine's only `[0x157822]` consumer is
`src/fifa96_engine/fifa96_match_run.c:587`
(`if (mr->flag_157820 != 0 || mr->flag_157822 != 0) return; /* 0x7C7F0/FD */`).
**The end word `[0x158822]` has no engine cell** — fresh `grep 158822
src include` returns only comments (`fifa96_match_run.c:3218`, `:3881`).

> **Note on FU-158 §3 (quoted, adjacent-cell hazard):** *"`0x36F36 if
> (byte[0x157822] != 0) [0x155C4E] |= 0x80; ; match-over -> hide ball`"*. That
> reads the **byte** `[0x157822]` (the keeper pause pair), not the word
> `[0x158822]` (the end word). The two cells are `0x1000` apart; the "match-over"
> label there should not be carried onto `[0x158822]`.

---

## 4. Verdict — the minimal mapping the tape's reach needs

### 4.1 What the tape's reach is

Per the approved spec (`docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md`
§6) the extended tape plays **half-time** (first period completion) and a
**tied regulation end → extra-time entry**, and stops at ET entry. Both are
**period completions**, so both hit the engine's `mark_over` at
`fifa96_match_run.c:4161` today. At each:

* **half-time** (completed index 0): the chooser writes phase `0x0C` and act
  `0xB`; the native does **not** match-over (the frame loop's AX==1 path takes
  the `< 2` half-time block at `0x4B1D2`).
* **tied regulation end → ET entry** (completed index 1 — flag set is CONDITIONAL,
  see FU228-L6): the chooser
  writes phase `0x13`/`0x14` and act `0xC`; `FUN_0008BAF0` takes the
  `0x8BCC8` branch (`[0x157AC0] != 0` → phase `0x13`), so **`[0x158822]` is
  never written** and the frame loop never enters the match-over screen.

So on the tape's entire reach the native end word stays `0` and the native
screen stays `ACTIVE`/period-end. **The end machine `FUN_0008BAF0` is not
required for this phase** — its three setter sites (`0x8BCD9`, `0x8BD8A`,
`0x8BDD4`) are not reached, and the `[0x158822]` cell need not be modelled.

### 4.2 What is required (the minimal landing)

The single behavioural divergence the tape's reach exposes is the engine's
**period-agnostic `mark_over`** (`fifa96_match_run.c:4161`). The minimal mapping
is to **drive the lifecycle exit from the native end-of-match condition**
instead of the raw FU-62 `period_ended`:

* the native condition (fresh, §2.3) is: enter `mark_over` iff the end word is
  set (`[0x158822]==1` from the phase-`0xC`/flag-clear decision at `0x8BCD9`;
  or `==2` from an ET terminal) **and** the period index is past half-time
  (`0x4B1C6 CMP [0x157AC2],2`, `mark_over` at `0x4B244`);
* for the tape's reach this condition is **false at both completions**, so —
  with the fix — the run continues through half-time and into ET entry without
  reaching `run_end`.

The honest Stage-2 shape: derive the native end-word *decision* from the engine
cells already live (`mr->global_157ac0`, `mr->state.period`, the chooser's
phase) and gate `mark_over` on it; **do not** port `FUN_0008BAF0`'s body (the
`0x960`/`0x780` camera sinks, the `[0x158818]` end timer, the `[0x10F378]`/
`[0x10F360]` latches) and **do not** add a `[0x158822]` cell unless a later
phase needs the harness return. This keeps the change inside
`fifa96_match_run_frame`'s tail and the already-ported chooser/roll cells.

> **Caveat (cross-slice).** The existing engine test
> `tests/test_engine_match_completion.c:121`
> (`test_period_end_resolve_returns_to_frontend`) pins the *old* contract
> ("a period end marks the lifecycle over" on the **first** completion,
> `state.period == 1`). Any landing that continues through half-time must update
> this test (and its siblings `:160`, `:196`) — that is a Stage-2 concern, not a
> recon write.

### 4.3 Cross-slice dependencies (not owned here)

* The **half-time kickoff restart** (native `FUN_00088860` → situation 1 →
  act 1 → phase `0x17`) and the **ET-entry** route (situation 8 → act 7 →
  phase `0x1D`) are the FU225/FU226/FU227 slices; this slice only says the
  lifecycle must not exit before them.
* The **extra-time flag** producer/ polarity (`[0x157AC0]`, W2 `0x8B2AC` sets on
  a **differing** score) is FU224/FU207; the spec's "tied regulation end → ET"
  wording depends on how a tie reaches ET (the `[0x14C2EE]` override or a
  front-end seed). Recorded, not asserted here.

---

## 5. Census impact

**None.** No new action/phase row is wired: the engine already owns the
`OVER→POST→EXIT` screens and registers the phase-`0x0C`/`0x13`/`0x14` rows
(*cross-cited* FU-213 §6 / FU-211); this slice only re-points the lifecycle
trigger. `floor(100 × 55/80) = 68` stands; no golden re-pin.

---

## 6. Residual legs

1. **FU228-L1 — the native frame-loop match-state branch is only partially
   decomposed.** The `AX==1` path's period-index branch (`0x4B1C6..0x4B24E`)
   and the `AX==2` path (`0x4B257..0x4B2A8`) select the half-time block, the
   screen-`2` transition (`0x4B244`), and the ET-over screen; only the
   `mark_over` site was confirmed first-hand here. Owner: the match-state /
   FU-64 lifecycle slice. (FU-64 §5 renders the whole `FUN_0004B100` branch; a
   fresh line-by-line pass is open.)
2. **FU228-L2 — the phase-`0xC` screen timeline gate is on `word[0x158818]`,
   not `[0x15881A]`.** Fresh bytes `0x8BC57 MOV EAX,[0x158816]; SAR EAX,0x10;
   CMP EAX,0x78` (= word `0x158818`) and the ET-half gates `0x8BD52`/`0x8BDAC`
   (same cell, `0x4B0`/`0x12C`). FU-213 §5 (*quoted*) renders these as
   `[0x15881A]`; the fresh reads show `word[0x158818]` (the act/end timer this
   same block accumulates at `0x8BC13`). Recorded as an erratum candidate;
   the FU-62 `[0x15881A]` period-limit cell is a **different** address. Owner:
   FU-213.
3. **FU228-L3 — the `[0x158822]` raw writers `0x74342` / `0x88D42` are
   function-less regions.** Both clear the word (CX=0 / SI) in reset blocks;
   the enclosing routines are not auto-created (`get_function_by_address`
   0x74340 = none, 0x88D42 = none). Owner: FU-196-L6 (the raw period-end
   family) / the match-reset slice.
4. **FU228-L4 — the engine's `mark_over` contract tests.** The three
   `tests/test_engine_match_completion.c` tests that pin "period end → OVER on
   the first completion" must be re-derived when the trigger is re-pointed.
   Owner: the Stage-2 lifecycle landing.
5. **FU228-L5 — the native screen timing is unmodelled.** The native phase-`0xC`
   screen holds until `word[0x158818] > 0x78` (and further gates `0x45001`,
   `0x10F378`); the engine compresses `OVER→POST→EXIT` into one step. Whether
   the compression is faithful for the tape's reach is not asserted here (the
   tape stops before the screen resolves). Owner: the FU-64 lifecycle slice.
6. **FU228-L6 — the "end machine not required" verdict is CONDITIONAL on
   `[0x157AC0]` being set at ET entry.** The decision at `0x8BCBF` is
   `CMP byte[0x157AC0],0 ; JZ 0x8BCD9`: flag **clear** writes `[0x158822]=1`
   (`0x8BCD9`) and the frame loop then enters the match-over screen, i.e. the
   **end machine IS entered**; flag **set** writes phase `0x13` (`0x8BCC8`) and
   the word stays `0`. This slice's verdict (§4.1) assumes the tied-regulation-
   end → ET-entry completion leaves the flag **set**; if a tied (0-0) end left it
   **clear**, `[0x158822]=1` would be written and the match would end. Whether a
   tie sets the flag is **not** settled here: W2 (`0x8B2AC`) raises
   `[0x157AC0]` on a **differing** score (`differ` in
   `fifa96_match_run.c:2618`), and the tie→ET route depends on the
   `[0x14C2EE]` force-ET override or a front-end seed. Owner: FU224/FU207
   (the flag producers/polarity). Until settled, the verdict is conditional on
   FU228-L6.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `image_base 0`, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `get_xrefs_to` — `0x158822` (**10**), `0x8BAF0` (**1**), `0x157AC0` (**19**),
  `0x14C32A` (**60**).
* `get_function_by_address` — `0x8BAF0` (`FUN_0008baf0`,
  `0x8BAF0..0x8C065`), `0x4B100` (`FUN_0004b100`, `0x4B100..0x4B2E7`),
  `0x4B02C` (`FUN_0004b02c`), `0x8AF38` (`FUN_0008af38`,
  `0x8AF38..0x8B687`), `0x8B9CC` (`FUN_0008b9cc`, `0x8B9CC..0x8BAEF`),
  `0x886D4` (`FUN_000886d4`); `0x74340` / `0x88D42` = **none**.
* `disassemble_bytes` (start, length B) — `0x8BAF0` (64), `0x8BB50` (170),
  `0x8BBF0` (120), `0x8BC62` (140), `0x8BCE7` (260), `0x8B58A` (280),
  `0x4B180` (330), `0x4B0E0` (40), `0x4B0FC` (28), `0x74330` (32),
  `0x88D30` (32), `0x88700` (36).
* `read_memory` — `0x158818` (24 B, all zero).

Engine/loader source read first-hand; docs cross-cited: FU-64 §2.1/§4.2/§5/§6
(the `[0x5FFC]` screens; the `0x4B244..0x4B249` `[0x5FFC]=2` → mark_over
mapping), FU-213 §1/§5/§8 (the end word census + FU213-L3), FU-211, FU-207,
FU-208, FU-143. No writes beyond this slice file; no
rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
untouched; no git commands.
