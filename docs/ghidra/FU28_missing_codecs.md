# FU-28 — the missing record codecs: capture attempt and asset scan

Date: 2026-10-03/04. Goal: golden vectors for the record selectors that
have no vector yet (`0x16` lz_16fb, `0x60/0x62/0x66/0x72` delta_prefix,
`0x7A` rle_row) so FU-22/FU-24/FU-25-style derivations could be
validated. Result: **honest negative** — no runtime path reached in
scripted gameplay dispatches those selectors, and the apparent
`[selector, 0xFB]` candidates in shipped assets are statistically
indistinguishable from noise. No vectors are committed by this slice.

## 1. Runtime sweep (FU-21 rig, per-method slots)

The FU-21 ISO (`build/fifa96-vgt21.iso`, method-selectable slots,
`LENGTH_CONTRACT_METHODS` bar) was run over flows not previously covered.
The entry probe records the raw `stream[0]` byte of every
`decode_record_dispatch` call in the trace, so the observed method set is
exact even for records the size filter excludes.

| session | keys | entry frames | observed raw |
|---|---|---|---|
| vgt21-4 | `fu17-deep-flows.keys` | 25 | `0x10` |
| vgt21-5 | `fu18-friendly-walk.keys` | 43 | `0x10` |
| vgt21-6 | `fu15-verified-nav.keys` | 27 | `0x10` |
| vgt21-7 | `fu18-friendly-cycle.keys` | 38 | `0x10` |
| vgt21-8 | `fu18-visitor-next.keys` | 45 | `0x10` |
| vgt21-9 | `fu17-simulate.keys` | 25 | `0x10` |
| vgt21-10 | none (looping intro, long wait) | 296 | `0x10`, `0x31`, `0x46` |

Command shape (sessions 4..6 verbatim in FU-21 §5; 7..10 identical):

```
DOSBOX_X=/tmp/opencode/dosbox-x-nocap \
python3 tools/fifa96_vgt_capture.py run --iso build/fifa96-vgt21.iso \
    --session vgt21-N --wait 360..480 --min-frames 20..100 --settle 180..240
```

Across all ten FU-21 sessions (700+ descriptor frames, front-end menus,
league/tournament screens, friendly team select/editor, a full 640x480
surface load and a 296-frame no-keys intro run) the dispatcher saw only
selectors **0x10 (refpack)**, **0x30 (huff, raw `0x31`)** and **0x46
(tree)**. `0x16`, `0x60`, `0x62`, `0x66`, `0x6A`, `0x6E`, `0x72`, `0x7A`
never fired.

## 2. Asset scan methodology and calibration

All ISO files were walked with `tools/fifa96_bind.iso_files` and scanned
for the record header shape `[sel, 0xFB, BE24]` with `sel & 0xFE` in the
dispatcher's valid set (FU-21 §2) and a plausibility filter
`0x20 <= BE24 <= min(0x40000, remaining_bytes)`.

Calibration against known truth:

* `record-10`'s 16-byte prefix occurs exactly once in the ISO, at
  `/FEDATA/LANG000.POG` offset `0x0` — the whole 6089-byte file is the
  record; it is a bare record asset.
* `record-30`'s 16-byte prefix occurs exactly once, inside
  `/ART/PLAYART.PVI` at offset `0xb4747` — a real raw record inside a
  BIGF/SHPI envelope container.
* `record-46`'s prefix does **not** occur raw anywhere (even 5 bytes):
  the tree record input captured at runtime exists only in an in-memory
  resolved/unpacked buffer (the dispatcher's `decode_stream_resolve`
  step), not verbatim in a file.

Noise floor: for a random 300 KB byte stream the filter matches about
once per selector (`13/65536 x 300e3/64 ~= 1`). Measured candidate rates
match that floor everywhere except `/ART/PLAYART.PVI`:

* `PLAYART.PVI` (807 KB): 73 x `0x30` candidates — the real huff records
  (record-30 is one of them); the few `0x60/0x62/0x66/0x72` candidates
  (1-3 each) are at chance level.
* `FEGFX/MOD*.POG` (25-400 KB screens): 0-3 candidates per file — chance
  level. These are the MOD screens already visited in FU-15/16/17
  (MOD2 friendly, MOD5 front-end, MOD6 calendar, MOD9 tournament); none
  dispatched a missing selector at runtime.
* `SOUND/QUE_*.ASF` (7-14 MB music): 3-42 candidates per selector —
  exactly the `file_size / 300 KB` chance rate. Entropy noise.
* `VIDEO/*.TGV`: contains 3609 `kVGT` frame magics; its candidate counts
  are also chance-level after the plausibility filter.

## 3. Offline decode cross-check

A throwaway decoder harness (`/tmp/opencode/fu28/decrec.c`, linking the
FU-23 `fifa96_refpack_decode`) decompressed every raw refpack candidate
in the ART/FEGFX/FEDATA/SOCCER files and searched the decoded outputs for
embedded `[sel, 0xFB]` headers of the missing selectors. Findings:

* `FEGFX/MOD5.POG` offset `0x200` is a real refpack record: 307200 bytes
  decoded (= 640x480, a full-screen surface).
* No embedded missing-selector records surfaced: the "inner" hits all
  carried absurd BE24 values (0x400000-0xe1de.. range), i.e. pixel data
  noise. One level of container unwrapping is not enough to reject every
  hypothesis, but combined with §1 the negative is consistent.

## 4. Conclusion

The dispatcher tree supports these selectors, but the shipped build, in
every context reachable by scripted headless input, dispatches only
`0x10`, `0x30` and `0x46`. The apparent missing-selector records in
assets are at the statistical noise floor; no capture or derivation
target exists for them yet. Dispositions:

* `0x16`, `0x60`, `0x62`, `0x66`, `0x72`, `0x7A` stay **UNSUPPORTED** in
  `fifa96_record_decode` (honest: no vector, no derived spec).
* `0x6A/0x6E` (literal copy) are ported from FU-19's transcription and
  synthetic tests, no asset vector needed.
* A future vector would have to come from an unscripted context (match
  replay / credits video playback) or from an asset whose unpacked form
  is captured at runtime; the FU-20/21 rig can capture it unchanged if
  such a call ever occurs.

## 5. Provenance

* Runtime: sessions `captures/session-vgt21-4..10` (git-ignored),
  commands in §1; observed sets via
  `fifa96_vgt_capture.observed_methods` (FU-21 entry descriptor frames).
* Scan: `tools/fifa96_bind.iso_files`; calibration searches for the
  committed vectors' 16-byte prefixes; the filtered per-file candidate
  table; `kVGT` magic census (3609).
* Offline: `cc -Iinclude src/fifa96_loader/fifa96_refpack.c
  /tmp/opencode/fu28/decrec.c -o decrec` (throwaway, not committed);
  run over 171 ART/FEGFX/FEDATA/SOCCER files.
* `make test`: 25/25 before and after (no code changed in this slice).
