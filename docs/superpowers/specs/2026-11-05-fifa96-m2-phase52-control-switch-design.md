# M2 phase-52 — the control-selection switch (`0x7D1D4`, FU-75 L4.5)

**Status:** proposed 2026-11-05. Supersedes no prior section; the phase-51
register stands as written.

## 1. The goal

Port the native `0x7D1D4..0x7D35E` **control-selection switch** (the
"select the nearest player" handler of the input-row machine) and wire the
engine's `case 0x7D1D4` stub (which today always returns 0). This closes
FU-75 L4.5 and the direct-arm consumer (`out.direct_arm`), and makes the
in-match player switch work.

The census does not move: the handler is not a dispatch slot, the port has no
new body slot, and the M2 tape is predicted byte-identical (no PASS edge and
the pinned gate's new record is rejected by the `+0x7B2` self gate). The
README title stays `100%`; wired/install-live stay `58/80` / `64/80`.

## 2. Evidence base (R12, fresh this session)

`/tmp/opencode/recon51/R12.md`; every address re-read from `/FIFA96.EXE`.

### 2.1 The body (`0x7D1D4..0x7D35E`)

1. `rec == [0x157A83]` (the controlled record) → return 0.
2. Distance gate: `dword[0x157750] < 0x50` selects threshold `0xC0`, else
   `0x120`; reject only when `word[rec+0x6B]` (`lane_x`) is **below** the
   threshold **and** `rec == [team+0x7B2]` (the team's controlled target).
3. Anchor triple = `0x157788/8C/90`. If the slot `+0x20`/`+0x21` bytes
   (read as `[slot+0x1D]>>24`/`[slot+0x1E]>>24`) are non-zero, add
   `sext8(slot+0x20) << 7` / `sext8(slot+0x21) << 7`; else add
   `sext16(word[0x1577C0]) << 4` / `sext16(word[0x1577C2]) << 4`
   (the `[0x1577BE]>>16`/`[0x1577C0]>>16` alias — spec erratum).
4. If `[0x1587A8] != 0` and phase ∈ {3,4,7}: NSEARCH from `rec+0x59` with
   skip = `sext8[[0x1587A8]+0x8D]` when the sides (`+0x826`) match, else -1;
   otherwise NSEARCH from the anchor triple with skip -1. `ECX = 0` both.
5. Result `== [0x1587AC]` → return 0; if non-null → `0x786A0(rec, result)`
   (the duel slot swap), `[team+0x7CB] = 0`, `0x78B00([result+0x20])` (the
   slot clears) → return 1.

### 2.2 Producers and cells

- `[0x1587A8]`: one writer `0x832FF` (`FUN_000832A8`) — live.
- `[0x1587AC]`: three writers `0x8330A`, `0x838D7`, `0x85777` — live.
- `[0x158730]` is not read by this body.
- `[ESP+0x60]` is the incoming `ECX` (`0x8DB7D`), zeroed by both callers →
  the NSEARCH "no result" fallback arm is unreachable from this handler.
- Engine inventory: `fifa96_action_duel_search` is the exact `0x8DB6C`
  port; `fifa96_action_duel_swap` is `0x786A0`; the slot fields
  `+0x20/+0x21` are `slot.anim_b/anim_c`; `lane_x`, `timer7cb`,
  `anchor_x/z`, `camera.vel_x/vel_z` exist. Missing: the `[0x1587A8]` /
  `[0x1587AC]` cells (the engine hard-stages `is_1578ac = 0`, OL-72) and the
  `[0x157AB0]` direct-arm consumer (staged 0).

## 3. Design

- **Library helper** `fifa96_outfield_select_step` in
  `port/src/fifa96_loader/fifa96_outfield.c/.h`, shaped like the row-04
  step: a state struct in (record id/team/pos/lane, controlled id, team
  target, anchor triple, slot anim_b/anim_c, `is_1578a8`, `is_1578ac`,
  phase, side) and an out struct (`switched`, `new_controlled`, the slot
  swap request fields). Reuse `fifa96_action_duel_search`,
  `fifa96_entity_distance` and the existing sign-extend helpers.
- **Engine wiring** `case 0x7D1D4u` in `fifa96_match_run.c`: stage the
  state from the pool, call the helper, and on success run
  `fifa96_action_duel_swap`, rebind `slot.entity` to the new controlled id,
  `timer7cb = 0`, and the `0x78B00` slot clears. Wire the
  `out.direct_arm` consumer to call the same step when the `0x7CB03`
  pre-gate fires (latent while `[0x157AB0]` is 0).
- **Cells** `is_1578a8`/`is_1578ac` on the run/outfield state, staged 0 until
  their native producers (`FUN_000832A8`/the throw-in tails) are wired; the
  leg is named (OL-72 extension).
- **Tests:** six `test_outfield.c` fixtures — self return 0, the distance
  reject, search A (side-matched skip), search B (anchor), the `[0x1587AC]`
  reject, and the success swap (assert the swap request, `timer7cb = 0`).
  One engine fixture drives `fifa96_match_run` input dispatch through
  `0x7D1D4` with a switchable record and asserts the new controlled id and
  the slot rebind. Mutation-RED: the self check, the distance gate, both
  search arms, the `0x1587AC` reject and the swap.

## 4. Invariants

- M1 `09b726b7…` and M2 `54f44987…` byte-identical (`cmp`); no re-pin.
- Census unchanged (`78/78`, `58/80`, `64/80`, `6` blocked, action `38/45`,
  phase `26/35`); README/ENGINE census strings untouched.
- `make check` green; docs append-only (FU-75 errata + ENGINE phase-52
  register + PROGRESS).
- No fake producer: the `is_1578a8`/`is_1578ac` staging is a named leg, and
  the switch works on the anchor path without them.

## 5. Acceptance

- The helper's six fixtures and the engine fixture pass; mutation checks are
  RED where named.
- M1/M2 `cmp` exact; the census print unchanged; `make check` green.
- ENGINE: FU-75 L4.5 closed; the remaining legs (`[0x1587A8]`/`[0x1587AC]`
  live producers, OL-68 `0x786A0` block move, `0x78B00` `+0x24`,
  `[0x157AB0]`) listed.

## 6. Out of scope

- The act-9 stage-0 camera/placement sinks (FU181-L5) — the next phase.
- The `PCCD.POG`/`PCDB` decode (row-03 clamp, `desc_16`, team identity) —
  data-format phase.
- The pitch/stadium, camera follow, sound and the wider player-visible
  register (R10).
