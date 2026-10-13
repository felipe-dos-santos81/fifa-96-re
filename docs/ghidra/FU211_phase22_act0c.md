# FU-211 — phase `0x22` / act `0xC` (`0x8B900`): the extra-time-SET act-slot timer — body timeline, OL-85 flag answer, install/invoke chain and the tape verdict (R59)

Provenance: R59, 2026-10-13, read-only. Authoritative program **`/FIFA96.EXE`**
(Ghidra MCP; scripts disabled; case-sensitive program argument; confirmed this
session by `get_current_program_info` — name `/FIFA96.EXE`, path
`game/hdd/FIFA96/FIFA96.EXE`, `watcom:LE:32:default`, 2706 functions,
min `0x10000`, `.image` overlay present). Every address/byte/xref-count below is
a fresh `disassemble_bytes` / `read_memory` / `get_xrefs_to` /
`get_function_by_address` / `decompile_function` of **this session** unless
marked *cross-cited* (then the frozen FU doc is named). Engine/loader source read
first-hand (`src/fifa96_engine/fifa96_match_act.c`,
`src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`include/fifa96_engine/fifa96_match_run.h`, `docs/ENGINE.md`). **No writes**:
no rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands; the ISO / `game/hdd/` untouched. The alias traps are
applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).

Scope (R59 brief): derive **phase `0x22` / act `0xC`** — the function at
`0x8B900` (verify first-hand). Full body timeline; does it set the OL-85 flag
`[0x157AC0]`; its stage chain; its writes; how it is installed/invoked; the tape
verdict (if wired, does M2 move); and precisely what the engine must add. This
slice refines FU-195 §3/§4 first-hand and lands three quoted errata against it
(§8).

Cross-reference at a glance: phase `0x22`'s body is the **same dword** as act
`0xC`'s slot — `0x110794[0x22] = 0x1107EC[0xC] = 0x8B900` (the FU165-L9 dual
identity; fresh §1). "Wiring phase `0x22`" in the engine census means
registering `fifa96_match_phase_table[0x22]`, which the act dispatcher resolves
for act `0xC` (engine §5).

---

## 1. Fresh address / slot confirmation

* **Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords,
  `0x110794..0x110820`). Row `0x22` bytes at file offset `0x110794 + 0x22*4 =
  0x11081C`: `00 b9 08 00` → **`[0x22] = 0x08B900`**. (Tail rows fresh:
  `[20]=08B688 [21]=08B874 [22]=08B900`.)
* **Act table `0x1107EC`** — fresh `read_memory` (72 B). Act `0xC` at
  `0x1107EC + 0xC*4 = 0x1107EC + 0x30 = 0x11081C`: `00 b9 08 00` →
  **`act 0xC = 0x08B900`**; act `0xB` at `0x1107EC + 0xB*4 = 0x110818` =
  `0x08B874`. Base relation `0x110794 + 0x16*4 = 0x1107EC`
  (fresh: `0x110794 + 0x58 = 0x1107EC`), so **act `a` → phase `0x16+a`**; act
  `0xC` → phase `0x22`. The bytes past act `0xC` are unrelated
  (`ff076000 38ce0700 ff074000 …`, fresh).
* **xrefs to `0x8B900`** — fresh `get_xrefs_to` = **1**, `DATA`, from
  `0x11081C` (the shared table slot). Nothing else references the body.
* **`get_function_by_address 0x8B900`** → **"No function found"** (a table-only
  target; the body is derived from raw disassembly, exactly as FU-195 §3 found).
* The 2-byte pad between the body and its producer: fresh `read_memory 0x8B9CA`
  (8 B) = `8b c0 53 51 52 80 3d c0` → `0x8B9CA: MOV EAX,EAX` (the alignment pad),
  `0x8B9CC: PUSH EBX` (the producer `FUN_0008B9CC`, §4).

---

## 2. The body `0x8B900` — full timeline (fresh, first-hand)

Fresh `disassemble_bytes 0x8B900` (`0x8B900..0x8B9C9`, 202 B, **67 instructions**;
`RET` at `0x8B9C9`). A **three-stage act-slot timer**. Registers `EBX/ECX/EDX/ESI`
saved (no record/argument is read — the act-tail bodies ignore `EAX`/`EDX`).

**Head (shared with the act-0xB/0x9/0x20 siblings):**
```
0x8B900 PUSH EBX; PUSH ECX; PUSH EDX; PUSH ESI
0x8B904 MOV AX,  [0x00157A64]     ; 66a1647a1500  — AX = word[0x157A64] (frame delta)
0x8B90A MOV DX,  [0x00158818]     ; 668b1518881500 — DX = act timer
0x8B911 ADD EDX, EAX              ; 01c2
0x8B913 MOV AL,  [0x00158829]     ; a029881500 — AL = stage byte
0x8B918 MOV [0x00158818], DX      ; 66891518881500 — timer += frame_delta
0x8B91F CMP AL, 1                 ; 3c01
0x8B921 JC  0x8B92E               ; 720b — stage 0 (AL<1) -> stage-0 block
0x8B923 JBE 0x8B95C               ; 7637 — stage 1 -> stage-1 block
0x8B925 CMP AL, 2                 ; 3c02
0x8B927 JZ  0x8B98F               ; 7466 — stage 2 -> stage-2 block
0x8B929 POP ESI;POP EDX;POP ECX;POP EBX; RET   ; stage > 2: no-op (5e 5a 59 5b c3)
```

**Stage 0 (`0x8B92E`):**
```
0x8B92E TEST AL, AL               ; 84c0  — AL == 0 here (only reached via 0x8B921 JC)
0x8B930 JNZ 0x8B9C5               ; 0f858f000000 — dead guard (see FU211-L5)
0x8B936 MOV EAX, 0x1E             ; b81e000000
0x8B93B CALL 0x000974DC           ; e89cbb0000 — the 0x1E event/sound (see FU211-L3)
0x8B940 MOV BL, [0x00158829]      ; 8a1d29881500 — BL = stage (0)
0x8B946 XOR ESI, ESI              ; 31f6
0x8B948 INC BL                    ; fec3 — BL = 1
0x8B94A MOV [0x00158818], SI      ; 66893518881500 — timer = 0
0x8B951 MOV [0x00158829], BL      ; 881d29881500 — stage = 1
0x8B95B RET
```

**Stage 1 (`0x8B95C`):**
```
0x8B95C MOV EAX, [0x00158816]     ; a116881500
0x8B961 SAR EAX, 0x10             ; c1f810 — alias trap: = word[0x15881A] (the timeline cell)
0x8B964 CMP EAX, 0x3C             ; 83f83c
0x8B967 JL  0x8B9C5               ; 7c5c — < 0x3C -> return (wait)
0x8B969 MOV EAX, 0x1E ; CALL 0x974DC   ; 0x974DC(0x1E)
0x8B973 MOV DH, [0x00158829]      ; 8a3529881500
0x8B979 XOR ECX, ECX              ; 31c9
0x8B97B INC DH                    ; fec6 — DH = 2
0x8B97D MOV [0x00158818], CX      ; 66890d18881500 — timer = 0
0x8B984 MOV [0x00158829], DH      ; 883529881500 — stage = 2
0x8B98E RET
```

**Stage 2 (`0x8B98F`, terminal):**
```
0x8B98F MOV EAX, [0x00158816] ; SAR EAX, 0x10   ; a116881500/c1f810 -> word[0x15881A]
0x8B997 CMP EAX, 0x3C ; JL 0x8B9C5              ; 83f83c/7c29 — < 0x3C -> return (wait)
0x8B99C MOV EAX, 0x1E ; CALL 0x974DC            ; 0x974DC(0x1E)
0x8B9A6 XOR EBX, EBX ; XOR EDX, EDX              ; 31db/31d2
0x8B9AA MOV [0x00158818], BX                    ; 66891d18881500 — timer = 0
0x8B9B1 XOR AH, AH                              ; 30e4
0x8B9B3 MOV [0x00158808], EDX                   ; 891508881500 — handler = 0 (act slot cleared)
0x8B9B9 MOV [0x00158828], AH                    ; 882528881500 — act id = 0
0x8B9BF MOV [0x00158829], AH                    ; 882529881500 — stage = 0
0x8B9C5 POP ESI;POP EDX;POP ECX;POP EBX; RET
```

**Derived semantics.** A **three-stage timer** on the act slot. The head adds the
frame delta to the act timer (`word[0x158818] += word[0x157A64]`, byte-identical
shape to the act-0xB `0x8B874`, act-9 `0x88F4C` and phase-0x20 `0x8B688` heads).
Each stage fires the `0x1E` event via `0x974DC`, resets the timer, and advances
(stage 0→1→2); stages 1 and 2 **wait** on the `word[0x15881A] >= 0x3C` gate
(alias trap `[0x158816]>>16`); the terminal stage 2 clears the act slot
(handler/id/stage/timer = 0). `stage > 2` is the `0x8B929` POP/RET no-op.

**Exact write set of the body (all 8 writes, fresh):**

| target | role | sites |
|---|---|---|
| `word[0x158818]` | act timer | `0x8B918` (+=delta), `0x8B94A`/`0x8B97D`/`0x8B9AA` (=0) |
| `byte[0x158829]` | act stage | `0x8B951`/`0x8B984` (advance), `0x8B9BF` (=0) |
| `dword[0x158808]` | installed handler | `0x8B9B3` (=0, terminal) |
| `byte[0x158828]` | act id | `0x8B9B9` (=0, terminal) |

Plus one call: `0x974DC(0x1E)` (three times). **The body performs no
`FUN_000740A0` phase write, touches no record, and — the R59 question — writes
`[0x157AC0]` nowhere.**

---

## 3. Does it set the OL-85 flag `[0x157AC0]`? — **No.**

Fresh `get_xrefs_to 0x157AC0` = **19** references. The **writers** are:

| site | function | fresh evidence |
|---|---|---|
| `0x8B2AC` | `FUN_0008AF38` | disasm `c605c07a150001` = `MOV byte[0x157AC0],0x1` |
| `0x8B2C1` | `FUN_0008AF38` | disasm `c605c07a150001` = `MOV byte[0x157AC0],0x1` |
| `0x8B3A2` | `FUN_0008AF38` | xref `WRITE` (not disassembled this slice) |
| `0x8B5CD` | `FUN_0008AF38` | disasm `8815c07a1500` = `MOV [0x157AC0],DL` (`DL=1` set at `0x8B5B9`) |
| `0x73F58` | `FUN_00073EE0` | xref `WRITE` (not disassembled this slice) |
| `0x389A4` | `FUN_00038630` | xref `WRITE` (not disassembled this slice) |
| `0x89E8C` | *(no function — raw body)* | xref `WRITE` |

**`0x8B900` is not among them, and neither is its producer `FUN_0008B9CC`** —
the producer only **reads** the flag (`0x8B9CF`, `0x8BAC6`, both fresh xref
`READ`). So: **the act-0xC body does not set OL-85; the OL-85 extra-time flag is
written by the clock harness `FUN_0008AF38` (and by `FUN_00073EE0` /
`FUN_00038630` / the `0x89E8C` body), and is read by the producer to choose
between act `0xB` and act `0xC`.**

The fresh harness set-site (`0x8B5A1..0x8B5CD`) is a **period-boundary tie /
decider test**: `0x8B5A1 CMP AX,[0x157AC7]` (score A vs score B),
`0x8B5AE JZ …` (tie → skip), `0x8B5B0 CMP byte[0x157AC0],0` / `0x8B5B7 JNZ`
(already on → skip), then `0x8B5B9 MOV DL,0x1 ; 0x8B5BB MOV EBX,0x20 ;
0x8B5CD MOV [0x157AC0],DL` — the flag is raised to 1 when the period ends
**decisive** (the native extra-time trigger). The arm then emits
(`0x8B5F5 CALL 0x8F188` with `EAX=0xC0`) and calls the producer
(`0x8B5FA CALL 0x8B9CC`). Note the `0x8B2A7 CMP AX,DX` / `0x8B2AC MOV
byte[0x157AC0],1` set-site is the twin at the other period boundary.

---

## 4. Install / invoke chain (fresh)

### 4.1 The act dispatcher `FUN_000888FC` (`0x888FC`)
Fresh `disassemble_bytes 0x888FC` (`0x888FC..0x888FC+`) confirms the install:
```
0x888FC PUSH ECX
0x888FD MOVSX CX, byte[0x00158828]   ; 660fbe0d28881500 — current act id (latch)
0x88905 CMP AX, CX                   ; 6639c8
0x88908 JZ  0x8893D                  ; 7433 — same act -> no reinstall
0x8890A MOV [0x00158828], AL         ; a228881500 — act id = AL
0x8890F CWDE; SHL EAX,0x2; ADD EAX,0x1107EC   ; 98/c1e002/05ec071100
0x88918 MOV [0x00158829], DL         ; 881529881500 — stage = DL
0x8891E XOR EDX, EDX
0x88920 MOV EAX, [EAX]               ; 8b00 — handler = act_table[act]
0x88922 MOV word[0x00158818], DX     ; 66891518881500 — timer = 0
0x88929 MOV [0x00158808], EAX        ; a308881500 — installed handler
0x8892E TEST BX, BX                  ; 6685db
0x88931 JZ  0x8893D                  ; 740a — BX==0: no invoke
0x88933 TEST EAX, EAX ; JZ 0x8893D   ; handler==0: no invoke
0x88937 CALL dword[0x00158808]       ; ff1508881500 — INVOKE-NOW
0x8893D POP ECX; RET
```
So `FUN_000888FC(EAX=act, DL=stage, BX=invoke-now)` installs `act_table[act]` into
`[0x158808]`, sets the stage to `DL` and the timer to 0, and (when `BX!=0`)
calls the handler **immediately**. For act `0xC` with `BX=1`, `DL=0`, `0x8B900`
runs its **stage 0** at once.

### 4.2 The producer `FUN_0008B9CC` (`0x8B9CC..0x8BAEF`)
Fresh xrefs: **3**, all `UNCONDITIONAL_CALL` inside `FUN_0008AF38` (`0x8B574`,
`0x8B5FA`, `0x8B616`). Fresh `decompile_function 0x8B9CC` + `disassemble_bytes`:
the tail (shared, unconditional) is
```
0x8BAC6 CMP byte[0x00157AC0], 0       ; 803dc07a150000
0x8BACD JZ  0x8BADB                   ; 740c
0x8BACF MOV EBX, 1                    ; bb01000000
0x8BAD4 MOV EAX, 0xC                  ; b80c000000  — extra-time ON  -> act 0xC
0x8BAD9 JMP 0x8BAE5                   ; eb0a
0x8BADB MOV EBX, 1 ; MOV EAX, 0xB     ; extra-time OFF -> act 0xB
0x8BAE5 XOR EDX, EDX                  ; 31d2 — stage 0
0x8BAE7 CALL 0x000888FC               ; e810ceffff — install act(EAX), stage 0, invoke-now(BX=1)
```
So the producer invokes **act `0xC` when `[0x157AC0] != 0`**, else act `0xB`,
always with `DL=0` and `BX=1`. The producer's **phase** write (via
`0x8BAC1 CALL 0x740A0`) is separate: extra-time OFF & period `< 4` → phase `0xC`
(resolved `EAX` at `0x8BAB9`); extra-time ON → phase `0x13`/`0x14`/`0x14` chosen
by the score tiebreak (`[0x157AC5]` vs `[0x157AC7]`) and the `FUN_00012230` /
`FUN_00012250` probes (`0x8BA37`/`0x8BA47`/`0x8BA5E`), with `FUN_00036200(3)` on
the last branch (`0x8BA92`). **The producer never writes phase `0x22`** — phase
`0x22` is reachable only as the act-`0xC` slot (the §1 dual identity).

### 4.3 The harness `FUN_0008AF38` and the clock tail
`FUN_0008AF38` (`0x8AF38..0x8B687`) is the clock harness. It calls the producer
on the period-rollover paths (`0x8B574` after `0x8B56F CALL 0x4C374`; `0x8B5FA`
on the flag-set arm; `0x8B616` on the period-4 arm) and increments the period
counter `[0x157AC2]` (`0x8B58A MOV [0x157AC2],BH`, fresh). Its **tail ticks the
installed act handler once per granted frame**:
```
0x8B669 CMP dword[0x00158808], 0      ; 833d0888150000
0x8B670 JZ  0x8B678                    ; 7406
0x8B672 CALL dword[0x00158808]        ; ff1508881500 — tick the act body
```
So `0x8B900` is (a) run immediately on install (`BX=1`) and (b) ticked each
granted frame via this tail. Fresh `get_xrefs_to 0x8AF38` = **2** (`0x4B1A6` in
`FUN_0004B100`, `0x7440B`), so the harness is live from the frame-update chain
`FUN_0004B100`.

**Chain summary:** frame update → `FUN_0004B100` → `FUN_0008AF38` (harness) →
period-rollover → `FUN_0008B9CC(0x8B574)` → reads `[0x157AC0]` → if set:
`0x8BAE7 CALL 0x888FC(0xC,0,1)` → installs + invokes `0x8B900` → ticked by
`0x8B672`.

---

## 5. The engine seam and the tape verdict

**Native slot → engine row.** The engine resolves the act handler as
`fifa96_action_phase_act(act) = 0x16 + act`
(`src/fifa96_loader/fifa96_action_handlers.c:1114`), so act `0xC` → phase `0x22`,
looked up in `fifa96_match_phase_table[0x22]`. Fresh first-hand source:
`fifa96_match_handlers.c:5127` registers `{0x22, NULL, …}` (row NULL), while the
body is **already ported and exported**:
`fifa96_match_action_phase_22` (`src/fifa96_engine/fifa96_match_run.c:6158`,
declared `include/fifa96_engine/fifa96_match_run.h:1605`). The port binds the
`0x974DC(0x1E)` event to `mr->ref_whistle = 0x1E` (the sibling phase-1C/1D
convention; FU195-L3).

**Dispatch guard.** `fifa96_match_act_dispatch`
(`src/fifa96_engine/fifa96_match_act.c`) resolves `phase`, then
`if (fifa96_match_phase_table[phase].fn == NULL) return FIFA96_OK;` — so with row
`0x22` NULL, an act-`0xC` install runs **nothing**; `fifa96_match_act_tick`
carries the same guard.

**The producer is hardwired off.** `fifa96_match_run_phase_drive`
(`src/fifa96_engine/fifa96_match_run.c:2564`) calls
`fifa96_action_phase_period_end((uint8_t)(mr->state.period - 1u), 0u, …)`
(`:2583`) — the second argument is the extra-time flag, **hardcoded `0u`** — then
`fifa96_match_act_dispatch(mr, out.act, 0u, 1)`. With `extra_time == 0` the
chooser (`fifa96_action_handlers.c:1164`) always returns `out.act = 0x0B`, so act
`0xC` is **never** dispatched. The engine's `[0x157AC0]` analogue has no producer
(the run-struct `extra_time` at `fifa96_match_run.h:278` is a separate overlay
field; the phase-drive comment `match_run.c:2557` names OL-85 and FU143).

### Tape verdict — **does M2 move if wired? No.**

1. **Register `fifa96_match_phase_table[0x22] = fifa96_match_action_phase_22`
   alone ⇒ byte-identical M2.** Nothing invokes phase `0x22`: the only invoker
   is act `0xC` via `fifa96_match_act_dispatch`, which `phase_drive` reaches only
   through `out.act = 0x0C`, which needs `extra_time != 0` — and the call site
   hardwires `0u`. Phase `0x22` is also absent from the native `FUN_000740A0`
   written set (`{0,1,2,3,4,5,8,9,0xA,0xB,0xC,0xD,0xF,0x10,0x11,0x13,0x14,0x15}`,
   FU-195 §5), so the per-record walk never stages it either. The row would be
   **wired-but-unreachable**.
2. **Even with a faithful extra-time producer, M2 is still tape-neutral.** The
   body writes only the act slot (`[0x158808]`/`[0x158828]`/`[0x158829]`) and
   `[0x158818]`, and stages the unported audio event. The M2 render reads exactly
   `pos_x/pos_y/pos_z`, `anim_id`, `frame`, `hidden` per record (FU-206 §5
   ground truth) and `mr->ref_whistle` is not render-read; the M2 v18 tape
   stages no extra-time period (it ends on the natural goal + post-goal hold; no
   four-period rollover). So the row is **interactive-only / dormant on M2** —
   identical to FU-206 §4 Tier 1 #3 and §5 PR-3.

**Census.** Current `51/80 = 63%` (`docs/ENGINE.md:8`, 32 action + 19 phase).
Registering row `0x22` would read `52/80 = 65%`, but only honestly if the OL-85
producer leg also lands; registration alone is a wired-but-unreachable row and
per FU-206 §5 must not be counted. This recon slice asserts no number (AGENTS.md
"no hand-set value"; the implementer recomputes `floor(100 × wired/80)` from the
landed set).

---

## 6. Precisely what the engine must add

To honestly wire phase `0x22` / act `0xC`:

1. **Register the slot** (the "wiring"): `fifa96_match_phase_table[0x22] =
   fifa96_match_action_phase_22` in `src/fifa96_engine/fifa96_match_handlers.c`
   (row currently NULL at `:5127`). No body change — `fifa96_match_action_phase_22`
   is already ported/exported.
2. **Thread the live extra-time flag** into `fifa96_match_run_phase_drive` →
   `fifa96_action_phase_period_end` (replace the hardcoded `0u` at
   `src/fifa96_engine/fifa96_match_run.c:2583`). The chooser already derives
   `out.act = 0x0C` when `extra_time != 0` (`fifa96_action_handlers.c:1164`), so
   no chooser change is needed.
3. **Port the `[0x157AC0]` producer** (the FU195-L2 owner): the harness
   `FUN_0008AF38` set-sites (`0x8B2AC`/`0x8B2C1`/`0x8B3A2`/`0x8B5CD`) — flag = 1
   on the decisive period boundary (the `[0x157AC5]` vs `[0x157AC7]` score test,
   §3) — plus the `[0x157AC2]` period-counter increment (`0x8B58A`) and the
   extra-time situation-8 route already stubbed in
   `fifa96_match_run_period_setup` mode 0 under `state.phase == 0xC &&
   global_157ac2 == 4` (`match_run.c:~3060`).
4. **No render-side work.** The body's only visible-in-principle effect is the
   `0x974DC(0x1E)` audio event (FU195-L3).

Absent (3), steps (1)+(2) are dead code on every fixture that does not reach an
extra-time period — including all of M2.

---

## 7. Numbered legs

1. **FU211-L1 — phase `0x22` is reachable only via act `0xC`, which is
   producer-dormant.** Registering `fifa96_match_phase_table[0x22]` alone neither
   dispatches nor moves M2; the honest census `+1` requires the `[0x157AC0]`
   extra-time producer (step 3). Do **not** count the row off registration alone
   (no hand-set census). Owner: the extra-time flag producer / FU195-L2.
2. **FU211-L2 — the `word[0x15881A] >= 0x3C` gate (fresh §2).** The body's
   stage-1/2 waits gate on the timeline cell `word[0x15881A]` (alias trap
   `[0x158816]>>16`, sites `0x8B961`/`0x8B997`); its producer is unported, so the
   derived body substitutes the act timer (`mr->act.timer >= 0x3C`, FU195-L4).
   Same divergence class as phase 0x1F/0x20 (FU175-L5/FU188-L2).
3. **FU211-L3 — the `0x974DC(0x1E)` audio sink.** `FUN_00064EC0` (ring push
   `FUN_00064EF0(1,·)`) + `FUN_00065CC0` (audio handle), unported; the derived
   bodies stage it as `mr->ref_whistle = 0x1E`. Whether the ring push is
   tape-visible is unverified; assume not until a `cmp` proves otherwise. Owner:
   FU195-L3.
4. **FU211-L4 — the extra-time producer body.** `FUN_0008AF38`'s flag set-sites
   (`0x8B2AC`/`0x8B2C1`/`0x8B3A2`/`0x8B5CD`), the score tiebreak
   (`[0x157AC5]`/`[0x157AC7]`), the period counter `[0x157AC2]` (increment
   `0x8B58A`, reset writers `0x73F82`/`0x4B0DE`/`0x4BA20`/`0x4BA31`) and the
   extra-time situation-8 route are a separate slice. Owner: the extra-time flag
   producer (FU195-L2).
5. **FU211-L5 — the redundant stage-0 guard.** Fresh `0x8B92E TEST AL,AL ;
   0x8B930 JNZ 0x8B9C5` is dead: `0x8B92E` is reached only via `0x8B921 JC`
   (`AL < 1` ⇒ `AL == 0`), so the `JNZ` never fires. Recorded as a fresh
   first-hand detail; not a semantic divergence (the derived body correctly
   treats it as stage-0 fall-through). No owner needed.

---

## 8. Errata against FU-195 (quoted; prior sections are not rewritten)

* **FU-195 §3, stage 0.** FU-195 §3 renders the stage-0 block as beginning
  "`; ---- stage 0 (0x8B92E) ----` / `0x8B936 MOV EAX, 0x1E ; 0x8B93B CALL
  0x974DC`". Fresh disassembly shows the stage-0 label is `0x8B92E` **with a
  two-instruction guard first**: `0x8B92E TEST AL,AL` (`84c0`); `0x8B930 JNZ
  0x8B9C5` (`0f858f000000`); the event is at `0x8B936`. FU-195 §3's table starts
  at `0x8B936` and omits the guard (see FU211-L5). Quoted so the frozen text is
  not rewritten.
* **FU-195 §4, the flag set-site.** FU-195 §4 says "`0x8B5D3 MOV
  [0x157AC0],DL(1)`". Fresh `disassemble_bytes 0x8B5A0` shows the **write** is at
  `0x8B5CD` (`8815c07a1500` = `MOV [0x157AC0],DL`, with `DL=1` set at `0x8B5B9`);
  `0x8B5D3` is the following `CMP AX,CX` (`6639c8`). The set-site address is
  **`0x8B5CD`**.
* **FU-195 §4, the producer decompile.** FU-195 §4's `FUN_0008B9CC` pseudocode
  is a simplification (it renders the extra-time branch as "phase 0x13/0x14 +
  side 0x157ABE/0x157ABF" without the tiebreak). Fresh `decompile_function
  0x8B9CC` + `disassemble_bytes` show the full extra-time branch: the period<4
  score test (`0x8B9F2 CMP AX,[0x157AC7]`, `0x8BA04 SETBE`), the period>=4
  `[0x1587D8]`/`[0x1587D9]` test (`0x8BA17`), the `FUN_00012230` /
  `FUN_00012250` probes (`0x8BA37`/`0x8BA47`/`0x8BA5E`), `FUN_00036200(3)`
  (`0x8BA92`), and the `SAR EDX,0x18` side extraction (`0x8BABE`/`0x8BABE`).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, 2706
  functions.
* `read_memory` — `0x110794` (140 B), `0x1107EC` (72 B), `0x157AC0` (16 B),
  `0x8B9CA` (8 B).
* `get_xrefs_to` — `0x8B900` (1, DATA), `0x8B9CC` (3), `0x8AF38` (2),
  `0x888FC` (11), `0x157AC0` (19), `0x157AC2` (56), `0x158808` (22).
* `disassemble_bytes` — `0x8B900` (202 B / 67 insns), `0x8B9CC` (292 B),
  `0x8B5A0` (128 B), `0x8B560` (65 B), `0x8B2A0` (48 B), `0x888FC` (84 B),
  `0x8B650` (64 B).
* `decompile_function` — `0x8B9CC`.
* `get_function_by_address` — `0x8B900` (none — table-only), `0x8AF38`,
  `0x89E8C` (none — raw body).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_act.c`
(whole); `src/fifa96_engine/fifa96_match_run.c` (2504..2600, the phase driver +
`extra_time = 0u`; 2985..3130, the situation/period producer; 6100..6190, the
ported phase-21/22 bodies); `src/fifa96_engine/fifa96_match_handlers.c`
(4967..5170, the two tables + the dispatch resolvers; the `{0x21, …}`/`{0x22,
NULL}` rows); `src/fifa96_loader/fifa96_action_handlers.c` (1112..1215,
`fifa96_action_phase_act` + `fifa96_action_phase_period_end` + the situation
table); `include/fifa96_engine/fifa96_match_run.h` (250..282, 1585..1610);
`docs/ENGINE.md` (line 8, the census; 564..591, 739..756, 983).

Docs cross-cited: FU-143 (§3.3/§4, the period chooser), FU-165 (the dual
identity), FU-175 (§1/§2, the act/clock seam), FU-190, FU-195 (§2/§3/§4/§5/§6.1,
FU195-L2/L3/L4 — the errata target), FU-196, FU-202, FU-206 (§4 Tier 1 #3, §5
PR-3, the render-read ground truth). AGENTS.md census rule. **No writes beyond
this slice file.**
