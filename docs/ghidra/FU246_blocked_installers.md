# FU-246 — the blocked-installer + no-producer cluster: action `03` (tape-visible), action `17` (permutation residual), phase `0x1A`/`0x1B`/`0x1C` (act-4/5/6 producers)

Provenance: M2 phase-23 wave **W23-7**, 2026-10-14, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` this session = `watcom:LE:32:default`,
`function_count` 2706, `symbol_count` 15239, `min_address` `0x10000`). Every
address/byte/xref below is a fresh `read_memory` / `disassemble_bytes` /
`decompile_function` / `get_xrefs_to` / `search_instructions` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). The
addresses quoted are the `/FIFA96.EXE` program addresses (the runtime ones).
The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at
`x+3`; verify CALL rel32; widths/inverted gates). No writes: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git
commands. Engine source read first-hand
(`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_match_act.c`, `include/fifa96_engine/*`, and the loader
`fifa96_action_handlers.c`).

Scope: the five rows the brief names — action **`03`** (tape-visible
blocked-installer), action **`17`** (permutation-residual installer), and the
phase **`0x1A`/`0x1B`/`0x1C`** act-4/5/6 producers — derived first-hand, then
the strict **wire-vs-ceiling** verdict per row under the AGENTS.md census rule
(a row counts only when a ported path installs **and** dispatches live).

---

## 1. Fresh table anchors + identity (all this session)

**Action table `0x1106E0` (fresh `read_memory`, 128 B).** dwords `[row] =
0x1106E0 + row*4`:

```
[0]=0007DB10 [1]=0007DBC0 [2]=0007DFCC [3]=0007E1A4 [4]=0007E7C8 [5]=0007F194
[6]=000801B4 [7]=000814B0 [8]=00081068 [9]=00080A00 [A]=00081738 [B]=00081908
[C]=00081C90 [D]=0008251C [E]=00082710 [F]=00082AD0 [10]=000855F0 [11]=00085DE4
[12]=00083D68 [13]=00084B00 [14]=0008784C [15]=00087CD0 [16]=00084630 [17]=00084730
[18]=000849B0 [19]=000746E4 [1A]=0007662C [1B]=00076D28 [1C]=00077728 [1D]=00074EB0
[1E]=0007550C [1F]=00076380
```

So **action row `03` = `0x7E1A4`** and **action row `17` = `0x84730`** — matches
FU-138 §1 / FU-181 §1 / FU-201 §1 byte-for-byte.

**Phase table `0x110794` (fresh `read_memory`, 140 B).** dwords `[phase] =
0x110794 + phase*4`; the rows in scope:

```
[02]=0006DCC8  [17]=00088DC8  [1A]=00089620  [1B]=000890EC  [1C]=00089110
```

**Act table `0x1107EC` (fresh `read_memory`, 64 B)** = `0x110794 + 0x16*4`:

```
[0]=00000000 [1]=00088DC8 [2]=0008922C [3]=00089FA4 [4]=00089620 [5]=000890EC
[6]=00089110 [7]=00089868 [8]=0008A798 [9]=00088F4C [A]=0008B688 [B]=0008B874 [C]=0008B900
```

So act `a` → phase `0x16+a`: **act 4 = phase `0x1A` = `0x89620`**, **act 5 =
phase `0x1B` = `0x890EC`**, **act 6 = phase `0x1C` = `0x89110`**. Byte-identical
to FU-193 §1 / FU-201 §1.

**The `[rec+0x1C]` staging (fresh).** `FUN_0006D920` `0x6D9A8..0x6D9BA`:

```
0x6D9A8 MOV EAX,[0x157A4A] ; 0x6D9AD SAR EAX,0x18 ; 0x6D9B0 SHL EAX,2
0x6D9B3 ADD EAX,0x110794   ; 0x6D9B8 MOV EAX,[EAX] ; 0x6D9BA MOV [EDX+0x1C],EAX
```

i.e. **`[rec+0x1C] = 0x110794[[0x157A4A]>>24]`** (the phase-handler pointer).
Fresh `get_xrefs_to 0x6D920` = **4** (`0x79983`, `0x6DA0A`, **`0x8D107`** the
native walk `FUN_0008D098`, `0x8CF73`) — the walk stages it per record.

**Engine census this session:** `docs/ENGINE.md:8` states **55/80 dispatch rows
wired (33 action + 22 phase)**; the five rows below are all `fn == NULL` in the
two handler tables (`fifa96_match_handlers.c:5104/5142/5241/5243/5245`,
`fifa96_match_handlers.c:5188..5259`). So this cluster's honest ceiling is
**0 new wired rows → 55/80 = 68% unchanged** unless a producer proves live.

---

## 2. Action `03` — the live phase-2 hot path, the frame-29 divergence, and the wire verdict

### 2.1 The row reference is the action table only (fresh)

Fresh `get_xrefs_to 0x7E1A4` = **1**, a `DATA` ref from `0x1106EC` (= `0x1106E0
+ 3*4`, the row-03 slot). Row `03` is reached only through
`fifa96_match_dispatch_action`. The action-table evidence string
(`fifa96_match_handlers.c:5105`) already records the leg posture.

### 2.2 The row-03 head (fresh `disassemble_bytes 0x7E1A4`, 128 B)

```
0x7E1A4 prologue (push x6, SUB ESP,8) ; 0x7E1AD ESI=EAX(rec)
0x7E1AF MOV byte[EAX+0x9E],1                 ; ran = 1
0x7E1B6 EAX=[rec] ; EDX=[EAX+0x7BA] ; CMP ESI,EDX ; JNZ 0x7E1CE
0x7E1C4 CALL 0x79B58 ; JMP 0x7E51B           ; guard A: rec == [team+0x7BA]
0x7E1CE MOV EAX,[0x157A4A] ; SAR EAX,0x18     ; the GLOBAL phase byte
0x7E1D6 CMP EAX,0x13 ; JZ 0x7E1E0
0x7E1DB CMP EAX,0x14 ; JNZ 0x7E224            ; else -> slot gate
0x7E1E0..0x7E215 : the phase-0x13/0x14 special arm
   MOV [ESI+0x55],0 ; EAX=[ESI+0x55] ; LEA EDX,[ESI+0x4D] ; [ESI+0x51]=EAX
   LEA EAX,[ESI+0x59] ; MOV [ESI+0x4D],0x780 ; CALL 0x8DCD4
   EAX=[ESP]>>16 ; CMP EAX,0x30 ; JGE 0x7E213 ; MOV [ESI+0x4D],0xAE0
   CALL 0x79B58 ; RET
```

### 2.3 The `[rec+0x1C]` call sites (fresh)

The row has **two** live `CALL dword ptr [ESI+0x1C]` sites, both `EAX=rec`,
`EDX=&rec[0x4D]`, `EBX=-1`:

* `0x7E248..0x7E252` (slot branch A): `LEA EDX,[ESI+0x4D] ; MOV EBX,-1 ; MOV
  EAX,ESI ; 0x7E252 CALL dword ptr [ESI+0x1C]` (fresh 48 B from `0x7E244`).
* `0x7E2D3..0x7E2DD` (idle core, fresh 64 B from `0x7E2D0`): `LEA
  EDX,[ESI+0x4D] ; MOV EBX,0xFFFFFFFF ; MOV EAX,ESI ; 0x7E2DD CALL dword ptr
  [ESI+0x1C]`, then `0x7E2E0 EAX=[ESI+0x63]>>22 (SAR 0x16) ; INC EAX ; DX=
  word[ESI+0x79] ; word[ESI+0x7B]=AX ; if AX>DX word[ESI+0x7B]=DX` (the placement
  counter), then `0x7E2F8 EAX=[0x157A4A]>>24 ; CMP EAX,2 ; JNZ 0x7E3A6`.

On the forced-decision walk the record is in phase 2, so `0x7E2DD` invokes
`0x110794[2] = 0x6DCC8`.

### 2.4 `0x6DCC8` rewrites the target triple (fresh `decompile_function 0x6DCC8`)

```
void FUN_0006dcc8(int *param_1,int *param_2,short param_3) {  // param_1=rec, param_2=&rec[0x4D]
  if (param_3 < 0) iVar5 = *(int*)(&DAT_001577de + *(byte*)(*param_1+0x826)*4) >> 0x10;  // per-side default
  else             iVar5 = (int)param_3;
  pcVar1 = (char*)(param_1[3] + iVar5*2);      // param_1[3] = [rec+0xC] metadata
  sVar6 = (short)*pcVar1 * 0x26;               // target_x
  sVar4 = (short)pcVar1[1] * 0x21;             // target_z
  if (*(char*)(*param_1+0x826) != 0) { sVar6 = -sVar6; sVar4 = -sVar4; }  // side flip
  param_2[1]=0; *param_2=sVar6; param_2[2]=sVar4;
}
```

Confirms: `[rec+0xC]`-sourced, `±0x26`/`±0x21`, side-flipped, writes
`rec+0x4D/+0x51/+0x55`. The engine's port is `match_phase_dcc8_apply`
(`fifa96_match_handlers.c:4579`, gated on `rec->phase_meta == NULL`; live via
the match-begin bind *cross-cited* FU-199 §7.2).

### 2.5 The recorded divergence + verdict (controller R7, owner FU177-L1)

The phase-17 T1 record (fresh: `tests/test_engine_m2.c:1352`;
`docs/ENGINE.md` phase-17 T1 bullet; `docs/PROGRESS.md:702/735`) states: **wiring
the row-03 slot moved the M2 golden at transcribed frame 29**, so the slot stays
`NULL`/uncounted. Fresh M2 evidence: `frame=29 hash=0b6b04786993a386 state=20/0-0`
(the null-backend `present_hash`, FNV-1a over the rendered planes+palette, so the
delta is a **presented-pixel** change). The T1 record itself notes the walk +
phase `0x02` (`0x6DCC8`) are **tape-inert** ("targets are render-invisible; the
render reads pos/anim/frame/hidden only") — so the frame-29 move is a *row-03
side effect beyond* the bare `0x6DCC8` target write. Candidate row-03 writes on
the forced code-3 record: `rec->ran = 1` (`0x7E1AF`), `e->timer93 = 0x10` (guard
A / special arm via `FUN_00079B58`), the placement counter `word[rec+0x7B]`
(`0x7E2E0..`), the phase-2 clamp on `rec+0x55` (`0x7E309`), the idle-hold target
copy, or a downstream locomotion tick consuming the newly-set target. The exact
field was **not re-instrumented** (read-only slice — no code, no test, no run):
recorded as **FU246-L1**.

The table comment (`fifa96_match_handlers.c:5105`) also phrases the divergence as
"its phase-0x13/0x14 special arm rewrites the target chain"; but the special arm
is entered only when the **global** phase is `0x13`/`0x14` (`0x7E1D6`), whereas
the forced-decision install (`match_row_reset`, `fifa96_match_handlers.c:2213`)
fires code 3 only when `state.phase == 2`. The two phrasings are reconciled only
by an instrumented run; the phase-2 `[rec+0x1C]`=0x6DCC8 path is the one already
proven live (FU-177 §3, FU-198 §5.3, the T1 record). Carried as **FU246-L1**.

**Verdict (wire-vs-ceiling): CEILING — the slot stays `NULL`/uncounted.** Wiring
row `03` is **not tape-neutral**: it moved M2 at frame 29 (R7). Standalone wiring
without the hot-path rewrite (`[rec+0x1C]` = `0x6DCC8`) would be a **fake**
(FU-177 §7 T3-R2, rejected). The honest completion is one of: (a) a Stage-2
re-pin with a written reason + frame diff (AGENTS.md M2 policy allows a reasoned
re-pin; M1 is immutable), if the frame-29 delta is proven the *faithful* native
side effect; or (b) keep the leg. Under the current model it is a **permanent
ceiling leg, owner FU177-L1**. **+0 wired.**

---

## 3. Action `17` — the act-4 stage-0 installer and the permutation residual

### 3.1 The row reference is the action table only (fresh)

Fresh `get_xrefs_to 0x84730` = **1**, a `DATA` ref from `0x11073C` (= `0x1106E0
+ 0x17*4`, the row-`0x17` slot). Fresh `disassemble_bytes 0x84730` (64 B): the
row is phase-gated — `0x8473B MOV EAX,[0x157A4A] ; SAR EAX,0x18 ; 0x84743 CMP
EAX,0xB ; JZ 0x8474F ; else 0x8474A JMP 0x849A1` (the shared reset). The engine
ports this as `fifa96_match_action_17` over `fifa96_action_stage_enter(gates =
{0x0B})` (`fifa96_match_handlers.c:3437`), slot still `NULL`
(`fifa96_match_handlers.c:5142`).

### 3.2 The sole installer is act-4 stage 0 (fresh)

The act-4 handler `0x89620` (`get_function_by_address 0x89620` = no function
defined; fresh `get_xrefs_to 0x89620` = **1**, `DATA` from `0x1107FC` = the
phase-`0x1A` slot). Its stage-0 install (fresh `disassemble_bytes 0x89700`, 96 B)
is:

```
0x89716 EDX=[EBP] ; DL=[EDX+0x826] ; AND EDX,0xFF      ; the record's team side
0x8971F MOV EAX,0xB                                    ; phase 0xB
0x8972A MOV ECX,1                                      ; invoke-now
0x8972F CALL 0x740A0                                   ; write phase 0xB
0x89734 XOR EBX,EBX                                    ; BL = 0 (staged)
0x89736 MOV EDX,0x17                                   ; action code 0x17
0x8973B MOV EAX,[0x1587EC]                             ; the matched record
0x89740 CALL 0x7D9A4                                   ; install row 0x17, invoke-now
```

So the **only** installer of action row `0x17` is `0x89740` inside act-4 stage 0
(`ECX=1` invoke-now), preceded by the phase-`0xB` write. The engine's port
(`fifa96_match_action_phase_1A`, `fifa96_match_run.c:6177`) installs with
`fifa96_match_entities_install(rec, 0x0B, 0x17, 0)` — **staged, no invoke**
(the deferred drain, FU197-L6).

### 3.3 The act-4 arm `0x8AB08..0x8AB62` and its gate (fresh `disassemble_bytes 0x8AB08`, 96 B)

```
0x8AB08 MOV EAX,0x1588A4 ; CALL 0x8C974        ; sort team 0
0x8AB12 MOV EAX,0x1590D9 ; CALL 0x8C974        ; sort team 1
0x8AB1C MOV EAX,[0x157A49] ; SAR EAX,0x18      ; alias byte [0x157A4C]
0x8AB24 CMP EAX,1 ; JZ 0x8AB63                 ; goal-side flag == 1 -> normal table
0x8AB29 CMP byte[0x157B8E],0 ; JNZ 0x8AB3B
0x8AB32 CMP byte[0x157B8F],0 ; JZ 0x8AB63      ; both residual counters clean -> normal
0x8AB3B MOV EAX,[ESP] ; MOV EBX,1 ; XOR EDX,EDX ; [0x158820]=AX ; EAX=4
0x8AB50 MOV word[0x15881E],CX ; 0x8AB57 CALL 0x888FC   ; FUN_000888FC(4, 0, BX=1)
```

The arm is entered from `FUN_0008A938` (`get_xrefs_to 0x8AB08` = **1**,
conditional jump from `0x8AAB8`), and takes act 4 only when
`[0x157A4C] != 1` **and** (`[0x157B8E] | [0x157B8F]) != 0`.

### 3.4 The permutation residual (fresh decompiles)

`FUN_0008C974` (fresh `decompile_function`; `get_xrefs_to 0x8C974` = **6**:
`0x8AAC4`/`0x8AACE` sit-1, `0x8AB0D`/`0x8AB17` sit-2..4, `0x9344A`/`0x93454`)
is a **selection sort of `current` onto `desired`** followed by a residual count:
`residual = #{k : 0x157BD2[side][k] != 0x14C360[side][k]}`, stored into
`[0x157B8E + side]`. The residual is **0 iff `0x157BD2` is a permutation of
`0x14C360`**.

`FUN_00073EE0` (fresh `decompile_function`; `get_xrefs_to 0x73EE0` = **3**:
`0x7407F`, `0x4B025`, `0x7428E`) is the match reset: it copies
`0x14C3A0→0x157B90`, `0x14C360→0x157BD2`, `0x14C3B0→0x157BA6` **and zeroes
`[0x157B8E]`/`[0x157B8F]`**. Because the reset copies `desired → current` at
match init, the residual after reset is **0**, so the arm is **not taken** on the
native begin path. The genuine trigger is a runtime **non-permutation** of
`0x157BD2` against `0x14C360` — a writer with no located producer (FU201-L1).

**Engine route (first-hand).** The engine models both producers, both gated:
the sit-2..4 arm at `fifa96_match_run.c:3174..3183` (gated on
`mr->formation_order_ok`) and the act-1 stage-1 hand-off at
`fifa96_match_run.c:6140..6142`. `formation_order_ok` defaults 0 and
`formation_mismatch[]` is 0 (the reset copy is not modelled), so both are dormant;
the situation table (`fifa96_action_handlers.c:1127`) has **no act-4 row**.

**Verdict (wire-vs-ceiling): CEILING — action `17` and phase `0x1A` stay
`NULL`/uncounted.** Un-gating `formation_order_ok` on a desired-order fill alone
is a **no-op** (current copied) or a **fake producer** (current zeroed → a
fabricated nonzero residual). No faithful producer exists → **+0 wired**. Owner
chain: FU201-L1 (the runtime non-permutation divergence producer),
FU201-L4/FU197-L4 (the fill landing), FU181-L1/FU197-L6 (row-17 entry /
invoke-now).

---

## 4. The act-4/5/6 producers — phase `0x1A` / `0x1B` / `0x1C`

The phase bodies have **only** the phase-table `DATA` reference (fresh
`get_xrefs_to`): `0x89620` → `1` (`0x1107FC`), `0x890EC` → `1` (`0x110800`),
`0x89110` → `1` (`0x110804`). No code producer stages them.

### 4.1 Act 4 / phase `0x1A` (`0x89620`) — no producer

**Two static act-4 sites** (fresh `get_xrefs_to 0x888FC`, see §4.4):
`0x8AB57` (the sit-2..4 arm §3.3) and `0x88EE8` (the act-1 body stage 1, `MOV
EAX,4`, `EBX=1`, `EDX=0`). Both are gated on the formation residual (§3.4). The
engine's situation table has **no act-4 row**. **Verdict: CEILING**
(FU193-L3 / FU201-L1). The body is ported (`fifa96_match_action_phase_1A`) and
exported for unit tests only.

### 4.2 Act 5 / phase `0x1B` (`0x890EC`) — no invoker

Fresh `disassemble_bytes 0x890EC` (48 B): the native body is a **pure act-slot
reset** — `XOR EDX,EDX ; XOR AH,AH ; word[0x158818]=0 ; byte[0x158828]=0 ;
byte[0x158829]=0 ; dword[0x158808]=0 ; RET`. **Fresh `FUN_000888FC` census
(`get_xrefs_to 0x888FC` = 11 sites)** with the `EAX` act argument decoded at each
site (`MOV EAX,imm` immediately before the CALL; fresh `disassemble_bytes`):

| site | EAX (act) | site | EAX (act) |
|---|---|---|---|
| `0x8A6FE` | 3 | `0x8AEEA` | 2 (stage 1) |
| `0x8A789` | 6 | `0x8AF1A` | 9 |
| `0x8AB57` | 4 | `0x8BAE7` | 0xB / 0xC (gated) |
| `0x8AB8E` | 0xA | `0x88EE8` | 4 |
| `0x8ABCF` | 1 | `0x8AED2` | 2 |
| `0x8AEBA` | 7 | | |

The act set invoked is `{1,2,3,4,6,7,9,0xA,0xB,0xC}` — **no `EAX=5`**, and no
site is register-derived to 5. Corroborating the phase-`0x1B` *writer* absence:
fresh `search_instructions mnemonic=MOV operand="EAX, 0x1b"` returns 20+ hits,
all in the front-end/`0x1xxxx`/`0x2xxxx` regions — none feeding a
`CALL 0x740A0` in the act machine. **Verdict: CEILING** (leg FU195-L1, owner
FU175-L2). Do not invent an invoker; the act domain {0..0xC} cannot select it.

### 4.3 Act 6 / phase `0x1C` (`0x89110`) — offside double-run

Fresh `disassemble_bytes 0x89110` (96 B) + `0x891C0` (96 B): the native body is a
**3-stage pre-restart timeline** — stage 0 writes **phase `0xA`** (`0x89156 MOV
EAX,0xA ; CALL 0x740A0`), stage 1/2 wait on `CALL 0x4BEC8`, and the terminal
(`0x891CC` clears `dword[0x158808]`, picks the fouled side `[[0x15888F]]→+0x7A6
→+0x826`, and **re-dispatches situation 9**: `0x891FF MOV EAX,9 ; 0x8920A CALL
0x8A938`).

**Sole producer** `0x8A789` (`MOV EAX,6 ; CALL 0x888FC`, fresh §4.2) in
`FUN_0008A43C` (fresh `get_xrefs_to 0x8A43C` = **2**: `0x79F2B` in
`FUN_00079D5C`, `0x81EBF` the row-`0x0C` contact body), gated on `EBX==3` and
`[0x14C2F2] != 0`. The engine has **no act-6 row** and the contact port issues no
act-6 request; the existing driver is the **offside machine**: `match_run_ref_step`
(`fifa96_match_run.c:6670..6679`, `case REF_OFFSIDE`) calls
`fifa96_ref_offside_sequence_step`, and `fifa96_match_action_phase_1C`
(`fifa96_match_run.c:6237`) **wraps the same** `fifa96_ref_offside_sequence_step`.
Wiring the phase-`0x1C` slot would therefore **double-run** the offside step.
**Verdict: CEILING** (FU193-L4, Ruling R8). Body ported/exported only.

---

## 5. Engine reachability verdicts (strict "engine reaches it")

| row | kind | native body | native producer (fresh census) | engine route | wire-vs-ceiling |
|---|---|---|---|---|---|
| **action `03`** | action | `0x7E1A4` hold/clamp (`fifa96_match_action_03`, ported) | reachable via the forced-decision code-3 install (`match_row_reset` 2213); the live phase-2 `[rec+0x1C]`=`0x6DCC8` rewrite is real | ported body, slot **NULL** (5104) | **CEILING** — wiring **moved M2 frame 29** (R7); not tape-neutral; standalone = fake |
| **action `17`** | action | `0x84730` line-up walk (`fifa96_match_action_17`, ported) | sole installer `0x89740` = act-4 stage 0 (`ECX=1` invoke-now) | ported body, slot **NULL** (5142) | **CEILING** — permutation residual is 0 after the reset copy; no faithful producer |
| **phase `0x1A`** | phase | `0x89620` act-4 formation-order walk (`fifa96_match_action_phase_1A`, ported) | two sites `0x8AB57` / `0x88EE8`, both residual-gated | ported body, slot **NULL** (5241) | **CEILING** (same residual gate) |
| **phase `0x1B`** | phase | `0x890EC` act-5 pure reset | **no `EAX=5` invoker** in the 11-site `FUN_000888FC` census; no `MOV EAX,0x1B` writer | slot **NULL** (5243) | **CEILING** |
| **phase `0x1C`** | phase | `0x89110` act-6 pre-restart timeline (`fifa96_match_action_phase_1C`, ported) | sole site `0x8A789` (`FUN_0008A43C`, `EBX==3`, `[0x14C2F2]!=0`) | slot **NULL** (5245); offside machine is the live driver | **CEILING** — wiring double-runs the offside step |

**Cluster census impact: +0 rows → 55/80 = 68% unchanged.** All five rows are
**permanent ceiling legs** under the current model, unless a Stage-2 task either
(a) lands the FU201-L1 runtime non-permutation producer (unlocks rows `17` and
phase `0x1A` together), or (b) takes a written-reason + frame-diff re-pin of M2
to wire action `03` faithfully.

---

## 6. Numbered legs

1. **FU246-L1 — the action-03 frame-29 divergence field.** The exact row-03
   side effect that moves the M2 presented frame at 29 when the slot is wired is
   not pinned statically (the phase-2 `0x6DCC8` target write alone is
   render-invisible per the phase-17 T1 record). Candidates: `rec->ran=1`
   (`0x7E1AF`), `e->timer93=0x10` (guard A / the `FUN_00079B58` arm),
   `word[rec+0x7B]` counter (`0x7E2E0`), the phase-2 `rec+0x55` clamp
   (`0x7E309`), or a downstream locomotion tick. Needs an instrumented
   differential run (Stage-2). Owner: FU177-L1.
2. **FU246-L2 — the action-03 tape ruling.** Wiring row `03` is a presented-
   frame move (frame 29). The Stage-2 policy call is: keep `NULL` (ceiling) or
   re-pin M2 with a written reason + frame diff (M1 immutable). Owner: controller
   / FU177-L1.
3. **FU246-L3 — action `17` / phase `0x1A` producer.** The arm + both kinds
   (sit-2..4 arm `0x8AB08`, act-1 stage-1 `0x88EE8`) are gated on the
   `FUN_0008C974` **permutation residual**; the reset `FUN_00073EE0` copies
   desired→current, so the residual is 0 and no native routine performs a
   non-permutation bulk write. The genuine trigger (a runtime non-permutation of
   `0x157BD2`) has no located writer. Inherits FU201-L1 (owner), FU201-L4
   (fill landing), FU181-L1/FU197-L6 (row-17 entry + invoke-now gap).
4. **FU246-L4 — phase `0x1B` / act-5 invoker.** The fresh 11-site
   `FUN_000888FC` census passes no `EAX=5`, no indirect/data reference exists, and
   no `0x740A0` writer emits phase `0x1B`; the act domain {0..0xC} cannot select
   it. Owner: FU175-L2 / FU195-L1. Do not invent an invoker.
5. **FU246-L5 — phase `0x1C` / act-6 producer.** Sole site `0x8A789` in
   `FUN_0008A43C` (`EBX==3`, `[0x14C2F2]!=0`), reached from the row-`0x0C`
   contact body (`0x81EBF`). The engine's contact port has only the act-3 / sit-9
   requests; the phase-`0x1C` body aliases the live offside machine
   (`fifa96_ref_offside_sequence_step`), so a naive wire double-runs it. Owner:
   the contact/referee machine (T2 / R8).
6. **FU246-L6 — `FUN_0008A43C` act-3 vs act-6 selection.** The `EBX` /
   `[0x14C2F2]` gating that chooses the act-3 arm (`0x8A6FE`) over the act-6 arm
   (`0x8A789`) was re-confirmed byte-level here but its caller-side state machine
   is FU-150's contact machinery. Carried.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info`.
* `read_memory` — `0x1106E0` (128 B), `0x110794` (140 B), `0x1107EC` (64 B).
* `disassemble_bytes` — `0x7E1A4` (128 B), `0x7E2D0` (64 B), `0x7E244` (48 B),
  `0x8AB08` (96 B), `0x89700` (96 B), `0x8A6F0` (40 B), `0x8A780` (40 B),
  `0x8AB80` (112 B), `0x8ABA8` (64 B), `0x8AEA0` (96 B), `0x88ED8` (32 B),
  `0x8AF00` (48 B), `0x8BAC8` (48 B), `0x890EC` (48 B), `0x89110` (96 B),
  `0x891C0` (96 B), `0x84730` (64 B), `0x6D9A0` (48 B).
* `decompile_function` — `0x6DCC8`, `0x8C974`, `0x73EE0`.
* `get_xrefs_to` — `0x7E1A4` (1), `0x84730` (1), `0x8AB08` (1), `0x888FC` (11),
  `0x8C974` (6), `0x73EE0` (3), `0x89620` (1), `0x890EC` (1), `0x89110` (1),
  `0x8A43C` (2), `0x6D920` (4), `0x740A0` (27).
* `search_instructions` — `mnemonic=MOV operand="EAX, 0x1b"` (20+ hits,
  front-end only).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`fifa96_match_action_03` 4846..4968, the row-03/17 table entries 5104/5142,
`match_forced_decision_code`/`match_row_reset` 2187..2222,
`match_row_phase_call` 4555, `match_phase_dcc8_apply` 4579, the phase table
5188..5259, `fifa96_match_dispatch_phase_record` 5287), `fifa96_match_run.c`
(the act-4 arm 3159..3184, `fifa96_match_action_phase_17` 6124,
`match_run_phase_1A_end` 6158, `fifa96_match_action_phase_1A` 6177,
`fifa96_match_action_phase_1C` 6237, `fifa96_match_run_offside_reception` 6542,
`match_run_ref_step` offside case 6670, the record-phase walk
`fifa96_match_run_record_phase_walk` 2494), `fifa96_match_act.c`,
`src/fifa96_loader/fifa96_action_handlers.c` (`fifa96_action_phase_situations`
1127..1146), tests `tests/test_engine_m2.c:1352`, `tests/test_engine_match_act.c`
(row-17/phase-1A/1C/1B tests).

Docs cross-cited: FU-137 §2, FU-138, FU-175 (§1/§3), FU-177 (§1..§7, legs
FU177-L1..L6), FU-181 (§1/§2), FU-193 (§1/§2/§3/§4/§5/L3/L4), FU-195
(FU195-L1), FU-197, FU-198 (§5.3), FU-199 §7.2, FU-201 (§1..§10), FU-203,
FU-214, FU-206, FU-222. AGENTS.md census rule. No writes beyond this slice file
and the workspace report.

---

## 8. Landing note (this recon slice)

All five rows are **wire-vs-ceiling = CEILING**: no ported path installs **and**
dispatches them live on the current engine, and no faked producer is admissible.
The cluster contributes **+0** to the dispatch census (**55/80 = 68% unchanged**).
The only honest unlock paths are the FU201-L1 runtime non-permutation producer
(action `17` + phase `0x1A`) and a Stage-2 written-reason M2 re-pin for action
`03`; phase `0x1B` (no invoker) and phase `0x1C` (offside double-run) are
permanent legs. No engine code, no test, no shared-doc edit, no git.
