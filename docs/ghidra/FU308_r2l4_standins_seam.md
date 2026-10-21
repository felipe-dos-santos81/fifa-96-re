# FU-308 — the `[0x157750]`/`ball.y` stand-ins and the keeper `follow_speed` seam

**Phase/Provenance:** Phase-29 Stage-1 recon wave (task 3/4, FU308), 2026-10-21,
read-only Ghidra MCP session on the authoritative program **`/FIFA96.EXE`**
(`get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image`
overlay, 2708 functions — the wave brief's 2706 is stale by two analysed
functions; name/path/format match exactly, `scripts` unused). Every
address/byte below is a fresh `disassemble_bytes` / `get_xrefs_to` /
`get_function_by_address` of this session unless marked *carried*. Engine
claims cite the built sources by file:line. No repo file changed except this
draft; no git, no Ghidra mutation, ISO untouched.

Parent spec: `docs/superpowers/specs/2026-10-21-fifa96-m2-phase29-visible-follow-cam-design.md`
§3 slice 3, §8. Frozen inputs carried: `docs/ghidra/FU171_sustained_follow_inputs.md`
(§2.1/§2.3/§3.2, the FU171-L7 leg) and `docs/ghidra/FU168_pan_arm_completion.md`.

---

## 1. Result in one line

**Four `ball.y` stand-ins remain for the native camera-Y cell `dword[0x157750]`
(row 05 `0x7F2C8`, row 06 `0x80247`/`0x8061B`, row 08 `0x81157`, and the
record machine's code-8 chase gate `0x7CCD8`); each is a mechanical re-stage to
`mr->render.camera.pos_y`, and the keeper seam is a one-line bind fix — the
CX=1 gate `0x78046..0x78051` reads the live word `0x1577BE` (`camera.speed`),
which the engine currently feeds from the *derived pre-tail snapshot*
`camera.follow_speed`.** Both slices GO; the seam has an exact granted-frame
RED witness (`vel_x` staged so the walk tail stores `speed = 0xF` while
`follow_speed` stays 0).

## 2. Scope and method

Two questions, answered first-hand:

1. **The stand-in list.** Which engine sites still stage
   `mr->entities.ball.y` for the native cell `dword[0x157750]`, what native
   instruction each feeds, and what the exact re-stage source is (FU171-L7).
2. **The keeper seam.** The native CX=1 gate's third operand (the
   `follow_speed < 0xA` test at `src/fifa96_loader/fifa96_keeper_cpu.c:182`):
   which native cell it reads, which engine field it is fed, and the
   consequence.

Method: fresh disassembly of each site; a fresh `get_xrefs_to 0x157750`
census; fresh `get_xrefs_to` on the two cells of the seam (`0x1577BE`,
`0x8D8EC`); the engine bind chain read line by line; the frame order proved
from the native caller bytes.

## 3. The cell, re-verified

### 3.1 `dword[0x157750]` is the camera triple's Y

Fresh `get_xrefs_to 0x157750` = **146 refs** (100 + 46 paged; matches the
FU264 §Step-1 census). The four sites in scope all load the dword (signed
`CMP`/`MOV` of 4 bytes), and the two "camera triple" copies confirm the cell is
the Y of `0x15774C..0x157754`: fresh `0x805DE..0x805E0` (`MOVSD ×3` from
`ESI = 0x15774C`) has its middle read listed as a `0x157750` ref, and so does
`0x81125`. Alias warning applied: `0x78046 MOV EAX,[0x1577BC]; 0x7804B SAR
EAX,0x10` reads the **high word `0x1577BE`**, not the word at `0x1577BC`
(FU292's "`0x1577BC` camera-band Z/extent word" is the *low* word — a
different cell; see §6.1).

### 3.2 The four native sites, fresh bytes

**Row 05 — the carrier stage-0 type-arm gate** (`fifa96_action_carrier_arm`,
native `0x7F194..0x7F665` *carried* from the engine header/FU-139):

```
0x7F2B5  CMP dword[EBP+0x5D],0        ; airborne record waits
0x7F2BF  MOV EAX,dword[EBP+0x8B]; SAR EAX,0x18   ; the +0x8E octant
0x7F2C8  MOV EBX,dword[0x157750]      ; <-- the camera-Y cell
0x7F2CE  MOVSX DX,byte[EAX+0x10F334]
0x7F2D6  MOVSX AX,byte[EAX+0x10F33C]
0x7F2E6  CMP EBX,0x38; JLE 0x7F304    ; camY > 0x38 -> type dirs, speed 0x60
0x7F2EB  MOV ECX,0x60
```

**Row 06 — the pursuit/carrier gate and the target gate** (row-06 handler
`0x801B4..`, *carried* FU-139 row table):

```
0x8022D  MOV EAX,[0x158724]           ; carrier
0x8023A  MOV EAX,dword[EAX+0x69]; SAR EAX,0x10; CMP EAX,0x90; JG 0x80250
0x80247  CMP dword[0x157750],0x70; JLE 0x8026D   ; camY > 0x70 -> install 4
0x80250  MOV ECX,1; MOV EDX,4; XOR EBX,EBX; CALL 0x7D9A4
...
0x8061B  CMP dword[0x157750],0x38; JG 0x809E6    ; camY > 0x38 -> skip the
                                                  ; target-selection block
```

**Row 08 — the receiver-timer arm** (row-08 handler `0x81068..0x814AF`,
*carried* FU-137):

```
0x8114C  MOV EAX,dword[EBP+0x69]; SAR EAX,0x10; CMP EAX,0x50; JG 0x81160
0x81157  CMP dword[0x157750],0x38; JLE 0x81185   ; lane <= 0x50 && camY <= 0x38
                                                  ; -> skip
0x81160  MOV EAX,EBP; CALL 0x79B58               ; lane > 0x50 || camY > 0x38
```

**The record machine's code-8 chase gate** (`FUN_0007CA54`, caller
`FUN_0008D8EC`):

```
0x7CCCD  MOV EDX,dword[EBP+0x69]; SAR EDX,0x10; CMP EDX,0x50; JGE 0x7CD29
0x7CCD8  CMP dword[0x157750],0x30; JGE 0x7CD29   ; camY >= 0x30 -> no chase
0x7CCE1  MOV ESI,[0x157A83] ...                  ; the user/side/unbound gate
```

All four are **signed 32-bit** compares of the same cell.

## 4. The exact stand-in list

The engine sites (fresh reads of the built sources):

| # | engine site | native read(s) | native gate | current source | exact re-stage |
|---|---|---|---|---|---|
| 1 | `fifa96_match_action_05`, `src/fifa96_engine/fifa96_match_handlers.c:2813` (`c.ball_height`) | `0x7F2C8` | `0x7F2E6` `camY > 0x38` → dribble type-table dirs, speed `0x60` (vs slot dirs `0x30`) | `mr->entities.ball.y` | `(int32_t)mr->render.camera.pos_y` |
| 2 | `fifa96_match_action_06`, `handlers.c:3086` (`s.ball_height`) | `0x80247`, `0x8061B` | `0x80247` `camY > 0x70` → carrier-gate install 4 (`0x7D9A4`); `0x8061B` `camY > 0x38` → skip the target block | `mr->entities.ball.y` | `(int32_t)mr->render.camera.pos_y` |
| 3 | `fifa96_match_action_08`, `handlers.c:2977` (`s.ball_height`) | `0x81157` | `lane <= 0x50 && camY > 0x38` → `0x79B58` receiver timer (+`+0x89 > 0x3C` reset) | `mr->entities.ball.y` | `(int32_t)mr->render.camera.pos_y` |
| 4 | `match_run_outfield_input`, `src/fifa96_engine/fifa96_match_run.c:912` (`s.chase.camera`) | `0x7CCD8` | `camY < 0x30` required for the code-8 chase install (with lane `< 0x50`) | `(uint16_t)mr->entities.ball.y` | `(int32_t)mr->render.camera.pos_y`; widen `fifa96_outfield_chase_state.camera` (`include/fifa96_loader/fifa96_outfield.h:52`) from `uint16_t` to `int32_t` to keep the signed compare |

**Controls (already live, not stand-ins).** Row 04 (`handlers.c:2647`,
`(int16_t)camera.pos_y`), the kick rows 07/0F (`handlers.c:962`, inside
`match_kick_from_record`) and the kick ctx (`handlers.c:1122`) already carry
the camera Y. So the FU171-L7 list is exactly these four — no fifth site was
found: every other `ball_height`-named engine field is one of the above or the
live row-04/kick path. Note the same native cell also reaches rows 06/08
through the camera-triple copies (`0x805DF`/`0x81125`), already live as
`s.camera_y`; the re-stage therefore makes `ball_height` and `camera_y` the
same value in those two rows, as natively.

## 5. What each re-stage changes (and the tape risk)

With the camera parked at Y 0 (the phase-28 live state) **all four are
behaviour-preserving only if the ball's Y is also 0**; the stand-in's error is
exactly the nonzero-ball windows:

* **row 05** — a live `camY > 0x38` switches the stage-0 dribble direction
  from the slot dirs (speed `0x30`) to the type-table kick dirs (speed
  `0x60`). LIVE the moment a camera-Y event exists; dead while Y = 0.
* **row 06** — `camY > 0x70` forces the carrier-gate install 4; `camY > 0x38`
  suppresses the target block. These are the sharpest behaviour changes: a
  mid-band/high camera changes row-06's own output, not just a metric.
* **row 08** — `camY > 0x38` (lane ≤ `0x50`) takes the receiver-timer arm.
* **chase** — the gate is *inverted* in sign with respect to the follow band:
  `camY < 0x30` is required, so a live mid-band Y **closes** the code-8 chase
  (native: the chase only happens under a low camera). The current stand-in
  keeps it open whenever the ball is low.

**Risk:** the M2 tape may stage a nonzero `ball.y` at a gated row, in which
case the re-stage *moves the tape*. Stage-2 must run `make check` plus
`./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` and stop
for the frame diff first (spec §5 invariant; the phase-14 row-04 re-stage is
the precedent for a legitimate live ripple). This is a Stage-2 gate, not a
native leg.

## 6. The keeper `follow_speed` seam

### 6.1 The native gate, fresh bytes (`FUN_00077EAC`, entry `0x77EAC`)

```
0x78032  MOV EAX,dword[EBP+0x69]; SAR EAX,0x10
0x78038  CMP EAX,0x20; JGE 0x78065        ; lane_x < 0x20
0x7803D  CMP dword[0x157750],0x4; JGE 0x78065   ; camY < 4
0x78046  MOV EAX,[0x1577BC]               ; the dword ...
0x7804B  SAR EAX,0x10                     ; ... >>16 = word[0x1577BE]
0x7804E  CMP EAX,0xA; JGE 0x78065         ; (int16)word[0x1577BE] < 0xA
0x78053  MOV ESI,0x15774C; LEA EDI,[EBP+0x4D]; MOV ECX,1; JMP 0x781DE
```

So the third operand is the **live camera speed word** `word[0x1577BE]`
(signed), not the derived gate snapshot. The engine's own loader struct
already documents this (`include/fifa96_loader/fifa96_keeper_cpu.h:76`:
`int16_t follow_speed; /* [0x1577BC] >> 16 = word[0x1577BE] (LIVE) */`); only
the frame bind violates it.

### 6.2 The cell's writers (fresh `get_xrefs_to 0x1577BE` = 70 refs)

Load-bearing writers, fresh bytes:

| writer | body | shape |
|---|---|---|
| `0x73CB8` | `FUN_000736AC` walk tail | `word[0x1577BE] = distance(vel_x,vel_z)` (the post-tail recompute) |
| `0x71EE5` | `FUN_00071DF4` first arm | `MOV [0x1577BE],AX` after `0x71ED5 MOV EAX,[0x1577BE]; SAR 0x10` (the alias read of `word[0x1577C0]`) — the follow's own `distance(dirs × 15)` |
| `0x77275` | row-1B pan | `MOV [0x1577BE],AX` after `0xCD514` |
| `0x70204` | `FUN_000700F4` (the setter reset) | `MOV word[0x1577BE],SI` with `SI=0` (next to `0x701FD word[0x1577BC]=0`) |
| `0x70ADE` | `FUN_000709D0` pan step | word store |
| `0x706FE`, `0x70798` | `FUN_00070544` | word stores |
| `0x71990`, `0x7108B`, `0x712FC`, `0x77C1C`, `0x77D2D`, `0x852B6` | the 0x71xxx / 0x77xxx bands | word stores (*carried* attribution where outside the read windows) |
| `0x7A91F`, `0x7F58A` | `FUN_0007A490` / the row-05 camera zero | `MOV word[0x1577BE],0` |

### 6.3 The engine split (the parked phase-28 finding)

* `fifa96_camera.follow_speed` (`include/fifa96_loader/fifa96_camera.h:63-69`)
  is **the pre-tail value at the 0x739CE gate point**, captured in
  `fifa96_camera_update_walk` (`src/fifa96_loader/fifa96_camera.c:80/149`) and
  legitimately consumed by `fifa96_match_run_camera_follow`
  (`fifa96_match_run.c:3886`).
* The entity frame forwards that snapshot —
  `match_run.c:1586 f->follow_speed = mr->render.camera.follow_speed;` — and
  the keeper seam binds it to the record:
  `src/fifa96_engine/fifa96_match_entities.c:778 kc_rec.follow_speed =
  frame->follow_speed;`.
* The **live cell** is `mr->render.camera.speed`, written by the walk tail
  (`camera.c:533`) and by the follow / pan / rate arms (`camera.c:777`,
  `camera.c:1067`; the follow arm is the 0x71EE5 write, `camera.c:525-533`).
* Frame order is first-hand: the native match body calls the camera walk at
  `0x743EF CALL 0x736AC` and the entity chain right after at `0x74429 CALL
  0x8D8EC` / `0x74430 CALL 0x8D8EC` (fresh `disassemble_bytes 0x743C0..0x74410`
  and `0x74420..0x74445`; `get_xrefs_to 0x8D8EC` = 4 calls,
  `0x4B2AB/0x4B2B2/0x74429/0x74430`). The engine mirrors that order:
  walk `match_run.c:4324` → follow `:4337` → entity frame `:4367` → entities
  update `:4382`. The keeper stage select runs from the entity chain
  (`FUN_0008D8EC` → `FUN_000782D0` at `0x78599` → `FUN_00077EAC`; fresh
  `get_xrefs_to 0x77EAC` = 1 call), i.e. **after** the walk tail and after the
  follow's write. Native reads the live cell; the engine reads the value from
  before the follow.

### 6.4 The CX=1 consequence

`camera.follow_speed` is the value *at the follow gate*, which is **before**
the follow arm and the tail recompute that write the live cell. Two concrete
divergences:

1. **Over-admission (the reachable one).** The row-1B pan writes the cell
   (`0x77275`, the FU305 live run's 5-unit pan) and leaves the pan counter
   untouched — the counter increments only inside `FUN_000709D0`
   (`camera.c:968`, the 0x70A65 step). The next frame's walk captures
   `follow_speed = 5`; the follow gate passes (`follow_speed != 0`,
   `pan_counter == 0`) and the follow arm writes
   `speed = |dirs × 15|` (`0xF` or `0x15` for a nonzero dir pair). Native CX=1
   then sees `>= 0xA` and **closes**; the engine's snapshot is still `5 < 0xA`
   and **opens**. This is exactly the FU305 §CX window ("the pan's 5-unit
   velocity keeps follow_speed < 0xA") — the snapshot is right at the 0x739CE
   gate and wrong at the keeper gate.
2. **Under-admission.** Every setter call clears the cell
   (`0x70204 word[0x1577BE]=0`; the engine `fifa96_camera_event_set` mirrors it
   at `camera.c:919 cam->speed = 0`) but does **not** touch
   `camera.follow_speed`. A setter that runs in the entity chain before a
   later keeper step leaves native at 0 (CX=1 open) and the engine at the
   previous snapshot (possibly `>= 0xA`, closed). The same-frame reach of that
   window depends on the entity-chain order and is not asserted (leg FU308-L4);
   the fix reads the cell, so it is order-agnostic.

### 6.5 The port shape

One line, no new state:

```c
/* src/fifa96_engine/fifa96_match_run.c:1586 — the native word 0x1577BE */
f->follow_speed = (int16_t)mr->render.camera.speed;   /* was camera.follow_speed */
```

* `fifa96_match_entities.c:778` and `fifa96_keeper_cpu.c:182` are unchanged
  (the bind and the `< 0xA` compare are already correct; the loader header
  already says LIVE).
* **This is the same correction the phase-28 final fix wave already made for
  row `0x1A`**: `src/fifa96_engine/fifa96_match_handlers.c:4471-4473` reads
  `int16_t follow = (int16_t)mr->render.camera.speed; /* word[0x1577BE] */`
  with the comment "`follow_speed` is only the derived 0x739CE snapshot, not
  word[0x1577BE]". The keeper bind at `match_run.c:1586` is the last surviving
  consumer of the wrong slice; no new pattern is introduced.
* `fifa96_camera.follow_speed` stays — its only remaining consumer is the
  0x739CE gate in `fifa96_match_run_camera_follow`; correct the frame-field
  comment (`include/fifa96_engine/fifa96_match_entities.h:328-333`, which
  already describes `0x1577BC >> 16`) rather than renaming public fields. If a
  rename is wanted, `fifa96_match_entities_frame.follow_speed` and
  `fifa96_keeper_cpu_record.follow_speed` would become `cam_speed`; that is
  cosmetic and not required for fidelity.
* No signedness change: `camera.speed` is `uint16_t`, the native read is
  `(int16_t)` of the same bits, and `(int16_t)` matches `gate_speed`'s cast.

### 6.6 The test seam

**Unit pin (loader, `tests/test_keeper_cpu.c`).** In `test_keeper_cpu_stage_select`
(or a new case) with the existing `kc_rec_zero()` + `cam_z = 0x800` +
`global_157a4c = 1` CX=1 opener: `rec.follow_speed = 9` → `stage == 1`;
`rec.follow_speed = 0xA` → `stage == 0` (the table path closes early: with
`band_fa == band_f6 == 0` the `pan_counter == 0` band gate `0x78144..0x78175`
returns; on the live frame path the `0x157824` side gate with the seam's
`0xFF` closes it first). Pins the threshold, not the bind.

**Granted-frame pin (the real seam).** Extend `test_keeper_installer_live_reach`
Part B (`tests/test_engine_match_frame.c:3722`) or add a sibling: same fixture,
but before `one_granted_frame` stage `mr.render.camera.vel_x = 0x0F`. The walk
computes `speed = distance(0xF,0) = 0xF` at its tail (`camera.c:533`) while
`gate_speed` was captured as 0, so `camera.follow_speed == 0`,
`camera.speed == 0xF` — the live cell and the snapshot split with no other
side effect (no integration: the entry `speed` was 0; the follow returns at
gate 2 because `follow_speed == 0`). Assertions:

* `k1->code == 0x19` and `(mr.dispatched_ok & (1ull << 0x1A)) == 0`;
* `mr.global_157c2c != 0x2C` (no G2 stamp);
* the stage selector's live view is `stage 0` (observable through the absent
  install; with all latch cells zero the table path returns at the
  `0x157824` side gate).

**RED bar:** on the phase-28 baseline (`f->follow_speed = camera.follow_speed`
= 0) the record takes CX=1, installs row `0x1A`, stamps `0x2C` and dispatches
it — the assertions fail. Mutation-RED rider: invert the bind back and the
test fails again. This mirrors the phase-28 row-`0x1A` precedent (the same
class of "derived snapshot instead of the `0x1577BE` cell" error, fixed in
`docs/ENGINE.md` "Final fix wave" (i)).

## 7. Evidence table

| claim | evidence (fresh unless marked) |
|---|---|
| program is `/FIFA96.EXE`, LE 32-bit, image base 0, `.image` overlay, 2708 functions | `get_current_program_info` |
| `[0x157750]` = camera triple Y; 146 refs | `get_xrefs_to 0x157750`; `0x805DE..0x805E0`, `0x81125` MOVSD copies |
| row-05 `0x7F2C8` read + `0x7F2E6` `> 0x38` gate | `disassemble_bytes 0x7F2B0..0x7F2D0`, `0x7F2C0..0x7F300` |
| row-06 `0x80247` (`> 0x70`, install 4) | `disassemble_bytes 0x8022D..0x80270` |
| row-06 `0x8061B` (`> 0x38`, skip target) | `disassemble_bytes 0x80610..0x80630` |
| row-08 `0x8114C`/`0x81157` (`lane > 0x50 \|\| camY > 0x38`) | `disassemble_bytes 0x81120..0x81170` |
| chase `0x7CCCD`/`0x7CCD8` (`lane < 0x50`, `camY < 0x30`) | `disassemble_bytes 0x7CCB0..0x7CD00` |
| engine stand-ins at `handlers.c:2813/2977/3086`, `match_run.c:912`; live controls `handlers.c:962/1122/2647` | source reads |
| loader consumers `action_handlers.c:424/1989/2094`, `outfield.c:165/768` | source reads |
| CX=1 native trio `0x78032..0x78060`, speed = `word[0x1577BE]` | `disassemble_bytes 0x78020..0x78070` |
| `0x1577BE` writer set incl. `0x71EE5`, `0x77275`, `0x70204` | `get_xrefs_to 0x1577BE` (70); `0x71ED0..0x71EF0`; `0x77220..0x77290`; `0x701F0..0x70210` |
| walk → entity-chain frame order | `0x743C0..0x74410`, `0x74420..0x74445`; `get_xrefs_to 0x8D8EC` (4) |
| keeper stage select is one call under `FUN_000782D0` | `get_xrefs_to 0x77EAC` (1, `0x78599`); `get_function_by_address 0x77EAC` = `0x77EAC..0x782CD` |
| engine bind chain + order | `match_run.c:1586/3886/4324/4337/4367/4382`, `match_entities.c:778`, `keeper_cpu.c:182`, `keeper_cpu.h:76`, `camera.c:80/149/525-533/533/777/919/967-968/1067`, `camera.h:63-69` |
| the same fix already exists for row `0x1A` | `match_handlers.c:4471-4473` (`camera.speed` + the "only the derived 0x739CE snapshot" comment) |
| native-reset speed clear is modelled | `0x70204` + `camera.c:919` |

## 8. Numbered legs

* **FU308-L1 — the row-05 second `[0x157750]` read has no engine consumer.**
  The same handler's stage-0 tail reads `0x7F4D0 CMP dword[0x157750],0x10;
  SETG AL` → `0x7F4E2 MOVSX ECX,AX` and calls the setter at `0x7F4E7` (height
  arg 0). That setter caller (FU171-L6) is unported, so the port cannot
  re-stage it yet; when it lands it must read `camera.pos_y` (the same live
  cell), never `ball.y`. Owner: FU171-L6 / the row-05 stage-0 tail.
* **FU308-L2 — the chase gate's signed compares.** The native gate is signed
  32-bit (`0x7CCD3 CMP EDX,0x50; JGE`, `0x7CCD8 CMP dword[0x157750],0x30;
  JGE`), the loader state is `uint16_t` for both `distance` and `camera`
  (`include/fifa96_loader/fifa96_outfield.h:51-52`) and compares unsigned
  (`src/fifa96_loader/fifa96_outfield.c:165`). Only divergent for negative
  inputs. The re-stage should widen `camera` to `int32_t`; `distance` stays
  `uint16_t` (same latent gap). Owner: `FUN_0007CA54` `0x7CCCD/0x7CCD8`.
* **FU308-L3 — negative camera-Y reachability.** L2 is observable only if a
  ported route can produce `camY < 0`. `fifa96_camera_height` returns 0 at and
  after `timer_limit` (`camera.c:711-717`) but the arc's interior sign is not
  proved, and the sequence-band cuts (`0x7456E` `rec+0x5D + 0x38`, `0x75D69`
  `0x50`) are outside this slice's windows. Owner: the `[0x157750]` writer set
  (FU168-L5 / FU264-L1).
* **FU308-L4 — the same-frame pre-keeper `0x1577BE` writers.** The entity-chain
  order between a mid-chain writer (the setter reset `0x70204`, the row-1B pan
  `0x77275`, the `FUN_0007A490`/row-05 camera zero `0x7A91F`/`0x7F58A`) and a
  later record's keeper step is not derived. The §6.5 bind reads the live cell
  and is therefore order-agnostic, but no test stages that window; the RED
  witness uses the walk-tail velocity (§6.6). Owner: the `FUN_0008D8EC`
  per-record order (FU-141).
* **FU308-L5 — the int16 truncation in the already-live rows.** The row-04 and
  kick re-stages store `(int16_t)camera.pos_y` (`handlers.c:962/2647`) while
  the native cell is a dword. Faithful for the bounded event heights
  (`<= 0x640`, the setter clamp) but not proved for every `0x157750` writer.
  Stage-2 may widen those fields to `int32_t`; today no ported route exceeds
  `0x7FFF`. Owner: FU171-L3 (`handlers.c:2647`).

## 9. Verdict

* **Stand-ins — GO.** Four mechanical re-stages, each with a fresh native
  read and a one-line source change; the discriminating tests are per-handler
  unit pins (camera Y and ball Y set to different values, asserting the native
  gate branch). The only caveat is the M2 comparison (§5), which is a Stage-2
  gate, not a blocker.
* **Keeper seam — GO.** One-line bind fix at `match_run.c:1586` with an exact
  granted-frame RED witness; no loader, header or test-contract change; the
  parked phase-28 finding closes with a live-cell read.
* **Both are independent** of the FU306/FU307 chaser/Y-producer verdicts: the
  stand-ins are no-ops at `camY == 0` and the seam witness stages only
  `camera.vel_x`. If FU307's Y producer lands, the stand-ins become live in
  the intended direction; if not, they remain correct and dead (and the tape
  check still applies).

---

## Review erratum (wave review, 2026-10-21)

*Appended section: the frozen text above stands; the correction quotes it
first. Read-only review round; no file outside this draft was touched.*

**Verdict:** GO — the four stand-in re-stages and the one-line keeper seam stand
unchanged; the seam now carries the **M2-golden gate** as a hard Stage-2 stop.

1. **The M2 gate binds the seam fix too, not only the stand-ins.** Quote (§5):
   "Stage-2 must run `make check` plus
   `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` and stop
   for the frame diff first". **Extended:** the same gate is binding for the
   §6.5 bind (`match_run.c:1586`, `follow_speed = camera.speed`). The seam
   changes the **CX=1 input** the keeper ladder reads (`0x7804E CMP EAX,0xA`
   over `word[0x1577BE]`), so it can move a keeper action on the tape even
   when no stand-in site is touched. Stage-2 must:

   * run the exact command
     `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` on the
     tree **before** the bind and again **after** it, so any diff is
     attributable to the one-line change;
   * **STOP on a diff** and report it — no re-pin without a written reason and
     a frame diff (the AGENTS.md m2 rule; M1 is immovable and untouched here);
   * keep `make check` green in the same commit.

   The stand-in re-stages (§4) already carry this gate in §5; this erratum
   closes the seam's silence on it — §9's "only caveat is the M2 comparison
   (§5)" must be read as covering both bullets, stand-ins **and** seam.
