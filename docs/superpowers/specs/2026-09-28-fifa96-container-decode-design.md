# FIFA96 Container Envelope + Codec Funnel — Design (Slice 2)

Date: 2026-09-28
Status: Approved in chat (option A), proceeding per standing fully-autonomous directive
Predecessor: `2026-09-27-fifa96-file-loader-design.md` (slice 1, shipped, published)

## 1. Context & verified facts

Slice 1 shipped a golden-tested C library (`src/fifa96_loader/`) for FIFA96 (DOS 16-bit EXE, 291 FUNs, Ghidra program `/fifa96.exe`). Slice 2 targets the asset **container envelope** and locates the **codec** inside the EXE.

Corrections from earlier passes (recorded so they don't resurface):
- A hex dump with a broken offset index produced a false "nibble control stream" reading of `fw1.qfs`. Rettracted; re-dumped with `xxd`.
- Asset tags are **raw ASCII inside the data files**, not in the EXE code (15 byte-pattern probes, 2 evidence passes: `.superpowers/sdd/2026-09-27-fifa96-file-loader/fu1-ghidra-pass2-report.md`). `search_strings` for PCNX/SHPI/GIMX/BIGF/kVGT on the EXE → 0 hits (re-confirmed 2026-09-28).

**Verified envelope (byte-identical across 3 formats, from `xxd` of goldens):**

| off | bytes (fw1.qfs) | bytes (pcindex.pog) | bytes (gameart0.pvi) | field |
|-----|-----------------|---------------------|----------------------|-------|
| 0-1 | `10 fb` | `10 fb` | `10 fb` | magic (u16 LE = 0xFB10) |
| 2-3 | `00 d4` → 0xD400 | `00 b6` → 0xB600 | `04 17` → 0x1704 | word A (u16 LE) |
| 4-5 | `40 e4` → 0xE440 | `40 e1` → 0xE140 | `a2 e3` → 0xE3A2 | word B (u16 LE) |
| 6-9 | `SHPI` | `PCNX` | `BIGF` | 4cc tag (raw ASCII) |
| 10+ | `40 d4 00 00 12 00 00 00 GIMX ...` | `0d 00 00 00 05 03 d8 01 ...` | `00 04 17 a2 00 00 ...` | format-specific tail |

- `fw1.qfs` contains a **second tag** `GIMX` at 0x12 — the file is a sequence of tagged blocks, not just one header+payload.
- `lengths[i]` in `soccer/lengths.dat` equals the **on-disk (compressed)** size for the first aligned entries (FW1.QFS → 9951 B); the CHN_*_BNK region is offset-shifted (documented in `docs/ghidra/FU1_FU2_closeout.md`).
- Existing slice-1 parsers: `fifa96_qfs_parse_hdr` already decodes `{magic, dec_len = LE u32 @2, tag @6}`; `fifa96_pog_parse_hdr` decodes `{magic, w1=u16@2, w2=u32@4}` (note the overlap — see §4).

**Codec funnel (from decompilation, pending confirmation pass in Task 2):**
- `FUN_11bd_5dd2` — file-read orchestration: sole caller of `file_read_far_dos`; calls `FUN_11bd_5d79` (byte-counted read loop, expects 8), then `FUN_11bd_6907` (10-byte context block at `[0x80]` with constant `0x6956/0x11bd/0x0d`), then **`FUN_1000_0b12`**, then `FUN_11bd_6102` (N-byte buffer copy).
- `FUN_1000_0b12` — contains the bit-manipulation core: `SHR AX, CL` @`1000:0b33`, `SHR CX, 1` @`1000:0b89`, INT 21h write, 8-byte chunk copy, and updates to many global tables (`0x28b9, 0x9f1, 0x9b6, 0x20`, repeated `+0xf8c/+0x1028` strided table, DTA-ish update). Strongest candidate for the decompression/context writer.
- `FUN_11bd_26d0` — INT 21h thunk (AH from arg struct; CF → `FUN_11bd_27de` error path).
- `FUN_11bd_22ad` — string/table formatter + error-dispatch (refuted as decoder in pass 2, cited as formatter).

## 2. Scope

In-scope (this slice):
1. **Envelope parser** (`fifa96_envelope`): unified `{magic, word_a, word_b, tag[4], tail_off, tail_len}` for the 3 verified formats + the fw1.qfs second-tag observation (`GIMX` @ 0x12, asserted). A block-chain walker is included ONLY if Task 2's funnel pass proves a deterministic next-tag location; otherwise the chain-walk finding is documented in FU-3. Goldens: `fw1.qfs`, `pcindex.pog`, `gameart0.pvi`.
2. **Codec funnel evidence pass** (Ghidra): confirm/correct the funnel (§1 last bullet) at instruction level; rename the confirmed functions with evidence-based names + plate comments; extend `docs/ghidra/loader_rename_map.md`; update the 5 decoder headers' citations (partial FU-1 resolution: the file-access + decode-funnel citations become concrete).
3. **Codec implementation OR documented finding**: if the decompilation yields a fully-deterministic algorithm (exact operand order, table sources, termination), implement `fifa96_codec_expand()` in C with whatever test vectors are provable. Otherwise: a precise `docs/ghidra/FU3_codec_runtime_capture.md` (algorithm pseudocode from decompilation + the exact DOSBox-X/INT21h trace steps needed to verify it), and `FU1_FU2_closeout.md` updated with the new FU-3.

Out-of-scope: pixel rendering, audio, match simulation, FU-2 CRC (still blocked on codec output), boot/main-loop (candidate slice 3).

## 3. Approach (single approach approved)

Option A: verifiable-increment + evidence funnel. No brute-forced codecs, no assumed algorithms: every C claim is golden-tested or instruction-cited; every unproven step is documented as FU-3 with the concrete experiment that would close it. This preserves slice 1's discipline (byte-identical goldens, honest citations, no fabricated behavior) and turns FU-1/FU-2 from "blocked" into "funnel identified, verification queued".

## 4. Architecture / components

```
src/fifa96_loader/
  fifa96_envelope.{h,c}   # new: magic/word-a/word-b/tag + block-chain walk (pure, bytes-in)
  (existing: fifa96_file, fifa96_tables, fifa96_pog, fifa96_qfs, fifa96_viv, fifa96_tgv — unchanged)
  fifa96_codec.{h,c}      # ONLY if Task 3 yields a provable algorithm
tests/
  test_envelope.c         # 3-format header goldens + fw1.qfs block chain (SHPI→GIMX→…) + negative paths
  test_codec.c            # ONLY with Task 3 success
docs/ghidra/
  loader_rename_map.md    # extended with codec funnel rows
  FU1_FU2_closeout.md     # updated: FU-1 partial (funnel cited), FU-3 added
  FU3_codec_runtime_capture.md  # new: algorithm pseudocode + verification experiment
```

Known inconsistency to resolve in Task 1: `fifa96_pog` w2 (u32 @4) overlaps the tag @6, while `fifa96_qfs` dec_len (u32 @2) straddles word A/B. The envelope struct supersedes both for header purposes; old parsers keep their behavior (they pass their goldens) but gain a pointer comment to the envelope layout. No breaking change to slice-1 API.

## 5. Data flow

`fifa96_file_read` → `fifa96_envelope_parse_hdr` → (if `WALK_BLOCKS` is adoptable) iterate: expect tag@6, read 4cc, locate next tag. **Block-walk strategy is itself Task 2 output** — Task 1 ships header-only for the 3 formats + the fw1.qfs 0x12 `GIMX` second-tag observation (asserted, not walked); Task 2's funnel pass determines whether a deterministic next-tag location exists (e.g. fixed inter-tag stride or an in-tail offset field) and only then does the chain reader get implemented.
→ (Task 3, if codec proven) `fifa96_codec_expand(payload)`.

Error codes: reuse `fifa96_err_t`; add `FIFA96_ERR_BAD_BLOCK` only if the block walk needs it (envelope tail truncation vs magic mismatch).

## 6. Risk & honesty rules (binding)

- No byte offset, byte value, or algorithm step enters code without a golden/`xxd`/instruction citation.
- Broken dumps are a known hazard: all byte assertions in tests must be derivable by `xxd -s OFF -l N` on the committed golden (cite the command in the test comment).
- The codec is **either** implemented with a full citation chain, **or** documented as FU-3 with the exact verification experiment. A half-implementation asserting "this decodes it" without a reference output is a defect.

## 7. Non-goals

No rendering, no gameplay, no audio, no FU-2 implementation, no boot/main-loop, no DOSBox modification, no writes to `/media/felipe/FIFAPCCD`.

## 8. Next step

writing-plans → 3 tasks (envelope+tests; codec funnel evidence pass + citations; codec impl-or-FU3) → SDD execution with per-task reviews → final review → push → stop and report.
