# FU-22: RefPack opcode map for `lz_refpack_decode` (0xB18F8)

Static transcription of the LZ/RefPack decoder selected by
`decode_record_dispatch` for selector 0x10/0x11
(`docs/ghidra/FU19_vgt_decode_kernels.md` §3.1; that document's open leg 6).
The spec below is derived instruction-by-instruction from the printed
disassembly of `/fifa96_le.bin` and is then validated byte-exactly against
the committed FU-21 golden vector
`tests/golden/vgt/record-10.{in,out}.bin`. No external EA RefPack
documentation was used: every formula cites operand-level output from this
binary. All addresses are link-time flat addresses (FU-4).

## 1. Scope and entry chain

* `0x9E718 decode_record_dispatch` masks the record selector
  (`0x9E759 AND AL,0xFE`, FU-19) and for 0x10/0x11 executes
  `0x9E7D4 CMP AL,0x10` / `0x9E7DC JBE 0x9E7E4` (both raw 0x10 and 0x11
  reach here because of the mask), then
  `0x9E7E4 PUSH 0x1`, `0x9E7E6 PUSH EDX`, `0x9E7E7 PUSH EBX`,
  `0x9E7E8 CALL 0xB18F8`, `0x9E7ED MOV ESI,EAX` (disassembled range
  `0x9E7D0..0x9E7F0`). `EBX` is the record base, `EDX` the caller's
  destination, and the pushed `0x1` is the decoder's third argument.
* `0xB18F8..0xB1A67` (136 instructions, `RET` at `0xB19F8`; body range from
  `get_function_by_address 0xB18F8`). The body contains **no `CALL`**: all
  decoding is inline; there are no helper functions to transcribe.
* `get_xrefs_to 0xB18F8` returns exactly one call site, `0x9E7E8` (the
  dispatcher). The `0x10/0x11` split is handled inside the record header,
  not by a separate function.

## 2. Calling convention and return

cdecl `(stream, dst, decode_flag)`; from the decompiler signature
`uint lz_refpack_decode(ushort *param_1, uint *param_2, int param_3)` and
the prologue reads:

* `0xB1900 MOV ECX,[EBP+0x10]` — `param_3` (`decode_flag`);
* `0xB1903 MOV EBX,[EBP+0x8]` — stream;
* `0xB1906 MOV EDI,[EBP+0xc]` — destination cursor.

Return value is the **24-bit big-endian length field parsed from the
header**, stored at `0xB1935 MOV [EBP-0x4],EAX` and loaded at
`0xB19F2 MOV EAX,[EBP-0x4]` (the single epilogue; also reached by the two
early-outs below). It is not the number of bytes actually written (see §7
and the 5-byte overrun in §9).

Early-outs, both returning the in-register `local_8`:

* `0xB1909 MOV [EBP-0x4],0x0`, `0xB1910 OR EBX,EBX`,
  `0xB1912 JZ 0xB19F2` — NULL stream returns 0.
* `0xB1938 CMP ECX,0x0`, `0xB193B JZ 0xB19F2` — with `param_3 == 0` the
  header is parsed and returned, and the command loop is skipped. The VGT
  dispatcher always passes 1 (`0x9E7E4 PUSH 0x1`), so the VGT path decodes.

## 3. Record header (inside the handler)

The dispatcher has already required `stream[1] == 0xFB`
(`0x9E747..0x9E751`, FU-19 §1); this kernel ignores `stream[1]`.

* `0xB1918 MOV AX,word [EBX]` reads the first word (only `AL` is used),
  `0xB191B LEA EBX,[EBX+2]`.
* `0xB191E AND AL,0x1` + `0xB1920 JZ 0xB1925` + `0xB1922 LEA EBX,[EBX+3]`:
  bit0 of `stream[0]` selects **3 extra bytes**. Layout therefore is either
  `[cmd,0xFB][BE24 len][commands…]` (raw 0x10) or
  `[cmd,0xFB][3 extra][BE24 len][commands…]` (raw 0x11).
* `0xB1925 XOR EAX,EAX` / `0xB1927 MOV AL,[EBX]` / `0xB1929 SHL EAX,0x10` /
  `0xB192c MOV AH,[EBX+1]` / `0xB192f MOV AL,[EBX+2]` /
  `0xB1932 LEA EBX,[EBX+3]`: **BE24** length `b0<<16 | b1<<8 | b2`, stored
  at `0xB1935` and later returned.
* Command stream starts after that field. For `record-10.in.bin`:
  `10 fb | 00 27 54 | e2 …`, length `0x002754` = 10068, first control at
  file offset 5.

## 4. Control-byte dispatch tree

The loop head is entered once with `ECX = 0`
(`0xB1941 XOR ECX,ECX`, `0xB1943 JMP 0xB198D`) and re-entered after every
command at `0xB1967`, `0xB198D`, `0xB19BE`, `0xB19DE`, `0xB1A42`, `0xB1A59`.
Every re-entry is preceded by a `MOVSB.REP`/`MOVSD.REP`, which leaves
`ECX == 0` (x86 REP semantics), so the `OR CL,[EBX]` at the re-entry sets
the flags purely from the control byte, and `OR CL,[EBX]; MOV EDX,[EBX]`
loads the little-endian dword at the control byte into `EDX`.

Class split, each step a printed conditional:

| test | instructions | result |
| --- | --- | --- |
| bit7 = 0 | `0xB198D OR CL,[EBX]` / `0xB198F MOV EDX,[EBX]` / `0xB1991 JNS 0xB196D` | class A |
| bit7 = 1, bit6 = 0 | `0xB1993 ADD CL,CL` / `0xB1995 JS 0xB19C8` | class B |
| bit7 = 1, bit6 = 1, bit5 = 0 | `0xB19C8 ADD CL,CL` / `0xB19CA JNS 0xB19FC` | class C |
| bit7..5 = 1, control < 0xFC | `0xB19CC CMP DL,0xFC` / `0xB19CF JNC 0xB19E8` | class D |
| bit7..5 = 1, control ≥ 0xFC | same `CMP`/`JNC` | end marker |

So the classes are exactly the contiguous ranges 0x00–0x7F (A),
0x80–0xBF (B), 0xC0–0xDF (C), 0xE0–0xFB (D), 0xFC–0xFF (end). Within each
literal/match class the order is **literals first, then the back-reference**;
the next control byte is read at the original control address plus
command size plus literal count.

## 5. Command table

`c` = control byte, `b1/b2/b3` = the next stream bytes. `dst` = current
output cursor (`EDI`). Source for a back-reference is
`EDI - 1 - distance` (`0xB195E`/`0xB1984`/`0xB19B2`/`0xB1A1B`
`LEA ESI,[ECX+EDI-0x1]` with `ECX = -distance`).

| class | control | bytes consumed | literals (src) | match length | distance | cited field ops |
| --- | --- | --- | --- | --- | --- | --- |
| A | 0x00–0x7F | `2 + (c&3)` | `c&3` at ctrl+2 | `((c&0x1C)>>2)+3` = 3..10 | `((c&0xE0)>>5)<<8 \| b1` = 0..0x3FF | lit `0xB196D AND ECX,0x3`, `0xB1948 LEA ESI,[EBX+2]`; len `0xB1953 AND EDX,0x1C`+`0xB1959 SHR EDX,0x2`+`0xB1962 LEA ECX,[EDX+3]`; dist `0xB194F MOV CH,DL`/`0xB1951 MOV CL,DH`/`0xB1956 SHR CH,0x5`; copy `0xB1965 MOVSB.REP` |
| B | 0x80–0xBF | `3 + (b1>>6)` | `b1>>6` at ctrl+3 | `(c&0x3F)+4` = 4..67 | `((b1&0x3F)<<8) \| b2` = 0..0x3FFF | lit `0xB1997 MOV CL,DH`+`0xB199C SHR ECX,0x6`+`0xB199F AND ECX,0x3`, `0xB1999 LEA ESI,[EBX+3]`; len `0xB19B6 AND EDX,0x3F`+`0xB19B9 LEA ECX,[EDX+4]`; dist `0xB19A8 SHR ECX,0x10`+`0xB19AB MOV CH,DH`+`0xB19AD AND CH,0x3F`; copy `0xB19BC MOVSB.REP` |
| C | 0xC0–0xDF | `4 + (c&3)` | `c&3` at ctrl+4 | `(((c&0x0C)<<6) \| b3)+5` = 5..773 | `((c&0x10)<<12) \| (b1<<8) \| b2` = 0..0x1FFFF | lit `0xB19FC MOV ECX,EDX`/`0xB1A01 AND ECX,0x3`, `0xB19FE LEA ESI,[EBX+4]`; dist `0xB1A0C AND ECX,0x10`/`0xB1A12 SHL ECX,0xC`/`0xB1A15 MOV CL,AH`/`0xB1A17 MOV CH,AL`; len `0xB1A1F ROL EDX,8`/`0xB1A22 SHR DH,2`/`0xB1A25 AND EDX,0x3FF`+`0xB1A30 LEA ECX,[EDX+5]` |
| D | 0xE0–0xFB | `1 + 4*n`, `n=(c&0x1F)+1` | `4*n` = 4..112 bytes at ctrl+1 (dword `MOVSD.REP`) | — | — | `0xB19D1 AND EDX,0x1F`+`0xB19D7 LEA ECX,[EDX+1]`, `0xB19D4 LEA ESI,[EBX+1]`, `0xB19DA MOVSD.REP` |
| end | 0xFC–0xFF | `1 + (c&3)` | `c&3` final bytes at ctrl+1 | — (stops the loop) | — | `0xB19CC CMP DL,0xFC`/`0xB19CF JNC 0xB19E8`; `0xB19EA LEA ESI,[EBX+1]`/`0xB19ED AND ECX,0x3`/`0xB19F0 MOVSB.REP` |

Notes on the layout arithmetic (all from the listed ops):

* Class A: the dword loaded at the control byte has `c` in bits 0–7 and `b1`
  in bits 8–15, so `0xB1953 AND EDX,0x1C` takes bits 2–4 **of the control
  byte** for the length, while `SHR CH,0x5` takes bits 5–7 of the control
  byte for the high distance bits. A 2-byte command either has no literals
  (`0xB1970 JNZ` falls through to `0xB1972 LEA EBX,[EBX+2]`, then the match
  copy) or `c&3` literals from ctrl+2 (`0xB1948`). Every class A command
  carries a match.
* Class B: `MOV ECX,EDX; SHR ECX,0x10` yields `b2 | b3<<8`; `MOV CH,DH`
  then replaces the high byte with `b1`, and `AND CH,0x3F` keeps bits 0–5.
  `b3` is read past the command but discarded (next control is at
  ctrl+3+lit).
* Class C: `ROL EDX,8` puts `b3` in the low byte and the control byte
  shifted down by 8; `SHR DH,2` moves control bits 2–3 to `DH` bits 0–1;
  `AND EDX,0x3FF` keeps exactly `((c>>2)&3)<<8 | b3`. The distance high bit
  is control bit 4 (`AND ECX,0x10`, `SHL ECX,0xC`).
* Class D count is in **dwords**; the command's literal payload is always a
  multiple of 4 bytes (4..112).

## 6. Copy implementation

* Literals and back-references are copied with `MOVSB.REP ES:EDI,ESI`
  (class A/B everywhere; class C when distance ≤ 4, `0xB1A2B CMP ECX,-0x4` /
  `0xB1A2E JGE 0xB1A54`) or `MOVSD.REP` + `MOVSB.REP` tail (class C when
  distance ≥ 5, `0xB1A36 SHR ECX,0x2`, `0xB1A39 AND EDX,0x3`,
  `0xB1A3C`/`0xB1A40`). Class D literal payload uses `MOVSD.REP`
  (`0xB19DA`).
* Copy direction is **forward**: no `CLD`/`STD` appears anywhere in the
  136-instruction listing; the kernel assumes the platform ABI's DF = 0
  and leaves it untouched (unlike `memmove_bytes 0xCD390`, which manages DF
  itself). A port must enter with DF = 0.
* Overlap is intentional and must be preserved: for distance 0 the source
  is the last output byte (`ESI = EDI-1`), so forward copying replicates
  the previous byte (class A example: control `0x0B`, `b1 0x00`, literals
  `08 1e 00`, distance 0, length 5 — record-10 offsets 18..22). For class C
  distance ≥ 5 the dword copy reads `src = dst-1-distance`; since
  `distance+1 ≥ 6`, every 4-byte source chunk ends at least 3 bytes below
  the destination chunk start, so the dword path is byte-for-byte
  equivalent to a forward byte copy. A byte-at-a-time forward simulator is
  therefore behaviourally exact, and that is what the FU-22 simulator
  implements.
* Output advance is the same for literals and matches (`EDI` moves by the
  copy count); each command's net output is `lit + match_len` (class D:
  `4*n`; end marker: `c&3`).

## 7. Termination and input extent

There is **no output-length check** and **no input-end check** in the
kernel: the only exit from the command loop is the end marker
`0xFC..0xFF`, detected at the loop head (`0xB19CC CMP DL,0xFC` /
`0xB19CF JNC 0xB19E8`). The declared BE24 length is never compared against
anything after it is stored at `0xB1935`.

Therefore the decoder's own consumption rule is:

```
consumed = stop_offset + 1 + (stop_control & 3)
```

where `stop_offset` is the offset of the end-marker byte from the record
base. (`0xB19EA LEA ESI,[EBX+1]` then `MOVSB.REP` with `ECX = c&3`).

For the golden vector this was determined by the simulator, which tracks
the same `EBX` advances derived in §5:

* end marker at `record-10.in.bin` offset **6088** = `0xFC`, low bits 0;
* consumed = **6089 bytes** of the 17408-byte slice (`record-10.json`
  `input_last_nonzero` = 8731, so the whole record lies inside the slice);
* declared/returned length = **10068** = `record-10.json` `header24` and
  `out_len`.

A corrupted stream without a `0xFC..0xFF` control byte would run until a
fault; short/truncated input has no defined behaviour (reads continue past
the slice). This is a decoder-level property, not an anomaly of the
capture.

## 8. Edge cases

* First control byte may be any class, including the end marker itself:
  the loop head is the same code path, so an all-stop record yields zero
  output and the declared length, consumed = 1 + (c&3).
* Raw 0x11 form: bit0 test at `0xB191E` skips 3 extra bytes before the
  BE24 field; not exercised by any committed vector.
* Maximum fields: class A length 10, class B length 67, class C length
  773; literal payloads class A/B/C 3 bytes max (2 bits), class D 112
  bytes; distance encodings 10/14/17 bits respectively.
* `param_3 == 0` is the header-probe mode (returns the length without
  touching the command stream); the VGT dispatcher always passes 1.
* NULL stream returns 0 (`0xB1912`).

## 9. Validation

Throwaway simulator (`/tmp/opencode/fu22/refpack_sim.py`, **not committed**)
implementing §3–§8 (byte-at-a-time forward copy for all matches) and
tracking the input cursor exactly as in §5/§7. Decisive command:

```
$ python3 /tmp/opencode/fu22/refpack_sim.py tests/golden/vgt/record-10.in.bin tests/golden/vgt/record-10.out.bin
declared=10068 written=10073 consumed=6089
expected_len=10068
out[:declared] == record-10.out.bin: True
declared==out_len: True
write overrun past declared length: 5 bytes
```

Result: the simulated decoder returns 10068, consumes 6089 bytes, and the
first 10068 output bytes are **byte-identical** to
`tests/golden/vgt/record-10.out.bin`
(sha256 `d1241738a3dcca0c4b205b6764d1494ac3719df720f10739b1b2caa4bba9fac5`).

The `written=10073` is a measured property of the command stream, not a
simulator error: the final class B command at input offset 6083
(`82 a7 44`, `b1>>6 = 2` literals `34 35`, length `(0x82&0x3F)+4 = 6`,
distance `0x2744`) writes 8 bytes starting at output offset 10065 and ends
at 10073. The golden `.out.bin` is exactly the returned length, so it
captures only the first 10068 bytes; bytes 10068–10072 were written but not
saved. This is consistent with §7 (no output-bound check) and is important
for a port: the destination buffer must accommodate the stream's write
count, not merely the returned length.

Class coverage of this vector (simulator instrumentation): A 1358
commands, B 289, D 184, end 1, **C 0**. Class C's field map is
disassembly-derived only. Earlier derivation iteration: the first run
differed at output byte 18 because the class-A match length had been read
from `b1`; re-reading `0xB1953 AND EDX,0x1C` (control bits 2–4) fixed it,
after which the whole prefix matched. That read-back is recorded here
because it is the one field whose source register was ambiguous on first
pass.

## 10. Open legs

1. **Class C (0xC0–0xDF) has no committed vector coverage** (0 occurrences
   in record-10). Its command size, literal/distance/length formulas and
   the two copy paths are derived from `0xB19FC..0xB1A63` but not
   validated against captured output. Same for the raw 0x11 header-skip
   form (§8).
2. **No independent asset cross-check.** Byte-exactness rests on the
   single FU-21 vector; the headers, stop rule and all formulas agree with
   it, but a second RefPack record with class C commands would pin the
   remaining class.
3. **Runtime DF state** is not observable statically; the spec assumes the
   x86 ABI's DF = 0 (§6). No `CLD` in this kernel.
4. **Malformed-input behaviour** (missing end marker, truncated
   literal/match payloads) is undefined: no bounds check exists, so
   behaviour is whatever the adjacent memory yields.

## 11. Provenance

Ghidra program `/fifa96_le.bin` (Raw Binary, `x86:LE:32:default`, base 0,
bridge 2026-10-03; program already carrying the FU-19 names):

* `decompile_function 0xB18F8` — signature `uint lz_refpack_decode(ushort *,
  uint *, int)`; confirmed the `param_3 != 0` gate and header parse.
* `disassemble_function 0xB18F8` — full 136-instruction listing quoted
  throughout (classes, field ops, copies, end marker, epilogue; `RET` at
  `0xB19F8`).
* `get_function_by_address 0xB18F8` — body `0xB18F8..0xB1A67`.
* `disassemble_bytes 0x9E7D0..0x9E7F0` — dispatcher selector tests,
  `0x9E7E4 PUSH 0x1` / `0x9E7E6 PUSH EDX` / `0x9E7E7 PUSH EBX` /
  `0x9E7E8 CALL 0xB18F8` / `0x9E7ED MOV ESI,EAX`.
* `get_xrefs_to 0xB18F8` — one reference, `0x9E7E8` in
  `decode_record_dispatch`.

Validation inputs (committed): `tests/golden/vgt/record-10.in.bin`
(17408 B, sha256 `86665a0df5b6f3790e4dabdecc1b4ec6424886cc6ef2797f8485543ddfbe6e9c`),
`record-10.out.bin` (10068 B), `record-10.json` (`header24` 10068,
`out_len` 10068, `input_last_nonzero` 8731). Simulator run recorded in §9;
simulator lives in `/tmp` and is intentionally not committed.

Self-review: every formula in §5 was re-read against the disassembly
listing; the whole 10068-byte output prefix matching byte-for-byte is the
end-to-end check that no field extraction was misassigned. The one
uncovered class is named in §10 rather than asserted.

## Errata (2026-10-03, quoted from FU-23)

* §8 "Maximum fields" prints class C length `773`; the formula
  `(((c&0x0C)<<6) | b3) + 5` reaches `0x3FF + 5 = 1028`. `773` is
  `0x300 + 5` and omits the `b3` term. Verified against the class C arm
  `0xB19FC..0xB1A63`: `0xB1A1F ROL EDX,8` / `0xB1A22 SHR DH,2` /
  `0xB1A25 AND EDX,0x3FF` (max 0x3FF) / `0xB1A30 LEA ECX,[EDX+5]`. The
  C port (`b5fa81d`, `src/fifa96_loader/fifa96_refpack.c`) implements the
  full range; the §5 table's `5..773` is superseded by `5..1028`.
