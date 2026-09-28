# FU-1 / FU-2 Closeout (FIFA96 file-loader follow-ups)

Date: 2026-09-28. Scope: honestly close out FU-1 (per-function Ghidra citations, spec §8) and FU-2 (crcvals reader + CRC, spec §7) using only findings that were actually verified. No invented citations.

## FU-1 — per-function Ghidra citations (spec §8)

Evidence reports (the two verified passes):

- `.superpowers/sdd/2026-09-27-fifa96-file-loader/fu1-ghidra-citation-report.md` (pass 1, read-only)
- `.superpowers/sdd/2026-09-27-fifa96-file-loader/fu1-ghidra-pass2-report.md` (pass 2, promotion + decompile + magic scan)

### What is VERIFIED

The 5 INT-21h file wrappers are CONFIRMED by disassembly (pass 1 §(a), each decompiled + disassembled fresh, all carrying the carry-idiom `SBB BX,BX` / `OR AX,BX`, handle cell `0xe70`):

| Wrapper | Address | AH | Counterpart in `fifa96_file.c` (plate-verified) |
|---|---|---|---|
| `file_open_dos` | `11bd:5fb8` | 0x3D/0x00 | `fifa96_file_read` (open path) |
| `file_seek_dos` | `11bd:5fca` | 0x42/0x00 | `fifa96_file_read_chunk` (seek path) |
| `file_read_dos` | `11bd:5fe2` | 0x3F | `fifa96_file_read` (read path) |
| `file_close_dos` | `11bd:5ff7` | 0x3E | `fifa96_file_free` (close path) |
| `file_read_far_dos` | `11bd:6003` | 0x3F (far buffer, `PUSH DS`/`POP DS` triplet) | `fifa96_file_read_chunk` (far-buffer read) |

### What is NOT locatable (and why)

The 5 asset-format decoders (POG/QFS/VIV/TGV and the fnames/lengths table readers) are **not statically locatable** in the EXE with the evidence standard the spec demands:

- All 5 format tags — PCNX, GIMX, SHPI, BIGF, kVGT — return **0 hits** in every encoding tried (ASCII / LE-16 / LE-32; 15 probes total, pass 2 §4.1). No 6-LE-16 `MOV` immediate matches either.
- No 12-byte NUL-padded name-record dispatch, no stride-4 u32 offset-table walk in scope, no `REP CMPS`-style string compare (0 hits program-wide).
- The "decoder lives in the 1000 segment" hypothesis is **REFUTED** by the pass-2 candidate audit: the 4 `1000:xxxx` call targets resolve to already-known 11bd-side functions — `1000:7b88` → `file_open_dos` (11bd:5fb8, the wrapper itself), `1000:3e7d` → `FUN_11bd_22ad` (decimal error/number formatter), `1000:7982` → `FUN_11bd_5db2` (thin thunk into `FUN_11bd_2718`), `1000:84d7` → `FUN_11bd_6907` (context-block writer). Segment 1000 is a DOS-overlay alias of 11bd; 1000-segment function count remains 2 (`caseD_0`, `FUN_1000_0b12`). Neither hypothesis (content-magic decoder, or filename-dispatch decoder) is confirmed by direct evidence.
- Note (pass 1 §(c)): segment overlap makes the 1000-vs-11bd mapping a Ghidra analysis artifact, not evidence that the decoders are absent — but no decoder-shaped code was found under either view.

### Conclusion / resolution

Cite the 5 **WRAPPER** functions in the 5 decoder header comments — they are real, verified, and are the genuine file-access entry points every decoder call goes through (`fifa96_file.c` is the shared I/O layer in the C port). State plainly that the format-specific parse logic is **not statically distinguishable in the EXE** and was reconstructed **behaviorally** from the golden CD data (tests/golden/ + the decoder tests), per the two pass reports above. No `FUN_11bd_XXXX` parser address is cited anywhere, because none was verified — inventing one would violate the spec's evidence standard.

The decoder headers are updated accordingly (see `include/fifa96_loader/fifa96_{pog,qfs,viv,tgv,tables}.h`).

## FU-2 — crcvals reader + CRC (spec §7)

Research was attempted and is closed out as a **genuine blocker**, not a data-only task:

- `crcvals.dat` is 438 B = 219 × u16 — consistent with ~217 name-aligned entries plus the two leading zero values.
- Name↔length alignment is **not cleanly 1:1**: in the CHN_*_BNK region the pairing is offset-shifted (e.g. name[23] = `CHN_ARG1.BNK` pairs length 21576, but the resolved file size is 32656).
- The crcvals values do **not** match any standard 16-bit CRC of the RAW file bytes: full sweep of poly {0x1021, 0x8005, 0xA001, 0x1DCF} × init {0, 0xFFFF, 0x8005, 0xA001} × ref-in × ref-out × xorout against raw bytes of FW11.QFS, FW12.QFS, FW2.QFS, and FW3.QFS produced no match.

Conclusion: the CRC is almost certainly computed over **transformed content** — post-decompression or payload-only — and cannot be reproduced from raw file data until the decompression/decoder is understood. Blocking this is a runtime/RE effort (DOSBox-X INT-21 trace, or full decoder RE), not something a data-only reader can responsibly guess at.

`FIFA96_ERR_CRC_MISMATCH` therefore **remains reserved with no producer** in `fifa96_err.h` — that reservation is correct as-is and is intentionally not changed. No crcvals reader was implemented (it would be guesswork). The spec's narrow-scope line (spec §7) is already correct and is not touched.

## Files changed by this closeout

- `docs/ghidra/FU1_FU2_closeout.md` (this file)
- `include/fifa96_loader/fifa96_pog.h` — pending citation replaced with verified wrapper citation + honest behavioral note
- `include/fifa96_loader/fifa96_qfs.h` — same
- `include/fifa96_loader/fifa96_viv.h` — same
- `include/fifa96_loader/fifa96_tgv.h` — same
- `include/fifa96_loader/fifa96_tables.h` — same

No code, no API, no error enum, no spec changes.

## FU-3 — codec runtime capture (2026-09-28)

Task 2's funnel pass CONFIRMED six behavioral names (`load_mf_object`,
`mem_grow_relocate`, `exec_loaded_image`, `alloc_retry_loop`,
`copy_bytes_far`, `int21_dispatch`) but its Block-walk verdict is DEFER: no
byte-decode transform and no next-tag/termination rule were observed, so the
(a)(b)(c) codec criterion fails on all three and no `fifa96_codec_expand` was
implemented. The runtime experiment that could still prove a codec — DOSBox-X
INT 21h trace delimiting one `load_mf_object` invocation, with input/output
buffer captures and byte-exact vectors — is recorded in
`docs/ghidra/FU3_codec_runtime_capture.md`, with `FUN_11bd_5db2` and the
`5992` else-branch (`5bdb`/`5c8b`) named as the highest-value trace targets.
