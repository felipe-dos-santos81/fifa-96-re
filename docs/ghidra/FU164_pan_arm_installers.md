# FU-164 — what each remaining `FUN_00071C94` pan arm needs first (installers + requirement lists)

Provenance: M2 phase-13 second recon wave **R12** (planned FU-164), 2026-10-11,
read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts
disabled; case-sensitive program argument; standard tools only). No repo file
was modified by this slice; no git commands. Every xref/table/byte claim below
is a fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes` /
`search_instructions` of this session. Addresses are LE-image flat addresses
(the FU-148/FU-152/FU-156 citation model); the alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`; each `EDX`-immediate
search hit was checked for the actual callee before being trusted — two false
positives were caught, §2.2).

Question (plan wave R12): **making the six remaining `FUN_00071C94` pan
callers reachable** (FU-156: all six unreachable). For each arm: what has to
land first, the ranked cost/value, and the overlaps with the FU-157 (live
camera producer set), FU-160 (jump-selector completion) and FU-162
(aux-0x32/throw-in neighbours) producers.

---

## 1. Scope and method

Fresh `get_xrefs_to 0x71C94` = **11** (the FU-148/FU-156 census holds, same
set). For each remaining site: containing body (fresh `get_function_by_address`
or the action-table row bounds), the invoke chain up (fresh xrefs), the call
gate (fresh disassembly), and the engine status (source reads + fresh dispatch
table). The installer question is answered by the dispatcher: `FUN_0007D9A4`
(`0x7D9A4..0x7DA93`, fresh) stores `byte[rec+0x91] = DX` (`0x7DA67`) and
`dword[rec+0x18] = 0x1106E0[code]` (`0x7DA74..0x7DA8F`) — **the action code is
installed by the caller of the dispatcher with its `DX` argument**, so the
installers are the fresh `CALL 0x7D9A4` xref sites (77 this session) filtered
by the code immediate.

---

## 2. Fresh census and corrections

### 2.1 The six sites (fresh)

| site | bytes (fresh) | target arithmetic | containing body (fresh) |
|---|---|---|---|
| `0x82A6C` | `e8 23 f2 fe ff` | `0x82A71 − 0x10DDD` | **row 0x0E** `0x82710..0x82ACF`, arm index 2 (corrected, §2.2) |
| `0x7F4E7` | `e8 a8 27 ff ff` | `0x7F4EC − 0xD858` | row 05 stage-0 tail `0x7F3A1..0x7F57B` (no function) |
| `0x6FA62` | `e8 2d 22 00 00` | `0x6FA67 + 0x222D` | `FUN_0006E8E8` arm `[33]` (aux 0x22) |
| `0x71B8A` | `e8 05 01 00 00` | `0x71B8F + 0x105` | `FUN_0007131C` angle arm |
| `0x77DC6` | `e8 c9 9e ff ff` | `0x77DCB − 0x6137` | row 1C `0x77728..0x77E94` common tail (no function) |
| `0x77423` | `e8 6c a8 ff ff` | `0x77428 − 0x5794` | row 1B `0x76D28..0x77727` stage-2 rush test (no function) |

Fresh action-table reads: `0x1106E0[d]` for `d ∈ {5, 0x0D, 0x0E, 0x0F, 0x1B,
0x1C, 0x21}` = `0x7F194, 0x8251C, 0x82710, 0x82AD0, 0x76D28, 0x77728,
0x85214` (the table continues past `0x110760`, §2.2 item 3). The six codes'
engine dispatch: rows `0x0D/0x0E/0x1B/0x1C` answer `-UNSUPPORTED` (NULL);
row `0x21` is **wired** (`fifa96_match_handlers.c:2877` area); row `05` is
wired.

### 2.2 Corrections to FU-156 (each first-hand)

1. **`0x82A6C` is row 0x0E arm 2, not row 0D arm 3.** Row 0D
   (`0x8251C..0x826FD`) RETs at `0x826FE` (`0x826FE RET`, fresh); row 0E
   starts at `0x82710` (fresh prologue `PUSH EBX/ECX/EDX/ESI/EDI/EBP` +
   `SUB ESP,0x14`); its head dispatches `CMP AL,3; JA 0x82AC5;
   JMP CS:[EAX*4+0x82700]` (`0x82748..0x82755`, fresh) and the fresh table
   read `0x82700` (16 B) = `{0x8275D, 0x827B1, 0x82806, 0x82AB8}` — the call
   `0x82A6C` lies in `0x82806..0x82AB7`, i.e. **`byte[rec+0x92] == 2`**.
   FU-156's "arm 3 (0x826D5..0x82ACE)" conflates the two sibling rows; the
   row-0D arm 3 it read (`0x826D5..0x826F3`: `0x79C50`, `+0x89 > 0x14` reset)
   contains no `0x71C94` call.
2. **FU-156 §3.2's `0x718A9` gate operands are the |camX|/|camZ| locals, not
   velocities.** `FUN_0007131C` takes no arguments; its head computes
   `word[ESP+0xC] = |camX|` from `[0x15774C]` (`0x71354..0x7136C`) and
   `word[ESP+0x10] = |camZ|` from `[0x157754]` (`0x71366..0x71383`). The arm
   gate is `CMP word[ESP+0x10], 0xB10 / JG` then `CMP word[ESP+0xC], 0x720 /
   JLE return` (`0x718A9..0x718C3`, fresh) — camera overshoot bounds; the
   angle arm's chains key on |camZ| ≥ 0xE74 / |camX| ≥ 0x8BC of the same
   locals (`0x719CF`, `0x71A5D`, `0x71AF2`) with the **previous** triple
   (`[0x157758]`/`[0x157760]`) as the corridor test. No velocity enters these
   gates.
3. **The action table is longer than the 132-byte window FU-156 read.** Rows
   `0x00..0x20` sit at `0x1106E0..0x11075F`; the table continues contiguously:
   fresh `read_memory 0x110760` (256 B) gives `[0x20]=0x84EEC`,
   `[0x21]=0x85214`, `[0x22]=0x8539C`, `[0x23]=0x82F84`, `[0x24]=0x86510`,
   `[0x25]=0x880CC`, `[0x26]=0x866F4`, `[0x27]=0x86820`, `[0x28]=0x870E8`,
   `[0x29]=0x874E4`, `[0x2A]=0x86A34`, `[0x2B]=0x87738`, `[0x2C]=0x84598`,
   `[0x2D..0x42]` = the `0x6DExx/0x6Exxx` band, `[0x43] = 0`, `[0x44..0x4F]` =
   the `0x8Dxxx/0x89xxx/0x8Bxxx` handlers; the pointer run ends at
   `0x11081C` (`[0x4F]=0x8B900`), after which a word-table structure starts
   (`0x110820`: `ff 07 60 00 ...`). No census claim breaks (all six arms' codes
   were inside the 132-byte window) but a future dispatcher read must not stop
   at `0x110760`. Row 0x21 (the `0x6FA62` gate code) = `0x85214`, the ported
   `fifa96_match_action_21` (`0x85214..0x8539B`).
4. **False-positive catches (alias discipline).** `0x70DAF MOV EDX,0x1c`
   (`FUN_00070C08`) and `0x7B0A7 MOV EDX,0xd` (`FUN_0007AE70` jump arm) are
   *arguments to `0x92820`/`0x928F0`*, not dispatcher installs (the fresh
   call targets at `0x70DBF`/`0x70B0AE` are not `0x7D9A4`); `0x80814 MOV
   EDX,0x1c` feeds `FUN_0006E598` (`CALL 0x6E598` at `0x80826`), an
   animation-id argument, not an install; and `0x7FB85 MOV EDX,0x0E`
   (`FUN_0007F7E0`) is the `0x92820` sink argument (`CALL 0x92820` at
   `0x7FB91`), not an install.

---

## 3. Per-arm requirement lists

### 3.1 `0x82A6C` — row 0x0E arm 2 (event transition; corrected attribution)

**Invoke chain (fresh).** Entity walk `FUN_0008D8EC` (`0x8DB4D` call to
`FUN_0007CA54`, `0x8DB35` to `FUN_000782D0`) → record machine → installer
`FUN_0007D9A4(rec, 0x0E)` → next frame's row-0x0E dispatch
(`0x1106E0[0x0E] = 0x82710`) → `+0x92 == 2` arm (`0x82806`) → `0x82A6C`.

**Installers of code 0x0E (fresh).** Two immediate installers: `FUN_00082DD0
0x82F70` (`MOV EDX,0x0E; EAX=ESI; EBX=0; ECX=1; CALL 0x7D9A4`, fresh — the
tackle-attempt body behind the wired row 0x23 `0x82F84..0x83163`; engine
`fifa96_match_action_23` sets `record.install = 0x0E` at
`fifa96_match_handlers.c:2203` via `fifa96_action_tackle_step`) and
`FUN_0007E600 0x7E7B2` (same shape, fresh; the FU-75 §6-named installer,
ported as the row-04 self-install `fifa96_outfield.c:579` and applied by the
wired row-04 `out.installs` loop). The engine also installs 0x0E from row 07's
defender request (`handlers.c:1008`). Separately the `0x7CF54` input handler
installs `0x09` (engine seam, `fifa96_match_run.c:718` case), whose row-0x09
body installs `0x0D` (`0x80F61`, §3.6 note). So the 0x0E **gate chain is
live**; only the row-0x0E body is missing.

**Call gate (first-hand, arm 2 tail).** `0x82A54 XOR AH,AH; MOV DX,
word[0x157750]; AND AL,0x3F; ADD EAX,EDX; LEA EDX,[ESP+0xC]; MOVSX EBX,AX;
MOV EAX,EBP; CALL 0x71C94` (`0x82A50..0x82A6C`) — the event height is
`(RNG & 0x3F) + [0x157750]` on the ball-height base, then
`0x8F188(0x6C,0x40)`, `0x651F0` x2, `0x974F0(0x1F4)`, `+0x92++`,
`[rec+0x89] = 0` (`0x82AA0..0x82AB2`); arm 3 (`0x82AB8`) is the `+0x44`
reset gate.

**What must land first (ordered).**

1. **Port the row-0x0E body** `0x82710..0x82ACF` (phase-2 gate `0x82723`;
   `+0x89 += [0x157A64]`; 4-arm table `0x82700`). Arm 0 (`0x8275D`) runs the
   `0x71B9C(0x12,&+0x4D,&+0x65)` predictor + `0x79C50`; arm 1
   (`0x827B1..0x82805`) is the row-0x12 install stage (`0x827D0
   FUN_0006E598(rec,0x12)`, FU-159/FU-160 §1 — the ranked #1 T3 target, and
   the engine already models the `0x79B58` receiver timer on that path); arm 2
   the event, arm 3 the reset.
2. **Arm-2 preconditions:** the polar vector staging `0x82806..0x82A50`
   (the `0x114E04` fold + `0x795A4` rotate — both primitives already ported)
   and the `0x8DC68` metric; `0x92820(rec,0x0B)` remains a sink request
   (OL-27); the live `[0x157750]` (T1's throw-in Y producer); the event setter
   (`fifa96_camera_event_set`) and the tracked bind (`FUN_00071C94 0x71D27`)
   already exist.
3. No new globals/cells: the arm reads only `[0x157750]` and the match RNG.

**Engine status.** Row 0x0E NULL in the dispatch table; the installer chain is
live. **Verdict: no today; cheap-to-medium to wire** — and it rides the
FU-159/FU-160 row-0x12 installer work.

### 3.2 `0x7F4E7` — row 05 stage-0 tail (`out.tail`, OL-63)

**Invoke chain (fresh).** Row-05 dispatch (wired) → stage-0 arm → the
`0x7F386/0x7F397` dir-byte writes (`[0x15872A]/[0x15872B]`, engine
`out.dirs`) → tail entry `0x7F3A1` (when `dir_x == 0`) or `0x7F3FC` (when
`dir_x != 0`, via `TEST SI,SI; JNZ` at `0x7F39C..0x7F39F`) → the algebra
converges at `0x7F48E` → the call `0x7F4E7`.

**Staged inputs (fresh `0x7F274..0x7F3A0`).** The type arm
(`ball_height > 0x38`) stages `dir_x/dir_z` from `0x10F334/0x10F33C[type8]`
and speed `0x60`; the slot arm stages them from `slot+0x20/+0x21` and speed
`0x30` (`0x7F2EB..0x7F32B`); the no-slot/no-team-gate fallback calls
`FUN_0007F7E0(rec, &[ESP+0x24], &[ESP+0x14], &[ESP+0x18])` (`0x7F361..0x7F36F`)
and only continues into the tail when `[rec+0x91] == 5`
(`0x7F374..0x7F380`). The tail then: copies the target triple (camera when
`word[+0x6B] > 0x20`, else record pos) into `rec+0x4D/+0x51/+0x55`
(`0x7F3A9..0x7F3CC`); exits when camera speed `[0x1577BE] <= word[+0x71]`
(`0x7F3CD..0x7F3D7`); on the dir_x==0 path replaces speed by
`word[+0x71] << 2` (`0x7F3E0`); adds `[0x157750]` to speed (`0x7F400`);
builds `p1 = speed * dir_x`, `p2 = speed * dir_z` (16-bit products,
`0x7F48E..0x7F4A7`); metric `0x8DC68(p1, p2)`; sink `0x92820(rec, 0x26)`
(`0x7F4C9`); then the call with `EBX=0`, `EDX=&{metric,p1,p2}`,
`ECX=(dword[0x157750] > 0x10)` (`0x7F4D0..0x7F4E7`). Post-call:
`word[0x15872D] = min(0x3C, base + [0x1577F8])` with `base = 4` (slot) or
`metric >> 2` (`0x7F4EC..0x7F51D`), then the `[0x157A4F]` gate → `0x79CCC`
mate pick over `rec+0x59` (`0x7F52F..0x7F542`, rec `+0x9A = 1` during) and,
when the returned mate != rec and a band word < 0xC0, `0x6DA64(rec, mate)`
(`0x7F54E..0x7F56D`).

**What must land first (ordered).**

1. **Port the tail block** `0x7F3A1..0x7F57B` (fresh, 137 insns) consuming
   `out.tail`; the loader already reports the staged `dir_x/dir_z/speed`
   (`fifa96_action_dribble_dir`, header `:219..224`; engine `out.dir_x/dir_z`,
   `speed` not exported yet) and the type/slot/fallback provenance is in
   `fifa96_action_handlers.c:470..516`.
2. **Port `FUN_0007F7E0`** (or at least its write contract on the three staged
   words) — the fallback path's continuation depends on it, and the tail's
   `word[ESP+0x24] != word[ESP+0x1C]` equality test (`0x7F414`) is only
   meaningful after its rewrites (leg L4).
3. **Record words** `+0x69/+0x6B/+0x6D/+0x71/+0x77`: the engine record has
   `lane_x`/`lane_z`/`bound`/`speed71` (match_run.c:1219, 1242-1245); `speed`
   (0x60/0x30) needs staging into the tail call.
4. **Event wiring:** `fifa96_camera_event_set(cam, p1, p2, 0,
   (ball_height > 0x10))` + `fifa96_camera_set_tracked(rec)` (the native
   `0x71C94 0x71D27` write); `word[0x15872D]` (new cell; its consumers are
   unsearched, leg L7). The sink `0x92820` is already an OL-27 request; the
   `0x79CCC` pick exists (`match_run.c:2093`); `0x6DA64` has no consumer.

**Engine status.** `out.tail` is set by the loader but has **no consumer**
(handlers.c:2489-2493, table entry 2841); `FUN_0007F7E0` unported.
**Verdict: no today; medium to wire.** Value: the live claim frame
(`test_live_claim_pan_row04_half_line`) currently fires only the row-04
half-line event; the tail adds the record variant on the same claim and the
`rec+0x4D` target writes.

### 3.3 `0x6FA62` — `FUN_0006E8E8` aux-0x22 arm

**Invoke chain (fresh).** `FUN_0006E8E8` callers = 2: `FUN_0007CA54 0x7CD30`
(outfield record machine) and `FUN_000782D0 0x785AD` (keeper machine); both
call sites immediately follow `EAX=EBP; CALL dword[EBP+0x18]`
(`0x7CD2B`/`0x785A8`, fresh `ff5518`). Fresh xrefs: `FUN_0007CA54` and
`FUN_000782D0` are called from `FUN_0008D8EC` (`0x8DB4D`/`0x8DB35`), which is
called from the per-frame match body `FUN_0004B100` (`0x4B2AB`/`0x4B2B2`) and
`0x74429/0x74430`. So the machine runs every frame for every record.

**Head/dispatch (fresh).** `EBP=rec`; `EAX=rec+0x28`; `DX=word[EAX+0xE]` =
`word[rec+0x36]` (the FU-84 frame aux); zero exits; else
`idx = (aux & 0xFF) - 1`, bound `0x39`, jump `CS:[EAX*4+0x6E800]`
(`0x6E8F3..0x6E928`); fresh table read `0x6E800` (58 dwords): `[33] =
0x6F9EB` (aux 0x22), `[34] = 0x6FAC7`, `[49] = 0x6FA8F` (aux 0x32).

**Arm gate (first-hand).** `0x6F9EB MOV EAX,[EBP+0x8E]; SAR 0x18; CMP
EAX,0x21; JNZ 0x6FA69` (byte `[rec+0x91]` = action code 0x21) and
`MOV EBX,[0x157A83]; CMP EBP,EBX; JNZ 0x6FA69` (rec == the controlled actor).
The event path scales the two per-type bytes
(`[0x10F334]/[0x10F33C][byte rec+0x8E] * 0x50`, `0x6FA0C..0x6FA30`), takes
`0x8DC68` over `(word[ESP+2], word[ESP+4])` with the head's `word[ESP+0]` as
the uninitialized x operand (FU156-L1), then the `[0x157750] > 0x10` ramp
gate and the call `EAX=EBP; EDX=ESP; EBX=0; ECX=(height>0x10)` (`0x6FA51..0x6FA62`).
The non-event path sets `byte[rec+0x44] = 1` (`0x6FA69..0x6FA6D`, i.e.
`byte[rec+0x28+0x1C]`); both paths toggle the aux bit out of `word[rec+0x36]`
(`EAX = 0x22 << CL; XOR; store`, `0x6FA71..0x6FA8A`).

**What must land first (ordered).**

1. **An aux-0x22 frame producer.** Row-aware scan of the ported byte-exact
   FU-84 frame table (`match_run_frame_records`, 0xD00 bytes; each of the 111
   `match_run_anim_rows` entries' `frame_table` walked at the 5-byte record
   stride, 738 frames): **zero** records carry aux low byte 0x22 (aux 0x0E
   twice and 0x3A once; 0x32 four times on rows `0x2C/0x2D/0x2E` frame 0/0/2
   and `0x5C` frame 1 — the same set FU-157 §4.1 names). The trigger
   therefore needs either ISO animation data or an unmodeled
   `word[rec+0x36]` writer (leg L2, carried FU156-L2). This is the **first
   blocker**.
2. **Port the head + arm 33 + the common aux tail** into the engine's existing
   seam: `match_run_record_jump_arm` (match_run.c:1018-1045, the aux-low-byte
   switch, currently 0x0E/0x3A/0x0F only) is called at match_run.c:1213,
   exactly the native `0x7CD30` position (after the handler dispatch, before
   the mover/BF20 block). The aux source `match_run_frame_aux(anim_id, frame)`
   (match_run.c:974) already reads the frame records; the record `+0x36` view
   is the aux word itself.
3. **Record `+0x44`/aux-bit writes:** the engine's `+0x44` model exists
   (`row44`); the aux toggle (`word[+0x36] ^= 0x22`) needs a record aux cell
   (today only the read path exists). The `+0x44 = 1` write on the non-event
   path is a writer the FU-159 byte44 model did not list (leg L8).
4. **Code 0x21 gate:** row 0x21 is wired (`0x85214`, fresh) and
   `rec == [0x157A83]` = the controlled actor (engine `entities.controlled`);
   no new installer needed.

**Engine status.** No `FUN_0006E8E8` port; the stub switch has no 0x22 arm.
**Verdict: no today; cheap port, value blocked by the missing producer.**
The event vector is `{word[+0x8E]-octant-scaled pairs}` — a "ball-thud /
landing" pan on the controlled record with the ball-height ramp.

### 3.4 `0x71B8A` — `FUN_0007131C` angle arm (FU-71 leg 9.6)

**Invoke chain (fresh).** `get_xrefs_to 0x7131C` = 1: `FUN_000736AC 0x73B9B`.
`get_xrefs_to 0x736AC` = 2: `FUN_0004B100 0x4B193` (the per-frame body) and
`0x743EF`. The call gate is fresh: `0x73B70 MOV EAX,[0x15774C]; abs; CMP
EAX,0x6C0; JG 0x73B9B` else `[0x157754] abs CMP 0xAB0; JLE 0x73BA0`
(`0x73B70..0x73B99`) — **out-of-bounds |camX| > 0x6C0 || |camZ| > 0xAB0**;
the engine's identical gate + call exists (`fifa96_match_run.c:3241-3244`,
`fifa96_camera_out_of_bounds`).

**Entry paths into the arm (fresh prologue scan).** `FUN_0007131C` takes no
arguments; the head computes the |camX|/|camZ| locals; four jumps reach
`0x718A9`: `0x71388` (armed `[0x15781D] != 0`), `0x713A0` (phase ∉ {2,0x10}),
`0x713C0` (small distances, `|camZ| <= 0xB20 && |camX| <= 0x730`), and the
post-announce fall-throughs (`0x714DA`, `0x71513`, `0x71530`, `0x7154D`).
Inside `0x718A9`: gate `|camZ| > 0xB10 || |camX| > 0x720`
(`0x718A9..0x718C3`), then `zone != 0` (`0x718EF`) or input bit 0 of
`([0x14C1D4]|[0x14C1D6])` clear (`0x718FF`) → **angle arm `0x719A0`**; else
the mirror (`0x71908..0x71990`, ported as `fifa96_camera_reflect`).

**Angle arm requirements (fresh body `0x719A0..0x71B98`, 215 insns).**

* Head: `word[0x1577F0] != 0 || word[0x1577BE] != 0` (`0x719A0..0x719B2`).
* Chain A (|camZ| ≥ 0xE74, `0x719CF..0x719D4`): if `[0x157750] < 0x24` and
  `|prev z| < 0xE74` → damped reflection `vel_z = −0x8DC50(vel_z, 1)` (the
  sign-preserving halve, engine `camera_shift_step`, camera.c:923); else the
  threshold `t = (|camZ| − 0xE74) / 2 + 0x24`, or `0x20` when the signed
  camera-z cell `[0x157754] < 0` (`0x71A08..0x71A2F`); flag when
  `t >= [0x157750]`, then lateral word `word[ESP+4] = ([0x157754] > 0 ?
  −0x80 : 0x180)` and `SI = (int16)[0x157750] + 0x40`
  (`0x71A34..0x71A5A`).
* Chain B (|camX| ≥ 0x8BC, `0x71A5D..0x71AF2`) mirrors chain A on X:
  corridor |prev x| vs 0x8BC, damping `vel_x = −0x8DC50(vel_x, 1)`
  (`0x71B1B..0x71B2A`), lateral `word[ESP+2] = ([0x15774C] > 0 ? −0x80 :
  0x180)` (`0x71A95..0x71AF0`), plus the small overshoot arm
  (`0x71AF2..0x71B19`: |camX| in [0x6D4,0x8BC), height < 0x1E, |prev x| <
  0x6D4 → damping).
* Tail: flag 0 → store the (possibly reflected) velocity words
  `[0x1577C0/C2]` and recompute the heading `[0x1577BE] = 0xCD514(vx,vz)`
  (`0x71972..0x71990`); flag 1 → speed-bucket announce
  `0x974DC(1 + ({s≤5:0, s≤8:1, s≤0xA:2, else 3}))` (`0x71B37..0x71B5B`),
  then `EAX = 0xCD514(word[ESP+2], word[ESP+4])` → `word[ESP] = angle`,
  `EDX = &{angle, lateral, 0}`, `EBX = SI`, `ECX = 0`,
  `EAX = [0x1577CA]` → `CALL 0x71C94` (`0x71B60..0x71B8A`).

**Helper status (fresh).** `0x8DC50` = the sign-preserving halve — ported as
`camera_shift_step` (camera.c:923, static). `0xCD514(a,b)` = a wrapper over
the ported `FUN_000CD474` (`fifa96_entity_angle`, `0xCD474` fresh: the
`0x14072C` ratio table) + `0xCE364/0xCE386` = the sine primitive
(`fifa96_entity_sine`; the identical structure is documented for
`fifa96_entity_intercept_band`, `fifa96_entity_update.h:59..81`). `0x974DC`
is an announce sink (no-op candidate).

**What must land first (ordered).**

1. **The previous-triple capture.** Native `FUN_000736AC` copies
   `0x157758/5C/60 <- 0x15774C/50/54` (`0x73756`, FU-157 §1) *before* the
   boundary call; the engine camera models no `prev_*` fields (grep: only the
   caller-supplied args of `fifa96_camera_reposition`). The angle arm reads
   `[0x157758]`/`[0x157760]` (`0x719E3`, `0x71A78`, `0x71B02`) — add the
   capture + fields (or pass them).
2. **The armer-head reposition wiring.** The native head calls `FUN_00070DE0`
   unconditionally at `0x7134F`; the port exists (`fifa96_camera_reposition`,
   camera.c:930) but has **no src caller** (fresh grep). The same prev triple
   feeds it.
3. **Extend `fifa96_match_goal_arm`** (match_run.c:2281-2317): the armed
   branch returns 0 for `goal_zone != 0` today; add the
   `zone != 0 || input_bit0 == 0 -> angle arm` dispatch (and decide the
   disarmed entries `0x713A0`/`0x713C0`, FU156-L5).
4. **Port the angle-arm body** `0x719A0..0x71B98` as a new camera helper
   reusing `camera_shift_step`, `fifa96_entity_angle` + `fifa96_entity_sine`
   (the CD514 pattern), and the already-ported event setter (call:
   `fifa96_camera_event_set(cam, lateral, 0, height + 0x40, 0)` — native
   words 1/2 are the seed pair, word 0 the bearing; the engine signature
   consumes the seed pair only, `camera.c:827`/`event_ramp` `:728`).
5. `0x974DC` announce: sink/no-op.

**Engine status.** Enclosing function live, arm unported (match_run.h
1148-1163). **Verdict: no today; medium (helpers exist) but the trigger needs
the camera beyond ±0x720/±0xB10** — far past the measured live pan
(`0 -> 0x90`). Fidelity piece, not a near-term mover; R2's named direct
overlap.

### 3.5 `0x77DC6` (row 1C) and `0x77423` (row 1B) — keeper dive / rush events

**Invoke chains (fresh).** Rows `0x1B = 0x76D28..0x77727` and
`0x1C = 0x77728..0x77E94` (FU-79 §4/§5); both unported (NULL, OL-34/OL-35).
The call sites: `0x77423` sits in the row-1B **stage-2 rush test**
(`0x76FC7..0x77636`, FU-79 §4.1); `0x77DC6` sits in the row-1C **common tail**
(`0x779DE..0x77E8B`, FU-79 §5) that every lunge variant reaches.

**Installers (fresh, up the chain).**

* **Code 0x1B:** row 0x1F body `0x7655C` (`MOV EDX,0x1B; CALL 0x7D9A4`,
  BX=1/ECX=1 — the stage-2 dive lunge, FU-79 §8) and `FUN_00077EAC 0x782A2`
  (the CPU decision, code 0x19 handler; sole caller `FUN_000782D0 0x78599`,
  fresh).
* **Code 0x1C:** `FUN_00076130 0x762A4` (`MOV EDX,0x1C`, after pos→out copy
  and `out.y = 0xA0`; also installs 0x19 at `0x76251` and 4 at `0x76233`,
  FU-79 §8.1) and `FUN_00077EAC 0x782BF`.
* **Row 0x1F install:** `FUN_0008D098 0x8D558` (fresh: `MOV EDX,0x1F`, then
  `[EBP+0x7B2] = EBP`), the state arm that also installs 0x13; **the engine
  already installs 0x1F on the non-controlled keeper for phase 6**
  (derived from `0x8D4A2`, match_run.c:2168) — the 0x1F entry is live, the
  body is missing.
* `FUN_00076130` is called from `FUN_000782D0` (computed, `0x7845D`) and the
  row-0x1F tail `0x76599` (fresh xrefs); `FUN_00077EAC` only from
  `FUN_000782D0 0x78599`. The keeper input machine `FUN_000782D0` itself is
  the record-0 walker of `FUN_0008D8EC`; the engine's record-0 keeper seam is
  documented as not modelled (match_run.c:1180-1184, `if (e->index != 0)`).

**What must land first (ordered).**

1. **Row 0x1F body** `0x76380..0x765C2` (stages 0-3; stage 2 installs 0x1B;
   stage 3 tail calls `FUN_00076130`). Entry install already live (phase 6).
2. **`FUN_00076130`** `0x76130..0x7636C` (the keeper input handler: 4/0x19/
   0x1B/0x1C install gates, FU-79 §8.1) and **`FUN_00077EAC`**
   `0x77EAC..0x782C3` (the code-0x19 CPU decision installing 0x1A/0x1B/0x1C,
   FU-79 §9.2) — both unported (fresh grep: no engine/loader port).
3. **The row bodies:** 1B `0x76D28..0x77727` (stages 0-4 incl. the
   `0x76FC7..0x77636` rush predicates) and/or 1C `0x77728..0x77E94` (stages
   0-2 + the common tail); both need `FUN_00076B28` (steering,
   `0x76B28..0x76D11`) and the helper set (`0x8DCD4`/`0x79C50`/`0x6E598`/
   `0x700F4`/`0x8DC68`/`0x8A938` — all ported primitives), plus
   `FUN_0006E444` for row 1C's timer (`0x77E6F..0x77E84`).
4. The engine's keeper chain is partial (`fifa96_keeper_claim_step`/
   `closedown_step` for rows 1E/1D exist; `keeper_input_decide` helpers in the
   loader), so this lands on top of that family.

**Engine status / verdict.** Both rows deep in the unported keeper chain; the
two pan calls ride the same installers, so they unlock together. **Verdict:
no today; deep.** Value: keeper rush/dive pans in penalty/rush situations
(phase 6/0xD) — visible but not on the phase-13 critical path.

### 3.6 The row-0D arm FU-156 answered for a different row — install chain (fresh, for completeness)

Row 0D (`0x8251C..0x826FD`, the actual "event transition" 4-arm body) is
separate. Its code is installed by the **row-0x09 body** (`0x80A00..0x81067`)
at `0x80F61` (`MOV EDX,0x0D; MOV EAX,ESI; XOR EBX,EBX; CALL 0x7D9A4`,
dispatcher xref `0x80F6A` fresh), whose own code 0x09 is installed by the
`0x7CF54` input handler (engine seam, match_run.c:718). Row 0D still has no
`0x71C94` call (its arm 3 is `0x826D5..0x826F3`); this section exists so the
FU-156 row-0D language is not silently orphaned.

---

## 4. Ranked cost/value table (for a future wiring slice)

| rank | call site | containing body | first blocker | cost | value now |
|---|---|---|---|---|---|
| 1 | `0x82A6C` | row 0x0E arm 2 (`0x82806..0x82AB7`) | row-0x0E body unwired; the 0x0E install chain is already live (rows 04/07/23 all install 0x0E engine-side) | **medium** (small row, helpers exist; shares the FU-160 row-0x12 installer work) | **high** — tackle-situation pan + unblocks the row-0x12 selector |
| 2 | `0x7F4E7` | row 05 stage-0 tail (OL-63) | the tail `0x7F3A1..0x7F57B` + `FUN_0007F7E0` unported; row 05 already live | **medium** (137-insn self-contained tail + fallback + staging; no new row) | **medium** — second pan event on the live claim frame + record-target writes |
| 3 | `0x6FA62` | `FUN_0006E8E8` aux-0x22 arm | **the aux-0x22 frame producer** — none in the executable frame records (fresh scan), ISO data/unmodeled writer; the port itself is small | **cheap port / blocked** (insert into `match_run_record_jump_arm`; aux reader exists) | low-medium today — value gated by the producer leg |
| 4 | `0x71B8A` | `FUN_0007131C` angle arm (FU-71 9.6) | the arm port + prev-triple capture + `goal_arm` extension; helpers (halve/angle/sine) exist; needs camera beyond ±0x720/±0xB10 | **medium** (215 insns, prev wiring, disarmed-path decision) | low-medium now (fidelity) — high once the follow makes out-of-bounds live |
| 5 | `0x77DC6` | row 1C common tail | keeper chain: rows 0x1F/1B/1C + `FUN_00076130`/`FUN_00077EAC` unported (0x1C installer inside them) | **deep** (≈4 unported bodies + steering helper) | low-medium — keeper dive pan |
| 6 | `0x77423` | row 1B stage-2 rush | same keeper chain + the `0x76FC7..0x77636` rush predicates (row body deeper) | **deep** (shares rank 5's upstream; the rush test adds the most) | low — keeper rush pan |

Ordering note: ranks 5/6 share their upstream (one keeper-chain slice unlocks
both); within the pair, row 1C's call is in the unconditional common tail
while row 1B's sits behind the full rush test, so 1C comes first.

---

## 5. Overlaps with the neighbouring producers

* **FU-157 (live camera producer set).** This slice confirms FU-157 §4/§5
  ("no `FUN_00071C94` arm is a precondition") and turns it into per-arm
  requirement lists; it also corrects the §3.3 secondary-mover attributions
  (`0x82A6C` is row 0x0E arm 2, not row 0D arm 3). FU-157's post-T3 extension
  path (throw-in Y → airborne kick → receive → follow) is gated on the
  **row-0x12 selector**: my rank-1 arm shares the row-0x0E body with that
  installer (the stage-0/1 arms, `FUN_0006E598(rec,0x12)` at `0x827D0`).
* **FU-160 (jump-selector gates, landed R8).** FU-160 §1/§4 find the row-0x12
  install fires from the row-0x0E body **stage 1** and rank "only the
  row-0x0E stage 0/1 port" as the #1 T3 target. My rank-1 arm is the **same
  row body, arm 2** — porting row 0x0E once lands the selector (arms 0/1)
  and the pan event (arm 2), so scope that slice to include the arm-2 event.
* **FU-162 (aux-0x32 family, landed R10).** Same machine `FUN_0006E8E8`: the
  aux-0x32 arm (`[49] = 0x6FA8F`) is a keeper camera cut **plus the
  `[0x157A6C]` pan veto** (FU-162 §2.4) — it shares the head/dispatch/common
  tail with the aux-0x22 arm, and its veto holds while a save lasts, so one
  port of the machine should model both and the veto cell (the engine's
  `event_suspended` already reads it).
* **FU-159 (jump/ack residuals).** The engine seam extension point is the
  same (`match_run_record_jump_arm`); the aux machine's non-event path writes
  `byte[rec+0x44] = 1` (`0x6FA69`) — a `+0x44` writer beyond FU-159's
  install-bit producer (leg L8).
* **FU-163 (close-up re-place + keeper-claim stage-5).** The keeper chain
  (ranks 5/6) shares the keeper situation context (rows 0x1D/0x1E/0x1F); a
  keeper-chain slice should be co-scoped with the FU-151 family.
* **FU-161 (arm impulsion).** No direct overlap (rows 0x28/0x2A).

---

## 6. Errata (quoted, first-hand)

1. **FU-156 §3.6 (the `0x82A6C` row/arm attribution).** Quote: "The head
   (0x8251C..0x8255F) … Fresh arm-table read: `[3] = 0x826D5`; the call lies
   in arm 3 (0x826D5..0x82ACE)". **Corrected**: row 0D RETs at `0x826FE`; the
   call `0x82A6C` is in **row 0x0E** (`0x1106E0[0x0E] = 0x82710`,
   `0x82710..0x82ACF`), its own 4-arm table `0x82700 = {0x8275D, 0x827B1,
   0x82806, 0x82AB8}`, **arm index 2** (`+0x92 == 2`, `0x82806..0x82AB7`).
2. **FU-156 §3.2 (the arm gate operands).** Quote: "as `|vel_z| > 0xB10 ||
   |vel_x| > 0x720`" and "the velocity/height threshold chain". **Corrected**:
   the operands are the **|camZ|/|camX| locals** (`word[ESP+0x10]`/`word[ESP+
   0xC]`, computed at `0x71366`/`0x71354` from `[0x157754]`/`[0x15774C]`),
   and the threshold chains are **overshoot-magnitude** algebras (the
   `0xE74`/`0x8BC` corridors over |camZ|/|camX| with the previous triple),
   not velocity terms.
3. **FU-156 §2 (the table window).** The fresh 132-byte `0x1106E0` read is
   rows `0x00..0x20`; the table continues `[0x21]=0x85214 … [0x4F]=0x8B900`
   (`0x110764..0x11081C`). No row named in FU-156 is affected, but the
   "indexed by code through `0x1106E0`" model must not assume the table ends
   at `0x110760`.
4. **FU-156 §3.3 (the aux-0x22 arm's `+0x44`).** FU-159's byte44 producer
   census ("every row install writes row_flags&1") is extended by a
   non-install writer: `FUN_0006E8E8`'s non-event path sets
   `byte[rec+0x44] = 1` (`0x6FA69..0x6FA6D`) — an animation-machine ack.

---

## 7. Numbered legs (new)

1. **FU164-L1 — row-0E arm-2 vector staging.** `0x82806..0x82A50` (the
   `0x114E04` fold/`0x795A4` rotate polar construction) is carried from
   FU-156 §3.6/FU-138 OL-22, not re-derived instruction-level here.
2. **FU164-L2 — the aux-0x22 producer.** Fresh row-aware scan of the ported
   byte-exact FU-84 frame table (`match_run_frame_records`, 0xD00 B; 111
   anim-row entries, 738 frames) finds **no** frame whose aux low byte is 0x22
   (0x0E twice, 0x3A once, 0x32 on rows 0x2C/0x2D/0x2E/0x5C). The producer is
   ISO animation data or an unmodeled `word[rec+0x36]` writer (carried
   FU156-L2).
3. **FU164-L3 — non-immediate installers.** The installer census used the
   `MOV EDX,imm` search plus the 77 fresh `CALL 0x7D9A4` sites; register-
   derived code arguments for `0x0D/0x1B/0x1C` are not exhaustively mapped.
4. **FU164-L4 — `FUN_0007F7E0` write contract.** Only its call and pointer
   arguments are fresh (`0x7F361..0x7F36F`); the three staged words it writes
   (the tail's equality test input) are un-derived.
5. **FU164-L5 — the prev triple.** The engine models no `0x157758/5C/60`
   fields; the capture point is `FUN_000736AC 0x73756` and the consumer is
   both the angle arm and `fifa96_camera_reposition`.
6. **FU164-L6 — keeper installer derivations.** The row-0x1F stage-2 RNG
   lunge, `FUN_00076130`'s gate algebra and `FUN_00077EAC`'s decision arms are
   FU-79 §4/§5/§8-quoted; this slice verified only their call sites, install
   codes and fresh xrefs.
7. **FU164-L7 — `[0x15872D]` consumers.** The row-05 tail's timer byte has no
   consumer census (its producer would land with the tail).
8. **FU164-L8 — the aux machine's `+0x44` writer** (`0x6FA69`): whether the
   FU-159 byte44 model should include this animation-machine ack is open.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_xrefs_to 0x71C94` (11), `0x7131C` (1), `0x736AC` (2), `0x6E8E8` (2),
`0x7CA54` (1), `0x782D0` (1), `0x8D8EC` (4), `0x77EAC` (1), `0x76130` (4),
`0x9E420` (1), `0x7D9A4` (77); `get_function_by_address 0x76D28/0x77728/
0x8251C/0x7F194/0x7F4E7` (no function), `0x7655C/0x80819/0x80F6A` (no
function), `0x762A4` (`FUN_00076130`), `0x9E686` (`FUN_0009E420`),
`0x8D558/0x8D540` (`FUN_0008D098`); `read_memory 0x1106E0` (132 B),
`0x110760` (256 B), `0x6E800` (232 B), `0x82700` (16 B); `disassemble_bytes`
`0x718A9` (752 B, the full arm), `0x7131C` (704 B, the head/entry paths),
`0x73B60` (96 B, the call gate), `0x71C94` (320 B), `0x8DC50` (112 B),
`0xCD474` (96 B), `0xCD514` (96 B), `0x7F3A1` (488 B, the tail),
`0x7F274` (301 B, the staging), `0x6E8E8` (80 B), `0x6F9E0` (240 B),
`0x7CCE4` (112 B), `0x78564` (96 B), `0x7D9A4` (240 B), `0x826B0` (176 B,
the row-0D RET + row-0E prologue), `0x82A50` (80 B), `0x82A9F` (48 B),
`0x82700`-adjacent windows, `0x8081B` (40 B), `0x76278` (48 B), `0x76500`
(120 B), `0x77EAC` (64 B), `0x8D4E0` (84 B), `0x8D540` (48 B), `0x7E7A0`
(32 B), `0x7FB78` (32 B), `0x82F58` (32 B);
`search_instructions` `operand "0x91]"` (18), `MOV EDX,0x1b` (15),
`MOV EDX,0x1c` (7), `MOV EDX,0xd` (21), `MOV EDX,0x9` (44), `MOV EDX,0xe`
(16), `MOV EDX,0x1f` (14). Engine/loader source read: `fifa96_match_run.c`
(150-215, 440-490, 600-720, 974-1045, 1130-1270, 2125-2180, 2255-2317,
3240-3250),
`fifa96_match_handlers.c` (2130-2210, 2395-2625, 2820-2945),
`include/fifa96_loader/fifa96_camera.h` (40-109, 280-400),
`src/fifa96_loader/fifa96_camera.c` (728-940),
`include/fifa96_loader/fifa96_action_handlers.h` (219-332),
`src/fifa96_loader/fifa96_action_handlers.c` (455-530),
`src/fifa96_loader/fifa96_outfield.c` (195-275),
`include/fifa96_loader/fifa96_entity_update.h` (10-90),
`include/fifa96_engine/fifa96_match_run.h` (285-300, 590-610).
No writes: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched. No git commands.
