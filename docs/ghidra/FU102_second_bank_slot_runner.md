# FU-102 — the second camera bank and the per-slot runner

Follow-on to FU-101 §8 leg 3 (the `0x7968` record array and the `[0x7DC8]`
duality), FU-100 §8 leg 4 (the duplicate descriptors 3/4/5) and FU-97 §1 (the
`FUN_0004D7E8` loops). This slice reads the second camera bank in full, finds
the duplicate types inside it, and maps the second, **slot-keyed** handler
runner `FUN_0004D908`.

Result in one line: **the 0x7508 camera table has a second ten-record bank at
`0x7968` (same `0x70` layout, ending exactly at `[0x7DC8]`), whose records
0/1/2 carry the duplicate types 3/4/5 and whose `+0x08` word is the record's
slot index; `FUN_0004D908` switches on that word through the nine-entry jump
table at `0x4D8E4` and calls the same class handlers
(`0x4E834`/`0x4E3A8`/`0x4EC9C`/`0x4DB38`) plus two new arms
(`0x4D98C` for slot 4, `0x4DDA8` for slot 8), with `EDX` = the type
descriptor; `FUN_0004D7E8` initialises bank B (dword 0 = index, six fields
zeroed) before zeroing the six descriptors.**

## 1. The two banks

`[0x7DC8]` addresses records in one of two identical `0x70`-stride ten-record
banks:

| bank | base | records | static types | `[0x7DC8]` set by |
|------|------|---------|--------------|-------------------|
| A (camera objects) | `0x7508` | 0..9 (`0x7508..0x7968`) | 0/1/2 for slots 0/1/2, -1 for 3..9 (FU-96/FU-97) | `FUN_0004D498`, `FUN_0004CEF4`, `FUN_0004CF7C` |
| B (records) | `0x7968` | 0..9 (`0x7968..0x7DC8`) | **3/4/5 for records 0/1/2**, -1 for 3..9 | `FUN_0004D04C` |

`read_memory 0x107968` (1120 B, the full bank) shows every bank-B record with
`class = -1` (dword 1) and `dword 2 = its index`; records 0/1/2 carry types
3/4/5 and the same geometry as camera 0 — `x = -0x780`, `y = 0xA0`,
`z = 0x1E`, angle 21, yaw `0xBF9C`, pitch 0xC8:

| rec | dword0 | dword1 | dword2 | dword3 |
|-----|--------|--------|--------|--------|
| 0 | 0 | -1 | 0 | **3** |
| 1 | 1 | -1 | 1 | **4** |
| 2 | 2 | -1 | 2 | **5** |
| 3..9 | i | -1 | i | -1 |

This closes FU-100 §8 leg 4: **types 3/4/5 are the bank-B record types**.
Types 3 ≡ 0 and 4 ≡ 1 byte-identically (FU-100 §5); type 5 differs from type 2
only in dword 3 (`0xA21C` vs `0xA7F8`). Bank A samples of dword 2
(`[camera+8]`): slots 0/1/2/5/7 = 0/1/2/5/7 — the **slot index** in both banks.

`get_xrefs_to 0x7968` yields only four references, all in `FUN_0004D7E8`
(loop bound `0x4D805`, data `0x4D860`, write `0x4D86C`) and `FUN_0004D04C`
(`0x4D0F5`); bank B is otherwise reachable only through `[0x7DC8]`.

## 2. `FUN_0004D7E8`'s three loops (correction to FU-97 §1)

The decompiled body is three loops, not two:

1. bank A (`puVar2 = &0x7508` while `!= &0x7968`): apply the preset entry
   (`[type+0x48] + preset*0x18`, FU-99) to each enabled camera and zero
   `+0x34/+0x38/+0x3C/+0x40/+0x44/+0x48`;
2. bank B (`piVar1 = &0x7968`, 10 iterations): `*piVar1 = i` (dword 0 =
   index) and zero `+0x34/+0x38/+0x3C/+0x40/+0x44/+0x48`;
3. the six type descriptors (`(&0x8B64)[i]` for `i = 0, 4, ..., 0x14`):
   zero `+0x34/+0x38/+0x10/+0x14/+0x18/+0x1C`.

FU-97 §1's "second loop initialises entry `i` and zeroes … of each type
descriptor" conflated loops 2 and 3; the record half is bank B. The tail
`[0x7784] = [0x7BE4] = 4000` stands.

## 3. `FUN_0004D908` — the slot-keyed runner

```
0x4d90b MOV EDX,[EAX + 0x8]                  ; slot word
0x4d90e CMP EDX,0x8
0x4d911 JA 0x4d987                           ; RET
0x4d913 JMP dword ptr CS:[EDX*4 + 0x3d8e4]
```

The table at code-object `0x3D8E4` (storage image `0x4D8E4`,
`read_memory` 44 B → `32d90300 1bd90300 49d90300 87d90300 60d90300 69d90300
87d90300 87d90300 7cd90300`) maps the slot (+`0x10000`) to:

| slot | target | action (`disassemble_bytes`) |
|------|--------|------------------------------|
| 0 | 0x4D932 | `EDX=[cam+0xC]; ECX=EBX=0; EDX=[EDX*4+0x8B64]; CALL 0x4E834` |
| 1 | 0x4D91B | same, `CALL 0x4E3A8` |
| 2 | 0x4D949 | same, `CALL 0x4EC9C` |
| 3 | 0x4D987 | `POP EDX/ECX/EBX; RET` (no-op) |
| 4 | 0x4D960 | `CALL 0x4D98C` |
| 5 | 0x4D969 | `ECX=[0x8B7C]; EBX=EDX=0; CALL 0x4DB38` |
| 6 | 0x4D987 | no-op |
| 7 | 0x4D987 | no-op |
| 8 | 0x4D97C | `EDX=[0x7DF0]; CALL 0x4DDA8` |
| >8 | 0x4D987 | no-op |

So the runner is the **slot**-keyed twin of `FUN_000505D0`'s class-keyed table
(FU-100 §2): slots 0/1/2/5 call the same four handlers, and slots 3/6/7 are
inert; slots 4 and 8 call two functions not yet mapped — `0x4D98C` (prologue
`PUSH EBX/ECX/EDX/ESI/EDI/EBP; EBP=EAX; EDI=[EAX+0x10]; ESI=[EAX+0x34]`, i.e.
a `+0x10 ← +0x34` block restore) and `0x4DDA8` (`EDX = [0x7DF0]`).

The handlers receive `EDX` = the **type descriptor** (`[[cam+0xC]*4+0x8B64]`),
`ECX = EBX = 0` — so `FUN_0004e834`'s `param_2[2]`/`param_2[0x14]` read
descriptor dword 2 (the behavior selector: 1 for types 1/4, 3 for 0/2/3/5)
and `desc+0x50` (the C-table record written by `FUN_0004F1C8`, FU-101 §2).
This refines FU-100 §3's "tracked entity from `param_2[0x14]`": `param_2` is
the descriptor and `param_2[0x14]` is its C record pointer.

Callers: `FUN_0004D04C` runs the runner **20×** after a view change (FU-101
§1's settle loop, `0x4D0C6..0x4D0DB`); `FUN_0004D2D4` runs it per frame on the
`(unsigned)cam[+0x04] >= 4` branch (`CALL 0x4D3E5`), i.e. for the disabled
(class `-1`) records.

## 4. Closures / errata

* **FU-100 §8 leg 4 — closed.** Types 3/4/5 are bank B's records 0/1/2; no
  runtime "type assignment path" is needed — the duplicate descriptors are
  the second bank's type set.
* **FU-101 §8 leg 3 — closed structurally.** The `0x7968` array is a full
  second bank of ten `0x70` records (bank B); `FUN_0004D04C` selects it into
  `[0x7DC8]`, the other drivers select bank A. Its semantic role (replay/TV?)
  stays open.
* **FU-97 §1 loop wording — corrected** (§2 above).
* **FU-100 §8 leg 7 (class-range hazard) — narrowed.** The four-entry class
  table (FU-100 §2) is complemented by this nine-entry slot table; the slot
  word is bounded by `JA` and the static values are 0..9 (slot 9 → no-op), so
  the class table is not the only dispatch.
* **FU-100 §3 refinement** — handler `param_2` = type descriptor (runner call
  proves it); `param_2[0x14]` = `desc+0x50` = the C record.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x4D7E8, 0x4D908,
0x4D498; `disassemble_bytes` 0x4D908 (64 B, head), 0x4D949 (80 B, cases
2/4/5/8/default); `read_memory` 0x107968 (1120 B, full bank B), 0x4D8E4
(44 B, jump table), 0x4D932 (24 B, reconciled case bytes), 0x107580/0x1075F0/
0x107740/0x107820 (bank-A slot words); `get_xrefs_to` 0x7968 (4). Address
mapping as FU-88 (data immediate `A` → storage `A+0x100000`; stored code
pointers and inline `CS:` tables → `+0x10000`). Analysis-only: no port,
capture-rig, ISO, or Ghidra-project change.

## 6. Open legs

1. **`0x4D98C`** (slot-4 restore `+0x10 ← +0x34`) and **`0x4DDA8`**
   (slot-8, `EDX = [0x7DF0]`) bodies.
2. **When the two dispatchers are used**: `FUN_000505D0`'s class table (keyed
   `[cam+0x04]`) vs `FUN_0004D908`'s slot table (keyed `[cam+0x08]`); the two
   disagree for slot 7 (class 1 vs slot no-op).
3. **Bank B's role** (replay/TV/transition targets?) and who reads the bank-B
   records besides `[0x7DC8]` consumers.
4. **`[0x7DF0]`** (slot-8 arm) and `[0x8B7C]` = 500 (slot-5 arm; FU-100 §2's
   700/800 uses).
5. **Descriptor dword 2** as a handler behavior selector (1/3, and the
   decompiled `== 4` branches) — which runtime path writes 4.
6. **Bank A slot words for slots 3/4/6/8/9** were not read (samples only);
   the static `dword2 = slot` rule may have exceptions there.
7. **`FUN_0004F1C8` + bank B**: it writes the descriptor of `[view+0xC]` when
   `[0x7DC8]` is a bank-B record (`type 3/4/5`), so `desc[0]`/`desc+0x50`
   mutation applies to the duplicate descriptors too (FU-101 §2) — the
   variant tables for types 3/4/5 are the same slot-keyed ones.
