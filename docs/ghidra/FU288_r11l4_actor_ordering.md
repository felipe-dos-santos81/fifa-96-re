# FU288 — R11-L4: the `[0x157A83]` actor-cell ordering (leg T2b-L1) + route/census synthesis

**Phase/Provenance:** M2 phase-26c recon-ahead, Task 8 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (case-sensitive). Confirmed this session
by `get_current_program_info`: `/FIFA96.EXE`, `watcom:LE:32:default`, image base
0, `.image` overlay present, 2706 functions. Every native address/byte below is
a **fresh** `disassemble_bytes` / `disassemble_function` / `decompile_function` /
`read_memory` / `get_xrefs_to` of *this* session unless marked **carried** (the
frozen `docs/ghidra/FU*` slice is named). Engine-source facts cite the built tree
by line (read-only). Alias traps applied (`dword[x]>>0x18` = the byte at `x+3`;
`>>0x10` = the word at `x+2`); CALL rel32 bytes verified against the disassembly
operands. No rename / comment / label / function-create / script / project-save;
`/FIFA96.EXE` untouched; the ISO never touched; no git command.

**Question (brief).** Pin the ordering that lets a controlled-side keeper become
the *actor* (`[0x157A83]`) between the ladder install (row `0x1B` on a keeper
while `[0x157A83]==0`) and the stage-4 sit-5 emit (which requires
`EBP==[0x157A83]`). Enumerate/classify the `[0x157A83]` writers, identify the
keeper-route actor-install, reconcile the engine approximation, and synthesise
the FU273 §3.3 route/census table with a census-impact + M2 re-pin statement.

**Result in one line.** **GO.** The ordering is **pinnable**, not a leg: the
ladder install (`FUN_000782D0`) is gated **`phase==2`**, the keeper-route
actor-write (row-`0x1B` head `0x76DBF`, row-`0x1A` head `0x7665C`) is gated
**`phase==0xD && side==[0x157AAC]>>0x18`** — **disjoint phase values**, so the
install and the actor transition can never share a frame; the head-write and the
stage-4 emit (`0x776D8`/`0x776DE` → `0x77707`) are the **same row-`0x1B`
dispatch** on the later `phase==0xD` frame. Leg **T2b-L1 resolves**. The *port*
of R11-L4 remains multi-task (FU273 §4.3 NO-GO stands for the port, not the
ordering pin).

---

## 1. The `[0x157A83]` writer census (fresh, this session)

Fresh `get_xrefs_to 0x157A83` = **112** refs: **22 WRITE**, 90 READ.
`ghidra_search_instructions(operand_pattern="0x157a83")` returned **0 matches**
(operand-rendering mismatch — see the tooling note in §5); the census below is
driven from the fresh xref list, each WRITE site disassembled fresh.

| # | write addr | enclosing | instruction (fresh bytes) | value | class |
|---|---|---|---|---|---|
| 1 | `0x73E6C` | `FUN_00073E28` | `MOV [0x157A83],ECX` (`890d837a1500`) | ECX=0 (`decompile`: `DAT_00157a83 = 0`) | **clear→0** |
| 2 | `0x8C3BE` | `FUN_0008C33C` | `MOV [0x157A83],EAX` (`a3837a1500`) | EAX=`[ESI+0x7b2]` | **set→rec** |
| 3 | `0x8CB73` | `FUN_0008C974` | `MOV [0x157A83],EDI` (`893d837a1500`) | EDI=0 (`XOR EDI,EDI`) | **clear→0** |
| 4 | `0x7A227` | `FUN_0007A084` | `MOV [0x157A83],EBX` (`891d837a1500`) | EBX=0 (`XOR EBX,EBX`) | **clear→0** |
| 5 | `0x75DCC` | row `0x1E` stage-5 | `MOV [0x157A83],EAX` (`a3837a1500`) | EAX=`[EBP-0xc]` | **set→rec** |
| 6 | `0x76AD8` | row `0x1A` body | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP) | **set→rec** |
| 7 | `0x77568` | row `0x1B` stage-2 | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP); next insn `CALL 0x6E598` | **set→rec** |
| 8 | `0x7F281` | (row body) | `MOV [0x157A83],ESI` (`8935837a1500`) | ESI=0 (`XOR ESI,ESI`) | **clear→0** |
| 9 | `0x7F299` | (row body) | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP) | **set→rec** |
| 10 | `0x83E40` | (row body) | `MOV [0x157A83],EDX` (`8915837a1500`) | EDX (rec ptr) | **set→rec** |
| 11 | `0x84F63` | (row body) | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP) | **set→rec** |
| 12 | `0x85E4B` | (row body) | `MOV [0x157A83],EDX` (`8915837a1500`) | EDX (rec ptr) | **set→rec** |
| 13 | `0x74F22` | row `0x1D` | `MOV [0x157A83],EAX` (`a3837a1500`) | EAX=`[EBP-0x8]` | **set→rec** |
| 14 | `0x755CF` | row `0x1E` | `MOV [0x157A83],EAX` (`a3837a1500`) | EAX=`[EBP-0xc]` | **set→rec** |
| 15 | `0x7665C` | **row `0x1A` head** | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP); gate `phase==0xD && side` | **set→rec** |
| 16 | `0x76DBF` | **row `0x1B` head** | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP); gate `phase==0xD && side` | **set→rec** |
| 17 | `0x7DC18` | (row body) | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP) | **set→rec** |
| 18 | `0x82546` | (row body) | `MOV [0x157A83],ECX` (`890d837a1500`) | ECX=0 (`XOR ECX,ECX`; gate `EBP==EBX`) | **clear→0** |
| 19 | `0x84B31` | (row body) | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP); gate stage<5 | **set→rec** |
| 20 | `0x84F25` | (row body) | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP); gate stage<3 | **set→rec** |
| 21 | `0x85635` | (row body) | `MOV [0x157A83],EBP` (`892d837a1500`) | rec (EBP); gate stage<3 | **set→rec** |
| 22 | `0x74578` | (row body) | `MOV [0x157A83],EDX` (`8915837a1500`) | EDX (rec ptr) | **set→rec** |

**Count: 5 clear→0** (`0x73E6C`, `0x8CB73`, `0x7A227`, `0x7F281`, `0x82546`)
**/ 17 set→rec.** No writer stores a non-record non-zero value (no "other").

The clear at `0x73E6C` is the whole-cell reset `FUN_00073E28` (`decompile`:
`DAT_00157a83 = 0` in a routine that also clears `DAT_00157a5c`, sets
`DAT_00157a73 = &DAT_0015774c`, etc.) — a match/state re-init. The other four
clears (`0x8CB73`, `0x7A227`, `0x7F281`, `0x82546`) are guarded zero-stores
(`XOR`-then-`MOV`, or `CMP/JNZ` zero-write) inside row bodies.

## 2. The ordering finding (the keeper route)

### 2.1 The two disjoint gates (fresh bytes)

**Ladder install — `phase==2`.** Fresh `disassemble_bytes 0x78570` +
`decompile_function 0x782D0` (`FUN_000782D0`):

```
0x7857B  MOV EAX,[0x157A4A] ; 0x78580 SAR EAX,0x18 ; 0x78583 CMP EAX,0x2 ; 0x78586 JNZ 0x7859E
0x78588  CMP dword[EBP+0x20],0x0 ; 0x7858c JNZ ...
0x7858e  CMP byte[0x157C5D],0x0 ; 0x78595 JZ ...
0x78597  MOV EAX,EBP ; 0x78599 CALL 0x77eac
```

`decompile 0x782D0` confirms: `if (((ram0x00157a4a >> 0x18 == 2) &&
(param_1[8] == 0)) && (DAT_00157c5d != '\0')) FUN_00077eac(param_1,uVar5);`.
So the ladder (the row-`0x1A`/`0x1B`/`0x1C`/code-`4` installer, `decompile
0x77EAC` — five arms `FUN_0007d9a4(param_1,0x1a/0x1b/0x1c/4,0,1)`) runs **only
when `[0x157A4A]>>0x18 == 2`**, and installs on `param_1` **while
`[0x157A83]==0`** (the ladder skips `param_1 == DAT_00157a83` and, when an actor
exists, the actor's side — `decompile 0x77EAC`).

**Actor-write — `phase==0xD && side`.** Fresh `disassemble_bytes 0x76D28`
(row-`0x1B` head) and `0x7662C` (row-`0x1A` head) are byte-identical in gate
shape:

```
0x76D33  MOV EAX,[0x157A4A] ; SAR EAX,0x18 ; 0x76D3D CMP EAX,0xd ; 0x76D40 JNZ 0x76dc5
0x76D46  EAX=[EBP] ; AL=[EAX+0x826] ; EDX=[0x157AAC] ; SAR EDX,0x18 ; CMP EDX,EAX ; JNZ 0x76dc5
...
0x76DBF  MOV [0x157A83],EBP
```

and the row-`0x1A` head:

```
0x7663C  CMP EAX,0xd ; 0x7663F JNZ 0x76662
0x76658  CMP EDX,EAX ; 0x7665A JNZ 0x76662
0x7665C  MOV [0x157A83],EBP
```

So **both keeper heads install the actor only on `phase==0xD` and
`side == [0x157AAC]>>0x18` (the controlled side)**.

### 2.2 The disjointness ⇒ the ordering

The ladder install reads `phase==2`; the actor-write reads `phase==0xD`. These
are **disjoint phase values**, so the install and the actor transition **cannot
coincide on one frame** — the ladder's own gate forbids it. The native sequence
is therefore pinned:

1. **Frame N (phase==2):** `FUN_0008D8EC` pre-pass → `FUN_000782D0(rec0)` →
   ladder `FUN_00077EAC` installs row `0x1B` (invoke-now, `FUN_0007d9a4(rec,0x1b,0,1)`)
   on a keeper record-0 **while `[0x157A83]==0`** (no actor). The invoked
   row-`0x1B` head runs, but its `phase==0xD` gate **fails at phase 2** → **no
   actor write yet**.
2. **Frames N+1…:** `FUN_000782D0`'s `(*(code*)param_1[6])(param_1)`
   (`0x785A8 CALL dword[EBP+0x18]`) dispatches row `0x1B` each frame. When the
   phase advances to `0xD` and the carrying keeper's `side == side_controlled`,
   the head sets `[0x157A83] := rec` (`0x76DBF`).
3. **Same frame (phase==0xD):** the same row-`0x1B` invocation continues
   stage 2→3→4; the stage-4 emit reads the actor it just wrote —
   `0x776D8 MOV EDI,[0x157A83]` / `0x776DE CMP EBP,EDI` / `0x77707 CALL 0x8A938`
   (fresh `disassemble_bytes 0x776C8`) — so the emit **sees the head's write on
   the same dispatch**.

**The pairing is: head-write ⇒ emit within one row-`0x1B` dispatch, on a
`phase==0xD` frame that is strictly later than the `phase==2` ladder install
frame.** That is the ordering leg T2b-L1 asked for, and it is **pinned by the
two gate immediates**.

### 2.3 The keeper-route actor-install identity

The **actor-install for the keeper route** is the **row-`0x1B` head write at
`0x76DBF`** (with its **row-`0x1A` sibling at `0x7665C`**), both gated
`phase==0xD && side==controlled`. The ladder install is **not** an actor-install
— it installs the *row*, not the actor, and only while `[0x157A83]==0`. The
disjoint-phase observation (`phase==2` ladder vs `phase==0xD` head write, §2.1)
applies to this **head** write only.

A second row-`0x1B` actor-write exists at `0x77568` (stage-2, before
`CALL 0x6E598`). It is **ungated**, but it is **mutually exclusive** with the
head write within one dispatch: the stage-2 arm entry
`0x76FC7→0x76FD5 CMP EBP,[0x157A83]; JZ 0x77636` (fresh bytes `3b2d837a1500` /
`0f8455060000`) diverts to the stage-4 head `0x77636` whenever `EBP==[0x157A83]`,
so the wide block — and hence `0x77568` — is reached **only when `EBP !=
[0x157A83]`**, i.e. only when the head did **not** set this record as the actor.
Thus **head-pass** (gated `phase==0xD && side`) jumps to `0x77636` and **skips**
`0x77568`; **head-fail** runs the wide block and `0x77568` **self-sets**
`[0x157A83]=rec`. The two writes never both fire on one dispatch.

The self-set then falls through
`0x77568 → 0x77631 → 0x77636 → 0x776C1 → 0x776D8/0x776DE` (where
`EBP==[0x157A83]` is now satisfied) `→ 0x77707` (`e82c320100` → `0x8A938`, the
sit-5 emit), and this emit carries **no phase gate**.

## 3. The engine approximation (reconciled)

`src/fifa96_engine/fifa96_match_handlers.c`:
- **`fifa96_match_action_1B`** (`:3692`, wired) ports the row-`0x1B` head
  (`:3711` `if (mr->state.phase == 0x0Du && team->side == ...side_controlled)`)
  and, inside it, sets `r->controlled = 1` (`:3724`) — the engine's record-level
  bind of the head's `[0x157A83]=rec`.
- `:3733` `actor_is_rec = (r->controlled != 0 || mr->entities.controlled == id)`
  — the engine's stand-in for the native's **live** `[0x157A83]` read. The
  comment (`:3726-3732`) documents the collapse: within one dispatch the pool
  cell lags, so the OR of the head-write and the persisted pool value replaces
  the native's live pointer read.
- Stage 2 (`:3762`) `if (mr->global_157a4c != 1u || actor_is_rec == 0) stage = 4;`
  and stage 4 (`:3781`) `fifa96_keeper_reposition_b_finish(actor_is_rec,
  human_side, …)`; `fifa96_keeper.c:255-260` returns `notify_code=5` when
  `is_controlled && human_side==0`; the tail (`:3787`) emits via
  `fifa96_match_run_situation(mr, notify_code)`.
- Rows `0x1B`/`0x1C`/`0x1D`/`0x1E`/`0x1F` are **wired** (`:5519`/`:5521`/`:5523`/`:5525`/`:5527`);
  row `0x1A` is **NULL/uncounted** (`:5517`). The installers: row-`0x1F`
  no-slot dive (`:3611`), phase-6 arm (`match_run.c:2838-2851`), phase-8/9 arm
  (`match_run.c:2854-2861`).

**Engine = native, in shape:** the engine's head gate is the native's
`phase==0xD && side`; the engine's stage-4 uses `actor_is_rec` exactly where the
native reads `[0x157A83]`.

**Engine ≠ native, two divergences:**
1. **Route (leg T2b-L6).** Native emit is `FUN_0008A938(5, side, BX=1)` (the
   queue/act-8 route); the engine emits via `fifa96_match_run_situation(mr, 5)`
   → `match_run_situation_side(mr, 5, 0u, 0)` (side=0, bx=0; `match_run.c:2949-2951`),
   the direct table-2 route. Owner: FU-269-L5.
2. **Installer phase (new leg FU288-L5).** The native installs row `0x1B` at
   **`phase==2`** via the ladder; the engine's ported installer is the phase-6
   arm → row `0x1F` no-slot dive at **`phase==6`** (`match_run.c:2838-2851`,
   `handlers.c:3611`). Same *shape* (install at a phase < 0xD, actor-set/emit at
   0xD), different phase value. The ladder (native's primary installer) is
   unported (FU-180-L4/L5).

**The engine does not model the cross-frame `2→0xD` split explicitly** — it
relies on the row surviving from its install frame to a phase-`0xD` dispatch,
which the native guarantees by the same gate disjointness pinned in §2.2.

## 4. Route/census synthesis (FU273 §3.3 reproduced)

| arrow (native) | engine site | status | owner |
|---|---|---|---|
| row `0x1B` head (`phase 0xD` + side) → `[0x157A83] = rec` | `handlers.c:3711/3724/3733` (approx `actor_is_rec`) | **ported (approx)** | installer unported: FU-186-L5 / FU-180-L4/L5 |
| row `0x1B` stage 4 (`EBP==[0x157A83]`) → `FUN_0008A938(5, side, BX=1)` | `handlers.c:3782/3787` | **ported, route divergence** | FU-269-L5 (T2b-L6) |
| → situation 5 → phase 9 → install row `0x1E` on the team's rec 0 | `match_run.c:2854-2861` | **ported** | — |
| → row `0x1E` claim stages 0..4 → stage 5 needs `anim_row==0x45` | `fifa96_keeper.c:709-722` + `handlers.c:476-533` | **unported bridge** | FU-163 R11-L1 |
| → release: `[0x157750] := 0x50` | `handlers.c:521-530` | **ported** | — |

### 4.1 Census impact

- Row `0x1A` (`:5517`) is `{0x1A, NULL, …}` → **uncounted**. Its body is ported
  (`fifa96_match_action_1A`) but the slot is NULL, so the census count (36 action
  + phase rows = **59/80**, `docs/ENGINE.md:8`/`:268`; README title
  `73%` = `floor(100×59/80)=73`) **excludes it**.
- **Mechanically**, assigning the already-ported body to the slot would count
  one action row → **59/80 → 60/80 = 75%**. **But** the repo convention (the
  rows-27/29/2C precedent, restated in the row-`0x1A` table comment `:5517`)
  counts a row only when it has a **live installer**. Row `0x1A`'s only installer
  is the **unported** ladder, so it must stay uncounted → **census unchanged at
  59/80**. **FU288 wires nothing (recon only) → census unchanged.**
- **M2 re-pin expectation:** no code change ⇒ the M1/M2 goldens are untouched
  (**no re-pin**; M1 `09b726b7…` immutable). **If** the R11-L4 port lands (the
  ladder + actor-ordering + the claim bridge), the row-`0x1B`→sit-5→phase-9→
  row-`0x1E`→claim→release chain becomes live, `camera.pos_y` becomes `0x50` on
  a granted frame, and the M2 golden (`m2-frames.txt`) **will change** → re-pin
  with a written reason + frame diff (AGENTS.md). The FU273 §4.4 RED-first reach
  test (baseline `pos_y==0`) is the gate for that re-pin.

## 5. GO/NO-GO + legs

**GO** — the actor ordering can be pinned (§2.2): the ladder phase (`2`) and the
actor-write phase (`0xD`) are disjoint gate immediates, and the head-write→emit
pair is one row-`0x1B` dispatch. **Leg T2b-L1 resolves.** Scope caveat: this
pins the *ordering*, not the *port* — FU273 §4.3's **NO-GO for the R11-L4 port**
(the whole keeper-CPU cluster, 13 unported surfaces) **stands unchanged**.

### Numbered open legs

* **FU288-L1 — the ladder install record identity.** `decompile 0x77EAC` installs
  on `param_1`, and `FUN_000782D0` is called with `EBP` = the team base
  (`0x8DB35`). Which physical record `param_1` names (record 0 = keeper?) and
  the two-team call sites (`0x4B2AB`/`0x4B2B2`) are not re-derived here. Owner:
  `FUN_0008D8EC`/`FUN_0004B100`.
* **FU288-L2 — the `2 → 0xD` phase advance.** What writes
  `[0x157A4A]>>0x18` from `2` to `0xD`, and whether a row-`0x1B`-carrying keeper
  survives the intervening phases without being overwritten by
  `FUN_000782D0`'s own `FUN_0007d9a4(rec, 0x19/0x4, 0, 0)` reinstall (`0x78576`).
  Owner: the phase machine (FU-143 / `match_run_ref_step_restart`).
* **FU288-L3 — the 8 unenclosed set-writers' owning rows.** `0x7F299`,
  `0x83E40`, `0x85E4B`, `0x7DC18`, `0x74578`, `0x84B31`, `0x84F25`, `0x85635`
  set `[0x157A83]` on record bodies not named in the fu273/fu288 keeper route;
  each identifies a distinct actor-claiming row. Owner: TBD (row census).
* **FU288-L4 — engine emit route divergence (T2b-L6).** Native
  `FUN_0008A938(5, side, BX=1)` (queue) vs engine `fifa96_match_run_situation(mr,5)`
  (side=0, bx=0; `match_run.c:2949-2951`). Owner: FU-269-L5.
* **FU288-L5 — engine installer phase mismatch (new).** Native installs row
  `0x1B` at `phase==2` (ladder); the engine's ported installer is the phase-6 arm
  → row-`0x1F` no-slot dive (`match_run.c:2838-2851`). Shape-preserving but
  value-divergent. Owner: FU-180-L4/L5.

**Tooling note (for the ledger).** `ghidra_search_instructions(operand_pattern=
"0x157a83")` returned **0 matches** (`instructions_scanned=235063,
truncated=false`) — the watcom LE operand for this global is **not** rendered
with the `0x` prefix this session, so the suggested search term found nothing.
Worked around with the fresh `get_xrefs_to 0x157A83` (112 refs → 22 WRITE) +
per-site `disassemble_bytes`. No write from the MCP tool; read-only.

---

## 6. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| `[0x157A83]` = 112 refs, 22 WRITE / 90 READ | `get_xrefs_to 0x157A83` |
| 22 WRITE-site classification (§1) | `disassemble_bytes` at `0x73E58`,`0x8C3B0`,`0x8CB60`,`0x7A218`,`0x75DC0`,`0x76AC8`,`0x77558`,`0x7F275`,`0x7F290`,`0x83E30`,`0x84F58`,`0x85E40`,`0x74F18`,`0x755C0`,`0x76650`,`0x76DB0`,`0x7DC10`,`0x82540`,`0x84B28`,`0x84F18`,`0x85628`,`0x74568` |
| `0x73E6C` is a whole-cell reset (`DAT_00157a83=0`) | `decompile_function 0x73E28` |
| ladder gate `phase==2 && rec[8]==0 && [0x157C5D]!=0` | `disassemble_bytes 0x78570`; `decompile_function 0x782D0` |
| ladder installs row on `param_1`; skips actor + actor's side | `decompile_function 0x77EAC` |
| row-`0x1B` head gate `phase==0xD` + side → `0x76DBF` | `disassemble_bytes 0x76D28` |
| row-`0x1A` head identical gate `phase==0xD` + side → `0x7665C` | `disassemble_bytes 0x7662C` |
| stage-4 emit reads actor: `0x776D8 MOV EDI,[0x157A83]` / `0x776DE CMP EBP,EDI` / `0x77707 CALL 0x8A938` | `disassemble_bytes 0x776C8` |
| engine `actor_is_rec` OR bridge | `fifa96_match_handlers.c:3733` |
| engine head set `r->controlled`; stage-4 `fifa96_keeper_reposition_b_finish` → notify 5 | `handlers.c:3724`, `:3762`, `:3781-3787`; `fifa96_keeper.c:252-263` |
| engine emit route `fifa96_match_run_situation(mr,5)` side=0/bx=0 | `match_run.c:2949-2951` |
| rows `0x1A` NULL / `0x1B`/`0x1C`/`0x1D`/`0x1E`/`0x1F` wired | `fifa96_match_handlers.c:5517/5519/5521/5523/5525/5527` |
| engine installer phase-6 arm → row `0x1F` | `match_run.c:2838-2851` |
| phase-8/9 arm → `0x1E`/`0x1D` on rec 0 | `match_run.c:2854-2861` |
| census 59/80; README `73%` | `docs/ENGINE.md:8`/`:268`; `README.md:1` |

**Carried docs (read, not rewritten):** FU273 §2.3/§3.3/§4/§5 (this slice's
parent).
