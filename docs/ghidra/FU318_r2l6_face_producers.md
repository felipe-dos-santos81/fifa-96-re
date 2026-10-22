# FU318 — the `+0x7D` face producer census (phase-32 R2-L6, FU314-L3)

**Phase/Provenance:** phase-32 recon-ahead wave, Stage-1 **Task 1 of 4** (`docs/superpowers/plans/2026-10-22-fifa96-m2-phase32-recon-ahead.md`), scoped by `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md` §2/§3.1/§8–§9. Read-only derivation: Ghidra MCP against **`/FIFA96.EXE`** (fresh this session) plus the repo's loader/engine sources and the phase-31 out-of-tree natural dump (`/tmp/opencode/fu314/recs3.txt`). No production code, no repo change beyond this draft, no git. All addresses are **flat image** addresses; LE globals are the `+0x100000` file-offset form.

## 1. The question

Natively `FUN_00079C50` writes **both** `word[+0x7D]` (the face) and `byte[+0x8E]` (the facing octant) at every non-zero-direction call site. The port splits that contract: `fifa96_arm_face` writes only the octant (`r->type`), and `fifa96_entity_face` writes the face at only a handful of sites. Which producer actually runs for a walking **code-4** record in natural play, what is the exact faithful port shape (FU314 R1 vs R2), and what is the M2 risk?

## 2. The native helper and the fresh caller census

### 2.1 The helper contract (fresh `disassemble_bytes 0x79C50/0x60`)

| Claim | Fresh evidence |
|---|---|
| Entry `0x79C50(EAX=rec, DX=dx, BX=dz)`; `DX==0 && BX==0` returns `MOVSX AX, byte[rec+0x8E]` and writes **neither** cell | `0x79C54 TEST DX,DX` / `0x79C57 JNZ` / `0x79C59 TEST BX,BX` / `0x79C5C JNZ` / `0x79C5E MOVSX AX,byte[EAX+0x8E]` / `0x79C68 RET` |
| Non-zero: `word[+0x7D] := 0xCD474(dx,dz)` | `0x79C69..0x79C71 PUSH BX; PUSH DX; CALL 0xCD474`; `0x79C76 MOV word[ESI+0x7D],AX` |
| Then `byte[+0x8E] := ((word[+0x7D] + 0x40) & 0x3FF) >> 7` (the dword re-read is the alias: `dword[+0x7B]>>16` **is** word `+0x7D`) | `0x79C7A MOV EAX,dword[ESI+0x7B]`; `0x79C7D SAR 0x10`; `0x79C80 ADD 0x40`; `0x79C83 AND 0x3FF`; `0x79C88 SAR 0x7`; `0x79C8E MOV byte[ESI+0x8E],AL` |
| The helper's return is the **sector** in both arms (`CBW` of `AL` after the store; the zero arm returns the stored sector) | `0x79C94 CBW` / `0x79C98 RET`; zero arm `0x79C5E` |
| The `(0,0)` arm is a sector **getter**, not a face write — this confirms the FU314 review erratum ("every native site writes both" is too strong) | same bytes; only one of the 40 sites passes a constant `(0,0)` (§2.2, site 24) |
| A second, dead writer exists at `0x79C99`: `FUN_00079C9C(rec, sector)` stores `byte[+0x8E]=DL` then `word[+0x7D] = (sector<5 ? sector<<7 : -(8-sector)<<7)` and returns the sector | `0x79C99..0x79CCB` fresh; **`get_xrefs_to 0x79C99` = 0** (dead in this build) |

### 2.2 The 40 `FUN_00079C50` call sites (fresh `get_xrefs_to 0x79C50` = **40**)

Argument classes: **A** = the call always writes both cells (constant or table vector, never `(0,0)`); **B** = constant `(0,0)` (writes neither; sector getter); **C** = runtime vector, writes both unless the vector happens to be exactly `(0,0)`. "Port" is the fresh engine/loader site; `—` = no port site found by the fresh grep + native address range (row entry table `0x1106E0`, fresh read; row 0x04 = `0x7E7C8..0x7F141`).

| # | Site | Native owner (row / function) | Vector source | Class | Port |
|---|---|---|---|---|---|
| 1 | `0x88A81` | `FUN_00088940` (period/place scanner; also `0x6E598` at `0x88A35/A9D`) | stack point pair | C | — |
| 2 | `0x79BCF` | `FUN_00079B6C` (position + face helper) | `p3 − p2` | C | `match_entities.c:563` (sector) / `handlers.c:3317` (both) |
| 3 | `0x6EEF1` | `FUN_0006E8E8` (anim event switch) | `−0x10F334/0x10F33C[byte+0x8E]` (8 nonzero dirs) | A | — |
| 4 | `0x6FDE4` | `FUN_0006E8E8` | same table | A | — |
| 5 | `0x6F2DC` | `FUN_0006E8E8` | `+0x73/+0x75` words, guarded by their own nonzero distance | A | — |
| 6 | `0x6F4A7` | `FUN_0006E8E8` | `+0x73/+0x75` (same guard) | A | — |
| 7 | `0x8E1CB` | `FUN_0008E008` (frame-wrap driver) | `−dir[byte+0x8E]` (8 nonzero dirs) | A | `match_run.c:1055` sector-only (face dropped) |
| 8 | `0x7AA09` | `FUN_0007A490` (ball pairing) | `0x158738` point (`+0x73A/+0x73C`) | C | `ball_pairing.c:196` sector-only |
| 9 | `0x756D0` | row `0x1E` keeper | `−pos` | C | `keeper.c:527` face local dropped |
| 10 | `0x7573C` | row `0x1E` keeper | `−pos` | C | `keeper.c:545` face local dropped |
| 11 | `0x75CDC` | row `0x1E` keeper | staged `vec_dx/vec_dz` | C | `keeper.c:708` face local dropped |
| 12 | `0x76012` | row `0x1E` keeper | staged `vec_dx/vec_dz` | C | `keeper.c:782` face local dropped |
| 13 | `0x7682E` | row `0x1A` | `−word[0x1577C0]/−word[0x1577C2]` | C | `handlers.c:4601` **both** |
| 14 | `0x76E61` | row `0x1B` | `−word[0x1577C0]/−word[0x1577C2]` | C | — |
| 15 | `0x7766B` | row `0x1B` | `−tracked pos` | C | — |
| 16 | `0x7F607` | row `0x05` (carrier stage 2) | slot bytes `[[rec+0x20]+0x20/+0x21]` | C | `handlers.c:2951` sector-only (`action_handlers.c:544` inline) |
| 17 | `0x80AAB` | row `0x09` | `0xA*focus + +0x6D/+0x6F` | C | `handlers.c:5021` **both** |
| 18 | `0x826E3` | row `0x0D` | `+0x71/+0x73` words | C | `action_handlers.c:1483` both, **row unwired** (FU178-L1) |
| 19 | `0x82794` | row `0x0E` | `+0x65/+0x67` words (from `0x71B9C`/`0x8DCD4`) | C | `handlers.c:1495` sector-only |
| 20 | `0x8552F` | `FUN_00085498` (set-piece delivery probe) | camera delta `+0x6D/+0x6F` | C | `handlers.c:1611` derived probe only (no face) |
| 21 | `0x85729` | row `0x10` | after `0x79B6C`: camera delta | C | — (row-0x10 body; no face site found) |
| 22 | `0x85C45` | row `0x10` | computed/`0x795A4` | C | — |
| 23 | `0x85F17` | row `0x11` | camera delta `+0x6D/+0x6F` | C | — |
| 24 | `0x86AC1` | row `0x2A` arm 0 | constant `(0,0)` (`SUB EAX,EAX; MOVSX EBX,AX; MOVSX EDX,AX`) | **B** | `arm_bodies.c:611` no-op call (faithful) |
| 25 | `0x86FA3` | row `0x2A` arm 10 | constant `(0,−100)` | A | `arm_bodies.c:751` sector-only |
| 26 | `0x86FBD` | row `0x2A` arm 10 | constant `(100,0)` | A | `arm_bodies.c:753` sector-only |
| 27 | `0x87F3B` | row `0x15` | toward the selected record | C | `handlers.c:6236` sector-only |
| 28 | `0x87F7C` | row `0x15` | toward the selected record | C | `handlers.c:6236` sector-only |
| 29 | `0x8842B` | row `0x25` | `0x1586D8` table point | C | `handlers.c:5091` sector-only (celebration arm) |
| 30 | `0x884E0` | row `0x25` | same | C | `handlers.c:5091` sector-only |
| 31 | `0x777B4` | row `0x1C` | `−word[0x1577C0]/−word[0x1577C2]` | C | — (FU186-L2) |
| 32 | `0x81193` | row `0x08` | camera delta `+0x6D/+0x6F` | C | — |
| 33 | `0x82D7C` | row `0x0F` | `0x158738` point (`RNG`-gated) | C | — |
| 34 | `0x846E0` | row `0x16` | `0x158738` point | C | `handlers.c:5317` **both** |
| 35 | `0x865C6` | row `0x24` | constant `(1,0)` (`MOV EDX,1`/`XOR EBX,EBX`) | A | `handlers.c:5394` **both** |
| 36 | `0x86913` | row `0x27` | `+0x65/+0x67` words | C | `arm_bodies.c:171` sector-only |
| 37 | `0x87127` | row `0x28` | `+0x65/+0x67` words | C | `arm_bodies.c:271` sector-only |
| 38 | `0x87ACC` | row `0x14` | `0x1586D8` table point | C | `handlers.c:5091` sector-only |
| 39 | `0x87B8D` | row `0x14` | `0x1586D8` table point | C | `handlers.c:5091` sector-only |
| 40 | `0x881CD` | row `0x25` | rotated arm point | C | — (the port's row-0x25 head marks `0x88106..0x881D2` as FU243-L4) |

Counts: **A = 8** (sites 3,4,5,6,7,25,26,35); **B = 1** (site 24); **C = 31**. The `(0,0)` early return makes a site a no-op only for class B (and for a class-C site in the exact frame its vector is zero — no class-C site is *statically* zero; sites 5/6 are guarded by a non-zero distance over the same word pair, so they always write when reached).

### 2.3 The census is not just `0x79C50` (fresh instruction search, destination filter)

`MOV` destinations found by `search_instructions` (operand `+ 0x7d]`, program-wide) — the **only** `word[+0x7D]` writers:

| Writer | Owner | Cells |
|---|---|---|
| `0x79C76` | `FUN_00079C50` | `+0x7D` + `+0x8E` |
| `0x79CBE` | `FUN_00079C9C` (dead: 0 xrefs) | `+0x7D` + `+0x8E` |
| `0x7C03F` | `FUN_0007BF20` mover turn | `+0x7D` + `+0x8E` |
| `0x8E350` | `FUN_0008E244` mover turn (twin) | `+0x7D` + `+0x8E` |
| `0x8E69A` | `FUN_0008E5BC` | `+0x7D` + constant sector `2` (`0x8E691`) |
| `0x8E9F7` | `FUN_0008E910` | `+0x7D` + constant sector `2` (`0x8E9EE`) |

`MOV` destinations for `byte[+0x8E]` (same search) — sector-only writers that never touch the face: `0x6F90A`/`0x6F945` (`FUN_0006E8E8`), `0x81BE3` (row `0x0B`, copies the sector from another record), `0x8E691`/`0x8E9EE` (constant `2`, above). So the native `+0x7D` writers are **three live families** — the `0x79C50` helper (40 sites), the mover turn (two identical copies), and the two arm/teleport bodies at `0x8E5BC`/`0x8E910` — plus the dead `0x79C9C` sibling. The 40-site census alone cannot answer the walker question.

## 3. The frame-wrap arm (`FUN_0008E008`, `0x8E1CB`)

| Claim | Fresh evidence |
|---|---|
| The `0x79C50` call is in the `descriptor[3] >= 0x70` branch of the animation advance | `decompile_function 0x8E008`: `bVar2 = *(byte*)(*piVar1+3)`; `if (bVar2 == 0x6F) … else if (bVar2 < 0x70) FUN_0008df84(rec,bVar2,…) else { iVar5 = *(int*)(p+0x8b)>>0x18; FUN_00079c50(p, -tab34[iVar5], -tab3c[iVar5]); FUN_0008df84(rec,bVar2-0x70,…); }` |
| Disassembly agrees and shows the table index is **byte `+0x8E`** (the record's own sector), and the vector is negated | `0x8E1A6 MOV EAX,dword[EDI+0x8B]`; `0x8E1AC SAR 0x18`; `0x8E1AF MOVSX DX,byte[EAX+0x10F33C]`; `0x8E1B7 MOVSX AX,byte[EAX+0x10F334]`; `0x8E1BF/1C1 NEG`; `0x8E1CB CALL 0x79C50` |
| `0x10F334`/`0x10F33C` are **8-entry** direction tables: `{0,1,1,1,0,-1,-1,-1}` / `{1,1,0,-1,-1,-1,0,1}`; the second begins exactly 8 bytes after the first, so bytes past entry 7 are other data and an out-of-range sector would read junk | `read_memory 0x10F334/0x40` and `0x10F33C/0x40`; the first 8 entries of each match; byte 8 of the second is `0x15` |
| Every entry of the 8-entry pair is non-zero → the arm always writes both cells when reached | same reads; `fifa96_match_dir_x/z[8]` (`fifa96_match_locomotion_tables.c:31-32`) |
| The arm fires only when the **animation's next row id** (`descriptor[3]`) is `>= 0x70`, i.e. at a row wrap into the mirror bank, not per frame | `0x8E1B1..0x8E1FC` (the `0x8DF84(rec, next-0x70)` follows the face call) |
| Fresh table `0x10EF00` (9-byte rows `{id,last_frame,flags,next_id,frame_ptr,sprite_bank}`): only **two** of the 111 rows have `next_id >= 0x70` — row `0x14 -> 0x7F` and row `0x52 -> 0x7E` | `read_memory 0x10EF00/0x3E7`, computed in this session |
| The port models this arm as a **sector flip only**: `e->type = (e->type + 4) & 7` and no `face7d` write | `match_run.c:1055-1059` (`fifa96_match_run_record_anim_successor`) |

So the frame-wrap arm is a **two-animation special case** (0x14/0x52) and cannot be the walker's per-frame producer. The port's derived `(type+4)&7` is algebraically right for the octant (the 8-entry table is consistent with `+0x200`), but the native `+0x7D` write (the face flipped by `0x200`) is dropped.

## 4. The locomotion turn — the natural per-frame producer

`disassemble_bytes 0x8E240/0x90` + `0x8E2B7..0x8E386` (and the identical twin at `0x7BF89..0x7C051`) give the full block:

| Claim | Fresh evidence |
|---|---|
| The turn is gated by `CMP byte[rec+0x43],0` (the native `direct_face` byte) | `0x8E29C CMP byte[EBX+0x43],0x0`; `0x8E2A0 JZ 0x8E368`; twin `0x7BF89/0x7BF8D` |
| The `direct_face` byte is written by **every animation row install**, not by gameplay: `+0x43 = descriptor[2] & 2`, `+0x44 = descriptor[2] & 1` | `decompile_function 0x6E598`: `bVar3 = *(byte*)(iVar6+2); … *(char*)(rec+0x45) = bVar3 & 4; *(char*)(rec+0x43) = bVar3 & 2; *(byte*)(rec+0x44) = bVar3 & 1` (the `CONCAT11 & 0x2FF` folds decode to those bits) |
| The only `+0x43` writers in the image are the two install twins (`0x6E6C4` `FUN_0006E598`, `0x8DFBA` `FUN_0008DF84`) and a row-0x22 clear (`0x8540C`); the two turns only read it | fresh `search_instructions` destination filter `+ 0x43],` (6 hits; 2 `CMP`s at `0x7BF89`/`0x8E29C`, 1 unrelated `0xAE2C9`); the pool twin's byte sequence `0x8DFAC..0x8DFC1` (`flags&4 -> +0x45`, `flags&2 -> +0x43`, `flags&1 -> +0x44`) re-read fresh |
| Fresh table `0x10EF00`: **38 of 111** rows have `flags & 2` set, including row `0x00` (`flags 0x07`) and row `0x13` (`flags 0x02`) | `read_memory 0x10EF00/0x3E7`; rows `0x00` and `0x13` re-read explicitly |
| If `+0x43 != 0`: `move_attr = dword[+0x63]>>16` (the **target** distance `+0x65`, target `+0x4D/+0x55` minus pos `+0x59/+0x61`); `> 0x60` ⇒ target face = `word[+0x7F]` (the desired facing, written at `0x8E298` from `0x8DD70(target delta)`); `<= 0x60` ⇒ target face = the `[0x157A73]` focus-point bearing | `0x8E24B..0x8E278` (the target deltas + distance); `0x8E2A6..0x8E2B5` (the `>0x60` arm); `0x8E2B7..0x8E2E5` (the focus arm: `MOV EDX,[0x157A73]; CX=word[EDX]; AX=word[EDX+8]; SUB` against `+0x59/+0x61`; `0x8DD70`) |
| The turn rate is `(0x10 − word[+0x71]) << 2` (`<< 4` with a slot), clamped both ways; then `+0x7D := face & 0x3FF` and `+0x8E := 0x1104D2[(face & 0x3FF) >> 5]` — **both** cells | `0x8E2F5..0x8E362`; the `0x1104D2` table read at `0x8E35C`; the face store at `0x8E350`; twin `0x7C03F/0x7C051` |
| The sector table `0x1104D2` is the 32-byte `{0,0,0,1,1,1,1,2,…,7,7,7,7,0}` block; the port's `fifa96_match_heading_table` is byte-identical | `read_memory 0x1104D2/0x20`; `fifa96_match_locomotion_tables.c:21-23` |
| The mover is the **live** per-record turn: `FUN_0007BF20` is called from the two record-machine tails (`0x785C5` in `FUN_000782D0`, `0x7CD48` in `FUN_0007CA54`) for every record; `FUN_0008E244` is the twin with 3 callers | fresh `get_xrefs_to 0x7BF20` = 2, `0x8E244` = 3 (`0x8E7F3`, `0x8E8F2`, `0x8E5B4`) |
| The focus pointer `[0x157A73]` is written by six sites; four set it to the **camera triple** `0x15774C` (`0x74065`, `0x74107`, `0x74274`, `0x8A3B9`), one to a scratch point `0x1587C0` (`0x89FBD`), one to a caller `ESI` (`0x73E60`); the turn reads it at `0x8E2B7` / `0x7BFA4` | fresh `get_xrefs_to 0x157A73` = 13 (6 WRITE / 7 READ); disassembly of each writer; `0x1587C0` is a 2-word scratch point written at `0x8A008` from a record's `+0x59` |

**Important:** `+0x65` is the distance to the record's **target** (`+0x67 = word[+0x4D] − word[+0x59]`, `+0x69 = word[+0x55] − word[+0x61]` — target triple minus position; `0x8E24B..0x8E263`), **not** the camera `lane`. So the arm selection at the gate is a target-distance question: `> 0x60` targets `+0x7F` (the target bearing), `<= 0x60` targets the `[0x157A73]` focus. Both arms write both cells. If the target or focus is the camera, the record's face converges to the **camera bearing** and the `0x7EE7D` gate (`|bearing − face| <= 0xAB`) passes; if it is the ball, the face converges to the ball bearing and the gate only passes from a position where the two bearings agree (the R2 geometry). This is the native R1 route and a **direct both-cell writer**, not one of the 40 `0x79C50` sites.

## 5. The port's split, site by site (fresh grep + reads)

### 5.1 `fifa96_arm_face` — sector only, with the correct `(0,0)` guard

`fifa96_arm_helpers.c:73-94` computes `dx = target.x − pos.x`, `dz = target.z − pos.z`, returns early on `(0,0)`, and writes **only** `*out_lane = ((angle+0x40)&0x3FF)>>7`. It is the octant half of `0x79C50`. Every caller stores it into a `type`/`octant`/`facing` field, never a `face7d`:

| Port site | Row / model | Native site | Writes |
|---|---|---|---|
| `handlers.c:1495` (`fifa96_match_action_0E`) | 0x0E | `0x82794` | `r->type` |
| `handlers.c:5091` (`match_celebration_arm`, rows 0x14/0x25) | 0x14/0x25 | `0x87ACC`, `0x87B8D`, `0x8842B`, `0x884E0` | `r->type` |
| `handlers.c:6236` (`match_row15_face`) | 0x15 | `0x87F3B`, `0x87F7C` | `r->type` |
| `match_entities.c:563` (kickoff place, `FUN_00079B6C` tail) | place | `0x79BCF` | `e->type` |
| `ball_pairing.c:196` | ball pair | `0x7AA09` | `out->face` → `actor->facing` (the `+0x8E` byte; the struct comment at `ball_pairing.c:65` says so) |
| `arm_bodies.c:171` (arm 27) | 0x27 | `0x86913` | `rec->type` |
| `arm_bodies.c:271` (arm 28) | 0x28 | `0x87127` | `rec->type` |
| `arm_bodies.c:611` (arm 2A/0) | 0x2A | `0x86AC1` | no-op (`pos,pos`; matches the native `(0,0)` guard) |
| `arm_bodies.c:751/753` (arm 2A/10) | 0x2A | `0x86FA3`/`0x86FBD` | `rec->type` |
| `action_handlers.c:544-555` (row-05 carrier stage 2, inline) | 0x05 | `0x7F607` | `out->face` (sector), then `handlers.c:2951` writes `r->type` only |

### 5.2 `fifa96_entity_face` — the true both-cell helper

`fifa96_entity_face` (`fifa96_entity_update.c:201-209`) writes `*face7d` and `*sector` and keeps the `(0,0)` early return. Its live sites:

| Port site | Row | Native site | Writes |
|---|---|---|---|
| `handlers.c:3317` (`fifa96_match_action_0B`) | 0x0B | `0x79BCF` (the `0x81ADA` `0x79B6C`) | `r->face7d` + `r->type` |
| `handlers.c:4601` (`fifa96_match_action_1A`) | 0x1A | `0x7682E` | both |
| `handlers.c:5021` (`fifa96_match_action_09`) | 0x09 | `0x80AAB` | both |
| `handlers.c:5317` (`fifa96_match_action_16`) | 0x16 | `0x846E0` | both |
| `handlers.c:5394` (`fifa96_match_action_24`) | 0x24 | `0x865C6` | both (constant `(1,0)`, FU314-L5a fixed) |
| `keeper.c:527/545/708/782` | 0x1E | `0x756D0/0x7573C/0x75CDC/0x76012` | **face local discarded**; only `s->sector` persists |
| `action_handlers.c:1483` (`fifa96_action_sequence_0d_step`) | 0x0D | `0x826E3` | both into `out->face7d`/`out->face_sector`; the row is **unwired** (FU178-L1) |
| `action_handlers.c:1607-1614` (`kick_face`, rows 07/0F) | 07/0F | the `0x79B6C` tail (`0x79BCF`) | both (`face_word7d` + `facing`) |

The split is real and exactly as the spec describes: the native helper writes both at every non-zero site, while the port has **one sector-only helper used at 10 places**, **one both-cell helper used at 5 engine rows + 4 keeper sites (face dropped) + 1 unwired row + 1 kick helper**. The engine rows that run the code-4 record (0x04) call **neither**: the row-04 body has no `0x79C50` call at all (the only call in the `0x7E/0x7F` range is `0x7F607`, row **0x05**).

## 6. Which producer runs for a walking code-4 record

**Measured port state (phase-31 natural dump, `recs3.txt`, record 1:1 = entity 12):** code 4 from step 1802; `anim_id = 0`, `has_slot = 0`, `face7d = 0x100`, `type = 2`; `px` 68→56 over steps 5055..5075 (≈0.5 px/frame) and 4 at step 11999. The row-04 band entry is step ~5065 (`lane 62`, bound 66).

**Static chain for the native record:**

1. The row-04 dispatch on the half-line path (`camY <= 0x50`, the measured `camY = 0`) takes `0x7EE4B`; it does **not** run the `0x7EE3C` anim-0x13 install, which is on the `camY > 0x50` fall-through only (`0x7EDD8 JLE 0x7EE4B`; the install block is `0x7EE1A..0x7EE49`).
2. The record's anim row is therefore whatever the last install left. The port's measured `anim_id = 0` (row `0x00`, flags `0x07`) and the creation/kickoff alternative row `0x26` (flags `0x07`) are **both armed** (`flags & 2`). The exact native row at the band entry is not proven (leg FU318-L4), but every plausible initial row is armed.
3. The per-frame mover (`FUN_0007BF20`, called from `0x7CD48`/`0x785C5` after the action handler) runs the `0x8E29C`-shaped turn because `+0x43 != 0`. The arm is selected by the **target** distance `+0x65` (recomputed at `0x8E267..0x8E278`, `pos → target`, not the camera lane): `> 0x60` targets the `+0x7F` target bearing, `<= 0x60` targets the `[0x157A73]` focus (the camera triple in the four camera-writer paths). Either arm turns at rate `(0x10 − speed) << 2` and writes **both** `+0x7D` and `+0x8E`. With the measured walk (~0.5 px/frame, so `speed` far below `0x10`), the turn is fast. Which arm ran is not measured (the dump has no target); the gate alignment holds when the target/focus is the camera, and otherwise only from the R2 geometry.
4. No `0x79C50` site runs for this record in the window: `0x865C6` ran once at the cutscene (the dump shows face `0` at step 9 and `0x100` at step 12 — the row-0x24 stage-0 constant); `0x7F607` is row 05 and requires `has_slot` (measured 0); `0x8E1CB` needs row 0x14/0x52 (measured row 0); the remaining sites belong to rows that do not dispatch on this record.

**Verdict on the producer:** the natural producer is the **locomotion turn** (`fifa96_action_locomotion_step`'s `direct_face` block), i.e. the FU314-L5 cell `+0x43` + the focus point — **not** the FU314-L3 helper-site coverage. The port has the turn's full logic (`action_handlers.c:202-246`, including the `move_attr > 0x60` and focus arms and the `0x1104D2` heading write into `state->heading`), and the mover writes it back (`match_run.c:532/537`), but stages `direct_face = 0` and `face_x/face_z = 0` (`match_run.c:498`, `511`), so the native producer is inert and the face stays at the step-12 `0x100`.

**Counter-evidence / limits:** (i) the focus pointer's live value at the band entry is not measured — the four camera writers and the `0x1587C0` scratch writer are state-dependent (FU319 owns it); (ii) the native initial anim row is a leg; (iii) the turn's arm (`> 0x60` vs `<= 0x60`) depends on the record's **target distance**, which the phase-31 dump does not carry — if the target is the ball, the turn faces the ball, not the camera, and R1 does not by itself produce a camera-aligned face; (iv) the turn's `speed` input is derived from the port's px slope, not dumped; (v) with the port's **parked** camera (FU-71 NO-GO) the focus arm would target the parked origin, which is the native cell but not the native camera-on-the-ball state — landing it can move the sim in a non-native direction.

## 7. Consumers of `+0x7D` and the M2 risk

| Consumer | Fresh evidence | Live? |
|---|---|---|
| Row-04 half-line gate `0x7EE5C` → `fifa96_outfield.c:632` | `handlers.c:2654` stages the live `r->face7d` (FU314-L2 already landed); `outfield.c:632` subtracts it | **yes — the intended change** |
| Row-04 defender decision `0x7E600` → `fifa96_outfield.c:384` (called at `:581`, `0x7EC89`) | `s->face_word7d` diff `> 0x100` rejects; a changed face can flip the decision/install | **yes — first-order collateral** |
| Row-0x0B ±0x100 face clamp (`0x82915..0x8295E`) | `handlers.c:1392-1397` | yes |
| Row-0x0B "found record" choice / velocity | `handlers.c:3268/3298`, `3427` (`sine(face7d)`) | yes |
| Row-0x1A movement face (0x7682E) | `handlers.c:4601` | yes |
| Row-0x1B deep block `0x7711B` (ported inside `fifa96_match_action_1F`) | `handlers.c:4061` (`angle − face7d`); the row-0x1C face is FU186-L2 | yes |
| Kick decisions `kick_decision`/`0x8164B` | `action_handlers.c:1598` (own `face_word7d`), `1705` (`opp_face_word` = `opp->face7d`, `handlers.c:1063`) | yes |
| Render sprite direction | `match_run.c:4131/4278` (`visual_dir` from the facing) | yes (visual) |
| Pool transfer | `match_run.c:1413` (load) / `1561` (store) | yes |
| Ball pairing | `ball_pairing.c:65/196` — its `actor->facing` is the **sector**; the ball's `+0x7D` is not modelled | sector-only |

**M2 risk:** enabling the turn writes `face7d`/`type` on **every frame the mover runs** for any record whose installed row has `flags & 2` — **38 of 111** rows, including the plausible initial rows 0x00/0x26 and the walk row 0x13. The diff is therefore **broad and AI-visible** (row-04 defender decisions/installs, row-0x0B/0x1A gates, kick decisions, render direction), not a pan-local change. It must be landed as a fidelity slice with the FU321 accounting, not as a witness convenience; the parked-camera confound means the moved frames may not all be the intended camera-facing consequence. The helper-site fidelity alone (the L3 migration) changes only the rows that call `fifa96_arm_face`; it does not produce the walker's face and its M2 exposure is row-local.

## 8. The faithful port shape

**The natural producer (minimal faithful landing, FU319):**

1. Model `+0x43` on the pool record (`e->direct_face43`), written by `fifa96_match_run_record_anim_install` as `(uint8_t)(row.flags & 2u)` — the native `FUN_0006E598` `+0x43 = descriptor[2] & 2` (fresh decompile). Not a gameplay-request byte, and not a constant.
2. In `match_run_record_mover` (`match_run.c:504-548`): `s.direct_face = r->direct_face43;` and stage the live focus point `s.face_x = <[0x157A73].x>; s.face_z = <[0x157A73].z>` (**the camera triple `0x15774C` when that is the live writer**), not `0` and not a constant. The `move_attr` target distance is already staged (`match_run.c:521-522`) and the `+0x7D`/`+0x8E` write-back is already present (`match_run.c:532/537`; the `+0x7F` desired face is `state->desired_facing`, consumed by the stride error at `action_handlers.c:258`).
3. `fifa96_action_locomotion_step` needs **no change**: the `move_attr > 0x60` arm, the focus arm, the shift/limit, the `0x3FF` fold and the `0x1104D2` heading table are already the native shape (`action_handlers.c:219-246`).

**The helper-site fidelity (FU314-L3 as fidelity, not as the witness fix):** at the five engine-record `fifa96_arm_face` sites, call `fifa96_entity_face(to.x−from.x, to.z−from.z, &r->face7d, &r->type)` (the `(0,0)` guard is inside it), or add the face field where the model has none (keeper/arm/ball). This is a row-local fidelity change; it does not make the walking code-4 record face the camera.

**The frame-wrap arm:** add the missing `face7d = (face ± 0x200) & 0x3FF` to `fifa96_match_run_record_anim_successor` (`match_run.c:1055-1059`), keeping the `(type+4)&7` sector flip. It fires only at rows 0x14/0x52 wraps and is not the walker's producer.

**What must NOT be done:** do not stage `face7d` or the focus to force the gate (spec §8.2); do not add a face write to a `0x79C50` site as the pan witness fix — none of them runs for the walker.

## 9. Verdict

**GO on the census and the production contract:** the 40 `0x79C50` sites are fully classified (8 always-both, 1 constant no-op, 31 runtime; the helper writes both cells only on non-zero, and the `(0,0)` arm returns the sector). The port's split is real: `fifa96_arm_face` is sector-only (10 sites) while `fifa96_entity_face` is the true both-cell helper (5 engine rows, 4 keeper sites with the face dropped, 1 unwired row, 1 kick inline).

**NO-GO on the L3 helper-site / frame-wrap route as the natural producer for the phase-32 witness:** neither family runs for the walking code-4 record. The only per-frame native writer is the **locomotion turn**, armed by the animation row's `flags & 2` (`+0x43`) and targeting the `[0x157A73]` focus; it writes both cells.

**GO (conditional) on the FU319 landing as the only faithful producer:** the shape is exact (§8), the turn is already ported, and the missing inputs are two live cells (`+0x43`, the focus). Conditions: (a) FU319 must prove the live focus value **and** the target distance/identity at the band entry (camera triple vs the `0x1587C0` scratch; target vs focus arm) — otherwise R1 is only a ball-facing turn, not a camera-facing one; and (b) the M2 diff must be accepted as a **broad fidelity move** (38/111 rows), with the parked-camera confound disclosed. Until both hold, the phase-32 witness stays the drive-only R2 route (FU320), and the producer work is a leg.

## 10. Legs

1. **FU318-L1 — the 40-site port mapping for the unported rows.** The table attributes each site to a row body by the fresh `0x1106E0` entry table + address range and the fresh grep; a per-site decompile of every unported row body (0x1B/0x1C/0x08/0x0F/0x10/0x11/0x25 and `FUN_0006E8E8`) is not done here. Owner: the row bodies + the animation event machine.
2. **FU318-L2 — the frame-wrap face write.** `match_run.c:1055-1059` flips the octant only; the native `0x8E1CB` writes `+0x7D` (and the sector from the face) too. Owner: the animation driver (`FUN_0008E008` slice).
3. **FU318-L3 — the `[0x157A73]` live value and the turn's arm at the band entry.** The focus pointer can be the camera triple `0x15774C` (four writers), the scratch `0x1587C0` (one writer) or a caller record `ESI` (one); the six writers are known, the live one is a match-state question. Independently, the arm is selected by the record's **target distance** (`+0x65`, `0x8E267..0x8E278`), which the phase-31 dump does not carry. Owner: FU319.
4. **FU318-L4 — the native anim row at the band entry and the turn's `speed`.** The port dump gives `anim_id = 0` (model default) and the px slope; the native creation install id (`FUN_0008CFAC` is caller-param driven) and `word[+0x71]` at the gate are not dumped. Owner: FU320's probe or a one-field dump.
5. **FU318-L5 — the keeper/arm face cells.** `keeper.c:527/545/708/782` discard the computed face; the arm record has no `face7d` field. The native writes both. Owner: the keeper/arm models.
6. **FU318-L6 — the row-0x0D both-cell output is dead.** `action_handlers.c:1483` writes `out->face7d` but the row has no engine installer (FU178-L1); the native site `0x826E3` cannot run in the port. Owner: the row-0x0D installer.
7. **FU318-L7 — the alternates.** `0x6F90A/0x6F945` (`FUN_0006E8E8` sector-only), `0x81BE3` (row-0x0B sector copy) and `0x8E691`/`0x8E9EE` (constant sector `2` + face) are outside the 40-site census and are not audited for port status here. Owner: the respective row/animation bodies.
8. **FU318-L8 — the FU314 stale line numbers.** FU314 §2.3 cites `handlers.c:5083/3311/4596/5014/5310/5382`; the fresh search finds `5091/3317/4601/5021/5317/5394` plus the newer `6236`, `ball_pairing.c:196`, `arm_bodies.c:171/271/611/751/753`. This doc's lines are the fresh ones.

## 11. Provenance and known limits

**Fresh this session (Ghidra MCP, `/FIFA96.EXE`):** `disassemble_bytes` `0x79C50/0x60`, `0x79C99/0x50`, `0x8E1A0/0x60`, `0x8E240/0x90`, `0x8E2B7/0xD0`, `0x8E360/0x60`, `0x8E2CE..0x8E386`, `0x7BF60/0x110`, `0x7F5D0/0xB0`, `0x7F680/0x60`, `0x7EE10/0x50`, `0x7E820/0x60`, `0x81BA0/0x60`, `0x89FF0/0x30`, `0x8A3D0/0x40`, `0x8CFA0/0x60`, `0x8CF00/0x60`, plus the 40-site argument windows (site − 0x30, length 0x36); `decompile_function` `0x8E008`, `0x6E8E8`, `0x6E598`, `0x8DCD4`, `0x79B6C`, `0xCD474`, `0x85498`, `0x89EB0`; `read_memory` `0x10F334/0x40`, `0x10F33C/0x40`, `0x10EF00/0x3E7`, `0x1104D2/0x20`, `0x1106E0/0x100`, `0x157A73/4`, `0x157588/8`; `get_xrefs_to` `0x79C50` (**40**), `0x79C99` (0), `0x7BF20` (2), `0x8E244` (3), `0x8E008` (3), `0x6E598` (84), `0x6E8E8` (2), `0x8E748` (1), `0x8E810` (1), `0x157A73` (13), `0x1587C0` (2); `search_instructions` `+0x7d]` (MOV-only 24 hits; all-mnemonic 41 hits — the same 6 destination writers) and `MOV` destinations `+0x8e]` (78 hits, 8 destination writes); `get_function_by_address` `0x88940`.

**Carried:** the row-04 body range `0x7E7C8..0x7F141` and the two pan arms (FU314 §2.1); the row-04 gate algebra (FU314 §2.2); the anim-row layout and the `0x10EF00` table provenance (FU-159, `match_run.c:40-159`); the FU-71 camera triple and the parked-camera NO-GO (FU157/FU314); the M1 `09b726b7…` and M2 `8434cfab…` baselines and the `60/80` census (`docs/ENGINE.md`).

**Measurement artifact:** `/tmp/opencode/fu314/recs3.txt` (the phase-31 3,598-granted-frame dump; fields `bt:bi:active:code:lane_x:bound:timer81:anim_id:has_slot:pos_x:pos_z:face7d:type:has_ball`, `probe.c:3466`). No repo file was modified; the ISO was only read.

**Known limits:** the argument classification of the 40 sites is from a fixed instruction window per site and the fresh xref list; a call reached with a zero vector at runtime is a no-op even in class C (the table marks this). The port's line numbers are `main` at this session's HEAD; FU314's are stale (leg FU318-L8). The turn's focus and the native anim row at the gate are **not** proven live (legs FU318-L3/L4); the M2 blast radius is derived from the 38/111 descriptor flags, not from a simulated diff (the wave may not run one). No claim is made that the native turn's focus is the camera for every match state.

## Review erratum (wave review, 2026-10-22)

**Quote (§8, the natural-producer shape step 2):** "The `move_attr` target distance is already staged (`match_run.c:521-522`)". **Corrected (cross-slice; FU319 §4/§7.3):** it is staged, but from `fifa96_arm_dist_stage` — the `0x8DCD4` octagon — not the native `+0x65` metric (`0x8DC68`) the turn's `>0x60` branch reads. FU319 measures **2** record-frames where the two metrics straddle `0x60` (max |Δ| = 115 units) and carries the correction as **FU319-L3**, a separately pinned sub-change (the same wrong metric also feeds the `>>19` stride attr). The verdict is unchanged: the staging shape is right, the metric is not yet the native one.

**Also (FU314 cite drift):** FU314 §2.3/§7 cites `handlers.c:3311/4596/5014/5310/5382`; the fresh sites are `3317/4601/5021/5317/5394`, plus `6236`, `ball_pairing.c:196`, `arm_bodies.c:171/271/611/751/753`. This is leg **FU318-L8**, recorded here; FU314 is not edited.
