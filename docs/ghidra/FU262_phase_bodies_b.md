# FU-262 (W25-7) — the unwired phase bodies B: rows `13`/`14`/`1A`/`1B`/`1C`

Provenance: M2 phase-25 wave **W25-7** (FU-262), 2026-10-14, **read-only** recon.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive
program argument; the 16-bit loader `/fifa96.exe` and the flat `/fifa96_le.bin` are
the wrong targets). Fresh `get_current_program_info` this session: **2706 functions /
15239 symbols**, `watcom:LE:32:default`, `min_address 00010000`, `max_address
.image::00174a2a`, one `.image` overlay (raw unloaded file bytes, not engine code —
every address below is default-space runtime). Every xref count, table byte, call
byte and instruction below is a fresh `get_xrefs_to` / `read_memory` /
`disassemble_bytes` of **this** session unless marked *cross-cited* (then the frozen
FU doc or engine source is named). Alias traps applied (`dword[x]>>16` = the word at
`x+2`; `>>24` = the byte at `x+3`). Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_handlers.c` (the phase table), `fifa96_match_run.c`
(the writer + collapsed arm), `fifa96_match_act.c`, `fifa96_match_phase_machine.c`,
`src/fifa96_loader/fifa96_action_handlers.c` (the chooser). Docs cross-cited:
`docs/ghidra/FU192_record_phase_handlers.md`, `FU239_et_rearm_and_live_phase.md`,
`FU237_extra_time_terminal.md`, `FU240_phase_1314_producer.md`,
`FU193_act_tail_17_1a_1c.md`, `FU195_act_tail_21_22.md`, `FU244_row_1a_ladder.md`,
`FU201_order_matrix_act4_arm.md`, `docs/ENGINE.md` (`:8`/`:268` the 58/80 census).
**No writes**: no rename/comment/label/function/script/project save; no git; no code;
no tests; ISO and `game/hdd/` untouched. Draft + run report only.

---

## Answer in one line

**All five slots (`0x13`,`0x14`,`0x1A`,`0x1B`,`0x1C`) stay NULL/uncounted — none is
wired by this recon.** The engine phase table `0x110794` (fresh) points `[0x13]` and
`[0x14]` at the shared per-record scatter body `0x6E244`, `[0x1A]` at `0x89620`
(act-4 formation walk), `[0x1B]` at `0x890EC` (a 13-insn pure reset) and `[0x1C]` at
`0x89110` (act-6 pre-restart timeline). The **phase-NUMBER** producers: `0x13` is
written by a live engine path (the ET chooser → `match_run_write_phase`); **`0x14` is
not** (the engine's probe cells are hardwired `{0,0,0,0}`); **`0x1A`/`0x1C` have
statically-present but dormant/duplicated producers** (act-4 formation residual;
act-6 offside machine); **`0x1B` has no producer at all**. The FU-239 collapsed arm
is real and deliberate: `fifa96_match_run_end_word` merges `0x13`/`0x14` into one
`act.timer > 0x4B0 → 2` arm and **omits** the native phase-`0x13` score-equality
guard `0x8BD95` (`[0x157AC5]==[0x157AC7]`), because adding it would hold the tape's
1-0 forced-ET terminal. Census headline unchanged (**58/80 = 72%**); M2 byte-identical.

---

## 1. Verdict table

| row | native body (`0x110794[n]`) | what it is | live engine writer of the phase NUMBER? | census |
|---|---|---|---|---|
| **13** | `0x6E244` (`0x1107E0`) | per-record camera-scatter (FU240) | **YES** — ET chooser `fifa96_action_phase_period_end` → `match_run_write_phase(0x13)`; v21 golden reaches it | NULL/uncounted (+0) |
| **14** | `0x6E244` (`0x1107E4`) | same body as `13` | **NO** — chooser 0x14 arms gated on probe cells hardwired `{0,0,0,0}` (OL-74/OL-75) | NULL/uncounted (+0) |
| **1A** | `0x89620` (`0x1107FC`) | act-4 formation-order/line-up walk | **NO** — act-4 producers exist (`0x88EE8`,`0x8AB57`) but gated dormant (FU-201/R49) | NULL/uncounted (+0) |
| **1B** | `0x890EC` (`0x110800`) | 13-insn pure reset (timer/stage/act-slot = 0) | **NO** — no act-5 invoker, no `0x740A0` writer | NULL/uncounted (+0) |
| **1C** | `0x89110` (`0x110804`) | act-6 pre-restart timeline (offside seq) | **NO** — native act-6 at `0x8A789`; engine drives the offside machine directly (double-run) | NULL/uncounted (+0) |

Net: **no row changes ceiling.** All five are the `{0x1A, NULL, …}` /
`{0x1B, NULL, …}` / `{0x1C, NULL, …}` slots in `fifa96_match_handlers.c`; the bodies
`fifa96_match_action_phase_1A` (`match_run.c:6178`) and `fifa96_match_action_phase_1C`
(`match_run.c:6238`) exist only for direct unit testing.

---

## 2. Step 1 — `13`/`14`: the bodies + the FU-239 collapsed arm

### 2.1 The phase table (fresh `read_memory 0x110794`, 128 B)

Row `n` is the dword at `0x110794 + 4n`; the act base is `0x1107EC`
(`= 0x110794 + 0x16*4`). Fresh bytes:

```
[13]=0x0006E244  [14]=0x0006E244  [15]=0x0006DCC8  [16]=0x00000000
[17]=0x00088DC8  [18]=0x0008922C  [19]=0x00089FA4  [1A]=0x00089620
[1B]=0x000890EC  [1C]=0x00089110  [1D]=0x00089868  [1E]=0x0008A798  [1F]=0x00088F4C
```

Fresh `get_xrefs_to 0x6E244` = **2 DATA** (`0x1107E0`, `0x1107E4`) — the two slots
are the body's **only** references. Fresh class table `0x1106AD` (24 B) + `0x1106C5`
(16 B): classes `[13]=[14]=0`, `[1A]=[1C]=[1D]=[1E]=1`, `[1B]=0` (confirmed against
FU-239 §3; `0x17`/`0x02` are the other class-1 rows).

### 2.2 The body `0x6E244` (fresh, FU240 §2 cross-cited)

`0x6E244..0x6E32A` (231 B). Entry `EAX` = a record (whose `[EAX]` is the team),
`EDX` = the target triple `&rec[0x4D]`. Gate `byte[rec+0x826] == [0x157AFF]`
(`[0x157AAC]>>24`); reads `word[0x10F368]`/`word[0x10F364]`, the active byte
`[rec+0x8D]`, `[0x157AC2]`; on the active arm draws the PRNG `0x92AC8` **four** times
per record (two magnitudes + two signs; FU240 errata: not "twice"); writes only
`target[0]` (`rec+0x4D`) and `target[2]` (`rec+0x55`). It touches no end word, phase,
score or shootout cell (FU237 §4).

### 2.3 The FU-239 collapsed arm + the missing `0x8BD95` guard

Fresh `disassemble_bytes 0x8BD40` (180 B) — the native end machine `FUN_0008BAF0`:

```
0x8BD45 MOV EAX,[0x157A4A] ; SAR EAX,0x18 ; 0x8BD4D CMP EAX,0x14 ; 0x8BD50 JNZ 0x8BD95
; ---- phase == 0x14 (0x8BD52): timer>0x4B0 OR (0x45001() && timer>0x258) -> word=2 ----
; ---- phase == 0x13 (0x8BD95) ----
0x8BD95 MOV AX,[0x00157ac5]                 ; 66a1c57a1500   score A
0x8BD9B CMP AX, word[0x00157ac7]            ; 663b05c77a1500 score B
0x8BDA2 JNZ 0x0008bde7                      ; 7543           DIFFER -> HOLD (no write)
0x8BDA4..0x8BDC9  timer>0x4B0 OR (0x45001() && timer>0x12C) gate (0x8BDC4 CMP EAX,0x12c)
0x8BDD4 MOV word[0x158822],2                ; 66c705228815000200  ET OVER (2)
0x8BDDD MOV dword[0x10F360],1               ; the one-shot latch
```

So the native **phase-`0x13` arm alone** requires `[0x157AC5]==[0x157AC7]`; the
phase-`0x14` arm has **no** score test (FU237 §1, FU239 §5). The engine
`fifa96_match_run_end_word` (`fifa96_match_run.c:4064..4081`) collapses both:

```c
if ((mr->state.phase == 0x13u || mr->state.phase == 0x14u) &&
    mr->act.timer > 0x4B0) return 2u;
```

— no score-equality test, and the per-phase `0x258`/`0x12C` second gates + the
`0x45001()` probe are dropped. The source comment names this **FU239-L5** and the
reason: the forced-ET M2 terminal is phase `0x13` with score **1-0** (differing);
the native `0x13` arm would **hold**, so the engine's ET-over is reached only
through the **collapsed** arm ("Reading-B model reach"). Adding the guard would hold
the tape's terminal. **Ruling: the collapse is deliberate and load-bearing; keep it.**

### 2.4 The phase-`0x13`/`0x14` NUMBER writers (fresh `get_xrefs_to`)

The only nonzero writer of `[0x157A4D]` is `FUN_000740A0` (`0x740AC`, fresh
`get_xrefs_to 0x157A4D` = 6: WRITE `0x73e4e`,`0x740ac`,`0x73d2c`; READ `0x740a0`,
`0x8d178`, `0x8316e`). Fresh `disassemble_bytes 0x8B9CC` / `0x8BA07..0x8BAA6`:

* `EBX = 0x13` (default, `0x8B9DE`); the score/d8-d9 `EDX`; the probe arms
  `0x8BA40`/`0x8BA52`/`0x8BA67` set **`EBX = 0x14`**; the write is the computed
  `MOVSX EAX,BX` → `CALL 0x740A0` at `0x8BAC1` (`0x8BABE SAR EDX,0x18`).
* `FUN_0008BAF0` also writes phase `0x13` from its phase-`0xC` branch:
  `0x8BCC8 MOV EAX,0x13` → `0x8BCCF CALL 0x740A0`.

**Errata to FU240-L1:** the native phase-`0x14` write site **is** statically located
(the three `MOV EBX,0x14` sites feeding the computed `0x8BAC1` call); FU240-L1's
"not statically located / no `MOV EAX,0x14 ; CALL 0x740A0`" is true only of the
literal-`EAX` form. The engine mirrors this via `fifa96_action_phase_period_end`
(`fifa96_action_handlers.c:1163`: default `phase=0x13`; `EBX`-analogue `probe[2]`/
`probe[3]`/`probe[edx]` select `0x14`), but `fifa96_match_run_phase_drive`
(`match_run.c:2652`/`:2697`) passes `probe = {0,0,0,0}` — so the engine **only ever
writes `0x13`**, never `0x14` (the probe producers are the OL-74/OL-75 legs,
FU213-L1/L7).

---

## 3. Step 2 — `1A`/`1B`/`1C`: the bodies + the act-tail producers

Fresh xrefs: `0x89620` = **1 DATA** (`0x1107FC`); `0x890EC` = **1 DATA**
(`0x110800`); `0x89110` = **1 DATA** (`0x110804`). Each body's only reference is its
phase-table slot; the phase slot equals the act slot (`0x110794[0x16+act] =
0x1107EC[act]`), so the body is reachable **only** through the phase row or the act
seam (`fifa96_match_act_dispatch` → `fifa96_action_phase_act(act) = 0x16+act`).

Fresh `disassemble_bytes`:

* **`0x89620` (row `1A`, act 4).** Head `word[0x158818] += word[0x157A64]`
  (`0x89629`); stage byte `[0x158829]`; stage 0 begins with
  `CMP byte[0x157B8E],0 ; SETZ AL` (`0x89655`) — the side select of the
  formation-order/line-up walk (`FU193 §3`, `FU244`).
* **`0x890EC` (row `1B`, act 5).** 13 instructions —
  `word[0x158818]=0 ; byte[0x158828]=0 ; byte[0x158829]=0 ; dword[0x158808]=0 ;
  RET` — a **pure reset** (FU195 §5).
* **`0x89110` (row `1C`, act 6).** Head `word[0x158818] += word[0x157A64]`; stage
  `[0x158829]` walk (`<1` → `0x89142`, `==1` → `0x89190`, `==2` → `0x891C0`);
  `0x89142 TEST AL,AL ; JNZ 0x8920F ; 0x8914A MOV EAX,0x15` — the act-6
  pre-restart timeline / offside sequence (FU193 §4).

### 3.1 The act-seam producer census (fresh `get_xrefs_to 0x888FC` = 11)

All 11 `FUN_000888FC` sites, EAX (act) freshly disassembled:

| site | function | act | note |
|---|---|---|---|
| `0x8a6fe` | `FUN_0008a43c` | 3 | |
| `0x8a789` | `FUN_0008a43c` | **6** | phase-2-gated pre-restart; the `0x89110` row |
| `0x8ab57` | `FUN_0008a938` | **4** | `0x8AB4B MOV EAX,0x4`; sit-2..4 formation arm |
| `0x8ab8e` | `FUN_0008a938` | 0xA | sit-0 |
| `0x8abcf` | `FUN_0008a938` | 1 | sit-1 |
| `0x8aeba` | `FUN_0008a938` | 7 | sit-8 |
| `0x8aed2` | `FUN_0008a938` | 2 | |
| `0x8aeea` | `FUN_0008a938` | 2 | `0x8AEE3 MOV EAX,0x2` |
| `0x8af1a` | `FUN_0008a938` | 8/9 | shared call: `0x8AAA3 JMP` sets 8, fall-through `0x8AF13 MOV EAX,0x9` |
| `0x8bae7` | `FUN_0008b9cc` | 0xB/0xC | period-end |
| `0x88ee8` | the phase-0x17 body (`0x88DC8`) | **4** | `0x88EE1 MOV EAX,0x4`; phase-0x17 stage-1 hand-off |

**No site passes act 5** → `0x1B` has no native producer either (FU195-L1 confirmed;
no `MOV EAX,0x1B`/`[0x157A4D]` writer). Act 4 exists (two sites) and act 6 exists
(one site) — so `0x1A`/`0x1C` have **statically-present** native producers.

### 3.2 The engine's act-4/act-6/act-5 dispatch (source, fresh read)

* **act 4** — `fifa96_match_run.c:3183` (sit-2..4 table-2 route) and `:6143`
  (phase-0x17 stage-1 hand-off). Both gated on `mr->formation_order_ok` /
  `formation_mismatch`; FU-201 §6 (R49) proves this gate is a **permutation
  residual** that is 0 even under a faithful fill (the reset copies desired→current),
  so the arm is **provably dormant**. `0x1A` producer dead.
* **act 6** — the engine has **no** `fifa96_match_act_dispatch(mr,6,…)`; the native
  act-6 route is realized by `fifa96_match_run_referee_step` →
  `fifa96_ref_offside_sequence_step` (`match_run.c:6676`), i.e. the offside machine
  **already drives** the `0x89110` sequence. Registering the `0x1C` row would
  double-run it (Ruling R8, FU193-L4).
* **act 5** — no engine dispatch; `fifa96_match_act_dispatch` is never called with 5.

---

## 4. Step 3 — the writers, per phase

| phase | native NUMBER writer (fresh) | live engine writer? |
|---|---|---|
| `13` | `FUN_0008B9CC` (`EBX=0x13`→`0x8BAC1`) + `FUN_0008BAF0` (`0x8BCCF`) | **yes** — `match_run_write_phase(0x13)`, ET chooser (golden frames ~6500+) |
| `14` | `FUN_0008B9CC` (`EBX=0x14` at `0x8BA40`/`0x8BA52`/`0x8BA67`→`0x8BAC1`) | **no** — engine probe cells `{0,0,0,0}`; never selects `0x14` |
| `1A` | act 4 (`0x88EE8`, `0x8AB57`) | **no** — formation residual dormant (FU-201/R49) |
| `1B` | *(none)* | **no** — no invoker, no `0x740A0` writer |
| `1C` | act 6 (`0x8A789`, `FUN_0008a43c`) | **no** — offside machine is the driver; row would double-run |

Fresh `get_xrefs_to 0x158808` (act slot) = **22** (6 reads incl. `0x888fc`
`0x88937`/the `0x8B669`-tail reads; 16 writes incl. the phase bodies' slot clears
`0x890d3`/`0x89106`/`0x89609`). The act slot is written by the `FUN_000888FC` seam
and cleared by the act-terminal bodies — it carries no phase NUMBER; the phase
number is `[0x157A4D]` only.

---

## 5. Step 4 — verdict, port shapes, census ruling

### 5.1 Port shapes

* **`13`/`14`** — port `0x6E244` as `fifa96_match_phase_13` over the walk cursor
  (`fifa96_match_dispatch_phase_record`): side gate `byte[0x157AFF]`; the
  `[0x157AC2] < 4` branch; `word[0x10F364/368]` reads; `fifa96_rng_step` **2 per
  coordinate** (4/record on the active arm); write `target_x`/`target_z`. **Do NOT
  fold the scatter into the per-frame `fifa96_match_phase_machine_step`** (native
  cadence = once per phase *write*, FU240 §5.3); add it to a once-per-write arm at
  the `match_run_write_phase` `fn != NULL` gate. **Blocked** on the M2 tape-inertness
  pin (the ET-entry draws share `mr->rng`). Currently a **CEILING**.
* **`1A`** — the body `fifa96_match_action_phase_1A` exists; wiring needs the act-4
  producer un-gated, which requires a runtime non-permutation with **no writer**
  (FU201-L1) → **stays a leg** (do not fabricate a residual).
* **`1B`** — a trivial reset body; **no producer** → stays NULL (a wired count would
  be un-dispatched/fake).
* **`1C`** — the body `fifa96_match_action_phase_1C` exists; wiring would double-run
  the ported offside machine → **stays a leg**.

### 5.2 Census ruling (AGENTS.md: install AND dispatch, else uncounted)

| phase | ruling |
|---|---|
| `13` | **NULL / uncounted (+0).** Number writable live, but the body is unported and wiring is not proven tape-inert (FU192-L2 / FU240-L2). |
| `14` | **NULL / uncounted (+0).** Number not even written live (probe cells unported, OL-74/OL-75). |
| `1A` | **NULL / uncounted (+0).** Producer present but dormant (FU-201/R49). |
| `1B` | **NULL / uncounted (+0).** No producer (dead). |
| `1C` | **NULL / uncounted (+0).** Producer duplicates the offside machine (R8). |

**Net: `58/80 = 72%` unchanged; M2 (`m2-frames.txt`) and M1 (`m1-frames.txt`)
byte-identical; no re-pin.** This slice un-gates nothing.

---

## 6. Residual legs (numbered; each with its native owner)

1. **FU262-L1 — phase `0x13`/`0x14` body `0x6E244` unported + the RNG tape-inertness
   gate.** The live `match_run_write_phase(0x13)` (ET chooser) would dispatch the row
   the moment it is registered; the four `0x92AC8` draws/record share `mr->rng` with
   the ~20 ET-over frames. Owner: FU-192 (FU192-L2) / FU-240 (FU240-L2).
2. **FU262-L2 — the phase-`0x14` NUMBER write is not live in the engine.** The
   chooser's `probe[2]`/`probe[3]`/`probe[edx]` cells are hardwired `{0,0,0,0}`
   (`match_run.c:2605`), so `fifa96_action_phase_period_end` always yields `0x13`.
   Producer: OL-74/OL-75 (FU213-L1/L7). **Errata: FU240-L1 corrected** — the native
   `0x14` write site (`0x8BA40`/`0x8BA52`/`0x8BA67` → `0x8BAC1`) **is** statically
   located. Owner: FU-240 / FU-213.
3. **FU262-L3 — phase `0x1A` / act-4 producers present but dormant.** `0x88EE8`
   (phase-`0x17` stage-1 hand-off) and `0x8AB57` (sit-2..4 arm); gate
   `formation_order_ok` / `formation_mismatch` is a permutation residual with no
   writer. Owner: FU-201 (FU201-L1) / FU-193 (FU193-L3) / FU-244.
4. **FU262-L4 — phase `0x1B` / act 5 has no producer at all.** Fresh 11-xref
   `0x888FC` census: acts `{1,2,3,4,6,7,8/9,0xA,0xB/0xC}`, **no 5**; no `0x740A0`
   phase-`0x1B` writer. Dead in the native too. Owner: FU-195 (FU195-L1).
5. **FU262-L5 — phase `0x1C` / act-6 producer unported as the row.** Native act-6 at
   `0x8A789` (`FUN_0008a43c`, phase-2-gated). The engine drives the ported
   `fifa96_ref_offside_sequence_step` directly; registering the row double-runs it.
   Owner: FU-193 (FU193-L4) / R8.
6. **FU262-L6 — the FU-239 collapsed arm (the missing `0x8BD95` guard).** The engine
   `fifa96_match_run_end_word` collapses `0x13`/`0x14` and omits the native
   phase-`0x13` score-equality guard `0x8BD95` (`[0x157AC5]==[0x157AC7]`) plus the
   per-phase `0x258`/`0x12C` second gates. Deliberate: the tape's 1-0 forced-ET
   terminal is the collapsed reach (adding the guard would hold it). Owner: FU-239
   (FU239-L5) / FU-237 (FU237-L1: the `0x13`-vs-`0x14` order is unpinned).
7. **FU262-L7 — the clock-class table `0x1106AD`.** Fresh: `0x13`/`0x14`/`0x1B` are
   class 0 (clock stops); `0x1A`/`0x1C` class 1. Reconciled with FU-239 §3; the
   class-1 set is `{0x02,0x17,0x1A,0x1C,0x1D,0x1E}`. No action.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`, `watcom:LE:32:default`,
  2706 functions, 15239 symbols, image base 0, min `0x10000`, max `.image::00174a2a`,
  `.image` overlay.
* `read_memory` — `0x110794` (128 B, the phase table), `0x1106AD` (24 B) +
  `0x1106C5` (16 B, the clock-class table).
* `disassemble_bytes` — `0x8BD40` (180 B, the `0x13`/`0x14` end arm + the `0x8BD95`
  guard), `0x8BC00` (230 B), `0x8B9CC` (60 B), `0x8BA07` (160 B), `0x8BAA0` (48 B),
  `0x8BACF` (40 B), `0x8BAF0` (130 B), `0x6E244` region (via FU240/xrefs), `0x89620`
  (80 B), `0x890EC` (40 B), `0x89110` (60 B), `0x8A43C` (120 B),
  `0x8A480`/`0x8A740` (80 B), and the 11 `0x888FC` sites (`0x8A6F0`/`0x8A780`/
  `0x8AB30`/`0x8AB50`/`0x8AB88`/`0x8ABCF`/`0x8AEB4`/`0x8AEC8`/`0x8AEDC`/`0x8AF12`/
  `0x88EE0`).
* `get_xrefs_to` — `0x6E244` (**2**), `0x89620` (**1**), `0x890EC` (**1**),
  `0x89110` (**1**), `0x740A0` (**27**), `0x888FC` (**11**), `0x157A4D` (**6**),
  `0x158808` (**22**), `0x157AC5` (**95**), `0x8A43C` (**2**).

Engine/loader source read first-hand: `fifa96_match_handlers.c` (the phase table
`0x13`/`0x14`/`0x1A`/`0x1B`/`0x1C`), `fifa96_match_run.c` (`match_run_write_phase`
`:2514`, `fifa96_match_run_phase_drive` `:2602`, `fifa96_match_run_end_word`
`:4064`, the act-4/act-6 producers `:3183`/`:6143`/`:6676`,
`fifa96_match_action_phase_1A` `:6178`, `fifa96_match_action_phase_1C` `:6238`),
`fifa96_match_act.c`, `fifa96_match_phase_machine.c`,
`src/fifa96_loader/fifa96_action_handlers.c` (`fifa96_action_phase_period_end`
`:1163`). No writes beyond this draft + report.
