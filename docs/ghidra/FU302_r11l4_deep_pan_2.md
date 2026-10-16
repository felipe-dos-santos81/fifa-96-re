# FU302 — the row-`0x1B` wide-path completion: the six folds, the `EDI` sign-match + install selection, the sit dispatch, `FUN_00076B28` steering — through the `0x77423` setter seat

**Phase/Provenance:** M2 phase-28 recon-ahead, Task 5 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0,
`.image` overlay, 2706 functions; confirmed this session by
`get_current_program_info`). Every native address/byte below is a **fresh**
`disassemble_bytes` / `disassemble_function` / `decompile_function` /
`read_memory` / `get_xrefs_to` of *this* session unless marked *carried* (the
frozen `docs/ghidra/FU285_*.md` / `FU293_*.md` / `FU294_*.md` slice is named).
Alias traps applied (`dword[x]>>16` = the word at `x+2`; `SAR EAX,0x18` after a
dword load = the byte at `+3`). Every CALL rel32 below was checked by arithmetic
from the fresh bytes. No rename / comment / label / function-create / script /
project-save; `/FIFA96.EXE` untouched; no git.

This draft **completes** FU293's block-level skeleton (`docs/ghidra/FU293_r11l4_row1B_l1_interp.md`,
the `0x7707D..0x7741F` interpolation): it pins the **six fold primitives** the
brief names, resolves the **`EDI` flag to an exact (not carried) predicate**,
records the **install-selection tree** with every call argument, opens the
**sit dispatch** (`0x92820(rec,0x1A)`), re-derives **`FUN_00076B28`** end to end,
and re-censuses the **`[0x1577CA]` stand-in** — i.e. the full port shape of the
wide path up to and including the `0x77423 CALL 0x71C94` seat (the setter itself
is already ported, FU294/FU-148 §12.1).

---

## 1. The six folds (fresh semantics + port status)

### 1.1 `FUN_0008dcd4` `0x8DCD4` — octagonal ground distance with dx/dz out

Fresh `disassemble_function 0x8DCD4` (61 insns, `0x8DCD4..0x8DD5B`). In:
`EAX` = position A word-record, `EDX` = position B word-record, `EBX` = out
struct. It computes `dx = word[B+0] - word[A+0]`, `dz = word[B+8] - word[A+8]`
(16-bit subs), stores `out[1] = dx`, `out[2] = dz`, then the octagonal
`max + f(min)` with `f = min/4`, or `3*min/8` when `min > max/2`, and stores
`out[0] = distance`. **Ported: `fifa96_arm_dist_stage`** (`fifa96_arm_helpers.c:38`,
FU-142b; word-truncating dx/dz + the same fold, verified line-by-line this
session). Wide call site fresh `0x7708F`: `0x8DCD4(EAX = rec+0x59,
EDX = keeper+0x59, EBX = ESP)`; the distance is read back as
`dword[ESP-2]>>16` = the word at `ESP+0` (alias).

### 1.2 `FUN_000cd474` `0xCD474` — atan2 in 0x400-per-circle units

Fresh `disassemble_function 0xCD474` (26 insns; args `[EBP+8]` = A, `[EBP+0xc]` = B).
`ratio = min(|A|,|B|)/max(|A|,|B|)` (the swap flag bit 2 = `|A|>|B|`),
index `= (ratio>>24) + bit23` into the 0x101-byte table `0x14072C`
(`disassemble_function` shows `MOVZX EAX,byte[EAX+0x14072c]`), then the
8-case quadrant jump table fresh `read_memory 0xCD4C0` =
`{0xCD4E7,0xCD4E0,0xCD4E9,0xCD4F2,0xCD4F9,0xCD4FD,0xCD504,0xCD50B}` (fresh
`disassemble_bytes 0xCD4DD`): `{+0, 0x100-, 0x200-, +0x100, NEG, -0x100,
-0x200, NEG-0x100}`. Equality returns 0x80 before the dispatch. So
`0xCD474(A,B) = atan2(A,B)`, result in `-0x200..0x200`. **Ported:
`fifa96_entity_angle`** (`fifa96_entity_update.c:75`, M2 T14/OL-41;
`fifa96_action_kick_angle` forwards). Wide call site fresh `0x77116`:
`0xCD474(A = word[rec+0x6B], B = word[rec+0x6D])` (`e859630500` → `0xCD474`).

### 1.3 `FUN_000cd514` `0xCD514` — trig-normalized magnitude (hypot) — **NOT ported**

Fresh `disassemble_function 0xCD514` (41 insns): calls
`0xCD474(A = [EBP+0xc] = arg2, B = [EBP+8] = arg1)` (push order fresh
`0xCD517`/`0xCD51A`), takes `|angle|`, folds `0x200 - |angle|` when `> 0x100`,
then: `angle <= 0x80` → `(|arg1| << 16) / FUN_000CE386(angle)`; else
`(|arg2| << 16) / FUN_000CE364(angle)`. Fresh `0xCE364` = the 0x114E04-table
sine fold (`sine(angle)`); fresh `0xCE386` = the same fold over `angle+0x100`
(`cosine`). With `angle = atan2(arg2, arg1)`, both branches are
`hypot(arg1,arg2)`: `|arg1|/cos` (`arg1 ≈ h·cos`) or `|arg2|/sin`. Wide call
site fresh `0x7726D`: `0xCD514(a = [0x1577C0], b = [0x1577C2])` (the pushes
fresh 0x7725B..0x7726C un-alias to the words at `0x1577C0`/`0x1577C2`), result
`word[0x1577BE] = hypot(pan_x, pan_z)`. **The engine has no exact port**; the
nearest sibling is `fifa96_entity_intercept_band`
(`fifa96_entity_update.c:145-165`: angle → fold → `sine(a)/sine(a+0x100)`
divisor → `(|dx|<<16)/divisor`), which performs the same operation on a
`(dx,dz)` pair — the arg-order equivalence across `0xCD514`'s six call sites
(fresh `get_xrefs_to 0xCD514` = 6: `FUN_0007131c` × 2, `FUN_000795b4`,
row-`0x1B` `0x7726D`, `FUN_000b3080` × 2) is **unproven** → **FU302-L1**.
Also fresh the `0x8DC50` reduce neighbours: `0x8DC50(v,n)` = signed shift
(`v>=0 ? v>>n : -((-v)>>n)`), **ported** as the static `camera_shift_step`
(`fifa96_camera.c:924`).

### 1.4 `FUN_0008dc68` `0x8DC68` — octagonal magnitude (short domain)

Fresh `disassemble_function 0x8DC68` (54 insns): `|a|`,`|b|` (short-domain),
`max + min/4` or `max + 3*min/8` (strict `min > max>>1`), returns the short.
**Ported: `fifa96_entity_distance`** (`fifa96_entity_update.c:187`, M2 T14).
Wide call site fresh `0x77380`: `EAX = dword[ESP+8]>>16` = `step_x`,
`EDX = dword[ESP+0xA]>>16` = `step_z` (the words just stored at `ESP+0xA`/`ESP+0xC`),
result `word[ESP+8] = mag(step_x, step_z)`.

### 1.5 `FUN_00074584` `0x74584` — the keeper-relation band predicate — **NOT ported**

Fresh `disassemble_function 0x74584` (40 insns, **`RET 0x10` = 4 cdecl args**;
reads `[ESP+0x10]`/`[ESP+0x18]`/`[ESP+0x1c]` after 3 pushes = args 1/3/4; arg 2
dead). Wide call fresh `0x771DA` (`e8a5d3ffff` → `0x74584`) pushes, in order,
`rec` (via EBP), `[keeper+0x61]`, `[keeper+0x5D]`, `[keeper+0x59]` — so
`arg1 = keeper.x`, `arg2 = keeper.y` (unused), `arg3 = keeper.z`, `arg4 = rec`:

```
d  = (int8) byte[ [[rec]+4] + 0xC ]              ; the descriptor byte
t  = (int8)(0x10 - d) * 2 + 0x3C0                ; 2*(16-d)+0x3C0
if (|keeper.x| > t) return 0
t2 = 0xB10 - t                                   ; 2*d+0x730
if (byte[[[rec]+0x826] side] == 1) return keeper.z > t2
return keeper.z < -t2
```

Fresh `get_xrefs_to 0x74584` = **2**: `0x771DA` (row `0x1B` wide) and `0x74B12`
— inside the **row-`0x19` body** (row-table head `0x746E4`, fresh
`read_memory 0x1106E0`), whose own call passes the camera triple
(`[0x15774C/50/54]`) as x/y/z + `EBP`. Unported → **FU302-L2**.

### 1.6 `FUN_000702f8` `0x702F8` — the steering ramp — ported

Fresh `disassemble_function 0x702F8` (31 insns): in `AX` = angle, `EDX` =
passthrough (returned in the high half, dropped by callers). `AX >= 0x640` →
0x94; `AX <= 0` → 3; else `0x94 - byte[ dword[0x157748] + ((0x640-AX)>>2) ]`
clamped `>= 0`. The `0x157748` cell holds the table pointer (installed by
`FUN_0006FFC0`; static table `0x10F4EE`, `camera.h:274-278`).
**Ported: `fifa96_camera_ramp`** (`fifa96_camera.h:278`). Fresh
`get_xrefs_to 0x702F8` = 25 (incl. the steering call `0x76B7C` and the setter
family `FUN_00070544`/`FUN_00071c40`).

---

## 2. The wide path, end to end (fresh block reads; corrections against FU293)

Fresh spans: `0x76FD5` (entry gates), `0x7707D` (blocks A–E-head),
`0x772AC` (blocks E-tail–G + the install tree). The block map is FU293 §1's;
the **new/corrected** facts are:

* **Descriptor seed (`0x77035..0x7707A`) — correction.** Fresh: with
  `d = [[rec+4]+9] >> 0x19`, the difficulty-1 branch computes
  `ECX = 8 - d`, `ESI = 2*(8-d)`, then **`ESI = 0xA0 - ESI` and
  `ECX = 0x60 - (8 - d)`** (`0x77044..0x7706D`: `MOV EAX,0x60; SUB EAX,ECX;
  MOV ECX,EAX`). FU293 §1 quotes only the intermediate `ECX = 8 - d` — the
  value entering Block A is **`0x60-(8-d) = 0x58+d`**. The difficulty≠1 branch
  fresh confirms `ECX = d + 0x70`, `ESI = 0xA0`.
* **Block A (`0x7707D..0x770D9`) — completion.** Fresh: `EDX = [0x1577CA]`; if
  non-zero the `0x8DCD4` call above; `d < 0x1E0` → `ESI -= ESI>>2`
  (`0x770A2..0x770AD`); difficulty `== 0` → `ECX -= 0x10 - (ratio>>0x18)`
  (`0x770B3..0x770C3`); then **`ECX -= ECX>>2` unconditionally**
  (`0x770C5..0x770CA`) — FU293's text omits this ¾ scale; `d >= 0x1E0` →
  `ECX += ratio>>0x19` (`0x770CE..0x770D7`).
* **Block B (`0x770D9..0x77162`)** fresh matches FU293: `[0x157750]==0 →
  ESI=0xC0`; the two `-> 0x77636` gates; `0xCD474(A = word[+0x6B],
  B = word[+0x6D])`; `heading = (angle - word[+0x7D]) & 0x3FF` (`0x7711B
  MOV CX,word[EBP+0x7D]; SUB EAX,ECX; AND CH,3`); `> 0x200 → -0x400`;
  `|heading| >= 0x80 && word[+0x69] > 0x20 → 0x77636`.
* **Block C (`0x77163..0x771E7`)** fresh matches FU293: threshold `0x780` (diff
  1) else `0x5A0`; `[0x1577CA]!=0 && |dword[keeper+0x61]| > threshold` →
  `ESI = (2-diff)*(rand&0xF)` (`0x92AC8`), clamp `<= 0x10`; then the fresh
  `0x74584` call above → `ECX = 1` when AL.
* **Block D (`0x771E8..0x7721A`)** fresh matches: `[0x157C57]!=0 → 0x774D7`;
  `EAX = (ratio>>0x19) >> CL` (CL = the keeper-relation flag); if
  `ESI >= EAX` and `[0x157750] <= 0x20` then `[0x157C58]==0 → 0x774DD`.
* **Block E (`0x7721B..0x77312`) — precision + the `0xCD514` use.**
  `[0x157C59]=1`; if `word[0x1577BE]==0`: `c0 = [0x1577C0] = (rand&7)+8`,
  `c2 = [0x1577C2] = (rand&7)+8`, then `[0x1577BE] = 0xCD514(c0, c2)`
  (hypot, §1.3). Always: `[0x1577C2] := -[0x1577C2]`; **if `[0x1577C0] > 0`
  then `+= (int8)byte[[rec]+0x1B]`, else `-=` it** (`0x77297..0x772B8` — FU293
  noted the add only). Then the `[[rec+4]+0xC]`-gated reduce: fresh
  `(rand&0xF) + 4 >= (([[rec+4]+0xC])>>0x18)` (`0x772BF..0x772D5`, `JNC` = the
  code-**2** arm) → `[0x1577C0] = 0x8DC50([0x1577C0], 2)` **and**
  `[0x1577C2] = 0x8DC50([0x1577C2], 2)` (`0x772E3..0x7730D`); the other
  (code-**3**) arm reduces **only `[0x1577C2]`** (`0x772D7..0x7730D`). FU293's
  "picks code 2/3 … writing `[0x1577C0]` and `[0x1577C2]`" is exact only for
  the code-2 arm.
* **Block F (`0x77313..0x7738A`) — the `EDI` flag + the vector.**
  `0x77313 CMP word[0x1577C2],0; SETG AL` → `b_pan = ([0x1577C2] > 0)`;
  `0x77320 MOV ECX,[0x157754]; TEST ECX,ECX; SETG AL` → `b_cam =
  (dword[0x157754] > 0)`; `CMP EDX,EAX; JNZ 0x7733F; 0x7733A MOV EDI,1`.
  **`EDI = 1` iff the two same-sign predicates are equal; there is no reset in
  the block — but the body head `0x76D28` runs `0x76D3B XOR EDI,EDI` (fresh,
  before the phase gate `0x76D40`), so on mismatch `EDI` is exactly 0.** The
  flag is therefore the exact predicate `sign_match`, not a carried register
  (FU293-L5's "it is a carry the whole block computes" is corrected: the reset
  is at the body head, `0x76D3B`). Vector construction fresh: `delta =
  dword[0x1577F2]>>16 - dword[0x1577F8]>>16` = `word[0x1577F4] - word[0x1577FA]`
  (`0x7733F..0x77350`); `step_x = (int16)(word[0x1577C0] * delta)` stored
  `word[ESP+0xA]` (`0x77354..0x7735E`); `step_z = (int16)(word[0x1577C2] *
  delta)` stored `word[ESP+0xC]` (`0x77363..0x7736D`); then the fresh `0x8DC68`
  call (§1.4) stores `word[ESP+8] = mag(step_x, step_z)`. **The 6-byte vector
  at `ESP+8` = `{mag, step_x, step_z}`**, which `0x71CC8/0x71CC9` copies
  wholesale to `[0x1577B8..BD]` — fresh support for the FU294 erratum's
  "`event_step_z`" reading of `[0x1577BC]` on **this** route (the FU292
  camera-band reading of the cell stays for its own writer family).
  `0x77392 byte[0x157C57] = 1`.
* **Block G (`0x7738A..0x7741F`) — dispatch + the seat.**
  `[0x157823]!=0` → clear it, per-side counter increments
  `word[0x157ACC + 2*((side^1)^swap)]` / `word[0x157AD0 + 2*(side^swap)]` via
  the fresh `FUN_000741B4` (`x ^ [0x157ABE]`, low half; the same XOR idiom as
  FU291's gate); `0x92820(EAX=rec, DL=0x1A)`; `FUN_000741B4(side)` →
  `(side^swap)==0 ? 5 : 0xE` → `0x651F0(code)` (fresh `0x773E3`/`0x7740B`,
  rel32-checked). Then the seat fresh `0x77410..0x77423`:
  `EDX = &vec (LEA ESP+8)`, `EAX = rec`, `ECX = 0`, `BL = 0` →
  `byte[0x157A6C] = 0` (the setter bail cleared **before** the call),
  `EBX = MOVSX SI` (the Block-C jitter 0..0x10), `CALL 0x71C94`
  (`e86ca8ffff` → `0x71C94`). So the native seat is
  `FUN_00071c94(EAX=rec, EDX=&{mag,step_x,step_z}, BX=jitter, ECX=0)`.
* **The install selection (`0x77428..0x774D2`) — the `EDI` consumer.** Fresh:
  `0x77428 EAX = [0x157A4A] >> 0x18` (byte `0x157A4D`) `== 0x10` **&&**
  `EDI == 0` → `0x8F188(EAX=6, EDX=rec, EBX=8)` (`e83e7d0100`) +
  `0x8ED40(EAX=rec, EDX=0x64)` (`e8ea780100`) → `JMP 0x774C0`. Else
  `EDI == 0` (`0x77458`): `p = (int16)word[0x1577C2] * dword[0x157754]`
  (`0x7745C..0x7746A`, the pan-z × cam-z sign product); `p >= 0` →
  `0x8F188(3, rec, 0x20)` + if `[0x1577CA]!=0` `0x8ED40(tracked, EDX=5)` +
  `0x8ED40(rec, 0x19)` → `JMP 0x774C0`; `p < 0` → `0x8ED40(rec, 5)` +
  `0x8F188(0, rec, 0x20)` (`0x774A6..0x774BB`). `EDI != 0` (sign match) →
  straight to the terminal. **Terminal `0x774C0`:** `EAX=4; BH=1;
  CALL 0x974DC` (`e810000200`), then `byte[0x157C55] = 1`, `JMP 0x77636`
  (stage-4 head). The sibling tail inside the `0x774DD` re-place
  (`0x7759C..0x77631`: the parallel `0x741B4`/`0x651F0`/`0x8F188(6,…)`/
  `0x8F188(2,…)`/`0x8ED40(…,0x19)`/`0x974DC(4)` tree) is **carried**
  (FU285 §4; engine comment `handlers.c:3736-3745`).

**Engine seat mapping (the completion).** The `0x71C94` port already exists:
`fifa96_camera_event_set(cam, seed_x, seed_z, height, ramp_param)`
(`fifa96_camera.c:827`) — it clears `tracked` (the `0x700F4` subset,
`camera.c:864`) and sets `event_param = clamp(height, [target_y, 0x640])`.
The wide seat therefore ports as, in order: `cam->event_suspended = 0`
(`[0x157A6C]`, `0x7741A`); `fifa96_camera_event_set(&mr->render.camera,
step_x, step_z, (int16_t)jitter, 0)`; `fifa96_camera_set_tracked(&cam, id)`
(the `0x71D27` bind — same pairing as `handlers.c:2717-2722`); and
`mr->keeper_latch[3] = 1` (`[0x157C57]`, `0x77392`). The three latches exist
(`keeper_latch[8]`, `match_run.h:680`; index = `0x157C54 + i`). `step_x`/
`step_z` are the `camera.event_step_x/z` cells (`camera.h:42`); the `mag`
first word has no engine sink on this route (`fifa96_camera_record_event`'s
`out->distance` is the existing reporting pattern, `camera.h:380`). The
install-selection sinks (`0x8F188` = the 10-slot event ring, `0x8ED40` = the
side-ring stat accumulator, `0x974DC` = the `FUN_00064ec0` sound pair) are
**unported** (OL-27/OL-82 family, `handlers.c:3870` stub keeps
`mr->match1b_wide_leg`).

---

## 3. The sit dispatch (fresh `0x92820`)

Fresh `disassemble_bytes 0x92820` (33 insns, `0x92820..0x92861`; in
`EAX = rec`, `DL = code`): `EBP = EAX`; `AL = byte[0x110F1C + MOVSX(DL)] & 1`;
**`byte[0x15B650] = DL`** (the sit code byte); if the table bit: `CALL
0xCB2A4` (the time/serial helper) → `[0x15B651]`, `MOVSD ×3` copies
`[0x157758..0x157763]` → `0x15B659`, `[0x15B655] = rec`. (The decompiler's `unique0x10000064`/`ram0x`
placeholders are these stores; the fresh bytes are the citation.) Then
`0x651F0(code)` (`void`, fresh decompile: guarded by `[0x155CE0]/[0x155D38]/
[0x155D08]`, stores `_DAT_00155D50 = code; FUN_000A72E7(code)`) and the
`FUN_000741B4` XOR above. **Unported** (OL-27/OL-82, the row-`0x1C` sibling
comment) → **FU302-L5**.

## 4. `FUN_00076B28` steering (fresh full derivation; FU293-L8 stands)

Fresh `disassemble_function 0x76B28` (155 insns, `0x76B28..0x76D11`; in
`EAX = rec`, `EBX = 0x60`, `EDX = 0xA0` — call site fresh `0x76F96..0x76FA2`,
row-`0x1B` stage 0). Writes: `word[+0x83] = (word[+0x51] - EBX + 0x20)`,
clamped `[0,0x60]` **via the just-written cell** (`0x76B5C EAX = dword[+0x81]
>>16` = the word at `+0x83`); if `<= 0` then `[+0x83]=0, [+0x85]=0`, else
`[+0x85] = 0x702F8(word[+0x83])`; `[+0x87]=0`; the second extent
`local2 = (word[+0x65] - EDX + 0x20)` clamped `[0x10,0x70]` (`ESP+0`);
`MOVSD ×3` pos copy `+0x59 → +0x4D`; `angle = 0xCD474(A = word[+0x67],
B = word[+0x69])`; `[+0x4D] += 0x795A4(local2, sine(angle))`,
`[+0x55] += 0x795A4(local2, sine(angle+0x100))` (`0x76BE6..0x76C4D`; the
fresh fold is the `0x114E04` table + SBB sign trick); `0x795B4(EAX=+0x59,
EDX=+0x4D, EBX=+0x65)`; if `[+0x85]!=0`: `k = min(local2 / word[+0x83], 0xF)`
(`IDIV`, `0x76C7E`, clamp `0x76C88`), `[+0x73] = 0x795A4(k, sine(angle))`,
`[+0x75] = 0x795A4(k, sine(angle+0x100))`, else both 0; finally
`[+0x71] = 0x8DC68(word[+0x73], word[+0x75])` (fresh `0x76D01`).
Fresh `get_xrefs_to 0x76B28` = **2** (`0x76FA2` row `0x1B` stage 0;
`0x779B4` row `0x1C` — carries FU285 §3). **Unported**
(`handlers.c:3923-3930`, FU285-L4/`FU186-L2`); its fold set is §1's plus
`0x795A4` (ported as the static `camera_mul16`, `camera.c:717`) and `0x795B4`
(the band helper — unported; its own xrefs include `0xCD514`).

## 5. The `[0x1577CA]` stand-in (FU293-L7 completion)

Fresh `get_xrefs_to 0x1577CA` = **88** (3 WRITE: `0x71D27` the setter store;
`0x70258 FUN_000700f4`; `0x70054 FUN_0006ffc0`), matching FU294's carried
census. The wide-path reads fresh: `0x76FE1` (the mutual-exclusion gate),
`0x7707D` (Block A: the keeper position pair `+0x59`/`+0x61` via `0x8DCD4`),
`0x7717D` (Block C: `|dword[keeper+0x61]|` vs the `0x780`/`0x5A0` threshold),
`0x771C1` (Block C: the `0x74584` call), `0x77482` (the install tree:
`0x8ED40(tracked, 5)`), plus `0x77602` (stage-4 head). Engine stand-ins:
`camera.tracked` (`camera.h:43-54`, `fifa96_camera_set_tracked`; the mutual
exclusion `handlers.c:3861`) and the `is_ball_track = (id == controlled)`
stand-in (`handlers.c:2963`). The wide path's **deep** reads (positions,
`+0x61`) have no producer behind those stand-ins → **FU302-L7**.

---

## 6. Port shape (the completion through the `0x77423` seat)

1. **Folds**: reuse `fifa96_arm_dist_stage` (`0x8DCD4`), `fifa96_entity_angle`
   (`0xCD474`), `fifa96_entity_distance` (`0x8DC68`), `fifa96_camera_ramp`
   (`0x702F8`); port `0xCD514` (or prove `fifa96_entity_intercept_band`'s arg
   equivalence — L1) and `0x74584` (L2); expose/copy `camera_shift_step`
   (`0x8DC50`, currently file-static).
2. **New state**: the pan cells `[0x1577BE/C0/C2]` (word; `[0x1577BE]` is the
   same cell the engine reads as `follow_speed`, `camera.h:55-61`, but this
   producer is unported — L3), the descriptor byte `[[rec]+4+0xC]` and the
   ratio `[[rec]+4+9]` (FU293-L2), the latches `[0x157C57]/[0x157C59]`
   (`keeper_latch[3]/[5]` exist), the head flag `[0x157823]`, the side counters
   `[0x157ACC]/[0x157AD0]`, the difficulty `[0x14C2F6]`.
3. **Body**: Blocks A–G per §2 (with the two corrections: the seed's final
   `ECX = 0x60-(8-d)`; the Block-A unconditional `ECX -= ECX>>2`), the `EDI`
   flag (exact, head-zeroed), and the install tree (§2) — the sinks are
   unported (L4).
4. **Seat**: `cam->event_suspended = 0`;
   `fifa96_camera_event_set(&cam, step_x, step_z, (int16_t)jitter, 0)`;
   `fifa96_camera_set_tracked(&cam, id)`; `mr->keeper_latch[3] = 1`;
   `mr->keeper_latch[1] = 1` at the terminal; retire `match1b_wide_leg`.
5. **Sit dispatch**: `0x92820(rec,0x1A)` / `0x741B4` / `0x651F0` (L5);
   **steering**: `FUN_00076B28` (L6).

**GO** for the port shape (all six fold semantics pinned; four already ported;
the seat's setter ported). **NO-GO for a live install** — unchanged from phase
27: the install/tree sinks and the sit dispatch are unported, and the end-to-end
blockers (P1 `[0x110680]` + the dead interlock arming path, FU296/FU257-L6)
stand. This task moves no census.

---

## 7. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| `0x8DCD4` out `{dist,dx,dz}` + octagonal fold; `0x8DC68` short magnitude; jump-table-only quadrant code | `disassemble_function 0x8DCD4`, `0x8DC68` |
| `0xCD474` atan2: table `0x14072C`, `SHR 0x18/ADC`, equality 0x80; jump table `{0xCD4E7,…,0xCD50B}` with `{+0,0x100-,0x200-,+0x100,NEG,-0x100,-0x200,NEG-0x100}` | `disassemble_function 0xCD474`; `read_memory 0xCD4C0`; `disassemble_bytes 0xCD4DD` |
| `0xCD514(a,b)` = `hypot` via `0xCE386` (=`sine(a+0x100)`)/`0xCE364` (=`sine(a)`) with the `[0,0x100]` fold and `(x<<16)/sine` | `disassemble_function 0xCD514`; `disassemble_bytes 0xCE364`/`0xCE386` |
| `0x74584` `RET 0x10`; `t=2*(16-d)+0x3C0`, `t2=0xB10-t`; side-1 `z>t2` vs `z<-t2`; arg2 dead; callers `{0x771DA, 0x74B12}` | `disassemble_function 0x74584`; `get_xrefs_to 0x74584` |
| `0x702F8` ramp; `0x157748` holds the table pointer | `disassemble_function 0x702F8` |
| seed: difficulty-1 `ESI=0xA0-2*(8-d)`, final `ECX=0x60-(8-d)`; else `ECX=d+0x70` | `disassemble_bytes 0x76FD5` (`0x77035..0x7707A`) |
| Block A: `ESI-=ESI>>2`; diff-0 `ECX -= 0x10-(ratio>>0x18)`; **`ECX-=ECX>>2`**; `d>=0x1E0` `ECX += ratio>>0x19` | `disassemble_bytes 0x7707D` |
| Block B gates + `0xCD474(A=w[+0x6B],B=w[+0x6D])` + `(angle-w[+0x7D])&0x3FF` + `-0x400` + `0x80/0x20` gate | `disassemble_bytes 0x7707D` |
| Block C threshold `0x780`/`0x5A0`, jitter `(2-diff)*(rand&0xF)` clamp `0x10`, `0x74584` call arg pushes `{rec,[k+0x61],[k+0x5D],[k+0x59]}` | `disassemble_bytes 0x7707D` |
| Block E: `0xCD514(c0,c2)` → `[0x1577BE]`; `c2:=-c2`; `c0±=byte[[rec]+0x1B]`; `(rand&0xF)+4 >= desc>>0x18` → code-2 (both cells) else code-3 (`c2` only) via `0x8DC50` | `disassemble_bytes 0x7707D`, `0x772AC` |
| `EDI = 1` iff `(w[0x1577C2]>0)==(d[0x157754]>0)`; body head `XOR EDI,EDI` @`0x76D3B` | `disassemble_bytes 0x772AC`, `0x76D28` |
| vector: `delta=w[0x1577F4]-w[0x1577FA]`; `step_x/step_z` = truncated pan×delta; `word[ESP+8]=0x8DC68(step_x,step_z)`; `[0x1577C57]=1` | `disassemble_bytes 0x772AC` |
| seat: `EDX=&ESP+8, EAX=rec, ECX=0, BX=MOVSX SI, [0x157A6C]=0`, `CALL 0x71C94` (rel32 `e86ca8ffff`) | `disassemble_bytes 0x772AC` |
| install tree: `[0x157A4A]>>0x18==0x10 && EDI==0` → `0x8F188(6,rec,8)`+`0x8ED40(rec,0x64)`; `EDI==0` → `pan_z*camZ ∓` → `0x8F188(3,rec,0x20)`+`0x8ED40(tracked,5)`+`0x8ED40(rec,0x19)` / `0x8ED40(rec,5)`+`0x8F188(0,rec,0x20)`; terminal `0x974DC(4)`+`[0x157C55]=1` | `disassemble_bytes 0x772AC`; `decompile_function 0x8F188/0x8ED40/0x974DC` |
| sit dispatch `0x741B4(side)` = `side^[0x157ABE]`; `(side^swap)==0 ? 5 : 0xE`; `0x92820(rec,0x1A)` stores `0x15B650/51/55/59`, gate table `0x110F1C[0x1A]` | `disassemble_bytes 0x772AC`, `0x92820`; `decompile_function 0x741B4/0x651F0` |
| `FUN_00076B28` writes `+0x83/85/87/73/75/71`, pos fold `+0x59→+0x4D` via `0x114E04`/`0x795A4`/`0x795B4`, angle `0xCD474(w[+0x67],w[+0x69])`, speed `0x8DC68`; call site `EBX=0x60,EDX=0xA0,EAX=EBP` | `disassemble_function 0x76B28`; `disassemble_bytes 0x76F91` |
| xrefs: `0x1577CA` = 88 (3 W), `0x74584` = 2, `0xCD514` = 6, `0x702F8` = 25, `0x76B28` = 2 | `get_xrefs_to` each |
| row table: `0x1B=0x76D28`, `0x1D=0x74EB0`, `0x19=0x746E4` (so `0x74B12` ∈ row `0x19`), `0x43=NULL` | `read_memory 0x1106E0` (288 B) |
| engine ports: `fifa96_arm_dist_stage` (`0x8DCD4`), `fifa96_entity_angle`/`fifa96_distance`/`intercept_band` (`0xCD474`/`0x8DC68`/`0xCD514`-sibling), `fifa96_camera_ramp` (`0x702F8`), `camera_shift_step` (`0x8DC50`, static), `fifa96_camera_event_set` (`0x71C94` subset), `camera_mul16` (`0x795A4`) | `fifa96_arm_helpers.c:38`; `fifa96_entity_update.c:63/75/145/187`; `fifa96_camera.h:42/54/55/278/280-301`; `fifa96_camera.c:717/827/864/924` |
| stub/legs: `match1b_wide_leg` (`handlers.c:3870`), sinks `0x8F188/0x8ED40/0x974DC/0x92820` OL-27/OL-82, steering FU186-L2, `[0x1577CA]` stand-ins | `fifa96_match_handlers.c:2963/3720-3745/3847-3872/3923-3930` |

**Carried docs (read, not rewritten):** FU285 §2/§3/§4 (the setter write set and
the `0x774DD` self-actor); FU293 §1 (the block skeleton this draft completes);
FU294 §1/§3 (the setter port contract); FU290 `docs/ENGINE.md:1765-1914` (the
keeper-CPU leg register); FU296 (the seam).

---

## 8. Errata (quoted corrections to the frozen slices)

* **FU293 §1 seed — the final `ECX`.** FU293: "if `==1`, `ECX = 8 -
  ([[rec+4]+9]>>0x19)`, `ESI = 0xA0 - 2*ECX`". Fresh `0x77044..0x7706D`: that
  `ECX` is the intermediate; the value entering Block A is **`0x60 - (8 - d)`
  (`0x58 + d`)**.
* **FU293 §1 Block A — the missing ¾.** FU293 lists `ESI = ESI*3/4` and the
  difficulty-0 `ECX` adjustment. Fresh `0x770C5..0x770CA` shows **`ECX -=
  ECX>>2` unconditionally** in the `d < 0x1E0` branch (the difficulty-0 adjust
  happens before it).
* **FU293 Block E — the reduce split.** FU293: "picks code `2`/`3` … writing
  `[0x1577C0]` and `[0x1577C2]`". Fresh: the code-3 arm reduces **only
  `[0x1577C2]`**; the gate is `(rand&0xF)+4 >= ([[rec+4]+0xC]>>0x18)` (`JNC` =
  code 2). Also `[0x1577C0]` subtracts `[[rec]+0x1B]` when `<= 0` (FU293 quotes
  the add only).
* **FU293-L5 — `EDI` is not a carry.** FU293-L5: "`EDI` is **not** reset here,
  so it is a carry the whole block computes". Fresh `0x76D3B` (`XOR EDI,EDI` at
  the row-`0x1B` body head, before the `0xD` phase gate) makes the flag the
  exact predicate `sign(word[0x1577C2]) == sign(dword[0x157754])`; the install
  selection's `EDI == 0` tests are deterministic.
* **FU294 erratum support (not a resolution).** FU294 left the `[0x1577BC]`
  reading unreconciled (event-step vs camera-band). Fresh, the wide route's
  `0x71CC8` copy stamps the third word with `pan_z * delta` — the
  `event_step_z` reading holds on this route; the FU292 camera-band writer
  family is untouched.

---

## 9. Numbered open legs (with native owners)

* **FU302-L1 — `0xCD514` has no exact port.** The trig-normalized magnitude
  (`hypot` via `0xCE386`/`0xCE364`) is not in the engine; the nearest sibling
  `fifa96_entity_intercept_band` (`fifa96_entity_update.c:145`) performs the
  same operation on `(dx,dz)` — the arg-order/label equivalence across
  `0xCD514`'s six callers is unproven. Owner: `0xCD514`; `0x7726D` (wide);
  `fifa96_entity_intercept_band`.
* **FU302-L2 — `0x74584` (keeper-relation band predicate) is unported.** Exact
  formula + arg pushes recorded (§1.5); the second caller is the row-`0x19`
  body (`0x74B12`, head `0x746E4`). Owner: `0x74584`; rows `0x19`/`0x1B`.
* **FU302-L3 — the pan-cell producer state `[0x1577BE]/[0x1577C0]/[0x1577C2]`.**
  Fresh `0x7721B..0x77312` seeds/negates/reduces them; the engine's only touch
  point is `follow_speed` (a derived read at the `0x739CE` gate, `camera.h:55`),
  not this producer. FU293-L1 stands, narrowed to these three cells. Owner:
  `0x7721B`; `fifa96_camera.h:55`.
* **FU302-L4 — the `EDI` flag + install-selection tree are unported.** Fresh
  `0x77313`/`0x77428..0x774C0` with the exact sink calls (§2); the sinks
  `0x8F188`/`0x8ED40`/`0x974DC` are themselves unported (OL-27/OL-82 family).
  Owner: `0x77428`; `handlers.c:3870` (`match1b_wide_leg`).
* **FU302-L5 — the sit dispatch `0x92820(rec,0x1A)` + `0x741B4`/`0x651F0` is
  unported.** Fresh stores `0x15B650/0x15B651/0x15B655/0x15B659`, the
  `0x110F1C` id-table gate, the `side^[0x157ABE]` index and the `5`/`0xE`
  selection recorded. Owner: OL-27 / OL-82; `0x92820`.
* **FU302-L6 — `FUN_00076B28` steering is unported (FU293-L8/FU285-L4
  stand).** The full write set and fold dependencies are recorded (§4); its
  band helper `0x795B4` is itself unported (its xrefs include `0xCD514`).
  Owner: `0x76B28`; FU186-L2.
* **FU302-L7 — the `[0x1577CA]` tracked-keeper deep reads are unattached.** The
  wide path reads the tracked keeper's position pair and `+0x61`; the engine's
  stand-ins (`camera.tracked`, `handlers.c:2963/3861`) carry none of that
  state. Owner: `0x7707D`/`0x771C1`; `fifa96_match_handlers.c:2963`.
* **FU302-L8 — the install-gate phase byte.** The wide tree's gate is
  `byte[0x157A4D] == 0x10` (fresh `0x77428`); the head's phase gate is the same
  byte `== 0xD` (fresh `0x76D33`), which the engine maps as `mr->state.phase`;
  the FU285-L2 `0x157A4C` (`global_157a4c`) vs `0x157A4D` reconciliation stays
  open. Owner: `0x77428`; `handlers.c:3788`.
* **FU302-L9 — dword-residue sign/product artifacts.** The native sign tests
  and products read dwords whose high halves are residue (`[0x157754]` at
  `0x77320`/`0x77461`; `[rec+0x61]` at `0x7718B`; the `MOV DX,…; IMUL` reads at
  `0x77354`/`0x77363`). A faithful port uses the word value; the residue is
  unreachable state and must not be re-derived. Owner: `0x77313`/`0x7717D`.
* **FU302-L10 — the seat's `BX` argument is the jitter (0..0x10), not a
  height.** `0x77420 MOVSX EBX,SI` feeds `0x71C94`'s `param_3` (FU294's table
  names it "height"); the setter's `[0x1577F0]` low-clamp to `[0x157750]` then
  usually dominates. The engine seat passes `(int16_t)jitter` and its own clamp
  preserves this. Owner: `0x77423`; `fifa96_camera_event_set`.

## Review erratum (wave review, 2026-10-16)

Wave review adjudicated FU301-L6's reading correct. Quoted correction: "the seat's BX height is camY-derived (`SI = dword[0x157750]`, nothing writes ESI through `0x77420`), so L10's 'BX = jitter' is superseded; the port must use the camY-derived height, not jitter, and the §6 seat-call discussion aligns accordingly."

**Verdict:** GO with correction — FU301-L6 adjudicated correct; L10's "BX = jitter" superseded; the port uses the camY-derived height (`SI = dword[0x157750]`, nothing writes ESI through `0x77420`).
