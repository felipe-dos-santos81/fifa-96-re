# FU-235 — the match-over screen timing and the phase-`0xC` presentation (phase-22 wave W22-5)

Provenance: M2 phase-22 Stage-1 recon, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`,
min `0x10000`, max `.image::00174a2a`, **2706** functions,
`memory_size 2757479`, `.image` overlay present). Every
address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `get_function_by_address` /
`get_xrefs_to` / `decompile_function` of **this** session unless marked
*cross-cited* (then the frozen FU doc / repo source is named). The alias traps
are applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`);
LE globals sit at `+0x100000` from the file offset; addresses are `/FIFA96.EXE`
runtime addresses. Engine/loader source read first-hand. **No writes**: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (W22-5 brief, canonical FU235): derive the **match-over screen's
timing/hold** and the **camera/presentation work** — the **D1 fidelity verdict**
(can the screen hold be modelled faithfully, or must it stay a compression?).
Steps: (1) the screen timing (`word[0x158818] > 0x78` + `0xB4`/`0x168` +
`0x45001`/`[0x10F378]` in `FUN_0008BAF0`'s phase-`0xC` block); (2) the camera +
presentation (`0x960`/`0x780`, `0x36200(3)`, `0x8DCD4`; render-visible vs sink);
(3) the screen machine `[0x5FFC]=2/3/4` and the engine's compression (FU228-L5);
(4) verdict + legs. Extends FU-64 §2/§5, FU-213 §5, FU-228, FU-120, FU-158,
FU-81 §1.5. **Read `docs/ghidra/FU228_post_period_lifecycle.md` first** (the
phase-`0xC` timing + gates + FU228-L5) and FU-64 §5 (the `[0x5FFC]` screens).

---

## Answer in one line

**The hold is a pure scalar dwell gate and is faithfully model-able — the
engine already carries every input it needs (`mr->act.timer` = `word[0x158818]`,
`mr->state.phase`, `mr->global_157ac0`); the only new cells are the
`0x45001` input nibble (`[0x1067EC]&0xF0`) and the `[0x10F378]` divert latch.
The "`0x960`/`0x780` camera hold" is a MISLABEL: those writes are the BALL
machine's lead/target staging (`[0x158830]` = the ball's lead, `[0x15880C]/14`
= the ball record, `[0x15885A]` = the ball heading, `[0x158852]` = the ball's
visual octant), staged into render slot 22 (the ball sprite) by `FUN_00036C70`
— so it is render-visible, NOT a sink, and the engine already ports its tail
(M2 phase-11 T2 / L14.8, `match_run_ball_machine`). The only genuine sink in the
block is `0x36200(3)` → `[0x105FC4]=3` (a view-preset selector).** The faithful
model is therefore: a phase-`0xC` dwell on the act timer, then the native
terminal; the residual work is the unported phase-`0xC`/`0x13`/`0x14` ball-lead
arms (the engine's existing leg L14.8.a), not a camera.

---

## 1. The phase-`0xC` screen timing (fresh)

### 1.1 The block and the act-timer accumulation

`FUN_0008BAF0` (`get_function_by_address 0x8BAF0` = `FUN_0008baf0`, body
`0x8BAF0..0x8C065`; fresh `get_xrefs_to 0x8BAF0` = **1**: the sole
`UNCONDITIONAL_CALL` from `0x8B664` in the per-frame clock `FUN_0008AF38`).
It dispatches on the **match phase** `EAX = [0x157A4A] >> 24` (= byte
`[0x157A4D]`) over `{6, 2, 0x0C, 0x13, 0x14}` (fresh `0x8BAF0` 120 B +
`0x8BC01` 240 B):

```
0x8BAFB EAX = [0x157A4A] >> 0x18         ; the CURRENT match phase
0x8BAF6 MOV EBP,0x1587EC                 ; the ball/object block base
0x8BB08 MOV dword[0x15884E],5            ; entry: max_speed = 5
0x8BB0E CMP EAX,6  ; JNZ 0x8BB55         ; phase 6 block
0x8BB55 CMP EAX,2  ; JNZ 0x8BC01         ; phase 2 block
0x8BC01 CMP EAX,0xC ; JNZ 0x8BCE7        ; <<< phase 0x0C  (the match-over screen)
0x8BCE7 CMP EAX,0x13 ; JZ 0x8BCF5
0x8BCEC CMP EAX,0x14 ; JNZ tail          ; phase 0x13 / 0x14 (the ET halves)
```

Fresh `disassemble_bytes 0x8BC01` (240 B) — **the phase-`0xC` block,
`0x8BC01..0x8BCE2`**, byte-exact:

```
0x8BC0A CMP dword[0x158808],0 ; JNZ 0x8BC20   ; an act installed? don't tick
0x8BC13 MOV AX,[0x157A64]                      ; the whole-frame delta
0x8BC19 ADD word[0x158818],AX                  ; <<< the act/end timer += delta
0x8BC20 MOV EBX,0x960
0x8BC25 MOV EAX,3
0x8BC2A MOV EDX,0x158830
0x8BC2F XOR ECX,ECX
0x8BC31 MOV dword[0x158830],EBX                ; [0x158830] = 0x960
0x8BC37 MOV dword[0x158834],ECX                ; [0x158834] = 0
0x8BC3D MOV dword[0x158838],ECX                ; [0x158838] = 0
0x8BC43 CALL 0x36200                           ; 0x36200(EAX=3) -> [0x105FC4]=3
0x8BC48 MOV EBX,0x15883c
0x8BC4D MOV EAX,0x15880c
0x8BC52 CALL 0x8dcd4                           ; 0x8DCD4([0x15880c],[0x158830],[0x15883c])
0x8BC57 MOV EAX,[0x158816] ; SAR EAX,0x10       ; = word[0x158818]  (alias trap)
0x8BC5F CMP EAX,0x78 ; JLE 0x8BDE7             ; <<< dwell gate 1 (120 ticks)
0x8BC68 CALL 0x45001 ; TEST AL,AL ; JNZ 0x8BC80
0x8BC71 MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0xB4 ; JLE 0x8BCA3
0x8BC80 CMP dword[0x10F378],0 ; JZ 0x8BCA3     ; the no-ET divert latch
0x8BC89 CALL 0x37F54 ; XOR EAX,EDX ; MOV [0x10F378],EAX ; MOV word[0x158818],DX(0)
0x8BC9E JMP 0x8BDE7                            ;    -> FUN_00037F54 (screen 5), timer reset
0x8BCA3 CALL 0x45001 ; TEST AL,AL ; JNZ 0x8BCBF
0x8BCAC MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x168 ; JLE 0x8BDE7  ; gate 2 (360)
0x8BCBF CMP byte[0x157AC0],0 ; JZ 0x8BCD9      ; <<< the EXTRA-TIME FLAG gate
0x8BCC8 MOV EAX,0x13 ; XOR EDX,EDX ; CALL 0x740A0   ; flag SET  -> phase 0x13 (ET)
0x8BCD9 MOV word[0x158822],1                    ; flag CLEAR -> MATCH OVER (end word := 1)
0x8BCE2 JMP 0x8BDE7
```

The gate reads `word[0x158818]` via the `dword[0x158816] >> 16` alias — the
**act timer the same block accumulates at `0x8BC19`**. (This confirms
FU228-L2 / the FU-221 §3 alias rule; FU-213 §5's "`word[0x15881A]`" render is
the misread — see §6 leg FU235-L7.)

### 1.2 The gate chain (fresh, as control flow)

With `T = word[0x158818]`, fresh control flow (the `JLE`/`JNZ`/`JG` above):

```text
T <= 0x78                          -> HOLD  (tail, no terminal)
T  > 0x78 and 0x45001() != 0       -> go to L80
T  > 0x78 and 0x45001() == 0 and T <= 0xB4 -> L3
T  > 0x78 and 0x45001() == 0 and T  > 0xB4 -> L80
L80: [0x10F378] == 0              -> L3
     [0x10F378] != 0              -> CALL 0x37F54; [0x10F378]=0; T=0  (screen 5)
L3 : 0x45001() != 0                -> TERMINAL
     0x45001() == 0 and T <= 0x168 -> HOLD
     0x45001() == 0 and T  > 0x168 -> TERMINAL
TERMINAL: CMP byte[0x157AC0],0 ; JZ 0x8BCD9
          flag SET   -> FUN_000740A0(0x13,0)  (phase 0x13; end word stays 0)
          flag CLEAR -> word[0x158822] := 1   (regulation MATCH OVER)
```

So the phase-`0xC` screen is a **dwell**: a minimum of `0x78` = 120 ticks
(the delta is `[0x157A64]`, FU-62 §4.3; at the 0x200 step the delta is 2, so
~60 frames), extended to `0x168` = 360 ticks unless the `0x45001` input gate
fires. `0x45001` = *"the player requested to skip"* (a key/confirm nibble); a
`[0x10F378]` set diverts to `FUN_00037F54` (screen `5`, the period-end/restart
screen) instead of the match-over terminal.

### 1.3 The callees (fresh, decompiled)

| call | fresh `decompile_function` | identity |
|---|---|---|
| `FUN_00045001` (`0x45001`) | `return DAT_001067ec & 0xf0;` | the **input/confirm nibble** `[0x1067EC] & 0xF0`; fresh `get_xrefs_to 0x45001` = **11** (4 in `FUN_0008BAF0` + the act bodies). `[0x1067EC]` is written by `FUN_00045D0D` (`0x45DD2`, the input/event/pace reset), `FUN_00045F6A` (`0x45F99`) and `FUN_00046246` (`0x462C5`/`0x46430`) — fresh `get_xrefs_to 0x1067EC` = 12 |
| `FUN_00036200` (`0x36200`) | `void FUN_00036200(a){ DAT_00105fc4 = a; }` | the **view/preset selector** `[0x105FC4]`; `0x36200(3)` sets preset 3; fresh `get_xrefs_to 0x36200` = **26** |
| `FUN_0008DCD4` (`0x8DCD4`) | the octagonal-distance triples writer (`out[0]` = distance, `out[1]` = dx, `out[2]` = dz; decompile in §2.1) | already ported (FU-73 §3.4 / FU-79: `fifa96_ball_pair_offset` / `fifa96_keeper_vec_from_delta`); fresh `get_xrefs_to 0x8DCD4` = **78** |
| `FUN_00037F54` (`0x37F54`) | `DAT_0014b028=0; FUN_00037b34(...); FUN_000396e4(0x13,..); FUN_00036bc0(5); DAT_0014af70=0; FUN_00053dc4(); FUN_00054028();` | the divert: **sets `[0x5FFC]=5`** (`FUN_00036BC0(5)`) — the period-end/restart screen, not the match-over |

### 1.4 The ET-half blocks (`0x8BCE7..0x8BDDD`, fresh 280 B)

The same machine runs for phases `0x13`/`0x14`:

```
0x8BCF5 CMP dword[0x158808],0 ; JNZ 0x8BD0B
0x8BCFE MOV AX,[0x157A64] ; ADD word[0x158818],AX      ; the same act timer
0x8BD0B MOV ESI,0x780 ; MOV EDX,[0x158814]
0x8BD18 MOV dword[0x158830],ESI                        ; [0x158830] = 0x780
0x8BD1E/[0x8BD24] [0x158834]=[0x158838]=0
0x8BD2A..0x8BD36  if |[0x158814]| < 0x60 -> [0x158830] = 0x61A8
0x8BD45 EAX = [0x157A4A]>>24 ; CMP EAX,0x14 ; JNZ 0x8BD95   ; phase test
   phase 0x14:  0x8BD52  T vs 0x4B0 ; (T<=0x4B0) 0x8BD61 CALL 0x45001
                0x8BD76  T vs 0x258 ; 0x8BD81 CMP [0x10F360],0 ; JNZ 0x8BDDD
                0x8BD8A  word[0x158822] := 2                  ; ET OVER
   phase 0x13:  0x8BD95  CMP [0x157AC5],[0x157AC7] ; JNZ tail ; scores differ -> hold
                0x8BDAC  T vs 0x4B0 ; 0x8BDB3 CALL 0x45001
                0x8BDC4  T vs 0x12C ; 0x8BDCB CMP [0x10F360],0 ; JNZ 0x8BDDD
                0x8BDD4  word[0x158822] := 2                  ; ET OVER
0x8BDDD MOV dword[0x10F360],1                             ; the ET-over latch
```

The ET-half dwell uses the same act timer with thresholds `0x4B0` (1200) and
`0x258`/`0x12C` (600/300), and a second latch `[0x10F360]` (fresh
`get_xrefs_to 0x10F360` = **5**). The phase-`0x13` arm additionally requires the
two team score words `[0x157AC5]`/`[0x157AC7]` to be **equal** (a differing
score holds — the winner/loser split is FU-237's slice).

---

## 2. The phase-`0xC` presentation — render-visible vs sink (fresh)

### 2.1 The block writes and the metric

The first three statements of the presentation block:

* `0x8BC31 MOV dword[0x158830],0x960` (and `[0x158834]=0`, `[0x158838]=0`) —
  a position triple `(0x960, 0, 0)` = `(2400, 0, 0)`.
* `0x8BC43 CALL 0x36200` with `EAX=3` (`0x8BC25`) — sets `[0x105FC4] = 3`
  (§1.3): the **view preset**.
* `0x8BC52 CALL 0x8DCD4` with `EAX=0x15880c`, `EDX=0x158830`, `EBX=0x15883c` —
  the octagonal metric `0x8DCD4([0x15880C], [0x158830], [0x15883C])` (fresh
  decompile: `sVar2 = *param_2 - *param_1`, `sVar3 = param_2[4] - param_1[4]`,
  `param_3[1]=dx`, `param_3[2]=dz`, `*param_3 = the octagonal length`). So
  `[0x15883C]` = distance, `[0x15883E]`/`[0x158840]` = the dx/dz words.

Fresh `read_memory 0x15880C` (96 B) is all zero at file-load.

### 2.2 The identities: this is the BALL machine, not a camera

The cells are the **ball object** (FU-120 §2, *cross-cited*: *"the 23rd snapshot
slot is the ball record at `0x5880C` (x/y/z, heading `0x5885A`, appearance
`0x58866`)"* — image `0x15880C`/`0x15885A`). Fresh `get_xrefs_to 0x15880C` =
**30** (writers `FUN_000886D4`/`FUN_0008A938`, the tail integration, readers in
the `0x89xxx`/`0x8Axxx` timeline family and `FUN_00036C70`). The engine's own
first-hand derivation (M2 phase-11 T2, `src/fifa96_engine/fifa96_match_run.c`
§L14.8) states it plainly: *"the only per-frame producer of the ball record
position `0x15880C/14` … i.e. the ball the composer stages into render slot 22
(`FUN_00036C70` `0x36E12`)"*, and the tail's `0x8DCD4` metric steers the ball:
`[0x158830]` is the **lead/target**, `[0x15880C]/14` the **ball record**,
`[0x15885A]` the **heading**, `[0x158852]` the ball's **visual octant byte**.

**Render-visible (fresh):** the composer `FUN_00036C70` (fresh
`get_xrefs_to 0x36C70` = **7**, including the frame-body drain `0x49FF5` and the
setup's period resolution `0x49528`) stages the 23 slots; its 23rd slot
(`i=0x16`) is seeded from `[0x15880C]` with the angle `(0x400 - [0x15885A]) &
0x3FF` (the earlier `decompile_function 0x36C70` shows
`*(undefined4*)&DAT_00155ab0+… = DAT_0015880c` and
`(0x400 - DAT_0015885a & 0x3ff)` for the last slot) and, per FU-158 §2,
`[0x158866]`/`[0x15887B]` as its anim/frame. That is the **ball sprite**. So the
phase-`0xC` ball-lead writes are **render-visible**, not a sink.

### 2.3 The tail (`0x8BDE7..0x8C055`)

Every phase-`0xC`/`0x13`/`0x14` iteration — and the `JLE`/`JMP` hold paths —
fall through to the shared tail `0x8BDE7`, which re-runs
`0x8DCD4([0x15880C],[0x158830],[0x15883C])`, derives the heading via
`FUN_0008DD70` (= `FUN_000CD474`, fresh decompile) into `[0x15885E]`,
steers `[0x15885A]` with the 10-bit fold (`& 0x3FF`, `0x200` fold) and the
`0x1104CF`/`0x1104D2` octant table into `[0x158852]`, and integrates the ball
position (`0x8C046 ADD [EBP+0x20],EDX`, `0x8C052 ADD [EBP+0x28],EAX` with
`[0x157A64]`), finishing `0x8C055 MOV EAX,0x1587EC; CALL 0x6FE84` (the generic
appearance machine). This tail is the **already-ported** `match_run_ball_machine`
(engine `fifa96_match_run.c` ~`:3908..4012`), which implements `phase == 2`'s
lead arm + the common tail; the phase-`6`/`0xC`/`0x13`/`0x14` lead arms are its
explicit leg **L14.8.a** (`fifa96_match_run.c:3888`, first-hand).

### 2.4 What is render-visible vs a sink (the D1 question)

| cell | fresh identity | render-visible? |
|---|---|---|
| `[0x158830]` (`0x960`/`0x780`/`0x4B0`) | the ball's **lead/target** triple; steered to by the tail | **render-visible** (moves the ball → slot 22) |
| `[0x15880C]/[0x158814]` | the **ball record** x/z; `FUN_00036C70` slot 22 | **render-visible** (the ball sprite) |
| `[0x15885A]` | the ball **heading** (staged as slot-22 angle) | **render-visible** |
| `[0x158852]` | the ball's **visual octant** (`0x1104D2` table) | **render-visible** (ball facing frame) |
| `0x8DCD4` | the octagonal metric primitive | already ported (not a sink) |
| `[0x105FC4]` (`0x36200(3)`) | the **view/preset** selector (read by `FUN_000365B0`'s scene-table switch and `FUN_00036208`'s pan) | **sink** (unmodelled; presentation-only) |

So the brief's "`0x960`/`0x780` camera hold" is a **mislabel**: these are the
ball machine's per-phase **lead offsets**, and the machine's output is drawn.
`0x36200(3)` is the only genuine presentation sink. `0x8DCD4` is a
already-ported primitive.

> **Erratum candidate (quoted).** FU-213 §5 renders the block as *"`0x8BC20..
> 0x8BC52` the `0x960` camera + `0x36200(3)` + `0x8DCD4` sinks"* and the ET arm
> as *"the `0x780` camera + `[0x158814]` hold"*. The fresh bytes plus FU-120 §2
> / FU-158 §2 / the engine's L14.8 derivation show `[0x158830]`/`[0x15880C]` are
> the ball's **lead/record**, not a camera. Recorded here; the FU-213 text is
> not rewritten (errata-in-later-sections rule).

---

## 3. The screen machine `[0x5FFC]=2/3/4` and the engine compression

### 3.1 The native screen-state cell (fresh)

The screen state is `[0x105FFC]` (the docs render it `[0x5FFC]`, the object-4
offset; fresh `get_xrefs_to 0x105FFC` = **7**, all in the `0x36Bxx` accessor
family — `get_xrefs_to 0x5FFC` returns 0 because the plain address resolves to
the un-aliased space):

* `FUN_00036BC0` (`0x36BC0`): `MOV [0x105FFC],EAX ; RET` — the writer.
* `FUN_00036BC8` (`0x36BC8`): `CMP EAX,[0x105FFC] ; SETZ AL ; AND EAX,0xFF` —
  the compare.
* `FUN_00036C3C` (`0x36C3C`): maps `6 -> 4`, then sets `[0x105FFC]=EDX` and
  returns `1` iff `EDX in {2,4}`.
* `FUN_00036BD8` (`0x36BD8`): `[0x105FFC] := FUN_00037DAC(7)`.

The scores the block consumes (fresh bytes from §1): `[0x5FFC]=0` active
(setup `0x4942B`), `=2` OVER (`0x4B249`, `MOV EAX,2; CALL 0x36BC0`), `=3` POST
(`0x49532`), `=4` EXIT (`0x497D3`/`0x49807`), `=5` the `FUN_00037F54` divert,
`6 -> 4`. (FU-64 §4/§5, *cross-cited*.)

### 3.2 The engine's compression (FU228-L5, fresh source)

`src/fifa96_engine/fifa96_match_run.c`:

* `fifa96_match_run_frame` (`:4015`) tail — the re-pointed `mark_over`
  (`:4163..4178`): `period_ended && past_half_time && end_word_decided` where
  `past_half_time = (mr->global_157ac2 >= 2u)` (native `0x4B1C6 CMP
  [0x157AC2],2`) and `end_word_decided = (mr->state.phase == 0x0Cu &&
  mr->global_157ac0 == 0u)` (native `0x8BCBF`/`0x8BCD9`).
* `fifa96_match_run_step` (`:4184`) → when `lc.screen == OVER` calls
  `fifa96_match_run_resolve` (`:4171`), which runs `OVER -> POST -> EXIT ->
  run_end` in one engine step (the FU228-L5 one-step compression; the source
  comment at `:4198` calls it *"deliberate for G1"*).

So the engine **marks over at the first `period_ended` on the phase-`0x0C`
screen** — it models the native *final gate* (`0x8BCBF`) but **not the
`word[0x158818]` dwell** that precedes it. The engine already carries the dwell
inputs: `mr->act.timer` is `[0x158818]` (engine `:6203`/`:6232`, FU-175 §2) and
the phase is `mr->state.phase`.

---

## 4. Verdict (D1) — model the hold, or keep the compression?

**Model it.** The phase-`0xC` hold is a pure scalar dwell and every input is
already present or trivially added:

1. **`mr->act.timer` = `word[0x158818]`** — the engine already accumulates it
   (the act rows and the act tick; `match_run_ball_machine`/`act_timer`). The
   phase-`0xC` block ticks it only when `[0x158808]==0` (no act installed).
2. **`mr->state.phase`** = `[0x157A4A]>>24` — the dispatch key.
3. **`mr->global_157ac0`** = `[0x157AC0]` — the extra-time flag (live since
   FU-213 T1); the terminal is `flag clear -> word 1` / `flag set -> phase 0x13`.
4. **New cells only:** the `0x45001` input nibble (`[0x1067EC]&0xF0`) and the
   `[0x10F378]` divert latch (and its `FUN_00037F54 -> [0x5FFC]=5` route).

So D1's "model the native timing gate; keep the `0x960` camera work a sink leg"
is **half right and half mislabelled**:

* the **timing gate** is model-able faithfully (a `>=0x78`-then-`>=0x168`
  scalar on `mr->act.timer`, with the input skip and the `[0x10F378]` divert);
* the "**`0x960` camera work**" is **not a camera and not a sink** — it is the
  **ball machine's lead staging**, render-visible (slot 22), and the engine
  **already ports its tail**. What remains is the engine's existing leg
  **L14.8.a** (the phase-`6`/`0xC`/`0x13`/`0x14` arms of `match_run_ball_machine`),
  not a new sink to declare.
* the **only** genuine sink in the block is `0x36200(3) -> [0x105FC4]=3`.

Honest Stage-2 shape: add a phase-`0xC` dwell gate in `fifa96_match_run_frame`
(accumulate `mr->act.timer` while `phase == 0x0C`, gate the existing
`mark_over` on `timer >= 0x168` — the tape's no-input path — with the input skip
and the `[0x10F378]` divert as numbered legs), and extend the ball machine with
the phase-`0xC`/`0x13`/`0x14` lead arms (L14.8.a) **iff** the acceptance tape
pins the ball sprite during the screen. No new action/phase row; the census
does not move.

---

## 5. Census impact

**None.** No new action/phase row is wired; the slice only reads the native
timing/presentation and the engine's existing (unported-arm) ball machine.
`floor(100 × 55/80) = 68` stands; no golden re-pin.

---

## 6. Residual legs

1. **FU235-L1 — the phase-`0xC`/`0x13`/`0x14` ball-lead arms are unported.**
   The `[0x158830]=0x960`/`0x780`/`0x4B0` writes and the ET-half `0x61A8` arm
   (`0x8BD3B`) are the ball machine's lead arm; the engine
   (`match_run_ball_machine`) implements only `phase == 2` + the common tail,
   with these arms the explicit leg L14.8.a (`fifa96_match_run.c:3888`). Owner:
   the phase-11 ball-machine slice. Render-visible if the tape pins the ball
   during the screen.
2. **FU235-L2 — the `[0x10F378]` no-extra-time divert is unmodelled.** Fresh
   `get_xrefs_to 0x10F378` = 4 (`0x8BC80` read, `0x8BC92` write in
   `FUN_0008BAF0`; `0x887F8` write in `FUN_000886D4`; `0x8B2DA` write in
   `FUN_0008AF38`). A set diverts to `FUN_00037F54 -> [0x5FFC]=5`, a different
   screen. Owner: the FU-213-L1 slice.
3. **FU235-L3 — the `0x45001` input gate is unmodelled.** `[0x1067EC] & 0xF0`
   (11 callers); the "skip the dwell" input. The cell is written by the
   input/pace reset (`FUN_00045D0D` `0x45DD2`) and the input handlers
   (`FUN_00045F6A`/`FUN_00046246`). Owner: the input/HUD slice (FU-148).
4. **FU235-L4 — the view-preset cell `[0x105FC4]` (`0x36200(3)`) is unmodelled.**
   The genuine presentation sink; read by `FUN_000365B0`'s scene-table switch
   (case 3 -> `PTR_DAT_00109a74 + 0x108`) and `FUN_00036208`'s pan clamps
   (`[0x105FB8]`/`[0x105FC0]`). Owner: the presentation slice.
5. **FU235-L5 — the ET-half dwell thresholds + the `[0x10F360]` latch are
   unmodelled.** The `0x4B0`/`0x258` (phase `0x14`) and `0x4B0`/`0x12C`
   (phase `0x13`) gates and the `0x8BDDD` latch (fresh `get_xrefs_to 0x10F360`
   = 5). Owner: FU-233 (the ET-half clock) / FU-237 (the ET terminal).
6. **FU235-L6 — erratum candidate (quoted): FU-213 §5's "`0x960` camera".**
   `[0x158830]`/`[0x15880C]` are the ball lead/record (FU-120 §2, FU-158 §2,
   engine L14.8), not a camera.
7. **FU235-L7 — FU-213 §5's gate cell render.** FU-213 §5 writes the gate as
   `word[0x15881A]`; the fresh bytes (`0x8BC57 MOV EAX,[0x158816]; SAR
   EAX,0x10`) read `word[0x158818]` — the alias, already recorded as FU228-L2.
   Same for the ET-half gates `0x8BD52`/`0x8BDAC`. Cross-cited, not re-argued.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `image_base 0`, max
  `.image::00174a2a`, `.image` overlay.
* `get_function_by_address` — `0x8BAF0` (`FUN_0008baf0`,
  `0x8BAF0..0x8C065`).
* `disassemble_bytes` (start, length B) — `0x8BAF0` (120), `0x8BC01` (240),
  `0x8BCE7` (280), `0x8BDE7` (140), `0x8BE20` (400), `0x8BFAF` (120),
  `0x8C026` (70), `0x893F0` (120), `0x8A170` (80), `0x89250` (80),
  `0x36BC0` (48), `0x36C3C` (48).
* `decompile_function` — `0x45001`, `0x36200`, `0x8DCD4`, `0x37F54`, `0x36C70`,
  `0x365B0`, `0x8A938`, `0x8C068`, `0x36208`, `0x8DD70`.
* `read_memory` — `0x15880C` (96 B, all zero).
* `get_xrefs_to` — `0x158822` (**10**), `0x8BAF0` (**1**), `0x45001` (**11**),
  `0x36200` (**26**), `0x8DCD4` (**78**), `0x105FFC` (**7**), `0x10F378` (**4**),
  `0x10F360` (**5**), `0x1067EC` (**12**), `0x105FC4` (**8**), `0x15880C`
  (**30**), `0x158830` (**31**), `0x15883C` (**7**), `0x15885E` (**9**),
  `0x158852` (**9**), `0x15885A` (**16**), `0x15883E` (**2**), `0x158840`
  (**1**), `0x157A73` (**13**), `0x36C70` (**7**), `0x5FFC` (**0** — the real
  cell is `0x105FFC`).

Engine/loader source read first-hand (`src/fifa96_engine/fifa96_match_run.c`
§L14.8 `:3825..4012`, `match_run_scene_stage` `:1571..1621`, `fifa96_match_run_frame`
`:4015..4178`, `fifa96_match_run_step` `:4184`). Docs cross-cited: FU-228
(the phase-`0xC` timing + FU228-L5), FU-213 §5 (the end machine + the camera
label), FU-64 §2/§4/§5 (the `[0x5FFC]` screens), FU-120 §2 (the ball record),
FU-158 §2 (the composer's slot 22), FU-81 §1.5 (the cinematic group). No writes
beyond this slice file; no rename/label/comment/function/script/project save;
`/FIFA96.EXE` and the ISO untouched; no git commands.
