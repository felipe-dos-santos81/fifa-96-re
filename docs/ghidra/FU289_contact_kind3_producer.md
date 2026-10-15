# FU289 — the live `contact_kind == 3` producer (taker rows `0x12`/`0x13`)

**Phase/Provenance:** M2 phase-26c recon-ahead (follow-up), Task 9 (bounded, read-only derivation); draft awaiting controller promotion.

Read-only Ghidra session, authoritative program exactly **`/FIFA96.EXE`**
(case-sensitive; `get_current_program_info` this session: `watcom:LE:32:default`,
image base 0, 2706 functions, 15239 symbols, `.image` overlay present). Every
native address/byte/ref-count below is a **fresh** `get_xrefs_to` /
`disassemble_function` / `disassemble_bytes` / `read_memory` of **this** session
unless marked *carried* (then the frozen `FU*` slice is named); engine/loader
source facts cite the built tree by line, read-only. LE-image globals are the
flat addresses Ghidra renders. Alias traps applied (`dword[x]>>16` = word at
`x+2`, `>>24` = byte at `x+3`); CALL bytes verified; no store mistaken for a
read. **No writes** — no rename / comment / label / function / script / project
save; `/FIFA96.EXE` untouched; no git command.

**Question (brief).** The taker rows `0x12`/`0x13` are wired-but-unraised
(LIVE-LEG, FU280). The attributed blocker is a native contact-kind cell,
variously cited as `[0x158882]` (≡ `contact_kind == 3`) and as a
`foul_kind`/`[0x15888D]` vs `contact_kind`/`[0x15888C]` divergence. Derive the
cell, the producer of the value 3, and the live-vs-leg ruling.

**Result in one line.** The contact-kind cell is **`[0x15888C]`** (byte
`contact_kind`) — **not** `[0x158882]` (a distinct ball-record terminal flag, the
act-2 gate's first OR-term, fresh 1-read/0-write). Its **sole** writer of the
value 3 is `0x8A772` in `FUN_0008A43C` — the **kind-3 (offside) arm**, reached
only from the offside check `FUN_00079D5C 0x79F2B`. The taker rows `0x12`/`0x13`
do not read the cell directly; the **restart lock** reads it at `0x894AD` to
select phase 7 (row `0x12`) vs phase 6 (row `0x13`). The engine's restart lock
reads the **wrong cell** (`foul_kind == 3`, `match_run.c:6002/6009`) where native
reads `contact_kind == 3`. **No live engine path raises `contact_kind == 3`**
(the offside event `fifa96_ref_offside_event` is behind a **callerless** entry) →
the live producer is a **LEG**; the cell swap is a **bounded GO** red-first fix.

---

## Step 1 — the cell census (fresh `get_xrefs_to`, all three cited cells)

### 1.1 `[0x15888C]` = `contact_kind` (the cell the restart lock reads)

Fresh `get_xrefs_to 0x15888C` = **12** (5 WRITE / 7 READ). Mapped by the engine
header `include/fifa96_loader/fifa96_referee.h:85` (`contact_kind [0x15888C]`;
field `struct fifa96_referee_state.contact_kind`, `:107`).

| writer | site | stores | value reachable |
|---|---|---|---|
| `0x8A400` | `FUN_0008A3FC` (registrar) | `MOV [0x0015888c],AL` — bytes `a28c881500` (fresh `disassemble_bytes 0x8a400`) | AL = entry kind; callers `0x80EE7` (kind 0/1) and `0x81BCF` (kind 2) → **never 3** |
| `0x8A49E` | `FUN_0008A43C` normal arm | `MOV byte[0x0015888c],DL` — bytes `88158c881500` (fresh `disassemble_bytes 0x8a49e`) | DL = kind; arm entered only for kind 1 or 2 (`0x8a46d`/`0x8a472`) → **never 3** |
| `0x8A772` | `FUN_0008A43C` **kind-3 (offside) arm** | `MOV [0x0015888c],AL` — bytes `a28c881500` (fresh `disassemble_bytes 0x8a772`) | AL = kind, arm entered only for kind 3 (`0x8a735 CMP EBX,0x3`) → **the sole "3" writer** |
| `0x93F3C` | set-piece body | `MOV byte[0x0015888c],BH` — bytes `883d8c881500`, `BH=1` set at `0x93F33` (`b701`), fresh `disassemble_bytes 0x93f10` | **1** |
| `0x94700` | set-piece body | `MOV byte[0x0015888c],DL` — bytes `88158c881500`, `DL=1` set at `0x946F8` (`b201`), fresh `disassemble_bytes 0x946e0` | **1** |

Fresh readers: `0x4BED6`, `0x82317`, `0x892D9`, `0x893BE`, `0x89429`, `0x894AD`,
`0x894DD`. The load-bearing **kind==3 test** is in the phase-0x18 restart lock,
fresh `disassemble_bytes 0x89480`:

```
0x894A2  CMP byte[0x00158882],0x0 ; JNZ 0x894c4   ; <<< the [0x158882] OR-term (ball flag)
0x894AB  XOR EAX,EAX
0x894AD  MOV AL,[0x0015888c]   ; a08c881500        ; <<< contact_kind
0x894B2  CMP EAX,0x3 ; JZ 0x894c4                 ; contact_kind == 3
0x894B7  CMP byte[0x0014c32a],0x0 ; JZ 0x89615     ; else the session gate
```

and the phase chooser, fresh `disassemble_bytes 0x89480` + `0x894f2`:

```
0x894DB  MOV AL,[0x0015888c] ; CMP EAX,0x3
0x894E7  MOV word[ESP],0x7 ; JMP 0x89556           ; contact_kind == 3  -> phase 7
0x894F2  MOV word[ESP],0x7                          ; else: default phase 7
0x8950F  CMP EDX,0x420 ; JGE 0x89556                ; |incident_x| band gate
0x89550  MOV word[ESP],0x6                          ; in-band -> phase 6
```

So the gate is `[0x158882] != 0 || contact_kind == 3 || [0x14C32A] != 0`
(the engine's own comment, `match_run.c:5960`, names this structure).

### 1.2 `[0x158882]` is NOT the contact kind (misattribution resolved)

Fresh `get_xrefs_to 0x158882` = **1**: a single READ at `0x894A2`
(`CMP byte[0x158882],0`, §1.1) and **zero writers**. The engine header itself
calls it the **act-2 gate** (`fifa96_referee.h:36`), and `match_run.c:5961-5965`
attributes it to the ball block `+0x96` (base `0x1587EC`, so `0x1587EC+0x96 =
0x158882`) written **indirectly** by `FUN_0006E724` as `row_flags & 1` — hence no
direct xref. `fifa96_match_entities.h:285` records the adjacent `head_gate`
`0x158881`. **Verdict:** `[0x158882]` is a distinct ball-record terminal flag and
an OR-term of the same gate — **not** `contact_kind == 3`. The FU280/BACKGROUND
attribution "`[0x158882]` ≡ contact_kind==3" is a **misattribution**.

### 1.3 `[0x15888D]` = `foul_kind` (the severity cell — a different cell)

Fresh `get_xrefs_to 0x15888D` = **12** (6 WRITE / 6 READ). Mapped
`fifa96_referee.h:86` (`foul_kind [0x15888D]`; field `:108`).

Writers: `0x8A48F` (`=0`, `FUN_0008A43C`), `0x8A603` (`=2`, bytes `c6058d88150002`), `0x8A60C` (`=1`),
**`0x8A639` (`MOV byte[0x0015888d],0x3` — bytes `c6058d88150003`, the sole "3")**, `0x93F2A` (`=0`, bytes `881d8d881500`),
`0x946FA` (`=0`, bytes `88258d881500`). Readers: `0x4BEAC`, `0x8A640`, `0x8A6BC`, `0x89318`, `0x8A193`,
`0x8A2E8`. So `foul_kind` and `contact_kind` are **adjacent but distinct** bytes
with **distinct** "3" producers (`0x8A639` vs `0x8A772`).

---

## Step 2 — the producer of the value 3

**Sole producer**: `0x8A772` in `FUN_0008A43C`, the **kind-3 arm**. Fresh
`get_xrefs_to 0x8A43C` = **2** — `0x79F2B` (`FUN_00079D5C`) and `0x81EBF`
(the row-0x0C re-call block, kind 1). Only `0x79F2B` enters with `AX = 3`
(`MOV EAX,0x3` at `0x79F1F`, carried FU276 §2.1 fresh). The kind-3 arm:

```
0x8A735  CMP EBX,0x3 ; JNZ 0x8a78e          ; only kind 3
0x8A73A  CMP dword[0x0014c2f2],0x0 ; JZ 0x8a78e   ; the offside-enable settings gate
0x8A743  MOV EBX,0x4 ; MOV EAX,0x15 ... CALL 0x8F188  ; the speech-0x15 event
0x8A759  MOV AL,[ESP]                        ; AL = the entry kind (=3)
0x8A772  MOV [0x0015888c],AL                 ; <<< contact_kind := 3  (the sole "3")
0x8A789  MOV EAX,0x6 ; CALL 0x888FC          ; ACT6 (the offside sequence)
```

**Route to it (fresh caller chain)**:

```
FUN_0007A084 0x7A448  (the offside receive arm)
  ← fresh get_xrefs_to 0x79D5C = 1 (only 0x7A448)
FUN_00079D5C 0x79F2B  (the kind-3 decision call, AX=3)
  ← reached from the kick/ball tail FUN_0007A490 + the animation-event machine
    FUN_0006E8E8 (0x6F56F)     [carried FU276 §2.2]
```

So `contact_kind == 3` is an **offside-event** value, produced downstream of the
receive arm — **not** a foul value.

**Route from the cell to the taker rows `0x12`/`0x13`.** The rows do **not** read
the cell directly (fresh `get_xrefs_to 0x15888C` has no `0x83xxx`/`0x84xxx`
reader). The cell drives their **phase gate**: the restart lock (Step 1.1) picks
phase 7 when `contact_kind == 3`, else the band rules; the phase arm then installs:

| row | native entry | gate | engine install | engine row gate line |
|---|---|---|---|---|
| `0x12` free-kick taker | `0x83D68..0x84597` | `{phase 2, phase 7}` | `match_run.c:2834` (phase 7) | `fifa96_match_handlers.c:1909` |
| `0x13` penalty taker | `0x84B00..0x84EEB` | `{phase 2, phase 6}` | `match_run.c:2847` (phase 6) | `fifa96_match_handlers.c:2050` |

(entries/gates carried FU-149 / FU-81, re-read this session in
`fifa96_match_handlers.c:1896/2037/5495/5497`). The phase 6/7 chooser is the
engine restart lock (`match_run.c:6002-6020`).

---

## Step 3 — the live-vs-leg ruling and the engine reconciliation

**No live engine path raises `contact_kind == 3` → LEG.** The only producer is the
offside event `0x8A772` = engine `fifa96_ref_offside_event`
(`src/fifa96_loader/fifa96_referee.c:205`: `state->contact_kind = 3u;`). Its
engine caller, `fifa96_match_run_offside_reception` (`fifa96_match_run.c:6572`,
which calls `fifa96_ref_offside_event` at `:6664`), has **no caller** (FU276-L2/L3,
fresh `src/` grep: definition + tests only). The engine's *live* receive arm drops
the `0x7A448` offside call (declared leg, `match_run.c:3768`). Hence the value 3
is unreachable live.

**Engine reconciliation (the divergence leg).** The restart lock reads
`mr->referee.foul_kind == 3u` (`fifa96_match_run.c:6002`, `:6009`) — the **wrong
cell**. Native `0x894AD` reads `contact_kind` = `[0x15888C]` (severity
`foul_kind` = `[0x15888D]` is a different byte, §1.3). The engine should read
`mr->referee.contact_kind`. Additionally, the engine's `foul_kind == 3` is itself
**unreachable**: `fifa96_ref_foul_decide` sets `foul_kind = 3` only when the
decision's `kind == 2` (`referee.c:91`), but the engine's sole caller path forces
`mr->referee.contact_kind = 1u` (`match_run.c:6542`) immediately before the
decision (`fifa96_ref_foul_decide` reads `kind = state->contact_kind`,
`referee.c:61`) → `kind == 1` → the `kind == 2` precondition never holds. So the
engine *currently* never takes the "phase-7 no-band" branch, and the wrong-cell
read is **latent** (no live behavior difference today) — but it is still a
wrong-byte port. Under the engine's row-0x0C path, `foul_kind ∈ {0,1,2}` ⇒
`foul_kind != 3` ⇒ the band logic runs, which is what native `contact_kind == 1`
⇒ `!= 3` also does; the two only diverge once a live offside (or any producer of
`contact_kind == 3`) exists.

**Verdict.** The **cell swap is a bounded GO** (a faithful, ~2-line correctness
fix: read `contact_kind`, not `foul_kind`). The **live producer is a LEG** — it
requires the offside chain (the `0x7A448` call ported into the live receive arm +
a raised `field_4c2f2`), which is bounded elsewhere (FU276-L2/L3). Rows
`0x12`/`0x13` therefore stay **LIVE-LEG** until the offside chain lands.

---

## Step 4 — the port shape and the RED-first test seam

**Producer (the faithful value-3 source).** The only faithful producer is the
offside event. Porting it live = FU276-L2 (call `fifa96_ref_offside_event` — or
the `0x7A448 → fifa96_ref_offside_check` chain — from inside the live
`fifa96_match_run_ball_receive`) behind a raised `field_4c2f2` (`match_run.c:6598`;
FU276-L3). The producer body already exists (`fifa96_ref_offside_event`,
`referee.c:192`) and already writes `contact_kind = 3` (`:205`); it is only the
**call site** that is missing. **No fake producer** (live-chain rule).

**Where the taker rows read it (the wrong cell).** Swap the restart-lock read:

```c
/* fifa96_match_run.c:6002 and :6009 */
-  if (!(mr->referee.foul_kind == 3u || mr->session_gate_14c32a != 0u)) {
+  if (!(mr->referee.contact_kind == 3u || mr->session_gate_14c32a != 0u)) {
...
-    if (mr->referee.foul_kind != 3u) {
+    if (mr->referee.contact_kind != 3u) {
```

The `match_run.c:5960` comment ("`kind == 3`") stays correct and should cite the
native `0x894AD`/`0x894DB` and `[0x15888C]`. The `foul_kind` severity draw
(`referee.c:88-93`) is untouched (it still feeds the speech select and the foul
log, `referee.c:257/109/115`).

**RED-first test seam.** A headless granted-frame test that stages
`mr->referee.contact_kind = 3` and drives the act-2 restart lock —
`fifa96_match_action_phase_18` (`match_run.c:6055`) or the `fifa96_match_run_phase_arm`
path — asserting the **phase-7 → row `0x12`** install (vs `foul_kind = 3` alone,
which must **not** force phase 7). The same test with `contact_kind == 1` in the
fouler-side band asserts **phase 6 → row `0x13`**. It is **RED** against the
current `foul_kind` read (the `contact_kind == 3` staging is ignored) and
**GREEN** after the swap. No tape re-pin is implied (the tape performs no
offside; FU276).

**GO/NO-GO.** **Bounded slice = the cell swap** (`match_run.c:6002/6009`,
contact_kind-red-first test) — GO. **The live raise = NO-GO / LEG** (blocked on
FU276-L2/L3); rows `0x12`/`0x13` remain LIVE-LEG.

---

## Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | fresh evidence |
|---|---|
| program identity (`/FIFA96.EXE`, `watcom:LE:32:default`, 2706 fns, `.image`) | `get_current_program_info` |
| `[0x15888C]` = 12 refs; writers `0x8a400`/`0x8a49e`/`0x8a772`/`0x93f3c`/`0x94700` | `get_xrefs_to 0x15888c` |
| `[0x15888D]` = 12 refs; sole "3" writer `0x8a639` | `get_xrefs_to 0x15888d`; `disassemble_function 0x8a43c` |
| `[0x158882]` = 1 READ (`0x894a2`), **0 writers** | `get_xrefs_to 0x158882` |
| the kind==3 test reads `[0x15888c]`, not `[0x158882]` | `disassemble_bytes 0x89480` (`0x894a2` vs `0x894ad`) |
| phase chooser: `contact_kind==3 → phase 7`, else band `→ 6` | `disassemble_bytes 0x89480`/`0x894f2` (`0x894db`/`0x894e7`/`0x89550`) |
| registrar `FUN_0008a3fc` stores `[0x15888c] := AL`; callers `0x80ee7`/`0x81bcf` | `disassemble_function 0x8a3fc`; `get_xrefs_to 0x8a3fc` |
| `0x80ee7` passes kind 0/1 (`SETNZ`/`CWDE`) | `disassemble_bytes 0x80ed4` |
| `FUN_0008a43c` kind-3 arm writes `[0x15888c] := 3` at `0x8a772` | `disassemble_function 0x8a43c` |
| `get_xrefs_to 0x8a43c` = 2 (`0x79f2b`, `0x81ebf`) | `get_xrefs_to 0x8a43c` |
| `get_xrefs_to 0x79d5c` = 1 (only `0x7a448`) | `get_xrefs_to 0x79d5c` |
| set-piece bodies write `[0x15888c] = 1`, `[0x15888d] = 0` | `disassemble_bytes 0x93f10`/`0x946e0` |
| engine map: `contact_kind [0x15888C]`, `foul_kind [0x15888D]`, act-2 gate `[0x158882]` | `include/fifa96_loader/fifa96_referee.h:36/85/86/107/108` (read-only) |
| engine `fifa96_ref_offside_event` writes `contact_kind = 3` | `src/fifa96_loader/fifa96_referee.c:205` (read-only) |
| engine restart lock reads `foul_kind == 3` (wrong cell) | `src/fifa96_engine/fifa96_match_run.c:6002/6009` (read-only) |
| engine path forces `contact_kind = 1` before the decision | `src/fifa96_engine/fifa96_match_run.c:6542` (read-only) |
| row `0x12` entry `0x83D68` gate {2,7}; row `0x13` entry `0x84B00` gate {2,6} | `src/fifa96_engine/fifa96_match_handlers.c:1896/1909/2037/2050/5495/5497` (read-only) |
| offside entry `fifa96_match_run_offside_reception` callerless | `src/` grep (definition `:6572` + tests only); FU276-L2 |

**Carried (prior evidence, not re-derived):** `docs/ghidra/FU280_live_trigger_census_2.md`
(the taker rows `0x12`/`0x13` LIVE-LEG ruling, FU280-L4), `FU276_fouls_offside_contact.md`
(§1 the contact gate / row-0x0C kind-1, §2.1 the `0x79f2b` kind-3 call, §2.2 the
receive-arm chain, FU276-L1/L2/L3), `FU268_fouls_offside_extratime.md`,
`FU150_fouls_referee_offside.md`, `FU-149`/`FU-81` (the taker row bodies), and the
`fifa96_referee.h` per-function contracts.

---

## Numbered open legs (FU289-L*, with native owners)

* **FU289-L1 — no live engine path raises `contact_kind == 3`; the producer is the
  offside event.** The sole writer of 3 into `[0x15888C]` is `0x8A772`
  (`FUN_0008A43C` kind-3 arm) ← `FUN_00079D5C 0x79F2B` ← `FUN_0007A084 0x7A448`;
  the engine's `fifa96_ref_offside_event` (`referee.c:205`) sits behind the
  **callerless** `fifa96_match_run_offside_reception` (`match_run.c:6572`). Native
  owner: the offside receive arm `FUN_0007A084 0x7A448`. Next hop: port the
  `0x7A448` call into the live receive arm + raise `field_4c2f2` (= FU276-L2/L3).
* **FU289-L2 — the engine restart lock reads the wrong cell.** `match_run.c:6002/6009`
  tests `foul_kind == 3` (`[0x15888D]`) where native `0x894AD`/`0x894DB` tests
  `contact_kind == 3` (`[0x15888C]`). Bounded GO fix (contact_kind-red-first test);
  native owner: `0x894AD`/`0x894DB` (the phase-0x18 restart lock). Latent only
  (both paths currently resolve to the band logic because `foul_kind == 3` is
  itself unreachable — `match_run.c:6542` forces kind 1).
* **FU289-L3 — `[0x158882]` is not `contact_kind`.** Fresh `get_xrefs_to` = 1 READ
  / 0 writers; it is the act-2 gate's ball-record terminal-flag OR-term
  (`fifa96_referee.h:36`; `match_run.c:5961`, written indirectly by `FUN_0006E724`).
  Native owner: `FUN_0006E724`. (Erratum to the FU280/BACKGROUND attribution — the
  cell is `[0x15888C]`, not `[0x158882]`.)
