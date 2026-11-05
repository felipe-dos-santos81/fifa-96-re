# M2 phase-51 — the natural-reach + fidelity wave

**Status:** proposed 2026-11-04. The phase-36…50 registers stand as written;
this spec is the authority for the phase-51 slices. It supersedes no prior
section.

## 1. The goal

Move the two remaining **honest** runtime losses and close two body-fidelity
legs, without touching the 78/78 port-coverage census:

1. wire the **phase-`0x13`** row (ported body, measured re-pin) — install-live
   `63 → 64`, natural `57 → 58`;
2. fix the **action-row-`0x03`** special arm (`0x7E1CE`) and wire it if the
   phase-2 clamp inputs are portable — install-live `64 → 65`, natural
   `58 → 59`; otherwise the arm fixes land with the slot still `NULL` and the
   wiring is the carried leg;
3. port **FU-71 leg 9.6** (`0x719A0..0x71B98`, the far-wall damp/event arm) —
   no census move, M2 byte-identical;
4. correct the **phase-6 stage-3 gate** to the native latch semantics
   (`[0x158882]` = "kind was 3 at stage 2"), keep the blocked rows blocked
   with an accurate reason, and retire the stale `[0x14C32A]`/`[0x158882]`
   framings.

The README title stays `100%` (78/78 bodies unchanged). The ENGINE headline
moves to the new install-live/natural counts only if T1/T2 wire.

## 2. Evidence base (fresh, this phase)

Ten read-only recon reports under `/tmp/opencode/recon51/` (R1–R10) plus the
in-flight R11 (row-03 clamp inputs). Every native address below was re-read
this session via the Ghidra MCP on `/FIFA96.EXE` unless marked carried.

### 2.1 Phase `0x13` (R5, high confidence)

- Body `0x6E244..0x6E32A`: side gate `0x6E25F`; inactive arm copies
  `word[0x10F364/0x10F368]` with the `ac2<4 && side==0` z negation; active
  arm draws **four** `0x92AC8` words in order z-mag/z-sign/x-mag/x-sign and
  writes only `+0x4D`/`+0x55`.
- Producer `0x8D693..0x8D722` (the `FUN_0008D098` case `0x13/0x14`):
  `ac2>=4` writes fixed `±0x2D0`/`0x5A0`, else x-then-z draw pairs `%0x1E0`;
  writes **only** `0x10F364`/`0x10F368`.
- `FUN_0008D098` has exactly two xrefs (`0x740C8`, `0x740DB`) — the producer
  runs **once per phase write**, per team, not per frame.
- Tape: `write_phase(0x13)` at steps **2502/3402/4302** (lines 2503/3403/
  4303), 20 dispatches / 32 draws per write, **96 draws per run**. The dwell
  is 2640 frames but the body dispatches only on the three write frames.
- Measured re-pin (scratch, same source): slot + write-seam scatter →
  first differing line **3185**, **3338/6522** lines, hash-only, state tags
  identical, 295087 B, sha
  `54f4498755a74f0945266c66c59164a239ebf1f5fd60d0f78e416fe5316d44fd`.
  Slot-only (no scatter) → first 3209, 3314 lines, sha `c231a549…`.
  Scatter in the per-frame machine → first 852, 5671 lines, sha `09642d40…`
  — **rejected** (native cadence and the forced window).

### 2.2 Action row `0x03` (R6, high confidence)

- The `0x7E1CE..0x7E223` arm is **already ported** as the `phase ==
  0x13/0x14` block of `fifa96_match_action_03`, but with a real misread:
  the `< 0x30` gate is `word[ESP+2]` = `FUN_0008DCD4`'s **`out[1]` = dx =
  0x780 − pos_x`** (sign-extended), *not* the `out[0]` metric. The port
  compares the 2-D metric (`fifa96_arm_dist_stage`), so it takes `0xAE0` for
  a different record set. Latent while the slot is NULL; must be fixed
  before any wiring.
- `FUN_00079B58` writes `+0x93 = 0x10` **only when `+0x99 == 0`**; the port
  inlines it unconditionally (both the arm and guard A).
- The phase-2 placement counter's `move_attr` is the mover's block-A
  distance (`0x8E273/0x8E278`), i.e. `distance >> 6`; the port passes
  literal 0 → `min(1, timer79)` instead of `min((distance>>6)+1, timer79)`.
- The phase-2 clamp inputs (`FUN_0008C418` team bounds and the opponent
  selector) and slot branch B (`FUN_00079C20`) are the remaining FU177-L1/L6
  legs; wiring row 03 before the clamp is a fabricated clamp (R6 §3).
- Tape impact of wiring: first moved golden line `frame=29`
  (`state=20/0-0` = phase `0x14`); the chained `present_hash` makes every
  later line hash-only, no `state=` changes. Re-pin required; M1 cannot move.

### 2.3 FU-71 leg 9.6 (R8, high confidence)

- `0x719A0..0x71B98` is a **far-wall damp/event arm**, not an angle
  reflection. Dispatch: `0x718A9` gate; `0x718EF` zone != 0; `0x718FF`
  `input_bit0 == 0` → `0x719A0`; the mirror path clears at `0x71908/0x7190E`.
- Two chains (`0x719CF`/`0x71A5D`) set a flag when `|camZ|_w >= 0xE74` /
  `|camX|_w >= 0x8BC` and the height/prev/threshold gates pass; the damps
  are `shift_step((int16)(-v),1)` (discardable if the flag fires). Flag 0
  stores vel and `speed = hypot(vx,vz)`; flag 1 seeds `±0x180`, posts
  `0x974DC(1..3)` (none for speed <= 5), then `0x71C94(tracked,
  {hypot(seedX,seedZ), seedX, seedZ}, height+0x40, 0)`.
- `input_bit0` is the front-end option **INVISIBLE WALLS** (default
  Disabled = 0), so the angle arm is the default path; the wall arm is the
  option-enabled path. Natural M2 camera never reaches the outer gate;
  M2 expected byte-identical.

### 2.4 Phase-6 gate (R1 + R3, corrected)

- Gate `0x894A2`: `[0x158882] != 0 || [0x15888C] == 3 || [0x14C32A] != 0`.
- `[0x158882]` = ball block `0x1587EC + 0x96` = `row_flags & 1`
  (writer `0x6E7E7`, reset `0x73D3A`); the stage-2 body latches it to
  `(contact_kind == 3)` before the one-call split at `0x89485`.
- R1 read the re-entry as a live ordinary path; R3 shows the two calls are
  one frame apart and the ball-row timer is >= 0x40, so the latch can only
  differ from `kind==3` through a new kind-3 event (offside/goal), not
  through a same-frame ball install. The honest ruling: **model the latch
  faithfully and keep the three blocked rows blocked** with a corrected
  reason; the mode-4 Practise term and the `0x8948D` stale-local read stay
  numbered legs.

## 3. Invariants

- M1 `09b726b7…` is immutable. M2 re-pins only with a written reason + the
  measured frame diff + the stratum-set check (`state=` tags and counts).
- No fake producers. A row whose producer is not native-live stays `NULL`
  and, if install-live, stays in `fifa96_match_port_blocked`.
- Test-side pins are mutation-RED where the plan says so.
- Docs append-only: corrections are quoted errata in the owning FU record or
  the ENGINE register; never rewrite a prior section.
- `make check` green before every commit; commit per task and push
  `origin main`.

## 4. Task shape

| Task | Change | Census | Golden |
|---|---|---|---|
| T1 | phase `0x13` slot + write-seam scatter (R5 plan) | install-live +1, natural +1 | M2 **v27** re-pin (`54f44987…` expected) |
| T2 | row `0x03` arm fidelity (dx gate, `+0x99`, counter); wire iff the clamp inputs are ported in the same task | +1 / +1 if wired | M2 **v28** if wired; byte-identical if NULL |
| T3 | FU-71 leg 9.6 wall arm | none | M2 byte-identical |
| T4 | phase-6 latch + blocked reasons | none (blocked stays 6) | M2 byte-identical expected |
| T5 | docs/census close-out, FU-339, PROGRESS, ENGINE | final numbers | — |

Final headline if T1+T2 wire: **78/78 bodies, 59/80 natural, 65/80
install-live, 6 blocked, action 39/45, phase 26/35**. If T2 keeps the slot
NULL: **58/80 natural, 64/80 install-live, action 38/45, phase 26/35** and
the row-03 wiring is FU177-L1's carried leg.

## 5. Acceptance

- Each task: focused tests green; the M1 `cmp` exact; the M2 transcript
  either byte-identical or re-pinned with the measured diff recorded.
- T1's in-tree transcript must match the predicted `54f44987…`; any other
  sha is a stop-and-report (the scratch prediction is the control).
- T3/T4: M2 byte-identical (`6f402095…` or the then-current sha).
- `make check` green (112/112 or the new count) at every commit.
- Close-out: README title still `100%`; ENGINE headline and census test
  agree with the code; the leg ledger carries honesty about what T2 did.

## 6. Out of scope (carried)

- Act-9 stage-0 camera/placement sinks (FU181-L5; R7 S1–S7) — phase 52.
- Phase `0x14` producer (Cup/Season front-end probes) — needs the
  competition front-end; install-live only, with a blocked entry.
- The six un-dispatchable action rows and the dead phase rows (R9): `0x0A`,
  `0x17`, `0x27`, `0x29`, `0x2C`, phases `0x0B/0x0E/0x12/0x1B` stay `NULL`
  with their verified reasons; phases `0x01/0x11` need the R6 single-owner
  refactor.
- Player-visible gaps (R10): pitch/stadium, camera follow, control switch,
  fouls/offside, set pieces, celebration/replay, ball height, animation,
  HUD identity, audio.

## 7. Errata (measured during execution)

- **§2.1 draw count.** The "96 draws per run" is the **body-only** count
  (3 writes × 32 body draws). The producer adds 4 draws per team per write
  (8/write), so a live write consumes **40** shared draws and the tape run
  **120**. The re-pin sha was unaffected; the in-tree riders pin 32 (ac2>=4)
  and 40 (ac2<4) per write, and FU-340 records body 96 + producer 24 = 120.
- **§2.2 T2b.** R11 proves the phase-2 clamp bounds are `PCCD.PDB`-data-blocked;
  the row-`0x03` slot stays `NULL`. The portable sub-inputs (opponent selector,
  slot branch B) land in Task 2. Wiring is carried as FU177-L1.
