# FU-68: match settings hand-off — the `0x49278` record and its translation into match config

Roadmap slice #8 of FU-58 §7 ("Match settings hand-off"). Derives the two
0x68-byte settings blocks (`0x49210`/`0x49278`), the outer driver's copy and
its bracketing call `FUN_0001B1C8(1)`, the settings→match-config translator
`FUN_0003749C` (blocks `0x4C1xx`/`0x5881A`), and the hand-off into match start
(`FUN_00018108` → `FUN_00011B7C` → `0x493A0`). Ports the translator as
`fifa96_settings_handoff` over caller-owned data.

Result in one line: **`0x49210` is exactly one 26-dword copy immediately below
`0x49278` and serves only as the outer driver's entry snapshot (`0x1DD5A`
copies `0x49278 → 0x49210`; the front-end exit restores it on code 10 at
`0x1EDCA` or reloads the resource on code 11 at `0x1EDAE`); the settings the
match actually consumes are produced by the translator `FUN_0003749C`
(entry `0x3749C`, called from both settings loaders and from the pre-match
path via `FUN_00011B7C`), which writes `settings[0xE]` through the half-length
table at flat `0x37170` `{2,4,6,8,10,20,45,0}` into `[0x4C1D1]` and then
`[0x5881A] = half*0x3C` / `[0x5881C] = half*0x14` (closing FU-62's
"configured half length"), `settings[0xD] → [0x4C302]` (the FU-62 class-2
clock halt), `settings[2] → [0x4C312]`, and seven more setting-derived
fields, with a fixed 1-minute/zero override when `[0x4C32A] != 0` (match
type 4).**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (program
  `fifa96_le.bin`, flat link addresses as in FU-4/FU-58..FU-67). Instructions
  below are read back with `disassemble_function`/`disassemble_bytes`/
  `read_memory`; the decompiler was used for callee characterisation.
* **Address convention (FU-49 §0, FU-66 §1, quoted).** Code is object 1
  (base `0x10000`), data/stack is object 4 (base `0x100000`); instruction
  operands are stored pre-relocation, so an encoded operand `X` names flat
  image address `base + X`. The blocks here are object-4 data: encoded
  `0x49210`/`0x49278` are the flat image addresses `0x149210`/`0x149278`
  (FU-49 §0 for `0x49278`), and encoded `0x4C1CC` is flat `0x14C1CC`
  (`0x4C1CC + 0x100000`). All `read_memory` byte claims below use the flat
  address; instruction operands are quoted exactly as Ghidra displays them
  (encoded).
* **Exhaustive reference scans.** `search_instructions` sweeps all 192,716
  defined instructions; because FU-64 showed callers/consumers can hide in
  undefined regions, every "only consumers" claim was cross-checked with a
  raw byte-pattern scan (`search_byte_patterns` on the 4-byte disp32, which
  matches defined and undefined memory alike), and every call-target claim
  with a full-image `E8 rel32` scan (`run_script_inline`, `ScanCalls.java`,
  analysis-only, not committed).
* **Ghidra listing defect met this slice (new errata).** In `FUN_0003749C`
  the listing leaves `0x3758D..0x3758F` undefined and resumes at `0x37590`
  as `RET`; the raw bytes are `89 0D 0A C3 04 00` = `MOV [0x4C30A],ECX`
  (`read_memory 0x37588`), which is part of the competition arm. All
  `FUN_0003749C` addresses below are quoted from the raw bytes /
  `search_instructions` (which decodes the true stream), not from the
  windowed listing.

## 1. The two settings blocks: `0x49210` and `0x49278`

### 1.1 Layout

| encoded | flat (obj4) | size | static image | role evidenced |
|---|---:|---:|---|---|
| `0x49210` | `0x149210` | 0x68 B (26 dwords) | all zero (`read_memory 0x149210 208`) | entry snapshot of the live block |
| `0x49278` | `0x149278` | 0x68 B (26 dwords) | all zero (`read_memory 0x149278 208`) | live settings (FU-49 §2.1) |

`0x49210 + 0x68 = 0x49278`: the two records are adjacent, same width, and
together form a 0xD0-byte pair. This answers the slice's "28 dwords?"
question: **26 dwords each** (`0x68 / 4`), the count FU-49 proved for the
values block; `0x49210` is not a wider record.

### 1.2 Every reference to `0x49210`

Raw 4-byte scan (`10920400`) finds exactly five instructions in the whole
image — there are no others, defined or undefined:

| site | function | instruction | role |
|---|---|---|---|
| `0x1DC4A` | dead helper `0x1DC3C` | `MOV EDI,0x49210` | `REP MOVSB` `0x49278 → 0x49210` (0 callers) |
| `0x1DC6F` | dead helper `0x1DC54` | `MOV ESI,0x49210` | gate + `REP MOVSB` `0x49210 → 0x49278` (0 callers) |
| `0x1DD60` | `FUN_0001DD40` driver | `MOV EDI,0x49210` | the entry snapshot copy (§2) |
| `0x1ED9B` | front-end exit tail | `MOV ESI,0x49210` | code-11 compare source (§3) |
| `0x1EDD0` | front-end exit tail | `MOV ESI,0x49210` | code-10 restore source (§3) |

The two helpers are byte-identical to the inlined driver/exit sequences but
have **zero static callers** (`get_xrefs_to 0x1DC3C`/`0x1DC54` = 0; the raw
`E8` scan reports `hits=0` for both), so they are dead copies kept by the
original toolchain. `0x49210` has **no field reader** anywhere: it is only
ever copied whole.

### 1.3 Every reference to `0x49278`

Raw scan (`78920400`) returns 27 instructions, all in the settings module and
its loaders: the getter `FUN_0001D940 0x1D943`; the descriptor/UI pair
`FUN_0001D95C 0x1D978/0x1DA03`; the setter `FUN_0001DA58` (11 accesses
`0x1DB0F..0x1DC14`); the raw dead setter `0x1DC34`; the two dead helpers
(`0x1DC45/0x1DC74`); the code-11 reload loader `FUN_0001DC80 0x1DC8E`; the
override loader `FUN_0001DCAC 0x1DCF2`; the driver (`0x1DD5B`); the defaults
installer `FUN_0001DE94 0x1DE9D`; the slider draw `FUN_0001DFB8 0x1E05E`;
and the front-end exit tail (`0x1ED96/0x1EDD5`). No match-module instruction
references the block directly — the match reads it only through
`FUN_0001D940` (§5) and through the translated config globals (§4).

## 2. The driver copy and why it follows `FUN_0001B1C8`

### 2.1 `FUN_0001DD40` head (byte-verified, `disassemble_function 0x1DD40`)

```
0x1DD46 MOV EBX,0x3
0x1DD4B MOV EDX,0x50
0x1DD50 MOV EAX,0x1
0x1DD55 MOV ECX,0x68
0x1DD5A MOV ESI,0x49278
0x1DD5F MOV EDI,0x49210
0x1DD64 CALL 0x1B1C8
0x1DD69 MOVSB.REP ES:EDI,ESI        ; 0x68 bytes 0x49278 -> 0x49210
...
0x1DD87 CALL 0x1E3A8                ; front-end loop
```

So the exact copy is `ECX=0x68` bytes from encoded `0x49278` (flat `0x149278`)
to encoded `0x49210` (flat `0x149210`), executed **once, before the driver's
phase loop**; `0x49278` is copied to `0x49210`, never the reverse. This
confirms FU-66 §2's sequence and fixes its semantics: the copy is a
pre-front-end **snapshot of the live settings**, not a load of settings into
the run-time block.

### 2.2 `FUN_0001B1C8` — module surface select

`FUN_0001B1C8` (entry `0x1B1C8`) is the module select FU-66 §2 described;
the byte listing adds the index-1 detail:

```
0x1B1CB MOV EDI,EBX
0x1B1CD LEA ESI,[EAX*8]
0x1B1D4 MOV EBX,[ESI+0x49120]      ; module descriptor data pointer
0x1B1DA TEST EBX,EBX
0x1B1DC JNZ 0x1B55D                ; loaded -> FUN_00018C10 on both VIV records (FU-66 §2)
0x1B1E2 CMP EAX,0x11
0x1B1E5 JA 0x1B57D
0x1B1EB JMP dword ptr CS:[EAX*4 + 0xB180]
```

For `EAX=1` (the driver's argument) the fallback entry is table flat `0x1B180`
cell 1 = encoded `0xB1F3` → flat `0x1B1F3` (`read_memory 0x1B180`):

```
0x1B1F3 MOV EAX,0x784
0x1B1F8 MOV EBX,EDX                 ; EDX = 0x50
0x1B1FA XOR ECX,ECX
0x1B1FC MOV EDX,0x47C30
0x1B201 CALL 0x18D68                ; load resource 0x784 -> [0x47C30]
0x1B206 MOV EDX,0x47DC0
0x1B20B MOV EBX,EDI                 ; EDI = EBX = 3
0x1B20D XOR ECX,ECX
0x1B20F MOV [ESI+0x49120],EAX       ; cache pointer @ [0x49128]
0x1B215 MOV EAX,0x790
0x1B21A CALL 0x18E64                ; load resource 0x790 -> [0x47DC0]
0x1B21F MOV [ESI+0x49124],EAX       ; cache pointer @ [0x4912C]
```

`0x49120 + 1*8` is the module-1 descriptor cell (FU-66 §2: `0x49120` is a
fixup target, object 4, flat `0x149120`; `read_memory 0x149120` is all zero
statically), and `[0x47C30]`/`[0x47DC0]` are the front-end VIV surface
descriptor pointers used by state 17 (`0x1EB1C`/`0x1EB26`, FU-66 §4). So
argument 1 selects/loads the **front-end module's two VIV resources** and
caches the returned pointers; other call sites pass other indices
(`0x1B1C8` has 11 callers).

### 2.3 Why the copy follows the call

No data dependency exists: `FUN_0001B1C8`'s body contains no reference to
`0x49210` or `0x49278` (both exhaustive reference lists above are confined to
`0x1DD5B`/`0x1DD5F` for the driver's own operands and to the settings
module), and the driver's module arguments (`EAX=1`, `EDX=0x50`, `EBX=3`)
and copy registers (`ECX=0x68`, `ESI`, `EDI`) are pre-loaded before the call
and preserved through it (`FUN_0001B1C8` itself pushes ECX/ESI/EDI at
`0x1B1C8..0x1B1CA` and pops them at `0x1B225..0x1B228`/`0x1B25B..0x1B25E`
before every local return). The evidenced statement
is therefore only the **order**: the driver loads the front-end module's
surfaces, then snapshots the settings, then enters the phase loop. Both are
pre-loop setup; the copy does not consume the call's result.

## 3. The snapshot's consumers: front-end exit codes 10 and 11

The front-end `FUN_0001E3A8` exit tail (byte-verified `0x1ED8B..0x1EDDA`;
FU-66 §4 had the shape, this slice reads the bytes):

```
0x1ED8B CMP EBP,0xB                     ; code 11 ("reload")
0x1ED90 MOV ECX,0x68
0x1ED95 MOV EDI,0x49278
0x1ED9A MOV ESI,0x49210
0x1ED9F XOR EAX,EAX
0x1EDA1 CMPSB.REPE ES:EDI,ESI           ; memcmp(0x49278, 0x49210)
0x1EDA3 JZ 0x1EDAA
0x1EDA5 SBB EAX,EAX
0x1EDA7 SBB EAX,-1                      ; EAX = memcmp sign
0x1EDAA TEST EAX,EAX
0x1EDAC JZ 0x1EDB3
0x1EDAE CALL 0x1DC80                    ; live != snapshot -> reload resource
0x1EDB3 CMP EBP,0xA                     ; code 10
0x1EDB8 MOV EAX,[0x4921C]               ; snapshot[3]
0x1EDBD CMP EAX,[0x49284]               ; live[3]
0x1EDC3 JZ 0x1EDCA
0x1EDC5 CALL 0x175B8
0x1EDCA MOV ECX,0x68
0x1EDCF MOV ESI,0x49210
0x1EDD4 MOV EDI,0x49278
0x1EDD9 MOVSB.REP ES:EDI,ESI            ; snapshot -> live (restore)
```

* **Code 11** compares live (`0x49278`) against the entry snapshot
  (`0x49210`); if the front-end changed the live block, `FUN_0001DC80`
  (`0x1DC80..0x1DCAB`) reloads the settings resource: `EAX=0x9B8;
  ECX=0x68; EBX=0x49278; CALL 0x18BF0` (name), `EDX=EAX; EAX=0x89F00;
  CALL 0x69358` (load into the `EBX` buffer of `ECX` bytes),
  `CALL 0x3749C` (re-translate, §4). FU-49 §2.6/FU-66 §4 called this path
  "settings reload"; the data flow confirms it.
* **Code 10** gates on live-vs-snapshot difference of dword index 3
  (`0x4921C` vs `0x49284`, both setting 3), calls `FUN_000175B8` on a
  difference, then copies snapshot→live, i.e. **restores the entry snapshot**.

**Erratum (FU-66 §4 event map, quoted): "10 | settings save | exit for
settings path; 11 | settings reload | exit for reload path".** The code-10
copy direction is `0x49210 → 0x49278` (`ES:EDI,ESI` with
`EDI=0x49278, ESI=0x49210` at `0x1EDCF..0x1EDD9`), so code 10 restores the
driver-entry snapshot instead of saving the front-end edits; "save" is not
supported by the bytes. Whether "restore" is a cancel or a defaults action
is not asserted (open leg); both paths revert the live block.

### 3.1 The loaders (FU-49 §2.5/§2.6, extended)

* `FUN_0001DE94` (`0x1DE94`): zeroes `0x68` bytes at `0x49278` and installs
  defaults (FU-49 §2.5).
* `FUN_0001DCAC` (`0x1DCAC`): loads the 0x68-byte resource (name id `0x9B8`,
  presence test `FUN_00068F30`, load `FUN_00069338` at `0x8A03C`, copy over
  `0x49278`), falls back to `FUN_0001DE94`, then at `0x1DD08..0x1DD12` calls
  `FUN_000175B8([0x49284])` (setting 3) and `CALL 0x3749C`. Its only static
  caller is `0x17D7F` (an undefined function in the `0x17Cxx` boot region;
  the same function calls the CRD loader `FUN_00065160` at `0x17D48`).
* Both loaders and the code-11 path end in the translator, so **every
  settings (re)load refreshes the match config**.

## 4. The translator `FUN_0003749C`

`FUN_0003749C` (entry `0x3749C`) is the settings→match-config transform. It
is called from exactly six sites (raw `E8` scan): `0x115B6` (`FUN_000115A0`),
`0x11BA1` (`FUN_00011B7C`), `0x1D8B6` (`FUN_0001D82E`), `0x1DCA3`
(`FUN_0001DC80`), `0x1DD12` (`FUN_0001DCAC`), `0x384C7` (`FUN_00038448`).

### 4.1 Head (byte-verified `0x3749C..0x374EF`)

```
0x3749C PUSH EBX/ECX/EDX/ESI/EDI
0x374A1 SUB ESP,0x20
0x374A4 MOV ECX,0x8
0x374A9 MOV EDI,ESP
0x374AB MOV ESI,0x27170          ; flat 0x37170 half-length table
0x374B0 MOV EDX,0x1010101
0x374B5 MOVSD.REP ES:EDI,ESI     ; copy 8 dwords to the stack
0x374B7 MOV EAX,0x4C1DC
0x374BC MOV ECX,0x4
0x374C1 CALL 0x9E8D0             ; fill 4 bytes at 0x4C1DC with 0x01
0x374C6 MOV EDX,0x1
0x374CB MOV AH,byte ptr [0x4C1D0] ; match-type byte
0x374D1 MOV [0x4C2EE],EDX
0x374D7 TEST AH,AH
0x374D9 JZ 0x374E7
0x374DB XOR EAX,EAX
0x374DD MOV AL,[0x4C1D0]
0x374E2 CMP EAX,0x3
0x374E5 JNZ 0x374EF
0x374E7 MOV [0x4C2EE],EBX        ; EBX=0
```

The half-length table is at flat `0x37170` (encoded `0x27170`, object 1):
`read_memory 0x37170` = dwords `{2,4,6,8,10,20,45,0}`. The other candidate
object bases are zero or code bytes (`0x127170`/`0x117170` zeros, `0x107170`
code, flat `0x27170` code), so the referenced table is the one at `0x37170`;
per-address fixup-object classification was not re-parsed (open leg).
`0x9E8D0` decompiles to a rotating-pattern fill (`in_EAX` destination,
`param_2` pattern, `param_1` count), here 4 bytes `0x01` at `0x4C1DC`.

`[0x4C1D0]` is the match-type byte written only by `FUN_0001B7B8`
(`DAT_0004c1d0 = AL`; `0x1B7B9`), which also sets
`[0x4C32A] = (type == 4)` (`0x1B7C8`/`0x1B7D5`; a redundant second writer
`0x32DFF` sets it to 1 immediately before `FUN_0001B7B8(4)` at `0x32E06`).
`[0x4C2EE]` is therefore a match-type flag: 1 unless type 0 or 3.

### 4.2 The settings mapping (`0x374EF..0x375A7`, raw-byte verified)

```
0x374EF MOV EAX,0x9   ; CALL 0x1D940 -> [0x4C2F6]
0x37500 MOV [0x4C2FE],0
0x37506 MOV [0x4C30E],0
0x3750C MOV [0x4C31A],0
0x37512 MOV EAX,0xF   ; CALL 0x1D940 -> [0x4C326]  (+ [0x4C31E]=0)
0x37527 MOV EAX,0x3   ; CALL 0x1D940 -> [0x4C2E6]
0x37536 MOV EAX,0xB   ; CALL 0x1D940 -> [0x4C30A]
0x37545 MOV EAX,0xA   ; CALL 0x1D940 -> [0x4C306]
0x37554 MOV EAX,0x10  ; CALL 0x1D940 -> [0x4C2F2]
0x37563 MOV EAX,0x2   ; CALL 0x1D940
0x3756D MOV DL,[0x4C32A]
0x37573 MOV [0x4C312],EAX
0x37578 TEST DL,DL
0x3757A JZ 0x375A9                 ; not type 4 -> settings path

; competition arm (raw bytes 0x3757C..0x375A7; Ghidra mis-decodes 0x37590):
0x3757C MOV EAX,0x1
0x37581 MOV [0x4C306],ECX          ; = 0
0x37587 MOV [0x4C2F2],ECX          ; = 0
0x3758D MOV [0x4C30A],ECX          ; = 0   <-- in the Ghidra gap
0x37593 MOV EDX,EAX
0x37595 MOV [0x4C316],EAX          ; = 1
0x3759A XOR DH,AH
0x3759C MOV [0x4C302],EAX          ; = 1
0x375A1 MOV [0x4C1D1],DH           ; = 1 minute
0x375A7 JMP 0x375D9

; settings arm:
0x375A9 MOV EAX,0x6   ; CALL 0x1D940 -> [0x4C316]
0x375B8 MOV EAX,0xD   ; CALL 0x1D940 -> [0x4C302]
0x375C7 MOV EAX,0xE   ; CALL 0x1D940
0x375D1 MOV AL,byte ptr [ESP+EAX*4]  ; table[settings[0xE]]
0x375D4 MOV [0x4C1D1],AL

; shared tail:
0x375D9 MOV DL,[0x4C1D1]
0x375E1 IMUL EAX,EDX,0x3C
0x375E4 MOV [0x5881A],AX           ; half seconds  = minutes * 60
0x375EA IMUL EAX,EDX,0x14
0x375ED MOV [0x5881C],AX           ; extra seconds = minutes * 20
0x375F3 CALL 0x1ADF8
0x375F8 MOV EAX,0x14  ; CALL 0x1D940
0x37602 CALL 0x4CE34               ; settings[0x14] -> FUN_0004CE34
0x37607 ADD ESP,0x20; POPs; RET
```

The whole function returns no status; the original reads
`[ESP+EAX*4]` with `EAX = settings[0xE]`, which is out-of-bounds for values
outside `[0,7]` (undefined read, not an error path) — the port rejects that
range (§6).

The config destination is one block: encoded `0x4C1CC` (flat `0x14C1CC`), and
`FUN_00011B7C`/`FUN_000115A0` zero `0x286` bytes there before calling the
translator (`0x11B93..0x11B9F` / `0x115A8..0x115B4`), so the translator only
writes its evidenced fields. `read_memory 0x14C1CC` is all zero statically.

## 5. Hand-off into match start and the config consumers

### 5.1 The pre-match refresh

`FUN_00018108` (main pre-match path, FU-64 §1.2) runs the hand-off before
setup:

```
0x1815D MOV EDX,0x6
0x18162 MOV EAX,EBX               ; EBX = 0x1C at entry
0x18164 CALL 0x11B7C
...
0x18357 MOV EAX,EBP               ; EBP = 1
0x1835D CALL 0x493A0              ; match setup (FU-64 §2)
```

`FUN_00011B7C` (`0x11B7C`) zeroes the 0x286-byte config block
(`0x11B93 MOV ECX,0x286 / 0x11B98 MOV EDI,0x4C1CC / 0x11B9F STOSB.REP`) and
calls `FUN_0003749C` at `0x11BA1`, then does its screen work
(`FUN_0001C9BC`, `FUN_00011590`, `FUN_00011620`). Its five callers are
`0x18164` (main pre-match), `0x18765` (boot flag block `FUN_00018680`),
`0x2B0B8`, `0x31231`, `0x32B78` (mode/competition screens). `FUN_000115A0`
(`0x115A0`) is the same zero+translate pattern (`0x115B6`), called from
`0x25A71`; `FUN_0001D82E` (`0x1D8B6`) does it after a settings-screen set.
So the config is rebuilt from the live settings on every pre-match screen,
not inside `0x493A0` itself.

### 5.2 Match setup `0x493A0` direct settings reads (byte-verified)

```
0x493E8 MOV EAX,0x18 ; CALL 0x1D940 ; 0x493F2 CALL 0x92AA0   ; seed from s[0x18]
0x493F7 MOV EAX,0x19 ; CALL 0x1D940 ; 0x49401 CALL 0x566A8   ; seed from s[0x19]
0x4943F MOV EAX,0x4  ; CALL 0x1D940 ; 0x4944B CALL 0x443E8   ; display mode from s[4]
```

`FUN_00092AA0`/`FUN_000566A8` seed 6 dwords each at `0x10F44`/`0x8FD0` with
`(int)string_byte + EAX*0x2000000` (FU-64 §2.1), `FUN_000443E8` clamps its
argument to `0..2` and selects the display tables at `0x67B4`/`0x67BC`
(decompile; FU-64 §2.1). No other settings lookups exist in the setup
listing (`0x493A0..0x495AF`).

### 5.3 Translated config consumers (evidenced)

| config slot | settings index | consumer evidence |
|---|---|---|
| `[0x4C302]` | 0xD | `FUN_0008AF38 0x8AF63`: class-2 clock runs only while it is 0 (FU-62 §4.2) |
| `[0x5881A]`/`[0x5881C]` | 0xE (table) | `FUN_0008AF38` period-end cases (FU-62 §4.4); `FUN_000886D4 0x88701/0x8872A` re-derives them from `[0x4C1D1]` |
| `[0x4C312]` | 2 | `FUN_00091DD8 0x91DF4`: gates the presentation/ambience body after music start (FU-62 §2) |
| `[0x4C2F6]` | 9 | `FUN_0004B308 0x4B317/0x4B334`: phase 3 → view class 1 (if 1) else 2; phase 4 → 3 (if 1) else 4 (camera clamp class, FU-62 §1.4); also read at `0x73A80`, `0x73E72`, `0x7A662`, `0x7B481`, `0x7C1DA`, `0x41BB0` |
| `[0x4C306]` | 0xA | `FUN_0008A43C 0x8A47A/0x8A4DC`: compared against 0 and 1 |
| `[0x4C2F2]` | 0x10 | `FUN_00079D5C 0x79D87`, `FUN_0008A43C 0x8A73A`: compared against 0 |
| `[0x4C326]` | 0xF | `FUN_0007B57C 0x7B589`: compared against 0 |
| `[0x4C30A]` | 0xB | `0x81FA7` (undefined function): nonzero → `FUN_00092AC8` |
| `[0x4C2EE]` | type flag | `FUN_0008AF38 0x8B2B8` and `0x88CB4` read it; a second writer `0x389AA` (`MOV ECX,1` at `0x3899F`) sets it to 1 |
| `[0x4C316]` | 6 | **no reader** (byte scan `16c30400` finds only its two writer instructions) — open leg |
| `[0x4C2E6]` | 3 | **no reader** (byte scan `e6c20400` finds only its writer) — open leg |

### 5.4 Other live-settings consumers (not via the translator)

| settings index | read site | role |
|---|---|---|
| 0 | `FUN_0001B008 0x1B044` | if nonzero → `FUN_0006504C(5)` (pre-match, FU-64 §1.2) |
| 1 | `FUN_00091F60 0x91FCF` | ambience enable (FU-49 §1.10) |
| 0x11 | `FUN_000920C0 0x9211D` | music-volume slider → ambience set-param (FU-49 §1.10) |
| 0x12/0x13 | `FUN_0001DFB8 0x1E05B` | slider bars (FU-49 §2.4) |
| 0x14 | `FUN_0004CD0C 0x4CD12` | indexes the tables at `0x74D0`/`0x74EC`, stores `[0x7DD0]/[0x7DD4]`; also `FUN_0004CE34` from the translator tail `0x37602` |
| 0x16 | `0x48AB0 0x48AB8` | undefined function uses `s[0x16]-1` as a screen-layout index (open) |
| 0x16/0x17 | `FUN_0004A830 0x4A861..0x4A872` | only on the `EAX != 0` path; the sole static caller `0x4AEF5` passes 0 (open) |

## 6. Port: `fifa96_settings` hand-off transform

`include/fifa96_loader/fifa96_settings.h` + `src/fifa96_loader/fifa96_settings.c`
(caller-owned `struct fifa96_match_config`, no globals, `-fifa96_err_t`, no
comments). Scope: the evidenced write set of `FUN_0003749C` §4, driven by the
port's own settings struct.

| original | port |
|---|---|
| `0x374EF..0x37573` s[9/0xF/3/0xB/0xA/0x10/2] → `0x4C2F6/0x4C326/0x4C2E6/0x4C30A/0x4C306/0x4C2F2/0x4C312` | `field_4c2f6` … `field_4c312` |
| `0x374FB/0x37506/0x3750C/0x37512` zero `0x4C2FE/0x4C30E/0x4C31A/0x4C31E` | `zero_4c2fe`/`zero_4c30e`/`zero_4c31a`/`zero_4c31e` |
| `0x374C6..0x374E9` `0x4C2EE = (type != 0 && type != 3)` | `flag_4c2ee` (from the `match_type` argument) |
| `0x3757A` competition arm `[0x4C32A] != 0` | derived as `match_type == 4` (FUN_0001B7B8 `0x1B7C8`/`0x1B7D5`) |
| `0x375A9..0x375D4` s[6]→`0x4C316`, s[0xD]→`0x4C302`, table[s[0xE]]→`0x4C1D1` | `field_4c316`, `clock_halt`, `half_length_minutes` |
| flat `0x37170` 8-dword table `{2,4,6,8,10,20,45,0}` | `fifa96_match_config_half_lengths` |
| `0x375E1..0x375ED` `[0x5881A]=h*60`, `[0x5881C]=h*20` | `period_length`, `extra_length` |

Not ported (app-side or unproven): the 4-byte `0x01` fill at `0x4C1DC`
(`0x374B7`, role unknown), `FUN_0001ADF8` (`0x375F3`), `FUN_0004CE34(s[0x14])`
(`0x37602`), the `[0x4921C]`/`[0x49284]` gate and `FUN_000175B8`, the
resource loaders (`FUN_0001DCAC`/`FUN_0001DC80`), the driver snapshot
(pure 0x68-byte copy of the 26 values; `struct` copy at the call site), and
all downstream consumers.

Boundary: the original indexes the 8-entry stack copy with an unbounded
`settings[0xE]` (OOB read). The port returns `-FIFA96_ERR_TRUNCATED` (leaving
the output untouched) for a half-length index outside `0..7`, matching the
settings module's invalid-input error. All other settings values are passed
through unclamped, exactly as stored.

## 7. Tests

`tests/test_settings_handoff.c` (suite 56 → **57**): all eight half-length
table entries; normal-type mapping of every field (s[9/0xF/3/0xB/0xA/0x10/2/6/0xD]
and the four zero fields); derived `period_length`/`extra_length`; the
type-0/3/4 flag values; competition override (fields zeroed, `0x4C316=1`,
`clock_halt=1`, half length 1); negative/raw settings pass-through;
out-of-range half-length index (8 and -1) → `-FIFA96_ERR_TRUNCATED` with the
output untouched; NULL settings/output → `-FIFA96_ERR_TRUNCATED`.

## 8. Errata (quoted)

* FU-66 §2: "then the 0x68-byte settings block is copied `0x49278 -> 0x49210`
  ... The copy is preceded by `FUN_0001B1C8(1)`" — **confirmed**; **extended**:
  `0x49210` is exactly one adjacent 26-dword record whose only live consumers
  are that copy (destination), the code-11 compare (source) and the code-10
  restore (source); `FUN_0001B1C8`'s body never references either block, so
  the ordering is pre-loop sequencing, not a data dependency.
* FU-66 §4 event map: "10 | settings save ... 11 | settings reload" —
  **corrected for 10**: `0x1EDCA..0x1EDD9` copies `0x49210 → 0x49278`
  (restore), so code 10 does not save the edited live block; code 11 reloads
  the settings resource through `FUN_0001DC80` (`0x1EDAE`) when live differs
  from the snapshot, as FU-66 described.
* FU-66 §8: "`[0x49210]`/`[0x49278]` ... working/persisted settings pair" —
  **refined**: `0x49278` is the live/edited/persisted block (setter
  `FUN_0001DA58`, getter `FUN_0001D940`), `0x49210` is the driver-entry
  snapshot (write-only from the driver; read only by the exit tail).
* FU-64 §2.1: "`FUN_0001D940(i)` ... the settings block copied to `0x49210`"
  — **corrected**: `FUN_0001D940` reads `[0x49278 + i*4]` (`0x1D943`); the
  `0x49210` block is the snapshot, not the getter's source.
* FU-62 §4.2: "`[0x4C302]` ... no static writer (`get_xrefs_to` → exactly the
  read at `0x8AF63`)" — **closed**: written by `FUN_0003749C` from
  `settings[0xD]` (`0x375C2`) or forced to 1 in the type-4 arm (`0x3759C`).
* FU-62 §4.6: "`[0x4C1D1]` = configured half length in minutes" — **closed**:
  `FUN_0003749C` writes `table[settings[0xE]]` (`0x375D1..0x375D4`) or 1
  (type 4), with the table at flat `0x37170` `{2,4,6,8,10,20,45,0}`; the
  `FUN_000886D4` recomputation (`0x88701/0x8872A`) is unchanged.
* FU-62 §2: "`[0x4C312]` ... no static writer" — **closed**: written by
  `FUN_0003749C` from `settings[2]` (`0x37573`).
* FU-58 §7 slice 8: "one 0x68-byte record" — **confirmed**; the pair is two
  adjacent 0x68-byte records (`0x49210`/`0x49278`).
* Ghidra listing defect: `FUN_0003749C`'s `0x3758D..0x3758F` are an
  undecoded gap and `0x37590` decodes as `RET`; the raw bytes are
  `MOV [0x4C30A],ECX` (Method, §4.1). The listing's
  `0x37591 ADD AL,0`/`0x375A7 JMP` window is a consequence of the gap.

## 9. Open legs

* **Code 10/11 semantic labels** ("save"/"reload"/"restore"/"defaults") are
  not asserted; the data directions are. Which input event produces 10/11 in
  `FUN_00016350` is not derived here.
* **`[0x4C316]` (s[6]) and `[0x4C2E6]` (s[3]) have no readers** in the whole
  image (raw byte scans `16c30400`/`e6c20400`); they may be consumed through
  a pointer or be dead.
* **4 bytes at `0x4C1DC` = `0x01`** (`FUN_0009E8D0` at `0x374C1`) and the
  `FUN_0009E8D0` role are not decomposed.
* **`FUN_0001B1C8`'s loaded-module arm** (`0x1B55D`, `FUN_00018C10`) and the
  other 10 call sites' indices are not decomposed; only index 1 is
  characterised.
* **`0x81FA7`, `0x48AB0` and `0x41BB0` consumers** sit in undefined regions
  and are quoted by site only.
* **`FUN_0004A830`'s settings path** (`0x4A861/0x4A86D`) is not reached by its
  only static caller (`0x4AEF5`, `EAX=0`); no pointer-table entry was found.
* **Object ownership** of each quoted global was not re-derived from the fixup
  stream this slice; the object-4 rule (`X + 0x100000`) is FU-49 §0 and the
  half-length table at flat `0x37170` is matched by content, not fixup.
* **`FUN_0004CE34`/`FUN_0004CD0C`** internals (settings 0x14 consumers) are
  not decomposed.
* The two dead helpers `0x1DC3C`/`0x1DC54` and the raw setter `0x1DC34` have
  no callers; why the toolchain emitted them is not asserted.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`disassemble_function` 0x1DD40; `decompile_function` 0x1B1C8, 0x1DC80,
0x1DFB8, 0x4A830, 0x4B308, 0x1B7B8, 0x4CE34, 0x9E8D0, 0x443E8, 0x3749C
(head only; body via bytes); `disassemble_bytes` 0x1DD40, 0x1ED80, 0x1D940,
0x1DC30, 0x1DCAC, 0x493A0, 0x1B1C0, 0x17D40, 0x37430, 0x374E0, 0x37588,
0x11B7C, 0x11B90, 0x115A0, 0x18150, 0x48AB0, 0x81F90, 0x32DF0, 0x1D8A0,
0x384B0; `read_memory` 0x149210, 0x149278, 0x14C1CC, 0x105344, 0x1052A0,
0x100000, 0x149120, 0x1B180, 0x37170, 0x27170, 0x37588;
`search_instructions` operand `49210`, `49278`, `4c1d1`, `4c1d0`, `4c316`,
`4c2f6`, `4c326`, `4c2e6`, `4c30a`, `4c306`, `4c2f2`, `4c2ee`;
`search_byte_patterns` `10920400`, `78920400`, `d1c10400`, `d0c10400`,
`16c30400`, `e6c20400`, `0ac30400`; `get_xrefs_to` 0x1D940, 0x1DC3C, 0x1DC54,
0x1DCAC, 0x1B1C8, 0x4A830, 0x4C32A, 0x115A0, 0x11B7C;
`get_function_by_address` 0x3749C, 0x374EF, 0x4CD0C;
`run_script_inline` (analysis-only) raw `E8 rel32` scan over the full image
for `0x1DC3C`, `0x1DC54`, `0x1DC80`, `0x1DCAC`, `0x1DE94`, `0x3749C`,
`0x37591`, `0x11B7C`, `0x115A0`, `0x4CE34`, `0x1B1C8`.

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change staged. Port
write set: `include/fifa96_loader/fifa96_settings.h`,
`src/fifa96_loader/fifa96_settings.c`, `tests/test_settings_handoff.c`,
`CMakeLists.txt` (one test block). `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
