# FU-140: M2 action cluster C — keeper rows (codes 0x19–0x1F)

Task 7 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`).
The spec's cluster C is "Keeper — keeper bodies/dispatch, hold/guard/intercept/
claim" (design §3.3, plan Task 7). This slice re-verifies the seven
keeper-classified action rows read-only in the authoritative program, derives
the remaining pure parts of their record-visible bodies (the keeper input
handler `FUN_00076130`, the row-`0x19` weighted camera fallback including the
`FUN_00074694` projection, and row `0x1F`'s prologue camera target), ports them
into `fifa96_keeper` with tests, and wires the one fully linear keeper row —
`0x1E` claim/throw — through the FU-137 dispatch seam with the minimal engine
record (the FU-138 OL-16 stopgap).

Result in one line: **all seven cluster-C rows re-verify against the
`0x1106E0` table dump (`19`=`0746E4`, `1A`=`07662C`, `1B`=`076D28`,
`1C`=`077728`, `1D`=`074EB0`, `1E`=`07550C`, `1F`=`076380`); row `1E` is now
ported and wired (`fifa96_match_action_1E` runs the tested
`fifa96_keeper_claim_place` over `mr->record`, records the `FUN_0007876C`
slot-merge request and the `FUN_000700F4` placement triple, and sets the
possession/actor flags); rows `19`/`1A`/`1B`/`1C`/`1D`/`1F` gain three derived,
tested pure helpers (`keeper_input_decide` for `FUN_00076130`,
`keeper_hold_fallback` for the `0x74B72..0x74C8D` weighted camera fallback,
`keeper_arm_camera` for the `0x763B1..0x76409` prologue) but stay
`-FIFA96_ERR_UNSUPPORTED` because their arm bodies and the 0xB2 record pool are
unported (OL-33..OL-37, carrying OL-16; the controller's C6 ruling keeps the
evidence-gated discipline while the pool/arms remain the wiring critical
path).**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit in every call). Ghidra
  **read-only**: no renames, comments, labels, function creation, scripts or
  project saves. `/fifa96_le.bin` and the MZ loader were not used.
* Tool calls made this slice:
  * `read_memory /FIFA96.EXE 0x1106E0` (180 B) — the full action table,
    matching FU-136 §1.1/FU-137 §1.1/FU-138 §1/FU-139 §1 dword for dword;
  * `disassemble_bytes` — the seven handler windows and support bodies:
    `0x746E4..0x7475F` (`19` prologue), `0x7478C..0x7483F` (`19` guard +
    predictor head), `0x74ACC..0x74C8F` (`19` carrier dive + weighted
    fallback), `0x74CDC..0x74D84` (guard clamp), `0x74694..0x746E3`
    (`FUN_00074694`), `0x8DC50..0x8DC68` (`FUN_0008DC50` trunc shift),
    `0x74EB0..0x74F70` (`1D` prologue + stage 0 head), `0x7550C..0x755D3`
    (`1E` full), `0x76130..0x7636C` (`FUN_00076130` full),
    `0x76380..0x7643F` (`1F` prologue + stage 0), `0x7643A..0x7647E`
    (`1F` stage 0 tail), `0x7647E..0x765C2` (`1F` stages 1-3 + tail),
    `0x7662C..0x766FF` (`1A` prologue + stage 0), `0x776C1..0x77727`
    (`1B` stage 4), `0x77728..0x777A0` (`1C` head/stage dispatch);
  * `disassemble_bytes 0x7876C..0x787C0` (`1E`'s slot-merge helper head) and
    `0x700F4..0x7013F` (`1E`'s camera-place helper head).
* **Address mapping.** In `/FIFA96.EXE` the loader's object-4 fixups are
  already applied, so data immediates render at their resolved flat addresses
  (`[0x0015774C]` = the camera/place triple, matching FU-137/FU-138/FU-139's
  native convention; FU-74/FU-79 wrote the pre-loader `0x5774C` form). The
  action table at `0x1106E0` is read directly; code addresses are identical to
  `/fifa96_le.bin` (FU-130 §6). `FIFA96.EXE` defines no functions at the table
  targets, so every row citation is `disassemble_bytes` output.
* Baseline at task start: `make check` = **98/98** (FU-139 commit `9392909`);
  after this task **98/98** with the extended `test_keeper` /
  `test_keeper_bodies` / `test_engine_match_handlers` binaries (no new test
  binary).
* Evidence chain: FU-136 §2 (row inventory/tasks), FU-137 §6.1 (row classes),
  FU-74 (keeper machine/selection), FU-79 (all seven bodies, port table). This
  doc re-verifies the slices it ports and corrects four FU-79 claims (see
  Errata).

## 1. Table re-verification (`0x1106E0[0x19..0x1F]`)

`read_memory 0x1106E0` (180 B = 45 dwords) returns for the keeper slots:

```
0x19 0x0746E4  0x1A 0x07662C  0x1B 0x076D28  0x1C 0x077728
0x1D 0x074EB0  0x1E 0x07550C  0x1F 0x076380
```

exactly FU-136 §1.1/FU-137 §1.1/FU-79 §1.1. The sole reader remains
`FUN_0007D9A4 @ 0x7DA77` (`EAX = code << 2; ADD EAX,0x1106E0`), and the call
contract is `EAX = record` only (FU-137 §2).

## 2. Cluster-C per-row derivation

Windows marked ✓ were re-read first-hand this slice; the body column is the
strongest of the fresh read and the named FU-79 derivation. Field numbers are
the native record offsets.

| code | handler | gate / entry (verified) | timer | stage / arms | record-visible body | class / open leg |
|---|---|---|---|---|---|---|
| 19 | `0746E4` | ✓ `[+0x9E]=1`, `[0x157C5D]=1`; deepest-keeper tracker `team[+0x7C7]` by signed `+0x6B` compare (`0x746F9..0x7472B`); `[+0x98]!=0` → copy pos→target and tail | — | — | ✓ guard (`0x7479D..0x74831`): `[0x157A83]==0`, `[rec+0x69]>>16 < 0x2D0`, `[0x157750]==0`, `team[+0x7C7]==rec`, opponent null or `rec[+0x6B]>=opp[+0x6B]` → camera triple → target, z clamped `±0xAE0` by `team[+0x826]`, `FUN_00079B58(rec)`; ✓ carrier dive (`0x74ACC..0x74B71`) `out.x = cam.x + (rec[+0x69]>>20)*carrier[+0x71]>>16`, same for z, y=0; ✓ weighted fallback (`0x74B72..0x74C8D`) ported (FU-140 §3.2); predictor `0x74832..0x74ACC` block-level only; no-slot `phase!=2` → `CALL [rec+0x1C]` (`0x74C98`) | not ported (partial): hold_track/guard/intercept + guard_clamp + FU-140 `hold_fallback` tested; predictor/guard arms OL-33 |
| 1A | `07662C` | ✓ `phase==0xD && team[+0x826]==[0x157AAC]>>24` → `[0x157A83]=rec` (`0x76634..0x7665C`); ✓ `+0x89 += delta` (`0x76662..0x76678`); ✓ stage `>4` → tail | ✓ dword add of `[0x157A64]` | `0x76618` 5 arms | ✓ stage 0 (`0x76693`): `FUN_000765C4(rec)` non-zero → `FUN_0007DAB4` + install `4` invoke-now; else `[+0x9E]=1`, `+0x92 += 2`, `+0x89=0`; stages 1/2 `FUN_00079B1C` + `[0x157C2C]` direction machine + common movement/event block (FU-79 §3.1/§3.2) | not ported (partial): `reposition_a_gate` tested; stage bodies OL-34 |
| 1B | `076D28` | phase `0xD` side match → camera place from `0xF334/0xF33C[type8]`, `[rec+0x9B]=1`, `[0x157A83]=rec` (FU-79 §4) | `+0x89 += delta` | `0x76D14` 5 arms | ✓ stage 4 (`0x776C1`): `[+0x92]=4`, `FUN_0007DAB4`, copy pos→target; `rec != [0x157A83]` → install `0x19`; else human side returns, CPU side `FUN_0008A938(5, side, 1)`; stage 2 rush test `0x76FC7..0x77636` (FU-79 §4.1) | not ported (partial): `reposition_b_finish` tested; rush/stage bodies OL-34 |
| 1C | `077728` | ✓ stage dispatch `0x7774F` (`0 <1 -> 0`, `==1 -> 1`, `==2 -> 2`, else tail) | ✓ `+0x89 += delta` (`0x77735..0x77749`) | inline 0/1/2 | ✓ stage 0 (`0x7776A`): `[+0x9E]=1`, `[0x157C56]=0`, `+0x92++`, `+0x89=0`; ✓ stage 1 (`0x7778E`): `FUN_0008DCD4(pos, target, vec+0x65)` then on-target arm `fifa96_keeper_lunge_track` (0x55 threshold, `0x3C` event, steer `0x80/0x40`); rotated arm `0x77856..` requires the `0x14E04` sine table; tail `0x779DE..0x77E8B` camera velocity/RNG/FUN_0006E444 timer (FU-79 §5) | not ported (partial): `lunge_track` tested; rotated arm/steering OL-35 |
| 1D | `074EB0` | ✓ prologue builds the camera target from `[0x744D0]` + `[0x157C5E] + side*0x18 + (([0x15777C] >= 0) ? 0xC : 0)` (`0x74EC3..0x74F10`); ✓ `stage < 3` → `[0x157A83]=rec` + `FUN_0007876C` (`0x74F1F..0x74F27`) | ✓ `+0x89 += delta` (`0x74F2C..0x74F45`) | `0x74E9C` 5 arms | stage 0 (`0x74F60`): `+0x44` anim bit + timer gates → camera place/event `0x26`/`0x4C324`/`[0x57AB2]=1`; stage 1 slot-button/`0x4B0` timer; stage 2 copy camera → target, `+0x69>>16 > 0x40` arms timer; stage 3 `[0x58743]=1`, clear vector or slot delta, `FUN_0007A490` ball staging event `0x30/0x31`; stage 4 reset (FU-79 §6) | not ported (partial): `clear_vector` tested; stage bodies OL-35 |
| 1E | `07550C` | ✓ full body (59 insns, `0x7550C..0x755D3`): `stage = [+0x8F]>>24`; `stage < 6 && [+0x20]==0` → `CALL 0x7876C` (`0x75536`); `stage >= 3` → return; `byte [+0x9B] != 0` → return; `type8 = [+0x8B]>>24`; `x = [+0x59] + (int8)0x10F334[type8]<<4 → 0x15774C`; `z = [+0x61] + (int8)0x10F33C[type8]<<4 → 0x157754`; `y = [+0x5D] + 0x38 → 0x157750`; `FUN_000700F4(rec, place, 1)`; `[+0x9B]=1`; `[0x157A83]=rec` | — | — | **ported + wired (FU-140 §4)**; slot-merge/camera/actor arms modelled as requests (OL-37) |
| 1F | `076380` | ✓ `[+0x20]==0 && team[+0x828]!=0` → `FUN_0007876C` (`0x76388..0x7639B`); ✓ stage `= [+0x8F]>>24`; `[0x157C5C]=0`; ✓ `stage < 2` → `x=0, y=0, z=(team[+0x826]!=0 || phase==0x10) ? 0xB10 : -0xB10`; `stage < 1` → `FUN_00079B6C(rec, 0x5774C, target)` (`0x763B1..0x76404`) | ✓ `+0x89 += delta` (`0x76409..0x7641F`) | `0x76370` 4 arms, `stage > 3` tail | ✓ stage 0 (`0x7643A`): `[+0x9E]=1`; `[0x15882A]==0` → return; event `0x28` via `FUN_0006E598(rec, 0x28, type8, 0)`, `+0x89=0`, `+0x92++`; stage 1 ball-ack gate `[0x158730]!=0 && byte[0x158746]!=0`; stage 2 slot branch: slot → stage 3 tail (`0x79B1C`, input handler when `word[slot+4]!=0`, `type8==0x1F && [0x157A4C]==0` → `FUN_0007DAB4`), no slot → RNG dive `fifa96_keeper_dive_target` install `0x1B` invoke-now (`0x764B0..0x76567`) | not ported (partial): `dive_target`/`arm_step` + FU-140 `arm_camera`/`input_decide` tested; RNG/install execution OL-36 |

Cross-family state shared by the table: `+0x89` action timer, `+0x92` stage
byte, `+0x8F` stage byte (row 1E/1F read-back), `+0x91` current code, `+0x8D`
active flag, `+0x9B` ball/possession flag, `+0x9E` "ran" byte, `+0x44`
animation-row terminal bit, `+0x20` control slot, `+0x69` lane/dz dword,
`[0x157A4A]>>24` phase, `[0x157A49]>>24` human-side phase, `[0x157A64]` frame
delta, `[0x157A83]` actor/controlled record, `[0x15774C/50/54]` camera/place
triple, `[0x157C5D]` CPU-keeper flag, `[0x157C5E]` per-side table, `[0x158730]`
ball actor, `[0x15882A]` cluster-C event flag, `0x110680[type8]` type gate.

## 3. Support-item derivations and the ported pure functions

### 3.1 Keeper input handler `FUN_00076130` (`0x76130..0x7636C`)

✓ read in full this slice (190 instructions). Inputs `EAX = rec`; the return
value is `([rec+0x91] != saved)` (the sole caller `0x76599` ignores it). Body:

* `0x7613B`: `[rec+0x20]==0` → return 0.
* `0x76148`: `AX = 1`; `phase = [0x157A4A]>>24`.
  * `phase != 2` (`0x761A5`): continue only if `[0x157A49]>>24 == 1 &&
    byte[0x15882A] != 0`; else return 0.
  * `phase == 2` (`0x7615B`): `|[0x15774C]| > 0x420` → `AX = 0`; else
    `AX = (team[+0x826] != 0) ? ([0x157754] >= 0x7B0) : ([0x157754] <= -0x7B0)`
    (the `JGE`/`JLE` at `0x7618F`/`0x7619F` jump to the type gate with `AX`
    still 1; the `XOR EAX,EAX` fall-throughs zero it).
* `0x761C8` type gate: `type8 = [rec+0x8E]>>24`; if `type8 != 0x1F` and
  `(byte[0x110680+type8] & 1) == 0` → return 0.
* `0x761F2`: `rec == [0x157A83]` or `type8 == 5` → return 0.
* `saved = (int8)[rec+0x91]`; `AX == 0` (`0x76224`): `[rec+0x69]>>16 > 0x60` →
  tail; else install `4` invoke-now.
* `AX != 0` and `[rec+0x69]>>16 <= 0x40` (`0x76246`): copy the `0x15774C`
  triple → record target; install `0x19`.
* else slot direction (`0x7626F`): `dx = (int8)slot[+0x20]`,
  `dz = (int8)slot[+0x21]`; both zero → copy pos → target, `target.y = 0xA0`,
  install `0x1C`; else `target = pos + dir*0x70`, `target.y = 0x40`, z clamp
  `±0xAF0`, `[0x157C5C]=0`, `FUN_0008DCD4(pos, target, rec+0x65)`, install
  `0x1B + (distance >= 0x70)`.
* `0x76350`: return `[rec+0x91] != saved`.

Ported as `fifa96_keeper_input_decide` (decision outputs: install/invoke,
copy flags, `[0x157C5C]` clear, target triple). The install execution
(`FUN_0007D9A4`), the `0x15882A` flag writers and `FUN_00079B6C` stay unported:
**OL-36**.

### 3.2 Row 19 weighted camera fallback (`0x74B72..0x74C8D`)

✓ read in full this slice, plus `FUN_00074694` (`0x74694..0x746E2`):

* base (the fallback's **z** coordinate): `base = (side == 0) ? -0x9F0 :
  0x9F0` (`0x74B78..0x74B95`, side = `team[+0x826]`); `base +=
  FUN_0008DC50((int16)([0x157752]>>16), 4)` (`0x74B99..0x74BB0`, a truncating
  shift toward zero); clamp `±0xAE0` (`0x74BBA..0x74BCC`).
* x gate (`0x74BD0..0x74BFE`): `lane = [rec+0x69]>>16`; `lane >= 0x780` or
  `|[0x157778]| <= 0x9F0` or `word[0x1577C2] == 0` → weighted expression; else
  `FUN_00074694`.
* weighted expression (`0x74C3A..0x74C7A`): `w = (int16)word[0x15774C]`;
  `w >= 0` → `x = (w>>3)+(w>>4)+(w>>5)`; negative → negate the 16-bit word,
  sum the shifts and negate the result (`NEG EBX`/`NEG EDI`). 16-bit wrap
  details: `w = -0x8000` yields `+0x1C00`.
* `FUN_00074694(base)` (`0x74694..0x746E2`): `num = |base| - |[0x157754]|`;
  `den = (([0x1577C0]>>16)<<4)`; `quot = num / |den|` (signed `IDIV`, so
  denominator 0 faults); `lead = (([0x1577BE]>>16)<<4)`; returns
  `[0x15774C] + lead * quot`; the caller clamps x to `±0xC0` (`0x74C0D..0x74C2D`).
* the local triple is `{x, 0, z}` (`[ESP+4]=0` at `0x74B74`) and is copied to
  the record target (`0x74C7C..0x74C83`).

Ported as `fifa96_keeper_hold_fallback(cam, side, cam_vel_z, lane, dir,
dir_word, vel_x, lead_x, &out)` (the raw camera dwords are arguments because
the native shifts the full 32-bit values; `den == 0` returns `-INVALID` where
the native `IDIV` faults). The `0x19` predictor block (`0x74832..0x74ACC`,
`FUN_00071C40`/`FUN_00071B9C`, the `0x74584`/`0x745EC` predicates) stays
unported: **OL-33**.

### 3.3 Row 1F prologue camera target (`0x763B1..0x76404`)

✓ read this slice. `stage >= 2` leaves the target untouched; otherwise
`target.x = 0`, `target.y = 0`, `target.z = (team[+0x826] != 0 || phase ==
0x10) ? 0xB10 : -0xB10` (`0x763B6..0x763E9`; the `JNZ` at `0x763CE` takes the
positive arm when the team-side byte is nonzero, the `phase==0x10` compare at
`0x763D8` is the second positive condition). `stage < 1` additionally arms the
`FUN_00079B6C(rec, 0x5774C, target)` camera hook (`0x763F5..0x76404`). Ported
as `fifa96_keeper_arm_camera`; the hook body stays unported (**OL-36**).

### 3.4 Ported pure functions

All caller-owned, table-free where the native reads a table through an
argument, negative error on NULL or on the natively-faulting denominator. Byte
citations are §3's windows.

| function | native site | semantics |
|---|---|---|
| `fifa96_keeper_hold_fallback(...)` | row 19 `0x74B72..0x74C8D` + `FUN_00074694` | z = `±0x9F0 + trunc_shift(cam_vel_z_word, 4)` clamped `±0xAE0`; x = weighted `(|cam.x|>>3 + >>4 + >>5)` or the `FUN_00074694` projection clamped `±0xC0`; y = 0 |
| `fifa96_keeper_arm_camera(stage, team_side, phase, &target, &hook)` | row 1F `0x763B1..0x76404` | `stage < 2` → target `(0, 0, ±0xB10)`; `stage < 1` → camera hook |
| `fifa96_keeper_input_decide(&in, &out)` | `FUN_00076130 0x76130..0x7636C` | full gate/box/type/actor decision tree → install/invoke, copy flags, target triple |

Existing tested helpers reused with the fresh derivation: `hold_track`,
`hold_guard`, `hold_intercept`, `guard_clamp`, `claim_place` (row 19 and 1E),
`clear_vector` (row 1D), `dive_target`/`arm_step` (row 1F),
`reposition_a_gate`/`reposition_b_finish`, `lunge_track` (row 1C). All thirteen
were re-verified against the FU-79 byte windows; no behavior change was needed.

### 3.5 Tests

* `tests/test_keeper_bodies.c`: new `hold_fallback` fixtures (both side seeds,
  trunc-shift positive/negative, both clamps, weighted positive/negative/16-bit
  wrap, `FUN_00074694` quotient/lead, `±0xC0` clamps, denominator-0 `INVALID`,
  NULLs); `arm_camera` (stage 0/1/2, both z signs, `phase 0x10`, target
  untouched at stage >= 2, NULLs); `input_decide` (slot gate, box edges
  `±0x420`/`±0x7B0` by side, `lane` `0x40`/`0x60`/`0x61` rows, phase/human/
  event gate, type gate incl. the `0x1F` bypass, actor/type-5 rejects,
  camera-copy/pos-copy/slot-direction branches incl. `0xAF0` clamp and the
  `0x1B`/`0x1C` distance split, NULLs); `_Static_assert` layout pins for both
  new structs.
* `tests/test_keeper.c`: `test_keeper_forced_rows` re-pins the FU-74/FU-140
  escalation rows (`0`/`0x19`/`4`/keep-current) and the type gate over the
  keeper codes.
* `tests/test_engine_match_handlers.c`: `test_action_1E_runs_claim_place` pins
  the wired row's outputs (helper request, placement, possession, actor flag,
  stage/has-ball/has-slot gates).

## 4. Engine wiring (row 1E)

The FU-137 seam's handler typedef takes only the run (`fn(mr)`), because the
native handler receives only the record (FU-137 §2). Row `1E` is the family's
only branch-linear body (FU-79 §7) and needs no other entity, so it binds to
the FU-138 minimal record with the native fields it reads:

* `fifa96_match_run.h` `struct fifa96_match_run_record` gains `pos_y` (+0x5D),
  `stage` (+0x8F), `has_ball` (+0x9B), the derived `place_x/y/z` sink for the
  native `0x15774C/50/54` triple (what `FUN_000700F4` consumes), the
  caller-supplied `place_offset_x/z` (the extracted object-4 bytes
  `0x10F334`/`0x10F33C[type8]`), and the requested `helper_request`
  (`FUN_0007876C`) / `controlled` (`[0x157A83] = rec`) flags — all write-only
  until the C8/C11 pool consumes them, exactly the FU-138 `ran`/`install`
  pattern.
* `fifa96_match_action_1E` (in `fifa96_match_handlers.c`) unpacks the record
  into `fifa96_keeper_point`, runs the tested `fifa96_keeper_claim_place`,
  always records `helper_request`, and on `claimed` records the place triple
  and sets `has_ball`/`controlled`. The native `+0x9E` is not written by the
  body, so `ran` stays untouched.
* `fifa96_match_action_table[0x1E] = {0x1E, fifa96_match_action_1E, ...}` —
  the FU-137 §6.1 row 1E class moves `unwired -> ported` (FU-137 errata), and
  `fifa96_engine` links `fifa96_keeper`.

Not wired / not modelled: the `FUN_0007876C` slot-merge body (its call is the
`helper_request`), the `FUN_000700F4` camera-place body (its input is the
`place_*` sink), the `[0x157A83]` actor pointer (the `controlled` flag), and
the 0xB2 record pool (OL-16, carried as OL-37).

## 5. Totals

| group | rows | state |
|---|---|---|
| ported + wired (cluster C) | `1E` | `FIFA96_OK` through `fifa96_match_dispatch_action` |
| pure parts advanced, wiring open | `19`, `1F` | still `-FIFA96_ERR_UNSUPPORTED`; three new tested helpers |
| documented, unported | `1A`, `1B`, `1C`, `1D` | `-FIFA96_ERR_UNSUPPORTED` with the leg named in §2/§6 |

The FU-137 action table now dispatches `2 × OK + 43 × UNSUPPORTED` (row `00`
from FU-138, row `1E` here); the phase table is unchanged
(`34 × UNSUPPORTED + 1 × NOT_FOUND`). Overall: **2 × FIFA96_OK, 77 ×
-FIFA96_ERR_UNSUPPORTED, 1 × -FIFA96_ERR_NOT_FOUND**.

## 6. Open legs

* **OL-33 — row 19 arms.** The prediction block `0x74832..0x74ACC` (camera
  angle projector `FUN_00071C40`/`FUN_00071B9C`, the `0x57C44/0x57C48` latches,
  the `0x74584`/`0x745EC` predicates, `FUN_00079B58` timer arm), the no-slot
  `phase != 2` `CALL [rec+0x1C]` method, the `FUN_00079C20` slot-move arm and
  the carrier-dive predicates are unported (FU-79 §2.5/§2.6, §14 legs 1-3).
* **OL-34 — rows 1A/1B stage bodies.** `FUN_000765C4` tracked-teammate
  predicate, the `[0x157C2C]` direction machine (`0x766FC..0x767E7`), the common
  movement/event block (`0x76816..0x76AA7`), the 1B stage-2 rush test
  (`0x76FC7..0x77636`, per-side `[0x4C2F6]` thresholds) and the stage 3/4
  finish side effects are unported (FU-79 §3/§4, §14 leg 4).
* **OL-35 — rows 1C/1D stage bodies.** The `0x14E04` sine-table rotation and
  `FUN_000795A4`/`FUN_00076B28` steering (1C rotated arm), the 1D per-side
  camera-target tables (`[0x744D0]`, `[0x157C5E]`), the slot-button/UIT blocks,
  `FUN_0007A490` ball staging event `0x30/0x31`, `FUN_00092820` and
  `FUN_0008A938` are unported (FU-79 §5/§6, §14 legs 5-6).
* **OL-36 — row 1F execution.** The `FUN_00076130` install execution
  (`FUN_0007D9A4`), the stage-2 RNG dive (`FUN_00092AC8`, `FUN_0006E444`
  timer), the `FUN_00079B6C` camera hook and the `FUN_0006E598` stage-0 event
  are unported; the decision bodies are tested (FU-79 §8, §14 leg 8).
* **OL-37 — row 1E arms (carries OL-16).** `FUN_0007876C` (slot merge,
  `0x7876C..`, writes the control slot from the team's `+0x828` pool),
  `FUN_000700F4` (`0x700F4..`, camera place from the `0x15774C` triple) and
  the `[0x157A83]` actor binding are modelled as record requests/flags; the
  engine's 0xB2 record pool, team blocks and installer remain the C8/C11
  replacement (carries FU-137 OL-1 / FU-139 OL-16).
* The keeper machine itself (`FUN_000782D0` / `FUN_0008D8EC` walk, the
  `0x110950..0x1109E0` input-row tables and `FUN_00077EAC` CPU decision) stays
  FU-137 OL-4/OL-10; the tested selection (`fifa96_keeper_select_action`,
  `fifa96_dispatch_*`) is unchanged.

## 7. Concerns

* FU-140 corrects four FU-79 claims (Errata), including the input handler's
  camera-box sense; the port is pinned to the bytes, not the old prose. A
  later wiring of `FUN_00076130` must use the corrected sense (inside means
  `side != 0 ? cam_z >= 0x7B0 : cam_z <= -0x7B0`, x within `±0x420`).
* `fifa96_keeper_hold_fallback` takes raw 32-bit camera/direction dwords
  because the native shifts the full values; passing pre-extracted `int16_t`
  would differ at the 16.16 wrap edge (e.g. `[0x1577C0] = 0x00008000`).
* `FUN_000700F4`'s camera update is the only row-1E side effect whose input is
  captured; the engine has no camera consumer for it yet, so the `place_*`
  fields are write-only like `install`/`ran` (OL-37).
* Row `1E` writes no `+0x9E`, unlike row `00`; the handler keeps `ran` intact
  so the two wired rows do not share an invented flag convention.
* The FU-137 §6.1 row `1E` cell and §7 totals changed in place (errata below);
  the engine `fifa96_match_handlers.c` evidence strings for the keeper rows now
  cite FU-140 and OL-33..OL-37.

## Errata (prior docs)

* **FU-79 §8.1 (input-handler camera box)** — the bytes at `0x76179..0x761A3`
  set the "inside" flag to 1 when `team[+0x826] != 0 ? [0x157754] >= 0x7B0 :
  [0x157754] <= -0x7B0` (`JGE`/`JLE` jump to the gate with `AX` still 1) and
  to 0 otherwise; FU-79 wrote the condition as setting the flag to 0. Corrected
  in §3.1 and pinned by `test_keeper_bodies::test_input_decide_camera_box`.
  Also `saved` is read with `MOVSX DX, byte [rec+0x91]` (`0x76214`), i.e.
  `(int8)`, not `(int16)`.
* **FU-79 §4 (row 1B stage 4)** — `0x776C3 MOV byte [EBP+0x92], 4` writes the
  stage byte 4 before `FUN_0007DAB4`; FU-79's stage table omitted the write.
  Doc-only: the arm is unported (OL-34).
* **FU-79 §6 (row 1D prologue)** — the per-side camera-target index is
  `[0x157C5E] + side*0x18 + (([0x15777C] >= 0) ? 0xC : 0)` (`0x74EED..0x74F09`:
  `SETGE` then `(4-1)<<2 = 0xC`, added when the compare holds); FU-79 wrote
  `([0x5777C]>=0 ? 0 : 0x24)`. Corrected; the block is unported (OL-35).
* **FU-79 §2.7 (row 19 fallback axes)** — the clamped base is the fallback's
  **z** coordinate and `FUN_00074694` supplies **x** (`0x74C3A` loads
  `0x15774C` as a word for the weighted arm); FU-79's prose did not pin the
  axes. Ported exactly in §3.2.
* **FU-79 §8.1 sole-caller/return** — confirmed: sole caller `0x76599`, return
  `([rec+0x91] != saved)` discarded by the caller; the port exposes the
  decision outputs instead.

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit): `read_memory
0x1106E0` (180 B); `disassemble_bytes` `0x74694..0x746E3`,
`0x746E4..0x7475F`, `0x7478C..0x7483F`, `0x74ACC..0x74C8F`,
`0x74CDC..0x74D84`, `0x74EB0..0x74F70`, `0x7550C..0x755D3`,
`0x76130..0x7636C`, `0x76380..0x7643F`, `0x7643A..0x7647E`,
`0x7647E..0x765C2`, `0x7662C..0x766FF`, `0x776C1..0x77727`,
`0x77728..0x777A0`, `0x7876C..0x787C0`, `0x700F4..0x7013F`,
`0x8DC50..0x8DC68`. No writes: no rename/comment/label/function/script/project
save. `/fifa96_le.bin` and `/fifa96.exe` untouched.

Repo: `make check` = **98/98** at task start and after (ASan/UBSan engine tests
included); M1 golden and pinned render hashes unchanged (no render path
touched). Port write set: `include/fifa96_loader/fifa96_keeper.h`,
`src/fifa96_loader/fifa96_keeper.c`, `tests/test_keeper.c`,
`tests/test_keeper_bodies.c`, `include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_handlers.c`, `CMakeLists.txt` (engine links
`fifa96_keeper`), `docs/ghidra/FU137_dispatch_mechanics.md` (errata).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
