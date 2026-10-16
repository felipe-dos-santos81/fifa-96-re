# FU299 — the interlock `[0x157C5D]` ordering (R11-L4)

**Phase/Provenance:** Phase-28 Stage-1 recon wave, Task 2/8 (`FU299`); read-only derivation on
`/FIFA96.EXE` (Ghidra MCP; fresh `get_current_program_info` this session: `watcom:LE:32:default`,
image base 0, `.image` overlay, 2706 functions). Every address below is a **fresh** read this
session unless marked *carried* (source named). No program state was written; no git was run.

The brief's `0x785A2` "clear inside `FUN_00077EAC`" is a **mis-attribution** (erratum E1 below):
fresh `get_function_by_address 0x785A2` places the clear in `FUN_000782D0` (body
`0x782D0..0x785D0`), immediately after the `CALL 0x77EAC` at `0x78599` returns.

---

## 1. The native cell — fresh census

Fresh `get_xrefs_to 0x157C5D` = **3** (the complete reader/writer set):

| ref | site | owner | meaning |
|---|---|---|---|
| READ | `0x7858E` `CMP byte[0x157C5D],0` / `0x78595 JZ 0x7859E` | `FUN_000782D0` | the **ladder gate** |
| WRITE (clear) | `0x785A2` `MOV byte[0x157C5D],CH` (`CH` zeroed at `0x7859E` `XOR CH,CH`; bytes `882d5d7c1500`) | `FUN_000782D0` | the **per-step clear** |
| WRITE (set) | `0x74707` `MOV byte[0x157C5D],AH` (`AH=1` set at `0x74700`; bytes `88255d7c1500`) | **no Ghidra function** (row-`0x19` body head `0x746E4`; dispatched via `CALL dword[rec+0x18]`) | the **row-`0x19` set** |

Fresh `disassemble_bytes 0x78570` (64 B) pins the full step order inside `FUN_000782D0`
(record-0 machine; sole caller `0x8DB35` in `FUN_0008D8EC` — fresh `get_xrefs_to 0x782D0` = 1):

```
0x784D3..0x784E9  permit gate  MOV EAX,[EBP+0x8E]; SAR 0x18; MOV AL,[EAX+0x110680];
                                AND AL,1; JZ 0x7859E          (fresh 0x784D0, bytes 0f84af000000)
0x784EF..0x78576  install tree (timer81/[0x157A4D]/ball_owner/owner_rel_5 -> 0x00/0x19/0x05/0x04;
                                CALL 0x7D9A4 deferred, EBX=0)
0x7857B..0x78586  gate: [0x157A4A]>>24 == 2     (phase; byte [0x157A4D])  JNZ 0x7859E
0x78588..0x7858C  gate: dword[EBP+0x20] == 0    (queued20)              JNZ 0x7859E
0x7858E..0x78595  gate: byte[0x157C5D] != 0     (THE INTERLOCK)          JZ  0x7859E
0x78597/0x78599   CALL 0x77EAC                  (the ladder seam)
0x7859E           XOR CH,CH
0x785A2           MOV byte[0x157C5D],CH         (CLEAR — unconditional)
0x785A8           CALL dword[EBP+0x18]          (the record's row dispatch; row 0x19 -> 0x746E4)
0x785AD           CALL 0x6E8E8
```

The clear is on **every** path: the permit-gate `JZ 0x7859E` (skip install tree **and** the whole
gate block **and** the ladder), every gate-fail `JZ/JNZ 0x7859E`, and the ladder-call fall-through
all converge on `0x7859E` → `0x785A2`. It runs whether or not the ladder ran. The set
(`disassemble_bytes 0x746E0`, 48 B: `0x746F9 [EAX+0x9E]=1`, `0x74700 MOV AH,1`, `0x74707`) is the
row-`0x19` body's **entry prologue** — unconditional every time that row dispatches.

## 2. The native ordering model (one cell, per step)

Within one `FUN_000782D0` step: **gate read → ladder → clear → the same step's row dispatch**. The
clear **precedes** the row body, so a row-`0x19` set lands *after* the step's clear and **survives
the step**; the next record-0 step's gate read (`0x7858E`) sees it. Semantics: the interlock is a
**one-step pulse** — "the row-`0x19` body ran in the immediately preceding record-0 step".

Cross-team (fresh `get_xrefs_to 0x8D8EC` = 4; `disassemble_bytes 0x4B29C`: `0x4B2A9 MOV EAX,EBX;
CALL 0x8D8EC` / `0x4B2B0 MOV EAX,EDX; CALL 0x8D8EC`): the frame driver calls the per-team walker
**twice per frame** (both team bases), and each team's walk runs its record-0 machine **first**
(fresh `disassemble_bytes 0x8DB20`: `0x8DB35 CALL 0x782D0`), then records 1..10 (`CALL 0x7CA54`).
Since the cell is **shared** and each step clears it, the last record-0 step of a frame (team 1's)
wipes the first team's same-frame set: team 1's step gate is armed **same-frame** by team 0's row
set; team 0's step gate can only be armed by team 1's **previous-frame** row set. (Derived; runtime
proof is FU299-L1.) An outfield record holding code `0x19` would set the cell with **no clear**
(no `[0x157C5D]` ref exists outside `FUN_000782D0`) — FU299-L2.

## 3. The port today — the inverted write-back

Engine paths (fresh reads):

- `src/fifa96_match_run.c:4249` — **frame-head bind** `cpu->interlock_157c5d = mr->global_157c5d`
  (before `fifa96_match_entities_update` at `:4254`);
- `src/fifa96_match_entities.c:780` — the step (per team, `i == 0` branch `:732-791`), **before**
  that record's `action(ctx, e)` (`:795-798`); the walk runs team 0 then team 1 (`:855-856`);
  the step gates on `cpu->interlock_157c5d` (`src/fifa96_loader/fifa96_keeper_cpu.c:65`) and clears
  it unconditionally (`:80` = native `0x785A2`);
- the record-0 dispatch runs the row body: row `0x19` → `src/fifa96_engine/fifa96_match_handlers.c:4096-4097`
  `r->ran = 1; mr->global_157c5d = 1;` (native `0x746F9`/`0x74707`) — **inside the same walk**;
- `src/fifa96_match_run.c:4257` — **frame-tail write-back** `mr->global_157c5d = cpu->interlock_157c5d`
  (the value the step already cleared), **after** the whole walk.

**Inversion:** the native clear runs *before* the row set; the engine re-runs the clear *after* it.
The row-`0x19` set is clobbered before the next bind can read it ⇒ on un-staged live frames the gate
is **always false** (`interlock != 0` never holds) ⇒ porting P1 `[0x110680]` alone still will not
arm the ladder. Second inversion: the bind is frame-level, so **team 1's step gate reads the
frame-head value, not team 0's same-frame set** — exactly the native arm that team 1's ladder uses.
Inert today (`permit == 0`; register quote below) and M2-invisible.

## 4. Fix options and the chosen shape

**Option A — in-frame publish à la `latch_written` (the T5 idiom, `run.c:1415-1418` + FU292:157).**
Publish `view → mr->global_157c5d` at the record-0 dispatch seam (next to the latch publish), delete
`:4257`. *Assessment:* the next-frame arming works, but team 1's step gate still reads the
frame-head bind — it **misses the same-frame cross-team arm**, which §2 shows is the primary arming
path for the team whose step runs second. The idiom fits the latch's direction (write→same-frame
read); the interlock's direction is read→later-write (next step), so the idiom is the wrong tool.

**Option B — the corrected bind/step/write-back order (CHOSEN).** Bind and publish hug the step:
per record-0 step, bind `mr->global_157c5d → cpu->interlock_157c5d` immediately **before**
`fifa96_keeper_cpu_step` and publish `cpu->interlock_157c5d → mr->global_157c5d` immediately
**after** it (`entities.c:780`), both before that record's `action()` dispatch; **delete**
`run.c:4249` and `run.c:4257`. The cell stays `mr->global_157c5d` — the row-`0x19` writer is
**unchanged**. Mechanics of the cell pointer (frame field vs machine field) are FU299-L3; the frozen
semantic is per-step bind-before/publish-after. This reproduces the native per-step order for both
teams, both arming paths (same-frame team-0→team-1, previous-frame team-1→team-0).

**Port shape (frozen):** one process cell (`mr->global_157c5d`) + per-step bind/publish at the
Seam A step (`entities.c:741-791`, per team, before the single dispatch); the frame-level bind
(`run.c:4249`) and write-back (`run.c:4257`) are deleted; the step (`fifa96_keeper_cpu.c:65/80`) and
the row-0x19 handler (`handlers.c:4097`) are untouched. **Zero-δ** while `permit == 0` (the ladder
block stays behind the permit gate; the cell is read by nothing else live — today's only reader is
the `:4249` bind itself).

**Test seam.** The FU296 Seam A fixtures, `tests/test_engine_match_frame.c`:
`test_keeper_installer_reach` (`:3486-3553`) and `test_keeper_installer_live_waiver` (`:3582-3624`)
stage `mr.global_157c5d = 1` at `:3507`/`:3600` precisely because the write-back clobbers the live
set (staging a **producer input** is the plan's legitimate rule, `:3460-3461`). Riders:
(a) **cell survival** — team 1's keeper (the frame's last step) in code `0x19`: after one frame
`mr.global_157c5d == 1` (RED today: `:4257` zeroes it); (b) **same-frame cross-team arm** — team 0's
keeper `0x19` (setter) + team 1's keeper `0x19` with the T4 ladder-open inputs and the actor moved
off team 1 rec 0 (`is_actor` skip, `0x77F36`): team 1's ladder installs `0x1B` in the **same** frame
(RED today: team 1's gate reads the frame-head bind); (c) the existing T4 reach keeps its staged P4
(it now models the previous-frame team-1 row). Unit tests `tests/test_keeper_cpu.c:229-287` are
unaffected if the machine field remains the backing cell (FU299-L3 mechanism choice).

## 5. Evidence table

| # | Claim | Evidence (fresh this session unless marked carried) |
|---|---|---|
| 1 | `[0x157C5D]` census: 1 reader, 2 writers | `get_xrefs_to 0x157C5D` = 3: read `0x7858E`, clear `0x785A2`, set `0x74707` |
| 2 | Step order gate→ladder→clear→row dispatch | `disassemble_bytes 0x78570` (64 B): `0x7857B/88/8E`, `0x78599` (`e80ef9ffff`), `0x7859E` (`30ed`), `0x785A2` (`882d5d7c1500`), `0x785A8` (`ff5518`) |
| 3 | Clear owned by `FUN_000782D0`, not `FUN_00077EAC` | `get_function_by_address 0x785A2` → `FUN_000782d0` (body `0x782d0..0x785d0`) |
| 4 | Permit gate skips tree+gate, clear still runs | `disassemble_bytes 0x784D0` (112 B): `0x784DC..0x784E9` (`0f84af000000` → `0x7859E`) |
| 5 | Set = row-`0x19` body entry, unconditional; not a function | `disassemble_bytes 0x746E0` (48 B): `0x746F9`/`0x74700`/`0x74707`; `get_function_by_address 0x74707` → none |
| 6 | Record-0 machine first in each team's walk; two per-team calls per frame | `get_xrefs_to 0x782D0`=1 (`0x8DB35`); `disassemble_bytes 0x8DB20` (`CALL 0x782D0` then records via `0x7CA54`); `get_xrefs_to 0x8D8EC`=4; `disassemble_bytes 0x4B29C` (`0x4B2AB`/`0x4B2B2`) |
| 7 | Engine inversion: bind `:4249` / step / row set / write-back `:4257` | `src/fifa96_match_run.c:4249,4254,4257`; `src/fifa96_match_entities.c:741-791,795,855-856`; `handlers.c:4096-4097`; `keeper_cpu.c:50,65,80` |
| 8 | T5 publish precedent (different cell/direction) | `run.c:1415-1418`; *carried* `docs/ghidra/FU292_r11l4_latch_cells.md:157` |
| 9 | Test seam stages the interlock | `tests/test_engine_match_frame.c:3486-3553` (`:3507`), `:3582-3624` (`:3600`), `:3460-3461`; `tests/test_keeper_cpu.c:229-287` |
| 10 | Co-blocker register + carried legs | `docs/ENGINE.md:1874-1900` (fresh read); *carried* `docs/ghidra/FU296_r11l4_installer_seam.md:208-210` (FU296-L3), FU257-L6 (`docs/ghidra/FU257_keeper_cpu_ladder.md:281-285`) |

**Errata (quoted).** **E1:** the brief/plan cite "the `0x785A2` clear inside `FUN_00077EAC`" —
wrong owner: fresh `get_function_by_address 0x785A2` = `FUN_000782D0`, after `CALL 0x77EAC` returns
(row 2/3). **E2:** FU296-L3's "`[0x157C5D]` … is **never read** by any engine code … set-and-
forgotten" is superseded for the engine side — the seam **does** bind it (`run.c:4249`) and reads it
via the step gate; the defect is the write-back clobber, not a missing reader. The **native** reader
exists as well (`0x7858E`, fresh), so the interlock was never truly read-less.

## 6. Numbered open legs

- **FU299-L1 (owner: `FUN_0004B100` `0x4B2AB`/`0x4B2B2` + `FUN_0008D8EC` `0x8DB35` + the shared
  cell).** The cross-team same-frame / cross-frame arming in §2 is *derived* from the fresh two-call
  walk, the per-team record-0-first order and the single cell; no runtime capture proves it. Port
  rider (b) tests the engine side.
- **FU299-L2 (owner: `FUN_0007CA54`; carried FU-179 row-04 producer `0x7E866`/`0x7E8C1`).** Can an
  outfield record hold code `0x19` (its row would set the cell with **no in-walk clear**, so the set
  persists across frames until the next record-0 step)? Only the record-0 installer tree is
  statically proven as a `0x19` producer this session; the carried row-04 producer's target record
  is not re-derived.
- **FU299-L3 (owner: the cell `[0x157C5D]` itself; port shape).** The pointer mechanics of the hug
  (cell carried on the entities frame vs on the machine object) is a Stage-2 plan choice; the frozen
  semantic is per-step bind-before/publish-after + deletion of `run.c:4249`/`:4257`.
- **FU299-L4 (owner: `FUN_0007D9A4` call sites `0x78576` deferred vs `0x782BB` invoke-now; carried
  FU296-L8).** A ladder install re-codes record 0 invoke-now inside the step, so the row-`0x19` set
  fires only on steps where the row-`0x19` body actually dispatches; the interlock's interaction
  with the invoke-now reconciliation is carried, not re-derived.
- **FU299-L5 (owner: the `[0x157C5C]` writers, FU292).** The latch's frame-tail write-back
  (`run.c:4258`) is currently idempotent under the T5 in-frame publish (no writer between the
  publish and the write-back); if a future handler writes `mr->global_157c5c` like row-`0x19` writes
  `[0x157C5D]`, the same inversion class appears — the Stage-2 fix should cover `:4257-4259` as a
  block, not the interlock line alone.

## Review erratum (wave review, 2026-10-16)

Wave review verdict: OK — Option B is mechanically sound. The review's fresh reads confirm the `[0x157C5D]` clear site is in `FUN_000782D0`, not inside a function named `0x77EAC` (the draft's E1 erratum stands).

**Verdict:** GO — Option B mechanically sound; `[0x157C5D]` clear site confirmed in `FUN_000782D0` (draft E1 erratum stands).
