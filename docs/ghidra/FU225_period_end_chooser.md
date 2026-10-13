# FU-225 — the `FUN_00088860` period-end chooser and its live invocation (phase-21 wave W21-3)

Provenance: M2 phase-21 recon wave **W21-3**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, executable `/home/felipe/code/fifa96-reversed/game/hdd/FIFA96/FIFA96.EXE`,
`watcom:LE:32:default`, image base `0`, min `0x10000`, max `.image::00174a2a`,
**2706** functions, 15239-ish symbol count, `.image` overlay present). Every
address/byte/instruction/xref-count below is a **fresh** `read_memory` /
`disassemble_bytes` / `disassemble_function` / `get_xrefs_to` /
`get_function_by_address` / `search_instructions` / `search_byte_patterns` of
**this** session unless marked *cross-cited* (then the frozen FU doc is named).
The alias traps are applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte
at `x+3`); LE-image globals sit at `+0x100000` from the file offset; addresses
are `/FIFA96.EXE` runtime addresses. Engine/loader source read first-hand. **No
writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (W21-3 brief): pin the native period-end chooser's exact **live
invocation** and any delta between it and the engine's `phase_drive` mirror — (1)
read the native chooser `FUN_00088860` whole body; (2) derive the live caller
`0x74244` (**FU215-L3**; stated to have no Ghidra function) — its enclosing
function, args and reachability (fresh `get_xrefs_to`); (3) diff against the
engine mirror (`fifa96_match_run_phase_drive` + `fifa96_action_phase_period_end`);
(4) verdict + legs. Extends **FU-196 §3**, **FU-213**, **FU-215** (esp. L3),
FU-216, FU-208/209.

**Premise correction (spec §1/§9).** The spec's baseline premise — that
`period_setup_live` *defaults to `0`* and **T4 flips `0 → 1`** (spec §1 lines
32–36; §9 line 200) — is **stale at the spec baseline**: the live `begin` path
already sets it `1` (`src/fifa96_engine/fifa96_match_run.c:2225`, fresh
first-hand; `:2092` seeds `0` in `reset` but `begin` overrides it). So T4's stated
"flip" is **moot**; the real T4 work is the chooser stand-in inputs (**D2–D4**,
§5.3) and the tape reach, not the switch (see §6).

**Answer in one line.** The brief's premise is **conflated and its live-caller
attribution is wrong**: `FUN_00088860` is *not* the mirror-source of
`fifa96_action_phase_period_end` (that mirror, per its own header, is
**`FUN_0008B9CC`** `0x8B9CC..0x8BAEF`; `FUN_00088860`'s mirror is
**`fifa96_match_run_period_setup(mode 0)`**), and the "live caller `0x74244`" sits
in a function whose entry is **`0x741C4`** — a near-twin of `FUN_0004B02C` that is
**statically unreferenced** (fresh `get_xrefs_to 0x741C4` = 0; fresh
`search_instructions` operand `0x00074…` contains **no** `0x741C4` target; fresh
`search_byte_patterns` for the LE32 `0x000741C4` = **0 matches**). The **live**
`FUN_00088860` caller is the *other* one, **`0x4B08E`** inside **`FUN_0004B02C`**
(mode 0), reached from the match loop **`FUN_000493A0`** (`0x4950A`). The engine
already mirrors that reachable native faithfully; flipping **`period_setup_live`
alone does not suffice** for the period-end spine, because that flag gates only
the *match-init* `period_setup(mode 1)` producer (`fifa96_match_run.c:2416`) and
touches neither the chooser call nor the mode-0 sit-8 producer (both gated
independently on the FU-62 completion / `global_157ac2 == 4`).

---

## 1. The native `FUN_00088860` (whole body, fresh)

`FUN_00088860` (`0x88860..0x888FA`, fresh `disassemble_function` = 48
instructions; fresh `disassemble_bytes 0x88860` 150 B) takes **no arguments**
(void; the two call sites pass nothing) and returns **void**. All inputs are
globals. Byte-verified body:

```
0x88860 PUSH EBX/ECX/EDX
0x88863 MOV EAX,[0x00157a4a] ; bytes a1 4a 7a 15 00
0x88868 SAR EAX,0x18
0x8886b CMP EAX,0xc          ; phase byte = [0x157A4A]>>24
0x8886e JNZ 0x000888f7       ; gate: phase != 0x0C -> return (bytes 0f 85 83 00 00 00)
0x88874 CALL 0x0007417c      ; side-byte setter (bytes e8 03 b9 fe ff)
0x88879 MOV AL,[0x00157aae] / 0x8887e MOV [0x00157aaf],AL
0x88883 CALL 0x0006ffc0      ; (e8 38 77 fe ff)
0x88888 PUSH 0 ; [0x10f330] ; [0x10f32c] ; [0x10f328]  (the 0x10F328/2c/30 triple)
0x8889f CALL 0x000700f4      ; camera/screen triple (e8 50 78 fe ff)
0x888a4 CALL 0x00073e28      ; (e8 7f b5 fe ff)
0x888a9 XOR EAX,EAX
0x888ab MOV AL,[0x00157ac2]  ; period index
0x888b0 CMP EAX,0x4
0x888b3 JNZ 0x000888e2       ; period != 4 -> situation 1 (bytes 75 2d)
   ; --- period == 4: extra-time period-end ---
0x888b5 CALL 0x00092ac8      ; RNG step (e8 0e a2 00 00)
0x888ba XOR AH,AH / 0x888bc AND AL,0x1 / 0x888c0 MOVSX EDX,AX / 0x888c3 MOV EAX,0x8
0x888c8 CALL 0x0008a938      ; situation 8, side (rng & 1), BX=0 (e8 6b 20 00 00)
0x888cd MOV EBX,0x8 / 0x888d2 MOV EAX,0xC1 / 0x888d7 XOR EDX,EDX
0x888d9 CALL 0x0008f188      ; (0xC1,0,8) sink (e8 aa 68 00 00)
0x888de POP/POP/POP ; RET
   ; --- period != 4: sit-1 else-arm ---
0x888e2 MOV EAX,0x1 / 0x888e7 MOV EDX,[0x00157aab] / 0x888ef SAR EDX,0x18
0x888f2 CALL 0x0008a938      ; situation 1, side [0x157AAB]>>24, BX=0 (e8 41 20 00 00)
0x888f7 POP/POP/POP ; RET
```

**Branches / gate polarity:** the head gate is `phase == 0x0C` (the `JNZ` at
`0x8886e` skips the whole body when `!= 0x0C` — positive assertion of the phase).
The second gate is `[0x157AC2] == 4` (`JNZ` at `0x888b3` selects the situation-1
else-arm). Both were re-read first-hand here and match FU-215 §3.2.

**Out-values:** none directly (void dispatcher). Its observable outputs are the
two `FUN_0008A938(situation, side, BX=0)` calls (situation 8 / side `rng&1`, or
situation 1 / side `[0x157AAB]>>24`) and the `FUN_0008F188(0xC1,0,8)` sink.

**Callers (fresh `get_xrefs_to 0x88860` = 2):**

| site | enclosing | byte-verified target |
|---|---|---|
| `0x4B08E` | `FUN_0004B02C` (mode-0 arm) | `e8 cd d7 03 00`: next `0x4B093` + `0x3D7CD` = `0x88860` ✓ |
| `0x74244` | *(no Ghidra function)* | `e8 17 46 01 00`: next `0x74249` + `0x14617` = `0x88860` ✓ |

Both CALL rel32s were computed first-hand (the brief's "verify the CALL
byte/rel32" note). **Only `0x4B08E` is statically reachable (§3).**

---

## 2. The *actual* mirror-source: `FUN_0008B9CC` (name reconciliation)

The brief (and spec §3/§4) call `FUN_00088860` "the chooser whose mirror is
`fifa96_action_phase_period_end`". That is a **conflation**. The header
(`include/fifa96_loader/fifa96_action_handlers.h:649`) states, and
`src/fifa96_engine/fifa96_match_run.c:2556` repeats, that
`fifa96_action_phase_period_end` ports **`FUN_0008B9CC`** (`0x8B9CC..0x8BAEF`).
`FUN_00088860`'s mirror is **`fifa96_match_run_period_setup(mode 0)`**
(`src/fifa96_engine/fifa96_match_run.c:3194..3209`, first-hand).

**`FUN_0008B9CC`** (fresh `disassemble_function` = 85 instructions; fresh
`disassemble_bytes 0x8B9CC` 292 B) is the period-change **chooser**: it reads
`[0x157AC0]` (extra-time flag), `[0x157AC2]` (period), `[0x157AC5]`/`[0x157AC7]`
(scores), `[0x1587D8]`/`[0x1587D9]` (per-side counters), `[0x157ABE]`/
`[0x157ABF]`/`[0x157AAC]>>24` (sides), invokes the `0x12230`/`0x12250` probes, then
writes the phase via `0x740A0` and invokes the act via `0x888FC`. Key gates,
byte-read first-hand:

* `0x8B9CF CMP byte[0x157AC0],0` / `0x8B9D6 JZ 0x8BAA7` — extra-time flag clear
  → the **no-extra-time arm** (`0x8BAA7..`): `period < 4` → phase `0x0C`
  (side `[0x157AAC]>>24`), else **no phase write**; then the common tail.
* `0x8B9E8 MOV EDX,3` — tie-break default 3 (equal scores); `0x8B9F0 JGE 0x8BA0B`
  (`period >= 4` → the `d8`/`d9` `SETLE` arm at `0x8BA17`), else `SETBE`
  (`0x8BA04`, unsigned `own <= other`).
* `0x8B9E3..` and the probe calls `0x8BA37 CALL 0x12230`, `0x8BA49 CALL 0x12250`,
  `0x8BA5E CALL 0x12250` → phase `0x14` on a nonzero probe.
* common tail `0x8BAC6 CMP byte[0x157AC0],0` / `0x8BACD JZ 0x8BADB`: `EAX=0xC`
  (extra-time, `0x8BAD4`) else `EAX=0xB`; `0x8BAE7 CALL 0x888FC` (act; byte-verified
  `e8 10 ce ff ff`: next `0x8BAEC` − `0x11F0` = `0x888FC` ✓).

**Chooser callers (fresh `get_xrefs_to 0x8B9CC` = 3):** `0x8B574`, `0x8B5FA`,
`0x8B616` — **all inside `FUN_0008AF38`** (the FU-62 clock rollover). So the
native chooser's live caller is the **clock**, not `0x74244`.

The engine mirror `fifa96_action_phase_period_end`
(`src/fifa96_loader/fifa96_action_handlers.c:1163..1210`) reproduces the above
branch-for-branch (see §5.2). It is **faithful**.

---

## 3. The "live caller `0x74244`" — enclosing function and reachability (fresh)

### 3.1 The enclosing function is `0x741C4` (reads **AL** mode; no Ghidra function)

`get_function_by_address 0x74244` = **none**; `get_function_by_address 0x741C4`
= **none**; `get_function_by_address 0x743EF` = none; `get_function_by_address
0x7440B` = none. Fresh `disassemble_bytes 0x741C4` (140 B) shows a clean standard
prologue at `0x741C4`:

```
0x741C4 PUSH EBX ; 0x741C5 PUSH ECX ; 0x741C6 PUSH EDX ; 0x741C7 PUSH ESI
0x741C8 PUSH EDI ; 0x741C9 PUSH EBP ; 0x741CA SUB ESP,4 (bytes 83 ec 04)
0x741CD MOV EBP,0x1588a4 ; 0x741D2 MOV EDI,0x1590d9
0x741D7 MOVSX CX,byte[0x157a4c] ; 0x741DF XOR ESI,ESI
0x741E5 MOV byte[0x157ab3],AH ; 0x741EB MOV byte[0x157a4f],DL
0x741F3 MOV word[0x157a64],SI ; 0x741FA MOV dword[0x157a5c],EDX
0x74200 TEST AL,AL ; 0x74202 JBE 0x7420d          ; AL <= 0  -> mode 0
0x74204 CMP AL,1  ; 0x74206 JZ  0x7424e          ; AL == 1  -> mode 1
0x74208 JMP 0x742e3                               ; AL >= 2  -> tail
```

The function's body runs `0x741C4..0x744CE` (fresh `disassemble_bytes 0x744A3`
shows the epilogue `0x744C2 MOV EAX,[ESP]; ADD ESP,4; POP…; 0x744CE RET`). Its
argument is **`AL`** (the low byte of `EAX`; calls use `AL` 0/1). The surrounding
addresses are all Ghidra functions (`FUN_00074110`, `FUN_0007412c`,
`FUN_0007417c`, `FUN_000741b4`, then `FUN_000744d4`, `FUN_000744f4`), so
`0x741C4` is a genuine, otherwise-unclaimed function entry.

### 3.2 Mode-0 arm is a near-twin of `FUN_0004B02C`'s mode-0 arm, **plus** a `[0x14C32A]` gate

Fresh `disassemble_bytes 0x741C4`/`0x74100` (bytes read first-hand):

```
0x7420d CALL 0x0007412c
0x74212 CMP dword[0x0014c2f2],0 ; JZ 0x74229
0x7421b MOV EAX,EBP ; 0x7421d CALL 0x0008cbb0 ; 0x74222 MOV EAX,EDI ; 0x74224 CALL 0x0008cbb0
0x74229 CMP dword[0x0014c32c],0 ; JZ 0x74237
0x74232 CALL 0x00078824
0x74237 CMP byte[0x0014c32a],0         ; <-- session gate
0x7423e JNZ 0x742e3
0x74244 CALL 0x00088860                ; e8 17 46 01 00
0x74249 JMP 0x742e3
```

Compare `FUN_0004B02C`'s mode-0 arm (fresh `disassemble_bytes 0x4B02C`, 140 B):

```
0x4B059 TEST EAX,EAX ; JBE 0x4B064 ; CMP EAX,1 ; JZ 0x4B095 ; JMP 0x4B0AA
0x4B064 CALL 0x0007412c
0x4B069 CMP dword[0x0014c2f2],0 ; JZ 0x4B080
0x4B072 MOV EAX,ESI ; CALL 0x0008cbb0 ; 0x4B079 MOV EAX,ECX ; CALL 0x0008cbb0
0x4B080 CMP dword[0x0014c32c],0 ; JZ 0x4B08e
0x4B089 CALL 0x00078824
0x4B08e CALL 0x00088860                ; e8 cd d7 03 00   (NO [0x14c32a] gate)
0x4B093 JMP 0x4B0aa
```

The mode-0 arms are **identical except** that Function B (`0x74237`) adds
`CMP byte[0x14c32a],0 / JNZ 0x742e3` before the `FUN_00088860` call. (Both mode-1
arms differ widely: `FUN_0004B02C` mode 1 is thin — `0x74034`,`0x7417c`, sit 0 at
`0x4B0A5`; Function B mode 1 is a large full match-init — `0x700f4`,`0x73ee0`,
`0x73e28`,`0x886d4`,`0x7d8d0`,`0x744f4`, the side-byte pair, `0x8c33c`×2,
`0x78824`, sit 0. Function B's tail `0x74427..0x74456` is a near-copy of
`FUN_0004B100`'s `0x4B2A9..0x4B2DA` tail; and Function B calls the clock
`FUN_0008AF38` at `0x7440B`.)

### 3.3 Reachability: **no static caller / reference found**

Fresh, this session:

* `get_xrefs_to 0x741C4` = **0 references**.
* `search_instructions` operand substring `741c4` over 235 032 instructions =
  **0 matches**.
* `search_instructions` operand substring `0x00074` over 235 032 instructions =
  308 matches, **none targeting `0x741C4`** (they target `FUN_00074034`,
  `FUN_0007412c`, `FUN_0007417c`, `FUN_000741b4`, `FUN_000744d4`,
  `FUN_000744f4`, `FUN_00074584`, … and *internal* branches of the region).
* `search_byte_patterns` for the LE32 absolute pointer `c4 41 07 00`
  (= `0x000741C4`) = **0 matches** (no table entry holds it).

Therefore **no statically resolvable call, jump or stored pointer reaches
`0x741C4`**. As a *live caller* of `FUN_00088860`, `0x74244` is **unsupported**:
the address is the CALL site inside a statically-unreferenced twin. Whether the
twin is genuinely dead code (a linker/Watcom duplicate of the
`FUN_0004B02C`+`FUN_0004B100` pair) or is entered by a mechanism outside the
disassembled image is **not statically provable here** → numbered leg
(**FU225-L1**).

### 3.4 The live `FUN_00088860` invocation is `0x4B08E`

`FUN_0004B02C` is reachable: fresh `get_xrefs_to 0x4B02C` = **2** =
`0x49415` (mode 1) and `0x4950A` (mode 0), both inside **`FUN_000493A0`**. Fresh
`get_xrefs_to 0x493A0` = **3** = `FUN_00018680 0x18819`, `FUN_00018108 0x18359`,
`FUN_00018014 0x180AA` (front-end). `FUN_000493A0` (fresh
`disassemble_function`, 130 instructions) is the match driver: a loop
(`0x494A6..0x4953C`) that calls `FUN_0004B02C(0)` each iteration at `0x4950A`
(`MOV EAX,EBX` with `EBX=0` from `0x494A4 XOR EBX,EBX`). So the **live**
period-setup invocation is `FUN_000493A0 → FUN_0004B02C(mode 0) → 0x4B08E →
FUN_00088860`.

For completeness, the live chooser invocation is the clock: fresh
`get_xrefs_to 0x8AF38` = **2** = `0x4B1A6` (`FUN_0004B100`) and `0x7440B`
(Function B — the dead twin). `FUN_0004B100` is reachable (fresh
`get_xrefs_to 0x4B100` = **3**: `FUN_000493A0 0x4951E`, `FUN_00049B28 0x49FBB`,
`FUN_0004A228 0x4A25A`). So the **live** chooser path is `FUN_000493A0 →
FUN_0004B100 → FUN_0008AF38 → FUN_0008B9CC` (the `0x8B574`/`0x8B5FA`/`0x8B616`
calls).

**Note (session gate, FU208-L8/FU226-owned):** the live `FUN_00088860` call at
`0x4B08E` has **no** `[0x14C32A]` gate; only the dead twin (`0x74237`) and the
`FUN_0008AF38` `0x8B65B` / `FUN_0004B100` `0x4B198` sites carry it. The engine
`session_gate_14c32a` gates the frame's screen scheduler
(`fifa96_match_run.c:4123`), not the `period_setup` call. Do **not** depend on
this slice for the session-gate verdict; stated here only as a cross-check.

---

## 4. The live invocation, drawn

```
FUN_000493A0  (match driver loop; reachable from FUN_00018014/00018108/00018680)
 ├─ 0x49415  FUN_0004B02C(mode 1)   → match-init situation 0 (0x4B0A5)   [engine: period_setup(1) @ begin, gated by period_setup_live]
 └─ 0x4950A  FUN_0004B02C(mode 0)   → 0x4B064 … 0x4B08E  CALL FUN_00088860  [engine: period_setup(0)]
     └─ FUN_00088860: phase==0x0C?  ── [0x157AC2]==4 ? sit 8 (rng&1) : sit 1 (side-controlled)  → FUN_0008A938
 ├─ 0x4951E  FUN_0004B100 → 0x4B1A6 CALL FUN_0008AF38 (clock) → 0x8B574/0x8B5FA/0x8B616 CALL FUN_0008B9CC  [engine: phase_drive → fifa96_action_phase_period_end]
 └─ (0x74244) FUN_00088860  ← DEAD TWIN (entry 0x741C4), 0 static refs → NOT the live invocation
```

---

## 5. The engine mirror and the deltas (step 3)

The engine's period-end spine is `fifa96_match_run_phase_drive`
(`src/fifa96_engine/fifa96_match_run.c:2584..2688`), called unconditionally from
`fifa96_match_run_frame` (`:4131`). It fuses the two native pieces: the chooser
(mirror of `FUN_0008B9CC`, `fifa96_action_phase_period_end`) and the setup
machine (mirror of `FUN_0004B02C`, `fifa96_match_run_period_setup`, itself
containing the `FUN_00088860` mirror).

### 5.1 `fifa96_match_run_period_setup(mode 0)` vs `FUN_00088860` — faithful

Engine (`:3199..3209`): `if (mr->state.phase == 0x0Cu) { if (mr->global_157ac2 == 4u) { rng; set_piece(8, rng&1, 0) } else { set_piece(1, side_controlled, 0) } }`.

Native (`0x88863`, `0x888ab`): `phase == 0x0C` gate; `[0x157AC2] == 4` →
`FUN_0008A938(8, rng&1, 0)` (sit 8), else `FUN_0008A938(1, [0x157AAB]>>24, 0)`
(sit 1). **Gate polarity identical.** The RNG draw `FUN_00092AC8` (native
`0x888b5`) is modeled by `fifa96_rng_step(&mr->rng,&r)` (`:3202`). The
`FUN_0008F188(0xC1,0,8)` sink and the head sinks are legs (FU225-L2).

### 5.2 `fifa96_action_phase_period_end` vs `FUN_0008B9CC` — faithful

Enumerated branch-for-branch (engine `fifa96_action_handlers.c:1163..1210`, native
`0x8B9CC..0x8BAEF`):

* **extra-time clear** (`[0x157AC0]==0` / `extra_time==0`): `act = 0x0B`; `phase
  0x0C` **only when `period < 4`** (`0x8BAA7 JH…` vs `:1174`), side
  `side_controlled`; else `phase = NONE` (no write). **Polarity identical.**
* **period < 4 vs >= 4**: `JGE 0x8BA0B` (`0x8B9F0`) vs `if (period < 4)` — same.
* **tie-break**: `SETBE` (unsigned own≤other) / equal→3 / `SETLE` (signed d8≤d9,
  `0x8BA23`) vs `score_own <= score_other` / `== → 3` / `(int8)d8 <= (int8)d9` —
  same signs/widths.
* **probes**: `0x12230(edx)` for edx∈{0,1}; `0x12250(0)`,`0x12250(1)` for edx==3
  vs `probe[edx]`, `probe[2]`, `probe[3]` — same ordering.
* **act tail**: `0x8BAC6 CMP byte[0x157AC0],0`: `EAX=0xC` if set else `0xB`;
  `0x8BAE7 CALL 0x888FC(act,0,1)`. Engine: `out.act`, and `match_run_act_dispatch`
  runs **unconditionally** after the phase write (`:2638`, mirroring the native
  `LAB_0008bac6` being outside the phase-write branch). **Same.**

So the two engine mirrors are faithful; the only *named* divergences are the
**stand-in inputs** and the **call wiring**, below.

### 5.3 Named divergences (step 3 list)

| # | dimension | native (live) | engine | verdict |
|---|---|---|---|---|
| **D1** | chooser's `period` arg | `FUN_0008B9CC` reads `[0x157AC2]` **before** the `0x8B58A` increment | passes `completed = state.period - 1` (`:2610`) | matches iff the FU-62 library's increment ≡ `0x8B58A` (documented); a derived assumption. |
| **D2** | `side_abe`/`side_abf` | live `[0x157ABE]`/`[0x157ABF]` (set by `FUN_0007417C`, e.g. `0x88874`/`0x742B4`) | hardwired `0u, 0u` (`:2625`, `:2673`) | **stand-in** (OL-75); only the extra-time arm reads them. |
| **D3** | `d8`/`d9` | live `[0x1587D8]`/`[0x1587D9]` (`0x8BA17`) | hardwired `0u, 0u` | **stand-in** (OL-75); period≥4 arm only. |
| **D4** | `probe[4]` | live `0x12230`/`0x12250` results | `{0,0,0,0}` (`:2587`) | **stand-in** (OL-74); upgrades phase 0x13→0x14 only. |
| **D5** | `FUN_00036200(3)` camera call on the edx==3 arm (`0x8BA97`) | present | unmodeled | leg (header-documented). |
| **D6** | mode-0 call gating | native `FUN_0004B02C(mode 0)` runs **every frame** and self-gates on `phase==0xC` | engine calls `period_setup(0)` **only** when `global_157ac2 == 4` (`:2654`) | the engine's mode-0 **sit-1 else-arm is unreachable from that site** (guard ⇒ index 4 ⇒ sit 8). **See FU225-L4.** |
| **D7** | extra `[0x14C32A]` gate | live `0x4B08E` has **none**; dead twin `0x74237` has it | none on `period_setup(0)` | engine matches the **live** site; the twin's gate is dead. (session gate FU208-L8/FU226) |
| **D8** | tie-break owner | W2 `0x8B2AC` / W4 `0x8B3A2` / W5 `0x8B5CD` in `FUN_0008AF38`; equal-score escapes `[0x14C2EE]`/`[0x10F378]` | `:2618..2619`, `:2664..2666`; escapes unported | W2/W4/W5 match; escapes are legs (FU213). |
| **D9** | `[0x157AB6]` reset + `[0x157AC2]` increment | `0x8B583`/`0x8B58A` | `global_157ac2 = state.period` (`:2643`) | matches (increment modeled by the library). |
| **D10** | per-frame common tail | `FUN_0004B02C` tail `0x4B0AA` (`0x8C418` refresh, bit-8 gate, `word[0x158822]=0`, `word[0x157A68]=0`, `0xCB2B6`) runs **every frame** | modeled in `period_setup` but run only on completion/begin | cadence delta (**FU225-L5**). |

---

## 6. Verdict (step 4)

**Does flipping `period_setup_live` alone suffice? No.**

* `period_setup_live` gates exactly one thing: `if (mr->period_setup_live)
  (void)fifa96_match_run_period_setup(mr, 1);` on the `begin` path
  (`src/fifa96_engine/fifa96_match_run.c:2416`; declared `:834`; seeded `0` at
  `:2092`, `1` at `:2225`). That is the **match-init situation-0 producer** (the
  native `FUN_000493A0 → FUN_0004B02C(1)`, `0x4B095`). It does **not** touch the
  period-end chooser call (`fifa96_action_phase_period_end`, `:2623`/`:2670` —
  gated on the FU-62 `clock_period_ended` completion) nor the mode-0 sit-8
  producer (`:2657` — gated on `global_157ac2 == 4`). Flipping it therefore
  changes the match-init situation 0 only.
* **The mirror need not be reconciled for faithfulness.** The engine already
  mirrors the *reachable* native faithfully (§5.1/§5.2). What remains for the
  period-end spine is (a) the chooser stand-in inputs (D2–D4, OL-74/OL-75) and
  (b) the tape actually reaching a completion. The single corrective finding of
  this slice is the **name/attribution** one: FU215-L3's "live caller `0x74244`"
  is **wrong** — `0x74244` is in a statically-unreferenced twin; the live
  `FUN_00088860` caller is `0x4B08E`.

**Hand-off to Stage 2 (T4):** do not gate or key the period-end work on
`period_setup_live`; verify the completion path directly. Treat `0x74244` as a
non-live twin (do not port its extra `[0x14C32A]` gate into the live call).

---

## 7. Residual legs (numbered; each with its native owner)

1. **FU225-L1 — Function B (`0x741C4`) reachability.** The enclosing function of
   the `0x74244` CALL has **no** statically resolvable caller/reference (fresh
   `get_xrefs_to 0x741C4` = 0; fresh `search_instructions` = 0; fresh
   `search_byte_patterns` `c4 41 07 00` = 0). It is either dead code (a
   `FUN_0004B02C`+`FUN_0004B100` duplicate) or entered outside the disassembled
   image; neither is provable statically. Corrects **FU215-L3** ("live caller
   `0x74244`"). Owner: FU215-L3 / FU196-L6.
2. **FU225-L2 — `FUN_00088860`'s head sinks and the `FUN_0008F188(0xC1,0,8)`
   sink.** The period-end screen setup `0x7417C` (side bytes), `0x6FFC0`,
   `0x700F4` (the `0x10F328/2c/30` camera triple) and `0x73E28`, plus the
   `0x8F188` sink, are not modeled in `fifa96_match_run_period_setup(mode 0)`.
   Owner: FU196-L2 family.
3. **FU225-L3 — the chooser stand-in inputs.** `side_abe`/`side_abf` (D2),
   `d8`/`d9` (D3) and the `0x12230`/`0x12250` probes (D4) are hardwired in
   `phase_drive`; their native producers are unported. Owner: OL-74 / OL-75
   (FU213).
4. **FU225-L4 — the engine's mode-0 call is over-gated.** `fifa96_match_run_phase_drive`
   calls `period_setup(mr, 0)` **only** when `global_157ac2 == 4` (`:2654`), so
   the mode-0 **sit-1 else-arm** (`0x888F2`, native `FUN_0004B02C(mode 0)` at
   period index ≠ 4) is unreachable from that site. Whether the engine's
   half-time/sit-1 route is supplied elsewhere is the FU227 slice's scope. Owner:
   FU227 / FU-208 §2.
5. **FU225-L5 — the per-frame cadence of the mode-0 machine.** Native
   `FUN_0004B02C(mode 0)` runs **every frame** (`FUN_000493A0` loop `0x4950A`)
   and its common tail (`0x4B0AA`: `0x8C418` refresh, `word[0x158822]=0`,
   `word[0x157A68]=0`, `0xCB2B6`) executes each call; the engine runs it only on
   completion/begin. Owner: phase-21 T4.
6. **FU225-L6 — the `FUN_00036200(3)` camera call** on the `FUN_0008B9CC`
   edx==3 arm (`0x8BA97`) stays unmodeled (header-documented). Owner:
   FU196-L2 / FU213.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`, `watcom:LE:32:default`,
  2706 functions, image base 0, min `0x10000`, max `.image::00174a2a`, `.image`
  overlay.
* `disassemble_function` — `0x88860` (48), `0x8B9CC` (85), `0x4B02C` (61),
  `0x8AF38` (475), `0x4B100` (140), `0x493A0` (130).
* `disassemble_bytes` — `0x88860` (150 B), `0x8B9CC` (292 B), `0x4B02C` (140 B),
  `0x741C4` (140 B), `0x74100` (360 B), `0x741F0` (300 B), `0x74317` (220 B),
  `0x743F0` (180 B), `0x744A3` (180 B).
* `read_memory` — `0x110794` (140 B; the phase table; slot `0x0C` = `0x6df4c`).
* `get_xrefs_to` — `0x88860` (**2**), `0x8B9CC` (**3**), `0x4B02C` (**2**),
  `0x493A0` (**3**), `0x8AF38` (**2**), `0x4B100` (**3**), `0x741C4` (**0**),
  `0x6DF4C` (1, DATA `0x1107c4`), `0x740A0` (27), `0x888FC` (11).
* `get_function_by_address` — `0x74244`/`0x741C4`/`0x743EF`/`0x7440B` = **none**;
  `0x74000`=`FUN_00073ee0`, `0x74050`=`FUN_00074034`, `0x74110`/`0x7412C`/
  `0x7417C`/`0x741B4`/`0x744D4` = `FUN_*`.
* `search_instructions` (program, 235 032–235 033 scanned) — operand `741c4` (0),
  operand `0x00074` (308, none targeting `0x741C4`).
* `search_byte_patterns` — `c4410700` (**0**).
* `find_code_gaps` (min 8) — used only to check for a plausible undisassembled
  caller island; inconclusive (many small gaps), recorded as part of FU225-L1.

Engine/loader source read first-hand:
`src/fifa96_loader/fifa96_action_handlers.c:1163..1210` (`fifa96_action_phase_period_end`);
`src/fifa96_engine/fifa96_match_run.c:2540..2688` (`phase_drive`, incl. `:2416`
the `period_setup_live` gate, `:2610` `completed = state.period-1`, `:2623`
`:2670` the chooser calls, `:2643` the roll, `:2654` the mode-0 gate, `:2657`),
`:3184..3220` (`fifa96_match_run_period_setup`), `:4123` (session gate), `:4131`
(frame → `phase_drive`); `include/fifa96_loader/fifa96_action_handlers.h:642..671`
(the `fifa96_action_phase_period_end` contract naming `FUN_0008B9CC`).
Docs cross-cited: FU-196 §3, FU-208 §2/§3, FU-209, FU-213, **FU-215** (§3.2/L3),
FU-216. No writes beyond this slice file; no rename/label/comment/function/script/
project save; `/FIFA96.EXE` and the ISO untouched; no git commands.

---

## Landing note

This slice **corrects the W21-3 premise**: `FUN_00088860` is not the
`fifa96_action_phase_period_end` mirror-source (that is `FUN_0008B9CC`), and its
"live caller `0x74244`" (FU215-L3) is a statically-unreferenced twin (entry
`0x741C4`, 0 refs). The **live** invocation is `FUN_000493A0 → FUN_0004B02C(mode
0) → 0x4B08E → FUN_00088860`, which the engine already mirrors faithfully via
`fifa96_match_run_period_setup(mode 0)`. Flipping `period_setup_live` alone does
**not** enable the period-end spine (it gates only the match-init `mode 1`
producer). No engine change is mandated by this slice; FU225-L1..L6 are the
residual legs.
