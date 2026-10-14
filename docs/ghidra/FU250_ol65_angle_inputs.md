# FU-250 (draft) — OL-65: the row-07 `0x816DF` angle inputs and the action-`0x22` gate

Provenance: M2 phase-24 wave **W24-3** (planned FU-250), 2026-10-14, **read-only**.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; standard tools only; **no** rename / comment /
label / function-create / script / project save). `get_current_program_info` this
session: name `FIFA96.EXE`, path `/FIFA96.EXE`, `watcom:LE:32:default`, image base
`0`, `min_address 0x10000`, **2706 functions / 15239 symbols**. Every
address/byte/instruction/xref below is a fresh `disassemble_bytes` /
`read_memory` / `get_xrefs_to` / `get_current_program_info` of **this session**
unless marked *cross-cited* (then the frozen FU doc is named). Engine/loader
source is read first-hand. The alias traps are applied (`dword[x]>>16` = the word
at `x+2`; `>>24` = the byte at `x+3`); the row-07 CALL rel32 is verified
byte-level. **No repo file was modified beyond this draft + the report; no git; no
subagents.**

Question (plan Task 3 / spec §4 slice 3): derive the `word[+0x6B]`/`word[+0x6D]`/
`word[+0x7D]` angle inputs whose `kick-machine-07` **staging** keeps the row-07
`0x816DF` `opponent_invoke` gate dead (owner FU185-L6 / FU247-L1) — the gate for
action `0x22`; confirm or correct the brute-force deadness claim; give the port
shape (OL-65) and the action-`0x22` census ruling.

---

## 1. Headline

- **The FU247 §2.3 / FU247-L1 "input-map erratum" is itself wrong.** It fell into
  the `dword[x]>>16` alias trap: it read `0x8169F MOV EDX,[ESI+0x6D]` /
  `0x816A2 MOV EAX,[ESI+0x6B]` and labelled the results `word[+0x6D]` /
  `word[+0x6B]`. Those are **dword** loads; `>>16` takes the *high* word:
  `dword[+0x6B]>>16 = word[+0x6D]` and `dword[+0x6D]>>16 = word[+0x6F]`.
  The native gate is therefore `entity_angle(word[+0x6D], word[+0x6F])` minus
  `word[+0x7D]` — exactly **FU185-L6's** map, not FU247's.
- **`opp_angle_x` is NOT mis-sourced.** The engine stages
  `s->opp_angle_x = opp->lane_z` (`handlers.c:983`), and pool `lane_z` **is**
  native `+0x6D`. That is the correct first angle argument. FU247-L1's
  "mis-sourced `opp_angle_x` from `lane_z`" claim is refuted by the fresh bytes.
- **The gate is dead because of two hardcoded zeros only:** `s->opp_angle_z = 0`
  (native `+0x6F`) and `s->opp_face_word = 0` (native `+0x7D`), `handlers.c:984-985`.
  All three native inputs are **pool-modelled and live-produced**
  (`lane_z`/`cam_dz6f`/`face7d`), so **OL-65 is a two-line staging-fix leg, not an
  absent-producer permanent ceiling.**
- **Census ruling:** action `0x22` stays **uncounted today** (`{0x22, NULL}`,
  `handlers.c:5522`; 58/80 = 72%). The blocker is the staging stand-in, not a
  missing producer; it becomes countable (58/80 → 59/80 = 73%) **only** after the
  two fields are re-sourced *and* a live replay proves `out.opponent_invoke`
  fires (the FU247-L1 differential). Do not wire in this read-only slice.

---

## 2. The three angle inputs (fresh, byte-level)

### 2.1 The native read — the alias trap

Fresh `disassemble_bytes 0x81691..0x816E4` (83 B; 25 instructions) and fresh
`read_memory 0x81697` (16 B: `66 81 7e 6b d0 00 7d 45 8b 56 6d 8b 46 6b c1 fa…`):

| addr | bytes | instruction | meaning |
|---|---|---|---|
| `0x81697` | `66 81 7e 6b d0 00` | `CMP word[ESI+0x6B],0xD0` | lane gate (unchanged) |
| `0x8169D` | `7d 45` | `JGE 0x816E4` | lane ≥ 0xD0 skip |
| `0x8169F` | `8b 56 6d` | `MOV EDX, dword[ESI+0x6D]` | **dword** load at +0x6D |
| `0x816A2` | `8b 46 6b` | `MOV EAX, dword[ESI+0x6B]` | **dword** load at +0x6B |
| `0x816A5` | `c1 fa 10` | `SAR EDX,0x10` | `EDX = word[+0x6F]` (0x6D+2) |
| `0x816A8` | `c1 f8 10` | `SAR EAX,0x10` | `EAX = word[+0x6D]` (0x6B+2) |
| `0x816AB` | `e8 c0 c6 00 00` | `CALL 0x8DD70` | `entity_angle` thunk |
| `0x816B0` | `66 2b 46 7d` | `SUB AX, word[ESI+0x7D]` | `angle − face7d` |
| `0x816B4` | `80 e4 03` | `AND AH,0x3` | fold to 10 bits |
| `0x816B7` | `0f bf d0` | `MOVSX EDX,AX` | |
| `0x816BA` | `81 fa 00 02 00 00` … | `CMP EDX,0x200` / `JLE` / `0x400−EAX` fold | |
| `0x816CB` | `98` | `CWDE` | |
| `0x816CC` | `83 f8 55` | `CMP EAX,0x55` | |
| `0x816CF` | `7d 13` | `JGE 0x816E4` | diff ≥ 0x55 skip |

So the two `MOV` operands are **dwords** at `+0x6B`/`+0x6D`, and the `SAR 16`
selects the high word of each. The high word of `[+0x6B]` is `+0x6D`; the high
word of `[+0x6D]` is `+0x6F`. **First argument = `word[+0x6D]`; second argument
= `word[+0x6F]`.** (FU247 §2.3 labelled these `+0x6B`/`+0x6D` — the alias trap.)

### 2.2 The thunk `0x8DD70` (fresh)

Fresh `disassemble_bytes 0x8DD70` (32 B): `PUSH EBX; PUSH ECX; MOVSX EDX,DX;
PUSH EDX; CWDE; PUSH EAX; CALL 0xCD474; ADD ESP,8; POP ECX; POP EBX; RET`. So
`EAX = x` (first/leftmost arg), `EDX = z` (second): `entity_angle(x = word[+0x6D],
z = word[+0x6F])`. Fresh `get_xrefs_to 0x8DD70` = **28** callers (a shared idiom),
including our gate site `0x816AB`; the same call shape appears in the ported
row-04 half-line gate (`fifa96_outfield.c:626`,
`entity_angle(state->word6d, state->word6f)` vs `face_word7d`) — an independent
in-repo witness that the second angle input is `+0x6F`, not `+0x6D`.

### 2.3 The native producers of the three inputs

* **`word[+0x6D]` = `cam_x − pos_x`; `word[+0x6F]` = `cam_z − pos_z`.** Fresh
  `disassemble_bytes 0x7C776..0x7C7CF` (BF20 lane block) confirms the writers:
  `0x7C782 MOV AX,[0x15774C]; SUB EAX,EDI([+0x59] pos_x); 0x7C78E MOV [+0x6D],AX`
  and `0x7C792 MOV AX,[0x157754]; SUB EAX,EDX([+0x61] pos_z); 0x7C79A MOV [+0x6F],AX`.
  It then computes the lane (`0x7C79E MOV EDX,dword[+0x6D]; 0x7C7A1 MOV
  EAX,dword[+0x6B]; SAR both 16; CALL 0x8DC68` = `entity_distance(cam_dx,cam_dz)`;
  `0x7C7AF MOV [+0x6B],AX`) — the same alias idiom, which proves the `+0x6B`/`+0x6D`
  dword loads are the `+0x6D`/`+0x6F` word reads.
* **`word[+0x7D]` = `face7d`**, produced by `FUN_00079C50` = `fifa96_entity_face`
  (`*face7d = entity_angle(dx,dz)`, `fifa96_entity_update.c`).

**Engine staging (first-hand source).** `match_kick_from_record`
(`handlers.c:982-985`):

```
s->opp_lane_word  = opp->lane_x;   /* +0x6B (<0xD0 gate) — correct */
s->opp_angle_x    = opp->lane_z;   /* +0x6D — CORRECT native x-arg */
s->opp_angle_z    = 0;             /* should be opp->cam_dz6f (+0x6F) */
s->opp_face_word  = 0;             /* should be opp->face7d  (+0x7D) */
```

**All three pool fields exist and are live.** `lane_z`/`cam_dz6f` are refreshed
per record every dispatched frame by the BF20 block
(`fifa96_match_run.c:1460-1461`; `e->lane_z = cam_dx; e->cam_dz6f = cam_dz`),
which runs at the tail of the same per-record dispatch as the row-07 handler
(native order: action handler then `0x7C776` lane block), so the handler reads the
previous frame's values exactly as the native does. `face7d` is written by the
face arms (`handlers.c:3200/4152/4446/4518`) and synced to the record each dispatch
(`fifa96_match_run.c:1378/1514`). The loader already passes
`fifa96_action_kick_angle(s->opp_angle_x, s->opp_angle_z)` as `(x,z)`
(`fifa96_action_handlers.c:1701`), and the field comments already name
`word[+0x6D]`/`word[+0x6F]` (`fifa96_action_handlers.h:918-919`).

---

## 3. The gate — brute-force claim confirmed, cause corrected

`0x816CC CMP EAX,0x55; 0x816CF JGE` fires when
`fold10(entity_angle(word[+0x6D], word[+0x6F]) − word[+0x7D]) < 0x55`.

Fresh `disassemble_bytes 0xCD474` (79 B) re-derives `fifa96_entity_angle`: with the
second argument `z = 0`, the flags path yields `value = 0x100` for `x > 0`, `0x80`
for `x == 0`, `−0x100` for `x < 0`; the engine’s current staging is
`entity_angle(lane_z, 0)` against `face_word == 0`, whose folded diffs are
`{0x100, 0x80, 0x100}` — all `> 0x55`. **The FU185-L6 / FU247 §2.3 brute-force
claim stands: `out.opponent_invoke` never fires.** But the *cause* is the two
hardcoded zeros (`opp_angle_z`, `opp_face_word`), **not** a mis-sourced
`opp_angle_x` and **not** an absent `+0x6F` producer. Once re-sourced from the
live pool fields, the gate compares the record-to-camera direction
`entity_angle(cam_dx, cam_dz)` against the record facing — a geometrically
satisfiable comparison (union of the other gates: opp code `6`, no slot,
`lane_word < 0xD0`, `timer89 < 5`, `kick_staged`).

The installer site is unchanged and fresh-confirmed: `0x816D1 MOV ECX,1`
(invoke-now), `0x816D6 MOV EDX,0x22`, `0x816DB MOV EAX,ESI`, `0x816DD XOR EBX,EBX`,
`0x816DF CALL 0x7D9A4` (rel32 `0xFFFFC2C0 = −0x3D40`; `0x816E4 − 0x3D40 = 0x7D9A4`
verified). Fresh `get_xrefs_to 0x816DF` = **0** (a CALL *site*, not a target).

---

## 4. The row-`0x22` body and what it needs

`0x110768` (slot `0x22`, fresh `read_memory 0x110768` = `9c 53 08 00`) ⇒
`0x1106E0[0x22] = 0x8539C`; fresh `get_xrefs_to 0x8539C` = **1** (`0x110768`,
`DATA`) — the body has no other static caller, so its only installer is row-07.
The body is already ported as `fifa96_match_action_22` (`handlers.c:4323`,
`0x8539C..0x85481`, FU-185 §2/§10) and exported for direct unit testing
(`fifa96_match_handlers.h:202`); its action-table slot is `{0x22, NULL}` at
`handlers.c:5522` (the phase-table `0x22` at `:5619` is a different row).

**What the body needs from the inputs:** stage 1 (`handlers.c:4354`) reads
`band = (int16_t)(r->lane >> 16)` = **`word[+0x6B]`** (the record lane) and feeds
`fifa96_action_sequence_press_event(band, timer89, &p)`. Because the installer
runs invoke-now on the *opponent target* record (`match_kick_invoke_opponent`,
`handlers.c:1122-1132`), `mr->record` is that opponent, so row-22's band is the
opponent's `+0x6B` lane — produced by the same BF20 block. The body needs **no new
producer**; it needs the installer's gate to fire (§3).

---

## 5. Errata (quoted, first-hand; prior sections never rewritten)

### 5.1 FU247 §2.3 / FU247-L1 — the input map (an alias-trap error)

FU247 §2.3 quote: *“the second input is `word[+0x6D]` (`0x8169F`/`0x816A5`), and
the *third* native term is `word[+0x7D]` (`0x816B0`). No `+0x6F` read exists
anywhere in `0x8164B..0x816E4` … FU185-L6 … name `word[+0x6F]` as the *second*
angle input.”* And FU247-L1: *“the native row-07 angle gate reads
`word[+0x6B]`/`word[+0x6D]` (x/z → `0xCD474`) … the kick-struct **staging**
(`opp_angle_x` mis-sourced from `lane_z`, and `opp_angle_z`/`opp_face_word`
hardcoded 0) …”*

**Corrected.** *Both* `+0x6B` and `+0x6D` operands are **dword** loads; the
`SAR 0x10` selects their high words, `word[+0x6D]` and `word[+0x6F]`. There is
indeed **no literal `+0x6F` operand** in `0x8164B..0x816E4` — but the *read*
target is nonetheless `word[+0x6F]`, via `dword[+0x6D]>>16`. FU247's own dword
mis-read is exactly the alias trap AGENTS.md warns about. Consequently:
(i) FU185-L6's map (`+0x6D`/`+0x6F`/`+0x7D`) is **correct**; (ii) the loader
header’s `opp_angle_x = word[+0x6D]` / `opp_angle_z = word[+0x6F]`
(`fifa96_action_handlers.h:918-919`) is **correct**; (iii) the engine’s
`opp_angle_x = opp->lane_z` is **correct**, not “mis-sourced”.
`docs/ghidra/FU247_exclusions_and_tape.md` §2.3 / FU247-L1 should carry this as a
new erratum; `docs/ENGINE.md` ~341 and `handlers.c:5523` name `word[+0x6F]` as
correct and need no change beyond the staging fix below.

### 5.2 The task-brief title map

The brief names `word[+0x6B]`/`word[+0x6D]`/`word[+0x7D]`. That map is FU247's,
not the native’s; the native is `word[+0x6D]`/`word[+0x6F]`/`word[+0x7D]`.

---

## 6. Port shape (OL-65)

1. **Re-source the two stand-ins** in `match_kick_from_record`
   (`fifa96_match_engine/fifa96_match_handlers.c:984-985`):
   `s->opp_angle_z = opp->cam_dz6f;` and `s->opp_face_word = opp->face7d;`
   (`opp_angle_x = opp->lane_z` **stays** — it is correct). Fix the two comments;
   the loader needs no change (it already passes `(x,z)`).
2. **Prove the gate on a live replay.** Force/observe the row-07 post-kick arm
   (`out.opponent_invoke`) after the re-source; the m2 tape has **no** live
   `opponent_invoke` today (`handlers.c:1166`), so this is a fixture/force test,
   not a tape test (FU247 §4.1: action `0x22` is install-live/tape-dormant).
3. **Only then** register the slot (`handlers.c:5522` `{0x22, NULL}` →
   `fifa96_match_action_22`), add the row to ENGINE.md’s counted set and recompute
   the README title **in the same commit** (AGENTS census rule). Until step 2
   passes, the row stays NULL/uncounted (the rows-27/29/2C precedent, Ruling W8).
4. **Mutation-RED riders:** NULLing the two re-sourced fields must abort at the
   row-07 gate assertion; a `NULL → fn` flip with a dead gate must abort (FU247
   §4.4 classes 1–3).

---

## 7. Census ruling for action `0x22`

* **Today: uncounted.** Slot `NULL` (`handlers.c:5522`); `dispatch_action(0x22)`
  returns `-FIFA96_ERR_UNSUPPORTED`. Census holds at **58/80 = 72%**.
* **Not a permanent ceiling.** The only blocker is a two-field staging stand-in;
  both sources (`cam_dz6f`, `face7d`) are pool fields with live producers, so the
  native gate is *statically satisfiable* after the re-source. FU247 §5’s
  “ceiling” label is refined to **producer-staging leg (OL-65)**.
* **Not countable until proven live.** The strict “engine reaches it” test still
  requires the gate to actually fire on a replay; whether the live camera/facing
  relationship produces `diff < 0x55` is the differential question (FU247-L1).
  Ruling recommendation: **leave `0x22` NULL/uncounted this wave**; the mover is
  step 3 above (58/80 → 59/80 = 73%) once step 2 lands.

---

## 8. Numbered legs (new)

1. **FU250-L1 — the FU247 §2.3 / FU247-L1 input map is an alias-trap error.**
   The native gate reads `word[+0x6D]`/`word[+0x6F]` (via `dword[+0x6B]>>16` /
   `dword[+0x6D]>>16`) and subtracts `word[+0x7D]`; FU185-L6 and
   `fifa96_action_handlers.h:918-919` are correct. `opp_angle_x = lane_z` is
   correct (not mis-sourced). **Owner: FU247 / OL-65.**
2. **FU250-L2 — the live-replay gate differential.** After the §6 re-source,
   prove `out.opponent_invoke` can fire on a forced/fixture replay (the m2 tape
   has no live `opponent_invoke`). **Owner: OL-65 / the action-`0x22` wiring task.**
3. **FU250-L3 — the `word[+0x6B]` lane-input composition.** `+0x6B` is the BF20
   `entity_distance(cam_dx,cam_dz)` metric, `+0x6D`/`+0x6F` the camera-delta
   words; a future refactor that changes the pool’s `lane_x` semantic (e.g. the
   `reset_lane` `band.band` writer, `fifa96_match_entities.c:197`) must keep the
   `+0x6D`/`+0x6F` pair intact for this gate. **Owner: FU-147 / FU-151.**
4. **FU185-L2/L3/L4/L5** (the `0x79B1C` word clears, the `+0x43` cell, the
   `0x6E598` anim sink, the second `0x22` installer family) and **FU185-L6 /
   OL-65** stand.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `read_memory 0x81697` (16 B), `0x110768` (8 B);
`disassemble_bytes 0x81691..0x816E4` (83 B), `0x8DD70` (32 B), `0xCD474` (79 B),
`0x7C776..0x7C7CF` (89 B); `get_xrefs_to 0x8539C` (1: `0x110768`), `0x816DF`
(0), `0x8DD70` (28, incl. `0x816AB`). Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_handlers.c` (900-1002, 1028-1169, 4313-4375,
5515-5530), `fifa96_match_run.c` (1360-1470), `fifa96_match_entities.c`
(139-209, 442-486), `include/fifa96_engine/fifa96_match_entities.h` (125-160),
`include/fifa96_engine/fifa96_match_handlers.h` (186-202),
`src/fifa96_loader/fifa96_action_handlers.c` (41-45, 335-350, 1630-1723),
`fifa96_outfield.c` (600-660, 760-790), `fifa96_entity_update.c` (63-119,
145-199), `include/fifa96_loader/fifa96_action_handlers.h` (905-934).
Docs cross-cited: FU185 (§2/§10/§11), FU247 (§2.2/§2.3/§4.1/§5/§6), FU-147/FU-151,
`docs/ENGINE.md` (7-9, 330-346, 469-480), AGENTS.md (alias traps; census rule).
No writes beyond this draft and the workspace report; no rename/label/comment/
function/script/project save; no git; no subagents.
