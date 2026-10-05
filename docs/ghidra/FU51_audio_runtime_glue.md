# FU-51 — audio runtime glue: sound init, channel handlers, the stream queue, play-by-id and start-one-voice

Date: 2026-10-05. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the runtime glue between the game and the FU-43/FU-47 arm chain —
`FUN_00064f70` sound-system init and its six channel arrays, the channel
setters `FUN_00065004`/`FUN_0006504c` and the `0xA0AC` handler table, the
stream queue flush `FUN_000a7e06` with its `0x5DB98`/`0x5DB9C` list, the
play-by-id dispatch `FUN_000a7728` (with `FUN_000a7705`/`FUN_000a76ea`/
`0xA771B`), the full `FUN_000a780e` start-one-voice body (the decompiler prunes
it), and the population status of the `DAT_00061c14` descriptor table.
Companions: FU-43 (`.BNK` registrar/descriptors), FU-47 (allocator, RNG,
sound-state gate), FU-35 (`1SNh`/`1SNd`/`1SNe` queue), FU-41 (`.VIV` banks),
FU-50 (voice-control wrappers, the EACSNDF record). Every claim cites an
instruction address or an image read.

## 0. Address frame and the EACSNDF record

Code addresses are flat image addresses (Ghidra listing). Data operands inside
the code are stored object-relative; as in FU-35/FU-43/FU-50 the listing shows
them as written and the cited image addresses resolve the same way (e.g. the
handler table operand `0xA0AC` reads at image `0x10A0AC`, the queue head
`0x5DB98` at image `0x15DB98`, the descriptor table `0x61C14` at image
`0x161C14`). The EACSNDF record is `0x61994 + voice*0x28`, 16 records
(FU-47 §1). Fields added or re-derived by this slice:

| stored | image | width | role | writer(s) | reader(s) |
| --- | --- | --- | --- | --- | --- |
| `+0x0A` | `0x16199E` | u8 | descriptor flags copy (`desc[0x1C]`) | `a780e` `0xA789A` | — |
| `+0x0B` | `0x16199F` | u8 | type: 1 = bank arm (`a780e`), 2 = stream arm (`a79dc`), 3 = direct `.VIV` play (`ba41d`) | `0xA7896`, `0xA7A66`, `0xBA4A1` | `a6cdc` `0xA6D2E` |
| `+0x0C` | `0x1619A0` | i32 | pitch (`a780e` randomizes, `a79dc`/`ba41d` zero) | `0xA78C8`/`0xA78CD`, `0xA7AA9`, `0xBA484` | `a6579` `0xA65FA` |
| `+0x10` | `0x1619A4` | u8 | 0x40-centred pan byte for the `a6579` pitch term | `a780e` `0xA787E` (caller-frame byte), `a79dc` `0xA7A48`, `ba41d` `0xBA47C` | `a6579` `0xA65E7` |
| `+0x11` | `0x1619A5` | u8 | pan scale (`desc[0x17]`; 0 retail) | `0xA7884`, `0xA7A4C`, `0xBA480` | `a6579` `0xA65DC` |
| `+0x13` | `0x1619A7` | u8 | stored steal priority | `0xA788A`, `0xA7A5B`, `0xBA48F` | allocator `0xA6367` |
| `+0x16` | `0x1619AA` | u8 | in-use: 1 = active, 0 = free | **set** `a780e` `0xA786E`, `a79dc` `0xA7A57`, `ba41d` `0xBA48B`; **cleared** `ba00e` `0xBA020` | wrappers `0xA6E49` etc. |
| `+0x17` | `0x1619AB` | u8 | global id (`0xFF` = none) | `0xA7890`, `0xA7A3D`, `0xBA471` | — |

`FUN_000a6579` (0xA6579..0xA6628) reads `+0x10`/`+0x11`/`+0x0C` and computes
the first `FUN_000b7fe8` argument as
`record+0xC + (record+0x10 - 0x40) * record+0x11 * 100 / 64`
(`MOVZX [ECX+0x11]; IMUL 0x64; MOVZX [ECX+0x10]; SUB 0x40; IMUL; CDQ; SHL
EDX,6; SBB EAX,EDX; SAR EAX,6; ADD [ECX+0xC]; PUSH EAX`
`0xA65DC..0xA65FD`; the `CDQ/SHL/SBB/SAR` tail is the signed /64 idiom, and
the product never overflows for byte operands). Every retail bank descriptor stores `+0x17 = 0`
(FU-43 §1.1), so `record+0x11 == 0` and the term vanishes; the `+0x10` byte is
otherwise the record's 0x40-centred pan. Its `a780e` writer is unusual: the
value comes from the caller's stack frame, not from a pushed argument (§5.3).

## 1. Sound-system init `FUN_00064f70` (`0x64F70..0x64FFA`)

Disassembly, re-verified against FU-50 §5:

```
0x64F73  CMP dword [0x55CE0],0; JNZ 0x64FF2    ; already up -> return flag
0x64F80  MOV EAX,1; CALL 0x68CFC               ; sound-config query (arg 1)
0x64F8A  MOV [0x55CE4],EAX
0x64F8F  TEST EAX,EAX; JLE 0x64FF2             ; query <= 0 -> early return
0x64F93  MOV EDX,-1; CALL 0xA6265              ; set sound state -1
0x64F9D  TEST EAX,EAX; SETGE DL; AND EDX,0xFF
0x64FA8  MOV [0x55CE0],EDX                     ; up = (a6265 ret >= 0)
0x64FAE  TEST EAX,EAX; JGE 0x64FC0
0x64FB2  CALL 0xA6A03; PUSH EAX; CALL 0xCBBE8  ; error path + print
0x64FC0  MOV ECX,1; XOR EAX,EAX
0x64FC7  ADD EAX,0x4                           ; EAX = 4,8,...,0x18
0x64FCC  MOV [EAX+0x55D14],ECX                 ; flag[i]  = 1
0x64FD2  MOV [EAX+0x55CFC],EBX                 ; current[i] = 0 (EBX=0)
0x64FD8  MOV [EAX+0x55CE4],EBX                 ; pending[i] = 0
0x64FDE  CMP EAX,0x18; JNZ 0x64FC7
0x64FE3  XOR EDX,EDX
0x64FE5  MOV EAX,EDX; INC EDX; CALL 0x6504C     ; apply channel i
0x64FED  CMP EDX,0x6; JL 0x64FE5
0x64FF2  MOV EAX,[0x55CE0]; POP EDX/ECX/EBX; RET
```

Three 6-dword arrays: pending `0x55CE8..0x55CFF`, current `0x55D00..0x55D17`,
flag `0x55D18..0x55D2F` (values 0/0/1). The brief's fact holds; the `i+1`
second argument of the apply call is dead (§2, errata 2).

## 2. Channel setters and the `0xA0AC` handler table

### 2.1 `FUN_00065004(ch, value)` (`0x65004..0x6504B`)

```
0x65006  MOV EBX,EAX                        ; ch
0x65008  MOV EAX,EDX                        ; value
0x6500A  TEST EDX,EDX; JGE 0x65010
0x6500E  XOR EAX,EDX                        ; negative -> 0
0x65010  CMP EAX,0x7F; JLE 0x6501A
0x65015  MOV EAX,0x7F                       ; >0x7F -> 0x7F
0x6501A  LEA EDX,[EBX*4]
0x65021  CMP dword [EDX+0x55D18],0; JZ 0x65043
0x6502A  MOV ECX,[EDX+0xA0AC]               ; handler
0x65030  MOV [EDX+0x55D00],EAX              ; current[ch] = clamped value
0x65036  TEST ECX,ECX; JZ return
0x6503A  CALL dword [EDX+0xA0AC]            ; handler(value) in EAX
0x65043  MOV [EDX+0x55CE8],EAX              ; flag 0 -> pending[ch] only
```

The clamp is 0..0x7F and the handler receives the clamped value in EAX
(the controller brief's `param_1`).

### 2.2 `FUN_0006504c(ch, param)` (`0x6504C..0x65081`)

```
0x6504E  SHL EAX,0x2
0x65051  MOV dword [EAX+0x55D18],1          ; flag[ch] = 1
0x6505B  MOV EDX,[EAX+0x55CE8]              ; pending[ch]  (EDX arg discarded)
0x65061  MOV EBX,[EAX+0xA0AC]               ; handler
0x65067  MOV [EAX+0x55D00],EDX              ; current[ch] = pending[ch]
0x6506D  TEST EBX,EBX; JZ return
0x65071  MOV EDX,EAX                        ; ch*4 (stale for the callee)
0x65073  MOV EAX,[EAX+0x55CE8]              ; pending[ch] is the argument
0x65079  CALL dword [EDX+0xA0AC]
```

The `param` in EDX is overwritten at `0x6505B` and never reaches the handler:
the handler is called with the **pending value** (0 after init). This corrects
the brief's "call handler [0xa0ac+ch*4](param)" (errata 2).

### 2.3 The handler table and channel meanings

The table is static object-1 data, read at image `0x10A0AC`
(`read 0x10A0AC` = `{0, 0x55448, 0x551D8, 0x56D54, 0x56D54, 0x55AF8, 0, 0}`);
the loader relocates each object-1 entry by `+0x10000`, giving the image
handlers `{NULL, 0x65448, 0x651D8, 0x66D54, 0x66D54, 0x65AF8, NULL, NULL}`.
There is no runtime writer — the only xrefs are the six indexed reads/calls in
`FUN_00065004`/`FUN_0006504c`/`FUN_00065084`. Each handler is gated on
`[0x55CE0]` (sound up) and a per-channel enable dword, then routes the value:

| ch | handler | evidence | derived meaning |
| --- | --- | --- | --- |
| 1 | `0x65448` | `CMP [0x55CE0]; CMP [0x55D34];` loops `[0xA128]` registered events calling `FUN_000653d8(event, value)` (`0x65461..0x6547F`); `FUN_000653d8` clamps 0..0x7F, calls `a6b21(handle, value)` on every active event handle and, when `value <= 0`, `a6cdc(handle)` + handle `-1` (`0x653F2..0x65438`) | SFX event bus volume; 0 stops all registered events |
| 2 | `0x651D8` | `CMP [0x55CE0]; CMP [0x55D38]; JMP 0xA7472`; `0xA7472` clamps 0..0x7F into `[0x148F6]` (image `0x1148F6`, static `0x7F`) | music/stream control value consumed by `FUN_000a7499` |
| 3, 4 | `0x66D54` | `CMP [0x55CE0]; CMP [0x55D3C]; JMP 0xA8064`; `0xA8064` stores `value & 0x7F` at `[0x14A99]` and, when `[0x15FCC] != 0`, calls `a6b21([0x14AA5], value)` (the FU-41 §3.2 streaming volume + current stream voice) | streamed-audio volume (both channels share the handler) |
| 5 | `0x65AF8` | `CMP [0x55CE0]; CMP [0x55D44]; MOV EBX,[0xA24C]; CALL a6b21(EBX, value)` (`0x65B0C..0x65B16`); `[0xA24C]` is the sample-bank voice handle written by `FUN_00065920`/`FUN_000659F8` (`0x659B0`/`0x65A87`) | music/phrase voice volume (FU-50 §6's music slider) |
| 0, 6, 7 | `NULL` | table entries 0/6/7 | unused by the init (only 0..5 applied) |

`FUN_00065084` disables a channel (saves current to pending, zeroes current,
calls the handler with 0, `0x65084..0x650D0`, FU-50 §5). `[0x55D04]` (current
channel 1) remains the play gate of `FUN_00065544` (FU-50 §6).

## 3. The stream queue: `FUN_000a7e06` and the `0x5DB98`/`0x5DB9C` list

### 3.1 Node structure

FU-35 §1/§4 already derived the `1SNh`/`1SNd`/`1SNe` companion-stream queue;
this slice re-derives the node from `FUN_000a79dc`/`FUN_000a7d3c`/`eacs_dequeue_next`:

| node offset | width | role | citation |
| --- | --- | --- | --- |
| `+0x00` | u32 | next link; `0` = end of the current chain, `-1` = `1SNe` end marker, `-2` = consumed/released | `MOV EDX,[EAX]` `0xA7E1F`; `MOV [ESI],-1` `0xA7DE1`; `MOV [EBX],-2` `0xA7C56` |
| `+0x04` | u32 | payload byte count (node-relative; the dequeue subtracts the 8-byte header) | `[EAX+0x4]` `0xA7E57`, `SUB EAX,0x8` `0xA7C1B` |
| `+0x08` | EACS | embedded 32-byte EACS header (`"EACS"` at node+8) | `LEA EBX,[EAX+8]; CMP [EBX],'EACS'` `0xA79E6..0xA79E9` |

Head `[0x5DB98]` (image `0x15DB98`) is the **last appended** node; tail
`[0x5DB9C]` (image `0x15DB9C`) is the **next to dequeue**. Append
(`FUN_000a7b2b` `0xA7B2B..0xA7B6A`, inlined in `FUN_000a79dc`
`0xA7AF8..0xA7B1D`): `node[0] = 0` unless already `-1`; if tail == 0 then
tail = node else `head[0] = node`; head = node.

Builders:

* `FUN_000a79dc` (`0xA79DC..0xA7B2A`, the `1SNh` path): validates the EACS
  magic (else state `[0x149CC] = 0`, return -2, `0xA79E9..0xA7A00`) and the
  voice byte 0..15 (else -3, `0xA7A05..0xA7A2F`); initializes the EACSNDF
  record (type 2, priority 0x65, `+0x10 = 0x40`, `+0x17 = 0xFF`,
  `+0x16 = 1`); writes `eacs+0x18 = eacs+0x28` (payload pointer),
  `eacs+0xC = (node[4] - 0x28) / (f8*f9)`; stores the stream globals
  `[0x149D0]` voice, `[0x149D4]` block size, `[0x149D8]` rate,
  `[0x149E4]` header, `[0x149E8] = 0`, `[0x149EC] = (f10==2) ? 4 : 1`; then
  appends if `[0x149CC] != 0` (`0xA7A34..0xA7B23`).
* `FUN_000a7b2b` (`0xA7B2B..0xA7B6A`): appends an arbitrary node; returns -1
  when `[0x149CC] == 0`.
* `FUN_000a7d3c` (`0xA7D3C..0xA7DF7`, FU-35 §1): tag dispatch — `1SNd`
  (`0x644E5331`) clears node[0] and appends; `1SNh` (`0x684E5331`) calls
  `FUN_000a79dc`; `1SNe` (`0x654E5331`) sets node[0] = -1 and appends via
  `FUN_000a7b2b`; `1SNl` (`0x6C4E5331`) runs the two seek callbacks and
  releases the node.

### 3.2 Flush `FUN_000a7e06` (`0xA7E06..0xA7E30`)

```
0xA7E07  MOV EAX,[0x5DB98]                  ; head
0xA7E0C  MOV dword [EAX],0xFFFFFFFE         ; head[0] = -2
0xA7E12  MOV EAX,[0x5DB9C]                  ; tail
0xA7E17  CMP EAX,[0x5DB98]
0xA7E1D  JZ 0xA7E29                         ; tail == head -> done
0xA7E1F  MOV EDX,[EAX]                      ; next = tail[0]
0xA7E21  MOV [0x5DB9C],EDX                  ; tail = next
0xA7E27  JMP 0xA7E0C                        ; [old tail] = -2
0xA7E29  MOV dword [EAX],0xFFFFFFFE         ; head again
```

So it writes -2 to the head, then follows the links from tail to head writing
-2 to every node, and leaves `tail == head` (the head/tail variables are not
zeroed). `-2` is the queue's consumed/released marker: the dequeue sets it on
each node it advances past (`MOV [EBX],0xFFFFFFFE` `0xA7C56`) and `FUN_000a7d3c`
uses it as a released-node tag. Callers: `FUN_000a6cdc` when the stopped
voice's record type is 2 (`CALL 0xA7E06` `0xA6D4E`, FU-50 §2.5) and the
per-frame voice tick `FUN_000b6ab3` (`CALL 0xA7E06` `0xB6AF4`, FU-35 §4).
`FUN_000a7ca4` resets the system (state `[0x149CC] = 3`, `[0x149D0] = -1`,
tail = head = 0, `[0x149E8] = 0`); `eacs_dequeue_next` (`0xA7BD7`, far,
stack out-params) advances the tail, returns `(node+8, (node[4]-8)/stride)`,
increments `[0x149E8]` and marks the consumed node -2; `FUN_000a7e31` reports
position from the same list (FU-35 §1).

## 4. Play-by-id `FUN_000a7728` (`0xA7728..0xA780D`)

### 4.1 Entry registers

`EAX = id`, `EDX = caller pan`, `EBX = caller volume`, `ECX` = dead (pushed by
the arm call and discarded). The wrappers pin the order:

```
FUN_000a76ea: EAX=id; MOV ECX,0x40; MOV EBX,0x7F; MOV EDX,-1; CALL a7728
              (0xA76ED..0xA76FC)
FUN_000a7705: EAX=id; MOV ECX,0x40; MOV EBX,EDX; MOV EDX,-1; CALL a7728
              (0xA7707..0xA7713)
0xA771B:      EAX=id; EDX=pan; EBX=volume; PUSH ECX; MOV ECX,0x40; CALL a7728
              (0xA771B..0xA7721)
```

### 4.2 Dispatch

```
0xA7738  TEST ESI,ESI; JL 0xA7744           ; ESI = id
0xA773C  CMP ESI,0x80; JC 0xA774D
0xA7744  MOV EAX,0xFFFFFFED                 ; -19
0xA774D  MOV EDX,ESI; SHL EDX,0x2
0xA7752  MOV EDI,[EDX+0x61C14]              ; descriptor
0xA7758  MOV [EBP-0x14],EDX                 ; id*4
0xA775B  TEST EDI,EDI; JZ 0xA7744
0xA775F  MOV DL,[EDI+0x1C]; AND DL,1; MOVZX EDX,DL
0xA7768  TEST EDX,EDX; JZ 0xA77FB           ; flag clear -> single
```

Single-voice arm (`0xA77FB..0xA7805`): `PUSH ECX` (dead), `EDX = id`,
`EAX = desc`, `ECX = EBX` (volume), `EBX = [EBP-0x10]` (pan), `CALL a780e`;
return its result directly.

Two-voice (`0xA7770..0xA77FA`):

```
0xA7770  MOV EAX,[EBP-0x10]                 ; pan
0xA7773  MOV EDX,EBX                        ; volume
0xA7775  CALL 0xA6717                       ; split -> local<<16|left<<8|right
0xA777A  MOV EDX,EAX; AND EDX,0xFF          ; [EBP-0x18] = right
0xA7782  SAR EAX,0x8                        ; [EBP-0x4]  = local (pan)
0xA778B  MOV ECX,EAX; AND ECX,0xFF          ; left = id's volume
0xA778D  PUSHFD; CLI                        ; interrupt critical section
0xA7793  PUSH [EBP-0xC]                     ; dead
0xA779C  EDX=ESI (id); EAX=EDI (desc); EBX=[EBP-0x4] (pan); CALL a780e
0xA77AB  TEST EAX,EAX; JGE 0xA77B7
0xA77AF  POPFD; return the first-arm error
0xA77B7  MOV EAX,[EBP-0x14]; MOV EAX,[EAX+0x61C18]   ; table[id+1]
0xA77C0  TEST EAX,EAX; JNZ 0xA77CA
0xA77C4  POPFD; JMP 0xA7744                 ; -19, first voice stays armed
0xA77CA  PUSH [EBP-0xC]; ECX=[EBP-0x18] (right); EBX=[EBP-0x4] (pan)
0xA77D3  LEA EDX,[ESI+1]; CALL a780e
0xA77DD  POPFD
0xA77DE  TEST EDI,EDI; JGE 0xA77EF
0xA77E2  MOV EAX,ESI; CALL 0xA6CDC          ; cleanup id's voice
0xA77E9  return the second-arm error
0xA77EF  MOV EAX,EDI; SHL EAX,0x10; OR EAX,[EBP-0x8]   ; (v2<<16)|v1
```

`[0x61C18 + id*4]` is the **next slot of the same table** (0x61C14 + 4), i.e.
`DAT_00061c14[id+1]`, which may live in a different registered bank (§6). The
second voice gets `EBX` = split local (pan, identical to the first) and `ECX` =
split right (volume). A null id+1 descriptor returns -19 with **no cleanup**,
leaving the first voice armed; only a failed second arm runs `FUN_000a6cdc(id)`.

## 5. Start-one-voice `FUN_000a780e` (`0xA780E..0xA799F`)

The decompiler prunes most of this body; the disassembly is the spec. Entry:
`EAX = desc`, `EDX = id`, `EBX = caller pan` (`-1` = descriptor fallback),
`ECX = caller volume`; `RET 0x4` pops one dead stack argument.

### 5.1 Order and return codes

```
0xA781F  MOVZX EDX,[EAX+0x14]               ; priority
0xA7823  MOV EAX,[EAX]                      ; voice bitmask
0xA7825  CALL 0xA62FA                       ; allocator (FU-47 §1)
0xA782A  MOV EDI,EAX; TEST EAX,EAX; JNZ 0xA783A
0xA7830  MOV EAX,0xFFFFFFEC                 ; -20 (alloc failed)
0xA783A  MOV EDX,[ESI+4]; MOV [EBP-0x10],EDX ; EACS pointer
0xA7840  CMP dword [EDX],0x53434145; JZ 0xA7852
0xA7848  MOV EAX,0xFFFFFFF6                 ; -10 (bad EACS magic)
0xA7852  CMP [0x15FC8],0; JLE 0xA7864
0xA785B  CMP [0x15FC8],5; JLE 0xA786E
0xA7864  MOV EAX,0xFFFFFFFC                 ; -4 (sound-state gate)
```

So: alloc failure `-0x14`, EACS magic mismatch `-10` (`0xFFFFFFF6`), gate
failure `-4`; the arm returns `(s8)record+0x12` (the chosen voice index) on
success (`MOVSX EAX,byte [EDI+0x12]` `0xA7998`). The allocator runs **before**
the magic and gate checks, so a blocked arm still moves the rotor and writes
`record+0x12` (FU-47 §1).

### 5.2 Record fill (`0xA786E..0xA7975`)

```
0xA786E  MOV byte [EAX+0x16],1              ; in-use
0xA7872  MOV [EAX],ESI                      ; +0x00 descriptor
0xA7874  MOV EDX,[ESI+8]; DEC EDX; MOV [EAX+4],EDX   ; +0x04 desc[8]-1
0xA787B  MOV DL,[EBP+0x10]; MOV [EAX+0x10],DL        ; +0x10 (see 5.3)
0xA7881  MOV DL,[ESI+0x17]; MOV [EAX+0x11],DL        ; +0x11 pan scale
0xA7887  MOV DL,[ESI+0x14]; MOV [EAX+0x13],DL        ; +0x13 priority
0xA788D  MOV DL,[EBP-0xC]; MOV [EAX+0x17],DL         ; +0x17 id
0xA7893  MOV DL,[ESI+0x1C]; MOV [EAX+0xB],1; MOV [EAX+0xA],DL  ; type/flags
```

then the descriptor pitch/volume/pan resolution (identical to the port's
`sfx_desc_pitch`/`sfx_desc_volume`/`sfx_desc_pan`, `0xA789D..0xA796F`),
`record+0x24 = ECX` (caller volume, `0xA7972`), `record+0x18 = 0`
(`0xA797F`), `FUN_000b9fdd` computes `record+0x26` (`0xA7986`),
`eacs[0x1C] = record+0x12` (`0xA798B..0xA798E`) and `FUN_000a6579(eacs)`
arms the mixer format (`0xA7993`, FU-43 §3.2).

### 5.3 The `+0x10` caller-frame byte

`0xA787B MOV DL,[EBP+0x10]` reads the dword **above** the single dead stack
argument. With `ENTER 0x14,0`, `[EBP+8]` is the dead argument (never read,
popped by `RET 0x4`), `[EBP+0xC]` is the caller's local at its `EBP-0x18`, and
`[EBP+0x10]` is the caller's local at its `EBP-0x14`:

* from `FUN_000a7728` (`ENTER 0x18`), `[EBP-0x14] = id*4` (`0xA7758`), so
  `record+0x10 = (u8)(id << 2)` for every bank arm;
* from `FUN_000a7499` (`ENTER 0x8`), `[EBP-0x4] = 0x5D884 + i*0x74` (the
  descriptor pointer), so `record+0x10 = (u8)descriptor`.

`record+0x10` is only read by `FUN_000a6579` through the `+0x11` scale term
(§0), and retail `desc[0x17] == 0` zeroes it. The fourth `a780e` caller
`FUN_000a7499` (`CALL 0xA780E` `0xA74FA`) is a mass-arm loop over
`[0x5D82C]` records of stride 0x74 at `0x5D884`, storing the returned voice at
`0x5D8D4 + i*0x74`; its table's role stays open (§9).

### 5.4 The `+0x16` writer resolution (FU-50 open leg 2)

The in-use byte is set to 1 by `a780e` at `0xA786E` after the gate, and
cleared by `FUN_000ba00e` at `0xBA020` (`MOV byte [EAX+0x16],0`), the
EACSNDF-record reset reached from the stop path `FUN_000b80fa` (called by
`a6cdc`, `0xB810A`) and from the sound-state stop `FUN_000a6505`
(`0xA6542..0xA654F`, FU-47 §3). The other type arms set it the same way
(`a79dc` `0xA7A57` type 2, `ba41d` `0xBA48B` type 3); `FUN_000ba00e` resets
the record to `{+0x16=0, +0x04=-1, +0x00=0, +0x17=0xFF, +0x18=0, +0x27=0x40,
+0x25=0x7F, +0x24=0x7F}` (`0xBA020..0xBA044`). FU-50 open leg 2 is closed.

## 6. Descriptor-table population status

* `DAT_00061c14` (image `0x161C14`) is BSS zero in the static image
  (`read 0x161C14` = 0) and is populated by the FU-43 registrar `FUN_000a75aa`:
  for each nonzero slot `i` of the 128-entry file table it stores the
  relocated descriptor pointer `MOV [EAX+0x61C14],EBX` with `EAX = i*4`
  (`0xA7622`), rejecting duplicates (`0xA7611..0xA7618`); the bank slot is
  stored at `[0x149B8 + slot*4]` (`0xA7651`). `FUN_000a765f` clears each slot
  of an unregistered bank (`MOV [EBX+0x61C14],0` `0xA76B4`). **Verified: the
  registrar is the populator.**
* `0x61C18` is **not a second table**: it is `DAT_00061c14 + 4`. The only
  reader, `FUN_000a7728` `0xA77BA`, indexes it with `id*4` (`[EBP-0x14]`), so
  `[0x61C18 + id*4] == DAT_00061c14[id+1]`. The brief's "128 entries each"
  is errata 4.
* `0x9F78` (image `0x109F78`) is a **static** object-1 dword array of
  pointers to 16-byte bank names (`read 0x109F78` = `0x2514, 0x2524, 0x2534,
  ...` -> image `0x102514` `"CHN_ARG0.BNK"`, FU-49 §0); `FUN_000652f0` reads
  it at `0x65313` (`MOV EDX,[EDX*4+0x9F78]`) to copy the bank name. It is
  relocated at load time by the LE loader, not runtime-built (errata 4).

## 7. Port mapping

New API in `fifa96_sfx.h/.c` (existing APIs unchanged):

| port | original |
| --- | --- |
| `struct fifa96_sfx_id_table.bank[128]` | `DAT_00061c14` (descriptor per id; NULL = absent) |
| `fifa96_sfx_play_id(m, ids, id, opts, out)` | `FUN_000a7728`: id/slot checks (`-NOT_FOUND` = original `-0x13`), descriptor `+0x1C` bit 0 dispatch, single arm through the existing `a780e` chain, two-voice split `FUN_000a6717` (`sfx_split`), first arm then `table[id+1]`, second-arm failure stops the first voice (`FUN_000a6cdc` analogue), packed `(v2<<16)|v1` |
| `sfx_arm_alloc_one` (internal) | `FUN_000a780e` allocator-first arm, reused by `fifa96_sfx_arm_alloc` and `fifa96_sfx_play_id` |

Faithfulness notes: the port keeps the original's non-atomic path — a null
`table[id+1]` returns `-NOT_FOUND` with the first voice still armed
(`0xA77C4` has no cleanup) — while a failed second arm stops it. The
`id == 127` two-voice case reads past the original table (slot 128); the port
returns `-NOT_FOUND` without arming the second voice.

Not ported (documented instead):

* the stream queue (§3) and its `1SNd/1SNe/1SNl` producers depend on the TGV
  companion stream, the runtime dequeue caller and the stream globals; FU-35
  already specifies them and they stay unported.
* the six channel handlers (§2) depend on the event registry (`0xA128`), the
  music/sample-bank globals (`0x148F6`, `0x14A99`, `0xA24C`) and the `.VIV`
  streaming state; only the array layout and handler meanings are documented.
* `FUN_000a64f70`'s channel-array init is covered by
  `fifa96_voice_registry_init` (FU-50 §7) at the record level; the channel
  arrays themselves are not modelled.

Tests: `tests/test_sfx_play_id.c` pins invalid id / null slot / absent bank
slot (`-NOT_FOUND`), the golden single-voice path, the two-voice packed result
and split volumes, a second descriptor from a different bank slot, the
first-voice-left-armed path, the second-arm cleanup (`-BAD_MAGIC` -> first
voice stopped), the flag-clear single path, ids 0/0x7F/0x80 boundaries and
the gate/opts guards.

## 8. Errata (quoted brief facts vs the disassembly)

1. Brief: "`FUN_000a780e(?, id, vol)` ... NULL → -20; `**(int**)(desc+4) ==
   0x53434145` ("EACS" magic) → -4 else -10". Actual: the magic **mismatch**
   is `0xFFFFFFF6` (-10, `0xA7848`); the sound-state gate is
   `0xFFFFFFFC` (-4, `0xA7864`). There is no -4 on the magic path.
2. Brief: "`FUN_0006504c(ch, param)` ... call handler `[0xa0ac+ch*4](param)`".
   Actual: `param` (EDX) is discarded at `0x6505B`; the handler receives
   `pending[ch]` (`MOV EAX,[EAX+0x55CE8]; CALL [EDX+0xA0AC]`
   `0x65073..0x65079`). The init's `FUN_0006504c(i, i+1)` second argument is
   dead for the same reason.
3. Brief: "`FUN_000a7705` ... = `FUN_000a7728(0x40)` (0x40 = the vol/flags
   arg)". Actual: `0xA7705` sets `ECX = 0x40` (a dead argument pushed and
   ignored by `a780e`), `EBX = EDX` is the caller **volume**, `EDX = -1` the
   pan (`0xA7707..0xA7713`). `0xA76EA` is the all-defaults wrapper
   (`EDX=-1, EBX=0x7F, ECX=0x40`).
4. Brief: "Descriptor tables `0x61c14`/`0x61c18` (128 entries each) and
   `0x9f78` (id→name) are zero/code statically — runtime-built". Actual:
   there is **one** descriptor table at `0x61c14` (BSS, populated by
   `FUN_000a75aa` `0xA7622`); `0x61c18 = 0x61c14 + 4`, read as `table[id+1]`;
   `0x9f78` is static object-1 data (name pointers, image `0x109F78`),
   relocated at load, not built at runtime.
5. Brief: "two-voice: `FUN_000a780e(split_hi, id, vol)`; if
   `[0x61c18+id*4] != 0` start second voice `FUN_000a780e(split_lo, id+1,
   vol)`". Actual register roles: `a780e(EAX=desc, EDX=id, EBX=pan,
   ECX=volume)`; both arms pass `EBX = split local` (the pan), the first
   `ECX = split left` and the second `ECX = split right` (`0xA777A..0xA77A0`,
   `0xA77CA..0xA77D3`). The second descriptor is `table[id+1]` and can come
   from a different bank.
6. Brief: "failure calls `FUN_000a6cdc` cleanup" for the two-voice path.
   Actual: only a **second-arm failure** calls it (`0xA77E2`); a missing
   `table[id+1]` returns -19 with no cleanup (`0xA77C4`).
7. Brief: "`FUN_000a7e06` ... walks head `PTR_DAT_0005db98` / tail
   `PTR_DAT_0005db9c` writing `0xFFFFFFFE` (-2) to each node's first dword".
   Actual: it writes -2 to the **head** first, then walks from **tail** to
   head (following `node[0]`) writing -2 to each, leaving `tail == head`; the
   head/tail pointers are not cleared (`0xA7E07..0xA7E30`). `-2` is the
   consumed/released link marker (also written by `eacs_dequeue_next`
   `0xA7C56`).
8. Brief: "`FUN_000a7e06` ... derive ... who builds it". Actual: the list is
   built by `FUN_000a79dc` (1SNh EACS arm), `FUN_000a7b2b` (append) and
   `FUN_000a7d3c` (tag dispatch), not by the mixer; FU-35 §1 already
   documents the tags (errata only for "builds it" being open).
9. Brief: "`FUN_000a780e` ... does it set `[voice+0x16]=1`? the +0x16 writer
   leg from FU-50". Actual: yes, `MOV byte [EAX+0x16],1` at `0xA786E`; the
   complementary clearer is `FUN_000ba00e` at `0xBA020`, closing FU-50 open
   leg 2. Also note `a780e` writes `+0x10` from the caller's frame (§5.3),
   not from an explicit argument.

## 9. Open legs

1. **`FUN_000a7499`'s stride-0x74 table.** The fourth `a780e` caller arms
   `[0x5D82C]` records at `0x5D884` (stores the returned voice at
   `0x5D8D4 + i*0x74`, clears `0x1498E`/`0x1499E`), gated by `[0x148F4]` and
   the value clamped into `[0x148F6]`; whether those records are EACSNDF
   descriptors and what the subsystem is (the music/stream mass-arm path) is
   not traced. The `record+0x10` byte for this caller is the descriptor
   pointer low byte.
2. **The `[EBP+0x10]` fifth input.** No caller sets it as a named argument;
   `a7728` and `a7499` happen to leave `id*4` / the descriptor pointer in the
   caller-frame slot. Its only consumer (`FUN_000a6579` via `record+0x11`) is
   zero in every retail bank (`desc[0x17] == 0`), so retail playback is
   unaffected.
3. **Two-voice static-only.** No retail descriptor sets `+0x1C` bit 0
   (FU-43 §4), and the `id == 127` case reads `0x61E14` (past the 128-slot
   table) in the original; the port rejects instead.
4. **Stream queue producer/dequeue caller.** `eacs_dequeue_next` still has no
   static caller (FU-35 open leg 3); the TGV companion poll and the
   sample-bank path are the capture targets.
5. **Channel enable flags.** `0x55D34` (ch1), `0x55D38` (ch2), `0x55D3C`
   (ch3/4) and `0x55D44` (ch5) gate the handlers; their writers and the UI
   path that drives them are not traced (FU-50 §6 / FU-49 §2 cover the
   channel-5 slider).
6. **`record+0x08`.** `FUN_000ba41d` copies `record+0x8` to `eacs+0x1D` but
   no writer in this slice sets it (the bank arm writes `+0x04/+0x0A/+0x0B/
   +0x0C/+0x10/+0x11/+0x13/+0x16/+0x17`); its producer is outside the frame.

## 10. Provenance (Ghidra, 2026-10-05, `/fifa96_le.bin`)

* Disassembled: `FUN_00064f70`, `FUN_00065004`, `FUN_0006504c`, `FUN_00065448`,
  `FUN_000651d8`, `FUN_00066d54`, `FUN_00065af8`, `FUN_000653d8`, `0xA7472`,
  `0xA8064`, `FUN_000a75aa`, `FUN_000a765f`, `FUN_000a7728`, `FUN_000a76ea`,
  `FUN_000a7705`, `0xA771B`, `FUN_000a780e`, `FUN_000a7e06`, `FUN_000a79dc`,
  `FUN_000a7b2b`, `FUN_000a7d3c`, `eacs_dequeue_next` (`0xA7BD7`),
  `FUN_000a7e31`, `FUN_000a7ca4`, `FUN_000a7499`, `FUN_000a6579`,
  `FUN_000ba41d`, `FUN_000ba00e`, `FUN_000b6ab3` call site.
* Xrefs: `0xA780E` (4 callers: `0xA77A3`, `0xA77D6`, `0xA7805`, `0xA74FA`),
  `0xA7E06` (2: `0xA6D4E`, `0xB6AF4`), `0x61C14` (5: `0xA7752`, `0xA7611`,
  `0xA7622`, `0xA762E`, `0xA76B4`), `0x61C18` (2: `0xA77BA`, `0xA76B4`),
  `0x5DB98`/`0x5DB9C` (13/16), `0xA0AC` (6 indexed accesses), `0xA24C` (9).
* Image reads: `0x10A0AC` (handler table `{0,0x55448,0x551D8,0x56D54,0x56D54,
  0x55AF8,0,0}`), `0x161C14` (zero), `0x109F78` (name pointers `0x2514...`),
  `0x1148F6` (`0x7F`), `0x1148F4` (0).
* Baseline: `make test` 41/41 before the port; 42/42 after it (new
  `tests/test_sfx_play_id.c`); ASan+UBSan build of the audio tests
  (`detect_leaks=0`) clean. This doc is the first tracked change of the slice.
