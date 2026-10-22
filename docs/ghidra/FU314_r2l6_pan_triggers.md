# FU314 — the row-04 half-line pan's natural triggers (phase-31 R2-L6)

**Phase/Provenance:** phase-31 recon-ahead wave, Stage-1 **Task 1 of 4** (`docs/superpowers/plans/2026-10-22-fifa96-m2-phase31-recon-ahead.md`), scoped by `docs/superpowers/specs/2026-10-22-fifa96-m2-phase31-natural-claim-pans-design.md` §2–§5/§8. Read-only derivation: Ghidra MCP against **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0, `.image` overlay — confirmed fresh this session) plus the repo's engine/test sources. No production code, no repo change beyond this draft, no git. All addresses are **flat image** addresses; LE globals are the `+0x100000` file-offset form.

## 1. The question

Which native/engine inputs arm the row-04 half-line pan, and why did the 12,000-step natural probe never fire it? Deliverable: the exact gate → pan chain with each input's live/staged status, the measured reason the probe's records never satisfied the gate, the minimal natural trigger, and **GO/NO-GO** on a natural pan (legs FU310-L5, the row-04 trigger census, `timer81`).

## 2. The native chain (fresh reads this session)

### 2.1 The entry and the two pan sites

| Claim | Evidence (fresh unless marked) |
|---|---|
| The action-row table entry for code `0x04` is the dword at `0x1106F0` = **`0x7E7C8`**; the neighbours are `0x1106F4 = 0x7F194` (row 05) and `0x110700 = 0x81068` (row 08) | `read_memory 0x1106C0/0x60` this session; `get_xrefs_to 0x7E7C8` → one `DATA` ref at `0x1106F0` |
| The row-04 body runs `0x7E7C8..0x7F141` (single `RET` at `0x7F141`) and is **not** a Ghidra function (the dispatch-table gap `0x7E7C4..0x7F144`) | `get_function_by_address 0x7E7C8/0x7ED89` → "No function found"; `disassemble_bytes 0x7F120` → `0x7F141 RET c3`; `get_function_by_address 0x7E600` → body_end `0x7E7C4` |
| The row-04 body contains exactly **two** `FUN_00071C94` calls: **`0x7EFCF`** (arm A) and **`0x7F0D1`** (arm B) | `get_xrefs_to 0x71C94` = **11**, of which `0x7EFCF` and `0x7F0D1` are the only sites inside `0x7E7C8..0x7F141`; bytes `e8 c0 2c ff ff` at `0x7EFCF` → `0x7EFD4 − 0xD340 = 0x71C94`; `e8 be 2b ff ff` at `0x7F0D1` |
| Both pan calls pass **height `EBX = 0`** (`0x7EFC4 XOR EBX,EBX` / `0x7F0C6 XOR EBX,EBX`) and a stack seed pair; arm A passes `ECX = 1` (`0x7EFAF`), arm B `ECX = 0` (`0x7F0B4`), and the event codes are `EDX = 0x15` (`0x7EFB9`) / `EDX = 0x1D` (`0x7F0BB`) | `disassemble_bytes 0x7EF5E/0xB0` and `0x7F020/0xC0` |

The gate block that the wave quotes (`0x7ED89..0x7EDE3`) is only the **admission**; the pan is built at `0x7EE4B..0x7F0D6`. The handoff is:

```
0x7ED89..0x7EDBD   lane/timer81 gate -> 0x7EDCE, or the receiver gate 0x79B58 + RET
0x7EDCE..0x7EDE3   lane<=bound, camY<=0x50 -> 0x7EE4B (else the 0x6E598 anim tail + RET)
0x7EE4B..0x7EF0C   angle gate, tracked/side sound arm, team-target swap, slot restore
0x7EF42..0x7EFE9   row byte == 0x13  -> arm A  (0x71C94 @0x7EFCF, code 0x15, ECX=1)
0x7EFEE..0x7F0D6   not tracked, camY!=0/has_slot/velocity seed -> arm B (0x71C94 @0x7F0D1, code 0x1D, ECX=0)
```

### 2.2 The gate, instruction by instruction, and its port

| Native | Instruction (fresh) | Meaning | Port (`fifa96_outfield.c`) |
|---|---|---|---|
| `0x7E7E2` *(carried: port comment `fifa96_outfield.c:416`, FU-142 K.5)* | `phase != 2 -> reset, RET` | phase gate | `:416` |
| `0x7E7F3` *(carried: port comment `fifa96_outfield.c:420`, FU-142 K.5)* | `word[+0x81] != 0 -> RET` | **timer81** countdown blocks the whole body | `:420` |
| `0x7ED89` | `MOV EAX,0x40` | threshold base | `:610` |
| `0x7ED8E` | `MOV BX,word[EBP+0x81]` | timer81 | `:610` |
| `0x7ED95` | `MOV CX,word[EBP+0x6b]` | **lane** = the `+0x6B` word | `:610` (`state->lane`) |
| `0x7ED99/9B` | `SUB EAX,EBX ; CMP AX,CX ; JGE 0x7EDCE` | `(0x40 − timer81) >= lane` → the pan tail | `:610` |
| `0x7EDA0` | `CMP EBP,[0x1577CA] ; JZ 0x7F138` | tracked exclusion on the **receiver** branch only | `:611` |
| `0x7EDB2` | `lane >= 0x90 -> RET` | receiver-branch range | `:612` |
| `0x7EDBD` | `CALL 0x79B58` | receiver timer (not the pan) | `:613` |
| `0x7EDCE` | `CMP CX,word[EBP+0x77] ; JG 0x7F138` | **lane <= bound** | `:617` |
| `0x7EDD8` | `CMP dword[0x157750],0x50 ; JLE 0x7EE4B` | **camY <= 0x50** → the half-line tail | `:618` |
| `0x7EDE1/EDEE` | `byte[+0x8D]==0 -> RET`, `dword[+0x5D]!=0 -> RET` | the non-half-line anim branch only | `:619` |
| `0x7EE4B..0x7EE57` | `EDX=dword[+0x6D]>>16 ; EAX=dword[+0x6B]>>16 ; CALL 0x8DD70` | angle from `+0x6D/+0x6F` (alias trap: `dword[+0x6B]>>16` **is** word `+0x6D`) | `:629` |
| `0x7EE5C` | `SUB AX,word[EBP+0x7d]` | **− face (+0x7D)** | `:632` |
| `0x7EE60` | `AND AH,0x3` | `& 0x3FF` | `:633` |
| `0x7EE66..0x7EE75` | `CMP EDX,0x200 ; JLE ; EDX=0x400−EAX` | fold to `0..0x200` | `:634` |
| `0x7EE78/7D` | `CMP EAX,0xAB ; JG 0x7F138` | **angle gate** | `:635` |
| `0x7EE83..0x7EEC0` | `[0x1577CA]`/team-side sound arm + the `+0x7B6/+0x7B2` swap | presentation/team bookkeeping | `:636-649` |
| `0x7EEEF..0x7EF42` | `[team+0x7E7]` corner arm, `+0x20` slot restore, `[+0x7CB]` | corner/slot returns | `:650-668` |
| `0x7EF42` | `AL=byte[[rec+0x28]] ; CMP EAX,0x13 ; JNZ 0x7EFEE` | **row byte 0x13 → arm A** | `:670` |
| `0x7EFEE` | `CMP EBP,[0x1577CA] ; JZ 0x7F0D6` | **arm B tracked exclusion** | `:682` |
| `0x7EFFA/0x7F003` | `CMP dword[0x157750],0 ; JZ 0x7F035` | camY != 0 → type seed `<<5` | `:685` |
| `0x7F035..0x7F054` | slot `+0x20/+0x21 <<6` | has_slot seed | `:689` |
| `0x7F056..0x7F09F` | `abs(camX)<=0x630 && abs(camZ)<=0xA20` → `word[+0x73/+0x75]<<2` | **velocity seed** (the camY==0, no-slot case) | `:692-700` |

### 2.3 The gate inputs' native producers

| Input | Native producer (fresh unless carried) | Live in the port? |
|---|---|---|
| `lane` (`+0x6B`) | `0x7C776..0x7C7AF` (the BF20 lane block): `bound := old lane` (`0x7C77E`), `cam_dx := word[0x15774C] − word[+0x59]` (`0x7C782..0x7C78E`), `cam_dz := word[0x157754] − word[+0x61]` (`0x7C792..0x7C79A`), `lane := 0x8DC68(cam_dx, cam_dz)` (`0x7C79E..0x7C7AF`); `0x8DC68` is `max(abs(dx),abs(dz)) + 3·min(abs(dx),abs(dz))/8` (verified exactly against 1,140 sampled pool lanes this session) | **live** — `match_run.c:1496-1516` (`fifa96_action_locomotion_track`, mirror `fifa96_action_handlers.c:335-352`) |
| `bound` (`+0x77`) | same block, `0x7C77E` = the previous frame's lane | **live** (`match_run.c:1504`) |
| `timer81` (`+0x81`) | `entity_timer_decay` (`match_entities.c:574-588`); the only reload in the port is row 0F (`fifa96_action_handlers.c:1856/1859`) | **live**, measured `0` in every row-04 dispatch |
| `camY` (`0x157750`) | the FU-71 camera triple Y (FU157 §3; carried); port = `render.camera.pos_y` | **live** (`handlers.c:2699`) |
| face (`+0x7D`) | `FUN_00079C50` (`0x79C50..0x79C98`, fresh): if `DX==0 && BX==0` return `[+0x8E]`; else `word[+0x7D] := 0xCD474(DX,BX)` and `byte[+0x8E] := ((dword[+0x7B]>>16)+0x40 & 0x3FF)>>7`. **40 callers** (fresh `get_xrefs_to 0x79C50`), incl. `0x8E1CB` (`FUN_0008E008`), row 0x24 `0x865C6`, row 0x16 `0x846E0`. Every native site writes **both** cells | pool cell **live** (`e->face7d`), but the port splits the helper: `fifa96_arm_face` writes only the octant `r->type` (`handlers.c:1495`, `5083`, `match_entities.c:563`) while `fifa96_entity_face` writes `r->face7d` at only **5** sites (`handlers.c:3311`, `4596`, `5014`, `5310`, `5382`); the row-04 staging is **staged 0** (`handlers.c:2646`) |
| row byte (`[[rec+0x28]]`, the current animation row id; the port's `anim_id`) | the animation install (`0x6E598` / `0x6E8E8`), port `match_run_record_anim_install` | **live** (`anim_id`), measured `0` in every row-04 dispatch |

Two fresh producer facts matter downstream:

* `FUN_0008E008` (the frame-wrap driver) calls `0x79C50(rec, −off_x[+0x8E], −off_z[+0x8E])` at **`0x8E1CB`**, but only in the animation branch whose row byte is `>= 0x70` (decompile `0x8E008` fresh: `iVar5 = *(int*)(p+0x8b)>>0x18; FUN_00079c50(p, -tab34[iVar5], -tab3c[iVar5])` under `bVar2 >= 0x70`). The tables are `0x10F334`/`0x10F33C` — the same `match_kick_dir_x/z` the row-04 arms index.
* Row 0x24's stage-0 face (`0x865C6`) passes the **constant vector `(DX=1, BX=0)`** (`0x865B5 MOV EDX,1`, `0x865BE XOR EBX,EBX`), i.e. face `0x100`; the port calls `fifa96_entity_face(r->pos_x, r->pos_z)` (`handlers.c:5381`) — a shape slip that lands on the same `0x100` for the measured staging.

## 3. The port shape (the row-04 half-line arm)

* `fifa96_match_action_04` (`handlers.c:2605`) stages the record + pool, runs `fifa96_outfield_row04_step` (`handlers.c:2743`), and applies the camera arm at **`handlers.c:2777-2796`**: `out.events != 0 && out.event_code != 0` → `fifa96_camera_event_set(&camera, out.event_x, out.event_z, 0, out.event_track_reload)` (**height 0**) → `fifa96_camera_event_dir` → `fifa96_camera_set_tracked(entity_id)` (the `0x71D27` bind) → the arm-A `[0x1577FA] = F2 + F2/4` timer (`0x7EFD4`). The integrator is the frame body (`match_run.c:4362`) whose walk call is `match_run.c:4413` (`fifa96_camera_update_walk`).
* The row body is a faithful transcription of §2.2: `fifa96_outfield.c:610-624` (the admission) and `:626-720` (the half-line body, both arms, codes `0x15`/`0x1D`, `event_track_reload` only on arm A).
* **Two port deviations from §2.3 sit on the gate's inputs**: (i) `s.face_word7d = 0` (`handlers.c:2646`, "word +0x7D unmodeled (OL-72)") while the native reads the live `+0x7D`; (ii) `s.is_ball_track` falls back to `id == controlled` when `[0x1577CA]` is unset (`handlers.c:2667-2673`), so the **user's own record can never arm B** in the port, whereas the native `CMP EBP,[0x1577CA]` (0x7EFEE) compares against a cell that is `0` at match start and therefore never excludes a real record (carried: FU157 §3.3).

## 4. Measurement (out-of-tree instrumented copy; repo untouched)

Method: a `/tmp` copy of `tests/test_engine_m2.c` with a per-granted-frame dump (`m2_natural_sample`) linking the built `build/libfifa96_*.a`; the same `run_natural_pass` (section seeder + `M2_KEYS`), 12,000 engine steps, ISO present; plus the same dump inside `run_scripted_pan_witness`. The sample is post-frame, so the **dispatch** tuple is the previous frame's tail (the BF20 lane block runs *after* the row-04 handler, `match_run.c:1491-1516`). A direct unit replay of the measured tuple through `fifa96_outfield_row04_step` cross-checks the body.

| Quantity (measured) | Value |
|---|---|
| granted frames sampled / window | **3,598** / 12,000 steps (matches `test_engine_m2.c:3603`) |
| frames with a code-4 record | 3,060 (from step **1802**, the phase-2 entry) |
| row-04 dispatch record-frames | **6,120** (2 per frame) |
| … in phase 2 with `timer81 == 0` | **6,120 / 6,120** |
| … passing `lane <= 0x40 − timer81` **and** `lane <= bound` | **2,081** (one record: team 1 #1, entity 12; steps **5065..11999**; lane 64→4, bound 66→4) |
| … passing the `0x7EE7D` angle gate (`abs(bearing − face) <= 0xAB`) | **0** — at every one of the 2,081: `face = 0x100`, bearing = `0x300`, Δ = **0x200 (512) > 0xAB (171)** |
| … with row byte `0x13` (arm A) | **0** — `anim_id == 0` in **6,120 / 6,120** dispatches |
| camera cells over the window | `(0,0,0)`, `pan_counter 0`, `tracked −1` (carried `test_engine_m2.c:3603-3617`, re-measured) |
| scripted witness (control) | 358 granted frames; 68 band candidates; **2** angle passes: step 335 rec 0:9 at `(−2,−2)`, `face 0`, bearing `0x80`, Δ `0x80`; step 515 rec 0:10 at `(134,−6)`, Δ `0xA8`; **pan fired at step 339, `tracked = 9`**, `max_x 0x120` |

Why `face = 0x100` for all 22 records: step 12 (the kickoff/cutscene row 0x24 on all records) runs row 0x24 stage 0, whose native face call is the constant vector `(1,0)` (`0x865B5/0x865BE/0x865C6`) → `0x100`. Nothing re-faces these records for the rest of the window: the port's only `+0x7D` writers are five row face arms (rows 0x0B `handlers.c:3311`, 0x1B `:4596`, 0x09 `:5014`, 0x16 `:5310`, 0x24 `:5382`; the native helper's other 35 call sites either write only the octant in the port or are unported), plus the mover's per-frame identity write (`match_run.c:532`, which cannot turn the face because its `+0x43` direct-face input is staged 0, `match_run.c:498`), and none of them changed the value after step 12.

## 5. Why the 12,000-step probe never fired the pan

**The half-line admission is not the blocker.** Natural play already puts a record inside the pan's near field: team 1 #1 closes from `px 1948` to `px 4` (lane `64 → 4`) and satisfies `lane <= 0x40 − timer81` and `lane <= bound` for **2,081 dispatches** (steps 5065..11999) with `timer81 = 0` and `camY = 0 <= 0x50`.

**The blocker is the `0x7EE7D` angle gate, i.e. the record's `+0x7D` face.** The record approaches the parked camera at `(0,0)` from `+x`; the camera bearing from the record is `0x300` (−x). Its face is `0x100` (+x, written by row 0x24 at step 12) → `|0x300 − 0x100| = 0x200` (180°), which the `0xAB` (≈60°) gate rejects. The port's **staged** `face_word7d = 0` does not rescue it either: `|0x300 − 0|` folds to `0x100` (90°), still `> 0xAB`. So the failure is *not* the face-staging deviation — it is the face/bearing pairing.

Arm A is independently dead in the window: `anim_id` is `0` in all 6,120 row-04 dispatches (the `0x13` row byte never occurs), so `0x7EF42` would always take `0x7EFEE` (arm B) — but the flow never reaches the arm selection at all: the angle gate bails at `0x7EE7D`.

**The structural reason (derived from the measured geometry).** The gate is *"a record near the camera, facing the camera"*. Natively the FU-71 camera sits on the play (the follow), so a player contesting the ball is inside the band **and** faces the ball — i.e. the camera — and the two conditions coincide. In the port the camera is parked at `(0,0)` (phase-30 NO-GO) while the kickoff ball sits at `x = 0x1E0` (`FIFA96_MATCH_ENTITY_KICKOFF_BALL_X`, `match_entities.c:532`), **0x1E0 east of the camera**. The band record therefore reaches the *parked* camera (`px 4`) while facing the ball (`face 0x100`, +x) — it is not contesting the ball at all, it is 0x1DC away from it — so the measured `Δ = 0x200` is exactly the camera↔ball offset appearing as a 180° face/bearing disagreement. Corollary for Stage-2: with the camera on the ball, a record contesting the ball sits inside the `0x40` band around the ball/camera *and* faces it, so the gate passes by construction; a pan that must *start* from a centre park cannot bootstrap itself through this arm, because the only record the centre park attracts is one that has left the ball.

Secondary (not the measured cause, but a live gate input): the port's `is_ball_track` controlled-fallback (`handlers.c:2667-2673`) excludes the user's record from arm B; the native `[0x1577CA]` is `0` until a pan binds it (`0x71D27`) and the clear sites store `0` (carried: FU157 §3.3, FU310 §2), so natively the comparison never excludes a real record (the cell's init is FU314-L4).

## 6. The minimal natural trigger

**The gate tuple (exact):** a record with `code == 4` dispatched in phase 2, `active != 0`, `timer81 == 0`, whose `lane` (camera→record distance from the BF20 block) is `<= 0x40` and `<= bound`, with `camY <= 0x50`, and whose **`+0x7D` face is within `±0xAB` of the camera bearing** (`fifa96_entity_angle(cam − pos)`); then arm B (`anim != 0x13`, not `[0x1577CA]`) seeds `(vel_x<<2, vel_z<<2)` and calls `0x71C94` with height 0 → `event_code 0x1D` → `fifa96_camera_event_set` + `set_tracked` → the walk.

Three live routes to that tuple, with status:

1. **R1 — a face arm that points the band record at the camera.** Live port producers of `+0x7D`: rows 0x0B (`0x82915..0x8295E`, `handlers.c:3311`), 0x1B (`:4596`), 0x09 (`:5014`), 0x16 (`0x846E0`, `:5310`, the ball band — its producer is the live phase-0x19 contact install, row 0x16 is COUNTED), 0x24 (`0x865C6`, `:5382`), plus the animation frame-wrap arm (`0x8E1CB`, gated on anim row byte `>= 0x70` — **unported**). Status: none ran inside the measured window, so this is a drive/contact question, not a gate gap.
2. **R2 — geometry alone.** With the live `face = 0x100` (+x, toward the ball at `0x1E0`) the record must enter the band **west** of the camera (`pos_x ∈ [−0x40, 0)`, `|pz| <= |px|·tan 60°`) — i.e. a record that has run *past* the parked camera while still watching the ball. The measured tape sends its only band record from the east; a mirrored drive (side flip / second half / restart / an over-running defender) would satisfy it with **no code change**.
3. **R3 — arm A (`anim_id == 0x13`)**, which needs the `0x13` row-byte producer; never live in the window (FU314-L6).

**Cheapest honest proof (spec §5):** a frame-path fixture that stages the record's **face producer input** (never a camera gate) at the measured band entry — e.g. team 1 #1 at step 5065 with `face7d = 0x300`. Verified by replaying the measured tuple through the ported body (`fifa96_outfield_row04_step`, out-of-tree driver: phase 2, `active 1`, `has_slot 0`, `timer81 0`, `lane 64`, `bound 66`, `word6d −64`, `word6f 0`, `face_word7d 0x300`, `ball_height 0`, `is_ball_track 0`): `events = 1`, `event_code = 0x1D`, `event_track_reload = 0` — i.e. the real row-04 body emits the arm-B pan; with `face 0` or `face 0x100` the same tuple emits nothing (`events = 0`). The scripted witness independently proves the downstream pan from an equivalent passing tuple (`(−2,−2)`, face 0, bearing `0x80`, Δ `0x80`, pan at step 339, tracked 9).

## 7. Legs

1. **FU314-L1 — the natural pan witness (carries FU310-L5).** Narrowed, not closed: the band entry is proven live (2,081 dispatches) and the only failing link is the angle input. Owner: phase-31 Stage-2 witness + FU317 (the extended natural probe / claim drive). *Not provable statically.*
2. **FU314-L2 — the row-04 `+0x7D` staging.** `handlers.c:2646` stages `0`; the native reads the live `+0x7D` (`0x7EE5C`). One-line fix (`s.face_word7d = e->face7d`), no measured effect on the current natural tape (both values fail), but it is required for any drive whose face ≈ bearing (and it makes the fixture honest). Owner: the row-04 staging.
3. **FU314-L3 — the `+0x7D` producer coverage.** Natively `FUN_00079C50` writes **both** `+0x7D` and `+0x8E` at all **40** call sites (fresh `get_xrefs_to 0x79C50`); the port splits it — `fifa96_arm_face` writes only the octant `r->type` (3 sites) and `fifa96_entity_face` writes `r->face7d` at only **5** (`handlers.c:3311`, `4596`, `5014`, `5310`, `5382`) — so the gate's `+0x7D` input is under-produced by construction. The per-frame `FUN_0008E008` arm (`0x8E1CB`, gated on the animation row byte `>= 0x70`, vector `−off[+0x8E]` from `0x10F334/0x10F33C`) is part of that gap; the port's `FUN_0008E008` slice is the frame-wrap row successor only (`match_run.c:1672/1769`). Owner: the animation driver + the face-helper sites.
4. **FU314-L4 — `[0x1577CA]` init + the controlled fallback.** The native cell's match-start value (BSS `0`, cleared by `0x700F4`) vs the port's `id == controlled` fallback (`handlers.c:2667-2673`): the fallback changes arm B for the user's record. Owner: the camera tracked cell / the row-04 staging.
5. **FU314-L5 — the mover's direct-face input.** `match_run_record_mover` stages `+0x43 direct_face = 0` (`match_run.c:498`, never set), so the native locomotion turn (`fifa96_action_locomotion_step`'s `direct_face` arm) can never rotate `+0x7D`; the native `0x8E29C CMP byte[+0x43],0` reads a live record byte. Owner: the mover staging.
6. **FU314-L5a — the row-0x24 face-vector shape.** Native `0x865C6` passes `(1,0)`; the port passes `(pos_x, pos_z)` (`handlers.c:5381`). Same `0x100` in the measured staging but not the native call shape. Owner: row 0x24.
7. **FU314-L6 — the `0x13` row-byte producer.** Arm A needs `byte[[rec+0x28]] == 0x13`; measured `anim_id == 0` in 6,120/6,120. Which anim/row installs `0x13` is unresolved. Owner: the animation install (`0x6E598`/`0x6E8E8`).
8. **FU314-L7 — the row-04 trigger census (bounded here).** The dispatch is the code-4 install; port sites seen: the row-04 self-install `0x7E85C` (`outfield.c:429`), row 0x21's ball-actor install 4 (`0x85375..0x8538D`, `handlers.c:2461`), row 0x06's carrier-gate install 4 (`handlers.c:6337`), row 0x1A's install 4 (`0x7669D`, port comment `handlers.c:4513`). A full native installer census belongs to FU315's claim chain. Owner: FU315.
9. **FU314-L8 — the AI/possession drive.** The natural probe's only band record approaches from `+x`; whether any drive (AI claims, restarts, side flip, an over-running defender) can bring a code-4 record into the band on the face side is open (the test's own note: the natural AI/possession invokers are partly unported). Owner: FU317.

## 8. Verdict

**NO-GO on a natural pan with the engine's live cells as measured.** Not because the pan is unwired — it is wired and faithful (`handlers.c:2777-2796`), and the near-field admission is live — but because the `0x7EE7D` **angle gate** rejects the only band entry the 12,000-step tape produces: `face 0x100` vs bearing `0x300`, Δ `0x200 > 0xAB`, in **2,081 / 2,081** band dispatches, with arm A independently dead (`anim_id 0` in 6,120/6,120). With the camera parked 0x1E0 from the ball, the record that reaches the band is the one that has left the ball, so its face points away from the camera — the arm cannot bootstrap from a centre park.

**GO on the chain itself:** the exact natural trigger is the tuple in §6, and it is reachable through **live** producers (R1 the row-0x16/0x0B/0x1B/0x09/0x24 face arms; R2 a band entry from the west; R3 the `0x13` row byte). No new native port is needed for the gate itself; the cheapest deterministic proof is the face-input fixture at the measured band entry, with the scripted witness already demonstrating the downstream pan from an equivalent tuple. What is *not* yet proven is a **natural** drive that produces the tuple — that is FU314-L8/FU317, and it is the only thing standing between this slice and FU310-L5.

The exact natural chain to prove:

```
phase 2, code 4 dispatch, active, timer81 == 0
  BF20 lane block (0x7C776..0x7C7AF): lane = dist(camera, rec), bound = prev lane
  lane <= 0x40 - timer81   and   lane <= bound          [measured: 2,081 passes]
  camY (camera.pos_y) <= 0x50                            [measured: 0]
  angle = 0x8DD70(cam_dx, cam_dz) - face(+0x7D); |angle| <= 0xAB   [measured: 0x200, FAIL]
  row byte != 0x13 and rec != [0x1577CA]                 [measured: anim 0, tracked -1]
  -> event_code 0x1D, seed = (word[+0x73]<<2, word[+0x75]<<2), height 0
  -> fifa96_camera_event_set + fifa96_camera_set_tracked -> fifa96_camera_update_walk
```

## 9. Provenance

**Fresh this session (Ghidra MCP, `/FIFA96.EXE`):** `get_current_program_info`; `disassemble_bytes` `0x7EB80/0x210`, `0x7ED50/0xB0`, `0x7EDFF/0x160`, `0x7EF5E/0xC0`, `0x7F020/0xC0`, `0x7F120/0x30`, `0x79C50/0x60`, `0x8E1A0/0x40`, `0x8E240/0x90`, `0x7C770/0x70`, `0x865A0/0x40`, `0x846C8/0x24`; `decompile_function 0x8E008`; `read_memory 0x1106C0/0x60`; `get_function_by_address` `0x7E600`, `0x79C50`, `0x7E7C8`, `0x7ED89`; `get_xrefs_to` `0x71C94` (11), `0x7ED89` (7), `0x79C50` (40), `0x7E7C8` (1 DATA), `0x1106F0` (0).

**Carried:** the gate quote and the six-arm census (FU157 §3.2/§3.3); the scripted witness `0→0x90→0x120`, two pans, Y=0 (FU310 §6, FU313 §5.3 P6); the natural-probe window and zero-camera baseline (FU313 §5/§8.2, `docs/ENGINE.md` phase-30); the camera cell map and the clear sites (`0x700F4`/`0x70258`/`0x70054`, FU157 §3.3/FU310 §2); the armer lines (`match_run.c:3155-3191`).

**Port/test sources read (read-only, cited by line):** `src/fifa96_engine/fifa96_match_handlers.c` (`:2605`, `:2634-2646`, `:2667-2673`, `:2699-2711`, `:2743`, `:2767-2796`, `:5290-5310`, `:5371-5385`, `:6334-6337`, `:6373`); `src/fifa96_loader/fifa96_outfield.c` (`:400-420`, `:575-624`, `:626-720`); `src/fifa96_loader/fifa96_action_handlers.c` (`:335-352`, `:1856-1859`); `src/fifa96_loader/fifa96_entity_update.c` (`:75-142`, `:201-206`); `src/fifa96_engine/fifa96_match_run.c` (`:489-534`, `:1324`, `:1333`, `:1413`, `:1491-1521`, `:1561`, `:1672`, `:1769`, `:4362-4413`); `src/fifa96_engine/fifa96_match_entities.c` (`:200`, `:529-572`, `:574-588`); `tests/test_engine_m2.c` (`:3446-3515`, `:3521-3620`, `:3623-3730`).

**Measurement artifacts (out of tree, `/tmp/opencode/fu314/`):** the patched copy `probe.c` (dump only; no gate/engine change), `recs3.txt` (natural, 3,598 frames), `wit.txt` (witness, 358 frames), `row04_unit.c` (the measured tuple replayed through the ported body), `angle_table.c` (the `0x8DD70`/`fifa96_entity_angle` octant map: `0=+z, 0x100=+x, 0x200=−z, 0x300=−x`). No repo file was modified; the ISO was only read.

**Known limits of this slice:** the dump samples post-frame, so the dispatch tuple is the previous tail (the lane block runs after the handler); the `0x7EC97` rng early-return is not observable, so the 2,081 figure is an upper bound on gate evaluations (the angle delta is `0x200` on every band frame regardless); and no claim is made that the native `+0x7D` at a *different* band entry would pass or fail.

## Review erratum (wave review, 2026-10-22)

**Quote (§5):** "**The blocker is the `0x7EE7D` angle gate, i.e. the record's `+0x7D` face.** ... in **2,081 / 2,081** band dispatches." **Fresh:** the gate bytes `0x7ED89..0x7EE7D` re-read; the FU317 DOWN drive adds a second true case of the same gate, not a refutation of the 2,081. `probe8` at the driven record 2: F=1512 (trace f=1511) `pos=(4,0), lane_z=0, cam_dz6f=0, face7d=0x100, bearing=0xFF00, Δ=0x200` (fail); F=1513 (trace f=1512) `lane_x=0, lane_z=0, cam_dz6f=0, face7d=0x100, bearing=0x80, Δ=0x80` (pass); `probe3`/`review_rec2` reproduce claim f=1514, event f=1513, pan f=1514..1540, place `−0x90`, tracked 2, pan `0→4`, Y/Z 0. The NO-GO is **tape-scoped**: the undriven `M2_KEYS` window never brings a code-4 record to `(0,0)`; the hold does. FU314's 2,081 (dispatch record-frames) and FU317's 2,082 (test-side algebra over granted frames) are one counter boundary apart, not a disagreement.

**Correction (root-cause quantities, §5):** the sentence "the kickoff ball sits at x = 0x1E0 ... the band record ... is 0x1DC away from it" is stale at the gate: `probe4` measures the ball at `(218,0,252)` at trace f=1512 (F=1513; the prior frame is `(214,248)`), record `(0,0,0)`, distance ≈333, not `(480,0,0)`. `face 0x100` is the step-12 row-0x24 constant (native `0x865B5`/`0x865BE`/`0x865C6` = `(1,0)`; port `handlers.c:5381` passes `(pos_x,pos_z)`), not a live ball-facing; it happens to remain within ≈`0x8C` of the ball bearing at the gate, but nothing re-faces the record. The qualitative "facing the ball, not the camera" survives; the "camera on the ball ⇒ gate passes by construction" corollary is a Stage-2 hypothesis, not a proof.

**Face verdict:** native-faithful value, port-defective data flow. `handlers.c:2646` stages `s.face_word7d = 0` instead of the live `e->face7d` (FU314-L2); `handlers.c:5381` passes the position vector where the native passes `(1,0)` (FU314-L5a). Neither fix makes the measured east approach pass (`face 0x100` vs bearing `0x300` is still Δ`0x200`); L2 is the prerequisite for a face-aligned route, L5a is a fidelity fix, and the live producer is L3/L5 (the `0x8E1CB` frame-wrap arm / `direct_face`). **Fresh helper check:** `0x79C50` writes both `+0x7D`/`+0x8E` only for a nonzero `(DX,BX)`; the `(0,0)` early return at `0x79C59..0x79C68` writes neither, so "every native site writes both" is too strong.

**Stage-2 consequence:** FU314-L1's one-pan witness is **met** by FU317 (Layer-1); the remaining leg is **FU314-L1a** (the face-aligned/robust route), owner L2+L3/L5 + the drive. The 2,081 NO-GO stays the undriven baseline and must not be cited as a contradiction of the driven witness.
