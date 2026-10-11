# FU-156 — reachability of the six remaining `FUN_00071C94` pan-camera callers

Provenance: M2 phase-12 recon wave R4, 2026-10-11, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive
program argument; standard tools only). No writes outside this draft; no
commits. Every xref count below is a fresh `get_xrefs_to` this session; every
quoted instruction/reference is a fresh `disassemble_bytes` / `read_memory`
of `/FIFA96.EXE`.

**Address model.** Code addresses are LE-image flat addresses identical to the
FU-148/FU-152 citation addresses; globals render as true flat addresses
(`0x157750`, `0x1106E0`). The `+0x100000` rule applies only when mapping a
flat-bin quote (`0x71C94` -> the FU-doc `0x71C94`); no such mapping is needed
here. The alias traps are applied: `dword[x]>>16` is the word at `x+2`,
`>>24` the byte at `x+3` (e.g. `dword[EBP+0x8E]>>24` = byte `[EBP+0x91]`, the
dispatched action code).

---

## 1. Scope and method

Classify the six `FUN_00071C94` callers still ledgered after phase-10 T2's
five wired sites (`0x7F4E7`, `0x71B8A`, `0x6FA62`, `0x77423`, `0x77DC6`,
`0x82A6C`): what invokes each (fresh xrefs up the chain), live-reachable in the
engine's current wired subset (yes/no/unknown + why), and overlap with the
phase-12 R1 (the `0x7CD60` second arm / tracked-side) and R2 (live camera
place/pan) producer questions.

Method: fresh `get_xrefs_to 0x71C94` (the census claim needed a fresh sweep);
per site, `get_function_by_address` (containing body), `disassemble_bytes`
over the caller window, and up-chain xrefs 2-3 levels. The action dispatch
chain was re-read first-hand from the action table `0x1106E0[code]`
(`FUN_0007D9A4` reader, FU-137 §1.1/§6.1).

---

## 2. Fresh census — `get_xrefs_to 0x71C94` = 11

Same 11-site set as FU-148 §13.4 / FU-152 §2.1 (no new/removed sites):

| site | containing body (fresh) | status (fresh docs + engine source) |
|---|---|---|
| `0x7EFCF`, `0x7F0D1` | row 04 body `0x1106E0[4]=0x7E7C8` | **wired** (phase-10 T2; `fifa96_match_action_04`, handlers.c `2427..2441`) |
| `0x75DC4` | row 1E body `0x1106E0[0x1E]=0x7550C` (stage 5) | **wired** (phase-9 T3) |
| `0x70DD1` | `FUN_00070C08` (reposition sub-body, `0x70C08..0x70DD3`) | **wired** (phase-10 T2, `fifa96_camera_record_event`) |
| `0x7A219` | `FUN_0007A084` (ball staging receive arm) | **wired** (phase-10 T1) |
| `0x7F4E7` | row 05 stage-0 tail (`0x7F3A1..0x7F57B`) | remaining (this doc §3.1) |
| `0x71B8A` | `FUN_0007131C` (camera-track boundary armer) | remaining (§3.2) |
| `0x6FA62` | `FUN_0006E8E8` (input-row machine post-handler tail) | remaining (§3.3) |
| `0x77423` | row 1B body `0x1106E0[0x1B]=0x76D28` | remaining (§3.4) |
| `0x77DC6` | row 1C body `0x1106E0[0x1C]=0x77728` | remaining (§3.5) |
| `0x82A6C` | row 0D body `0x1106E0[0x0D]=0x8251C`, arm 3 | remaining (§3.6) |

Fresh `from_function` attribution of the 11 sites: `0x71B8A` -> `FUN_0007131c`,
`0x70DD1` -> `FUN_00070c08`, `0x7A219` -> `FUN_0007a084`, `0x6FA62` ->
`FUN_0006e8e8`; the other seven sites are inside row bodies Ghidra does not
define as functions (FU-138 §7 concern, unchanged).

**Fresh action-table read** (`read_memory 0x1106E0`, 132 B): `[4]=0x7E7C8`,
`[5]=0x7F194`, `[0x0D]=0x8251C`, `[0x1A]=0x7662C`, `[0x1B]=0x76D28`,
`[0x1C]=0x77728`, `[0x1D]=0x74EB0`, `[0x1E]=0x7550C`. The table is dword-wide
and indexed by code through `FUN_0007D9A4` (`EAX=code; SHL EAX,2;
ADD EAX,0x1106E0`, FU-137 §1.1). The engine dispatch for codes `0D/1B/1C` is
`-FIFA96_ERR_UNSUPPORTED` (ENGINE.md 21/80 wired; FU-137 §6.1 rows
`0D/1B/1C` "not ported (partial)").

---

## 3. Per-caller classification

### 3.1 `0x7F4E7` — row 05 stage-0 tail (carrier row)

Call bytes `0x7F4E7`: `e8 a8 27 ff ff` -> `0x71C94` (rel32 `-0xD858`,
0x7F4EC + rel = 0x71C94).

* **Driver chain (fresh).** Entity walk -> `FUN_0007D9A4(code=5)` ->
  `0x1106E0[5] = 0x7F194` (fresh table read) -> row-05 stage-0 ->
  the tail block `0x7F374..0x7F57B`. The row is **wired** (M2 phase-9 T2,
  `fifa96_match_action_05` / `fifa96_action_carrier_arm`), and the M2 tape
  dispatches a live code-5 record (ENGINE.md phase-10 entry).
* **Gating (first-hand).** `0x7F374 MOV EAX,[EBP+0x8E]; SAR EAX,0x18;
  CMP EAX,5; JNZ 0x7F65C` — the tail only runs when
  **byte `[rec+0x91] == 5`** (the carrier code). It then stages
  `[0x15872A]/[0x15872B]` and enters the stack target algebra
  (`0x7F3A1..`); both entry variants converge on the block at `0x7F48E`
  (the `0x8DC68` metric at `0x7F4BA`) and the call at `0x7F4E7`:
  `EAX=EBP` (rec = the carrier), `EDX=&[ESP+0xC]` (the staged local triple),
  `EBX=0` (height), `ECX=(int16)AX` where
  `CMP dword[0x157750],0x10; SETG AL` (`0x7F4D0..0x7F4E2`) — i.e.
  **ramp param 1 iff the ball height `[0x157750] > 0x10`**. The call is
  preceded by the `0x92820(EBP, 0x26)` sink (`0x7F4C9..0x7F4CB`).
* **Engine status.** The ported `fifa96_action_carrier_arm` reports the tail
  as `out.tail` with **no derived consumer** (handlers.c `2484..2489`):
  the native block is unported (OL-63), so the engine never reaches this
  call. The engine also stages `ball_height` from `mr->entities.ball.y`
  (handlers.c 2510, leg 15.b), not the exact cell.
* **Verdict: no** (engine) — containing row live, call inside the unported
  OL-63 tail. Native-reachable on a code-5 carrier stage-0 with the tail
  entry conditions.
* **Overlap.** Pan-event/tracked-side producer: `FUN_00071C94` stores the
  carrier as `[0x1577CA]` (`0x71D27`) — a candidate *tracked-side* producer
  (R1 §15.c family). The same call is the row-05 half of the ball/height pan
  family R2 consumes. Flag to R1/R2.

### 3.2 `0x71B8A` — the camera-track boundary armer's angle arm

Call bytes `0x71B8A`: `e8 05 01 00 00` -> `0x71C94` (0x71B8F + 0x105).

* **Driver chain (fresh xrefs).** `get_xrefs_to 0x71C94` attributes the call
  to `FUN_0007131c` (`0x7131C..0x71B98`). Fresh xrefs to `FUN_0007131C` =
  **1**: `FUN_000736AC 0x73B9B` — the camera track (`fifa96_camera_update`).
  Fresh xrefs to `FUN_000736AC` = 2: `FUN_0004B100 0x4B193` (the per-frame
  match body; the engine's frame loop) and `0x743EF`. So the drive is the
  live per-frame camera track, not a gameplay row.
* **Call gate (first-hand).** `0x73B70 MOV EAX,[0x15774C]; abs; CMP EAX,0x6C0;
  JG 0x73B9B`; else `[0x157754]` abs `CMP EAX,0xAB0; JLE 0x73BA0` — the
  armer runs iff **|camera.x| > 0x6C0 OR |camera.z| > 0xAB0**
  (`0x73B70..0x73B99`; matches the engine gate at match_run.c 2883).
  Inside `FUN_0007131C` the angle arm is entered at `0x718A9` from three
  paths (`0x71388` JNZ when byte `[0x15781D]` != 0; `0x713A0` JNZ when
  `[0x157A4D]` (phase byte) not in {2, 0x10}; `0x713C0` JLE when the distance
  gates fail) and then requires its own
  `|vel_z| > 0xB10 || |vel_x| > 0x720` (`0x718B0..0x718C3`). Zone
  `[0x15781E]` != 0 or range bit0 (`[0x14C1D4]|[0x14C1D6] & 1`) clear routes
  past the mirror to `0x719A0`; there
  `[0x1577F0] != 0 || [0x1577BE] != 0`, then the velocity/height threshold
  chain (`0x719DA/0x71A08`, `0x71A5D/0x71AC5/0x71AF2/0x71B1B`) sets the
  local flag when **the ball height `[0x157750]` exceeds the
  velocity-scaled threshold**; the flagged path reaches `0x71B60`: vector
  angle from `0xCD514`, word `[ESP+2]/[ESP+4]` = ±0x180 lateral, then
  `MOVSX EBX, SI` with `SI = [0x157750] + 0x40`,
  `EAX = [0x1577CA]` (tracked), `ECX = 0`, `CALL 0x71C94` — a high-ball pan
  event on the tracked record. The flag-0 path joins `0x71972`
  (`[0x1577BE] = 0xCD514(...)`, no event).
* **Engine status.** The containing function is live: `fifa96_match_run_frame`
  calls `fifa96_match_goal_arm` whenever `fifa96_camera_out_of_bounds`
  (match_run.c 2878..2885; FU-145 §3 item 1). But that port covers the
  arm body `0x71390..0x713F7` and the already-armed zone==0 reflect only; the
  header contract names this exact arm "the unported angle arm, FU-71 leg
  9.6" (match_run.h 1149..1153), and the ported arm body returns 0 for
  `goal_zone != 0`.
* **Verdict: no** (engine) — live enclosing function, unported arm. Native
  fires under: camera out of bounds AND (armed or phase ∉ {2, 0x10}, or the
  small-distance fall-through) AND the velocity/height threshold chain above.
* **Overlap.** Direct **R2** overlap (same function the engine ships as the
  armer; this is the camera track's own pan producer). Also note a model
  divergence: natively the `0x718A9` arm runs when *disarmed* (phase
  ∉ {2, 0x10}, or phase 2/0x10 with |vel_z|≤0xB20 and |vel_x|≤0x730), while
  the engine's disarmed path returns 0 at the arming gates — see leg 5.
  Overlaps R1 only via the `[0x1577CA]` tracked re-bind.

### 3.3 `0x6FA62` — `FUN_0006E8E8` post-handler tail, aux state `0x22`

Call bytes `0x6FA62`: `e8 2d 22 00 00` -> `0x71C94` (0x6FA67 + 0x222D).

* **Driver chain (fresh xrefs).** Containing `FUN_0006E8E8`
  (`0x6E8E8..0x6FE83`); fresh xrefs = 2: `FUN_000782D0 0x785AD` (the keeper
  input-row machine) and `FUN_0007CA54 0x7CD30` (the outfield input-row
  machine). Both call sites immediately follow the machine's indirect action
  dispatch (`CALL dword ptr [EBP+0x18]` at `0x785A8` / `0x7CD2B`): the tail
  runs per record per frame after the handler. `FUN_0006E8E8` dispatches on
  the record's animation **aux word** — `MOV DX, word[EAX+0xE]` with
  `EAX = rec+0x28` (`0x6E8F3..0x6E8F6`, i.e. `word[rec+0x36]`, the FU-84
  `word[frame+2]` aux) -> `DEC; CMP AX,0x39; JA` -> 58-entry jump table
  `CS:[EAX*4+0x6E800]` (`0x6E918..0x6E928`). Fresh table read: entry
  `[33] = 0x6F9EB`; the arm runs `0x6F9EB..0x6FA8F` (next entry `[49] =
  0x6FA8F`), so **the call fires only when the aux low byte = 0x22**.
* **Arm gate (first-hand).** `0x6F9EB MOV EAX,[EBP+0x8E]; SAR EAX,0x18;
  CMP EAX,0x21; JNZ 0x6FA69` then `MOV EBX,[0x157A83]; CMP EBP,EBX; JNZ
  0x6FA69` — requires **byte `[rec+0x91] == 0x21` (action code 0x21) AND
  rec == `[0x157A83]` (the controlled record)**. Otherwise the arm only sets
  the `[rec+0x1C]=1` ack and toggles the `0x22 << CL` bit of
  `word[rec+0x36]` and exits. The event path scales two per-type bytes
  (`[0x10F334]/[0x10F33C][byte[rec+0x8E]] * 0x50`, `0x6F9F9..0x6FA30`),
  takes the `0x8DC68` metric, then the same `[0x157750] > 0x10` ramp gate
  and `EBX=0` event: `MOV EAX,EBP; CALL 0x71C94` (rec = the controlled
  record).
* **Engine status.** No engine port of `FUN_0006E8E8` exists
  (`grep 6e8e8/6E800` over `src/`, `include/` = 0 hits); the input-row
  machine subset the engine runs (`fifa96_outfield_input_row`,
  `src/fifa96_loader/fifa96_outfield.c:202`, wired at match_run.c:687) ends
  at the edge dispatch and carries no tail; the engine record
  (`fifa96_match_run_record`) has **no +0x36 aux field**.
* **Verdict: no** (engine) — the tail is native-machine-only and unported.
  Native-reachable when a controlled record carries code `0x21` and an
  animation frame whose aux word low byte = 0x22 (leg 2).
* **Overlap.** **R1 adjacency**: the call sits one instruction after the
  `0x7CD60` handler dispatch inside the same `FUN_0007CA54` invocation
  (`0x7CD2B` indirect -> `0x7CD30` tail) — if T1/T2 wires the `0x7CD60`
  second arm, this is the same machine's next native call. Also a camera
  pan-event producer (R2 family).

### 3.4 `0x77423` — row 1B (keeper reposition B) rush event

Call bytes `0x77423`: `e8 6c a8 ff ff` -> `0x71C94` (0x77428 - 0x5794).

* **Driver chain (fresh).** `0x1106E0[0x1B] = 0x76D28` (fresh table read) —
  action code `0x1B`, keeper reposition B (`0x76D28..0x77727`, FU-79 §4).
  The containing stage-2 rush test is `0x76FC7..0x77636` (FU-79 §4.1).
* **Local gate (first-hand).** Entry block `0x77320` computes the event
  velocity from the timer difference (`[0x1577F2]-[0x1577F8]` ×
  `[0x1577C0]/[0x1577C2]`, `0x7733F..0x7736D`), takes the `0x8DC68` metric,
  sets `byte[0x157C57]=1` (`0x77392`) and, when the one-shot latch
  `byte[0x157823] != 0`, increments the `0x157ACC` counter (`0x77398..0x773B9`);
  then `0x773C1..0x7740B` bump the `0x157AD0` counter and fire
  `0x92820(EBP,0x1A)` + `0x651F0(0xE|0x5)`, and the call at `0x77423`:
  `EAX=EBP`, `EDX=&[ESP+8]`, `EBX=movsx SI`, `ECX=0`, with
  `byte[0x157A6C]=0` immediately before. Upstream pocket predicates
  (`[0x57C55]==0`, `rec != [0x57A83]`, `rec != [0x577CA]`, the
  `[rec+0x28]`/settings-word `[0x4C2F6]`/`0x74584` distance gates) are
  FU-79 §4.1-quoted — see leg 3.
* **Engine status.** Row `1B` is not in the wired 21 (ENGINE.md) and FU-137
  §6.1 lists it "not ported (partial)" (`keeper_reposition_*` helpers only):
  the engine dispatch answers `-FIFA96_ERR_UNSUPPORTED` for code `0x1B`;
  the body is unported.
* **Verdict: no** (engine, row unwired) — native-reachable only on a live
  keeper rush (phase-0xD/set-piece situations) satisfying the §4.1 gates.
* **Overlap.** Pan-event family (R2), behind an unwired keeper row; no R1
  contact.

### 3.5 `0x77DC6` — row 1C (keeper dive/lunge) camera event

Call bytes `0x77DC6`: `e8 c9 9e ff ff` -> `0x71C94` (0x77DCB - 0x6137).

* **Driver chain (fresh).** `0x1106E0[0x1C] = 0x77728` (fresh table read) —
  action code `0x1C`, keeper dive/lunge (`0x77728..0x77E94`, FU-79 §5).
  The call is in the **common tail** (`0x779DE..0x77E8B`, FU-79 §5).
* **Local gate (first-hand).** The tail's anim variant select is at
  `0x77C22..0x77C2F` (`MOV EAX,[EBP+0x28]; MOV AL,[EAX]; CMP EAX,0x3C; JNZ
  0x77CF6`): variant `[rec+0x28][0] == 0x3C` builds the camera velocity from
  `[0x1577BC` rotated by `0x795A4` (`0x77C35..0x77C61`); the else variant
  uses `[rec+0x73]<<2`/`[rec+0x75]<<2` (`0x77CF6..0x77D0A`). **Both variants
  converge** on `0x77D10` (the `0x8DC68` metric), the `0x700F4` seed call
  (`0x77D6C..0x77D82`, args 0/EDI/EAX/EDX), the slot lookup + `0x651F0`
  (`0x77D87..0x77DAA`), `0x92820(EBP,0x19)` (`0x77DB6`), and the event:
  `MOVSX EBX,SI; XOR ECX,ECX; LEA EDX,[ESP+0xC]; MOV EAX,EBP; CALL 0x71C94`.
  The tail's top-level gate (`word[rec+0x81]==0`, `[rec+0x3A]>>24` in
  {2,3}) is FU-79 §5-quoted — see leg 4.
* **Engine status.** Row `1C` unwired ("not ported (partial)",
  `keeper_lunge_track` helper only, FU-137 §6.1 / FU-140 OL-35): dispatch
  `-FIFA96_ERR_UNSUPPORTED`.
* **Verdict: no** (engine, row unwired). Native-reachable in a keeper dive
  situation via the common tail.
* **Overlap.** Pan-event family (R2), unwired keeper row; no R1 contact.

### 3.6 `0x82A6C` — row 0D arm 3 event

Call bytes `0x82A6C`: `e8 23 f2 fe ff` -> `0x71C94` (0x82A71 - 0x10DDD).

* **Driver chain (fresh).** `0x1106E0[0x0D] = 0x8251C` (fresh table read) —
  action code `0x0D`. The head (`0x8251C..0x8255F`) accumulates
  `[rec+0x89] += [0x157A64]`, clears `[0x157A83]` when it is rec
  (`0x82540..0x82546`), then `MOV AL,[EBP+0x92]; CMP AL,3; JA 0x826F8;
  JMP CS:[EAX*4+0x8250C]`. Fresh arm-table read: `[3] = 0x826D5`; the call
  lies in arm 3 (`0x826D5..0x82ACE`), i.e. it fires when
  **`byte[rec+0x92] == 3`**.
* **Local gate (first-hand).** Arm 3 builds a polar vector with the
  `0x114E04` fold and `0x795A4` rotate (`0x82950..0x829FF`), calls
  `0x8DC68`, `0x92820(EBP,0x0B)` (`0x82A4A`), `0x92AC8` (`0x82A4F`) and
  the event with `EBX = (int16)((rand & 0x3F) + [0x157750])` — the random
  event height on the ball-height base (`0x82A56..0x82A6A`) — then
  `0x8F188(EAX=0x6C, EBX=0x40)`, the `0x651F0`/`0x974F0(0x1F4)` sinks,
  `byte[rec+0x92]++`, `[rec+0x89]=0` and the optional `0x7DAB4` reset
  request (`0x82A71..0x82AC8`).
* **Engine status.** Row `0D` unwired ("not ported (partial)"; FU-137 §6.1 /
  FU-138 OL-22): dispatch `-FIFA96_ERR_UNSUPPORTED`.
* **Verdict: no** (engine, row unwired). Native-reachable when a code-0D
  record's stage byte `+0x92` reaches 3.
* **Overlap.** Pan-event family (R2), unwired row; no R1 contact.

---

## 4. Verdict table (engine's current wired subset)

| site | containing body | invoked by | engine-reachable now | gating cell/condition | R1/R2 overlap |
|---|---|---|---|---|---|
| `0x7F4E7` | row 05 stage-0 tail | row-05 dispatch (wired) -> tail | **no** | `byte[rec+0x91]==5`; tail `out.tail` unported (OL-63); ramp `[0x157750]>0x10` | R1/R2 family: tracked=carrier bind; ball-height pan |
| `0x71B8A` | `FUN_0007131C` angle arm | `FUN_000736AC` camera track (live) | **no** | out-of-bounds `|camX|>0x6C0 \|\| |camZ|>0xAB0`; `|vel_z|>0xB10 \|\| |vel_x|>0x720`; zone/range routing; height-vs-velocity flag | **R2 direct** (unported FU-71 leg 9.6); R1 via `[0x1577CA]` |
| `0x6FA62` | `FUN_0006E8E8` tail arm idx 33 | `FUN_0007CA54 0x7CD30` / `FUN_000782D0 0x785AD` (unported tails) | **no** | aux `word[rec+0x36]` low byte `0x22`; `byte[rec+0x91]==0x21`; rec == `[0x157A83]` | **R1 adjacency** (same machine as `0x7CD60`); R2 family |
| `0x77423` | row 1B (`0x76D28`) rush arm | row-1B dispatch (unwired) | **no** | stage-2 rush gates (FU-79 §4.1); `byte[0x157823]` one-shot | R2 family; unwired row |
| `0x77DC6` | row 1C (`0x77728`) common tail | row-1C dispatch (unwired) | **no** | `word[rec+0x81]==0`; `[rec+0x28][0]==0x3C` variant; `[rec+0x3A]>>24 in {2,3}` (FU-79 §5) | R2 family; unwired row |
| `0x82A6C` | row 0D (`0x8251C`) arm 3 | row-0D dispatch (unwired) | **no** | `byte[rec+0x92]==3` (table `0x8250C[3]=0x826D5`) | R2 family; unwired row |

All six are **no** in the current engine; the distinct reasons are the
unported tail (row 05 / armer / machine tail) and the unwired row bodies
(1B/1C/0D). No caller is "unknown": every one has a proven containing body
and invoke chain.

---

## 5. Overlap flags for R1 / R2

* **R2 (live camera place/pan).** `0x71B8A` is the strongest overlap: it is
  the camera track's own boundary pan-event producer (the function the engine
  already runs as `fifa96_match_goal_arm`; the arm is the documented unported
  FU-71 leg 9.6). `0x7F4E7`, `0x6FA62`, `0x77423`, `0x77DC6`, `0x82A6C` are
  all `FUN_00071C94` pan-event producers on records and belong to the same
  family; they sit behind unported tails/unwired rows.
* **R1 (the `0x7CD60` second arm / tracked-side).** `0x6FA62` is the direct
  adjacency: `FUN_0006E8E8` is called at `0x7CD30`, one instruction after the
  `0x7CD60` handler is invoked through `CALL [EBP+0x18]` at `0x7CD2B` in the
  same `FUN_0007CA54` machine. `0x7F4E7` is a tracked-side producer candidate
  (`FUN_00071C94` stores the carrier into `[0x1577CA]` at `0x71D27`), the
  surface §15.c names.
* None of the six is a `FUN_00073E08` place-family site (the R2 place
  question is separate); all six are event/pan (`0x71C94`) sites.

---

## 6. Numbered legs

1. **FU156-L1 — `0x6FA62` arm vector x-word source.** In the `FUN_0006E8E8`
   frame the arm writes only `word[ESP+2]`/`word[ESP+4]`
   (`0x6FA17/0x6FA30`); the event vector passed as `EDX=ESP` (`0x6FA5B`)
   includes `word[ESP+0]`, which the head (`0x6E8EE..0x6E928`) does not
   initialize. The `0x8DC68` call masks the low half by `SAR 0x10`
   (`0x6FA39..0x6FA3F`), but `FUN_00071C94` copies all six bytes
   (`0x71CC8/0x71CC9`) — whether the native event x word is a don't-care
   stack residue or written outside the read window is unproven.
2. **FU156-L2 — aux `0x22` animation producer.** The arm-33 selection reads
   `word[rec+0x36]`, FU-84's `word[frame+2]` animation aux; the frame
   carrying aux low byte `0x22` lives in animation data (ISO), not statically
   provable from the EXE, and the engine record models no +0x36 field — the
   arm cannot be woken engine-side until that word is staged.
3. **FU156-L3 — row 1B stage-2 upstream predicates.** The local block
   (`0x77320..0x77427`) is first-hand; the entrance algebra
   (`0x76FC7..0x77320`: `[0x57C55]`, `rec != [0x57A83]`, `rec != [0x577CA]`,
   `[rec+0x28]` pocket, `[0x4C2F6]`, `0x74584`, `[0x57750]`) is FU-79 §4.1
   quoted, not re-derived instruction-level in this slice.
4. **FU156-L4 — row 1C common-tail top-level gate.** The local block
   (`0x77C04..0x77DC6`) is first-hand; `word[rec+0x81]==0` and
   `[rec+0x3A]>>24 in {2,3}` are FU-79 §5 quoted, not re-derived here.
5. **FU156-L5 — armer disarmed-path divergence.** Natively `0x718A9` is also
   entered **disarmed** (`0x713A0` phase ∉ {2,0x10}, `0x713C0` small
   distances) and can mirror (`0x71908..0x7196d`) or run the angle arm;
   `fifa96_match_goal_arm` returns 0 for those states (its reflect arm runs
   only when `goal_armed`). Whether the engine should model the disarmed
   mirror/angle paths is open (R2/T1 scope).
6. **FU156-L6 — row 0D arm-3 semantics.** The arm boundary (`0x826D5`,
   table `0x8250C[3]`) and the event/stage-advance tail are first-hand, but
   the row-sequence situation that drives `byte[rec+0x92]` to 3 is not named
   (FU-138 leaves arms 1-3 open).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_xrefs_to 0x71C94` (11), `0x7131c` (1), `0x6e8e8` (2), `0x736ac` (2);
`get_function_by_address 0x7131c`, `0x6e8e8`, `0x7ca54`, `0x782d0` (and the
four row sites -> "no function", first-hand); `disassemble_bytes`
`0x7F3A1`(480B), `0x71A80`(280B), `0x7131C`(340B), `0x718A9`(300B),
`0x719D4`(180B), `0x71C94`(300B), `0x6E8E8`(200B), `0x6F9E0`(96B),
`0x6FA20`(256B), `0x77320`(162B), `0x77280`(160B), `0x774C0`... window
`0x773C0`(256B), `0x77C00`(114B), `0x77C70`(182B), `0x77D20`(256B),
`0x82950`(180B), `0x8251C`(75B), `0x82A00`(256B), `0x7CD00`(110B),
`0x78580`(90B), `0x73B60`(64B), `0x7F370`(50B); `read_memory 0x6E800`
(232 B), `0x1106E0` (132 B), `0x8250C` (16 B). Engine source read:
`src/fifa96_engine/fifa96_match_run.c` (1915..1994, 2840..2914),
`fifa96_match_handlers.c` (2250..2310, 2395..2464, 2466..2560),
`include/fifa96_engine/fifa96_match_run.h` (393..470, 1125..1194);
`src/fifa96_loader/fifa96_outfield.c` (195..260). No writes: no rename,
comment, label, function, script or project save; `/FIFA96.EXE` and
`/fifa96_le.bin` untouched. No git commands.
