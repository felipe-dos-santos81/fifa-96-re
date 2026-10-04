# FU-24: Huffman opcode map for `huff_32fb_decode` (0x9C7E0)

Static transcription of the bit-packed codec selected by
`decode_record_dispatch` for selectors 0x30/0x32/0x34
(`docs/ghidra/FU19_vgt_decode_kernels.md` §3.3, open leg 3; selector-set
correction in `docs/ghidra/FU21_method_vectors.md` §2). The spec below is
derived instruction-by-instruction from the printed disassembly of
`/fifa96_le.bin` and is validated byte-exactly against the committed
FU-21 golden vector `tests/golden/vgt/record-30.{in,out}.bin` (raw
`stream[0]` = 0x31, i.e. the 8-byte header form). No external Huffman or
EA documentation was used: every formula and bit assignment cites
operand-level output from this binary. All addresses are link-time flat
addresses (FU-4), as in FU-19/FU-22.

## 1. Scope and entry chain

* `0x9E718 decode_record_dispatch` masks the record selector
  (`0x9E759 AND AL,0xFE`, FU-19) and for 0x30/0x32/0x34 reaches the huff
  arm (FU-21 §2: `0x9E7B6 CMP AL,0x30` falls through to `0x9E7FF`).
  Disassembled call site `0x9E7F4..0x9E80B`:
  `0x9E7FF MOV EBX,0x1`, `0x9E804 MOV EAX,EDI` (stream),
  `0x9E806 CALL 0x9C7E0`, `0x9E80B MOV ESI,EAX` (return collected for the
  common epilogue at `0x9E859`). `EDX` (destination) is passed through
  unchanged from the dispatcher.
* `0x9C7E0..0x9D9CF` (1198 instructions, single `RET` at `0x9D9CF`; body
  range from `disassemble_function 0x9C7E0`). The body contains **no
  `CALL`**: header parse, both tables, the decode loop and the
  post-transforms are all inline.
* Entry-state use: `EAX` = stream, `EDX` = destination (saved at
  `0x9C7EA MOV [ESP+0x4C4],EDX` = output base and
  `0x9C7F1 MOV [ESP+0x4C8],EDX` = output cursor), `EBX` = table-enable
  flag (the dispatcher's `0x9E7FF MOV EBX,0x1`; `0x9C9FE TEST EBX` /
  `0x9CA00 JZ 0x9D36D` skips table construction when zero — no VGT caller
  passes zero).
* NULL stream returns 0: `0x9C801 TEST EAX` / `0x9C803 JZ 0x9D9BE` with
  `EAX` zeroed at `0x9C7F8 XOR EDX,EDX` / `0x9C7FA MOV [ESP+0x4D0],EDX`.

## 2. Calling convention and return

`__fastcall`-style register args (`EAX` stream, `EDX` dst, `EBX` flag;
the decompiler signature `uint huff_32fb_decode(undefined4, byte *)`
exposes only `EAX`). Prologue `0x9C7E0..0x9C7E4` saves
`ECX/ESI/EDI/EBP` and reserves `0x52C` bytes.

Return is the **declared output size** built at
`0x9C987 MOV [ESP+0x4D0],EAX` and `0x9C9F7 MOV [ESP+0x4D0],ECX`, loaded
once at the exit `0x9D9BE MOV EAX,[ESP+0x4D0]` (epilogue
`0x9D9C5 ADD ESP,0x52C` then `POP`s and `RET`). It is not the number of
bytes actually written by the decode loop (see §6, §7) — for record-30
they happen to agree (2271).

## 3. Header layout

`stream[0]&1` (`0x9C87B MOV AH,[ESP+0x4D5]`, `0x9C885 TEST AH,0x1`,
`0x9C888 JZ 0x9C903`) selects the two forms. Bit0 **clear** jumps past
the extra-header code (`0x9C903`); bit0 **set** falls through
`0x9C88A..0x9C900`, which shifts four more bytes (`stream[4..7]`) into
the bit staging buffer and sets `EDI=8` (`0x9C8F2 MOV EDI,0x8`).

| form | raw selector | bytes | declared size | special | first bit |
| --- | --- | --- | --- | --- | --- |
| 4-byte | bit0 = 0 (0x30/0x32/0x34) | `[magic 2][BE24 len][special]` | BE24 `d[2..4]` | `d[5]` | MSB of `d[6]` |
| 8-byte | bit0 = 1 (0x31/0x33/0x35) | `[magic 2][BE24 extra][BE24 len][special]` | BE24 `d[5..7]` | `d[8]` | MSB of `d[9]` |

Derivation, both forms:

* `magic`/staging: `0x9C809..0x9C823` assemble `stream[0]<<8|stream[1]`
  into `[ESP+0x520]` and `0x9C82E/0x9C833/0x9C838` keep the BE16 magic in
  `[ESP+0x4D4]`.
* Declared size: `EBP` is the top byte of the staged word
  (`0x9C90A MOV EBP,ESI`, `0x9C912 SHR EBP,0x18`); the low 16 bits are
  stored at `0x9C987 MOV [ESP+0x4D0],EAX` after
  `0x9C981 SHR EAX,0x10`; the high byte is OR'd in at
  `0x9C9EB MOV ECX,[ESP+0x4D0]`, `0x9C9F2 SHL EBP,0x10`,
  `0x9C9F5 OR ECX,EBP`, `0x9C9F7 MOV [ESP+0x4D0],ECX`.
  * 8-byte form: `EBP = d[5]`, low = `d[6]d[7]` → declared =
    `BE24(d[5..7])`. record-30: `00 08 df` = **2271**.
  * 4-byte form: `EBP = d[2]`, low = `d[3]d[4]` → declared =
    `BE24(d[2..4])`.
* Special symbol (`local_14` in the decompile): the next 8 bits,
  `0x9CA06 MOV EAX,ESI`, `0x9CA08 SUB EDI,0x8`, `0x9CA0B SHR EAX,0x18`,
  `0x9CA11 MOV [ESP+0x528],AL`. record-30: `d[8]=0x2F` and the probe
  read `special=0x2f`.
* `local_68` = magic with bit0 cleared: `0x9C90F AND DL,0xFE`,
  `0x9C918 MOV [ESP+0x4D5],DL`; compared as `0x32FB`/`0x34FB` in §8.
* The 8-byte form's `d[2..4]` field is read by the dispatcher as the
  generic BE24 "header24" (`0x9E731..0x9E743`, FU-19 §1) but is never
  used by this kernel; record-30 has header24 = 4696 = 0x1258 while the
  decoded length is 2271 (FU-21 §3 already recorded this divergence).

## 4. Bit reader and the variable-length value code

**Reader state.** The kernel keeps a 32-bit staging window
`buf = [ESP+0x520]`, a 32-bit shift register `ESI`, a counter `EDI`
(nominally `valid_bits - 16`), and a byte cursor `[ESP+0x524]`. A refill
(R1) is the repeated instruction block, e.g. `0x9C923..0x9C975`:
`0x9C92C MOV DL,[EAX]` / `0x9C935 SHL EAX,0x8` / `0x9C938 OR EDX,EAX`
(append first byte), the same pair for the second byte
(`0x9C94C`, `0x9C958`, `0x9C95E`), `0x9C96E MOV ESI,EDX`,
`0x9C956 NEG ECX` (CL = -EDI) + `0x9C973 SHL ESI,CL`,
`0x9C95B ADD EDI,0x10`, and cursor `+2` (`0x9C970`/`0x9C975`). In the
main loop the same refill appears as an in-line variant (R2,
`0x9D305..0x9D364`) that computes `CL = 16-EDI`
(`0x9D323 MOV ECX,0x10`, `0x9D345 SUB ECX,EDI`, `0x9D364 SHL ESI,CL`)
and does not touch `EDI` (the entry path has already added 16 at
`0x9D438`).

Semantics (what the clean simulator implements): because `ESI = buf <<
(-EDI)` while `EDI < 0`, the register always holds `EDI+16` valid bits
at its top and the refill appends the next 16 stream bits; the detail is
buffering only. The code stream is consumed **MSB-first**. After the
header each table/decode read is a `consume(n)` of the top `n` bits
followed by `EDI -= n` and a refill when `EDI < 0`.

**Value code.** All three places that read an integer (code-length
counts, symbol-order gaps, special-run lengths) use one prefix code:
`0^j || binary(v)`, where `v >= 4` and `bitlen(v) = j+3`; i.e. `j`
leading zero bits, then `v` written MSB-first in `j+3` bits. Total bits
consumed = `2j+3`. Instruction evidence:

* top bit set (`0x9CAC9 TEST ESI` / `0x9CACB JGE 0x9CB43` not taken):
  `0x9CAD2 SHR EBX,0x1D` takes the top 3 bits, `0x9CAD5 SHL ESI,0x3`,
  `0x9CACF SUB EDI,0x3`, optional refill `0x9CAE0..0x9CB3E`, then
  `0x9CDAA SUB EBX,0x4` — the three bits are `1xx` = `v` in 4..7.
* top bit clear and `ESI>>16 != 0` (B1, `0x9CB43..0x9CB4A`): the
  `0x9CB50..0x9CB5A` loop shifts until the sign bit is set, `0x9CB5C
  LEA EBX,[EAX-1]` + `0x9CB5F SUB EDI,EBX` account for the leading
  zeros, `0x9CB61 ADD ESI,ESI` consumes the first 1; the common
  extraction `0x9CC3F..0x9CD3E` reads the remaining `k` bits and
  `0x9CD9F..0x9CDA8` (`MOV EAX,1`, `SHL EAX,CL`, `ADD EBX,EAX`) folds in
  `1<<k`; `0x9CDAA SUB EBX,0x4` produces the count.
* top bit clear and `ESI>>16 == 0` (B2, `0x9CBCB..0x9CC3B`): a
  leading-zero loop with per-bit `DEC EDI` and in-loop refills, then the
  same common extraction — the same `0^j || binary(v)` value, for
  `j >= 16`. Not exercised by record-30 (see §10.3).

Examples (`v`, bit pattern, bits): `4=100` (3), `5=101` (3),
`8=01000` (5), `14=01110` (5), `44=000101100` (9), `83=00001010011`
(11). The callers then apply: `cnt = v-4` (§5), `n = v-3` (§5),
`r = v-4` (§7). The B2 and B1 paths are the same code; the binary merely
batches the `EDI` accounting differently (sign-extension of the loop
counts at `0x9CB58`/`0x9CF01` is the top-bit test).

## 5. Canonical table construction

The tables are built on the stack (base addresses relative to `ESP`;
FU-19 §3.3 named the locals): code lengths `local_53c` at `+0x000`,
symbols `local_43c` at `+0x100`, symbol order `local_23c` at `+0x300`,
`cum` at `+0x400`, `fc` at `+0x440`, `bc` at `+0x480`, plus the used
bitmap `local_33c` at `+0x200`.

**(a) Code-length counts.** Init `0x9CA75..0x9CA9B`: `L=1`, shift `15`,
`local_5c=0` (total symbols), `local_58=0` (code-space position),
`fc/bc` byte offsets `4`, `cum` shift `15`. Per iteration
(`0x9CAA2..0x9CAC2`):

```
local_58 *= 2                       ; 0x9CAA9 ADD EAX,EAX
bc[L]    = local_58 - local_5c      ; 0x9CAB9 SUB EAX,EBX (EBX=local_5c)
                                    ;   0x9CABB MOV EBX,[ESP+0x510] (offset)
                                    ;   0x9CAC2 MOV [ESP+EBX+0x480],EAX
v        = read_value()             ; 0x9CAC9..0x9CDA8 (§4)
cnt      = v - 4                    ; 0x9CDAA SUB EBX,0x4
fc[L]    = cnt                      ; 0x9CDBD MOV [ESP+EAX+0x440],EBX
local_58 += cnt                     ; 0x9CDCD ADD EAX,EBX
local_5c += cnt                     ; 0x9CDCB ADD EBP,EBX
cum[L]   = cnt ? (local_58 << (16-L)) & 0xffff : 0
L += 1                              ; 0x9CE03 INC EDX
continue while cnt == 0 or cum[L] != 0   ; 0x9CE1C JZ / 0x9CE24 JNZ
```

The loop stops at the first length with `cnt != 0` and `cum[L] == 0`;
the exit stores `maxlen = L-1` (`0x9CE2F DEC EDX`, `0x9CE3E` writes
`cum[maxlen] = 0xffffffff` at `0x9CE2A MOV EBX,0xffffffff`). In
algebraic terms, with `P`/`N` the `local_58`/`local_5c` values **before**
the iteration and `P' = 2P + cnt` afterwards,
`bc[L] = 2P - N` and `cum[L] = P' << (16-L)` (for `cnt != 0`), so
`bc[L]` is the first L-bit code minus the count of shorter symbols and
`cum[L]` is the first (L+1)-bit code scaled to 16 bits. `cum` is used by
the long-code search in §6; `cum[maxlen] = -1` is the sentinel.

record-30: counts by length 1..11 = `0, 0, 0, 0, 1, 10, 40, 79, 77, 35,
14`; total 256; maxlen 11.

**(b) Symbol order.** `0x9CE76..0x9D17B` reads `total` values with the
same code; for each, `n = v-3` (`0x9D136 SUB EBX,0x4`, `0x9D139 INC
EBX`) and the symbol cursor `b` (initial `0xFF`, `0x9CE5E MOV AL,0xFF`)
is advanced: `0x9D13C INC AL` / `0x9D142 CMP CL,[ESP+EDX+0x200]` /
`0x9D14B DEC EBX` until `EBX == 0` (`0x9D14E JNZ`). The landed byte is
marked in the used bitmap (`0x9D150/0x9D154 MOV byte [ESP+EDX+0x200],1`)
and appended to `order` (`0x9D164/0x9D16B MOV byte [ESP+EDX+0x2ff],AL`),
looping while the index < total (`0x9D179 CMP` / `0x9D17B JL`). So `n`
is a 1-based count of not-yet-used symbols to advance in circular byte
order; record-30's `order[0..255]` was dumped and matches this rule.

**(c) 8-bit prefix tables.** `len53` is filled with `0x40`
(`0x9D181..0x9D19D`, 64 dword stores of `0x40404040`). For `L = 1..8`
(`0x9D205 CMP EBP,0x9` / `0x9D208 JGE 0x9D36D` breaks above 8) and each
symbol of length L in `order` sequence, `2^(8-L)` consecutive entries
are written (`0x9D215 MOV EBP,1` / `0x9D21A SHL EBP,CL` with CL from
`7` down; the inner loop `0x9D28F..0x9D2AE` stores the marker to
`len53` at `0x9D2A3 MOV [EDX-1],CL` and the symbol to `sym43` at
`0x9D298 MOV [EBX-1],CL`). The special symbol
is stored with marker byte `0x60` instead of its length
(`0x9D265 CMP AL,[ESP+0x528]`, `0x9D270 MOV [ESP+0x4CC],EAX` records
its length as `local_70`, `0x9D27E MOV [ESP+0x4F4],0x60`).
Uncovered entries stay `0x40`; those are the prefixes shared by long
codes (L > 8).

record-30 (dumped at `0x9D36D` entry): `len53[0..7]=05`,
`[8..47]=06`, `[48..127]=07`, `[128..206]=08` except
`len53[138]=0x60` (the special, 11th length-8 symbol), `[207..255]=0x40`;
`sym43[138]=0x2F` and the surrounding entries are the `order` list.

## 6. Decode loop

Head `0x9D36D`: `0x9D36F SHR EDX,0x18` takes the top 8 bits and
`0x9D372 MOV DL,[ESP+EDX]` reads `len53`; `0x9D375 AND EDX,0xFF`,
`0x9D37B SUB EDI,EDX` charges the code length, `0x9D37D TEST EDI` /
`0x9D37F JL 0x9D438` handles the refill boundary. The loop is unrolled
five times (`0x9D385..0x9D430`) writing `sym43[top8]` to the output
cursor for each short code and `SHL ESI,CL` by the length; when the
counter goes negative it either takes the in-loop R2 refill
(`0x9D438 ADD EDI,0x10` / `0x9D2F0..0x9D366`) or the slow path
(`0x9D443 SUB EDI,0x10`, `0x9D446 ADD EDI,EDX` restores the counter).

Slow path (markers `0x40` and `0x60`):

* `0x9D448 CMP EDX,0x60` / `0x9D44B JZ 0x9D46D`: the special uses its
  recorded length `local_70` (`0x9D46D MOV EDX,[ESP+0x4CC]`).
* Otherwise (long code, `len53 == 0x40`) the real length is found by
  comparing the 16-bit prefix against `cum`: `0x9D44D MOV EDX,0x8`,
  `0x9D452 MOV EAX,ESI`, `0x9D459 SHR EAX,0x10`, then
  `0x9D45C MOV EBP,[ESP+ECX+0x404]` (`cum[9]`, `ECX` starts `0x20`) /
  `0x9D463 ADD ECX,0x4` / `0x9D466 INC EDX` / `0x9D467 CMP EAX,EBP` /
  `0x9D469 JC 0x9D474` — exit at the first L with
  `(ESI>>16) < cum[L]`, with `cum[maxlen] = -1` guaranteeing
  termination.
* Symbol fetch `0x9D474..0x9D493`: `0x9D47B SUB ECX,EDX` (32-L),
  `0x9D484 SHR EAX,CL` (top L bits), `0x9D47D MOV EBX,[ESP+EDX*4+0x480]`
  (`bc[L]`), `0x9D488 SUB EAX,EBX`, `0x9D493 MOV AL,[ESP+EAX+0x300]`
  (`order`). So `symbol = order[(top_L bits) - bc[L]]`, the same
  first-code-minus-shorter-symbols indexing that makes `order` the
  canonical decode table.
* Special test `0x9D48C MOV DL,[ESP+0x528]` / `0x9D49C CMP AL,DL` /
  `0x9D49E JZ 0x9D4B4`; a non-special symbol is written and the loop
  continues (`0x9D4A4..0x9D4AF`, and after an optional refill
  `0x9D4B8..0x9D50A`, `0x9D511 CMP AL,[ESP+0x528]` /
  `0x9D51A..0x9D52D`).

## 7. Output rules: runs, escape, terminator

A decoded `symbol == special` transfers to `0x9D532` (output cursor in
`EDX`); the kernel then reads a second value with the §4 code
(`0x9D539` sign path, `0x9D5B3` B1, `0x9D63B` B2, common extraction
`0x9D6AA..0x9D853`) and computes `r = v - 4` (`0x9D857 SUB EBX,0x4`).

| test | instructions | action |
| --- | --- | --- |
| `r != 0` | `0x9D85A JZ` not taken; `0x9D85C LEA EAX,[EDX+EBX]`; `0x9D85F MOV BL,[EDX-1]`; `0x9D862 INC EDX` / `0x9D863 MOV [EDX-1],BL` / `0x9D866 CMP` / `0x9D868 JC` | append `r` copies of the previous output byte |
| `r == 0`, next bit = 1 | `0x9D86F MOV EBX,ESI` / `0x9D871 DEC EDI` / `0x9D872 SHR EBX,0x1F` / `0x9D875 ADD ESI,ESI`; refill `0x9D87B..0x9D8D2`; `0x9D8D4 TEST EBX` / `0x9D8D6 JNZ 0x9D957` | end of stream (go to §8) |
| `r == 0`, next bit = 0 | `0x9D8DC MOV EAX,ESI` / `0x9D8DE SUB EDI,0x8` / `0x9D8E1 SHR EAX,0x18` / `0x9D8E4 SHL ESI,0x8` / `0x9D8E7 MOV BL,AL`; refill; `0x9D946 MOV EAX,[ESP+0x4C8]` / `0x9D950 MOV [EAX],BL` | append one literal byte (the 8 bits after the flag) |

Note the task-facing naming: the `uVar14 - 4 == 0` arm is the
terminator/escape arm; the **repeat-last-byte** arm is `r != 0`. The
special symbol itself is never written.

There is **no output-length bound**: the only reads of the declared size
`[ESP+0x4D0]` in the whole body are the two header stores
(`0x9C987`, `0x9C9F7`), the transform setup (`0x9D95E`) and the return
(`0x9D9BE`); the loop's `EDI` checks are bit-counter checks. Termination
is exclusively the `r == 0`/flag-1 end marker above.

record-30 output accounting (simulator instrumentation): 1753 short
codes (L 5..8) + 503 long codes (L 9..11) + 7 repeat runs of 2 bytes
each + 1 escape = 2271 bytes. The escape is literal `0x2F` (the special
value itself) at output offset 178; the runs are at offsets 429, 580,
634, 1432, 1688, 1883, 2154. Total code length use: L5 53, L6 313,
L7 686, L8 701, L9 407, L10 81, L11 15.

## 8. Declared size, return and the 0x32FB/0x34FB transforms

After the end marker, `0x9D957 MOV EDI,[ESP+0x4C4]` (base) /
`0x9D95E MOV EBX,[ESP+0x4D0]` (declared) / `0x9D965 MOV ECX,[ESP+0x4D4]`
(magic) / `0x9D96C ADD EDI,EBX` (end) select the transform:

* `0x9D96E CMP ECX,0x32FB` / `0x9D974 JNZ 0x9D995`: inclusive
  **byte prefix sum** over `[base, base+declared)`:
  `0x9D985 XOR EDX,EDX` / `0x9D987 MOV DL,[EAX]` / `0x9D98A ADD EBX,EDX`
  / `0x9D98C MOV [EAX-1],BL`, loop `0x9D98F CMP` / `0x9D991 JNC`.
* `0x9D995 CMP ECX,0x34FB` / `0x9D99B JNZ 0x9D9BE`: inclusive **double
  prefix sum** (`EBX` accumulates the first sum at `0x9D9B2 ADD EBX,ECX`,
  `EDX` the second at `0x9D9B5 ADD EDX,EBX`, store `0x9D9B7 MOV
  [EAX-1],DL`, loop `0x9D9BA`/`0x9D9BC`).
* Otherwise return unchanged.

Because `local_68` is masked (`0x9C90F AND DL,0xFE`), raw selectors
0x32/0x33 reach the 0x32FB arm and 0x34/0x35 the 0x34FB arm. The
transform length is the declared size, not the written count.

Return: `0x9D9BE MOV EAX,[ESP+0x4D0]`; record-30 returns **2271**, which
matches both `record-30.json` `out_len` and the byte count written (the
encoder placed the end marker after exactly the declared number of
bytes). The dispatcher's `header24` for record-30 (4696) is the other
BE24 field and is not touched by this kernel.

## 9. Validation

Throwaway simulator `/tmp/opencode/fu24/huff_sim.py` (**not committed**)
implementing §3–§8 exactly. Decisive command:

```
$ python3 /tmp/opencode/fu24/huff_sim.py tests/golden/vgt/record-30.in.bin tests/golden/vgt/record-30.out.bin
declared=2271 written=2271
expected_len=2271
out == expected: True
declared == expected_len: True
```

Result: the simulator returns 2271 and its 2271 output bytes are
**byte-identical** to `tests/golden/vgt/record-30.out.bin` (sha256
`f494440bb4f0719769fc393fda731f91b55a21a6a82760dc9b30ddd6d3776992`).
A second, independently written instruction-faithful emulator of the
whole 1198-instruction body (`/tmp/opencode/fu24/x86emul.py`, derived by
transliterating the printed disassembly) reproduces the same result
(`return EAX=0x8df (2271)`, `byte-identical: True`), and a table
cross-check (`xcheck.py`) compares the emulator's stack arrays at
`0x9D36D` entry with §5's construction: `declared, special, maxlen,
len53, sym43, order, special_len, fc, bc, cum` all **MATCH** (10/10).

Transform/variant cross-check (`variants.py`): the same record-30 input
with only the raw selector byte changed, run through both
implementations, agrees byte-for-byte and in return value for
`0x31` (0x30FB, no transform), `0x33` (0x32FB, prefix sum) and `0x35`
(0x34FB, double prefix sum): `ALL MATCH`. The 0x33/0x35 runs exercise
§8's transforms on a real bitstream, but there is no captured golden
output for those selectors (FU-21 §6), so this is
specification-vs-transliteration agreement, not asset validation. The
bitstream is consumed to logical bit 18295 (2287 bytes including the
terminator bit); the binary's buffered cursor is at `d+2290` (16-bit
refill granularity).

## 10. Open legs

1. **No committed vector for the 0x32FB/0x34FB transforms.** The paths
   are disassembly-derived and cross-checked against the instruction
   emulator with synthetic raw 0x33/0x35 selectors on the record-30
   bitstream (§9), but no captured asset output exercises them; FU-21
   observed only raw 0x31.
2. **No vector for the 4-byte header form (`stream[0]&1 == 0`).** Its
   layout is derived from the same instruction path
   (`EBP=d[2]`, declared `BE24(d[2..4])`, special `d[5]`, bits from
   `d[6]`), but no valid 4-byte stream exists in the committed set;
   forcing record-30 through it overruns the fixed stack tables (the
   kernel has no bounds checks), so it is not simulator-validated.
3. **B2 branch of the value code unexercised.** All j in record-30 are
   ≤ 5; the `ESI>>16 == 0` path (`0x9CBCB`/`0x9CF6F`/`0x9D63B`) is
   derived as the same `0^j || binary(v)` code but never fires.
4. **Special symbol with length > 8.** `local_70` is only recorded for
   L ≤ 8; if the special were in the long-code group the `0x40` search
   supplies the length instead. Derived, untested.
5. **Incomplete code space (`total < 256`).** The fill covers only
   `L ≤ 8` and stops at `maxlen`; unfilled `len53` entries stay `0x40`,
   so a bit pattern with no assigned code routes into the long-code
   search instead of an error. Untested.
6. **Malformed input.** No input-end or output-end check exists:
   truncated data reads past the slice and a missing end marker loops
   indefinitely; the transform may read `declared` bytes even when the
   loop wrote fewer.
7. **Why the 8-byte form carries the unused BE24 at `d[2..4]`** (the
   dispatcher's header24) and how the special byte is chosen by the
   encoder are not visible in this kernel.

## 11. Provenance

Ghidra program `/fifa96_le.bin` (Raw Binary, `x86:LE:32:default`,
base 0, bridge 2026-10-03; program already carrying the FU-19 names):

* `decompile_function 0x9C7E0` — signature, header split, table locals
  (`local_53c`/`local_43c`/`local_23c`/`aiStack_fc`/`aiStack_bc`/
  `auStack_13c`), special handling and transforms.
* `disassemble_function 0x9C7E0` — full 1198-instruction listing quoted
  throughout (saved; every address above read from it).
* `disassemble_bytes 0x9E7F4..0x9E80B` — dispatcher huff arm:
  `0x9E7FF MOV EBX,0x1` / `0x9E804 MOV EAX,EDI` / `0x9E806 CALL
  0x9C7E0` / `0x9E80B MOV ESI,EAX`.
* `search_instructions` second pass in `huff_32fb_decode`:
  `CMP ECX,0x32fb` → `0x9D96E` only; `CMP ECX,0x34fb` → `0x9D995`
  only; all six `[ESP+0x4d0]` refs (`0x9C7FA`, `0x9C987`, `0x9C9EB`,
  `0x9C9F7`, `0x9D95E`, `0x9D9BE`); all three `[ESP+0x528]` refs
  (`0x9CA11`, `0x9D265`, `0x9D48C`); `AND DL,0xfe` → `0x9C90F`;
  `JZ 0x9D86F` → `0x9D85A`; `JNZ 0x9D957` → `0x9D8D6`;
  `CMP AL,[ESP+0x528]` → `0x9D511`.
* Golden inputs (committed): `tests/golden/vgt/record-30.in.bin`
  (17408 B, slice from `captures/session-vgt21-3`),
  `record-30.out.bin` (2271 B), `record-30.json` (`method_raw` 49,
  `out_len` 2271, `header24` 4696).
* Simulator and scaffolds in `/tmp/opencode/fu24/` (intentionally not
  committed): `huff_sim.py` (derived spec), `x86emul.py` (faithful
  transliteration), `xcheck.py` (table cross-check), `variants.py`
  (selector/transform cross-check).

Self-review: every formula above was re-read against the printed
listing and the emulator's live state; the full 2271-byte output and
return value matching, plus the 10/10 table cross-check, are the
end-to-end evidence that no field extraction was misassigned. Items not
covered by the committed vector are named in §10 rather than asserted.
