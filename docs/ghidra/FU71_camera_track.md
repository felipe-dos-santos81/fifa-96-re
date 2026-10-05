# FU-71: the match camera — follow state, anchors and the pan/event path

Slice S3 (camera/track integration) of FU-67 §5, extended from the mapped
`FUN_000736AC` to the full camera state block `0x57744..0x57829`, the
integration/anchoring math, the target selection used by `FUN_0008D8EC`, the
boundary reflection in `FUN_0007131C`, and the producer/consumers of the
`[0x57784]`, `[0x57A9F]` and `[0x577FA]` anchors left open by FU-67/FU-70.
Ports the clean pieces as `fifa96_camera`.

Result in one line: **the match camera is a per-frame velocity integrator over
the dword position triple `0x5774C/50/54`: while not paused (`[0x57820]`) and
`speed=[0x577BE]!=0` (or an event is armed, `[0x577F0]!=0`) `FUN_000736AC`
advances the word timer `[0x577FA] += delta`, accumulates the 16-bit word
velocities `[0x577C0]/[0x577C2]` over `delta` ticks into `[0x577C6]/[0x577C8]`,
sets the displacement magnitude `[0x577C4] = metric(acc_x, acc_z)` (the FU-67
`FUN_0008DC68` metric), integrates `X += acc_x`, `Z += acc_z`, snapshots the
position into the two anchors `0x57788`/`0x57794` when the timer passes their
stored times `[0x57800]`/`[0x57806]`, then may interpolate (rate bytes
`[0x57816]/[0x57817]`, cursor `[0x577EE]` vs `0x70`/`0x90` by view class
`[0x4C2F6]`) and finally refreshes `speed = metric(vel_x, vel_z)`;
`FUN_0008D8EC` picks the nearest record to anchor 0 (`timer < [0x57800]`),
anchor 1 (`timer < [0x57806]`) or the target vector `0x57770` plus
`vel*0x20`; `FUN_0007131C`'s pan arm copies the position to `0x5777C` (so
`[0x57784]` **is the camera Z at pan time**, the FU-67 open leg), posts the
side from its sign, and mirrors the camera about `±0xE40`/`±0x1620` when
`|X|>0x720`/`|Z|>0xB10` while input bit 0 is set and no transition is
active.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-70). Every
  instruction quoted below was read back this slice with
  `disassemble_function`/`disassemble_bytes`; the decompiler was not used for
  the quoted bodies (FU-67 errata: the `0x72xx`/`0x92xx` pair prunes or
  mis-infers blocks).
* **Dword-alias rule (this slice's central refinement).** The camera scratch
  slots are *consecutive 16-bit words* (`0x577B8..0x577C8`) and the timer
  cluster (`0x577EC..0x577FA`). Ghidra renders many accesses as dword loads of
  the form `MOV EAX,[addr]; SAR EAX,0x10`; the value is then the **next** word,
  not the named one. All FU-67 §4.1 statements of the form `[X]>>16` are
  restated below at word level (errata 1–5). The word map used throughout:

  | word | name | role |
  |---|---|---|
  | `0x577B8` | `scale_b8` | `speed*[0x577F8]` scratch (`0x707EB`) |
  | `0x577BA` | `scale_ba` | `vel_x*[0x577F8]` scratch (`0x707CD`) |
  | `0x577BC` | `scale_bc` | `vel_z*[0x577F8]` scratch (`0x707DC`) |
  | `0x577BE` | `speed` | `metric(vel_x, vel_z)` (`0x73CB8`); heading byte in the reflect arm (`0x71990`) |
  | `0x577C0` | `vel_x` | camera X velocity word |
  | `0x577C2` | `vel_z` | camera Z velocity word |
  | `0x577C4` | `step` | `metric(acc_x, acc_z)` (`0x7384A`) |
  | `0x577C6` | `acc_x` | `delta`-tick X accumulation (`0x73821`) |
  | `0x577C8` | `acc_z` | `delta`-tick Z accumulation (`0x73828`) |
  | `0x577EC` | scratch | never read as a word in this slice |
  | `0x577EE` | `event_cursor` | `+= step` (`0x73AA8`), reset on interpolation (`0x73B4E`) |
  | `0x577F0` | `event_param` | event/zoom parameter; `>0x10` gate (`0x739DC`) |
  | `0x577F2` | `timer_min` | event start time (`FUN_00070B94 0x70BC3`, `0x73A42`) |
  | `0x577F4` | `timer_limit` | event end limit (`0x737BF`, `0x73AD1`) |
  | `0x577F6` | `timer_saved` | start captured by `FUN_00070544` (`0x705C4`) |
  | `0x577F8` | `timer_span` | limit−start (`0x705E8`) |
  | `0x577FA` | `timer` | word timer |

* All numeric claims (offsets, constants, comparison signs) are quoted from
  the listings; semantic labels beyond what the instructions do are not
  asserted.

## 1. The camera state block

### 1.1 Position and vector fields

| address | width | role | evidence |
|---|---|---|---|
| `0x57744` | dword | pointer to the height word-table | `FUN_00070B94 0x70BEC` (`MOV EAX,[0x57744]`; `MOV AX,[EDX+EAX]`) |
| `0x57748` | dword | pointer to the zoom byte-table | `FUN_000702F8 0x70324` (`MOV EDX,[0x57748]`; `MOV AL,[EDX+EAX]`) |
| `0x5774C` | dword | camera X | reset `FUN_000700F4 0x7014E`; integrate `0x73855..0x7386B`; reflect `0x7193C`; cut `FUN_00070DE0 0x7130D` |
| `0x57750` | dword | camera Y (height) | `FUN_00070B94(timer)` result `0x737B4`; drift gate `0x736C3`; height-cross trigger `0x737C6` |
| `0x57754` | dword | camera Z | integrate `0x7385E..0x73883`; reflect `0x7196D` |
| `0x57758`/`5C`/`60` | 3 dwords | previous-frame position copy | `MOVSD`×3 `0x7375B..0x7375D`; read `0x737C8` (`[0x5775C]`), `FUN_00070DE0 0x70DF2` |
| `0x57764`/`68`/`6C` | 3 dwords | pan base position | written `FUN_00070544 0x7054B..0x70557`, reset `0x70197`; read `0x707F6/0x7080C/0x70887/0x70955` |
| `0x57770`/`74`/`78` | 3 dwords | follow target vector | reset=pos `0x701BB`; set `0x70807` (`FUN_00070544`) and `0x73B30` (interp); `[0x57774]=0` at `0x7081D`; read by selection `0x8D984`, tracker `0x72B8F/0x72B97`, `FUN_00071DF4 0x71F93`, `FUN_000703E8 0x70439` |
| `0x5777C`/`80`/`84` | 3 dwords | pan-time position snapshot, Y forced 0 | `MOVSD`×3 `0x713E3..0x713E5` then `[0x57780]=0` `0x713EC`; consumers in §8 |
| `0x57788`/`8C`/`90` | 3 dwords | anchor 0 | frame snapshot `0x7388E..0x738A0`; `FUN_00070544 0x7088F/0x708B2`; interpolation `0x73B36/0x73B48`; selection `0x8D959` |
| `0x57794`/`98`/`9C` | 3 dwords | anchor 1 | frame snapshot `0x738B0..0x738C2`; `FUN_00070544 0x7095D/0x70976`; selection `0x8D96E` |

The vectors are 3 dwords `(x, y, z)`; the entity search reads only the low
words of `+0` and `+8` (`FUN_0008DE8C 0x8DED2/0x8DEC8`), so the selection
target is `(int16_t)vector[0]`, `(int16_t)vector[2]`.

### 1.2 Event, anchor and status fields

| address | width | role | evidence |
|---|---|---|---|
| `0x577D6`/`D7`/`DA` | bytes/word | `FUN_0006D870(pos)` output triple; zoom inputs | `0x73BAA/0x73BC3`, `0x73C3F/0x73C72`; reset copies `0x702A4..0x702D7` |
| `0x577DE`/`E2` | bytes | render-pick output pair | `0x73BE0/0x73C0E`, fallback copy `0x73C24..0x73C33` |
| `0x577E6`/`EA` | bytes | `FUN_0006D870(target)` output pair | `0x73BB9/0x73BC3`, copied from D6/DA at `0x702BC..0x702D7` |
| `0x577FC`/`FE` | words | zoom-scaled output coordinates | `0x73C7A/0x73CA0`; zeroed `0x702D0/0x702DF` |
| `0x57800` | word | anchor-0 time | set `0x73898`, `FUN_00070544 0x7086A/0x70906`; selection `0x8D94E` |
| `0x57802`/`04` | words | pan scale state | `FUN_00070544 0x708AB/0x708C9/0x708D3/0x708F8`; reset `0x701A1` |
| `0x57806` | word | anchor-1 time | set `0x738BA`, `FUN_00070544 0x70938/0x70990`; selection `0x8D963` |
| `0x5780C`/`0E` | words | turn accumulators | `0x738C9..0x7396D`; `FUN_00070544 0x707A8/0x707BE` |
| `0x57810`/`12` | words | mixed turn accumulators | `0x73991..0x73998` |
| `0x57814` | byte | quantised direction | `((word10>>8)&1)<<2 \| ((word12>>7)&3)` (`0x7399F..0x739C1`) |
| `0x57815` | byte | rate index | zeroed `0x702E6`; read `0x71F10/0x71F35` |
| `0x57816`/`17`/`18` | bytes | interpolation rates | set `FUN_00071DF4 0x71F4D/0x71F53`; zeroed `0x7099A/0x709A0`, `0x70227`; interp `0x73AC9/0x73AEC` |
| `0x57819`/`1A`/`1B` | bytes | event step weights | from table `0x104AB` (`0x70119/0x70125`); read `0x705D7/0x70628/0x70A72` |
| `0x5781C` | byte | pan lockout counter | `+= delta`, wraps at `0x1E` (`0x71325..0x71349`); read `0x71202` |
| `0x5781D` | byte | pan-active flag | set `0x713DB`, cleared `0x71908`, `FUN_000740A0 0x740F6`; read `0x7137D`, `0x72AD3`, `0x8B635` |
| `0x5781E` | byte | `FUN_00070074`-flags-zero boolean | written `0x7026C/0x7028A`, `0x713F7`, `0x71DE9`; read `0x718EF` etc. |
| `0x57820` | byte | paused/no-update flag | written `0x7027D/0x7028A`, `0x71DE9`; read `0x7375E`, `0x7C7F0/0x7C90F` |
| `0x57821` | byte | interpolation lock | `++` `0x70A65`; read `0x739C6/0x7063D/0x70A53` |
| `0x57822` | byte | paired with `0x57820` | `0x70278/0x70284` |
| `0x57824` | byte | height-trigger direction | `0x703FF/0x7052B` |
| `0x57825`/`27`/`29` | words | previous-frame `speed`/`vel_x`/`vel_z` | `0x73728..0x73750` |

`[0x57744]`/`[0x57748]` table pointers have no writer in this slice (open leg
9.1). `FUN_00070B94` returns 0 when the timer passed the limit or
`event_param <= 0` (`0x70BA3..0x70BB0`) and otherwise indexes the
`[0x57744]` word table at `2*(0x94 - |timer_min - input|)` minus a second
term (`0x70BD6..0x70BFD`); the height-slot assignment is
`[0x57750] = FUN_00070B94(timer)` (`0x737A1..0x737B4`).

## 2. Per-frame update `FUN_000736AC`

Sole caller `0x4B193` in `FUN_0004B100` (FU-67 §4 step 6; `get_xrefs_to
0x736AC` → `{0x4B193}`). Exact order of the 385-instruction body:

| # | site | action |
|---|---|---|
| 1 | `0x736B2..0x73722` | if `speed > 0xC` (signed word) and `camY > 0xA0`: RNG (`FUN_00092AC8`) drifts `vel_x`/`vel_z` by ±1 word |
| 2 | `0x73723` | `FUN_00092864` (history-ring tick) |
| 3 | `0x73728..0x73750` | `0x57825/27/29` = `speed`/`vel_x`/`vel_z` word shadows |
| 4 | `0x73756` | `FUN_00072AC4` (tracker; FU-67 §2) |
| 5 | `0x7375B..0x7375D` | previous position `0x57758` = `0x5774C` triple |
| 6 | `0x7375E` | if `[0x57820]!=0` return (paused) |
| 7 | `0x7376B` | if `speed==0 && event_param==0` → `0x73B70` (tail; timer untouched) |
| 8 | `0x73783..0x7379A` | `timer += delta` (`delta = [0x57A64]` word) |
| 9 | `0x737A1..0x737B4` | if `event_param!=0`: `camY = FUN_00070B94(timer)` |
| 10 | `0x737B9..0x737DA` | if `timer > timer_limit` **or** (`0x5775C`(prev Y) `>0` and `camY<=0`): `FUN_000709D0` (pan step) |
| 11 | `0x737DF` | if `speed==0` → `0x73B70` |
| 12 | `0x737ED..0x73832` | `acc_x=acc_z=0`; loop `delta` times: `acc_x^=vel_x`, `acc_z^=vel_z` (16-bit) |
| 13 | `0x73834..0x7384A` | `step = FUN_0008DC68(acc_x, acc_z)` |
| 14 | `0x73850..0x73883` | `camX += acc_x`; `camZ += acc_z` |
| 15 | `0x73874..0x738C2` | if `timer > [0x57800]`: anchor0 = position, `[0x57800]=timer`; same for `[0x57806]`/anchor1 |
| 16 | `0x738C3..0x73973` | turn accumulators from `camY` vs `8` and `acc_x/acc_z<<4` |
| 17 | `0x7399F..0x739C1` | direction byte `0x57814` |
| 18 | `0x739C6` | if `[0x57821]!=0` → `0x73B70` |
| 19 | `0x739CE` | if `speed==0` → `0x73B70` |
| 20 | `0x739DC..0x739E7` | if `event_param <= 0x10` → `0x73B70` |
| 21 | `0x739ED..0x73A03` | if both rates zero → `0x73B5B`; if input bit 2 (`[0x4C1D4]\|[0x4C1D6]` bit 1) → interpolate |
| 22 | `0x73A1E..0x73A7B` | ball special arm: if `[0x577CA]!=0`, `[[ball+4]][0]==0x18D8`, `event_param > 0xF0`, `timer_min < timer < timer_min+3` → `timer = timer_min` |
| 23 | `0x73A80..0x73AA8` | `threshold = view_class ? 0x90 : 0x70`; if `event_cursor >= threshold` interpolate, else `event_cursor += step` |
| 24 | `0x73AB4..0x73B59` | interpolation (rate ramp, §5) |
| 25 | `0x73B5B..0x73B6B` | if ball has `[+0x20]`: `FUN_00071DF4` |
| 26 | `0x73B70..0x73B9B` | if `\|camX\|>0x6C0` or `\|camZ\|>0xAB0`: `FUN_0007131C` |
| 27 | `0x73BA0..0x73C38` | `FUN_0006D870` render picks; ball-follow pick on team `+0x7B2` type 5 |
| 28 | `0x73C38..0x73CB8` | zoom outputs `0x577FC/FE`; `speed = metric(vel_x, vel_z)` |

Steps 8–15 are the "follow" core; 16–17, 27 and the `FUN_0006D870`/zoom
outputs are render-side and outside the port. Steps 10, 22, 25 call event
machines (`FUN_000709D0`, `FUN_00071DF4`, `FUN_00070DE0` via `FUN_0007131C`)
that are mapped, not decomposed (open legs 9.3–9.5).

## 3. Follow integration, word semantics

The listing at `0x737ED..0x73883` reads (`disassemble_function 0x736AC`):

```
0x737EF  [0x577C6]=0; [0x577C8]=0
0x73800  loop delta times (signed delta>0):
0x73815    ECX += EDX (EDX=word[0x577C0]); 0x73821 [0x577C6]=CX
0x7381F    ESI += EDX (EDX=word[0x577C2]); 0x73828 [0x577C8]=SI
0x73834  EDX=[0x577C6] dword >>16 = word[0x577C8]
0x7383A  EAX=[0x577C4] dword >>16 = word[0x577C6]
0x73845  CALL 0x8DC68
0x7384A  [0x577C4]=AX
0x73850  EAX=[0x577C4]>>16 = word[0x577C6]; camX += EAX
0x73866  EAX=[0x577C6]>>16 = word[0x577C8]; camZ += EAX
```

so: `acc_x/acc_z = sum of delta word additions of vel_x/vel_z` (equal to a
16-bit `vel*delta` with `delta>0`, and exactly 0 when `delta<=0` because the
loop is skipped after the explicit zeroing), `step = metric(acc_x, acc_z)`,
and the **position integrates the accumulations**, not the step (errata 2).
The comparisons at `0x73874/0x738A1` are signed word `>` (`CMP AX,DI / JLE`,
`0x73889/0x738AE`), so a wrapped timer is negative and does not refresh an
anchor. The metric is FU-67 §3.3's `FUN_0008DC68`, ported as
`fifa96_entity_distance`.

The tail (`0x73C9B..0x73CB8`) computes `speed = metric(word[0x577C0],
word[0x577C2]) = metric(vel_x, vel_z)` via the same aliasing (errata 5); it
runs on every path except the paused early return (`0x7375E` jumps to the
final `RET` at `0x73CBE`).

## 4. Target vector selection in `FUN_0008D8EC`

The phase-2 arm (`0x8D929..0x8D9B7`, re-read with `disassemble_bytes
0x8D920`):

```
0x8D948  AX=[0x577FA]; CMP AX,[0x57800]; JGE 0x8D963
            copy 0x57788 triple to stack; bucket 0
0x8D963  CMP AX,[0x57806]; JGE 0x8D978
            copy 0x57794 triple to stack; bucket 1
0x8D978  copy 0x57770 triple to stack
0x8D97F  EAX=[0x577BE] dword >>16 = word[0x577C0]
0x8D98D  SHL EAX,5; ADD [stack+0],EAX
0x8D992  EAX=[0x577C0] dword >>16 = word[0x577C2]
0x8D99E  SHL EAX,5; ADD [stack+8],EAX
0x8D9B2  FUN_0008DE8C(stack, team, skip=0, out=NULL) -> team+0x7B2
```

So the target is anchor 0 / anchor 1 / `target + vel*0x20` by the signed
`timer` vs `[0x57800]`/`[0x57806]`, and the third bucket extrapolates the
target vector by the current camera velocity × 0x20 (`SHL 5`; the port uses
`*32`, same value without the UB left-shift). This is FU-70's
`fifa96_control_target_bucket` case, now with the exact points: bucket 0 →
`(0x57788[0], 0x57788[8])`, bucket 1 → `(0x57794[0], 0x57794[8])`, bucket 2 →
`(0x57770[0] + vel_x*0x20, 0x57770[8] + vel_z*0x20)`.

## 5. Interpolation/event arm (`0x73AB4..0x73B59`)

Gates: `event_param > 0x10` (signed, `0x739DC`), at least one rate byte
non-zero (`0x739ED`), and either input bit 2 (`0x73A03`) or
`event_cursor >= (view_class ? 0x90 : 0x70)` (signed, `0x73A80..0x73AA0`).
Otherwise the cursor accumulates `step` (`0x73AA8`) and the arm returns. When
it runs:

```
0x73AD1  AX=[0x577F4] (timer_limit); DI=[0x577FA] (timer)
0x73AE5  EAX = AX - DI (16-bit wrap)
0x73AC9  DX=sign8([0x57816]); CX=[0x577C0]; ECX += EDX; [0x577C0]=CX
0x73AEC  BX=sign8([0x57817]); SI=[0x577C2]; ESI += EBX; [0x577C2]=SI
0x73AE9  EDX *= EAX; 0x73B26 DX -> int16 delta
0x73AFD  EBX *= EAX; 0x73B2C BX -> int16 delta
0x73B1A  ECX=[0x57770]; ESI=[0x57778]
0x73B30  [0x57770]=ECX+dx; [0x57778]=ESI+dz
0x73B36  EDI=[0x57788]; EBP=[0x57790]
0x73B40  [0x57788]=EDI+dx; [0x57790]=EBP+dz
0x73B4E  [0x577EE]=0
0x73B54  FUN_000703E8
```

i.e. velocities ramp by the rate bytes and the target and anchor 0 are
extrapolated by `rate * remaining` (`remaining = (uint16_t)(timer_limit -
timer)`, product truncated to 16 bits); anchor 1 and `target_y` are not
touched. `FUN_000703E8` (`0x703E8..0x70541`) is a pure geometry/render gate
that recomputes `[0x57823]`/`[0x57824]` from the target vector and the ball
side (`0x70439..0x7052B`); not ported.

`FUN_00071DF4` (`0x71DF4..0x7200F`) is the producer of the rate bytes for the
ball sub-object event: for the special ball type `0x18D8` with
`[ball+0x20]!=0` and `event_param > 0xF0` it sets `vel_x/vel_z` from
`[sub+0x20]/[sub+0x21] * 0xF` clamped to ±0xF (`0x71E5B..0x71E95`; `0x71E9A` writes the
`0xFFF1` negative clamp) and recomputes `speed`
(`0x71ECF..0x71EE5`); otherwise, when `[0x57815]>=0`, `[0x577F8]>>16 <= 0x1E`
(alias: `timer_span` word) and the sub-object exists, it loads the rate pair
from table `0x10E169` indexed by `[sub+0x1D]>>24`, `[sub+0x1E]>>24` plus
`[0x57815]<<3` (`0x71F0E..0x71F53`) and posts command-ring ids 0x1D/0x1E
(`0x71F93..0x72006`). The table was not dumped (open leg 9.4).

## 6. Boundary reflection (`FUN_0007131C`)

`FUN_000736AC` calls `FUN_0007131C` when `|camX| > 0x6C0` or
`|camZ| > 0xAB0` (`0x73B70..0x73B9B`). The pan/event body is 571
instructions; the outer gate is `|camZ| > 0xB20 || |camX| > 0x730`
(`0x713A6..0x713C0`) and the arm ported here is `0x718A9..0x71990`:

```
0x718A9  if !(|camZ|>0xB10 || |camX|>0x720) return
0x718C9  BX=vel_x; CX=vel_z; DI=speed
0x718EF  if [0x5781E]!=0 -> angle arm (0x719A0)
0x718FF  if (input[0x4C1D4]|[0x4C1D6])&1 == 0 -> angle arm
0x71908  [0x5781D]=0; [0x5781E]=0
0x71914  if |camX|>0x720: EBX=-vel_x;
0x7192A    camX = (camX>0 ? 0xE40 : -0xE40) - camX
0x71941  if |camZ|>0xB10: ECX=-vel_z;
0x7195B    camZ = (camZ>0 ? 0x1620 : -0x1620) - camZ
0x71972  [0x577C0]=BX; [0x577C2]=CX
0x7198D  [0x577BE]=FUN_000CD514(vx,vz)   (heading; not ported)
```

so the camera is mirrored about the lines `X=±0xE40`, `Z=±0x1620` and both
velocity words are negated, but **only when input bit 0 is set and the
transition flag `[0x5781E]` is clear**; otherwise (`0x719A0..0x71B98`) an
angle-reflection arm with the `0xE169` table and `FUN_0008DC50` runs (open
leg 9.6). The same `0x720`/`0xB10` constants appear in FU-69's front-end view
record (`FUN_0004D2D4`, indices 0xD/0xF), so they are shared view bounds.

The producer of the pan snapshot is in the same function: when
`[0x5781D]==0`, phase is 2 or 0x10, and `|camZ|>0xB20 || |camX|>0x730`,
`0x713DB` sets `[0x5781D]=1`, `0x713E3..0x713E5` copy the position triple to
`0x5777C`, `0x713E6` sets `[0x57ACB]=1`, `0x713EC` zeroes `[0x57780]`, and
`0x713F2` refreshes `[0x5781E]` via `FUN_00070074`; the rest of the arm posts
announce/command ids (`0x71400..0x718A4`, including the score arm
`0x714AF..0x714CE`) and the clamp at `0x718A9` (§8).

## 7. `[0x57784]` producer and consumers

**Producer.** `[0x5777C]` is the 3-dword pan-time snapshot; its third dword
`0x57784` is written by the third `MOVSD` at `0x713E5` (`get_xrefs_to
0x57784` lists the sole WRITE at `0x713E5`), copying `[0x57754]` (camera Z).
`[0x57780]` is zeroed immediately after (`0x713EC`), so the vector is
`(camX, 0, camZ)` at the moment the pan event arms. Consumers
(`get_xrefs_to 0x57784` plus the read windows):

| site | function | evidenced use |
|---|---|---|
| `0x71413` | `FUN_0007131C` | `CMP [0x57784],0; SETL` compared to the `[0x57A83]` side byte `[[0x57A83]][+0x826]`; selects ring type 0x17/0x18 (`0x71439/0x71440`) |
| `0x7257F` | `FUN_00072478` | `CMP [0x57784],0; SETL` after `[[EBP+0x8D]]!=0` — side select for the ball-only tracker arm |
| `0x8896B`, `0x889B6`, `0x88B99` | `FUN_00088940` | `EDX = \|[0x57784]\|` (`0x88971..0x88977`), magnitude vs `0xB20` (`0x8897D`), sign as side, and `SETL` at the announce tail |
| `0x831E1` | unnamed (`0x831xx`) | copies the `0x5777C` triple to `0x57A77` (`0x831DF..0x831E1`), then tests `[0x5777C]` |
| `0x8D2E3` | unnamed (`0x8D2xx`) | loads `0x57784`/`0x57780`/`0x5777C` and pushes them as a coordinate triple |

So FU-67's open leg "[0x57784] producing coordinate not located" is closed:
it is the camera Z captured by the pan arm, not an independent coordinate.

## 8. Consumers of `[0x57A9F]` and `[0x577FA]`

`[0x57A9F]` (selection entity) writes: reset `FUN_00073E28 0x73E66` (0), and
the selection pass (`0x88955/0x889E0/0x88AB3/0x88AE3`, FU-67 §1). Reads
(`get_xrefs_to 0x57A9F`, windows re-read this slice):

| site | evidenced use |
|---|---|
| `0x88B2A` | `FUN_00088940` tail: `FUN_0008A938(6, [0x57A9F][0][+0x826])` — side of the selected entity |
| `0x4BD9F` | `FUN_0004BD38`: `CMP EDX,[0x57A9F]; JNZ` with phase byte `== ESI` → returns 2 (predicate "candidate is the selection") |
| `0x6E068`, `0x6E0DF`, `0x6E104` | unnamed (`0x6E05C` prologue): compare `[0x57A9F][0]` (team pointer) with the argument entity's `[0]`; then distance `metric([sel+0x59]-[arg+0x59], [sel+0x61]-[arg+0x61])` with `FUN_0008DC68` (`0x6E0FF`) |
| `0x87586` | unnamed (`0x875xx`): `EAX=[0x57A9F]; EBX=[EAX+0x8A]>>24` — dispatch key from the selected entity |
| `0x87D16` | unnamed (`0x87Dxx`): `if (EAX==1) EAX=[0x587D4] else EAX=[0x57A9F]`, then destination `EBP+0x4D` |
| `0x89E0C` | unnamed (`0x89Exx`): `CMP EAX,[0x57A9F]; SETZ; [0x587E8]=(arg==selection)` |
| `0x8AD0F` | unnamed (`0x8ADxx`): `[0x57A9F][0][+0x826]` — team side of the selection |
| `0x8AD69` | unnamed: copies `[0x57A9F][+4]` into `[0x57B16]`, then `CALL 0x8ECC4` |
| `0x73E66` | writer (reset), §7/§9 |

Naming beyond "the selected entity" is not asserted; the consumers are all
selector/identity/distance uses.

`[0x577FA]` (camera timer) consumers (`get_xrefs_to 0x577FA`):

| site | function | evidenced use |
|---|---|---|
| `0x7378A/9A`, `0x737B9`, `0x7387D`, `0x738A1`, `0x73A42`, `0x73A74`, `0x73AD1` | `FUN_000736AC` | timer advance, limit/bucket/interp comparisons, reset to `timer_min` |
| `0x7128D` | `FUN_00070DE0` | `INC word [0x577FA]` in a loop while `FUN_00070B94(timer) <= [ESP+0x10]` — the cut code walks the timer until the height curve passes a target |
| `0x705E1`, `0x7061A` | `FUN_00070544` | pan setup: timer = `[0x577F6]`, or 0 when no event |
| `0x733C7` | `FUN_00072AC4` | tracker threshold read |
| `0x8D948` | `FUN_0008D8EC` | anchor selection (§4) |
| `0x71BC3` | `FUN_00071B9C` | `timer + arg` fed to `FUN_00070B94`, result stored to `[EBX+4]` |
| `0x4B61B` | `FUN_0004B5FC` | `timer >= timer_min` predicate (`SETGE`) |
| `0x71C7D` | unnamed (`0x71Cxx`) | `(arg + timer_min) - timer` sign gate |
| `0x7C882` | unnamed (`0x7C8xx`) | `timer <= timer_min` predicate (`SETLE`) |
| `0x7EFE3` | unnamed (`0x7EFxx`) | `timer = timer_min + timer_min/4` (arithmetic shift) |
| `0x83000`, `0x830B8` | unnamed (`0x82Fxx`) | `event_param > 0x70 && timer < [0x57800]` then reads `[[EBP+0x20]+0x10] & 0x40` |

## 9. Reset/init, `FUN_00073E28` and the view class

* `FUN_00074034` is the match-side camera+selection reset: it pushes four
  args (`0`, `[0xF328]`, `[0xF32C]`, `[0xF330]`; `RET 0x10`), calls
  `FUN_000700F4` (`0x7405E`), sets `[0x57A73]=0x5774C`, calls
  `FUN_0004C31C`, `FUN_00073EE0`, `FUN_00073E28` (`0x74084`), `FUN_000886D4`,
  `FUN_0007D8D0` and `FUN_000744F4`. Callers: `0x4B095` (match setup, just
  before `FUN_0004B100`) and `FUN_00092E2C 0x92E45`.
* `FUN_000700F4` (133 instructions) copies the pushed triple into
  `0x5774C..0x57757` (`0x7014E MOVSD`×3), fills `0x57764`/`0x57770`/`0x57788`/
  `0x57794` with the same position (`0x70197..0x701D7`), zeroes velocities,
  `speed`, `step`, accumulators, `timer`, `timer_limit`, `event_param`,
  `event_cursor`, anchors times, the `0x5780C..0x57818` words/bytes and
  `0x577CA` (`0x701D8..0x70260`), reads two setting-derived words
  `[0x4C2FE]`/`[0x4C2FA]` through table `0x104AB` into `[0x57819/1A/1B]`
  (`0x700FC..0x70146`), sets `[0x5781E]`/`[0x57820]`/`[0x57822]` from
  `FUN_00070074(pos)` and the fourth argument (`0x70267..0x7028A`), calls
  `FUN_0006D870(pos, 0x577D6, 0x577DA)` and copies the pair to
  `0x577DE/E2/E6/EA` (`0x70290..0x702D7`), zeroes `0x577FC/FE`.
* `FUN_00073E28` (51 instructions) resets `[0x57A9F]`, `[0x57A83]`,
  `[0x57AA7]=0x5774C`, `[0x57A4C..4E]`, `[0x57A64]=0` (frame delta),
  `[0x57A6A/6C/70]`, `[0x57AAB]=0xFF`, then with
  `EAX=[0x4C2F6]` selects `EDX = (EAX ? 0x30 : 0x40)`, calls
  `FUN_000702F8` and stores `EDX + 0x70` in `[0x57A6D]`
  (`0x73E95..0x73EB8`), then `FUN_0007F144`/`FUN_0007A028`.
* `FUN_000702F8` maps a signed distance to a zoom byte: `>= 0x640` → `0x94`;
  `<= 0` → `3`; else `0x94 - [0x57748][(0x640-AX)>>2]` clamped at 0
  (`0x702F8..0x70346`). `FUN_00070074(vector, out_flags)` classifies a
  position: bit 3 `|z|<0xB10`, bit 2 `|z|>=0xB90`, bit 0 `x<-0xD0`,
  bit 1 `x>=0xD0`, bit 4 when `y` is below a `|z|`-dependent threshold
  (`0xA0` for `|z|<=0xB40`, else `0xBE0-|z|`), returns `flags==0`
  (`0x70074..0x700F1`).
* `0x4C2F6` (view class) is written by `FUN_0003749C 0x374FB` as
  `FUN_0001D940(9)` — settings slot 9 (FU-68 §4/§5 row). `FUN_0004B308
  0x4B317/0x4B334` rewrites it to 1/2/3/4 for phases 3/4 (FU-68 row). Its
  camera uses are `0x73A80` (interp threshold 0x70/0x90) and `0x73E72`
  (reset zoom, `0x30`/`0x40`); other readers are outside this slice.

## 10. Helper check: `FUN_000795B4` / `FUN_00079C50`

Both are **not** in the camera chain (`get_xrefs_to` shows no camera caller):

* `FUN_000795B4(a, b, out)` (`0x795B4..0x795F0`, 26 instructions) writes
  `out+2 = b[0]-a[0]` (word X), `out+4 = b[8]-a[8]` (word Z), then
  `out[0] = FUN_000CD514(dx, dz)` (integer angle via table `0x4072C`).
* `FUN_00079C50(entity, DX, BX)` (`0x79C50..0x79C98`) returns the entity's
  `+0x8E` byte when `DX|BX == 0`, else stores `FUN_000CD474(DX,BX)` at
  `+0x7D` and `+0x8E = (([entity+0x7B]>>16)+0x40 & 0x3FF)>>7`.

They are player facing/turn helpers (called from `FUN_0008D8EC`'s
interception arm, `FUN_00088940`, `FUN_0007A084`, record machines); the
camera uses only the same angle primitives inside `FUN_0007131C`/`FUN_00070544`
(open leg 9.6).

## 11. Port: `fifa96_camera`

`include/fifa96_loader/fifa96_camera.h` + `src/fifa96_loader/fifa96_camera.c`
(caller-owned state, no globals, no comments, `-fifa96_err_t` for invalid
arguments). Links `fifa96_entity_update` for the metric. Scope: the follow
core of `FUN_000736AC` steps 8–15/20–24, the target selection of
`FUN_0008D8EC 0x8D948..0x8D9B7`, and the axis mirror of `FUN_0007131C
0x718A9..0x71990`.

| original | port |
|---|---|
| position `0x5774C/50/54`, target `0x57770`, anchors `0x57788`/`0x57794`, timer cluster, word velocities/step/acc/speed, rates, `[0x57820]` | `fifa96_camera` fields (same names as §1) |
| `FUN_000700F4` pure field reset (position copied to the four vectors; velocities/timers/anchors zeroed) | `fifa96_camera_init` (table bytes `0x57819/1A/1B`, `FUN_00070074` flags, paused arg and `FUN_0006D870` outputs stay caller-side) |
| `FUN_000736AC` steps 8–15: timer advance, `acc = vel*delta` (or 0 when `delta<=0`), `step = distance(acc_x, acc_z)`, `pos += acc`, signed-`>` anchor snapshots | `fifa96_camera_update` |
| steps 20–24: `event_param > 0x10`, rates non-zero, input bit 2 or `cursor >= (view_class?0x90:0x70)`, cursor `+= step`, else rate ramp + `target/anchor0 += rate*remaining`, cursor reset, final `speed = distance(vel_x, vel_z)` | same `fifa96_camera_update` |
| `FUN_0008D8EC 0x8D948..0x8D9B7` signed bucket chain, anchor word reads, bucket-2 `target + vel*0x20` offset | `fifa96_camera_target` (returns 0/1/2 and writes `(int16_t)` point; feeds `fifa96_entity_find_nearest`) |
| `FUN_0007131C 0x718A9..0x71990` gate + `[0x5781E]`/input-bit-0 gating + mirror about `±0xE40`/`±0x1620` + velocity negation | `fifa96_camera_reflect` (returns 1 when it mirrored; transition flag/inputs are caller arguments) |
| `0x73B70..0x73B9B` `\|X\|>0x6C0 \|\| \|Z\|>0xAB0` | `fifa96_camera_out_of_bounds(x, z)` |
| heading write `[0x577BE]=FUN_000CD514(vx,vz)` in the reflect arm | not ported (integer atan2 table `0x4072C`) |
| `FUN_00070B94` height curve, `FUN_000702F8`/`0x104AB`/`0xE169` tables, pan setup `FUN_00070544`, pan step `FUN_000709D0`, cut `FUN_00070DE0`, ball special arm, RNG drift, `[0x57821]` interpolation lock, `[0x5781C]` lockout, turn accumulators, `FUN_0006D870`/zoom render outputs | not ported (globals/objects/tables outside the clean piece) |
| `fifa96_camera_update` does not trigger `FUN_000709D0`; a caller that lets `timer` pass `timer_limit` keeps the ported behavior at the pre-trigger step | documented divergence |

## 12. Tests (`tests/test_camera.c`, suite 59 → **60**)

* `fifa96_camera_init`: all four position vectors set from the triple,
  every velocity/timer/rate/flag field zeroed, NULL error.
* `fifa96_camera_update` gates: paused no-op (timer included); `speed==0 &&
  event_param==0` no-op; `speed==0 && event_param!=0` advances only the
  timer; NULL error.
* follow: `delta=3`, `vel=(4,-2)` → `acc=(12,-6)`, `step=13`,
  `pos=(112,200,294)`; `delta=0` and `delta=-1` clear `acc`/`step` and leave
  the position; `delta=-1` wraps the timer to `0xFFFF`; `speed` tail
  `distance(vel_x, vel_z)`.
* anchors: refresh on `timer > anchor_time` only (strict), independent
  per anchor, signed compare with a wrapped timer (`0x8000`/`0xFFFF`).
* event arm: `event_param<=0x10` and zero-rate gates; cursor accumulates
  `step` under view class 0 (0x70) and non-zero (0x90) with signed cursor
  (`0xFFFF`); interpolation `remaining = timer_limit - timer` (negative
  wrap), velocity ramp, `target`/`anchor0` x/z extrapolation with y
  untouched, cursor reset, new speed; input bit 2 forces interpolation.
* `fifa96_camera_target`: all three buckets at the signed boundaries, word
  truncation of the anchor dwords, bucket-2 `vel*0x20` for both signs,
  NULL errors, and a wiring test that feeds the bucket-2 point into
  `fifa96_entity_find_nearest` (nearest of three candidates).
* `fifa96_camera_out_of_bounds`: boundaries `0x6C0`/`0xAB0`, negatives,
  `INT32_MIN` wrap (stays in bounds, as `NEG`), `INT32_MAX`.
* `fifa96_camera_reflect`: in-bounds no-op; X-only and Z-only mirrors with
  the right sign branch and velocity negations; both axes; `0x721`/`0xB11`
  boundaries; input-bit-0 and transition gates; NULL error.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_camera.c src/fifa96_loader/fifa96_camera.c
src/fifa96_loader/fifa96_entity_update.c` runs clean. `make test`: 59/59
before, **60/60 after**.

## 13. Errata (quoted)

* FU-67 §4.1: "RNG drift … gated by `[0x577BC]>>16 > 0xC`" — **refined**:
  the dword at `0x577BC` aliases words `0x577BC`/`0x577BE`, so the gate is
  `speed (word 0x577BE) > 0xC` (`0x736B2..0x736BA`).
* FU-67 §4.1: "`[0x577C4] = FUN_0008DC68([0x577C4]>>16, [0x577C6]>>16)` then
  `[0x5774C] += [0x577C4]>>16` and `[0x57754] += [0x577C6]>>16`" —
  **corrected**: the shifts read the following words; it is
  `step = distance(acc_x, acc_z)` and `camX += acc_x`, `camZ += acc_z`
  (`0x7383A..0x73883`).
* FU-67 §4.1: "`[0x57750] = FUN_00070B94([0x577F8]>>16)`" — **corrected**:
  the argument is the word timer `[0x577FA]` (`0x737A6..0x737AE`).
* FU-67 §4.1: "`[0x577BE] = FUN_0008DC68([0x577BE]>>16, [0x577C0]>>16)`" —
  **corrected**: the arguments are `vel_x`/`vel_z`
  (`0x73CA7..0x73CB3`), i.e. `speed = distance(vel_x, vel_z)`.
* FU-67 §4.1: "`[0x577F0]!=0` … boundary check `FUN_000709D0` when
  `[0x577FA] > [0x577F4]`" — **extended**: the trigger is
  `timer > timer_limit` **or** (previous `0x5775C > 0` and new height `<= 0`)
  (`0x737B9..0x737DA`).
* FU-67 §4.1: "the mixed `0x57810/0x57812`, and the quantised direction byte
  `0x57814`" — **derived**: `direction = ((word0x57810>>8)&1)<<2 |
  ((word0x57812>>7)&3)` (`0x7399F..0x739C1`); render-side, not ported.
* FU-67 §4.1: "the per-tick integration … `[0x577C4] = FUN_0008DC68(...)`"
  — the per-tick loop is exactly `delta` 16-bit additions of the velocity
  words (`0x73800..0x73832`), equal to a mod-65536 multiply for `delta>0`.
* FU-70 §2.3: "target = 0x57770 + `([0x577BE]>>16)*0x20` on X + `([0x577C0]>>16)*0x20`"
  — **confirmed and sharpened**: the aliased values are `vel_x` and `vel_z`
  (`0x8D97F/0x8D992`), the comparisons are signed `JGE`
  (`0x8D955/0x8D96A`), and the factor is `SHL 5`.
* FU-69 §2/§4 anchors "camera reset `FUN_0004FA36`, transition
  `FUN_000510DC`" — **scope clarification**: those are the front-end/view
  camera-control reset and the transition manager (`disassemble_bytes
  0x4FA20/0x510C0`: `MOV EAX,1; CALL 0x37114` and the `0x510DC` prologue),
  not the match camera block derived here; the match reset is
  `FUN_00074034`→`FUN_000700F4`+`FUN_00073E28`. The `0x720`/`0xB10` bounds
  match FU-69's `FUN_0004D2D4` view record clamps.
* FU-67 §4.1 "`[0x57784]` producing coordinate was not located" —
  **closed**: sole writer `0x713E5` copies camera Z into `0x5777C+8` (§7).
* FU-67 §8 "`[0x5781E]`/`[0x5781D]`: only the gates are evidenced" —
  **extended**: `FUN_000700F4 0x7026C` sets `[0x5781E] = FUN_00070074(pos)==0`,
  `0x713DB` sets `[0x5781D]=1` at pan arm, `0x71908` clears both, and
  `FUN_00071C94 0x71DE9` rewrites `[0x57820]`.

## 14. Open legs

1. **`[0x57744]`/`[0x57748]` table pointers** (height and zoom curves) have
   no writer found this slice; the height/zoom curves are not ported.
2. **`0x104AB` table** (event step weights `0x57819/1A/1B`) and `0x10E169`
   rate table: encoded/absolute flat form not resolved (FU-67 address
   convention; not re-read).
3. **`FUN_00070544`/`FUN_000709D0` pan setup/step**: the velocity/orientation
   math (`FUN_000795A4` fixed multiply, `0x14E04` sine table, `FUN_000CD514`
   angles) is mapped, not decomposed; the port does not reproduce it.
4. **`FUN_00070DE0`** (camera cut/reposition, 387 instructions) including the
   `0x7128D` timer walk and `FUN_00071DF4`'s `0x10E169` rate source: bodies
   cited only.
5. **`FUN_00071C94`** (writes `event_param`/`paused`, calls `FUN_00070544`)
   and `FUN_00070C08` (reads `event_cursor`/`speed`): callers and role not
   derived.
6. **Angle arm of `FUN_0007131C 0x719A0..0x71B98`** and the whole announce
   table `0x71400..0x718A4`: only the mirror arm is ported.
7. **Who drives `vel_x`/`vel_z` in normal play** beyond the event functions
   (`FUN_00070544`/`FUN_000709D0`/`FUN_00070DE0`/`FUN_00071DF4` and
   `FUN_0007A490 0x7A92D`) is not established; the ball-event write at
   `0x7A92D` is cited only.
8. **`[0x57A9F]` consumers' host functions** (`0x6E05C`, `0x875xx`,
   `0x87Dxx`, `0x89Exx`, `0x8ADxx`, `0x831xx`, `0x8D2xx`) are undefined
   regions in this Ghidra database; only the quoted instructions are claimed.
9. **`[0x577FA]` non-camera consumers** (`FUN_00071B9C`, `FUN_0004B5FC`,
   `0x7C8xx`, `0x7EFxx`, `0x82Fxx`) are cited from windows, not derived.
10. **`[0x57A9B]` consumer** remains unfound (FU-67 open leg carried).
11. **`0x57750` height consumers** other than the drift/trigger gates and
    `FUN_00070DE0` are not derived; the height curve is not ported.
12. **Pan snapshot consumers** `0x831xx`/`0x8D2xx` (render/projection?) are
    only quoted.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x736AC, 0x795B4, 0x79C50, 0x700F4, 0x7131C, 0x73E28,
0x702F8, 0x70B94, 0x70DE0, 0x70544, 0x709D0, 0x70074, 0x703E8, 0x6D870,
0x71DF4, 0x74034, 0xCD514, 0xCD474, 0x795A4;
`disassemble_bytes` 0x4FA20 (48 B), 0x510C0 (48 B), 0x8D920 (168 B),
0x4BD90, 0x6E050, 0x6E0D5, 0x87576, 0x87D06, 0x89DFC, 0x8ACFF, 0x8AD59,
0x7C872, 0x71BB3, 0x4B60B, 0x71C6D, 0x7EFD3, 0x82FF0, 0x88960, 0x72570,
0x88B90, 0x831D1, 0x8D2D9, 0x374E0;
`get_xrefs_to` 0x57784, 0x5774C, 0x57A9F, 0x4C2F6, 0x73E28, 0x700F4,
0x577FA, 0x74034, 0x736AC, 0x795B4, 0x79C50, 0x577EE, 0x577F0, 0x577C0,
0x5781D, 0x57820, 0x57770, 0x577BE, 0x70544;
`get_function_by_address` 0x89E0C, 0x87586, 0x6E068, 0x8AD0F, 0x70C08;
`search_instructions` operand `0x57a9f`, `0x577fa`, `0x4c2f6` (no memory
matches; byte windows used instead).

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_camera.h`,
`src/fifa96_loader/fifa96_camera.c`, `tests/test_camera.c`, `CMakeLists.txt`
(one library/test block). `make test`: 59/59 before, **60/60 after**;
ASan+UBSan `test_camera` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
