# FU293 — row-`0x1B` stage-2 wide descriptor/pan-vector interpolation + `FUN_00076B28` steering

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 4 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0,
`.image` overlay, 2706 functions; confirmed this session by
`get_current_program_info`). Every native address/byte below is a **fresh**
`disassemble_bytes` / `disassemble_function` / `get_xrefs_to` of *this* session
unless marked *carried* (the frozen `docs/ghidra/FU285_r11l4_row1B_wide.md`
slice is read-only background). Alias traps applied. No rename / comment / label /
function-create / script / project-save; `/FIFA96.EXE` untouched; no git.

---

## 1. Port shape

`0x7707D..0x7741F` is the **stage-2 wide descriptor/pan-vector interpolation**
(the `FU285-L1` block). It is *not* a Ghidra function — it is reached only by
fall-through inside the row-`0x1B` body (row `0x1B` = `0x76D28..0x77727`, no
function; FU285-L8). Block by block (fresh `disassemble_bytes 0x76FD5` /
`0x7707D` / `0x77420`):

* **Descriptor half-extent seed (`0x77035..0x7707A`, enters at `0x7707D`).**
  `[0x14C2F6]` selects the base extent: if `==1`, `ECX = 8 - ([[rec+4]+9]>>0x19)`,
  `ESI = 0xA0 - 2*ECX`; else `ECX = ([[rec+4]+9]>>0x19) + 0x70`, `ESI = 0xA0`.
  `EBP=rec`. So the two live inputs are the difficulty word `[0x14C2F6]` and the
  record-side descriptor ratio `[[rec+4]+0x9]` (a 20.12-ish byte pair).
* **Block A — keeper-distance descriptor scale (`0x7707D..0x770D9`).** If the
  tracked-keeper cell `[0x1577CA] != 0`: `CALL 0x8DCD4` (line/distance over
  `rec+0x59` vs keeper`+0x59`) → `d>>16`. `d < 0x1E0`: `ESI = ESI*3/4` and (only
  when `[0x14C2F6]==0`) `ECX -= 0x10 - ([[rec+4]+9]>>0x18)`; `d >= 0x1E0`:
  `ECX += ([[rec+4]+9]>>0x19)`. Output: the modulated `ESI` (X extent) / `ECX`.
* **Block B — bounds + heading gate (`0x770D9..0x77162`).** If `[0x157750]==0`
  then `ESI=0xC0`. Gates (all `-> 0x77636` the stage-4 head): `[rec+0x69]>>16 >
  ESI`; `ECX + [rec+0x5D] < [0x157750]`; and — after
  `angle = 0xCD474([rec+0x6B]>>16,[rec+0x6D]>>16) - [rec+0x7D]`, masked to a
  10-bit heading — `|angle| >= 0x80 && [rec+0x69]>>16 > 0x20`.
* **Block C — random pan jitter + keeper-relation flag (`0x77163..0x771E7`).**
  `ESI=0`, `ECX=0`. Threshold `= 0x780` if `[0x14C2F6]==1` else `0x5A0`; if
  `[0x1577CA]!=0` and `|[0x1577CA]+0x61| > threshold`, `ESI =
  (2 - [0x14C2F6]) * (rand(0x92AC8) & 0xF)`, clamped `<= 0x10`. If `[0x1577CA]!=0`,
  `CALL 0x74584(keeper.pos, rec)` → if `AL`, `ECX=1` (the keeper-relation flag).
* **Block D — latch + route (`0x771E8..0x7721A`).** If `[0x157C57]!=0` →
  `0x774D7` (falls to the stage-4 head). Else if `ESI < ([[rec+4]+9]>>0x19)>>ECX`
  or `[0x157750] > 0x20`: if `[0x157C58]==0` → `JMP 0x774DD` (the re-place block);
  else continue to `0x7721B`.
* **Block E — pan-cell (pan-vector) integration (`0x7721B..0x77312`).**
  `[0x157C59]=1`. `AX=[0x1577BE]` (pan-x): if zero, seed `[0x1577C0]=rand&7 + 8`,
  `[0x1577C2]=rand&7 + 8`, and `[0x1577BE] = 0xCD514([0x1577BE]>>16,[0x1577C0]>>16)`.
  Negate `[0x1577C2]`; add the signed byte `[[rec+4]+0x1B]` to `[0x1577C0]`
  (sign of `BX`); a `rand(0x92AC8)`-seeded branch (`([[rec+4]+0xC]>>0x18)`)
  picks code `2`/`3` for the `0x8DC50` reduce, writing `[0x1577C0]` and
  `[0x1577C2]`. These three words are the **pan cells**.
* **Block F — pan-vector → camera delta + `EDI` (`0x77313..0x7738A`).**
  `EDI := 1` iff `sign([0x1577C2]) == sign([0x157754])` (`0x7731B..0x7733A`;
  `EDI` is **not** reset here, so it is a carry the whole block computes).
  Load `[0x1577F2]>>16` / `[0x1577F8]>>16`, delta, then
  `word[ESP+0xA] = [0x1577C0]*delta`, `word[ESP+0xC] = [0x1577C2]*delta`, and
  `word[ESP+0x8] = 0x8DC68([ESP+8]>>16,[ESP+0xA]>>16)`. `[0x157C57]=1` (post).
  **`[ESP+0x8..0xC]` is the computed 6-byte pan vector.**
* **Block G — side dispatch + hand-off (`0x7738A..0x7741F`).** `[0x157823]`
  branch → per-side counter increments (`0x157ACC`/`0x157AD0` via `0x741B4`);
  `CALL 0x92820(rec,0x1A)`; `side = [[rec]+0x826]` → `0x741B4` → code `0xE`/`0x5`
  → `CALL 0x651F0`. Then `EDX = ESP+8` (the pan vector), `EAX = EBP` (rec),
  `ECX=0`, `[0x157A6C]=0` (clear pan latch), `EBX = MOVSX SI` (the jitter), and
  `0x77420/0x77423 CALL 0x71C94` — the pan/camera-event setter, which copies the
  vector: `0x71CBC MOV EDI,0x1577B8; CALL 0x700F4; MOVSD/MOVSW` → so the
  interpolated vector **is** the 6-byte pan record `[0x1577B8..0x1577BD]`.

**Outputs of the interpolation:** the 6-byte pan vector at `ESP+8`
(`[0x1577B8..BD]` after `0x71C94`), the three pan cells
`[0x1577BE]`/`[0x1577C0]`/`[0x1577C2]`, the latches `[0x157C57]=1` /
`[0x157C59]=1`, the `[0x157A6C]` clear, the **`EDI` flag**, and two side
effects that consume `EDI` after the setter: `0x77435` (`[0x157A4A]>>0x18==0x10`
`&& EDI==0` → `0x8F188(6,rec,8)` + `0x8ED40(rec,0x64)`) and `0x77458` (`EDI==0`
→ the `0x8F188(3,rec,0x20)` / `0x8ED40(rec,5)` / `0x8ED40(rec,0x19)` arm; `EDI!=0`
→ straight to the `0x774C0` terminal `0x974DC(4)` + `[0x157C55]=1`). `0x71C94`
`PUSH ESI/EDI/EBP` at entry (`0x71C94/95/96`), so `EDI` survives the call —
**the T5-noted `EDI` dependence is real and spans the `0x71C94` call.**

**`FUN_00076B28` (steering, `0x76B28..0x76D11`, 155 insns; fresh
`disassemble_function 0x76B28`).** `EAX=rec`, `EBX`/`EDX` = half-extents
(`0x60`/`0xA0` at the row-`0x1B` caller `0x76FA2`). Writes:
`[rec+0x83] = [rec+0x51] - EBX + 0x20` (clamp `[0,0x60]`, `0x76B3C`); `[rec+0x85]`
`= 0` if the clamp was `<=0` (`0x76B51`) else `0x702F8([rec+0x81]>>16)`
(`0x76B81`); `[rec+0x87]=0` (`0x76B9E`); a `word[ESP] = [rec+0x65] - EDX + 0x20`
clamped `[0x10,0x70]`; the position fold via the `0x114E04` ramp table +
`0x795A4` into `[rec+0x4D]`/`[rec+0x55]` (MOVSD from `rec+0x59`); the `0x795B4`
band helper over `(rec+0x65,rec+0x4D,rec+0x59)`; the directional block
(`IDIV` by `[rec+0x83]>>16`, clamp `<=0xF`, `0x114E04`+`0x795A4`) writing
`[rec+0x73]`/`[rec+0x75]` (`0x76CFD`/`0x76CF6`); and `CALL 0x8DC68 →
[rec+0x71]` (`0x76D06`). Fresh `get_xrefs_to 0x76B28` = **2**
(`0x76FA2` row `0x1B` stage 0; `0x779B4` row `0x1C`) — matches the
FU285 §3 expectation.

**Reachability of the L1 state from the ported engine.** It **stays a leg.**
The engine has `mr->match1b_wide_leg` (`fifa96_match_handlers.c:3870`) as an
explicit stub and models only the surface cells: `keeper_latch[3/4/5]`
(`[0x157C57/58/59]`), `keeper_cam_y/z` (`[0x157750/4]`), `render.camera.tracked`
(the `[0x1577CA]` stand-in) and `keeper_cpu head_flag` (`[0x157823]`). The
interpolation's actual inputs are **absent**: no storage for the pan cells
`[0x1577BE/C0/C2]`, the pair `[0x1577F2]/[0x1577F8]`, the pan record
`[0x1577B8..BD]`, the difficulty word `[0x14C2F6]`, or the descriptor ratio
`[[rec+4]+0x9]`/`[[rec+4]+0xC]` (grep over `src include` returns no producer for
any of them; `[0x1577CA]` appears only as the `id == controlled` stand-in).
So the ported engine reaches the row-`0x1B` route only through the collapsed
`match1b_wide_leg` stub; a faithful body needs the whole descriptor/pan-cell
state machine above.

---

## 2. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| descriptor seed `0x77035..0x7707A`: `[0x14C2F6]` → `ESI`/`ECX` from `[[rec+4]+9]>>0x19` | `disassemble_bytes 0x76FD5` |
| Block A keeper-distance scale: `0x8DCD4`, `d<0x1E0` → `ESI*3/4`; `d>=0x1E0` → `ECX+=` | `disassemble_bytes 0x7707D` |
| Block B bounds/heading: `[0x157750]==0→ESI=0xC0`; `0xCD474` angle vs `[rec+0x7D]`, `0x80` gate | `disassemble_bytes 0x7707D` |
| Block C jitter `0x92AC8` + threshold `0x780`/`0x5A0` + `0x74584` keeper-relation flag | `disassemble_bytes 0x7707D` |
| Block D `[0x157C57]` → `0x774D7`; `[0x157C58]==0` → `0x774DD` | `disassemble_bytes 0x7707D` |
| Block E pan cells `[0x1577BE/C0/C2]` seeded/negated/`0xCD514`/`0x8DC50` | `disassemble_bytes 0x7707D` |
| Block F `EDI := sign([0x1577C2])==sign([0x157754])` (`0x7733A`); vector `[ESP+8..C]` via `0x8DC68`; `[0x157C57]=1` | `disassemble_bytes 0x7707D` |
| Block G `0x92820(0x1A)`/`0x741B4`/`0x651F0`; `[0x157A6C]=0`; `EDX=ESP+8`,`EBX=SI` → `0x71C94` | `disassemble_bytes 0x7707D`; `disassemble_bytes 0x77420` |
| `0x71C94` copies `EDX` vector → `[0x1577B8]` (`MOVSD/MOVSW`) | `disassemble_bytes 0x71C94` |
| `0x71C94` `PUSH ESI/EDI/EBP` → `EDI` survives → gates `0x77435`/`0x77458` | `disassemble_bytes 0x71C94`; `disassemble_bytes 0x77420` |
| `EDI==0` arm selects `0x8F188`(2/3/6)/`0x8ED40`(5/0x19/0x64); else `0x774C0` `0x974DC(4)`+`[0x157C55]=1` | `disassemble_bytes 0x77420` |
| `FUN_00076B28` writes `[rec+0x83/85/87/73/75/71]`, folds pos via `0x114E04`/`0x795A4`/`0x795B4` | `disassemble_function 0x76B28` |
| `0x76B28` refs = 2 (`0x76FA2` row `0x1B`, `0x779B4` row `0x1C`) | `get_xrefs_to 0x76B28` |
| engine stub `mr->match1b_wide_leg`; no pan-cell/descriptor producers | `fifa96_match_handlers.c:3853-3872`; grep `src include` |

**Carried docs (read, not rewritten):** FU285 §1/§3 (FU285-L1/L4); FU273 §2.1/§2.2/§2.3; FU180 legs L1-L5.

---

## 3. Numbered open legs (with native owners)

* **FU293-L1 — the pan cells `[0x1577BE]`/`[0x1577C0]`/`[0x1577C2]` are unported.**
  Fresh `0x7721B..0x77312` seeds/negates/reduces them (`0xCD514`, `0x8DC50`); no
  engine storage exists. This is the load-bearing L1 state. Owner: `0x7721B`
  (`FU285-L1`) + `fifa96_match_action_1B` `match1b_wide_leg`.
* **FU293-L2 — the descriptor inputs `[0x14C2F6]` (difficulty) and
  `[[rec+4]+0x9]`/`[[rec+4]+0xC]` (ratio) are unported.** Fresh `0x77035..0x77101`
  and `0x772BF`; no producer in the engine. Owner: `0x77035`; `handlers.c:3853`.
* **FU293-L3 — the pan-vector pair `[0x1577F2]`/`[0x1577F8]` is unported.**
  Fresh `0x7733F..0x7736D` multiplies the pan cells by `([0x1577F2]>>16 -
  [0x1577F8]>>16)`. Owner: `0x7733F`; `handlers.c:3857`.
* **FU293-L4 — the fold primitives `0x8DCD4`/`0xCD474`/`0xCD514`/`0x8DC68`/
  `0x74584`/`0x702F8`.** Only `0x8DC50` is ported (`fifa96_camera.c:923`); the
  rest are named sinks. Owner: FU180-L1/L3; `fifa96_camera.c:923`.
* **FU293-L5 — the `EDI` sign-match + the `0x77435`/`0x77458` install selection.**
  Fresh `0x77313..0x7733A` computes `EDI` (pan-z/cam-z sign match, survives the
  `0x71C94` call); it gates `0x8F188`/`0x8ED40` codes 2/3/5/6/0x19/0x64. Owner:
  `0x77313` + `0x77435`; `handlers.c:3870` (the `match1b_wide_leg` stub).
* **FU293-L6 — the `0x92820(rec,0x1A)` + `0x741B4`/`0x651F0` sit dispatch.**
  Fresh `0x773E3..0x7740B`; the terminal `0x774C0` `0x974DC(4)`. Owner:
  `0x773DC`/`0x774C0`; OL-27 / OL-82.
* **FU293-L7 — `[0x1577CA]` (tracked keeper) is a stand-in only.** The engine
  models it as `id == controlled` (`handlers.c:2963`, `render.camera.tracked`);
  blocks A/C read it natively (`0x77083`, `0x771C1`). Owner: `0x77083`;
  `handlers.c:2963` / FU285-L1.
* **FU293-L8 — `FUN_00076B28` steering is unported (FU285-L4).** Fresh
  `0x76B28` writes `[rec+0x83/85/87/73/75/71]`; refs = 2 (`0x76FA2`, `0x779B4`).
  Owner: `0x76B28`; FU180-L3.
* **FU293-L9 — row `0x1B` is not a Ghidra function.** Fresh
  `disassemble_bytes`/`get_function_by_address` (via FU285-L8): the body is
  dispatched only through `0x1106E0[0x1B]=0x76D28`; the `0x7707D..0x7741F`
  interpolation is interior fall-through with no function boundary, so
  decompiler output is unavailable (block-level disassembly only). Owner:
  FU285-L8.
