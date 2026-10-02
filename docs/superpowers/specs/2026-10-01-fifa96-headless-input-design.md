# FIFA96 Headless Scripted Input — Design

**Date:** 2026-10-01
**Status:** approved design (brainstorming), pending spec review
**Spec of record for:** `docs/superpowers/plans/` implementation plan (to follow)

## Goal

Let the headless capture rig drive the FIFA96 guest with scripted keystrokes
so traces can reach input-gated code. First target: the two entry points that
FU-12 proved are unreachable from an unattended startup (`FUN_00014c18` at
`0x14C18`, `FUN_00023b38` at `0x23B38`); general target: a reusable input
facility for every later gameplay-gated probe.

## Context

* The capture rig (`run-fifa96-capture.sh`) generates a DOSBox-X conf and runs
  the game with `-silent`; windowed mode is unusable on this host and host-side
  input (xdotool) cannot reach the client window.
* This DOSBox-X build supports `AUTOTYPE` (`AUTOTYPE [-w WAIT] [-p PACE] key
  [key...]`, key names such as `enter`, `space`, `esc`, `up`, `kp_8`, `,`
  separators). AUTOTYPE injects at the emulated-keyboard level, so it reaches
  the guest whether it reads the BIOS buffer (`INT 16h`), the keyboard IRQ
  (`INT 9h` at `0xBC9CF`), or polls the controller (`in al,60h` at `0xCB94F`).
* `tools/trace_probe.sh` and the TSR probe stack are proven; the runner
  already supports `ISO`, `SESSION`, `HEADLESS`, `TIMEOUT`.

## Non-goals

* No TSR changes; no host window-focus work.
* No interactive/real-time input; scripts are fixed sequences with waits.
* No changes to default capture behavior when no keys file is supplied.
* No new probe targets beyond sites 7 and 8 in this slice.

## Interface: keys file

Plain text, one step per line:

```
# comment
WAIT KEYS...
```

* `WAIT` — decimal seconds, integer or float (e.g. `5`, `2.5`); the delay
  after the previous step. The helper emits CUMULATIVE absolute waits
  (`AUTOTYPE -w <sum of waits so far>`), because every `AUTOTYPE` command is
  scheduled at autoexec time and its `-w` is relative to its own invocation —
  cumulative waits preserve step order under that scheduling.
* `KEYS...` — one or more AUTOTYPE key names separated by whitespace and/or
  commas (AUTOTYPE's own argument syntax).
* Blank lines and lines whose first non-space character is `#` are ignored.
* A malformed line (missing/invalid wait, no keys) makes the parser exit
  nonzero with `error: line N: <reason>`. An invalid `--pace` value is the
  same class of error (`error: --pace: <reason>`).

Mapping: each step becomes exactly
`AUTOTYPE -w <cumulative WAIT> -p <PACE> <KEYS...>`.
`PACE` defaults to `0.1` seconds and is overridable with `--pace`.

## Components

**`tools/fifa96_keys.py`** (new) — pure string transform; no DOSBox
dependency. CLI: `fifa96_keys.py FILE [--pace P] [--check]`. Without
`--check`, prints one `AUTOTYPE` line per step to stdout. With `--check`,
prints nothing and exits 0/1 after validating.

**`tests/test_keys.py`** (new, CTest `test_keys`) — unit tests: comment/blank
skipping, integer and float waits, cumulative absolute waits across steps,
multi-key lines, comma separators, default and overridden pace, exact
`AUTOTYPE` output, and `--check` acceptance and rejection. Suite becomes 18
tests.

**`run-fifa96-capture.sh`** (modified) — gains `KEYS_FILE` (unset or empty:
today's behavior, byte-identical). When set: the file must exist and validate
(via `--check`); its generated lines are inserted into the `[autoexec]`
heredoc before the `FIFA96.EXE` line. Failure aborts before DOSBox starts.

**`tools/keys/`** (new) — named key sequences; starts with `skip-intro.keys`,
refined during the empirical loop.

## Data flow

`KEYS_FILE` → `fifa96_keys.py` → `AUTOTYPE` lines → DOSBox-X `[autoexec]` →
emulated keyboard → guest input path → game advances → T_PROBE frames and/or
new FILE opens in `captures/session-*/trace.bin`.

## Empirical loop and success criteria

Probe `0x14C18` (site 7) and `0x23B38` (site 8) with a keys file, decode with
`tools/fifa96_probe.py`, adjust the sequence, cap at six attempts per target.
Success for a target = at least one T_PROBE frame whose `caller_link` matches
its static census (`+5`). A target that still sees zero frames is recorded
honestly with the run's new FILE-open evidence (proof of forward progress) or
the unchanged trace mix (no progress). No fabricated results.

## Error handling

* Keys file missing/unreadable → run script aborts with a message; no DOSBox.
* Malformed keys line → `--check` exits nonzero with line number; run aborts.
* AUTOTYPE keys ignored by the guest → hidden-complexity stop: report and
  reconsider the TSR keyboard-injection fallback; do not improvise it in this
  slice.

## Testing

* Unit: `tests/test_keys.py` (contract above); `make test` must be 18/18.
* Integration: the two probe runs above; their decoder output and trace
  provenance are quoted in `docs/ghidra/FU13_headless_input.md`.
* `sh -n run-fifa96-capture.sh` stays clean.

## Deliverables

1. `feat(keys): scripted headless input for capture runs` — helper, tests,
   CTest registration, runner change, `tools/keys/skip-intro.keys`.
2. `docs(fu13): headless input and menu-gated entry reachability` — the input
   facility, the discovered sequence, and site 7/8 results with verbatim
   evidence.

## Risks

* The correct menu sequence is unknown; several 2–3 minute runs may be needed.
* The game may gate progression on timing rather than a specific key;
  `-w` waits and `-p` pacing are the tuning knobs.
* If the intro cannot be skipped by keys, the fallback is waiting through it
  (longer `TIMEOUT`) before injecting; recorded as a failed-attempt finding.
