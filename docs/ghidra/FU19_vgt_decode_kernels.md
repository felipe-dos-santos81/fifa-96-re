# FU-19: VGT decode kernels (protected-mode image)

Static transcription of the protected-mode VGT decode kernels in
`fifa96_le.bin` (the FU-4 linear-executable image used for FU-5). All
addresses are **link-time flat addresses**; the running game adds the
`0x1FC010` relocation delta (FU-4), and the measured probe delta in FU-11 is
`0x1FC000`. No runtime work was done in this slice; every claim below is
sourced to Ghidra decompilation/disassembly run against
`/fifa96_le.bin` (provenance at the end).

FU-5 (`FU5_vgt_decoders.md`) owns the entry chain
(`vgt_stream_poll 0x67BA8`, `vgt_dispatch 0xAE4BC`, `vgt_decode_f 0xADEFC`,
`vgt_decode_k 0xAE218`) and the tag routing. This document goes one level
deeper: the record decoder, every dispatched handler, and the fVGT pipeline
helpers. Where this slice contradicts FU-5, the correction is called out in
line and repeated in "Open legs / corrections".

## Scope

* `0x9E718` record dispatcher (FU-5: "kVGT command stream") — the loop
  control flow, header parse, dispatch and return.
* `0x9E860` wrapper and the `0xAE474` direct call from `vgt_decode_k`
  (FU-11's unmatched caller, return address `0xAE479`).
* Every dispatch target: `0xB18F8` (0x10/0x11), `0x9E1E4` (0x16),
  `0x9C7E0` (0x32..0x34), `0x9DA14` (0x46), `0x9DC1C` (0x60..0x72),
  `0x9DC94` (0x7A/0x7B), and the literal copy `0xCD390` (0x6A).
* fVGT pipeline: `0xADD60`, `0xADDF0`, `0xBA8F0`, `0xBA994`, plus the two
  FU-5 steps that are *inline* in `vgt_decode_f` rather than separate
  functions (index-table expansion, 16-byte block copy).

The Ghidra program was renamed in this pass (addresses first; rename table
below) to carry the FU-19 findings into later slices.

| address | FU-19 name | previous |
| --- | --- | --- |
| `0x9E718` | `decode_record_dispatch` | `FUN_0009e718` |
| `0x9E860` | `decode_record_strict` | `FUN_0009e860` |
| `0x9E3EC` | `decode_stream_resolve` | `FUN_0009e3ec` |
| `0x9E420` | `decode_method_classify` | `FUN_0009e420` |
| `0x9E890` | `decode_size_probe` | `FUN_0009e890` |
| `0xB18F8` | `lz_refpack_decode` | `FUN_000b18f8` |
| `0x9E1E4` | `lz_16fb_decode` | `FUN_0009e1e4` |
| `0x9C7E0` | `huff_32fb_decode` | `FUN_0009c7e0` |
| `0x9DA14` | `tree_47fb_decode` | `FUN_0009da14` |
| `0x9DC1C` | `delta_prefix_decode` | `FUN_0009dc1c` |
| `0x9DC94` | `rle_row_decode` | `FUN_0009dc94` |
| `0xCD390` | `memmove_bytes` | `FUN_000cd390` |
| `0x9D9D0` | `tree_emit_symbol` | `FUN_0009d9d0` |
| `0xADD60` | `unpack_signed_fields` | `FUN_000add60` |
| `0xADDF0` | `expand_palette_block` | `FUN_000addf0` |
| `0xBA8F0` | `unpack_block_indices` | `FUN_000ba8f0` |
| `0xBA994` | `composite_4x4_blocks` | `FUN_000ba994` |

## 1. The record dispatcher is not a loop

`decode_record_dispatch` (`0x9E718..0x9E85E`, 112 instructions, `RET` at
`0x9E85E`) is a **single-record decompressor dispatcher**, not a command
stream interpreter. The 112-instruction body has **no backward branch**:
every conditional exits forward to `0x9E859` (`MOV EAX,ESI`, the return
epilogue), verified by walking the disassembly of the whole body
(`ghidra_disassemble_function 0x9E718`). One call consumes exactly one
self-describing compressed record; no caller-visible pointer is advanced
and the callers (below) invoke it once per buffer.

Call shape (cdecl; args read at `0x9E71B`/`0x9E71F`):

```
decode_record_dispatch(stream /*[esp+0x10]*/, dst /*[esp+0x14]*/, strict /*[esp+0x18]*/)
```

Header/validation, in instruction order:

* `0x9E723 CALL 0x9E3EC` — `decode_stream_resolve(stream)` (section 2).
* `0x9E731 ADD EAX,0x2` + `0x9E734 MOV EAX,[EAX]` + `0x9E736 BSWAP EAX` +
  `0x9E741 SHR EAX,CL` (CL=8) — a **24-bit big-endian length** from record
  bytes `[2..4]`, saved in `EBP` (`0x9E743 MOV EBP,EAX`). This parse runs
  **before** the marker test (it reads bytes `[2..4]` of any record).
* `0x9E747 MOV AL,byte ptr [EBX+0x1]` + `0x9E74C CMP EAX,0xFB` +
  `0x9E751 JNZ 0x9E859` — `stream[1]` must be `0xFB`; otherwise `ESI` was
  zeroed at `0x9E74A` (`XOR ESI,ESI`) and the function returns 0.
* `0x9E757 MOV AL,byte ptr [EBX]` + `0x9E759 AND AL,0xFE` — the selector is
  `stream[0] & 0xFE`; the dispatch tree is `0x9E75B..0x9E7E2`.
* Handlers are invoked with `dst` (`EDX`) passed through unchanged; the
  return value is always `ESI` (`0x9E859`).

So the `stream[1] == 0xFB` byte is the **signature byte of a compression
method family** and `stream[0] & 0xFE` is the method id. Corroboration:
`decode_method_classify` (`0x9E420`, section 3) maps exactly these bytes to
small method-class numbers, and `decode_size_probe` (`0x9E890`) reads the
same 24-bit length only when the class is in `[1..0x1E]`. FU-5's
"command/run-stream codec" phrasing is superseded: there is no command
loop, no run list, and no per-call pointer advance at this level.

### Error path and `strict`

Unknown selector: `0x9E840 CMP dword ptr [ESP+0x18],0x0` + `0x9E845 JZ
0x9E859` (return 0 silently). If `strict != 0`, `0x9E847 XOR EAX,EAX` +
`0x9E849 MOV AL,byte ptr [EDI]` + `0x9E84B PUSH EAX` + `0x9E84C PUSH
0x33DC` + `0x9E851 CALL 0xCBBE8` (`decode_error(cmd, &DAT_000033DC)`) then
return 0. `decode_record_strict` (`0x9E860..0x9E874`) is the 8-instruction
wrapper that passes `strict = 1` (`0x9E860 PUSH 0x1`, `0x9E86C CALL
0x9E718`).

`DAT_000033DC` (the error format string) probes as zero bytes in the LE
image (`read_memory 0x33D0`, 32 bytes, all `00`) — it is BSS, populated at
runtime; the message text is therefore an open leg.

### Dispatch argument conventions

| selector | target | call site | argument registers |
| --- | --- | --- | --- |
| 0x10/0x11 | `lz_refpack_decode` | `0x9E7E8` | cdecl `(EBX=stream, EDX=dst, ECX=1)`; `0x9E7E4 PUSH 0x1`, `0x9E7E6 PUSH EDX`, `0x9E7E7 PUSH EBX` |
| 0x16 | `lz_16fb_decode` | `0x9E7F6` | EAX=stream (`0x9E7F4 MOV EAX,EBX`), EDX=dst |
| 0x32/0x34 | `huff_32fb_decode` | `0x9E806` | EAX=stream (`0x9E804 MOV EAX,EDI`), EBX=1 (`0x9E7FF`), EDX=dst |
| 0x46 | `tree_47fb_decode` | `0x9E811` | EAX=stream (`0x9E80F MOV EAX,EBX`), EDX=dst |
| 0x60,0x62,0x66,0x6E,0x70,0x72 | `delta_prefix_decode` | `0x9E81C` | EAX=stream (`0x9E81A MOV EAX,EDI`), EDX=dst |
| 0x6A | `memmove_bytes` | `0x9E82D` | cdecl `(EDI=stream+5, EDX=dst, EBP=len)`; `0x9E827 ADD EDI,0x5`, `0x9E82B MOV ESI,EBP` |
| 0x7A/0x7B | `rle_row_decode` | `0x9E839` | EAX=stream (`0x9E837 MOV EAX,EBX`), EDX=dst |
| other | `decode_error` | `0x9E851` | cdecl `(cmd byte, &DAT_000033DC)` if strict |

## 2. Stream resolution: `decode_stream_resolve` (`0x9E3EC..0x9E41C`)

`0x9E3F1 MOV ESI,EAX` keeps the input pointer; `0x9E3F3 MOV EBX,0x9` and
`0x9E3F8 MOV EDX,0x33D0` set up `FUN_000AF37B` (`0x9E3FD CALL`), a
bounded `memcmp`-style compare of up to 9 bytes (`FUN_000AF37B` returns 0
when the first bytes match or the source byte is NUL). If it returns 0,
`0x9E40D SCASB.REPNE ES:EDI` (strlen) runs and `0x9E413 ADD ESI,ECX`
advances `ESI` past the NUL; otherwise `0x9E404 JNZ 0x9E415` returns the
pointer unchanged. Because `0x33D0` is zero in the image, the advance can
only trigger if the runtime initializes a 9-byte prefix there; for kVGT
payloads (which start with a `..FB` compression header, never a NUL) this
function is the identity. Named-open-leg.

`decode_method_classify` (`0x9E420..0x9E715`) is the companion probe: it
reads `stream[1] != 0xFB` first (`decompile`: `if (pbVar2[1] != 0xfb)`),
maps the `stream[0] & 0xFE` selector to a method-class id (0x10/0x11→0xC,
0x16→0xF, 0x32→9, 0x34→10, 0x46→5, 0x66→0x1B, 0x6A→0x1F, 0x6E→1, 0x70→0x11,
0x72→0xB, 0x7A/0x7B→2/3/4 selected by bytes at +5/+6, unknown→0x12), and
returns 0x21 for a header whose BE32 at [0..3] is `0x3F3`. `decode_size_probe`
(`0x9E890..0x9E8C6`) chains both: `0x9E894 CALL 0x9E3EC`, `0x9E89B CALL
0x9E420`, then returns the 24-bit BE length (`0x9E8B5 BSWAP EAX`; `0x9E8C0
SHR EAX,CL`) only when `0x9E8A4 JLE` / `0x9E8A9 JG` keep the class in
`(0,0x1E]`; otherwise 0. Caller `FUN_000CAD30 0xCAE32` uses that as
"compressed vs raw": `if (FUN_0009e890(uVar4) == 0) memmove_bytes(...)` else
`decode_record_dispatch(...)` (decompile of `0xCAD30`).

## 3. Per-handler transcription

Termination column = the condition that ends the decode loop. "Out" is the
`dst` pointer passed through the dispatcher.

| selector (`stream[0]&0xFE`) | raw first bytes seen | handler | record layout | out | termination |
| --- | --- | --- | --- | --- | --- |
| 0x10/0x11 | 0x10FB, 0x11FB | `lz_refpack_decode 0xB18F8` | 2 hdr + [3 extra if bit0] + 3-byte BE len + LZ body | `param_2` | end marker control `DL >= 0xFC` |
| 0x16 | 0x16FB, 0x17FB | `lz_16fb_decode 0x9E1E4` | 2 magic + (0x16FB? 3-byte BE len : 4-byte LE len) + LZSS body | state output cursor (param_2) | output count reaches header length |
| 0x32/0x34 | 0x32FB..0x35FB | `huff_32fb_decode 0x9C7E0` | 2 magic + [4 extra if bit0] + bit-packed header, table, body | `param_2` | remaining-length counter exhausted; then prefix-sum transform for 0x32FB/0x34FB |
| 0x46 | 0x46FB, 0x47FB | `tree_47fb_decode 0x9DA14` | 2 magic + [3 extra if 0x47FB] + 3-byte BE len + table + flag/bit stream | `param_2` | zero literal under a positive flag |
| 0x60,0x62,0x66,0x6E,0x70,0x72 | 0x60FB..0x73FB | `delta_prefix_decode 0x9DC1C` | 2 magic + (0x62FB? +3 : 0x66FB? +4 : 0) + 3-byte BE len + delta bytes | `param_2` | output count reaches length |
| 0x6A | 0x6AFB, 0x6BFB | `memmove_bytes 0xCD390` | 2 cmd + 3-byte BE len + raw bytes | `param_2` | byte count reaches length |
| 0x7A | 0x7AFB, 0x7BFB | `rle_row_decode 0x9DC94` | 2 magic + [3 extra if 0x7BFB] + 3-byte BE len + stride u8 + width u8 + descriptors | `param_2` | zero signed descriptor |
| other | — | `decode_error` | — | — | immediate return 0 |

Note the literal-copy length is the same bytes `[2..4]` parsed up front at
`0x9E734..0x9E743`; the handler pushes it as `EBP` (`0x9E825 PUSH EBP`) and
returns it in `ESI` (`0x9E82B MOV ESI,EBP`).

### 3.1 `lz_refpack_decode` (`0xB18F8..0xB1A67`, 136 insns)

EA-style LZ record with an explicit end marker.

* Header: `0xB1918 MOV AX,word ptr [EBX]` (first byte inspected), `0xB191B
  LEA EBX,[EBX+0x2]`, `0xB191E AND AL,0x1` + `0xB1920 JZ` + `0xB1922 LEA
  EBX,[EBX+0x3]` — bit0 of `stream[0]` skips 3 extra header bytes.
  `0xB1925..0xB1932` assembles a **3-byte big-endian length** into `EAX`,
  stored at `0xB1935 MOV dword ptr [EBP-0x4],EAX`; that value is the return
  (`0xB19F2 MOV EAX,dword ptr [EBP-0x4]`).
* Control loop: `0xB198D OR CL,byte ptr [EBX]` + `0xB198F MOV EDX,dword ptr
  [EBX]` + `0xB1991 JNS 0xB196D`; `0xB196D AND ECX,0x3` selects the literal
  count. Literal+match path: `0xB1948 LEA ESI,[EBX+0x2]`, `0xB194B
  MOVSB.REP ES:EDI,ESI` copies `flag&3` literals, then match distance from
  `(EDX>>5)` negated (`0xB1956 SHR CH,0x5`, `0xB195C NEG ECX`, `0xB195E LEA
  ESI,[ECX+EDI-0x1]`) and match length `(EDX&0x1C)>>2 + 3` (`0xB1953 AND
  EDX,0x1C`, `0xB1959 SHR EDX,0x2`, `0xB1962 LEA ECX,[EDX+0x3]`, `0xB1965
  MOVSB.REP`).
* Four control classes exist in the body (`0xB1993 ADD CL,CL` / `0xB19C8
  ADD CL,CL` split on sign, plus the `0xB1972` and `0xB1948` variants); the
  exhaustive published opcode table was not re-derived here (open leg).
* End marker: `0xB19CC CMP DL,0xFC` + `0xB19CF JNC 0xB19E8` — when the
  control class is `>= 0xC0` and the low byte is `0xFC..0xFF`, `0xB19EA LEA
  ESI,[EBX+0x1]`, `0xB19ED AND ECX,0x3`, `0xB19F0 MOVSB.REP` copies the last
  0..3 literals and falls into the return at `0xB19F2`. This is the only
  exit from the decode loop.
* `decode_record_dispatch` passes `ECX=1` (`0x9E7E4 PUSH 0x1`), so the
  `0xB1938 CMP ECX,0x0` early-out is never taken from the VGT path.

### 3.2 `lz_16fb_decode` (`0x9E1E4..0x9E3E9`, 186 insns)

LZSS with a 0x1000-byte ring and two symbol readers.

* Allocates a zeroed `0xFC4` state via `FUN_00098BF8(&DAT_000033CC,0xF6C1,0)`
  (`0x9E1FC CALL`) and fills two cursor pairs (`0x9E21F..0x9E235`):
  state words hold `{start,input}` and `{start,dst}`.
* Magic: `0x9E241 MOV EAX,dword ptr [EAX]`, `0x9E243 BSWAP EAX`, `0x9E24E
  SHR EAX,CL` (CL=16 → BE16 at `[0..1]`), `0x9E250 CMP EAX,0x16FB` +
  `0x9E255 JNZ 0x9E2A4`. `0x16FB` records take a **24-bit BE length**
  (`0x9E26C..0x9E2A2`, bytes `b0<<16|b1<<8|b2`); all others (raw first byte
  0x17) take a **32-bit LE length** (`0x9E2A4..0x9E2FF`, `b0 | b1<<8 |
  b2<<16 | b3<<24`).
* `0x9E31F MOV EBP,dword ptr [EBX+0x8]` = decoded length. The ring lives at
  state+0x18 (`0x9E315 LEA EDI,[EBX+0x18]`, zero-filled `0xFC4` bytes with
  `0x9E31D STOSB.REP`); positions are masked `& 0xFFF` (`0x9E352 AND
  EDX,0xFFF`).
* Per iteration: `0x9E32E CALL 0x9E160`; if the returned symbol is
  `>= 0x100` (`0x9E335 CMP EAX,0x100`, `0x9E33A JGE 0x9E35A`) it is a
  match: `0x9E35C CALL 0x9E1A0`, length `0x9E361 SUB ESI,0xFD`, distance
  `0x9E36D SUB EDI,EAX` + `0x9E373 DEC EDI` + `0x9E376 AND EDI,0xFFF`,
  copied byte-wise `0x9E384..0x9E3C8` from ring to output (and ring).
  Otherwise the symbol is written literally (`0x9E33C..0x9E345`), ring
  mirror `0x9E34E MOV byte ptr [ESI+EBX*0x1+0x18],AL`.
* Termination: `0x9E3CA CMP ECX,dword ptr [EBX+0x8]` + `0x9E3CD JC
  0x9E32C` — stops when the produced count reaches the header length. The
  state is freed (`0x9E3D7 CALL 0x993EC`) and the count returned in `EAX`
  (`0x9E3DF MOV EAX,ESI`).
* `FUN_0009E160` (`0x9E160..0x9E19F`) and `FUN_0009E1A0`
  (`0x9E1A0..0x9E1E3`) are the symbol readers; both index static tables
  (`&DAT_0000F6B3`/`&DAT_0000ECEB` and state offsets `+0x12864`/`+0x12964`).
  Their exact table layout/bit contract is **not** transcribed (open leg).

### 3.3 `huff_32fb_decode` (`0x9C7E0..0x9D9CF`, 1198 insns, leaf)

Bit-packed (Huffman-style) codec with a post-pass arithmetic transform.
Despite its size it makes **no calls** — the decoder and the two transforms
are all inline.

* Selector/magic: `0x9C809 MOV DL,byte ptr [EAX]` + `0x9C812 SHL EDX,0x8` +
  `0x9C815 MOV CL,byte ptr [EAX-0x1]` + `0x9C81F OR ECX,EDX` gives
  `stream[0]<<8|stream[1]`; it is reduced to the BE16 word at
  `0x9C833 SHL ESI,0x10` + `0x9C838 SHR ESI,0x10` and kept at `[ESP+0x4D4]`
  (`0x9C85C MOV dword ptr [ESP+0x4D4],ESI`); compared at the end.
* Header bit0: `0x9C87B MOV AH,byte ptr [ESP+0x4D5]`, `0x9C885 TEST
  AH,0x1` + `0x9C888 JZ 0x9C903` — bit0 of `stream[0]` selects an 8-byte
  header (extra reads at `0x9C88A..0x9C903`). The 4/8-byte header is
  consumed into a bit buffer (`[ESP+0x520]`, cursor `[ESP+0x524]`).
* Decoded length: `0x9C987 MOV dword ptr [ESP+0x4D0],EAX` (16-bit part,
  `SHR EAX,0x10` at `0x9C981`), then `0x9C9EB MOV ECX,[ESP+0x4D0]` +
  `0x9C9F5 OR ECX,EBP` + `0x9C9F7 MOV [ESP+0x4D0],ECX` (second 16-bit part
  shifted in at `0x9C9F2 SHL EBP,0x10`).
* The function builds a code-length table, a symbol table and an ordering
  table in stack locals (decompiler `local_53c`, `local_43c`, `local_23c`,
  `auStack_13c`); the exact canonical-code construction was not transcribed
  (open leg). `0x9C9FE TEST EBX,EBX` + `0x9CA00 JZ 0x9D36D` skips it when
  `EBX==0`; the VGT dispatcher passes `EBX=1` (`0x9E7FF MOV EBX,0x1`), so
  the table branch runs.
* Main decode loop head `0x9D36D MOV EDX,ESI`; `0x9D36F SHR EDX,0x18`;
  `0x9D372 MOV DL,byte ptr [ESP+EDX*0x1]` (code length); `0x9D37B SUB
  EDI,EDX` + `0x9D37F JL 0x9D438`; symbol fetch `0x9D393 MOV AL,byte ptr
  [ESP+EAX*0x1+0x100]` + `0x9D39C MOV byte ptr [EBX],AL` (literal/matched
  symbol written to the output cursor `[ESP+0x4C8]`); bit buffer refill
  `0x9D39A SHL ESI,CL`. The output cursor advances through
  `0x9D366 MOV dword ptr [ESP+0x4C8],EDX`.
* End of stream: `0x9D957 MOV EDI,dword ptr [ESP+0x4C4]` (output base),
  `0x9D95E MOV EBX,dword ptr [ESP+0x4D0]` (decoded length), `0x9D965 MOV
  ECX,dword ptr [ESP+0x4D4]` (magic), `0x9D96C ADD EDI,EBX`.
* Transform: `0x9D96E CMP ECX,0x32FB` + `0x9D974 JNZ 0x9D995` — for
  `0x32FB`, a **prefix-sum** pass over the output: `0x9D985 XOR EDX,EDX` +
  `0x9D987 MOV DL,byte ptr [EAX]` + `0x9D98A ADD EBX,EDX` + `0x9D98C MOV
  byte ptr [EAX-0x1],BL`, looped until `0x9D98F CMP EAX,ECX` + `0x9D991
  JNC`. For `0x9D995 CMP ECX,0x34FB` (non-`0x34FB` returns), a **double
  prefix-sum**: `0x9D9B2 ADD EBX,ECX` (first integral) then `0x9D9B5 ADD
  EDX,EBX` (second) + `0x9D9B7 MOV byte ptr [EAX-0x1],DL`, loop
  `0x9D9BA..0x9D9BC`.
* Return: `0x9D9BE MOV EAX,dword ptr [ESP+0x4D0]` + `RET 0x9D9CF`.

### 3.4 `tree_47fb_decode` (`0x9DA14..0x9DC1A`, 151 insns)

Flat-tree codec (nodes as parallel byte tables) plus literal escapes.

* Three 256-byte globals are installed per call: `0x9DA30 MOV
  [0x5BA9C],EDX` (flags), `0x9DA38 MOV [0x5BA90],EDX` (child A), `0x9DA47
  MOV [0x5BA98],EDX` (child B). `0x9DAB1` clears flags to 0.
* Magic: `0x9DA5A MOV DL,[EAX]` + `0x9DA5E SHL EDX,8` + `0x9DA61 MOV
  AL,[EBX]` + `0x9DA63 ADD EDX,EAX` (BE16 `[0..1]`), `0x9DA66 CMP
  EDX,0x47FB`; if matched `0x9DA6E ADD EBX,0x3`.
* `0x9DA71..0x9DAA2` assembles a **3-byte BE length** into `[ESP+0x304]`.
  `0x9DABF MOV AL,[EBX]` + `0x9DAC1 MOV byte ptr [EDX+EAX*0x1],0x1` seeds
  the root in the flags table. `0x9DAC7 MOV AL,[EBX+1]` (entry count) +
  `0x9DACA ADD EBX,0x2`.
* Table fill `0x9DAE1..0x9DB36`: each 3-byte entry `[key, a, b]` stores
  `childA[key]=a` (`0x9DAFC/0x9DB03`), `childB[key]=b` (`0x9DB16/0x9DB20`),
  `flags[key]=0xFF` (`0x9DB30 MOV byte ptr [ECX+EDX*0x1],0xFF`).
* Main loop `0x9DB38`: `0x9DB40 MOV DL,byte ptr [EBX]`; `0x9DB42 MOV
  CL,byte ptr [ECX+EDX*0x1]` (flag); `0x9DB46 TEST CL,CL` + `0x9DB48 JNZ
  0x9DB50`; flag 0 → literal: `0x9DB4A INC ESI` + `0x9DB4B MOV byte ptr
  [ESI-0x1],DL`.
* `0x9DB50 JGE 0x9DBEB` — positive flag: `0x9DBEB XOR EDX,EDX` + `0x9DBED
  MOV DL,byte ptr [EBX]` + `0x9DBEF INC EBX` + `0x9DBF0 TEST EDX,EDX` +
  `0x9DBF2 JZ 0x9DBFD` is the **terminator** (a zero literal); otherwise
  `0x9DBF5 MOV byte ptr [ESI-0x1],DL` and continue.
* Negative flag → tree walk at `0x9DB56`: `0x9DB67 MOV AL,byte ptr
  [EDX+EAX*0x1]` (child A), then while `flags[child] != 0`
  (`0x9DB74 CMP byte ptr [ECX+EDI*0x1],0x0`) call `tree_emit_symbol`
  (`0x9DB87 CALL 0x9D9D0`, `0x9DBCA CALL 0x9D9D0`), following child B
  (`0x9DBAA MOV AL,byte ptr [EDX+EAX*0x1]`); literals emitted at
  `0x9DB9C`/`0x9DBE3`.
* Return: `0x9DBFD MOV EAX,dword ptr [ESP+0x304]`. `tree_emit_symbol`
  (`0x9D9D0..0x9DA10`) is a recursive leaf that emits a byte
  (`*DAT_0005BA8C = in_AL; DAT_0005BA8C++`) after expanding internal nodes;
  the child-table semantic (which bit selects which child) is **not**
  statically resolved here (open leg).

### 3.5 `delta_prefix_decode` (`0x9DC1C..0x9DC92`, 54 insns)

* `0x9DC2A..0x9DC38`: `BE16 = stream[0]<<8|stream[1]`.
  `0x9DC3B CMP ECX,0x62FB` + `0x9DC43 ADD EDX,0x3` (payload at +5);
  `0x9DC48 CMP ECX,0x66FB` + `0x9DC50 ADD EDX,0x4` (payload at +6); default
  payload at +2.
* `0x9DC53..0x9DC6C` assembles a 3-byte BE length in `EDI`; `0x9DC6E LEA
  ESI,[EAX+EDI*0x1]` sets the output end (`EAX`=dst).
* Loop `0x9DC77`: `0x9DC79 MOV CL,byte ptr [EDX]` + `0x9DC7B ADD EBX,ECX` +
  `0x9DC7E AND EBX,0xFF` + `0x9DC85 MOV byte ptr [EAX-0x1],BL` — the output
  is the **byte-wise cumulative (prefix) sum** of the input. Termination:
  `0x9DC88 CMP EAX,ESI` + `0x9DC8A JC 0x9DC77`. Return length `0x9DC8C MOV
  EAX,EDI`.

### 3.6 `rle_row_decode` (`0x9DC94..0x9DD4E`, 85 insns)

* `0x9DCB7 CMP EBX,0x7BFB` + `0x9DCBF ADD EAX,0x3` (payload at +5 for
  `0x7BFB`, else +2). `0x9DCC4..0x9DCE4` assembles a 3-byte BE length in
  `ESI` (`0x9DCF0 MOV [ESP],ESI`).
* Header: `0x9DCE8 MOVZX EBP,byte ptr [EAX-0x1]` = stride (`uVar7`);
  `0x9DCEC MOV BL,byte ptr [EAX]` + `0x9DCEF DEC EBX` = descriptor byte
  width minus 1.
* Descriptor loop `0x9DCF7`: `0x9DCFC MOVSX EDI,byte ptr [EAX-0x1]`, then
  `width` big-endian bytes are accumulated (`0x9DD04 SHL EDI,0x8`, `0x9DD0B
  ADD EDI,ESI`) into a signed count.
* Negative count: `0x9DD14 NEG EBX` + `0x9DD18 IMUL EBX,EBP` → copy
  `(-count)*stride` raw bytes (`0x9DD1B..0x9DD25`). Zero: `0x9DD27 JZ
  0x9DD3F` **terminates** the record. Positive count: repeat count
  `0x9DD29 LEA ESI,[EDI+0x1]`, inner loop copies `stride` bytes
  (`0x9DD2E..0x9DD36`) then rewinds the source `0x9DD38 SUB EAX,EBP` and
  repeats, restoring `0x9DD3D ADD EAX,EBP`.
* Loop condition `0x9DD3F TEST EDI,EDI` + `0x9DD41 JNZ 0x9DCF7`; return
  length `0x9DD43 MOV EAX,dword ptr [ESP]`. So runs are whole rows
  (`stride` repeated copies) or literal row spans — a row-RLE.

### 3.7 `memmove_bytes` (`0xCD390..0xCD3F3`, 50 insns)

No transform. cdecl `(src, dst, len)`: `0xCD395 MOV ESI,dword ptr [EBP+0x8]`,
`0xCD398 MOV EDI,dword ptr [EBP+0xc]`, `0xCD39B MOV ECX,dword ptr [EBP+0x10]`.
If `0xCD39E CMP EDI,ESI` + `0xCD3A0 JA 0xCD3D3`, the copy runs **backwards**
(`0xCD3D3 STD`, `0xCD3D4/0xCD3D8 LEA ...-0x4`, `0xCD3DF MOVSD.REP`, tail
`0xCD3ED MOVSB.REP`, `0xCD3EF CLD`); otherwise small copies use `0xCD3A8
MOVSB.REP` (`0xCD3A3 CMP ECX,0x10`) and larger use aligned
`0xCD3C6 MOVSD.REP` + byte tail. This is the 0x6A literal writer:
src = `stream+5` (`0x9E827 ADD EDI,0x5`), dst = dispatcher `EDX`, len =
bytes `[2..4]`.

## 4. The `0x9E860` call relationship and the unmatched `0xAE474`

`decode_record_strict` (`0x9E860`) is a one-line wrapper:
`PUSH 0x1` (`0x9E860`) / `CALL 0x9E718` (`0x9E86C`). `get_xrefs_to 0x9E860`
returns **10 direct call sites in 8 functions**:

`0x14C65` (FUN_00014c18), `0x18CE8` (FUN_00018c90), `0x23C0C`
(FUN_00023b38), `0x24DE6` (FUN_00024b00), `0x4A32A` (FUN_0004a2e0),
`0x4A376` (FUN_0004a344), `0x78E5E`/`0x78EAB`/`0x78EB7` (FUN_00078dac),
`0xAE474` (`vgt_decode_k`).

FU-11 (`FU11_dpmi_probe_findings.md`, site 2) reported 135/140 runtime frames
returning `caller_link=0xAE479` and flagged it UNMATCHED against the FU-9
census. Statically it is the **call inside `vgt_decode_k`**: `0xAE474` holds
`e8 e7 03 ff ff` = `CALL 0x9E860`, return `0xAE479`. The two arguments are
pushed at `0xAE467` (`ctx[10]+0x10`, the surface pixel area) and `0xAE473`
(`payload + 0x14 + 3*palette_count`, see section 5). So the FU-11 probe was
exercising the kVGT path itself.

## 5. kVGT decode context (`vgt_decode_k 0xAE218..0xAE48F`, 197 insns)

FU-5 owns the header mapping; the measured instruction anchors are:
`0xAE22E` reads BE16 at +8 (width) into `ctx[0]` (`0xAE241 MOV [ESI],EAX`),
`0xAE24F`/`0xAE262` +10 (height) into `ctx[1]`, `0xAE273`/`0xAE282` +12 into
`ctx[2]`, `0xAE295`/`0xAE2A4` +14 (palette count) into `ctx[3]` (`EBP`).

* Palette loop `0xAE2AD..0xAE2CE`: reads 3 bytes at `payload+0x14+3*i`
  (`0xAE2B3 MOV BL,byte ptr [EAX+0x14]`, `0xAE2B9 [EAX+0x15]`, `0xAE2C5
  [EAX+0x13]` after `0xAE2BC ADD EAX,0x3`) and writes `ctx+0x44`,
  `ctx+0x45`, `ctx+0x46` before advancing `EDI+=3`.
* Four old buffers are freed if set (`0xAE2E7`, `0xAE2FF`, `0xAE316`,
  `0xAE32D`).
* Scratch allocation: `0xAE2D4 IMUL EAX,[ESP+0xC]` (W*H), `0xAE2D9 ADD
  EAX,0x3`, `0xAE2DC AND AL,0xFC` → surface size. `ctx[10] = alloc(size+0x10)`
  (`0xAE34B`), `ctx[0xb] = alloc(size)` (`0xAE362`), `ctx[0xd] = alloc(H*8)`
  (`0xAE380`), `ctx[0xc] = alloc(size/16*4)` (`0xAE3A5`).
* Surface headers: 16 zero bytes then tag `0x7B` (`0xAE3E0 STOSB.REP`,
  `0xAE3EB OR byte ptr [EAX],0x7B`), BE16 width at `+4` (`0xAE3F5 MOV word
  ptr [EAX+0x4],DX`), BE16 height at `+6` (`0xAE400`); repeated for the
  second buffer (`0xAE40E..0xAE434`).
* Row table `0xAE438 IMUL EDI,dword ptr [ESP+0x10]` with `EDI` negated at
  `0xAE432 NEG EDI` (so start = `-W*H`), loop `0xAE447..0xAE458` storing
  `ctx[0xd][i] = -W*H + i*W` (`0xAE44A MOV dword ptr [EDX+EAX*0x1],EDI` +
  `0xAE454 ADD EDI,EDX` where `EDX=W`).
* Decode call: `0xAE467 PUSH EAX` (`ctx[10]+0x10`), `0xAE46E ADD EAX,0x14`,
  `0xAE46C SUB EDX,EBP` + `0xAE471 ADD EAX,EDX` (stream =
  `payload+0x14+3*count`), `0xAE473 PUSH EAX`, `0xAE474 CALL 0x9E860`.
  The return value is ignored; `0xAE481 MOV dword ptr [ESP+0x4],EBX` sets
  the success flag returned at `0xAE485`.
* `vgt_dispatch` (`0xAE4BC..0xAE51A`) returns `ctx[10]` (`[param_1+0x28]`,
  decompile) — the decoded surface pointer, not a size; FU-5's "decoded size
  lands at context+0x28" is superseded.

`vgt_stream_poll` (FU-5) saves the decoded palette after dispatch:
decompile shows `if (DAT_000563fc != 0) FUN_000cd390(DAT_000563e8 + 0x44,
&LAB_000560c0, 0x300)`, i.e. `ctx+0x44 -> 0x560C0` (direction confirmed by
`memmove_bytes` param order). Only two callers enter the poll:
`FUN_00068108` and `FUN_00068194` (`get_function_callers 0x67BA8`).

## 6. fVGT pipeline (`vgt_decode_f` and helpers)

### 6.1 `vgt_decode_f` (`0xADEFC..0xAE215`, 259 insns)

Header: BE16 at +8/+10/+12/+14 are read as `dword>>16` at `0xADF34`
(width→`ctx[4]` `0xADF4E`), `0xADF54` (height→`ctx[5]` `0xADF6C`), `0xADF72`
(field B→`ctx[6]` `0xADF8A`), `0xADF90` (field K→`ctx[7]` `0xADFA3`).
At entry `ctx[10]`/`ctx[0xb]` are swapped (`0xADF25 MOV EAX,[EBP+0x28]`,
`0xADF28 MOV EDX,[EBP+0x2C]`, `0xADF2B`/`0xADF31`).

Scratch allocation: `ctx[0xf] = alloc(width*8)` (`0xADFF4 SHL EAX,0x3`,
`0xADFFD CALL 0x98BF8`), `ctx[0x10] = alloc(width*4)` (`0xAE01B SHL EAX,2`,
`0xAE024`), `ctx[0xe] = alloc((height+B)*16)` (`0xAE050 ADD EDI,[ESP+0xC]`,
`0xAE07C SHL EAX,0x4`, `0xAE085`).

**Step 1 (correction to FU-5).** `unpack_signed_fields` is called at
`0xAE0D5` with `EAX=ctx[0xf]` (`0xAE0D2 MOV EAX,[EBP+0x3C]`),
`EDX=payload+0x14` (`0xAE0C1 ADD ESI,0x14`, `0xAE0CC MOV EDX,ESI`),
`EBX=2*width` (`0xAE0C6 LEA EBX,[EDX+EDX*0x1]`), `ECX=10` bits
(`0xAE0B9 MOV ECX,0xA`). It therefore **unpacks the 2*width signed 10-bit
index table into `ctx[0xf]`** — it does not build a row table at `ctx[0xd]`
as FU-5 states. The table is `((width*20+0x1F)&~0x1F)>>3` bytes
(`0xAE0AD..0xAE0CE`, result `[ESP+0x1C]`), matching FU-5's step 2 formula.

**Step 3 (inline, no separate function).** Expansion loop
`0xAE0F6..0xAE127`: row base `= ctx[0xd] + height*4`
(`0xAE0DE MOV EDX,[EBP+0x34]`, `0xAE0E1 SHL EAX,0x2`, `0xAE0E8 ADD EDX,EAX`,
saved `0xAE0EE`); per entry, `0xAE104 MOV EDX,dword ptr [EDI+EBX+0x4]`
(`ctx[0xf][i+1]`), `0xAE10C SHL EDX,0x2`, `0xAE10F ADD EDX,ESI` (row base),
`0xAE114 MOV EDX,dword ptr [EDX]`, `0xAE116 MOV ESI,dword ptr [EDI+EBX]`
(`ctx[0xf][i]`), `0xAE11C ADD EDX,ESI`, `0xAE121 MOV dword ptr
[EBX+EAX*0x1-0x4],EDX` (`ctx[0x10][i]`). Net effect:
`ctx[0x10][i] = ctx[0xf][i] + ctx[0xd][height + ctx[0xf][i+1]]`.
The producer of `ctx[0xd]` on the fVGT path is **not** in `vgt_decode_f`
(it is a read-only reference there) — open leg.

**Step 4 (inline, no separate function).** Raw 16-byte block copy:
source advances past the packed table `0xAE131 ADD EDI,EAX`,
`0xAE146 MOV ESI,[ESP+0x20]`, `0xAE14A MOV ECX,EAX` (`height*16`,
`0xAE137 SHL EAX,0x4`), `0xAE14C MOV EDI,[EBP+0x38]` (`ctx[0xe]`),
`0xAE14F MOVSB.REP` — copies **height** raw 16-byte records (field at +10),
not block_count as FU-5 states.

`expand_palette_block` call `0xAE185`: `0xAE16E MOV ECX,[ESP+0xC]` (field B)
pushed, `0xAE17D ADD EAX,EDX` (`ctx[0xe]+height*16`) pushed as dst,
`0xAE180 MOV ESI,[ESP+0x28]` pushed as src (post-raw-block payload);
B records of 8 bytes expand to 16.

Row stream: `0xAE19B MOV EAX,dword ptr [ESP]` (`ctx[0]`, pitch),
`0xAE19E IMUL EAX,EDX` (`ctx[1]`, height), `0xAE1A5 SAR EAX,0x4` → count =
`pitch*height/16`; `0xAE1A8 IMUL EDX,EAX` with `EDX=[ESP+0x4]` (field K) and
`0xAE1B1 SAR EDX,0x3` — if the K-bit stream is non-empty, call
`unpack_block_indices` at `0xAE1CB`: pushes right-to-left are `0xAE1C4 PUSH
ECX` (K), `0xAE1C5 PUSH EAX` (count), `0xAE1C6 PUSH EDI` (post-expansion
payload src), then `0xAE1C7 MOV EDI,[EBP+0x30]` + `0xAE1CA PUSH EDI`
(`ctx[0xc]` dst).

Composite: `0xAE203 CALL 0xBA994` (`composite_4x4_blocks`) with 9 args
pushed right-to-left at `0xAE1D3..0xAE202`: param_9 `height>>2` (`0xAE1D9`),
param_8 `pitch>>2` (`0xAE1E0`), param_7 `pitch` (`0xAE1E4`), param_6
`ctx[0xe]` (`0xAE1E8`), param_5 `ctx[0x10]` (`0xAE1EC`), param_4 width
`ctx[4]` (`0xAE1F3`), param_3 `ctx[10]+0x10` (`0xAE1F7`), param_2
`ctx[0xb]+0x10` (`0xAE1FE`), param_1 `ctx[0xc]` (`0xAE202`).
Return is the local success flag (`0xAE20B MOV EAX,[ESP+0x14]`).

### 6.2 `unpack_signed_fields` (`0xADD60..0xADDED`, 54 insns)

Register-argument leaf: `EAX`=dest, `EDX`=packed source, `EBX`=count,
`ECX`=bits. `0xADD71 LEA EAX,[EBX-0x1]` + `0xADD74 IMUL EAX,ECX` starts at bit
`(count-1)*bits`; the loop (`0xADDAF..0xADDE5`) loads a dword
(`0xADDBB MOV ESI,dword ptr [ESI]`), shifts with `CL=bit&7`
(`0xADDC0 SHR ESI,CL`), masks `(1<<bits)-1` (`0xADDC8 MOV EBP,[ESP+0x10]`),
sign-extends if bit `bits-1` is set (`0xADDD4 TEST ECX,ESI` where
`ESI=1<<(bits-1)`, `0xADDD8 OR ECX,dword ptr [ESP+0x4]` where the mask is
`-1<<bits`) and stores **descending** (`0xADDDD MOV dword ptr [EDX-0x4],ECX`,
`0xADDE0 SUB EDX,0x4`). For the fVGT call `(count=2W, bits=10)` this yields
`2W` signed 10-bit values in reverse order.

### 6.3 `expand_palette_block` (`0xADDF0..0xADEFA`, 97 insns)

cdecl `(src, dst, count)` (`0xADDF3 MOV EAX,[ESP+0x10]` src, `0xADDF7
EDX=[ESP+0x14]` dst, `0xADDFB EBP=[ESP+0x18]` count). Per record: `0xADE09
MOV EBX,dword ptr [EAX+0x4]` (the 4 index bytes), then 16 palette lookups
`(EBX >> (30-2k)) & 3` into `src[k]` (`0xADE0E SHR EDI,0x1E` + `0xADE11
AND EDI,0x3` + `0xADE14 MOV CL,byte ptr [EDI+EAX*0x1]`; the last two use
`0xADEDE AND EBX,0x3`), writing 16 bytes (`0xADEF1 JL 0xADE09`), advancing
the source by 8 (`0xADED1 ADD EAX,0x8`) and dst by 0x10
(`0xADEE4 ADD EDX,0x10`). Palette = first 4 bytes, indices = next 4 bytes;
matches FU-5 step 5.

### 6.4 `unpack_block_indices` (`0xBA8F0..0xBA990`, 63 insns)

cdecl `(dst, src, count, bits)`: `0xBA8F8 MOV ECX,[ESP+0x1C]` (bits),
`0xBA909 MOV EAX,0x1` + `0xBA90E SHL EAX,CL` + `0xBA913 LEA EBP,[EAX-0x1]`
mask. `0xBA910 SUB EDX,0x4` + `0xBA916 JL` handles the `count<4` tail.
Main loop `0xBA918..0xBA966` extracts **four values per iteration**:
bit offset `EBX`, values 1/2 from a dword at `EBX>>3` (`0xBA91C SHR EAX,0x3`,
`0xBA922 MOV EAX,dword ptr [EAX+ESI*0x1]`, rotate-in `0xBA925 SHRD EAX,EAX,CL`,
mask `0xBA92A AND ECX,EBP`, store `0xBA92C MOV dword ptr [EDI],ECX`, second
value `0xBA931 SHRD EAX,EAX,CL` + `0xBA936 MOV [EDI+0x4],EAX`); values 3/4
from bit offset `EBX + 2*bits` (`0xBA939 LEA EAX,[EBX+ECX*0x2]`,
`0xBA945 MOV EAX,[EAX+ESI*0x1]`, stores `0xBA94F`/`0xBA95A`); then `0xBA963
LEA EBX,[EBX+ECX*0x4]`. Tail loop `0xBA96D..0xBA98A` stores single values
and advances the bit cursor by `bits` (`0xBA981 ADD EBX,[ESP]`). The 4-value
interleaving is what makes this a row-stream unpacker.

### 6.5 `composite_4x4_blocks` (`0xBA994..0xBAA45`, 66 insns)

9-argument compositor; writes to `param_3` (`ctx[10]+0x10`) reading
`param_2` (`ctx[0xb]+0x10`) and `param_6` (`ctx[0xe]`). `0xBA9B0 MOV EAX,
[EBP+0xC]` + `0xBA9B3 SUB EAX,[EBP+0x10]` + `0xBA9B6 MOV [EBP-0xC],EAX`
precomputes the source-destination delta; `0xBA9BF LEA ECX,[EDX+EDX*0x2]`
(`3*pitch`). Per index: `0xBA9D0 MOV EAX,dword ptr [ESI]` (next entry from
`ctx[0xc]`), `0xBA9D5 CMP EAX,dword ptr [EBP+0x14]` (vs width); if
`< width`, `0xBA9DD MOV EBX,dword ptr [EBX+EAX*0x4]` (`ctx[0x10][idx]`) +
`0xBA9E0/0xBA9E3` computes the source and copies four dwords to the four
rows of the destination (`0xBA9E7`/`0xBA9EC`/`0xBA9F2`/`0xBA9F8`). If
`>= width`, `0xBA9D8 JGE 0xBAA0E` takes a full 16-byte block from `ctx[0xe]`
at `(idx-width)*16` (`0xBAA0E MOV EBX,[EBP-0x8]`, `0xBAA11 LEA EAX,[EAX*8]`,
`0xBAA18 LEA EBX,[EBX+EAX*2]`) and stores its four dwords per row
(`0xBAA1B..0xBAA2E`). Row advance `0xBAA3A ADD EDI,ECX` (3*pitch) and row
counter `0xBAA3C SUB dword ptr [EBP+0x28],0x1` + `0xBAA40 JG 0xBA9C5`;
inner count `0xBA9FE SUB dword ptr [EBP+0x24],0x1` + `0xBAA02 JLE
0xBAA3A`.

## 7. Port notes (proposed C functions)

| kernel | address | proposed C |
| --- | --- | --- |
| record dispatcher + header | `0x9E718` | `f96_decode_record(const u8 *stream, u8 *dst, int strict, u32 *out_len)` |
| strict wrapper | `0x9E860` | `f96_decode_record_strict(...)` = above with strict=1 |
| stream resolve | `0x9E3EC` | `static const u8 *f96_decode_stream_resolve(const u8 *p)` |
| method classify | `0x9E420` | `static int f96_decode_method_classify(const u8 *p, int *hdr_extra)` |
| size probe | `0x9E890` | `static u32 f96_decode_size_probe(const u8 *p)` |
| refpack LZ | `0xB18F8` | `static u32 f96_lz_refpack_decode(const u8 *src, u8 *dst)` |
| 0x16FB LZSS | `0x9E1E4` | `static u32 f96_lz_16fb_decode(const u8 *src, u8 *dst)` |
| 0x32FB bit codec | `0x9C7E0` | `static u32 f96_huff_32fb_decode(const u8 *src, u8 *dst)` |
| 0x47FB tree codec | `0x9DA14` | `static u32 f96_tree_47fb_decode(const u8 *src, u8 *dst)` |
| delta/prefix codec | `0x9DC1C` | `static u32 f96_delta_prefix_decode(const u8 *src, u8 *dst)` |
| row RLE | `0x9DC94` | `static u32 f96_rle_row_decode(const u8 *src, u8 *dst)` |
| block copy | `0xCD390` | `f96_memmove(void *dst, const void *src, u32 len)` |
| kVGT decoder | `0xAE218` | `int f96_vgt_decode_k(struct f96_vgt_ctx *ctx, const u8 *payload)` |
| fVGT decoder | `0xADEFC` | `int f96_vgt_decode_f(struct f96_vgt_ctx *ctx, const u8 *payload)` |
| 10-bit unpack | `0xADD60` | `void f96_unpack_signed_fields(int32_t *dst, const u8 *src, int count, int bits)` |
| palette expand | `0xADDF0` | `void f96_expand_palette_block(const u8 *src, u8 *dst, int count)` |
| row-stream unpack | `0xBA8F0` | `void f96_unpack_block_indices(uint32_t *dst, const u8 *src, int count, int bits)` |
| 4x4 composite | `0xBA994` | `void f96_composite_4x4_blocks(uint32_t *idx, const u8 *prev, u8 *surface, int width, const uint32_t *rowtab, const u8 *blocks, int pitch, int pitch4, int rows4)` |

Behavioural requirements a C port must preserve (measured):

* `f96_decode_record` parses bytes `[2..4]` BE24 **before** validating
  `stream[1] == 0xFB` (`0x9E734..0x9E751`).
* `stream[0]&0xFE` selects the method; odd first bytes are legal and change
  header length inside the handlers (`0x9E759`).
* Unknown methods return 0; strict mode additionally calls the error printer
  (`0x9E840..0x9E856`).
* `f96_vgt_decode_k` ignores the decoder's return and reports success by the
  allocation flag (`0xAE481`); the dispatch return is the surface pointer
  (`ctx[10]`, `vgt_dispatch`).
* fVGT index expansion and raw-block copy are inline in
  `f96_vgt_decode_f`; do not look for helper functions (`0xAE0F6..0xAE127`,
  `0xAE142..0xAE14F`).

## 8. Open legs / corrections

1. **FU-5 corrections.** (a) `0x9E718` is a single-record method dispatcher,
   not a command/run-stream loop; no pointer advance, no backward edge.
   (b) `unpack_signed_fields` (`0xADD60`) fills `ctx[0xf]`, not a row table
   at `ctx[0xd]`; the fVGT producer of `ctx[0xd]` (read at `0xAE0DE`) is not
   statically located in this slice. (c) fVGT raw 16-byte block copy count
   is the field at +10; `expand_palette_block` count is the field at +12.
   (d) `vgt_dispatch` returns `ctx[10]` (surface pointer), not a decoded
   size. (e) `0xAE474` is `vgt_decode_k` itself, so FU-11's "unmatched
   caller" is fully attributed.
2. **`ctx[0]`/`ctx[1]` (pitch/height) and `ctx[0xd]` source for fVGT.**
   `vgt_decode_f` reads them (`0xAE0DE`, `0xAE19B`) but never writes them;
   the 0x344-byte context allocator `FUN_000AE490` only zeroes the buffer.
   Populated by out-of-chain game state not found in this slice.
3. **`huff_32fb_decode` header/table.** The bit layout feeding the code
   length/symbol/order tables (`[ESP+0x4D0]` assembly at `0x9C987/0x9C9F7`,
   stack locals `local_53c`/`local_43c`/`local_23c`) is decompiler-visible
   but not transcribed instruction-by-instruction; the `0x32FB`/`0x34FB`
   transforms and length semantics are confirmed.
4. **`tree_47fb_decode` tree semantics.** Child-table bit selection and the
   `tree_emit_symbol` recursion contract (`0x9D9D0..0x9DA10`) are not
   resolved; the record framing, table fill and zero-literal terminator are.
5. **`lz_16fb_decode` symbol readers.** `FUN_0009E160`/`FUN_0009E1A0`
   (`0x9E160..0x9E19F`, `0x9E1A0..0x9E1E3`) index static tables; the
   literal/match symbol coding is not transcribed.
6. **`lz_refpack_decode` opcode table.** The four control classes and the
   `0xFC` end marker are cited, but the full literal/match length/distance
   field map was not re-derived.
7. **Runtime-initialized data.** `0x33D0` (resolve prefix) and `0x33DC`
   (error format) are zero in the image; no static text exists there.
8. **Raw 0x17 handling.** Selector 0x16 also accepts `0x17FB`, which takes
   the 32-bit LE length path (`0x9E2A4..0x9E2FF`); no asset using it was
   observed statically.
9. **Thunk/clean-decompile check.** All functions in scope decompiled
   cleanly except `decode_method_classify`, whose decompiler output drops
   two unreachable blocks (`0x9E617`, `0x9E6A0`) with warnings; the cited
   classification arms are intact. No thunks were encountered in the cited
   set.

## 9. Provenance

Ghidra program `/fifa96_le.bin` (Raw Binary, x86:LE:32:default, base 0),
MCP bridge connected 2026-10-03. Calls run for this document:

* `decompile_function`: `0x9E718`, `0x9E860`, `0x9E3EC`, `0xAF37B`,
  `0xB18F8`, `0x9E1E4`, `0x9C7E0`, `0x9DA14`, `0x9DC1C`, `0x9DC94`,
  `0xCD390`, `0x9E420`, `0x9E890`, `0x9E160`, `0x9E1A0`, `0xADEFC`,
  `0xADD60`, `0xADDF0`, `0xBA8F0`, `0xBA994`, `0xAE218`, `0xAE4BC`,
  `0x67BA8`, `0xAE490`, `0x9D9D0`, `0x68108`, `0x68194`, `0x679F4`,
  `0xAE738`, `0x68B58`, `0xCAD30`.
* `disassemble_function`: `0x9E718` (112 insns), `0x9E860`, `0x9E3EC`,
  `0x9E890`, `0xB18F8` (136), `0x9E1E4` (186), `0x9C7E0` (1198, saved from
  truncated tool output), `0x9DA14` (151), `0x9DC1C` (54), `0x9DC94` (85),
  `0xCD390` (50), `0xADEFC` (259), `0xADD60` (54), `0xADDF0` (97),
  `0xBA8F0` (63), `0xBA994` (66), `0xAE218` (197).
* `get_function_by_address` for body ranges (`0x9E420`→`0x9E715`,
  `0x9D9D0`→`0x9DA10`, `0xB18F8`→`0xB1A67`, `0xAE4BC`→`0xAE51A`,
  `0x9E160`→`0x9E19F`, `0x9E1A0`→`0x9E1E3`).
* `get_function_callers`: `0x9E718`, `0x9E860`, `0x67BA8`, `0xADD60`.
* `get_xrefs_to`: `0x9E860` (10 call sites), `0x9E718` (2).
* `read_memory 0x33D0` (32 bytes) — all zero.
* `rename_function` (17 functions, table at the top) and `save_program`.
* Static cross-check for callers: `python3 tools/fifa96_callers.py`? Not
  run in this slice; caller lists above come from `get_xrefs_to` /
  `get_function_callers`.

Self-review: every algorithmic claim above was written from the listed tool
output. The decisive addresses were re-verified in a second pass with
`ghidra_search_instructions` filters: all 10 `0x9E860` call sites, both
`0x9E718` call sites, all 9 dispatcher `CALL`s, all 4 fVGT helper `CALL`s,
`BSWAP`/`CMP 0xFB`/`AND 0xFE` in `0x9E718`, and the `0x16FB`, `0x32FB`,
`0x34FB`, `0x47FB`, `0x62FB`, `0x66FB`, `0x7BFB` markers, plus
`OR [EAX],0x7B` in `vgt_decode_k`. Inline-script auditing was unavailable
(`GHIDRA_MCP_ALLOW_SCRIPTS` unset); byte-level checks outside that search
set rely on the captured decompile/disassemble output listed above.
