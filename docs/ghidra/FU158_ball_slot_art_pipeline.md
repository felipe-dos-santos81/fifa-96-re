# FU-158 — the ball-slot art pipeline: why slot 22 resolves a PLAYART pose, and what natively draws the ball

Recon wave R6 of `docs/superpowers/plans/2026-10-11-fifa96-m2-phase13-live-camera-ball-art.md`
(FU-158, leg T2-L1 of FU-154 §9). Read-only slice: no repo file was edited except this
doc and the workspace report.

**Result in one line:** the engine's pose is the *faithful* output of the native entity
resolver for the ball slot — native row 3's `+8` bank byte is 3 and animator record 3 is
PLAYART entry 3 = `run.fsh` (first-hand) — but the pitch ball is **not drawn by that
path natively**: `FUN_00056FA4` draws it at the sorted list's value-0 sentinel through
`FUN_000572E8` using the **GAMEART0 `ball.fsh` bank (8 frames, 16×16)**, whose frame is
the low nibble of the composer-maintained byte `[0x155C4E]`, advanced while the camera
rate words are non-zero. That bank is **already staged by the engine** (live probe:
`r->banks[128]`, 8 frames, 16×16), so T2 is a render-path wiring job, not an asset job.

## Method

* Static work on the open Ghidra MCP session for `/FIFA96.EXE` (LE 32, Watcom,
  functions/decompile/disasm as cited). Every address below is a Ghidra flat address;
  LE data immediates were read at their flat home (the `+0x100000` rule).
* First-hand asset census: `/ART/PLAYART.PVI` and `/ART/GAMEART0.PVI` read from the
  read-only ISO, decoded with the committed codecs (`fifa96_refpack_decode` +
  `fifa96_bigf_parse`, throwaway probe `/tmp/opencode/r6probe.c`) and cross-checked
  with `tests/golden/gameart0.pvi` via `build/fifa96_play`.
* First-hand staging probe: `/tmp/opencode/r6banks.c` (links `build/libfifa96_engine.a`,
  runs `game/FIFAPCCD96.iso`) prints the live `r->banks[]` map.
* Alias traps observed: the `[EAX+0x5A]>>16` reads below are the word at `+0x5C`
  (`0x158848`, the ball speed word) and the `[0x109A78]`/`[0x109A74]` names are the
  *pointers* stored at those globals (image-initialized, first-hand read).

## 1. The engine chain as it stands (re-verified)

The trace named in the plan (`slot22 … anim=3 bank=3 frame=8`) is produced by:

1. `match_run_scene_stage` stages slot 22 from the ball pool: `ball->stage.anim_id =
   mr->entities.ball.appear_row` (`src/fifa96_engine/fifa96_match_run.c:1398`) and
   `e->bank_index = row.sprite_bank` from the FU-84 row (`:1419`); `appear_row` is
   seeded 3 by `fifa96_match_entities_init`/`kickoff_place`
   (`src/fifa96_engine/fifa96_match_entities.c:63,:498`).
2. `fifa96_match_run_render` resolves with `fifa96_render_resolve(anim_id=3, frame,
   direction, r->frames, bank_index=3, r->banks, …)` (`fifa96_match_run.c:4110-4112`).
3. `fifa96_render_resolve`'s default class folds `dir` (`dir>=5 → 8-dir`, mirror) and
   returns `banks[3]` at `banks[3].step * dir' + sprite`, where
   `sprite = frames[fi*5+4]` (`src/fifa96_loader/fifa96_render.c:122-170`).
4. `r->frames` is the documented **synthetic identity table** (`sprite = frame index`,
   `fifa96_match_run.c:4566-4576`), so frame 8 / dir 6 → dir' 2, sprite 8,
   `step = 60/5 = 12` → offset 32 (`fifa96_sprite_stride`, `fifa96_sprite.c:123-168`).
5. `banks[3]` is PLAYART entry 3 because `fifa96_match_run_stage` lays the player
   container (91 banks) first and GAMEART0 (60 banks) after (`:4544`, `:4578-4641`).

First-hand asset check: PLAYART entry 3 = **`run.fsh` (61904 B)**; the whole 91-name
PLAYART list (decoded this task) contains **no ball entry** (only keeper "with ball"
poses: `gstball`, `gldball`, `ghdball`, `gguball`, `gwkball`). Engine frame 24 (the
`ballrow3` dump from the phase-11 scratch) is a running-player pose — the leg's
"playart player pose" is real and is exactly `run.fsh`.

## 2. The native row path says the same thing (the pose is not a port bug)

* **Row 3 is first-hand**: `read_memory 0x10EF00` (64 B) → row 3 at `0x10EF1B` is
  `03 0B 07 03 7E E2 10 00 03` = id 3, last frame 0x0B, flags 07, next 3,
  frame table `0x10E27E`, **bank 3**. `0x10E27E`'s 12 frame records are
  `{duration 0x20, aux 0, sprite 0..0x0B}` (first-hand `read_memory 0x10E278`, 80 B).
* **The runtime row table is that image**: the only store to `0x157588` is
  `0x73D91 MOV dword [0x157588], 0x10EF00` (bytes `c7 05 88 75 15 00 00 ef 10 00`);
  `get_xrefs_to 0x10EF00` = that store only.
* **The resolver indexes the same table**: `FUN_00078FAC` (`0x78FDF EDI=[0x57588];
  EDI += id*9`, `0x78FF6 EBP=byte[[EDI+4]+ECX*5+4]`, `0x78FFD CL=[EDI+8]`, FU-85 §1)
  with the default class's `offset = rec[+4]*dir' + sprite` (FU-85 §1.1). Bank index 3
  selects animator record 3.
* **Animator record 3 is PLAYART entry 3**: `FUN_00078F1C` is called only from
  `FUN_0004AAE0 0x4AB17`; it reads resource slot `0x47` (`0x78F25 EAX=0x47; CALL
  0x4AFB8`; `0x4C0DC = 0x4BFC0 + 0x47*4`) and fills `{+0=i, +0x10=entry i, +0xC=probe,
  +0x14=handle}`; `FUN_0004AAE0` is the `art/playart.pvi` loader (`0x4AAF6 MOV
  [0x4C0DC],EAX`; names at `0x101D44` "PLAYER"/`0x101D4C`). `FUN_00078C68`/`0x78B5C`
  gives record 3's stride by `count/5` (not in the `0x18..0x5A` switch classes).
* **The ball reset installs row 3 first-hand**: `FUN_000886D4` `0x887A6 MOV
  EAX,0x1587EC` (ball block), `0x887C7 MOV EDX,0x3`, `0x887D5 CALL 0x6E724` (with
  `EBX=0`, bytes `e8 4a 0f fe ff` at the call site); `FUN_0006E724` stores
  `[obj+0x7A] = [0x157588] + row*9` (`0x6E7A2..0x6E7B6`) and the `flags&2` gate into
  `[obj+0x95]` (flat `0x158881`). Before the call the reset zeroes `0x158866` itself
  (`0x88724`), so `[0x158866]` = the row-3 pointer.
* **The composer stages that pointer as the anim id**: `FUN_00036C70` `0x36E20 MOV
  ECX,[0x158866]; 0x36E2D MOV CL,[ECX]; 0x36E32 MOV [0x55C20+slot22],CL`; the same
  block stages the position triple from `0x15880C` and the frame from `0x15887B`,
  **with no hidden marker** (`0x36E12..0x36E47`, first-hand).

**Answer to the "frame-record bank field" question:** there is none. A FU-84 frame
record is 5 bytes `{duration u16, aux u16, sprite u8}` (FU-85 §1; first-hand row-3
records above). The sprite bank comes only from the anim row's `+8` byte. The ball's
`bank=3` is `row[3].sprite_bank`; there is no second bank source in the records.

## 3. The native ball *entity* appearance machine is a generic object machine

`FUN_0008BAF0`'s tail (`0x8C055 MOV EAX,0x1587EC; CALL 0x6FE84`, first-hand) runs the
generic appearance machine on the ball every machine frame:

* `FUN_0006FE84` advances the frame counter (dword at `obj+0x8C`; its top byte at
  `obj+0x8F` = `0x15887B` is what the composer stages) and, at row end, installs a
  successor row from the word table at `0x11052A` indexed by the **high word of the
  dword at `obj+0x5A`** = the ball's speed word `[0x158848]` (`0x6FEF8 MOV
  EDX,[EAX+0x5a]; SAR EDX,0x10; ADD EDX,EDX; ADD EDX,0x11052A`; `+2` more when the
  direction byte is negative and `word[obj+0x5C] != 0`).
* First-hand `read_memory 0x11052A` (64 B): `00 50 01 50 01 44 02 40 02 38 02 34 03 30
  03 24 …` → low bytes 0,1,1,2,2,2,3,3 = successors `xstandd/walk/jog/run` for speeds
  0..7 — i.e. the ball's *entity* animation is driven through shared player locomotion
  rows; it is not a ball-art selector.
* `FUN_0006E724`'s `param_2 == 0` branch keeps rows `0x62..0x65` or rolls a random
  `0x63/0x64/0x65` (`0x6E739..0x6E78C`, decompiled + disasm) — the same idle-variety
  path the other sequencer callers (`0x8941D`, `0x8A18C`, `0x8C2CE`) use. Another sign
  the row path is generic object machinery.

So the entity resolver legitimately resolves row 3 → PLAYART `run.fsh` for slot 22;
whether the retail *also blits* that pose is a separate question (leg L158.1).

## 4. The native pitch-ball draw (the real ball sprites)

### 4.1 The sentinel trigger in the draw walk

`FUN_00056FA4` (`0x56FA4..0x5707E`, 63 insns, called from the match presenter
`FUN_00058BC0 0x58C9A/0x58D2D` and the stadium bands `FUN_00058AC4` ×3) walks the
sorted list:

```
0x56FEA MOV ESI,[EDI+0x14E6F0]      ; list value
0x56FF0 TEST ESI,ESI; JNZ 0x57008   ; non-zero -> entity path (FUN_00057158)
0x56FF4 CALL 0x4FF54                ; camera-z band gate  (see 4.4)
0x56FF9 TEST EAX,EAX; JNZ 0x57065   ;   non-zero -> skip
0x57001 CALL 0x572E8                ; THE BALL DRAW (value-0 sentinel)
```

The value-0 entry is the empty slot FUN_00056CF4 seeds (`0x56D60` loop writes the list
`0..32`; the walk uses `[0x14EAE8] = 0x18 = 24`), and its depth key is the ball's own
projected triple copied to `[0x54370]` in `FUN_00057754` (`0x57779 MOV ESI,[0x154374];
0x57785 MOV EDI,[0x154370]; MOVSD ×3`; `[0x109A74]` = `0x155AB0` and `[0x154374]` =
block B+0xCB4 per `FUN_00058E00 0x58F20`). So the ball draws in painter order at its
own depth.

### 4.2 `FUN_000572E8` — the ball presenter

```
0x57356/0x5736A  bit 0x80 of byte[[0x109A78]] -> return (hidden)
0x57382  frame = byte[[0x109A78]] & 0xF ; clamp vs [0x108FEC] (count):
0x5739D  if (frame >= count || frame < 0) { byte &= 0xF0; frame = 0; }   ; sanitizer
0x573AC  handle = [0x14EAEC] ; frame_ptr = FUN_000A1920(handle, frame)
0x5741A  if single: FUN_00057278(EAX=[0x154374], EDX=[0x154378], EBX=frame_ptr)
0x573C6  if trail: FUN_000A3090(5, [0x14EAF0] ring …) + 5× FUN_00057278
```

* `[0x109A78]` is an image-initialized pointer = **`0x155C4E`** (first-hand
  `read_memory 0x109A70`, 24 B: `a4 5a 15 00 | b0 5a 15 00 | 4e 5c 15 00 | 50 5c 15 00
  | 51 5c 15 00 | 52 5c 15 00`), i.e. the byte immediately after the staged 23-slot
  frame array (`0x55C37+22 = 0x55C4D`) — the composer-staged **ball display byte**.
* `[0x14EAEC]`/`[0x108FEC]` are filled by `FUN_000567D0` (`0x5684A MOV EAX,0x29; CALL
  0x4AFB8`; `0x56857 [0x14EAEC]=handle`; `0x5685C FUN_000A1918` count; frames touched
  in the loop). Resource slot `0x29` = name-table `0x107370` entry 41 = `"ball"`
  (FU-152 §2.5, fresh) → retail GAMEART0 resolves `ball.fsh` (entry 37; fresh
  `fifa96_play` summary `8 frames, frame=0 16x16`, reproduced by my probe: 2256 B
  entry, engine bank 128 count=8 f0=16×16).
* `FUN_00057278` blits the frame at the screen pair via `FUN_0009B664` (scaled by
  `[0x109000]`); the trail is the 5-slot ring `[0x14EAF0]`, index `[0x109038]`,
  timer `[0x10903C]` advancing every >6 ticks (`0x57306..0x57348`).
* `FUN_00057440` is a second, single-frame variant of the same draw (same handle and
  display byte; position `[0x154374]/[0x154378]`, `0x5748D..0x574AC`) — a second
  pass/shadow (leg L158.3). `FUN_00057754`'s tail (`0x58210..0x58282`) draws **ball
  frame 0** through the quad mapper `FUN_000A3380` (FU-89 §3.2 "the ball/indicator
  quad path"); its walk skips value 0 (`0x57CDA JZ`).

### 4.3 The roll byte `[0x155C4E]` is produced by the composer

`FUN_00036C70 0x36EFC..0x36F83` (first-hand):

```
0x36EFC  if (byte[0x15781C] == 0) skip setup
0x36F00  if (!([0x155C4E] & 0x40)) { [0x155C4E] |= 0x40;
             [0x155C50] = FUN_000627F4([0x155AA4], [0x155AA8]);   ; staged camera x/z
             byte[0x155C51] = 1; }
0x36F36  if (byte[0x157822] != 0) [0x155C4E] |= 0x80;   ; match-over -> hide ball
0x36F48  else [0x155C4E] &= 0x7F;
0x36F4F  if (word[0x1577C2] != 0 || word[0x1577C0] != 0) {
0x36F63     lo = [0x155C4E] & 0xF; [0x155C4E] = ([0x155C4E] & 0xF0) | ((lo+1) & 0xF);
         }
0x36F89  [0x155C4F] = FUN_00064EE4();
```

`0x1577C0`/`0x1577C2` are the FU-71 camera velocity words (`vel_x`/`vel_z`, FU-71 §4
table; 91/54 xrefs, FU-148 §5). So the ball **rolls +1 per composer frame while the
camera is moving**, and the hide bit is the match-over word `0x157822`. The roll byte
is the exact analogue of the engine's missing per-frame art state.

### 4.4 The band gate

`FUN_0004FF54` reads `[[0x109A70]+8]` = the staged camera triple's z (`0x155AA4..AC`)
and returns 1/2 when it leaves `[-0xB10, 0xB10]`, else 0 — the same band constants
`FUN_00058AC4` uses on `[obj+8]`. The dedicated ball draw is therefore suppressed when
the camera is at the field ends (the presenter draws in bands).

## 5. Is the native ball art in the engine's loaded set? Yes

First-hand live staging probe (`/tmp/opencode/r6banks`): `bank_count = 151`; the
GAMEART0 half is `91..150`; **`banks[128]` = GAMEART0 entry 37 = `ball.fsh`
(count 8, frame0 16×16)** and `banks[142]` = entry 51 = `ball2.qfs` (count 8, 33×33).
No asset change is needed for T2; the requested "strip table" is the already-staged
GAMEART0 ball bank. (`ball2.qfs`'s 8-frame 33×33 bank has no derived consumer — leg.)

## 6. What flips the resolved art from the pose to a ball

The lever is **where the ball slot's sprite is sourced**, not the row table:

* Do **not** edit `match_run_anim_rows` row 3 (first-hand native data; the pose
  provenance must stay). A "table fix" that rewrote row 3's `+8` to a GAMEART0 index
  cannot work either: the resolver's animator budget is the PLAYART 91-entry array
  (`0x57DE8` is filled from slot `0x47` only), and the engine's bank namespace for
  GAMEART0 starts at `player.count` (91).
* The honest flip is the **dedicated ball draw**: at the ball slot (or the value-0
  sentinel), draw `r->banks[ball_bank]` (the staged GAMEART0 `ball.fsh`) with frame =
  the roll byte (clamped to the bank's frame count) at the ball's projected position
  and scale — the `FUN_000572E8` single-frame semantics — and stop resolving the FU-84
  row art for that slot. The pose is then gone; no other table moves.

## 7. Numbered legs (L158.x)

* **L158.1 — does the native entity pass also blit slot 22's row-3 pose?** The
  composer stages it unhidden (`0x36E12..0x36E47`) and both entity walks' gates
  (`FUN_00056FA4 0x57008..0x57046`; `FUN_00057754 0x57CE0..0x57DFB`) are dynamic
  screen-band/depth forms (rect fields from the presenter argument), so static
  analysis cannot decide whether the retail culls it. A DOSBox-X frame with the ball
  on screen (or a live staging trace) closes this. Until then the port should draw the
  ball art and suppress the pose (drawing both is certainly not the retail look).
* **L158.2 — the roll cadence in engine terms.** The native advance gate is the
  *camera* rate words (`0x1577C0`/`C2`), which are the T1/R2-L4 live-camera leg; the
  engine stand-in (ball velocity non-zero) must be disclosed until T1 lands. The
  bit-0x40 latch and the `0x155C50..52` angle/flag bytes are produced in the composer
  (`0x36F00..0x36F2F`, `0x36F89`) but their consumers (the trail/indicator passes) are
  not derived.
* **L158.3 — the ball secondary draws.** The 5-slot trail ring (`[0x14EAF0]`,
  `[0x109038]`, timer `[0x10903C]` step 6), the second draw `FUN_00057440`, the
  `FUN_00057754` tail ball-frame-0 quad (`0x58246..0x58282`, FU-89 §3.2) and the
  `ball2.qfs` bank (`banks[142]`) have no ported consumer.
* **L158.4 — the `[0x109A78]` byte is a view-control byte, not ball-only.** Its bit
  `0x40` switches the stadium point count in `FUN_000590B0` (`0x115` vs `0x166`) and
  `FUN_00037154/0x37164` read it too; the full writer/clear set of bit `0x40` and the
  `[0x155C4F..52]` cluster is not derived.

## 8. T2 wiring recommendation (concrete)

Files: `include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`, plus tests named below. No loader/asset or
ISO change.

1. **Capture the ball bank by name.** In `struct match_stage_set` add
   `int32_t ball_index;` (init `-1`), and in `match_stage_set_build` match the pitch
   entry name `"ball.fsh"` next to the existing `PALsys.fsh`/`Frames.fsh`/`clockfnt.fsh`
   matches (`fifa96_match_run.c:4426-4433`; define
   `#define MATCH_RUN_BALL_BANK "ball.fsh"` beside `MATCH_RUN_PALETTE_BANK`, `:1617`).
   In `fifa96_match_run_stage`, set a new
   `r->ball_bank = (pitch.ball_index >= 0) ? (uint8_t)(player.count + pitch.ball_index)
   : 0xFF;` (first-hand: `player.count = 91`, ball at `128`; keep `0xFF` = absent so
   no-ISO/synthetic fixtures keep the current path).
2. **Add the roll byte.** New `uint8_t ball_roll;` on `fifa96_match_run_render`. Port
   `0x36F4F..0x36F83`: once per granted frame, when the ball record has motion
   (`mr->entities.ball.vel_x != 0 || vel_z != 0`; the native gate is the camera-rate
   words — disclose as the L158.2 stand-in), `ball_roll = (ball_roll + 1) & 0x0F`
   (low-nibble fidelity); hide the draw while the engine's match-over equivalent
   holds (native `0x157822` → bit 0x80; leave unhidden if the engine has no such
   state yet).
3. **Draw the ball bank at the ball slot.** In `fifa96_match_run_render`'s walk, for
   the ball slot (`slot == FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS`,
   i.e. 22) with `r->ball_bank < r->bank_count`: bypass `fifa96_render_resolve`, take
   `frame = ball_roll` clamped to `r->banks[ball_bank].count`, select the frame with
   `fifa96_render_bank_at`/`fifa96_sprite_frame_parse`, and draw it with the already
   computed `slots[slot].clean` x/y and `scale` (the existing `match_run_draw_sprite`
   call, `:4130`). Direction is deliberately not folded (native semantics). Fall back
   to the current resolver path when `ball_bank == 0xFF`.
   Alternatively handle the value-0 sentinel (`:4084`) with the same draw; the slot-22
   branch is the smaller change and yields the same pixels in the engine's list model.
4. **Tests (RED first).** `tests/test_engine_match_render.c`: pin that slot 22's drawn
   frame comes from the ball bank (16×16; sprite pointer inside the ball bank's
   region), that `ball_roll` advances on ball motion and wraps at the frame count, and
   that the row-3 pose no longer draws for that slot. `tests/test_engine_m2.c:~2210`
   (the staged art-identity stratum "frame 0 of the synthetic per-row table, leg
   T2-L1") re-pins to the ball bank frame. Re-pin `m2-frames.txt` with the written
   reason + frame diff (the plan expects the pixel change); M1 must not move.
   `test_ball_machine_moves_ball_slot_pixels`'s canvas hash re-pins with the same
   reason (its fixture stages synthetic banks: keep the `0xFF` fallback or extend the
   fixture with a ball bank).
5. **Do not** touch `match_run_anim_rows`, the PLAYART staging order, or the resolver.

## Provenance

Ghidra MCP `/FIFA96.EXE`: `disassemble_function` 0x6E724, 0x6FE84, 0x8C24C, 0x886D4,
0x572E8, 0x57278, 0x57440, 0x56FA4, 0x57754, 0x58AC4, 0x58BC0, 0x4FF54, 0x56CF4,
0x567D0, 0x365B0, 0x37154, 0x4A6BC, 0x78F1C (xrefs), 0x73D90 window;
`disassemble_bytes` 0x36D9E (130 B), 0x36E49 (70 B), 0x36E00 (90 B), 0x36E8D (260 B),
0x73D60 (80 B), 0x8C020 (60 B); `read_memory` 0x10EF00, 0x10E278, 0x11052A, 0x109A70,
0x10EF1B (via the 0x10EF00 read); `search_instructions` operands `4afb8`, `4c064`,
`109a78`, `109a74`, `154374`, `14eaec`, `108fec`, `155c4e`, `155c50`;
`get_xrefs_to` 0x78F1C, 0x10EF00, 0x6E724, 0x56FA4, 0x57754, 0x109A78, 0x109A74,
0x886D4, 0x8B688.
Asset probes (read-only): `/tmp/opencode/r6probe.c` (PLAYART/GAMEART0 decode + name
census), `/tmp/opencode/r6banks.c` (live engine bank map), `build/fifa96_play` against
`tests/golden/gameart0.pvi`. Scratch only; not committed.
Carried docs: FU-84 §3.1/§4, FU-85 §1/§2/§4, FU-86 §1/§2/§4, FU-89 §3.2, FU-71 §4,
FU-148 §16, FU-152 §2.5, FU-154 §9, plan R6.

## 9. Implementation status (M2 phase-13 T2, 2026-10-11)

T2 implemented the §6/§8 recommendation (T2-L1: slot 22 draws the real ball
art). Every native window quoted below was re-verified first-hand before
coding (fresh `disassemble_bytes`/`read_memory`/`fifa96_play` this task).

* **The bank capture (step 1).** `fifa96_match_run_stage` now matches the
  pitch container's BIGF name `ball.fsh` beside `PALsys.fsh`/`Frames.fsh`/
  `clockfnt.fsh`/`playfnt.fsh` and commits
  `render.ball_bank = player.count + entry` on success (first-hand: GAMEART0
  `auto`/`sprite --name ball.fsh`/`--entry 37` = 60 entries, entry 37
  `ball.fsh` 8 frames 16x16 — reproduced this task — and PLAYART = 91
  entries, so the engine bank index is 128). No `ball.fsh` (or an index past
  the byte bank domain) keeps the `0xFF` sentinel and the FU-84 resolve; the
  ISO staging test pins both (`test_engine_match_staging.c`).
* **The roll byte (step 2).** `match_run_scene_stage` advances
  `render.ball_roll` once per granted frame with the exact native byte op from
  `0x36F4F..0x36F83` (first-hand bytes this task): `AL = (byte & 0xF) + 1`
  OR'd into the high nibble (`MOV AL,[0x155C4E]; AND AL,0xF; MOV BL,[0x155C4E];
  AND BL,0xF0; INC EAX; OR BH,AL` — the carry lands in bit 4, **not** a plain
  `(lo+1) & 0xF`; §8 step 2's sketch is corrected here), gated by the ball
  record's motion as the disclosed stand-in for the native camera rate words
  `0x1577C0`/`C2` (L158.2). The presenter's out-of-range sanitizer
  (`0x57382..0x573A9`, first-hand this task: `frame = byte & 0xF;
  if (frame >= [0x108FEC]) { byte &= 0xF0; frame = 0; }` — the native write-
  back) is applied in the composer right after the advance: the drawn-frame
  sequence then matches the native drawn sequence (continuous 0..count-1
  cycling) when the ball is drawn every frame; the native clears at draw time
  only (culled windows may differ — carried under L158.2).
* **The dedicated draw (step 3).** `fifa96_match_run_render`'s walk draws
  `r->banks[r->ball_bank]` frame `ball_roll & 0xF` (sanitized to 0 when
  >= the bank count) through `fifa96_render_bank_at` + the existing
  `match_run_draw_sprite` at slot 22's FU-85 clean x/y/scale, bypassing
  `fifa96_render_resolve` for that slot; direction is not folded (native
  semantics). The value-0 sentinel branch stays the skip (`:4084`), per §6.
  The presenter's own blit scale cell `[0x109000]` and screen-pair source
  (`FUN_00057278` -> `FUN_0009B664`, §4.2) are unported: the port uses the
  slot's FU-85 projection scale as the derived stand-in (new leg **L158.5**).
* **No table edits.** `match_run_anim_rows` (row 3), the PLAYART staging and
  the resolver are untouched; the FU-84 row-3 install remains the first-hand
  record of what the pose path means.
* **Tests (RED first).**
  `tests/test_engine_match_render.c::test_ball_slot_draws_ball_bank_frame`
  (the draw sources the captured bank; roll selects the frame; the sanitizer
  and the 0xFF fallback) and `::test_ball_roll_advances_with_ball_motion`
  (parked = no advance; +1 per moving frame with the count-2 sanitize cycle
  asserted per frame) — both RED against the pre-fix port (observed asserts:
  centre pixel 0x22 -> 0x44; roll 0 vs expected 1). The ISO staging test pins
  `ball_bank == 128` (and the GAMEFLD1 re-stage falling back to the sentinel);
  the M2 tape's frame-9 stratum pins `ball_bank == 128` too.
* **Goldens: M2 re-pinned to v13 with the written reason + frame diff** (the
  intended visible-quality upgrade; `tests/test_engine_m2.c` v13 section, 85
  lines, lines 1..8 identical, 77 hash-only lines 9..85 differ, zero `state=`
  diffs; first differing line frame 9 `ab8bded120648826` ->
  `40270bb4a15d2302`; new M2 sha `5f7e7692…`). **M1 unmoved** `09b726b7…`
  (byte-exact `cmp`). `make check` 108/108 (sanitized); wired census unchanged
  at 21/80 (no action row touched).
* **Smoke (`DISPLAY=:1`, fresh `p13-t2-*`; honest).** The live SDL run reaches
  the match and the kickoff view draws the ball **as a soccer ball** — the
  `ball.fsh` sprite is unambiguous in the crop
  (`docs/screens/p13-t2-ball-crop.png`, 4x); the previous pose art is gone.
  After the kick the ball leaves the fixed view within ~1 s (the phase-2
  lead/ kick moves it; no live follow — R2-L4), so the roll frames do not land
  in the stills: the roll's runtime evidence is the unit test's per-frame
  sequence and the M2 v13 re-pin. Captures
  `docs/screens/p13-t2-{frontend,kickoff,live-play,ball-crop}.png`.
* **Legs.** L158.1 (native entity-pass slot-22 pose suppression — DOSBox-X)
  unchanged: the pose is no longer *drawn* by the port, but the native retail
  cull question is still open. L158.2 now also carries the presenter
  sanitizer's draw-time write-back versus the composer-site emulation and the
  ball-motion stand-in for the camera-rate gate. L158.3 unchanged (trail ring,
  `FUN_00057440`, frame-0 quad, `ball2.qfs`). L158.4 unchanged
  (`[0x109A78]`/bit 0x40). **New L158.5**: the ball presenter's own blit scale
  `[0x109000]` and the `FUN_00057278` screen-pair source (the port draws
  through the slot's FU-85 projection).
