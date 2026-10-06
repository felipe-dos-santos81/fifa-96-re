# FU-114 — erratum: the "0x4AFxx teardown / free tail" is runtime scratch data

Follow-on to FU-110 §1 (the "allocator caller" at `0x4AF18`) and FU-111 §2
(the "buffer free tail" `0x4AEEE..0x4AF1D`). While chasing FU-111 §8 leg 1,
`FUN_0003dbb0` was found writing into the middle of that byte range — so the
bytes cannot be executable, and the derived claims are retracted.

Result in one line: **`0x4AE44..0x4AF1D` is a runtime scratch cluster (pointer
globals, a pointer array and two sprintf caption buffers) written by the UI
screen builder `FUN_0003dbb0`; the static bytes happen to decode as coherent
call sequences, which is exactly why the earlier sweep took them for a
teardown tail — the `0x4AF18 → FUN_00063ebc` xref is a disassembly artifact,
and `FUN_00063ebc` has no static caller.**

## 1. The writer `FUN_0003dbb0`

`FUN_0003dbb0` is an indirect-entry **UI screen builder** (no static caller):
it formats two strings and draws them plus a button (`FUN_00015960`, the same
draw helper the replay HUD uses, FU-111 §5). The writes that settle the
question are all absolute:

```
FUN_000603ec();
_LAB_0004ae90        = FUN_00011bec(...);        // pointer array entry 0
_LAB_0004ae90_4      = FUN_00011bec(...);        // entry 1
_LAB_0004ae3c_4      = FUN_0001771c();
FUN_00099e9f(0x4AEEC, &DAT_0000195c);            // caption 1
FUN_00099e9f(0x4AF0A, &DAT_00001964);            // caption 2
_LAB_0004ae48        = &LAB_0004af0a;            // string pointer
DAT_0004ae44         = &LAB_0004aeec;            // string pointer
PTR_DAT_0004ae4c     = FUN_0001771c();
...
DAT_0004af8c = DAT_0004afd0 * 5;
iVar22 = *(int *)(&LAB_0004ae90 + DAT_0004afcc * 4);   // pointer array read
```

`FUN_00099e9f(dest, fmt)` is the `sprintf` used throughout the presentation
code (FU-111 §5): `iVar1 = FUN_000afd4f(&LAB_00089e8c, fmt);
*(dest + iVar1) = 0`. So `0x4AEEC` and `0x4AF0A` are **caption buffers**; a
caption longer than 2 resp. 14 bytes overwrites the "instructions" the earlier
sweep quoted (the `CALL 0x43D94` block sits at `0x4AEEE`, two bytes into the
first buffer; the `XOR EAX,EAX; JMP 0x63EBC` sits at `0x4AF16`, twelve bytes
into the second). The pointer writes at `0x4AE44`/`0x4AE48` overwrite the
first sequence's middle. `0x4AE90` is a pointer array (`FUN_0003dbb0` writes
its two entries), read indexed by `[0x4AFCC]`.

The neighbouring globals `0x4AF2C`, `0x4AF38`, `0x4AF3C` (cleared by the same
function) and `0x4AF8C`, `0x4AFCC`, `0x4AFD0` (scale/index) complete the
cluster.

## 2. What the bytes decode as, and why that is a trap

The static image at `0x4AE00..0x4AF1D` is a long, coherent-looking run of
5-byte `CALL`s to real functions (`0x58E00`, `0x4B454`, `0x92F04`, `0x49138`,
…) ending in a textbook epilogue at `0x4AE9B..0x4AEA4` (`ADD ESP,0xC; POP
ESI; POP EDX; POP ECX; POP EBX; RET`), then a second run up to the
"free" `XOR EAX,EAX; JMP 0x63EBC` at `0x4AF16`. Linear disassembly (and the
scoped disassembly runs in FU-110/FU-111) produced instructions and xrefs from
it. But the same addresses are written at runtime by §1, so the sequences are
dead-code remnants reused as scratch — the project's "raw-byte quotes must
reconcile" rule catches exactly this case.

## 3. Retractions

* **FU-110 §1 — retracted.** "allocator `FUN_00063ebc` from `0x4AF18`" was
  the xref from the bogus `JMP` inside the second caption buffer. After the
  cleanup, `get_xrefs_to 0x63EBC` is **empty**; `FUN_00063ebc` (the replay
  buffer alloc/free, FU-108 §1) has **no static caller** — it is an
  indirect/table or loader-side entry (open leg).
* **FU-111 §2 — retracted.** The "buffer free tail `0x4AEEE..0x4AF1D`" and
  the teardown sequence `CALL 0x43D94 / 0x4A830 / 0x4B454 / 0x92F04 /
  0x49138 / 0x58E00 → JMP 0x63EBC` do not exist as a routine. The named
  callees are still independently real functions (their own bodies are
  elsewhere); only this call sequence is spurious.
* **FU-111 §8 leg 1 — answered negatively.** The orphan tail's "entry" is
  not missing: there is no routine there. The remaining open question is the
  real caller of `FUN_00063ebc`.
* **FU-108 §6 leg 2 / FU-109 §7 leg 2** (the `FUN_0004a448` allocator and
  the free) — unchanged as functions, but their call sites are again
  unknown.

## 4. Cleanup performed (Ghidra project)

* `clearListing(0x4AE00, 0x4AF1F)` — 44 instructions removed (this includes
  the instructions created by this session's earlier `disassemble_bytes`
  probes at `0x4AE00`/`0x4AE81`, which had extended the artifact).
* Plate comments added at `0x4AE44`, `0x4AE90`, `0x4AEEC`, `0x4AF0A` marking
  the cluster as runtime data, not code.
* `get_xrefs_to(0x63EBC)` re-run: 0 references.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x3DBB0, 0x99E9F;
`read_memory` 0x4AE44 (220 B), 0x4AE90 (96 B), 0x4AE00 (raw, earlier);
`search_instructions` `4ae44` (1 write), `4ae48` (1 write), `4aee` (30,
all data accesses), `4aef` (0), `4af0` (2, `FUN_0003dbb0` pointers);
`clearListing` 0x4AE00..0x4AF1F; `get_xrefs_to` 0x63EBC. Address mapping as
FU-88.

## 6. Open legs

1. `FUN_00063ebc`'s real caller — indirect entry, or the 16-bit loader's
   phase table (the LE-image sweep cannot see it).
2. Which UI screen `FUN_0003dbb0` builds (its two captions and the
   `0x4AE90` pointer array's objects).
3. Whether `0x4AE00..0x4AE43` (cleared as unresolved) belonged to a genuine
   routine or the same scratch pattern.
