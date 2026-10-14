# FU-240 — the phase-`0x13`/`0x14` scatter producer (`FUN_0008D098` case block) and the tape ruling

Provenance: M2 phase-23 census/ceiling recon-ahead, W23-1 (2026-10-14), **read-only**.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive
program argument; `get_current_program_info` this session: watcom `LE:32:default`,
image base `0`, `min_address 0x10000`, `max_address .image::00174a2a`, 2706 functions,
15239 symbols, `.image` overlay present). Every address/byte/instruction below is a
fresh `disassemble_bytes` / `read_memory` / `get_xrefs_to` / `decompile_function` of
**this session** unless marked *cross-cited* (the frozen doc is named). No writes:
no rename, comment, label, function, script or project-save; `/FIFA96.EXE` untouched;
no git; no code/test edits. Alias traps applied (`dword[x]>>24` = byte at `x+3`).

**Answer in one line:** the `FUN_0008D098` case-`0x13`/`0x14` block is only **half**
unported — its *install* half is already the wired FU-142a machine
(`fifa96_match_phase_machine_step`) that drives the counted action rows `0x26`/`0x28`/`0x2A`
— and the remaining *scatter* half feeds the `0x6E244` body whose **only** engine dispatch
(the now-live FU-239 extra-time chooser `match_run_write_phase(mr, 0x13)`) would inject
shared-`0x92AC8`-RNG draws into the frozen M2 golden; the rows `0x13`/`0x14` are therefore a
**CEILING (latent `+2`, blocked by the FU192-L2 tape-inertness pin)**, not a safe mover.

---

## 1. The phase table entries (fresh)

`read_memory 0x110794` (32 B, fresh) → `[00]=0x6DE34 [01]=0x6E1D0 [02]=0x6DCC8
[03]=0x6DE44 [04]=0x6DE44 [05]=0x6E05C [06]=0x6DD9C [07]=0x6DE44` — byte-identical to
FU-192 §1. `read_memory 0x1107D0` (32 B, fresh) → `[0F]=0x6DE34 [10]=0x6E004
[11]=0x6E1C8 [12]=0x6E1D0 **[13]=0x6E244 [14]=0x6E244** [15]=0x6DCC8 [16]=0`. The two
phase-`0x13`/`0x14` slots both point at the single body `0x6E244`; the table entries are the
body's **only** references (`get_xrefs_to 0x6E244` → 2 DATA: `0x1107E0`, `0x1107E4`).

## 2. The body `0x6E244` — `0x6E244..0x6E32A` (231 B), the camera-scatter writer

Fresh `disassemble_bytes 0x6E244` (whole body). ABI: entry `EAX` = a record whose `[EAX]`
is the team pointer, `EDX` = the target triple (`&rec[0x4D]`); it writes `target[0]`/`target[2]`
and **never** `target[1]`.

```
6E24C  EDX=[0x157AAC]; SAR EDX,0x18   ; EDX = byte 0x157AAF (the current side; alias)
6E252  EAX=[EAX]                       ; EAX = team
6E259  BL=byte[EAX+0x826]              ; record side
6E25F  CMP EBX,EDX ; JNZ 6E326         ; gate: side == gside else bare RET
6E267  BX=word[0x10F368]; SI=word[0x10F364]   ; the scatter cells (WORD reads)
6E26E  DL=byte[EDI+0x8D]               ; rec active flag
6E27B  TEST DL,DL ; JNZ 6E2AC
       ;; DL==0 (inactive): target[2] = ±BX by ([0x157AC2]<4 & side); target[0]=SI; RET
6E2AC  ;; DL!=0 (active):  CALL 0x92AC8 ; DIV 0x90 ; CALL 0x92AC8 ; TEST AL,1 ; EAX=±(r1)
6E2D0  EBX += EAX
6E2D4  AL=[0x157AC2]; if <4: target[2]=±EBX by side ; else target[2]=EBX
6E2FB  MOV EBX,0x90; XOR EDX,EDX; CALL 0x92AC8; DIV EBX; CALL 0x92AC8; TEST AL,1; EAX=±(r2)
6E31F  ESI += EAX ; 6E321 MOVSX EAX,SI ; MOV [ECX],EAX   ; target[0] = SI ± r2
```

Reads: `[0x157AAC]` (→ byte `0x157AAF`), `[rec+0x826]` (side), `[rec+0x8D]` (active),
`[0x157AC2]` (period index), `word[0x10F368]`, `word[0x10F364]`, RNG `0x92AC8` (four CALL
sites: `0x6E2B3`, `0x6E2BA`, `0x6E302`, `0x6E309` — two magnitudes + two sign draws).
Writes: `[ECX]` (=`target[0]`, rec`+0x4D`) and `[ECX+8]` (=`target[2]`, rec`+0x55`) only.
FU-192 §2.10's read list is confirmed; **errata in the brief wording:** the body calls
`0x92AC8` **four** times per *active* record (two per coordinate), not "twice".

## 3. The producer — `FUN_0008D098` (`0x8D098..0x8D820`), case `0x13`/`0x14`

Fresh `decompile_function 0x8D098` + `disassemble_bytes 0x8D690` (the arm window
`0x8D693..0x8D757`). Its dispatch is `switch(DAT_00157a4d)` = the byte at `0x157A4D`
(= `[0x157A4A]>>24`; §4). The shared `case 0x13: case 0x14:` arm:

```
8D699  AL=[0x157AC2]; CMP EAX,4; JGE 8D703          ; period index < 4 ?
  <4:  8D6A9 ECX=0x1E0; CALL 0x92AC8; DIV ECX       ; r1 = rand % 0x1E0
       8D6B7 CALL 0x92AC8; TEST AL,1                ; sign
       8D6D4 [0x10F364] = ±r1   ; 8D6DB [0x10F368] = 0x5A0
       8D6E1 CALL 0x92AC8; DIV ECX; CALL 0x92AC8; TEST AL,1
       8D6FB ADD [0x10F368], ±r2                    ; 0x5A0 ± r2
  >=4: 8D703 CMP byte[EBP+0x826],0                   ; side
       8D718 [0x10F364] = side?0x2D0:-0x2D0 ; [0x10F368] = 0x5A0
8D728  EDX=[0x157AAC]>>24 ; AL=[EBP+0x826] ; CMP ; JZ 8D75F
       if side != gside: FUN_0008CEB8(param,0,10,0x26,-1)        ; uncontrolled install
       else if phase==0x13: code = ([0x157AC3+2]!=byte[0x157AC7]) ? 0x25 : 3
       else: FUN_0008CEB8(param,0,10,0x28,-1) + install 0x2A + [0x10F35C]=0
```

So the block has exactly **two halves**:
* **scatter** (`0x10F364`/`0x10F368` writes + 4 `0x92AC8` draws) — **unported**;
* **installs** (`0x25`/`0x26`/`0x28`/`0x2A` + the `[0x10F35C]=0` clear) — **already
  ported and wired** as `fifa96_match_phase_machine_step`
  (`src/fifa96_engine/fifa96_match_phase_machine.c:52`, per-frame at
  `fifa96_match_run.c:4198`), driving the counted action rows `0x26`/`0x28`/`0x2A`.

Fresh `get_xrefs_to 0x0010F364` (5): WRITE `0x7D995` (`FUN_0007d8d0`, the reset —
`read_memory`/disasm `0x7D98x` zeroes `0x10F35C/360/364/368/36C`), WRITE `0x8D6D4` +
READ `0x6E274` (this producer/body), READ `0x87159`. Fresh `get_xrefs_to 0x0010F368` (6):
WRITE `0x7D98F` (reset), WRITE `0x8D6DB` + R/W `0x8D6FB` + WRITE `0x8D722` (producer),
READ `0x87161`, READ `0x6E267` (body). The cells are DWORD-written but the body reads only
their low WORDs.

## 4. The writer path — who writes phase `0x13`/`0x14` into `[0x157A4A]`

`[0x157A4A]` is a dword whose **top byte `0x157A4D`** is the phase. Fresh
`get_xrefs_to 0x00157A4D` → WRITE sites only: `0x73E4E` (`FUN_00073e28`, `XOR AH,AH` ⇒ 0),
`0x73D2C` (`FUN_00073d10`, `STOSB` zero-fill), and **`0x740AC`** (`FUN_000740A0`, `MOV
[0x157A4D],AL`). So the only nonzero phase writer is `FUN_000740A0`.

Fresh `disassemble_function 0x740A0`:
```
740A0  MOV AH,[0x157A4D] ; MOV [0x157A4E],AH   ; old phase -> 0x157A4E
740AC  MOV [0x157A4D],AL                        ; NEW PHASE = AL
740BA  MOV byte[0x157AAF],DL                    ; side -> the body's compare byte
740C8  CALL 0x8D098 (team[DL]) ; 740DB CALL 0x8D098 (team[DL^1])
```
Fresh `get_xrefs_to 0x740A0` = **27** direct callers. The static phase each passes (fresh
disasm of every site) is `{0,1,2,4,5,8,9,0xA,0xB,0xC,0xD,0xF,0x10,0x11,0x15}` plus two
dynamic sites. The **only** site passing `0x13` is **`0x8BCC8`** in `FUN_0008baf0`:
`CMP byte[0x157AC0],0 ; JZ … ; MOV EAX,0x13 ; XOR EDX,EDX ; CALL 0x740A0` — i.e. gated on
the extra-time flag `[0x157AC0] != 0`. No static `MOV EAX,0x14 ; CALL 0x740A0` exists
(byte-pattern `b814000000`: all `0x14` sites call `0x4C374`/`0x8F0C4`/`0x4C374`, **not**
`0x740A0`); the only computed-AL caller is `0x895ac` (`EAX = ([ESP-2] >> 16)`, a
save/restore-phase helper that could carry `0x14`). The native `0x14` phase is therefore
produced by the period-end chooser (computed AL), which the engine ports as
`fifa96_action_phase_period_end` (`fifa96_action_handlers.c:1163`: default `phase=0x13`,
probe arms → `phase=0x14`).

**Engine reachability (fresh source read):** `fifa96_match_run_phase_drive`
(`fifa96_match_run.c:2602`) handles the live period completion and, in its ended-branch,
runs `match_run_write_phase(mr, out.phase)` with `out.phase == 0x13`/`0x14` from that
chooser (`:2658`, `:2704`) — gated on the live extra-time flag `mr->global_157ac0 == 1`
(FU-213/FU-239). `match_run_write_phase` (`:2514`) runs the per-record phase walk
`fifa96_match_run_record_phase_walk` **iff `fifa96_match_phase_table[phase].fn != NULL`**
(`:2528..2532`). The M2 v21 golden's tail is exactly this live route (`tests/golden/
engine/m2-frames.txt` frames `~6500..6522` read `state=19/1-0`). So the engine's
`match_run_write_phase` path **does reach** phase `0x13` today — the walk would dispatch
the `0x6E244` row the moment it is wired. (The rows are currently NULL:
`fifa96_match_handlers.c:5227/5229`.)

## 5. Tape ruling + verdict

**Tape facts (fresh):** `tests/golden/engine/m2-frames.txt` is 6522 lines; the `state=`
values present are `{0,1,2,4,12,19,20,21}`. Phase `0x13` (`state=19`) is on the tape in
**two distinct windows**: (a) the **forced** kickoff segment frames `6..25`
(`tests/test_engine_m2.c:2387` `fifa96_match_state_set_phase(&mr->state,0x13)` — a test
injection that **bypasses** `match_run_write_phase`) and (b) the **live** extra-time tail
(the FU-239 ET chooser, frames `~6500+`). Phase `0x14` (`state=20`) is the forced second
window frames `26..45` (`:2399`).

**Ruling — CEILING (latent `+2`, blocked).** The rows stay NULL/uncounted:
1. **The producer is already half-ported.** The `FUN_0008D098` case-`0x13`/`0x14`
   dispatch-visible half (the `0x25/0x26/0x28/0x2A` installs) is the wired FU-142a machine
   `fifa96_match_phase_machine_step` and already drives the counted action rows
   `0x26/0x28/0x2A` on the tape. The marginal new work is only the **RNG scatter** + its
   **`0x6E244` consumer** — precisely the part FU-192 §5 leg #10 (FU192-L2) parks.
2. **Wiring is not proven tape-inert.** The `0x6E244` body's only engine dispatch is the
   per-record walk reachable through `match_run_write_phase(0x13/0x14)`. The v21 golden's
   live ET tail *is* such a write, so wiring the row would run the walk on the ET-entry
   frame, consuming the **shared** `0x92AC8` RNG (`mr->rng`, four draws per active record)
   ahead of the ~20 ET-over frames whose render path also reads `mr->rng` (the per-frame
   camera walk `fifa96_match_run.c:4147`, the camera event `:615`, the phase/ball arms).
   Whether that shifts the frozen golden is unproven — the exact FU192-L2 risk, now
   *sharpened* (a live M2 writer, not a hypothetical one), not closed.
3. **Placing the scatter in the existing arm breaks the tape by construction.** The only
   existing case-`0x13`/`0x14` port (`fifa96_match_phase_machine_step`) runs **per granted
   frame** while `state.phase` is `0x13`/`0x14` — i.e. through the whole forced `6..45`
   window *and* the ET tail. Folding the RNG scatter there would draw 4×/record **per
   frame**, a native-cadence divergence (native = once per phase *write*) that would move
   the forced window itself.

**`+2` port shape (if later unblocked):** port `0x6E244` as `fifa96_match_phase_13/14`
over the walk cursor (writes `target_x`/`target_z`; `BYTE[0x157AAF]` side gate;
`[0x157AC2]<4` branch; `WORD[0x10F364/368]` reads; `fifa96_rng_step` × 2 per coordinate),
add the scatter to a **once-per-write** arm (not the per-frame step) at
`match_run_write_phase`'s `fn != NULL` gate, and wire `fifa96_match_phase_table[0x13]` and
`[0x14]` → `+2` (census `55/80 → 57/80`). **Gate:** the M2 golden must stay byte-exact
(§5.2); if the ET-entry RNG draws are downstream-inert on the ET tail the rows wire cleanly;
if not, wiring is a re-pin, and the rows stay a ceiling. This recon does **not** wire them.

---

## Residual legs (numbered; each with its native owner)

1. **FU240-L1 — the native phase-`0x14` writer site is not statically located.** No
   `MOV EAX,0x14 ; CALL 0x740A0` exists; the only computed-AL `FUN_000740A0` caller is
   `0x895ac` (`([ESP-2]>>16)`). The engine emits `0x14` from `fifa96_action_phase_period_end`
   (period-end chooser); the exact native `0x14` write + its save cell are unlocated.
   Owner: FU-239 (extra-time route) / a follow-up.
2. **FU240-L2 — the ET-entry RNG-inertness pin (the `+2` gate).** Prove
   `match_run_write_phase(mr,0x13)` at the M2 v21 ET-entry leaves the golden byte-exact when
   the `0x6E244` row is wired (or accept a re-pin). Owner: FU-192 (FU192-L2) / T3.
3. **FU240-L3 — `[0x157A4E]` (the previous-phase cell) is written by `FUN_000740A0` but has
   no engine field.** Consumers of `0x157A4E` and whether the ET route needs it: unmodelled.
4. **FU240-L4 — the `0x87159`/`0x87161` readers of `0x10F364/0x10F368`** (a second consumer
   besides `0x6E244`) are not derived; their containing function and whether they are on a
   live path are unlocated. Owner: this FU / T3.
5. **FU240-L5 — `FUN_0007d8d0` zeroes `0x10F35C/360/364/368/36C`.** The reset's cadence and
   the `0x10F35C` chase flag's consumers (`mr->global_10f35c` is already modelled) are not
   fully bound to this producer pair.
