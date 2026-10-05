# FU-50 — voice-control wrappers and the per-voice gain chain

Date: 2026-10-05. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the five `0xA6xxx` voice-control wrappers behind the FU-49 event
registry — ready/active (`FUN_000a6e1f`), set-volume (`FUN_000a6bb3`),
set-pan (`FUN_000a6b21`), start-slide (`FUN_000a6aa6`), stop/release
(`FUN_000a6cdc`) — their error codes and record-field writes, the per-voice
gain chain `FUN_000b9fdd -> FUN_000b811b -> FUN_000a66ab -> mixer
+0x64/+0x68`, the sound-system init `FUN_00064f70` and its six channel
arrays, and the `[0x55d04]` play gate's producer. Companions: FU-43 (`.BNK`
record/arm), FU-46 (pan/gain calibration), FU-47 (allocator, RNG, sound-state
gate), FU-48 (audio edge cases), FU-49 (event registry and settings).
Every claim cites an instruction address or an image read.

## 0. Address frame and the voice record

Code addresses are flat image addresses (Ghidra listing). Data operands inside
the code are stored object-relative and relocated by the LE loader (FU-41 §0,
FU-4 §0); the listing shows them as written, so object-4 data resolves to
image + `0x100000` and object-1 data to image + `0x10000`. Examples used
below: EACSNDF record base `0x61994` -> image `0x161994`, sound-state gate
`0x15FC8` -> image `0x115FC8`, master volume `0x15FD6` -> image `0x115FD6`,
mixer voice pointer table `0x406D8` -> image `0x1406D8`, channel handler
table `0xA0AC` -> image `0x10A0AC`.

The wrappers all address the same 16 EACSNDF records
(`voice = 0x61994 + handle*0x28`, `IMUL EAX,EAX,0x28` e.g. `0xA6E46`,
`0xA6BEB..0xA6BF3`, `0xA6AD4`, `0xA6D0F..0xA6D12`, `0xB9FE0..0xB9FE8`).
Fields resolved by this slice:

| stored | image | width | role | writer(s) | reader(s) |
| --- | --- | --- | --- | --- | --- |
| `+0x0B` | `0x16199F` | u8 | type; `2` = stop runs the ring/fade cleanup | arm path (outside slice) | `FUN_000a6cdc` `0xA6D2E` |
| `+0x16` | `0x1619AA` | u8 | in-use state: `1` = active, `0` = free | arm `MOV [EAX+0x16],1` `0xA786E` (FU-47) | `0xA6E49`, `0xA6C0D`, `0xA6B78`, `0xA6AD7`, `0xA6D18` |
| `+0x18` | `0x1619AC` | i32 | slide step (`(target - s8[+0x24]) << 16 / frames`) | `FUN_000a6aa6` `0xA6B1A` | slide consumer (outside slice) |
| `+0x1C` | `0x1619B0` | i32 | slide start `(s8)[+0x24] << 16` | `FUN_000a6aa6` `0xA6B09` | slide consumer |
| `+0x20` | `0x1619B4` | i32 | slide target `target << 16` | `FUN_000a6aa6` `0xA6AFF` | slide consumer |
| `+0x24` | `0x1619B8` | u8 (signed read) | caller-volume factor: `b9fdd` multiplier; written by `FUN_000a6b21` | `0xA6B8A`; master refresh `0xA6DD0` | `0xA6B02/0xA6B0C` (s8), `0xB9FEE` (s8), `0xA6DC9` (s8) |
| `+0x25` | `0x1619B9` | u8 (signed read) | descriptor volume after randomization | arm path (FU-43/FU-46) | `0xB9FEA` (s8) |
| `+0x26` | `0x1619BA` | u8 (signed read) | computed gain `master*scale*caller/0x3F01` | `FUN_000b9fdd` `MOV [EBX+0x26],AL` `0xBA007` | `0xB8130` push `0xA6C29/0xA6B94` (s8), `FUN_000a662c` (FU-46) |
| `+0x27` | `0x1619BB` | u8 | pan byte fed to `FUN_000a66ab` (mirror at 0x7F, split at 0x40); written by `FUN_000a6bb3` | `0xA6C1F` | `0xB8133` push `0xA6C31/0xA6B9C` (MOVZX) |

The two control wrappers write the *opposite* field their caller-side names
suggest: `FUN_000a6bb3` (reached from the FU-49 event-volume path
`FUN_00065488` `0x654C3`) writes `+0x27`, which `FUN_000a66ab` treats as the
pan; `FUN_000a6b21` (reached from the FU-49 stop path `FUN_000653d8`
`0x65420`, and from the master refresh `0xA6DD0`) writes `+0x24`, which
`FUN_000b9fdd` treats as a volume factor. See §8 errata 3.

## 1. Wrapper table

All wrappers take the handle in EAX. `FUN_000a6aa6` additionally reads EDX =
frames and EBX = target (§2.4); the others take EDX = parameter. Common
prologue (each cited in §2): handle valid iff `0 <= handle < 0x10`, else
return `0xFFFFFFF5` (-11); sound-state gate `[0x15FC8]` must be signed 1..5,
else `0xFFFFFFFC` (-4); then `voice = 0x61994 + handle*0x28`.

| wrapper | args | per-wrapper gates | error codes | writes | calls | returns |
| --- | --- | --- | --- | --- | --- | --- |
| `FUN_000a6e1f` ready/active | handle | — | -11, -4 | none | none | `0` if `[+0x16]==1`, else `1` |
| `FUN_000a6bb3` set-volume | handle, vol | vol `0..0xFF` | -11, -4, -14 (range), -13 (state) | `[+0x27] = (u8)vol` | `0xB9FDD` (handle), `0xB811B` (handle, `[+0x27]`, `[+0x26]`) | `0` |
| `FUN_000a6b21` set-pan | handle, pan | pan `0..0x7F` | -11, -4, -12 (range), -13 (state) | `[+0x24] = (u8)pan` | `0xB9FDD` (handle), `0xB811B` (handle, `[+0x27]`, `[+0x26]`) | `0` |
| `FUN_000a6aa6` start-slide | handle, frames (EDX), target (EBX) | `[+0x16]==1`; `frames <= 0 -> 1` | -11, -4, -1 (state) | `[+0x20]`, `[+0x1C]`, `[+0x18]` | none | `0` |
| `FUN_000a6cdc` stop/release | handle | `[+0x16]!=0` | -11, -4, -13 | `[0x149CC]=0`, `[0x149E8]=0` when type==2 | `0xB80FA` (handle); `0xA7E06` when type==2 | `0` |

`FUN_000a6e1f` has no state check beyond `== 1`; `[+0x16]` values 2..255 also
return `1` (`CMP EAX,1 / JNZ 0xA6E58` `0xA6E50..0xA6E53`). It is called by
`FUN_00065544` with the *old* handle (`0x6557F`), where nonzero means "start
again". The wrappers preserve `EBX/ECX/EDX/ESI` as needed for their callers'
stale registers (§2.2).

## 2. Per-wrapper disassembly

### 2.1 Ready/active `FUN_000a6e1f` (`0xA6E1F..0xA6E5D`)

```
0xA6E1F  TEST EAX,EAX; JL 0xA6E28
0xA6E23  CMP EAX,0x10; JL 0xA6E2E
0xA6E28  MOV EAX,0xFFFFFFF5            ; -11
0xA6E2E  CMP dword [0x15FC8],0; JLE 0xA6E40
0xA6E37  CMP dword [0x15FC8],5; JLE 0xA6E46
0xA6E40  MOV EAX,0xFFFFFFFC            ; -4
0xA6E46  IMUL EAX,EAX,0x28
0xA6E49  MOVZX EAX,byte [EAX+0x619AA]  ; +0x16
0xA6E50  CMP EAX,1; JNZ 0xA6E58
0xA6E55  XOR EAX,EAX                   ; 0
0xA6E58  MOV EAX,1                     ; 1
```

### 2.2 Set-volume `FUN_000a6bb3` (`0xA6BB3..0xA6C47`)

```
0xA6BB3  PUSH EBX; PUSH ECX; ENTER 0x4,0
0xA6BB9  MOV EBX,EAX                   ; handle
0xA6BBB  MOV [EBP-0x4],EDX             ; vol
         handle range -> -11            ; 0xA6BBE..0xA6BC7
         gate 1..5 else -4              ; 0xA6BD0..0xA6BE2
0xA6BEB  IMUL EDX,EAX,0x28; MOV EAX,0x61994; ADD EAX,EDX
0xA6BF5  CMP dword [EBP-0x4],0xFF; JG 0xA6C04
0xA6BFE  CMP dword [EBP-0x4],0x0;  JGE 0xA6C0D
0xA6C04  MOV EAX,0xFFFFFFF2            ; -14
0xA6C0D  CMP byte [EAX+0x16],0; JNZ 0xA6C1C
0xA6C13  MOV EAX,0xFFFFFFF3            ; -13
0xA6C1C  MOV CL,[EBP-0x4]
0xA6C1F  MOV [EAX+0x27],CL             ; pan field
0xA6C22  MOV EAX,EBX
0xA6C24  CALL 0xB9FDD                  ; recompute +0x26
0xA6C29  MOVSX EAX,byte [EDX+0x619BA]  ; +0x26 (EDX = handle*0x28)
0xA6C30  PUSH EAX
0xA6C31  MOVZX EAX,byte [EDX+0x619BB]  ; +0x27
0xA6C38  PUSH EAX
0xA6C39  PUSH EBX
0xA6C3A  CALL 0xB811B                  ; (handle, +0x27, +0x26)
0xA6C42  XOR EAX,EAX                   ; 0
```

The range check precedes the state check (`0xA6BF5` then `0xA6C0D`), so an
out-of-range volume returns -14 even when the voice is free. `FUN_000b9fdd`
preserves EDX (`PUSH EDX` `0xB9FDF`, `POP EDX` `0xBA00A`), which is why
`0xA6C29` can use the stale `handle*0x28` as the record base.

### 2.3 Set-pan `FUN_000a6b21` (`0xA6B21..0xA6BB2`)

Identical shape; the only differences: range bound `0x7F` and error
`0xFFFFFFF4` (-12) at `0xA6B63..0xA6B6F`; store `MOV [EAX+0x24],CL` at
`0xA6B8A`; `CALL 0xB9FDD` `0xA6B8F`; `CALL 0xB811B` `0xA6BA5` with the same
`(+0x27, +0x26)` argument pair. Range (`0xA6B63`) again precedes state
(`0xA6B78`).

### 2.4 Start-slide `FUN_000a6aa6` (`0xA6AA6..0xA6B20`)

Entry: EAX = handle, EDX = frames, EBX = target. Prologue
`PUSH ECX; MOV ECX,EDX; MOV EDX,EBX` (`0xA6AA6..0xA6AA9`) moves frames into
ECX and target into EDX, so the original signature is
`FUN_000a6aa6(handle, frames, target)`. Call sites confirm: `FUN_00065b28`
sets `MOV EBX,0xFFFFFFFF` (target = -1) then `MOV EDX,ESI` (frames) before
`CALL 0xA6AA6` (`0x65B40..0x65B4C`); `FUN_00067e94` sets `MOV EBX,0xFFFFFFFF`
then `MOV EDX,0x64` (100 frames) (`0x67ECB..0x67EDA`). The remaining callers
are `FUN_00065510` (event set-param, which leaves EBX to its own caller),
`FUN_00065b70` and `FUN_00065ba0`.

```
0xA6AAB  handle range -> -11           ; 0xA6AAB..0xA6AB9
0xA6ABB  gate 1..5 else -4             ; 0xA6ABB..0xA6AD3
0xA6AD4  IMUL EAX,EAX,0x28
0xA6AD7  MOVZX EBX,byte [EAX+0x619AA]  ; +0x16
0xA6ADE  CMP EBX,1; JZ 0xA6AEA
0xA6AE3  MOV EAX,0xFFFFFFFF            ; -1
0xA6AEA  MOV EBX,0x61994; ADD EBX,EAX  ; record
0xA6AF1  TEST ECX,ECX; JG 0xA6AFA
0xA6AF5  MOV ECX,1                     ; frames <= 0 -> 1
0xA6AFA  MOV EAX,EDX                   ; target
0xA6AFC  SHL EAX,0x10
0xA6AFF  MOV [EBX+0x20],EAX            ; target << 16
0xA6B02  MOVSX EAX,byte [EBX+0x24]
0xA6B06  SHL EAX,0x10
0xA6B09  MOV [EBX+0x1C],EAX            ; (s8)caller << 16
0xA6B0C  MOVSX EAX,byte [EBX+0x24]
0xA6B10  SUB EDX,EAX                   ; target - (s8)caller (32-bit)
0xA6B12  MOV EAX,EDX
0xA6B14  SHL EAX,0x10
0xA6B17  CDQ
0xA6B18  IDIV ECX                      ; signed / frames
0xA6B1A  MOV [EBX+0x18],EAX            ; step
0xA6B1D  XOR EAX,EAX                   ; 0
```

The numerator is the 32-bit `(target - s8[+0x24]) << 16` sign-extended by
`CDQ`; `IDIV` truncates toward zero and frames is at least 1.

### 2.5 Stop/release `FUN_000a6cdc` (`0xA6CDC..0xA6D59`)

```
0xA6CDC  PUSH EBX; PUSH ECX; PUSH EDX; PUSH ESI
         handle range -> -11           ; 0xA6CE0..0xA6CF2
         gate 1..5 else -4             ; 0xA6CF3..0xA6D0E
0xA6D0F  IMUL ESI,EAX,0x28; ADD ESI,0x61994
0xA6D18  CMP byte [ESI+0x16],0; JNZ 0xA6D28
0xA6D1E  MOV EAX,0xFFFFFFF3            ; -13
0xA6D28  PUSH EAX
0xA6D29  CALL 0xB80FA                  ; free the mixer voice
0xA6D2E  MOVZX EAX,byte [ESI+0xB]      ; type (ESI preserved by b80fa)
0xA6D35  CMP EAX,2; JNZ 0xA6D53
0xA6D3A  MOV dword [0x149CC],0
0xA6D44  MOV dword [0x149E8],0
0xA6D4E  CALL 0xA7E06                  ; ring/fade cleanup (FU-41 arm ring)
0xA6D53  XOR EAX,EAX                   ; 0
```

`FUN_000b80fa` (`0xB80FA..0xB811A`) resolves the same mixer pointer table
`[0x406D8+handle*4]`, clears `byte [esi]` (the mixer channel state) and calls
the hardware free `0xBA00E(handle)`; it preserves ESI/EBX/EAX. The EACSNDF
record's `+0x16` is **not** cleared by this wrapper (no write to it in the
body); the only writer in this slice's frame is the arm's `0xA786E`.

## 3. Per-voice gain chain

### 3.1 `FUN_000b9fdd` — combined gain byte (`0xB9FDD..0xBA00D`)

```
0xB9FE0  IMUL EAX,EAX,0x28
0xB9FE3  MOV EBX,0x61994; ADD EBX,EAX
0xB9FEA  MOVSX ECX,byte [EBX+0x25]     ; s8 scale
0xB9FEE  MOVSX EAX,byte [EBX+0x24]     ; s8 caller
0xB9FF2  IMUL ECX,EAX
0xB9FF5  MOVSX EAX,byte [0x15FD6]      ; s8 master
0xB9FFC  IMUL EAX,ECX
0xB9FFF  MOV ECX,0x3F01
0xBA004  CDQ
0xBA005  IDIV ECX                      ; signed division
0xBA007  MOV [EBX+0x26],AL             ; low byte, no clamp
```

So `[+0x26] = (u8)( (s8)master * (s8)[+0x25] * (s8)[+0x24] / 0x3F01 )`, all
three factors sign-extended and the division signed (IDIV); only AL is
stored. Image `0x115FD6` holds `0x7F` (read `0x115FD6` = `7f 00`), so the
static master default is unity. The only writer is the master setter
`0xA6D9B..0xA6DE1` (`AND AL,0x7F; MOV [0x15FD6],AL` `0xA6D9E..0xA6DA0`),
which then walks all 16 records and, for every `[+0x16]==1`, re-runs
`FUN_000a6b21(handle, (s8)[+0x24])` (`0xA6DB7..0xA6DDC`) — the refresh path
that makes `FUN_000a6b21` the "re-apply volume" wrapper.

### 3.2 `FUN_000b811b` — apply to the mixer channel (`0xB811B..0xB8154`)

```
0xB8121  MOV ESI,[EBP+0x8]             ; handle
0xB8124  MOV ESI,[ESI*4+0x406D8]       ; mixer voice pointer
0xB812B  CMP byte [ESI],0; JZ 0xB8150  ; inactive -> no-op
0xB8130  PUSH [EBP+0x10]               ; arg3 = record +0x26 (gain)
0xB8133  PUSH [EBP+0xC]                ; arg2 = record +0x27 (pan)
0xB8136  CALL 0xA66AB                  ; packed = a66ab(pan, gain)
0xB813E  MOVZX EBX,AX                  ; L = (u16)packed
0xB8141  SHR EAX,0x10                  ; R = packed >> 16
0xB8144  SHL EBX,0xA
0xB8147  SHL EAX,0xA
0xB814A  MOV [ESI+0x64],EBX            ; left reader gain = L << 10
0xB814D  MOV [ESI+0x68],EAX            ; right reader gain = R << 10
```

The call site pushes `+0x26` first and `+0x27` second, so cdecl gives
`a66ab(arg2, arg3) = a66ab(+0x27, +0x26)`: the record's pan byte is the
mirror/split input and the computed gain byte is the scaled factor. The
readers consume `[ch+0x64]>>10` / `[ch+0x68]>>10` (`0xB92C6..0xB92CF`, FU-46
§2.3), i.e. the **runtime** extraction is `L = (u16)packed`,
`R = packed >> 16` — different from the arm-time `FUN_000a6579` extraction
`L = packed & 0x7F` for >0x7F (negative) gain bytes (§8 errata 4).

### 3.3 `FUN_000a66ab` — pan/gain split (`0xA66AB..0xA6716`)

```
0xA66AE  MOV ECX,[EBP+0x8]             ; arg1 = pan
0xA66B1  CMP ECX,0x7F; JLE 0xA66BE
0xA66B6  MOV ECX,0xFF; SUB ECX,[EBP+0x8]  ; >0x7F mirrors
0xA66BE  CMP ECX,0x40; JGE 0xA66CC
0xA66C3  MOV EBX,0x7F; ADD ECX,ECX     ; p<0x40: Lfac 0x7F, Rfac 2p
0xA66CC  JLE 0xA66E9
0xA66CE  MOV EAX,0x7F; SUB EAX,ECX; IMUL EAX,0x7E
0xA66D8  MOV EBX,0x3E; CDQ; IDIV EBX   ; p>0x40: Lfac (0x7F-p)*0x7E/0x3E
0xA66E0  MOV ECX,0x7F; MOV EBX,EAX     ; Rfac 0x7F
0xA66E9  MOV EBX,0x7F; MOV ECX,EBX     ; p==0x40: 0x7F/0x7F
0xA66F0  MOV EAX,[EBP+0xC]; IMUL EAX,EBX; XOR EDX,EDX; DIV 0x7F; MOV EBX,EAX
0xA6701  MOV EAX,[EBP+0xC]; IMUL EAX,ECX; XOR EDX,EDX; DIV 0x7F
0xA6710  SHL EAX,0x10
0xA6713  OR EAX,EBX                    ; packed = (qr<<16)|ql
```

`arg2` is multiplied by each factor with a 32-bit `IMUL` and divided by 0x7F
with an unsigned `DIV` (`XOR EDX,EDX`). This is exactly the math of the
FU-46 port `fifa96_mixer_pan_gains_wide`; the port keeps the arm-time
extraction (`L = packed & 0x7F`, `R = packed >> 16`, FU-48 §4) and needs the
runtime extraction variant for this chain (§7).

### 3.4 Mixer voice fields

`[0x406D8+handle*4]` is the FU-37 mixer channel pointer table (8 references:
`0xB8103`, `0xB8124`, `0xB8161`, `0xB818B`, `0xB81B0`, `0xB81F8`, `0xB7F8E`,
`0xB7FF2`). `byte [ch]` is the channel state; `ch+0x64`/`ch+0x68` are the
left/right reader gains, stored `x<<10` and read `>>10` (FU-46 §2.3). In the
port's `fifa96_mixer` these are `voices[handle].active`, `.gain_l` and
`.gain_r`, the latter two normalised to the `>>10` values (mixer.h).

## 4. Error-code table

| value | name (port) | producer | meaning |
| --- | --- | --- | --- |
| `0xFFFFFFF5` | `FIFA96_ERR_VOICE_HANDLE` (11) | all five | handle outside `0..0xF` |
| `0xFFFFFFFC` | `FIFA96_ERR_TRUNCATED` (4) | all five | `[0x15FC8]` outside signed 1..5 |
| `0xFFFFFFF3` | `FIFA96_ERR_VOICE_STATE` (13) | `a6bb3` `0xA6C13`, `a6b21` `0xA6B7E`, `a6cdc` `0xA6D1E` | `[+0x16]==0` |
| `0xFFFFFFF2` | `FIFA96_ERR_VOICE_VOLUME` (14) | `a6bb3` `0xA6C04` | vol `<0` or `>0xFF` |
| `0xFFFFFFF4` | `FIFA96_ERR_VOICE_PAN` (12) | `a6b21` `0xA6B6F` | pan `<0` or `>0x7F` |
| `0xFFFFFFFF` | `FIFA96_ERR_VOICE_SLIDE` (1) | `a6aa6` `0xA6AE3` | `[+0x16] != 1` |
| `0` | `FIFA96_OK` | all five | success; `a6e1f` returns 1 for "not ready" |

Error precedence (verified order): handle -> gate -> parameter range ->
state. The range checks are signed dword compares, so negative parameters
take the range error.

## 5. Sound-system init `FUN_00064f70` (`0x64F70..0x64FFA`)

```
0x64F73  CMP dword [0x55CE0],0; JNZ 0x64FF2   ; already up -> return it
0x64F80  MOV EAX,1
0x64F85  CALL 0x68CFC                          ; query, arg 1
0x64F8A  MOV [0x55CE4],EAX
0x64F8F  TEST EAX,EAX; JLE 0x64FF2             ; query <= 0 -> return early
0x64F93  MOV EDX,-1
0x64F98  CALL 0xA6265                          ; set sound state, arg -1
0x64F9D  TEST EAX,EAX
0x64F9F  SETGE DL; AND EDX,0xFF
0x64FA8  MOV [0x55CE0],EDX                     ; up = (a6265 ret >= 0)
0x64FAE  TEST EAX,EAX; JGE 0x64FC0
0x64FB2  CALL 0xA6A03; PUSH EAX; CALL 0xCBBE8  ; error path + print
0x64FC0  MOV ECX,1; XOR EAX,EAX
loop (0x64FC7..0x64FE1, EAX = 4,8,...,0x18):
  XOR EBX,EBX
  MOV [EAX+0x55D14],ECX                        ; flag[i] = 1
  MOV [EAX+0x55CFC],EBX                        ; pending[i] = 0
  MOV [EAX+0x55CE4],EBX                        ; current[i] = 0
0x64FE3  XOR EDX,EDX
loop (0x64FE5..0x64FF0, EDX = 0..5):
  MOV EAX,EDX; INC EDX; CALL 0x6504C           ; apply channel (i, i+1)
0x64FF2  MOV EAX,[0x55CE0]                     ; return up flag
```

Three 6-dword arrays: pending `0x55CE8..0x55CFF`, current
`0x55D00..0x55D17`, flag `0x55D18..0x55D2F` (values 0/0/1 after init). The
channel handler table is object-1 `0xA0AC` -> image `0x10A0AC`; the static
image holds `{0, 0x55448, 0x551D8, 0x56D54, 0x56D54, 0x55AF8, 0, 0}`, i.e.
image `{NULL, 0x65448, 0x651D8, 0x66D54, 0x66D54, 0x65AF8, NULL, NULL}`
after the loader's +0x10000 relocation.

`FUN_0006504c` (`0x6504C..0x65081`) sets `flag[i]=1`, copies
`pending[i] -> current[i]`, and calls `handler[i](pending[i])`.
`FUN_00065004` (`0x65004..0x6504B`) is the indexed runtime writer: clamps the
value to `0..0x7F` (`0x6500A..0x65015`), then if `flag[ch]==0` stores the
pending value (`0x65043`), else stores the current value (`0x65030`) and
calls `handler[ch](value)` (`0x6503A`). `FUN_00065084` (`0x65084..0x650D0`)
disables a channel: saves `current[ch]` to `pending[ch]`, clears
`current[ch]`, calls `handler[ch](0)`.

## 6. `[0x55D04]` producer status

Ghidra xrefs to `0x55D04` are exactly two: `WRITE` at `0x64FD2`
(`FUN_00064f70`, `MOV [EAX+0x55CFC],EBX` with EAX=8 -> zero) and `READ` at
`0x65560` (`FUN_00065544`, `CMP dword [0x55D04],0; JLE return`). The
positive writer is the indexed store `MOV [EDX+0x55D00],EAX` in
`FUN_00065004` (`0x65030`), where `EDX = channel*4` and EAX is the clamped
`0..0x7F` value; it is reached from `FUN_0001adf8` (`0x1AF85..0x1AF8C`:
`MOV EAX,5; MOV EDX,ESI; CALL 0x65004`) and from the `FUN_0006504c` apply
loop with the pending value. So `[0x55D04]` is the *current* value of channel
1; the play gate `> 0` is the SFX-category channel being enabled. Channel 5
carries the music-volume slider (`FUN_0001b0f0` disables it via
`0x65084`, `FUN_0001adf8` sets it via `0x65004(5, volume)`); channel 1's
handler is the unnamed stop-range entry `0x65448` (FU-49 §1.8). The remaining
channel-to-subsystem mapping and `handler[i]` semantics stay open (§9).

## 7. Port mapping

New module `include/fifa96_loader/fifa96_voice.h` /
`src/fifa96_loader/fifa96_voice.c`, linked against `fifa96_mixer`. Chosen
over extending `fifa96_sfx`: the wrappers need only the mixer pan/gain split,
not the `.BNK`/EACS arm chain, and `struct fifa96_sfx_voice` already denotes
the arm's per-voice output record.

| port | original |
| --- | --- |
| `struct fifa96_voice` fields `state/type/caller/volume/gain/pan/slide_*` | record `+0x16/+0x0B/+0x24/+0x25/+0x26/+0x27/+0x18/+0x1C/+0x20` |
| `struct fifa96_voice_registry.sound_state` | `[0x15FC8]` |
| `struct fifa96_voice_registry.master` | `[0x15FD6]` (image default 0x7F) |
| `fifa96_voice_registry_init` | BSS-zero records + image master; gate 0 like the image |
| `fifa96_voice_active` | `FUN_000a6e1f`: 0 when `state==1`, else 1 |
| `fifa96_voice_set_volume` | `FUN_000a6bb3`: range 0..0xFF, writes `pan` (`+0x27`), `gain`, applies |
| `fifa96_voice_set_pan` | `FUN_000a6b21`: range 0..0x7F, writes `caller` (`+0x24`), `gain`, applies |
| `fifa96_voice_start_slide` | `FUN_000a6aa6`: `(handle, frames, target)`, truncating signed step |
| `fifa96_voice_stop` | `FUN_000a6cdc`: mixer free + type==2 cleanup hook |
| `fifa96_voice_gain` | `FUN_000b9fdd`: `(s8)global_gain * (s8)scale * (s8)pan / 0x3F01` low byte |
| `fifa96_mixer_pan_packed` (mixer addition) | `FUN_000a66ab` return value `(qr<<16)|ql` |
| `fifa96_voice_gain_packed` | `FUN_000b811b` stores: `L<<10`, `R<<10` from the packed value |
| `fifa96_voice_apply_gains` | `FUN_000b811b`: inactive no-op; sets `gain_l/gain_r` (the port's `+0x64/+0x68 >> 10` reader gains) |
| `FIFA96_ERR_VOICE_*` | §4 error values |

The FU-46 `fifa96_mixer_pan_gains_wide` keeps the arm-time extraction
(`L = packed & 0x7F`, `0xA6605`); the new `fifa96_mixer_pan_packed` exposes
the shared `FUN_000a66ab` packed value, and `fifa96_voice_apply_gains` uses
the runtime extraction `L = (u16)packed`, `R = packed >> 16` (`0xB813E`).
Both agree for every retail `0..0x7F` gain; they differ only for >0x7F
(negative) gain bytes, where the runtime reader gains are wide positive
values (e.g. pan 0x40, gain 0x80: ql=qr=0x02040790, packed=0x07940790,
L=0x0790, R=0x0794; the arm-time L is `0x10`).

## 8. Errata (quoted)

1. Brief: "`FUN_000a6aa6(handle, target, frames)` @0xa6aa6 (note: uses EBX in
   EDX and ECX for args — derive exact arg order from the disassembly;
   prologue `PUSH ECX; MOV ECX,EDX; MOV EDX,EBX`)". Actual: EDX is **frames**
   (moved to ECX) and EBX is **target** (moved to EDX), so the order is
   `FUN_000a6aa6(handle, frames, target)`; `0xA6AF1 TEST ECX,ECX` clamps
   frames, `0xA6AFC SHL EAX,0x10` shifts target. Caller `FUN_00065b28`
   (`MOV EBX,0xFFFFFFFF; MOV EDX,ESI; CALL 0xA6AA6` `0x65B40..0x65B4C`)
   pins it.
2. Brief: "`[voice+0x16]==0` → `0xfffffff3` (-13); vol outside 0..0xFF →
   `0xfffffff2` (-14)" for `a6bb3` (and the analogous `a6b21` bullet). Actual
   order: the range check runs first (`0xA6BF5..0xA6C0C` vol, `0xA6B63..0xA6B6F`
   pan), the state check second (`0xA6C0D`, `0xA6B78`). With both bad, the
   original returns the range error (-14/-12), not -13.
3. Brief: "`FUN_000a6bb3(handle, vol)` ... store vol byte at `[voice+0x27]`"
   and "`FUN_000a6b21(handle, pan)` ... store at `[voice+0x24]`". Mechanically
   correct, but the record roles are the reverse of the labels: `+0x27` is the
   byte `FUN_000a66ab` mirrors at 0x7F and splits at 0x40 (the pan), and
   `+0x24` is the byte `FUN_000b9fdd` multiplies as a volume factor
   (`0xB9FEE`, and the master refresh re-applies it through `a6b21`
   `0xA6DD0`). So the wrapper reached from the event-volume path
   (`FUN_00065488` `0x654C3`) writes the record's pan byte, and the wrapper
   reached from the stop path (`FUN_000653d8` `0x65420`) writes the record's
   caller-volume byte. The port keeps the requested API names
   (`set_volume` -> `+0x27`, `set_pan` -> `+0x24`) and mirrors the mechanics;
   the record fields are named `pan`/`caller` after the EACSNDF roles.
4. Brief: "`FUN_000a66ab(vol, pan)` returns packed `(R<<16)|L`" and
   "`FUN_000b811b` ... `L = (u16)ret << 10`, `R = (ret>>16) << 10`". The
   extraction is right, but `a66ab`'s first argument is the pan (mirrored at
   0x7F, split at 0x40) and the second the gain factor; the call site passes
   `(+0x27, +0x26)`, so the labels in the brief are swapped relative to the
   values. The port's `fifa96_mixer_pan_gains_wide(pan, gain)` names follow
   the actual roles.
5. Existing port function vs `FUN_000b811b`: `fifa96_mixer_pan_gains_wide`
   implements the arm-time consumer extraction `L = packed & 0x7F`
   (`FUN_000a6579` `0xA6605`), not the runtime `L = (u16)packed`
   (`0xB813E`). Identical for 0..0x7F gain bytes, divergent for >=0x80. The
   slice adds `fifa96_mixer_pan_packed` and the b811b-exact
   `fifa96_voice_gain_packed`/`apply_gains`.
6. Brief: "`[0x55d04]` ... its only static write is here (set 0) — the
   incrementing writer is dynamic". Confirmed: the indexed writer is
   `FUN_00065004`'s `MOV [EDX+0x55D00],EAX` (`0x65030`), a clamp-to-0..0x7F
   store (not an increment) reached from `FUN_0001adf8` `0x1AF85..0x1AF8C`;
   this closes FU-49 open leg 2.
7. Brief: "`[0x55ce4]=FUN_00068cfc()`; `FUN_000a6265(-1)` →
   `[0x55ce0] = (ret >= 0)`". The query is `FUN_00068cfc(1)` (EAX=1,
   `0x64F80..0x64F85`), and if its result is `<= 0` the function returns
   before calling `FUN_000a6265` (`0x64F8F..0x64F91`); the `>= 0` flag is
   `a6265`'s return.

## 9. Open legs

1. **Slide consumers.** `+0x18/+0x1C/+0x20` are written by `a6aa6`; which
   mixer/timer code interpolates them (and at what tick) is outside this
   slice. `FUN_00065510`'s caller leaves EBX as the target, so the event
   set-param path's target is caller-register state, not a stack argument.
2. **`+0x16` completion.** No wrapper clears the EACSNDF `+0x16`; `a6cdc`
   frees only the mixer channel (`0xB810A`). The writer that returns records
   to the free pool (completion callback or `0xBA00E` path) was not located.
3. **Channel semantics.** `handler[i]` (`0x65448`, `0x651D8`, `0x66D54`,
   `0x65AF8`) and the six channel-to-subsystem mapping are unresolved; the
   UI path `FUN_0001adf8` iterates five 9-byte records at `0x48D00` and
   drives channel 5.
4. **Master refresh.** `FUN_000a6d9b` re-runs `a6b21` per active voice with
   the signed `+0x24` byte; the range check means a stored byte >0x7F would
   be rejected, but the arm stores 0..0x7F, so the path is consistent.
5. **`0xA7E06`/`0x149CC`/`0x149E8`.** The type==2 cleanup target is the
   FU-41 arm-ring teardown; not decoded here.

## 10. Provenance (Ghidra, 2026-10-05, `/fifa96_le.bin`)

* Disassembled: `FUN_000a6e1f`, `FUN_000a6bb3`, `FUN_000a6b21`,
  `FUN_000a6aa6`, `FUN_000a6cdc`, `FUN_000b9fdd`, `FUN_000b811b`,
  `FUN_000b80fa`, `FUN_000a66ab`, `FUN_00064f70`, `FUN_0006504c`,
  `FUN_00065004`, `FUN_00065084`, `0xA6D5A..0xA6E1D` (stop-all loop, master
  setter, state getter), `FUN_00065544`, `FUN_00065488`, `FUN_000653d8`,
  `FUN_00065510`, `FUN_000655ac`, `0x65448`, `FUN_00065b28`, `FUN_00067e94`,
  `FUN_0001adf8`, `FUN_0001b0f0`, `0xB92C6..0xB9305` (PCM16 reader).
* Callers/xrefs: `0xA6BB3` (1: `0x65488`), `0xA6B21` (3: `0x653D8`,
  `0xA6EAD`, `0xB6AB3`), `0xA6AA6` (5: `0x65510`, `0x65B28`, `0x65B70`,
  `0x65BA0`, `0x67E94`), `0x55D04` (write `0x64FD2`, read `0x65560`),
  `0x15FD6` (read `0xB9FF5`, write `0xA6DA0`), `0x406D8` (8),
  `0x55D00`/`0x55CE8` indexed accesses (4 each).
* Image reads: `0x115FC8` (gate = 0), `0x115FD6` (master = `0x7F`),
  `0x10A0AC` (channel handler table `{0,0x55448,0x551D8,0x56D54,0x56D54,
  0x55AF8,0,0}`), `0x161994` frame check via the `0x619AA`/`0x619BA`/
  `0x619BB`/`0x619B8` operands.
* Baseline: `make test` 40/40 before the port; 41/41 after it (new
  `tests/test_voice_control.c`). This doc is the first tracked change of the
  slice.
