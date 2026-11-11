# FU-348: substitution appearance remap rebuild; the ball-vector/vertical re-read; row-05 tail and class-3 camera status

**Provenance.** Phase-87 close-out, 2026-11-09. Fresh Ghidra MCP reads on the
authoritative program **`/FIFA96.EXE`** (case-sensitive, scripts disabled):
`disassemble_bytes` `0x7A140` (220 B) and `0x7F360` (544 B); `decompile_function`
`0x47298`, `0x7A490`, `0x7A084`, `0x7B9C4`, `0x7B194`, `0x795B4`, `0x4DB38`,
`0x4C7D0`, `0x4C77C`, `0x4C7F8`, `0x49388`, `0x6400C`; `get_xrefs_to`
`0x158810`, `0x15880C`, `0x158814`, `0x158738`, `0x15873A`, `0x15873E`;
`disassemble_bytes` `0x84BB0` (48 B) and `0x8C280` (48 B). Prior sections are
not rewritten; corrections land as quoted errata. The engine read set is
`match_run.c` (`match_run_stage_live_kit`, `match_run_apply_player_remap`,
`pause_sub_apply`, `match_run_ball_machine`) and `fifa96_camera.c`.

**Verdict in one line.** The substitution path now rebuilds the per-pitch
appearance remap from the updated lineup (fixing the outgoing player's
skin/hair staying on the pitch), and a fresh re-read settles two open legs:
the ball event vector is `{distance, dx, dz}` plus a camera-trajectory word,
not a vertical triple, and the rendered ball record's Y has no per-frame
producer — native vertical ball physics is **not a leg because it does not
exist in the read paths**. The row-05 tail is a portable target/release/camera
algebra (shape below); the full class-3 camera integrator remains open.

## 1. The substitution remap defect and fix

**Root cause.** `match_run_stage_live_kit` builds
`render.remap_entity[side][pitch]` once, from the then-current `sub_lineup`.
`fifa96_match_run_pause_sub_apply` updates `sub_lineup[side][out_pitch]` and
the bench mask, but never rebuilt the table. The renderer selects the remap by
pitch record (`match_run.c:9745`), so the incoming player kept the outgoing
player's skin/hair classes until the next full `stage_pitch` call. This is a
projection defect, not a native-store defect: the native template pass
`FUN_00047298` reads the current roster descriptor on every pass.

**Native anchor.** `FUN_00047298` (`0x47298..0x47813`) walks the roster
descriptors and writes the 0x8E-byte palette-translation blocks from
`FUN_0004C098`/`FUN_0004C00C` (`0x4C098`/`0x4C00C`) and the `0x1070D4` hair /
`0x1070E4` skin tables. The lineup order is the `0x14C360` desired matrix
(`get_xrefs_to 0x14C360` = 58 sites; the setup fill at `0x117F0`, the sort
`FUN_00012010`, the desired→current reset `FUN_00073EE0`; FU-201/FU-248).
The port's `sub_lineup` is the derived projection of that order; rebuilding the
remap from it is the correct portable behaviour.

**Fix.** `match_run_build_entity_remaps` is now a single builder used by both
`match_run_stage_live_kit` (atomic local commit) and
`match_run_rebuild_entity_remaps` (after a substitution). The rebuild is gated
on `render.team_remap_ready`; it copies `render.remap_team[side]` and overlays
each assigned roster slot's native classes. `pause_sub_apply` calls it
immediately after `sub_lineup`/`sub_bench_avail` change. No score, phase,
entity, ball, camera or census cell is written.

**Test.** `test_engine_match_render.c::test_live_substitution_rebuilds_remap`
stages pitch 1 as roster 0 (skin `'3'` → class 2 base `0xE4`, hair `'4'` →
class 0), substitutes roster 5 (skin `'1'` → class 3 base `0xDB`, hair `'4'`),
and pins `remap_entity[0][1][0xDB] == 0xDB`, `[0x75] == 0xDF`, `[0xD7] == 0`,
with pitch 2 still at the default `0xF6`/`0xD7`. The test fails on the
pre-fix tree at the first `0xDB == 0xDB` assert (observed RED).

**Residual.** The native `0x14C360` desired-order producer/sort and the full
per-roster `FUN_00047298` 23-player loop stay open (FU-201-L2/FU-248-L2,
P15-L3). The fixed path is correct for the derived `sub_lineup`; it does not
claim the native order.

## 2. The ball event vector is `{distance, dx, dz}`, not `{x, y, z}`

Fresh `disassemble_bytes 0x7A140` (the `FUN_0007A084` staging site) and the
`0x7F3A1` tail settle the field widths:

```
0x7A149  CALL 0x795B4(rec+0x59, target, 0x158738)
0x7A14E  EAX=[0x15873A] >> 16 ; PUSH
0x7A157  EAX=[0x158738] >> 16 ; PUSH
0x7A15F  EDX=0x80
0x7A165  word[0x15873E]=DX
0x7A16C  CALL 0xCD474                     ; angle of the dx/dz words
0x7A171  BX=word[0x158738]
0x7A178  ADD BX,0x5A0
0x7A188  word[0x158738]=BX                ; radius + 0x5A0
0x7A1AC  EBX=[0x158736]>>16
0x7A1B9  CALL 0x795A4                     ; speed * sin(angle)
0x7A1BE  word[0x15873A]=AX                 ; dx
0x7A1C4  EAX=[0x15873E]>>16
...
0x7A1FC  CALL 0x795A4
0x7A201  word[0x15873C]=AX                 ; dz
0x7A207  EDX=0x158738
0x7A219  CALL 0x71C94
```

`FUN_000795B4` is `{out[0]=distance; out[1]=dx; out[2]=dz}` (fresh decompile:
it writes `param_3[1] = param_2[0]-param_1[0]`, `param_3[2] =
param_2[4]-param_1[4]`, then `*param_3 = metric`). The kick path
`FUN_0007B9C4` writes the same triple from the row clamp/angle fold
(`0x7B9EF..0x7BE1F`), and `FUN_0007B194` leaves `{speed, dx, dz}` there.
`0x5873E` is the separate trajectory word (the `FUN_00070B94` camera-height /
angle parameter), not a vertical ball coordinate.

So the block is:

| offset | width | meaning |
|---|---|---|
| `0x158738` | word | distance/speed/radius (the `0x8DC68`/`0x8DCD4` metric) |
| `0x15873A` | word | dx (horizontal) |
| `0x15873C` | word | dz (horizontal) |
| `0x15873E` | word | trajectory (camera height/angle parameter, capped `0x460`) |
| `0x158740` | word | angle (from `0xCD474` over dx/dz) |
| `0x158742` | byte | flags |
| `0x158743` | byte | event code |

The event vector is therefore a 6-byte planar triple. There is no Y component
to integrate.

## 3. The rendered ball record has no vertical producer

The rendered ball is the pool record at `0x1587EC` (stride `0xB2`), x
`0x15880C`, y `0x158810`, z `0x158814`; `FUN_00036C70` stages it into render
slot 22. Fresh `get_xrefs_to 0x158810` (9 refs):

| site | type | body | meaning |
|---|---|---|---|
| `0x88761` | WRITE | `FUN_000886D4` | reset/teleport placement |
| `0x8C295` | WRITE | `FUN_0008C24C` | commit copies the staged target triple |
| `0x8C299` | WRITE | `FUN_0008C24C` | **then forces y := 0** |
| `0x84BC6` | WRITE | row-0x13 penalty placement | copies the camera triple (one-cut place) |
| `0x8BB75`, `0x36E1E`, `0x88777`, `0x89423`, `0x8A2A5` | READ | ball machine / render / readers | — |

The per-frame producer `FUN_0008BAF0` **reads** y at `0x8BB75` but writes only
x/z (the tail `0x8BDF1..0x8C052`). Fresh `disassemble_bytes 0x8C280` confirms
the commit path: `MOVSD` x3 from the staged triple, then
`MOV dword[0x158810],0` (`0x8C299`). The penalty cut `0x84BB0` copies the
camera triple, so y can be nonzero only as a one-shot place, never as a
velocity/integration state.

**Verdict.** No gravity, bounce, friction, vertical velocity or per-frame
vertical integrator exists in the ball paths read. The ball is a ground-plane
sprite; the native kick trajectory word is a camera/event parameter. Adding
vertical ball physics would be invented behaviour. The leg is closed as
**not-native**; if a future visual loft is wanted, it must be a labelled
presentation stand-in, not a claimed native producer.

## 4. Row-05 stage-0 tail `0x7F3A1..0x7F57B` — portable shape

Fresh `disassemble_bytes 0x7F360` (544 B) resolves the last stack ambiguity and
gives the exact algebra. Stack frame after `SUB ESP,0x28`:

| slot | word/dword | meaning |
|---|---|---|
| `[ESP+0x0c]` | word | event distance (written by `0x8DC68`) |
| `[ESP+0x0e]` | word | event step x (`speed * dir_x` low word) |
| `[ESP+0x10]` | word | event step z (`speed * dir_z` low word) |
| `[ESP+0x0a]` dword | — | overlaps the distance word; `>> 0x12` = `distance >> 2` |
| `[ESP+0x14]` | word | dir_z |
| `[ESP+0x18]` | word | speed |
| `[ESP+0x1c]` | dword | type dir_x (`0x10F334[type8]`) low word |
| `[ESP+0x20]` | dword | type dir_z (`0x10F33C[type8]`) low word |
| `[ESP+0x24]` | word | dir_x (also `[ESP+0x26]` high word) |

Entry from the dir-byte writes (`0x7F386`/`0x7F397`): `[0x15872A]=dir_x`,
`[0x15872B]=dir_z`, `SI=dir_x`.

1. **Both dirs zero** (`0x7F39C..0x7F3F7`): read `lane =
   (int16)(dword[+0x69] >> 16)`. If `lane > 0x20`, copy the camera triple
   `0x15774C/50/54` to the local target; else copy the record triple
   `+0x59/+0x5D/+0x61`. Copy the local target to `+0x4D/+0x51/+0x55`.
   If `word[0x1577BE] <= word[+0x71]`, return. Else `speed = word[+0x71] << 2`
   (low word) and `dir_x/dir_z = type_dir_x/z`; continue at 0x7F3FC.
2. **Common speed** (`0x7F3FC..0x7F40E`): `speed = (int16)(speed +
   (int16)word[0x157750])` (camera Y). If zero, return.
3. **Type-dir target override** (`0x7F414..0x7F467`): if `dir_x ==
   type_dir_x` and `dir_z == type_dir_z`, write
   `+0x4D = [0x15774C] + (int16)(dword[+0x6B] >> 16)` **= camera_x +
   word[+0x6D]** and `+0x55 = [0x157754] + (int16)(dword[+0x6D] >> 16)` **=
   camera_z + word[+0x6F]**; `+0x51` keeps the camera Y copy. If
   `(int16)word[+0x6B] > 0x28`, return. If `word[+0x6B] > word[+0x77]`,
   return. If both dirs are nonzero, add
   `(int16)((int16)(word[+0x71] << 3) * 0xB5 >> 8)` to speed.

**Erratum to FU251 §2.** FU251 renders the first target-x expression as
`+ (int32)word[+0x6B]`; the fresh bytes `0x7F42A MOV EDX,dword[EBP+0x6B];
0x7F432 SAR EDX,0x10` read the **word at +0x6D**, not the +0x6B lane (the
AGENTS.md `dword[x]>>16` alias rule). The +0x6B lane is used only by the
both-dirs-zero seed (`0x7F3A9 dword[+0x69]>>16`) and the two lane gates
(`0x7F44A dword[+0x69]>>16`, `0x7F459 word[+0x6B]`). The helper therefore
takes both `lane` (+0x6B) and `word_6d` (+0x6D).
4. **Event vector** (`0x7F48E..0x7F4E7`): `step_x = (int16)(speed * dir_x)`,
   `step_z = (int16)(speed * dir_z)`; `distance = 0x8DC68(step_x, step_z)`;
   store `distance` in the event vector; `FUN_00092820(rec, 0x26)`;
   `height_flag = (int16)[0x157750] > 0x10`; `FUN_00071C94(rec, &vector,
   height_flag, 0)` (the event vector is `{distance, step_x, step_z}`).
5. **Release countdown** (`0x7F4EC..0x7F51D`): `r = has_slot ? 4 :
   (distance >> 2)`; if `word[0x157750] != 0`, `r += word[0x1577F8]`; clamp
   `(int16)r > 0x3C` to `0x3C`; `byte[0x15872D] = (uint8_t)r`.
6. **Parity arm** (`0x7F522..0x7F569`): if `byte[0x157A4F] != 0`, set
   `byte[rec+0x9A]=1`, call `0x79CCC([rec], rec+0x59, &speed, 0)`, clear
   `byte[rec+0x9A]`; if the result is not `rec` and `speed < 0xC0`, call
   `0x6DA64(rec, result)`.

The target-write side is the native "settle the dribble target and arm the
release" arm; the `0x71C94` call is the camera event (not a ball record write).
The `0x92820(0x26)`, `0x79CCC`, `0x6DA64` and `0x157A4F` bodies stay numbered
requests (OL-27, FU251-L5..L7).

### 4.1 Engine binding (partial, live human carrier)

`fifa96_action_carrier_tail_step` implements the whole algebra above as a pure
helper (NULL in/out -> `-FIFA96_ERR_INVALID`). `test_action_possession` pins the
zero-dir camera/record seed and early-out, the type-dir target override and
lane gates, the both-dir speed add (`0x10 << 3` × `0xB5 >> 8`), the speed-zero
exit, the `has_slot ? 4 : distance >> 2` release plus the `0x1577F8` span, the
`0x3C` clamp and the parity request.

`fifa96_match_action_05` fills the helper from the live record (`e->lane_x`,
`e->lane_z` (word +0x6D), `cam_dz6f`, `bound`, `speed71`), the camera
(`pos_*`, `speed`, `ramp_divisor`), the arm's resolved dirs/speed and the
`match_kick_dir_x/z` table. The side effects are applied only when **all** of these hold:

```
live_start && input_state[0] != 0 && out.dirs != 0 &&
id == slot.entity && ball.carrier == id
```

so the non-live M1/M2 tapes stay byte-exact, and idle/AI carriers keep the
previous live glue until the tail/camera interaction is measured (the
`test_live_opponent_presses_and_claims` fixture pins that path). On the bound
path the helper writes the target triple, `ball.pos_release` and the `0x71C94`
event (then `0x71D27` tracked = the record). This is a partial binding of
FU348-L3, not a claim that the full row is live for every carrier.

**Erratum to FU251 L2.** FU251 §6 L2 left the `[ESP+0x0a] >> 0x12` counter
seed unresolved. The fresh layout resolves it: the dword at `[ESP+0x0a]` is
the overlapping `{unknown word, distance word}`, so the shift is exactly
`distance >> 2`; no additional producer is needed. FU251 L1 (`S0c`) is the
same `0x8DC68` result stored at `[ESP+0x0c]`.

## 5. Class-3 camera integrator `FUN_0004DB38` — status

Fresh decompile `0x4DB38..0x4DDA4` (the class-3/slot-5 side camera body):

```
if (FUN_0006400C() == 0) param_4 += 100;
FUN_0004C7D0(); FUN_0004C77C(); FUN_0004C7F8(&local, param_4);
cam[+0x10] = cam[+0x34] - step_x; cam[+0x18] = cam[+0x3C] - step_z;
cam[+0x14] eases toward cam[+0x38] - step_y;
clamp x ±0x420, z ±0x810, y >= extraout_ECX;
yaw += (target - cam[+0x58]) * tick / 0x1E0000;
pitch += (target - cam[+0x5C]) * tick / 0x3C0000;
pitch clamp [0x2000, 63000] (fold 0x8000);
cam[+0x5A] = cam[+0x5E] = 0;
tail copy cam[+0x40/+0x44/+0x48] = cam[+0x34/+0x38/+0x3C];
```

`FUN_00049388` is `return [0x107300]` (the tick), `FUN_0006400C` is
`[0x109A98] == 0`. The position/target helpers are `FUN_0004C7D0`/`0x4C77C`
(both `-0xA2A10()`), `FUN_0004C7F8` (the `0x4C414`/`0x4C3B0`/`0xA2991`/
`0xA2914` matrix build), and `FUN_0004A2A10` (the shared atan). The engine's
`fifa96_camera_behavior_action` is the clamp-only subset (FU-152 §2.8; the
comment names the missing ball-angle slew and `FUN_0004C7xx` tracking). The
live render eye remains the Phase-81 class-3 side envelope
(`FIFA96_MATCH_RUN_SIDE_*` + derived focus), not this integrator.

**Consequence for the ball-follow.** The derived `carrier.pos + dir*0x20`
glue in `match_run_ball_machine` is a workaround for the unported camera
director/follow, not for `0x7A490`: the possession staging never writes the
rendered ball record. The ball record's only per-frame producer is
`FUN_0008BAF0`, whose phase-2 target is camera-relative. Until the class-3
director (or an equivalent native camera place/follow) is live, removing the
glue would leave the ball at the phase-2 camera lead while the carrier moves
away. The correct next dependency is the camera, not another possession
formula.

## 6. Verification and legs

**Verification.** `make check` **124/124**; M1 `cmp` exact
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`; M2
`cmp` exact `41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`.
The census is unchanged (78/78 ported; 58/80 wired). The remap test is
ISO-gated like the rest of `test_engine_match_render`'s live cases; the tail
tests are pure `test_action_possession` cases. The windowed smoke (Friendly,
Brazil vs Germany) reaches the match, keeps the ball visible at the carrier's
feet and advances through possession: `docs/screens/p87-live-tail-possession.png`.
The earlier observed fast match clock in that smoke is a separate pacing
question, not part of this slice.

**Open legs.**

* **FU348-L1 — native lineup matrix.** The `0x14C360` desired-order producer
  (`0x117F0` setup, `FUN_00012010` sort, `FUN_00041494` swap/restore) is still
  unported; `sub_lineup` is the derived map. The remap rebuild is correct for
  that map, not a claim of the native order.
* **FU348-L2 — full `FUN_00047298` pass.** The engine applies the per-pitch
  kit/skin/hair projection; the native 23-descriptor pool, the
  `FUN_0001CAEC(9/0xD)` gates and the bench/descriptor provenance remain
  open (Phase-86 legs).
* **FU348-L3 — row-05 tail binding.** The helper is fully ported and tested;
  the engine binding is the live human-carrier/held-pad path only (§4.1).
  Idle/AI carriers, the `0x92820(0x26)`/`0x79CCC`/`0x6DA64` bodies and the
  full `0x157A4F` parity arm remain open (OL-27/FU251-L5..L7).
* **FU348-L4 — class-3 camera integrator.** `FUN_0004DB38` and its
  `0x4C7D0/77C/7F8/49388` helpers are mapped but not ported; the live eye is
  the derived envelope. This is the prerequisite for replacing the carried-ball
  glue with the native camera-relative ball lead.
* **FU348-L5 — vertical ball physics.** Closed as **not-native** by §3 of this
  document; do not add a gravity/bounce model to the native ball record.

## 7. FU348-L2 closure (2026-11-11)

**Erratum (quoted).** §6 lists "FU348-L2 — full `FUN_00047298` pass ... remain
open (Phase-86 legs)". This is closed for the five-template/23-block builder by
the 23-descriptor remap slice (`b562ae3`, `705417e`): the native
`FUN_00047298` pass is ported as `fifa96_match_remap_build`
(`port/src/fifa96_loader/fifa96_match_remap_native.{h,c}`), and the engine now
builds its entity remaps from the 23-block pool instead of the per-pitch band
approximation.

The closed builder records the native class pairing (`class_a` = `FUN_0004C00C`
→ the `0x1070E4` skin base and the class-0 `0xE3` clear; `class_b` =
`FUN_0004C098` → the `0x1070D4` hair ramp), the template class per slot (home 0
= 2, home 1..10 = 0, away 0 = 3, away 1..10 = 1), the extra class 4 with
skin/hair class 2/2, and the six-band `remap_team` overlay that preserves the
live kit colours over the native blocks.

The remaining sub-legs stay open:

* **FU348-L2a — extra consumer.** The 23rd pool slot has no proved consumer
  (`get_xrefs_to 0x14BF9C` = 0); it is stored native into `render.remap_extra`
  with no draw path.
* **FU348-L2b — GK shade patch.** `FUN_00048DC0` (the native per-sprite palette
  install for pool blocks 0/0xB) is unported; the engine's `0xFF` source-0
  adapter is the documented bridge.
* **FU348-L2c — option gates.** The `FUN_0001CAEC(9/0xD)` skin/hair blanking
  gates are staged 0 (the engine has no option cell); the builder's
  `home_blank`/`away_blank` inputs are passed `0,0` at the live call site.
* **FU348-L2d — special branch.** The `0x107080[class] > 0x15` special branch is
  unreachable for the five template classes (selectors 0/0/14/15/13) and is
  intentionally omitted.
