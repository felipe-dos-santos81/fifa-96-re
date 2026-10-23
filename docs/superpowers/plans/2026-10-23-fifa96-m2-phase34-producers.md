# FIFA 96 M2 phase-34 producers — the no-slot pan travel (NO-GO close-out)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the phase-34 close-out — the append-only register and narrative for the measured no-slot result (the algebra, the distribution, the **NO-GO natural witness**, the `+0x79` root gap, the frequency evidence) — with **no production, test or golden change**. The phase is a measurement/NO-GO phase (the spec's §5 NO-GO branch).

**Architecture:** One task: the close-out commit. The wave (FU326–FU329 + its errata) is the authority; the register records it, the narrative hands the next phase its owner leg, the invariants prove the tree unmoved.

**Tech Stack:** C17/CTest (the gate only), the append-only docs.

**Spec:** `docs/superpowers/specs/2026-10-23-fifa96-m2-phase34-noslot-pan-design.md` — read its **Plan erratum (wave close)** first; the §2/§4 larger-seed premise is inverted and the witness is NO-GO.

## Decisions carried into this plan (the frozen wave, FU326–FU329 + errata)

- **FU326 — GO on the algebra and the port parity; NO-GO on the larger-seed premise.** The seed words are the stride ramp table at `0x10F680` (1 280 words, min **−15** / max **+15**) → the no-slot seed ≤ `0x3C`, `v0 = trunc(w/3) ∈ [−5,5]`, `T ≤ 0x90`; the `0x174` is arithmetic-only; the "fewer pans for larger no-slot seeds" reading inverts (13 x / 20 z pans is a floor). The port's 22 audit cells match the fresh bytes — **no staging fix**. Legs: FU326-L1 (the 87/51 non-locomotion writers), L2 (camY int16), L3 (single-slot dir model), L4 (the hardcoded `0x1104AB` rate pair).
- **FU327 — the measured distribution (GO register, NO-GO witness).** Five row-04 gate passes on the current tree: **1 no-slot** — the natural f=1517, entity 12, seed `(−4,0)`, `v0 (0,0)`, `T 0`, 0 changed frames (the phase-33 silent bind reproduced exactly); **4 slot controls** — seed `(−0x40,0)`, `v0 (−5,0)`, `T −0x90`, 27 changed frames, the `5,3,2,1,0` decay, two per driven pass. No no-slot `|v0| ≥ 1`; the measured band `|w| ≤ 1` on all 3 598 frames. Cell corrections to FU323: `active 1` (not 2 — the 2 is `row43`), `dir (0,0)` (the dir is entity 2's), and the driven seg[1] is a **row-04 event of entity 1** (code 4, `has_slot 1`); the harness's "code 3" is entity 2's pool cell.
- **FU328 — NO-GO, no nominee; the root gap.** `timer79 (+0x79)` — the native placement timer — has **no port producer**, so the placement counter nibble is 0 on every record-frame, `stride_len` clamps to 1, `attr ≤ 1`, `|velocity| ≤ 1`, `|seed| ≤ 4`, `v0 = 0`. The 32-run sweep (verbatim natural 12k–160k steps, half-time/restart attempts, every key family; no staged slot/seed/face cells) found no witness; the labelled diagnostic (staging only the locomotion limit) proves the chain (`+0x79` 3/6/15 → `v0` −1/−2/−4 → `T` −0xC/−0x26/−0x84). Frequency: **1 no-slot band entry / 3 598 granted frames (0.028 %), 0 pans**; the long window stalls at the half-time kickoff (FU328-L4). Legs: L1 the `+0x79` producer gap (**the natural-pan owner from now on**), L2 the row-0x03 NULL + staged `move_attr = 0`, L3 the native `+0x79` for entity 12 unmeasured, L4 the undriven second-half stall, L5 drive-family coverage, L6 filter ≠ gate, L7 wrap classification, **L8 the `+0x79` fix's M2 exposure**.
- **FU329 — Branch B (the acceptance design recorded, no test change).** No assert of today's zero seed; A1–A3 stay; the acceptance tuple + `|v0| ≥ 1` + the per-axis `T(s)` + the segment cells are designed for when the `+0x79` producer lands (the future A3 re-pin rides it); the golden is Case A (byte-identical) for this phase.

## Global Constraints

- `make check` green (strict `-Wall -Wextra -Werror`, ASan/UBSan; ~210 s → ≥600 s timeout) before the commit; work on `main`; commit and push.
- **Docs-only:** no production/test/golden change. M1 `09b726b7…` / M2 `d5446144…` must be byte-identical; the census holds `60/80 = 75%`; the README title untouched unless a fresh count says otherwise.
- Append-only with quoted errata; the register's honest wording: **no "natural pan" claim** — the phase's result is the measured NO-GO + the root gap; the acceptance design is **recorded, not pinned**.
- The `+0x79` producer is a **future, separately pinned golden-moving fidelity change** (FU328-L1/L3/L8): the native value measured first; no code in this phase.
- The ISO is read-only; no new captures.

## Review Focus

1. **The NO-GO honesty** — the register must not imply a natural pan was achieved; the silent bind is the consequence of the missing `+0x79` producer, not of the no-slot arm alone.
2. **The leg owners** — FU326-L1..L4, FU327-L1..L7, FU328-L1..L8, FU329-L1..L11 all named with owners; the `+0x79` producer promoted as the natural-pan owner; FU316-L1's disposition recorded (the travel model settled within the locomotion bound).
3. **The inversion wording** — the FU316 §4.3 larger-seed premise corrected (the stride table `|w| ≤ 15`), citing FU321 §3.5's carried bound.
4. **The invariants** — the docs-only proof (the goldens cmp, the census, the tree).
5. **The next-phase hand-off** — the candidates named (the `+0x79` producer first; the claim/carrier fusion; the armer/follow; the half-time stall).

---

### Task 1: The close-out register and narrative

**Files:**
- Modify: `docs/ENGINE.md` (append the phase-34 register), `docs/PROGRESS.md` (append the narrative)
- Test: the final verification commands

**Interfaces:**
- Consumes: the frozen wave FU326–FU329 (+ the phase-34 errata in FU323, FU316, the spec, and the phase-33 register); the phase-33 close-out register's format (the file's tail).
- Produces: the frozen record; the next-phase hand-off.

- [ ] **Step 1: Append the register** (append-only, quoted-errata style): the verdict (the no-slot arm algebra + the port parity + the **NO-GO natural witness** + the `+0x79` root gap); the algebra table (the stride bound ±15, seed ≤ 0x3C, `v0` ∈ [−5,5], `T ≤ 0x90`, the inversion of the larger-seed premise); the distribution (5 gate passes: 1 no-slot, 4 slot controls; the silent bind; the cell corrections); the reachability (the 32-run sweep, the diagnostic chain, the frequency `1/3 598`); the wave verdicts + errata pointers (FU326/FU327/FU328/FU329; FU323's cell erratum; FU316's seed erratum; the spec's Plan erratum; the phase-33 register's seg[1] erratum); the **legs with owners** (FU326-L1..L4, FU327-L1..L7, FU328-L1..L8, FU329-L1..L11), **FU328-L1 promoted as the natural-pan owner** (`+0x79`), FU316-L1's disposition (the travel model settled within the locomotion bound; the non-locomotion escape open), and the FU329 Branch B acceptance recorded-not-pinned.
- [ ] **Step 2: The PROGRESS narrative** — the no-slot result honestly (the arm is native-faithful; the tape's records are all parked; the pan needs the placement-timer producer); the next-phase candidates: **the `+0x79` producer (the natural-pan owner)**, the claim/carrier fusion (FU315-L1/FU316-L8), the armer/follow (FU316-L6/L9), the undriven second-half stall (FU328-L4).
- [ ] **Step 3: The invariants:** fresh census re-count (`60/80` expected — verify the census line; update the README only if it moved); `make check` 111/111; M1/M2 `cmp` byte-identical (`09b726b7…` / `d5446144…`); the tree sweep.
- [ ] **Step 4: ONE commit** `docs(fu316): the phase-34 close-out — the no-slot pan travel (NO-GO; the +0x79 root gap)`, push `origin main`.
- [ ] **Step 5: Final report** — the register, the invariants, the hand-off.
