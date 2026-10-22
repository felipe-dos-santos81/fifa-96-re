# FU-310 — the camera bootstrap: every restart/place writer, its dispatch reach, and the GO/NO-GO

**Phase/Provenance:** M2 phase-30 Stage-1 recon-ahead wave (Task 1/4), 2026-10-21,
read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument). This session's `get_current_program_info` =
`FIFA96.EXE`, `watcom:LE:32:default`, image base 0, `.image` overlay,
**2708** functions. Every address/byte below is a fresh `disassemble_bytes` /
`decompile_function` / `read_memory` / `get_xrefs_to` of **this** session unless
marked *carried* (then the source is named). The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`); LE globals are
quoted at their runtime/Ghidra addresses (the `+0x100000` rule concerns the flat
file offset, not these). No writes: no rename/comment/label/function/script/save;
`/FIFA96.EXE` untouched; the ISO untouched; no git.

Question (plan Task 1 / spec §3 item 1): **which natural route first takes the
camera place/Y off `(0,0,0)` in the port, and what does it cost?** For each
writer of the camera triple in the restart/place set: the native dispatch row +
gate, the engine's dispatch reach (is the row body actually reached in natural
play?), the missing producer, and the cost.

---

## 1. Verdict in one line

**The only camera writer that is live in the port today is the row-04 half-line
pan (`0x7ED89..0x7EDE3` → `FUN_00071C94` → `FUN_000736AC`), and it moves the
*place* only (`pos_x/pos_z`; never Y). Every restart cut in the census is
faithful-or-nearly-faithful code that is dispatch-dead from `(0,0,0)`, and the
three independent upstream gates are the reason: (a) the goal scanner
`situation 2/3/4` producer needs `goal_armed`, whose armer (`0x713A6..`) needs
the camera out of bounds `|x| > 0x730 || |z| > 0xB20`; (b) the free-kick/penalty
hand-off aborts under the ordinary byte-0 session at `0x89487`, so only a kind-3
(offside) contact ever writes phase 6/7; (c) the keeper band's ladder AX prepass
needs `|cam_x| < 0x420 && |cam_z| > 0x7B0`, which `(0,0,0)` fails. **GO on the
pan as the first place off zero (zero producer cost, measured live); NO-GO on
the restart/place family as a *root* bootstrap.** The cheapest *new* work is not
a camera port: it is the witness + one upstream gate (the armer's live
precondition, FU269-L2), exactly the spec §6 route-analysis outcome.

---

## 2. The native writer census (fresh, this session)

Fresh `get_xrefs_to 0x700F4` = **29** `UNCONDITIONAL_CALL` sites (the lean place
reset; no DATA refs). Attributing all 29 by their containing body (fresh
`get_function_by_address` / the action table `0x1106E0`, carried FU-149 §1.6 /
FU-269 §1.1):

| # | site | containing body | triple placed | Y? |
|---|---|---|---|---|
| 1 | `0x7000F` | `FUN_0006FFC0` (reset helper) | pass-through | — |
| 2 | `0x6FAAC` | `FUN_0006E8E8` aux-0x32 arm | `[EBP+0x59/5D/61]` = rec pos, flag 1 | rec.y |
| 3 | `0x71CC3` | `FUN_00071C94` (event setter) | current triple (preserve) | — |
| 4 | `0x7405E` / `0x7426D` / `0x74573` | `FUN_00074034` + helpers (match begin) | `[0x10F328/2C/30]` = 0 | 0 |
| 5 | `0x8889F` | `FUN_00088860` (phase-0x0C reset) | `[0x10F328/2C/30]` = 0 | 0 |
| 6 | `0x92EBF` | `FUN_00092E2C` (match installer) | begin triple | 0 |
| 7 | `0x7DC01` | row `0x01` kickoff body | kickoff triple = 0 | 0 |
| 8 | `0x88E6A` / `0x88FA0` | act-1 kickoff bodies | `[0x10F328/2C/30]` = 0 | 0 |
| 9 | `0x87C49` | row `0x14` stage 2 (celebration release) | 0/0/0 | 0 |
| 10 | `0x8B737` | `FUN_0008BAF0` (period/ball machine) | current triple | — |
| 11 | **`0x8D301`** | `FUN_0008D098` **phase-3 arm** | `[0x15777C/80/84]` (goal snapshot) | snap.y |
| 12 | **`0x8D3CA`** | phase-4 arm | `FUN_0007D360(snap)` = ±0x710/0/±0xB00 | **0** |
| 13 | **`0x8D511`** | phase-6 arm | `FUN_00073DC4` spot 0/0/±0x8D0 | **0** |
| 14 | **`0x856CB`** | row `0x10` stage 0 (throw-in snapshot) | `0x15777C` triple | snap.y |
| 15 | **`0x8596B`** | row `0x10` stage 3 (throw-in cut) | rec.pos + octant<<6, **Y := 0x80** | **0x80** |
| 16 | **`0x85E98`** | row `0x11` stage 1 (corner) | ±0x710/0/±0xB00 | **0** |
| 17 | **`0x83EC2`** | row `0x12` stage 0 (free kick) | `0x158897` incident triple, z clamped ±0x9F0 | incident y (0) |
| 18 | **`0x84B6B`** | row `0x13` head (penalty) | 0/0/±0x8D0 | **0** |
| 19 | **`0x84F84`** | row `0x20` arm 0 (restart commit) | 0/0/0x8D0 | **0** |
| 20 | **`0x74F99` / `0x750B5`** | row `0x1D` stages 0/1 (goal kick) | `0x1104F2 + side*0x18 + 12*flag` = ±0x1D0/0/±0xA00 | **0** |
| 21 | **`0x755C7` / `0x75976`** | row `0x1E` head / stage-3 hold | rec.pos + offset, Y = rec.y + 0x38 (head) | **0x38** |
| 22 | **`0x76DBA` / `0x77542`** | row `0x1B` head (phase 0xD) / stage 2 | rec.pos + sector<<4, Y = rec.y + 0x38 | **0x38** |
| 23 | `0x77D82` | inside the `0x77xxx` keeper band (helper `0x8DC68`) | a 4-push triple, flag 0 | unproven (L4) |

Fresh bytes of the four cuts this task names (all quoted verbatim):

```
; row 0x20 arm 0 (0x84F5C..): the "restart cut" — Z only, Y := 0
0x84f5c  ba4c771500     MOV EDX,0x15774C
0x84f61  31c0           XOR EAX,EAX
0x84f63  892d837a1500   MOV [0x157A83],EBP
0x84f69  e856eefeff     CALL 0x73DC4        ; FUN_00073DC4(EAX=0): *p=0, p[1]=0, p[2]=0x8D0
0x84f6e  6a00           PUSH 0              ; flag
0x84f70..0x84f83       PUSH [0x157754] / [0x157750] / [0x15774C]
0x84f84  e86bb1feff     CALL 0x700F4
```

`FUN_00073DC4` (fresh decompile): `param_2[1] = 0; if (param_1 == 1) { *p = 0; p[2] = 0xfffff730; return; } *p = 0; p[2] = 0x8d0;` —
so the row-0x20 cut is `(0, 0, 0x8D0)`, and the penalty head's call
(`0x84B4F CALL 0x73DC4` with `EAX = [team+0x826]`, fresh `0x84B20..0x84B54`)
gives `(0, 0, ±0x8D0)`.

```
; throw-in stage-3 cut (0x85902..0x8596B) — Y := 0x80, preserved
0x85904  bf4c771500     MOV EDI,0x15774C
0x85911  b880000000     MOV EAX,0x80
0x85916  a5 a5 a5       MOVSD x3 (ESI = rec+0x59)
0x85919  a350771500     MOV [0x157750],EAX  ; *** 0x80 ***
0x85927  6a00           PUSH 0
0x85950  8b3d50771500   MOV EDI,[0x157750]  ; the just-written 0x80
0x8596b  e884a7feff     CALL 0x700F4
```

```
; corner stage-1 (0x85E7D..0x85E98)
0x85e7d  8d55dc         LEA EDX,[EBP-0x24]
0x85e80  b87c771500     MOV EAX,0x15777C
0x85e85  e8d674ffff     CALL 0x7D360        ; fresh decompile: *p=0x710*sign(snap.x); p[1]=0; p[2]=0xB00*sign(snap.z)
0x85e8a  6a00           PUSH 0
0x85e98  e857a2feff     CALL 0x700F4
```

```
; free-kick stage 0 (0x83E72..0x83EC2)
0x83e72  bf4c771500     MOV EDI,0x15774C
0x83e77  be97881500     MOV ESI,0x158897    ; the incident triple
0x83e7c  a5 a5 a5       MOVSD x3
0x83e85  81fff0090000   CMP EDI,0x9F0       ; z clamp
0x83e8d/0x83ea1        MOV [0x157754],±0x9F0
0x83eab  6a00           PUSH 0
0x83ebb..              PUSH [0x15774C] / [0x157750] / [0x157754]
```

```
; row 0x1D (goal kick) stage 0/1 triple table — Y := 0 in all four entries
0x74ec3..0x74f10   ESI = [0x157C5E] + side*0x18 + (([0x15777C] >= 0) ? 0xC : 0)
                   MOVSD x3 -> [EBP-0x28..-0x18]   ; the local triple
0x74f83  6a00      PUSH 0
0x74f99  e856b1ffff CALL 0x700F4
0x7509a  8a..9b    MOV AL,[rec+0x9B] ; PUSH EAX = the pause flag
0x750b5  e83ab0ffff CALL 0x700F4
```

`FUN_000744F4` (fresh decompile) writes `[0x157C5E] = 0x1104F2`; fresh
`read_memory 0x1104F2` (48 B) = the four triples
`(-0x1D0, 0, -0xA00)`, `(0x1D0, 0, -0xA00)`, `(-0x1D0, 0, 0xA00)`,
`(0x1D0, 0, 0xA00)` — i.e. **the goal-kick cut is (±0x1D0, 0, ±0xA00)**.

Fresh `read_memory 0x10F328` (12 B) = `00000000 00000000 00000000` — the
kickoff triple is the image-zero static, as carried (FU-157 §1).

**What this census settles about the "goal-kick row" (FU-220's "row20").** The
`0x84F69`→`0x84F84` cut is **row `0x20` arm 0** (body `0x84EEC`, arm table
`0x84ED0[0] = 0x84F5C`), the act-7/phase-`0x1D` **restart-commit** cut
(`(0,0,0x8D0)`, flag 0). It is *not* the goal kick: situation 4 → phase 8 →
`FUN_0008D098`'s phase-8/9 arm installs **row `0x1D`** on the controlled team's
record 0 (fresh `0x8D5D9..0x8D62C`: `MOV ECX,3`; `ADD EAX,0x1D` where
`EAX = (phase == 9)`; `CALL 0x7D9A4`), and row `0x1D` places the camera itself
at `(±0x1D0, 0, ±0xA00)`. FU-220's "row20" attribution is correct for the
`0x84F69` cut; the goal-kick cut lives in row `0x1D`.

---

## 3. The three upstream gates (fresh bytes)

### 3.1 The goal scanner's armer needs the camera out of bounds

Situation 2/3/4 have exactly one open-play producer in the port and the native:
the goal-mouth scanner `FUN_00088940` (carried FU-149 §1.3/FU-145). It is gated
on the armed state; fresh `0x71390..0x713BB`:

```
0x71390  a14a7a1500   MOV EAX,[0x157A4A]
0x71395  c1f818       SAR EAX,0x18
0x71398  83f802       CMP EAX,0x2          ; phase 2
0x7139b  7409         JZ  0x713A6
0x7139d  83f810       CMP EAX,0x10         ; or phase 0x10
0x713a0  0f8503050000 JNZ 0x718A9
0x713a6  8b44240e     MOV EAX,[ESP+0xE]    ; abs(z) word
0x713aa  c1f810       SAR EAX,0x10
0x713ad  3d200b0000   CMP EAX,0xB20
0x713b2  7f12         JG  0x713C6          ; |z| > 0xB20 -> arm
0x713b4  8b44240a     MOV EAX,[ESP+0xA]    ; abs(x) word
0x713bb  3d30070000   CMP EAX,0x730        ; |x| > 0x730 -> arm
```

The engine reproduces it (`fifa96_match_goal_arm`
`src/fifa96_engine/fifa96_match_run.c:3155-3191`, the gate at `:3176-3178`), and
the call site is the per-frame OOB branch (`match_run.c:4400-4402` →
`fifa96_camera_out_of_bounds`, `src/fifa96_loader/fifa96_camera.c:189-191`:
`|x| > 0x6C0 || |z| > 0xAB0`). At `(0,0,0)` this never fires, so
`fifa96_match_run_goal_scan` returns at its first gate (`match_run.c:3458`).

**No set-piece place arms the scanner**: the largest corner/goal-kick place is
`0xB00 < 0xB20` in z and `0x710 < 0x730` in x; the penalty/row-0x20 place is
`0x8D0 < 0xB20`; the goal-kick place is `0xA00 < 0xB20`. Only the pan/follow can
cross the threshold. (A throw-in taker standing at |x| ≈ 0xB10 would arm — but
reaching a throw-in already requires the scanner. The circle is closed.)

### 3.2 The free-kick/penalty hand-off aborts under the byte-0 session

The foul chain (`row 0x0B` duel → `row 0x0C` contact → `sit 9` → act 2) is
ported and input-drivable (see §5), but the act-2 body aborts before the phase
6/7 decision. Fresh `0x8943C..0x894BE`:

```
0x8944a  83f803       CMP EAX,0x3              ; the contact kind
0x8944d  741a         JZ  0x89469
0x8944f  ba49000000   MOV EDX,0x49             ; kind != 3 -> ball row 0x49 (flags 0 -> [0x158882]=0)
0x89464  e8bb52feff   CALL 0x6E724
0x89469..0x8947f      [0x158818]=0; stage 2 -> 3
0x8947a  a02ac31400   MOV AL,[0x14C32A]
0x89487  0f8488010000 JZ  0x89615              ; *** byte-0 session: abort ***
0x894a2  803d8288150000 CMP byte[0x158882],0
0x894a9  7519         JNZ 0x894C4
0x894ab  a08c881500   MOV AL,[0x15888C]
0x894b2  83f803       CMP EAX,0x3
0x894b5  740d         JZ  0x894C4
0x894b7  803d2ac3140000 CMP byte[0x14C32A],0
0x894be  0f8451010000 JZ  0x89615
```

So the native free-kick/penalty write needs `kind == 3` (offside), or the
`[0x158882]` ball-row flag left set by the keep/reroll arm, or a byte-1 session.
The engine models exactly that compression (`match_run.c:6216`:
`if (!(contact_kind == 3 || session_gate != 0)) return 1;` — the FU-166-L6
"kind proxy"); with the live session gate 0 (`match_run.c:2376`) a natural foul
(contact kinds 1/2) holds armed at the act-2 stage-3 gate and never writes phase
7/6.

### 3.3 The keeper ladder's AX prepass needs a far place

FU-307-L1/FU-311: the stage-select AX prepass (`0x77F9A`/`0x77FA5`, engine
`src/fifa96_loader/fifa96_keeper_cpu.c:167`) is
`|cam_x| < 0x420 && |cam_z| > 0x7B0`; at `(0,0,0)` the second term fails. The
band writer itself (`0x7751B` = `rec.pos_y + 0x38`) is landed and fixture-proven
(FU-307 §3.5); only its upstream place is missing. **The AX gate is the one
precondition a set-piece place *can* satisfy**: the penalty/row-0x20 place
`(0,0,±0x8D0)` gives `|x| = 0 < 0x420`, `|z| = 0x8D0 > 0x7B0` ✔ (the corner's
`|x| = 0x710` fails the first term — the phase-30 review focus item 2
confirmed).

---

## 4. The bootstrap census table

`live?` = the row body is reached on a granted frame in the port with no staged
camera cells. `cost` = what Tier-2 must add to make the place run (0 = none).

| route | native sites | engine site | dispatch gate | live? | cost |
|---|---|---|---|---|---|
| **A. kickoff reset** | `0x88E4B..0x88E6A`, `0x88FA0`, `0x7DC01`, `0x87C49`, `0x7405E` | `match_run.c:1894` (`reset_render`), `:1645` (place drain), `handlers.c:5122` (row 0x14) | phase 1 / kickoff; row 0x14 stage 2 | yes (every kickoff) | **0** — but it *writes zero*; it is the source of `(0,0,0)`, not a bootstrap |
| **B. throw-in cut** | `0x85911..0x8596B` (Y=0x80), `0x856CB` (stage-0 snapshot) | `handlers.c:1752` (Y=0x80), `:1690` (snapshot) | row `0x10` stages 0/3; phase gate `{2,3}` (`:1663`); frame ≥ 4 (`:1730`) | **no** | sit 2 producer = the scanner (A1 gate) or row-0x10's own requeue; **circular** |
| **C. corner cut** | `0x85E7D..0x85E98` (±0x710/0/±0xB00) | `handlers.c:1837` | row `0x11` stage 1; phase gate `{2,4}` (`:1814`) | **no** | sit 3 producer = the scanner; and `|x|=0x710` fails the keeper AX gate |
| **D. free-kick cut** | `0x83E72..0x83EC2` (incident triple, y=0) | `handlers.c:1983` (y literal 0) | row `0x12` stage 0; phase gate `{2,7}` (`:1955`) | **no** | sit 9 → act 2 → phase 7; the act-2 byte-0 abort (`0x89487`); needs kind 3 (offside) |
| **E. penalty cut** | `0x84B6B`, phase-6 arm `0x8D511` (0/0/±0x8D0) | `handlers.c:2107`; `match_run.c:3016` | row `0x13` head; phase gate `{2,6}` (`:2096`); `stage92 < 5` | **no** | same act-2 gate + the penalty band; **the place would open the keeper AX gate** |
| **F. restart-commit cut (row `0x20`)** | `0x84F69`→`0x84F84` (0/0/0x8D0) | `handlers.c:5844-5851` — **no `camera_init` at all** (the camera is the *focus* stand-in) | row `0x20`; installed only by act-7 phase-`0x1D` arm 0 (`match_run.c:6538-6556`, gate `actor_1587d4`) ← sit 8 | **no** | sit 8 (period end) + the `[0x1587D4]` producer; plus the FU-220-L4/L5 camera port |
| **G. goal-kick cut (row `0x1D`)** | `0x74F99`, `0x750B5`, table `0x1104F2` (±0x1D0/0/±0xA00) | `handlers.c:679-690`; `keeper.c:816-863`; staging `handlers.c:592-639` | row `0x1D`; installed by the phase-8 arm (`match_run.c:3026-3034`) ← sit 4 ← scanner | **no** | sit 4 (scanner) **and** the port places `keeper_cam_*` + `cam_off_z = 0` instead of the `0x1104F2` triple (`handlers.c:633`) — a real re-source needed |
| **H. keeper band** | `0x7751B` (Y = rec.y + 0x38), `0x755A9`, `0x75A0F`, `0x75D69`, `0x76DB5` | `handlers.c:560-562` (drain), `:438/521` (claim), `keeper.c:833-836` | ladder CX=4 ← `|x|<0x420 && |z|>0x7B0`; row `0x1B`/`0x1E` installs | **no** | a prior place with `|z| > 0x7B0`; no writer code (FU-307-L1 = FU-311) |
| **I. the pan integrator** | `0x7ED89..0x7EDE3` (row-04 half-line), `0x71C94`, `0x736AC` `0x7386B/0x73883` | event `handlers.c:2741-2752`; integrate `match_run.c:4370` → `camera.c:110-111` | no install — inside the row-04 body; needs `lane <= 0x40 - timer81`, `lane < 0x90`, `lane <= bound`, `camY <= 0x50` | **YES** (measured `0→0x90→0x120`; FU-157 §3.4, FU-163 §1.2/§3) | **0** — assert + mutation rider only |
| **J. aux-0x32 close cut** | `0x6FA8F..0x6FAAC` (rec pos + pause flag 1) | **unported** (`match_run_record_jump_arm` has no 0x32 case) | the anim selector for rows 44/45/46/92 | **no** | FU153-T1-L1 / FU-157 R5-L2 (the live animation selector) |

---

## 5. The port shape (what is faithful, what is not)

* **The set-piece dispatch is no longer queue-blocked** — FU-163's stated reason
  is superseded. `fifa96_match_run_begin` seeds `session_gate_14c32a = 0`
  (`match_run.c:2376`, the FU-166/R14 settled Friendly value), so
  `fifa96_match_run_set_piece` takes the **direct** path
  (`match_run.c:3290-3315` skips the queue; `:3317-3331` is the `bx != 0` phase-0
  + act-8 route; `:3372` the table-2 route) and
  `match_run_situation_side` → `fifa96_match_run_phase_arm`
  (`match_run.c:2935-3061`) writes phases 3/4/6/7/8/9 and installs the taker
  rows. Only tests stage the byte-1 session (fresh grep: the live reset paths
  `match_run.c:1850/2199/2376` all write 0).
* **Row `0x10` is wired end-to-end** (`handlers.c:1655-1797`), including the
  native `Y := 0x80` (`:1748-1753` — the FU-157 §4.2 restore) and the
  record-relative octant offsets. Its stage-0 snapshot place (`:1690`) and the
  phase-3 arm's snapshot place (`match_run.c:3042`) are equally faithful.
* **Row `0x11` is faithful** (`handlers.c:1829-1848`): the ±0x710/0/±0xB00 probe
  (`:1835-1837`) matches fresh `FUN_0007D360`.
* **Row `0x12` is faithful modulo the Y source** (`handlers.c:1979-1988`): the
  native pushes the incident triple's own y (`0x83E7D` is a `MOVSD`-middle
  store, FU-307 §2.2 item 17); the engine passes a literal `0`. Observationally
  equal today because both the native adjudicator (`0x8A4D7`) and the engine
  (`fifa96_referee.c:74`) force the incident y to 0 — a fidelity nit (L1).
* **Row `0x13` is faithful** (`handlers.c:2100-2112`) and its phase-6 arm place
  (`match_run.c:3016`) matches fresh `FUN_00073DC4`.
* **Row `0x20` does not place the camera at all** (`handlers.c:5844-5851`): the
  native `0x73DC4`/`0x700F4` pair is replaced by `mr->render.camera` as the
  focus stand-in. This is the recorded FU-220-L4/L5 gap, and it means the
  restart-commit cut cannot move the camera even if the row dispatched.
* **Row `0x1D` places the wrong triple** (`keeper.c:816-863` with the staging at
  `handlers.c:592-639`): the native local triple is
  `0x1104F2 + side*0x18 + 12*flag(snap.x)`; the engine passes
  `keeper_cam_x/z` (process cells, `0` until a claim/closedown writes them) and
  `cam_off_z = 0` with the comment *"the 0x157C5E per-side table (producer
  unported, leg)"* (`handlers.c:633`). So the goal-kick place in the port is
  `(0, camY, 0)` — a no-op at bootstrap, and *actively* wrong once the camera
  has moved.
* **The pan is fully faithful and live**: the row-04 half-line arm stages
  `event_code` with height `0` (`handlers.c:2741-2743`), binds `tracked`
  (`:2752`), and `fifa96_camera_update_walk` integrates the velocity into
  `pos_x/pos_z` (`camera.c:101-111`); it never writes Y except through the arc
  (`camera.c:92-93`, gated on `event_param != 0`).
* **The fould chain is live-input-drivable**: the pad handler `0x7CE38`
  ("pressed code-0 want 0x60") installs row `0x0B` invoke-now
  (`match_run.c:702-709`); row `0x0B` arm 1 installs row `0x0C` on the nearest
  opponent (`handlers.c:3327`); row `0x0C` stage 0 calls
  `fifa96_match_run_contact` (`handlers.c:3398-3399`) → `sit 9` `BX=1`
  (`match_run.c:6776-6780`). The chain then reaches act 2 and stops at §3.2.

---

## 6. The exact first natural sequence (the pan)

Gates read fresh in the engine: the row-04 half-line branch requires the record
to be within `0x40 - timer81` of the camera and under its bound (the native
`0x7ED89..0x7EDE3`, carried FU-157 §3.2), with `camY <= 0x50` (true at 0).

1. **Kickoff** — act 1 → phase 1 (class 0) → row `0x01` → situation `0x0B` →
   phase 2 (class 1), camera `(0,0,0)` (`match_run.c:1894/2376`, FU-143).
2. **A driven record crosses the camera's near field** (any lane
   `≤ 0x40 − timer81`, `< 0x90`, `≤ bound`; natural in open play — the
   controlled record starts on the camera's centre spot).
3. **Row `0x04` stage 1 half-line arm** emits `out.events` with
   `event_code = 0x1D` (`handlers.c:2741`).
4. `fifa96_camera_event_set(cam, step_x, step_z, 0, …)` seeds `vel_x/vel_z`
   (the `FUN_00071C94` port) and `fifa96_camera_set_tracked` binds the record
   (`handlers.c:2742-2752`). *This runs in the entity chain (after the frame's
   camera update), so the move lands on the **next** granted frame.*
5. **Next granted frame:** `fifa96_match_run_frame` → `fifa96_camera_update_walk`
   (`match_run.c:4370`) integrates `pos_x += acc_x`, `pos_z += acc_z`
   (`camera.c:110-111`; native `0x7386B`/`0x73883`) → **the place is off
   `(0,0,0)`** (measured `0 → 0x90` in 24 frames, then a second slide to
   `0x120`; FU-157 §3.4, FU-163 §1.2).

**The second hop (Y) is the one that is blocked.** After step 5 the camera is at
`(±0x120, 0, 0)`: `|z| = 0 < 0x7B0` (keeper AX gate fails), `|x| = 0x120 <
0x6C0 < 0x730` (the OOB call site and the armer both fail), and the pan never
writes Y. The two Y sources that a
place could unlock are the **penalty cut** `(0,0,±0x8D0)` (opens the keeper AX
gate, then the band writes `0x38`) and the **throw-in cut** `0x80` (opens
nothing; it is itself scanner-gated). Both are behind §3.2/§3.1.

---

## 7. Legs to settle (explicit)

### 7.1 FU-163's row-`0x10` leg — **reason stale, conclusion stands**

FU-163 §4/§6(d) said: *"row 0x10 needs situation 2 → phase 3; the queue consumer
is unported"*. **Corrected:** the queue consumer is still unported (FU-269-L1),
but the live dispatch no longer depends on it — the port seeds the session gate
**0** (`match_run.c:2376`), so `fifa96_match_run_set_piece` routes to the
table-2 direct path (`match_run.c:3290-3372`) and phase 3 is written by the
phase arm (`match_run.c:3037-3045`, native `0x8D2A3`). The remaining blocker is
the **situation-2 producer**: `fifa96_match_run_goal_scan` is gated on
`goal_armed` (`match_run.c:3458`), set only by `fifa96_match_goal_arm` when the
camera is OOB (`:3176-3178`, native `0x713A6..0x713BB`). The conclusion — row
`0x10` is not live at `(0,0,0)` — survives; the cause is the armer, not the
queue.

### 7.2 FU-149's restart set — **L4 narrowed, L5 closed by decision**

* **L4** (queue vs phase-row reachability): **narrowed.** With the gate 0 the
  table-2 rows are the live route and phases 3/4/8/9 are writable; the residual
  is the *producer* (the scanner's armer), not the queue consumer.
* **L5** (session-gate producer): **closed by decision** — FU-166/R14 seeds 0
  for every ordinary played match (`match_run.c:2357-2376`); the byte-1
  (Practise) session stays a test staging (`tests/test_engine_m2.c:2893`).
* L1 (`[0x157AAC]` writer) / L6 (act-8 timeline) / L13 (taker row residue)
  stand unchanged.

### 7.3 The goal-kick row identity — **settled**

Row `0x20` (body `0x84EEC`, arm 0 `0x84F5C`, the `0x84F69`/`0x84F84` cut) is the
**restart-commit** cut `(0,0,0x8D0)`, installed by the act-7/phase-`0x1D` arm 0
(`0x899A2`) on the sit-8 actor. The **goal kick** is situation 4 → phase 8 →
row **`0x1D`** (`0x74EB0`), whose own camera cut is `(±0x1D0, 0, ±0xA00)` from
the `0x1104F2` table. FU-220's "row20" is right for `0x84F69`; it is the
restart-commit row, not the goal-kick row.

---

## 8. Numbered legs (native owner named)

* **FU310-L1 — the free-kick cut's Y source.** Native `0x83E7D` pushes the
  incident triple's y; the engine passes literal `0` (`handlers.c:1983`).
  Observationally equal while the adjudicator forces y = 0, but a faithful port
  reads `[0x15889B]`. Owner: `fifa96_match_action_12` + `0x83E72..0x83EC2`.
* **FU310-L2 — row `0x1D`'s place triple.** The engine uses `keeper_cam_*` +
  `cam_off_z = 0`; the native is `0x1104F2 + side*0x18 + 12*(([0x15777C] >= 0))`
  (`0x74EC3..0x74F10`). Owner: `fifa96_keeper_closedown_stage`
  (`handlers.c:633`) + `0x744F4`/`0x1104F2`.
* **FU310-L3 — row `0x20`'s camera cells.** `0x73DC4`/`0x700F4` are absent
  (`handlers.c:5844-5851`); the row places no camera. Owner:
  `fifa96_match_action_20` arm 0 + `0x84F69`/`0x84F84` (FU-220-L4/L5).
* **FU310-L4 — the `0x77D82` place.** An unattributed `CALL 0x700F4` inside the
  `0x77xxx` keeper band (a 4-push triple built by `0x8DC68`, flag 0). Not in the
  FU-307 Y-writer census (it is a `0x7014F` pass-through), and its owner body is
  not identified by `get_function_by_address` in the jump-table band. Owner: the
  row-`0x1A`/`0x1B` bodies; settle with a fresh function/branch walk.
* **FU310-L5 — the natural pan witness.** No granted-frame natural-play sample
  has shown the row-04 half-line gate open (`(0,0,0)` in the phase-29 tape and
  both `make game` bursts, FU-307-L1/FU-309-L1). Owner: the row-04 body
  (`0x7ED89..0x7EDE3`) + the input script; FU-313's harness.
* **FU310-L6 — the act-2 byte-0 abort is a game-design consequence.** Under the
  ordinary session the native free kick/penalty hand-off is kind-3-only
  (`0x89487`, `0x894A2..0x894BE`); the phase-30 Stage-2 plan must either accept
  a kind-3 (offside) producer or record a deliberate session decision. Owner:
  `FUN_0008A43C`/the act-2 body `0x8922C` + `[0x158882]` (`0x6E724`).
* **FU310-L7 — the armer's live precondition.** `goal_armed` has no open-play
  producer short of the pan crossing `0x730`/`0xB20` (FU-269-L2). Owner:
  `FUN_0007131C`/`0x713A6` + the pan (`0x736AC`).

---

## 9. GO/NO-GO

**GO — the row-04 half-line pan as the first natural place off `(0,0,0)`.**
Chain (every link first-hand in this tree):

```
kickoff (act 1 -> phase 1) -> row 0x01 -> sit 0x0B -> phase 2
  -> a record inside (0x40 - timer81) of the camera, lane < 0x90, <= bound, camY <= 0x50
  -> row 0x04 stage 1 half-line arm (native 0x7ED89..0x7EDE3)
  -> fifa96_camera_event_set(step, 0)          [handlers.c:2741-2743; native 0x7EFCF/0x7F0D1]
  -> fifa96_camera_set_tracked                  [handlers.c:2752; native 0x71D27]
  -> next granted frame: fifa96_camera_update_walk
     pos_x += acc_x, pos_z += acc_z             [camera.c:110-111; native 0x7386B/0x73883]
  -> the camera place is off (0,0,0)            [measured 0 -> 0x90 -> 0x120]
```

Cost: **zero producer code**; one Layer-1 witness + one mutation rider on the
row-04 event arm (the FU-157 §9 step-1 precedent). The route is *place-only*: it
never writes Y and it cannot reach the keeper AX gate or the armer, so it does
not by itself satisfy the phase-30 visible-follow goal.

**NO-GO — every restart/place route as the *root* bootstrap**:

* throw-in `0x8596B` / corner `0x85E98` / goal kick `0x74F99` — need situation
  2/3/4 ← the scanner ← `goal_armed` ← camera OOB (§3.1); no set-piece place
  arms it (all sit below `0x730`/`0xB20`). The port's goal-kick place is
  additionally mis-sourced (L2);
* free kick `0x83EC2` / penalty `0x84B6B` — need sit 9 → act 2 → phase 7/6, and
  the act-2 body aborts under the byte-0 session at `0x89487`; only a kind-3
  (offside) contact completes (§3.2, L6);
* restart-commit `0x84F84` — needs sit 8 (period end), and the port's row `0x20`
  places no camera at all (L3);
* keeper band — needs a prior `|cam_z| > 0x7B0` place (§3.3; FU-307-L1, FU-311);
* aux-0x32 — unported and selector-gated (FU-153 T1-L1).

**The cheapest honest next step** is therefore not a camera producer: it is
(a) the natural pan witness (FU-313's harness, L5), and (b) one upstream gate —
either the armer's live precondition (L7) or the offside/act-2 route (L6) — to
turn the second hop (Y / band / restart) on. The spec §6 route-analysis outcome
is the honest close if neither lands.

---

## 10. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base
  0, `.image` overlay, 2708 functions.
* `get_xrefs_to` — `0x700F4` (**29** calls, list captured), `0x744D0` (1 READ),
  `0x157C5E` (1 WRITE `0x74507`, 1 READ `0x74EE1`), `0x744F4` (2).
* `search_instructions` — operand `0x000700f4` (29 sites, 235036 scanned, not
  truncated).
* `decompile_function` — `0x73DC4`, `0x7D360`, `0x744F4`.
* `read_memory` — `0x10F328` (12 B, zero), `0x1104F2` (48 B, the four triples),
  `0x744D0` (16 B), `0x74E9C` (20 B, the row-0x1D stage table).
* `disassemble_bytes` — `0x84F5C..0x84FB0` (row-0x20 arm 0), `0x85900..0x85980`
  (throw-in stage 3), `0x85E70..0x85EB0` (corner), `0x83E60..0x83EC0` (free
  kick), `0x84B20..0x84B80` (penalty head + `0x73DC4` call), `0x88E30..0x88E80`
  (kickoff reset), `0x74EB0..0x74FB0` (row-0x1D head + stage 0), `0x75080..0x750D0`
  (row-0x1D stage 1), `0x8D2A3..0x8D310` (phase-3 arm), `0x8D5D9..0x8D632`
  (phase-8/9 arm), `0x71390..0x713C0` (the armer gate), `0x8A938..0x8A9A0` (the
  dispatcher head), `0x8943C..0x894D0` (the act-2 byte-0 abort + `[0x158882]`
  gate), `0x77D50..0x77DA0` (the unattributed place).

Engine/loader source read (read-only): `src/fifa96_engine/fifa96_match_run.c`
(1615-1664 place drain, 1850-1900 reset, 2280-2440 begin seeds, 2935-3061 phase
arm, 3121-3232 situation/armer, 3260-3420 set_piece head/goal scan, 3447-3520
scan, 3690-3760 screen steps, 4370-4492 the frame path, 5844-5900 row-0x20,
6198-6274 act-2, 6519-6574 phase-0x1D, 6731-6784 contact);
`src/fifa96_engine/fifa96_match_handlers.c` (510-690 keeper place/row-0x1D,
1305-1408 pan arm, 1600-1800 row 0x10, 1806-1935 row 0x11, 1955-2081 row 0x12,
2083-2219 row 0x13, 2380-2439 row-04 dispatch, 2731-2760 row-04 pan event,
3175-3483 rows 0x0B/0x0C, 5122 row-0x14 kickoff, 5828-5919 row 0x20, 6296-6350
action table); `src/fifa96_loader/fifa96_keeper.c` (770-889 closedown);
`src/fifa96_loader/fifa96_camera.c` (70-191 walk/reflect/OOB);
`src/fifa96_loader/fifa96_referee.c` (45-121 foul decide);
`src/fifa96_loader/fifa96_settings.c` (14-30 defaults, `value[0x0A] = 2`);
`include/fifa96_engine/fifa96_match_run.h` (874-900, 1402-1430).

Carried docs: FU-149 (§1.1-§1.8, §2, §6-§7), FU-157 (§1, §3.2-§3.4, §4.1-§4.2,
§5), FU-163 (§1.2, §2-§5), FU-166 (the byte-0 session + FU166-L6),
FU-168 (§2.2), FU-171 (§2), FU-220 (§3.2, §5), FU-269 (§1.3-§1.5),
FU-307 (§2.2, §3.5, §9.2), FU-309 (FU309-L1), `docs/ENGINE.md` (the 60/80
census line and the phase-29 register).

Status: recon complete; **this draft is the only file written**; no census
change (60/80); no golden touched; no git.

---

## Review erratum (wave review, 2026-10-21)

The frozen text above stands; only the following claims change. Fresh reads this
review (`/FIFA96.EXE`, `watcom:LE:32:default`, base 0) + fresh gdb on the
existing `build/test_engine_m2`.

1. **The pan's "live" provenance.** Quote: "**The only camera writer that is
   live in the port today is the row-04 half-line pan (`0x7ED89..0x7EDE3` →
   `FUN_00071C94` → `FUN_000736AC`), and it moves the *place* only
   (`pos_x/pos_z`; never Y).**" The pan is wired and does move the place
   through `fifa96_camera_update_walk` (`match_run.c:4370` →
   `camera.c:110-111`), but its only measurement is the **probe** of FU157
   §3.4/FU163 §1.2 (null backend + ISO; scripted kickoff → KICK → V request →
   walk to the carrier; the claim frame has record 9 at `(0,0,-2)`, lane 2;
   the camera walks `0 → 0x90 → 0x120`). It is **not** natural `make game` play
   and **not** the M2 tape: FU313's 12 000-step natural probe is 3 598 granted
   frames with every camera cell zero (`/tmp/opencode/fu313_sel`). The chain's
   first link is the **row-0x04 install from that claim/kick**, not "any record
   crosses the near field". *Stage-2 consequence:* the GO is a scripted-input
   GO under spec §5; the probe must append the claim/kick after phase 2; the
   pan is one-shot and Y-less, so it cannot satisfy the sustained-follow clause
   (P7 stays RED).

2. **The act-2 gate is not an OR.** Quote: "So the native free-kick/penalty
   write needs `kind == 3` (offside), or the `[0x158882]` ball-row flag left set
   by the keep/reroll arm, or a byte-1 session." Fresh `0x8947A MOV
   AL,[0x14C32A]; 0x89485 TEST AL,AL; 0x89487 JZ 0x89615` aborts **whenever
   the session byte is 0**, before every other term; `kind == 3` is only the
   `0x8944A CMP EAX,0x3` ball-row `0x49` choice, and `[0x158882]` is tested at
   `0x894A2` **after** the session gate. The engine's `kind == 3 || session !=
   0` (`match_run.c:6216`) is a port compression, not a native OR. *Stage-2
   consequence:* the kind-3 route is not an honest free-kick/penalty bootstrap;
   the compression is **FU310-L8**; the only qualifying keeper place
   (`(0,0,±0x8D0)`) stays natively unreachable.

3. **The armer's `|x|` branch.** Quote: "`0x713bb  3d30070000   CMP EAX,0x730
   ; |x| > 0x730 -> arm`." `0x713BB` is only the compare; the branch is
   `0x713C0 JLE 0x718A9` (skip when `|x| <= 0x730`). The operands are the words
   at `[ESP+0xC]`/`[ESP+0x10]` (abs x/z, stored at `0x71366`/`0x71383`), read
   through the `dword[ESP+0xA]`/`dword[ESP+0xE] >> 16` alias (fresh
   `0x7131C..0x7138B`, `0x71390..0x713C6`). *Stage-2 consequence:* the mutation
   rider must target `0x713C0`.

4. **The near-miss is two-layered.** Quote: "**No set-piece place arms the
   scanner**: the largest corner/goal-kick place is `0xB00 < 0xB20` in z and
   `0x710 < 0x730` in x." The call site `fifa96_camera_out_of_bounds` is
   `|x| > 0x6C0 || |z| > 0xAB0` (`camera.c:189-191`); the armer inside is
   `|z| > 0xB20 || |x| > 0x730` (`0x713A6..0x713C0`). The corner
   `(±0x710,0,±0xB00)` **does reach** the boundary handler (`0x710 > 0x6C0`)
   and fails the armer by `0x20`; the penalty `0x8D0` and goal-kick `0xA00`
   fail the call site (z < `0xAB0`) as well as the armer. *Stage-2 consequence:*
   the corner is the only set-piece that even enters the handler, and it still
   cannot arm.

5. **The cheapest next step.** Quote: "**The cheapest honest next step** is
   therefore not a camera producer: it is (a) the natural pan witness (FU-313's
   harness, L5), and (b) one upstream gate — either the armer's live
   precondition (L7) or the offside/act-2 route (L6) — to turn the second hop
   (Y / band / restart) on." L6 is refused by item 2; only L7 remains, and the
   measured pan reach (`0x120`/`0x288`) is far short of `0x730`. Whether
   repeated scripted claims can walk the camera past `0x730` (and the scanner
   then produce a situation) is unproven: new leg **FU310-L9**. *Stage-2
   consequence:* a bounded armer spike, not a committed bootstrap.

---

## Erratum — the act-2 stage conflation (T3 fix round, 2026-10-22)

The frozen text and the review erratum above stand; this append-only erratum
corrects item 2's scope. Quoted (review erratum item 2):

> "Fresh `0x8947A MOV AL,[0x14C32A]; 0x89485 TEST AL,AL; 0x89487 JZ 0x89615`
> aborts **whenever the session byte is 0**, before every other term; `kind ==
> 3` is only the `0x8944A CMP EAX,0x3` ball-row `0x49` choice, and
> `[0x158882]` is tested at `0x894A2` **after** the session gate. The engine's
> `kind == 3 || session != 0` (`match_run.c:6216`) is a port compression, not a
> native OR."

**Correction — the gate is per-stage, and `kind == 3` does pass at stage 3.**
Fresh this round (`/FIFA96.EXE`, `watcom:LE:32:default`, base 0):
`disassemble_bytes 0x894A2..0x894D1`, `read_memory 0x89214` (24 B). The stage
table entry 3 is `a2 94 08 00` = `0x894A2`, i.e. the stage-2 body's abort
re-enters at the stage-3 body, whose fresh bytes are:

```
0x894a2  803d8288150000   CMP byte ptr [0x158882], 0
0x894a9  7519             JNZ 0x894C4             ; ball-row flag -> decision
0x894ab  31c0             XOR EAX, EAX
0x894ad  a08c881500       MOV AL, [0x15888C]      ; the contact-kind cell
0x894b2  83f803           CMP EAX, 0x3
0x894b5  740d             JZ  0x894C4             ; *** kind == 3 -> decision ***
0x894b7  803d2ac3140000   CMP byte ptr [0x14C32A], 0
0x894be  0f8451010000     JZ  0x89615             ; byte-0 session -> abort
0x894c4  8a3529881500     MOV DH, byte ptr [0x158829]   ; the decision
```

The stage-2 body's abort (quoted unchanged, `0x8947A..0x89487`) happens **after**
it increments the stage byte (`0x89472 MOV CH,[0x158829]; 0x89478 INC CH;
0x8947F MOV [0x158829],CH`), so a byte-0 session splits the machine and the
next driver call runs the stage-3 body above, which tests the flag, then
`kind == 3`, then the session. The native two-call net under byte 0 + kind 3 is
therefore a **decision** on the re-entry call; `kind == 3` alone never passes at
**stage 2** (the only place the quoted sentence is true), but it does pass at
**stage 3**. `[0x158882]` remains the only unmodelled term (assumed 0,
FU166-L6).

*Port consequence:* the engine models engine stage 1 = native stage 2 (the
byte-0 split hold) and engine stage 2 = native stage 3 (`kind == 3 || session
!= 0`); the flat compression is only wrong as a **single** gate (it would apply
the stage-3 kind pass on the stage-2 call). FU310-L8 is re-scoped to that
stage distinction. Item 2's "the kind-3 route is not an honest free-kick/
penalty bootstrap" does not follow from the gate alone: under byte 0 a kind-3
(offside) contact reaches the stage-3 body on the re-entry call and decides
phase 7/6. Whether that route is a *bootstrap* depends on the offside
producer's live inputs (the live receive arm was shown reachable in FU276-L2),
which this erratum does not settle.
