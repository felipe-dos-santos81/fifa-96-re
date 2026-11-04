# FU-337 — the port-coverage census: 78/78 native dispatch bodies

**Scope.** The phase-36 completion metric. 2026-11-04.

## 1. Two questions, two tables

The dispatch tables answer **runtime wiring**: is there a live engine path that
installs/dispatches this row? The wired census deliberately excludes ported
bodies with no live producer (the rows-27/29/2C precedent), so it has a hard
ceiling below 100%: two native slots have no body at all.

The port-coverage table answers **reverse-engineering completeness**: does every
native slot that has a body have a faithful C implementation? A dead or
never-written row still has a body to reverse and port; it simply has no live
producer. The two questions are now separate artifacts.

## 2. The native census

| Measure | Count |
|---|---|
| Action slots (`0x1106E0[45]`) | 45 |
| Phase slots (`0x110794[35]`) | 35 |
| Total slots | 80 |
| Body-less slots | 2 |
| Native-body slots | **78** |
| Bodies ported before phase 36 | 75 |
| Bodies added in phase 36 | 3 |

The two body-less slots are:

- **action `0x2B`** — `0x1106E0[0x2B] = 0x87738`, the terminal byte of row
  29's shared epilogue (`0x8772F..0x87738`), a bare `RET`; fresh
  `disassemble_bytes 0x874E4..0x87739` gives three RETs (`0x8757D`,
  `0x87675`, `0x87738`) and `get_xrefs_to 0x8772F` returns six in-body
  branches. No distinct body.
- **phase `0x16`** — `0x110794[0x16] = 0`, the loader INT3 sentinel; the only
  zero dword in either table.

## 3. The three missing bodies

| Slot | Native entry | Port |
|---|---|---|
| action `0x0A` | `0x81738..0x81905` | `fifa96_match_action_0A` (FU-334) |
| phase `0x13/0x14` | `0x6E244..0x6E32A` | `fifa96_match_phase_13` (FU-335) |
| phase `0x1B` | `0x890EC..0x8910D` | `fifa96_match_action_phase_1B` (FU-336) |

The remaining 75 bodies were already ported, including the loader-shaped bodies
`fifa96_action_sequence_0d_step`, `fifa96_arm_27_step`, `fifa96_arm_29_step`
and `fifa96_arm_2c_step`. Their native call shape is not `int (*)(mr)`, so the
port table carries typed pointers (`body_0d_fn`, `arm_fn`) instead of `fn`; the
test asserts the corresponding pointer is non-NULL, so a renamed symbol is a
link error, not a stale string. The formation rows `0x01/0x11/0x12` were given
a real per-record body in the fix round (`fifa96_match_phase_01`, which reads
the native `[rec+8]` cell via `phase_meta8`), so they use `fn` like the other
row bodies.

## 4. The artifact

`port/src/fifa96_engine/fifa96_match_handlers.h`:
`enum fifa96_match_port_kind { NONE, BODY, NO_BODY }`,
`struct fifa96_match_port_row`, `FIFA96_MATCH_PORT_SLOTS = 80`,
`FIFA96_MATCH_PORT_NATIVE_ROWS = 78`, the table and three accessors.

`port/src/fifa96_engine/fifa96_match_handlers.c`: the 80-row table in native order
(action `0x00..0x2C`, then phase `0x00..0x22`), one row per slot. The table is
the authority for the completion percentage. The dispatch tables remain the
authority for wiring; the test refuses a mismatch between a non-NULL dispatch
`fn` and the port row's `fn`. The loader-shaped bodies carry typed function
pointers (`arm_fn`, `body_0d_fn`, `formation_fn`) so a renamed or missing
symbol is a link error, not a stale `body` string.

The seven rows whose dispatch handler exists but whose natural producer is
structurally blocked (FU-338 §4) are listed in
`fifa96_match_port_blocked[7]`. `fifa96_match_port_install_live()` is the
superset count; `fifa96_match_port_wired()` subtracts the blocked set (the
spec's natural-reach wired metric).

`port/tests/test_engine_match_port_census.c` asserts:

- 80 slots, 78 `BODY`, 2 `NO_BODY`, 0 `NONE`;
- the two no-body slots are exactly action `0x2B` and phase `0x16`;
- every `BODY` row has at least one typed body pointer, and every dispatch
  `fn` is the same function the port table names;
- the wired counts are `37 action + 25 phase = 62` install-live and
  `62 - 7 = 55` natural (after the phase-36 `0x0D`/`0x1C` wiring);
- the canonical line:
  `port-census: bodies 78/78 (100%) no-body 2 wired 55/80 install-live 62/80 action 37/45 phase 25/35`.

## 5. The derivation

```
ported_percentage = floor(100 * fifa96_match_port_bodies() / 78)
                  = floor(100 * 78 / 78) = 100
wired_percentage  = floor(100 * fifa96_match_port_wired() / 80)
                  = floor(100 * 55 / 80) = 68   (natural, 7 producer-blocked)
install_live_pct  = floor(100 * fifa96_match_port_install_live() / 80)
                  = floor(100 * 62 / 80) = 77
```

The README title carries the first number and says so; the ENGINE headline
carries both. The wired count is not a completion gate: a row with no producer
cannot be dispatched without fabricating one, which the evidence rules forbid.
The 7 counted-but-natural-reach-blocked rows are recorded in FU-338 §4; they
stay in the wired count under the project's "installer-live" definition and are
named as producer legs.

## 6. Why the metric changed

AGENTS.md previously derived the README percentage from the wired census. That
census cannot reach 100%: two slots have no body, and several native-body rows
have no producer. The phase-36 wave splits the two questions and makes the
title measure port completeness, which is what "reverse engineered and ported"
means. The wired census remains the stricter, separately reported metric.

## 7. Errata — row `0x25`'s natural producer (2026-11-04)

**Supersedes the §3/§4 counts and the §5 derivation for the natural-reach
metric; the port-coverage numbers are unchanged.** Action `0x25` is no longer
producer-blocked: `fifa96_match_phase_machine_step`'s phase-`0x13` arm now
compares the live `mr->score[0] != mr->score[1]` pair (the engine cells for the
native goal words `[0x157AC5]`/`[0x157AC7]`, `0x8D76C`/`0x8D772`, full 16-bit),
and the test-only `ac5`/`ac7` byte mirrors are gone (OL-81 closed).

Current numbers:

- `fifa96_match_port_blocked[5]` (`FIFA96_MATCH_PORT_BLOCKED_ROWS 5`).
- `37 action + 25 phase = 62` install-live (unchanged);
  `62 - 5 = 57` natural.
- canonical line:
  `port-census: bodies 78/78 (100%) no-body 2 wired 57/80 install-live 62/80 action 37/45 phase 25/35`.

```
ported_percentage = floor(100 * 78 / 78) = 100
wired_percentage  = floor(100 * 57 / 80) = 71   (natural, 5 producer-blocked)
install_live_pct  = floor(100 * 62 / 80) = 77
```

Phase `0x1F` also left the blocked set: the native sit-0xC install uses
`0x8AF0E EBX=1`, and `0x88937 CALL [0x158808]` invokes the stage-0 body once
before the match-init act-0xA supersession. The README title stays `100%`
(port coverage); the ENGINE headline carries `57/80` wired and `62/80`
install-live. The five remaining counted-but-blocked rows are FU-338 §4
(minus `0x25` and phase `0x1F`).
