# FU-335 — phases `0x13/0x14`: the camera-scatter body and the `0x8D693` producer

**Scope.** The shared phase body `0x6E244` and the `FUN_0008D098` case
`0x13/0x14` scatter producer. Phase-36 wave, 2026-11-04.

## 1. The body `0x6E244..0x6E32A` `[M]`

79 instructions, single-record, no loop, no reset; one shared tail
`0x6E326 POP EBP/EDI/ESI/ECX; RET`. Entry `EAX = rec`, `EDX = &rec+0x4D`.

```
0x6E24C  EDX = [0x157AAC]; SAR 24
0x6E252  EAX = [rec] (team); BL = byte[team+0x826]
0x6E25F  CMP EBX,EDX; JNZ 0x6E326               ; side gate: out untouched
0x6E267  BX = word[0x10F368]                    ; cell z
0x6E26E  DL = byte[rec+0x8D]                    ; active
0x6E274  SI = word[0x10F364]                    ; cell x
0x6E27B  TEST DL,DL; JNZ 0x6E2AC                ; active arm
; inactive (record 0 only): no draws
0x6E281  DL = [0x157AC2]; CMP 4; JGE 0x6E2A1
0x6E28C  CMP byte[team+0x826],0; JNZ 0x6E29C
0x6E295  MOVSX EAX,BX; NEG; JMP 0x6E2A4         ; side 0 negates z
0x6E29C/0x6E2A1  MOVSX EAX,BX                   ; no negation
0x6E2A4  [ECX+8] = EAX                          ; target_z
0x6E2A7  JMP 0x6E321
; active: four 0x92AC8 draws (magnitude, sign per axis)
0x6E2AC  EBP = 0x90; CALL 0x92AC8; DIV EBP      ; d1 % 0x90
0x6E2BA  CALL 0x92AC8; TEST AL,1                ; d2 bit 0 -> sign
0x6E2CD  EBX = (int16)BX; ADD EBX,EAX
0x6E2D2  ac2 < 4 && side == 0 -> NEG
0x6E2F8  [ECX+8] = EAX                          ; target_z
0x6E302  CALL 0x92AC8; DIV 0x90                 ; d3 % 0x90
0x6E309  CALL 0x92AC8; TEST AL,1                ; d4 bit 0 -> sign
0x6E31C  ESI = (int16)SI; ADD ESI,EAX           ; x never negated
0x6E324  [ECX] = EAX                            ; target_x
0x6E326  pop/pop/pop/pop; RET
```

Writes only `+0x4D` (x) and `+0x55` (z); `+0x51` (y) is untouched. The z
negation gate is `ac2 < 4 && side == 0`; the scatter itself is unconditional.

**Errata.** FU-192 §2.10 says "two RNG uses"; the active arm uses **four**
(FU-262 §2.2 already said four). FU-83 §3.1 item 7's "and `[0x157AC2] < 4`"
is wrong: the period byte gates only the z sign. FU-83 §2's "per-record loop"
is the caller `FUN_0008D098`; the body has no loop.

## 2. The producer `0x8D693..0x8D722` `[M]`

The dispatch table `0x8D040[0x13] = [0x14] = 0x8D693`.

```
0x8D693  EAX = 0xD; CALL 0x651F0                ; sink (leg)
0x8D69D  AL = [0x157AC2]; CMP 4; JGE 0x8D703
0x8D6A9  ECX = 0x1E0; CALL 0x92AC8; DIV ECX     ; d1 % 0x1E0
0x8D6B7  CALL 0x92AC8; TEST AL,1                ; sign
0x8D6D4  [0x10F364] = EAX                       ; x
0x8D6DB  [0x10F368] = 0x5A0                     ; z base
0x8D6E1  CALL 0x92AC8; DIV ECX                  ; d3 % 0x1E0
0x8D6E8  CALL 0x92AC8; sign
0x8D6FB  ADD [0x10F368],EAX                     ; z = 0x5A0 +/- d3
0x8D703  EAX = side ? +0x2D0 : -0x2D0
0x8D71D  [0x10F364] = EAX; [0x10F368] = 0x5A0
0x8D728  tail: other side -> install 0x26; phase 0x13 -> 3/0x25; phase
         0x14 -> 0x28/0x2A on the first free record + [0x10F35C] = 0
```

The cells have five/six writers/readers `[M]`: `0x10F364` writes
`0x7D995`, `0x8D6D4`, `0x8D71D`, reads `0x87159` (row 0x28 arm 0) and
`0x6E274`; `0x10F368` writes `0x7D98F`, `0x8D6DB`, `0x8D6FB`, `0x8D722`,
reads `0x87161` and `0x6E267`. The RNG is already ported
(`fifa96_rng_step`, six-word state, `w[0]` word return).

## 3. The port and the wireability verdict

`fifa96_match_phase_13` (registered for both `0x13` and `0x14` in the port
table only) and `fifa96_match_phase_machine_scatter`. Fixtures in
`port/tests/test_engine_match_act.c`:
- inactive, `ac2 2`, cells `0x1234/0xC8` -> `x 0x1234`, `z -0xC8`, y untouched,
  no RNG draw; `ac2 4` -> `z +0xC8`;
- active, seed 0, cells `0/0x5A0` -> `z -0x5F0`, `x 0x1F`; `ac2 4` ->
  `z +0x5F0`, `x 0x1F`;
- the side gate leaves the target and the RNG untouched;
- scatter seed 0: team0 `x 0x20`, `z 0x61F`; team1 `x -0x2D`, `z 0x5C3`; next
  draw `0xD483`; `ac2 4` -> `-0x2D0/+0x2D0`, `0x5A0`, no draws.

**Verdict:** phase `0x13` **is** written live (`match_run_write_phase` from the
ET chooser; the golden carries `state=19`), and the body consumes the shared
RNG stream, so registering it would move the M2 tape (the phase-36 R1a
estimate: 22 dispatches, ~96 draws before frame 6522). The slots stay NULL and
the producer is not called from the live walk; the row is ported-but-dormant
until the measured re-pin slice. Phase `0x14` has no live writer (the probe
words are hardwired `{0,0,0,0}`) -> port-only.

**Legs:** R1a-L1 (the engine splits the native per-team composite; the arm step
runs once per frame after both walks), R1a-L2 (tape inertness unproven), R1a-L3
(`0x651F0(0xD)` unported), R1a-L4 (do not add draws to the every-frame step),
R1a-L6 (the cells also feed action row 0x28 arm 0).
