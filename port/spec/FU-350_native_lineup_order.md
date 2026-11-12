# FU-350 — native line-up order and appearance mapping

Date: 2026-11-09. Scope: the match-start pitch-record -> roster-slot map
(`0x14C360` / `FUN_00012010`) and the skin/hair mapping that reads it. The
full `FUN_00047298` 23-descriptor pool and the in-match order swaps remain
open legs.

## 1. Root cause

The port built `sub_lineup[side][pitch]` with a derived role/rating pick:
walk the `0x11033A` formation blocks and choose the highest-rated unused
player matching each block's position (`fifa96_match_run_sub_rebuild`). The
native match does not rate-sort the starting 11. It uses the authored 16-slot
desired order at `0x14C360`: the first 11 entries are the starting line-up,
bound to pitch records by `FUN_0008C758` as

```
rec+4 = 0x157BD2[side][byte[rec+0x8D]]
```

where `rec+0x8D` is the pitch-record index 0..10. The appearance readers
`FUN_0004C00C`/`FUN_0004C098` then take the hair/skin digits from that same
order entry, so the wrong order shows the right player's face at the wrong
pitch position. Brazil/Germany make this visible: the derived German order
puts Buchwald on pitch 2 and Schulz on pitch 4; the native order is the
reverse, so the visible skin class at those two pitch records changes.

## 2. Native evidence

### 2.1 The 0x25-byte slot decoder (`FUN_000109A0` -> `FUN_000103D0`)

`FUN_00011620` calls `FUN_000109A0(mode, idx, i, slot)` for each roster row;
`FUN_000109A0` resolves the PCCD player record and calls
`FUN_000103D0(record, slot)`. `FUN_000103D0` is an 11-field dispatcher:
descriptor flags at `0x104ECC` (8-byte entries), handlers at `0x103A4`.
Disassembly (fresh `disassemble_function 0x103D0`) gives the source -> slot
mapping:

| PCCD source | size | slot destination | handler |
|---|---:|---|---|
| `+0x00` id | 4 | `+0x00` | `0x10481` |
| `+0x04` byte | 1 | `+0x08` | `0x1048C` |
| `+0x05` name offset | 4 (indirect) | `+0x04` pointer | `0x10493` |
| `+0x09` position | 1 | `+0x09` | `0x10498` |
| `+0x0A` byte | 1 | `+0x0A` | `0x1049F` |
| `+0x0B..+0x1B` block | 17 | `+0x0C..+0x1C` | `0x104A6` |
| `+0x1C` hair | 1 | `+0x1D` | `0x104B3` |
| `+0x1D` skin | 1 | `+0x1E` | `0x104BA` |
| `+0x1E` byte | 1 | `+0x1F` | `0x104C1` |
| `+0x1F..+0x22` block | 4 | `+0x20..+0x23` | `0x104C8` |
| `+0x23` byte | 1 | `+0x0B` | `0x104D5` |

PCCD player stride is `0x24`; the slot is `0x25` bytes. The two consequences
that matter here:

- native `slot+9` = PCCD `+0x09` (the position), not a shifted byte;
- native `slot+0xC` = PCCD `+0x0B`, the second byte of the parsed skills
  block, **not** the overall rating at `+0x1B`.

The appearance pair is unchanged from the documented relationship:
native `slot+0x1D`/`+0x1E` = PCCD `+0x1C`/`+0x1D`.

### 2.2 The order producer (`FUN_00012010`)

`FUN_00011620` builds the natural slots at
`0x1430B2 + side*0x250 + i*0x25`, points `0x14C360 + side*0x7B + i*4` at
each, and points `0x14C35C + side*0x7B` at the team header
`0x143018 + side*0x49`. The team header is decoded by `FUN_000101C8`; its
16-byte key block is header `+0x35..+0x44`.

`FUN_00011F0C(team)` reads each key as

```
*(int *)(header + 0x32 + slot) >> 24  ==  header[0x35 + slot]
```

and selects the unused slot with the lowest signed key, skipping `-1`
(fresh `decompile_function 0x11F0C`). If no keyed slot remains,
`FUN_00012010` calls `FUN_00011E48(key, team)` with the static fallback
template at `0x10010` (first-hand `read_memory`, 16 bytes):

```
47 44 44 44 44 4D 4D 4D 4D 46 46 FF FF FF FF FF
G  D  D  D  D  M  M  M  M  F  F  -1 -1 -1 -1 -1
```

`FUN_00011E48` selects the unused slot whose `slot+9` equals the key
(`-1` matches any position) with the lowest signed `slot+0xC`; strict `<`
means the first slot wins on a tie. It marks the slot used. `FUN_00011F6C`
and `FUN_00011FB4` are the inverse-permutation helpers used by the later
swap passes.

`FUN_00012010` then marks the bench goalkeepers and every slot with
`0x14C3A0 & 0x7F != 0` as used, and runs two swap passes over the
`0x14C3A0` companion for entries with `(byte & 0x7F) > 1` or
`(byte & 0x80) != 0`. `0x14C3A0` is cleared and populated by
`FUN_00035A1C` from a 0x84-record event table (values 1/2/3, code 4 also
sets `0x80`); this is the competition card/injury replacement pass, not the
default friendly order. With no matching event records the companion is
all zero and the swap passes are no-ops, so the match-start order is the
`FUN_00011F0C`/`FUN_00011E48` primary pass above.

### 2.3 The appearance readers index the order, not the entity

Fresh `decompile_function 0x4C00C`/`0x4C098`:

```
if (DAT_001074B8 == 0) entry = 0x14C360[side][pitch];
else                    entry = 0x157BD2[side][pitch];
hair = *(int *)(entry + 0x1A) >> 24;   // entry +0x1D
skin = *(int *)(entry + 0x1B) >> 24;   // entry +0x1E
```

`DAT_001074B8` is set by `FUN_0004BF04` (0 = desired, 1 = current).
`FUN_00047298` builds the 0x8E-byte remaps in a 0x17-iteration loop; it
reads 11 entries per side plus one extra descriptor. The port's per-pitch
projection covers the 11 visible players; the 23rd descriptor remains an
open leg.

The digit maps match the port exactly:

- `FUN_0004C00C` (hair): `'0'->3, '1'->2, '4'->0, else 1`.
- `FUN_0004C098` (skin): `'0'/'2'->1, '1'->3, '3'->2, '4'->0, else the
  hair class when it is 3, otherwise 0`.

## 3. Retail measurement

First-hand PCCD/PCDB read through the engine asset path. Team header keys are
signed decimal from `read_memory` at record `+0x35`; the order is the native
primary pass.

Brazil (mode-4 index 6, rid 43, record `0x1F87`, formation 1):

```
keys: 0x29 0x14 0x18 0x21 FF FF 0x17 FF FF 0x22 0x2B FF 0x15 0x1F 0x00 0x1E
order: 14 1 12 6 2 15 13 3 9 0 10 | 4 7 11 8 5
starters: Taffarel(G) Branco(D) Santos(D) Jorginho(D) Cafu(D)
          Zinho(M) Silva(M) Dunga(M) Rai(M) Bebeto(F) Romario(F)
```

Germany (mode-4 index 21, rid 56, record `0x267C`, formation 1):

```
keys: 0x18 FF 0x22 0x1F FF 0x00 0x29 0x14 FF 0x17 0x21 0x2B 0x1E 0x15 FF FF
order: 5 7 13 9 0 12 3 10 2 6 11 | 8 4 15 1 14
starters: Illgner(G) Kohler(D) Schulz(D) Matthaus(D) Buchwald(D)
          Sammer(M) Hassler(M) Muller(M) Effenberg(M) Klinsmann(F) Riedle(F)
```

The first 11 are the assertions in `test_engine_pccd` and
`test_engine_team_data`. The full order is also pinned; the bench tail is
the static fallback pass (`-1` = any position, lowest `slot+0xC` first).

## 4. Port implementation

- `fifa96_pccd_team` gains `order_key[16]` from PCCD team `+0x35`;
  `fifa96_pccd_player` gains `sort_key` from PCCD player `+0x0B`.
- `fifa96_pccd_lineup_order` implements the `FUN_00012010` primary pass:
  header-key minimum, then the `GDDDDMMMMFF -1...` fallback by position,
  then the signed `+0x0B` tie-break. All keys are `int8_t`; a plain `char`
  is unsigned on the port's current target and would turn `0xFF` into 255.
- `fifa96_team_data_lineup_order` resolves a LANG id through the 44-team map
  and calls the parser pass.
- The bridge stores the result in `mr->roster_order[side]` and sets
  `roster_order_ok[side]`; `fifa96_match_run_sub_rebuild` validates that it
  is a permutation and uses the first 11 as `sub_lineup`. Synthetic/no-PCCD
  runs keep the previous role/rating fallback, so the deterministic tapes
  and direct-begin fixtures are unchanged.

The pitch-record binding is direct: the port's entity pool sets
`e->active = i` (the `rec+0x8D` analog), so `sub_lineup[pitch] = order[pitch]`
is the `FUN_0008C758` binding.

## 5. Verification

- `make check`: 124/124.
- M1 `cmp` byte-exact, sha `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`.
- M2 `cmp` byte-exact, sha `41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`.
- New tests: Brazil/Germany keys and full orders (`test_engine_pccd`),
  LANG-id handoff (`test_engine_team_data`), the native-vs-fallback branch
  and permutation validation (`test_engine_live_control`), the bridge
  integration (`test_engine_match_bridge`), and the native order ->
  skin-remap composition for Germany pitches 2/4 (`test_engine_match_render`).

## 6. Legs and errata

**Erratum (Phase 86 legs, quoted).** "The native lineup matrix `0x14C360` is
unported (the pitch -> roster map is derived, and a substitution does not
rebuild `remap_entity` yet)" — the **producer** and the pitch -> roster map
are now ported for the default friendly order; Phase 87 already closed the
substitution remap rebuild. The full `FUN_00047298` pass still remains.

**Erratum (Phase 87, quoted).** "The native `0x14C360` desired-order
producer ... remain legs" — the producer is ported; the in-match order swaps
are not.

Open legs:

- **FU350-L1** competition card/injury replacement (`FUN_00035A1C` plus the
  two `FUN_00012010` companion-byte swap passes);
- **FU350-L2** front-end edited-line-up re-sort (the port has no authored
  formation-edit menu; the PCCD default order is used);
- **FU350-L3** the full `FUN_00047298` 23-descriptor pool (the port's
  per-pitch projection covers the 11 visible players per side);
- **FU350-L4** in-match order swaps `FUN_00041494` / `FUN_0008CBB0` /
  `FUN_0008EAB8` / `FUN_0009343C` (substitution/placement order).

**Closure (FU350-L3, 2026-11-11).** FU350-L3 is closed for the pool: the full
`FUN_00047298` 23-descriptor builder is ported (`fifa96_match_remap_build`,
`port/src/fifa96_loader/fifa96_match_remap_native.{h,c}`) and the engine builds
its entity remaps from the 23-block pool, storing the extra block in
`render.remap_extra`. The 23rd block's consumer remains unproved
(`get_xrefs_to 0x14BF9C` = 0) and stays a numbered leg; the `FUN_00048DC0` GK
shade patch, the `FUN_0001CAEC(9/0xD)` option gates and the unreachable
`0x107080[class] > 0x15` branch also stay open (see the FU-348 §7 L2 closure).

## 7. The `0x107080` writer evidence (2026-11-11)

**Erratum (quoted).** §6's "the `FUN_00048DC0` GK shade patch ... stay open"
is corrected by FU-348 §8: the body is the kit-editor preview palette
transform, and the `0x107080` branch closure below carries the writer detail.

**Fresh xrefs.** `get_xrefs_to 0x107080` returns four references: the DATA
reads `0x47450` (`MOV EBX,[EBX+0x107080]`), `0x47586` (`MOV
EBX,[EAX*4+0x107080]`, `EAX` from the slot map at `[ESP+EAX+0x480]`, i.e. the
template index) and `0x47769` (`CMP dword[EAX*4+0x107080],0x16`, `EAX` from
`0x47762 MOV EAX,[ESP+0x4ec]`), all in `FUN_00047298`; the only WRITE is
`0x487D9 MOV [ESI+0x107080],EAX` in `FUN_000481EC`. `get_xrefs_to 0x481EC`
returns one caller, `0x48D57` in `FUN_00048D38`; the fresh `FUN_00048D38`
decompile runs `FUN_000481EC` → `FUN_00048AB0` → `FUN_00047298(1)` →
`FUN_000471EC` → `FUN_00048C8C`/`FUN_00048B60`.

**The writer.** `0x487A0 MOV EBP,0x15` stages the blank-kit selector into
`[ESP+0x34]`/`[ESP+0xc]`; `0x487BB MOV EDX,0x15` stages the away side. The
write-loop head is `0x487C8 XOR ECX,ECX / XOR EDI,EDI / XOR ESI,ESI`, the
store is `0x487D9` from `0x487D5 MOV EAX,[ESP+EDI+0xc]`, and `0x48895 ADD
EDI,0x14` / `0x48899 ADD ESI,0x4` / `0x488A3 CMP ECX,0x4` / `0x488A6 JL
0x487D5` close four dwords. The fresh image read at `0x107080` is the static
five `{0,0,14,15,13}` (all `< 0x16`); `0x106AFD`/`0x106AFE` read 6/5. The
writer's four sources are `[ESP+0xc]`/`[ESP+0x34]` (the `0x105284` gates
6/7/8) and `[ESP+0x20]`/`[ESP+0x48]` (the `0x105298` gates 10/0xB/0xC),
staging `0x16`/`0x17`/`0x15`. The writer runs at live match setup (`0x4AE76`
in `FUN_0004AD4C`, called from `0x49424` in `FUN_000493A0`, the match main
loop) as well as from the front-end/team-editor path (`0x372B4` in
`FUN_0003726C`); `FUN_00048D38` then calls `FUN_00047298(1)`, which reads the
just-written selectors. The static image five is the pre-init image content;
the runtime array is written from the lineup/kit cells at match init. With
the port's staged 0 the selectors stay below `0x16`; with non-default kit
cells the branch is reachable, which is the `FUN_0001CAEC` option-UI leg. The
lineup-order producer remains `FUN_00012010`; `0x107080` is the
template-selector table, not the order matrix. See FU-348 §8 (A4).
