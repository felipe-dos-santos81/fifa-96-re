# FU-234 — the end-word harness return and the run's return to the front-end (phase-22 wave W22-4)

Provenance: M2 phase-22 Stage-1 recon, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, `memory_size 2757479`, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `get_function_by_address` / `get_xrefs_to`
of **this** session unless marked *cross-cited* (then the frozen FU doc is
named). The alias traps are applied (`dword[x]>>16` = the word at `x+2`,
`>>24` = the byte at `x+3`); LE globals sit at **`+0x100000`** from the file
offset, so a flat-image address `X` in FU-64/FU-155 renders as `X+0x100000`
here; addresses below are `/FIFA96.EXE` runtime unless marked *flat*. Engine
source read first-hand. **No writes**: no rename, comment, label, function,
script or project save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/`
untouched; no git commands.

Scope (W22-4 brief): derive the clock harness's **end-word return** and the
run's **return to the front-end**. Steps: (1) `FUN_0008AF38`'s tail — the
end-word return + the session gate; (2) the native leave/resume targets, the
`[0x105FFC]` screens (FU-64's `[0x5FFC]`), and the engine's `run_end`/EXIT
(`fifa96_match_run.c`) + the mode switch to the front-end; (3) verdict = the
exact return path the tape can pin + numbered legs. This extends FU-64, FU-155,
FU-166, FU-175, FU-228.

---

## Answer in one line

**The harness returns the end word unconditionally — `0x8B678 MOV
AX,word[0x158822]` — but its only producer, the end machine `FUN_0008BAF0`, is
entered at `0x8B664` only when the session byte `[0x14C32A]==0` (the ordinary
match, FU-166), and it is the *phase/flag* gate of FU-228 §2.2 that decides
whether the word is ever set; the run's return to the front-end is a
**separate** path — the native match loop stages `[0x105FFC]=4` (EXIT) +
`[0x107304]=3` (LEAVE), the setup `FUN_000493A0` tears down and returns
`[0x7304]==2`, and the menu caller runs `FUN_00017DF0` (front-end restore);
the engine ports exactly this lifecycle as `fifa96_match_lifecycle`
(`OPEN→OVER(2)→POST(3)→EXIT(4)`, targets LEAVE 3/RESUME 2) and `fifa96_match_run_end`
→ `eng->mode = FIFA96_ENGINE_MODE_FRONTEND`.** The tape can pin the **engine
lifecycle return** (phase-22 T5) but **not the native harness-return value**:
the engine has no `[0x158822]` cell (phase-22 T2 adds `mr->end_word_158822`,
spec §9). One cross-slice tension is recorded (FU234-L7).

---

## 1. Method

* Fresh `disassemble_bytes` of the harness tail `0x8B64A` (64 B), the earlier
  session-gate read `0x8B3A8` (32 B), the helper `FUN_00036BC0` `0x36BC0`
  (64 B), the second harness caller `0x74400` (32 B), the native leave staging
  `0x497C0` (56 B), the teardown `0x49541` (112 B) and the menu caller
  `0x180A0` (36 B).
* Fresh `get_function_by_address` `0x8AF38` (`FUN_0008af38`,
  `0x8AF38..0x8B687`), `0x8BAF0` (`FUN_0008baf0`, `0x8BAF0..0x8C065`),
  `0x493A0` (`FUN_000493a0`, `0x493A0..0x495AF`).
* Fresh `get_xrefs_to` `0x8AF38` (**2**), `0x158822` (**10**), `0x8BAF0`
  (**1**), `0x14C32A` (**60**), `0x107304` (**17**), `0x105FFC` (**7**),
  `0x158808` (**22**), `0x5FFC` (**0**) and `0x7304` (**0** — the flat
  spellings; the runtime cells are `0x105FFC` / `0x107304`, §3.2).
* Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
  (`:1778..1783`, `:4118..4193`, `:4195..4244`, `:2584..2696`),
  `src/fifa96_engine/fifa96_engine.c` (`:151..247`),
  `src/fifa96_loader/fifa96_match_lifecycle.c` (whole),
  `include/fifa96_loader/fifa96_match_lifecycle.h` (whole),
  `include/fifa96_engine/fifa96_match_run.h` (`:520`, `:610`, `:622`,
  `:656..657`, `:751`, `:774`), and the tape `tests/test_engine_m2.c`
  (`:2256..2265`, `:2743..2779`).
* Docs cross-cited: FU-64 §1/§2.1/§4/§5 (the `[0x5FFC]` screens, the
  start/leave/teardown, the flat addresses), FU-155 §2/§3/§5 (the
  `[0x14C32A]` gate), FU-166 §2/§5/§7 (session identity, byte 0 = ordinary),
  FU-228 §1.1/§2.3/§4.1 (the harness tail dispatch + the end word), FU-175 §2
  (the `[0x158808]` act tick), the phase-22 spec §3/§9.

---

## 2. The harness tail and the end-word return (Step 1)

### 2.1 The tail, byte-exact (fresh)

Fresh `disassemble_bytes 0x8B64A` (64 B) — the `FUN_0008AF38` tail
(`0x8B64A..0x8B687`, then the next function's prologue):

```
0x8B64A 31 db                     XOR   EBX, EBX
0x8B64C 30 ed                     XOR   CH, CH
0x8B64E 66 89 1d ba7a1500         MOV   word [0x157ABA], BX      ; = 0
0x8B655 88 2d 2d881500            MOV   byte [0x15882D], CH      ; = 0
0x8B65B 80 3d 2ac31400 00         CMP   byte [0x14C32A], 0x0     ; <<< SESSION GATE
0x8B662 75 05                     JNZ   0x8B669                  ; !=0 -> skip the machine
0x8B664 e8 87 04 00 00            CALL  0x8BAF0                  ; <<< THE END MACHINE
0x8B669 83 3d 08881500 00         CMP   dword [0x158808], 0x0    ; installed-act pointer?
0x8B670 74 06                     JZ    0x8B678
0x8B672 ff 15 08881500            CALL  dword [0x158808]           ; the act tick (FU-175 §2)
0x8B678 66 a1 22881500            MOV   AX, word [0x158822]      ; <<< RETURN the end word
0x8B67E 83 c4 04                  ADD   ESP, 0x4
0x8B681 5d / 5f / 5e / 5a / 59 / 5b   POP EBP/EDI/ESI/EDX/ECX/EBX
0x8B687 c3                        RET
```

This is byte-identical to FU-228 §2.3's quoted tail. Two facts are decisive for
this slice:

1. **The return is unconditional.** `0x8B678 MOV AX,word[0x158822]` is reached
   on every exit of `FUN_0008AF38`; the session gate at `0x8B65B` gates only the
   **call** at `0x8B664`, never the read/return. So the harness always hands its
   caller the current end word (whatever it is).
2. **The only setter of that word is the session-gated call.** Fresh
   `get_xrefs_to 0x158822` = **10**, and the three non-zero setters
   (`0x8BCD9=1`, `0x8BD8A=2`, `0x8BDD4=2`) all live inside `FUN_0008BAF0`
   (`get_xrefs_to 0x8BAF0` = **1**, the sole call site `0x8B664`). The other
   seven refs are clears (`0x4B0E7`, `0x4B29B`, `0x8871B`, `0x74342`,
   `0x88D42`) and two readers (`0x4B106` in `FUN_0004B100`, `0x8B678` here) —
   *cross-cited* FU-228 §2.1 (same census of 10).

### 2.2 The session gate `[0x14C32A]` (fresh + FU-166)

Fresh `get_xrefs_to 0x14C32A` = **60** (3 writes, 57 reads) — *cross-cited*
FU-155 §1 (same 60). The two reads inside this harness are:

```
0x8B3B7 80 3d 2ac31400 00   CMP byte [0x14C32A], 0    ; (after 0x8B3AE CMP byte[0x15882D],0 / JZ)
0x8B3BE 75 07               JNZ 0x8B3C7               ; !=0 -> skip
0x8B3C0 31 c0               XOR EAX, EAX
0x8B3C2 e8 3d 8d fc ff      CALL 0x54104              ; iff [0x15882D]!=0 && [0x14C32A]==0
0x8B65B 80 3d 2ac31400 00   CMP byte [0x14C32A], 0    ; the tail gate above
```

So `[0x14C32A]` gates **two** calls in the harness both with polarity `== 0`
(the `0x8B3B7` call additionally requires `[0x15882D]!=0`). *Cross-cited*
FU-166 §2/§5 settles the byte's meaning: `[0x14C32A] = ([0x14C1D0]==4)` is a
**per-GAME-SELECT session flag**; **byte 0 = every ordinary played match**
(Friendly/Leagues/Tournaments/Playoffs/boot demo); **byte 1 = the single
Practise session** (`ptmsel`, `FUN_00032DE0`). FU-166 §7 + its T3 status
append: the engine's settled live default is **byte 0**
(`fifa96_match_run_begin` seeds `session_gate_14c32a = 0`; fresh
`grep "session_gate_14c32a =" fifa96_match_run.c` = `:1772`, `:2073`, `:2202`
all `= 0`), so the end machine **is** entered in the ordinary session.

> **Erratum candidate (FU-155 §6.3, quoted, not rewritten there):** FU-155 §6.3
> states the engine "seeds `session_gate_14c32a = 1` for live matches
> (`fifa96_match_run.c:1462`)" and runs the ball machine unconditionally. The
> fresh source shows the live default is **0** (FU-166 T3 settled it); the
> `1442`/`1462` line is no longer the seed. Recorded, not corrected here.

### 2.3 The two consumers of the returned AX (fresh)

`FUN_0008AF38` has exactly **two** callers (fresh `get_xrefs_to 0x8AF38` = 2):

* **`0x4B1A6` in `FUN_0004B100`** (the per-frame match body). Fresh bytes
  (FU-228 §2.3, re-quoted): `0x4B1A6 CALL 0x8AF38` → `0x4B1AB CMP AX,1 / JC
  0x4B2A9` (AX==0 → ongoing) / `0x4B1B5 JBE 0x4B1C6` (AX==1 → period-index
  branch) / `0x4B1B7 CMP AX,2 / JZ 0x4B257` (AX==2 → ET-over screen).
* **`0x7440B` in an unfctioned setup/reset block** (fresh `disassemble_bytes
  0x74400`: the JMP at `0x74409` targets `0x74427`; `0x7440B CALL 0x8AF38` →
  `0x74410 MOV [ESP],EAX` → `0x74413 TEST AX,AX / JZ 0x74427` → `0x74418 CWDE`
  → `0x74419 CMP EAX,1 / JNZ 0x744C2`). The **same** AX==0-ongoing /
  AX==1-branch shape as the frame loop, in the reset block adjacent to
  `0x74342` (the `[0x158822]` clear, FU228-L3). *Owner: FU234-L1.*

So the harness-return contract is a **word in AX** consumed by two sites;
both read only AX (`CMP AX,1`, `TEST AX,AX`), never the upper EAX.

### 2.4 Where the word's value comes from (cross-cited FU-228)

The value itself is decided by `FUN_0008BAF0` (fresh body `0x8BAF0..0x8C065`).
Per FU-228 §2.2 (its phase dispatch `{6,2,0x0C,0x13,0x14}`) the setters are
gated terminals: **phase-`0xC` screen with the extra-time flag `[0x157AC0]`
CLEAR → word 1** (`0x8BCD9`); flag SET → phase `0x13`, word stays 0
(`0x8BCC8`); **ET-half terminal phase `0x13`/`0x14` → word 2** (`0x8BD8A`/
`0x8BDD4`, latched `[0x10F360]`). FU-228 §1.1 further records that the engine
calls this same function **the ball machine** (`fifa96_match_run.c:4147..4152`
`match_run_ball_machine`, phase-11 T2 / L14.8) — the **same `FUN_0008BAF0`**:
one function whose phase-`2` arm is the open-play ball lead and whose
phase-`0xC`/`0x13`/`0x14` arms are the end-word decision. The engine ports the
ball-record part but **not** the end-word arm (`fifa96_match_run.c:3888`
"L14.8.a the … match-over state words (`0x158822`; the engine lifecycle owns
exits)").

---

## 3. The front-end transition (Step 2)

### 3.1 The native leave/resume targets (fresh)

The match loop `FUN_000495B0` stages the exit (fresh `disassemble_bytes
0x497C0`):

```
0x497C2 CALL 0x6D1B2 / CMP EAX,1 / JNZ 0x497D3   ; device != 1 arm
0x497CC CALL 0x45D0D / JMP 0x497E8
0x497D3 MOV EAX, 0x4                             ; EXIT
0x497D8 MOV EDX, 0x3                             ; LEAVE target
0x497DD CALL 0x36BC0                             ; [0x105FFC] := 4   (fresh helper below)
0x497E2 89 15 04731000  MOV [0x107304], EDX      ; [0x7304] := 3     (the leave target)
0x497E8 CMP EBX,[0x107304] / JZ 0x495D1          ; loop while selector == target
```

`FUN_00036BC0` is the one-line screen setter (fresh `0x36BC0`):
`a3 fc5f1000  MOV [0x105FFC],EAX ; c3 RET`; its reader `FUN_00036BC8`
(`0x36BC8 CMP EAX,[0x105FFC] ; SETZ AL`) returns `[0x5FFC]==EAX`. So **EXIT**
is `[0x105FFC]=4` and the **leave target** is `[0x107304]=3`. The engine maps
these two constants directly: `FIFA96_MATCH_SCREEN_EXIT = 4` and
`FIFA96_MATCH_LIFECYCLE_LEAVE = 3u` (*cross-cited* FU-64 §5/§6; fresh
`include/fifa96_loader/fifa96_match_lifecycle.h:10,13`).

### 3.2 The `[0x5FFC]` screens (FU-64) and the address convention

FU-64 (§5, quoted) maps the match screen state `[0x5FFC]`: `0` active
(`0x4942B`), `2` over (`0x4B249`), `3` post (`0x49532`), `4` exit (`0x497DD`,
`0x49807`), `5`/`7` checked at leave, `6→4` (`FUN_00036C3C 0x36C60`). FU-64 was
produced on the **flat** program `/fifa96_le.bin`, where the cell is `0x5FFC`;
the **runtime** cell in this session is **`0x105FFC`** (fresh `get_xrefs_to
0x5FFC` = **0**; `0x105FFC` = **7** — the setter `0x36BC0`, the reader
`0x36BC8`, the exit spellings `0x36BD8`/`0x36C0C`/`0x36C3C`). The `+0x100000`
delta is uniform: FU-64's `[0x5FFC]`/`[0x58822]`/`[0x7304]`/`[0x72FC]`/
`[0x1471C]`/`[0x5094]` are this session's `[0x105FFC]`/`[0x158822]`/
`[0x107304]`/`[0x1072FC]`/`[0x11471C]`/`[0x105094]`.

### 3.3 The teardown and the menu caller's front-end restore (fresh)

The setup wrapper `FUN_000493A0` (`0x493A0..0x495AF`) runs the loop and, on
`[0x105FFC]==4`, the teardown (fresh `disassemble_bytes 0x49541`):

```
0x49541 CALL 0x45D0D                     ; input/event/pace reset
0x49546 C7 05 24731000 01000000  MOV dword [0x107324], 0x1   ; hold the 30 Hz pace
0x49550 CALL 0x45D0D
0x49555 PUSH 0x49320 / CALL 0x9F684      ; cancel the registered callback
0x49562 CALL 0x45D0D
0x49567 CALL 0x4AEA4                     ; match teardown body
0x4956C CALL 0x67948                     ; audio/voice cleanup
0x49571 XOR EDX,EDX / MOV ECX,[0x107304]
0x49579 89 15 fc721000  MOV [0x1072FC], EDX   ; active = 0
0x4957F 89 15 1c471100  MOV [0x11471C], EDX   ; unload = 0
0x49585 CMP ECX,0x3 / JNZ 0x4959A
0x4958A MOV ESI,0x2 / CALL 0x6D7EC / MOV [0x107304],ESI   ; post-exit -> target 2 (RESUME)
0x4959A CMP dword [0x107304],0x2 / SETZ AL / AND EAX,0xFF ; return [0x7304]==2
0x495AF RET
```

The return value (`AL = ([0x7304]==2)`) steers only the main pre-match caller
(FU-64 §4.4). The **menu/competition caller** `FUN_00018014` performs the
**front-end screen restore** (fresh `disassemble_bytes 0x180A0`):

```
0x180AA CALL 0x493A0                     ; run the match (selector 0)
0x180AF 83 3d 94501000 00  CMP dword [0x105094], 0   ; the front-end confirm flag
0x180B6 75 07              JNZ 0x180BF
0x180B8 31 c0              XOR EAX,EAX
0x180BA CALL 0x17DF0                     ; <<< the front-end screen restore
```

So the run's native return lands in `FUN_00017DF0` (the menu redraw/restore,
FU-64 §4.4) when `[0x5094]==0`. This is the front-end transition on the menu
path; the other two callers (`FUN_00018108`, `FUN_00018680`) run their own
post-return sequences (*cross-cited* FU-64 §4.4).

### 3.4 The engine's `run_end` / EXIT + the mode switch (fresh source)

`src/fifa96_engine/fifa96_match_run.c`:

* `fifa96_match_run_resolve` (`:4195`) — `OVER` → `resolve_over` → `POST` →
  `request_exit` → `EXIT`; `if (should_exit()) return fifa96_match_run_end(mr)`
  (`:4204`).
* `fifa96_match_run_step` (`:4208`) — `if (should_exit()) … run_end` (`:4212`);
  `else if (screen == OVER) resolve` (`:4222`) — the one-step compression
  (FU228-L5).
* `fifa96_match_run_end` (`:4231`):

```c
int rc = fifa96_match_lifecycle_end(&mr->lc, &mr->pace);
struct fifa96_engine *eng = mr->engine;
match_run_release_stage(mr);
match_run_release_meta(mr);
mr->running = 0;
if (eng) {
  if (eng->match == mr) eng->match = NULL;
  if (eng->mode == FIFA96_ENGINE_MODE_MATCH) eng->mode = FIFA96_ENGINE_MODE_FRONTEND;  /* :4241 */
}
return rc;
```

* `fifa96_match_run_post_exit` (`:1778`) — reached from
  `fifa96_match_lifecycle_end` iff `target == LEAVE (3)`
  (`fifa96_match_lifecycle.c:94..99`); sets `mr->engine->mode =
  FIFA96_ENGINE_MODE_FRONTEND` (`:1781`).
* `src/fifa96_engine/fifa96_engine.c:229..230` — the MATCH arm's belt-and-braces
  `if (!e->match) e->mode = FIFA96_ENGINE_MODE_FRONTEND;`.

So the engine's front-end return is **structurally** the native leave/teardown:
`OVER(2)→POST(3)→EXIT(4)` with `target = LEAVE(3)`, the `end` sequence
(pace hold → cancel callback → teardown → unload=0 → post-exit → target=RESUME(2)),
exactly FU-64 §6's mapping. It is **decoupled** from the end-word path: the
engine reads no `[0x158822]` cell (fresh
`grep 158822 src include` = comments only, `fifa96_match_run.c:3218`, `:3881`);
its `mark_over` trigger is the derived proxy `end_word_decided = (state.phase ==
0x0C && global_157ac0 == 0)` + `past_half_time (global_157ac2 >= 2)` +
`period_ended` (`:4183..4191`, the phase-21 re-point / FU228-L6).

---

## 4. Verdict — the return path the tape can pin (Step 3)

### 4.1 The tape's session and reach

The v21 tape stages the goal scenario with the **Practise** byte-1 gate
(`tests/test_engine_m2.c:2264 mr->session_gate_14c32a = 1`) but **opens the
ordinary session at half-time** (`:2759 mr->session_gate_14c32a = 0`, comment
"the period-end route's caller requires the ordinary-match session gate OPEN
(`[0x14C32A] == 0`, FU-227)"). So from the half-time completion onward the
tape's native value is **byte 0**, i.e. the end machine `0x8B664` **is** called
on every granted frame (byte==0 → `JNZ` not taken). The tape then plays a
differing-score regulation end (flag `global_157ac0 = 1`) → the chooser's ET
arm (phase `0x13`, `:2778`) and stops at ET entry (phase-21 §2 non-goal).

### 4.2 The two return paths, and which is pinnable

| return path | native mechanism | engine counterpart | tape pinnable? |
|---|---|---|---|
| **harness end-word return** | `0x8B678 MOV AX,[0x158822]`; producer `FUN_0008BAF0` (session-gated `0x8B65B`, then the FU-228 phase/flag gate) | **none** — no `[0x158822]` cell; the `end_word_decided` proxy at `:4185` | **No** (yet) — phase-22 **T2** adds `mr->end_word_158822` (spec §9). The tape sees the *proxy*, not the word |
| **run → front-end** | `[0x105FFC]=4` + `[0x107304]=3` (leave), `FUN_000493A0` teardown, `FUN_00017DF0` restore | `fifa96_match_lifecycle` + `fifa96_match_run_end` → `eng->mode = FRONTEND` (`:4241`), `post_exit` (`:1781`) | **Yes** — phase-22 **T5** (acceptance D2: through the return) |

**The exact return path the tape can pin is the engine lifecycle return:**
`mark_over` (once T2/T3 make it end-word-driven) → next `fifa96_match_run_step`
→ `fifa96_match_run_resolve` (`OVER→POST→EXIT`) → `should_exit` →
`fifa96_match_run_end` → `eng->mode = FIFA96_ENGINE_MODE_FRONTEND` (`:4241`),
with `post_exit` (`:1781`) re-asserting the mode. This is the port of the native
`[0x105FFC]=4` + `[0x107304]=3` leave and the `FUN_000493A0` teardown; it is
what acceptance D2 ("through the return to the front-end") exercises.

The **native harness return** (`AX=[0x158822]`) is a mechanism the engine has
**not** ported: it is the *producer* of the match-over entry, not the
front-end transition, and until T2 lands the `end_word_158822` cell the tape
cannot pin its value. What the tape *can* pin about it today is only its
**gating structure** (the session byte's polarity), which the engine models as
the derived `end_word_decided` proxy — a proxy that **excludes** the `==2`
ET-over event by record (`:4177..4182`, FU228-L4).

### 4.3 The one-line landing

The tape can pin the **engine's** run-end → front-end mode switch (the
phase-22 T5 path, `fifa96_match_run_end` `:4241` + `post_exit` `:1781`), which
is the faithful port of the native lifecycle leave/teardown (`[0x105FFC]=4`,
`[0x107304]=3`, `FUN_000493A0`, `FUN_00017DF0`). It **cannot** pin the native
harness end-word return until phase-22 T2 adds the `[0x158822]` cell; the
engine's `end_word_decided` proxy is the current stand-in.

---

## 5. Census impact

**None.** No new action/phase row is wired: the engine already owns the
`OVER→POST→EXIT` lifecycle and the front-end mode switch; this slice only maps
the harness-return and the leave/resume path. The phase-22 plan's "likely zero
new rows" caveat stands (spec §5). `floor(100 × 55/80) = 68` (fresh
`docs/ENGINE.md:8` `55/80`; fresh `README.md:1` `(68%)`); no golden re-pin.

---

## 6. Residual legs

1. **FU234-L1 — the second harness caller `0x7440B` is not decomposed.** The
   unfctioned setup/reset block (`0x74400..0x744C2`; the `JMP 0x74427` at
   `0x74409`, the AX branch at `0x74413..0x7441C`) consumes the harness end
   word the same way the frame loop does, in the region adjacent to the
   `[0x158822]` clear `0x74342` (FU228-L3 / FU-196-L6). Owner: the setup/reset
   block slice (`0x74220..0x743EF`, FU-155 L155.4).
2. **FU234-L2 — the harness return ABI upper bits.** `0x8B678 MOV
   AX,[0x158822]` leaves the upper EAX undefined; both consumers read only AX.
   Recorded for the T2 port (the engine should model a 16-bit word). Low risk.
3. **FU234-L3 — the `0x8B3B7` session-gated call's callee.** `CALL 0x54104`
   runs iff `[0x15882D]!=0 && [0x14C32A]==0`; the `[0x15882D]` producer and
   `FUN_00054104` are not decomposed here (outside the end-word scope; the
   harness tail clears `[0x15882D]=0` at `0x8B655`). Owner: the clock-body
   slice.
4. **FU234-L4 — `word[0x157ABA]` is cleared at `0x8B64E` with no located
   writer/reader.** The tail zeroes it in the same block as the end-word
   return; not the end word. Owner: the clock-body slice.
5. **FU234-L5 — the engine has no `[0x158822]` cell.** The native harness-return
   path has no engine counterpart; phase-22 T2 adds `mr->end_word_158822`
   (spec §9). Until then the tape can pin only the derived proxy, not the
   word's value transition. This is the slice's central hand-off.
6. **FU234-L6 — the front-end screen restore `FUN_00017DF0` is not ported.**
   The native menu redraw after the match (the `[0x5094]==0` arm at `0x180BA`)
   is the actual front-end transition on the menu path; the engine's return is
   the mode flag only (`eng->mode = FRONTEND`), the SDL frontend re-entering
   via `fifa96_engine_step`'s FRONTEND arm (`fifa96_engine.c:195..210`).
   Whether the tape's D2 return needs any of `FUN_00017DF0` is a T5 question.
7. **FU234-L7 (cross-slice) — FU-228 §4.1's "the native end word stays 0 on
   the tape's reach" is in tension with the phase-`0xC` flag-clear write at the
   half-time completion.** At the half-time completion (index 0, flag
   `global_157ac0 == 0`, tape-asserted `test_engine_m2.c:2755`) and the tape's
   opened byte-0 session, the end machine's phase-`0xC` block reaches
   `0x8BCBF CMP byte[0x157AC0],0 / JZ 0x8BCD9 MOV word[0x158822],1`. FU-228 §4.1
   ("the native end word stays 0") and FU-228 §1.1 ("a period completion alone
   does not end the match") are not obviously consistent here; the frame loop
   then takes the index-`<2` half-time block (`0x4B1C6`/`0x4B1D2`), so the
   *match-over screen* is still not entered — but the word may be `1`. Whether
   the half-time block clears it (making §4.1 true) is not decomposed. Owner:
   **FU-231** (the end machine body) / **FU-232** (the frame-loop branch) /
   **FU-236** (the word-`1` route). Not asserted here.
8. **FU234-L8 — the `[0x158808]` act tick ordering.** The tail's
   `0x8B669 CMP [0x158808],0 / 0x8B672 CALL [0x158808]` runs after the end
   machine and before the return; the engine's `fifa96_match_act_tick`
   (`:4161`) is the port (FU-175 §2). Not re-derived here.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `image_base 0`, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `get_xrefs_to` — `0x8AF38` (**2**: `0x4B1A6`, `0x7440B`), `0x158822`
  (**10**), `0x8BAF0` (**1**: `0x8B664`), `0x14C32A` (**60**), `0x107304`
  (**17**), `0x105FFC` (**7**), `0x158808` (**22**), `0x5FFC` (**0**),
  `0x7304` (**0**).
* `get_function_by_address` — `0x8AF38` (`FUN_0008af38`,
  `0x8AF38..0x8B687`), `0x8BAF0` (`FUN_0008baf0`, `0x8BAF0..0x8C065`),
  `0x493A0` (`FUN_000493a0`, `0x493A0..0x495AF`).
* `disassemble_bytes` (start, length B) — `0x8B64A` (64), `0x8B3A8` (32),
  `0x36BC0` (64), `0x74400` (32), `0x497C0` (56), `0x49541` (112), `0x180A0`
  (36).
* Engine/loader source read first-hand; docs cross-cited: FU-64
  §1/§2.1/§4.4/§5/§6 (the `[0x5FFC]` screens, the leave/teardown, the
  `FUN_00017DF0` restore, the flat addresses), FU-155 §1/§2/§3/§6.3 (the
  `[0x14C32A]` gate + the engine-seed erratum), FU-166 §2/§5/§7 (session
  identity; byte 0 = ordinary), FU-228 §1.1/§2.1/§2.3/§4.1 (the harness tail
  dispatch, the end word, the end-machine body), FU-175 §2 (the act tick),
  the phase-22 spec §3/§5/§9. No writes beyond this slice file; no
  rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
  untouched; no git commands.
