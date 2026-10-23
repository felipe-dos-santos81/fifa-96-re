# FIFA 96 M2 phase-35 design — the placement timer (`+0x79`) and the natural pan

## 1. Goal

Land the native placement-limit producer (`+0x79`, FU328-L1) and its `+0x7B`
counter staging (FU328-L2) as a separately pinned, golden-moving fidelity
change, then settle the natural no-slot pan on the undriven tape: with the
placement limit supplied, the band record's placement counter can exceed 3,
its stride word can exceed 2, and the no-slot seed arm can reach
`|v0| ≥ 1` — the phase-34 diagnostic (`+0x79` 3/6/15 → `v0` −1/−2/−4 →
`T` −0xC/−0x26/−0x84) proves the chain. The phase either lands the **natural
no-slot pan acceptance** (the undriven tape's first real pan, the
three-phase camera arc's goal) or records the measured producer + the honest
distribution and the remaining legs.

## 2. What phase 34 left (the binding starting state)

- The no-slot seed arm is native-faithful and the port's seed staging is at
  parity (FU326: 22 cells); the travel is bounded by the stride table
  (`|w| ≤ 15` → `v0 ∈ [−5,5]` → `T ≤ 0x90`), and the no-slot premise's
  larger-seed reading is inverted (FU326/FU316's erratum).
- The measured natural result is NO-GO **with the root named**: `timer79
  (+0x79)` has **no port producer**; the placement counter nibble is 0 on
  every record-frame (`+0x7B = min(([+0x63]>>22)+1, word[+0x79])`), the
  stride clamps to 1, `attr ≤ 1`, `|velocity| ≤ 1`, `|seed| ≤ 4`, `v0 = 0`
  — the silent bind (FU327/FU328; the register's phase-34 tail).
- The native side: `FUN_0007997C` (`0x799F0..0x79A4D`) sets `+0x79` from the
  `0x10F394` table byte OR `[rec+0x9D]`; the range seen is 6..15, but the
  native value for the tape's entity 12 is **unmeasured** (FU328-L3); "6..15
  is not a proven cap" (FU328's erratum 2).
- The port assumes `+0x79 = 0` in places: `r->timer7b = 0` at the row-09
  staging (`handlers.c:5034`, the OL-82 note), plus the `timer7b = timer79`
  copies (`entities.c:94/209/744`) and the various `timer7b` writers
  (`handlers.c:497/737/749/848/866/2344/4861`). The producer's blast radius
  is therefore both the **golden** (every record's movement) and the
  **row-09 velocity staging**.
- The acceptance design exists but is unpinned: **FU329 Branch B** (the
  undriven tape, the arm-B tuple, `has_slot 0`, `|v0| ≥ 1`, the per-axis
  `T(s)`, the segment cells; no staged slot/seed/face cells). The phase-33
  inversion's A3 window (`tests/test_engine_m2.c:3972-3979`) must re-pin with
  the new pacing (FU329's design; the A3 constants ride this producer).
- Pins: M1 `09b726b7…` immutable; M2 `d5446144…` is the current pin (the
  phase-33 re-pin); census `60/80 = 75%`.

## 3. Scope

**In slice:**
- The **native producer census**: `FUN_0007997C` instruction by instruction,
  the `0x10F394` table (its rows/values/states), the `[rec+0x9D]` OR (OL-72),
  every `+0x79` writer/reader, the value semantics, the per-record values at
  kickoff (all rows/teams), and the timing in the frame chain — a fresh
  Ghidra read; FU84 §6 carried where it holds and re-checked where the wave
  flagged it ("not a proven cap").
- The **port audit + producer design**: every `timer79`/`timer7b` site, the
  OL-82 assumption, the row-09 velocity staging's interaction, the exact
  producer shape (which cell/writer/timing), the RED/GREEN unit pins, and
  the FU328-L1/L2/L3/L8 mapping. A divergence from the wave's items is fine;
  a guessed value is not.
- The **measured blast radius** (out-of-tree patched build with the
  producer): the whole-formation movement, the M2 diff (first-diff frame,
  size, `state=` suffixes, strata), the natural band entry and the pan under
  the producer (the FU329 Branch B tuple), the frequency, and the golden
  exposure (FU328-L8).
- The **acceptance + governance**: the natural no-slot pan acceptance
  (Branch B concretized), the A3 re-pin, the Layer-2 live design, the
  faithful-move recipe (the stratum-set gate precedent, the first-diff rule,
  the stop-and-report), and the hazard list.

**Out slice (stays legs, never asserted):**
- FU324-L5 (the `+0x7B` nibble staging's metric sensitivity — the trigger
  arrives with this phase, but the re-measure is its own clause), FU326-L1
  (the non-locomotion `+0x73/+0x75` writers), FU328-L4 (the undriven
  second-half stall — unless the producer's diff exposes it), FU328-L5-L7,
  the claim/carrier fusion (FU315-L1, FU316-L8), the armer/follow
  (FU316-L6/L9), and the FU319-L8 camera-origin guard.
- No AI/decision changes beyond what the placement timer natively feeds; no
  drive staging of the slot/seed/face/timer cells in the natural witness.

## 4. Native authority (carried, not re-derived)

- The port's `+0x79`/`+0x7B` chain: the counter `word[+0x7B] =
  min(([+0x63]>>22)+1, word[+0x79])` (fresh `0x7E2E0..0x7E2F4`, FU328 §4.3);
  the row-09 velocity staging reading `timer7b`; the seed arm consuming the
  stride word (FU326 §3/§4).
- The diagnostic chain (phase 34, out-of-tree): `+0x79` 3/6/15 → the same
  natural record at f 867/704/607, `v0` −1/−2/−4, `T` −0xC/−0x26/−0x84,
  exact decay (FU328 §5.4; the wave's labelled diagnostic — never a
  witness).
- The native `+0x79` producer `FUN_0007997C` + the `0x10F394` table OR
  `[rec+0x9D]` (FU84 §6; FU328-L1/L3) — **to be pinned fresh**; the values
  6..15 are a seen range, not a cap.

## 5. Structure

Two stages, the phase-33/34 shape:

- **Stage 1 (recon-ahead wave):** four read-only slices → FU330–FU333, one
  agent each, drafts under `docs/ghidra/`, whole-wave review + quoted errata,
  promoted one-per-commit. Probes are out-of-tree; the repo receives only
  the four draft docs.
- **Stage 2 (SDD execution):** the producer plan. Sketch: **P1** the native
  producer + the row-09 interplay + the unit pins (a separately pinned,
  golden-moving commit — the golden re-pin rides it, with the faithful-move
  accounting); **P2** the natural no-slot pan acceptance (Branch B + the A3
  re-pin; test-side only; if the measured tuple exists) — or the measured
  distribution + legs if it does not; **P3** the Layer-2 live acceptance (the
  drive); **P4** the close-out register.

## 6. Acceptance (conditional on the wave's measured numbers)

- **The producer:** the native values pinned fresh (the table + the OR, per
  row/state, the store timing); the port's staging matches; the row-09
  interplay pinned (the `timer7b = 0` assumption retired or corrected).
- **The witness (if the measured values produce it):** on the **undriven
  natural tape** (`M2_KEYS` verbatim, no added holds, no staged
  slot/seed/face/timer cells), a row-04 gate pass with `has_slot 0` and
  **`|v0| ≥ 1`**, the event binding, and the place moving by `T(s)` over the
  decay (pan `0→4` per segment); the gate tuple recorded, not derived; the
  frequency recorded.
- **NO-GO (if not):** the measured producer + the full distribution
  (which records got which `+0x79`/`+0x7B`, the stride/seed/v0/T per gate
  pass), the port-fidelity verdict, and the legs — no invented drive.
- **Golden governance:** M2 re-pins **only** in P1's commit, under the
  faithful-move policy: the written reason (the first differing line, the
  moved-line count, the `state=` suffixes unchanged), the stratum-set gate
  (the phase-33 Outcome-B precedent), M1 `cmp` byte-identical, and the
  stop-and-report tripwires (a diff before the first producer-attributable
  frame, or an unexplained moved frame). The A3 window re-pins in P2 with
  the measured pacing.
- **Census/README:** `60/80 = 75%` survives (the producer is a state-fidelity
  change, not a new action row); the README title is recomputed only if a
  fresh census says otherwise.
- **Layer 2 (if the witness lands):** the undriven live run; trace-primary,
  the AE relative, the sprite-only bbox caveat, the timing-waiver option.

## 7. Honest-stop rule

Any measurement contradicting the carried chain becomes a numbered leg with
its native owner — never asserted, never explained away. A "witness" that
passes at `lane = 0`, with `v0 = 0`, or with a zero place delta is NO-GO.
Any staged slot/seed/face/timer cell in a "natural" witness is NO-GO. A
golden diff that cannot be attributed to the producer's frame chain is a
stop-and-report, never a re-pin.

## 8. Decisions (this spec)

1. The producer is a **separately pinned fidelity change**: its own commit,
   its own RED/GREEN, its own golden accounting — never folded into the
   acceptance commit.
2. The native values are **measured first** (the table + the OR; the store
   timing); the port's values come from the native, never from the
   diagnostic or the wave's seen range.
3. The acceptance vehicle is the **undriven natural tape**; the diagnostic
   chain is a causal proof, not a witness.
4. The row-09 interplay and the OL-82 assumption are audited and retired
   explicitly; a value change there is measured, not assumed.
5. The FU324-L5 re-measure rides the producer's `+0x7B` staging (the wave's
   named trigger); it is a clause in the register, not a silent re-measure.
6. The M2 move is expected to be large (the whole formation's movement);
   the faithful-move accounting is mandatory, and the stop-and-report rule
   binds.

## 9. Open items the wave settles

1. **FU330** — the native `+0x79` producer census (`FUN_0007997C`, the
   `0x10F394` table, the `[rec+0x9D]` OR, the values/timing, the per-record
   kickoff values) — a fresh Ghidra read.
2. **FU331** — the port audit + the producer design (every `timer79`/
   `timer7b` site, the row-09 interplay, the RED/GREEN plan, the FU328
   mapping).
3. **FU332** — the measured blast radius (the out-of-tree patched build: the
   whole-formation movement, the M2 diff, the natural band entry/pan, the
   frequency, the golden exposure).
4. **FU333** — the acceptance and governance (the Branch B concretized, the
   A3 re-pin, the Layer-2, the faithful-move recipe, the hazards), arguing
   from the measured numbers.

## Plan erratum (wave close, 2026-10-23)

The wave measured; these premises are **corrected by it** (append-only; the
sections above stand as written):

1. **The cap premise (§2/§4).** Quote: "the range seen is 6..15, but … '6..15
   is not a proven cap'". **Corrected (FU330, fresh):** `+0x79` is a tier-table
   lookup — `word[+0x79] = byte[0x10F394 + byte[[rec+4]+0xD]] | byte[+0x9D]`
   — with **15 the proven in-range maximum** (the table's `0x0F` at indices
   16..18; the `+0x9D` OR-writer only emits 0/0x0F). **6 is not a floor** (P2
   reaches 3, and 2 by composition), and no universal cap holds only because
   the index is **unguarded** (an out-of-range descriptor byte reads the
   `0x0FF394` continuation). The kickoff value depends on the roster byte
   (unmeasured — FU330-L1); the port's `player_d = 0` stages **6** on all
   records (declared staging, not native per-record).
2. **The port's assumption (§2).** Quote: "the port assumes `+0x79 = 0` in
   places: … the copies … and the various `timer7b` writers". **Corrected
   (FU331):** the copies (`entities.c:94/209/744`) **inherit** the value; the
   only false assumption is the literal at `handlers.c:5034` (OL-82). The
   row-09 native is `word[+0x7B] = word[+0x79]*2` (fresh `0x80B3D..0x80B43`),
   so a live 6 gives `+0x7B = 12`, `vel73 = 12`, `speed71 = 12`,
   `target = pos + 96`; the row-09 path is **inert** on the measured tape
   (0 code-9 record-frames — FU332).
3. **The A3 re-pin's commit (§6).** Quote: "The A3 window re-pins in P2 with
   the measured pacing." **Corrected (FU333-L2):** it lands in **P1** — a
   golden re-pin commit may not be red; P2 (test-side) cannot move M2 and only
   verifies.
4. **The branch name (§2).** Quote: "**FU329 Branch B**". **Corrected
   (FU333):** the landed case is **Branch A** (the witness exists, measured —
   FU332); Branch B (the measured NO-GO record) does not apply.
5. **The carried cite (§4).** Quote: "(FU328 §4.3)". **Corrected:** the
   `+0x7B` counter's fresh derivation is **FU328 §3.3** (the §4.3 label is
   wrong).
6. **The witness's character (§1/§6).** Quote: "the natural no-slot pan
   acceptance (the undriven tape's first real pan)". **Corrected (FU332):**
   the pan is **producer-conditioned** — measured at the port-reachable
   `+0x79 = 6` (entry f=703, event f=704, entity 12, `has_slot 0`, lane 56,
   `vel73 −6`, seed `−0x18`, `v0 −2`, `T_x −0x26` = −38, 13 frames, pan 2;
   at 15: event f=607, `v0 −4`, `T −0x84`, 27 frames, pan 4); the **native
   per-record value is unmeasured** (the roster index), so the witness is
   native-faithful in mechanism but route-conditioned in value — the register
   must say so. The silent bind (f=1517) is gone. M2's first differing line
   is **852**, 5 671/6 522 lines, one contiguous tail 852..6522, the 10-tag
   stratum set intact (`d5446144…` → `6aa2703b…` at 6 / `bd971062…` at 15);
   the pool diverges at step 12 (the first mover, id 10) — the render's
   852-chain is the phase-33 attribution class.

