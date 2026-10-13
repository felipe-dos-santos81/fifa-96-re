# FU-217 — the `[0x1587D4]` actor-cell producer (phase-20 wave R65)

Provenance: read-only, 2026-10-13. Authoritative program **`/FIFA96.EXE`**
(Ghidra MCP; scripts disabled; case-sensitive program argument; confirmed this
session by `get_current_program_info` — name `FIFA96.EXE`, path `/FIFA96.EXE`,
`watcom:LE:32:default`, image base `0`, `min_address 0x10000`,
`max_address .image::00174a2a`, 2706 functions, `.image` overlay present). Every
address/byte/instruction/xref below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `get_function_by_address` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). LE-image
globals sit at `+0x100000` from the file offset; the addresses here are
`/FIFA96.EXE` program (runtime) addresses. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`; the sit-8 arm's
`[ESP]`/`[ESP-2]` pair, §3.1). Engine source read first-hand
(`src/fifa96_engine/fifa96_match_run.c`, `fifa96_match_handlers.c`,
`fifa96_match_entities.c`; `include/fifa96_engine/fifa96_match_run.h`,
`fifa96_match_entities.h`; `src/fifa96_loader/fifa96_action_handlers.c`;
`tests/test_engine_m2.c`). **No writes**: no rename, comment, label, function,
script or project save; `/FIFA96.EXE` untouched; ISO/HDD untouched; no git
commands.

Scope (R65 brief): derive the **producer of the actor record `[0x1587D4]`** —
the value the phase-`0x1D` arm-0 installer `0x899A2` writes code `0x20` onto —
closing FU210-L3 + FU194-L8 (+ FU145-L4, FU208-L7). Every reader vs writer of
the cell; the bind `0x8AE4B`/`0x8AE54` in `FUN_0008A938`'s sit-8 arm; its
per-side source table `0x1587CC` and the selector `FUN_000897F4`; the gate; the
row-`0x20` reachability (tape-dormant vs live) and the exact engine extension
point. Extends FU-210 (§2/§3/§7.3), FU-209 (§1.5/§3), FU-208 (§2/§3/§6/§7),
FU-194 (§3.4/§6.1/L8), FU-189 (§3), FU-145 (§4/L4). Model: FU-208.

---

## 1. Verdict (one line)

**`[0x1587D4]` is the set-piece *actor* record; its producer is the sit-8 arm
`0x8AE08..0x8AE54` of `FUN_0008A938`, guarded by `byte[0x157A4C] != 1`, which
stores the per-side source `[0x1587CC + side*4]` — the first *non-occupied*
outfield record (index `1..10`, `byte[+0x9A]==0`) selected by
`FUN_000897F4(side, 0xA)` — into the cell. Porting that bind (plus flipping the
row slot) makes row `0x20` genuinely install-live via the already-registered
phase-`0x1D` route, so the row is **countable (+1 → `53/80 = 66%`) yet
tape-dormant** (the M2 tape reaches no `[0x157AC2] == 4` completion, so sit 8
never fires and the cell never moves).**

| fact | fresh value |
|---|---|
| cell | `[0x1587D4]` = dword, image default `0x00000000` (fresh `read_memory 0x1587D4`) |
| per-side table | `[0x1587CC]` (side 0) / `[0x1587D0]` (side 1) — 3 consecutive dwords `0x1587CC/0x1587D0/0x1587D4`, all-zero default (`read_memory 0x1587CC` 32 B) |
| **the writer** | `0x8AE54 MOV [0x1587D4],EAX` in `FUN_0008A938` (sit-8 arm); `EAX = [0x1587CC + side*4]` loaded at `0x8AE4B` |
| the per-side writer | `0x89832 MOV [side*4+0x1587CC],EAX` in `FUN_000897F4` |
| 2nd writer | `0x89DCF MOV [0x1587D4],EAX` (phase-`0x1D` arm-8 no-winner rebind, `EAX=[0x1587CC+(side^1)*4]`) |
| the gate | `0x8AE08 MOV EAX,[0x157A49]; SAR 0x18; CMP 1; JZ 0x8AEAE` = `byte[0x157A4C] != 1` |
| xref count | `get_xrefs_to 0x1587D4` = **14** (12 READ, 2 WRITE) |
| producer callers | `get_xrefs_to 0x897F4` = **3** (`0x8AE30`, `0x8AE3F` sit-8 arm; `0x89D9F` phase-`0x1D` arm 8) |
| row-0x20 installer | `0x899A2` (phase-`0x1D` arm 0) → `FUN_0007D9A4([0x1587D4],0x20,0,0)`; slot `0x110760 = 0x84EEC` (fresh `read_memory 0x110760` = `ec 4e 08 00`) |

**Closes:** FU210-L3 (the actor-cell producer), FU194-L8 (the actor is always an
outfield record), FU145-L4 (the `[0x1587D4]` record identity is derived),
FU208-L7 (the `0x8AE19..0x8AE54` re-init body is now byte-derived).

---

## 2. The cell `[0x1587D4]` and its 3-dword neighbourhood (fresh)

### 2.1 The storage

* Fresh `read_memory 0x1587D4` (4 B) = `00 00 00 00` — the image default is
  NULL (a stale/zero image has no actor until the sit-8 arm binds one).
* Fresh `read_memory 0x1587CC` (32 B) = all-zero. The **three live dwords** are
  `0x1587CC` (side-0 actor), `0x1587D0` (side-1 actor) and `0x1587D4` (the
  active actor); `[0x1587D0]` has `get_xrefs_to 0x1587D0` = **0** (it is only
  ever reached as `[0x1587CC + side*4]`, never by its own address). The
  neighbouring bytes `0x1587D8/0x1587D9/0x1587E6..E9` are the sit-8 arm's
  per-side state cells (§3.1 clears them), not part of this pointer trio.

### 2.2 The 14 references — READERS vs WRITERS (fresh `get_xrefs_to 0x1587D4`)

| from | fn | kind | class |
|---|---|---|---|
| `0x8AE54` | `FUN_0008A938` | **WRITE** | **the sit-8 arm bind (THE producer)** |
| `0x89DCF` | (no fn) | **WRITE** | phase-`0x1D` arm-8 no-winner rebind (§5) |
| `0x89871` | `FUN_00089868` (phase-`0x1D` head) | READ | the installer source `[ESP+8]` |
| `0x889A4` | `FUN_00088940` | READ | `[[actor]+0x826]` side, gated `[0x157A4C]==1` (FU-145) |
| `0x6E382` | `FUN_0006E330` | READ | `[[actor]+0x8E]>>24` code vs `0x20` |
| `0x87D0F` | (no fn) | READ | `[[actor]+…]`, gated `[0x157A4C]==1` else `[0x157A9F]` |
| `0x4C33D` | `FUN_0004C324` | READ | the set-piece side picker (`[0x157AC2]>=4` arm, §6) |
| `0x4B1E5` / `0x4B265` | `FUN_0004B100` | READ | period/situation reader |
| `0x4B713` | `FUN_0004B6FC` | READ | reader |
| `0x4B7E4` | `FUN_0004B7D0` | READ | reader |
| `0x4BD4C` / `0x4BD7B` | `FUN_0004BD38` | READ | reader |
| `0x4BE34` | `FUN_0004BE30` | READ | reader |

Only **two** writes exist. The `0x6E382` reader is the direct confirmation that
the row-`0x20` install lands on this exact record (fresh
`disassemble_bytes 0x6E382`: `MOV EAX,[0x1587D4]; TEST EAX,EAX; JZ …;
EAX=[[actor]+0x8E]>>24; CMP EAX,0x20` — "does the actor carry code `0x20`?").
The `0x889A4` reader (fresh) is the FU-145 goal-side source:
`if [0x157A49]>>24 == 1 { actor=[0x1587D4]; side=[[actor]+0x826] }` — the
`[0x157A4C]==1` gate that only the sit-8 re-init turns on (§3).

---

## 3. The producer — the sit-8 arm `0x8AE19..0x8AE54` in `FUN_0008A938`

### 3.1 Byte-level (`disassemble_bytes 0x8AE08`, 160 B, fresh)

```
0x8AE08  MOV EAX,[0x157A49]                ; a1497a1500
0x8AE0D  SAR EAX,0x18                      ; = byte 0x157A4C (the bind flag)
0x8AE10  CMP EAX,0x1 ; 0x8AE13 JZ 0x8AEAE  ; flag == 1 -> SKIP the whole block
0x8AE19  MOV CL,0x1
0x8AE1B  MOV AL,[ESP]                      ; AL = the SIT-8 SIDE arg (param_3, saved at 0x8A940)
0x8AE1E  MOV EDX,0xA                        ; start index 0xA
0x8AE23  MOV [0x157AAF],AL                  ; store side
0x8AE28  XOR EAX,EAX                        ; EAX = 0 (side 0)
0x8AE2A  MOV byte[0x157A4C],CL              ; *** flag := 1 ***
0x8AE30  CALL 0x897F4                       ; FUN_000897F4(side=0, start=0xA) -> [0x1587CC]
0x8AE35  MOV EDX,0xA
0x8AE3A  MOV EAX,1                          ; side 1
0x8AE3F  CALL 0x897F4                       ; FUN_000897F4(side=1, start=0xA) -> [0x1587D0]
0x8AE44  MOV EAX,[ESP-0x2]                  ; (= [ESP-2], bytes 8b4424fe)
0x8AE48  SAR EAX,0x10                       ; >>16 -> the SIDE word (see note)
0x8AE4B  MOV EAX,[EAX*4+0x1587CC]           ; 8b0485cc871500  *** the per-side actor ***
0x8AE52  XOR CH,CH
0x8AE54  MOV [0x1587D4],EAX                 ; a3d4871500      *** THE BIND ***
0x8AE59..0x8AE6B  clear bytes 0x1587E8/0x1587E7/0x1587D9/0x1587D8
0x8AE71  MOV AL,[ESP] ; 0x8AE74 XOR CL,CL ; 0x8AE76 MOV [0x1587E6],AL   ; side state
             … (the arm-8 selection-state reset loop, FU194-L7)
0x8AE9E  XOR EBX,EBX ; word[0x157AC7]=0 ; word[0x157AC5]=0   ; clear the score pair
0x8AEAE  MOV EBX,0x1                        ; invoke-now
0x8AEB3  MOV EAX,0x7 ; 0x8AEB8 XOR EDX,EDX ; 0x8AEBA CALL 0x888FC   ; ACT 7
```

**The `[ESP]` / `[ESP-2]` alias.** The dispatcher head `0x8A940 MOV word[ESP],DX`
saves the side (param_3) over the low word of the frame's return slot; so
`[ESP]` = side and the dword `[ESP-2] = [ESP-2..ESP+1]` has the side in its
**high** word, which `SAR EAX,0x10` extracts. Both reads therefore yield the
same sit-8 side → `[0x1587D4] = [0x1587CC + side*4]`. (Byte `8b4424fe` = the
`[ESP-2]` load; trap applied.)

### 3.2 The per-side source selector `FUN_000897F4` (fresh, `0x897F4..0x8983C`)

Fresh `get_function_by_address 0x897F4` = `FUN_000897F4(short param_1, short
param_2)` (37 instrs). Fresh `disassemble_bytes 0x897F4`:

```
0x897F7  ESI = EAX (param_1) ; 0x897F9 EBX = MOVSX SI   ; EBX = side
0x897FC  TEST DX,DX ; JZ 0x89809                        ; DX==0 -> EDX=1
0x89801  EAX = MOVSX DX ; 0x89804 CMP EAX,0xB ; JL 0x8980E
0x89809  MOV EDX,1                                       ; clamp: start index in 1..10
0x8980E  ECX = 0x1588A4 + side*0x835                     ; team base
0x8981B  EAX = MOVSX DX ; *0xB2 ; ADD EAX,ECX            ; rec = base + index*0xB2
0x89826  CMP byte[EAX+0x9A],0 ; 0x8982D JZ 0x89832       ; free record -> store
0x8982F  INC EDX ; 0x89830 JMP 0x897FC                   ; else next (wraps 11 -> 1)
0x89832  MOV [EBX*4 + 0x1587CC],EAX                      ; *** table[side] = rec ***
```

So `FUN_000897F4(side, start)` scans the team's records from index `start`
(clamped to `1..10`; `0` and `≥0xB` become `1`) and, in the order
`start, start+1, …, 10, 1, …, start-1` (it wraps to 1 when the index reaches
`0xB`), stores the first record with `byte[rec+0x9A] == 0` into
`[0x1587CC + side*4]`. **Index 0 is never scanned** — the keeper/record-0 slot
is excluded (the `SKIP_9A` field doc: `+0x9A` = "exclusion (occupied record)").
The engine field is `skip_9a` (`fifa96_match_entities.h:90`), so the actor is
always a **non-occupied outfield record** (`index != 0`).

Its **three** fresh callers (`get_xrefs_to 0x897F4` = 3):

| site | fn | args |
|---|---|---|
| `0x8AE30` | `FUN_0008A938` (sit-8 arm) | `side 0`, `start 0xA` |
| `0x8AE3F` | `FUN_0008A938` (sit-8 arm) | `side 1`, `start 0xA` |
| `0x89D9F` | phase-`0x1D` arm 8 (no-winner rebind, §5) | `side = [team+0x826]`, `start = byte[actor+0x8D]-1` |

### 3.3 The exact stored value

`[0x1587D4] = [0x1587CC + side*4]` where `[0x1587CC + side*4]` is the *record
pointer* selected by §3.2 (`team_base(side) + index*0xB2`, `index ∈ 1..10`,
`byte[rec+0x9A]==0`). It is **not** the team base and **not** record 0. This is
the record the phase-`0x1D` arm-0 `0x899A2` then passes to
`FUN_0007D9A4(rec, code=0x20, stage=0, invoke=0)` (FU-210 §2/§3).

---

## 4. The gate

The bind block is entered **iff `byte[0x157A4C] != 1`** (`0x8AE08..0x8AE13`).
`[0x157A4C]` is the "actor already bound / set-piece in progress" latch:

* **Fresh `get_xrefs_to 0x157A4C` = 7.** Writes: `0x8AE2A` (`FUN_0008A938`,
  the sit-8 arm, `:= 1`), `0x89E9A` (phase-`0x1D` arm 9 terminal, `:= 0`,
  FU-209 §2.10), `0x73E42` (`FUN_00073E28`), `0x73D2C` (`FUN_00073D10`).
  Reads: `0x765AC`, `0x741D7`, the `0x73D10` DATA ref, plus the engine-side
  consumers below.
* So on the **first** sit-8 in a set-piece sequence: `[0x157A4C] == 0` → the
  block runs (binds the actor, sets the latch). On a **repeat** sit-8 while the
  latch is set, the block is skipped and act 7 is invoked directly (`0x8AEAE`).
  The latch is cleared by the act-7 terminal arm 9 (`0x89E9A`).

The arm also re-asserts the **side** (`[0x157AAF] = side`, `0x8AE23`) and resets
the arm-8 selection state (`0x1587E6..E9`/`0x1587D8/9`, §3.1) — the actor and
the selection state are bound together.

---

## 5. The second writer — the phase-`0x1D` arm-8 no-winner rebind (`0x89DCF`)

The phase-`0x1D` handler `0x89868` (body `0x89868..0x89EAF`) has a second
`[0x1587D4]` write inside arm 8 (the `0x89BB6..0x89E6D` selection block,
FU-209 §2.9 / FU194-L7). Fresh `disassemble_bytes 0x89D30` (176 B):

```
0x89D79  TEST DI,DI ; 0x89D7C JGE 0x89DDE        ; DI >= 0 -> the winner path
0x89D7E  EAX=[ESP+8] (actor) ; AX = MOVSX byte[EAX+0x8D] ; DEC EAX
0x89D8B  EDX = (n-1)
0x89D8E  EAX=[ESP+8] ; EAX=[EAX] ; AL=byte[team+0x826]   ; the actor's team side
0x89D9F  CALL 0x897F4                            ; FUN_000897F4(side, start=n-1)
0x89DA4..0x89DB0  the same team byte ; XOR AL,0x1        ; side ^ 1
0x89DB2  AND EAX,0xFF ; 0x89DB7 DL=0 ; XOR ECX,ECX
0x89DBB  MOV byte[0x158829],DL                   ; stage := 0 (restart)
0x89DC1  MOV EAX,[EAX*4+0x1587CC]                ; *** table[side^1] ***
0x89DC8  MOV word[0x158818],CX                   ; timer := 0
0x89DCF  MOV [0x1587D4],EAX                      ; *** re-bind to the OTHER side ***
0x89DD4  ADD ESP,0x14 ; POP…; RET                ; epilogue
```

When the arm-8 selection finds no winner (`DI < 0`), act 7 **re-binds the actor
to the opposite team's free record** (`[0x1587CC + (side^1)*4]`, re-selected by
the same `FUN_000897F4` with a start index of `byte[actor+0x8D]-1`) and resets
the stage to 0. This is the only other writer; it is part of the unported arm-8
selection block (FU194-L7 / FU-209 §2.9), not a separate producer.

---

## 6. Why the phase-19 path never reached the cell

Two independent reasons, both fresh:

1. **The producer is sit-8-only (native).** `FUN_000897F4` is called only from
   the sit-8 arm (`0x8AE30`/`0x8AE3F`) and the phase-`0x1D` arm-8 rebind
   (`0x89D9F`); nothing else populates `[0x1587CC]`/`[0x1587D0]`/`[0x1587D4]`.
   Sit 8 is itself reached only via the extra-time selector (FU-208 §4:
   `[0x157AC2] == 4` at a phase-`0x0C` period-end, or the bit-8 arm).
2. **The engine has no model of the re-init block.** The engine's sit-8 route
   (`fifa96_match_run_set_piece` → `match_run_situation_side`, FU-208 §6) uses
   the situation table (`fifa96_action_phase_situations[8] = {NONE, 7, 0}`,
   `fifa96_action_handlers.c:1142`) and dispatches act 7 — but it never runs the
   `0x8AE19..0x8AE54` re-init. So `mr->actor_1587d4` is staged
   `FIFA96_MATCH_ENTITY_NONE` at reset/begin (`fifa96_match_run.c:2036`,
   `:2156`) and **no engine source ever sets `mr->global_157a4c = 1`** (fresh
   grep: it is only assigned `0` at `:2041`/`:2159`/`:6132`). With the cell
   `NONE`, the ported phase-`0x1D` arm-0 install is a null no-op
   (`fifa96_match_run.c:6110..6118`), so no record carries code `0x20`.

The phase-`0x1D` **row** is nevertheless COUNTED (T2, +1 → `52/80`): the
period-index roll (T1/FU-213) reaches `global_157ac2 == 4` at an
extra-time-entering completion and `phase_drive` runs `period_setup(mode 0)`
(`fifa96_match_run.c:2650..2653`), which dispatches sit 8. So the act-7 route
is live; only the actor cell is missing.

---

## 7. Row-`0x20` reachability — is it live or tape-dormant?

### 7.1 The chain, with the bind ported

```
period-index roll == 4  (phase_drive, match_run.c:2650)
  -> period_setup(mode 0)  (match_run.c:3161)  ->  set_piece(8, side, 0)
  -> [NEW] sit-8 re-init: global_157a4c := 1 ; actor_1587d4 := free_record(side)
  -> match_run_situation_side(8) -> act_dispatch(7, 0, deferred)  (match_run.c:2889)
  -> clock-tail act tick -> phase 0x1D arm 0 (match_run.c:6086)
  -> entities_install(actor, 0x20, 0)  (match_run.c:6117)
  -> outfield record machine  (match_run.c:1394 `if (e->index != 0u)`) dispatches
     fifa96_match_action_20  (match_run.c:1410)
```

Every link is already engine-live **except** the sit-8 re-init. With the bind
ported, the actor is a real non-occupied outfield record (`index != 0`, so the
outfield machine actioned it — `match_run.c:1394`; `skip_9a == 0`, so
`fifa96_match_entities_install` succeeds — `fifa96_match_entities.c:76`), so
the row-`0x20` body is genuinely dispatched.

### 7.2 The two verdicts

* **Live (extra-time path): YES.** With the bind ported and the row slot wired,
  row `0x20` installs and runs on the engine's extra-time path — the same
  producer standard that made phase `0x1D` countable (T2/FU-208/FU-213).
  This resolves FU210-L5's condition ("row `0x20` stays uncounted until the
  coupled installer + sit-8 producer land").
* **Tape (M2): byte-identical.** The M2 tape reaches **no** `global_157ac2 == 4`
  completion (FU-208 §7 + FU-210 §7.4: M2 `441415fc…` unchanged by the T2/T3
  landings), so sit 8 never fires, the re-init never runs, and the cell stays
  `NONE`; `fifa96_match_action_20` is dispatched **0 times**. M1 `09b726b7…`
  unmoved. No re-pin.

### 7.3 Census

Given the row slot is flipped (`fifa96_match_action_table[0x20].fn: NULL →
fifa96_match_action_20`, `fifa96_match_handlers.c:5149`) together with the
ported bind, the row is a genuine install-live producer → **census `+1`
(action `0x20` → `53/80 = 66%`)**, matching FU-208 §6.2's combined "+2 (phase
`0x1D` + action `0x20`)" (phase `0x1D` already landed `+1` at T2). This slice
makes **no engine change**; the verified count is unchanged.

---

## 8. The engine extension point (exact)

**File / region:** `src/fifa96_engine/fifa96_match_run.c`, the sit-8 re-init in
`fifa96_match_run_set_piece`'s **direct-path BX==0** branch — insert immediately
before the terminal `return match_run_situation_side(mr, situation, side, 1);`
at **`match_run.c:3133`** (after the sit-2..4 formation arm `:3121` and the
sit-6 fallback `:3132`). It must also **not** run on the queue path
(`:3065..3090`), which is native-exact (the queue arm `0x8AA60` never touches
the cell).

**What to add:**

1. **A free-record selector helper** mirroring `FUN_000897F4` — e.g.
   `static int32_t match_run_free_record(const struct fifa96_match_run *mr,
   uint32_t team, int start)` near `match_run_phase_pick` (`match_run.c:2696`):
   scan indices in the order `start, start+1, …, 10, 1, …, start-1` (clamp
   `start` to `1..10`; index `0` never scanned), return the pool id
   `team*FIFA96_MATCH_ENTITY_RECORDS + index` of the first record with
   `skip_9a == 0`. (Native stores the pointer; the engine pool id is the
   established `actor_1587d4` convention — `fifa96_match_run.h:633`.)
2. **The re-init block** in `set_piece` direct path:

   ```
   if (situation == 8u && mr->global_157a4c != 1u) {
     mr->global_157a4c = 1u;                              /* 0x8AE2A */
     /* 0x8AE23 [0x157AAF] = side — the engine has no global_157aaf cell
        (only comments); model it or fold into the actor bind (leg FU217-L5). */
     (void)match_run_free_record(mr, 0u, 0xA);            /* 0x8AE30 (side 0) */
     (void)match_run_free_record(mr, 1u, 0xA);            /* 0x8AE3F (side 1) */
     mr->actor_1587d4 = match_run_free_record(mr, side, 0xA); /* 0x8AE4B/54 */
   }
   ```

   (Optionally also model the per-side cells `0x1587CC`/`0x1587D0` so the
   phase-`0x1D` arm-8 second writer (`0x89DCF`) can reuse them — leg FU217-L3.)
3. **Flip the row slot:** `fifa96_match_handlers.c:5149` `{0x20, NULL, …}` →
   `{0x20, fifa96_match_action_20, …}` (the body already exists; FU-210 landing
   T3).

**Fixture:** `tests/test_engine_m2.c` (`:3599..` the period-4 selector block) /
`tests/test_engine_match_act.c`: after `fifa96_match_run_init`, set
`mr.state.period = 4`, `phase = 2`, `clock_period_ended = 1`,
`side_controlled = 0`, then `fifa96_match_run_phase_drive(&mr) == 1`; assert
`mr.global_157a4c == 1`, `mr.actor_1587d4 == (team side)*11 + <first free
record≥1>`, then `fifa96_match_act_tick` → arm 0 installs code `0x20` on that
record (deferred, `stage92 == 0`), and a subsequent per-record walk dispatches
`fifa96_match_action_20`. The current fixture
(`tests/test_engine_m2.c:3662`) hand-sets `actor_1587d4 = 1`; the real producer
replaces that hand-set.

---

## 9. Numbered open legs

1. **FU217-L1 — the `FUN_000897F4` scan exactness + the `+0x9A` occupancy
   source.** The wrap order (`start, …, 10, 1, …`) and the "no free record ⇒
   the loop cycles `1..10` forever" behaviour are byte-derived but not
   field-verified; and *who* sets `byte[rec+0x9A]` (the "occupied" mark) is not
   traced here. The engine's `skip_9a` mirrors the bit (FU-206/FU-210 §4), but
   the native writer that *occupies* a record is a separate producer.
2. **FU217-L2 — the phase-`0x1D` arm-8 second writer (`0x89DCF`).** The
   no-winner rebind (`[0x1587CC+(side^1)*4]`, start index `byte[actor+0x8D]-1`)
   sits inside the unported arm-8 selection block (`0x89BB6..0x89E6D`,
   FU194-L7 / FU-209 §2.9); its semantics (`DI < 0` = "no winner") are cited,
   not resolved.
3. **FU217-L3 — the engine model of the per-side table `0x1587CC`/`0x1587D0`.**
   The engine currently carries only the flat `actor_1587d4`; the native keeps
   two per-side cells plus the active cell. Whether the port needs the two
   cells (for the arm-8 rebind, FU217-L2) or only the active cell is an
   engineering choice; the native layout is byte-derived here.
4. **FU217-L4 — the `FUN_000886D4` dead `MOV EAX,0x1587CC`.** Fresh
   `disassemble_bytes 0x887E1` + `0x887FE..0x8885F` shows `MOV EAX,0x1587CC` at
   `0x887E1` with no intervening call/consumer before EAX is recomputed
   (`0x8883C`); the xref is typed DATA/PARAM but no reader resolves. Flagged,
   not asserted (a possible Ghidra typing artifact).
5. **FU217-L5 — the `[0x157AAF]` side store.** The sit-8 arm writes
   `[0x157AAF] = side` (`0x8AE23`); the engine has no `global_157aaf` cell
   (only comments at `fifa96_match_handlers.c:1623/1911/4299`,
   `fifa96_match_run.c:5690/5765`), so the re-init must fold it or add a cell.
   Coupled with the `global_157a4c` latch model.
6. **FU217-L6 — the act-7 invoke cadence.** Native sit 8 invokes act 7
   **now** (`0x8AEAE MOV EBX,1` → `0x8AEBA CALL 0x888FC`); the engine defers
   (`match_run.c:2889`, `invoke = (out.act == 2u)`). Carried as FU208-L4; it
   shifts the arm-0 install by one granted frame but does not change the
   row-`0x20` reachability (the clock-tail tick still runs arm 0).
7. **FU217-L7 — the FU145-L4 side stand-in.** `FUN_00088940`'s `[0x157A4C]==1`
   goal-side source (`0x889A4`) and the ball-team source `[0x1577CA]` still back
   onto unported cells; only the *record identity* is now derived (this slice).
   The goal-scan stand-in (snapshot sign) remains until a live pan source lands
   (FU145-L4).

---

## 10. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, `min_address 0x10000`, `max_address .image::00174a2a`,
  2706 functions, `.image` overlay.
* `read_memory` — `0x1587CC` (32 B, the 3-dword neighbourhood, all-zero),
  `0x1587D4` (4 B, the cell), `0x110760` (4 B, action slot `0x20` = `0x84EEC`).
* `disassemble_bytes` — `0x8AE08` (160 B, the sit-8 arm + the bind),
  `0x8AE4B` (64 B), `0x8AE9E` (24 B, the invoke-now tail), `0x897F4` (120 B, the
  full selector), `0x89868` (40 B, the phase-`0x1D` head), `0x89D30` (176 B, the
  arm-8 no-winner rebind), `0x87CF0` (48 B), `0x887FE` (104 B), `0x88990`
  (40 B), `0x6E330` (40 B) + `0x6E360` (64 B), `0x4C324` (80 B), `0x4B1D0`
  (32 B).
* `get_xrefs_to` — `0x1587D4` (**14**: 12 READ, 2 WRITE), `0x1587CC` (**4**:
  `0x89832` WRITE, `0x8AE4B`/`0x89DC1` READ/DATA, `0x887E1` PARAM), `0x1587D0`
  (**0**), `0x897F4` (**3**), `0x157A4C` (**7**), `0x157AAF` (**11**).
* `get_function_by_address` — `0x8AE4B` → `FUN_0008A938` (`0x8A938..0x8AF25`),
  `0x897F4` → `FUN_000897F4`, `0x4B100` → `FUN_0004B100`, `0x4C324` →
  `FUN_0004C324`, `0x88940` → `FUN_00088940` (`0x88940..0x88C0E`),
  `0x6E330` → `FUN_0006E330`, `0x886D4` → `FUN_000886D4`; `0x87D0F`, `0x89DCF`
  → "no function found".

Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_run.c` (`:1394`/`:1405..1410` the outfield
record machine; `:1607` `stage.hidden = skip_9a`; `:2036`/`:2156` the
`actor_1587d4` reset; `:2041`/`:2159` `global_157a4c` reset; `:2494` the
`skip_9a` walk skip; `:2650..2653` the period-4 `phase_drive` producer; `:2696`
`match_run_phase_pick`; `:2847..2894` `match_run_situation_side`; `:3059..3134`
`set_piece` (the queue guard `:3065`, the formation arm `:3121`, the terminal
return `:3133`); `:3145..3181` `period_setup` (the `:3161` mode-0 sit-8, the
`:3175` bit-8 gate); `:6082..6145` `fifa96_match_action_phase_1D` (arm 0, the
`:6110..6118` gated install));
`src/fifa96_engine/fifa96_match_handlers.c` (`:5149` the row-`0x20` NULL slot;
`:5150` the FU-210 note; `:3572`/`:3752`/`:3985`/`:4893` the `global_157a4c`
consumers);
`src/fifa96_engine/fifa96_match_entities.c` (`:76` the `skip_9a` install
guard);
`include/fifa96_engine/fifa96_match_run.h` (`:633` `actor_1587d4`, `:662`
`global_157a4c`); `include/fifa96_engine/fifa96_match_entities.h` (`:63` the
`index` field, `:90` the `skip_9a` field, `:44/45` the team/record counts);
`src/fifa96_loader/fifa96_action_handlers.c` (`:1142` the sit-8 situation row);
`tests/test_engine_m2.c` (`:3599..3668` the period-4/sit-8/phase-`0x1D`/actor
fixtures).

Docs cross-cited: FU-82 (§2.1/§3.7), FU-145 (§4/L4, the `[0x1587D4]` identity),
FU-189 (§3, the row-0x20 installer hunt), FU-194 (§3.2/§3.4/§6.1/L7/L8),
FU-196 (§1/§2), FU-205 (§2/§3), FU-208 (§1/§2/§3/§4/§6/§7), FU-209 (§1.5/§2/§3),
FU-210 (§2/§3/§7.3/§8), FU-213, ENGINE.md census line, AGENTS.md.

No writes beyond this slice file; no rename/label/comment/function/script/
project save; no git.

---

## Erratum (append)

**FU210-L3 / FU194-L8 / FU145-L4 are RESOLVED by the M2 phase-20 T3 bind.**
This slice's §1 records the producer as derived-but-unported ("**Closes:** FU210-L3
… FU194-L8 … FU145-L4") and §8 gives the extension point as an exact plan. The T3
landing (`feat(engine,fu217): the sit-8 actor-cell bind + action row 0x20 —
55/80 = 68%`) ports it exactly as §8 prescribes: `match_run_free_record` (the
native `FUN_000897F4` wrap scan `start, …, 10, 1, …, start-1`, index 0 never
scanned), the sit-8 re-init block in `fifa96_match_run_set_piece`'s **direct**
path (guarded on `global_157a4c != 1`; the queue path untouched), the per-side
`actor_1587cc[2]` cells, and the `{0x20, NULL} → fifa96_match_action_20` slot
flip. The actor cell is now **live** on the extra-time path — a real period-4
completion binds it and the coupled row-`0x20` install dispatches — so
**FU210-L3 / FU194-L8 / FU145-L4 are resolved**, not open. **Comment correction
(no rewrite):** §6.2's "no engine source ever sets `mr->global_157a4c = 1`" is
now false — the T3 re-init sets it. The remaining legs are **FU217-L1** (the
`FUN_000897F4` scan exactness + the `+0x9A` occupancy source), **FU217-L2** (the
phase-`0x1D` arm-8 no-winner rebind `0x89DCF`), **FU217-L3** (the per-side cells'
consumer) and **FU217-L5** (the `[0x157AAF]` side store — no engine cell) —
registered with native owners in `docs/ENGINE.md`'s phase-20 leg register. Prior
sections are not rewritten.
