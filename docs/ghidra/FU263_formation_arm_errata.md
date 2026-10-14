# FU-263 (draft) — the act-4 formation arm (`17`) re-verified, and the phase-24 corrections sweep

Provenance: M2 phase-25 wave **W25-8** (planned FU-263), 2026-10-14, **read-only**.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive
program argument; standard tools only; **no** rename / comment / label / function /
script / project save). `get_current_program_info` this session: `FIFA96.EXE`,
`watcom:LE:32:default`, `min_address 0x10000`, `max_address .image::00174a2a`,
**2706 functions / 15239 symbols** (matches the phase-24 baseline). Every
address/byte/xref below is a fresh `decompile_function` / `disassemble_bytes` /
`get_xrefs_to` of **this** session unless marked *cross-cited* (then a frozen FU
doc is named). Engine source read first-hand. The alias traps are applied
(`dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`). **No repo file was
modified beyond this draft + the report; no git; no code/test edits; no subagents.**

Question (plan Task 8 / spec §4 slice 8): re-verify the act-4 formation-arm
verdict (`17` — phase-24 `FU248` proved it GATED) against the frozen `FU248`, and
assemble the **phase-24 corrections sweep** (FU247 OL-65 erratum, OL-83
unification, `ENGINE.md:4037` row-18 over-list, FU-84 §1 errata, FU201-L1 owner)
as a port/errata list for Stage 2.

---

## Answer in one line

**Row `17` stays a leg — the phase-24 GATED verdict re-confirms.** Fresh this
slice: `FUN_00011620` is a **front-end producer** (5 callers, all front-end
ranges `0x18xxx..0x32xxx`); the reset `FUN_00073EE0` is the **only bulk writer of
`0x157BD2`** and copies desired→current + zeroes both residual counters; the only
candidate divergence producer `FUN_00041494` **case 5** writes `0x14C360`
(**desired only**) at `0x41B12`/`0x41B5F`/`0x41B91` and never `0x157BD2`. No
statically-provable **persistent** non-permutation exists, so action row `0x17`
stays `{0x17, NULL, …}` (`fifa96_match_handlers.c:5500`, fresh read) — **a
numbered leg, not countable.** The phase-24 corrections sweep resolves to **5
errata, 0 new wires, 0 census change** (58/80 = 72% holds).

---

## 1. Formation-arm re-verification (fresh confirmations vs the frozen `FU248`)

Four fresh confirmations (not a full re-derivation; the frozen `FU248` body
stands):

### 1.1 `FUN_00011620` is a front-end producer

`get_xrefs_to 0x11620` (fresh) = **5** — exactly the frozen count:

```
0x11BB6 (FUN_00011b7c)   0x11BC5 (FUN_00011b7c)
0x25B4C (FUN_00024b00)   0x25B6B (FUN_00024b00)   0x25C64 (FUN_00024b00)
```

`get_xrefs_to 0x11B7C` (fresh) = **5** — the *next* level, every caller in a
front-end/menu range:

```
0x18765 FUN_00018680   0x18164 FUN_00018108   0x31231 FUN_000302dc
0x32B78 FUN_00032520   0x2B0B8 FUN_00029f68
```

`0x187xx`/`0x181xx`/`0x312xx`/`0x32Bxx`/`0x2B0xx` are all front-end/menu
addresses, **not** the match loop. **Confirmed** — the desired-order fill is a
menu producer (`FU248` §1 / §2.1).

### 1.2 The reset `FUN_00073EE0` copies desired→current and zeroes the counters

Fresh `decompile_function 0x73EE0` (abridged):

```
DAT_00157b8e = 0; DAT_00157b8f = 0;              /* the two residual counters */
for (side = 0; side < 2; side++)
  for (i = 0; i < 0xb; i++) {
    DAT_00157b90[side*0xb + i] = DAT_0014c3a0[side*0x7b + i];       /* companion byte  */
    *(u32*)(&DAT_00157bd2 + side*0x2c + i*4)
        = *(u32*)((int)&DAT_0014c360 + side*0x7b + i*4);            /* desired -> current */
    *(u16*)(&DAT_00157ba6 + side*0x16 + i*2)
        = *(u16*)((int)&DAT_0014c3b0 + side*0x7b + i*2);            /* word companion  */
  }
```

`get_xrefs_to 0x73EE0` (fresh) = **3** (`FUN_00074034` `0x7407F`, `FUN_0004B020`
`0x4B025`, `0x7428E`) — matches `FU248` §3.2. **Immediately after the reset
`current == desired` and the residual is 0. Confirmed.**

### 1.3 The only divergence candidate — `FUN_00041494` case 5 — is desired-only

Fresh `disassemble_bytes 0x41B00` (160 B). The swap pair writes **`0x14C360`
only**:

```
0x41B12  MOV ECX, dword[ESI+0x14C360]   ; save des[i]
0x41B56  MOV ECX, dword[EBP+0x14C360]   ; des[j]
0x41B5F  MOV dword[ESI+0x14C360], ECX   ; des[i] = des[j]     <-- WRITE (desired only)
0x41B65  MOV CL, byte[EBP+0x14C3A0]     ; companion byte
0x41B6B  MOV byte[EDX+EAX+0x14C3A0], CL ; companion WRITE
0x41B91  MOV dword[ESI+0x14C360], ECX   ; des[j] = saved     <-- WRITE (desired only)
```

**No `0x157BD2` (current) write anywhere in the pair.** Confirms `FU248` §4.2:
the swap is multiset-preserving within the desired array; a residual ≥ 1 only if
the indices straddle the `0..10`/`11..15` boundary, and that divergence is
reconciled inside the same flow by `FUN_0008EAB8`/`FUN_0009343C` (case 7). The
window is **dynamic** and not statically provable — this is `FU201-L1`, not a
producer.

### 1.4 Fresh writer census of both matrices (counts re-confirm)

`get_xrefs_to 0x14C360` (fresh) = **58** (matches `FU248`); `get_xrefs_to
0x157BD2` (fresh) = **29** (matches `FU248`). The container set is unchanged:
`FUN_00073EE0` (copy), `FUN_0008C974` (sort), `FUN_0008CBB0` (same-i,j swap of
both), `FUN_0008EAB8`/`FUN_0009343C` (reconcile), `FUN_00011620`/`FUN_00011978`/
`FUN_00012010` (front-end), `FUN_00041494` (case 5 desired-only + one other
desired write `0x4160A`), the `0x82xxx` block, row-`0x17` stage-2 `0x84892`,
act-4 stage-0 `0x896D7`, `FUN_0008C758` (reader). **No non-reset,
non-reconciler producer of a persistent non-permutation. Confirmed.**

### 1.5 The census posture (fresh code read)

`fifa96_match_handlers.c:5500` is `{0x17, NULL, "… -UNSUPPORTED"}` — the row is
NULL/uncounted. **Verdict: action row `0x17` stays a numbered leg
(`FU248-L1` = `FU201-L1`).** Census unchanged at **58/80 = 72%**.

> Scope note (concern, not an erratum request): `ENGINE.md:929-930` cites the
> row-`0x17`/phase-`0x1A` slots at `fifa96_match_handlers.c:5012`/`:5111`, but
> the fresh table puts them at `:5500`/`:5603` (`FU248` §5). A stale line-ref in
> `ENGINE.md` (line drift), flagged for the controller — see Leg `FU263-L3`.

---

## 2. The phase-24 corrections sweep (5 errata)

Each erratum: **target file + line**, the current (wrong) wording, and the
**corrected wording**. Per AGENTS evidence rules, prior sections are never
rewritten — each lands as a **quoted errata block appended to the named doc** (a
`docs(fuNNN)` commit) or a code-comment fix; the source derivations already
exist in the phase-24 slices cited.

### Erratum 1 — `FU247` OL-65 input map (the alias-trap error), corrected by `FU250`

- **Target:** `docs/ghidra/FU247_exclusions_and_tape.md`
  - §2.3, **lines 153-166** (esp. the quote at **156-157**: *"the second input is
    `word[+0x6D]` (`0x8169F`/`0x816A5`) … No `+0x6F` read exists anywhere in
    `0x8164B..0x816E4`"*).
  - §2.2 table, **lines 88-92** (`0x816A5 SAR EDX,0x10 (**z = word +0x6D**)` /
    `0x816A8 SAR EAX,0x10 (**x = word +0x6B**)`).
  - §6, **lines 367-375** (`FU247-L1`, the same mis-map).
- **Wrong:** the row-07 angle gate reads `word[+0x6B]` / `word[+0x6D]`.
- **Corrected:** both operands are **dword** loads; the `SAR 0x10` selects the
  high word — `word[+0x6D]` (from `dword[+0x6B]>>16`, sites `0x816A2`/`0x816A8`)
  and `word[+0x6F]` (from `dword[+0x6D]>>16`, sites `0x8169F`/`0x816A5`) — minus
  `word[+0x7D]` (`0x816B0`). The gate is
  `fold10(entity_angle(word[+0x6D], word[+0x6F]) − word[+0x7D]) < 0x55`.
  Consequently: (i) `FU185-L6`'s map (`+0x6D`/`+0x6F`/`+0x7D`) is **correct**;
  (ii) `fifa96_action_handlers.h:918-919` is **correct**; (iii)
  `opp_angle_x = opp->lane_z` is **correct** (not "mis-sourced"). The gate's
  deadness is caused **only** by the two hardcoded zeros (`opp_angle_z` and
  `opp_face_word`) in the staging. Source: `FU250` §5.1 (fresh; `FU250-L1`).
- **Also:** `docs/ENGINE.md` ~341 and `handlers.c:5523` already name `word[+0x6F]`
  correctly — **no change needed** beyond the staging fix (`FU250` §5.1).

### Erratum 2 — OL-83 unification: unify on `record.type`, retire `actor_type`

- **Target (code, Stage 2):**
  - `include/fifa96_engine/fifa96_match_entities.h:78` — `uint8_t actor_type;
    /* +0x8B >> 24 */` **is the same native byte `+0x8E` as `type`** (`:76-78`);
    the two are two access idioms for one byte (`byte[rec+0x8E]` vs
    `dword[rec+0x8B]>>24`). **Corrected:** retire the C field (or make it a
    `#define` alias of `type`); keep `record.type`.
  - `src/fifa96_engine/fifa96_match_handlers.c` — repoint the wired `type8`
    readers from `r->actor_type` to `r->type` at **`:906`, `:2341`, `:2534`,
    `:2779`, `:3004`**; the write-back at **`:1073`** (`a.actor_type = s->type8;`)
    and the pool copy at `fifa96_match_run.c:1316` (`r->actor_type = e->actor_type;`)
    collapse onto `type`.
  - `include/fifa96_engine/fifa96_match_run.h:474` (field decl) and **`:503`**
    (comment) follow.
- **Wrong:** the pool carries two fields that alias one native byte; `actor_type`
  is **never written** by any wired body, so every wired `actor_type` read sees a
  stale 0 while the native would see the last face write.
- **Corrected:** one field (`record.type`). This is the engine's own FU-151 §3.5
  wording ("the legacy alias `actor_type`"); `handlers.c:1685` already states the
  readers should use `record.type`. Source: `FU253` §3.3 (`FU253-L3`, MUST be
  applied before any reader is re-wired) + `FU252` §5.1 (`FU252-L4`, the wider
  writer set: ≥9 writer functions).
- **Doc target:** `docs/ENGINE.md:4032-4040` OL-83 paragraph and
  `include/fifa96_engine/fifa96_match_run.h:503` — rewrite the OL-83 note to the
  "one native byte, unify on `record.type`" ruling.

### Erratum 3 — the row-18 over-list (`ENGINE.md:4037`, also `run.h:503`)

- **Target:** `docs/ENGINE.md:4036-4038`, the current wording:
  > *"OL-83: rows 04/06/07/18 read `record.actor_type` for the native `+0x8E`
  > byte while rows 08/28/2A write `record.type` …"*
- **Wrong:** row **`18`** is listed as an `actor_type`/`+0x8E` reader.
- **Corrected:** drop row **`18`** from the reader list. Fresh `FU253` §2.4/§3.3:
  the row-18 body `0x849B0..0x84AE1` touches only `+0x28`, `+0x65`, `+0x63`,
  `+0x89`, `+0x92`, `+0x9A`, `+0x7B` — **no `+0x8E` and no `+0x8B` access**. The
  actual wired `actor_type` readers are rows **`04`/`06`/`07`** (+ the shared kick
  binder and rows `0B`/`21`), per `FU253` §3.2.
- **Second site of the same error:**
  `include/fifa96_engine/fifa96_match_run.h:503` — the comment *"row-04/06/07/18
  readers use `actor_type`, OL-83"* carries the identical over-list; correct to
  `04/06/07`. Source: `FU253` §3.3 ("Erratum candidate (doc)").

### Erratum 4 — `FU-84` §1: the reroll RNG is **not** discarded

- **Target:** `docs/ghidra/FU84_animation.md` §1, **lines 97-100**, the current
  wording:
  > *"… advances the RNG stream and **re-selects the current id** (the RNG result
  > is discarded); with no current row it also advances the stream and selects
  > row 0."*
- **Wrong:** "the RNG result is discarded."
- **Corrected:** the RNG result **is** the rerolled id. Fresh `FU254` §3/§9:
  `0x6E659 CALL 0x92AC8; EAX = rng & 3; JMP CS:[EAX*4 + 0x6E588]`, and the fresh
  `read_memory 0x6E588` table `{0x6E675, 0x6E679, 0x6E680, 0x6E675}` sets
  `EDX = {0, 0x62, 0x65, 0}` for `rng&3 = {0,1,2,3}`. So the keep/reroll arm is a
  **non-zero-anim producer** (id `0x62`/`0x65`) — the engine's derived fallback 0
  diverges (`FU254-L2`). `FU-142 §OL-52`'s "`RNG & 3 -> {0,0x62,0x65}`" is
  consistent and is extended with `case 3 -> 0`. Land as an appended errata block
  (`FU254` §9).
- **Note (not an erratum):** `FU-84` §1's method note about the `0x6E724` RNG tail
  stays open (`FU254` §9 second bullet).

### Erratum 5 — `FU201-L1` owner: `FUN_00041494` case 5

- **Target:** the `FU201-L1` registration sites, to add the named candidate owner:
  - `docs/ghidra/FU201_order_matrix_act4_arm.md` §8 leg 1, **lines 369-376** —
    the current text ends *"**Owner: a dynamic-trace leg; the R47 setup does not
    settle it.**"* **Corrected: add the named candidate owner
    `FUN_00041494` (`0x41494`) case 5**, the in-match substitution swap
    (`0x41B12`/`0x41B5F`/`0x41B91`, desired-only, reconciled in the same flow by
    the case-7 `FUN_0008EAB8`/`FUN_0009343C` pass). The leg becomes a
    **dynamic-trace leg with a named native owner**.
  - `docs/ghidra/FU201_order_matrix_act4_arm.md` §? (the "Owners carried" line
    ~**`:494`**) — same owner annotation.
  - `docs/ENGINE.md:926` — the parenthetical *"(**FU201-L1**, the
    substitution/line-up divergence; carries FU197-L1)"* — add the owner
    `FUN_00041494` case 5.
- **Wrong:** `FU201-L1` is registered owner-less ("a dynamic-trace leg").
- **Corrected:** owner = `FUN_00041494` case 5 (the in-match substitution swap),
  plus the reset `FUN_00073EE0` and the sorter `FUN_0008C974` as the equalising
  bookends. Source: `FU248` §4.2/§4.3 + §Residual legs (`FU248-L1` = `FU201-L1`).
  This is a **leg-owner annotation**, not a wire.

---

## 3. Port shape / handoff

- **Errara 1 and 4** are **doc-only** appended errata (no code): `docs(fu247)`
  and `docs(fu84)` errata commits, quoting the corrected derivation.
- **Erratum 3** is **doc-only** (`ENGINE.md` + the `run.h:503` comment).
- **Erratum 2** is a **Stage-2 code change** (one field: repoint `type8 = r->type`,
  retire `actor_type`) plus the OL-83 doc rewrite — a fidelity fix, **no wire, no
  census change**.
- **Erratum 5** is a **doc/leg owner annotation**.
- **No Stage-2 wire** results from this slice: row `0x17`, phase `0x1A` and phase
  `0x0B` stay NULL. The act-4 arm, the two `FUN_0008C974` sorts and the
  `FUN_00011620`/`FUN_00012010` fill remain ported-but-gated / staged
  (`FU248` §5, unchanged).
- **Census: unchanged, 58/80 = 72%.** README title does not move.

---

## 4. Numbered legs

1. **FU263-L1 — the `FU247` OL-65 erratum promotion (doc).** `FU250` §5.1 /
   `FU250-L1` proves `FU247` §2.3 / `FU247-L1` mis-map the row-07 angle inputs
   (alias trap): the native is `word[+0x6D]`/`word[+0x6F]`/`word[+0x7D]`.
   **Owner: the `FU247` doc owner / OL-65.** Evidence: `FU250` §2.1/§5.1;
   AGENTS alias traps.
2. **FU263-L2 — the OL-83 unification + reader-list correction (code + doc).**
   Unify `actor_type` → `record.type`; correct the reader list to
   `04/06/07` (+ kick binder, `0B`/`21`) at `ENGINE.md:4037` and
   `fifa96_match_run.h:503`. **Owner: the FU-142/OL-83 follow-up (Stage-2
   fidelity task).** Evidence: `FU253` §3.3 (`FU253-L3`), `FU252` §5.1 (`FU252-L4`).
3. **FU263-L3 — the `ENGINE.md` stale handler line-refs (doc, newly surfaced).**
   `ENGINE.md:929-930` cites row `0x17`/phase `0x1A` at
   `fifa96_match_handlers.c:5012`/`:5111`; the fresh table has them at
   `:5500`/`:5603` (`FU248` §5, fresh read this slice). Also `ENGINE.md:944`
   records the M2 golden sha as `7b4b241a…` while `FU247` §4.1 (v22, 6522 lines)
   gives `b281a9b2…`. **Owner: the controller at reconciliation.** Evidence:
   fresh `handlers.c:5500` read; `FU247` §4.1.
4. **FU263-L4 — the `FU-84` §1 reroll errata promotion (doc).** `FU254` §9 shows
   the RNG result **is** the rerolled id `{0,0x62,0x65,0}`. **Owner: the `FU-84`
   doc owner / the anim task (`FU254-L2`).** Evidence: `FU254` §3/§9.
5. **FU263-L5 — `FU201-L1` owner annotation (doc).** Name `FUN_00041494` case 5
   as the candidate owner. **Owner: the `FU201` doc owner** (carries `FU248-L1`).
   Evidence: `FU248` §4.2/§4.3.

---

## 5. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info` (2706 functions / 15239 symbols);
`get_xrefs_to 0x11620` (5), `0x11B7C` (5), `0x73EE0` (3), `0x14C360` (58),
`0x157BD2` (29); `decompile_function 0x73EE0`; `disassemble_bytes 0x41B00`
(160 B). Engine source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`:906`, `:1073`, `:2341`, `:2534`, `:2779`, `:3004`, `:5500`),
`src/fifa96_engine/fifa96_match_run.c:1316`,
`include/fifa96_engine/fifa96_match_entities.h:76-78`,
`include/fifa96_engine/fifa96_match_run.h:474/503`.
Docs read/handled: `docs/ghidra/FU248_formation_setup.md`,
`FU250_ol65_angle_inputs.md`, `FU252_row04_18_23.md`, `FU253_row08_fields.md`,
`FU254_anim_sink_producers.md`, `FU247_exclusions_and_tape.md`,
`FU201_order_matrix_act4_arm.md`, `FU84_animation.md`,
`docs/ENGINE.md` (`:918-951`, `:4032-4059`); AGENTS.md (alias traps, census
rule, errata-never-rewrite). No writes beyond this draft and the report; no
rename/label/comment/function/script/project save; no git; no subagents.
