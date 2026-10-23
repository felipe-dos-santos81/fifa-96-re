# FU319 — the mover's `+0x43` direct-face producer and semantics (phase-32 R2-L6)

**Phase/Provenance:** phase-32 recon-ahead wave, Stage-1 **Task 2 of 4** (`docs/superpowers/plans/2026-10-22-fifa96-m2-phase32-recon-ahead.md`), scoped by `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md` §2/§3.2/§8–§9. Read-only derivation: fresh Ghidra MCP against **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0, `.image` overlay — `get_current_program_info` this session), the repo's engine sources, and one out-of-tree measurement probe (`/tmp/opencode/fu319/`, linking `build/libfifa96_*.a`; no repo file modified, ISO read-only). No production code, no test/golden change, no git. All addresses are **flat image** addresses; LE globals are quoted in the `+0x100000` file-offset form.

## 1. The question

The port's `match_run_record_mover` stages the native `+0x43` byte as `direct_face = 0` (`src/fifa96_engine/fifa96_match_run.c:498`), so the native locomotion turn (`0x8E29C`/`0x7BF89`, ported at `src/fifa96_loader/fifa96_action_handlers.c:219-246`) can never rotate the record's `+0x7D`. What is the native `+0x43` producer and its exact semantics, what else does the turn read (`[0x157A73]`, `face_x/face_z`, `move_attr`, `speed`/`has_slot`), what must the port stage, what sim cells change, and what is the M2 exposure? Deliverable: the producer/semantics, the port field and staging shape, the expected sim/golden diff, **GO/NO-GO**, legs `FU319-L1…`.

## 2. The native `+0x43` producer (fresh)

### 2.1 The census: three writers, two readers, no other site

| Claim | Evidence (fresh this session) |
|---|---|
| `+0x43` is written at exactly **three** record sites — `0x6E6C4` (`FUN_0006E598`), `0x8DFBA` (`FUN_0008DF84`), `0x8540C` (no function; row-0x22 body) — and **read at exactly two**, `0x7BF89` (`FUN_0007BF20`) and `0x8E29C` (`FUN_0008E244`) | `search_instructions operand_pattern "+ 0x43]"` → `match_count 6`, `truncated false`: the three writes, the two reads, and `0xAE2C9` (excluded in the next row). The wider `"0x43"` scan (40 hits, truncated) found no other `[reg+0x43]` form |
| `0xAE2C9 MOV byte[EDI+0x43],BL` is **not** a match record: `FUN_000AE218` walks a **3-byte-stride** struct (`0xAE2B3`/`0xAE2BC` `[EAX+0x14]`/`[EAX+0x15]`, `ADD EAX,3`; `0xAE2C2 ADD EDI,3`) with `dword[ESI+0xC]`/`dword[ESI+0x28]` count fields | `disassemble_bytes 0xAE2A0/0x50`; matches FU159 §4.1's fifth-`+0x44`-store exclusion (`FUN_000AE218`, "outside the match-record address family") |
| The two reads are the **only** consumers, and both are the locomotion turn (the `FUN_0007BF20`/`FUN_0008E244` twins) | `0x7BF89 CMP byte[EBP+0x43],0` / `0x8E29C CMP byte[EBX+0x43],0` — fresh bytes `80 7d 43 00` / `80 7b 43 00` |

### 2.2 The write site, instruction by instruction

Both writers are the **animation-row install**, and both write the *same four* flag bytes from the row's flags byte:

```
0x6E6A9  MOV EDX, [0x157588]          ; row table base (111 x 9 B, installed by 0x73D91)
0x6E6A0/A7  LEA EAX,[EDX*8] ; ADD EAX,EDX          ; id*9
0x6E6B1  MOV [ESI+0x28], EDX          ; rec->row_ptr = table + id*9
0x6E6B4  MOV AL, byte[EDX+0x2]        ; row flags
0x6E6B7/ B9/ BC  AH = AL & 4 ; [ESI+0x45] = AH
0x6E6BF/ C1/ C4  AH = AL & 2 ; [ESI+0x43] = AH      <-- the producer
0x6E6C7/ C9/ CC  AH = AL & 1 ; [ESI+0x44] = AH
0x6E6D8  MOV byte[ESI+0x46], 1        ; reload marker
0x6E6DC..0x6E6FB  [ESI+0x3F] = 2 / -2 / 0 (flags & 0x10 / & 0x20)
0x6E6FE..0x6E706  CALL 0x6E490 ; 0x6E70B word[ESI+0x32] = 0
```

| Claim | Evidence (fresh) |
|---|---|
| `byte[rec+0x43] := row_flags & 0x02` at `0x6E6C4`, i.e. the value set is **{0, 2}** and the consumer's test is a zero/nonzero flag | `disassemble_bytes 0x6E680/0x90`, bytes `80 e4 02` `88 66 43`; the alias traps listed in `AGENTS.md` (word-at-`addr+2` / byte-at-`addr+3`) do not apply to a byte store |
| The pool twin `FUN_0008DF84` does the identical derivation: `AL = DL & 2` (`0x8DFB8`) → `[ESI+0x43] = AL` (`0x8DFBA`), with `+0x45` (`0x8DFB3`) and `+0x44` (`0x8DFC1`) beside it | `disassemble_bytes 0x8DF84/0x80`; `FUN_0008DF84` has **7 callers**: 5 inside `FUN_0008E008` (`0x8E0F9`, `0x8E12F`, `0x8E19A`, `0x8E1F7`, `0x8E218`) plus `FUN_0008E5BC`/`FUN_0008E910` (fresh `get_xrefs_to 0x8DF84`) |
| The only **clear** is `0x8540C MOV byte[ESI+0x43],0`, in the row-**0x22** body (entry `0x8539C PUSH EBX`, phase gate `0x853AB CMP [0x157A4A]>>24,2`), stage 0: `+0x9E=1` → `0x79B1C` snap → `+0x43=0` → `+0x89=0` → `+0x92++` | `disassemble_bytes 0x853C0/0x90` and `0x85390/0x20`; the port names the same body `fifa96_match_action_22` (`0x8539C..0x85481`, FU-185 §2) and records the missing cell as FU185-L3 (`handlers.c:5218`) |
| `+0x43` is a **latch**, not a per-frame derivation: it is rewritten only at an install and cleared only by row 0x22 stage 0. `FUN_0007997C` clears the row **pointer** (`0x799BF MOV dword[EBP+0x28],0`) **without** touching `+0x43` | `disassemble_bytes 0x7997C/0x60`; the store at `0x799BF` |

### 2.3 Semantics — the row's "steer the facing" bit

| Claim | Evidence (fresh) |
|---|---|
| The value is the current animation row's flags bit 1; the 111 rows `0x00..0x6E` are the 9-byte records at flat **`0x10EF00`**, and `[0x157588]` is installed from there | `read_memory 0x10EF00/0x40` (row 0 `00 00 07 …`, row 4 `04 05 00 …`, row 5 `05 05 02 …`), `0x10EFA0/0x20` (row 0x13 `13 03 02 …`), `0x10F050/0x18` (row 0x26 `26 00 07 …`); port table parity: 111 rows, `src/fifa96_engine/fifa96_match_run.c:47-159` |
| **38 of 111** rows in the table the port carries verbatim carry bit 1. Notably row **0** = `0x07` (bit 1 **set**), row **0x26** = `0x07` (set), row **0x04** (the walk row) = `0x00` (**clear**), row **0x05**/**0x13** = `0x02` (set), row **0x24** = `0x04` (clear) | fresh table reads above (rows 0–5, 0x13, 0x26) + the port table dump (`match_run.c:47-159`, 111 rows) |
| The bit's meaning is *"this row steers the facing"*, and the port already uses the same bit for the **ball** heading steer (`0x158881`, the reset's row-3 install `FUN_0006E724(...,3,0)` → row 3 flags `0x07` → `&2 = 2`) | `src/fifa96_engine/fifa96_match_run.c:4246-4261` (`b->head_gate`, the `0x8BE4C..0x8BE82` focus-point arm) — the in-repo precedent for both the bit's semantics and the staging shape |
| **It is live for every record in normal play.** The kickoff placement tail (`FUN_00079B6C` `0x79BE0..0x79C1C`) installs a row for every record: `0x79BF1 CMP byte[rec+0x8D],0` → `active ? 0 : 0x26` → `0x79C13 CALL 0x6E598`. Both targets (row 0, row 0x26) are flags `0x07` → `+0x43 = 2`. The team placement `FUN_0008E5BC` does the same by another route: `0x8E69E XOR EDX,EDX` (row 0) → `0x8E6AF CALL 0x8DF84` (twin `0x8EA08`/`0x8EA0C`) | `disassemble_bytes 0x79BE0/0x40`, `0x8E690/0x30`, `0x8E9F0/0x30`; `FUN_0006E598`'s id clamp maps negative/**≥0x6F** to 0 (`0x6E68E..0x6E69B`), so id 0 is a real row, not "none" |

**Consequence.** The native turn is not a fringe path: after the kickoff placement every record's `+0x43` is `2`, and it stays `2` until an install rewrites it (with the new row's bit 1) or row 0x22 stage 0 clears it. The port's `direct_face = 0` therefore suppresses a **live, universal** native behaviour, not an edge case. The value is `2` on every path through that install: if the call lands **outside** phase 2 (`0x6E613`/`0x6E61C` not taken) with `DX = 0`, the head's `0x92AC8`/jump-table arm (`0x6E659..0x6E685`) may substitute row `0x62` or `0x65` (`0x6E675`/`0x6E679`/`0x6E680`), and both are flags `0x07` as well (fresh `read_memory 0x10F270/0x24`: rows `0x62..0x65` all `.. 00 07 ..`).

## 3. The consumer — the locomotion turn (fresh bytes, both twins)

`0x8E29C..0x8E362` (`FUN_0008E244`) and `0x7BF89..0x7C051` (`FUN_0007BF20`) are instruction-for-instruction the same block (operand registers only):

| Native | Instruction (fresh) | Meaning | Port (`fifa96_action_handlers.c`) |
|---|---|---|---|
| `0x8E29C` | `CMP byte[EBX+0x43],0 ; JZ 0x8E368` | no turn when the latch is clear | `:219` |
| `0x8E2A6` | `MOV EAX,dword[EBX+0x63] ; SAR EAX,0x10 ; CMP EAX,0x60 ; JLE …` | `move_attr >> 16` = **word `+0x65`** = distance left | `:221` |
| `0x8E2B1` | `MOV AX,word[EBX+0x7F]` | distance **> 0x60** → face the **target** (the block-B desired facing) | `:222` |
| `0x8E2B7` | `MOV EDX,dword[0x157A73] ; … CX=[EDX] ; AX=[EDX+8] ; SUB pos…` | else face the **focus point**: `P[0]`, `P[8]` (x, z) | `:223-228` |
| `0x8E2D0` | `TEST CX,CX ; JNZ ; TEST AX,AX ; JZ 0x8E2E7` | both deltas zero → keep `+0x7D` | `:229-230` |
| `0x8E2EB` | `SUB AX,word[EBX+0x7D] ; JZ 0x8E368` | error = target − face, skip if 0 | `:233` |
| `0x8E2F5` | `AND AH,0x3` / `CMP 0x200` / `SUB EAX,0x400` | fold to −0x200..0x200 | `:233` |
| `0x8E309` | `EDX=0x10 ; SUB DX,word[+0x71] ; SHL EDX,2 ; [rec+0x20]!=0 → SHL EDX,2` | turn limit `(0x10 − speed) << (slot ? 4 : 2)` | `:235-236` |
| `0x8E31F..0x8E336` | clamp to ±limit (sign-extended compares) | | `:238-240` |
| `0x8E338..0x8E350` | `ADD AX,word[+0x7D] ; AND AH,3 ; fold ; MOV word[+0x7D],AX` | **writes the face** | `:241-242` |
| `0x8E354..0x8E362` | `(face & 0x3FF) >> 5` → `byte[0x1104D2 + i]` → `byte[+0x8E]` | **writes the octant** from the heading table | `:243` |

### 3.1 The turn's other inputs — the focus point `[0x157A73]`

| Claim | Evidence (fresh) |
|---|---|
| `[0x157A73]` is a **pointer to a vec3**; the turn reads `+0` (x) and `+8` (z). Its default/immediate value is the **camera triple `0x15774C`** (x `0x15774C`, y `0x157750`, z `0x157754`) | `0x8E2B7`/`0x7BFA4` bytes; `read_memory`-backed xrefs: `0x74107 MOV dword[0x157A73],0x15774C` in the phase-2 entry `0x740ED..0x7410D` (`disassemble_bytes 0x740A0/0x70`), `0x74274 MOV [0x157A73],EBX` with `EBX=0x15774C` (`0x74268`/`0x7426D`), `0x73E60`/`0x74065` (caller-supplied pointers) |
| The **non-default** values are the foul spot `0x1587C0` (`0x89FBD`, FU-187 §1 / FU-150) and the foul-row restore (`0x8A3B9`, FU-150 row 6). The other readers are the kick aim (`0x8BE4C`), the placement reset (`0x799B5`), `FUN_0008C068` (`0x8C0B0`) and `0x90661/0x90688` | fresh `get_xrefs_to 0x157A73` = 13 (6 WRITE, 7 READ); `disassemble_bytes 0x89FB0/0x18`, `0x8A3B0/0x18` |
| The port already models the *same* focus arm for the ball heading with the camera triple as the point and cites the foul-spot override as FU-150's | `src/fifa96_engine/fifa96_match_run.c:4246-4261` |
| `move_attr` is `dword[rec+0x63]`; its high word is the **block-A distance `+0x65`** (metric `0x8DC68`, stored by the mover's own block A), and `>>19` for the stride attr is `distance >> 3` | carried: FU77 §1.2 `0x8E267..0x8E278` (byte-for-byte block A); the turn's `SAR EAX,0x10` at `0x8E2A9` fresh |
| `speed` = word `+0x71`; `[rec+0x20]` is the control-slot dword (zero/nonzero only) | `0x8E30E`/`0x8E312` fresh |

## 4. The port audit

| Claim | Evidence |
|---|---|
| The **latch cell does not exist**: `struct fifa96_match_entity` and `struct fifa96_match_run_record` carry `row44` (+0x44) but no `+0x43`; `fifa96_anim_flags` already decomposes the row byte (`terminal`/`bit1`/`bit2`/`lean`) but no engine site consumes `bit1` | `include/fifa96_engine/fifa96_match_entities.h:98-103`, `:165`; `include/fifa96_engine/fifa96_match_run.h:489`, `:500`; `include/fifa96_loader/fifa96_animation.h:23-30`, `src/fifa96_loader/fifa96_animation.c:25-34` (single definition, no `src/` caller) |
| The installs write `row44` only: `:1029` (`row.flags & 1`) and `:1067` (successor); neither writes `+0x43` | `src/fifa96_engine/fifa96_match_run.c:1016-1035`, `:1039-1070` |
| The mover stages `direct_face = 0` (memset) and never sets `face_x`/`face_z`; it writes the face back at `:532` from a value the turn never changes | `src/fifa96_engine/fifa96_match_run.c:511` (`memset`), `:498` (comment), `:527-537` |
| The kickoff placement installs the row **without** the install function (`fifa96_arm_anim_select(active ? 0 : 0x26, …)`), so no `row43`/`row44` write; `row44` is recovered later from `anim_id` by the drain (`:1352`) — which is exactly why the missing `+0x43` never surfaced | `src/fifa96_engine/fifa96_match_entities.c:565-568`; `src/fifa96_engine/fifa96_match_run.c:1349-1354` |
| The row-0x22 clear is documented but not modelled (`/* 0x8540C byte[+0x43] = 0 … */` at `:5218`) | `src/fifa96_engine/fifa96_match_handlers.c:5193-5221`; FU185-L3 |
| `move_attr` is staged from `fifa96_arm_dist_stage` = `0x8DCD4`, **not** the `0x8DC68` metric the native stores at `+0x65` | `match_run.c:521-522`; `src/fifa96_loader/fifa96_arm_helpers.c:38-71` (the `0x8DCD4` octagon) vs `fifa96_entity_distance` (`0x8DC68`) |
| The **turn function itself is already pinned** (all branches: target, focus, zero-delta keep, `direct_face==0` no-op); only the *staging* is untested | `tests/test_action_locomotion.c:118-149` (`test_step_direct_face`) |
| The staged-zero inputs are declared in FU-77 §5 (locomotion) and ENGINE.md §… (`+0x6F` stride rate, `+0x43` direct-face, `0x57A73`) | `docs/ghidra/FU77_locomotion.md:596-600`; `docs/ENGINE.md:5037`, `:5096` |

## 5. The minimal faithful staging

**Fields.** Add a `uint8_t row43` latch beside `row44` in `struct fifa96_match_entity` and `struct fifa96_match_run_record` (native `byte[rec+0x43]`, values `{0,2}`, zero at pool init).

**Writers (three, matching the native census).**
1. `fifa96_match_run_record_anim_install` — `e->row43 = (uint8_t)(row.flags & 0x02u);` beside `row44` (`0x6E6C4`).
2. `fifa96_match_run_record_anim_successor` — the `0x8DF84` twin of the frame-wrap install (`0x8DFBA`), beside the `row44` write at `:1067`.
3. The kickoff placement (`fifa96_match_entities.c:565-568`) — the native tail is `FUN_0006E598(rec, active ? 0 : 0x26, 0)` (`0x79C13`), i.e. a real install: set `row43` from the selected row's flags (the narrow write, which reproduces the native `+0x44 = 1` through the same `row.flags & 1`) rather than routing the whole call through `fifa96_match_run_record_anim_install`, whose `anim_timer`/`aux_seen` side effects are collateral at a placement site. `0x8E5BC`/`0x8E910` (the team placement passes, no `src/` site) install the same row 0 by the other route; their value lands identically.

**Clear (one).** `fifa96_match_action_22` stage 0 (`handlers.c:5214-5221`) — `r->row43 = 0; e->row43 = 0;` at the `0x8540C` site (the pool and the staging record agree at the record loop's drain, `match_run.c:1561`).

**Mover staging.** In `match_run_record_mover`:
```c
s.direct_face = r->row43;                       /* +0x43 */
s.face_x = (int16_t)mr->render.camera.pos_x;    /* [0x157A73] -> 0x15774C x */
s.face_z = (int16_t)mr->render.camera.pos_z;    /* [0x157A73] -> 0x157754 z */
```
`face_x/face_z` are `int16_t` in `fifa96_action_locomotion` and the native reads **words** (`MOV CX,[EDX]`, `MOV AX,[EDX+8]`), so the truncation to 16 bits is the native width, not a slip.

**What it changes.**
- **The face and the octant**: for every record whose latched row has bit 1 (which after kickoff is *every* record, §2.3), `+0x7D` rotates toward its target (distance > 0x60) or the camera focus (distance ≤ 0x60) at ≤ `(0x10 − speed) << (slot ? 4 : 2)` per frame, and `+0x8E` follows the `0x1104D2` heading table.
- **The gates that read the face**: the row-04 half-line angle gate `0x7EE5C` (`outfield.c:632`, via the live staging `handlers.c:2654`), the row-04 defender decision (`outfield.c:384`), the row-0x0B `±0x100` turn clamp (`handlers.c:1393`, and its opponent gate `:1063`), the row-0x25 sine fold (`:3425`), the row-0x1B face read (`:4061`), and the row-0x16/0x24 re-faces (`:5318`, `:5395`).
- **The ball pairing / AI**: only through the face-reading gates above and through `+0x8E` in the impulsion arms (`0x6EE1B`/`0x6F5E8`/`0x6E997`, port `match_run_record_impulse`) when the current frame's aux code is one of them. For the records that turn in the measured window the aux word is `0x0000` (row 0x26 frame 0 = `40 00 00 00 00`, fresh `read_memory 0x10E5EE/0x0A`), so no impulsion feedback exists there.
- **The focus point is in the minimal slice**, per spec §9.2: it *is* in the native chain (`0x8E2B7`) and the default value is the camera triple the port already holds. The non-default focus pointers are a leg (FU319-L1).

**Not in the minimal slice (rejected shortcuts).**
- `s.direct_face = (row.flags(anim_id) & 2)` derived in the mover (the `row44` derivation shape): it coincides with the native latch *at kickoff* (row 0) but diverges after a `+0x28 = 0` reset (`0x799BF` keeps `+0x43`) and after the row-0x22 clear — both would be re-invented as `2`.
- Any staging of `face_x/face_z` from the record's target: the native point is the global focus, not a per-record cell.

## 6. The expected sim and M2 diff

**Measured state of the natural tape (`M2_KEYS`, 12 000 steps, ISO present, out-of-tree probe `/tmp/opencode/fu319/probe.c`, dump `anim.txt`, 67 320 lines = 3 060 phase-2 granted frames × 22 records):**

| Quantity | Measured |
|---|---|
| phase-2 granted frames / code-4 record-frames | 3 060 / 6 120 |
| code-4 records | entity 2 (team 0 #2) and entity 12 (team 1 #1) |
| `anim_id` distribution | code 4 → **0** (6 120/6 120); code 3 → 0 (55 079); code 0x19 → **0x26** (6 120, entities 0 and 11) |
| `face7d` over the window | **0x100 for all 22 records on all 3 060 frames** (the `0x8E = 2`/face 0x100 placement constant, `0x8E691` octant 2 / `0x8E69A` face) |
| band record (lane ≤ 0x40, `0x8DC68`, post-frame) | entity 12 only: **2 082** frames (FU314's 2 081 dispatch-side count is one boundary apart, as its erratum records) |
| `entities.controlled` / `camera.tracked` | 1 (team 0 #1) / −1 (none) on every sampled frame |

**Open-loop turn replay** (the §3 formula applied per record-frame to the measured pos/target/speed/slot, face iterated; positions/targets are *not* re-integrated, so this is a prediction, not a run):

| Quantity | Result |
|---|---|
| entity 12 (the band record), nonzero turn frames | **9**, steps 1802..1829 (`0x100` → `0x140` → … → `−0x100` = `0x300`), then constant |
| entity 12, face at the band | **`−0x100` (=`0x300`)** on every band frame; bearing to the camera `0x300`; **Δ = 0 on 2 082 / 2 082** band frames. The target distance `+0x65` is `4..64` (`≤ 0x60`) on every band frame, so the turn takes the **focus** branch on all of them (`2 082 / 2 082`), and the target `(0,0)` coincides with the parked camera, so the target branch would give the same bearing |
| entity 2 (the other code-4 record) | never inside the band (lane > 0x40); 3 turn frames |
| `is_ball_track` at the gate | `id 12 != controlled 1` and `tracked = −1` → **0**, i.e. the arm-B tracked exclusion does not refuse the record (the FU314-L4 fallback is benign here) |

**Intended consequence (the gate).** With `+0x43` live, the band record's face is already `0x300` at the band entry: it has arrived (`+0x65 ≤ 0x60`) and faces the **focus point**, which is the camera triple — the `0x7EE7D` angle gate passes on every band frame, the row-04 body emits arm B (event `0x1D`), and the half-line pan arms on the **undriven** `M2_KEYS` tape (the phase-31 witness needed a hold only because the face was frozen). The pan then moves the camera/place, so the render hash changes from the first pan frame onward.

**Collateral.** Every record also turns (their latch is `2` from the kickoff row-0 install): over the 67 320 record-frames the replay turns on **4 140** (6.1 %), with each record carrying between **3 and 275** nonzero-turn frames, and the 22 steady-state faces are **20 distinct values** (none `0x100`). So `+0x7D`/`+0x8E` diverge broadly, but the turn itself is a short settling transient per record, not a per-frame spin. The face itself is **not** staged into the render (`match_run_scene_stage` copies pos/anim_id/frame/hidden only, `match_run.c:1679-1691`), so the hash moves only through (a) the pan's camera motion, (b) any downstream face-reading gate that changes a target/velocity, and (c) the `move_attr` metric divergence of FU319-L3.

**M2 accounting (predicted, not applied).**
1. The natural-pass baseline asserts flip **by design**: `tests/test_engine_m2.c:3701-3707` pins `camera_left_zero == 0` and `camera_zero_frames == granted_frames` as "the phase-30 NO-GO witness; **the bootstrap producer will flip it** and the P7/P8 window/lockout assertions land with it (FU313-L1)". The Stage-2 commit must invert them and land the P7/P8 assertions in the same commit.
2. Re-pin procedure (never in this wave): `./build/test_engine_m2 > /tmp/m2.new`; `diff -u tests/golden/engine/m2-frames.txt /tmp/m2.new | head` for the first differing frame; count changed `frame=` lines; prove the `state=` suffixes are unchanged (`diff <(cut -d' ' -f3- …)`); show the changed frames are the first pan frame onward (or the first intra-window movement frame) and nothing before; M1 must `cmp` byte-identical (`09b726b7…`); then re-pin with the written reason in `tests/test_engine_m2.c`'s provenance in the house form ("first differing line frame N, M hash lines, `state=` suffixes unchanged") plus the register pin; census stays `60/80`.
3. Any diff that starts **before** the first face-driven event, or that moves a frame no lane/arm can explain, is a stop-and-report (spec §8.3), not a re-pin.

## 7. Legs

1. **FU319-L1 — the non-default focus pointers.** The minimal slice stages `[0x157A73] = 0x15774C` (the camera triple). The foul spot `0x1587C0` (`0x89FBD`), the foul restore (`0x8A3B9`) and the phase setters (`0x73E60`, `0x74065`, `0x74274`) have no engine cell. Owner: the camera focus cell + the foul machine (FU-150/FU-187).
2. **FU319-L2 — the "row pointer NULL, latch set" state.** `FUN_0007997C` clears `+0x28` without `+0x43` (`0x799BF`); the port's `anim_id == 0` cannot distinguish a row-0 install from a cleared pointer, so the latch is *modelled* as a byte (correct at every install) but the post-reset state is not expressible. Owner: the anim install / pool staging. Does not change the measured window (no such reset after kickoff).
3. **FU319-L3 — the `move_attr` metric.** The native `+0x63 >> 16`/`>> 19` is the block-A `0x8DC68` distance; the port stages the `0x8DCD4` arm metric (`match_run.c:521-522`). Measured: **2** record-frames in the window straddle the `>0x60` branch (team 1 #6 and #10 at steps 4912/5292, `0x8DC68 = 97` vs `0x8DCD4 = 96`); max |Δ| = 115 units over the window. The `>>19` stride attr has the same wrong metric with an unmeasured blast radius. Owner: the mover staging (**must be a separately pinned sub-change**, because the stride index moves for every record).
4. **FU319-L4 — the frame-wrap install's frame position.** The native `FUN_0008DF84` has 5 sites inside `FUN_0008E008`; the port's single `fifa96_match_run_record_anim_successor` is called from the scene stage (`match_run.c:1781`), i.e. after the frame body. The `row43` write there inherits that existing one-frame placement (the same placement `row44` already has); the native per-frame ordering is not re-derived here. Owner: the FU-84 per-frame driver.
5. **FU319-L5 — the sibling flag cells.** `+0x45` (`flags & 4`) and `+0x3F` (`2/−2/0` from `flags & 0x10/0x20`) have no port cell either; `fifa96_animation_flags` exposes `bit2`/`lean`, unused. Their consumer census is not taken here. Owner: the anim install.
6. **FU319-L6 — (checked, not a leg) the face range parity.** The native masks to 10 bits and folds to `−0x200..0x200` before the store (`0x8E33C..0x8E350`); the port's fold (`:241-242`) lands the same range, so no divergence is claimed here. Re-open only if Stage-2 measures a wrap edge.
7. **FU319-L7 — the M2 blast radius.** Measured here only as the prediction of §6 (the pan from the first band frame; the face-changed records' downstream gates). Stage-2 must produce the actual frame diff.

## 8. Verdict

**GO on staging the live `+0x43` (and the focus point) as FU314-L5's fix.** The producer is fully proven and singular: `byte[rec+0x43] := row_flags & 0x02` at the two anim-row installs (`0x6E6C4`, `0x8DFBA`), cleared only by the row-0x22 stage-0 arm (`0x8540C`), read only by the locomotion turn (`0x7BF89`/`0x8E29C`). The native value is **2 for every record after the kickoff placement** (row 0 / row 0x26, flags `0x07`), so the port's staged `0` suppresses a live, universal behaviour; the turn's remaining inputs are exactly `move_attr` (`+0x63` high word), the focus pointer (default the camera triple), `speed` and the `+0x20` control slot, all of which the port holds except the focus staging.

**GO with a large, intended M2 move — and the phase-32 witness may fall out of it.** The open-loop replay shows the band record settles at face `0x300` = its target = the camera park, so the row-04 angle gate passes `2 082 / 2 082` band frames and arm B arms on the **undriven** tape. That is the intended consequence (the pan is the feature), it flips the natural-pass zero-camera baseline that the test itself marks as "the bootstrap producer will flip it", and it is exactly the `faithful-move` policy's case: a frame diff, a written reason, the register pin, M1 untouched.

**NO-GO on two shortcuts:** (i) deriving `direct_face` in the mover from `anim_id`'s row flags — it re-invents the latch the native keeps across the `+0x28 = 0` reset and the row-0x22 clear (FU319-L2); (ii) folding the `move_attr` metric fix into the same pin without its own RED/GREEN and blast-radius measurement (FU319-L3) — the stride index reads the same wrong metric for every record.

## 9. Provenance

**Fresh this session (Ghidra MCP, `/FIFA96.EXE`, `watcom:LE:32:default`, image base 0):** `get_current_program_info`; `get_xrefs_to` `0x8E244` (3), `0x7BF20` (2), `0x6E598` (84), `0x8DF84` (7), `0x157A73` (13), `0x8E5BC` (5); `search_instructions "0x43"` (40, truncated) and `"+ 0x43]"` (6, complete); `disassemble_bytes` `0x6E598/0xF0`, `0x6E680/0x90`, `0x8DF84/0x80`, `0x8E29C/0xD8`, `0x7BF89/0xE0`, `0x85320/0x60`, `0x85390/0x20`, `0x853C0/0x90`, `0x7997C/0x60`, `0x79BE0/0x40`, `0x8E690/0x30`, `0x8E9F0/0x30`, `0x73E50/0x18`, `0x74058/0x18`, `0x740A0/0x70`, `0x74268/0x18`, `0x89FB0/0x18`, `0x8A3B0/0x18`, `0xAE2A0/0x50`; `read_memory` `0x10EF00/0x40`, `0x10EFA0/0x20`, `0x10F050/0x18`, `0x10F044/0x0A`, `0x10F270/0x24`, `0x10E5EE/0x0A`.

**Carried (re-checked against the port sources, not re-derived):** FU77 §1.2 block A–E; FU159 §4.1 (the four flag stores and the `row_flags & 1` reading); FU185 §2/FU185-L3 (the `0x8540C` clear and the missing cell); FU314 §2.3/§3/§7.5 and its review erratum (the live `+0x7D` staging FU314-L2 and the measured band tuple); FU-188 §4 (row 0x24's `(1,0)` face).

**Cross-check with FU318 (sibling draft `docs/ghidra/FU318_r2l6_face_producers.md`, landed during this slice):** it reaches the same producer (`+0x43 = descriptor[2] & 2` at the row install) and the same staging shape (`direct_face` + the live focus); FU319 adds the `0x8540C` clear (the one non-install writer) and the proof that the value is `2` for every record after the kickoff placement. No conflict.

No prior-doc statement is contradicted here: FU314 §5/§7.5 (`+0x7D` live, the mover's `+0x43` staged 0), FU159 §4.1 (the flag stores), FU185-L3 (the missing `+0x43` cell) and `docs/ENGINE.md:5037` are **extended, not corrected** — FU319 supplies the producer and the staging shape those texts name as absent.

**Port/test sources read (read-only, cited by line):** `src/fifa96_engine/fifa96_match_run.c` (`:47-159`, `:489-548`, `:1002-1070`, `:1349-1354`, `:1413`, `:1561`, `:1680-1690`, `:1781`, `:4246-4261`); `src/fifa96_engine/fifa96_match_handlers.c` (`:1063`, `:1393`, `:2605-2654`, `:2667-2680`, `:3311`, `:3425`, `:4061`, `:5193-5221`, `:5266`, `:5290-5318`, `:5354-5395`, `:5928-5934`); `src/fifa96_engine/fifa96_match_entities.c` (`:529-571`); `src/fifa96_loader/fifa96_action_handlers.c` (`:190-246`); `src/fifa96_loader/fifa96_arm_helpers.c` (`:38-71`); `src/fifa96_loader/fifa96_animation.c` (`:25-34`); `include/fifa96_loader/fifa96_action_handlers.h` (`:122-146`); `include/fifa96_engine/fifa96_match_entities.h` (`:98-103`, `:165`); `include/fifa96_engine/fifa96_match_run.h` (`:489`, `:500`, `:511`); `tests/test_action_locomotion.c` (`:118-149`); `tests/test_engine_m2.c` (`:3638-3717`, `:4117`).

**Measurement artifacts (out of tree, `/tmp/opencode/fu319/`):** `probe.c` (a copy of `tests/test_engine_m2.c` with a `FU319_DUMP`-gated `run_natural_pass`-only main and a per-granted-frame, phase-2-only dump of the 22 records' `code`/`active`/`anim_id`/`face7d`/`type`/`speed71`/pos/target/`has_slot` plus `entities.controlled` and `camera.tracked`), its build line (the `build/CMakeFiles/test_engine_m2.dir/link.txt` library order), `anim.txt` (67 320 lines), `probe.log`. Two arithmetic replays over `anim.txt` (the §3 turn formula; the `0x8DC68` vs `0x8DCD4` comparison). No repo file was modified; the ISO was only read.

## 10. Known limits

- The turn replay of §6 is **open-loop**: positions/targets are the measured (face-staged-zero) ones, not re-integrated under the changed face. The face trajectory and the gate pass are therefore a prediction; the band record's target *is* its own park point, so the conclusion is robust to the re-integration, but Stage-2 must measure.
- The M2 hash covers the null backend's framebuffer planes + palette (`src/fifa96_engine/platform_null.c:39-44`), and the scene staging does **not** pass `+0x7D`/`+0x8E` (`match_run.c:1679-1691`), so the first hash change is expected at the first camera move, not at the first face change. That expectation is not proven frame-by-frame here.
- The band count is post-frame sampled (`2 082`), one boundary away from FU314's dispatch-side `2 081`; neither is a contradiction of the other.
- The `+0x43` value is asserted `2` at kickoff from three install sites whose *execution order* in the port's staging is not re-derived (the port only reproduces the *value* through the install call it would add); FU319-L4 carries the ordering.
- No claim is made about `+0x45`/`+0x3F` (FU319-L5) or about the non-default focus pointers (FU319-L1).

## Review erratum (phase-33 wave, 2026-10-22)

Quote (§7 leg 3, and §4's row): the `move_attr` metric divergence is measured
"**2** record-frames in the window straddle the `>0x60` branch …; max |Δ| = 115
units over the window". **Corrected:** the phase-33 wave's fresh measurement
(FU324 §measure, its patched metric-only build and the 4 000 800-pair port
comparison) reproduces **no straddle and no moved frame** — the named rows give
`0x8DC68` = `0x8DCD4` = 96 on the named steps, the stride arm's clamp
(`attr = min(dist>>3, stride_len ≤ 3)`) absorbs the index shift, and the
`max+min/3` mutant also moves 0 frames. FU319-L3's *direction* (the native
metric is `0x8DC68`, the port stages `0x8DCD4`) stands; its measured magnitude
does not. The leg's owner text is now FU324-L1 (the erratum leg); the
nibble-conditional re-measure is FU324-L5. Also measured here: the low word of
`+0x63` has no writer (dead space, FU324-L2).
