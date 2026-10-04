# FU-47 — voice allocation, the randomization RNG and its seeding, the sound-state gate

Date: 2026-10-04. Program `/fifa96_le.bin` (flat LE link image, base 0,
Ghidra project `fifa96`; byte-identical copy at
`/tmp/opencode/fu39/fifa96_le.bin`). Scope: close the three mixer/playback
legs FU-45 left open — `FUN_000a62fa` voice allocation, `FUN_000cbc4c` (the
arm's randomization source) with its seeding, and the `[0x15FC8]` sound-state
gate the bank arm checks. Companion slices: FU-43 (`.BNK` format/arm path),
FU-46 (mixer calibration; `sfx_gain` and pan), FU-41 (bank loader). Every
claim cites an instruction address or an image read.

Address frame (FU-41 §0, FU-43 §0): code addresses are flat image addresses;
data operands inside the code are object-4 offsets and their bytes land at
image + 0x100000. Examples used below: EACSNDF record base `0x61994` ->
image `0x161994`, RNG state `0x12E68` -> image `0x112E68`, gate `0x15FC8` ->
image `0x115FC8`, mode callback table `0x14830` -> image `0x114830`, seeder
key `0x1D98` -> image `0x101D98`, rotor `0x148AC` -> image `0x1148AC`
(zero in the static image). The LE image is not relocated (FU-4), so stored
operands are shown as written and the +0x100000 mapping above resolves them.

## 1. Voice allocation `FUN_000a62fa` (0xA62FA..0xA6383)

Caller contract (`FUN_000a780e` @ 0xA780E): `EAX` = descriptor `+0x00`
voice bitmask, `EDX` = `MOVZX` byte descriptor `+0x14` priority
(`0xA781F..0xA7825`). The allocator scans the 16 EACSNDF records
(`0x61994 + voice*0x28`, i.e. `IMUL EDX,EAX,0x28; ADD EDX,0x61994`
`0xA6316..0xA6319`) in two phases starting at the rotor `DAT_000148AC`.
The record layout it reads:

| record | width | field | reader |
|---|---|---|---|
| +0x12 | u8 | chosen voice index (written on success) | `MOV [EDX+0x12],AL` `0xA6325` |
| +0x13 | u8 | stored priority (set by the arm) | `MOVZX ECX,byte [EDX+0x13]` `0xA6367` |
| +0x16 | u8 | in-use byte, 0 = free | `CMP byte [EDX+0x16],0` `0xA631F` |

### 1.1 Phase 1 — first free allowed voice (0xA62FA..0xA6348)

```
EAX = DAT_000148AC                          ; 0xA6302  rotor
for (n = 0; n < 16; n++) {                  ; 0xA6344/0xA6348  EBX bound 0x10
  EDX = 1 << (EAX & 31);                    ; 0xA6309..0xA6310
  if (EDX & mask) {                         ; 0xA6312/0xA6314
    rec = 0x61994 + EAX*0x28;               ; 0xA6316..0xA6319
    if (rec[0x16] == 0) goto choose;        ; 0xA631F/0xA6323
  }
  EAX = (EAX + 1) & 15;                     ; 0xA633C..0xA6342
}
```

### 1.2 Phase 2 — priority steal (0xA634A..0xA6383)

If no free allowed record exists, the scan restarts at the rotor and accepts
the first allowed record whose stored priority is `<=` the new priority:

```
EAX = DAT_000148AC;                         ; 0xA634A
for (n = 0; n < 16; n++) {                  ; 0xA6377/0xA637B
  if ((1 << EAX) & mask) {                  ; 0xA6358..0xA635C
    EDX = 0x61994 + EAX*0x28;
    ECX = (u8) rec[0x13];                   ; 0xA6367  MOVZX
    if (priority >= ECX) goto choose;       ; 0xA636B CMP EDI,ECX; 0xA636D JNC
  }
  EAX = (EAX + 1) & 15;
}
return NULL;                                ; 0xA637D XOR EAX,EAX
```

`JNC` is unsigned `>=`, so the comparison is on bytes (`new < stored` skips,
`new == stored` steals). Phase 2 never tests `+0x16`; that is equivalent to
phase 1 having already consumed every free in-mask record, so a phase-2 hit is
an active record. On success (`0xA6325..0xA6330`):

```
rec[0x12] = (u8)EAX;                        ; 0xA6325  chosen index
DAT_000148AC = (EAX + 1) & 15;              ; 0xA6328..0xA6330
return rec;                                 ; 0xA6335 MOV EAX,EDX
```

The allocator **does not mark the record in use**: `FUN_000a780e` writes
`rec[0x16] = 1` only after the EACS magic and sound-state checks
(`MOV byte [EAX+0x16],1` `0xA786E`) and the descriptor priority at
`0xA788A`. A failed alloc returns NULL (`0xA637D`), which the arm maps to
`-0x14` (`0xA7830`), and the rotor is left where it stopped.

`FUN_000a780e` consumes the result in this order (all cited in FU-43 §3.2,
re-checked here): alloc `0xA781F`; EACS tag `0xA7840` -> `-10`; gate
`0xA7852..0xA7869` -> `-4`; record fill including priority `0xA7887..0xA788A`;
randomization draws `0xA789D..0xA796F`; gain `FUN_000b9fdd` `0xA7986`;
`eacs[0x1C] = voice` `0xA798B..0xA798E`; `FUN_000a6579` `0xA7991`.

### 1.3 The two-voice path allocates twice (`FUN_000a7728`)

With descriptor `+0x1C & 1` set (`0xA775F..0xA776A`), `FUN_000a7728` splits
pan/volume (`FUN_000a6717` `0xA7775`), raises interrupts-off via
`PUSHFD; CLI` (`0xA778D/0xA778E`), arms **id** (`0xA77A3`), requires the next
id's table slot (`MOV EAX,[EDX+0x61C18]` `0xA77BA` = `DAT_00061c14[id+1]`),
arms **id+1** (`0xA77D6`), on failure calls cleanup `FUN_000a6cdc(id)`
(`0xA77E2..0xA77E4`) and restores flags (`POPFD`), and returns
`(voice2 << 16) | voice1` (`0xA77EF..0xA77F4`). Each arm runs its own
`FUN_000a62fa`, i.e. each voice is allocated from **its own descriptor's**
mask/priority. The `CLI` window is an interrupt critical section, not modelled
by the port.

## 2. The randomization RNG `FUN_000cbc4c` (0xCBC4C..0xCBCB7)

The arm calls this at `0xA78AF` (pitch `+0x0C`), `0xA78E1` (volume `+0x1A`)
and `0xA7938` (pan `+0x1D`), each time consuming `SHR EAX,0x10`
(`0xA78B6`, `0xA78E6`, `0xA793D`). The public `rand` thunk is at `0xCB665`
(`thunk_FUN_000cbc4c`), so the same routine served the game's C `rand()`.

### 2.1 State and update

Six 32-bit words (object-4, image 0x112E68..0x112E7F). Naming
`w0` = `[0x12E68]` … `w5` = `[0x12E7C]` in address order, the listing is:

```
0xCBC4C  MOV EAX,[0x12E7C]          ; EAX = w5
0xCBC51  ADD EAX,[0x12E78]          ; EAX += w4
0xCBC57  MOV [0x12E78],EAX          ; w4 = low
0xCBC5C  ADC EAX,[0x12E74]          ; EAX += w3 + carry
0xCBC62  MOV [0x12E74],EAX          ; w3 = low
0xCBC67  ADC EAX,[0x12E70]          ; w2
0xCBC6D  MOV [0x12E70],EAX
0xCBC72  ADC EAX,[0x12E6C]          ; w1
0xCBC78  MOV [0x12E6C],EAX
0xCBC7D  ADC EAX,[0x12E68]          ; w0
0xCBC83  MOV [0x12E68],EAX
0xCBC88  INC dword [0x12E7C]        ; w5 += 1
0xCBC8E  JNZ 0xCBCB7                ; no carry cascade -> return
0xCBC90  INC dword [0x12E78]        ; carry: w4 += 1, ...
0xCBC96  JNZ 0xCBCB7
0xCBC98  INC dword [0x12E74]
...                                 ; w3, w2, w1 (0xCBCA0..0xCBCAE)
0xCBCB0  INC dword [0x12E68]        ; w0 += 1
0xCBCB6  INC EAX                    ; full 192-bit wrap: INC EAX
0xCBCB7  RET
```

So one step is: add the low word `w5` into the 160-bit running sum through
`w4..w0` with carries (`ADD`/`ADC`), then increment the 192-bit counter whose
least significant word is `w5` (`INC` at `0xCBC88` with a carry cascade up
through `w0`). The stored values are the suffix sums
`w4' = w5+w4`, `w3' = w5+w4+w3`, …, `w0' = w5+w4+w3+w2+w1+w0` (each mod
2^32), and `w5' = w5 + 1`. The **return value is `w0'`** — the word stored at
`0x12E68` (`0xCBC7D..0xCBC83`) — except when the whole state wraps to zero,
where `INC EAX` (`0xCBCB6`) adds one. The ADC chain takes the carry from the
immediately preceding `ADD`/`ADC`; the trailing `INC`/`JNZ` cascade at
`0xCBC88..0xCBCB0` is a separate multi-word increment of `w5` and only the
final `INC EAX` changes the returned register.

Hand-computed first step from the image's initial state (§2.2), 32-bit:

```
EAX = 0x6FDF3B64 + 0x9E353F7D = 0x1_0E147AE1 -> w4=0x0E147AE1, CF=1
EAX = 0x0E147AE1 + 0x0702C49C + 1 = 0x15173F7E -> w3=0x15173F7E, CF=0
EAX = 0x15173F7E + 0xC624DD2F     = 0xDB3C1CAD -> w2=0xDB3C1CAD, CF=0
EAX = 0xDB3C1CAD + 0x883126E9     = 0x1_636D4396 -> w1=0x636D4396, CF=1
EAX = 0x636D4396 + 0xF22D0E56 + 1 = 0x1_559A51ED -> w0=return=0x559A51ED
```

This is **not a standard LCG/MT form**: the update is a prefix-sum transform
of six 32-bit words plus a counter increment; no match was found for the six
constants or the recurrence in the literature (the derived listing above is
the specification). The first ten outputs from the image state are pinned in
`tests/test_sfx_state.c`: `559A51ED 274EA7F5 E827F7E0 76B65632 7F1B3FEB
EAE8D5EE 5101186F 6575225A 696464C0 9ADBE245`.

### 2.2 Initial state and `FUN_000cbcb8` (the constant seeder)

Image 0x112E68 holds the six dwords

```
0xF22D0E56 0x883126E9 0xC624DD2F 0x0702C49C 0x9E353F7D 0x6FDF3B64
```

`FUN_000cbcb8` (`0xCBCB8..0xCBCFB`) seeds by cumulative constants:

```
EAX = seed;                                 ; 0xCBCBB/0xCBCBE
EAX += 0xF22D0E56; [0x12E68] = EAX;         ; 0xCBCC3
EAX += 0x96041893; [0x12E6C] = EAX;         ; 0xCBCC8/0xCBCCD
EAX += 0x3DF3B646; [0x12E70] = EAX;         ; 0xCBCD2/0xCBCD7
EAX += 0x40DDE76D; [0x12E74] = EAX;         ; 0xCBCDC/0xCBCE1
EAX += 0x97327AE1; [0x12E78] = EAX;         ; 0xCBCE6/0xCBCEB
EAX += 0xD1A9FBE7; [0x12E7C] = EAX;         ; 0xCBCF0/0xCBCF5
```

The image table is exactly `FUN_000cbcb8(0)` (the cumulative sums of those
six constants), so the default state with no seeding is seed 0.

### 2.3 The arcade seeder (0x4C698) and the two call sites

A second seeder writes the same six words from a 16-byte string at image
0x101D98 = `"ArCaDe-CoInOp"` and a shift of the seed:

```
0x4C69B  MOV ECX,EAX                 ; ECX = seed
0x4C69D  MOV EDX,0x1D98              ; key = image 0x101D98
0x4C6A2  SHL ECX,0x19                ; seed << 25
0x4C6A5  XOR EAX,EAX
loop (0x4C6A7..0x4C6B9):
  EBX = (s8) byte [EDX];             ; MOVSX 0x4C6A7
  EBX += ECX;                         ; 0x4C6AD
  EAX += 4;
  [EAX + 0x12E64] = EBX;             ; 0x4C6B0 -> words 0x12E68..0x12E7C
until EAX == 0x18
```

i.e. `w[i] = (seed << 25) + (int8)"ArCaDe-CoInOp"[i]`, `i = 0..5`. Raw `E8`
scan (`/tmp/opencode/fu47/scan.py`, §5) locates the two seeding call sites
Ghidra's xrefs miss:

* `0x493E3 CALL 0x4C698` in the unnamed reset/init routine starting `0x493A0`
  (called from `0x180AA`, `0x18359`, `0x18819`), with `EAX = FUN_000cb2a4()`
  at `0x493DE` — the tick counter read (`FUN_000cb2a4` returns
  `[0x12E88]`, image 0x112E88, zero in the static image; game timing also
  calls it at `FUN_000679F4` `0x67B00`).
* `0x17CC7 CALL 0xCBCB8` (Ghidra-unnamed init path inside `FUN_00017B78`'s
  region): the seed is `(u8[ESP+0x66] * FUN_000cb2a4() + u8[ESP+0x67]) << 16`
  (`0x17CAE..0x17CC6`), i.e. a time-derived high half.

Writers to the state words are only `FUN_000cbc4c`, `FUN_000cbcb8` and the
`0x4C698` seeder (xrefs to `0x12E68` and the raw scan); no other seeding
path exists in the image. Which of the two boot seeders lands last depends on
startup order, so retail consumes a time-seeded sequence; the port defaults
to the deterministic seed-0 image state and exposes both seeders.

## 3. The sound-state gate `[0x15FC8]`

`FUN_000a780e` checks the dword at object-4 `0x15FC8` (image 0x115FC8) after
the EACS tag and before the record fill:

```
0xA7852  CMP dword [0x15FC8],0    ; signed
0xA7859  JLE 0xA7864              ; <= 0 -> error
0xA785B  CMP dword [0x15FC8],5
0xA7862  JLE 0xA786E              ; <= 5 -> arm
0xA7864  MOV EAX,0xFFFFFFFC       ; -4
0xA7869  JMP return
```

Gate semantics: **state must be signed 1..5**; 0 and anything > 5 (and
negatives) return `-4`. The two writers:

* `FUN_000a6505` (`0xA6505..0xA650F`, stop path): tears down the current mode
  via the callback `[state*0x14 + 0x14834]` (`0xA6532`), stores
  `[0x15FC8] = 0` (`0xA6538`), then stops all 16 voices through
  `FUN_000ba00e` (`0xA6542..0xA654F`).
* `FUN_000a6265` (`0xA6265`, set path): rejects `state < 0 || state > 5`
  (`0xA626D..0xA6274`, error `-4` at `0xA6276`), calls the stop path
  (`0xA6280`), runs the mode setup callback
  `[state*0x14 + 0x14830]` (`0xA62CA`), and on success stores
  `[0x15FC8] = state` (`0xA62E8`); on failure stores 0 (`0xA62D4`).
  Mode records are 0x14 bytes each in the object-4 table at image 0x114830
  (mode 0 `0x96560`, mode 1 `0xA58BC`, mode 2 `0xA5A37`, mode 3 `0xA5E01`,
  mode 4 `0xA61A4`, mode 5 `0xA669E` at record+0).

`FUN_000a6265` is reached from `FUN_00064F70` (`0xA64F98`) with `EDX = -1`
and `EAX` = the sound-config byte `FUN_00068cfc(1)` (`0x64F80..0x64F85`),
itself called from the sound-system init `FUN_000679F4` -> `0x67A33`. So
state 1..5 is "a successfully initialized sound driver mode"; 0 is "sound
system stopped/no mode", which is exactly why the arm refuses to play when
the gate is 0. The gate is not a mixer-volume state and is not per voice.

## 4. Port

`fifa96_mixer.h/.c` (FU-47 additions):

* `struct fifa96_mixer_voice.priority` — record+0x13 stored steal priority
  (0 when never set). `fifa96_mixer_set_priority(m, voice, prio)` mirrors the
  arm's `MOV DL,[ESI+0x14]; MOV [EAX+0x13],DL` `0xA7887..0xA788A`.
* `struct fifa96_mixer.next_voice` — the `DAT_000148AC` rotor (0 after
  `fifa96_mixer_init`, like the BSS image).
* `fifa96_mixer_alloc_voice(m, mask, priority)` — §1 exactly: phase-1 free
  scan, phase-2 `stored <= priority` unsigned steal, rotor advance on success,
  `-1` when no allowed voice wins. The chosen voice is not activated (the
  caller arms it), matching `FUN_000a780e`'s post-gate `+0x16` write.
* `struct fifa96_mixer.sound_state` — `[0x15FC8]`, default **1** after
  `fifa96_mixer_init` so pre-FU-47 callers keep arming (behavior change
  note: previously the gate did not exist; the original starts at BSS 0 and
  is set by `FUN_000a6265`). `fifa96_mixer_set_sound_state(m, state)`
  validates 0..5 like `FUN_000a6265` (0 = stopped via the `FUN_000a6505`
  analogue, 1..5 = modes). The arm gate lives in `sfx_resolve` between the
  EACS tag check and the first randomization draw, exactly
  `0xA7852..0xA7869` and before `0xA789D`; a blocked arm returns
  `-(FIFA96_ERR_TRUNCATED)` (the original's `-4`, same mapping the EACS
  parser already uses).

`fifa96_sfx.h/.c`:

* `fifa96_sfx_arm_alloc(m, bank, id, opts, out)` — the original's arm
  signature: resolves the entry, allocates from the descriptor mask/priority
  (§1), then arms the chosen voice through the same resolve/start helpers.
  The two-voice path allocates twice (id then id+1, each from its own
  descriptor) and returns `(voice2 << 16) | voice1`; on any error the port
  leaves no voice armed (`fifa96_mixer_stop`), the FU-43 port's documented
  atomicity.
* Allocation failure returns `FIFA96_ERR_NO_VOICE` (new enum value 8) for the
  original's `-0x14` (`0xA7830`).
* `fifa96_sfx_rng` (`w[0]` = `0x12E68` … `w[5]` = `0x12E7C`) with
  `fifa96_sfx_rng_init` (image table, = seed 0), `fifa96_sfx_rng_seed`
  (`FUN_000cbcb8`), `fifa96_sfx_rng_seed_arcade` (0x4C698), `fifa96_sfx_rng_next`
  (`FUN_000cbc4c`, returns `w0'`) and `fifa96_sfx_rng_default` (the
  `fifa96_sfx_rand_fn` adapter). The arm's `rand() >> 16` consumption is
  unchanged, and the injectable `opts->rand` hook stays: a NULL provider plus
  a nonzero span still returns `UNSUPPORTED` (no behavior change for the
  existing tests). The default provider is opt-in
  (`opts.rand = fifa96_sfx_rng_default; opts.rand_ctx = &rng;`), so tests can
  still script draws.

## 5. Structural validation (throwaway, not committed)

Assets: `/tmp/opencode/fu47/scan.py` (raw `E8` call-target scan + RNG model),
run against the byte-identical image copy. Decisive output:

```
direct rel32 calls:
  call at 0x17cc7 -> 0xcbcb8
  call at 0x493e3 -> 0x4c698
dword pointers: none for either
initial state 0x112e68: f22d0e56 883126e9 c624dd2f 0702c49c 9e353f7d 6fdf3b64
seed(0)==image: True
first step trace: (see §2.1) -> 0x559a51ed
seed0 outputs: 559a51ed 274ea7f5 e827f7e0 76b65632 7f1b3feb eae8d5ee
               5101186f 6575225a 696464c0 9adbe245 ...
seed(1) state: f22d0e57 883126ea c624dd30 0702c49d 9e353f7e 6fdf3b65
seed(1) outputs: 559a51f3 274ea80a e827f818 76b656b0
arcade(0x12345678) state: f0000041 f0000072 f0000043 f0000061 f0000044 f0000065
arcade outputs: a0000204 b0000733 8000135f 20002bfc
```

The two call sites behind `FUN_000cbcb8`/0x4C698 were invisible to Ghidra's
xref pass (the enclosing regions were not functions), which is why the scan
was needed. The gate writers and values were validated from the disassembly
(`0xA62D4/0xA62E8/0xA6538`) and the image table at 0x112E68 / gate BSS at
0x115FC8. No public LCG/MT form matches §2's recurrence or constants; the
port implements the instruction-level pseudocode.

## 6. Open legs

1. `>0x7F` split volumes (FU-45's remaining cap leg): `sfx_split` still caps
   the packed split volumes at 0x7F and documents the original's signed
   read-back; only the static-only invalid-pan branch with id < 5 can reach
   it.
2. The two-voice descriptor path remains static-only in retail (no `+0x1C`
   bit 0), and the original's `PUSHFD; CLI` critical section around the two
   arms is not modelled.
3. The time source `DAT_00012E88` is zero in the image and written by a
   routine at `0xCB2CB` (`XOR EAX,EAX; MOV [0x12E88],EAX`) plus an
   interrupt-driven increment not traced here; the port's seeders take the
   seed as an argument.
4. Allocation failure consumes the rotor position and writes `rec[0x12]`
   before the gate, as the original does; the port preserves this and only
   omits the original's unmodelled `FUN_000a6cdc` cleanup on the two-voice
   path (it stops the voice instead). In the two-voice port the second
   allocation runs after both arms resolve, where the original allocates
   id+1 first inside its `FUN_000a780e`; only failure-path rotor state
   differs, the draw order is the same.
5. FU-46's legs unchanged (f10==2 accounting, queue producer, `ctx[5]`,
   `VID_HIPP`, `0x563e0` EOF).

## 7. Provenance (Ghidra calls, 2026-10-04, `/fifa96_le.bin`)

* Decompiled: `FUN_000a62fa`, `FUN_000a780e`, `FUN_000a7728`,
  `FUN_000a75aa`, `FUN_000a6265`, `FUN_000a6505`, `FUN_00064f70`,
  `FUN_000679f4`, `FUN_00068cfc`, `FUN_000cbc4c`, `FUN_000cbcb8`,
  `FUN_000cb2a4`, `thunk_FUN_000cbc4c`.
* Disassembled: `0xA62FA..0xA6383` (allocator), `0xA780E..0xA799F` (arm),
  `0xA7728..0xA780D` (id lookup/two-voice), `0xA6265..0xA62F9` (state set),
  `0xA6505..0xA655F` (state stop), `0xA64F70..0xA64FFA` (sound init),
  `0xCBC4C..0xCBCB7` (RNG), `0xCBCB8..0xCBCFB` (seed), `0x4C698..0x4C6BB`
  (arcade seed), `0x4C640..0x4C6BB`, `0x493A0..0x4941F` (init + call site),
  `0x17C80..0x17CFF` (srand call site), `0x64F70..0x64FFA`.
* Memory/raw scans: image `0x112E68` (seed-0 state), `0x101D98`
  (`"ArCaDe-CoInOp"`), `0x114830` (mode records), `0x1D98`/`0x14830`/
  `0x12E88` (address-frame check), raw `E8` call-target scan
  (`/tmp/opencode/fu47/scan.py`), `56 0e 2d f2` byte search.
* Xrefs: state words `0x12E68`/`0x12E7C`/`0x12E88`, gate `0x15FC8`, rotor
  `0x148AC`, `FUN_000cbcb8`, `FUN_000cbc4c`/thunk, `FUN_00064f70`,
  `FUN_000679f4`, `FUN_000a6265`, `FUN_000a6505`.
* Baseline: `make test` 36/36 before the port; 37/37 after it (new
  `tests/test_sfx_state.c`, allocation/RNG/gate + ASan/UBSan clean). This doc
  is the first tracked change of the slice.
