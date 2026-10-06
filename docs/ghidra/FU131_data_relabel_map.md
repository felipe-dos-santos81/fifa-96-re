# FU-131 — the data relabel map (`+0x100000`), applied

Cycle 2, Task 2 (turn the data graph on). FU-130 established that the image's
data references are segment-relative and must be read at `X + 0x100000`, and
that Ghidra's native Watcom LE loader already applies the LE fixups, exposing
the corrected program as `/FIFA96.EXE`. This slice **adopts that native
program as the authoritative corrected image** and annotates the known data
globals at their real addresses.

Result in one line: **the corrected program is `/FIFA96.EXE`
(`watcom:LE:32:default`); at the same code addresses as `/fifa96_le.bin` its
operands carry the `+0x100000` addend, its data xrefs resolve (e.g. `READ`
`0004a6e3 -> 0x107370`), and the 14 target globals now carry plate comments at
their real addresses; the resource-name consumer `FUN_0004a6bc` confirmed the
`0x107370` table and the `GAMEART` / `art/gameart0.pvi` strings at
`0x101C64` / `0x101C6C`.**

## 0. Branch taken

Task 1 returned `REIMPORT_VIABLE: yes`, so the nominal route was Step 2a
(re-import). The controller ruling carried into this task prefers the **native
program already present** (`/FIFA96.EXE`) as the corrected image rather than
importing a duplicate. Step 2a's *verification* is therefore performed against
that program, and the Step 2b relabel list is applied at the **real** addresses
on it.

* Program used: **`FIFA96.EXE`** (Ghidra project path `/FIFA96.EXE`).
* `ghidra_list_open_programs` → `FIFA96.EXE` current, language
  `watcom:LE:32:default`, compiler `watcom`, 2706 functions / 15239 symbols,
  1 overlay space (`.image`), `image_base` `0x0`.
* `ghidra_list_project_files(folder="/")` → `FIFA96.EXE` (Program, v3) and
  `fifa96_le.bin` (Program, v15) both present. No duplicate was imported.
* `/fifa96_le.bin` is left untouched (the pre-correction flat image retained
  as the witness for the offset form).

### Code addresses match (Step 1)

The consumer function is byte-identical in address in both programs:

| program | `get_function_by_address 0x4a6bc` |
|---|---|
| `/FIFA96.EXE` | `FUN_0004a6bc`, body `0x4a6bc..0x4a82c` |
| `/fifa96_le.bin` | `FUN_0004a6bc`, body `0x4a6bc..0x4a82c` |

Native operand forms vs. flat operand forms at identical addresses
(`search_instructions`):

| address | `/FIFA96.EXE` (native, fixups applied) | `/fifa96_le.bin` (flat, unapplied) |
|---|---|---|
| `0x4a6e3` | `MOV EDI, dword ptr [ESI + 0x107370]` | `MOV EDI, dword ptr [ESI + 0x7370]` |
| `0x4a71c` | `MOV EAX, dword ptr [ESI + 0x107370]` | `MOV EAX, dword ptr [ESI + 0x7370]` |
| `0x4a6cd` | `MOV EDX, 0x101c64` | `MOV EDX, 0x1c64` |

`search_instructions(program="FIFA96.EXE", operand_pattern="0x107370")` → 4
hits, all in `FUN_0004a6bc`; `search_instructions(program="fifa96_le.bin",
operand_pattern="0x7370")` → 2 hits in the same function.

### Data xref resolves (Step 2a verification)

```
ghidra_get_bulk_xrefs(program="FIFA96.EXE", addresses="0x107370")
{"0x107370":[{"from":"0004a6e3","type":"READ"}]}
```

A code `READ` xref now resolves. On the flat image there is none (FU-130 §6).

## 1. The map

`code operand` is the literal as it appears in the **un-fixed** image
(`/fifa96_le.bin`); the real address is `+0x100000`. On `/FIFA96.EXE` the same
operand already reads as the real address (e.g. `[0x0010736c]`), i.e. the fixup
is applied in place. `current bytes` are read from `/FIFA96.EXE` via
`ghidra_inspect_memory_content`; `meaning` and `provenance` cite the slice that
established the semantics (see FU-129 §5 erratum) plus the native witness
instruction.

| code operand | real data address | current bytes (native) | meaning | provenance |
|---|---|---|---|---|
| `[0x736C]` | `0x10736C` | `00 00 00 00` (BSS) | primary resource-archive handle, set/cleared by `FUN_0004a6bc` | FU-128 §1,§2; native `MOV [0x0010736c],EAX` @ `0x4a6de`, `MOV EAX,[0x0010736c]` @ `0x4a7da` |
| `[ESI+0x7370]` (dword-indexed) | `0x107370` | `30 1A 10 00  38 1A 10 00  40 1A 10 00 …` (16 dwords → `0x101A30`+) | resource-name offset table | FU-129 §1,§5; FU-128 §1; native `[... ESI + 0x107370]` ×4 @ `0x4a6e3/71c/75d/799` |
| `[0x9090]` | `0x109090` | `00 00 00 00` (runtime `… 01 00 00 00`) | frame counter #1 | FU-116/FU-127; native `CMP [0x00109090],0x3` @ `0x58cae`, `INC [0x00109090]` @ `0x58cc7` |
| `[0x9094]` | `0x109094` | `01 00 00 00` | frame counter #2 | FU-116/FU-127; native `MOV EAX,[0x00109094]` @ `0x58bdd`, `MOV [0x00109094],EBX` @ `0x59142` |
| `0x4BFC0` | `0x14BFC0` | all `00` (BSS, filled at runtime) | resource-handle table (63+ dwords) | FU-119 §2, FU-128 §1,§2; native `MOV EAX,0x14bfc0` @ `0x4a807`, `MOV EAX,[EAX*4+0x14bfc0]` @ `0x4afb8` |
| `0x55CE0` | `0x155CE0` | all `00` (BSS) | audio/MIDI device block | FU-115; native `CMP/MOV [...]` 50+ sites `0x64f73..0x678ef` |
| `[0x677C]` | `0x10677C` | all `00` | engine state dword | FU-126/FU-127; native `CMP [0x0010677c],0x1` @ `0x436f2`, `MOV [0x0010677c],0x3` @ `0x437d6` |
| `[0x6780]` | `0x106780` | all `00` | engine state dword | FU-126/FU-127; native `MOV EAX,[0x00106780]` @ `0x43608`, `CMP [0x00106780],0x2` @ `0x43d1f` |
| `[0x7300]` | `0x107300` | `00 00 00 00` (next dword `01 00 00 00`) | engine state/pointer dword | FU-126/FU-127; native `MOV [0x00107300],EDX` @ `0x492de`, `MOV EAX,[0x00107300]` @ `0x4937c` |
| `[0x7304]` | `0x107304` | `00 00 00 00` (then `01 00 00 00`) | engine state/counter dword | FU-126/FU-127; native `MOV ECX,[0x00107304]` @ `0x49573`, `CMP [0x00107304],0x4` @ `0x4a1a9` |
| `[0x730C]` | `0x10730C` | all `00` | engine state/pointer dword | FU-126/FU-127; native `MOV [0x0010730c],EBX` @ `0x493be`, `MOV ESI,[0x0010730c]` @ `0x496a1` |
| `0x4AE44` | `0x14AE44` | all `00` | scratch, element of dword array based `0x14AE40` | FU-114; native `MOV EDX,[EDX*4+0x14ae40]` @ `0x3da89`, `MOV [0x0014ae44],EBP` @ `0x3dc97` |
| `0x4AEEC` | `0x14AEEC` | all `00` | scratch/string buffer | FU-114; native `PUSH 0x14aeec` @ `0x3dc59`, `MOV EBP,0x14aeec` @ `0x3dc7b` |
| `0x4AF0A` | `0x14AF0A` | all `00` | scratch/string buffer | FU-114; native `PUSH 0x14af0a` @ `0x3dc76`, `MOV EAX,0x14af0a` @ `0x3dc85` |

Two adjacent strings are part of the same archive-open site (annotated too):

| code operand | real data address | current bytes | meaning | provenance |
|---|---|---|---|---|
| `0x1C64` | `0x101C64` | `"GAMEART\0"` | archive id string | site `0x4a6cd` (`MOV EDX,0x101c64`); native `PARAM` xref `0x4a6cd -> 0x101C64` |
| `0x1C6C` | `0x101C6C` | `"art/gameart0.pvi\0"` | resource archive path | site `0x4a6d2` (`MOV EAX,0x101c6c`); native `PARAM` xref `0x4a6d2 -> 0x101C6C` |

Verbatim byte reads (`ghidra_inspect_memory_content`, `/FIFA96.EXE`):

```
0x101C64: 47 41 4D 45 41 52 54 00 61 72 74 2F 67 61 6D 65 ...   "GAMEART\0art/gameart0.pvi"
0x101C6C: 61 72 74 2F 67 61 6D 65 61 72 74 30 2E 70 76 69 00   "art/gameart0.pvi\0lay%s.fmt"
0x10736C: 00 00 00 00 30 1A 10 00 38 1A 10 00 40 1A 10 00      (handle; table starts at +4)
0x107370: 30 1A 10 00 38 1A 10 00 40 1A 10 00 ...  A4 1A 10 00   (16 dwords -> 0x101A30..0x101AA4)
0x109090: 00 00 00 00 01 00 00 00 ...                          (counters)
0x109094: 01 00 00 00 ...
0x14BFC0: 00×32 ...
0x155CE0: 00×32 ...
0x10677C: 00×16
0x106780: 00×16
0x107300: 00 00 00 00 00 00 00 00 01 00 00 00 ...
0x107304: 00 00 00 00 01 00 00 00 ...
0x10730C: 00×16
0x14AE44: 00×16
0x14AEEC: 00×16
0x14AF0A: 00×16
```

## 2. String-consumer verification (Step 3)

`ghidra_search_instructions(program="FIFA96.EXE", operand_pattern="0x107370")`
returns 4 hits, all inside `FUN_0004a6bc`:

```
0004a6e3  MOV EDI, dword ptr [ESI + 0x107370]
0004a71c  MOV EAX, dword ptr [ESI + 0x107370]
0004a75d  MOV EBX, dword ptr [ESI + 0x107370]
0004a799  MOV EDI, dword ptr [ESI + 0x107370]
```

`ghidra_decompile_function(program="FIFA96.EXE", address="0x4a6bc")` shows the
`0x107370` table is consumed as a per-slot name parameter and the archive is
opened from the `0x101C6C` path (symbols auto-derived by the loader):

```c
if (param_1 != 0) {
    DAT_0010736c = FUN_0004a344(s_art_gameart0_pvi_00101c6c);
    ...
    do {
      FUN_00099e9f(local_40,uVar3,param_3,uVar2,(int)local_40,(byte *)s__s_fmt_00101c80);
      iVar1 = FUN_000a2614();
      *(int *)((int)&DAT_0014bfc0 + iVar6) = iVar1;
      ...
    } while (iVar5 != 0x78);
    ...
    do {
      uVar4 = *(undefined4 *)((int)&PTR_s_352ko_00107370 + iVar6);
      FUN_00099e9f(local_40,uVar3,uVar4,uVar2,(int)local_40,(byte *)s__s__s_00101c94);
      ...
    } while (iVar5 != 0xe0);
    ...
}
```

So `FUN_0004a6bc` opens the `GAMEART` / `art/gameart0.pvi` archive into
`DAT_0010736c` and resolves one name per slot from the `0x107370` table,
storing handles into `DAT_0014bfc0`. This matches FU-128 exactly, now with the
real addresses and resolved xrefs.

## 3. Changes made to the Ghidra project

Program `/FIFA96.EXE`, all via `ghidra_set_comment(type="plate")`, then
`ghidra_save_program`:

* `0x10736C`, `0x107370`, `0x109090`, `0x109094`, `0x14BFC0`, `0x155CE0`,
  `0x10677C`, `0x106780`, `0x107300`, `0x107304`, `0x10730C`, `0x14AE44`,
  `0x14AEEC`, `0x14AF0A` (the Step 2b minimum list), plus `0x101C64` and
  `0x101C6C` (the archive strings).
* Comment template: `<meaning> (code operand 0xX; +0x100000). See FU131.`
* No existing comments were present at any of these addresses
  (`batch_get_comments` returned 0/16 with comments beforehand); no labels were
  renamed. The loader had already created meaningful symbols
  (`DAT_0010736c`, `PTR_s_352ko_00107370`, `DAT_0014bfc0`,
  `s_art_gameart0_pvi_00101c6c`, …), which were preserved.
* `/fifa96_le.bin` was not modified.

## 4. Provenance

Ghidra MCP (all on `/FIFA96.EXE` unless noted):
`list_open_programs`, `list_instances`, `list_project_files("/")`,
`get_current_program_info`, `get_function_by_address` (`0x4a6bc`, both
programs), `search_instructions` (`0x107370` native, `0x7370` flat),
`get_bulk_xrefs` (14 addresses), `get_xrefs_to` `0x101C64`/`0x101C6C`,
`inspect_memory_content` (16 addresses), `search_instructions` for the
operand forms (`109094`, `155ce0`, `10677c`, `106780`, `107300`, `107304`,
`10730c`, `14ae44`, `14aeec`, `14af0a`, `10736c`, `14bfc0`),
`disassemble_bytes` `0x58ca0`/`0x3da80`, `decompile_function` `0x4a6bc`,
`batch_get_comments`, `set_comment` ×16, `save_program`.

## 5. Open legs

1. **`0x14AE44` is not independently referenced.** Its two native xrefs are the
   dword array base `0x14AE40` (`MOV EDX,[EDX*4+0x14ae40]` @ `0x3da89`, whose
   index reaches `0x14AE44`) and an explicit write @ `0x3dc97`. The higher-level
   meaning of the `0x14AE40` array (FU-114 "scratch") remains untyped.
2. **`0x155CE0` subsystem unknown.** The 50+ native references are all
   `CMP/MOV` on the dword; the audio/MIDI interpretation (FU-115) is asserted
   there, not re-derived here.
3. **`0x10677C`/`0x106780`/`0x107300`/`0x107304`/`0x10730C` meanings are
   coarse.** They are annotated as "engine state dword" from FU-126/FU-127;
   their exact field semantics are not re-established in this slice.
4. **The native `.image` overlay.** `/FIFA96.EXE` carries a whole-file `.image`
   overlay at `0x0` (FU-130 open leg 4). No plain-address read here resolved
   into it (`.object4` answered at `0x10xxxx`), but its effect on corpus-wide
   analysis is unverified.
5. **Uniform `+0x100000` vs. the full fixup tables.** This slice relies on the
   native loader, so it inherits FU-130 open leg 3: whether object-2/3 targets
   (`0x0E0000`/`0x0F0000`) also appear as relocated references is unchecked.
6. **Flat-image operand forms not re-verified per-row.** The `code operand`
   column is taken from FU-129 §5 / FU-128; only `0x7370`, `0x1c64` were
   re-checked against `/fifa96_le.bin` in this slice.
