# FU-170 — the post-goal restart chain, engine-side (R18): sit-0xC → act 9 → sit-3 and the act-2 fold

Provenance: recon draft, phase-14 wave R18 (2026-10-12), read-only on
**`/FIFA96.EXE`** (explicit `program` argument in every call; no renames,
comments, labels, function creation, scripts or project saves). Scope: the
exact **engine-side sequence** for the M2 tape's leg-0 restart chain
(FU-165 §3), which FU-165 legs block a live port (L1/L2/L3/L7 named in the
plan; L4..L6/L8/L9 assessed in passing), the FU-166-L6 act-2 fold, and the
minimal T3 wiring rec. All windows below are first-hand this slice unless
marked *cross-cited* (then the frozen FU doc is named).

Fresh censuses this slice (all first-hand): `get_xrefs_to 0x740A0` = **27**
(matches FU-143 §3.1 / FU-165 §1); `search_instructions` CALL `0x8A938` =
**39** (matches FU-149 §1 / FU-165 §1; the two leg-0 call sites among them);
`word[0x15881A]` sites = **21** (3 writers, 18 reads); `[0x158816]` dword
sites = **19** (reads only — no writer of the dword or of `word[0x158816]`);
`word[0x158818]` sites = **82**; the `[0x15882A]` cell = **24** sites.
Tables re-read fresh: leg table `0x93B80`, leg-0 id table `0x93B98`, act-9
stage table `0x88F3C`, act handler table `0x1107EC`, situation table
`0x8A904`.

**Answer in one line.** The tape's leg-0 restart is a three-link chain the
engine already half-owns — step 0 queues situation 0xC (`0x93C41`) → the
latched-direct dispatcher (`0x8A964`) reaches the act-9 entry `0x8AF0E`
(`FUN_000888FC(9, stage 0, invoke)`) → act 9 (`0x88F4C`, stages 0..3) writes
phase 0x15 and, after its **stage-3 timer** `word[0x158818] >= 0x3C`
(~30 granted frames — **FU165-L2 as written is falsified**: the alias target
is `word[0x158818]`, the act timer, not the config cell `word[0x15881A]`),
sets `[0x15882A] = 1` → step 1's gate opens → step 2 queues situation 3
(`0x93CE4`) → the `0x8AB08` arm (clean formation counters → normal table
`0x8ABF3` → corner counter + **phase 4**) → the already-wired corner taker
0x11 → phase 2. The engine is missing exactly **three links**: the SETUP0
sit-0xC call, a derived act-9 producer (phase 0x15 + the 30-frame timer), and
the SETUP2 sit-3 call; every downstream consumer is already wired. FU166-L6's
armed act-2 split is **not re-entered** by this chain (different
situations/acts; the native singleton act slot `[0x158808]` is overwritten by
act 9, never resumed).

## 1. Method and traps

* **Alias rule applied at every load** (AGENTS.md): `dword[a]>>16` = word at
  `a+2`, `>>24` = byte at `a+3`. **FU-165's method note mis-applies it once**:
  it lists `0x158816` → `word[0x15881A]`; the rule gives `0x158816+2 =
  0x158818` (§9 erratum). Its other rows (`0x15881C`→`0x15881E`,
  `0x15881E`→`0x158820`, `0x158828`→byte `0x15882B`, `0x158829`→byte
  `0x15882C`, `0x15882B`→byte `0x15882E`, `0x15882C`→byte `0x15882F`) are
  correct.
* CALL targets decoded from the CALL bytes (rel32 resolved by Ghidra);
  load-bearing rel32s re-checked by hand (`0x93C41 → 0x8A938`, `0x93CE4 →
  0x8A938`, `0x8AF1A → 0x888FC`, `0x8AB57 → 0x888FC`, `0x93DB2 → 0x740A0`,
  `0x93DD6 → 0x935A0`).
* Fresh `get_xrefs_to` for the caller-count claims (27/39 above).

## 2. Evidence — the leg-0 handler chain (all steps re-read this slice)

Fresh `disassemble_bytes 0x93BBC..0x93DE3` (139 instructions). The head
`0x93BC4..0x93BD7`: `[0x15B688] += word[0x157A64]` (the granted-frame delta),
`EAX=[0x15B6B0]`, `CMP EAX,5 / JA 0x93DDB`, `JMP CS:[EAX*4+0x93B80]`.
Fresh `read_memory 0x93B80` (24 B) = `{0x93BEE, 0x93C59, 0x93C7B, 0x93CFE,
0x93D2A, 0x93DCA}` — the six steps of FU-165 §1/§3 stand.

* **Step 0 `0x93BEE..0x93C58`** (re-verified): the ±0x720 hint write
  `[0x157A77]` (`0x93BF7` `c7 05 77 7a 15 00 20 07 00 00`); `0x93C24 XOR
  AH,AH`; `0x93C2A [0x157A7B]=0`; **`0x93C30 MOV byte [0x15882A],AH`**
  (`88 25 2a 88 15 00`); `0x93C3B [0x157A7F]=0xB10`; **`0x93C36 MOV EAX,0xC`
  + `0x93C41 CALL 0x8A938`** (`e8 f2 6c ff ff`; EBX=0 `0x93C26`, EDX=0
  `0x93C28`) — situation 0xC, side 0, BX=0; `0x93C4D [0x15B688]=0` (EDI=0);
  `0x93C53` step++ → **falls into step 1 in the same call**.
* **Step 1 `0x93C59`**: `CMP byte [0x15882A],0 / JZ 0x93DDB` — the gate
  waits on the act-9 producer.
* **Step 2 `0x93C7B..0x93CE4`**: `[0x15781D]=1`; the MOVSD copies
  `0x158897→0x15777C` and `→0x15B6C8`; the duration seed
  `0x1110EC[mode*24+leg]` (the same table `screen_install` uses); `word
  [0x157AB4]/[0x157AB6]=0`; **`0x93CDD MOV EAX,3 / 0x93CE2 XOR EDX,EDX /
  0x93CE4 CALL 0x8A938`** (`e8 4f 6c ff ff`) — situation 3, side 0, BX=0;
  step→3.
* **Step 3 `0x93CFE`**: `MOV EAX,[0x157A4A] / SAR 0x18 / CMP EAX,2 / JNZ
  0x93DDB`; on phase 2 `[0x15B6C0]=0` then step++.
* **Step 4 `0x93D2A..0x93DC5`**: `CMP [0x15B6C0],0 / JZ`; re-latch
  `[0x15B6C0]=1`; `CALL 0x974DC`; id read; `IDIV 0x3C`; the `0x93B98` id
  table (fresh 36 B: `{0x93D93,0x93D9F,0x93D93,0x93DA8,0x93D9F,0x93DA8,
  0x93DA8,0x93DA8,0x93D93}` = ids `{1,0,1,-1,0,-1,-1,-1,1}`); writers
  `0x93944(1)`/`0x93944(0)`/no-score `[0x15B6A0]++`; **`0x93DAE XOR EDX,EDX /
  0x93DB0 XOR EAX,EAX / 0x93DB2 CALL 0x740A0`** — phase 0; then
  `0x93DBF [0x15B688]=0` and step++.
* **Step 5 `0x93DCA`**: `CMP dword [0x15B688],0xB4` (`81 3d 88 b6 15 00 b4 00
  00 00`) / `JLE 0x93DDB` / `CALL 0x935A0` — the pure timer wait. With
  `word[0x157A64] = 2` per granted frame the advance fires on the **91st
  post-goal granted frame** (timer 182 > 180); the tape's 20-step window
  (~6 grants) is ~85 granted frames short.

**The latch lifecycle that makes the chain direct** (fresh census of
`[0x15B6C0]`, 32 sites): the POST re-latches (`0x93D46`, `MOV ECX=1`); only
the step-3 PHASE gate clears it (`0x93D17`, EBP=0); `FUN_00092D8C`'s install
clears it (`0x92DCD`, EDI) and the dispatcher head tests it (`0x8A964`). The
advance `FUN_000935A0` does **not** touch it, so the re-entered step-0 sit-0xC
call takes the direct route. Engine mirror: `situation_pending` (POST
re-latch `:2873`; step-3 clear `:2974`; dispatcher head `set_piece` `:2643`).

### 2.1 Dispatcher head (fresh `0x8AA54..0x8AACB`)

`0x8AA7B TEST BX,BX / JZ 0x8AAA8` (latched-direct, BX=0) → `0x8AAA8 CMP CX,1
/ JC 0x8AB63` (sit 0 → tail); `JBE 0x8AABF` (sit 1); `CMP CX,4 / JBE 0x8AB08`
(sits 2..4); **`JMP 0x8AB63`** (sit 0xC > 4 → tail). Tail `0x8AB63..0x8AB7A`:
`[0x157AAF]=sit; CMP CX,0xC / JA 0x8AF1F; JMP CS:[EAX*4+0x8A904]`. Fresh
`read_memory 0x8A904` (52 B = 13 entries): `[3]=0x8ABF3` (sit 3),
`[0xB]=0x8AEF6` (sit 0xB, `0x8AF02 CALL 0x740A0(2,side)` — the sole phase-2
site), **`[0xC]=0x8AF0E`** (sit 0xC).

### 2.2 Act-9 entry and the sit-3 arm

* `0x8AF0E..0x8AF25` (fresh): `MOV EBX,1 / MOV EAX,9 / XOR EDX,EDX / CALL
  0x888FC` — **act 9, stage 0, invoke**. `FUN_000888FC` head (fresh
  `0x888FC..0x88929`): `MOVSX CX,[0x158828]; CMP AX,CX; JZ 0x8893D` (same
  code → invoke without resetting the stage); else `[0x158828]=AL` (code),
  `EAX = 0x1107EC + code*4`, `[0x158829]=DL` (stage), `word[0x158818]=0`,
  `[0x158808]=[EAX]` (handler). The act table fresh `0x1107EC` (64 B):
  `[4]=0x89620`, `[8]=0x8A798`, **`[9]=0x88F4C`**, `[0xC]=0x8B900` — FU-165
  §3.2 stands.
* `0x8AB08..0x8AB61` (fresh): `EAX=0x1588A4; CALL 0x8C974; EAX=0x1590D9;
  CALL 0x8C974` (formation-order scans); `[0x157A49]>>24 == 1 → 0x8AB63`;
  `byte[0x157B8E]==0` and `byte[0x157B8F]==0 → 0x8AB63`; else
  `EAX=[ESP]` (situation), `EBX=1`, `XOR EDX,EDX` (stage 0),
  `word[0x158820]=AX` (side), `EAX=4`, `word[0x15881E]=CX` (situation),
  `CALL 0x888FC` — **act 4, stage 0**.
* `0x8ABF3..0x8AC27` (fresh, the sit-3 normal-table entry): `EAX=[ESP-2]>>16;
  CALL 0x741B4` (the `side ^ [0x157ABE]` helper); `BX=[EAX*2+0x157AD4]; INC;
  store` — **the corner counter**; `EDX=[ESP-2]>>16; EAX=4; CALL 0x740A0` —
  **phase 4** (the corner arm). So on clean formation counters sit 3 never
  reaches act 4/act 8; it goes straight to the corner counter + phase 4.

## 3. Evidence — act 9 (`0x88F4C..0x890E9`, fresh, 120 instructions)

* Head `0x88F51`: `AX=[0x157A64]; DX=word[0x158818]; ADD EDX,EAX;
  word[0x158818]=DX` — **the act timer is `word[0x158818]`**; `AL=[0x158829]
  / CMP AL,3 / JA return / JMP CS:[EAX*4+0x88F3C]`. Fresh `read_memory
  0x88F3C` (16 B) = `{0x88F81, 0x88FED, 0x89064, 0x890B1}` (stages 0..3).
* **Stage 0 `0x88F81..0x88FEC`**: camera reset to the `0x10F328/2C/30`
  triple (`PUSH 0,… / CALL 0x700F4`); `[0x15882A]=0` (`0x88F9A`); `CALL
  0x73E28`; `EDX=[0x157AAC] SAR 0x18; EAX=0x15; CALL 0x740A0` — **phase 0x15
  on the controlled side**; `CALL 0x73E08`; the `0x4C324` presentation;
  `[0x158818]=0`, stage→1 (`0x88FD6..0x88FE7`). Then the leg router
  `0x88FED..0x89010`: legs **0/1/3 fall into the stage-1 body at `0x89015`
  in the same call**; legs 2/4/5 zero the timer and stage += 2 (skip to 3).
* **Stage 1 `0x89015..0x89063`**: `[0x14C15A]=1`; `CALL 0x74110` → `TEST
  EAX / JZ return` (stays at stage 1 while the walk returns 0); `CALL
  0x93850(0)` / retry with 1; `[EDX+0x19]>>24; CALL 0x361B0`; stage→2
  (timer zeroed), falls into stage 2 in the same call.
* **Stage 2 `0x89064..0x890B0`**: `CALL 0x93850(0)` / retry 1; `DX=[EAX+4];
  DH=0; DL &= 0x70`; `==0 → CALL 0x93930; return` (stays at stage 2);
  else `CALL 0x361A4`; stage→3 (timer zeroed), falls into stage 3.
* **Stage 3 `0x890B1..0x890E3`**: `MOV EAX,[0x00158816]` (`a1 16 88 15 00`)
  → `SAR EAX,0x10` (`c1 f8 10`) → **`word[0x158818]`** — `CMP EAX,0x3C /
  JL return`; else `[0x15882A]=1` (CL=1), `word[0x158818]=0`,
  `[0x158808]=0` (act uninstalled), `[0x158828]=0`, `[0x158829]=0` — **the
  act ends and the gate opens**.

**The act tick** (fresh `0x8B650..0x8B68F`): `0x8B65B CMP byte[0x14C32A],0 /
JNZ 0x8B669; 0x8B664 CALL 0x8BAF0` (the ball machine, byte-0 only);
**`0x8B669 CMP dword[0x158808],0 / JZ 0x8B678 / 0x8B672 CALL dword
[0x158808]`** — inside `FUN_0008AF38`'s tail, so **every act ticks once per
granted frame, any phase, regardless of the session byte**.

**Timing consequence (corrected).** Stage 3 is a timer wait: 0x3C units / 2
per granted frame = **30 granted frames** after the stage-2→3 transition
(zeroed in the same call). When the stage 1/2 gates pass in the queue frame
(a free record exists and the `0x93850` slot's `[+4] & 0x70` is nonzero), the
whole act is **1 + 30 granted frames**; when they do not, stage 1/2 loop
(FU165-L1, unchanged).

## 4. Evidence — the `word[0x15881A]` erratum (FU165-L2 falsified)

Fresh census: `word[0x15881A]` has exactly **three writers** and is otherwise
read 18 times:

| writer | bytes | value |
|---|---|---|
| `0x375E4` `FUN_0003749C` | `66 a3 1a 88 15 00` | `AX` (a config loader; also writes `word[0x15881C]` at `0x375ED`) |
| `0x4B514` `FUN_0004B508` | `66 89 15 1a 88 15 00` | `DX = 0x3C`; `0x4B51B` sets `word[0x15881C] = 0x1E` |
| `0x88710` `FUN_000886D4` (the match reset) | `66 a3 1a 88 15 00` | `AX = byte[0x14C1D1] * 0x3C` (`0x8870D IMUL EAX,EDX,0x3C`, `DL=[0x14C1D1]`); `0x88730` then `word[0x15881C] = byte[0x14C1D1] * 0x14` |

`[0x158816]` has **no writer at all** (19 dword reads). The act heads and
stage transitions own `word[0x158818]` (82 sites). So:

* `dword[0x158816] >> 16` = `word[0x158818]` = **the act/machine timer** — the
  value the act gates compare. FU-143 §10/§11's timer reading and the
  engine's tick-based act-1 producer (`fifa96_match_run.c:3476`) are correct.
* FU-165 §3.3/§3.6 ("`[0x158816]>>16 = word[0x15881A]`") and FU165-L2
  ("config/timeline gates, not timer waits") are **falsified** by the
  arithmetic in the same doc's own method note (`dword[a]>>16 = word[a+2]` →
  `0x158816+2 = 0x158818`). Consequence for the rec: FU-165 R1(b)'s
  "effectively the next frame" compression would move the restart ~30 granted
  frames (~90 engine steps) early; the native-shaped gate is the act timer.

## 5. Engine mapping — what is wired vs missing

| native piece (leg 0) | engine status (fresh repo reads) |
|---|---|
| TAIL timer > 0xB4 → `FUN_000935A0` → re-install + step 0 | **WIRED** (`match_run_screen_advance`, `fifa96_match_run.c:2999`, called at `:2990`) |
| step 0 `[0x15882A]=0` (`0x93C30`) | **WIRED** (SETUP0 `:2936`) |
| step 0 sit-0xC queue (`0x93C41`) | **MISSING** — SETUP0 is a bare gate clear |
| sit-0xC → act 9 stage 0 (`0x8AF0E` → `0x88F4C`) | **MISSING** — `match_run_situation_side` only acts on `out.act == 2` (`:2515`); situation 0xC's row is `{NONE, act 9}` (`fifa96_action_handlers.c:1145`) so `set_piece(0xC)` is currently a no-op; the engine has no act machine |
| act 9 stage 0 phase-0x15 write | **MISSING** — `match_run_write_phase` accepts 0x15 (rows table `fifa96_action_handlers.c:1092`; class 0), but nothing calls it; the phase-0x15 arm (`0x8D040[0x15]=0x8D77B`) is missing from `match_run_phase_arm` (`:2395` — covers 3/4/6/7/8/9/0xD) |
| act 9 stage-3 timer → `[0x15882A]=1` | **MISSING** — this is the GATE producer (the engine's second freeze layer) |
| step-1 gate `[0x15882A]!=0` | **WIRED** (GATE `:2950`) but frozen: SETUP0 clears it and nothing re-sets it at phase 0/0x15 |
| step 2 sit-3 queue (`0x93CE4`) + the two `0x157AB4/6` clears | **MISSING** — SETUP2 is a bare step++ (`:2955`); the engine has no `0x157AB4/6` fields (only a comment reference `:3756`) |
| pan re-arm + `0x158897` MOVSDs | **SKIPPED by decision** (FU-146 §11 errata 3) |
| duration re-seed (step 2 `[0x15B694]`) | WIRED in `screen_install` at begin instead (errata 2); the native step-2 re-seed writes the same mode/leg value, so no engine delta |
| sit 3 (clean counters) → corner counter + phase 4 + arm | **WIRED** (`set_piece` direct → `match_run_situation_side(3,0,1)` → `{phase 4, corner_increment 1}` → `match_run_write_phase` + `fifa96_match_run_phase_arm(mr, 4)`, `:2505..2509`) |
| corner taker row 0x11 | **WIRED** (`fifa96_match_action_11`, `fifa96_match_handlers.c:1630`; stage 0 `timer89 < 0x3C` = 30 granted frames; stage 7 resolves via `match_sp_resolve` → `fifa96_match_run_situation(0xB)` → phase 2) |
| step-3 PHASE gate + step-4 POST | **WIRED** (`:2970`/`:2978`) |
| act-4 dirty-counter path + act 8 + `0x110680[code]&1` + `0xF0` timer | **MISSING** (FU-149 L12; clean counters never reach it) |
| act-2 armed split (sit 9/0xA family) | WIRED as the FU-166 model (`match_run_ref_step_restart`, `:5104`) — not on the leg-0 path |

**Frame budget for the tape extension (engine steps).** The tape grants one
frame ≈ every 3 steps (measured: the v10 comment's grants at steps
47/50/53/56/59/62). The TAIL is 91 granted frames ≈ 273 steps; act 9 adds
≥ 31 granted frames ≈ 93 steps (1 cascade + 30 stage-3); the corner taker adds
its 30-frame stage-0 wait plus the delivery cascade ≈ ≥ 100 steps. So the
restart completes roughly 460–600 steps past the goal (~frame 66 → ~step
530–670), i.e. a v14 tape in the ~540–700-line range — additive (the first 85
lines cannot move: the advance fires ~85 granted frames later than the tape's
current stop).

## 6. The FU-166-L6 fold — does the restart re-enter the armed split?

No, on both readings:

1. **Different machines.** The act-2 armed split lives in the act slot
   `[0x158808] = 0x8922C` (installed by `FUN_000888FC(2, stage)` from the
   sit-9/0xA rows `0x8AEC6`/`0x8AEDE` and the contact sequences). The leg-0
   chain's act invocations are act 9 (`0x8AF0E`) and, only on the
   dirty-counter path, act 4 (`0x8AB57`); the sit-3 clean path invokes no act
   at all. Nothing on the leg-0 path calls `FUN_000888FC(2, …)`, and the
   stage-3 gate's cells (`[0x158882]` ball-row terminal flag, `[0x14C32A]`)
   are not written by the chain (fresh: act 9 touches neither). The chain's
   only shared cell with act 2 is `[0x15882A]` (act 9 stages 0/3; act-2
   teardown `0x895EC` sets, completion `0x8955F` clears) — and since only one
   act is installed at a time, only one of them can be mid-flight.
2. **The singleton slot kills, not resumes.** If an act-2 split were armed
   (stage 3 held under byte 0) when sit 0xC dispatched, `FUN_000888FC` would
   overwrite `[0x158808]` with `0x88F4C` — the act-2 machine is silently
   terminated (no teardown poke). Conversely the armed split only exists
   under byte 0 (where the goal path never queues a goal and the screen
   machine never runs; `[0x14C32A] == 0` makes `0x89487` abort) — so the two
   cannot co-occur in a native session either: under byte 1 the split passes
   in one call (`!= 0` term), under byte 0 the restart chain never starts.
   **Net: the restart neither re-enters nor observes the armed split.**

**Engine-side residual (bounded, numbered §8-L5):** the port models the act-2
split as a separate `ref_machine` cell that keeps ticking every granted frame
(`:3538`), while the native slot is a singleton an act-9 install would
overwrite. A future fixture that stages a held act-2 split and then a queued
goal would run both machines where the native runs only act 9. No current
tape/fixture does this.

## 7. The minimal live sequence rec for T3

Scope note first: the goal-consumer screen machine (and therefore this whole
chain) only runs under **`session_gate_14c32a != 0`** — natively the Practise
(byte-1) session. The engine's settled live default is byte 0 (ordinary
match: the direct `0x8AA7B` goal arm, no screen). So T3's acceptance surface
is the staged-gate M2 tape (v13 → v14) and any byte-1-staged fixture — a live
SDL smoke of the restart needs a queued-goal scenario staged with gate 1.
State this in the T3 test header (it is not a defect of the rec).

Working on `src/fifa96_engine/fifa96_match_run.c` (leg 0 only):

1. **SETUP0 (leg 0; legs 1/3 share the step-0 shape but need their own
   step-2 queues — §8-L3)** — after `mr->global_5882a = 0` (`:2936`), call
   `(void)fifa96_match_run_set_piece(mr, 0x0Cu, 0u, 0u);` (the native order
   `0x93C30` then `0x93C41`). The latched-direct route is preserved because
   `situation_pending` is still 1 (the POST re-latch) — no queue-arm change.
2. **Derived act-9 arm** — in `match_run_situation_side` (symmetry with the
   act-2 branch at `:2515`), add `if (out.act == 9u)`: (a)
   `match_run_write_phase(mr, 0x15u)` (the native stage-0
   `FUN_000740A0(0x15, controlled)`); (b) arm the derived act state
   (`act9_armed = 1; act9_timer = 0`). Do **not** clear `situation_pending`
   (the native sit-0xC dispatch does not touch `[0x15B6C0]`).
3. **Derived act tick** — in `fifa96_match_run_frame`, inside the
   `for (granted)` body **after `match_run_ball_machine(mr)`** (`:3571`;
   native `0x8B672` follows the ball machine `0x8B664`): if `act9_armed`,
   `act9_timer += state.frame_delta`; at `>= 0x3C` set `global_5882a = 1u`
   and disarm (the native stage-3 timer; stages 1/2 are compressed to pass —
   L1). Placing it there gives the port one honest act-tick site; leave the
   existing act-2 model where it is (`:3538`) and note the ordering
   divergence in the comment.
4. **SETUP2 (leg 0)** — `(void)fifa96_match_run_set_piece(mr, 3u, 0u, 0u);`
   (the native `0x93CE4`). The two `word[0x157AB4]/[0x157AB6]` clears need
   fields that do not exist; skip with a comment (display-only, §8-L2).
   (Native legs 1/3 queue sit 0xA/sit 2 here — not T3.)
5. **Nothing else.** Step 1's GATE opens on `global_5882a`; step 2's sit-3
   direct path already counts the corner and writes+arms phase 4; the phase-4
   arm installs action 0x11; the corner taker machine already runs (its stage
   0 waits 0x3C = 30 granted frames); its stage-7 resolve already writes
   phase 2; step 3 clears the latch and step 4 POSTs for the next situation.
6. **Acceptance (T4).** Extend the M2 harness past `M2_POST_GOAL_FRAMES` (the
   break at `tests/test_engine_m2.c:2413`) to the restart completion (e.g.
   `screen_step == 4` after a phase-2 resume, or a cap ≈ 700 steps); the
   first 85 lines must stay byte-identical (the advance is ~85 granted frames
   beyond the current stop — additive v14). The m 72 / `M2_M_FINAL` strata
   (`mask_final == M2_WIRED_MASK`, `(dispatched_ok & M2_TAKER_ROWS_MASK) == 0`,
   the phase/queue pins) must be re-scoped: the extension adds the taker rows
   (0x11 dispatch, relay code 4, `corner_count` 1) and the state line gains
   `phase=0x15` then `phase=4` windows. Wired census unaffected (no action
   row joins; the chain is screen-machine + one derived act).

## 8. Numbered legs (FU-170)

1. **FU170-L1 — act-9 stage 1/2 gate fidelity** (inherits FU165-L1): the
   `0x74110` walk, the `0x93850` 4-slot `[+4]&0x70` gate with `0x93930`, the
   `0x361B0`/`0x361A4` presentations and `[0x14C15A]=1` are byte-derived only;
   the derived route always passes them. Divergence only when a native run
   blocks at stage 1/2 (extra frames). Settle by a dynamic trace of
   `[0x158829]`/`[0x15882A]` after a native goal.
2. **FU170-L2 — act-9 stage-0 side effects.** The camera reset
   (`0x700F4` with the `[0x10F328/2C/30]` triple), `FUN_00073E28`,
   `FUN_00073E08`, the `0x4C324` presentation and the phase-0x15 arm
   (`0x8D040[0x15] = 0x8D77B`, install 3 — also FU165-L8) stay unported; the
   derived route writes phase 0x15 only, so records keep the pre-goal
   positions into phase 4 (FU-165 R1 risk, now scoped).
3. **FU170-L3 — per-leg re-queues (legs 1/2/4/5).** Their step-0/step-2 sit
   calls (`0x93EC1/0x93F75`, `0x94120`, `0x945A5`, `0x947AB`) and their
   downstreams (act 2 wired; act 1 and row 0x1D missing) are not wired by
   T3; the leg table is front-end-selected (FU165-L3/L4).
4. **FU170-L4 — the act-4/act-8 dirty-counter path.** `[0x157B8E]/[0x157B8F]`
   formation-order counters, the `[0x157A49]>>24 == 1` gate, act 4's stage-1
   `0x110680[code] & 1` flag + `word[0x158818] >= 0xF0` timer, the act-8
   re-dispatch (the engine's P1 `bx` compression covers the end state). Only
   reachable when substitutions/formation changes dirty the counters during
   the goal; the M2/derived sit-3 always takes the clean path.
5. **FU170-L5 — act-slot singleton vs the engine's separate `ref_machine`.**
   A native act-9 install overwrites an armed act-2 handler; the engine would
   run both. Combined contact+goal scenarios only; bounded.
6. **FU170-L6 — `word[0x15881A]` live semantics.** The cell is written by the
   reset (`[0x14C1D1]*0x3C`), `FUN_0004B508` (0x3C) and `FUN_0003749C`, and
   read by the clock body/ball machine; the act gates do not use it. Needed
   only if a port models the native clock cells directly (FU165-L2 re-scoped,
   not closed).
7. **FU170-L7 — acceptance measurement** (inherits FU165-L7): no native run
   measures the act-9 stage 1/2 durations or the corner taker's natural
   resolution from the goal-celebration positions; the extended tape measures
   the derived route only. The rec's ~460–600-step budget is derived, not
   measured.
8. **FU170-L8 — derived-route timing compression sign-off.** The 30-frame
   stage-3 timer is modeled, but the stage 0→1→2→3 single-frame cascade and
   the skipped presentation make the derived restart timing
   `native - (stage1/2 extra frames) + 0`; record the frame diff in the v14
   re-pin reason (T4).

## 9. Discrepancies / errata (prior sections not rewritten)

* **FU-165 §3.3 / §3.6 / L2 alias error (the load-bearing one).**
  Quoted from FU-165: "`0x158816` → `word[0x15881A]`" and "the act-9 stage-3
  `>= 0x3C` ... are therefore config/timeline gates, not timer waits".
  Corrected (§4 above): the alias target is `word[0x158818]` (the act timer),
  so the stage-3 gate — and act 1's `0x78` gate and act 4's `0xF0` gate — are
  timer waits. FU-143 §10/§11 stand; FU-165 R1(b)'s "effectively the next
  frame" and FU165-L2 as written dissolve (re-scoped as FU170-L6).
* **FU-165 R1(b) timing.** The native act-9 compression cost is ~31 granted
  frames, not one frame; the tape-extension budget in FU-165 §7 R1 ("≥91
  granted frames past frame 66") is the TAIL only — add ~31 + the taker
  cascade (this slice).
* **FU-165 §3.3 act-9 stage-3 wording "word[0x15881A]"** also propagated into
  the §9 risk list ("Alias traps at `0x158816`/…"); the trap list's rule is
  right, its example target is not.

## 10. Method record (tool calls, read-only)

`disassemble_bytes`: `0x93BBC..0x93DE3`, `0x88F4C..0x890EF`,
`0x8B650..0x8B68F`, `0x88EE0..0x88F07`, `0x886D4..0x88742`,
`0x4B508..0x4B547`, `0x3749C..0x374F2`, `0x888FC..0x88929`,
`0x8AB08..0x8AB61`, `0x8ABF3..0x8AC2A`, `0x8AF0E..0x8AF25`,
`0x89480..0x894DF`, `0x895C0..0x895FF`, `0x89775..0x897EC`,
`0x8A798..0x8A7C7`, `0x8AA54..0x8AACB`.
`read_memory`: `0x93B80` (24 B), `0x93B98` (36 B), `0x88F3C` (16 B),
`0x1107EC` (64 B), `0x8A904` (52 B).
`search_instructions`: `0x0015881a` (21), `0x00158816` (19),
`0x00158818` (82), `0x0015881c` (9), `0x0015882a` (24), `0x0015b6c0` (32),
`0x00157a64` (80+), CALL `0x0008a938` (39). `get_xrefs_to 0x740A0` (27).
Engine reads: `fifa96_match_run.c` (`:2395`, `:2492..2526`, `:2637..2691`,
`:2864..2915`, `:2917..2997`, `:2999..3031`, `:3069..3113`, `:3441..3586`,
`:5060..5153`), `fifa96_match_handlers.c:1421..1475/1623..1764`,
`fifa96_action_handlers.c:1120..1160/1070..1108`,
`fifa96_match_state.c:47..102`, `tests/test_engine_m2.c:1182..1198/1628..2157`
and the golden `tests/golden/engine/m2-frames.txt` (tail). No writes: no
rename/comment/label/function/script/project save; the retail ISO was not
read or written.
