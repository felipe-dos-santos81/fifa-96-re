# FU-199 — the formation/line-up setup `FUN_00011620`/`FUN_00012010` (the keystone): the id, the desired-order matrix, and the real producer of the `[rec+8/+0xC/+0x10/+0x14]` OL-13 metadata

Provenance: M2 phase-18 wave **R47**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` this session = `watcom:LE:32:default`,
`image_base 0x0`, `min_address 0x10000`, 2706 functions). Every
address/byte/xref/instruction below is a fresh `read_memory` /
`disassemble_bytes` / `disassemble_function` / `decompile_function` /
`get_function_by_address` / `get_xrefs_to` / `search_instructions` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). The
addresses quoted are the `/FIFA96.EXE` program addresses (already the runtime
ones). The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte
at `x+3`). No writes: no rename, comment, label, function, script or project
save; `/FIFA96.EXE` untouched; no git commands.

Scope (the brief): derive `FUN_00011620`/`FUN_00012010` byte-level — what they
read (the roster, the `[0x14C1E4]`/`[0x14C1E5]` id, the `352ko.fmt` family), what
per-record metadata they write (the brief's `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`,
the OL-13 source of FU192-L1) and the `0x14C360`/`0x157BD2` desired/current-order
matrices (FU197-L4); the fresh caller census (which call reaches it — the match
begin or a new setup step?); and where the outputs must land in the engine's
record/team model.

**Headline (fresh).** `FUN_00011620` is the **roster-driven formation builder**:
it reads the **`PCCD.DB`** team/player database (not the `.fmt`), decodes the
team's 16-player roster into the `0x1430B2` slot records, and publishes the team's
**formation id** (`[0x14C1E4+team]`) and **desired line-up order**
(`0x14C360 + team*0x7B`). `FUN_00012010` is the **line-up sort** over that order.
It runs from the **front-end team/formation-selection screens**, not the match
begin. It does **not** write `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`: those OL-13
metadata pointers are written at match begin by **`FUN_0006D920`**, keyed by the
formation id, from the **`.fmt` family** `FUN_0004C384(id*6 + {0,1,2,4})`. So the
brief's premise is *corrected*: `FUN_00011620` produces the **id** that roots the
OL-13 family, not the pointers themselves (§5).

---

## 1. Fresh anchors

`get_function_by_address`:

* `FUN_00011620` — body `0x11620..0x11976` (854 B).
* `FUN_00012010` — body `0x12010..0x12227` (535 B).
* `FUN_00011978` — the setup twin (reads an in-memory team record `param_2`).
* `FUN_000101c8` — the record decoder dispatcher (`0x101C8..0x103A1`).
* `FUN_000109a0` — per-player record loader (`0x109A0..0x109F0`).
* `FUN_000115cc` — the string-arena interner (`0x115CC..0x1161D`).

`read_memory 0x11033a` (93 B) — the formation **layout table**, byte-identical to
the engine's `match_formation_layout_table` (`fifa96_match_entities.c:366`), 5
rows × `0x1D`:

```
id0: 00 00 01 00 FF FF FF FF  01 03 01 02 03 FF FF  02 05 04 05 06 07 08  03 02 09 0A FF FF FF
id1: 01 00 01 00 FF FF FF FF  01 04 01 02 03 04 FF  02 04 05 06 07 08 FF  03 02 09 0A FF FF FF
id2: 02 ...
id3: 03 ...
id4: 04 ...
```

`read_memory 0x100000` (32 B) — the loader strings: `01 01 01 00 "PCCD.DB" 00
"PCDB" 00 b9 80 8a "PCINDEX.NDX" 00`. So `s_PCCD_DB_00100004` = **`PCCD.DB`**,
the team/player database object.

`read_memory 0x101a30` (96 B) + `0x107370` (48 B) — the formation-name family:
the pointer table `0x107370` = `{0x101A30, 0x101A38, 0x101A40, 0x101A48,
0x101A50, 0x101A5C, 0x101A64, 0x101A6C, 0x101A74, 0x101A7C, 0x101A50, 0x101A84}`
→ strings `"352ko"`, `"352pl"`, `"352pk"`, `"352ps"`, `"freekick"`, `"352sp"`,
`"442ko"`, `"442pl"`, `"442pk"`, `"442ps"`, `"freekick"`, `"442sp"` (6 names per
formation id; *cross-cited* FU-130 §"0x101A30", FU-148 §3 for the `.fmt` sugar
`FUN_0004A6BC` appends).

`read_memory 0x14c1e0` (32 B) = all zero — `0x14C1E4` and the `0x14C360` matrix
are **BSS** in the static image: they are produced at runtime, which is exactly
what this slice derives.

---

## 2. The fresh caller census (the brief's key question)

Fresh `get_xrefs_to`:

* **`FUN_00011620` = 5**: `0x11BB6`/`0x11BC5` (both in `FUN_00011b7c`) and
  `0x25B4C`/`0x25B6B`/`0x25C64` (all in `FUN_00024b00`).
* **`FUN_00012010` = 12**: `FUN_00018680` (×2), `FUN_00018108` (×2),
  `FUN_00024b00` (×2), `FUN_000302dc` (×2), `FUN_00032520` (×2),
  `FUN_00029f68` (×2).
* **`FUN_00011b7c` = 5**: `FUN_00018680`, `FUN_00018108`, `FUN_000302dc`,
  `FUN_00032520`, `FUN_00029f68` — the **same** `0x18xxx`/`0x29xxx`/`0x30xxx`/
  `0x32xxx` cluster that calls `FUN_00012010`.

**Verdict: the setup runs from the front-end, not the match begin.** The caller
set is the team/formation-selection menu cluster. `FUN_00024b00` is a large
screen routine that renders with the strings `s_ftmsel_inv_00100a9c` /
`s__ptmsel_inv_00100aa7` ("formation-team select"), fills the per-side formation
cells `DAT_00105470`/`DAT_00105474` and `DAT_00105478`/`DAT_0010547c`, and at its
`LAB_00025cfb` tail calls:

```
FUN_00011620(0, DAT_00105470, DAT_00105474)      ; team 0 (formation id, roster idx)
FUN_00011620(1, DAT_00105470, DAT_00105474)      ; (param_1==3 path)
...
FUN_00011978(0, ...) / FUN_00011978(1, ...)      ; the in-memory twin
FUN_00012010(0) / FUN_00012010(1)                ; the line-up sort
```

`FUN_00011b7c` is the "load the DB, then build both teams' line-ups" helper:
`FUN_00010020` (loads `PCCD.DB`), `FUN_00073cd0` (a reset), zeroes the
`0x14C1CC..0x14C1E4` region (`0x286` words), `FUN_0003749c`/`FUN_0001c9bc`/
`FUN_00011590` (input/range/string housekeeping), then
`FUN_00011620(0, param_1, id0)` and `FUN_00011620(1, param_3, id1)`.

**No match-loop caller exists** for either function. The match begin
(`FUN_00074034`, `FUN_0004b020`) calls the **reset** `FUN_00073EE0` (which only
**copies** the already-built `0x14C360` into `0x157BD2`, §6) and, via
`FUN_0007412C` (§5), **consumes** the ids `FUN_00011620` produced.

> **Erratum (quoted).** `fifa96_match_run.c:2226` calls `FUN_00011B7C` "the native
> match-init". Fresh xrefs show `FUN_00011B7C` is entered from the five
> front-end selection functions above and itself loads `PCCD.DB` and builds both
> line-ups. It is a **pre-match selection/setup step** (run when a team/formation
> is chosen), not the in-match begin. The in-match begin's relationship to the
> ids is the **consumer** `FUN_0007412C` (§5).

---

## 3. `FUN_00011620` byte-level (fresh `disassemble_function 0x11620`, 189 instrs)

`FUN_00011620(EAX=team, EDX=mode, EBX=idx)`.

### 3.1 Reads (the roster)

`0x11666..0x1168F`: it navigates the `PCCD.DB` object `DAT_00104f24`
(`[0x104f24]`; loaded by `FUN_00010020` from `s_PCCD_DB` "PCCD.DB", §1) to the
team/mode/explicit-index record and calls `FUN_000101c8(record, stackbuf)` — the
record **decoder** (`0x101C8`, a jump-table dispatcher over the record type,
`WARNING: Could not recover jumptable at 0x102A6`). It repeats the decode into
`&DAT_00143018 + team*0x49` (the per-team header). `0x1169B` reads the decoded
`[ESP+0x45]` = the slot **count**; `0x116DE` clamps it to `0x10`; when the count
is `< 0x10` it re-decodes a second record (`0x1172E`, into `[ESP+0x4C]`) and
takes *its* count (`0x11735 MOV EAX,[ESP+0x91]`).

### 3.2 The per-slot loop (`0x117A2..0x11873`)

For `i = 0..count-1`:

```
0x117E4 CALL 0x109a0          ; FUN_000109a0(mode, idx, i, slot = 0x1430B2 + team*0x250 + i*0x25)
0x117F0 MOV [EBP + 0x14c360], ESI   ; **0x14C360 + team*0x7B + i*4 = &slot**   <- the desired-order matrix
0x117F6 MOV EAX,[slot + 0x17] ; SAR 0x18 ; if > 0xF -> [slot+0x17] = 0xF      (0x117F6..0x11818)
0x11826 MOV [EDX + 0x14c3b0], AX    ; **0x14C3B0 + team*0x7B + i*2 = (0xF - slot[+0x1A]) * 0x200**
0x11837 if mode==4 && slot[+0xB]!=0 -> slot[+0xA] = slot[+0xB]
```

`FUN_000109a0(mode, idx, i, dest)` (fresh `decompile`) navigates the same
`PCCD.DB` object to the `(mode, idx, i)` player record and calls
`FUN_000103d0(record, dest)` — the per-player decoder (also a jump-table
dispatcher). So each `0x1430B2` slot is one decoded roster row (id dword at
`[slot+0]`, interned name pointer at `[slot+4]`, position code at `[slot+9]`,
sort key at `[slot+0xC]`, …), 0x25 bytes, 16 per team.

### 3.3 The team header + id (`0x11879..0x11976`)

```
0x11895 AL=byte[0x14302A + team*0x49] ; MOV [EDX + 0x14c1e4], AL   ; **id -> [0x14C1E4 + team]**
0x118A1 AL=byte[0x143028 + team*0x49] ; MOV [EDX + 0x14c334], AL   ; companion byte
0x118B4.. 0x14C344/0x14C348/0x14C34C/0x14C350 = sign-extended
          header bytes 0x143036/0x143037/0x143038/0x143039
0x118F4/0x11905/0x11916  three header char* (0x14301C/0x143020/0x143024) -> FUN_000115cc (interned)
0x11934..  per slot, slot[+4] -> FUN_000115cc (the name interner, arena 0x143552)
```

Also, before the loop: `0x1164E [0x1496E0 + team*4] = -1`; `0x11660
[0x143010 + team*4] = (-1 | idx if mode==4)`; `0x116D7 [0x1430AA + team*4] =
mode`; `0x1175F [0x14C1CC + team*2] = word`; `0x1178B [0x14305D + team*0x49] =
0x10`; `0x1179C [0x14C35C + team*0x7B] = &0x143018 + team*0x49` (the team-header
pointer that `FUN_00012010`'s `FUN_00011f0c` reads through `0x14C35C`).

**Summary — `FUN_00011620` writes only:** the `PCCD.DB`-derived per-team header
`0x143018+team*0x49`, the 16 slot records `0x1430B2+team*0x250`, the header
pointer `0x14C35C`, the **desired order** `0x14C360` (pointer array), its
companion `0x14C3A0` bytes and `0x14C3B0` words, and the **id**
`[0x14C1E4+team]`. **It never writes `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`** —
it never touches the `0x158xxx` entity records at all (fresh `search_instructions`
for `0x1588a4` shows no site in `FUN_00011620`).

---

## 4. `FUN_00012010` byte-level (fresh `disassemble_function 0x12010`, 176 instrs)

`FUN_00012010(EBP=team)` — the **line-up sort**, the consumer of `FUN_00011620`'s
`0x14C360` order.

```
0x12024..0x1202F  save 0x10010 template; CALL 0x11e48(-1,-1)  ; clear the 0x143000 used-array
0x12046..0x1205A  loop: read 16 bytes  byte[0x14C39D + team*0x7B + k] -> local[k*4]   (the position codes)
0x1208E..0x120C4  for i = 0..0xF:
                    EAX = FUN_00011f0c(team, ...)      ; pick the slot of min header key
                    if EAX == -1: EAX = FUN_00011e48(local[id], team)   ; else by position code
                    slot = 0x1430B2 + team*0x250 + EAX*0x25
                    [ECX + 0x14c35c] = slot           ; rewrite 0x14C360? (0x120B4)
                    [ESI + 0x14c39f] = local[EAX*4]   ; write the 0x14C39D/0x14C3A0 byte
0x120FF..0x12129  for k = 0xB..0xF: if slot[k+0x9] == 'G' (0x47) -> mark 0x143000[FUN_00011f6c(k)] = 1
0x12131..0x12144  mark 0x143000[i] for every nonzero local byte
0x1215F..0x12218  for pass = 0..1: for i: if the goalkeeper/outfield gate holds,
                    FUN_00011e48 -> FUN_00011fb4 -> swap [ECX+0x14C360] and
                    [EAX*4 + 0x14C360], and swap the 0x14C3A0 bytes   (0x121CD/0x121D7 the two 0x14C360 stores)
```

`FUN_00011e48(gkflag, team)` (fresh `decompile`) selects the lowest
`slot[+0xC]` among the slots whose `slot[+9] == gkflag` (the `0x1430BB`/
`0x1430BE` reads) and not yet marked; `param_1 == -1` clears the mark array.
`FUN_00011f6c`/`FUN_00011fb4` return the slot index whose `[slot+0]` equals the
`0x14C360[i]`-pointed record's first dword. So `FUN_00012010` **permutes the
`0x14C360` pointer array and its `0x14C3A0` byte companion so the goalkeeper/lines
are placed** — a pure permutation of the desired order (it does not change the
set, only the order). Fresh `search_instructions operand 0x14c360` = **58** and
`operand 0x14c1e4` xref set = **10** (below) confirm the writer/reader split.

---

## 5. The OL-13 metadata — the real producer (the brief's `[rec+0xC]/+0x10/+0x14`)

The brief asks what the setup writes to `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`
(the OL-13 source, FU192-L1). **Fresh answer: not `FUN_00011620`.** The pointer
arrays are written at match begin by `FUN_0006D920`, keyed by the formation id.

### 5.1 The id → record chain (fresh)

```
FUN_0007412C (fresh) :
   FUN_0008ce78( 0x1588A4 + [0x157ABE]*0x835 , &DAT_0014c35c , (ushort)[0x14C1E4] )
   FUN_0008ce78( 0x1588A4 + [0x157ABF]*0x835 , &DAT_0014c3d7 , (ushort)[0x14C1E5] )
```

`FUN_0007412C` reads the two teams' ids `[0x14C1E4]`/`[0x14C1E5]` (the cells
`FUN_00011620` wrote) and applies each to its team record base via `FUN_0008CE78`
(fresh disasm `0x8CE78`): `MOV [EAX+0x7aa],EDX; MOVSX EDX,BX; CALL 0x6d9c4` —
`FUN_0006D9C4(team, id, 0)`.

`FUN_0006D9C4(team, id)` (fresh `decompile`) sets `[team+0x7AE] = 0x11033A +
id*0x1D` (the layout row), and for each of the 4 layout blocks walks its slot
list, writes `[rec+0x24] = role`, and calls `FUN_0006D920(rec)`.

`FUN_0006D920(rec)` — fresh `disassemble_function` (51 instrs), **byte-level**
(decompiler mis-modelled the loader return; the disasm is authoritative):

```
0x6D926 EAX=[rec] ; 0x6D928 EAX=[team+0x7AE] ; 0x6D92E MOVSX BX,byte[EAX]   ; id = layout[0]
0x6D932 IMUL EBX,EBX,6                                                     ; id*6
0x6D93B MOVSX CX,byte[rec+0x8D]                                            ; rec index 0..10
0x6D943 SAR ESI,0x18 ; SHL ESI,2                                           ; (rec[0x8A]>>24)*4
0x6D94B AX=BX ; 0x6D951 CALL 0x4C384 ; 0x6D956 ADD EAX,ESI ; 0x6D95E [rec+0x8]=EAX
0x6D958 IMUL ESI,ECX,0x256
0x6D963 EAX=id*6+1 ; 0x6D969 CALL 0x4C384 ; 0x6D96E ADD EAX,ESI ; 0x6D970 [rec+0xc]=EAX
0x6D975 LEA ESI,[ECX*8] ; 0x6D981 IMUL ECX,ECX,0x958
0x6D97C EAX=id*6+2 ; 0x6D98C CALL 0x4C384 ; 0x6D991 ADD EAX,ESI ; 0x6D996 [rec+0x10]=EAX
0x6D993 EBX+=4 ; 0x6D99B AX=BX ; 0x6D99E CALL 0x4C384 ; 0x6D9A3 ADD EAX,ECX ; 0x6D9A5 [rec+0x14]=EAX
0x6D9A8 EAX=[0x157A4A]>>24 ... ; 0x6D9BA [rec+0x1c] = 0x110794[byte 0x157A4D]   ; the phase handler
```

So, **definitively**:

| field | source (fresh) |
|---|---|
| `[rec+0x8]`  | `FUN_0004C384(id*6+0) + (rec[0x8A]>>24)*4` |
| `[rec+0xC]`  | `FUN_0004C384(id*6+1) + rec_index*0x256` |
| `[rec+0x10]` | `FUN_0004C384(id*6+2) + rec_index*8` |
| `[rec+0x14]` | `FUN_0004C384(id*6+4) + rec_index*0x958` |
| `[rec+0x1C]` | `0x110794[byte[0x157A4D]]` (the phase table) |

`FUN_0004C384(n)` (fresh `decompile`) = `FUN_0004AFB8(n)` =
`(&DAT_0014BFC0)[n]` — the **loaded-resource-pointer cache**. `FUN_0004AFB8`
reads `0x14BFC0 + n*4`; `FUN_0004A6BC` fills it with `sprintf("%s.fmt", table[n])`
over the `0x107370` name table (*cross-cited* FU-148 §3/§4, FU-144 §1). The four
slots `id*6+{0,1,2,4}` are the formation id's **`.fmt` family**
(`read_memory 0x107370`/`0x101A30`, §1: id 0 → `352ko`, `352pl`, `352pk`,
`352ps`, `freekick`, `352sp`, so `{+0,+1,+2,+4}` = `352ko.fmt`, `352pl.fmt`,
`352pk.fmt`, `freekick.fmt`).

### 5.2 Verdict — the brief's premise, corrected

> **Erratum (to the R47 brief premise / FU192-L1).** `FUN_00011620` does **not**
> write `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`. The OL-13 metadata pointers are
> produced by **`FUN_0006D920`**, entered at match begin
> `FUN_0007412C → FUN_0008CE78 → FUN_0006D9C4 → FUN_0006D920`, keyed by the
> **formation id** (`FUN_0004C384(id*6 + {0,1,2,4})` = the `352ko.fmt` family).
> `FUN_00011620` is the **id producer** (`[0x14C1E4]`/`[0x14C1E5]`) and the
> desired-order producer (`0x14C360`) — the root of the chain, one hop upstream
> of the metadata. FU192-L1's "the source is the OL-13 loader metadata" is now
> pinned: **`FUN_0006D920` + the `FUN_0004C384` `.fmt` family**, and the id it is
> keyed by is written by the setup `FUN_00011620`/`FUN_0008EA70`.

Fresh `get_xrefs_to`: `FUN_0006D9C4` = 2 (`FUN_0008CE78`, `FUN_0008D098`);
`FUN_0006D920` = 4 (`FUN_0007997C`, `FUN_0006D9C4`, `FUN_0008D098`,
`FUN_0008CF60`); `FUN_0004C384` = 6 (all `FUN_0006D920`); `FUN_0007412C` = 6
(`FUN_0004b02c`, `FUN_0007417c`, `FUN_00041494`, `FUN_0008ea70`, `0x7420D`,
`0x742C0`). The id setter `FUN_0008EA70(side, id)` (fresh `decompile`) is exactly
`(&DAT_0014C1E4)[side] = id; FUN_0007412C();` — the engine's
`fifa96_match_run_set_formation` mirror (`fifa96_match_run.c:2360`).

---

## 6. The `0x14C360` / `0x157BD2` matrices (FU197-L4)

**Producers of the desired order `0x14C360` (fresh `search_instructions`, 58
sites).** The seed producers are `FUN_00011620` (`0x117F0`) and its twin
`FUN_00011978` (`0x11A1E`, which copies an already-decoded in-memory team record
`param_2[0x32..0x7B]` into the same outputs); the **sort** `FUN_00012010`
(`0x121CD`/`0x121D7`); the in-match order routines `FUN_0008C974`,
`FUN_0008CBB0`, `FUN_0008EAB8`, `FUN_0009343C`, `FUN_00041494`; the `0x82xxx`
swap; and act-4 stage 0 `0x896D7`. (This re-confirms FU-197 §5.4 — no
placement-phase-handler writer.)

**The reset copy (`FUN_00073EE0`, fresh disasm `0x73FC4..0x74018`).** For side
`[ESP]` 0..1 and record `ESI` 0..10:

```
0x73FD0 MOV CL,[EBX + EAX + 0x14c3a0]  ; desired byte -> [EDI + EAX + 0x157b90]
0x73FF0 MOV ECX,[EBP + 0x14c360]       ; desired ptr  -> [EDI + 0x157bd2]
0x74000 MOV AX,[EBX + EAX + 0x14c3b0]  ; desired word -> [EDX + 0x157ba6]
```

and `0x73F76 [0x157B8E]=0`, `0x73FB8 [0x157B8F]=0` (the mismatch counters). So
after a reset **current order == desired order** and the `FUN_0008C974` residual
is 0 (§ of FU-197 §4). `FUN_00073EE0`'s callers (fresh) = `FUN_00074034`
(`0x7407F`, the match begin), `FUN_0004B020` (`0x4B025`), `0x7428E`.

**The other `0x157BD2` writers (fresh `search_instructions`, 30 sites):** the
reset copy (above), the row-0x17 stage-2 sync `0x84892`, and the in-match order
routines `FUN_0008C974`, `FUN_0008CBB0`, `FUN_0008EAB8`, `FUN_0009343C`. No
writer inside the `0x6Dxxx`/`0x6Exxx` placement family.

---

## 7. Where the outputs land in the engine (the T1 conversion rec)

The engine already carries every output cell — all currently dormant:

* **The id.** `mr->formation[side]` (set by `fifa96_match_run_set_formation`,
  `fifa96_match_run.c:2367`, the `FUN_0008EA70` mirror) **is** the native
  `[0x14C1E4+side]`, and already selects the `.fmt` via
  `fifa96_match_formation_fmt_name`. **No new field.**
* **The desired/current order.** `mr->formation_order[2][11]` /
  `mr->formation_current[2][11]` / `mr->formation_mismatch[2]` /
  `formation_order_ok` exist (`fifa96_match_run.h:807..810`); the sorter mirror
  `fifa96_match_run_formation_sort` (`:2877`) and the arm (`:2961`) exist.
* **The metadata.** `rec->phase_meta` (`[rec+0xC]`), `rec->phase_meta10`
  (`[rec+0x10]`), `rec->phase_meta14` (`[rec+0x14]`) exist
  (`fifa96_match_entities.h:179..187`), live-NULL.

**T1 rec (the keystone), concrete:**

1. **The setup site — extend the run setup, mirror `FUN_00011620`.** In the
   team/formation selection path (equivalently `fifa96_match_run_set_formation`),
   after setting `mr->formation[side] = id`, build the **desired order**:
   `formation_order[side][i] = <record id for formation `id`, slot i>` from
   `fifa96_match_formation_layout(id)` (the `0x11033A` table is already ported),
   and set `formation_order_ok = 1`. The reset (`fifa96_match_run.c:2069/2194`)
   then fills `formation_current = formation_order` (the `FUN_00073EE0`
   desired→current copy, §6). **This does not change the chosen `.fmt`** (the id
   already drives `match_run_formation_seed`), so it is additive.
2. **The metadata producer — mirror `FUN_0006D920` (the OL-13 unblock).** Land a
   `match_run_record_meta_bind(mr)` at the match begin (the `FUN_0007412C` site,
   alongside `match_run_formation_seed`): for each team and record, load the
   formation id's `.fmt` family (`FUN_0004C384(id*6 + {0,1,2,4})` over
   `GAMEART0.PVI`) and stage `rec->phase_meta`/`phase_meta10`/`phase_meta14`
   (and the `[rec+8]` formation pointer, if the seed is switched from the
   by-value `fifa96_scene_formation_place` to the pointer model). **Record index
   and strides from §5.1.**
3. **The matrix fills.** `formation_current` from the reset copy (equal to
   desired); `formation_mismatch[side] = 0` at reset. Only an **in-match order
   routine** (a substitution, FU197-L2) perturbs `formation_current` and makes
   the arm fire.

**Row-0x17 verdict:** the keystone alone does **not** unblock row 0x17. Wiring
(1) makes `formation_order == formation_current` → the `FUN_0008C974` residual
stays 0 → the arm stays dormant. Row 0x17 needs an in-match `0x157BD2`
perturbation (FU197-L2), which the M2 tape never exercises. **Row 0x17 stays
NULL/uncounted.** The keystone's real unblock is the **OL-13 metadata** (§8).

---

## 8. The per-write tape-dormancy argument

The M2 tape's visited per-record phases are `{0x00, 0x02, 0x04, 0x13, 0x14,
0x15}` (FU-192 §4, fresh grep). Per writer, with the consuming body on/off the
tape:

| write | consumed by | on the M2 tape? | dormancy |
|---|---|---|---|
| `[0x14C1E4]/[0x14C1E5]` id | `FUN_0007412C` → the `.fmt` seed | the seed is already modelled (`match_run_formation_seed`, same id source) | **Dormant — already-modelled**; a faithful id write is a no-op |
| `0x14C360` desired order | `FUN_0008C974` residual (the arm) | arm dormant (current==desired) | **Dormant** iff `formation_current` mirrors it (the reset does) |
| `0x157BD2` current order | `FUN_0008C974`, phase 0x1A loop | arm dormant | **Dormant** (equal to desired after reset) |
| `[rec+8]` (`id*6+0`, `.fmt`) | `0x6E1D0` (phases **01/12**), `0x6D920` | **01/12 OFF tape** (FU-192 §4) | **Dormant** |
| `[rec+0xC]` (`id*6+1`) | **`0x6DCC8` (phases 0x02/0x15)** | **ON tape** (frames 46-65/773-792/370-469) | **NOT dormant** |
| `[rec+0x10]` (`id*6+2`) | `0x6DD9C` (phase **06**) | OFF tape | **Dormant** |
| `[rec+0x14]` (`id*6+4`) | **`0x6DE44` (phases 0x03/0x04/0x07)** | **row 04 ON tape** (303 frames) | **NOT dormant** |

So: **`FUN_00011620`'s own outputs (id + desired order) are all tape-dormant** —
which is exactly why wiring them is census-neutral and safe. The **OL-13 metadata
writes are tape-VISIBLE** (`[rec+0xC]`/`[rec+0x14]` feed the on-tape `0x6DCC8`
and `0x6DE44`), so landing the metadata producer (§7.2) **moves M2** and must
carry a documented re-pin; it also **un-gates phase rows 0x02/0x15** (the
`0x6DCC8` body, T1-owned per FU192 R1) — the real census mover behind the
keystone, but a tape-visible one.

---

## 9. Numbered legs

1. **FU199-L1 — the `FUN_0004C384`/`FUN_0004A6BC` `.fmt` family names.** The four
   slots `id*6+{0,1,2,4}` are cited via the `0x107370` table + `0x101A30`
   strings; the exact mapping of slot `+4` (the `freekick`/`352sp` order) to the
   `[rec+0x14]` (rows 03/04/07) semantics is not verified against the loaded
   resource content (no ISO read this wave).
2. **FU199-L2 — `FUN_0008CE78` / `FUN_0008D098`'s id source.** `FUN_0008D098`
   passes `byte[[team+0x7AE]]` (the layout row's own id) to `FUN_0006D9C4`;
   `FUN_0007412C` passes `[0x14C1E4]`. The two agree only because `[team+0x7AE]`
   was already set from `[0x14C1E4]`; the ordering of the `FUN_0007412C` /
   `FUN_0008D098` calls within a frame is not pinned here.
3. **FU199-L3 — the second record decode.** `FUN_00011620`'s `count < 0x10`
   branch (`0x1172E`) re-decodes a second `PCCD.DB` record and takes its count
   (the "bench"/reserve row?). Its identity and the `mode = 0` reset are derived
   but not content-verified.
4. **FU199-L4 — `FUN_00012010`'s `0x14C39D`/`0x14C39F`/`0x14C3A0` byte
   companions.** The three overlapping per-team byte arrays (offsets
   `+0x3D`/`+0x3F`/`+0x40` of the `0x7B` block) are written as a permutation but
   their field names (position code vs. a saved copy) are only sketched.
5. **FU199-L5 — `FUN_00011978`'s in-memory source.** The twin reads
   `param_2[0x32..0x7B]` (a decoded team record) and reproduces the same outputs;
   the producer of that in-memory record (a prior `FUN_00011620`? a cache?) is
   not traced.
6. **FU199-L6 — `FUN_00041494` (the in-match line-up routine).** A `0x14C360`
   writer and `FUN_0007412C` caller (fresh xrefs 4: `FUN_00038004`,
   `FUN_00038630`); its role (substitution/formation change) is the FU197-L2
   order-routine leg and is not decomposed here.
7. **FU199-L7 — the OL-13 resource *content*.** The `[rec+8/+0xC/+0x10/+0x14]`
   strides (`×4`, `×0x256`, `×8`, `×0x958`) are byte-verified; the resource
   layout (player-record fields) inside each `.fmt` sibling is not read (the
   unported loader `FUN_0004C384`/`FUN_0004A6BC` content model).

---

## 10. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` (FIFA96.EXE, watcom LE, image base 0, 2706 funcs).
* `decompile_function` — `0x11620`, `0x12010`, `0x101c8`, `0x109a0`, `0x115cc`,
  `0x103d0`, `0x11b7c`, `0x11e48`, `0x11f0c`, `0x11f6c`, `0x11fb4`, `0x11978`,
  `0x6d9c4`, `0x24b00`, `0x8c2e0`, `0x6d920`, `0x4c384`, `0x8e810`, `0x7412c`,
  `0x74034`, `0x4b020`, `0x10020`, `0x8ea70`, `0x4afb8`.
* `disassemble_function` — `0x11620` (189), `0x12010` (176), `0x6d920` (51).
* `disassemble_bytes` — `0x8ce60` (60 B), `0x8d090` (140 B), `0x73ee0` (180 B),
  `0x73f90` (160 B).
* `get_function_by_address` — `0x109a0`, `0x101c8`, `0x115cc`, `0x11620`
  (`0x11620..0x11976`), `0x12010` (`0x12010..0x12227`).
* `get_xrefs_to` — `0x11620` (5), `0x12010` (12), `0x11b7c` (5), `0x14c1e4`
  (10), `0x8ce78` (2), `0x41494` (4), `0x73ee0` (3), `0x6d9c4` (2), `0x6d920`
  (4), `0x4c384` (6), `0x8c2e0` (2), `0x7412c` (6), `0x74034` (2), `0x8e810`
  (1), `0x104f24` (40), `0x14c360` (58), `0x1496e0` (9), `0x143018` (8),
  `0x14bfc0` (5).
* `search_instructions` — operand `0x1588a4` (50), `0x1430b2` (7), `0x157bd2`
  (30).
* `read_memory` — `0x14c1e0` (32 B), `0x11033a` (93 B), `0x100000` (32 B),
  `0x101a30` (96 B), `0x107370` (48 B).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(1860..1930, 2040..2260, 2350..2380, 2860..2906, 2990..3010, 5380..5400,
5690..5730), `src/fifa96_engine/fifa96_match_entities.c` (330..400),
`include/fifa96_engine/fifa96_match_entities.h` (60..190),
`include/fifa96_engine/fifa96_match_run.h` (790..812, 1290..1300, 1450..1470),
`src/fifa96_engine/fifa96_match_handlers.c` (4576..4594, 4660..4735, 4972,
5059..5125), `src/fifa96_loader/fifa96_referee.h` (36/58),
`tests/golden/engine/m2-frames.txt` (`state=` census).

Docs cross-cited: FU-83, FU-89 §11, FU-130, FU-137 §3, FU-144 §1, FU-148 §3/§4,
FU-175, FU-177 §2/§3/§7, FU-181, FU-183, FU-190, FU-191 §6, FU-192 §1/§3/§4,
FU-193..FU-198. AGENTS.md census rule. The alias traps
(`[0x157A49]>>24` = `[0x157A4C]`; `[0x14C1E4]` = `(&0x14302A)[team*0x49]`) are
applied. No writes beyond this slice file and the workspace report.

---

## 11. Status (M2 phase-18 T1, 2026-10-12) — landed

The §7.2 rec is landed. **The engine metadata representation** is
`fifa96_match_entities_bind_record_meta` (`include/fifa96_engine/
fifa96_match_entities.h` + `.c`): per record of a team it stages
`phase_meta`/`phase_meta10`/`phase_meta14` (`[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`)
at `idx = (int8_t)byte[rec+0x8D]` (the pool `active`) with the native strides
`idx*0x256` bytes / `idx*2` dwords (native 8 bytes) / `idx*0x256` dwords (native
`0x958` bytes); a NULL blob clears the pointer (the native gate). **The loader**
is `match_run_record_meta_bind` (`src/fifa96_engine/fifa96_match_run.c`): it
decodes the GAMEART0 bank (`match_stage_container` + `fifa96_bigf_parse`), finds
the formation id's `.fmt` family by name (`match_formation_family`: for id 0
`352pl.fmt`/`352pk.fmt`/`freekick.fmt`; fresh `read_memory 0x107370`/`0x101A30`
this wave — every 6-name group is {ko,pl,pk,ps,freekick,sp}, slot 4 the shared
`0x101A50` "freekick"), copies the blobs into `mr->meta_owner` (the pool-handled
owner) and calls the bind. It soft-fails (pointers stay NULL) with no assets/ISO
or a missing family entry — the pre-T1 gated no-op.

**Placement (erratum to §7.2).** The bind is the **last** begin step, **after**
the derived front-end screen install (`fifa96_match_run_screen_install`). The
native `FUN_0007412C → FUN_0008CE78 → FUN_0006D9C4 → FUN_0006D920` bind belongs
to the match begin, while the engine models the front-end match-screen install
(`FUN_00092D8C`, which writes phase 0x15, FU-170 §7) as a begin step. Binding
after it keeps that derived front-end walk metadata-free (the native front-end
precedes the bind). §7.2's "alongside `match_run_formation_seed`" placement
(before the derived screen install) would let that derived phase-0x15 write run
the walk with the metadata bound — a derived-ordering artifact, not a native
effect (measured: it moved the whole M2 transcript from frame 12; the corrected
placement is byte-identical).

**Tape (§8).** The landed bind is **M2 byte-identical** (`7b4b241a…`, 792 lines,
`cmp` clean); **no re-pin**. The on-tape rows DO run — the `0x6DCC8` body was
measured firing at frames 209/766 (phase 2) and 363 (phase 0x15) — yet the
presented frames are unchanged, so §8's "NOT dormant" concern is resolved by
measurement: those target writes are render-invisible on this tape. M1
`09b726b7…` unmoved. **Census: `48/80 = 60%`** — the eight rows
(`0x02`/`0x03`/`0x04`/`0x06`/`0x07`/`0x08`/`0x09`/`0x15`) were already
wired/counted; T1 makes them perform their native work (fidelity, not a count).
**Legs:** FU199-L1..L7 stand; the residual half is FU200-L3 (the
`0x1577DE`/`0x157754`/`0x1577D6/D8` index selectors, staged 0).

Provenance (this status): engine source read first-hand
(`fifa96_match_entities.{c,h}`, `fifa96_match_run.{c,h}`, `fifa96_scene.{c,h}`);
the blob bytes from the first-hand GAMEART0.PVI decode; the Ghidra MCP
`read_memory 0x107370`/`0x101A30` above. No `/FIFA96.EXE` writes.
