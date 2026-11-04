# FU339 — phase-43 live-leg audit: act-9 stages 1/2, row-05 claim tail, and the stage-0 order

**Phase/Provenance:** M2 phase-43 recon/verification, 2026-11-04. Fresh
read-only Ghidra MCP session on **`/FIFA96.EXE`** (case-sensitive; program
argument explicit; scripts disabled). Every address below is a first-hand
`disassemble_bytes` / `decompile_function` / `get_xrefs_to` read unless marked
*carried*. The engine/test source was read only; the only repo writes are this
spec and the comment correction landing with it. No golden was changed.

**Verdict in one line:** the three candidate live legs are now byte-derived,
but each changes a measured test baseline (natural claim pan, long-ball probe,
or M2 restart timing), so none is wired in this phase. The stale act-9 stage-3
alias comment is corrected; the legs stay explicit.

---

## 1. Act-9 / phase-0x1F stages 1 and 2 (FU181-L5)

Fresh `read_memory 0x88F3C` (16 B) = `81 8f 08 00 | ed 8f 08 00 | 64 90 08
00 | b1 90 08 00` → `{0x88F81, 0x88FED, 0x89064, 0x890B1}`. Act 9 → phase
0x1F is fresh `read_memory 0x1107EC[9] = 0x88F4C`.

### 1.1 Stage 1 (`0x88FED..0x89063`)

Fresh `disassemble_bytes 0x88FED` (0x76 B):

```
88fed  MOV ESI,[0x15B680]          ; the screen/leg selector
88ff3  CMP ESI,3
88ff6  JZ  0x89015
88ff8  TEST ESI,ESI
88ffa  JZ  0x89015
88ffc  CMP ESI,1
88fff  JZ  0x89015
89001  XOR EAX,EAX
89003  MOV [0x158818],AX           ; not in {0,1,3}: timer=0
89009  ADD byte[0x158829],2        ; stage += 2
89010  JMP 0x890E4                 ; epilogue
89015  MOV byte[0x14C15A],1        ; write-only cell (2 writers, 0 readers)
8901c  CALL 0x74110
89021  TEST EAX,EAX
89023  JZ  0x890E4                 ; hold until 0x74110 returns nonzero
89029  XOR EAX,EAX
8902b  CALL 0x93850                ; side 0 slot
89030  MOV EDX,EAX
89035  TEST EAX,EAX
89037  JNZ 0x89040
89036  MOV EAX,1
8903b  CALL 0x93850                ; side 1 retry
89040  MOV EDX,EAX
89042  MOV EAX,[EDX+0x19]
89045  SAR EAX,0x18                 ; byte slot+0x1C = player index
89048  CALL 0x361B0                ; camera pan target := index
8904d  MOV BL,[0x158829]
89053  XOR ECX,ECX
89055  INC BL
89057  MOV [0x158818],CX           ; timer=0
8905e  MOV [0x158829],BL           ; stage=2, then falls into 0x89064
```

`0x74110` is a two-bank predicate, not a parameterised routine. Fresh
`read_memory 0x74110` (24 B): `B8 A4 88 15 00 E8 FA 8E 01 00 85 C0 75 01 C3
B8 D9 90 15 00 E9 EB 8E 01 00`; it calls `0x8D014` on `0x1588A4` then (if
nonzero) tail-jumps `0x8D014` on `0x1590D9`. `0x8D014` walks 11 records at
stride `0xB2` and returns **0 if any record has `+0x9E == 0 && +0x9A == 0`**
(a free record), else 1. Thus stage 1 holds while **either team has a free
record**.

`0x93850` is fresh `disassemble_function`: four slots at `0x157C64`, stride
`0x25`, matched on the sign-extended byte `slot+0x22` (side); it returns the
slot pointer or 0. `[EDX+0x19]>>24` is `SAR` of the dword, i.e. the byte at
`slot+0x1C` (the player/controller index). The engine has one
`struct fifa96_control_slot` (`mr->slot`, size `0x24`, native stride `0x25`)
and one released word; the four-slot walk is FU315-L5.

`0x361B0` is the camera pan setter (`[0x105FB4] = index`, focus triple copy,
`[0x105FC4] = 1`, `[0x105FB0] = 1`); `0x361A4` clears `[0x105FB0]`;
`0x93930` copies the camera focus triple into `0x158897/89B/89F`. These are
presentation/camera sinks; the stage-1 advance does not read them.

### 1.2 Stage 2 (`0x89064..0x890B0`)

Fresh bytes: repeat the `0x93850` side-0/side-1 lookup (`0x89066/0x89074`),
then `MOV DX,[EAX+4]; XOR DH,DH; AND DL,0x70; AND EDX,0xFFFF; JNZ 0x89095`
(`0x89079..0x89088`). A zero `pressed & 0x70` takes
`CALL 0x93930; RET` (`0x8908A`) and **stays at stage 2**. A nonzero chord
takes `CALL 0x361A4`, stage 3, timer 0, then falls into the stage-3 gate.

### 1.3 Why it is not wired

A faithful stage 1 would hold while any record is free; the M2 tape has free
records. A faithful stage 2 would wait for a `0x70` pressed chord; the M2 tape's
last press is KICK, and the restart window carries no `0x70` edge. Wiring
either arm therefore stalls the current restart rather than matching it.
Stage 3's gate is **`word[0x158818]`** (the act timer; `0x890B1 MOV
EAX,[0x158816]; SAR 0x10`), not `word[0x15881A]`; the engine already uses
`a->timer`, so the behavior is native-exact and only stale comments were
wrong (corrected this phase).

The remaining work is a producer/drive phase for the four-slot model and a
tape that presses the pan chord, plus the `0x74110` record-busy predicate if
the stage-1 hold is to be measured. Until then stages 1/2 stay compressed
(FU181-L5).

---

## 2. Row-0x05 stage-0 claim tail (FU315-L1 / FU316-L8)

Fresh `disassemble_bytes 0x7F3A1..0x7F57B` (133 instructions). The tail is
entered after the dir-byte writes at `0x7F386/0x7F397`:

* `0x7F39C TEST SI,SI`; if actual `dir_x != 0` jump `0x7F3FC`. If
  `dir_x == 0`, `0x7F3A1 CMP word[ESP+0x14],0` (actual `dir_z`); a nonzero
  `dir_z` also jumps `0x7F3FC`.
* Both dirs zero (`0x7F3A9..0x7F3F7`): copy the camera triple into the local
  target when `(int16)dword[+0x69]>>16 > 0x20`, else the record triple
  `+0x59/+0x5D/+0x61`; write it to `+0x4D/+0x51/+0x55`. Then
  `word[0x1577BE] <= word[+0x71]` returns; otherwise `speed =
  word[+0x71] << 2` and `dir_x/dir_z = 0x10F334/0x10F33C[byte[+0x8E]]`.
* `0x7F3FC`: `speed += word[0x157750]` (camera Y); zero returns.
* `0x7F414`: if actual dirs equal the type dirs, write
  `+0x4D = [0x15774C] + (int16)dword[+0x6B]>>16` and
  `+0x55 = [0x157754] + (int16)dword[+0x6D]>>16` (the `+0x6D/+0x6F` words, not
  `+0x6B/+0x6D`); `word[+0x6B] > 0x28` or `> word[+0x77]` returns; then
  `speed += (int16)((int16)(word[+0x71] << 3) * 0xB5 >> 8)` only when both
  dirs are nonzero.
* `0x7F48E`: `seed_x = (int16)(speed * dir_x)`, `seed_z = (int16)(speed *
  dir_z)`; metric = `0x8DC68(seed_x, seed_z)` (`fifa96_entity_distance`);
  `CALL 0x92820(rec,0x26)` (sound sink); `CALL 0x71C94(rec,&seed,height=0,
  ramp_param = dword[0x157750] > 0x10)`; this stores the seed at
  `0x1577B8/BA/BC`, binds `0x1577CA = rec`, and runs `0x70544`.
* `0x7F4EC`: `r = has_slot ? 4 : ((int16)metric >> 2)`; if
  `dword[0x157750] != 0` add `word[0x1577F8]` (the event setter's F8); clamp
  `(int16)r > 0x3C` to `0x3C`; store `[0x15872D] = (uint8)r`.
* `0x7F522` (parity arm): if `byte[0x157A4F] != 0`, call `0x79CCC` and, when
  the returned record is not self and the distance `< 0xC0`, call `0x6DA64`
  (a role/pointer swap). This is still unported (FU315-L1).

### 2.1 Wiring experiment (reverted)

A bounded implementation of the vector/event/release part (leaving `0x92820`
and the `0x7F522` parity arm as legs) compiled and passed the new unit pin:
with camera Y 8 and `has_slot` set, the release is `4 + F8 = 0x11` (F8 0xD on
the idle-height arm after the setter clamps height to the camera Y). It is
**not** tape/test-neutral:

* the natural-claim probe's first segment moved from stop 702 / 27 changed
  frames / stop_x `-0x90` to stop 850 / 175 / `-0x736`;
* `test_live_airborne_long_ball_seed_then_band2_follow` fails `cross_ok`;
* the row-05 unit claim path changes `ball.pos_release` from 0 to 0x11.

These are plausible native consequences of the newly ported tail, but they
require a dedicated re-characterization of the natural-claim and long-ball
probes, not a silent edit of the fixtures. The experiment was reverted; the
row stays unwired for this tail.

---

## 3. Act-9 stage 0 order (`0x88F81..0x88FE7`)

Fresh bytes show the native order after the `[0x15882A]=0` clear:

```
CALL 0x700F4      ; camera reset with [0x10F328/2C/30] = 0,0,0
CALL 0x73E28      ; match-state reset
EAX=0x15; CALL 0x740A0
CALL 0x73E08      ; placement commit
CALL 0x4C324      ; controlled-side presentation byte
timer=0; stage++
```

The ported handler currently does the gate clear, phase write, and stage-3
compression. Two native pieces are worth a later phase, but both move measured
baselines:

* `0x88FE0` zeroes `word[0x158818]`; adding it moves the M2 restart gate
  assertion (`gate_step - phase15_step == 93`) and the restart fixture's
  `mr.act.timer == 4`.
* `0x700F4` (zero camera triple) and `0x73E08` (placement commit) move the
  M2 scenario record placement and the live held-key record position when
  added alone.

`0x73E28` (match-state reset) and `0x4C324` remain unported; adding only the
available helpers would be a partial order. This phase records the order and
leaves FU181-L5/FU170-L2 open rather than re-pinning M2 without the full
stage context.

---

## 4. Numbered legs (unchanged/new)

* **FU181-L5** — act-9 stage 1/2 gates: `0x74110` (record-busy), the four-slot
  `0x93850`, the `0x361B0/0x361A4/0x93930` presentations, and the `0x70` press
  producer. Stage 3's gate is correctly `word[0x158818]`.
* **FU315-L1 / FU316-L8** — the row-05 claim tail is fully derived here; the
  wiring is deferred until the natural-claim/long-ball probes are
  re-characterized. `0x92820(rec,0x26)` and the `0x7F522` parity search remain
  sub-legs.
* **FU170-L2 / FU181 stage-0 order** — `0x73E28`, `0x4C324`, the native timer
  zero, and the camera/placement order need a dedicated M2 re-pin phase.
* **FU316-L9** — the unarmed `0x7131C` mirror remains open (not touched here).

---

## 5. Verification

This phase lands comments/spec only: no engine behavior, no dispatch table, no
census. The intended gate is M1 `09b726b7…` and M2 v25 `ea18ee1d…` exact and
`make check` 112/112. The row-05 experiment and the act-9 timer/camera
experiments were reverted before the gate.
