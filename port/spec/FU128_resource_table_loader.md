# FU-128 — the resource-handle table loader `FUN_0004a6bc`

Follow-on to FU-119 §2/§6 leg 2 (the `[0x4BFC0]` resource table's filler). The
filler is `FUN_0004a6bc`, whose body was cut by a mis-disassembled region at
`0x4A730`; repaired this slice to `0x4A6BC..0x4A82C`.

Result in one line: **`FUN_0004a6bc(EAX != 0)` loads a resource archive
(`FUN_0004a344(0x1C64, 0x1C6C)` → `[0x736C]`) and resolves a name per slot —
formatted from `0x1C80`/`0x1C88`/`0x1C9C` with parameters from the `0x7370`
table — via `FUN_000a2614`, storing the handles into `0x4BFC0 + 4k`; the
unload path (`EAX == 0`) frees the archive and zeroes 63 dwords at `0x4BFC0`
with `FUN_0009e907`.**

## 1. The load (EAX != 0), from the disassembly

```
0x4A6CD MOV EDX,0x1C64
0x4A6D2 MOV EAX,0x1C6C
0x4A6D7 CALL 0x4A344                  ; archive/table handle
0x4A6DE MOV [0x736C],EAX              ; handle
loop A (ESI = 0, step 4, until 0x78):
    EDI = [ESI + 0x7370]              ; per-slot parameter
    FUN_00099e9f(local, 0x1C80, EDI)  ; format the name
    ADD ESI,4
    [ESI + 0x4BFBC] = FUN_000a2614(handle, local)   ; -> [0x4BFC0 + k*4]
loop B (continues ESI, step 4, until 0xFC):
    same with format 0x1C88, then (after a repaired gap) 0x1C9C
    [ESI + 0x4BFBC] = handle lookup
final:
    [0x4C0E0] = FUN_000a2614(handle, 0x1CA4-variant)
```

So the table spans `0x4BFC0` through the single extra slot at `0x4C0E0`; the
name templates are `0x1C80`/`0x1C88`/`0x1C9C`/`0x1CA4` and the parameters
come from the `0x7370` table (the same table the unload error path formats).
`FUN_000a2614` is the name→handle resolver (the same call FU-114 §1's
`FUN_0004a6bc` reference used).

## 2. The unload (EAX == 0)

```
0x4A7F7 if ([0x736C] == 0) return;
0x4A801 CALL 0x993EC                  ; free the archive
0x4A807 MOV EAX,0x4BFC0
0x4A819 MOV ECX,0x3F                  ; 63 dwords
0x4A81E CALL 0x9E907                  ; fill with 0
0x4A813 [0x736C] = 0
```

`FUN_0009e907(EAX = dest, ECX = count, EDX = value)` is a memset-with-
alignment: 63 dwords at `0x4BFC0` are zeroed. This matches
`FUN_0004a830(0)`'s tail clear (`0x4BFC0 + 0xFC..0x118`, FU-119 §2) — the
unload clears the main array, the other clear handles the tail entries.

Consumers: the getter `FUN_0004afb8(EAX)` (FU-119 §2) and the HUD decode chain
`FUN_0004afc0` → `decode_size_probe`/`FUN_00098c38`/`decode_record_strict`
(FU-116 §3).

## 3. Closures / errata

* **FU-119 §6 leg 2 — closed.** The `[0x4BFC0]` filler is `FUN_0004a6bc`
  (load with `FUN_0004a344`/`FUN_000a2614`, unload with `FUN_000993ec`/
  `FUN_0009e907`).
* **FU-116 §3 — refined.** The HUD resource is loaded here from the
  `0x1C64`/`0x1C6C` archive with names built from the `0x7370` parameter
  table.
* **FU-114 §1 — refined.** `FUN_0004a6bc`'s `FUN_00099e9f` call belongs to
  this loader, not the UI scratch path.
* Ghidra-project change: cleared the mis-disassembled `0x4A730..0x4A79A`
  region and repaired `FUN_0004a6bc` (body now `0x4A6BC..0x4A82C`).

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: instruction dump `0x4A6BC..0x4A82C`;
`clearListing` `0x4A730..0x4A79B` + `DisassembleCommand` from `0x4A71C`;
`decompile_function` 0x4A6BC, 0x9E907; `search_instructions 4bfc0` (FU-119).
Address mapping as FU-88.

## 5. Open legs

1. `FUN_0004a344` (archive open), `FUN_000a2614` (name resolver),
   `FUN_0009e907` (fill) and the `0x1C64`/`0x1C6C`/`0x1C80`/`0x1C88`/
   `0x1C9C`/`0x1CA4` template strings.
2. The `0x7370` parameter table's source.
3. The `0x4C0E0` extra handle's consumer.
