# FU-272 — the R11-L4 derivation: the port shape of the keeper stage-5 release's live installer

Promoted canonical slice — phase-26a (FU272); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**.

**Provenance.** Ghidra MCP, **read-only**, program exactly `/FIFA96.EXE`
(`watcom:LE:32:default`, image base 0, `.image` overlay present; confirmed
`get_current_program_info` this session). Every native address/byte below is a
**fresh** `disassemble_bytes` / `read_memory` / `get_xrefs_to` /
`search_instructions` of this session unless marked *carried* (the frozen
`docs/ghidra/FU*` slice is named). Engine-source facts cite the built tree by
line. Alias traps applied (`dword[x]>>16` = word at `x+2`; `>>24` = byte at
`x+3`; CALL bytes verified). No rename / comment / label / function / script /
project save; `/FIFA96.EXE` untouched; no git command.

Phase: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase26a-camera-ball-visibility.md`,
Task **T2a** (bounded, read-only derivation), promoted to canonical by the
phase-26a Task 4+5 close-out. No production code; the derivation and its
**NO-GO — phase-scale** verdict are unchanged by promotion.

**Question (brief).** Derive exactly what "port R11-L4" means for the keeper
stage-5 release (`camY := 0x50`, FU264 §2.1) to install and run on a **live**
engine path, and rule it **bounded** (a handful of producers + a RED-first reach
test) or **phase-scale** (a whole queue/machine port).

**Result in one line.** **Phase-scale → NO-GO.** The keeper stage-5 release lives
inside row `0x1E`, which the engine installs via **phase 9 ← situation 5**; the
only native situation-5 producers are the keeper rows `0x1A`/`0x1B`
(`0x76B1C`/`0x77707`), and **neither is dispatched on any live engine path** —
their installers are the unported **keeper CPU chain**
(`FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`, plus the row-0x1F / phase-6 arm),
an entire keeper-AI machine (FU-151/FU-269's own cluster). This is not "a handful
of producers". Recommend the **honest Layer-2 waiver** (spec §7.1) for phase-26a
and route **R11-L4 to a later phase** (the keeper cluster, 26b).

---

## 1. Q1 — the situation-5 **queue (id 7)**: what it is, and where it stops

### 1.1 The situation dispatcher `FUN_0008A938` — the head and the queue arm

Fresh `disassemble_bytes 0x8A960` (128 B):

```
0x8A964  CMP dword[0x0015b6c0],0x0        ; a situation already pending?
0x8A96B  JNZ 0x8AA7B                       ; yes -> DIRECT path
0x8A971  MOV ESI,[ESP]                     ; the side word
0x8A974  TEST SI,SI ; SETZ AL ; AND EAX,0xff
0x8A982  MOV [0x0015b6b8],EAX              ; the sit_side_pending store
0x8A97F  SUB ECX,0x2                       ; ECX = situation ; index = situation - 2
0x8A987  CMP CX,0x8 ; JA 0x8AA60            ; index > 8 -> the default arm (id 0xA)
0x8A996  JMP dword CS:[EAX*4 + 0x8A8E0]     ; <-- table-1 (the QUEUE table)
```

The preceding gates (fresh bytes quoted in FU267 §0, re-verified by the window
above): `0x8A944 TEST AX,AX; JZ 0x8AA7B` (id 0 → direct), `0x8A94E CMP
EAX,0xB; JZ 0x8AA7B` (id 0xB → direct), `0x8A957 CMP byte[0x14C32A],0; JZ
0x8AA7B` (session gate `[0x14C32A]` closed → direct). So the **queue arm**
(`0x8A971..`) is taken only when `id ∉ {0, 0xB}`, the session gate is **open**
(`[0x14C32A] != 0`), and no situation is pending.

### 1.2 Table-1 (`0x8A8E0`) — situation 5 and 7 both queue **id 7**

Fresh `read_memory 0x8A8E0` (40 B, 10 dwords) decoded:

| index (= situation−2) | situation | arm | stores |
|---|---|---|---|
| 0,1,2 | 2,3,4 | `0x8A99E` | id 9 (side-dependent) |
| **3** | **5** | **`0x8A9CD`** | **`[0x15B6A8] := 7`** |
| 4 | 6 | `0x8A9E8` | id 5/6 |
| **5** | **7** | **`0x8A9CD`** | **`[0x15B6A8] := 7`** |
| 6 | 8 | `0x8AA60` | the default id 0xA |
| 7,8 | 9,10 | `0x8AA23` | id 1/2 |
| (9) | — | unreached (`CX > 8` → default) | — |

Fresh `disassemble_bytes 0x8A9CD` region (quoted from the 0x8A960 window):

```
0x8A9CD  MOV dword[0x0015b6a8],0x7      ; c705a8b61500 07000000   <-- id 7
0x8A9D7  MOV dword[0x0015b6c0],0x1      ; the pending latch
```

**So "the situation-5 queue (id 7)" is exactly this:** a situation-5 (or
situation-7) emit takes the table-1 queue arm and stores the screen id **7** in
`[0x15B6A8]` plus the pending latch `[0x15B6C0] := 1`.

### 1.3 The drain — the id 7 goes to the **screen machinery**, never phase 9

The engine mirrors the queue directly (`fifa96_match_run_set_piece`, fresh read
`src/fifa96_engine/fifa96_match_run.c:3111-3142`):

```
3123  switch (situation) {
3127    case 5u: case 7u:
3128      mr->situation_id = 7u;          /* the native 0x8A9CD store */
3130    case 6u: ...
3136    default: mr->situation_id = 0x0Au;
3140  mr->situation_pending = 1u;
```

`mr->situation_id` / `mr->situation_pending` are the FU-146 **screen** cells, and
their only consumer is `match_run_screen_post` (`match_run.c:3446-3497`): it
reads `id = mr->situation_id` (`:3449`), maps it through the per-leg tables
`match_run_screen_ids[leg]` (`:3419-3429`) to a **score side** (`:3479-3481`),
and then `match_run_write_phase(mr, 0)` (`:3494`). For id 7 the leg-0/1/3 tables
(`{1,0,1,-1,0,-1,-1,-1,1}`) return **−1** (no score); the drain never writes any
phase, least of all phase 9.

**The route that does reach phase 9 is the *direct* (BX=0) table-2 arm.**
Fresh `read_memory 0x8A904` (52 B, 13 dwords) gives table-2 `[5] = 0x8ADD8`, and
fresh `disassemble_bytes 0x8ADD8`:

```
0x8ADD8  MOV EDX,[ESP-2] ; MOV EAX,0x9 ; SAR EDX,0x10 ; CALL 0x000740a0
```

i.e. `FUN_000740A0(9, side)` = **write phase 9**. The engine models this exactly
as `fifa96_action_phase_situations[5] = { .phase = 9u }`
(`src/fifa96_loader/fifa96_action_handlers.c:1138`) →
`match_run_situation_side` (`match_run.c:2896-2912`) → `phase_arm(9)` → **install
row `0x1E` on the controlled team's record 0** (`match_run.c:2853-2861`).

### 1.4 Where it stops — the gate that fails live

The **native emit of situation 5 is BX=1** at both producers (verified in §2), so
natively the emit takes the **queue** (id 7) whenever the session gate is open —
and the queue drain is the screen machinery, not phase 9. The phase-9 route is
reached only via the direct BX=0 arm or the act-8 sweep re-dispatch
(`0x8A8CE`, FU-267 §2).

**But the binding gate on the live path is upstream of the queue:** rows `1A`/`1B`
(the sit-5 producers) are **never dispatched** on a live engine path (§2.4). No
dispatch ⇒ no emit ⇒ no queue, no phase 9, no row `0x1E`, no stage-5 release.
That dispatch gap — **R11-L4** — is the whole of the problem.

---

## 2. Q2 — what rows `1A`/`1B` are natively

### 2.1 Identity (fresh action-table read)

Fresh `read_memory 0x1106E0 + 0x1A*4` (= `0x110748`, 16 B): `2c 66 07 00 | 28 6d
07 00 | 28 77 07 00 | b0 4e 07 00` → `0x1106E0[0x1A] = 0x7662C`,
`[0x1B] = 0x76D28`, `[0x1C] = 0x77728`, `[0x1D] = 0x74EB0`. So:

* **row `0x1A` = `0x7662C`** — the keeper **reposition A** (5-arm stage machine;
  FU-186 §2.1).
* **row `0x1B` = `0x76D28`** — the keeper **reposition / rush B** (5-arm stage
  machine `{0x76DF6,0x76E3B,0x76FC7,0x7769F,0x776C1}`; FU-180 §2).

### 2.2 What each emits (fresh bytes)

* **row `0x1A`** — **situation 7 (BX=0)** at `0x76A90` and **situation 5 (BX=1)**
  at `0x76B1C`:
  ```
  0x76A83 MOV EAX,0x7 ; 0x76A88 AND EDX,0xff ; 0x76A8E XOR EBX,EBX ; 0x76A90 CALL 0x8A938
  0x76B0C MOV EBX,0x1 ; 0x76B11 MOV EAX,0x5 ; 0x76B16 AND EDX,0xff ; 0x76B1C CALL 0x8A938
  ```
* **row `0x1B`** — **situation 7 (BX=0)** at `0x77589` and **situation 5 (BX=1)**
  at `0x77707` (the stage-4 finish):
  ```
  0x7757C MOV EAX,0x7 ; 0x77587 XOR EBX,EBX ; 0x77589 CALL 0x8A938
  0x776F7 MOV EBX,0x1 ; 0x776FC MOV EAX,0x5 ; 0x77707 CALL 0x8A938   ; the stage-4 tail
  ```

So **both** situation-5 producers emit with **BX=1** (the queue / act-8 route),
and **both** situation-7 producers emit with BX=0.

### 2.3 What installs them (the CPU chain)

Fresh `get_xrefs_to 0x77EAC` = **1** (`0x78599` in `FUN_000782D0`);
`get_xrefs_to 0x782D0` = **1** (`0x8DB35` in `FUN_0008D8EC`);
`get_xrefs_to 0x8D8EC` = **4** (`0x4B2AB`/`0x4B2B2` in `FUN_0004B100` — the
per-frame update chain — and `0x74429`/`0x74430`). The ladder
`FUN_00077EAC 0x78200` installs (FU-186 §1.1/§3, carried and re-confirmed by the
single xref): `0x1A ×3` (`0x78277/0x7824F/0x78227`), `0x1B` (`0x78299`), `0x1C`
(`0x782B6`). Row `0x1B` also has a second installer at `0x76557` (row-0x1F
stage-2 no-slot dive, invoke-now; FU-180 §1.1).

### 2.4 Engine state today — no live dispatch of `1A`/`1B`

* Engine action table (`src/fifa96_engine/fifa96_match_handlers.c`):
  `{0x1A, NULL, …}` (`:5512`, ported body, slot deliberately NULL — FU-186 §8),
  `{0x1B, fifa96_match_action_1B, …}` (`:5514`, **wired/counted**),
  `{0x1C, fifa96_match_action_1C, …}` (`:5516`).
* The wired `fifa96_match_action_1F` installs `0x1B` invoke-now
  (`fifa96_match_handlers.c:3611/3616`) — **but** row `0x1F`'s only engine
  install site is the **phase-6 arm** (`match_run.c:2849`), and **no *proven-live*
  path writes phase 6.** The engine's only phase-6 writer is the FU-150 foul/ref
  restart: `match_run_ref_step_restart` (fresh read `match_run.c:5956-6013`)
  computes `uint8_t phase = 7u; … phase = 6u;` (`:5980-5990`, the `:5987/5989`
  stores) then calls `match_run_write_phase(mr, phase)` (`:5996`) +
  `fifa96_match_run_phase_arm(mr, phase)` (`:5998`); it is dispatched from the
  phase-0x18 / act-2 body `fifa96_match_action_phase_18` (`:6027-6032`) on the
  sit-9/sit-0xA → act-2 route (`match_run_situation_side`, `:2929-2943`).
  `match_run_ref_apply` (`:5890`, `out->phase_write`) is a second variable phase
  write on the same chain. That chain is the **FU-150 foul/ref family**, which
  **FU271 §4.1 labels LIVE-LEG** ("ported producer, live path structurally
  present, unproven end-to-end"; FU-268-L1) — so phase 6 is *located but not
  proven live*. (A literal grep for `write_phase(mr, 6` or the native `MOV
  EAX,0x6` misses this **variable** path; the first draft's "never writes phase
  6" was wrong.)
* The **second** row-`0x1B` installer `0x78299` and the whole
  `FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` chain are **unported**
  (FU-186 §1.3/§6 L5-L7; FU-180 §4.3/L4-L5; FU-151 §8 item 8).
* Row `0x1B`'s **stage-2 wide body** (which carries the `0x71C94` pan-setter call
  at `0x77423`) is **unported** (`fifa96_match_action_1B` keeps a bounded
  stage-2; FU-180-L1). So even a dispatched row `0x1B` would not run that
  `0x71C94` caller — a second, independent gap.

**Conclusion for Q2:** rows `0x1A`/`0x1B` are the two keeper reposition/rush
machines that emit situation 5 (BX=1) / 7 (BX=0); the engine has their bodies
(mostly) but **no proven-live installer** — the installer is the unported keeper
CPU chain, plus (for row 0x1F) a phase-6 emitter whose only writer is the
unproven FU-150 foul chain (LIVE-LEG, FU271 §4.1).

---

## 3. Q3 — the **minimal port** to make the keeper stage-5 release install live

The release itself is the ported `fifa96_match_action_1E` stage-5 arm
(`fifa96_match_handlers.c:521-530`: `camera.pos_y = s->cam_y;` +
`fifa96_camera_event_set(&camera, 0, 0, 0x50, 0)` + `camera_set_tracked`), and the
install (`phase_arm(9)` → `0x1E` on the controlled record 0) is landed. **What is
missing is the live installer of the sit-5 producers.** The items below are the
native producers/seams involved; **the crux is the 3-machine installer chain
(items 1-3) plus the shared predicates/steering (items 4-6)**, not the row bodies
— row `0x1A` stage-0 and row `0x1B` stages 0/1/3/4 (and row `0x1C`) are *already
ported* (`handlers.c:5512-5517`); the unported row-body parts are the arms in
items 7-8. Every item is a **real native producer** (none is a stand-in; the
live-chain rule forbids a staged gate):

| # | native producer | address | what it does | engine owner-leg |
|---|---|---|---|---|
| 1 | `FUN_0008D8EC` driver pre-pass | `0x8D8EC..0x8DB6A` | per-frame walk; calls the record-0 machine at `0x8DB35` | FU186-L7 / FU-151 §8 item 8 |
| 2 | `FUN_000782D0` record-0 machine | `0x782D0..0x785D0` | installs the record's own action (4/0x19); calls the CPU decision at `0x78599` | FU186-L6 / FU180-L4 |
| 3 | `FUN_00077EAC` keeper CPU decision / ladder | `0x77EAC..0x782CD` | **the `0x1A`/`0x1B`/`0x1C` installer** (`0x78227/4F/77`, `0x78299`, `0x782B6`) | FU186-L5 / FU180-L5 |
| 4 | `FUN_000765C4` tracked-teammate predicate | `0x765C4..0x76615` | shared by the ladder (`0x781EC`) and row `0x1A` | FU186-L4 |
| 5 | `FUN_00076B28` keeper steering | `0x76B28..0x76D11` | row `0x1B` stage-1 / row `0x1C` tail | FU180-L3 / FU186-L2 |
| 6 | `0x114E04` sine rotation + `0x795A4` fold | `0x114E04` table | row `0x1B` stage 0/1 + row `0x1C` | FU186-L3 / FU180-L2 |
| 7 | row `0x1A` body remainder | `0x766FC..0x76AE8` | the `[0x157C2C]` direction machine + movement/event block + stages 1-4 | FU186-L1 |
| 8 | row `0x1B` body remainder | `0x76FD5..0x77636` (stage-2 wide, incl. `0x71C94` @ `0x77423`), stage 3 | the rush body + the pan setter | FU180-L1 |
| 9 | row-0x1F installer chain | phase-6 arm `0x8D4A2`/`0x8D558` (`FUN_0008D098`) + **a phase-6 producer** (located, dormant behind the LIVE-LEG FU-150 foul/ref chain) | the second `0x1B` installer (`0x76557`) | FU-149 §set-piece / FU176 |

Additionally, reaching the release **inside** row `0x1E` needs the claim
machine's own unported bridge — **R11-L1 `FUN_0006E598`** (the claim
anim-selection bridge: "without it the claim machine's stages 5–7 are
unreachable even after the row is installed", FU-163 §7), plus R11-L2 (the place
pause flag) and R11-L3 (the claim offset tables).

**Engine seams they land on:** the per-record phase walk /
`fifa96_match_dispatch_phase_record` (the phase-6 → row-`0x1F` install), the pool
`fifa96_match_entities_install`, `fifa96_match_run_situation`,
`fifa96_match_run_phase_arm` phase 8/9 (`match_run.c:2853-2861`),
`fifa96_match_action_1E` (the release arm, `handlers.c:521-530`),
`fifa96_camera_event_set` and `fifa96_camera_set_tracked`.

---

## 4. Q4 — GO / NO-GO

### 4.1 Verdict: **NO-GO — phase-scale.**

R11-L4 is **not** "a handful of producers + an obvious RED-first reach test". It
is the **whole keeper-AI machine**: the driver pre-pass, the record-0 machine,
the CPU decision ladder, the steering, the tracked-teammate predicate, the sine
rotation, the row-`1A`/`1B` body remainders, **and** the row-0x1F / phase-6
installer chain, **and** the claim anim bridge (R11-L1). That is the
`FU-151`/`FU-269` **keeper cluster** the spec's own split already assigns to
**phase-26b** ("keeper restart — the `FU-151` rows `1D`/`1E` live producers").

**Size estimate (honest):** the crux is the **3-machine installer chain**
(`FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`, items 1-3) plus the shared
predicates/steering (items 4-6), the **unported row-`1A`/`1B` body arms** (items
7-8 — not the whole bodies: row `0x1A` stage-0 and row `0x1B` stages 0/1/3/4, and
row `0x1C`, are *already ported*, `handlers.c:5512-5517`), the row-0x1F /
phase-6 chain (item 9, dormant behind the LIVE-LEG FU-150 foul chain), and the
claim bridge (R11-L1, R11-L2/L3). A multi-task port spanning FU-186 L1-L7,
FU-180 L1-L5, FU-151 §8 and FU-163 R11-L1 — not a bounded slice.

**Recommendation.** Take the **honest Layer-2 waiver (spec §7.1)** for phase-26a:
the follow-cam and the row-0E pan stay **granted-frame-only** (as FU-264/FU-271
already rule), and the camera chain is not made live this phase. **Route R11-L4
to a later phase** — the keeper cluster (26b), where rows `1A`/`1B` and the ladder
are the natural co-scope. Do **not** invent a stand-in raiser (the live-chain
rule / no-fakes rail).

### 4.2 The RED-first reach test (the design that would prove `camera.pos_y != 0` end-to-end)

To be added **when** the R11-L4 producer lands (it must fail on the baseline —
`camera.pos_y == 0`):

> **Ports to add (the engine seams):** a `fifa96_keeper_cpu_step` (the port of
> the `FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC` installer chain) wired into
> the per-frame walk; the phase-6 arm → row-`0x1F` install (or the CPU ladder's
> `0x78299`); the unported row-`0x1A`/`0x1B` body arms (the stage-2 wide /
> direction-machine / steering remains); and the claim bridge (`FUN_0006E598` →
> the `fifa96_keeper_claim_*` staging). Then a **granted-frame** ISO scenario
> drives that step so **row `0x1B` dispatches**, reaches its **stage-4 tail**
> (`fifa96_keeper_reposition_b_finish` → `notify = 5`) with the keeper as the
> *controlled* actor, and asserts in this order:
> (a) the sit-5 emit ran and `phase == 9`;
> (b) row `0x1E` installed on the controlled team's record 0;
> (c) the claim machine reached **stage 5** and the release arm fired;
> (d) **`fifa96_camera.pos_y == 0x50`**, `fifa96_camera.tracked != 0`, and the
>     row-0E window admits (`0x20 < pos_y < 0x70` on the next granted frame).
>
> Baseline behaviour: `camera.pos_y == 0` every granted frame (the arc is
> event-starved, FU-264) — the test is RED until the ports land. A
> **mutation-RED rider** (stub `fifa96_keeper_cpu_step` / the sit-5 emit) must
> break it.

This test is exactly the one T2b would carry **if** R11-L4 were routed into 26a;
since it is phase-scale, it becomes the reach-test spec for the later keeper
phase.

---

## 5. Q5 — numbered open legs (with native owners)

* **T2a-L1 — the live installer of rows `0x1A`/`0x1B` (the sit-5/7 producers).**
  The keeper CPU chain `FUN_0008D8EC` (`0x8D8EC`) → `FUN_000782D0` (`0x782D0`) →
  `FUN_00077EAC` (`0x77EAC`) — its ladder arms install `0x1A`/`0x1B`/`0x1C`
  (`0x78227`/`0x7824F`/`0x78277`/`0x78299`/`0x782B6`). Owner: the keeper cluster
  (FU-186-L5/L6/L7, FU-180-L4/L5, FU-151 §8 item 8). **This is R11-L4.**
* **T2a-L2 — the row-0x1F / phase-6 installer chain (located, dormant).** Row
  `0x1F`'s native installer is the phase-6 arm `0x8D4A2`/`0x8D558` inside
  `FUN_0008D098`. The phase-6 writer **is located**: `match_run_ref_step_restart`
  (`match_run.c:5956-6013`, the phase-0x18 / act-2 body `:6027-6032`) writes
  phase 6 (`:5987/5989` → `:5996`/`:5998`) on the sit-9/sit-0xA → act-2 route
  (`match_run_ref_apply` `:5890` is a second variable write on the same chain).
  Only its **liveness is unproven** — the FU-150 foul/ref chain is **LIVE-LEG**
  (FU271 §4.1, FU-268-L1). Owner: the FU-150 foul chain + the row-0x1F body
  remainder (FU-176-L1/L2/L4).
* **T2a-L3 — the row body remainders.** Row `0x1A` `[0x157C2C]` direction machine
  + movement/event block + stages 1-4 (`0x766FC..0x76AE8`, FU186-L1); row `0x1B`
  stage-1 rotation + stage-2 wide body (incl. the `0x71C94` pan-setter call
  `0x77423`) + stage 3 (`0x76FD5..0x77636`, FU180-L1/L2/L3); `FUN_00076B28`
  steering (`0x76B28`, FU180-L3); the `0x114E04` sine table (FU186-L3); the
  `FUN_000765C4` predicate (`0x765C4`, FU186-L4).
* **T2a-L4 — the claim-machine stage-5 reach (R11-L1/L2/L3).** The claim
  anim-selection bridge `FUN_0006E598` (R11-L1), the place pause flag (R11-L2),
  and the claim offset tables `0x10F334/0x10F33C` from the record sector (R11-L3)
  — without R11-L1 the stage-5 release is unreachable even with row `0x1E`
  installed (FU-163 §7). Owner: FU-163 / `fifa96_keeper.c`.
* **T2a-L5 — the situation-5 queue (id 7) consumer.** The native queue drain
  (`0x8A9CD` → `[0x15B6A8]=7` / `[0x15B6C0]=1`) is consumed by the FU-146 screen
  machinery, which the engine models only partially and which never reaches
  phase 9 (the per-leg `match_run_screen_ids` id-7 mapping is −1). "Wire the
  situation-5 queue (id 7) through its native consumer" is therefore **also** not
  a short route — it is the unported FU-146 screen machinery
  (`FU269-L1`). Owner: FU-146/FU-149.
* **T2a-L6 — the emit-route divergence (FU269-L5).** Native emits sit-5 with
  **BX=1** (queue / act-8 sweep); the engine's row-`0x1B` tail calls the
  **direct** `fifa96_match_run_situation(mr, 5)` (side=0, bx=0) — the direct
  table-2 route to phase 9, bypassing the queue. Favorable for reaching phase 9,
  but a fidelity divergence that must be disclosed when R11-L4 lands. Owner:
  FU269-L5.

---

## 6. Doc errata candidates (quoted, first-hand)

1. **`docs/ghidra/FU265_place_pan_ball_framing.md` §1.4, table rows 6 and 9.**
   The table asserts the `FUN_00071C94` caller bodies at `0x77423` (row 1B) and
   `0x77dc6` (row 1C) are "**NO** (row unwired) … dispatch `-FIFA96_ERR_UNSUPPORTED`".
   Fresh `fifa96_match_handlers.c:5514/5516` show rows `0x1B` and `0x1C` are
   **wired/counted**. The accurate statement is: the rows are wired, but the
   specific **stage-2 wide arm** (`0x77423`) / row-1C tail (`0x77dc6`) that carry
   those `0x71C94` callers are unported (FU-180-L1 / FU-186-L2). The two
   `0x71C94` callers are therefore not reached, but not for the stated reason.
2. **`docs/ghidra/FU265` §1.4, row 6 wording ("row 1B body … row unwired").**
   Same as (1): row 1B's slot is wired since phase-15 T2 (FU-180 §8); the
   unported part is its stage-2 wide body.

---

## 7. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| situation dispatcher head + queue-arm gates | `disassemble_bytes 0x8A960` (128 B) |
| table-1 `0x8A8E0`: sit 5/7 → `0x8A9CD` | `read_memory 0x8A8E0` (40 B) |
| `0x8A9CD` stores `[0x15B6A8]=7` + pending | `disassemble_bytes 0x8A960` window (`0x8A9CD`) |
| table-2 `[5]=0x8ADD8` → `FUN_000740A0(9)` (phase 9) | `read_memory 0x8A904` (52 B); `disassemble_bytes 0x8ADD8` |
| row `0x1A` = `0x7662C`; row `0x1B` = `0x76D28` | `read_memory 0x1106E0+0x1A*4` (16 B) |
| row `0x1A` emits sit-7 BX=0 (`0x76A90`), sit-5 BX=1 (`0x76B1C`) | `disassemble_bytes 0x76A70` / `0x76AF0` |
| row `0x1B` emits sit-7 BX=0 (`0x77589`), sit-5 BX=1 (`0x77707`) | `disassemble_bytes 0x77570` / `0x776D0` |
| keeper stage-5 release `camY := 0x50` | `disassemble_bytes 0x75D30` (`0x75D4A`/`0x75D69`) |
| CPU chain telemetry: `FUN_00077EAC`←`FUN_000782D0`←`FUN_0008D8EC`←`FUN_0004B100` | `get_xrefs_to` `0x77EAC`(1), `0x782D0`(1), `0x8D8EC`(4) |
| phase-6 arm `0x8D4A2` (the phase-6 install block) | `disassemble_bytes 0x8D490` |
| engine phase-6 writer = the FU-150 foul/ref restart (variable `phase`) | `match_run.c:5956-5999` (`phase = 6u` `:5987/5989`; `write_phase`/`phase_arm` `:5996/5998`) + `:5890`; act-2 body `:6027-6032` |
| that phase-6 chain is LIVE-LEG (not proven live) | carried `docs/ghidra/FU271_live_trigger_census.md` §4.1 (`FU-268-L1`) |
| engine: row `0x1A` NULL / `0x1B` / `0x1C` wired; phase-8/9 arm installs `0x1D`/`0x1E` | `fifa96_match_handlers.c:5512/5514/5516`; `fifa96_match_run.c:2853-2861` |
| engine: row-0x1E stage-5 release arm | `fifa96_match_handlers.c:514-530` |
| engine: sit-5/7 queue arm stores id 7 | `fifa96_match_run.c:3111-3142`; drain `:3446-3497` |
| engine: row-0x1F installs `0x1B` invoke-now | `fifa96_match_handlers.c:3611/3616` |

**Engine source read (read-only):** `fifa96_match_run.c` (2762-3200, 3430-3730),
`fifa96_match_handlers.c` (460-640, 3660-3936, 5454-5640),
`fifa96_action_handlers.c` (1121-1161), `fifa96_keeper.c` (via FU-180/FU-186).

**Carried docs (read, not rewritten):** FU-264 §2.1/§4.1, FU-265 §5/§1.4,
FU-271 §Step 2 (keeper row) / FU271-L5, FU-269 §1.3-1.6/§4.2-4.3, FU-267 §1,
FU-163 §7 (R11-L4), FU-168 T2 status, FU-171 T2/T3 status, FU-180 §1-6/§8,
FU-186 §1-6/§8, FU-151 §8, the phase-26 spec §5/§7.1, the phase-26a plan T2.
