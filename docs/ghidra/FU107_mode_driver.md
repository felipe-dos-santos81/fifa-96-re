# FU-107 — the mode driver `FUN_000642FC` (the replay/photo state machine)

Follow-on to FU-106 §6 leg 1 (the decompile that timed out) and FU-106 §1
(the mode byte). This slice disassembles the 201-instruction body and maps its
mode dispatch, input flags and exit path.

Result in one line: **`FUN_000642FC(EAX = ticks)` drives `[0x9A98]` through
`0x80 → 0x81 → 0x82 → {0x83, 0x84, 0x85, 0x86} → 0x82`, with the exit path
clearing to mode 0 and calling the camera driver `FUN_0004CEF4`; the five
states dispatch through the table at image `0x642E8` and the per-frame input
word `[0x9AA4]` selects camera pans (`FUN_0004CA08`/`FUN_0004CA40`).**

## 1. The entry and the 0x80/0x81 arms

```
0x64302 MOV EDI,EAX                  ; tick/arg
0x64304 MOV EBP,[0x9a98]             ; mode
0x64310 MOV [0x9aa4],0               ; input word
0x64316 TEST EBP,EBP ; JZ 0x6465a    ; mode 0: return
0x6431e CMP EBP,0x80 ; JNZ 0x64330
0x64326 MOV [0x9a98],0x81            ; 0x80 -> 0x81
0x64330 CMP [0x9a98],0x81 ; JNZ 0x64385
```

the `0x81` arm (`0x6433c..0x64380`) calls `FUN_00053d7c` (with
`FUN_000478fc`/`FUN_00044d7c` when it returns 0), zeroes `[0x9a94]` and
`[0x9a8c]`, calls `FUN_00063cbc` and `FUN_0004d134`, sets **mode 0x82**, and
zeroes `[0x9aa8]`/`[0x9ac8]`.

## 2. The per-frame input word and the exit

```
0x64385 CALL 0x00064aa4 ; [0x9aa4] = EAX
0x6438f CALL 0x00045025 ; [0x9aa4] |= (EAX & 0xFF)
0x643a1 MOV AH,[0x9ac8] ; if (previous && new) clear the low byte bit 0
0x643c9..0x643e9  CMP EBP,{0x84,0x85,0x86,0x87} -> MOV [0x9a98],0x82
0x643f3 CMP EDX,0x1     ; the "confirm" flag
0x643f8 CMP [0x9a98],0x82 -> mode 0x83 + FUN_000974d8 + [0x9a94]/[0x9ac0] loop
                     else  -> FUN_000974d8 + mode 0x82
0x64449 CALL 0x000451f1(2); if 0 and ([0x9aa4] & 0x80) == 0 -> fall through
0x64460 CALL 0x000543d4 ; XOR EAX; CALL 0x00015594
0x6446e MOV [0x4f60],0
0x64473 MOV [0x9a98],0          ; exit to normal mode
0x64478 CALL 0x0004afa0 -> [0x9ac4]
0x64487 CALL 0x0004cef4         ; the camera re-select driver (FU-101 §4)
0x6448c CALL 0x00036c70 ; CALL 0x00053d9c ; OR [0x9aa4],0x80
```

so the same `FUN_0004CEF4` camera driver that `FUN_00064e8c` (FU-106 §3)
calls also runs on this exit path.

## 3. The pan inputs

Bits of `[0x9aa4]` (the low byte is the fresh input, the `AH` byte the
previous frame's):

| bits | action |
|------|--------|
| `0x10`, `0x4` | `ECX = 4` (`0x64531`); with `0x40` instead: `FUN_0004ca08(0x32)` |
| `0x10`, `0x8` | `ECX = -4` |
| `0x40`, `0x8` | `FUN_0004ca08(-0x32)` |
| `0x1` / `0x2` | `EBX = 4` / `-4` (horizontal arm) |
| `0x1` / `0x2` (`0x40` set) | `ESI = 4` / `-4` (vertical arm) |
| `0x20` | `[0x9a8c] = ([0x9a8c] + 1) mod 6`; at 5, `FUN_0004ca40(EAX=ECX, EDX=ESI)` |

The input byte handling also suppresses a fresh byte when the previous one
was already set (`0x643a1..0x643c6`, `[0x9ac8]` stores the last word).

## 4. The state dispatch (modes 0x82..0x86)

```
0x6459b MOV EAX,[0x9a98] ; SUB EAX,0x82 ; CMP EAX,4 ; JA 0x6465a
0x645ae JMP dword ptr CS:[EAX*0x4 + 0x542e8]
```

the table at image `0x642E8` (`read_memory 0x642E8`, 5 dwords →
`b6450500 c5450500 18460500 04460500 2c460500`, values + `0x10000`):

| state | entry | action (`disassemble_bytes`) |
|-------|-------|------------------------------|
| 0x82 | 0x645B6 | `[0x9a9c] = 0`; return (`0x645b6..0x645c4`) |
| 0x83 | 0x645C5 | `[0x9aa8] += EDI`, then the countdown loop `0x645CB`: `FUN_00063d34`, while `0 < i <= [0x9aa8]`: `[0x9aa8] -= i`, `FUN_00063cbc`, `FUN_0006428c`, re-read |
| 0x84 | 0x64618 | `FUN_000974d8`; loop 2×: `FUN_00063cbc` |
| 0x85 | 0x64604 | `FUN_000974d8`; loop 2×: `FUN_00063e54` |
| 0x86 | 0x6462C | `FUN_000974d8`; if `[0x9a9c] > 0`: `[0x9a9c]--`; else `FUN_00063cbc` and `[0x9a9c] = [0x9aa0]` |
| else | 0x6465A | epilogue return |

`[0x9a9c]`/`[0x9aa0]` are a replay/photo count and its reload value; modes
0x84/0x85/0x86 are the transient replay arms and 0x83 the active countdown
(consistent with FU-106 §2's `FUN_00064dfc`).

## 5. Closures / errata

* **FU-106 §6 leg 1 — closed.** `FUN_000642FC` is mapped; the decompiler
  timeout was replaced by `disassemble_function` (201 instructions).
* **FU-106 §1 mode table — extended.** The full static cycle is
  `0x80 → 0x81 → 0x82 → {0x83,0x84,0x85,0x86} → 0x82`, with `0x87` mapped
  back to `0x82` and the exit writing `0`.
* **Camera link — confirmed once more.** The exit path calls `FUN_0004CEF4`
  (`0x64487`), the same driver `FUN_00064e8c` uses.
* **Ghidra decode note** — the `0x64500..0x64504` window carries the mode
  `0x86` write behind a `CMP EDX,0x10` block the linear listing mis-decoded
  (`CLI; ADD DH,[EBP+0xA]`); the write at `0x64504` is the citation.

## 6. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_function` 0x642FC (201 insns);
`disassemble_bytes` 0x645B0 (32 B), 0x64604 (88 B); `read_memory` 0x642E8
(20 B, jump table; the `CS:`-relative `0x542E8` + `0x10000` resolve),
0x542E8 (20 B, refuted as the table base). Address mapping as FU-88.
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 7. Open legs

1. **`FUN_00064AA4`/`FUN_00045025`/`FUN_000451F1(2)`** — the input word
   sources; `FUN_000543D4`, `FUN_00015594`, `[0x4F60]`, `[0x9AC4]`,
   `FUN_00053D9C`.
2. **`FUN_00063CBC`/`FUN_00063D34`/`FUN_00063E54`/`FUN_0006428C`** — the
   replay queue step/reset calls.
3. **`FUN_0004CA08`/`FUN_0004CA40`** — the pan/step actions (FU-104 §2's
   neighbours).
4. **`FUN_000974D8`** (called on every state transition) and
   `FUN_0004D134`/`FUN_00053D7C`/`FUN_000478FC`/`FUN_00044D7C` (the 0x81
   arm).
5. **`[0x9A9C]`/`[0x9AA0]` semantics** (replay-photo count) and the
   `[0x9AA4]` bit map beyond the pan bits.
