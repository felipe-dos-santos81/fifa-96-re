**Phase/Provenance:** M2 phase-26c recon-ahead, Task 5 (bounded, read-only derivation); draft awaiting controller promotion.

# FU285 — row-`0x1B` stage-2 wide / stage-3 + the `0x71C94` pan setter (port shape)

**Question (brief).** Fresh-verify the carried FU-273 §2.1/§2.2/§2.3 read of the
row-`0x1B` head gate and the stage-4 tail; describe the unported stage-2 wide +
stage-3 body (`0x76FD5..0x77636`, FU180-L1), the `0x71C94` pan/camera-event setter
at `0x77423` (FU180-L2), the `FUN_00076B28` keeper steering (FU180-L3); pin what
the `0x71C94` setter writes; and decide **GO / NO-GO** on whether the stage-4
sit-5 emit (`0x77707`) can be reached on a **live** native path.

Authoritative program **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0,
`.image` overlay, 2706 functions; confirmed this session by
`get_current_program_info`). Every native address/byte below is a **fresh**
`disassemble_bytes` / `disassemble_function` / `read_memory` / `get_xrefs_to` /
`get_function_by_address` of *this* session unless marked *carried* (the frozen
`docs/ghidra/FU*` slice is named). Alias traps applied (`dword[x]>>0x18` = the
byte at `x+3`). No rename / comment / label / function-create / script /
project-save; `/FIFA96.EXE` untouched; no git.

---

## 1. Port shape

The row-`0x1B` body is `0x76D28..0x77727` (fresh: Ghidra has **no function** at
`0x76D28*` — dispatched only via the action table `0x1106E0[0x1B]=0x76D28`;
the next row entry at `0x77728` is row `0x1C`). Five pieces:

* **Head (`0x76D28..0x76DC5`)** — fresh `disassemble_bytes 0x76D28`:
  `0x76D33` `[0x157A4A]>>0x18 == 0xD`, `0x76D46..0x76D5D`
  `([0x157AAC]>>0x18) == byte[rec[+0x826]]` (side). On pass the per-sector camera
  place: sector `= [rec+0x8B]>>0x18`; `[0x15774C] = pos.x + ([sector+0x10F331]>>0x18)<<4`;
  `[0x157754] = pos.z + ([sector+0x10F339]>>0x18)<<4`; `0x76DA3 EAX = [rec+0x5D]+0x38`;
  `0x76DAE byte[rec+0x9B]=1`; `0x76DB5 [0x157750]=EAX` (camY); `0x76DBA CALL 0x700F4`
  (camera-place primitive, args camX/camY/camZ/1); `0x76DBF [0x157A83]=rec`.
  On **fail** (`phase!=0xD` **or** side!=controlled) it jumps to `0x76DC5` and still
  runs the stage switch — the head camera/actor write is the **only** gated part.
* **Stage switch (`0x76DDE..0x76DEE`)** — `AL = byte[rec+0x92]` (stage); `CMP AL,4; JA exit`;
  `JMP dword CS:[EAX*4 + 0x76D14]`. Fresh `read_memory 0x76D14` =
  `{0x76DF6, 0x76E3B, 0x76FC7, 0x7769F, 0x776C1}` = **stages 0/1/2/3/4**.
* **Stage 0/1 (`0x76DF6` / `0x76E3B`)** — stage 0 clears the `0x157C54/57/58/59/5A/5B`
  latches, `byte[rec+0x9E]=1`, `[rec+0x89]=0`, runs the `0x8DCD4`/`0x79C50`/`0xCD474`
  and `0x114E04`/`0x795A4` metric/heading block (`[rec+0x51]+=0x20`, `[rec+0x65]+=0x40`,
  folds pos into `[rec+0x4D]/[rec+0x55]`, `[0x157C54]`), `CALL 0x6E598` (`0x76F91`),
  `CALL 0x76B28` (`0x76FA2`, EBX=0x60 EDX=0xA0), `[rec+0x92]++`.
* **Stage 2 wide (`0x76FC7`/`0x76FD5..0x77636`)** — the rush/gate body. Fresh
  `disassemble_bytes 0x76FD5`: `EBP==[0x157A83]`/`EBP==[0x1577CA]` → `0x77636`;
  the `[0x157C5C]` latch → clear + `JMP 0x774DD`; the `[rec+0x6B]≷[rec+0x77]` +
  `[ESP+0xe]∈[0x34,0x3A]` band gate; `[0x14C2F6]` ∈ {0x780/0x5A0}; the pitch bounds
  (ESI=0xA0/0xC0) vs `[rec+0x69]` and camY `[0x157750]`; the pan cells
  `[0x1577BE]/[0x1577C0]/[0x1577C2]` driven by `0x92AC8` (rand) + `0xCD514`/`0x8DC50`/
  `0x8DC68` interpolation and `[0x1577F2]/[0x1577F8]`; `[0x157C57/58/59]`; the
  `0x77423 CALL 0x71C94` pan setter; the `0x8F188`/`0x8ED40` sub-installs (codes
  2/3/5/6/0x19); `0x774C0` (`0x974DC(4)`, `[0x157C55]=1`) → `0x77636`; and the
  `0x774DD` re-place block (below).
* **Stage 3 (`0x7769F`) / Stage 4 (`0x776C1`)** — fresh: `0x7769F CMP byte[rec+0x44],0;
  JZ exit; [rec+0x89]=0; [rec+0x92]++`. Stage 4 `0x776C1 MOV byte[rec+0x92],4;
  CALL 0x7DAB4; MOVSD pos→[rec+0x4D]; 0x776D8 EDI=[0x157A83]; 0x776DE CMP EBP,EDI;
  0x776E2 [0x157A49]>>0x18==1 → exit; 0x776EF..0x77707 EAX=5/EBX=1/EDX=side CALL 0x8A938`
  (the sit-5 emit); else `0x7770E install row 0x19`. Epilogue `0x7771E`.

The **one thing the engine lacks** is the whole stage-2 wide block: the `0x774DD`
re-place + `0x71C94` pan setter + `0x76B28` steering + the `0x6E598` claim bridge +
the sit-7 emit. `fifa96_match_action_1B` (`handlers.c:3692`) collapses
`0x76FD5..0x77636` into two gates (`handlers.c:3760-3765`), so the port must add
the wide body (FU180-L1/L2/L3) and re-shape `actor_is_rec`.

---

## 2. The `0x71C94` pan/camera-event setter (FU180-L2) — what it writes

Fresh `disassemble_function 0x71C94` (`FUN_00071c94`, body `0x71C94..0x71DF2`,
`get_function_by_address` = real function; `get_xrefs_to 0x71C94` = **11**, incl.
`0x77423` row `0x1B`, `0x77DC6` row `0x1C`, `0x75DC4` row `0x1E`):

```
0x71C99  CMP byte[0x157A6C],0 ; JNZ 0x71DEF          ; already latched -> return
0x71CA6  PUSH 0 ; ... [0x15774C],[0x157750],[0x157754] ; CALL 0x700F4   ; place camera triple (flag 0)
0x71CC8  MOVSD/MOVSW [0x1577B8..0x1577BD] := *EDX    ; a 6-byte pan record (EDX arg)
0x71CCB  [0x1577F0] := max( BX , [0x157750] )        ; clamp <= 0x640
0x71D09  [side*4 + 0x1577CE] := EBP                  ; per-side tracked actor
0x71D27  [0x1577CA] := EBP                           ; tracked keeper cell
0x71D2D  byte[EBP+0x9B] := 0 ; [0x158777] := 0       ; clear take flag + slot
0x71D3A/0x71D49/0x71D5A  side-record [+0x7BF]/[+0x7CB] := 0
0x71D65  CALL 0x70544 -> byte[0x157815] ; (record+0x20/+0x21 two-dir table [0x10E169])
0x71DCB  CALL 0x8DD70 -> (heading+0x40)&0x3FF >>7 -> byte[0x157815]
0x71DE3  byte[0x157822] := 0 ; byte[0x157820] := 0
```

So the setter writes: the camera triple (via `0x700F4`, flag 0), the 6-byte pan
record `[0x1577B8..0x1577BD]`, the pan-Y `[0x1577F0]` (max/`0x640` clamp), the
tracked-keeper cells `[0x1577CA]` **and the per-side array `[0x1577CE+side*4]`**,
the **pan-direction byte `[0x157815]`** (from the `0x10E169` two-dir table or the
`0x8DD70` heading), and clears `[0x157822]`/`[0x157820]`, `byte[rec+0x9B]`,
`[0x158777]` and the side-record fields. None of these are in the engine.

---

## 3. `FUN_00076B28` keeper steering (FU180-L3)

Fresh `disassemble_function 0x76B28` (body `0x76B28..0x76D11`, 155 insns).
`EAX=rec`, `EBX`/`EDX` = the half-extents (`0x60`/`0xA0` at the row-`0x1B` call
`0x76FA2`). It writes: `[rec+0x83] = [rec+0x51] - EBX + 0x20` (clamped ≤0x60) and
`[rec+0x85]` (`0x702F8`); `word[ESP] = [rec+0x65] - EDX + 0x20` (clamped `[0x10,0x70]`);
folds the position via the `0x114E04` linear ramp table + `0x795A4` into `[rec+0x4D]/[rec+0x55]`;
the `0x795B4` band helper over `([rec+0x65],[rec+0x4D],[rec+0x59])`; the directional
`[rec+0x85]` block → `[rec+0x73]/[rec+0x75]`; `CALL 0x8DC68 → [rec+0x71]`.
Fresh `get_xrefs_to 0x76B28` = **2**: `0x76FA2` (row `0x1B` stage 0) and `0x779B4`
(row `0x1C`). Shares the T2b-L3 helpers `0x114E04`/`0x795A4`/`0x795B4`.

---

## 4. The actor route and the stage-4 emit reachability (leg T2b-L1)

Fresh bytes show **two** writers of `[0x157A83] = rec` inside row `0x1B` (both
`892d837a1500`): the **gated head** at `0x76DBF`, and a **second, ungated** write
at `0x77568` inside the `0x774DD` stage-2-wide re-place block. The `0x774DD` block
is reached from the wide logic (`0x77009 JMP 0x774DD`; `0x77215 JZ 0x774DD`),
re-places the camera, `0x77568 [0x157A83]=EBP`, `0x7756E CALL 0x6E598`, then emits
**sit-7** (`0x77589 EAX=7, EDX=side, EBX=0 CALL 0x8A938`), and **falls through to
`0x77636`** — the stage-4 head.

So within **one dispatch** a row-`0x1B` record can set itself the actor at
`0x77568` and reach the stage-4 head; the emit `0x77707` then re-checks
`EBP==[0x157A83]` (`0x776DE`) — which is satisfied. The remaining emit gates are
`byte[rec+0x44]!=0` (`0x77636`) and `[0x157A49]>>0x18!=1` (`0x776E2`, the same
phase byte read at `0x77648`). No external actor transition is required.

The engine approximates the actor test with `actor_is_rec = (r->controlled != 0 ||
mr->entities.controlled == id)` (`handlers.c:3733`), which models only the **head**
`controlled` write, **not** the `0x77568` self-write. The engine's stage machine
(`handlers.c:3736-3789`) maps the native stages exactly for the emit decision:
`row44==0 → return`; `global_157a4c!=1 || !actor_is_rec → stage 4 else stage 3`;
stage 4 → `fifa96_keeper_reposition_b_finish`
(`src/fifa96_loader/fifa96_keeper.c:252-263`: controlled → `notify = human?0:5`,
else `install 0x19`).

---

## 5. GO / NO-GO

**GO — the stage-4 sit-5 emit (`0x77707`) is reachable on a live native path.** The
actor gate is **self-satisfying**: the row-`0x1B` body's own stage-2-wide `0x774DD`
block writes `[0x157A83]=EBP` at `0x77568` (no phase/side gate) and falls to the
stage-4 head, so the emit's `EBP==[0x157A83]` check passes without any external
actor transition. This **narrows** FU-273's T2b-L1 (which cited only the head
writer `0x76DBF`).

**Caveat (port-scale).** A faithful native replay still needs the whole stage-2
wide block ported, because the self-actor write *and* the `0x71C94`/`0x76B28`/`0x6E598`
work live there (FU180-L1/L2/L3) — the engine today reaches row `0x1B` via the
row-`0x1F` route (`handlers.c:3611`) with `actor_is_rec` from the head path only.
The emit **logic** is GO (reachable); the wide **body** stays a bounded slice.

---

## 6. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| row-`0x1B` head gates `0x76D33`/`0x76D46..5D`; camera place `0x76D6A..92`; `0x76DAE [rec+0x9B]=1`; `0x76DB5 [0x157750]=[rec+0x5D]+0x38`; `0x76DBA CALL 0x700F4`; `0x76DBF [0x157A83]=EBP` | `disassemble_bytes 0x76D28` |
| stage switch `JMP CS:[EAX*4+0x76D14]`; table = `{0x76DF6,0x76E3B,0x76FC7,0x7769F,0x776C1}` | `disassemble_bytes 0x76DC7`; `read_memory 0x76D14` |
| stage-2 wide `0x76FD5..0x77636` (gates + pan cells + `0x77423 CALL 0x71C94`) | `disassemble_bytes 0x76FD5`; `disassemble_bytes 0x771D4`; `disassemble_bytes 0x77490` |
| `0x774DD` re-place sets `[0x157A83]=EBP` `0x77568`; `CALL 0x6E598 0x7756E`; sit-7 emit `0x77589` | `disassemble_bytes 0x77490` |
| stage-4 head `0x77636..0x776BB` (3rd `0x6E598 0x77682`) + tail `0x776C1..0x77707` (EBP==`[0x157A83]`; `[0x157A49]>>0x18==1` skip; else `0x19`) | `disassemble_bytes 0x77636`; `disassemble_bytes 0x776C0` |
| `0x7770E` install row `0x19`; epilogue `0x7771E`; row `0x1C` starts `0x77728` | `disassemble_bytes 0x776C0`; `disassemble_bytes 0x7771E` |
| `FUN_00071C94` writes (camera triple, `[0x1577B8]`, `[0x1577F0]`, `[0x1577CA]`, `[0x1577CE+side*4]`, `[0x157815]`, `[0x157822]`/`[0x157820]`) | `disassemble_function 0x71C94`; `get_function_by_address 0x71C94` |
| `FUN_00076B28` steering writes `[+0x83/85/87/73/75/71]` folds pos via `0x114E04/0x795A4/0x795B4` | `disassemble_function 0x76B28` |
| `0x76B28` refs = 2 (`0x76FA2` row `0x1B`, `0x779B4` row `0x1C`) | `get_xrefs_to 0x76B28` |
| `0x71C94` refs = 11 (incl. `0x77423`) | `get_xrefs_to 0x71C94` |
| `0x6E598` refs = 84; three in row `0x1B` (`0x76F91`,`0x7756E`,`0x77682`) | `get_xrefs_to 0x6E598` |
| row `0x1B` has no Ghidra function | `get_function_by_address 0x76D28` = "No function found" |
| engine `fifa96_match_action_1B`; actor_is_rec; stage machine; stage-4 finish→notify 5 / install 0x19 | `handlers.c:3692-3790`; `src/fifa96_loader/fifa96_keeper.c:252-263` |

**Carried docs (read, not rewritten):** FU-273 §2.1/§2.2/§2.3; FU-272; FU180
legs L1-L5; FU186 legs L1-L7.

---

## 7. Numbered open legs (with native owners)

* **FU285-L1 — the ungated row-`0x1B` self-actor write at `0x77568`.** FU-273 §2.3
  cites only the head writer `0x76DBF`; fresh bytes show a **second** writer
  `[0x157A83]=EBP` at `0x77568` (inside the `0x774DD` stage-2-wide block) with no
  `phase`/`side` gate, and it falls to the stage-4 head. This is the mechanism that
  makes the stage-4 emit self-satisfying and **narrows T2b-L1**. The engine's
  `actor_is_rec` (`handlers.c:3733`) does not model it. Owner: `0x774DD` (FU180-L1)
  + `fifa96_match_action_1B`.
* **FU285-L2 — the stage-4 emit phase gate `[0x157A49]>>0x18==1 → skip`.** Fresh
  `0x776E2` (`CMP EAX,1; JZ 0x7771E`) is a gate FU-273 omitted from the tail byte
  list; it is the same phase byte read at `0x77648`. The engine routes it via
  `mr->global_157a4c == 1u` (`handlers.c:3762`). Native vs engine cell naming
  (`[0x157A49]` vs `global_157a4c`) is **not reconciled** — could be an engine
  alias/mislabel. Owner: `handlers.c:3762`; native `0x77648`/`0x776E2`.
* **FU285-L3 — `FUN_00071C94` pan/camera-event setter (FU180-L2).** Writes the
  `[0x1577B8..0x1577BD]` 6-byte pan record, `[0x1577F0]` (max/`0x640`), the tracked
  cells `[0x1577CA]`/`[0x1577CE+side*4]`, the dir byte `[0x157815]` (`0x10E169`
  table / `0x8DD70`), and clears `[0x157822]/[0x157820]`. Unported; the per-side
  tracked array and the dir byte are the port surface. Owner: FU180-L2.
* **FU285-L4 — `FUN_00076B28` steering (FU180-L3).** Writes
  `[rec+0x83/85/87/73/75/71]`; shares `0x114E04`/`0x795A4`/`0x795B4`; refs 2
  (`0x76FA2` row `0x1B`, `0x779B4` row `0x1C`). Unported. Owner: FU180-L3.
* **FU285-L5 — the three `0x6E598` calls inside row `0x1B`.** Fresh
  `get_xrefs_to 0x6E598` locates `0x76F91`, `0x7756E`, `0x77682` — the R11-L1
  claim anim-selection bridge is invoked by row `0x1B` (not only row `0x1E`).
  Owner: FU163 R11-L1.
* **FU285-L6 — the sit-7 emit at `0x77589`** (`EAX=7, EDX=side, EBX=0`), a sibling
  of the sit-5 emit inside the `0x774DD` block. The engine routes row `0x1B` sit-5
  only (`handlers.c:3787`). Owner: FU269-L5.
* **FU285-L7 — the head camera-place primitive `0x76DBA CALL 0x700F4`** (flag 1)
  and its sibling at `0x77542` (flag 1) / `0x71CC3` (flag 0). FU-273's head byte
  list omitted the `0x76DBA` call. Owner: FU180-L1.
* **FU285-L8 — the row-`0x1B` body is not a Ghidra function.** Dispatched only
  through the action table (`0x1106E0[0x1B]=0x76D28`); body `0x76D28..0x77727`
  (RET), next row `0x1C` at `0x77728`. Recorded for the byte-exact boundary; no
  under- or over-read in this derivation.
