# FU-226 — the sit-8 RNG draw: null-backend determinism + the `[0x14C32A]` session gate's live value (phase-21 recon W21-4)

Provenance: M2 phase-21 recon wave **W21-4**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`,
max `.image::00174a2a`, **2706** functions, **15239** symbols, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `get_xrefs_to` / `get_function_by_address`
of **this** session unless marked *cross-cited* (then the frozen FU doc is
named). The alias traps are applied (`dword[x]>>16` = the word at `x+2`,
`>>24` = the byte at `x+3`); LE globals sit at `+0x100000` from the file offset;
addresses are `/FIFA96.EXE` runtime addresses. Engine/loader source read
first-hand. **No writes**: no rename, comment, label, function, script or project
save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (W21-4 brief): prove whether the sit-8 route's single RNG draw is
deterministic under the engine's **null backend** — the **T1 determinism gate**
precondition (`docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md`
§5 T1, §7 risk 1) — and pin the session gate `[0x14C32A]`'s live value at a fresh
match begin and whether the sit-8 route is gated on it (**FU208-L8 / FU218-L3**).
Extends FU-208 (§2/§4/§7 L8), FU-215, FU-216, FU-217, FU-218 (§1/§2/§5), FU-141.
Step 1 reads the sit-8 arm `FUN_0008A938` `0x8AE08..0x8AE54` (the bind; phase-20 T3)
and the RNG side `0x888C8`; Step 2 the engine RNG + `platform_null.c`; Step 3 the
session gate; Step 4 the verdict.

**Answer in one line.** Within `FUN_00088860`'s `phase == 0xC && [0x157AC2] == 4`
branch, the sit-8 route consumes **one** RNG draw — the `FUN_00092AC8` call at
`0x888B5`, taken **unconditionally** inside that branch, **before** the
session-gated dispatcher `0x888C8`; the dispatched arm `0x8AE08..0x8AE54` itself
contains **no** RNG call (so the "one draw" is the *route's*, not the helper's —
`FUN_00092AC8` is a 186-caller shared helper, §1.3). In the engine that draw is
`fifa96_rng_step(&mr->rng, &r)` (`fifa96_match_run.c:3202`), and the entire RNG is
a **constant-seeded, pure integer** generator: the sole production seed is the
literal `0` (`fifa96_rng_seed(&mr->rng, 0)`, `fifa96_match_run.c:2151`), the step
is closed-form 16-bit arithmetic (`src/fifa96_loader/fifa96_rng.c:11/20`), and the
null backend supplies **no entropy** (scripted tape `n_poll`, step-counted
`n_now_ns`, no-op `n_sleep_ns`; `src/fifa96_engine/platform_null.c:25/56/60`).
**Therefore the draw is deterministic (byte-stable) → the T1 gate will not stop
on it**; a tape that fires the route yields a *reproducible* new golden (an
earned re-pin), never nondeterminism. The session gate `[0x14C32A]` is **`0`** at
a fresh ordinary match begin and the sit-8 route **is** gated on it: `== 0` opens
the dispatcher's **direct** path (`0x8A957 CMP …JZ 0x8AA7B` → the table-2 arm
`0x8AE08` → act 7 invoke-now), `!= 0` (the single-team **Practise** mode-4
session) queues it. The engine seeds 0 and models the direct path.

---

## 1. The sit-8 route and its single RNG draw (native, fresh)

### 1.1 The producer-caller `FUN_00088860` — the draw is before the dispatch

Fresh `disassemble_bytes 0x888A0` (80 B) — the period-end body (callers *cross-cited*
FU-215 §3.2: `0x4B08E` and `0x74244`):

```
0x888AB MOV AL,[0x00157AC2]         ; a0 c2 7a 15 00   period index
0x888B0 CMP EAX,0x4                 ; 83 f8 04
0x888B3 JNZ 0x000888E2              ; 75 2d              != 4 -> situation 1
0x888B5 CALL 0x00092AC8             ; e8 0e a2 00 00   <<< THE RNG DRAW (FUN_00092AC8)
0x888BA XOR AH,AH                   ; 30 e4
0x888BC AND AL,0x1                  ; 24 01              side = rng & 1
0x888BE XOR EBX,EBX                 ; 31 db              BX = 0
0x888C0 MOVSX EDX,AX                ; 0f bf d0
0x888C3 MOV EAX,0x8                 ; b8 08 00 00 00     situation 8
0x888C8 CALL 0x0008A938             ; e8 6b 20 00 00   <<< SITUATION 8, side rng&1, BX 0
0x888CD MOV EBX,0x8 ; MOV EAX,0xC1 ; XOR EDX,EDX ; CALL 0x8F188   ; the 0xC1/8 sink
0x888E2 MOV EAX,0x1 ; EDX=[0x157AAB]>>24 ; CALL 0x8A938          ; else SITUATION 1
```

The draw at `0x888B5` sits **inside** the `[0x157AC2] == 4` gate and **before**
the `0x888C8` dispatch: it is consumed whenever the branch is entered,
**independent of** the session-gate outcome inside `FUN_0008A938` (§3.2). Its
return value is used **only** as the side (`rng & 1`).

### 1.2 The dispatcher `FUN_0008A938` and the sit-8 arm `0x8AE08..0x8AE54` — no RNG

Fresh `disassemble_bytes 0x8A938` (200 B) — the dispatcher head:

```
0x8A93E MOV ECX,EAX                 ; 89 c1              ECX = situation
0x8A940 MOV word[ESP],DX            ; 66 89 14 24        save side
0x8A944 TEST AX,AX / JZ 0x8AA7B      ; sit 0 -> direct
0x8A94E CMP EAX,0xB / JZ 0x8AA7B     ; sit 0xB -> direct
0x8A957 CMP byte[0x0014C32A],0x0    ; 80 3d 2a c3 14 00 00   <<< SESSION GATE
0x8A95E JZ  0x0008AA7B              ; 0f 84 17 01 00 00  gate == 0 -> DIRECT
0x8A964 CMP dword[0x0015B6C0],0x0   ; 83 3d c0 b6 15 00 00   pending
0x8A96B JNZ 0x0008AA7B              ; 0f 85 0a 01 00 00  else DIRECT
0x8A971 … (the queue path: [0x15B6B8]=side, table-1 0x8A8E0, latch + RET)
0x8A996 JMP dword CS:[EAX*4+0x8A8E0]; 2e ff 24 85 e0 a8 08 00  the queue table
```

Fresh `disassemble_bytes 0x8AE00` (100 B) + `0x8AE60` (120 B) — the arm-8 body
(`0x8A904[8] = 0x8AE08`, *cross-cited* FU-208 §1.2 / FU-218 §2.2) reached on the
**direct** path (`0x8AA7B` BX=0 → `0x8AAA8` → `0x8AB63` → table-2):

```
0x8AE08 MOV EAX,[0x00157A49]        ; a1 49 7a 15 00    the [0x157A4C] bind-flag word
0x8AE0D SAR EAX,0x18               ; c1 f8 18          = byte 0x157A4C
0x8AE10 CMP EAX,0x1 / JZ 0x8AEAE    ; 83 f8 01 / 0f 84 95 00 00 00  flag==1 -> skip re-init
0x8AE19 MOV CL,0x1 ; AL=[ESP](side) ; MOV [0x157AAF],AL ; MOV byte[0x157A4C],CL
0x8AE30 CALL 0x897F4(EAX=0, EDX=0xA)   ; e8 bf e9 ff ff   side 0 free record
0x8AE3F CALL 0x897F4(EAX=1, EDX=0xA)   ; e8 b0 e9 ff ff   side 1 free record
0x8AE4B MOV EAX,[EAX*4+0x001587CC] ; 8b 04 85 cc 87 15 00  per-side actor
0x8AE54 MOV [0x001587D4],EAX       ; a3 d4 87 15 00     <<< THE BIND
0x8AE9E XOR EBX,EBX ; word[0x157AC7]=BX ; word[0x157AC5]=BX   ; clear score pair
0x8AEAE MOV EBX,0x1                ; bb 01 00 00 00     invoke-now
0x8AEB3 MOV EAX,0x7                ; b8 07 00 00 00     ACT 7
0x8AEB8 XOR EDX,EDX                ; 31 d2             side 0
0x8AEBA CALL 0x000888FC            ; e8 3d da ff ff     the act seam
```

**The whole sit-8 arm (`0x8AE08..0x8AE54`, the phase-20 T3 bind) contains no RNG
call.** Its actor selector `FUN_000897F4` is a deterministic wrap-scan
(*cross-cited* FU-217 §3.2 — index wrap `start..10,1..start-1`, index 0 never
scanned, no RNG). The route's **only** RNG consumption is the single `0x888B5`
draw of §1.1.

### 1.3 The generator `FUN_00092AC8` (the drawn cell)

Fresh `disassemble_bytes 0x92AC8` (80 B) + `0x92B17` (80 B) + `0x92B66` (48 B):
a six-word additive generator with a carry chain, state dwords at
`0x110F44/0x110F48/0x110F4C/0x110F50/0x110F54/0x110F58` (loaded at
`0x92ACD MOV EAX,[0x110F58]`, `0x92AD2 [0x110F54]`, `0x92ADC [0x110F50]`,
`0x92AF5 [0x110F4C]`, `0x92B0E [0x110F48]`, `0x92B27 [0x110F44]`), returning the
new low word (`w[0]`). It reads/writes **only** those six cells — **no I/O, no
external input** (*cross-cited* FU-141 / `include/fifa96_loader/fifa96_rng.h:1-9`,
the same generator and cell set). It is a **widely shared** RNG helper — a fresh
`get_xrefs_to 0x92AC8` = **186** callers program-wide (e.g. `FUN_0007B194` ×23,
`FUN_0007F7E0` ×9, `FUN_00072478` ×7, `FUN_0008776C` ×7, `FUN_0008D098` ×5,
`FUN_000736AC` ×4; the sit-8 `0x888B5` is **one** of them). The sit-8 route
invokes it **once**, at `0x888B5` (§1.1) — do **not** claim uniqueness for the
helper.

---

## 2. The engine RNG and the null-backend determinism source

### 2.1 The engine RNG is pure

`src/fifa96_loader/fifa96_rng.c` — fresh first-hand:

* `fifa96_rng_seed` (`:11..18`): `w[i] = (int8)fifa96_rng_seed_bytes[i] + (seed << 25)`
  for the six-word state, from a **compile-time** 6-byte table
  (`fifa96_rng_seed_bytes` = the first six bytes of `"ArCaDe"`, `:7..9`). No I/O,
  no time, no platform call.
* `fifa96_rng_step` (`:20..57`): a closed-form six-word additive step with a
  carry chain; reads/writes only `rng->w[0..5]`; returns `*value = (uint16_t)w[0]`.
  No I/O.

State shape: `struct fifa96_rng { uint32_t w[6]; }`
(`include/fifa96_loader/fifa96_rng.h:16..18`) — the six native cells of §1.3.

### 2.2 The sole production seed is the literal `0`

`src/fifa96_engine/fifa96_match_run.c:2148..2151` (fresh first-hand):

```c
/* FU-142d: a fresh match seeds the RNG (the native FUN_000493A0 match-init
 * seed call 0x493F2) with the derived seed 0 … */
(void)fifa96_rng_seed(&mr->rng, 0);
```

A fresh match-init always seeds with the **constant `0`** — the value does not
depend on the clock, the platform, or any input. A source-wide grep for
`fifa96_rng_seed` finds this as the **only** production seed call (the rest are
tests). So the engine's RNG stream is a fixed function of the number of prior
draws.

### 2.3 The null backend supplies no entropy

`src/fifa96_engine/platform_null.c` — fresh first-hand:

* `n_poll` (`:25..34`): returns **one** scripted `cfg.tape[tape_pos++]` entry per
  call (the deterministic input tape).
* `n_now_ns` (`:56..59`): `s->cfg.step_ns * (s->st.presents + 1u)` — a
  **step-counted virtual clock**, not the host clock.
* `n_sleep_ns` (`:60`): a no-op.
* `n_present` (`:35..46`): FNV-1a over the frame planes/palette — a deterministic
  hash.
* No `rand`, `time`, `gettimeofday`, `clock_gettime` or entropy source anywhere
  in the file.

The only consumer of `now_ns` is the fixed-step clock
(`src/fifa96_engine/fifa96_engine.c:146`/`:155..157`); it is **not** wired to the
RNG seed. So the null backend contributes **zero** nondeterminism to the RNG.

### 2.4 The engine's sit-8 draw site

`src/fifa96_engine/fifa96_match_run.c:3199..3203` (fresh first-hand), the
`fifa96_match_run_period_setup(mode 0)` arm (native `FUN_00088860`):

```c
if (mr->state.phase == 0x0Cu) {                 /* 0x88863 */
  if (mr->global_157ac2 == 4u) {                /* 0x888AB */
    uint16_t r = 0;
    (void)fifa96_rng_step(&mr->rng, &r);        /* FUN_00092AC8  @0x888B5 */
    (void)fifa96_match_run_set_piece(mr, 8u, (uint8_t)(r & 1u), 0u); /* 0x888C8 */
  } else { /* situation 1 */ }
}
```

reached from `fifa96_match_run_phase_drive` at
`fifa96_match_run.c:2643` (`mr->global_157ac2 = mr->state.period`, the `0x8B58A`
roll) and `:2654..2657` (`if (mr->global_157ac2 == 4u) period_setup(mr, 0)`).
Every predicate on the path (`state.phase`, `state.period`, the RNG words) is
deterministic state.

**§2 verdict.** The engine's sit-8 draw is `fifa96_rng_step` on a
constant-seeded pure generator, on a deterministic predicate, with no entropy
from the null backend → **deterministic and byte-stable** across replays.

---

## 3. The `[0x14C32A]` session gate (fresh)

### 3.1 The three writers — all front-end session entries

Fresh `get_xrefs_to 0x14C32A` = **60** refs, **3 WRITE / 57 READ** — matching
*FU-218 §1.1*. The writers, fresh first-hand:

| addr | fn | bytes | semantics |
|---|---|---|---|
| `0x1B7C8` | `FUN_0001B7B8` | `c6 05 2a c3 14 00 01` | `MOV byte[0x14C32A],1` (arm: `EAX == 4`) |
| `0x1B7D5` | `FUN_0001B7B8` | `88 25 2a c3 14 00` | `MOV byte[0x14C32A],AH` (AH=0; arm: `EAX != 4`) |
| `0x32DFF` | `FUN_00032DE0` | `c6 05 2a c3 14 00 01` | `MOV byte[0x14C32A],1` (result-code-4), then `0x32E0B CALL 0x1B7B8(4)` |

Fresh `disassemble_bytes 0x1B7B0` (48 B):
`0x1B7B9 MOV [0x14C1D0],AL; … 0x1B7C3 CMP EAX,4; 0x1B7C6 JNZ 0x1B7D1; 0x1B7C8 MOV
byte[0x14C32A],1` … `0x1B7D1 XOR AH,AH; 0x1B7D5 MOV byte[0x14C32A],AH; 0x1B7DB
MOV [0x15B680],EDX`. So `[0x14C32A] = ([0x14C1D0] == 4)` — set to `1` **only** by
the single-team **Practise** launcher, `0` for every other mode (agrees
*cross-cited* FU-218 §1.2 / FU-148 §18 / FU-155). **There is no in-match writer**
— the byte is session-constant.

### 3.2 The dispatcher gate and the period-end caller gate

* **Dispatcher** `0x8A957` (§1.2): `CMP byte[0x14C32A],0 / JZ 0x8AA7B`. The byte
  is tested **inside** `FUN_0008A938`, **after** the `0x888B5` RNG draw of §1.1 —
  so the draw is consumed regardless of the gate, and the gate then chooses
  direct (`==0`, arm `0x8AE08` runs) vs queue (`!=0`, table-1 latch only).
* **Period-end caller `0x74244` — a DEAD TWIN, not the live invocation** (fresh
  `disassemble_bytes 0x74218`, 64 B):
  `0x74237 CMP byte[0x14C32A],0 / 0x7423E JNZ 0x742E3 / 0x74244 CALL 0x88860`.
  When the gate is `!= 0` the caller **skips `FUN_00088860` entirely** (so on the
  `0x74244` path the `0x888B5` draw is *not* consumed). When `== 0` the caller
  runs the period-end body → the draw fires. `0x74237` is a fresh READ at
  `get_xrefs_to 0x14C32A`; the skip target `0x742E3` is the caller's joint tail.
  **Scope (FU225-L1):** this `0x74244` caller sits in the enclosing function
  `0x741C4`, which is **statically unreferenced** (fresh `get_xrefs_to 0x741C4` =
  0) — a dead twin of `FUN_0004B02C`. The **live** `FUN_00088860` call site is
  `0x4B08E` (inside `FUN_0004B02C`, fresh `get_xrefs_to 0x88860` = `{0x4B08E,
  0x74244}`), reached `FUN_000493A0 → FUN_0004B02C(mode 0)`, and it carries **no**
  `[0x14C32A]` gate. So the `[0x14C32A]==0` caller gate here is a **dead-twin-only**
  property; the live route reaches `FUN_00088860` ungated (FU225-L1 / §5.1).

### 3.3 The live value at a fresh match begin — `0`

The engine seeds the cell to `0` at fresh reset and at begin:
`fifa96_match_run.c:1772` (release), `:2073` (reset), `:2202` (match begin,
under the comment *"seeds the session gate 0 ([0x14C32A])"*). A fresh ordinary
match is mode `!= 4` → native `0` (§3.1). **Live value = `0`.**

### 3.4 Is the sit-8 route gated on it? Yes — `0` opens the *direct* path

The gate polarity is inverted vs intuition (matching *FU-208 §2* / *FU-218 §1.3*):
`[0x14C32A] == 0` (ordinary session) → the dispatcher's **direct** chute
`0x8AA7B → 0x8AAA8 → 0x8AB63 → 0x8A904[8]=0x8AE08` → act 7 invoke-now;
`!= 0` (Practise) → the queue arm (`sit 8 → 0x8AA60`, latch id `0xA`, no act).
So at the **live** value `0` the sit-8 route **runs** (given `phase == 0xC &&
[0x157AC2] == 4`); the engine's `fifa96_match_run_set_piece` head models exactly
this (`fifa96_match_run.c:3091`: the queue arm is taken only when
`session_gate_14c32a && !situation_pending`, i.e. the direct path is live when
the gate is 0).

---

## 4. Verdict

### 4.1 The live sit-8 draw is null-backend-safe — YES

The draw is `fifa96_rng_step` on the `mr->rng` generator that is:
seeded by the **literal constant `0`** (§2.2); stepped by **pure integer
arithmetic** (§2.1); fed by **no entropy** from the null backend (§2.3); and
reached only on a **deterministic state predicate** (§2.4). This is the same
constant-seeded generator that has produced the byte-identical M1/M2 goldens
(*cross-cited*: M1 `09b726b7…`, M2 `441415fc…`). Therefore the sit-8 draw is
**deterministic and byte-stable across replays**.

### 4.2 T1 gate impact — it will NOT stop on the draw

The T1 gate (`…phase21-live-period-end-design.md` §5) requires the extended tape to
be **byte-stable across ≥3 replays** before any re-pin. The sit-8 draw cannot
break that: on any fixed scripted tape the draw fires at a fixed frame with a
fixed value. **A fired sit-8 route changes *which* bytes the golden holds (the
RNG stream shifts by one draw and the phase-`0x1D` route installs new state), but
the new bytes are reproducible** — a legitimate, documented **re-pin** (the T6
step), not a determinism stop. There is no evidence of any nondeterministic input
to the draw.

### 4.3 Tape posture (current tape)

The current v20 M2 tape reaches **no** `[0x157AC2] == 4` completion
(*cross-cited* FU-215 §4.3 / FU-218 §5.4), so `phase_drive` never calls
`period_setup(mode 0)` and the `0x888B5` draw is **dormant** — consistent with M2
`441415fc…` byte-identical across the phase-19/20 landings. This slice changes no
code, so the current golden stays `441415fc…`.

---

## 5. Engine parity notes

### 5.1 The caller-level gate (`0x74237`) is a DEAD-TWIN property (FU225-L1)

The engine's `phase_drive` calls `period_setup(mr, 0)` on `global_157ac2 == 4`
**without** a session-gate guard (`fifa96_match_run.c:2654..2657`), so it consumes
the `0x888B5` draw regardless of `session_gate_14c32a`. The native `0x74244`
caller only skips the whole body (draw included) when `[0x14C32A] != 0` (§3.2) —
but that caller is the **statically-unreferenced twin** `0x741C4` (fresh
`get_xrefs_to 0x741C4` = 0; **FU225-L1**); the **live** call site `0x4B08E`
(`FUN_0004B02C`, via `FUN_000493A0`) carries **no** `[0x14C32A]` gate. The engine
therefore matches the **live** site, and the `0x74237` gate is a dead-twin-only
artifact. The only residual is the unmodelled **mode-4 Practise** dispatcher-queue
session (`FU-218-L3`) — leg **FU226-L2**.

### 5.2 The act-7 invoke cadence is already fixed

The native sit-8 arm invokes act 7 **now** (`0x8AEAE MOV EBX,1`);
`match_run_situation_side` now passes `invoke = 1` for act 7
(`fifa96_match_run.c:2915..2916`: `(out.act == 2u || out.act == 7u) ? 1 : 0` —
the FU-219 fix). So **FU208-L4 / FU217-L6 are resolved in the current source**; no
re-pin, no cadence leg remains here.

### 5.3 `period_setup_live` is `1` (the begin path already activates it)

`fifa96_match_run.c:2092`/`:2225` — set to `1` at match begin
(`:2225`) but the phase-21 spec §1 notes the live switch is `T4`'s to flip. The
draw's reachability at T1 does not depend on it (T1 extends *scripted input*, not
the switch).

---

## 6. Census impact

**None.** This is a read-only recon slice: no engine code changes, no row is
wired or re-counted. The census stays **`55/80 = 68%`** (`33 action + 22 phase`;
dispatch `24 UNSUP / 55 OK / 1 NOTF`, sum 80) — consistent with
`docs/ENGINE.md:8`/`:268`, the phase-21 spec §1 baseline, and `AGENTS.md`. The
sit-8 RNG draw is on an already-counted route (phase `0x1D`/row `0x20`). M1
`09b726b7…` unmoved; M2 `441415fc…` unchanged.

---

## 7. Residual legs

1. **FU226-L1 — the native match-init seed provenance.** The native seed call
   `FUN_000493A0` `0x493F2 CALL 0x92AA0` (fresh; sole caller of `0x92AA0`,
   `get_xrefs_to` = 1) passes whatever `FUN_0001D940(0x18)` returns. Fresh
   `disassemble_bytes 0x1D940` = `MOV EAX,[EAX*4+0x149278]; RET`, i.e.
   `[0x149278 + 0x18*4] = [0x1492D8]`; fresh `read_memory 0x1492D8` = all-zero
   (16 B) and fresh `get_xrefs_to 0x1492D8` = **0** (reached only via the indexed
   table form in `FUN_0001DA58`/`FUN_0001DCAC`/`FUN_0001DE94`/`FUN_0001E3A8`).
   Whether the native seed is a constant `0` (the engine's FU-142d model) or a
   runtime resource-table value is **not resolved here**. This does **not** affect
   the engine verdict (the engine seeds the constant `0` regardless); it is a
   *native-parity* leg. Owner: FU-141 / FU-142d (the RNG derivation).
2. **FU226-L2 — the dead-twin `0x74237` caller-level session gate.** Native
   `0x74244` is gated `[0x14C32A] == 0` (skip when `!= 0`) — but `0x74244` is the
   call site inside the **statically-unreferenced twin** `0x741C4` (fresh
   `get_xrefs_to 0x741C4` = 0; **FU225-L1**), while the **live** `FUN_00088860`
   call site `0x4B08E` (`FUN_0004B02C`, via `FUN_000493A0`) is **ungated**. The
   engine's `phase_drive` mode-0 call (`fifa96_match_run.c:2654..2657`) is
   likewise unguarded, so it matches the **live** site. The residual is only the
   unmodelled mode-4 Practise dispatcher-queue session. Owner: FU-196 (the
   `phase_drive` producer slice) / FU-218-L3.
3. **FU226-L3 — the mode-4 Practise session is unmodelled (carries FU218-L3).**
   The engine has no `session_gate_14c32a = 1` writer, so the queue path
   (`0x8AA60`, the sit-8 latch id `0xA`) and the dead-twin `0x74237` skip
   (`FU225-L1`; the live `0x4B08E` call is ungated) are never
   exercised. Not a missing producer — the native has no in-match writer either
   (§3.1) — only an unmodelled session. Owner: FU-146 S3 / FU-149 L4.
4. **FU226-L4 — the engine RNG transcription is cited, not byte-diffed.** The
   native generator (`FUN_00092AC8`, six cells `0x110F44..0x110F58`) was
   fresh-confirmed in structure; the engine's `fifa96_rng.c` is the FU-141
   transcription. A byte-for-byte state/sequence equivalence check between the two
   is not performed here. Owner: FU-141.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, 15239 symbols, image base `0`,
  min `0x10000`, max `.image::00174a2a`, `.image` overlay.
* `disassemble_bytes` (start, length B): `0x8AE00` (100, the bind + tail),
  `0x8AE60` (120, the arm-8 tail/act-7 invoke), `0x888A0` (80, the producer-caller
  + the RNG draw), `0x8A938` (200, the dispatcher head), `0x8AA58` (96, the
  direct-path chute), `0x8AAA8` (140, the table-2 hub), `0x92AC8` (80) + `0x92B17`
  (80) + `0x92B66` (48) (the generator `FUN_00092AC8`), `0x92AA0` (48, the seed
  `FUN_00092AA0`), `0x1B7B0` (48, writer `FUN_0001B7B8`), `0x32DE0` (48, writer
  `FUN_00032DE0`), `0x74218` (64, the `0x74237`/`0x74244` caller gate),
  `0x493B8` (64) + `0x493E0` (48, the match-init seed call), `0x1D940` (64,
  `FUN_0001D940`).
* `read_memory`: `0x1492D8` (16 B, zero), `0x149278` (16 B, zero).
* `get_xrefs_to`: `0x14C32A` (**60** = 3 WRITE / 57 READ), `0x92AA0` (**1**),
  `0x92AC8` (**186** callers, incl. the sit-8 `0x888B5` from `FUN_00088860`),
  `0x1492D8` (**0**), `0x149278` (33).
* `get_function_by_address`: `0x1D940` (`FUN_0001d940`).

Engine/loader source read first-hand: `src/fifa96_engine/platform_null.c` (whole:
`:25` `n_poll`, `:56..59` `n_now_ns`, `:60` `n_sleep_ns`, `:35..46` `n_present`);
`src/fifa96_loader/fifa96_rng.c` (`:7..9` the seed table, `:11..18` seed, `:20..57`
step); `include/fifa96_loader/fifa96_rng.h` (`:16..18` the six-word state, `:1..9`
the native mapping); `src/fifa96_engine/fifa96_match_run.c` (`:1772`/`:2073`/`:2202`
the `session_gate_14c32a = 0` seeds; `:2092`/`:2225` `period_setup_live`;
`:2148..2151` the seed call; `:2643` the roll; `:2654..2657` the mode-0 call;
`:2870..2919` `match_run_situation_side` incl. `:2915..2916` the act-7 invoke-now;
`:3085..3172` `set_piece` incl. `:3091` the session gate and `:3165..3171` the
sit-8 bind; `:3199..3203` the mode-0 RNG draw); `src/fifa96_engine/fifa96_engine.c`
(`:146`/`:155..157` the clock-only `now_ns` use). Docs cross-cited: FU-141, FU-142d,
FU-148 §18, FU-155, FU-196, FU-208 §1/§2/§4/§7 L8, FU-214, FU-215 §3.2/§4.3, FU-216,
FU-217 §3/§6/§7, FU-218 §1/§2/§5/L3, FU-219; the phase-21 spec §5/§7. No writes
beyond this slice file; no rename/label/comment/function/script/project save;
`/FIFA96.EXE` and the ISO untouched; no git commands.

---

## Landing note

W21-4 is a **read-only recon slice** (FU-226). It settles the T1-gate precondition
and the FU208-L8/FU218-L3 session-gate leg:

* **The sit-8 RNG draw is null-backend-safe (deterministic).** One
  `FUN_00092AC8` call at `0x888B5`; the engine `fifa96_rng_step` on the
  constant-seeded pure generator `mr->rng` (`fifa96_rng_seed(&mr->rng, 0)`,
  `fifa96_match_run.c:2151`); no entropy from `platform_null.c`. The T1 gate will
  **not** stop on it; a fired route yields a reproducible re-pin, not drift.
* **The session gate `[0x14C32A]` is `0` at a fresh ordinary match begin** and the
  sit-8 route **is** gated on it: `0` opens the dispatcher's **direct** path
  (`0x8A957 JZ 0x8AA7B` → arm `0x8AE08` → act 7); the engine seeds 0 and models the
  direct path. The unmodelled **mode-4 Practise** queue session is the residual
  (FU226-L2/L3, carrying FU218-L3).
* **FU208-L4 / FU217-L6 (act-7 invoke cadence) are resolved** in the current
  source (`match_run.c:2916`, FU-219).
* **No code, no census move, no golden re-pin.** The current tape (`441415fc…`) is
  dormant on the draw; `55/80 = 68%` stands.
