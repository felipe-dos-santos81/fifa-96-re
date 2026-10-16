# FU301 — the FU293 interpolation part 1: the pan-cell state (`0x7721B..0x77312`), the descriptor/difficulty inputs, the `[0x1577F2]`/`[0x1577F8]` pan-vector pair and the block-by-block port shape of `0x77035..0x77423`

**Phase/Provenance:** M2 phase-28 stage-1 recon wave, Task 4/8 (`FU301`; bounded,
read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0,
`.image` overlay, 2706 functions; confirmed this session by
`get_current_program_info`). Every native address/byte below is a **fresh**
`disassemble_bytes` / `decompile_function` / `read_memory` / `get_xrefs_to` of
*this* session unless marked *carried* (the frozen `docs/ghidra/FU*` slice is
named). Alias traps applied (`dword[x]>>0x19` = bit 1 of the byte at `x+3`;
`dword[0x1577BE]>>16` = `word[0x1577C0]`; `dword[0x1577F2]>>16` =
`word[0x1577F4]`; `dword[0x1577C0]>>16` = `word[0x1577C2]`). No rename / comment
/ label / function-create / script / project-save; `/FIFA96.EXE` untouched; no
git.

**Scope.** The first half of the FU293 wide interpolation: the descriptor seed
(`0x77035..0x7707A`), blocks A–E (`0x7707D..0x77312`), the pan-vector formation
(`0x7733F..0x7738A`) and the hand-off (`0x7738A..0x77423`). The fold internals
(`0x8DCD4`/`0xCD474`/`0xCD514`/`0x8DC68`/`0x74584`/`0x702F8`), the `EDI`
sign-match + install selection, the sit dispatch, `FUN_00076B28` steering and
the `[0x1577CA]` stand-in stay **FU302**'s completion (named per block below).

---

## 1. Port shape

Order matters: the wide route is entered by fall-through from
`0x76FD5..0x77032` (the FU285 gates) at `0x77035`; every block below is fresh
`disassemble_bytes` of this session.

### 1.1 Seed — base extents from difficulty + descriptor ratio (`0x77035..0x7707A`)

* `EAX=[0x14C2F6]`; `ESI=0xA0`.
* `EAX==1`: `ECX = 8 − (dword[desc+9]>>0x19)`; `ESI = 0xA0 − 2·ECX`;
  `ECX = 0x60 − ECX`. (`desc = [rec+4]`; `dword[desc+9]>>0x19` = **bit 1 of
  `byte[desc+0xC]`**.)
* else: `ECX = (dword[desc+9]>>0x19) + 0x70`; `ESI=0xA0`.

So the seed's live inputs are the **difficulty word `[0x14C2F6]`** and the
**descriptor ratio bit `byte[desc+0xC]` bit 1**; the outputs are the X extent
`ESI` and the lane extent `ECX`.

### 1.2 Block A — keeper-distance descriptor scale (`0x7707D..0x770D8`)

* `[0x1577CA]==0` → Block B (0x770D9).
* `d = 0x8DCD4(rec+0x59, [0x1577CA]+0x59)` — the fold writes its word result
  into the caller scratch (`EBX=ESP`, `0x77087`); the caller reads it back via
  `dword[ESP−2]>>16` (`0x77094/0x77098`, the word at `[ESP]`).
* `d < 0x1E0`: `ESI −= ESI>>2` (×3/4); **and, only when `[0x14C2F6]==0`,
  `ECX −= 0x10 − (dword[desc+9]>>0x18)`** (= `0x10 − bit 0 of byte[desc+0xC]`);
  **then `ECX −= ECX>>2` (×3/4) unconditionally on this path** (`0x770C5..0x770CC`
  — the ×3/4 step FU293 §1's rendering omitted).
* `d ≥ 0x1E0`: `ECX += dword[desc+9]>>0x19` (bit 1 of `byte[desc+0xC]`).

### 1.3 Block B — bounds + heading gate (`0x770D9..0x77162`)

* `[0x157750]==0` → `ESI=0xC0` (`0x770D9..0x770E5`).
* Exit → `0x77636` (the stage-4 head) if `word[rec+0x69]>>16 > ESI` (signed).
* Exit if `ECX + dword[rec+0x5D] < dword[0x157750]` (signed).
* Heading: `h = 0xCD474(word[rec+0x6B], word[rec+0x6D])` (fresh decompile:
  0x200-scaled atan2); `h = (h − word[rec+0x7D]) & 0x3FF`; sign-fold
  (`>0x200 → −0x400`); exit if `|h| ≥ 0x80 && word[rec+0x69]>>16 > 0x20`.

### 1.4 Block C — jitter + keeper-relation flag (`0x77163..0x771E7`)

* `jitter(ESI)=0`, `flag(ECX)=0`; `thr = ([0x14C2F6]==1) ? 0x780 : 0x5A0`.
* If `[0x1577CA]!=0` and `|dword[[0x1577CA]+0x61]| > thr`:
  `jitter = (2 − [0x14C2F6])·(rand(0x92AC8) & 0xF)`; clamp `jitter ≤ 0x10`.
* If `[0x1577CA]!=0` and `0x74584([0x1577CA]+0x59, [0x1577CA]+0x5D,
  [0x1577CA]+0x61, rec) != 0`: `flag=1`.

### 1.5 Block D — latch + route (`0x771E8..0x7721A`)

* `byte[0x157C57]!=0` → `0x774D7` (stage-4 route).
* `X = (dword[desc+9]>>0x19) >> flag` (`0x771F9..0x771FF`, the bit-1 ratio
  shifted by the keeper flag).
* `jitter < X` → latch test at `0x7720E`; else `dword[0x157750] > 0x20` → pan
  block; latch test otherwise.
* Latch test: `byte[0x157C58]==0` → `0x774DD` (the re-place block); else the
  pan block.

### 1.6 Block E — pan-cell integration (`0x7721B..0x77312`) — the load-bearing L1 state

```
0x7721B  byte[0x157C59] = 1
0x77222  AX = word[0x1577BE];  ESI = dword[0x157750]     ; see 1.9 (BX source)
0x7722E  if (AX == 0) {
0x77233    word[0x1577C0] = (rand(0x92AC8) & 7) + 8      ; [8,15]
0x77247    word[0x1577C2] = (rand(0x92AC8) & 7) + 8      ; [8,15]
0x7725B    word[0x1577BE] = 0xCD514(word[0x1577C0], word[0x1577C2])
             ; args via the dword aliases ([0x1577BE]>>16 = C0, [0x1577C0]>>16 = C2)
         }
0x7727B  word[0x1577C2] = -word[0x1577C2]
0x77284  BX = word[0x1577C0]; b = MOVSX int8[desc+0x1B]
0x77292  if (BX > 0) word[0x1577C0] = BX + b   else word[0x1577C0] = BX - b
0x772BF  EAX = (rand(0x92AC8) & 0xF) + 4; EDX = (int8)byte[desc+0xF]
0x772D5  if (EAX >= EDX unsigned) {                      ; path A
0x772E3    word[0x1577C0] = 0x8DC50(word[0x1577C0], 2)
0x772FB    word[0x1577C2] = 0x8DC50(word[0x1577C2], 2)
         } else {                                        ; path B
0x772D7    word[0x1577C2] = 0x8DC50(word[0x1577C2], 3)
         }
```

`0x8DC50` is fresh-decompiled as the signed truncating shift `|v|>>n` (already
ported: `camera_shift_step`, `fifa96_camera.c:923`). So Block E writes:
**C0 = seed[8,15] → ±int8 jitter → >>2|keep; C2 = (seed[8,15]) → negate →
>>2|>>3; BE = CD514(C0,C2) on the zero-guard only.** The 6-byte pan-vector
record `[0x1577B8..0xBD]` is *not* written here — it is written by the setter
(§1.10).

### 1.7 Block F — `EDI` sign-match (FU302) + pan-vector formation (`0x77313..0x7738A`)

* `0x77313..0x7733A` computes `EDI := 1` iff `sign(word[0x1577C2]) ==
  sign(dword[0x157754])` — **FU302** (the `EDI` install selection).
* `0x7733F..0x77350`: `delta(EAX) = word[0x1577F4] − word[0x1577FA]` via the
  alias reads `dword[0x1577F2]>>16` / `dword[0x1577F8]>>16` — i.e. the ported
  `timer_limit − timer` (see the pair table). **The delta is read live; both
  words have writers besides `FUN_00070544` (`FUN_000736AC` writes
  `[0x1577FA]` at `0x7379A`/`0x73A74`, `0x7EFE3` elsewhere), so it is not a
  constant `ramp_divisor`.**
* `0x77354..0x7736D`: `word[ESP+0xA] = low16(C0·delta)`;
  `word[ESP+0xC] = low16(C2·delta)` (`IMUL` 32-bit, word store).
* `0x77372..0x77385`: `word[ESP+0x8] = 0x8DC68(word[ESP+0xA], word[ESP+0xC])`.
  **The computed 6-byte pan vector is `[ESP+0x8..0xD] = {dist, C0·delta,
  C2·delta}`.**
* `0x7738A`: `byte[0x157C57] = 1` (post-vector latch).

### 1.8 Block G — side dispatch + hand-off (`0x7738A..0x77420`; sit dispatch = FU302's completion)

* If `byte[0x157823]!=0`: clear it and bump the per-side counters
  (`word[0x157ACC]`/`word[0x157AD0]` via `0x741B4`, `0x7739C..0x773D4`).
* `0x92820(rec, 0x1A)` (`0x773E3`); `side = byte[[rec]+0x826]` → `0x741B4` →
  code `0xE`/`0x5` → `0x651F0` (`0x773E8..0x7740B`) — **FU302** (sit dispatch).
* `EDX = ESP+8` (the vector), `EAX = EBP` (rec), `ECX = 0`, `BL = 0`,
  `byte[0x157A6C] = 0` (`0x77410..0x7741A`).
* `0x77420 MOVSX EBX, SI` — **see 1.9**; `0x77423 CALL 0x71C94` (the setter).

### 1.9 The hand-off BX argument — fresh correction of FU293 §1

`ESI` is reloaded at `0x77228` (`MOV ESI, dword[0x157750]`) and is not written
again before `0x77420`, so `MOVSX EBX, SI` passes the **sign-extended low word
of the `[0x157750]` camY dword** to `0x71C94`'s `BX`. FU293 §1's parenthetical
"(the jitter)" is contradicted by these bytes (the Block C jitter `ESI` is dead
after Block D's `CMP`). `0x71C94` consumes it at `0x71CCB..0x71CE5`:
`word[0x1577F0] = BX`, then `IF MOVSX(BX) < dword[0x157750] THEN
word[0x1577F0] = word[0x157750]` (the max-clamp). The load's *intent* is
deliberate reuse of the camY cell as the setter's height seed; the HLL
register-allocation assumption that `0x92AC8`/`0x8DC50`/`0xCD514` preserve
`ESI` on the seed path is a leg (FU301-L6).

### 1.10 The setter round-trip (`0x71C94` entry, fresh) and the record

Fresh `disassemble_bytes 0x71C94`: bail if `byte[0x157A6C]!=0` (`0x71C99`);
`CALL 0x700F4` — the **full event reset** — then `MOVSD+MOVSW` copy of the
`EDX` vector into `[0x1577B8..0x1577BD]` (`0x71CC3..0x71CC9`). Fresh
`0x701D8..0x70204` shows `FUN_000700F4` zeroing `word[0x1577B8]`,
`word[0x1577BA]`, `word[0x1577BC]`, `word[0x1577BE]`, `word[0x1577C0]`,
`word[0x1577C2]` (plus `[0x1577F0..0x1577FA]` at `0x70159..0x701A8`). The
setter then `CALL 0x70544` at `0x71D65` (fresh `get_xrefs_to 0x70544` = 2:
`0x70B88` `FUN_000709D0`, `0x71D65` `FUN_00071C94`), whose fast path divides the
just-stored step pair by `F8` (`0x705E8` = `F4−F6`) back into `vel_x`/`vel_z`
(`0x7065B`/`0x706D9`), recomputes `speed` (`0x706FE`/`0x70798` area, carried
FU-152 §2.9) and runs the `>0x19` atan walk. **Net: the wide block's vector
words 1/2 return as the pan cells through the `F8` divisor — the same pass
re-derives `BE/C0/C2` from the record.**

### 1.11 The cells — writer + value domain (fresh censuses, this session)

| cell | in-window writer(s) | full fresh writer census (`get_xrefs_to`) | value domain (fresh-derived; external writers carried FU157/FU167/FU171) | engine model (fresh read) |
|---|---|---|---|---|
| `word[0x1577BE]` (speed) | `0x77275` (zero-guard seed = `0xCD514(C0,C2)`) | 15 W / 70 refs: `0x73CB8` `FUN_000736AC`, `0x70ADE` `FUN_000709D0`, `0x706FE`+`0x70798` `FUN_00070544`, `0x71990` `FUN_0007131C`, `0x7108B`+`0x712FC` `FUN_00070DE0`, `0x70204` clear, `0x71EE5` `FUN_00071DF4`, `0x7A91F` clear, `0x7F58A`, `0x77C1C`/`0x77D2D` (row `0x1C`), `0x852B6` | signed word; the low word of `0xCD514`'s `|v|·2^16 / trig` quotient (fresh decompile); consumer-gated as the camera speed (FU167/FU171 carried) | `fifa96_camera.speed` (`camera.h:15`) |
| `word[0x1577C0]` (lead x) | `0x77241` seed, `0x772A3` +b, `0x772B8` −b, `0x772F5` `>>2` | 23 W + 2 RW / 91 refs: `0x73B0C` (+`0x736FA`/`0x73703` RW `FUN_000736AC`), `0x70B43`, `0x7065B`/`0x706AA`/`0x70756`, `0x7197A`, `0x71053`/`0x71274`, `0x701EC` clear, `0x71E63`/`0x71E84`/`0x71E9A`, `0x7A92D` clear, `0x76892`, `0x7F59E`, `0x77C92`/`0x77CE6`/`0x77CFD`, `0x852C4` | signed word; in-window `[8,15] → ±int8[desc+0x1B] → >>2|keep` | `fifa96_camera.vel_x` (`camera.h:12`); `s.lead_x = 0` stand-ins `handlers.c:2651/2967/3075` (OL-72/82/69); consumer `outfield.c:539-540` |
| `word[0x1577C2]` (lead z) | `0x77255` seed, `0x7728B` negate, `0x7730D` `>>2`/`>>3` | 21 W + 2 RW / 54 refs: `0x73B13` (+`0x73713`/`0x7371C` RW), `0x70B80`, `0x706D9`/`0x70792`, `0x71981`, `0x70FA1`/`0x71085`/`0x712DF`, `0x701E5` clear, `0x71E71`/`0x71EB0`/`0x71EC6`, `0x7A926` clear, `0x768B2`, `0x7F591`, `0x77CDD`/`0x77D0A`, `0x852BD` | signed word; in-window `−[8,15] → >>2|>>3` | `fifa96_camera.vel_z` (`camera.h:12`); `screen_lead_z` staged 0 (`match_run.h:949`) |
| `[0x14C2F6]` (difficulty) | — (read at `0x77035`, `0x770A7`, `0x77163`, `0x771A9`) | 2 W / 19 refs: `0x374FB` `FUN_0003749C` and `0x3878C` `FUN_00038630`, **both `= FUN_0001D940(9)` = `dword[0x149278 + 9*4]`** (fresh decompiles); reads also `0x4B317`/`0x4B334`, `0x73A80`, `0x73E72`, `0x7C1DA`, `0x419F9`/`0x41A4C`/`0x41BB0`, `0x7A662`, `0x82031`/`0x82043`/`0x82283`, `0x7B481` | runtime option-9 dword; the static table is all-zero (fresh `read_memory 0x149278` 68 B); option setters named fresh: `FUN_0001DA58`/`FUN_0001DCAC`/`FUN_0001DE94`/`FUN_0001E3A8` (xrefs to `0x149278`); in-window used as `{==1, !=1}` and `(2−x)` | unmodeled: OL-66 zero-image default (`handlers.c:960`); `goal_gate` caller-supplied (`ball_pairing.h:249`) |
| `byte[desc+0xC]` bits `{0,1}` (ratio) | — (read at `0x77044`/`0x7704F`, `0x770B6`/`0x770BE`, `0x771F9`/`0x771FF`) | writer **unlocated** (`desc = [rec+4]`; the binding is the FU167/FU168/FU176/FU204 leg class) | 2-bit code: bit 1 ∈ {0,1} → extents (`8−bit1`, `0x70+bit1`), bit 0 ∈ {0,1} → the `0x10−bit0` adjust | no model (OL-65 roster descriptor, `handlers.c:956-960`) |
| `byte[desc+0xF]` (reduce threshold) | — (read at `0x772BF`/`0x772CD`) | writer **unlocated** (same class) | int8; compared (`JNC`, unsigned) against `(rand&0xF)+4`; negative ⇒ path B always | no model |
| `int8[desc+0x1B]` (rate jitter) | — (read at `0x7729C`/`0x772B1`) | writer **unlocated** (same class) | int8 added/subtracted to C0 by sign | no model |
| `[0x1577F2]` (F2 peak) | — (read at `0x7733F` via `>>16`) | 3 W / 14 refs: `0x7058B`+`0x70605` `FUN_00070544`, `0x70160` clear | fresh `0x702F8` decompile: `ramp(F0)` ∈ `{0..0x94}` (`>0x63F→0x94`, `<1→3`, else `0x94−T16[(0x640−F0)>>2]≥0`); idle arm `6` | `fifa96_camera.event_peak` (`camera.h:68`), written by `camera_event_ramp` (`camera.c:759`) |
| `[0x1577F8]` (F8 divisor) | — (read at `0x77345` via `>>16`) | 3 W / 20 refs: `0x705E8`+`0x70613` `FUN_00070544`, `0x70187` clear | `F4−F6`; idle arm `0xC` (fresh `0x705F0..0x7061A`) | `fifa96_camera.ramp_divisor` (`camera.h:37`, set `camera.c:760`) |
| `[0x1577F4]` / `[0x1577FA]` (the delta aliases; `F4`/`F6`) | — (delta read at `0x7733F..0x77350`) | F4: 3 W / 12 refs (`0x7059A`, `0x7060C`, `0x70167`); F6: 6 W / 26 refs (`0x705E1`, `0x7061A`, `0x701A8`, `0x7379A`, `0x73A74`, `0x7EFE3`) | `F4 = 2·F2`; `F6 = ±ramp(F0 − word[0x157768]) + F0` (fresh `0x705A0..0x705E8`), idle 0 | `fifa96_camera.timer_limit` / `timer` (`camera.h:16-17`, `camera.c:747/756`); `FUN_000736AC` also writes the timer (ported walk) |

### 1.12 What the port must add (vs what exists)

**Exists (fresh reads).** `fifa96_camera` already carries every cell of this
window: `vel_x`/`vel_z` (`camera.h:12`) = the pan cells; `speed` (`:15`) = the
speed word; `timer`/`timer_limit` (`:16-17`), `event_peak` (`:68`),
`ramp_divisor` (`:37`) = the pair family; `camera_event_ramp`
(`fifa96_camera.c:728-774`) is the `FUN_00070544` port (computes F2/F4/F6/F8,
divides the seed pair by `F8`, recomputes the distance); `camera_shift_step`
(`:923`) is `0x8DC50`. **This supersedes FU293-L1/L3's "no engine storage
exists" for these cells**: the FU-152 §2.9 camera port already carried them,
so FU293 §1's absence claim (§1 "The interpolation's actual inputs are
absent…") was inaccurate for `vel_x/vel_z/speed` and the
`event_peak/ramp_divisor/timer_limit/timer` family.

**Missing (the wide body).** (a) the Block E writer state machine (seed /
`CD514` / negate / `±int8[desc+0x1B]` / `8DC50` `>>2|>>3`) — today the lead
consumers read `lead_x/lead_z = 0` stand-ins and the `camera_zero`/`0x7F58A`
clears; (b) the descriptor bytes + `[0x14C2F6]` inputs; (c) the
`delta = timer_limit − timer` scale and the `{8DC68, C0·delta, C2·delta}`
vector; (d) the `0x71C94` hand-off with the computed vector (the current stub
`mr->match1b_wide_leg` at `handlers.c:3870` only binds `camera.tracked` via
`fifa96_camera_set_tracked`, `0x77423`→`0x71D27`). The port shape is therefore
**field-level**: write the real Block E state machine into `vel_x/vel_z/speed`,
then call `fifa96_camera_event_set(&mr->render.camera, v1, v2, event_param,
ramp_param)` (the native sequence reads the record it just wrote — the engine
signature takes the pair directly, carried FU-152/FU294).

---

## 2. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| seed extents `0x77035..0x7707A` (`[0x14C2F6]`, bit1 `byte[desc+0xC]` → ESI/ECX) | `disassemble_bytes 0x77035` |
| Block A `0x7707D..0x770D8` (`0x8DCD4` scratch read, `d<0x1E0` ×3/4 + the `0x10−bit0` adjust, the unconditional `ECX ×3/4` at `0x770C5`) | `disassemble_bytes 0x7707D` |
| Block B heading gate (`0xCD474` at `0x77116`, `&0x3FF` mask `0x77123`, `0x80`/`0x20` gates) | `disassemble_bytes 0x7707D` |
| Block C jitter (`0x92AC8`, `(2−difficulty)·(rand&0xF)`, `≤0x10`, `0x74584` 4-arg call at `0x771DA`) | `disassemble_bytes 0x7707D` |
| Block D route (`0x157C57`→`0x774D7`; `>>flag` at `0x771FF`; `0x157C58`→`0x774DD`) | `disassemble_bytes 0x771E8` |
| Block E writes: `0x77241`/`0x77255` seeds, `0x77275` `CD514`, `0x7728B` negate, `0x772A3`/`0x772B8` ±b, `0x772D7`/`0x772E3`/`0x772FB`/`0x7730D` reduces | `disassemble_bytes 0x771E8` |
| Block F vector `0x7733F..0x77385` (`timer_limit−timer`, `IMUL` pair, `0x8DC68`) + `0x7738A [0x157C57]=1` | `disassemble_bytes 0x77313` |
| Block G hand-off `0x77410..0x7741A` (`EDX=ESP+8`, `ECX=0`, `[0x157A6C]=0`) | `disassemble_bytes 0x77313` |
| `0x77420 MOVSX EBX,SI`; `0x77423 CALL 0x71C94`; `0x77428..0x77458` `[0x157A49]>>0x18==0x10`/`EDI` gates | `disassemble_bytes 0x7741B` |
| `0x71C94` bail + `CALL 0x700F4` + `MOVSD/MOVSW` record copy + `BX` max-clamp (`0x71CCB..0x71CE5`) | `disassemble_bytes 0x71C94` |
| `FUN_000700F4` zeroes `[0x1577B8/BA/BC/BE/C0/C2]` (`0x701D8..0x70204`) and `[0x1577F0..0x1577FA]` (`0x70159..0x701A8`) | `disassemble_bytes 0x701D0`; `disassemble_bytes 0x70140` |
| `FUN_00070544` writes `F2`/`F4`/`F6`/`F8`/`[0x1577FA]` (peak/ramp; idle arm `6/0xC/0/0xC`) | `disassemble_bytes 0x70544` |
| `FUN_00070544` callers = `FUN_000709D0 0x70B88`, `FUN_00071C94 0x71D65` | `get_xrefs_to 0x70544` |
| `0xCD514` = `|v|·2^16 / (0xCE386|0xCE364)` after the `0xCD474` angle abs/`0x200` fold | `decompile_function 0xCD514` |
| `0xCD474` = 2-arg 0x200-scaled atan2 (jump table on the octant) | `decompile_function 0xCD474` |
| `0x702F8` = height ramp `{0x94 cap, 3 floor, 0x94−T16[(0x640−F0)>>2]}` | `decompile_function 0x702F8` |
| `0x8DC50` = signed truncating `|v|>>n` | `decompile_function 0x8DC50` |
| `[0x14C2F6]` writers `0x374FB`/`0x3878C` both `= FUN_0001D940(9)` | `decompile_function 0x3749C`; `decompile_function 0x38630` |
| `FUN_0001D940(n) = dword[0x149278+n*4]`; static table all-zero | `decompile_function 0x1D940`; `read_memory 0x149278` |
| option-table setter family (`FUN_0001DA58`/`FUN_0001DCAC`/`FUN_0001DE94`/`FUN_0001E3A8`) | `get_xrefs_to 0x149278`; `get_xrefs_to 0x14929C` (none) |
| writer censuses (BE 15 W/70; C0 23 W+2 RW/91; C2 21 W+2 RW/54; F2 3 W/14; F8 3 W/20; F4 3 W/12; F6 6 W/26) | `get_xrefs_to 0x1577BE`/`0x1577C0`/`0x1577C2`/`0x1577F2`/`0x1577F8`/`0x1577F4`/`0x1577FA` |
| engine cells `vel_x/vel_z/speed/timer/timer_limit/ramp_divisor/event_peak/follow_speed`; `camera_event_ramp`; `camera_shift_step` | `fifa96_camera.h:12/15/16/17/37/61/68`; `fifa96_camera.c:728-774/923` |
| stub `match1b_wide_leg` + `fifa96_camera_set_tracked`; lead stand-ins; `screen_lead_z` staged 0; OL-65/OL-66 | `handlers.c:3870-3872`; `handlers.c:2651/2967/3075`; `match_run.h:704/949`; `handlers.c:956-960` |
| `[0x1577BE]` engine model (`camera.speed`, follow gate) | `fifa96_camera.h:15`; `match_run.c:3776`; `match_entities.c:771` |

**Carried docs (read, not rewritten):** FU285 §1/§3 (the row-`0x1B` map);
FU293 §1 (the L1 interpolation first read — corrected in §1.7/§1.9 and the
legs); FU-148 §2/§5.2/§14 (the camera/event cells); FU-152 §2.9 (the event
port); FU157/FU167/FU171 (the pan-cell semantics); FU204/FU256 (the
`[rec+4]`-descriptor class); FU290–FU297 register (`docs/ENGINE.md:1765-1914`).

---

## 3. Numbered open legs (with native owners)

* **FU301-L1 — the pan-cell writer state machine is unported (the L1 state).**
  The cells exist as `fifa96_camera.vel_x/vel_z/speed` (fresh `camera.h:12/15`),
  but no engine body performs Block E's seed / `CD514` / negate /
  `±int8[desc+0x1B]` / `0x8DC50 >>2|>>3` sequence; the consumers read the
  `lead_x/lead_z = 0` stand-ins (`handlers.c:2651/2967/3075`) and the clears
  (`ball_pairing.c:173`, `action_handlers.c:522`). **Corrects FU293-L1's "no
  engine storage exists."** Owner: `0x7721B..0x77312`; `mr->match1b_wide_leg`.
* **FU301-L2 — `[0x14C2F6]` is unmodeled.** Fresh writers: `FUN_0003749C`
  `0x374FB` / `FUN_00038630` `0x3878C`, both `= FUN_0001D940(9)` =
  `dword[0x149278+0x24]`; static table zero; the option-9 producer family is
  `FUN_0001DA58`/`FUN_0001DCAC`/`FUN_0001DE94`/`FUN_0001E3A8`. Engine: OL-66
  zero-image default (`handlers.c:960`) / `goal_gate` caller-supplied. Owner:
  `0x374FB`/`0x3878C` + the `0x149278` table; `handlers.c:960`.
* **FU301-L3 — the ratio descriptors (`byte[desc+0xC]` bits {0,1},
  `byte[desc+0xF]` int8, `int8[desc+0x1B]`) have no located producer.** Fresh
  in-window reads at `0x77044/0x7704F`, `0x770B6/0x770BE`, `0x771F9/0x771FF`,
  `0x7729C/0x772B1`, `0x772BF/0x772CD`; the `[rec+4]` binding is the
  FU167/FU168/FU176/FU204 leg class; engine: OL-65 roster descriptor unported
  (`handlers.c:956-960`). Owner: those sites; `handlers.c:3924`.
* **FU301-L4 — the `[0x1577F2]`/`[0x1577F8]`/`[0x1577F4]`/`[0x1577FA]` family
  is ported, but the wide-block alias delta has no consumer.** Writers fresh:
  `FUN_00070544` (`0x7058B`/`0x70605` F2; `0x7059A`/`0x7060C` F4; `0x705E1`/
  `0x7061A` F6; `0x705E8`/`0x70613` F8) + the `FUN_000700F4` clears
  (`0x70160`/`0x70167`/`0x70187`/`0x701A8`); the port has
  `event_peak`/`timer_limit`/`timer`/`ramp_divisor` (`camera.c:747-760`).
  `delta` must be read live as `timer_limit − timer`: `[0x1577FA]` also has
  `FUN_000736AC` (`0x7379A`/`0x73A74`) and `0x7EFE3` writers. Owner:
  `0x7733F..0x77350`; `camera_event_ramp`.
* **FU301-L5 — the pan vector + hand-off are the `match1b_wide_leg` stub.**
  The 6-byte vector `{0x8DC68(C0·delta, C2·delta), C0·delta, C2·delta}`
  (`0x77354..0x77385`) → `0x71C94` record copy `[0x1577B8..0xBD]`; the engine
  marker + `fifa96_camera_set_tracked` (`handlers.c:3870-3872`) is the
  `0x71D27` bind only; the setter round-trip (record → `F8` divide → `vel_x/
  vel_z` → `speed`) is ported (`camera_event_ramp`). Owner: `0x7733F..0x77423`.
* **FU301-L6 — the hand-off `BX` arg is camY-derived, not the jitter (FU293 §1
  erratum).** Fresh: `ESI` reloaded at `0x77228` (`dword[0x157750]`), so
  `0x77420 MOVSX EBX,SI` passes sign-extended `word[0x157750]` into
  `0x71CCB..0x71CE5`'s `[0x1577F0]` max-clamp. The register-allocation
  assumption that the intervening calls preserve `ESI` is itself unverified.
  Owner: `0x77228`/`0x77420`; `fifa96_camera_event_set`.
* **FU301-L7 — the seed/extent machine (`0x77035..0x77162`) is unported.**
  The `[0x14C2F6]`-conditioned extents, the `0x8DCD4` keeper-distance scale and
  the `0xCD474` heading gate (exits to `0x77636`) have no engine consumers; the
  stage machine collapses to the band gate (`handlers.c:3859-3876`). Owner:
  `0x77035..0x77162`; `fifa96_match_action_1B`.
* **FU301-L8 — the wide-block latches.** Block E's `byte[0x157C59]=1`
  (`0x7721B`) and the post-vector `byte[0x157C57]=1` (`0x7738C`) have no engine
  write site (the stub writes `[0x157C55]` = `keeper_latch[1]`,
  `handlers.c:3872`; the `[0x157C57/58/59]` = `keeper_latch[3/4/5]` naming is
  carried FU293/FU292). Owner: `0x7721B`/`0x7738C`.
* **FU301-L9 — the `0x92AC8` stream positions are a determinism contract.**
  Block E consumes the RNG at fixed points (two draws only on the `BE==0`
  seed, one jitter draw only inside the Block C condition, one reduce draw
  always); any port must consume the identical stream positions or the null
  backend diverges. Owner: `0x92AC8` call sites `0x7719F`/`0x77233`/`0x77247`/
  `0x772C5`.

## Review erratum (wave review, 2026-10-16)

Wave review adjudicated the seat-argument dispute in this draft's favor: `SI = dword[0x157750]` loaded at `0x77228`; FU302-L10's "BX = jitter" is wrong.

**Verdict:** GO — seat-argument reading upheld (`SI = dword[0x157750]` loaded at `0x77228`); FU302-L10 "BX = jitter" superseded.
