# FIFA 96 M2 phase-35 hardening — the test-side residuals

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clear the phase-35 final review's test-side residuals in one hardening commit: the witness pin precision, the claim/offset segment guards, the sample-cap and identity closures, the no-ISO gating, and the comment fixes — **no production change, no golden change**.

**Architecture:** One task, one commit. All edits are in `tests/`; M2 `6aa2703b…` and M1 `09b726b7…` must stay byte-identical, and `make check` must stay 111/111.

**Tech Stack:** C17/CTest.

**Spec:** `docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md` (the phase-35 authority; this slice touches none of its product claims)

## Global Constraints

- Test-side only: `tests/*.c`. No `src/`, `include/`, or `tests/golden/` change; the goldens `cmp` byte-identical; the census `60/80 = 75%`; the README untouched.
- `make check` green (strict, ASan/UBSan) before the commit; work on `main`; one commit; push.
- Every item below is a finding from the phase-35 final review; do not broaden the slice.

## The items (the final review's deferred minors, fix-now)

1. **The arm-distinction assert is x-only** — `tests/test_engine_m2.c:4243`: make it the vector form `((seed_x != dir_x<<6) || (seed_z != dir_z<<6))`, matching the structural claim.
2. **The claim/offset segment guard is off by one** — `tests/test_engine_m2.c:4682`/`:4698` (and the natural sampler's twin): `struct m2_claim_segment seg[2]` with a `<= 2u` guard indexes `seg[2]` at `segments == 3`. Change the guard to `< 2u` (the safe form the natural sampler already uses relative to its array size), or enlarge the array — pick the safer minimal fix and say which.
3. **The `M2_NAT_PAN_SEGMENTS 4` silent cap** — `tests/test_engine_m2.c:2673`/`:7037-7045`: add an assert that the recorded segment count ≤ the cap (the overflow would silently truncate); keep the cap.
4. **The closure `g->id == ev->id`** — `tests/test_engine_m2.c:4205-4215`: add the identity assert (both are the band entity; the measured value is 12).
5. **The no-ISO ring/speed asserts** — `tests/test_engine_m2.c:3948-3950`/`:4012-4013`: they run unconditionally while FU322-L10 declares the no-ISO window unmeasured/recorded-only. Either gate them with `with_iso` or state at the site that they are structural invariants that the measured no-ISO run satisfies — pick one and say which.
6. **The claim per-segment cells are not reset at a segment start** — `tests/test_engine_m2.c:4683-4695`: reset them (the natural path's `memset(sg, …)` is the precedent).
7. **The P1 pin samples two records** — `tests/test_engine_match_entities.c:1215`: loop all 22 records instead of the two sampled (the init loop writes all 22).
8. **The row-06 `s.byte9d` site has no independent pin** — `tests/test_engine_match_handlers.c:3124`: add a pin (a nonzero pool `byte9d` reaching the row-06 consumer) or state why it is covered by the row-04 pin.
9. **Comment fixes:** `tests/test_engine_m2.c:2702-2705` (the nonexistent frequency fields — they are computed in `main`), `tests/test_engine_match_handlers.c:1726` (row 04's `desc_e` is still hardcoded 0; only `byte9d` is pool-backed) — correct both.
10. **`ev->code == 5`** — `tests/test_engine_m2.c:4324`: label it one-shot evidence (keep; do not drop — it is measured).

## Task 1: The hardening commit

**Files:** `tests/test_engine_m2.c`, `tests/test_engine_match_entities.c`, `tests/test_engine_match_handlers.c`

**Steps:**
- [ ] Apply items 1–10; keep the changes minimal and in the files' existing style.
- [ ] Focused binaries green; `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` and the M1 `cmp` **byte-identical**; `make check` 111/111 (timeout ≥600 s).
- [ ] ONE commit `test(fu330): the phase-35 test-side hardening`, push `origin main`.
- [ ] Report: the per-item record, the cmp evidence, the suite.
