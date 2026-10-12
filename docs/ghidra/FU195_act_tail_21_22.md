# FU-195 — act-tail cohort 3: phase `0x21`/`0x22` (`0x8B874`/`0x8B900`) + the act-5/`0x1B` blocked verdict

Provenance: M2 phase-17 wave **R43**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every address/byte/xref-count below is a fresh
`disassemble_bytes` / `read_memory` / `get_xrefs_to` / `decompile_function` of
**this session** unless marked *cross-cited* (then the frozen FU doc is named).
No writes: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; no git commands. Engine source read first-hand
(`fifa96_match_act.{c,h}`, `fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_loader/fifa96_action_handlers.{c,h}`). The alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).

Scope (R43 brief): derive the two act-tail handlers **phase `0x21` (`0x8B874`,
act 0xB)** and **phase `0x22` (`0x8B900`, act 0xC)** — bodies + producers — and
**re-verify first-hand** the FU175-L2 / FU-190 §5 claim that act **5** / phase
`0x1B` has **no** static `CALL 0x888FC` passing `EAX=5` (a fresh 11-xref census).
Bounded: if `0x1B` has no invoker it stays a numbered leg — no inventer is
invented.

---

## 1. Fresh act-table / phase-table confirmation (the dual identity)

Fresh `read_memory 0x1107EC` (64 B), decoded:

```
act 0..4  : 00000000 c88d0800 2c920800 a49f0800 20960800
act 5..9  : ec900800 10910800 68980800 98a70800 4c8f0800
act A..C  : 88b60800 74b80800 00b90800
tail      : ff076000 38ce0700 ff074000
```

Fresh `read_memory 0x110794` (140 B = 35 dwords), rows `0x17..0x22`:

```
[17]=088DC8 [18]=08922C [19]=089FA4 [1A]=089620 [1B]=0890EC [1C]=089110
[1D]=089868 [1E]=08A798 [1F]=088F4C [20]=08B688 [21]=08B874 [22]=08B900
```

So `0x1107EC[0xB] = 0x110818 = 0x110794[0x21] = 0x8B874` and
`0x1107EC[0xC] = 0x11081C = 0x110794[0x22] = 0x8B900`. The act-table slot and
the phase-table row for phases `0x17..0x22` are the **same dword** (the base
relation `0x1107EC = 0x110794 + 0x16*4`, *cross-cited* FU-175 §1/FU-190 §1, fresh
bytes agree) — the FU165-L9 **dual identity**. Consequence for `0x21`/`0x22`:
the same body is (a) invoked immediately when `FUN_000888FC` installs act 0xB/0xC
with `BX=1` (invoke-now), and (b) ticked once per granted frame by the clock tail
`0x8B669 CMP dword[0x158808],0 / 0x8B672 CALL dword[0x158808]` (*cross-cited*
FU-175 §2; the tick takes **no record argument** — the two act-tail bodies below
ignore `EAX`/`EDX` completely, unlike the per-record placement rows).

Fresh `get_xrefs_to 0x8B874` = **1** (`0x110818`, DATA — the shared slot) and
fresh `get_xrefs_to 0x8B900` = **1** (`0x11081C`, DATA — the shared slot). Each
body has exactly one reference: the table slot. Nothing else calls either body.

---

## 2. The phase-`0x21` / act-0xB body `0x8B874`

Fresh `disassemble_bytes 0x8B874` (140 B; `0x8B874..0x8B8FC`, 45 insns). This is
a **table-only target** — Ghidra has no function object there
(`decompile_function 0x8B874` -> "No function found"), so the body is derived
from the raw disassembly. The body:

```
0x8B874 PUSH EBX / PUSH ECX / PUSH EDX
0x8B877 MOV AX, [0x00157A64]              ; AX = word[0x157A64] (the frame delta)
0x8B87D MOV DX, [0x00158818]              ; DX = act timer
0x8B884 ADD EDX, EAX                      ; timer += delta
0x8B886 MOV AL, [0x00158829]              ; AL = stage
0x8B88B MOV [0x00158818], DX              ; store timer
0x8B892 TEST AL, AL
0x8B894 JBE 0x8B89E                       ; stage == 0 -> 0x8B89E
0x8B896 CMP AL, 1
0x8B898 JZ  0x8B8C3                       ; stage == 1 -> 0x8B8C3
0x8B89A POP EDX/ECX/EBX; 0x8B89D RET      ; stage > 1 -> no-op

; ---- stage 0 (0x8B89E) ----
0x8B89E MOV EAX, 0x1E
0x8B8A3 CALL 0x000974DC                   ; 0x974DC(0x1E)  [event/sound id 0x1E]
0x8B8A8 MOV DH, [0x00158829]              ; DH = stage
0x8B8B0 INC DH
0x8B8B2 MOV [0x00158818], CX(=0)          ; timer = 0
0x8B8B9 MOV [0x00158829], DH              ; stage = 1
0x8B8C2 RET

; ---- stage 1 (0x8B8C3) ----
0x8B8C3 MOV EAX, [0x00158816]
0x8B8C8 SAR EAX, 0x10                     ; alias trap: = word[0x15881A] (timeline cell)
0x8B8CB CMP EAX, 0x3C
0x8B8CE JL  0x8B8F9                       ; < 0x3C -> return (wait)
0x8B8D0 MOV EAX, 0x1E ; CALL 0x000974DC   ; 0x974DC(0x1E)
0x8B8DA XOR EBX, EBX / XOR EDX, EDX
0x8B8DE MOV [0x00158818], BX(=0)          ; timer = 0
0x8B8E5 XOR AH, AH
0x8B8E7 MOV [0x00158808], EDX(=0)         ; handler = 0  (act slot cleared)
0x8B8ED MOV [0x00158828], AH(=0)          ; act id = 0
0x8B8F3 MOV [0x00158829], AH(=0)          ; stage = 0
0x8B8FC RET
```

Derived semantics: a **two-stage timer**. Each tick adds the frame delta to the
act timer (`word[0x158818] += word[0x157A64]`, identical head to the act-9
`0x88F4C` and phase-0x20 `0x8B688` bodies, *cross-cited* FU-175 §3 / FU-188 §2).
Stage 0: fire the `0x1E` event, reset the timer, advance to stage 1. Stage 1:
when the `word[0x15881A]` gate reaches `0x3C`, fire the `0x1E` event and **clear
the act slot** (handler/id/stage/timer all zero). Stage > 1: no-op. The body
performs **no `FUN_000740A0` phase write** and touches no record — it is a pure
act-slot reset/timer with an audio event. This is the native "extra-time clear"
(no-extra-time period-end) handler chosen by the producer in §4.

`0x974DC` (fresh `decompile_function`): `FUN_00064EC0(); FUN_00065CC0(EAX)`. The
fresh disassembly (`0x974DC`) is `PUSH EDX / MOV EDX,EAX / CALL 0x64EC0 /
MOV EAX,EDX / CALL 0x65CC0 / POP EDX / RET`, i.e. the event id (`0x1E`) is
saved in EDX and forwarded. `FUN_00064EC0` (fresh decompile) pushes onto the
20-slot ring at `0x155C8C` via `FUN_00064EF0(1, EDX)` (fresh decompile: the ring
push); `FUN_00065CC0` (fresh decompile) sets `_DAT_0010A254 = FUN_000A7705(id,
[0x155D30])` when the audio state is live. So `0x974DC(0x1E)` = the audio
event/sound id `0x1E` (30) — an unported audio sink (leg, §8).

---

## 3. The phase-`0x22` / act-0xC body `0x8B900`

Fresh `disassemble_bytes 0x8B900` (120 B; `0x8B900..0x8B977`) plus fresh
`disassemble_bytes 0x8B973` (96 B; `0x8B973..0x8B9C9`) cover the whole body
(`0x8B900..0x8B9C9`, `RET` at `0x8B9C9`). Also a table-only target
(`decompile_function 0x8B900` -> "No function found"). Control flow:

```
0x8B900 PUSH EBX/ECX/EDX/ESI
0x8B904 MOV AX, [0x00157A64]              ; frame delta
0x8B90A MOV DX, [0x00158818]              ; act timer
0x8B911 ADD EDX, EAX ; 0x8B918 store      ; timer += delta
0x8B913 MOV AL, [0x00158829]              ; AL = stage
0x8B91F CMP AL, 1 ; 0x8B921 JC 0x8B92E    ; stage == 0 -> 0x8B92E
0x8B923 JBE 0x8B95C                       ; stage == 1 -> 0x8B95C
0x8B925 CMP AL, 2 ; 0x8B927 JZ 0x8B98F    ; stage == 2 -> 0x8B98F
0x8B929 RET                               ; stage > 2 -> no-op

; ---- stage 0 (0x8B92E) ----
0x8B936 MOV EAX, 0x1E ; 0x8B93B CALL 0x974DC   ; 0x974DC(0x1E)
0x8B93B..0x8B951: timer = 0; stage = 1; RET

; ---- stage 1 (0x8B95C) ----
0x8B95C MOV EAX, [0x00158816] ; SAR EAX,0x10 (= word[0x15881A])
0x8B964 CMP EAX, 0x3C ; JL 0x8B9C5             ; < 0x3C -> return
0x8B969 MOV EAX,0x1E ; CALL 0x974DC            ; 0x974DC(0x1E)
0x8B973..0x8B98E: timer = 0; stage = 2; RET

; ---- stage 2 (0x8B98F) ----
0x8B98F MOV EAX, [0x00158816] ; SAR EAX,0x10 (= word[0x15881A])
0x8B99A CMP EAX, 0x3C ; JL 0x8B9C5             ; < 0x3C -> return
0x8B99C MOV EAX,0x1E ; CALL 0x974DC            ; 0x974DC(0x1E)
0x8B9A6 XOR EBX,EBX / XOR EDX,EDX
0x8B9AA MOV [0x00158818], BX(=0)               ; timer = 0
0x8B9B3 MOV [0x00158808], EDX(=0)              ; handler = 0
0x8B9B9 MOV [0x00158828], AH(=0)               ; act id = 0
0x8B9BF MOV [0x00158829], AH(=0)               ; stage = 0
0x8B9C9 RET
```

Derived semantics: the **three-stage** form of §2 (the native "extra-time set"
period-end handler). Same head (`timer += frame delta`), same `word[0x15881A] >=
0x3C` gate, same `0x974DC(0x1E)` event at each stage advance; stage 2 clears the
act slot. No phase write, no record touch. `0x8B9CA` is a 2-byte `MOV EAX,EAX`
alignment pad before the producer `FUN_0008B9CC` at `0x8B9CC`.

**Both bodies are pure act-slot timers** — the act-tail cohort's "no per-record
effect" shape. Porting either row cannot change a record's state; the only
non-act side effect is the audio event `0x1E` (`0x974DC`, §2).

---

## 4. The producer `FUN_0008B9CC` (`0x8B9CC..0x8BAEF`) and its clock-harness chain

Fresh `get_function_by_address 0x8B9CC` = `FUN_0008b9cc`, body
`0x8B9CC..0x8BAEF`. Fresh `decompile_function 0x8B9CC`:

```c
void FUN_0008b9cc(void) {
  if (DAT_00157ac0 == 0) {                 /* extra-time flag OFF */
    if (3 < DAT_00157ac2) goto LAB_0008bac6;     /* period >= 4 -> no phase */
    uVar2 = 0xc;                                  /* else phase 0xC */
    uVar6 = (ushort)(char)([0x157AAC] >> 0x18);   /* situation/side byte */
  } else { /* extra-time ON -> phase 0x13/0x14 + side 0x157ABE/0x157ABF */ ... }
  FUN_000740a0(uVar2, uVar6);                     /* phase write at 0x8BAC1 */
LAB_0008bac6:
  sVar3 = (DAT_00157ac0 == 0) ? 0xb : 0xc;        /* 0x8BACF/0x8BADB */
  FUN_000888fc(sVar3, 0, 1);                      /* 0x8BAE7 CALL 0x888FC */
  return;
}
```

Fresh `disassemble_bytes 0x8B9CC` (340 B) confirms the tail bytes first-hand:
`0x8BACF MOV EBX,0x1; 0x8BAD4 MOV EAX,0xC; 0x8BAD9 JMP 0x8BAE5` (extra-time set)
else `0x8BADD MOV EBX,0x1; 0x8BAE2 MOV EAX,0xB; 0x8BAE5 XOR EDX,EDX;
0x8BAE7 CALL 0x000888FC` (bytes `e810ceffff`). So the producer invokes **act 0xB
when `[0x157AC0]==0`, act 0xC when `[0x157AC0]!=0`**, with **DL = 0** (stage 0)
and **BX = 1** (invoke-now). The act-0xB/0xC install is the invoke-now path that
runs §2/§3 stage 0 immediately, then the clock tail ticks it.

**Producer chain — `FUN_0008AF38` (the clock harness).** Fresh
`get_xrefs_to 0x8B9CC` = **3**, all inside `FUN_0008AF38` (fresh
`get_function_by_address 0x8AF38` = `0x8AF38..0x8B687`): `0x8B574`, `0x8B5FA`,
`0x8B616`. Fresh `disassemble_bytes 0x8B540` (256 B) shows the three sites on the
period-rollover paths:
* `0x8B574` — reached after `0x8B56F CALL 0x4C374` on the first rollover arm;
  then `0x8B579 MOV BH,[0x157AC2]; 0x8B581 INC BH; 0x8B58A MOV [0x157AC2],BH`
  (the `[0x157AC2]` period increment), gated by the `[0x157AC5]`/`[0x157AC7]`
  score words and `[0x157AC0]`.
* `0x8B5FA` — on the `[0x157AC0]`-set arm (`0x8B5BB MOV EBX,0x20; ...;
  0x8B5D3 MOV [0x157AC0],DL(1)` sets the extra-time flag, then the `0x8B5F5`
  emit and `0x8B5FA CALL 0x8B9CC`).
* `0x8B616` — the `[0x157AC2]==4 && [0x157AC0]!=0` arm, then
  `0x8B61B INC byte [0x157AC2]`.

So `FUN_0008B9CC` is the period-boundary chooser, called from the clock harness
`FUN_0008AF38` (the native the engine's `fifa96_match_run_phase_drive` mirrors,
*cross-cited* FU-143 §4/FU-175 §2). `[0x157AC0]` is the extra-time flag,
`[0x157AC2]` the period counter.

Fresh `get_xrefs_to 0x8AF38` = **2** (`0x4B1A6` in `FUN_0004B100`, `0x7440B`).
The harness runs from the frame update chain (`FUN_0004B100`), so the
period-rollover producer is **live per granted frame** when a completion is
staged.

---

## 5. Fresh re-verification of the act-5 / phase-`0x1B` blocked verdict (FU175-L2)

Fresh `get_xrefs_to 0x888FC` = **11**, all `UNCONDITIONAL_CALL` (no `INDIRECT`,
no `DATA` — so there is **no** stored function pointer and **no** computed
invoker anywhere). The 11 sites, each with the `EAX` set immediately before the
call (fresh disassembly of every site this session):

| # | site | function | EAX | act | source |
|---|---|---|---|---|---|
| 1 | `0x8AB57` | `FUN_0008A938` | 4 (`0x8AB4B MOV EAX,0x4`) | 4 | sit-3 direct arm |
| 2 | `0x8AF1A` | `FUN_0008A938` | 9 (`0x8AF13`) / 8 via `0x8AAA3 JMP` | 9 / 8 | sit-0xC / sit-0·0xB fallback |
| 3 | `0x8BAE7` | `FUN_0008B9CC` | 0xB / 0xC (`0x8BAE2`/`0x8BAD4`) | 0xB/0xC | period-end chooser (§4) |
| 4 | `0x8A6FE` | `FUN_0008A43C` | 3 (`0x8A6F9`) | 3 | |
| 5 | `0x8A789` | `FUN_0008A43C` | 6 (`0x8A784`) | 6 | |
| 6 | `0x8AB8E` | `FUN_0008A938` | 0xA (`0x8AB87`) | 0xA | sit 0 |
| 7 | `0x8ABCF` | `FUN_0008A938` | 1 (`0x8ABC7 MOV EAX,EBX=1`) | 1 | sit 1 |
| 8 | `0x8AEBA` | `FUN_0008A938` | 7 (`0x8AEB3`) | 7 | sit 8 |
| 9 | `0x8AED2` | `FUN_0008A938` | 2 (`0x8AECB`) | 2 | sit 9 |
| 10 | `0x8AEEA` | `FUN_0008A938` | 2 (`0x8AEE3`), EDX=1 | 2 (stage 1) | sit 0xA |
| 11 | `0x88EE8` | act-1 body (`0x88DC8`) | 4 (`0x88EE1`) | 4 | |

Union of statically-invoked acts: **{1,2,3,4,6,7,8,9,0xA,0xB,0xC}**. **No site
sets `EAX=5`.** The sit-0/sit-0xB `BX!=0` fallback is confirmed fresh as act 8:
`0x8AA93 MOV [0x15882C],AL; 0x8AA98 MOV EAX,0x8; 0x8AA9D MOV [0x15882B],CL;
0x8AAA3 JMP 0x8AF1A` (bytes `e972040000`) — the jump lands directly on the
`0x8AF1A CALL`, so that shared tail carries EAX=8 (fallback) or EAX=9
(fall-through from `0x8AF13`).

**Act-5/`0x1B` corroboration.** Fresh `get_xrefs_to 0x890EC` (the act-5 =
phase-0x1B body) = **1** — the phase/act table slot `0x110800`
(`0x110794 + 0x1B*4 = 0x1107EC + 5*4`). Fresh `disassemble_bytes 0x890EC` (80 B)
shows a pure-reset body: `XOR EDX,EDX; XOR AH,AH; MOV [0x158818],DX;
MOV [0x158828],AH; XOR EDX,EDX; MOV [0x158829],AH; MOV [0x158808],EDX; RET`
(id/stage/timer/handler all zeroed, no `FUN_000740A0`). And no `FUN_000740A0`
site writes phase `0x1B` (the written set is `{0,1,2,3,4,5,8,9,0xA,0xB,0xC,0xD,
0xF,0x10,0x11,0x13,0x14,0x15}`, *cross-cited* FU-175 §7B; independently, the act
domain `{0..0xC}` cannot select phase `0x1B` unless a caller passes `EAX=5`),
so the per-record walker cannot reach it either.

**Verdict: the FU175-L2 / FU-190 §5 negative holds.** Act 5 / phase `0x1B` has
**no static invoker** — not among the 11 direct call sites (none passes `EAX=5`),
no indirect/data reference to `0x888FC` exists, and no phase writer emits
`0x1B`. It is **dead** and stays a numbered leg (FU195-L1); no invoker is
invented. This is a **negative census, not a search gap** (all 11 sites re-read
fresh this session).

---

## 6. The engine seam and the T6 wiring recipe

First-hand engine source:

* `fifa96_match_run_phase_drive` (`fifa96_match_run.c:2422`) is the derived
  `FUN_0008B9CC` **mirror**: it evaluates the completion (`clock_period_ended`),
  calls `fifa96_action_phase_period_end(period-1, 0u, ...)` and applies
  `match_run_write_phase(mr, out.phase)`. It does **not** call the act selector —
  the FU-143 comment (line ~2410) says "the native then invokes the act selector,
  whose handler body is unported". **That is the T6 gap.**
* `fifa96_action_phase_period_end` (`fifa96_action_handlers.c:1170`) already
  derives `out.act = 0x0B` (extra-time clear) / `0x0C` (extra-time set) and
  `out.phase` (`0x0C`/`0x13`/`0x14`/NONE); the engine passes `extra_time` as a
  **hardcoded `0u`** (`match_run.c:2441`), so `out.act` is always `0x0B` today.
* `fifa96_match_act_dispatch` (`fifa96_match_act.c:38`) is the `FUN_000888FC`
  mirror; `fifa96_match_dispatch_phase` (`fifa96_match_handlers.c:4700`) resolves
  `fifa96_match_phase_table[phase]`.
* `fifa96_match_phase_table[0x21]` and `[0x22]` are `NULL` today
  (`fifa96_match_handlers.c:4677`/`4678`), decs `"FU-137 §6: FU-136 phase 21/22:
  not ported; FU-83 timeline; OL-13"`.
* The phase-0x20 / act-0xA body `fifa96_match_action_phase_20`
  (`fifa96_match_run.c:5422`) is the **exact template** for a two/three-stage
  act-tail timer body (head `act.timer += frame_delta`, stage dispatch, slot
  clear).

### 6.1 T6 wiring recommendation

1. **Add the act-selector invocation** in `fifa96_match_run_phase_drive`, after
   the phase-write block (the `0x8BAE7 CALL 0x888FC` mirror; BX=1, DL=0):
   ```c
   /* FU-195: FUN_0008B9CC tail 0x8BAE7 calls FUN_000888FC(out.act, 0, 1). */
   rc = fifa96_match_act_dispatch(mr, out.act, 0u, 1);
   if (rc < 0) return rc;
   ```
   `out.act` already carries `0x0B`/`0x0C`.
2. **Port `fifa96_match_action_phase_21`** (act 0xB, `0x8B874`) into
   `fifa96_match_run.c` mirroring `fifa96_match_action_phase_20`; register
   `fifa96_match_phase_table[0x21] = fifa96_match_action_phase_21` and declare it
   in `fifa96_match_run.h`. Derived stages (§2): head `act.timer +=
   frame_delta`; stage 0 -> `0x974DC(0x1E)` event, timer=0, stage=1; stage 1 ->
   if `timer >= 0x3C` event + clear slot (id/stage/timer = 0); else no-op.
3. **Port `fifa96_match_action_phase_22`** (act 0xC, `0x8B900`) as the
   three-stage analogue (stages 0/1/2, `timer >= 0x3C` gates, terminal clear),
   **export it for direct unit testing but leave `fifa96_match_phase_table[0x22]
   = NULL`** — its only producer is the extra-time path, which the engine
   hardwires off (§6 note). This is the FU-185/FU-188 "ported body, no live
   installer" precedent.

**Reachability verdict.** With (1)+(2): the native route producer
`FUN_0008B9CC` -> `0x8BAE7 CALL 0x888FC(0xB,0,1)` is fully mirrored
(`fifa96_match_run_phase_drive` -> `fifa96_action_phase_period_end` -> the new
`fifa96_match_act_dispatch`), so **phase 0x21 / act 0xB is honestly
producer-live** (+1). Phase 0x22 / act 0xC is **producer-dormant** (the extra
time flag `[0x157AC0]` has no engine producer; `phase_drive` passes `0u`), so it
stays a leg (FU195-L2) — the census does not count it.

**Tape expectation.** Both bodies touch only the act slot (`mr->act`) and the
unported audio sink `0x974DC(0x1E)`; the act-tail timer is render-invisible and
the period-end path does not fire a completion in the short golden fixtures. So
T6 should be **tape-neutral** (the phase-0x1F precedent, FU-175 §10) — but the
implementer must `cmp`-verify M1/M2 and re-pin only if a real diff appears.

---

## 7. Honest census

Baseline (verified, *cross-cited* FU-190 §9/AGENTS.md): **34/80 = 42%**
(31 action + phase `0x18`/`0x19`/`0x1F`). The two candidate movers:

| row | producer (native) | engine status | verdict |
|---|---|---|---|
| **phase 0x21** / act 0xB | `0x8BAE7` (extra-time clear) | LIVE once T6 wires the dispatch (`out.act = 0x0B`, the default) | **+1 honest** |
| **phase 0x22** / act 0xC | `0x8BAE7` (extra-time set) | producer-dormant: `phase_drive` passes `extra_time = 0u`; `[0x157AC0]` writer unported | **leg (uncounted)** |

Arithmetic: `floor(100 × 35/80) = floor(43.75) = 43%` for +phase 0x21. If a later
slice also ports the `[0x157AC0]` extra-time producer, `0x22` lands too and the
count is `36/80 = 45%`. **No hand-set number** — the implementer recomputes
`floor(100 × wired/80)` from the landed set (AGENTS.md rule).

---

## 8. Numbered legs

1. **FU195-L1 — act 5 / phase `0x1B` has no invoker (confirmed dead).** Fresh
   11-xref census of `0x888FC`: no site passes `EAX=5`; no indirect/data
   reference to `0x888FC`; no `FUN_000740A0` writer emits phase `0x1B`. The body
   `0x890EC` is referenced only by its table slot `0x110800`. Owner: FU175-L2.
   Do **not** invent an invoker.
2. **FU195-L2 — phase `0x22` / act 0xC producer-dormant.** The native invoker
   exists (`0x8BAE7`, `[0x157AC0] != 0`), but the engine's only period-end caller
   passes `extra_time = 0u` (`fifa96_match_run.c:2441`) and `[0x157AC0]`'s writer
   (`0x8B5D3`, a period-1 completion side effect) is unported. The `0x8B900` body
   is portable but has no live installer until the extra-time flag has a
   producer. Owner: the `[0x157AC0]` producer.
3. **FU195-L3 — the `0x974DC(0x1E)` audio sink.** Both bodies' only non-act side
   effect is the `0x1E` event (`FUN_00064EC0` ring push `FUN_00064EF0(1)` +
   `FUN_00065CC0` audio handle set). Unported; staged as a no-op in the derived
   bodies (the FU-175 §10 sink precedent). Whether the ring push is tape-visible
   is unverified — assume not until a `cmp` proves otherwise.
4. **FU195-L4 — the `word[0x15881A]` gate.** The native stage-1/2 waits gate on
   `word[0x15881A] >= 0x3C` (alias trap: `[0x158816]>>16`), the same timeline
   cell the act-9 stage-3 gate reads (*cross-cited* FU175-L5). The derived bodies
   substitute the act timer (`timer >= 0x3C`, ~30 granted frames at
   `frame_delta = 2`); the cell's producer is unported. Same divergence class as
   phase 0x1F/0x20 (FU175-L5/FU188-L2).
5. **FU195-L5 — the act-table extent bound.** `0x1107EC[0xB]/[0xC]` are the last
   two live act slots; the dwords past `[0xC]` (`0x006007FF`, `0x0007CE38`,
   `0x004007FF`, fresh §1) are unrelated. `fifa96_match_act_dispatch` already
   bounds `act <= 0xC` (FU175-L6).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `read_memory` — `0x1107EC` (64 B), `0x110794` (140 B), `0x157AB0` (32 B),
  `0x157A60` (16 B).
* `get_xrefs_to` — `0x888FC` (11), `0x8B874` (1), `0x8B900` (1), `0x8B9CC` (3),
  `0x8AF38` (2), `0x890EC` (1), `0x974DC` (42).
* `disassemble_bytes` — `0x8B874` (140 B), `0x8B900` (120 B), `0x8B973` (96 B),
  `0x8B9CC` (340 B), `0x8B540` (256 B), `0x8AB40` (160 B), `0x8AE90` (112 B),
  `0x8AF00` (48 B), `0x8A6E0` (48 B), `0x8A770` (48 B), `0x88ED0` (48 B),
  `0x8AA90` (32 B), `0x890EC` (80 B), `0x974DC` (48 B).
* `decompile_function` — `0x8B9CC`, `0x888FC`, `0x974DC`, `0x64EC0`, `0x65CC0`,
  `0x64EF0`; `decompile_function 0x8B874`/`0x8B900` -> no function (table-only
  targets).
* `get_function_by_address` — `0x8B9CC`, `0x8AF38`.

Engine/loader source read first-hand: `include/fifa96_engine/fifa96_match_act.h`
(whole), `src/fifa96_engine/fifa96_match_act.c` (whole),
`src/fifa96_engine/fifa96_match_handlers.c` (4590..4705, the two tables + both
dispatch resolvers), `src/fifa96_engine/fifa96_match_run.c` (2384..2451,
2620..2680, 3725..3775, 5407..5445, 5470..5500),
`include/fifa96_loader/fifa96_action_handlers.h` (636..672),
`src/fifa96_loader/fifa96_action_handlers.c` (1090..1215). Docs cross-cited:
FU-137 (§6, OL-13), FU-143 (§4), FU-175 (§1/§2/§3/§7/§10, FU175-L2/L5/L6),
FU-182 (§4), FU-185 (§2/§10), FU-188 (§2/§6), FU-190 (§1/§4/§5/§9, FU190-L6);
the phase-16/17 plans; AGENTS.md census rule. No writes beyond this slice file
and the workspace report.
