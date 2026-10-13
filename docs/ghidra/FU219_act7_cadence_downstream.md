# FU-219 — the act-7 invoke-now cadence (FU208-L4) + the row-`0x19` / phase-`0x10` downstream installs (phase-20 wave R67)

Provenance: M2 phase-20 wave **R67**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; this session `get_current_program_info` = `FIFA96.EXE`, path
`/FIFA96.EXE`, `watcom:LE:32:default`, `image_base 0`, `min_address 0x10000`,
`max_address .image::00174a2a`, 2706 functions, 15239 symbols, `.image` overlay
present). Every address/byte/instruction/xref below is a fresh `read_memory` /
`disassemble_bytes` / `disassemble_function` / `get_function_by_address` /
`get_xrefs_to` / `search_instructions` / `get_current_program_info` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). LE-image
globals sit at `+0x100000`; the addresses here are `/FIFA96.EXE` program
(runtime) addresses. The alias traps are applied (`dword[x]>>16` = the word at
`x+2`, `>>24` = the byte at `x+3`). No writes: no rename, comment, label,
function, script or project save; `/FIFA96.EXE` untouched; the ISO/`game/hdd/`
untouched; no git commands. Model: FU-209. This slice is a **derivation** (no
production code). It extends FU-209 (`FU209_act7_phase1d.md`), FU-208
(`FU208_situation8_selector.md`) and FU-205 (`FU205_gated_noop_verification.md`)
and does not rewrite them.

Scope (the R67 brief): (1) the **act-7 invoke-now cadence** — from the invoke-now
site `0x8AEAE` trace the exact install + tick sequence and settle, byte-level,
whether the arm-0 body runs the **same frame** as the sit-8 dispatch or is
**deferred one frame** through `BX` / the clock tail `0x8B669`; state whether the
port matches the **FU208-L4** claim; (2) the **downstream installs** — enumerate
**every** store in the phase-`0x1D` arm-0 region `0x898B4..0x899ED` (the phase
`0x10` writer, the `[0x15882A]` clear, the row-`0x19` install, the gated
row-`0x20` install `0x899A2`) and confirm the port arm-by-arm; (3) **row `0x19`**
— what it is (action-table slot `0x1106E0 + 0x19*4`) and whether it is wired;
(4) the **exit** — how act 7 retires and what re-installs the next act; (5) the
**engine extension point** and M2 **tape reachability**.

---

## 0. One-line answer

**Same-frame.** Native sit 8 (`0x8AEAE MOV EBX,1`) passes `BX=1` into the act
seam `FUN_000888FC`, whose tail `0x8892E TEST BX,BX` / `0x88931 JZ` / `0x88937
CALL dword[0x158808]` invokes the phase-`0x1D` handler `0x89868` **synchronously,
on the dispatch frame** (gated only by the `0x88908` act-change latch); the clock
tail `0x8B669` is the *deferred* path, taken only when `BX=0`. The port does
**not** match: `fifa96_match_run_situation_side` dispatches act 7 with
`invoke = (out.act == 2u ? 1 : 0)` = **0** (`fifa96_match_run.c:2889-2890`), so
its arm-0 body lands at the clock-tail tick `fifa96_match_act_tick` instead of at
the dispatch. Because the engine's sit-8 producer path (`phase_drive` →
`period_setup(mode 0)`, `:4092`) and the tick (`:4114`) share one
`fifa96_match_run_frame`, the arm-0 *install frame* is not shifted on the live
path, but the ported body is a one-arm-per-call `switch` (no fall-through), so
its arm progression lags the native's inline cascade by one granted frame —
**FU208-L4 stands**. The arm-0 stores (phase-`0x10` writer `0x898DD`, the
`[0x15882A]`/`[0x157AB1]` clears, the row-`0x19` install `0x8990A`, the row-`0x20`
install `0x899A2`, the row-`0x1F` install `0x899CD`, the `0x7A028` reset) are
enumerated in §3; the port matches the phase-`0x10` write, the `[0x15882A]`
clear and the row-`0x19` install, and **legs** the two `[ESP+8]`/`[ESP+0xC]`
record-pointer installs. Row `0x19` = the action-table body `0x746E4`
(`fifa96_match_action_19`) and **is wired** (`handlers.c:5135`). The act retires
through arm 9 (`0x89E6E`), which clears its own slot and sets the extra-time flag
`[0x157AC0]=1` (`0x89E8C`) — **no situation re-dispatch**; the next act is
re-installed by whichever producer next calls the situation dispatcher
`FUN_0008A938`. The engine gap is the single invoke expression at
`fifa96_match_run.c:2890`; the M2 tape never reaches it (golden `441415fc…`,
byte-identical — the route is tape-dormant).

---

## 1. Fresh anchors

### 1.1 The entry and the act/phase tables (verified first-hand)

* Fresh `read_memory 0x1107EC` (40 B) = `00000000 c88d0800 2c920800 a49f0800
  20960800 ec900800 10910800 68980800 98a70800 4c8f0800` →
  **`0x1107EC[7] = 0x00089868`** (index 7, byte offset `0x1C`: `68 98 08 00`).
  The act table is `0x110794 + 0x16*4`, so **act 7 → phase `0x16+7 = 0x1D`**
  (the FU165-L9 dual identity; byte-identical to FU-208 §3 / FU-209 §1.2).
* Fresh `read_memory 0x110808` is *cross-cited* FU-209 §1.1 = `68 98 08 00` =
  **`0x89868`** = phase slot `0x1D` (`0x110794 + 0x1D*4`). Phase slot `0x1D` and
  act slot `7` both resolve to the same handler `0x89868`.
* The sole static reference to `0x89868` is the phase-table DATA slot
  *cross-cited* FU-209 §1.1 (`get_xrefs_to 0x89868` = 1, from `0x110808`); the
  only dispatch route is the phase seam (`CALL dword[0x158808]`).

### 1.2 The act seam `FUN_000888FC` — fresh `disassemble_function` (20 insns) + fresh bytes

```
0x888fc PUSH ECX
0x888fd MOVSX CX,byte[0x158828]     ; CX = current act id
0x88905 CMP AX,CX
0x88908 JZ 0x8893d                  ; <<< ACT-CHANGE LATCH: same act -> skip install AND invoke
0x8890a MOV [0x158828],AL           ; act id := new
0x8890f CWDE
0x88910 SHL EAX,2
0x88913 ADD EAX,0x1107ec
0x88918 MOV byte[0x158829],DL       ; stage := DL
0x8891e XOR EDX,EDX
0x88920 MOV EAX,[EAX]               ; handler := act_table[act]        (8b00)
0x88922 MOV word[0x158818],DX       ; timer := 0                       (66891518881500)
0x88929 MOV [0x158808],EAX          ; act slot := handler              (a308881500)
0x8892e TEST BX,BX                  ; (6685db)
0x88931 JZ 0x8893d                  ; <<< BX==0 -> NO invoke (defer)   (740a)
0x88933 TEST EAX,EAX                ; (85c0)
0x88935 JZ 0x8893d                  ; handler==0 -> no invoke          (7406)
0x88937 CALL dword[0x158808]        ; <<< INVOKE-NOW (ff1508881500)
0x8893d POP ECX
0x8893e RET
```

The three bytes that settle the cadence are **`0x8892E = 66 85 db`** (`TEST
BX,BX`), **`0x88931 = 74 0a`** (`JZ 0x8893D`) and **`0x88937 = ff 15 08 88 15
00`** (`CALL dword[0x158808]`): with `BX!=0` and a non-NULL handler, the handler
runs **synchronously inside this call** — i.e. within the caller's frame.

### 1.3 The sit-8 arm tail `0x8AEAE` — fresh `disassemble_bytes 0x8AE08` (200 B)

```
0x8AE08 MOV EAX,[0x157A49] / SAR EAX,0x18 / CMP EAX,1 / JZ 0x8AEAE
        ; gate byte [0x157A4C]: == 1 -> jump straight to the invoke
0x8AE19..0x8AE9E  (the match re-init: [0x157AAF]/[0x157A4C] writes, the
        FUN_000897F4(0xA) pair, the [0x1587CC+side*4] -> [0x1587D4] actor bind,
        the score-pair clear [0x157AC7]/[0x157AC5])
0x8AEAE MOV EBX,1                   ; <<< INVOKE-NOW  (bb01000000)
0x8AEB3 MOV EAX,7                   ; ACT 7          (b807000000)
0x8AEB8 XOR EDX,EDX                 ; stage 0        (31d2)
0x8AEBA CALL 0x888FC                ; <<< the act seam (e83ddaffff)
0x8AEBF ADD ESP,0x4 / POP EBP/ESI/ECX / RET
```

`0x8AEAE MOV EBX,1` is the brief's "invoke-now" and `0x8AEBA CALL 0x888FC` the
binding call (fresh call bytes recompute to `0x888FC`: `0x8AEBF - 0x25C3 =
0x888FC`). The `0x8AE08` gate reads `[0x157A49]>>24` = byte **`0x157A4C`**
(fresh bytes `a1497a1500` / `c1f818`; the write at `0x8AE2A = 880d4c7a1500`) —
the FU208-L5 byte drift, re-confirmed, not `0x157A4D`.

### 1.4 The clock tail `0x8B669` — fresh `disassemble_bytes 0x8B650` (64 B)

```
0x8B65b CMP byte[0x14c32a],0        ; (803d2ac3140000)
0x8B662 JNZ 0x8B669                 ; (7505)
0x8B664 CALL 0x8BAF0                ; (e887040000)
0x8B669 CMP dword[0x158808],0       ; (833d0888150000)
0x8B670 JZ 0x8B678                  ; (7406)
0x8B672 CALL dword[0x158808]        ; <<< the per-granted-frame tick (ff1508881500)
0x8B678...
```

This is the **deferred** mechanism: it calls the installed slot once per granted
frame *unconditionally* (no `BX`). For act 7 it is the *second* call of the
frame (the invoke-now in §1.3 is the first — see §2).

### 1.5 The 10-arm table `0x89840` and the epilogue (cross-cited / fresh)

* Fresh `read_memory` of the arm table is *cross-cited* FU-209 §1.4
  (`0x89840` = `{0x898B4, 0x899EE, 0x89A4E, 0x89A7A, 0x89AEA, 0x89B16, 0x89B50,
  0x89B94, 0x89BB6, 0x89E6E}`); the dispatcher head `0x898A0 CMP AL,9 / JA
  0x89EA6` and `0x898AC JMP dword CS:[EDX*4 + 0x89840]` are FU-209 §1.5.
* Fresh `disassemble_bytes 0x89EA6` (32 B): `0x89EA6 ADD ESP,0x14 ; 0x89EA9 POP
  EBP ; POP EDI ; POP ESI ; POP EDX ; POP ECX ; POP EBX ; RET` (bytes
  `83c414 5d 5f 5e 5a 59 5b c3`) — the shared epilogue all gates jump to.

### 1.6 The native frame driver `FUN_000493A0` — fresh `disassemble_function` (130 insns)

The per-frame loop head is `0x494A6`; inside one iteration:

```
0x49508 MOV EAX,EBX (0)
0x4950a CALL 0x4B02C          ; <<< the period machine (the sit-8 producer)
0x4950f CMP EDX,0x4 / JL 0x49523
0x49514 MOV EAX,0x200 / CALL 0x4C394
0x4951e CALL 0x4B100          ; <<< the entity chain (-> FUN_0008AF38, the clock body)
0x49523 CALL 0x53A48 / ...
```

Fresh `get_xrefs_to 0x4B02C` = **2**, both UNCONDITIONAL_CALL from
`FUN_000493a0` (`0x49415`, `0x4950a`). Fresh `get_xrefs_to 0x4B100` = **3**
(`FUN_000493a0` `0x4951e`, `FUN_00049b28` `0x49fbb`, `FUN_0004a228` `0x4a25a`).
Fresh `search_instructions` (`CALL 0x4B02C` **within** `FUN_0008AF38`, 475 insns
scanned) = **0 matches**: the clock body `FUN_0008AF38` (which carries the tail
`0x8B669`) does **not** call the period machine. So within a single
`FUN_000493A0` iteration the sit-8 producer (`0x4950a`) runs **before** the
entity/clock chain (`0x4951e`) — the sit-8 dispatch and the clock tail are in the
**same frame** (subject to the `0x4950f` `EDX>=4` entity-chain gate, §7 L4).

---

## 2. The invoke-now cadence — same-frame vs deferred

### 2.1 The native install + tick sequence (fresh, exact)

On the frame that selects situation 8, the native order is:

1. `FUN_000493A0` `0x4950a` → `FUN_0004B02C` (the period machine) — the sit-8
   producer (bit-8 `0x4B0CB`/`0x4B0D9` or the `FUN_00088860` phase-`0xC`
   `0x888C8` arm, *cross-cited* FU-208 §4.1/§4.2) → `FUN_0008A938(8, 0, BX=0)`
   → the direct-path hub `0x8AB63` → the table-2 arm `0x8AE08`.
2. The sit-8 arm reaches `0x8AEAE MOV EBX,1` / `0x8AEB3 MOV EAX,7` /
   `0x8AEB8 XOR EDX,EDX` / `0x8AEBA CALL 0x888FC`.
3. `FUN_000888FC`: **if** `[0x158828] != 7` (`0x88908` latch clear), it stores
   `[0x158828]=7`, `[0x158829]=0`, `[0x158818]=0`, `[0x158808]=0x89868`, and
   **because `BX=1`** (`0x8892E`/`0x88931`) and `EAX=0x89868 != 0` (`0x88933`/
   `0x88935`), executes `0x88937 CALL dword[0x158808]` → `0x89868`.
4. `0x89868` arm 0 runs **now** (§3): phase-`0x10` write, row-`0x19` install,
   stage→1 — and, being a **fall-through cascade**, continues into arm 1 (and as
   many successive arms as the gates allow) *within the same call*.
5. Later in the **same** frame, `0x4951e CALL 0x4B100` → `FUN_0008AF38` →
   `0x8B669` runs `CALL dword[0x158808]` again — a *second* call of the act-7
   handler on the sit-8 frame, at whatever stage the cascade reached.

So the arm-0 body is invoked **the same frame as the sit-8 dispatch**, and the
frame's clock tail invokes the handler a second time. The `0x8B669` tail is the
**deferred** route: an act installed with `BX=0` (or a stale slot from a prior
install/clear) is run only at the tail, on the next call.

### 2.2 The act-change latch (the one caveat)

`0x88908 JZ 0x8893D` skips **both** the install and the invoke when the requested
act equals the current act id. Act 7 is produced only by situation 8, and the
engine's sole live sit-8 source is the mode-0 period-end selector — so on the
transition *into* act 7 the latch is clear and the invoke-now fires. If sit 8
re-dispatched while `[0x158828]` were already `7`, the whole seam would be a
no-op (no re-install, no re-invoke). Recorded as a residual leg (L2).

### 2.3 The port

* `fifa96_match_run_situation_side` (`fifa96_match_run.c:2847`) reaches act 7 via
  `fifa96_action_phase_situation(8)` = `{NONE, 7, 0}` (`fifa96_action_handlers.c:1142`)
  and dispatches with
  ```
  2889  fifa96_match_act_dispatch(mr, out.act, out.stage,
  2890                               out.act == 2u ? 1 : 0);
  ```
  For act 7 this passes **`invoke = 0`** — the deferred path.
* `fifa96_match_act_dispatch` (`fifa96_match_act.c:38`) mirrors the seam: it
  latches `a->id`/`timer`/`stage` and, with `invoke == 0`, **returns without
  dispatching** (`match_act.c:49 if (!invoke) return FIFA96_OK;`). The body then
  runs at the clock tail `fifa96_match_act_tick` (`match_act.c:55`, the engine's
  `0x8B669` mirror).
* The engine's own frame structure is `fifa96_match_run_frame`
  (`fifa96_match_run.c:3975`). It calls `fifa96_match_run_phase_drive` (`:4092`,
  which holds the mode-0 sit-8 producer at `:2653` → `fifa96_match_run_period_setup(mr, 0)`)
  **before** `fifa96_match_act_tick` (`:4114`). So the deferred arm-0 body still
  executes in the **same** `fifa96_match_run_frame` as the sit-8 dispatch — but
  at the tick tail (after `match_run_goal_scan` and `match_run_ball_machine`),
  not inline at the dispatch.

### 2.4 Verdict (vs the FU208-L4 claim)

| axis | native | port |
|---|---|---|
| invoke flag for act 7 | `BX=1` (`0x8AEAE`) | `invoke = 0` (`fifa96_match_run.c:2890`) |
| body timing | **inline at the dispatch** (`0x88937`) | at the clock-tail tick (`:4114`) |
| body shape | fall-through cascade (arm 0 → arm 1 → …) | `switch` — **one arm per call** (`match_run.c:6085`) |
| arm-0 install frame | sit-8 frame F | sit-8 frame F (via the same-tick tail) |
| arm-1..N progression | some advance in frame F | first advance in frame F+1 |

**The port does not match the native invoke-now cadence.** FU208-L4's "native
invokes act 7 now; the engine defers" is **confirmed** and still open. The
observable consequence is a **one-granted-frame lag** in the act-7 arm
progression (the native's invoke-now runs the arm-0 cascade and the same frame's
clock tail advances it, whereas the engine runs arm 0 at the tick and arm 1 at
the next frame's tick). On the live path the arm-0 *install* itself lands on the
sit-8 frame, so the divergence is the arm lag (and intra-frame order), not the
install frame.

---

## 3. The downstream installs — every store in the arm-0 region `0x898B4..0x899ED`

Fresh `disassemble_bytes 0x898B4` (320 B, 84 insns, ending inside arm 1 at
`0x899F0`). Every **store** in the region, in order:

| # | addr | bytes | store | port |
|---|---|---|---|---|
| 1 | `0x898BB` | `880d2a881500` | `MOV byte[0x15882A],CL` — clear `[0x15882A]` (CL=0 from `0x898B4 XOR CL,CL`) | ✅ `mr->global_5882a = 0` (`match_run.c:6087`) |
| 2 | `0x898C4` | `880db17a1500` | `MOV byte[0x157AB1],CL` — clear `[0x157AB1]` (unmodelled process flag) | ❌ leg (no cell; FU-89 §11/FU-189 §3) |
| 3 | `0x898DD` | `e8bea7feff` | `CALL 0x740A0` — **phase-`0x10` writer**, gated `0x898CA CMP EAX,0x10 / 0x898CD JZ 0x898E2` (EAX = `[0x157A4A]>>24` = byte `[0x157A4D]`; EDX = `[0x157AAC]>>24` = byte `[0x157AAF]`) | ✅ `if (state.phase != 0x10) match_run_write_phase(mr, 0x10)` (`match_run.c:6090-6092`) |
| 4 | `0x89903` | `c6859b00000000` | `MOV byte[EBP+0x9B],0` — `records[0].+0x9B = 0` (EBP=0x1588A4) | ⚠️ not written; per-record byte not modelled (leg L3) |
| 5 | `0x898F3`+`0x8990A` | `ba19000000` / `e89540ffff` | `EDX=0x19` + `CALL 0x7D9A4` — **row-`0x19` install** on `records[0]` (per team) | ✅ `fifa96_match_entities_install(records[0], phase, 0x19u, 0)` (`match_run.c:6098-6100`) |
| 6 | `0x8990F`+`0x89923` | `ba26000000` / `e8704cfeff` | anim `0x26` on `records[0]` (`CALL 0x6E598`) | ❌ leg (anim sink) |
| 7 | `0x8992A` | `c6864d01000000` | `MOV byte[ESI+0x14D],0` — `records[i].+0x9B = 0` | ⚠️ as #4 |
| 8 | `0x89931` | `8a864c010000` | `MOV AL,byte[ESI+0x14C]` — read `records[i].+0x9A` (the loop gate) | ✅ implied by the install gate (loop only installs on `+0x9A==0`; see FU-209 §4 indexing) |
| 9 | `0x89941`+`0x8994C` | `ba03000000` / `e85340ffff` | `EDX=3` + `CALL 0x7D9A4` — record-loop install (code 3 → `0x19` rewrite) on `records[1..10]` | ⚠️ leg (FU194-L7: the loop's row-3 install is not ported) |
| 10 | `0x89960` | `e8334cfeff` | anim `0` on `records[i]` (`CALL 0x6E598`) | ❌ leg (anim sink) |
| 11 | `0x8999C` | `8990b2070000` | `MOV [EAX+0x7B2],EDX` — `[actor-team+0x7B2] = actor rec` | ❌ leg (no per-team `+0x7B2` active-record cell) |
| 12 | `0x899A2`+`0x899AB` | `ba20000000` / `e8f43fffff` | `EDX=0x20` + `CALL 0x7D9A4` — **row-`0x20` install** on the actor rec `[ESP+8]=[0x1587D4]` | ✅ gated `fifa96_match_entities_install(actor, phase, 0x20u, 0)` on `mr->actor_1587d4` (`match_run.c:6110-6117`) |
| 13 | `0x899BE` | `8990b2070000` | `MOV [EAX+0x7B2],EDX` — `[ball-team+0x7B2] = ball rec` | ❌ leg |
| 14 | `0x899C4`+`0x899CD` | `ba1f000000` / `e8d23fffff` | `EDX=0x1F` + `CALL 0x7D9A4` — **row-`0x1F` install** on the ball rec `[ESP+0xC]` | ❌ leg (FU209-L2/FU194-L7) |
| 15 | `0x899D2` | `e85106ffff` | `CALL 0x7A028` — referee/incident block reset | ❌ leg (FU194-L7) |
| 16 | `0x899E1` | `66893d18881500` | `MOV word[0x158818],DI` — timer `= 0` (DI=0 from `0x899DD XOR EDI,EDI`) | ✅ `mr->act.timer = 0` (`match_run.c:6119`) |
| 17 | `0x899E8` | `882d29881500` | `MOV byte[0x158829],CH` — **stage := stage+1** (`0x899D7 MOV CH,[0x158829]; 0x899DF INC CH`) | ✅ `mr->act.stage = 1` (`match_run.c:6120`) |

**Row-`0x20` "gated"?** In the native, `0x899A2` is **unconditional** in the body
(no branch guards it); the effective gate is (a) the installer `0x7D9A4`'s own
`rec==0 -> ret` / `byte[rec+0x9A]!=0 -> ret` / `code==byte[rec+0x91]->ret`
contract (*cross-cited* FU-209 §3.1), and (b) the target cell `[0x1587D4]`
(produced by the sit-8 arm's `0x8AE4B` bind, *cross-cited* FU-208 §3). The
**port** materialises the gate explicitly: `if (mr->actor_1587d4 >= 0 && …)`
(`match_run.c:6110`). On the live path `mr->actor_1587d4 == FIFA96_MATCH_ENTITY_NONE`
(reset `match_run.c:2036/2156`), so the install is a null no-op — the FU210-L5
leg (no live producer for `[0x1587D4]`; the sit-8 bind `0x8AE4B` is FU210-L3).

**Port match summary.** The port matches the phase-`0x10` write (#3), the
`[0x15882A]` clear (#1), the row-`0x19` base install (#5), the timer/stage writes
(#16/#17), and the gated row-`0x20` installer (#12). It **legs**: the
`[0x157AB1]` clear (#2), the per-record `+0x9B` clears (#4/#7), the loop row-3
install (#9), the three anim sinks (#6/#8/#10), the two `+0x7B2` active-record
writes (#11/#13), the ball row-`0x1F` install (#14) and the `0x7A028` reset
(#15). This refines FU-205 §3(c) / FU-209 §6.4.

Fresh call-byte recomputes: `0x8990A e89540ffff → 0x7D9A4`; `0x8994C e85340ffff
→ 0x7D9A4`; `0x899AB e8f43fffff → 0x7D9A4`; `0x899CD e8d23fffff → 0x7D9A4`;
`0x899D2 e85106ffff → 0x7A028`; `0x898DD e8bea7feff → 0x740A0`; `0x89923
e8704cfeff → 0x6E598`; `0x89960 e8334cfeff → 0x6E598` — all agree with Ghidra.

---

## 4. Row `0x19` — what it is and whether it is wired

* **The slot.** Fresh `read_memory 0x1106E0` (140 B), the action table at
  `0x1106E0`; index `0x19` at byte offset `0x64` = `e4 46 07 00` =
  **`0x000746E4`** (the same read also yields `[0x1F] = 0x00076380` at `0x7C` and
  `[0x20] = 0x00084EEC` at `0x80`, byte-identical to FU-209 §3/§4). So **row
  `0x19` = the body `FUN_000746E4`**.
* **The body.** Fresh `disassemble_bytes 0x746E4` (32 B): `PUSH EBX/ECX/EDX/ESI/
  EDI/EBP ; SUB ESP,0x4C ; MOV ECX,EAX ; MOV EDX,0x9F0 ; MOV EBX,0xAE0 ; MOV
  byte[EAX+0x9E],1 ; MOV AH,1 ; MOV ESI,0xC0` (bytes `53 51 52 56 57 55 83ec4c
  89c1 baf0090000 bbe00a0000 c6809e00000001 b401 bec0000000`) — a real
  6-register frame body (the keeper-hold machine, `0x746E4..0x74CB7`).
* **Install sites.** Row `0x19` is installed by: the arm-0 base install
  (`0x8990A`, per team on `records[0]`), the arm-0 record loop (`0x8994C`, code
  3 → `0x19` rewrite on `records[1..10]`), the row-`0x1F` dive chain
  (*cross-cited* the engine note `handlers.c:5148`) and the row-`0x04`
  carrier/forced/chase installs (`0x7E866`/`0x7E8C1`, *cross-cited*
  `handlers.c:5135`).
* **Wired in the port?** **Yes.** `fifa96_match_handlers.c:5135`:
  `{0x19, fifa96_match_action_19, "FU-179 §3/§6 rank 1 (phase-15 T2): row 19
  keeper-hold body ported (0x746E4..0x74CB7) …"}`. The body is wired and in the
  counted set (`docs/ENGINE.md:8`; one of the 32 action rows of `53/80 = 66%`).
  So the phase-`0x1D` arm-0 row-`0x19` *install* lands on the wired body — no
  `-UNSUPPORTED` on that path.

`0x19` is **not** a phase row: the *phase* table's `0x19` slot is a separate
`0x110794[0x19] = 0x89FA4` (*cross-cited* FU-209 §1.1), a different function. The
two tables are kept distinct (the "name collision" note of FU-203 §2.4).

---

## 5. The exit — how act 7 retires and what re-installs the next act

### 5.1 Arm 9, the terminal — fresh `disassemble_bytes 0x89E6E` (60 B)

```
0x89E6E CALL 0x4BEC8                 ; (e85520fcff) the wait predicate
0x89E73 TEST AX,AX
0x89E76 JNZ 0x89EA6                  ; (752e) predicate != 0 -> epilogue (wait)
0x89E78 MOV DH,1                     ; (b601)
0x89E7A MOV word[0x158818],AX        ; (66a318881500) timer := 0 (AX=0)
0x89E80 XOR BL,BL / 0x89E82 XOR CL,CL / 0x89E8A XOR EBX,EBX / 0x89E98 XOR BH,BH
0x89E84 MOV byte[0x158828],BL        ; (881d28881500) act id := 0
0x89E8C MOV byte[0x157ac0],DH        ; (8835c07a1500) <<< EXTRA-TIME FLAG := 1
0x89E92 MOV dword[0x158808],EBX      ; (891d08881500) act slot := 0
0x89E9A MOV byte[0x157a4c],CL        ; (880d4c7a1500) goal-side flag := 0
0x89EA0 MOV byte[0x158829],BH        ; (883d29881500) stage := 0
0x89EA6 ADD ESP,0x14 ; POP EBP/EDI/ESI/EDX/ECX/EBX ; RET
```

**Retirement is a pure termination.** Act 7's terminal clears its own act slot
(`[0x158828]=0`, `[0x158808]=0`, `[0x158829]=0`, `[0x158818]=0`), clears the
goal-side flag `[0x157A4C]`, and **sets the extra-time flag `[0x157AC0]=1`**
(`0x89E8C`) — its only act-7 write-back. There is **no** situation re-dispatch
(unlike act 8 `0x8A798` and act 0xA `0x8B688`, which re-dispatch a stored
situation). After the slot clear, the clock tail `0x8B669` no-ops
(`CMP dword[0x158808],0 / JZ`).

**What re-installs the next act?** Nothing inside act 7. The next act is
installed by whichever producer *next* calls the situation dispatcher
`FUN_0008A938` (→ the seam `FUN_000888FC`) — the period machine `FUN_0004B02C`,
the goal/clock chain, or a per-record handler. The engine mirrors this:
`fifa96_match_action_phase_1D` arm 9 (`match_run.c:6131-6139`) clears
`mr->act.id/stage/timer` and `mr->global_157a4c`; `fifa96_match_act_tick`
subsequently no-ops (`match_act.c:60 if (a->id == 0) return FIFA96_OK;`). The
native `[0x157AC0]=1` write-back (`0x89E8C`) is still **not** mirrored — the
FU209-L5 leg (the cell `mr->global_157ac0` exists, FU-213 T1; the arm-9 write
does not).

---

## 6. Engine extension point and M2 tape reachability

### 6.1 The exact gap

The cadence gap is **one expression**: `fifa96_match_run.c:2889-2890`

```
rc = (fifa96_err_t)fifa96_match_act_dispatch(mr, out.act, out.stage,
                                             out.act == 2u ? 1 : 0);
```

To match the native invoke-now, the sit-8/act-7 route must pass `invoke = 1`
(as act 2 does), so `fifa96_match_act_dispatch` runs arm 0 at the dispatch
(`match_act.c:52`), and the same-frame `fifa96_match_act_tick` then advances arm
1 — restoring the native two-calls-in-frame-F cadence. No other engine site is
involved: the seam (`match_act.c:38-52`) and the tick (`match_act.c:55-66`)
already implement the `BX=1`/`BX=0` split correctly. This is the **FU208-L4**
leg; it is unconditional on the tape (below).

Adjacent landings already done: phase `0x1D` is registered (`handlers.c:5236`),
the row-`0x19` and phase-`0x10` writes are in place (§3), and the gated
row-`0x20` installer is ported (§3 #12). The remaining arm-0 legs (§3 #2/#4/#7/#9
/#11/#13/#14/#15) and the actor-cell producer (FU210-L3) are orthogonal.

### 6.2 Tape reachability — **dormant**

* The current M2 golden is `sha256 = 441415fc…` (fresh `sha256sum
  tests/golden/engine/m2-frames.txt`), byte-identical to the FU-208 §Landing
  pin; M1 `09b726b7…` (immutable) is unmoved. FU-222 §0 confirms `53/80 = 66%`
  and the byte-identical M2 golden.
* The act-7 route is entered only through **situation 8**, whose two native
  producers are both inert in the port: the bit-8 disjunct (`0x4B0CB`) has no
  located writer (FU208-L1) and the `[0x157AC2] == 4` disjunct needs a period
  completion reaching index 4 (FU-208 §Landing / FU-213 §2). The M2 tape reaches
  no such completion (the sit-8 tail is tape-dormant: FU-206 §5 PR-7/PR-8, FU-214
  §C-1/C-2, FU-222 §0).
* Therefore the cadence divergence is **not observable on M2** (nothing
  installs act 7), and the FU208-L4 fix would move no presented frame. It is a
  correctness/faithfulness gap, not a tape-visible one — consistent with the
  FU-214 "visible only if fired" verdict for the sit-8 cohort.

---

## 7. Numbered legs

1. **FU219-L1 — the invoke-now cadence (FU208-L4) is unported.** Native sit 8
   invokes act 7 now (`0x8AEAE MOV EBX,1`; seam `0x8892E`/`0x88937`); the port
   passes `invoke = 0` (`fifa96_match_run.c:2890`). The fix is that one
   expression (act 7 + invoke-now). Tape-dormant (§6.2). Owner: FU208-L4.
2. **FU219-L2 — the seam act-change latch `0x88908`.** The seam skips install
   *and* invoke when `[0x158828] == act`. The port mirrors it (`match_act.c:45 if
   (a->id == act) return FIFA96_OK;`). Not re-derived here; no consequence for
   the transition into act 7 (the latch is clear), but a re-dispatch of sit 8
   while `[0x158828]==7` is a no-op natively — unproven whether any producer can
   do so. Owner: FU208/FU219.
3. **FU219-L3 — the arm-0 per-record byte writes are unmodelled.** The native
   clears `records[i].+0x9B` (`0x89903`/`0x8992A`), reads `records[i].+0x9A`
   (`0x89931`), runs the loop row-3 install (`0x8994C`) and anims `0x26`/`0`
   (sinks). The port models only the `+0x9A==0` gate implicitly and the base
   row-`0x19` install; the `+0x9B`/anim/row-3 loop is a leg. Owner: FU194-L7.
4. **FU219-L4 — the native frame-boundary gate `0x4950F`.** `FUN_000493A0` gates
   the entity/clock chain (`0x4951E CALL 0x4B100`) on `EDX >= 4`, and the
   sit-8 producer (`0x4950A CALL 0x4B02C`) runs before it; the producer of the
   `EDX` tested at `0x4950F` is **not** derived here, so "the clock tail runs in
   the same frame as the sit-8 dispatch" rests on this gate being satisfied. The
   engine's own order is proven same-frame (§2.3); the native pair is same-frame
   *given* `EDX>=4`. Owner: FU-141/FU219. (Does not affect the §2 invoke-now
   verdict, which rests on the seam bytes alone.)
5. **FU219-L5 — the arm-9 `[0x157AC0]=1` write-back (FU209-L5).** The act-7
   terminal sets the extra-time flag (`0x89E8C`); the ported arm 9 omits it
   (`match_run.c:6131-6139`). Unchanged. Owner: FU209-L5.
6. **FU219-L6 — the fall-through vs `switch` body shape.** The native body is a
   cascade (arm 0 → arm 1 → … in one call); the ported body is a one-arm `switch`
   (`match_run.c:6085`). Combined with L1 this is the full cadence divergence;
   fixing L1 restores the native two-calls-per-frame behaviour under the port's
   one-arm-per-call model, but the cascade's *within-call* multi-arm advance is
   still not modelled. Unproven whether any tape/fixture exercises a multi-arm
   advance. Owner: FU219/FU194.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base
  `0`, min `0x10000`, max `.image::00174a2a`, 2706 functions.
* `read_memory`: `0x1106E0` (140 B, the action table — `[0x19]=0x746E4`,
  `[0x1F]=0x76380`, `[0x20]=0x84EEC`), `0x1107EC` (40 B, the act table —
  `[7]=0x89868`).
* `disassemble_bytes` (start, length B): `0x88920` (32, the seam tail bytes),
  `0x8AE08` (200, the sit-8 arm), `0x898B4` (320, the arm-0 region),
  `0x8B650` (64, the clock tail `0x8B669`), `0x89E6E` (60, arm 9),
  `0x89EA6` (32, the epilogue), `0x746E4` (32, the row-`0x19` body head).
* `disassemble_function`: `0x888FC` (20 insns, the act seam), `0x493A0` (130
  insns, the frame driver `FUN_000493a0`).
* `get_function_by_address`: `0x8AF38` (`FUN_0008af38`, body
  `0x8AF38..0x8B687` — carries the clock tail), `0x4B02C` (`FUN_0004b02c`, body
  `0x4B02C..0x4B0FE`).
* `get_xrefs_to`: `0x4B02C` (**2**, both `FUN_000493a0` `0x49415`/`0x4950a`),
  `0x4B100` (**3**, incl. `FUN_000493a0` `0x4951e`).
* `search_instructions` (`CALL 0x4B02C` within `FUN_0008af38`, 475 insns) =
  **0 matches** (the clock body does not call the period machine).

Engine/loader source read first-hand:

* `src/fifa96_engine/fifa96_match_run.c` — `:2036`/`:2156` `actor_1587d4 = NONE`;
  `:2653` the mode-0 `period_setup` call; `:2847-2897` `match_run_situation_side`
  (the `:2889-2890` invoke expression); `:3145-3178` `fifa96_match_run_period_setup`
  (the `:3164` `0x888C8` sit-8 arm, the `:3176` bit-8 arm); `:3975`
  `fifa96_match_run_frame`; `:4092` `phase_drive` call; `:4114` `act_tick` call;
  `:6082` `fifa96_match_action_phase_1D` (arm 0 `:6085-6121`; the row-`0x19`
  install `:6098-6100`; the gated row-`0x20` install `:6110-6117`; arm 9
  `:6131-6139`).
* `src/fifa96_engine/fifa96_match_act.c` — `:38-52` `fifa96_match_act_dispatch`
  (`:45` the latch, `:51` `if (!invoke) return`), `:55-66`
  `fifa96_match_act_tick` (`:60` the `id==0` no-op).
* `src/fifa96_engine/fifa96_match_handlers.c` — `:5135` row `0x19`
  (`fifa96_match_action_19`), `:5149` row `0x20` `NULL`, `:5236` phase `0x1D`
  (`fifa96_match_action_phase_1D`).
* `src/fifa96_loader/fifa96_action_handlers.c` — `:1142` situation 8 =
  `{NONE, 7, 0, OPEN_LEG, 0}`.
* `tests/golden/engine/m2-frames.txt` `sha256 441415fc…`, `m1-frames.txt`
  `sha256 09b726b7…` (fresh `sha256sum`).

Docs cross-cited: FU-89 §11, FU-175 §1/§2, FU-189 §1/§3, FU-194 (§2/§3.2/§6.1/
§6.2), FU-205 (§2/§3(c)/T5.2), FU-206 (§4 Tier-3 #12/§5), FU-208 (§3/§4/§6.2/
Landing), FU-209 (§1/§3/§4/§5/§6.4/L2/L4/L5), FU-210-L3/L5, FU-213 §2, FU-214
§C-1/C-2, FU-222 §0.

No writes beyond this slice file; no rename/label/comment/function/script/project
save; no git.

---

## Erratum (register — does not rewrite prior sections)

**FU-209 §2.1, arm-0 tail — the row-`0x20` installer is not itself "gated".**
FU-209 §2.1 annotates `0x899A2` as "ROW-`0x20` INSTALL (deferred)". The
*deferred* attribute is right (ECX/EBX=0 at `0x89998`/`0x8999A`), but the "gate"
FU-209 §2.1 and §6.4 attribute to the *actor cell* is not a branch in the native
body: `0x899A2` is unconditional; the gate is the installer's own `rec==0 ->
ret` guard and the target `[0x1587D4]` value. The port makes it an explicit
`if (mr->actor_1587d4 >= 0)` (`match_run.c:6110`) — behaviourally equivalent for
a NULL/0 actor, noted for precision. (§3.)

**FU-208 §6.2(4) / FU-209 §6.4 — the cadence's frame count.** Both describe the
engine's act-7 invoke as "deferred to the clock tail". Fresh first-hand, because
the engine's sit-8 producer path and the tick share one `fifa96_match_run_frame`,
the arm-0 **install** still lands on the sit-8 frame; the divergence is the
*invoke flag*, the *intra-frame position* and the *arm progression lag* (one
granted frame), not the install frame. (§2.4; leg FU219-L1.)
