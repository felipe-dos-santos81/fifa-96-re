# FIFA 96 M2 phase-12 — live camera, visible ball

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Wave R runs as four parallel READ-ONLY recon subagents; implementation tasks stay serialized (one implementer at a time).

**Goal:** close the two visible-surface legs — **15.a** (the follow-cam never visibly follows: no live event crosses band-2 `d == 0x70` at a kick frame; suspect the `0x7CD60` second-arm tracked-side producer, leg 15.c/OL-68) and **L14.8.e** (the ball never renders on screen: it sits behind the parked/stand-in camera; needs a live camera place/pan in open play). Accelerate the RE with a **parallel read-only recon wave (R1–R4, four subagents, Ghidra-MCP)** whose outputs freeze as FU-153…FU-156, then a serialized implementation wave. Riders: the `[0x14C32A]` gate census (the ball machine's legal run window) and the engine sanitizer build (the known UB-detection gap).

**Architecture:** Derivations from FU-148 §15.3/§16.4, FU-75, the phase-11 T1/T2 reports, and the phase-11 final review. Consumers: the f-up9 camera chain (`fifa96_camera_rate_event`, follow arm), the T1-phase-11 height/key producers, the T2-phase-11 ball machine (`fifa96_match_run_ball_machine`), the render seam. Serialized implementers; parallel read-only recon is the only parallel surface.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (test TUs; T3 extends to the engine target), Ghidra MCP read-only, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU148…`, `FU75…`, `FU145…`; predecessor `docs/superpowers/plans/2026-10-11-fifa96-m2-phase11-visible-ball.md` + its final review.

## Global Constraints

- Baseline: `make check` **108/108**; wired 21/80; HEAD `c591575` (+plan commit). M1 `09b726b7…`; M2 `5524cf40…` (85 lines).
- Evidence floor: first-hand bytes/addresses; **fresh `get_xrefs_to` for census claims**; `/FIFA96.EXE` (case-sensitive); +0x100000 rule; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`) — four strikes across the project; verify CALL bytes.
- Determinism: null backend source of truth; **M1 never moves**; M2 re-pins only for intended upgrades with written reason + frame diff (expected: none).
- No fakes; unreachable → numbered legs; single situation entry; ISO-gated tests skip without the ISO.
- **Wired census rule:** if a task changes the wired count, recompute the README title percentage `floor(100 × wired/total)` per AGENTS.md in the same commit. Status narrative → `docs/PROGRESS.md`.
- Commits `feat/fix/docs/test`; implementers commit and push per dispatch; controller verifies (`make check` + tape `cmp` + tree clean).

---

## Wave R — parallel read-only recon (gate: four FU slices + reports, committed)

R1–R4 run **concurrently**. Each is read-only on the repo: it writes ONLY its new `docs/ghidra/FU1NN_*.md` slice and its workspace report; **no git write commands, no edits to existing files, no code**. The controller commits FU-153…FU-156 in one `docs(fu153-fu156)` commit after the wave returns.

### R1 → FU-153: `0x7CD60` second arm + the band-2 crossing (unblocks 15.a)
- Question: what live event can produce a trajectory crossing band-2 (`d == 0x70` exactly) at a kick frame? Derive the `0x7CD60` second arm's full preconditions and its missing tracked-side producer (leg 15.c/OL-68): arm selects `rec == [0x1577CA]`; `[target+0x20] == 0`; `FUN_000786A0`; `[+0x820] = 0x3C`.
- Derive: who sets `[0x1577CA]` live (fresh xrefs); how a live long-ball kick reaches the second arm; whether the second arm is the lofted-delivery producer that makes the arc cross `d == 0x70` at a kick frame (compare with the stage-0 JGE at `fifa96_action_handlers.c:1544` and the band-2 `d >= 0x70` gates).
- Output: FU-153 with byte cites + a concrete T1 wiring recommendation, or the next-hop producer as a numbered leg.

### R2 → FU-154: live camera place/pan in open play + ball on-canvas (unblocks L14.8.e)
- Question: what moves the camera place (`FUN_00079F3C` and the pan integrator inputs) from live play so the ball's camera-derived phase-2 lead lands on canvas?
- Derive: the ball lead computation (`0x8BB7C..0x8BBF6`, camera-derived — identify which camera cells it reads); the camera place/pan producers whose cold-start/stand-in state blocks movement (the `(0,0,0)` stand-in camera origin); what a live kick/pan needs to move the camera in open play.
- Output: FU-154 with the T2 wiring recommendation (or the next-hop producer as a numbered leg).

### R3 → FU-155: `[0x14C32A]` census and polarity (machine run window)
- Fresh writer census (known: `0x1B7C8`/`0x1B7D5` in `FUN_0001B7B8`, `0x32DFF` in `FUN_00032DE0`) + both call-site polarities (`0x4B198` runs iff `!= 0`; `0x8B65B` runs iff `== 0`) + whether any runtime path toggles the byte between the two calls.
- Output: FU-155 concluding the ball machine's legal run window (or a numbered leg if not statically decidable).

### R4 → FU-156: remaining `FUN_00071C94` pan-caller reachability
- Fresh `get_xrefs_to 0x71C94`; classify the six remaining callers (`0x7F4E7`, `0x71B8A`, `0x6FA62`, `0x77423`, `0x77DC6`, `0x82A6C`) one line each: what invokes them (fresh xrefs), live-reachable yes/no/unknown + why, and overlap with R1/R2's producers.
- Output: FU-156.

---

### Task 1: T1 — follow-cam wiring (from R1/R4)

**Files:** per R1's recommendation (engine run/camera paths, possibly `fifa96_ball_pairing.c` second arm, `fifa96_match_run.c`); tests + FU-153 status + `docs/ENGINE.md`.
**Produces:** the live event crossing band-2 at a kick frame → the follow arm holds → **smoke shows the camera visibly following**.
**Gate T1:** visible follow (or the deepest provable step with the next-hop as a numbered leg).
- [ ] RED test → implement → GREEN → gate (`make check` + golden decision + smoke `p12-t1-*`) → commit `feat(engine,fu153): live follow — band-2 crossing (15.a)`.

### Task 2: T2 — ball-visibility wiring (from R2; coupled to T1)

**Files:** per R2's recommendation (engine camera/ball paths, render seam); tests + FU-154 status + `docs/ENGINE.md`.
**Produces:** the camera place/pan live in open play → the ball's lead lands on canvas → **smoke shows the ball on screen**.
**Gate T2:** visible ball (or deepest provable + numbered leg).
- [ ] RED test → implement → GREEN → gate + smoke `p12-t2-*` → commit `feat(engine,fu154): on-screen ball — live camera place (L14.8.e)`.

### Task 3: T3 — engine sanitizer build

**Files:** `CMakeLists.txt` (engine target + `fifa96` executable flags), any real UB fixes surfaced (or legs), `docs/ENGINE.md`.
**Produces:** `fifa96_engine` and the SDL3 main built with `-fsanitize=address,undefined -fno-sanitize-recover=all`; suite green; **triage clause:** pre-existing UB found → fix real issues or leg-number them with byte/stack evidence; do not derail the phase.
**Gate T3:** sanitized engine build green (or triaged with legs).
- [ ] Enable → run full suite + M1/M2 → triage → commit `build(engine): sanitize the engine target (phase-12 T3)`.

### Task 4: T4 — acceptance v12 + smoke + docs + final review

**Files:** `tests/test_engine_m2.c` (+ golden iff genuinely moved — reason + diff), `docs/ENGINE.md`, `docs/PROGRESS.md`, README title check (census), `docs/screens/*`.
**Produces:** v12 strata for the phase's visible legs; the honest smoke record (follow? ball?); remaining forcings with legs; final-review material.
- [ ] Tests → tape decision → smoke → docs → commit `test(engine): phase-12 acceptance tape v12 and docs` → whole-plan final review (one fix wave max) → close.

---

## Self-Review Notes

- **Coverage:** follow-cam 15.a → R1/T1; ball L14.8.e → R2/T2; gate census → R3 (doc/leg); pan callers → R4 (feeds T1); sanitizer → T3; acceptance → T4. Deferred: FU-149..152 carried legs, FU-146 restart legs, palette translation content, remaining writer sites.
- **Dependency order:** Wave R (parallel) → T1 → T2 (T2's camera coupling wants T1's movement) → T3 → T4.
- **Split rule:** any recon question that cannot bound its answer in one slice → land the bounded part and number the rest as legs (the wave does not stretch).
- **Determinism:** live producers may move live frames; M2 re-pin only if T1/T2 genuinely change the tape with reason + frame diff; M1 never moves.
