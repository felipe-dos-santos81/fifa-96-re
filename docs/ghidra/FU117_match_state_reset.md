# FU-117 — the match/camera state reset and the `[0x4E578]` mode

Follow-on to FU-116 §6 leg 2 (`[0x4E578]`, the camera mode) and FU-110 §1
(the arming aggregator). This slice quotes the reset `FUN_000537f8` — the
single static caller of which is `FUN_0004a228` — and the fields it touches.

Result in one line: **`FUN_000537f8` clears the `0x4E570..0x4E5A4` state
block, seeds `[0x4E584]=0x50`, copies eight dwords from `0x8C3C` into
`0x4E5A8`, stores the mode at `[0x4E588]` (with the `0x10` special case
`0xC`/`[0x4E574]=1`/`[0x4E57C]=0x10`), then resets the `0x4E5C8..0x4E69C`
match-timer cluster (`[0x4E680]=0x12C`, `[0x4E5D4]=0x2BF20`, `[0x4E69C]=-1`,
`[0x4E5F0]=1`, `[0x4E538]=1`, `[0x8E04]=-1`).**

## 1. The reset (`FUN_000537f8`, 0x537F8..0x5392E)

Disassembly, in order:

```
PUSH EBX/ECX/EDX/ESI/EDI/EBP
EBP=0x50; ECX=8; EDI=0x4E5A8; EDX=0; ESI=0x8C3C
[0x4E570]=[0x4E574]=[0x4E578]=[0x4E57C]=[0x4E580]=0
[0x4E584]=0x50
CALL 0x4B380; [0x4E588]=EAX                  ; mode byte
[0x4E58C]=[0x4E590]=[0x4E594]=[0x4E598]=0    ; the replay phase counter + neighbours
MOVSD.REP ES:EDI,ESI                         ; 8 dwords 0x8C3C -> 0x4E5A8..0x4E5C7
[0x4E59C]=[0x4E5A0]=[0x4E5A4]=0
if (EAX == 0x10) { [0x4E588]=0xC; [0x4E574]=1; [0x4E57C]=0x10; }
EDX=0x12C; EBX=0x2BF20; EDI=EBP=EAX=0
[0x4E674]=[0x4E678]=[0x4E67C]=0
[0x4E680]=0x12C
[0x4E684]=[0x4E688]=[0x4E698]=0
[0x4E5C8]=[0x4E5CC]=[0x4E5D0]=0
[0x4E5D4]=0x2BF20
[0x4E5D8]=[0x4E5DC]=[0x4E5EC]=0
[0x4E510]=[0x4E530]=[0x4E534]=[0x4E660]=0
[0x4E69C]=-1; [0x4E5F0]=1; [0x4E538]=1; [0x8E04]=-1
RET
```

The `0x8C3C` table is zero in the static image (runtime-filled or BSS), so
the eight copied dwords are an initial pattern the caller supplies. The
`0x10` mode special case (`[0x4E588]=0xC`, `[0x4E574]=1`, `[0x4E57C]=0x10`)
matches the `mode == 0x10` branch of the arming aggregator (FU-111 §1), which
calls `FUN_0004c394`/`FUN_0004b100` in that mode.

## 2. Field links

| field | role |
|-------|------|
| `[0x4E58C]` | replay/camera phase counter (FU-116 §2: window `0xF1..0x168`; `0x53d58`/`0x53d7c`/`0x53d84`) |
| `[0x4E688]`, `[0x4E598]`, `[0x4E538]` | pause flags the replay control handler's callers set (`FUN_000512b0`, FU-116 §1) |
| `[0x4E578]` | camera mode: reset to 0 here, read by `FUN_00053d9c` (`!=0 && !=6`), written at the orphan site `0x51421` |
| `[0x4E584]` = `0x50` (80) | count/limit seed |
| `[0x4E680]` = `0x12C` (300) | timer seed |
| `[0x4E5D4]` = `0x2BF20` (180000) | large timer/rate seed |
| `[0x8E04]` = `-1` | `FUN_000510dc`'s neighbourhood (`[0x8E0C]`/`[0x8E10]`, FU-116 §1) |

`FUN_000537f8`'s only static caller is `FUN_0004a228` (the replay/ring arming
aggregator, FU-110/FU-111 §1); so the phase counter and the replay ring are
both armed by the same match-phase entry.

## 3. Closures / errata

* **FU-116 §6 leg 2 — partially closed.** The reset that seeds `[0x4E578]`,
  `[0x4E58C]` and the pause flags is quoted; the writer at `0x51421` is
  orphan (no function), so the mode's live transitions stay open.
* **FU-111 §1 — refined.** The `FUN_0004a228` chain is
  `FUN_000537f8` (camera/match state reset, this slice) → `FUN_0004cd0c` →
  `FUN_0004cba0` → `FUN_00064030`/`FUN_00064080` (ring reset/enable).
* **FU-101 §5 — refined.** The "camera state init" is this same reset; the
  camera-mode field is `[0x4E578]` (0 idle, 6 an excluded mode), not a
  separate structure.

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_function` 0x537F8 (70 insns);
`decompile_function` 0x537F8, 0x53D9C; `get_function_callers` 0x537F8 (1);
`search_instructions` `4e578` (3), `4e58c` (14, FU-116);
`read_memory` 0x8C3C (32 B, zero). Address mapping as FU-88.

## 5. Open legs

1. The orphan `[0x4E578]` writer at `0x51421` (function entry not created;
   neighbours `FUN_00051270`/`FUN_000512b0`/`FUN_000510dc`).
2. The `0x8C3C` eight-dword pattern's source (runtime-filled here).
3. The timer seeds `0x50`/`0x12C`/`0x2BF20` and their consumers.
