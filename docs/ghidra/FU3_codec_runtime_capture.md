# FU-3 — codec runtime capture

Source: Task 2 funnel pass (program `/fifa96.exe`, project `fifa96`), evidence rows in
`docs/ghidra/loader_rename_map.md#codec-funnel` and the `### Block-walk` verdict
(DEFER). No new evidence was gathered for this doc; every address below is copied
from the committed map (`docs/ghidra/loader_rename_map.md#codec-funnel` + `### Block-walk`).

## Algorithm pseudocode (from decompilation)

No byte-decode transform was observed in any funnel member, so there is no
decode kernel to transcribe. What follows is the orchestration skeleton each
member was CONFIRMED to perform (per-FUN, with instruction addresses):

- `FUN_11bd_5992` (caller, unrenamed): reads one word via `file_read_dos`;
  `if (word == 0x4d)` (`'M'`; decompile `*(short *)(puVar3 + -0x10c) == 0x4d`)
  calls `load_mf_object` once, else falls to `5bdb`/`5c8b`. An earlier
  `'M'/'F'` magic check sits in the same caller. Single-shot: at most one
  `load_mf_object` call per invocation, no loop back.
- `load_mf_object` (ex-`FUN_11bd_5dd2`, body `11bd:5dd2..11bd:5faa`, 194 insns):
  writes `0x4d` marker (`MOV byte ptr [SI],0x4d` at `11bd:5dde`); calls
  `file_read_dos`, `file_read_far_dos`, `alloc_retry_loop` (`5d79`),
  `FUN_11bd_5db2` (3×, role unknown), `copy_bytes_far` (`6102`, via
  `CALL 0x1000:7cd2` at `11bd:5f89`), `exec_loaded_image` (`6907`, via
  `CALL 0x1000:84d7` at `11bd:5f48`), `int21_dispatch` (`26d0`),
  `mem_grow_relocate` (`1000:0b12`, via `CALLF 0x1000:0b12` at `11bd:5f6b`);
  returns (`RET` at `11bd:5faa`), no loop back to a header read.
- `mem_grow_relocate` (ex-`FUN_1000_0b12`, body `1000:0b12..1000:0c0c`,
  107 insns): `SHR AX,CL` at `1000:0b33` (bytes `d3e8`); `SHR CX,0x1` at
  `1000:0b89`; `MOV AH,0x4a; INT 0x21` at `1000:0b50..0b56` (DOS realloc);
  `MOVSW.REP` at `1000:0b8b`; segment-table stores
  `MOV ES:[0x28b9],AX` / `MOV [0x9f1],AX` / `MOV [0x9b6],AX` / `MOV [0x20],AX`
  at `1000:0b96..0ba0`; 0x27-entry retarget loop at `1000:0bce..0be7`.
  Shared infrastructure: 7 callers (`11bd:3844`, `11bd:3986`, `11bd:3ed8`,
  `11bd:5c8b`, `11bd:5dd2`, `11bd:7290`, `11bd:76db`).
- `exec_loaded_image` (ex-`FUN_11bd_6907`, body `11bd:6907..11bd:6955`,
  37 insns): `MOV SS,DX` at `11bd:694c`, `MOV SP,word ptr [BX+0x10]` at
  `11bd:694e`, `PUSH AX; PUSH CX; RETF` at `11bd:6953..6955` — SS/SP switch +
  far transfer to the loaded image. No byte transform.
- `alloc_retry_loop` (ex-`FUN_11bd_5d79`, body `11bd:5d79..11bd:5db1`,
  25 insns): `MOV byte ptr [BP-0xd],0x48` (AH=48h DOS-alloc probe) at
  `11bd:5d85`; `CALL 0x1000:42a0` (`int21_dispatch` thunk) at `11bd:5d8e`;
  `CMP word ptr [BP-0xe],0x8` retry loop at `11bd:5d9e`; returns segment or 0
  (`SUB AX,AX` at `11bd:5dac`).
- `copy_bytes_far` (ex-`FUN_11bd_6102`): 17-insn body is pure far copy,
  `MOVSB.REP ES:DI,SI` at `11bd:6117`. No transform.
- `int21_dispatch` (ex-`FUN_11bd_26d0`, 32 insns): loads AX/BX/CX/DX/SI/DI
  from a register block (`MOV AX,[DI]` … `MOV DI,[DI+0xa]` at
  `11bd:26d8..26e6`), `INT 0x21` at `11bd:26e9`, writes results back, `JC`
  error path at `11bd:2700` (→ `FUN_11bd_27de`), CF stored at block+0xc.

Correction carried over from Task 2: no member references any game-asset tag
(`SHPI`, `PCNX`, `kVGT`, `BNK`) or envelope field, so the funnel is NOT
attributed to any single game-asset decoder.

## Why static proof is insufficient

Codec path requires (a) exact bit/operand order cited to instruction addresses,
(b) every table/constant source cited to an address or golden bytes,
(c) termination cited (count field or sentinel with address). All three fail:

- (a) FAILED: no bit-level decode kernel exists to order. The only shift-form
  instructions observed are word-granularity paragraph arithmetic in
  `mem_grow_relocate` (`SHR AX,CL` at `1000:0b33`, `SHR CX,1` at `1000:0b89`),
  and the only move is a verbatim far copy (`MOVSB.REP` at `11bd:6117` /
  `1000:0b8b`). The hypothesized bit-core (e.g. `SHR AX,CL` as a bit reader)
  is refuted by context: DOS realloc + backward slide + table retarget.
- (b) FAILED: no decode tables or constants were observed. The only
  table-like structures are the segment-retarget stores at
  `1000:0b96..1000:0ba0` (+ loop `1000:0bce..0be7`), which patch relocation
  entries, not decode symbols. `get_xrefs_to` on `0x28b9`/`0x9f1`/`0x9b6`/`0x20`
  returned 0 refs (segment-relative `ES:` writes, not indexed xrefs).
- (c) FAILED: no deterministic next-tag/termination rule was observed
  (Block-walk verdict DEFER). `load_mf_object` runs once and returns (`RET` at
  `11bd:5faa`); caller `FUN_11bd_5992` invokes it at most once per call
  (branch on word `== 0x4d`, no iteration). Missing: outer driver loop
  re-invoking `5992`/`5dd2`, role of `FUN_11bd_5db2` (3 calls inside `5dd2`),
  else-branch (`5bdb`/`5c8b`) parse, any in-tail offset/stride to the next tag.

## Verification experiment (DOSBox-X INT 21h trace)

1. Run the game under tracing: `./run-fifa96.sh` (DOSBox-X session hosting
   `/fifa96.exe` from project `fifa96`).
2. Trace INT 21h file reads: break on `file_read_dos` (`11bd:5fe2`, AH=3F) and
   `file_read_far_dos` (`11bd:6003`, AH=3F far buffer) to log each read's
   handle (`[0xe70]` cell), byte count (CX), and target buffer (DS:DX);
   break on the `FUN_11bd_5992` word-compare branch (`== 0x4d`) and on
   `CALLF 0x1000:0b12` at `11bd:5f6b` to delimit one `load_mf_object`
   (`11bd:5dd2`) invocation. Candidate golden inputs to exercise while
   tracing (attribution unproven — the funnel references no format tag):
   `tests/golden/fw1.qfs`, `tests/golden/pcindex.pog`, `tests/golden/gameart0.pvi`.
3. Capture: for one traced `load_mf_object` invocation, dump the input bytes
   as seen in the far buffer filled by `file_read_far_dos`, and dump the
   output bytes at the destination of `copy_bytes_far` (`MOVSB.REP` at
   `11bd:6117`; params off stack) and/or the relocated image after
   `mem_grow_relocate` returns. Record buffer segment:offsets and lengths
   from the trace.
4. Compare: feed the captured input bytes to `fifa96_codec_expand` (once
   implemented from a future proven verdict) and byte-compare its output
   against the captured output bytes. Until the three gaps above are closed
   by trace evidence (driver loop, `5db2` role, else-branch, next-tag rule),
   no `fifa96_codec_expand` implementation may claim to decode anything.

## Test vectors it would yield

Input capture → expected-output capture, both dumped with `xxd` from the
traced buffers (commands instantiated once the trace in §3 fixes buffer
addresses; golden-file bytes are NOT substitutes for traced buffer bytes):

```
xxd -s <input_offset> -l <input_len> <traced_input_dump.bin>   # input bytes at file_read_far_dos buffer
xxd -s <output_offset> -l <output_len> <traced_output_dump.bin> # output bytes at copy/relocated destination
```

Each future vector must cite: source FUN + instruction address, input buffer
address + `xxd` bytes, output buffer address + `xxd` bytes. Vectors copied
verbatim into `tests/test_codec.c` only after the (a)(b)(c) reassessment
passes ADOPT.

## Refined trace targets (2026-09-28 delimitation pass)

| Breakpoint | Delimits | Source |
|------------|----------|--------|
| `11bd:5aa1` (`CMP word ptr [BP+0xfef6],0x4d` in dispatch_object_load) | MF vs script branch per object | map § Load-path delimitation |
| `11bd:5aa8` (`CALL load_mf_object`) | MF-branch invocation | map § Load-path delimitation |
| `11bd:5ab1` + `11bd:5ab9` (`CALL 5bdb`, `CALL 5c8b`) | else-branch (script parse + word table) | map § Load-path delimitation |
| `11bd:5dd2` entry / `RET` at `11bd:5faa` (`load_mf_object`) | one load invocation, entry to return | map § Codec funnel (`5dd2` row: body + `CALLF` at `5f6b`) + FU-3 § Algorithm pseudocode |
| `FUN_11bd_2d9c` dispatch sites (`CALL 0x1000:7562` at `11bd:2e80` unconditional; `CALL 0x1000:7562` at `11bd:2e98` conditional on `FUN_11bd_6028` result, `OR AX,AX` at `11bd:2e91` + `JNZ` at `11bd:2e93`) | the at-most-two dispatch_object_load calls per run | map § Driver loop (EXHAUSTED) |
| `11bd:5f6b` (`CALLF 0x1000:0b12`) | mem_grow_relocate within a load | map § Codec funnel (`5dd2` row cites this call) |
| `11bd:5f5a` / `11bd:5f61` / `11bd:5fa1` (`mem_free_dos` calls on load_mf_object cleanup/fail paths) | `5db2` role sites (closed in delimitation pass) | map § Load-path delimitation (`5db2` row) |
| `11bd:5fe2` / `11bd:6003` (`file_read_dos` / `file_read_far_dos`, AH=3F) | every traced read's handle/count/buffer | FU-3 § Verification experiment |
