# M2 phase-36 — the completion census: 100% of native dispatch bodies ported

**Status:** proposed 2026-11-04. Supersedes no prior section; the phase-35
register stands as written.

## 1. The goal

Close the port surface of the match dispatch layer: every native dispatch slot
that has a body must have a faithful C implementation, and every row whose
producer is live must be wired. The README title percentage is re-derived from
this **port-coverage** census, not from runtime wiring alone. The wired count
stays a separate, stricter metric.

## 2. Why the metric moves

The phase-35 close-out counts `60/80` wired rows (`37 action + 23 phase`).
That census is honest but it is a reachability census: a row whose body is
ported but whose installer is dead or unreachable is deliberately not counted
(the rows-27/29/2C precedent). It therefore has a hard ceiling below 100%:
two slots have no native body at all.

A dispatch slot is one of:

- **native-body slot** — the native table entry points at executable code
  (including a shared body or a bare `RET`);
- **body-less slot** — the native entry is the loader INT3 sentinel (`0`) or a
  shared epilogue that is not a row body.

The port is complete when every native-body slot has a C implementation. A
dead or never-written row still has a body to reverse and port; it simply has
no live producer, which is a separate question.

Current census, first-hand from the two tables in
`src/fifa96_engine/fifa96_match_handlers.c`:

| Measure | Count |
|---|---|
| Action slots | 45 (`0x1106E0[45]`) |
| Phase slots | 35 (`0x110794[35]`) |
| Total slots | 80 |
| Body-less slots | 2 — phase `0x16` (native `0` / INT3) and action `0x2B` (shared row-29 epilogue `RET` at `0x87738`) |
| Native-body slots | **78** |
| Bodies already ported | 75 |
| Bodies missing | **3** — action `0x0A` (`0x81738`), phases `0x13/0x14` (shared `0x6E244`), phase `0x1B` (`0x890EC`) |
| Wired rows | 60 (37 action + 23 phase) |

`75/78 = 96%` on the port-coverage metric today; `60/80 = 75%` on the wired
metric. The three missing bodies are the whole gap to `78/78 = 100%`.

## 3. The completion definition

Let `ported` be the number of native-body slots whose C implementation is
linked into the engine and covered by at least one unit fixture; let
`native_body` be `78` (80 minus the two body-less slots). The README title is

```
floor(100 * ported / native_body)
```

so `78/78 = 100%`. The ENGINE headline keeps both numbers:
`78/78 native bodies ported (100%) — N/80 dispatch slots wired`.

The `N` may remain below 78. That is not a completion failure: a row with no
producer cannot be dispatched without fabricating one, which the evidence
rules forbid. Each such row is recorded as ported-but-dormant with the fresh
producer census that justifies the dormancy.

## 4. The three body ports

### 4.1 Action `0x0A` — `0x81738..0x818F5`

The only unported action body. FU-76 §2, FU-138 §2.5 and FU-177 §4 carry the
shape: phase `== 2` gate, stages 0/1, ball-relative run/placement
(`NEAREST`, `ANGLE`, `FMUL`, `TMR93`, `RESET`). The installer `0x7CDD8` is in
an xref-dead cluster (`0x7CDD8..0x7CF1F`, FU-242 §6), so the body is
**ported, not wired** unless a fresh indirect-entry proof is found.

### 4.2 Phases `0x13/0x14` — `0x6E244..0x6E32A`

The shared camera-scatter body (FU-83 §2/§3.1, FU-192 §2.10). It consumes the
PRNG `0x92AC8` twice per active record and the `0x10F364/0x10F368` cells; the
producer is the `FUN_0008D098` case-`0x13/0x14` scatter. The port must model
the RNG consumption and the cell producer, then the row can be wired if the
match's phase machine reaches `0x13/0x14`.

### 4.3 Phase `0x1B` — `0x890EC`

The act-5 pure-reset body (FU-83 §2/§3.2, FU-195 §5). A fresh invoker census
must confirm the absence of an `EAX=5` invoker before the row is declared
ported-but-dormant.

## 5. Wiring wave

After the three bodies land, wire every row whose producer is provably live:

- action `0x03` with phases `0x13/0x14` (the tape-visible special-arm conflict,
  FU-177/FU-258);
- action `0x0D` through the row-09 arm-1 install (`0x80F61`, FU-178/FU-190);
- phase `0x0B`/`0x0D` through the now-wired row-`0x1A`/`0x1B` situation-7
  producers (FU-261);
- phase `0x1C` through the offside route as the sole owner (FU-193 §4);
- any further row a fresh census re-opens (the FU-259 precedent: a stale
  "no producer" verdict is refuted only by first-hand evidence).

Every wiring is install-live or dispatch-live and is pinned by a fixture. A
change that moves a presented frame is re-pinned only with a written reason
and a frame diff; the default is tape-neutral.

## 6. Evidence rules

- Each new body cites a raw disassembly window and a reconciled byte read.
- Each new producer claim cites a fresh `get_xrefs_to` in the same session.
- No row is wired without a producer; no body-less slot is counted.
- Prior registers are append-only; corrections land as quoted errata.
