# FU294 — the `0x71C94` pan/camera-event setter and its `[0x1577B8]` / `[0x1577CA]` consumers

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 5 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0, `.image`
overlay, 2706 functions; confirmed this session by `get_current_program_info`). Every
native address/byte below is a **fresh** `disassemble_function` / `disassemble_bytes` /
`read_memory` / `get_xrefs_to` / `get_function_by_address` of *this* session unless marked
*carried* (the frozen `docs/ghidra/FU*` slice is named). Alias traps applied
(`dword[0x1577EE]>>0x10` = the word at `0x1577F0`). No rename / comment / label /
function-create / script / project-save; `/FIFA96.EXE` untouched; no git.

---

## 1. The setter and its write set

`FUN_00071c94(int *param_1 = EAX = rec, undefined4 *param_2 = EDX = vec seed,
short param_3 = BX = height, ECX sign param)`, body `0x71C94..0x71DF2` (91 insns, fresh
`disassemble_function`; real function per `get_function_by_address`). Gate at entry:
`0x71C99 CMP byte[0x157A6C],0 ; JNZ 0x71DEF` — **early return** if the event is suspended
(the FU-148 §2.1(c) bail). `0x71CC3 CALL 0x700F4` (flag 0, `PUSH 0` at `0x71CA6`) is the
camera-place primitive FU285-L7. Every store below is fresh `read_memory 0x71D07` (229 B) +
`disassemble_function 0x71C94`.

| cell | width | raw bytes | insn | value | semantics | engine field |
|---|---|---|---|---|---|---|
| `[0x1577B8..0x1577BD]` | 6 B | `A5 66 A5` | `0x71CC8` (MOVSD; MOVSW, `EDI=0x1577B8` @`0x71CBE`) | `*param_2` (EDX vec) | event seed triple `{bearing, step_x, step_z}` | `camera.event_step_x/z` (`[0x1577BA]/[0x1577BC]`, camera.h:42) + bearing |
| `[0x1577F0]` | word | `66 89 1D F0 77 15 00` | `0x71CD4` | `BX` = height | event height → **low-clamped** to `[0x157750]` (`0x71CE5 66 A3 F0 77 15 00`) → **high-clamped** `0x640` (`0x71CFA`, reads its own just-stored word via `dword[0x1577EE]>>16`) | `camera.event_param` |
| `[0x1577CA]` | dword | `89 2D CA 77 15 00` | `0x71D27` | `EBP` = rec | tracked event/keeper actor record | `camera.tracked` |
| `[0x1577CE + side*4]` | dword | `89 2C 85 CE 77 15 00` | `0x71D1D` | `EBP`; `side=byte[[rec]+0x826]` | per-side tracked actor array | **UNMODELED** |
| `[0x157815]` | byte | `88 15 15 78 15 00` / `C6 05 15 78 15 00 FF` / `A2 15 78 15 00` | `0x71D6A` / `0x71DB1` / `0x71DDC` | `DL`(0x70544 ret) / `0xFF` / `AL`(0x8DD70 heading) | pan-direction byte (`0x10E169` two-dir table or `0x8DD70`) | **UNMODELED** |
| `[0x157822]` | byte | `88 1D 22 78 15 00` | `0x71DE3` | `0` | clear (smoothing) | **leg** |
| `[0x157820]` | byte | `88 1D 20 78 15 00` | `0x71DE9` | `0` | clear (smoothing) | **leg** |
| `[0x158777]` | dword | `89 1D 77 87 15 00` | `0x71D0C` | `0` | clear take slot | **leg** |
| `byte[rec+0x9B]` | byte | `C6 85 9B 00 00 00 00` | `0x71D2D` | `0` | clear rec take flag (release) | modeled (FU151) |
| `[[rec]+0x7A6]+0x7BF` | dword | `89 98 BF 07 00 00` | `0x71D3A` | `0` | side-record field clear | **leg** |
| `[[rec]+0x7A6]+0x7CB` | dword | `89 98 CB 07 00 00` | `0x71D49` | `0` | side-record field clear | **leg** |
| `[[rec]]+0x7CB` | dword | `89 98 CB 07 00 00` | `0x71D5A` | `0` (cond, `EBP != [rec+0x7CB]`) | rec-side field clear | **leg** |
| camera triple | — | — | `CALL 0x700F4` @`0x71CC3` (flag 0) | `[0x15774C/50/54]` | place camera (FU285-L7) | `camera.pos_x/y/z` |

The dir byte is written three ways: the `0x70544` return in `DL` (`0x71D6A`), then either
the slot dir pair path (`0x71DB1 0xFF` when both `[rec+0x20]+0x20/+0x21` are 0 and
`word[0x1577BE]==0`, else the `0x10E169[[rec+0x20]+0x21*4 + (+0x20)]` table byte `0x71D9E`),
or the `0x8DD70(heading)` path → `(heading+0x40)&0x3FF >>7` (`0x71DDC`). `CALL 0x70544`
rel32 at `0x71D65` = `e8 da e7 ff ff` → `0x70544` (verified); `CALL 0x8DD70` rel32
`e8 a0 bf 01 00` → `0x8DD70`.

---

## 2. Consumers (fresh `get_xrefs_to`)

### `[0x1577CA]` — fresh = **88** (3 WRITE, ~85 READ)

**Writers:** `0x71D27` `FUN_00071c94` (store rec); clears `0x70258` `FUN_000700f4`,
`0x70054` `FUN_0006ffc0`.

**Camera/pan read consumers:** `FUN_000736ac` (`0x73A1E`,`0x73B5B` — the every-frame
`FUN_00071DF4` follow gate, camera.h:1136-1151); `FUN_00071df4`
(`0x71DF8`,`0x71E4E`,`0x71F0E`,`0x72000`); `FUN_0007131c` (`0x71497..0x7189E`,`0x71B85` —
the boundary pan armer FU-145 §3); `FUN_00070de0` (`0x70F3F`,`0x70F60`,`0x70FC9`,`0x70FEA`
— reposition); `FUN_00072478` (`0x72610..0x72AAF`); `FUN_000703e8` (`0x70485`);
`FUN_00072ac4` (`0x72AFD`); `FUN_0007bf20` (`0x7C828`); `FUN_0007ca54` (`0x7CB4B`);
`FUN_0007cd60` (`0x7CD9E`); `FUN_000765c4` (`0x765C7` — keeper `tracked_present`);
`FUN_000782d0` (`0x783E3`); `FUN_0007e600` (`0x7E62F`); `FUN_00088940`
(`0x88B6E`,`0x88BE3`); `FUN_00089eb0` (`0x89EB5`).

**Row bodies (no Ghidra function):** `0x76FE1`/`0x7707D`/`0x7717D`/`0x771C1`/`0x77482`/
`0x77602` — row `0x1B` wide, the tracked-keeper mutual exclusion `EBP==[0x1577CA]`; `0x77E3D`
(row `0x1C`); `0x8104E`/`0x82806`/`0x81498`/`0x82DE7` (row `0x0D` body); `0x7E29E`/`0x7E994`/
`0x7EDA0`/`0x7EE83`/`0x7EFEE`/`0x7C940` (rows 04/presentation).

### `[0x1577B8]` — fresh = **6**

**Writers:** `0x71CC8` `FUN_00071c94` (the 6-byte copy), `0x701DE` `FUN_000700f4`,
`0x707EB` `FUN_00070544`. **Readers:** `0x70649`,`0x707F1` `FUN_00070544`. **DATA/LEA:**
`0x71CBE` `FUN_00071c94`.

The record's middle word `[0x1577BA]` (fresh = **5**, `event_step_x`): WRITE `0x701D8`
`FUN_000700f4`, `0x707CD` `FUN_00070544`; READ `0x70661`,`0x70801` `FUN_00070544`,
`0x70C47` `FUN_00070c08`.

---

## 3. Engine camera mapping

`fifa96_camera` (`include/fifa96_loader/fifa96_camera.h`) already maps the ported subset:

| native cell | engine field | evidence |
|---|---|---|
| `[0x1577CA]` | `camera.tracked` (int32); `FIFA96_CAMERA_TRACKED_NONE` = native NULL | camera.h:43-54,84-87; port `fifa96_camera_set_tracked` |
| `[0x1577B8..BD]` | the event seed; `camera.event_step_x/z` (`[0x1577BA]/[0x1577BC]`) + bearing | camera.h:42,280-301; port `fifa96_camera_event_set` (FU-148 §12.1) |
| `[0x1577F0]` | `camera.event_param` | camera.h:18,62-68 |
| `[0x1577CE+side*4]` | — | **UNMODELED** (FU294-L1) |
| `[0x157815]` + `[0x157820]/[0x157822]` | — | **UNMODELED** (OL-T11-79; FU294-L2/L3) |
| `[0x158777]`, side-record clears | — | **UNMODELED** (FU294-L5) |

Wired call sites: `handlers.c:552`/`:1379`/`:2722` (`fifa96_camera_event_set` +
`fifa96_camera_set_tracked`), and the row-`0x1B` wide `0x77423 CALL 0x71C94`
(`handlers.c:3871`, FU285). The `0x71C94` xref set (fresh = **11**: `0x71B8A`, `0x70DD1`,
`0x7A219`, `0x6FA62`, `0x75DC4`, `0x77423`, `0x7F4E7`, `0x82A6C`, `0x77DC6`, `0x7EFCF`,
`0x7F0D1`) matches FU-148 §13.4 / FU-152 carried.

---

## 4. GO / NO-GO

**GO — the setter is already ported.** `fifa96_camera_event_set` (the FUN_00071C94 +
FUN_00070544 subset, FU-148 §12.1) is complete and its tracked-cell write is ported as
`fifa96_camera_set_tracked` → `camera.tracked`; the row-`0x1B` `0x77423` caller is wired
(handlers.c:3871). The unported residual is narrow and bounded by the leg list below — the
per-side array `[0x1577CE+side*4]`, the dir byte `[0x157815]`, the `0x15780C..0x157822`
smoothing family and the `[0x158777]`/side-record clears. **NO-GO only for those legs**, not
for the setter core.

---

## 5. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| `FUN_00071c94` body `0x71C94..0x71DF2`, sig `(int*,undefined4*,short)`, 91 insns | `disassemble_function 0x71C94`; `get_function_by_address 0x71C94` |
| bail gate `0x71C99 CMP byte[0x157A6C],0 ; JNZ 0x71DEF`; `0x71CA6 PUSH 0`; `0x71CC3 CALL 0x700F4` | `disassemble_function 0x71C94` |
| 6-byte copy `A5 66 A5` @`0x71CC8` → `[0x1577B8..BD]` (`EDI=0x1577B8` @`0x71CBE`) | `disassemble_function`; `read_memory 0x71CC8` |
| `[0x1577F0]` store/low/high clamp `0x71CD4`/`0x71CE5`/`0x71CFA`, self-read `dword[0x1577EE]>>16` | `disassemble_function`; `read_memory 0x71D07` (229 B) |
| `[0x1577CA]` store `89 2D CA 77 15 00` @`0x71D27` | `read_memory 0x71D07` |
| per-side `[0x1577CE+side*4]` `89 2C 85 CE 77 15 00` @`0x71D1D`; `side=byte[[rec]+0x826]` | `read_memory 0x71D07` |
| dir byte `[0x157815]` writers `0x71D6A`/`0x71DB1`/`0x71DDC`; clears `0x158777`/`0x157822`/`0x157820`; `byte[rec+0x9B]` | `read_memory 0x71D07` |
| `CALL 0x70544` rel32 `e8 da e7 ff ff`; `CALL 0x8DD70` rel32 `e8 a0 bf 01 00` | `read_memory 0x71D07` |
| xrefs to `0x71C94` = 11; `0x1577CA` = 88 (3 WRITE); `0x1577B8` = 6; `0x1577BA` = 5; `0x1577CE` = 3; `0x157815` = 6; `0x158777` = 6 | `get_xrefs_to` each |
| engine mapping / ports / wired call sites | `include/fifa96_loader/fifa96_camera.h:7-69,280-301`; `handlers.c:552,1379,2722,3847-3871` |

**Carried docs (read, not rewritten):** FU285 §5 (the `0x71C94` read this derivation
re-verifies); FU-148 §12.1/§13.4; FU-145 §3 / OL-T11-79; FU-152 §2.9.

---

## 6. Numbered open legs (with native owners)

* **FU294-L1 — the per-side tracked array `[0x1577CE+side*4]`.** Writer `0x71D1D` (the
  setter); `side = byte[[rec]+0x826]`. Only two other refs (`FUN_0006ffc0 0x70059`,
  `FUN_00088940 0x88AAC`). Unmodeled in `fifa96_camera`. Owner: `fifa96_camera`.
* **FU294-L2 — the pan-direction byte `[0x157815]`.** Three writers
  (`0x71D6A`/`0x71DB1`/`0x71DDC`), two readers (`FUN_00071df4 0x71EF0`/`0x71F35`); the
  `0x10E169` two-dir table and the `0x8DD70` heading fold. Unmodeled. Owner: OL-T11-79.
* **FU294-L3 — the smoothing clears `[0x157820]`/`[0x157822]`** (the `0x15780C..0x157822`
  smoothing family) and the `[0x158777]` take-slot clear. Unmodeled. Owner: OL-T11-79.
* **FU294-L4 — `[0x1577B8]` is shared with the camera-place primitive.** Fresh xrefs show
  `FUN_000700f4 0x701DE` and `FUN_00070544 0x707EB` also write it, so the 6-byte seed is not
  exclusive to the setter; the engine ports the seed only through `fifa96_camera_event_set`.
  Owner: `fifa96_camera_event_set` / `FUN_000700f4`.
* **FU294-L5 — the side-record clears.** `0x71D3A`/`0x71D49`/`0x71D5A` write the
  `[rec+0x7A6]` side-stats descriptor (+0x7BF/+0x7CB) and the `[[rec]]+0x7CB` rec-side field;
  unmodeled. Owner: FU-152 §2.9.
