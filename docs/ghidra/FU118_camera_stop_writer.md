# FU-118 — the camera-mode stop writer and the replay button edge

Follow-on to FU-117 §5 leg 1 (the orphan `[0x4E578]` writer at `0x51421`).
The writer is inside `FUN_000513ec`, whose body was truncated by a
mis-disassembled boundary at `0x513ff`; this slice repairs it and names the
neighbouring replay button edge.

Result in one line: **`FUN_000513ec` (body `0x513EC..0x51440`, repaired) is
the camera/replay stop routine — it clears `[0x4E584]`/`[0x4E580]`, calls the
`[0x4E5A8]` callback (the first of the eight dwords `FUN_000537f8` copies from
`0x8C3C`), sets camera mode `[0x4E578] = 2`, and on first entry clears the
recorder gate (`FUN_00064074`, sets `[0x4E574] = 1`); `FUN_00051370` is the
button edge that calls `FUN_000510dc(EAX=1)` when `FUN_0004ff0c()` admits it.**

## 1. `FUN_000513ec` — the stop routine

```
[0x4E584] = 0; [0x4E580] = 0;
if ([0x4E5A8] != 0) (*[0x4E5A8])();     // the copied table's callback slot
[0x4E578] = 2;                          // camera mode
if ([0x4E574] == 0) { FUN_00064074(); [0x4E574] = 1; }   // recorder gate clear
```

* Caller: `FUN_0008784c` (a live-play function; `get_function_callers
  0x513EC` = 1).
* `FUN_00064074` is the recorder-gate clear (FU-109 §3); with `0x53DC4`
  (FU-110) these are its only two callers.
* The `[0x4E5A8]` callback is byte 0 of the eight dwords seeded by
  `FUN_000537f8` from `0x8C3C` (FU-117 §1) — so the reset and the stop are a
  pair over the same slot table.

The body was truncated at `0x513fe` because the *next* instruction was
mis-boundaried: the real stream is `0x513F9 MOV [0x4E584],EDX`;
`0x513FF MOV [0x4E580],EDX`; `0x51405 MOV EDX,0x4E570` — Ghidra had an
`ADC EAX,0x4E580` starting at `0x51400`. Clear/re-disassemble from `0x513FF`
and the body runs contiguous to the `RET` at `0x51440` (same artifact class
as `0x49FD8`/`0x646E4`, FU-111 §5).

## 2. `FUN_00051370` — the button edge (created)

```
if (FUN_0004ff0c() == 0) { FUN_0004ff24(); return 0; }
FUN_0008f178();
if (EDX == 0) FUN_000510dc(EAX=1);       // arm the replay control handler
else { FUN_00043610(); FUN_0004cd70(); FUN_00063734(); }
return 1;
```

No static caller — an indirect button handler. Its `FUN_000510dc(EAX=1)`
call is one of the two `in_EAX != 0` arms FU-116 §1 named (the other is
`FUN_00051270`/`FUN_000512b0`).

`FUN_000513bc` (adjacent) selects `[0x8E1C] = 0x1D` when its input is 1 or
`FUN_000566d0() & 1`, else `0x1C`.

## 3. Closures / errata

* **FU-117 §5 leg 1 — closed.** The `[0x4E578]` writer is `FUN_000513ec`
  (not an orphan); the `0x51421` site is its mode store.
* **FU-116 §1 — refined.** The button edge path is `FUN_00051370` →
  `FUN_000510dc(EAX=1)`; `FUN_000510dc`'s `in_EAX != 0` arm is reachable.
* **FU-110 §1 — refined.** `FUN_00064074`'s callers are `FUN_000513ec`
  (stop) and `FUN_000053dc4` — the recorder gate is cleared by the camera
  stop, not the ring teardown (FU-114).
* Ghidra-project change: cleared `0x513FF..0x51405` and re-disassembled
  `0x513FF`; recreated `FUN_000513ec` (body `0x513EC..0x51440`); created
  `FUN_00051370` (body `0x51370..0x513B9`).

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble`/`decompile_function` 0x513EC
(repaired), 0x51370 (created), 0x513BC; `get_function_callers` 0x513EC (1:
0x8784C), 0x51370 (0); instruction sweep `0x51300..0x51440`. Address mapping
as FU-88.

## 5. Open legs

1. `FUN_0004ff0c`/`FUN_0004ff24` (the button edge's gate) and `FUN_0008f178`.
2. `[0x8E1C]` (0x1C/0x1D) consumers and `FUN_000566d0`.
3. The `0x8C3C` eight-dword table's live content (FU-117 §5 leg 2).
