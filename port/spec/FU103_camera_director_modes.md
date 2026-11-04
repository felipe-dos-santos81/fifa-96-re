# FU-103 — the camera director routines and the state-mode writes

Follow-on to FU-101 §8 leg 2 (the `0x514A3..0x51B56` undefined region) and
FU-100 §8 leg 2 (the state writer of `[0x4E57C]`). This slice resolves the
region: it is a family of small director routines whose `[0x4E57C]` writes set
the camera modes consumed by `FUN_000505D0`'s 32-entry table.

Result in one line: **the `0x51xxx` region is a set of camera-director routines
(entry `FUN_000513EC` at `0x513EC`; helpers `0x510C0`/`0x510DC`/`0x51270`/
`0x512B0`/`0x513BC`/`0x51AB8`/`0x51C98`/`0x51CC0`) that write the state
`[0x4E57C]` with values `0x1F` (idle, four sites), `0x1C`, `0x14` and `0x06`,
gated on the counters `[0x8E34]`/`[0x8E38]` and paired with the camera-9 cuts
`FUN_0004CC98`/`FUN_0004FD50`; `FUN_000537F8`'s `0x10` and these values are
the complete static state set.**

## 1. The director entry `FUN_000513EC`

`get_function_by_address 0x513EC` gives body `0x513EC..0x513FE`, but the raw
bytes (`read_memory 0x513FC`, 16 B → `e504008915 80e50400 ba70e504 0085c9`)
show the continuation: the function head is

```
0x513ec PUSH ECX/EDX/ESI/EDI/EBP
0x513f1 XOR EDX,EDX
0x513f3 MOV ECX,[0x4E5A8]        ; block count/pointer from FUN_000537F8's copy
0x513f9 MOV [0x4E584],EDX
0x513ff MOV [0x4E580],EDX        ; (Ghidra cut the body one byte early here)
0x51405 MOV EDX,0x4E570          ; the state block (FU-101 §5)
0x5140a TEST ECX,ECX
```

so the entry zeroes the two dwords ahead of the block and walks the
`[0x4E570..]` state with `ECX = [0x4E5A8]`. It is called from `0x4B1D2`
(unnamed) and `FUN_0008784C` (`0x87C9D`); its body between `0x5140A` and the
first write site is still open (the region was never function-delimited —
Ghidra's `0x51xxx` functions are only the small helpers).

## 2. The `[0x4E57C]` writes

`get_xrefs_to 0x4E57C`'s seven `0x51xxx` write sites, each now with its value
and tail (`disassemble_bytes` windows):

| site | value | quoted context |
|------|-------|----------------|
| `0x514A3` | **0x1F** | `MOV EAX,0x1F` (`0x51492`), `[0x8E14]=[0x8E18]=ECX`, then `CALL 0x4FD50` (the 40-unit camera-9 cut, FU-101 §4) |
| `0x51819` | **0x1F** | `MOV EDX,0x1F`, `[0x8E14]=[0x8E18]=EBX=0`, `CALL 0x4FD50` |
| `0x518F3` | **0x1C** | `MOV EAX,0x1C` (`0x518E2`), `[0x8E14]=[0x8E18]=ECX`, `CALL 0x4FD50`; the routine continues with the `0x4E674` gate |
| `0x519A3` | **0x14** | `MOV EDX,0x14`, `[0x4E59C] += EAX`, then `POP EDX/EBX; RET` (`0x519AF..0x519B1`) — a complete small routine |
| `0x51AD0` | **0x1F** | `MOV EDX,0x1F`, `EBX=[0x8E34]`; if `EBX == 0`: `CALL 0x4CC98` (cinematic cut) and `[0x8E3C]--` (`0x51ADA..0x51AE6`) |
| `0x51B2D` | **0x06** | `MOV EDI,6`, `[0x8E34]=0`, `JMP 0x51B3C` (epilogue); the skipped arm `0x51B35 CALL 0x4FF24` |
| `0x51B56` | **0x1F** | `MOV EBX,0x1F`, `ECX=[0x8E38]`; if 0: `CALL 0x4CC98` + `CALL 0x51C98` |

Against FU-100 §4's table: `0x1F` → `0x5105D` (epilogue-only, a true no-op
mode), `0x1C` → `0x50FFF`, `0x14` → `0x51013`, `0x06` → `0x50E43` (shared
with the `0x10` state written by `FUN_000537F8` §5), `0x1D` → `0x50FDF` (the
`FUN_000513BC` random pick below). So **`0x1F` is the idle state the director
returns to**, and the state writes are the mode transitions.

The routines are separated by `RET` and two-byte `MOV EAX,EAX` (`8b c0`) pads
(e.g. `0x519B1`/`0x519B2`, `0x51B41`/`0x51B42`), so the region is a sequence
of director arms, not one body.

## 3. The director helpers (decompiled)

* `FUN_000510C0` (`0x510C0..0x510D8`, called from `0x49A69`, `FUN_00049B28`
  ×2, `0x4A117`): if `FUN_00063FD0() != 0` then `FUN_00064E8C(...)` and
  `[0x4E58C] = EDX`.
* `FUN_000510DC` (`0x510DC..0x51256`, called from `FUN_00051270`,
  `FUN_000512B0`, `0x5138A`): the mode countdown — `[0x8E0C]`/`[0x8E10]`
  (0 → `0xF0` ramp), `[0x4E58C]` (clamped at `0x169`), with
  `FUN_00045069`, `FUN_00064E74`, `FUN_00064DFC`, `FUN_00064E8C`; the
  `[0x8E0C] == 1` path calls `FUN_000642FC` then the **no-return**
  `FUN_0004CD88`.
* `FUN_00051270` (`0x51270..0x512AC`): `FUN_0008F178()`; `EDX == 0` →
  `FUN_000510DC(EAX, 0)`, else `FUN_00043610` + `FUN_0004CD70` +
  `FUN_00063734`.
* `FUN_000512B0` (`0x512B0..0x51367`): same dual path; gates
  `[0x4E688]`/`[0x4E598]`/`[0x4E538]` (the last only when
  `FUN_0004B5F4() > 3`), `FUN_00054018` / `FUN_00053E08` on the
  `[0x4E674]`/`[0x4E5C8]` counters.
* `FUN_000513BC` (`0x513BC..0x513EA`, called from `0x8D490`): sets
  `[0x8E1C] = 0x1D`, or `0x1C` when `FUN_000566d0() & 1 == 0` — the random
  pick behind states `0x1C`/`0x1D`.
* `FUN_00051AB8` (`0x51AB8`, 5 B): `return [0x4E574]` — the flag
  `FUN_000537F8` sets when the mode byte is `0x10`.
* `FUN_00051C98` (`0x51C98..0x51CBD`): `[0x8E3C]--`; at zero `FUN_00047888`;
  returns `([0x8E3C] < 1)`.
* `FUN_00051CC0` (`0x51CC0..0x51CD0`, called from `0x53BF9`): zeroes
  `[0x4E584]`/`[0x4E580]`.
* Also in the band: `0x51068` (8-byte function) and the `0x51071` helper
  (`PUSH ESI/EDI; if EAX: copy [0x7DC8]+0x10 xyz to EAX`), called from
  `FUN_00091E64`; `0x513EC`'s sibling `0x5138A` calls `FUN_000510DC`.

`get_xrefs_to` shows the call sites into the band from the match/entity code:
`0x4512E` (`FUN_0004511D`), `0x45EE4` (`FUN_00045D0D`), `0x49A69`/`0x49C25`/
`0x49ED4`/`0x49FA4`/`0x49FCF`/`0x4A117` (`FUN_00049B28`), `0x4B1D2`,
`0x53BF9`, `0x87C9D` (`FUN_0008784C`), `0x8D490`, `0x91E7E`/`0x91E83`
(`FUN_00091E64`) — the director is driven by the match phases, not the camera
band itself.

## 4. Closures / errata

* **FU-101 §8 leg 2 — closed structurally.** The `0x514A3..0x51B56` region is
  a sequence of director arms (separated by `RET` + `8b c0` pads); the seven
  state writes carry `0x1F`/`0x1C`/`0x14`/`0x06`.
* **FU-100 §8 leg 2 — closed for the state values.** `0x1F` is the idle
  state (case `0x5105D`, no camera change); `0x10` (mode byte), `0x1C`,
  `0x14`, `0x06`, `0x1D` (random pick) complete the static set, together with
  the FU-100 unknown-state default.
* **FU-100 §6 remark** ("`0x1F` jumps straight to the epilogue") — confirmed
  as a real idle mode, written four times by the director.
* **Ghidra boundary note** — `FUN_000513EC` is cut at `0x513FE` by the
  one-byte decode slip at `0x513FF` (`89 15 80 E5 04 00`); the raw read is
  the citation, per the FU-88 method.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_function_by_address` 0x513EC, 0x510C0,
0x51AB8, 0x513BC, 0x51CC0, 0x51270, 0x512B0, 0x510DC, 0x51C98;
`disassemble_bytes` 0x513EC (32 B), 0x51490 (48 B), 0x518E0 (48 B), 0x51809
(40 B), 0x51993 (40 B), 0x51AC0 (40 B), 0x51B1D (40 B), 0x51B46 (40 B);
`decompile_function` 0x510C0, 0x510DC, 0x51270, 0x512B0, 0x513BC, 0x51AB8,
0x51C98, 0x51CC0; `read_memory` 0x513FC (16 B); `search_instructions` CALL
`00051` (24 hits); `get_xrefs_to` 0x4E57C (10). Address mapping as FU-88.
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 6. Open legs

1. **The director body** between `0x5140A` and `0x51492` and between the
   arms (the region is still functionless in the project; hand-disassembly
   only).
2. **The counters**: `[0x8E14]`/`[0x8E18]` (the pair written with every
   `0x1F`/`0x1C` state), `[0x8E34]`, `[0x8E38]`, `[0x8E3C]`, `[0x8E1C]`,
   `[0x4E58C]`, `[0x4E59C]`, `[0x4E674]`, `[0x4E688]`, `[0x4E598]`,
   `[0x4E538]`, `[0x4E5A8]`.
3. **`FUN_0004CD88`** (called no-return from `FUN_000510DC`) — fatal/exit
   path?
4. **`FUN_000642FC`/`FUN_00064270`/`FUN_00064E74`/`FUN_00064DFC`/
   `FUN_00064E8C`** countdown subsystem and `FUN_00047888` (called at
   `[0x8E3C] == 0`).
5. **The `0x49B28`/`0x4511D`/`0x45D0D` callers** — which match phases drive
   the director and with what arguments (the `0x51xxx` arms take EAX/EDX).
6. **`[0x8E0C]`/`[0x8E10]`** and the `0xF0`/`0x168`/`0x169` limits.
