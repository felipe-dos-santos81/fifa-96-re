# FU-185 — row-0x22 sequence/press body + the install-target (team-target) producer

Provenance: M2 phase-16 wave **R33** (planned FU-185), 2026-10-12, read-only.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; standard tools only). No repo file was
modified by this slice beyond this new doc; no git commands; no subagents.
Every xref/table/byte claim below is a fresh `get_xrefs_to` / `read_memory` /
`disassemble_bytes` / `get_function_by_address` / `search_instructions` of
**this session** unless marked *cross-cited* (then the frozen FU doc is named).
Engine/loader source is read first-hand this session. Addresses are LE-image
flat addresses; the alias traps are applied (`dword[x]>>16` = word at `x+2`,
`>>24` = byte at `x+3`).

Question (plan R33): derive the row-0x22 body (FU-136 `sequence_press_event`;
OL-9) and the team-target producer that makes its install target real; give a
T2 wiring rec and the honest census verdict under the strict "engine reaches
it" test — is `0x22` countable now, or leg-gated on OL-56?

---

## 1. Scope, method, headline

* The **action-table slot `0x22`** is `0x1106E0[0x22] = 0x8539C` (fresh
  `read_memory 0x1106E0` 180 B; the slot dword is the bytes `9c 53 08 00`).
  Fresh `get_xrefs_to 0x8539C` = **1**, from `0x110768` (the action-table slot
  itself, `DATA`) — the body has **no other static reference**, so `0x8539C` is
  the row-0x22 handler and nothing else calls it directly. Fresh
  `get_function_by_address 0x8539C` = *no function* (a bare action-table
  target, mid-body).
* The **installer** (`FUN_0007D9A4`) installs the row: fresh
  `read_memory 0x1106E0` and the installer body confirm
  `dword[rec+0x18] = 0x1106E0[code]` (`0x7DA74..0x7DA8F`).
* The **installer invocation** for code `0x22` is the row-07 body at
  `0x816D1..0x816DF` (fresh `disassemble_bytes 0x815D0`/`0x816A0`), reached
  through the *wired* engine row `fifa96_match_action_07` (`handlers.c:1160`).
* The **install target** is the opponent team's target record
  (`[[rec]+0x7A6]+0x7B2`, fresh) — and its producer is **ported and live**
  (§4). So the "OL-56 team-target producer" premise in FU-182 §4 is an
  **errata** (§5.1); OL-56 is a different row-28 surface and FU-164 is the pan
  arms.

**Headline:** the row-0x22 body is a self-contained 3-stage sequence/press
machine over already-ported helpers; the installer path is live and its target
producer is live. `0x22` is **not** leg-gated on OL-56 — it is an honest `+1`
mover for T2 once its own body is ported and its slot registered (OL-9 is the
only real blocker). It is **not** countable *today* only because the engine
slot `0x22` is still `NULL` (`handlers.c:4091`).

---

## 2. The row-0x22 body `0x8539C..0x85481` (fresh, byte-level)

Fresh `disassemble_bytes 0x8539C` (240 B; 70 instructions; the last few are the
next body, not this row). The whole body ends at `0x85481 RET` (the prologue
pops `ESI/EDX/ECX/EBX` and RETs). Fresh `disassemble_bytes` of the region:

| addr | bytes | instruction | meaning |
|---|---|---|---|
| `0x8539C` | `53 51 52 56` | `PUSH EBX; PUSH ECX; PUSH EDX; PUSH ESI` | prologue |
| `0x853A0` | `89 c6` | `MOV ESI,EAX` | `ESI = rec` |
| `0x853A2` | `8b 0d 4a7a1500` | `MOV ECX,[0x157A4A]` | phase dword |
| `0x853A8` | `c1 f9 18` | `SAR ECX,0x18` | `ECX = byte[0x157A4D]` (phase) |
| `0x853AB` | `83 f9 02` | `CMP ECX,2` | phase gate |
| `0x853AE` | `0f 85 c4 000000` | `JNZ 0x85478` | not phase 2 → reset |
| `0x853B4` | `31 c9` | `XOR ECX,ECX` | |
| `0x853B6` | `8b 90 89000000` | `MOV EDX,[EAX+0x89]` | `timer89` dword |
| `0x853BC` | `66 8b 0d 647a1500` | `MOV CX,word[0x157A64]` | frame delta word |
| `0x853C3` | `01 ca` | `ADD EDX,ECX` | `timer89 += delta` |
| `0x853C5` | `89 90 89000000` | `MOV [EAX+0x89],EDX` | store `timer89` |
| `0x853CB` | `e8 4c47ffff` | `CALL 0x79B1C` | snap tail (`target := pos`, clears words) |
| `0x853D0` | `8a 86 92000000` | `MOV AL,byte[ESI+0x92]` | stage latch |
| `0x853D6` | `3c 01` | `CMP AL,1` | |
| `0x853D8` | `72 0f` | `JC 0x853E9` | stage 0 |
| `0x853DA` | `76 4c` | `JBE 0x85428` | stage 1 |
| `0x853DC` | `3c 02` | `CMP AL,2` | |
| `0x853DE` | `0f 84 8c 000000` | `JZ 0x85470` | stage 2 |
| `0x853E4..0x853E8` | `5e 5a 59 5b c3` | `POP…RET` | stage > 2: return |

### Stage 0 `0x853E9..0x85427`
| addr | bytes | instruction | meaning |
|---|---|---|---|
| `0x853E9` | `84 c0` | `TEST AL,AL` | (redundant; AL==0) |
| `0x853EB` | `0f 85 8c 000000` | `JNZ 0x8547D` | |
| `0x853F1` | `80 be 8d000000 00` | `CMP byte[ESI+0x8D],0` | `active` |
| `0x853F8` | `0f 84 78 000000` | `JZ 0x85476` | inactive → reset |
| `0x853FE` | `89 f0` | `MOV EAX,ESI` | |
| `0x85400` | `c6 86 9e000000 01` | `MOV byte[ESI+0x9E],1` | `ran = 1` |
| `0x85407` | `e8 1047ffff` | `CALL 0x79B1C` | snap |
| `0x8540C` | `c6 46 43 00` | `MOV byte[ESI+0x43],0` | `+0x43 = 0` |
| `0x85410` | `8a 96 92000000` | `MOV DL,byte[ESI+0x92]` | |
| `0x85416` | `c7 86 89000000 00000000` | `MOV dword[ESI+0x89],0` | `timer89 = 0` |
| `0x85420/22` | `fe c2` / `88 96 92000000` | `INC DL; MOV [ESI+0x92],DL` | advance |

### Stage 1 `0x85428..0x8546F`
| addr | bytes | instruction | meaning |
|---|---|---|---|
| `0x85428` | `8b 4e 69` | `MOV ECX,[ESI+0x69]` | lane dword |
| `0x8542B` | `c1 f9 10` | `SAR ECX,0x10` | `ECX = word[ESI+0x6B]` (lane band) |
| `0x8542E` | `83 f9 30` | `CMP ECX,0x30` | |
| `0x85431` | `7e 0e` | `JLE 0x85441` | band ≤ 0x30 → fire |
| `0x85433` | `83 be 89000000 1e` | `CMP dword[ESI+0x89],0x1E` | else timer gate |
| `0x8543A` | `7f 3a` | `JG 0x85476` | timer > 0x1E → reset |
| `0x8543C..0x85440` | `5e 5a 59 5b c3` | `POP…RET` | else wait |
| `0x85441` | `ba 4f000000` | `MOV EDX,0x4F` | animation id 0x4F |
| `0x85446` | `89 f0` | `MOV EAX,ESI` | |
| `0x85448` | `8b 8e 8b000000` | `MOV ECX,[ESI+0x8B]` | |
| `0x8544E` | `31 db` | `XOR EBX,EBX` | |
| `0x85450` | `c1 f9 18` | `SAR ECX,0x18` | `ECX = byte[ESI+0x8E]` (type) |
| `0x85453` | `e8 4091feff` | `CALL 0x6E598` | animation `FUN_0006E598(rec,0x4F,type,0)` |
| `0x85458` | `8a b6 92000000` | `MOV DH,byte[ESI+0x92]` | |
| `0x8545E` | `c7 86 89000000 00000000` | `MOV dword[ESI+0x89],0` | `timer89 = 0` |
| `0x85468/6A` | `fe c6` / `88 b6 92000000` | `INC DH; MOV [ESI+0x92],DH` | advance |

### Stage 2 `0x85470..0x85481`
| addr | bytes | instruction | meaning |
|---|---|---|---|
| `0x85470` | `80 7e 44 00` | `CMP byte[ESI+0x44],0` | anim ack byte |
| `0x85474` | `74 07` | `JZ 0x8547D` | no ack → pop/ret |
| `0x85476` | `89 f0` | `MOV EAX,ESI` | reset arm entry |
| `0x85478` | `e8 3786ffff` | `CALL 0x7DAB4` | `FUN_0007DAB4(rec)` reset |
| `0x8547D..0x85481` | `5e 5a 59 5b c3` | `POP…RET` | |

**Body summary.** A phase-2-only, 3-stage sequence body: prologue → phase gate
(`dword[0x157A4A]>>24 == 2`, else reset) → `timer89 += word[0x157A64]` →
`FUN_00079B1C` snap → stage latch `byte[+0x92]` switch:
`0`: `active(+0x8D) == 0 ? reset : { +0x9E = 1; snap; +0x43 = 0; timer89 = 0;
     stage++ }`;
`1`: `word[+0x6B] > 0x30 ? (timer89 > 0x1E ? reset : wait) : { anim(rec, 0x4F,
     byte[+0x8E]); timer89 = 0; stage++ }`;
`2`: `byte[+0x44] != 0 ? reset : ret`;
`>2`: ret. Every terminating arm that ends the row's participation runs
`FUN_0007DAB4(rec)` when `+0x44` is set (the shared stage-family convention,
FU-82 §4). This matches the FU-82 §3.8 derivation; the byte-level is
re-confirmed here and the specific globals are `[0x157A4A]` (phase dword) and
`[0x157A64]` (frame delta word). The stage-1 "height" name in FU-82/FU-136 is
the lane band `word[+0x6B]` (= `dword[+0x69]>>16`), not a Y height.

**Process-global / helper surface (all already modeled or a leg):**
`[0x157A4A]>>24` = `mr->state.phase`; `[0x157A64]` = `r->delta`;
`FUN_00079B1C` = the snap (`target := pos`, lane/velocity word clears; the
engine carries it as the loader `snap` request, e.g. `fifa96_action_handlers.h`
`:321`/`:260` and `fifa96_outfield.h:392`; the direct engine mapping is
`handlers.c:1016/1500/3254` — `FUN_00079B1C` is *not* itself a C symbol yet, see
leg FU185-L2); `FUN_0006E598` = the animation selector (OL-52 unmodeled sink);
`FUN_0007DAB4` = `match_row_reset` (engine, `handlers.c`); the
`fifa96_action_stage_*` helpers (FU-81) cover the timer/advance/reset
conventions; `fifa96_action_sequence_press_event` (FU-136,
`fifa96_action_handlers.c:1360`) already binds the stage-1 `height`/`timer`
decision. `+0x43` (the anim row's flags byte) has no engine cell (leg
FU185-L3).

---

## 3. The installer and the row-07 install site (fresh)

### 3.1 `FUN_0007D9A4` installer contract (fresh `disassemble_bytes 0x7D9A4`)

`EAX = rec`, `DX = code`, `BX = staged`, `ECX = invoke-now` (moved to `EDI`).
Fresh body confirms the FU-182/FU-137 port contract:
`0x7D9B6 TEST EAX,EAX; JZ ret`; `0x7D9BE CMP byte[rec+0x9A],0; JNZ ret`
(inactive skip); `0x7D9CB MOVSX AX,byte[rec+0x91]; CMP AX,DX; JZ ret`
(same-code skip); the `+0x98` anim gate; the `0x7DA26` code-3→`0x19` inactive
remap (`0x7DA2F..0x7DA3B`); the `0x7DA42` code-5/`0x21` `+0x9F` carrier-bit
set/clear; `0x7DA63` stage the (possibly remapped) code;
`0x7DA67 MOV [rec+0x91],AL`; `0x7DA6D..0x7DA7C MOV EAX,0x1106E0[code]` (the
handler); `0x7DA7E MOV dword[rec+0x89],0`; `0x7DA88 MOV byte[rec+0x9E],0`;
`0x7DA8F MOV [rec+0x18],EAX`; `0x7DA92 MOV AL,[ESP]; MOV [rec+0x92],AL`;
`0x7DA9B MOV AX,[rec+0x79]; MOV [rec+0x7B],AX`; and the invoke-now tail
`0x7DAA3 TEST DI,DI; JZ 0x7DAAD; 0x7DAA8 MOV EAX,ESI; 0x7DAAA CALL [ESI+0x18]`.

### 3.2 The row-07 install site `0x816D1..0x816DF` (fresh)

Fresh `disassemble_bytes 0x815D0` (208 B) — the code-7 stage-1 tail:

| addr | bytes | instruction |
|---|---|---|
| `0x81665` | `8b 45 00` | `MOV EAX,[EBP]` (team base, `[rec+0]`) |
| `0x81668` | `8b b0 a6070000` | `MOV ESI,[EAX+0x7A6]` (opponent team base) |
| `0x8166E` | `8b 95 89000000` | `MOV EDX,[EBP+0x89]` (`timer89`) |
| `0x81674` | `8b b6 b2070000` | `MOV ESI,[ESI+0x7B2]` (`opponent team target`) |
| `0x8167A` | `83 fa 05` | `CMP EDX,5` |
| `0x8167D` | `7d 65` | `JGE 0x816E4` (timer89 ≥ 5 skip) |
| `0x8167F` | `85 f6` | `TEST ESI,ESI` |
| `0x81681` | `74 61` | `JZ 0x816E4` (no opp target skip) |
| `0x81683` | `8b 86 8e000000` | `MOV EAX,[ESI+0x8E]` |
| `0x81689` | `c1 f8 18` | `SAR EAX,0x18` (`EAX = byte[opp+0x91]` = opp action code) |
| `0x8168C` | `83 f8 06` | `CMP EAX,6` |
| `0x8168F` | `75 53` | `JNZ 0x816E4` (opp code ≠ 6 skip) |
| `0x81691` | `83 7e 20 00` | `CMP dword[ESI+0x20],0` (`has_slot`) |
| `0x81695` | `75 4d` | `JNZ 0x816E4` |
| `0x81697` | `66 81 7e 6b d000` | `CMP word[ESI+0x6B],0xD0` (lane band) |
| `0x8169D` | `7d 45` | `JGE 0x816E4` |
| `0x8169F..0x816AB` | … | `MOV EDX,[ESI+0x6D]; …; CALL 0x8DD70` (angle metric) |
| `0x816B0` | `66 2b 46 7d` | `SUB AX,word[ESI+0x7D]` (facing) |
| `0x816B4..0x816C9` | … | `AND AH,0x3`; `MOVSX EDX,AX`; fold `EDX = 0x400 − EAX` when `> 0x200` |
| `0x816CB` | `98` | `CWDE` |
| `0x816CC` | `83 f8 55` | `CMP EAX,0x55` |
| `0x816CF` | `7d 13` | `JGE 0x816E4` (diff ≥ 0x55 skip) |
| `0x816D1` | `b9 01000000` | `MOV ECX,1` (**invoke-now**) |
| `0x816D6` | `ba 22000000` | `MOV EDX,0x22` (**code 0x22**) |
| `0x816DB` | `89 f0` | `MOV EAX,ESI` (**the opponent target record**) |
| `0x816DD` | `31 db` | `XOR EBX,EBX` (**staged = 0**) |
| `0x816DF` | `e8 c0 c2 ffff` | `CALL 0x7D9A4` (`0x816E4 − 0x3D40 = 0x7D9A4`, verified) |
| `0x816E4..` | … | advance: `timer89 = 0`, `stage++` |

So the row-07 stage-1 arm installs code `0x22` **invoke-now** on the **opponent
team's target record** (native `[[rec]+0x7A6]+0x7B2`), gated on the opponent
target's `byte[+0x91] == 6`, no slot, lane `< 0xD0`, and the angle diff
`< 0x55`. This is exactly the engine gate in `fifa96_action_handlers.c:1697`
(`s->opp_present && s->opp_type == 6u && s->opp_has_slot == 0u &&
s->opp_lane_word < 0xD0`, `diff < 0x55`).

---

## 4. The install target's producer (the real one)

The install target is the **opponent team's `target` field** (`[team+0x7B2]`),
represented in the engine as `mr->entities.team[1-team].target`
(`fifa96_match_handlers.c:1119`, `match_kick_team` = the acting record's team).
Fresh `search_instructions operand "0x7b2]"` (150720 instructions, truncated at
80 matches) shows the native `+0x7B2` writers/readers are manifold
(`0x7A384`, `0x7DF5B`, `0x7E027`, `0x7E82D`, `0x7E8EF`, `0x7EED8`, `0x7F20E`,
`0x80867`/`0x80876`, `0x8102A`, `0x81F29`, `0x844DC`, `0x84D93`, `0x85C96`,
`0x8642B`, `0x8999C`/`0x899BE`, `0x8C37B`, `0x8CB9E`, `0x8D1DC`, `0x8D265`,
`0x8D31A`, `0x8D3E9`, `0x8D46F`, …). The governing producer for the receiver
scan is `FUN_0007A084`:

* fresh `0x7A384 MOV dword[EDX+0x7B2],EAX` (= `team->target = receiver`) and
  `0x7A398 MOV dword[EAX+0x7B2],0` (= the opponent's target cleared);
* fresh `get_xrefs_to 0x7A084` = **3**: `FUN_0006E8E8 0x6F56F`,
  `FUN_0007A490 0x7A8C7`/`0x7A8EF` (the ball-staging tail).

**Engine status (first-hand source this session):** this producer is **ported
and wired**. `fifa96_match_run_ball_receive` is the port of `FUN_0007A084` and
writes the same cells: `fifa96_match_run.c:3396 team->target = pair->receiver;`
`…:3397 team->second = NONE;` `…:3398 mr->entities.team[1u-t].target = NONE;`.
It is reached on the live kick path via `match_kick_run` (`handlers.c:1106`,
the `out->receive` gate). Independent live writers of `team[].target`:
`fifa96_match_run.c:2499/2532/2545/2548/2557/2571/2583` (the phase arms),
`fifa96_match_handlers.c:269` (row 01), `:346` (row 02), `:1584` (set-piece
pick), `:2652` (row 04), `:2813` (row 05), `:3060` (row 06),
`fifa96_match_entities.c:134` (`team_select`) and `:209` (`reset_lane`).

**Conclusion:** the opponent team target is produced by ported, live code, so
`mr->entities.team[1-team].target` (→ `s->opp_present`) is satisfiable on the
live kick path. The `opponent_invoke` gate can fire, so the installer's target
is **real**, not dormant.

---

## 5. Errata (quoted, first-hand)

### 5.1 FU-182 §4 / the plan's `0x22` "producer-dormant" nuance (the OL-56 citation)

FU-182 §4 quote: *"the installer fires but its target is the other team's
`target` record, whose producer is unported (FU-164/OL-56: the team-target
producer); the arm is therefore code-live but producer-dormant until OL-56
lands."*

**Corrected.** The install target is the opponent team target
`[[rec]+0x7A6]+0x7B2` (§3.2), and its producer **is ported and live** (§4:
`fifa96_match_run_ball_receive` `match_run.c:3396`, plus the phase arms and
rows 01/02/04/05/06 and `team_select`/`reset_lane`). The two documents FU-182
cites do **not** describe this producer:

* **OL-56** (FU-142 §6) is *"row-28 process-global inputs and the match RNG
  seed"* — `[0x10F358]`, `[0x10F35C]`, `[0x10F364]`, `[0x10F368]`, `[0x157AC2]`
  and `FUN_0001D940(0x18)`. It is unrelated to the team target.
* **FU-164** is the `FUN_00071C94` pan-arm installer census; FU164-L2 is the
  **aux-`0x22` animation-frame** producer (the `FUN_0006E8E8` arm 33) — a
  *frame aux byte* `0x22`, not the *action code* `0x22` and not a team target.

So "OL-56 team-target producer" conflates two unrelated `0x22`s (action code
vs. frame aux byte) and a row-28 globals leg. There is **no OL-56 gate** on
row `0x22`.

### 5.2 The engine's `opp_type` source byte (row-07 gate fidelity)

`match_kick_from_record` sets `s->opp_type = opp->type` (`handlers.c:975`)
where the pool `type` field is *"native byte +0x8E (the face octant)"*
(`include/fifa96_engine/fifa96_match_entities.h:76`; it is written by
`fifa96_arm_face` only, `fifa96_match_entities.c:512`, values 0..7). The native
gate reads `byte[opp+0x91]` (the action **code**): `0x81683 MOV EAX,[ESI+0x8E];
SAR EAX,0x18` = byte `+0x91` (§3.2). The engine's own `s->type = r->code`
(`handlers.c:907`) is the correct byte for the *acting* record, but the
*opponent* uses the octant. This does not block the install (octant 6 is
reachable), but the `opp_type == 6` compare is not the native test; T2 should
read the pool `code` for `opp_type` (see leg FU185-L1).

---

## 6. T2 wiring rec

Implement the row-0x22 body and register slot `0x22`, reusing the existing
helpers; no new RE is required.

1. **Port the body** as `fifa96_match_action_22(struct fifa96_match_run *mr)` in
   `src/fifa96_engine/fifa96_match_handlers.c`, binding the native
   `0x8539C..0x85481` to `mr->record`:
   * phase gate `mr->state.phase == 2` else `match_row_reset`;
   * `r->timer89 += r->delta`;
   * snap: `r->target_x/y/z = r->pos_x/y/z` plus the `FUN_00079B1C` word clears
     (the loader already carries this as the `snap` request; mirror the existing
     engine snap sites, `handlers.c:1016/1500/3254`);
   * stage 0 (`timer89 = 0`, `ran = 1`, `+0x43 = 0`, snap, advance) gated on
     `r->active`;
   * stage 1 via the ported `fifa96_action_sequence_press_event(height, timer89,
     &out)` (`fifa96_action_handlers.c:1360`) where `height` is the native lane
     band `word[+0x6B]` (= `r->lane >> 16`); on `fire` emit the animation
     request `anim_id = 0x4F`, `type = r->actor_type` (OL-52), `timer89 = 0`,
     advance; on `reset` → `match_row_reset`;
   * stage 2: `r->row44 != 0` → `match_row_reset`, else return;
   * every reset arm = `match_row_reset`.
2. **Register the slot**: replace `{0x22, NULL, …}` (`handlers.c:4091`) with
   `{0x22, fifa96_match_action_22, …}` and add the row-0x22 row to ENGINE.md's
   counted set; recompute the README title in the same commit.
3. **Fix the installer-gate byte** (§5.2, leg FU185-L1): source
   `s->opp_type` from the opponent record's *code* byte, not the face octant
   (one-line: `s->opp_type = opp->code;` at `handlers.c:975`, subject to the
   existing `+0x91` mapping rationale at line 907).
4. **Tests**: a RED/GREEN unit fixture for the three stages (phase gate,
   `active` reset, band>0x30 wait/reset, band≤0x30 fire, stage-2 `+0x44`
   reset) + a mutation-RED discriminator on each gate; a live-chain test only
   if the row-07 `opponent_invoke` arm is fixture-reachable. T2 is expected
   tape-dormant (install-live, not tape-dispatched).

**Census effect:** `0x22` NULL → non-NULL = `+1` action row. On the plan's
baseline (plan §Global Constraints: `29/80 = 36%`) this is `30/80 = 37%`; on
FU-182's `24/80` snapshot it is `25/80`. No phase row moves.

---

## 7. Honest census verdict ("engine reaches it" test)

* **Not countable now.** The engine action-table slot `0x22` is `NULL`
  (`handlers.c:4091`); the body is unported (OL-9). No wiring exists yet.
* **Not leg-gated on OL-56.** The strict test's two halves both pass today:
  (a) the native installer invocation exists (`0x816DF`, fresh) and (b) the
  engine already reaches it — row 07 is wired (`handlers.c:1161`,
  `out.opponent_invoke` → `match_kick_invoke_opponent`, `handlers.c:1117`), and
  the install target's producer (`team[1-team].target`) is ported and live
  (§4). There is no unported producer standing between the engine and the
  install.
* **Verdict:** `0x22` is an **honest T2 `+1` mover** gated only on its own body
  port (OL-9) plus the row-07 gate-fidelity fix (§5.2). Under the strict test
  it **qualifies**; an `0x22`-stays-uncounted outcome would be a *false
  negative* caused by the phantom OL-56 premise. (Fidelity caveat, not a
  reachability block: `out.opponent_invoke` also needs the opponent target to
  be running code 6 / no slot / lane `< 0xD0` / angle diff `< 0x55` — a
  conditional but statically satisfiable gate.)

---

## 8. Numbered legs (new)

1. **FU185-L1 — the row-07 `opp_type` source byte.** `s->opp_type = opp->type`
   (`handlers.c:975`) reads the pool face octant (+0x8E) whereas the native gate
   reads `byte[opp+0x91]` (`0x81683`); fix to the pool `code` for an exact
   `opponent_invoke` gate. Does not block reachability (octant 6 fires).
2. **FU185-L2 — `FUN_00079B1C` as a direct C symbol.** The snap is carried as a
   request flag and applied inline at several engine sites (`handlers.c`
   `1016/1500/3254`, `match_sp_snap` `1500`) and the `fifa96_action_handlers.h`
   `snap` fields; there is no single `fifa96_*_snap` function matching
   `0x79B1C`'s exact word-clear set for the row-22 body. Bind the exact clears
   (`+0x69/+0x71/+0x9C/+0x65/+0x67/+0x73/+0x75`, FU-82 §2) or reuse the closest
   site and cite the delta.
3. **FU185-L3 — the `+0x43` anim-flags cell.** Row-22 stage 0 writes
   `byte[rec+0x43] = 0`; the pool models `row44` (+0x44) but no `+0x43` cell
   (the `FUN_0006E598` row-flags side effect). Track as a staged byte or carry
   as an unmodeled write.
4. **FU185-L4 — the `0x6E598` animation request.** Stage 1's anim id `0x4F`
   (+type) is the FU-136/FU-82 event mechanism; the engine has no `0x6E598`
   consumer (OL-52). Row-22 counting does not require it; fidelity does.
5. **FU185-L5 — the second `0x22` installer family.** Fresh
   `search_instructions "MOV EDX,0x22"` = 5 hits, of which only `0x816D6` is
   the installer (`0x7D9A4`); the other four (`0x26F9A`, `0x2B9B3`, `0x498D9`,
   `0x6531F`) are `0x22x` immediates, not installs. Register-derived installers
   were not exhaustively mapped (FU-164-L3 treatment).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `read_memory 0x1106E0` (180 B);
`disassemble_bytes 0x8539C` (240 B), `0x816A0` (112 B), `0x815D0` (208 B),
`0x814B0` (200 B), `0x7D9A4` (240 B), `0x7DA92` (34 B);
`get_function_by_address 0x816D6` (none), `0x8539C` (none), `0x7A084`
(`FUN_0007A084`, `0x7A084..0x7A456`), `0x79B1C` (`FUN_00079B1C`),
`0x6E598` (`FUN_0006E598`); `get_xrefs_to 0x8539C` (1: `0x110768`), `0x7A084`
(3: `0x6F56F`, `0x7A8C7`, `0x7A8EF`); `search_instructions "MOV EDX,0x22"`
(5), `operand "0x7b2]"` (80, truncated).
Engine/loader source read: `src/fifa96_engine/fifa96_match_handlers.c`
(240-360, 860-1010, 1080-1170, 1550-1600, 2620-2720, 2790-2830, 3020-3075,
4035-4095), `src/fifa96_engine/fifa96_match_run.c` (2470-2600, 3340-3410),
`src/fifa96_engine/fifa96_match_entities.c` (1-60, 100-220),
`include/fifa96_engine/fifa96_match_entities.h` (1-265),
`src/fifa96_loader/fifa96_action_handlers.c` (1350-1400, 1590-1730),
`include/fifa96_loader/fifa96_action_handlers.h` (255-330, 680-730),
`include/fifa96_loader/fifa96_outfield.h` (380-405). Docs cross-cited:
FU-82 §3.8, FU-136, FU-137, FU-142 §6/K.6.5, FU-164, FU-182; AGENTS.md census
rule; the phase-16 plan §R33/§T2. No writes beyond this slice and the workspace
report; no rename/label/comment/function/script/project save; no git.
