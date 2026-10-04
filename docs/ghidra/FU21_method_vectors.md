# FU-21 — per-method VGT golden vectors

Date: 2026-10-03. Program: `/fifa96_le.bin` (LE link-time flat
addresses). Rig: `tools/fifa96_vgt_capture.py` + the two-point cave in
obj1's zero run (`0x6728D` scratch, `0x67291` entry probe, `0x672F7`
return capture) introduced in FU-20 and extended to per-method slots in
`feat(rig): per-method vgt golden slots` (7816640).

Goal: one committed golden pair per VGT record selector observed at
`decode_record_dispatch` (`0x9E718`). Result: selector `0x30` (huff) and
`0x46` (tree) captured alongside the existing `0x10` (refpack); ten other
valid selectors were never observed in the reachable screens (honest
zeros, §6).

## 1. Committed vectors

| vector | raw `stream[0]` | selector | decoder | out_len | input slice |
|---|---|---|---|---|---|
| `tests/golden/vgt/record-10.*` | 0x10 | 0x10 | `lz_refpack_decode` (0xB18F8) | 10068 | 17408 B (FU-20) |
| `tests/golden/vgt/record-30.*` | 0x31 | 0x30 | `huff_32fb_decode` (0x9C7E0) | 2271 | 17408 B |
| `tests/golden/vgt/record-46.*` | 0x46 | 0x46 | `tree_47fb_decode` (0x9DA14) | 4696 | 17408 B |

Every vector satisfies: `in[1] == 0xFB`, selector in the dispatcher's
valid set, non-trivial output, `out_len` == the captured decoder return;
`record-10.*` bytes are pinned by SHA-256 in `tests/test_vgt_capture.py`.

## 2. Dispatcher selector set (FU-19 errata)

`decode_record_dispatch` (`0x9E718..0x9E85E`) masks `stream[0] & 0xFE`
(`0x9E757 MOV AL,[EBX]` / `0x9E759 AND AL,0xFE`) and walks this tree
(verbatim call sites from `ghidra_disassemble_function 0x9E718`):

| selector | handler | call site | FU-19 table |
|---|---|---|---|
| 0x10 | `lz_refpack_decode` | `0x9E7E4` | correct |
| 0x16 | `lz_16fb_decode` | `0x9E7F4` | correct |
| 0x30, 0x32, 0x34 | `huff_32fb_decode` | `0x9E7FF` | **0x30 missing** |
| 0x46 | `tree_47fb_decode` | `0x9E80F` | correct |
| 0x60, 0x62, 0x66, 0x72 | `delta_prefix_decode` | `0x9E81A` | **0x70 listed (invalid)** |
| 0x6A, 0x6E | literal copy `memmove_bytes` | `0x9E825` | **0x6E grouped with delta** |
| 0x7A | `rle_row_decode` | `0x9E837` | correct |
| anything else | error/return 0 | `0x9E840` | correct |

The three errata are decisive at the byte level: `0x9E7B6 CMP AL,0x30`
falls through to `0x9E7FF` (huff, `JBE`); `0x9E78E CMP AL,0x6E` +
`JZ 0x9E825` routes `0x6E` to the literal copy; `0x9E796 JMP 0x9E840`
sends `0x70` to the error path (only `0x72` reaches delta via
`0x9E773 JBE 0x9E81A`). FU-19 §"dispatch argument conventions" therefore
over-counted delta and under-counted huff.

## 3. Length semantics (why the success bar is method-aware)

Every handler path leaves the decoder's return in ESI (`MOV ESI,EAX` at
`0x9E7ED`, `0x9E7FB`, `0x9E80B`, `0x9E816`, `0x9E821`; literal copy
`MOV ESI,EBP` at `0x9E82B`; unknown selector `XOR ESI,ESI` at `0x9E74A`)
and the common epilogue returns it (`0x9E859 MOV EAX,ESI`). The cave
captures exactly that many output bytes.

The generic header parse (`0x9E731 ADD EAX,2` … `0x9E743 MOV EBP,EAX`)
loads a 24-bit big-endian field at `stream+2` into EBP. That field equals
the decoded length only for:

* `0x10` — evidence: record-10 (10068 == 10068), the FU-20 vector.
* `0x6A`/`0x6E` — by construction: `0x9E825 PUSH EBP` … `MOV ESI,EBP`
  (`0x9E82B`) makes the return equal EBP.

For the other methods the field is not the decoded length:
`huff_32fb_decode` returns its own stream-declared size (local_6c; 2271
for record-30 while `header24` = 4696), and `tree_47fb_decode` returns
its own BE24 (local_18; 4696 for record-46, equal to `header24` here but
not enforced). Accordingly `tools/fifa96_vgt_capture.py` enforces
`length_matches` only for `LENGTH_CONTRACT_METHODS = {0x10, 0x6A, 0x6E}`
and keeps `header24` in every JSON as informational.

## 4. Slot and extraction semantics

* The cave stores the **raw** `stream[0]` byte in the slot
  (`movzx edi,byte [ebp]` / `mov [eax+4],edi`); the selector is
  `raw & 0xFE`. Copy-once is per raw byte, so a second record of the same
  selector with a different low bit would be stored as a second slot;
  extraction then reports a selector duplicate (copy-once is enforced at
  the selector level there).
* record-30 is the first odd raw value in a vector (`0x31`). Files and
  JSON are named/keyed by selector (`method = 0x30`), with `method_raw =
  0x31` recorded for provenance.
* `extract`/`extract_records` accept a `--method` selector filter.
  Unrequested slots are parsed but not validated against the success bar,
  so one bad slot no longer blocks extraction of the good ones
  (`test_filter_ignores_bad_unrequested_slot`); a requested selector with
  no slot is a hard error.
* Legacy FU-20 single-block dumps still parse
  (`test_legacy_single_block_still_parses`).
* `record-10.json` was backfilled with `method_raw`/`length_contract`
  metadata; `.in.bin`/`.out.bin` are byte-identical (SHA-256 pins).

## 5. Sessions (verbatim)

```
# patched ISO (FU-20 flag, reused for all six sessions)
python3 tools/fifa96_patch.py --iso game/FIFAPCCD96.iso \
    --out build/fifa96-vgt21.iso --target 0x9E718 --vgt-capture \
    --cave 0x6728D --site-id 1

# sessions 4..6 of this pass (1..3 from the interrupted first pass used
# the same patched ISO; their AUTOTYPE lines live in each session's
# dosbox.conf under captures/, which is git-ignored)
DOSBOX_X=/tmp/opencode/dosbox-x-nocap \
python3 tools/fifa96_vgt_capture.py run --iso build/fifa96-vgt21.iso \
    --session vgt21-4 --wait 420 --min-frames 20 --settle 300 \
    --keys tools/keys/fu17-deep-flows.keys
DOSBOX_X=/tmp/opencode/dosbox-x-nocap \
python3 tools/fifa96_vgt_capture.py run --iso build/fifa96-vgt21.iso \
    --session vgt21-5 --wait 420 --min-frames 20 --settle 300 \
    --keys tools/keys/fu18-friendly-walk.keys
DOSBOX_X=/tmp/opencode/dosbox-x-nocap \
python3 tools/fifa96_vgt_capture.py run --iso build/fifa96-vgt21.iso \
    --session vgt21-6 --wait 420 --min-frames 20 --settle 300 \
    --keys tools/keys/fu15-verified-nav.keys

# extraction (vgt21-3 held the huff + tree slots)
python3 tools/fifa96_vgt_capture.py extract \
    --dump captures/session-vgt21-3/guest.bin \
    --trace captures/session-vgt21-3/trace.bin --session vgt21-3 \
    --method 0x30 --method 0x46
```

Per-session observations:

| session | observed selectors (`raw`) | slots |
|---|---|---|
| vgt21-1 | `0x83` | one `0x10` slot, `out_len 0` — rejected by the bar |
| vgt21-2 | `0x10` | `0x10` (10068) — same front-end record as record-10 |
| vgt21-3 | `0x10`, `0x31`, `0x46` | `0x30` (2271) + `0x46` (4696) extracted |
| vgt21-4 | `0x10` | `0x10` |
| vgt21-5 | `0x10` | `0x10` |
| vgt21-6 | `0x10` | `0x10` |

Environment note: `/tmp/opencode/dosbox-x-nocap` (the memory-dumpable
DOSBox-X) had been cleared from `/tmp` and was recreated as
`cp /usr/bin/dosbox-x /tmp/opencode/dosbox-x-nocap` (the file-capability
`cap_net_raw=ep` is not copied, so the copy's guest RAM is dumpable).
`FIFACAP.COM` is unchanged from FU-20 (`build/FIFACAP.COM`, sha256
`83dd6a1de341e0bbd53ccc0a47ebf5071818efe9351ffa4155f21e775b89a30b`).

## 6. Honest zeros

Across all six sessions (front-end menus, league/tournament screens,
friendly team select/editor, deep-flow navigation) the following valid
selectors were **never observed**: `0x16`, `0x32`, `0x34`, `0x60`,
`0x62`, `0x66`, `0x72`, `0x6A`, `0x6E`, `0x7A`. No vectors are
fabricated for them. Raw `0x83` (selector `0x82`, dispatcher error path)
was observed once — memory adjacent to records can carry a `0xFB` second
byte, so the `method_known` bar is load-bearing.

## 7. What this enables

The C port can now implement and validate, byte-for-byte:
`lz_refpack_decode` (record-10), `huff_32fb_decode` (record-30), and
`tree_47fb_decode` (record-46). Each decoder must consume
`record-XX.in.bin`, return the recorded `out_len`, and reproduce
`record-XX.out.bin` exactly. The remaining decoders still need either
vectors from other screens or static completion of the FU-19 open legs.

## 8. Provenance

* Ghidra `/fifa96_le.bin` (bridge 2026-10-03):
  `disassemble_function 0x9E718` (full 112-instruction body, selector
  tree and epilogue); `decompile_function 0x9C7E0` (huff return =
  stream-declared size) and `0x9DA14` (tree return = own BE24).
* `make test`: **21/21** (new: `test_vgt_slots.py` selector/contract
  cases; extended `test_vgt_capture.py` committed-vector checks).
* Extraction math and delta handling are inherited from FU-20
  (`docs/ghidra/FU20_vgt_golden_capture.md`), including the
  `0x1FC000` load delta recorded per session.
