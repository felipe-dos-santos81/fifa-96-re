# FU296 — R11-L4: the engine installer seam (how the ported machines go live)

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 7 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive; confirmed this session by
`ghidra_get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay,
2706 functions). Read-only: no rename/comment/label/function-create/script/save; `/FIFA96.EXE`
untouched; ISO untouched; no git. Every native address/byte below is a **fresh** `get_xrefs_to` /
`disassemble_bytes` of *this* session unless marked *carried* (FU281/FU282/FU283, named).
Engine-source facts cite the built tree by line (read-only). No file is modified except this draft.

**Scope.** FU282 ported the record-0 machine `fifa96_keeper_cpu_step` and FU283 ported the ladder
`fifa96_keeper_cpu_ladder_run`, but the module `src/fifa96_loader/fifa96_keeper_cpu.c` is compiled
*only* into `test_keeper_cpu` (CMake `target_link_libraries(test_keeper_cpu PRIVATE fifa96_engine
fifa96_keeper_cpu)`, `CMakeLists.txt:599`) and is **absent from the `fifa96_engine` library source
list** (`CMakeLists.txt:432-450` — `fifa96_keeper` is linked, `fifa96_keeper_cpu` is not). No engine
translation unit calls `fifa96_keeper_cpu_step`. This document derives the three seams that turn the
ported machines live: (a) the per-frame call site, (b) the minimal producer set, (c) the RED-first
live-reach test.

---

## 1. The native frame order (fresh)

`FUN_000782D0` (the record-0 machine) has a **sole caller**, `0x8DB35` in `FUN_0008D8EC` — fresh
`get_xrefs_to 0x782D0` = 1 (`UNCONDITIONAL_CALL`). `FUN_0008D8EC` (the per-team pre-pass) is called
four times — fresh `get_xrefs_to 0x8D8EC` = 4: `0x4B2AB`/`0x4B2B2` in `FUN_0004B100`, `0x74429`/
`0x74430` (the unrecovered camera driver). Fresh `disassemble_bytes 0x4B295` (32 B) shows the two
`FUN_0004B100` calls land **after the end-block tail** (`0x4B297 MOV word[0x158822],SI`, `0x4B2A8
RET`, then `0x4B2A9 MOV EAX,EBX; 0x4B2AB CALL 0x8D8EC; 0x4B2B0 MOV EAX,EDX; 0x4B2B2 CALL 0x8D8EC`).

Inside `FUN_0008D8EC` the record-0 call (`0x8DB35`, *carried* FU281 §1(g)) sits at the **head of the
`0xB2`-stride record walk** and is followed by `0x785A8 CALL dword[EBP+0x18]` inside `FUN_000782D0`
(*carried* FU282 §1.5). So the native order is: **install (deferred, `0x78576`) → optionally the
ladder (`0x78599`) → the ONE record action dispatch (`0x785A8`)**. The engine already models exactly
one dispatch per record per frame: `fifa96_match_entities_team_update` (`src/fifa96_match_entities.c:696`)
calls `action(ctx, e)` once per record (`:708`), and `action` is `match_run_dispatch_entity`
(`fifa96_match_run.c:1279`), which for record 0 runs the shared tail (`:1409
fifa96_match_dispatch_action(mr, e->code)`). FU281 §3 (`carried`) already ruled that a *standalone*
pre-pass call double-dispatches; the seam below is deliberately the **AUGMENT** form.

---

## 2.(a) Seam A — the per-frame call site

**The `fifa96_keeper_cpu_step` call belongs in the `i == 0` branch of the FU-141 per-record walk,
before the single `action(ctx, e)` call.** Concretely, `src/fifa96_match_entities.c:704` (the
current `if (i == 0) e->timer7b = e->timer79;` fragment of `FUN_000782D0`):

```
for (i …) {
  e = &t->records[i];
  if (i == 0) {
      e->timer7b = e->timer79;              /* existing 0x7836A/0x78374 fragment */
      /* NEW: the record-0 machine skeleton, staged-zero producers */
      fifa96_keeper_cpu_step(&<cpu for team>, roster(e, t, pool/frame), &kc_out);
      if (kc_out.install) e->code = kc_out.install_code;         /* 0x78576 target */
      if (kc_out.ladder_out.install) e->code = kc_out.ladder_out.install_code; /* 0x78599 arms */
  }
  else if (e->skip_9a != 0) continue;
  entity_timer_decay(e, frame->delta);
  if (action) action(ctx, e);               /* the SINGLE dispatch -> 0x785A8 / 0x7CD29 */
}
```

Why here and nowhere else:
- It is the **same walk the native pre-pass drives** (`FUN_0008D8EC` → `FUN_000782D0`), so no second
  pre-pass is introduced (FU281 §3 double-dispatch caution stays honoured).
- Writing `e->code` **before** `action(ctx, e)` makes the existing `match_run_dispatch_entity`
  (`:1409`) run the installed row exactly once — the native `0x78576` install → `0x785A8 CALL
  dword[EBP+0x18]` order. The `match_run_dispatch_entity` outfield gate `if (e->index != 0u)`
  (`:1393`) already excludes record 0 from the outfield input-row machine, so the keeper path cannot
  collide with the outfield path.
- The machine must **not** be called from `fifa96_match_run_frame` (`fifa96_match_run.c:4219-4222`)
  as a separate step: that is *after* the walk already dispatched record 0, hence a double dispatch.

**State home.** The `fifa96_keeper_cpu` struct (`include/fifa96_loader/fifa96_keeper_cpu.h:76`)
carries `head_flag`/`counter_ad0`/`counter_acc`/`force_install`/`interlock_157c5d`/`latch_157c5c`.
The engine already keeps distinct run cells `global_157c5c`/`global_157c5d` (`include/…/fifa96_match_run.h:678-679`,
zeroed `fifa96_match_run.c:2044-2045,2167-2168`) and `keeper_latch_157ab2` (`:660`). The seam must
**bind** the cpu's `interlock_157c5d`/`latch_157c5c`/`force_install` to those run cells each frame
(read run → step → write run), so the native `[0x157C5D]` interlock and `[0x157AB2]` latch stay single
process cells. Owner: this task's promotion; duplications are FU296-L1.

**Linking.** Seam A is a no-op until `src/fifa96_loader/fifa96_keeper_cpu.c` is added to the
`fifa96_engine` source list (`CMakeLists.txt:432-450`) — currently absent. Owner: FU296-L1.

---

## 3.(b) The minimal producer set for a live ladder install

A ladder install (rows `0x1A`/`0x1B`/`0x1C`/`0x4`) fires only when the whole chain below is open on a
**live frame**. Each producer cell → its engine owner (already modelled) or its open leg.

### 3.1 To reach the ladder gate at all (`fifa96_keeper_cpu.c:48-67`)

| # | native cell | guards | engine owner / leg |
|---|---|---|---|
| P1 | `[0x110680][code] & 1` (`0x784DC`, `permit`) | skips **both** the install tree and the ladder gate (`JZ 0x7859E`; FU282 §1.3) | **LEG FU282-L5** — the per-action-code install-permission table is unported; the caller must supply it (header `:25`). |
| P2 | `phase == 2` (`0x7857B`) | ladder gate | OWNED — `mr->state.phase` (`match_run_entity_frame` `:1538`). |
| P3 | `dword[rec+0x20] == 0` (`0x78588`, `queued20`) | no queued action | OWNED — the slot/queued binding (`entity.has_slot`/`mr->slot.entity`; `match_run_dispatch_entity:1370`). |
| P4 | `[0x157C5D] != 0` (`0x7858E`, `interlock`) | the one-shot gate | PRODUCER EXISTS but is UNWIRED: sole setter `0x74707` (fresh `get_xrefs_to 0x157C5D` = 3: read `0x7858e`, clear `0x785a2`, set `0x74707`), the row-`0x19` keeper-hold body → engine `fifa96_match_handlers.c:4097 mr->global_157c5d = 1`. The run cell is **never read** by any engine code today (grep: only written/cleared). Owner: the seam (bind, §2) + FU296-L3. |

So the machine's install tree and ladder gate require **P1** (unported) and, for the ladder, **P4**
(set by a live row-`0x19` body, but unread today). The clear `fifa96_keeper_cpu.c:68` (`0x785A2`)
must be written back to `mr->global_157c5d`.

### 3.2 To pass the ladder head gates (`fifa96_keeper_cpu.c:97-109`)

| # | native cell | gate | engine owner / leg |
|---|---|---|---|
| P5 | `byte[rec+0x91] == 0x19` (`0x77EC2`) | keeper must be in row `0x19` | OWNED — `entity.code` (entities.h:71). Row `0x19` body is ported + installer live (`fifa96_match_action_19`, handlers table `0x19`). |
| P6 | `[0x14C1D4][idx] AH&2` (`0x77EF0`, `prepass_ah2`) | per-side prepass table | **table PORTED** as `fifa96_input_range_words` (`src/fifa96_input.c:86`) → `mr->input_range` (`src/fifa96_match_run.c:484/2271`) incl. the bit-9 gate (`cells[2]/[5]==1 → |=0x200`, `input.c:97-98`); only the `prepass_ah2` **glue** is absent (LEG FU283-L1 / FU291-L2) — caller supplies a real `0`. |
| P7 | `byte[rec+0x9A] == 0` (`0x77EF6`, `gate_9a`) | skip state | OWNED — `entity.skip_9a` (entities.h:96). |
| P8 | `*(param_1[10]) == 'T'` (`0x77F03`, `is_T`) | the `+0x28` row-string | **LEG** — the `+0x28` anim-row descriptor is not a modelled cell; effectively `0` while no `'T'` descriptor is bound. |
| P9 | `word[rec+0x81] == 0` (`0x77F16`, `timer81`) | timer clear | OWNED — `entity.timer81` (entities.h:117). |
| P10 | `rec != [0x157A83]` (`0x77F36`, `is_actor`) | not the actor | OWNED — `mr->entities.controlled` (entities.h:304). |
| P11 | actor side != rec side (`0x77F51`) | opposite side | OWNED — `team.side` + actor `team.side`. |
| P12 | `[0x1577CA]` tracked + side + `byte +0x91` (`0x765C4`) | the code-`4` fall-through | `tracked_present` OWNED (`render.camera.tracked`, `fifa96_match_run.c:571`); `tracked_same_side`/`tracked_code` **LEG** (unported tracked record); `global_158740` **LEG**. |

### 3.3 The blocker — the stage index (`fifa96_keeper_cpu_ladder_run:112`)

| # | native cell | gate | engine owner / leg |
|---|---|---|---|
| P13 | `CX ∈ 1..5` (`rec->stage`, `0x781E1 TEST CX,CX; JZ 0x782C4`) | the selector index | **LEG FU283-L6 — NO LIVE PRODUCER.** The stage machine (`0x77F95..0x781A2`) reads four input families: `[0x14C1D4]` (table **ported**, only the `prepass_ah2` glue absent), the `0x10E184`/`0x10E1C4`/`0x10E1CE` stage tables, the `[0x157C4C]`/`[0x157C48]`/`[0x157C2C]` latches, and the stage geometry words `[0x1577F4/F8]`/`[0x1577BC]`/`[0x157821]`/`[0x158740]` (the latter three groups still unported). |

**This is the decisive producer.** With the seam wired and P1–P12 forced open but **no live stage
index**, `rec->stage == 0` and the ladder returns at `0x781E1` — `out.install == 0`. The ladder
cannot self-raise a stage on the current engine state.

### 3.4 The engine side of an install (what the install then reaches)

The lane table (`fifa96_match_handlers.c:5575`) maps the ladder's installs to engine bodies:
`0x1B` → `fifa96_match_action_1B` (**wired**, installer live: row-`0x1F` no-slot dive `0x76557`),
`0x1C` → `fifa96_match_action_1C` (**wired**, installer live), `0x4` → `fifa96_match_action_04`
(wired). **`0x1A` → NULL** — the rows-27/29/2C precedent: an install of `0x1A` dispatches
`-FIFA96_ERR_UNSUPPORTED` (tolerated by the chain, no movement). So the only ladder arms that
reach a real body are `0x1B`/`0x1C`/`0x4`; the `0x1A` arms stay a no-op until FU186-L1.

---

## 4.(c) The RED-first live-reach test seam

A test that drives one granted frame of a live match and asserts the install reached the entity —
**failing on the phase-26c baseline**.

**Fixture (drive):** a normal match run (`fifa96_match_run_begin`, the m2 rig shows a fresh match has
`render.camera.pos_y == 0`, `tests/test_engine_m2.c:2482`), then stage, for `team 0` record 0:
- `entity.code = 0x19` (P5, keeper-hold), `entity.index = 0`;
- `entity.skip_9a = 0` (P7), `entity.timer81 = 0` (P9);
- the record is **not** the controlled actor, or is on the opposite side of it (P10/P11);
- `mr->slot.entity != 0` so the record has no queued action (P3, `dword[+0x20] == 0`);
- the cpu's gate cells: `permit = 1` (P1), `prepass_ah2 = 0` (P6);
- **the caller-supplied live stage index `stage = 4`** (P13) on the keeper record — the header
  already documents `stage` as caller-supplied (`fifa96_keeper_cpu.h:38-40`);
- `mr->global_157c5d = 1` (P4, the row-`0x19` setter's cell).

**Assert (RED):** after `fifa96_match_run_frame`:
- `team0.records[0].code == 0x1B` (the ladder's CX-4 arm, `0x78294/0x78299`, reached the entity's one
  dispatch), **and**
- the observable the native path moves: `mr->render.camera.pos_y != 0` (the row-`0x1B` camera-place
  body) — or, if the chosen arm leaves the camera, the record `pos_*` change the row body writes.

On the phase-26c baseline the machine is unlinked/unwired, so `records[0].code` stays `0x19` and
`camera.pos_y` stays `0` (`tests/test_engine_m2.c:2482`) → the assertion fails (RED). Once Seams A
+ the bind land, the staged frame drives the ported ladder and the assertion goes green.

**Seam-appropriate assertion caveat.** The cleanest RED observable is `records[0].code == 0x1B`
(directly names "the install reached the entity"); the `camera.pos_y != 0` observable additionally
exercises the wired row-`0x1B` body and is the baseline the task names. A test should assert the
first, and (optionally) the second. Owner: FU296-L4.

---

## 5. GO / NO-GO

**Seam A (per-frame call): GO.** The call site is unambiguous (`fifa96_match_entities.c:704`, the
`i == 0` branch, before the single `action(ctx, e)`), it preserves the one-dispatch invariant, and the
module just needs to be added to the `fifa96_engine` link.

**A faithful self-raised live install: NO-GO.** The ladder's selector index `CX` (P13) has **no live
producer** — the stage machine's other input families are unported (FU283-L6), so `stage == 0` and
every arm returns with no install. P1 (`[0x110680]`, FU282-L5) is
likewise unported; P6 (`[0x14C1D4]`, FU283-L1) is **ported as a table** but has no live
`prepass_ah2` glue. With today's engine cells only a *staged* stage index can raise an install.

**A test-staged live install: CONDITIONAL GO.** With Seam A + the bind (§2) and the caller-supplied
`stage`/`permit`/`prepass_ah2`, the ported ladder installs `0x1B`/`0x1C`/`0x4`, the entity's single
dispatch runs the wired body, and the RED-first test (§4) goes green. This is a **bounded,
zero-golden-delta** slice *only if* the staged producers are test-only (default `stage = 0` keeps the
live tape unchanged); wiring a real stage index is the phase-scale keeper-CPU cluster (FU273 NO-GO),
unchanged.

**Tooling:** none. Fresh reads this session (`get_xrefs_to 0x782D0`=1, `get_xrefs_to 0x157C5D`=3,
`get_xrefs_to 0x8D8EC`=4, `disassemble_bytes 0x4B295`); program stayed `/FIFA96.EXE`; no write outside
this draft.

---

## 6. Numbered open legs

- **FU296-L1 (owner: this task's promotion / build).** Link `src/fifa96_loader/fifa96_keeper_cpu.c`
  into `fifa96_engine` (`CMakeLists.txt:432-450`) and reconcile the `fifa96_keeper_cpu` struct's
  `interlock_157c5d`/`latch_157c5c`/`force_install` with the run cells `global_157c5d`/`global_157c5c`/
  `keeper_latch_157ab2` (single process cell; the cpu is a *view*, not a second copy).
- **FU296-L2 (owner: FU282-L5).** P1 — the `0x110680` install-permission table. Unported; without it
  the machine's gate at `0x784DC` (and hence the ladder gate) cannot open on a live record.
- **FU296-L3 (owner: FU179 / row-0x19).** P4 — `[0x157C5D]` is set live by the row-`0x19` body
  (`fifa96_match_handlers.c:4097`) but is **never read** by any engine code; the seam must bind it into
  the machine. Until then the interlock is set-and-forgotten on the ported path (FU282-L7).
- **FU296-L4 (owner: FU296 follow-up / tests).** The RED-first live-reach test (§4): decide whether
  the primary assertion is `records[0].code == 0x1B` or `camera.pos_y != 0`, and whether the staged
  producers are test-only (zero golden delta) or promoted.
- **FU296-L5 (owner: FU283-L6).** P13 — the live stage index. The **blocker** for a self-raised live
  install; four input families (`[0x14C1D4]` **ported as a table**, `0x10E1xx`, `[0x157C4x]`, the geometry words).
- **FU296-L6 (owner: FU283-L1).** P6 — `prepass_ah2` (`[0x14C1D4]` `AH&2`); the
  `[0x14C1D4]` table is **ported** (`fifa96_input_range_words`, `src/fifa96_input.c:86`),
  only the `prepass_ah2` glue is absent; it must be a real `0`.
- **FU296-L7 (owner: FU186-L1).** The `0x1A` ladder arms install a **NULL** engine slot
  (`fifa96_match_action_table[0x1A]`, `handlers.c`); the row body is ported but uncounted, so the
  `0x1A` installs are inert until FU186-L1. Only `0x1B`/`0x1C`/`0x4` reach live bodies today.
- **FU296-L8 (owner: FU296 follow-up).** The invoke-now reconciliation: a ladder install is
  `FUN_0007D9A4(rec,code,0,ECX=1)` (invoke-now, FU283 §4) while the machine's own install is deferred
  (`ECX=0`, FU282 §1.3); the native therefore runs a ladder-installed row at the install **and** again
  at `0x785A8`. The engine's single `action(ctx, e)` runs it once; the outfield precedent
  (`match_run_dispatch_entity:1393-1407` re-stages then re-dispatches) is the model to reconcile this.

*(End of draft.)*

## Closures / errata (phase-27 T5)

* **P13 / FU296-L5 — ADVANCED (the stage index is now live).** FU290's machine
  is ported as `fifa96_keeper_cpu_stage_select` and wired into Seam A: the seam
  sets `kc_rec.stage` **from the machine**, retiring the T3 `rec.stage = e->stage`
  caller bridge. The `AX` prepass gate and the `CX=1` arm read only live cells,
  so a **non-zero** `CX` now exists from real engine state. The remaining
  blocker to an end-to-end **install** is P1 (`[0x110680]` install-permission,
  FU296-L2/FU282-L5) — a different unported producer, zero live — plus the
  `CX=1`→row-`0x1A` NULL lane (FU296-L7). M2 stays byte-identical; census holds
  `59/80`.
* **§3.3 / §4 fixture note superseded.** The RED-first test's "caller-supplied
  live stage index `stage = 4`" is replaced by the machine's **inputs**
  (`mr.render.camera.pos_z` for the `AX` gate, `keeper->lane`, an opposite-side
  `mr.entities.controlled`); T4's `test_keeper_installer_reach` now computes
  CX=4 faithfully. The `[0x157C5C]` latch is set by the machine's own CX=4 arm
  (`0x780D7`), not staged.
* **P4 / FU296-L3 — partially closed.** `keeper_cpu.latch_157c5c` is now
  written by the ladder and bound to `mr->global_157c5c` (the native row-`0x1B`
  `0x76FF4` reader). The interlock `[0x157C5D]` bind is unchanged.

### Fix round (T5 review)

* The ladder's `+0x4D` copy side effect is now **ported** (Seam A writes
  `e->target_*`), and the machine's `[0x157C5C]` write is **published in-frame**
  by the dispatch (the frame-level bind alone lagged a frame); the T4 reach no
  longer stages `global_157c5c`.
* The stage-table `C` entry for `A=5` was corrected to `0` (fresh
  `read_memory`); see FU290's `Fix round`.
* P13 remains ADVANCED: the machine is live-wired, but the end-to-end install
  still needs P1 (`[0x110680]`, FU296-L2). Census `59/80`; M2 byte-identical.

## Erratum (phase-27 T8 close-out)

* **The T5 text "the seam sets `kc_rec.stage` **from the machine**" is imprecise
  (quoted above, kept verbatim).** `fifa96_keeper_cpu_record` has **no** `stage`
  field and the seam writes none; `fifa96_keeper_cpu_stage_select` computes `CX`
  into a **local** `fifa96_keeper_cpu_stage_out.stage`, and the seam consumes
  `kc_out.install` / `kc_out.ladder_out.install`. (The same correction lands in
  `FU290_r11l4_stage_select.md`.)
