# FU-124 — the frame tick computation

Follow-on to FU-123 §5 leg 1 (`FUN_000492d4`, the orphan writers
`0x496A7`/`0x49A5E`/`0x49AA2`). This slice decomposes the timer-delta routine
behind the shared frame tick `[0x7300]`.

Result in one line: **`FUN_00049280(EAX = 0..9)` reads a timer
(`FUN_000cb2a4`), scales it by `12/5`, keeps a 10-entry last-value table at
`0x7334 + idx*4` and returns the per-slot delta in `EAX`; `FUN_000492d4` and
the orphan per-state handlers store a word into the shared tick `[0x7300]` —
two of them (`0x49A5E`, `0x49AA2`) store `EDX` (the restored entry value, 0),
one (`0x496A7`) stores `EAX` (the delta), a discrepancy carried as an open
leg.**

## 1. The timer-delta routine `FUN_00049280`

```
if (EAX < 0 || EAX >= 10) return 0;
EAX = FUN_000cb2a4();                 // timer read
EDX = EAX;                            // value
EAX = EAX*4 - EDX;                    // *3
EDX = EAX*4;                          // *12
IDIV 5;                               // /5   -> scaled current
EDX = EAX - [0x7334 + idx*4];         // delta
[0x7334 + idx*4] = EAX;               // store current
EAX = EDX;                            // return delta
POP EDX;                              // caller's EDX restored
```

So the delta ends in **EAX** (a `MOV EAX,EDX` at `0x492C5`); `EDX` at return
is the caller's entry value (the function saves/restores it). The 10-slot
table `0x7334..0x735B` keeps the last scaled timer per slot.

`FUN_000492d4` (the wrapper): `[0x7300] = high32(FUN_00049280(idx, 0))` — the
decompiler's "high half" is the restored EDX, i.e. the caller's EDX, while
the true delta is returned in EAX (custom register convention).

## 2. The `[0x7300]` writers

| site | store | in |
|------|-------|----|
| `0x496A7` | `MOV [0x7300], EAX` | orphan frame handler (`... CALL 0x49280; MOV [0x7300],EAX; CMP EBX,[0x730C] ...`) |
| `0x49A5E` | `MOV [0x7300], EDX` | orphan handler at `0x49A54` (`XOR EAX,EAX; XOR EDX,EDX; CALL 0x49280; ...`) |
| `0x49AA2` | `MOV [0x7300], EDX` | orphan handler at `0x49A98` (same shape) |

The two `EDX` stores write the zero the handlers passed in (`XOR EDX,EDX`),
i.e. they would zero the tick; the `EAX` store writes the true delta. Either
the two handlers are state-specific "tick freeze" paths or their store is a
mis-decode — the opcode is `89 15` (`MOV r/m32, EDX`), so the bytes are as
written. Carried open.

Readers: `FUN_00049388`/`FUN_0004937c` (`return [0x7300]`), consumed by the
replay control timeline and the view blend (FU-116 §1, FU-122 §2).

## 3. Closures / errata

* **FU-123 §5 leg 1 — partially closed.** `FUN_00049280` is the timer-delta
  routine and `[0x7334]` its 10-slot table; `FUN_000492d4`'s "high half" is
  the restored EDX, with the delta in EAX.
* **FU-123 §2 — refined.** The tick source is the delta of a ×12/5-scaled
  `FUN_000cb2a4` timer read, not a raw counter.
* **FU-123 §5 leg 1 (orphans) — open.** The three orphan writer handlers
  still have no created functions; the EDX/EAX discrepancy is recorded.

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_function` 0x49280 (33 insns);
`decompile_function` 0x49280, 0x492D4; instruction context
`0x49670..0x496C0`, `0x49A30..0x49ACF`; `search_instructions 7300` (6).
Address mapping as FU-88.

## 5. Open legs

1. The three orphan handlers' entries (`<0x4966C`, `0x49A54`, `0x49A98`) and
   whether the EDX stores are deliberate.
2. `FUN_000cb2a4` (the timer read) and the `12/5` scale's meaning.
3. `[0x730C]`/`[0x7314]`/`[0x7318]` (the neighbouring frame counters).
