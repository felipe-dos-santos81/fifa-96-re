# FIFA96 Headless Scripted Input Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the headless capture rig a reusable, tested key-script facility (keys file → `AUTOTYPE` lines) and use it to reach the two input-gated probe targets (`FUN_00014c18` at `0x14C18`, `FUN_00023b38` at `0x23B38`) that FU-12 proved unreachable unattended.

**Architecture:** A pure-Python helper converts a `WAIT KEYS...` step file into cumulative-wait `AUTOTYPE` lines; `run-fifa96-capture.sh` gains `KEYS_FILE` (unset → today's behavior) and injects those lines into the generated DOSBox conf's `[autoexec]`; a `DRY_RUN=1` mode prints the conf for testability. The existing probe runner drives the empirical loop, and results land in a FU-13 evidence doc.

**Tech Stack:** Python 3 unittest under CTest, POSIX shell, DOSBox-X `AUTOTYPE`, the proven DPMI probe rig (`tools/trace_probe.sh`, `tools/fifa96_probe.py`).

**Spec:** `docs/superpowers/specs/2026-10-01-fifa96-headless-input-design.md` (approved; includes the cumulative-wait correction `b2590a8`).

## Global Constraints

- All 17 existing CTest tests stay green; this slice adds `test_keys` (suite becomes 18). Only ADD tests.
- `game/FIFAPCCD96.iso` is read-only; `captures/`, `build/`, and `fifa96.rep/**` are never staged.
- No TSR changes. Default capture behavior when `KEYS_FILE` is unset must be byte-identical (same conf except nothing added, same launch command).
- Commit messages exactly as specified in each task. Do not `git push`.
- The empirical loop never fabricates a result: zero-frame outcomes are recorded with trace size and (when available) new FILE-open evidence.
- Probe cross-check semantics stay `caller_link = static opcode + 5` (census via `tools/fifa96_callers.py`; `0x14C18` → {`0x25D69`, `0x27B47`}; `0x23B38` → the twelve census sites + 5).

---

### Task 1: Keys-file parser and AUTOTYPE emitter

**Files:**
- Create: `tools/fifa96_keys.py`
- Create: `tests/test_keys.py`
- Modify: `CMakeLists.txt` (add `test_keys` in the Python block)

**Interfaces:**
- File format: blank/`#` lines ignored; step lines `WAIT KEYS...`; `WAIT` decimal seconds (int/float, ≥ 0) counted since the previous step; `KEYS...` kept verbatim (AUTOTYPE token syntax, commas allowed).
- `parse_keys(text) -> list[tuple[float, str]]`; raises `KeysError` with `line N:` in the message.
- `autotype_lines(steps, pace=0.1) -> list[str]`; each line `AUTOTYPE -w <cumulative> -p <pace> <keys>` with `:g` formatting.
- CLI: `fifa96_keys.py FILE [--pace P] [--check]`; prints the lines, or validates silently; errors go to stderr as `error: ...` with exit 1.

- [ ] **Step 1: Write the failing tests**

```python
# tests/test_keys.py
import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fifa96_keys as keys  # noqa: E402


class TestParse(unittest.TestCase):
    def test_comments_and_blanks_skipped(self):
        self.assertEqual(keys.parse_keys("# c\n\n  5 enter\n"),
                         [(5.0, "enter")])

    def test_int_and_float_waits(self):
        self.assertEqual(keys.parse_keys("5 enter\n2.5 esc\n"),
                         [(5.0, "enter"), (2.5, "esc")])

    def test_multi_key_line_kept_verbatim(self):
        self.assertEqual(keys.parse_keys("3 up enter , kp_8\n"),
                         [(3.0, "up enter , kp_8")])

    def test_missing_keys_raises_with_line(self):
        with self.assertRaises(keys.KeysError) as cm:
            keys.parse_keys("5\n")
        self.assertIn("line 1", str(cm.exception))

    def test_invalid_wait_raises(self):
        with self.assertRaises(keys.KeysError):
            keys.parse_keys("soon enter\n")

    def test_negative_wait_raises(self):
        with self.assertRaises(keys.KeysError):
            keys.parse_keys("-1 enter\n")


class TestAutotype(unittest.TestCase):
    def test_cumulative_waits_preserve_order(self):
        steps = keys.parse_keys("8 enter\n4 esc\n")
        self.assertEqual(keys.autotype_lines(steps),
                         ["AUTOTYPE -w 8 -p 0.1 enter",
                          "AUTOTYPE -w 12 -p 0.1 esc"])

    def test_pace_override(self):
        steps = keys.parse_keys("2 enter\n")
        self.assertEqual(keys.autotype_lines(steps, pace=0.3),
                         ["AUTOTYPE -w 2 -p 0.3 enter"])


class TestCli(unittest.TestCase):
    def _write(self, text):
        fh = tempfile.NamedTemporaryFile("w", suffix=".keys", delete=False)
        fh.write(text)
        fh.close()
        self.addCleanup(Path(fh.name).unlink)
        return fh.name

    def _run(self, argv):
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            rc = keys.main(argv)
        return rc, out.getvalue(), err.getvalue()

    def test_print_output(self):
        rc, out, _ = self._run([self._write("5 enter\n")])
        self.assertEqual((rc, out), (0, "AUTOTYPE -w 5 -p 0.1 enter\n"))

    def test_check_ok_is_silent(self):
        rc, out, _ = self._run(["--check", self._write("5 enter\n")])
        self.assertEqual((rc, out), (0, ""))

    def test_check_bad_reports_line(self):
        rc, out, err = self._run(["--check", self._write("nope\n")])
        self.assertEqual(rc, 1)
        self.assertIn("line 1", err)

    def test_bad_pace_reports(self):
        rc, _, err = self._run(["--pace", "fast", self._write("5 enter\n")])
        self.assertEqual(rc, 1)
        self.assertIn("--pace", err)

    def test_missing_file_reports(self):
        rc, _, err = self._run(["/nonexistent.keys"])
        self.assertEqual(rc, 1)
        self.assertIn("error:", err)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python3 tests/test_keys.py`
Expected: `ModuleNotFoundError: No module named 'fifa96_keys'`.

- [ ] **Step 3: Implement the helper**

```python
#!/usr/bin/env python3
"""fifa96_keys.py — turn a key-step file into DOSBox-X AUTOTYPE lines.

File format (one step per line):
    WAIT KEYS...
WAIT is decimal seconds since the previous step; KEYS are AUTOTYPE tokens
(e.g. `enter`, `esc`, `up`, `kp_8`, commas allowed). Waits are emitted
cumulatively: every AUTOTYPE command is scheduled at autoexec time and its
`-w` is relative to its own invocation, so cumulative absolutes preserve
step order.
"""
import argparse
import sys


class KeysError(ValueError):
    pass


def parse_keys(text):
    steps = []
    for n, line in enumerate(text.splitlines(), 1):
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        parts = s.split(None, 1)
        if len(parts) < 2 or not parts[1].strip():
            raise KeysError(f"line {n}: missing keys")
        try:
            wait = float(parts[0])
        except ValueError:
            raise KeysError(f"line {n}: invalid wait {parts[0]!r}")
        if wait < 0:
            raise KeysError(f"line {n}: negative wait")
        steps.append((wait, parts[1].strip()))
    return steps


def autotype_lines(steps, pace=0.1):
    out = []
    total = 0.0
    for wait, key_text in steps:
        total += wait
        out.append(f"AUTOTYPE -w {total:g} -p {pace:g} {key_text}")
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description="keys file -> AUTOTYPE lines")
    ap.add_argument("file")
    ap.add_argument("--pace", default="0.1")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args(argv)
    try:
        pace = float(args.pace)
        if pace < 0:
            raise ValueError
    except ValueError:
        print(f"error: --pace: invalid pace {args.pace!r}", file=sys.stderr)
        return 1
    try:
        with open(args.file) as fh:
            steps = parse_keys(fh.read())
    except (OSError, KeysError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.check:
        return 0
    for line in autotype_lines(steps, pace=pace):
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 4: Register and run the suite**

```cmake
# CMakeLists.txt, inside the if(Python3_Interpreter_FOUND) block
  add_test(NAME test_keys COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_keys.py)
```

Run: `make test`
Expected: 18/18 pass.

- [ ] **Step 5: Commit**

```bash
git add tools/fifa96_keys.py tests/test_keys.py CMakeLists.txt
git commit -m "feat(keys): parse key-step files into cumulative AUTOTYPE lines"
```

---

### Task 2: Capture-runner integration and key library

**Files:**
- Modify: `run-fifa96-capture.sh`
- Create: `tools/keys/skip-intro.keys`

**Interfaces:**
- Consumes: `fifa96_keys.py FILE` / `--check` (Task 1).
- Produces: `KEYS_FILE` env (unset/empty → unchanged run) and `DRY_RUN=1` (print the generated conf and exit 0 before launching DOSBox); unused by default.

- [ ] **Step 1: Add the key wiring and dry-run**

At the top of `run-fifa96-capture.sh`, beside the other defaults:

```sh
KEYS_FILE=${KEYS_FILE:-}
DRY_RUN=${DRY_RUN:-0}
```

After the `[ -f "$COM" ]` check, add:

```sh
KEYS_LINES=""
if [ -n "$KEYS_FILE" ]; then
  [ -f "$KEYS_FILE" ] || { echo "missing keys file: $KEYS_FILE" >&2; exit 1; }
  python3 "$DIR/tools/fifa96_keys.py" --check "$KEYS_FILE"
  KEYS_LINES=$(python3 "$DIR/tools/fifa96_keys.py" "$KEYS_FILE")
fi
```

Inside the conf heredoc, immediately before the `FIFA96.EXE` line, add:

```
$KEYS_LINES
```

Just before the final DOSBox invocation block, add:

```sh
if [ "$DRY_RUN" = "1" ]; then
  cat "$CONF"
  exit 0
fi
```

- [ ] **Step 2: Write the starter key sequence**

```text
# skip-intro.keys — candidate intro progression, refined empirically.
# WAIT is seconds since the previous step (emitted cumulatively).
8 enter
4 enter
4 space
4 esc
```

- [ ] **Step 3: Verify behavior**

Run:
```sh
sh -n run-fifa96-capture.sh
DRY_RUN=1 KEYS_FILE=tools/keys/skip-intro.keys ./run-fifa96-capture.sh | grep -c '^AUTOTYPE'
DRY_RUN=1 ./run-fifa96-capture.sh | grep -c '^AUTOTYPE' || true
```
Expected: syntax clean; first count `4`; second count `0` (default unchanged).

- [ ] **Step 4: Run the suite and commit**

Run: `make test` (18/18).

```bash
git add run-fifa96-capture.sh tools/keys/skip-intro.keys
git commit -m "feat(keys): scripted headless input for capture runs"
```

---

### Task 3: Empirical campaign and FU-13

**Files:**
- Modify: `tools/keys/skip-intro.keys` (as refined)
- Create: `docs/ghidra/FU13_headless_input.md`

**Interfaces:**
- Consumes: `KEYS_FILE` (Task 2), `tools/trace_probe.sh TARGET SITE SESSION`, `tools/fifa96_probe.py` (existing), `tools/fifa96_callers.py` (existing).
- Produces: FU-13 with the facility description, the discovered sequence, and per-target reachability results, quoting verbatim decoder lines and trace provenance.

- [ ] **Step 1: Site 7 attempts (max 6)**

For each attempt: adjust `tools/keys/skip-intro.keys` if the previous attempt made no forward progress, then run

```sh
KEYS_FILE=tools/keys/skip-intro.keys sh tools/trace_probe.sh 0x14C18 7 probe-14c18-keys
```

Read the decoder output. Success = ≥1 `T_PROBE site=7` frame with `caller_link` in `{0x25D69, 0x27B47}`. Record every attempt (session name, keys steps, trace size, frames or zero). Forward progress without frames = new FILE opens appearing in the trace compared with the FU-12 baseline (`captures/session-probe-14c18/trace.bin`, 314,318 bytes). Stop at success or 6 attempts.

- [ ] **Step 2: Site 8 attempts (max 6)**

Same loop for

```sh
KEYS_FILE=tools/keys/skip-intro.keys sh tools/trace_probe.sh 0x23B38 8 probe-23b38-keys
```

with the twelve-site census `+5` as the match set, comparing against the FU-12 baseline (`captures/session-probe-23b38/trace.bin`, 314,753 bytes). Stop at success or 6 attempts.

- [ ] **Step 3: Write FU-13**

Create `docs/ghidra/FU13_headless_input.md`: the `KEYS_FILE` facility and file format; the discovered sequence with its cumulative waits; a per-target table (attempt, session dir, trace bytes, frames, matched `caller_link`s or zero-frame verdict); explicit statement when input failed to reach a target (no fabricated pass); capture provenance.

- [ ] **Step 4: Run the suite and commit**

Run: `make test` (18/18).

```bash
git add tools/keys/skip-intro.keys docs/ghidra/FU13_headless_input.md
git commit -m "docs(fu13): headless input and menu-gated entry reachability"
```

---

## Self-Review (ran before save)

- **Spec coverage:** keys format + cumulative waits (T1 tests `cumulative_waits_preserve_order` ← spec interface); helper CLI `--check`/`--pace` (T1 ← spec components); runner `KEYS_FILE` + default-unchanged + abort-on-bad-file (T2 ← spec components/error handling); keys library (T2 ← spec); empirical loop cap 6 with honest zero-frame recording (T3 ← spec empirical loop); FU-13 deliverable (T3 ← spec deliverables). No TSR changes, ISO read-only, suite 18 (constraints).
- **Placeholder scan:** no TBD/TODO; test code verbatim; the helper implementation complete; exact commands and expected outputs given; the starter keys file is concrete.
- **Type consistency:** `parse_keys`/`autotype_lines`/`KeysError`/`main` names match between tests and implementation; env names `KEYS_FILE`/`DRY_RUN` match between T2 and T3; `AUTOTYPE -w <cumulative> -p <pace>` matches T1 tests, the spec, and the T2 dry-run grep; match sets `{0x25D69, 0x27B47}` and site-8 `+5` agree with `tools/fifa96_callers.py` output.

(End of file)
