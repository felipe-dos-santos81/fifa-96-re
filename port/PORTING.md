# Porting rules — FIFA 96 (DOS, 1995) → native C

The binding rules are [`../AGENTS.md`](../AGENTS.md) at the repository root.
The short version:

- Every address/byte claim cites a disassembly output or a reconciled raw-byte
  read; anything not statically provable is a numbered open leg.
- The null backend is the determinism source of truth; the SDL3 backend must
  match it.
- `make check` from the repository root is the gate.
- The original ISO (`../data/game/FIFAPCCD96.iso`) is read-only; never write to it.

The C port lives in `src/` (loader and engine), the tests in `tests/`, and the
host runner is `tools/fifa96_play.c`.
