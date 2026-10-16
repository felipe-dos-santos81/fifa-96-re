# FU-305 — the Layer-2 acceptance surface + the M2 tape impact (R11-L4)

**Phase/Provenance:** M2 phase-28 recon-ahead wave (Stage-1, task 8/8), 2026-10-16,
read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; this session `get_current_program_info` =
`watcom:LE:32:default`, image base 0, `.image` overlay, 2706 functions). Every
address/byte below is a fresh `disassemble_function` / `disassemble_bytes` /
`read_memory` / `get_xrefs_to` / `search_instructions` /
`get_function_by_address` of **this** session unless marked *carried* (then the
frozen FU doc is named). The alias traps are applied (`dword[x]>>16` = the word
at `x+2`; `>>24` = the byte at `x+3`). No writes: no rename/comment/label/
function/script/save; `/FIFA96.EXE` untouched; the ISO untouched; no git.

Scope (spec §3 item 8): the **Layer-2 acceptance surface** — the scene/render
readers of the `camera.*` fields (which presented bytes move when the camera
cells move), the `make game` screens + AE witness, the **FU196-L6 stay-out
check** (`FUN_0007417C`, spec §3 Out), the **M2 tape-side prediction** (which
cells must be non-zero for the live permit+ladder to arm, against the tape's
forced phases and the `phase==2` ladder gate) and the **re-pin decision
inputs**. This slice produces no code; its "port shape" is the acceptance
contract Stage-2 must satisfy.

---

## 1. The port shape (the acceptance contract this slice fixes)

**Contract.** The Layer-2 goal is the follow/pan camera **visibly moving** in
`make game` (spec §1/§5), staged behind the Layer-1 hard gate (a granted-frame
reach test in which real engine state arms the ladder, a non-zero `CX`
installs, the installed arms dispatch, and the camera cells move through the
real seam — with mutation-RED riders; it must fail on the phase-27 baseline).
Layer-2 evidence is the pair **(a) the SDL3 screen capture + AE witness** and
**(b) the headless cell witness** (the Layer-1 test's camera-cell assertions;
the same commit supplies the attribution — screens alone cannot separate camera
translation from entity animation). The tape policy is fixed by spec §5: **M1
immutable; M2 byte-identical through the staged tasks — re-pinned only with a
written reason + frame diff; a live path that legitimately arms on the natural
tape stops the task for that diff first**; the census moves only on a proven
live install+dispatch with a faithful route (the FU297 dead-count discipline),
and any move updates README title + `docs/ENGINE.md` + `docs/PROGRESS.md` +
`AGENTS.md` + the test pins in the same commit.

**The presented-bytes witnesses.** On the **null backend** the presented frame
hash covers every plane row and the palette (`platform_null.c:35-45`: four
planes × `stride*240` + 768 palette bytes), so any camera move that changes any
drawn pixel changes the golden `hash=` line for every following frame until the
scene returns to a pixel-identical state. In the **SDL3 window** (960×720
integer-scaled) the established AE witness is the scene-band crop **`y < 591`**
of the 960×720 window (the HUD bar/clock sits below it; ENGINE
phase-9 pins: AE 45710/45902 for the LEFT hold, 28535 for UP, 0 for the
camera-blind RIGHT walk).

---

## 2. Which presented bytes move when the camera cells move

### 2.1 The engine render consumer (fresh source read, this session)

`fifa96_match_run_render` (`src/fifa96_engine/fifa96_match_run.c:5022`) is the
only match-scene consumer of the FU-71 camera block:

- `:5095-5098` — `cam_src = { camera.pos_x, camera.pos_y, camera.pos_z }` →
  `fifa96_render_camera_stage` (the ±0x1770 / y-clamp copy,
  `src/fifa96_loader/fifa96_render.c:4-17`), the scene origin;
- `:5105` — `camera.pos_y` also feeds `fifa96_scene_threshold` (the near-depth
  gate), so a **pos_y** change can cull/uncull sprites (`:5181`
  `fifa96_scene_slot_gate`);
- `:5128-5140` — every entity's relative vector is `slot.pos − cam`, rotated by
  the yaw/pitch matrix (`:5086`) and projected; so a **pos_x/pos_z** change
  translates **every** staged sprite's screen x/y and may reorder the
  z-sort (`:5157-5165`) → different overdraw;
- `:5239` — the actual pixel blits (the presented bytes).

The camera block is advanced once per granted frame before the entity walk:
`fifa96_camera_update_walk` (`match_run.c:4202`, the FU-71 integrator: `pos_x/pos_z
+= vel`, `pos_y = the FUN_00070B94 arc` when an event is live, `speed`/
`follow_speed`) and `fifa96_match_run_camera_follow` (`match_run.c:3769-3788`,
gated on `pan_counter==0`, `follow_speed!=0`, `event_param>0x10`, zero rates,
`tracked` = a slotted record; then `fifa96_camera_rate_event` sets `vel_x/vel_z`
from the slot dir bytes). The ladder/row path writes this block through the
ported `fifa96_camera_event_set` / `fifa96_camera_set_tracked` (FU294, wired at
`handlers.c:3871` in the row-`0x1B` head) and the FU293 pan-cell interpolation.

### 2.2 The native render reader (fresh, load-bearing)

**`FUN_00036C70`** (the native scene-staging builder; fresh
`disassemble_function`, 231 insns) copies the camera triple into the
presentation triple and builds the record list:

- `0x36C95..0x36C9F` — `MOV ESI,0x15774C` (the FU-71 camera triple) →
  `MOVSD ×3` into `0x155AA4/8/C` (the replay branch uses `[0x157A6F]` first,
  `0x36C76..0x36C93`);
- `0x36CA2..0x36CE6` — the bounds: `±0x1770` for x/z (else `-0xFA0`), y clamped
  `[0, 0x3E80]` — exactly the ported `fifa96_render_camera_stage`;
- `0x36D1A..0x36E75` — the per-record staging out of `[rec+0x59]` (the +0x59
  triple → `0x155AB0` stride 0xC; `[rec+0x28][0]` → `0x155C20`; `[rec+0x3D]` →
  `0x155C37`; the `+0x9A` skip → the `-0x2710` sentinel) — the world-coordinate
  list the projector then consumes;
- `0x36D02` / `0x36D7F` — `MOV DL, byte[0x157ABE]` / `[0x157ABF]` select which
  team block is staged first (the **presentation team order**);
- `0x36F4F..0x36F61` — `word[0x1577C2]` / `word[0x1577C0]` (the pan/lead pair)
  drive the `0x155C4E` flag nibble in the staged list.

Fresh `get_xrefs_to 0x15774C` = **260** references (first 40 read: `FUN_000736AC`
the camera update, `FUN_0007131C` the boundary handler, `FUN_00071C94` the event
setter, `FUN_00072AC4`, `FUN_00072478`, `FUN_0008BAF0`, `FUN_0007BF20`,
`FUN_00070C08`, `FUN_0007997C`, **`FUN_00077EAC` the ladder**, `FUN_0006FFC0`,
`FUN_0008D098`, `FUN_00070DE0`, `FUN_00036C70`, …) — the camera triple is both
the scene origin and the ladder's stage-select input.

**Consequence.** The cells that move presented bytes are the FU-71 camera
`pos_x/pos_y/pos_z` (scene origin + near gate), the yaw/pitch (matrix — not
written by the follow/pan chain; the pose feed owns them), and the pan/lead
words only to the extent they feed the camera through the follow path. A single
camera-cell step of one world unit moves pixels in *every* drawn sprite; the
FU293 interpolation and the row-camera writes are therefore directly
screen-visible.

---

## 3. The Layer-2 acceptance procedure (screens + AE witness)

Run at the phase close (after the Layer-1 gate is green), on the acceptance
host, ISO present, SDL3 build, `DISPLAY=:1`:

1. **Interactive run.** `make game`; walkthrough exactly as the phase-8/9
   recipes: intro RETURN skip → front-end BACKSPACE (panel) → RETURN (panel
   confirm → match). Let the clock start (`00:0x` in the HUD).
2. **The moving pair.** Hold a direction key (the phase-9 precedent: a 3 s
   hold) so the follow/pan chain runs; capture
   `docs/screens/p28-<tag>-pan-a.png` (at clock second `T`) and
   `docs/screens/p28-<tag>-pan-b.png` (at the same clock second `T`, so the HUD
   text does not tick). Capture the **control pair**
   `p28-<tag>-still-a/b.png` with no key held, same second.
3. **The AE witness.** Compare the **scene band** only — the `y < 591` crop of
   the 960×720 window — so the HUD bar/clock cannot contribute:
   `compare -metric AE` on the cropped pan pair must be **> 0** (the phase-9
   scene-band magnitudes are ~2.9e4 (UP) … ~4.6e4 (LEFT); a camera pan shifts
   the whole band), and the cropped still pair must be **0**. Record both
   numbers.
4. **The attribution witness (same commit).** The screens prove *pixels moved*;
   they cannot prove the *camera* moved. The Layer-1 granted-frame test must
   assert the camera cells (`render.camera.pos_x/pos_z`, the pan cells /
   `fifa96_camera_event_set` outputs) changed across the compared frames, and
   the reach test's mutation-RED riders must be in place. If the tape arms
   (§5), the M2 frame diff is a third witness.
5. **The honest remainder.** Any part of the visible-motion chain the FU293
   remainder cannot reach is a numbered waiver (spec §5) — never a staged
   camera or a widened fixture.

---

## 4. The FU196-L6 stay-out check (spec §3 Out)

Fresh `FUN_0007417C` (0x7417C..0x741B2, `disassemble_function`, 12 insns):

```
MOV AL,[0x157AC2] ; XOR AL,[0x157AC3] ; AND AL,1
MOV [0x157ABE],AL ; XOR AL,1 ; MOV [0x157ABF],AL     ; the side-flag pair
CALL 0x7412C                                          ; the OL-13 bind
MOV EAX,0x1588A4 ; CALL 0x8C33C                       ; team 0 reset-lane
MOV EAX,0x1590D9 ; CALL 0x8C33C                       ; team 1 reset-lane
JMP 0x78824                                           ; the slot bind (tail)
```

Fresh `get_xrefs_to 0x7417C` = **4** callers: `0x4B09A` (`FUN_0004B02C`, the
match-init mode-1 arm), `0x88874` (`FUN_00088860`, the period-end), `0x92E4A`
(`FUN_00092E2C`), `0x8B832` (unowned — the act-0xA terminal family).

**It moves M2 through two independent fresh channels.**

1. `[0x157ABE]` is written **only** by this body and `0x742B4` (fresh
   `get_xrefs_to 0x157ABE` = 9 refs: WRITE `0x74189`, WRITE `0x742B4`, READs
   `0x36D02`, `0x4BAB5`, `0x74130`, `0x788B0`, `0x741B7`, `0x8BA75`, `0x370BF`).
   Its readers include **`FUN_00036C70:0x36D02`** (the presentation team order,
   §2.2) and **`FUN_000741B4`** (`0x741B7`, fresh function dump: the
   `(EAX ^ [0x157ABE]) & 1` fold the keeper-CPU ladder calls at `0x77ED9` to
   index `word[0x14C1D4]/[0x14C1D6]`). Wiring the sink therefore re-orders the
   presented scene **and** flips the ladder's bit-9 index.
2. The two `FUN_0008C33C` calls are the reset-lane pass whose `0x8C3BE` tail is
   the FU295 actor lift (`[0x157A83] = team->target` on the controlled side;
   carried FU295 §2.2; the engine port `fifa96_match_entities_reset_lane` is
   caller-less today, FU295-L1). A live `[0x157A83]` un-gates
   `team_select_intercept` (`src/fifa96_engine/fifa96_match_entities.c:611-685`;
   the guard at `:621`), which writes the nearest record's target triple and
   `team->flag7be` every phase-2 frame.

**Verdict: STAY OUT (spec §3 Out stands).** The body writes **no camera cell**
(its only stores are `0x157ABE/BF`; the callees' documented writes are the OL-13
bind, the reset-lane record state and the slot bind — carried FU199/FU295/FU-70),
so it **cannot produce the Layer-2 visible motion**, which rides the
ladder → row-`0x1B` → camera-setter path and the FU-71 follow update. Wiring
it can only move M2 through the two channels above; if a later stage ever needs
it, spec §5's re-pin rule governs (written reason + frame diff first).

---

## 5. The M2 tape-side prediction and the Stage-2 check

### 5.1 The arm cells (fresh bytes)

The record-0 machine's gates (fresh `disassemble_bytes 0x784C0`, 240 B):

- **permit** `0x784D3..0x784E9`: `MOV EAX,[EBP+0x8E]; SAR EAX,0x18` = byte
  `[rec+0x91]` (the code) → `MOV AL,[EAX+0x110680]; AND AL,1; JZ 0x7859E` — the
  permit gate skips **both** the install tree and the ladder;
- **install tree** `0x784EF..0x78576`: `timer81!=0 → target 0x00`; else
  `phase!=2 → target 0x19`; else `ball_owner==0 → 0x19`; else `owner_rel_5!=0 →
  0x19`; else `code==5 ? 5 : 4`; the same-code guard `0x78560`; on a change
  `[0x157AB2]=1` + `CALL 0x7D9A4`;
- **ladder gate** `0x7857B..0x78599`: `[0x157A4A]>>0x18 == 2` (`0x7857B`),
  `dword[EBP+0x20]==0` (`0x78588`), `byte[0x157C5D]!=0` (`0x7858E`) → `CALL
  0x77EAC` (the ladder); `0x785A2` clears `[0x157C5D]`; `0x785A8` dispatches
  the row (`CALL dword[EBP+0x18]`).

**P1 is a static const table, not a produced cell (new, fresh).**
`read_memory 0x110680` (96 B) = `03 00 00 03 03 03 03 02 00 … 02 00 01 02 02 00
02 02 02 02 … 01 00 00 01 00 01 01 01 … 03 01`. Fresh
`search_instructions` operand `0x110680` scans 234 988 instructions and returns
**19 matches, every one `MOV reg, byte ptr [reg+0x110680]` (a read)**; fresh
`get_xrefs_to 0x110680` = 19 (the same sites). **There is no writer instruction
in the program.** The permit=1 code set (bit 0) is
`{0x00, 0x03, 0x04, 0x05, 0x06, 0x19, 0x2F, 0x44, 0x47, 0x49, 0x4A, 0x4B,
0x5D, 0x5E}`. So "porting P1" (FU298) is *reading a const table* — deterministic
for every code, unlike a produced cell.

The ladder prologue (fresh `0x77EAC..0x77F4B` + `0x77F90..0x78051`):
`code==0x19` (`0x77EC2`); the side fold `CALL 0x741B4` + `word[EAX*2+0x14C1D4]`
+ `AND AH,2` (`0x77EDE..0x77EF0`); `+0x9A==0`; `[[rec+0x28]][0]!=0x54`;
`timer81!=0 → the +0x59→+0x4D pos copy` (`0x77F20`); the actor/side skip
(`0x77F2E..0x77F4B`); the **AX prepass** `SI<0x420` (`0x77F9A`),
`DI>0x7B0` (`0x77FA5`), then `[0x157A49]>>0x18 == 1 ? cam_z>0 : (cam_z>0) ^
(side==0)` (`0x77FAD..0x77FF0`); then the **CX machine**: `CX=1` needs
`[rec+0x69]>>16 < 0x20`, `dword[0x157750] < 4`, `dword[0x1577BC]>>16 (= word
0x1577BE)` `< 0xA` (`0x78032..0x78051`), else the CX=4 arm / stage-table path
(FU290, carried).

### 5.2 The tape's supply (fresh test/golden reads)

- Forced phases: `m 1` → 0x13, `m 21` → 0x14, `m 41` → phase 2 + the 1 s period
  + the staged rows 00/04/06/07/08/0F/18/1E/21/23 on team 0 records 1..10
  (`tests/test_engine_m2.c:1755-1761, 1916-1919, 2356-2568`). No other phase is
  forced.
- Phase-2 windows (`awk` over `tests/golden/engine/m2-frames.txt`, 6522 lines):
  frames **46..65, 773..845, 1803..2502, 2703..3402, 3603..4302** — **2193**
  `state=2` lines total; the `phase==2` half of the ladder gate is *satisfied
  by the tape's own forced/natural schedule*.
- The camera: fresh pin at the mechanics entry — `pos_x=pos_y=pos_z=0`,
  `vel_x/vel_z/timer/pan_counter/... = 0` (`test_engine_m2.c:2482-2492`); the
  **m 42 band scenario parks it at `(0, 0, 0xB00)`** and runs the natural pan
  (`test_engine_m2.c:2622`, `fifa96_camera_init(..., 0, 0, 0xB00)`). `0xB00 >
  0x7B0` — so the AX prepass's **distance half opens only after m 42**, and the
  `(cam_z>0) ^ (side==0)` fold (with `[0x157A4C]!=1`) admits **the `side==0`
  keeper** (team 0, record 0).
- The interlock: row `0x19` writes `[0x157C5D]=1` in its prologue
  (`src/fifa96_engine/fifa96_match_handlers.c:4097`; fresh `get_xrefs_to
  0x157C5D` = 3: READ `0x7858E` + WRITE `0x785A2` inside `FUN_000782D0`, WRITE
  `0x74707` in the row-`0x19` body). The row already dispatches on the tape
  (M2_WIRED_MASK bit `0x19`, `test_engine_m2.c:1883-1889`), but the current
  ordering clobbers it (`match_run.c:4249` reads → `keeper_cpu.c:80` clears →
  `match_run.c:4257` writes back), so the ladder gate is always false until
  **FU299** fixes the read→step→write order (or publishes in-frame like the
  `[0x157C5C]` `latch_written` precedent).

### 5.3 The prediction (per Stage-2 task)

| cell / gate | native read (fresh) | must be | the tape's supply |
|---|---|---|---|
| permit | `[0x110680+code]&1` `0x784DC` | `!=0` | static table; record-0's code decides; `{0,3,4,5,6,0x19,…}` permit |
| phase | `[0x157A4A]>>24==2` `0x7857B` | `==2` | **satisfied** (2193 `state=2` frames) |
| slot | `dword[rec+0x20]==0` `0x78588` | `==0` | record 0 carries no slot (only the taker does) — census L1 |
| interlock | `[0x157C5D]!=0` `0x7858E` | `!=0` | row-`0x19` dispatch writes it each phase-2 frame — **dead until FU299** |
| ladder head | `code==0x19` `0x77EC2` | `==0x19` | FU298's install tree produces it on record 0 (`phase!=2 → 0x19`; `phase==2 ∧ ball_owner==0 → 0x19`) |
| bit-9 gate | `word[0x14C1D4/6]` bit 9 `0x77EE0..EF0` | `0` | the tape's `input_range` is 0 (`test_engine_m2.c:2524`) — **open** |
| AX prepass | `|cam.x|<0x420 ∧ |cam.z|>0x7B0` + side fold `0x77F9A..0x77FF0` | `ax=1` | camera `(0,0,0)` until m 41 (closed); parked `(0,0,0xB00)` at m 42 → **opens for the team-0 keeper** in the later phase-2 windows |
| actor skip | `[0x157A83]!=rec` + side `0x77F2E..0x77F4B` | pass | the actor is the slotted taker, not record 0 — census L1 |
| `CX` | the CX=1 / CX=4 / stage-table arms `0x78038..0x781E1` | `!=0` | CX=1 arm: `lane<0x20`, `cam_y<4`, `follow_speed<0xA` (camera parked y=0; the pan's 5-unit velocity keeps follow_speed `< 0xA`) — the post-m-42 windows are the arm-risk windows |

**Prediction.** (1) **FU298 alone** turns the install tree live for record 0 on
*every* granted frame (the `phase!=2 → 0x19` arm fires on the forced 0x13/0x14
windows, the first granted frame included): the record-0 code flips `0 → 0x19`
and the row-`0x19` body starts dispatching per phase-2 frame. Whether the
presented hash moves **on the same frame** is measured, not assumed (the v15
precedent: a row-`0x19` dispatch at the tape's state was byte-exact); the
phase-2 row-`0x19` path (guard/predictor/carrier target writes) is the first
genuine move risk. (2) **The ladder installs** (CX≠0 → codes 0x1A/0x1B/0x1C/4)
become reachable on the tape only in the **post-m-42 phase-2 windows** (the AX
distance half), for record 0 of team 0, and only with FU299's interlock fix and
the stage-select inputs surviving (the FU292 latch writers are still unported —
the stage-table path refuses on `delta <= lim`, keeping CX=0 there; the CX=1
arm is the reachable one). If it fires, row-`0x1A`/`0x1B` bodies write targets /
camera cells → **M2 moves legitimately** → stop for the diff (spec §5) before
anything else. (3) The early phase-2 window (frames 46..65, camera at the
origin) **cannot** arm the ladder itself — the distance half is closed.

### 5.4 The Stage-2 check commands

```
make check                                                             # the gate
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt       # M1 immutable
./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt       # M2 byte-identity
```

Run after **each** Stage-2 task (the tasks land one-per-commit). On a `cmp`
failure: capture the byte offset → the first divergent frame, `diff` the two
transcripts and list the affected `state=` windows, verify M1 is unmoved, and
classify — *legitimate live-path arm* (the §5.3 prediction) vs *regression* —
before touching anything. A **legitimate** arm stops the task and runs the
re-pin protocol (§6).

---

## 6. The re-pin decision inputs

1. **M1 immovable.** `tests/golden/engine/m1-frames.txt`, sha256
   `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` — never
   moves; verify with the `cmp` above.
2. **M2.** `tests/golden/engine/m2-frames.txt`, sha256
   `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a`, 6522
   lines — re-pinned **only** with (a) the written reason (the native path +
   its fresh evidence: the 0x784DC table read / the §5.3 gate), (b) the frame
   diff (first divergence, affected windows, `head -N new | cmp - old` prefix
   proof), (c) M1 unmoved, (d) the classification above.
3. **The census.** `59/80 = 73%` (`36 action + 23 phase`) holds unless a proven
   live install+dispatch with a faithful route moves it; a move updates the
   README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` +
   `tests/test_engine_m2.c` pins **in the same commit** (spec §5).
4. **The Layer-2 artifacts.** The §3 screens + AE numbers and the Layer-1
   camera-cell assertions land with the phase close-out; if the tape stays
   byte-identical, the Layer-2 witness rests on those two only (the
   tape-dormant convention).

---

## 7. Evidence table

| # | claim | fresh evidence (this session unless *carried*) |
|---|---|---|
| 1 | program = `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image`, 2706 fns | `get_current_program_info` |
| 2 | `FUN_0007417C` body: side pair → `0x7412C` → 2× `0x8C33C` → `JMP 0x78824` | `disassemble_function 0x7417C` (12 insns) |
| 3 | 4 callers of `0x7417C` | `get_xrefs_to 0x7417C` = 4 (`0x4B09A`/`0x88874`/`0x92E4A`/`0x8B832`) |
| 4 | `[0x157ABE]` writers = `0x74189` + `0x742B4`; readers incl. `0x36D02`, `0x741B7` | `get_xrefs_to 0x157ABE` = 9 |
| 5 | the ladder's side fold reads `[0x157ABE]` | `disassemble_function 0x741B4` (`MOV DL,[0x157ABE]; XOR; AND 1`) |
| 6 | `FUN_00036C70` copies `0x15774C` → `0x155AA4`; bounds ±0x1770 / y [0,0x3E80]; team order at `0x36D02/D7F`; pan pair `0x36F4F/F59` | `disassemble_function 0x36C70` (231 insns) |
| 7 | `0x15774C` has 260 consumers | `get_xrefs_to 0x15774C` = 260 (first 40 listed) |
| 8 | permit gate `0x784DC` = `[rec+0x91]` → `[0x110680]&1`; install tree `0x784EF..0x78576`; ladder gate `0x7857B..0x78599`; clear `0x785A2` | `disassemble_bytes 0x784C0` (240 B) |
| 9 | `[0x110680]` is static; **no writer**; 19 read sites | `read_memory 0x110680` (96 B); `search_instructions 0x110680` = 19 (all `MOV reg,[reg+0x110680]`); `get_xrefs_to 0x110680` = 19 |
| 10 | permit=1 codes `{0,3,4,5,6,0x19,0x2F,0x44,0x47,0x49,0x4A,0x4B,0x5D,0x5E}` | derived from #9 (bit 0) |
| 11 | ladder prologue: `code==0x19`; `CALL 0x741B4` + `word[EAX*2+0x14C1D4]` + bit 9; `+0x9A`; `0x54`; `timer81` → pos copy; actor skip | `disassemble_bytes 0x77EAC` (160 B) |
| 12 | AX prepass `SI<0x420 ∧ DI>0x7B0` + `[0x157A49]>>24`/`[0x157754]` side fold; the AX==0 ∧ lane<0x40 arm `FUN_00092AC8`/`FUN_0006DC88`; CX=1 bytes | `disassemble_bytes 0x77F90` (120 B) + `0x78003` (80 B) |
| 13 | `FUN_0006DC88` = `undefined F(int*, short)`, body `0x6DC88..0x6DCC7` | `get_function_by_address 0x6DC88` |
| 14 | `[0x157C5D]`: one read + one clear in `FUN_000782D0`, one write at `0x74707` (row `0x19`) | `get_xrefs_to 0x157C5D` = 3 |
| 15 | render consumes `camera.pos_*` → cam_src → rel → project → blit; `pos_y` → near gate | `match_run.c:5022,5095-5105,5128-5140,5181,5239` |
| 16 | camera advanced once per granted frame (`update_walk` + `camera_follow`); follow gates | `match_run.c:4202-4215,3769-3788` |
| 17 | Seam A staging: `permit` = cpu view (`:766`), `side_xor` (`:765`); `e->code` overwritten by installs (`:781-783`) | `fifa96_match_entities.c:732-792` |
| 18 | the machine's gates: permit `:50`, ladder `phase==2 ∧ queued20==0 ∧ interlock!=0` `:65`, clear `:80`; stage-select AX gate `:139`, CX=1 `:156`, stage-table refusal `:189-197` | `fifa96_keeper_cpu.c:27-82,115-208,210-299`; *carried* FU283/FU290/FU291 |
| 19 | row-`0x19` prologue writes `[0x157C5D]=1` | `handlers.c:4097`; *carried* FU-179 |
| 20 | present hash = planes × `stride*240` + palette | `platform_null.c:35-45` |
| 21 | camera stage = the ±0x1770/y clamp | `fifa96_render.c:4-17` |
| 22 | m 41 pins camera `(0,0,0)`; m 42 parks `(0,0,0xB00)`; forced phases/rows; `input_range=0` | `test_engine_m2.c:1755-1761,1916-1919,2356-2568,2482-2524,2622` |
| 23 | phase-2 windows 46..65, 773..845, 1803..2502, 2703..3402, 3603..4302 (2193 `state=2` lines of 6522) | `tests/golden/engine/m2-frames.txt` (`awk` state map) |
| 24 | M1 `09b726b7…`, M2 `b281a9b2…` (6522 lines) | `sha256sum` of the two goldens |

---

## 8. Numbered legs (each with its native owner)

1. **FU305-L1 — the record-0 census on the m2 tape** (the machine's inputs:
   `code`/`timer81` for the install tree; `phase`; and for the ladder head /
   gate `has_slot` (`+0x20`), `[rec+0x28][0]`, `lane69`, `timer81` across the
   phase-2 windows). Not derivable from the hash transcript; owner: the
   Stage-2 FU298 task's pre-change census (native owner `FUN_000782D0`
   `0x784C0..0x785A8`).
2. **FU305-L2 — the tape-side verdict on the AX prepass + CX=1 arm** for the
   post-m-42 windows (does the parked `0xB00` camera + the side fold + the
   lane/cam_y/follow_speed gates produce a non-zero `CX` on the tape?).
   Owner: `FUN_00077EAC` (`0x77F9A`, `0x77FA5`, `0x78032..0x78051`); decided by
   the Stage-2 `cmp`.
3. **FU305-L3 — the `[0x157A4C]` value on the tape** (the AX branch select
   `0x77FB3`); owner `FUN_00077EAC:0x77FAD..0x77FEE` / the FU-217 sit-8 arm
   (`byte[0x157A4C] != 1` guard, off-tape while the bit-8 gate is dead).
4. **FU305-L4 — the unported `AX==0 ∧ lane<0x40` arm**: the native
   `0x78009..0x7802A` calls `FUN_00092AC8` (the RNG) and
   `FUN_0006DC88(rec, 2 + (rng&1))`; the engine returns at `0x77FF5`
   (`keeper_cpu.c:148`) and never models it. Owner: `FUN_0006DC88`
   (`0x6DC88..0x6DCC7`) + `FUN_00092AC8`; matters once the ladder head goes
   live.
5. **FU305-L5 — the interactive visible-motion trigger without the FU293
   interpolation**: the follow/pan chain needs a live event source (the
   every-frame auto-camera's `[0x1577EE].hi > 0xF0` arm and the FU293 pan
   writers); owners: `FUN_00071DF4` first arm + FU293-L1/L3 (carried).
6. **FU305-L6 — the AE attribution limit**: the scene-band AE cannot separate
   camera translation from entity animation; the Layer-1 camera-cell assertions
   (same commit) are the attribution witness. Owner: the render staging
   `FUN_00036C70:0x36C9F` (the camera-triple copy) + the acceptance procedure.
7. **FU305-L7 — the second `[0x157ABE]` writer `0x742B4`** (no Ghidra
   function; the period-end family — the FU196-L6-adjacent class). Owner:
   `0x742B4` (resolve the containing function; carried FU196-L6); if wired it
   moves the presentation team order and the ladder side fold like `0x7417C`.

## Review erratum (wave review, 2026-10-16)

Wave review adjudication: the state-indexed permit table ends at index `0x2C` (45 bytes; FU298 wins). The extra "permit=1" codes {0x2F,0x44,0x47,0x49,0x4A,0x4B,0x5D,0x5E} are mode-table bytes (`0x1106AD` = idx 0x2D opens mode table 1; `0x1106C3` opens table 2), not permit entries. The Stage-2 P1 port bounds reads to ≤0x2C; FU298-L1 stays open.

**Verdict:** GO — state-indexed permit table ends at index `0x2C` (45 bytes; FU298 wins); the extra "permit=1" codes {0x2F,0x44,0x47,0x49,0x4A,0x4B,0x5D,0x5E} are mode-table bytes (`0x1106AD` idx 0x2D opens mode table 1; `0x1106C3` opens table 2), not permit entries; Stage-2 P1 bounds reads to ≤0x2C; FU298-L1 stays open.
