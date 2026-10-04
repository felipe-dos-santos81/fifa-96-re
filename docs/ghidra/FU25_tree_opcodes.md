# FU-25: Tree opcode map for `tree_47fb_decode` (0x9DA14)

Static transcription of the flat-tree codec selected by
`decode_record_dispatch` for selector 0x46
(`docs/ghidra/FU19_vgt_decode_kernels.md` §3.4, open leg 4; selector-set
check in `docs/ghidra/FU21_method_vectors.md` §2). The spec below is
derived instruction-by-instruction from the printed disassembly of
`/fifa96_le.bin` and is validated byte-exactly against the committed
FU-21 golden vector `tests/golden/vgt/record-46.{in,out}.bin` (raw
`stream[0]` = 0x46, i.e. the 2-byte-magic form). No external tree/Huffman
documentation was used: every field and traversal step cites
operand-level output from this binary. All addresses are link-time flat
addresses (FU-4), as in FU-19/FU-22/FU-24.

**Open-leg 4 resolved, and its framing corrected.** The tree is **not**
bit-selected: there is no bit reader anywhere in the kernel. The decode
stream is a byte sequence whose bytes are *symbol keys*; per-key
`flags`/`childA`/`childB` tables decide literal, root terminator, or a
deterministic tree expansion. The full 151-instruction body contains no
shift/bit-test on stream data except the header byte assembly (§3).

## 1. Scope and entry chain

* `0x9E718 decode_record_dispatch` masks the selector
  (`0x9E759 AND AL,0xFE`, FU-19) and for 0x46 reaches the tree arm.
  Disassembled call site (`disassemble_bytes 0x9E7F8..0x9E813`):
  `0x9E80F MOV EAX,EBX`, `0x9E811 CALL 0x9DA14`, with `EBX` = record
  base (`EAX` = stream) and `EDX` passed through unchanged as the
  destination. The dispatcher collects the return at `0x9E816 MOV ESI,EAX`
  (FU-21 §3) and returns it in the common epilogue.
* `0x9DA14..0x9DC1A` (151 instructions, single `RET` at `0x9DC1A`;
  body range from `get_function_by_address 0x9DA14`). The body calls
  exactly one function, twice: `0x9DB87` and `0x9DBCA`, both
  `CALL 0x9D9D0 tree_emit_symbol` (`search_instructions` on
  `tree_47fb_decode`, mnemonic `CALL`: 2 matches).
* `tree_emit_symbol` (`0x9D9D0..0x9DA10`, 21 instructions) is a
  recursive leaf, self-called at `0x9D9EF`; it has no other callees.
* All working tables/markers are **static globals**, not arguments; the
  kernel installs and restores them per call:
  `0x9DA30 MOV [0x5BA9C],EDX` with `EDX = ESP+0x100` (flags),
  `0x9DA38 MOV [0x5BA90],EDX` with `EDX = ESP` (child A),
  `0x9DA47 MOV [0x5BA98],EDX` with `EDX = ESP+0x200` (child B);
  `[0x5BA94]` is the input cursor and `[0x5BA8C]` the output cursor
  (19 `MOV` references to the 0x5BA9x block were enumerated by
  `search_instructions` on operand `5ba9`; the `tree_emit_symbol` block
  by operand `5ba`).

## 2. Calling convention and return

`__fastcall`-style register args: `EAX` = stream, `EDX` = destination
(the decompiler signature is `int tree_47fb_decode(undefined4 param_1,
byte *param_2)`; `param_2` is `EDX`). Prologue
`0x9DA14..0x9DA18` saves `EBX/ECX/ESI/EDI` and reserves `0x30C` bytes;
`0x9DA1E MOV ESI,EDX` moves the destination into the running output
cursor.

* `0x9DA20 XOR EDX,EDX` / `0x9DA22 MOV [ESP+0x304],EDX` initializes the
  return local; `0x9DA4D TEST EAX,EAX` / `0x9DA4F JZ 0x9DBFD` returns 0
  for a NULL stream.
* Return is `0x9DBFD MOV EAX,[ESP+0x304]` — the kernel's own BE24
  declared length built in §3 (or 0 for NULL). It is **not** the count of
  bytes written (§6). Epilogue `0x9DC10 ADD ESP,0x30C`, pops, `RET` at
  `0x9DC1A`; before returning it writes the final cursors to the globals
  (`0x9DC04 MOV [0x5BA8C],ESI`, `0x9DC0A MOV [0x5BA94],EBX`).

## 3. Header layout

Magic: `0x9DA57 LEA EBX,[EAX+1]`, `0x9DA5A MOV DL,[EAX]`,
`0x9DA5E SHL EDX,8`, `0x9DA61 MOV AL,[EBX]`, `0x9DA63 ADD EDX,EAX`
assembles `stream[0]<<8|stream[1]`; `0x9DA66 CMP EDX,0x47FB` /
`0x9DA6C JNZ 0x9DA71` / `0x9DA6E ADD EBX,0x3` bumps the payload base from
`stream+2` to `stream+5` when the magic is 0x47FB. The dispatcher's
`stream[1] == 0xFB` requirement (FU-19 §1) still applies.

Declared length (`0x9DA71..0x9DAA2`, stored at `[ESP+0x304]`): `b0` at
payload[0] (`0x9DA75`), `b1` at payload[1] (`0x9DA77`, `SHL EAX,8` at
`0x9DA81`, `ADD EAX,EDX` at `0x9DA84`), `INC EBX` (`0x9DA8D`), `SHL
EDX,8` (`0x9DA97`), `b2` at payload[2] (`0x9DA9A MOV AL,[EBX+1]`,
`0x9DAA0 ADD EDX,EAX`) — **BE24** `b0<<16 | b1<<8 | b2`.

Root seed: `0x9DABF MOV AL,[EBX]` (payload[3]), `0x9DAC1 MOV
[EDX+EAX],0x1` (flags[root] = 1). Entry count: `0x9DAC7 MOV AL,[EBX+1]`
(payload[4]), `0x9DACA ADD EBX,0x2` → entries start at payload[5],
stored at `[ESP+0x308]` (`0x9DACD`).

| form | raw selector | magic | payload at | declared | root | count | entries |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2-byte | 0x46 | 0x46FB (≠ 0x47FB) | +2 | BE24 `d[2..4]` | `d[5]` | `d[6]` | `d[7…]` |
| bumped | 0x47 | 0x47FB | +5 | BE24 `d[5..7]` | `d[8]` | `d[9]` | `d[10…]` |

Nothing else is read from the header: no bit fields, no flags word.

## 4. Table construction

`0x9DAA9..0x9DABB` clears `flags[0..255]` (`MOV byte [EDX+EAX],0x0` over
the `0x5BA9C` pointer). Then `flags[root] = 1` (`0x9DAC1`). The entry
loop `i = 0..count-1` (`0x9DAE1..0x9DB36`) reads three bytes per entry
`[key, a, b]` and:

* `key` = `0x9DAE9 MOV DL,[EBX]`;
* `childA[key] = a`: `0x9DAFA MOV CL,[EDI]` (EDI = EBX+1),
  `0x9DB03 MOV [EDI],CL` with EDI = `childA+key` (`0x9DAE3`, `0x9DAEE`);
* `childB[key] = b`: `0x9DB16 MOV CL,[EDI]` (EDI = EBX, which was
  advanced by 2 at `0x9DAF0`), `0x9DB20 MOV [EDI],CL` with EDI =
  `childB+key` (`0x9DB05`, `0x9DB0B`);
* `flags[key] = 0xFF`: `0x9DB22`/`0x9DB30 MOV [ECX+EDX],0xFF`;
* `0x9DB28 INC EBX` advances 3 bytes per entry.

So after construction the flag byte has exactly three values: **0** for
an unlisted symbol (literal), **1** for the root seed, **0xFF** for a
listed key (internal). Because the entry loop runs after the seed, a
root that is also listed ends with `0xFF` and loses its terminator role
(untested, §10.3).

## 5. Decode loop

`0x9DB38..0x9DBF8`. Cursor `EBX` continues at `payload + 5 + 3*count`;
output cursor is `ESI`. Per iteration, `sym` is one stream byte:

```
0x9DB40 MOV DL,[EBX]        ; sym
0x9DB42 MOV CL,[ECX+EDX]    ; f = flags[sym]  (ECX = flags base)
0x9DB45 INC EBX             ; consume sym
0x9DB46 TEST CL,CL
0x9DB48 JNZ 0x9DB50
0x9DB4A INC ESI             ; f == 0: literal
0x9DB4B MOV [ESI-1],DL      ;   emit sym
0x9DB4E JMP 0x9DB38
```

### 5.1 Positive flag (the root): literal-or-terminator

`0x9DB50 JGE 0x9DBEB` (signed test: flag 1 is non-negative; 0xFF is not):

```
0x9DBEB XOR EDX,EDX
0x9DBED MOV DL,[EBX]        ; next byte
0x9DBEF INC EBX             ; consume it
0x9DBF0 TEST EDX,EDX
0x9DBF2 JZ 0x9DBFD          ; 0 -> terminate (record end, §6)
0x9DBF4 INC ESI
0x9DBF5 MOV [ESI-1],DL      ; non-zero -> emit as literal
0x9DBF8 JMP 0x9DB38
```

The root symbol byte itself is consumed but never emitted. Encountering
the root is the **only** terminator: the decode ends when a root byte is
followed by a zero byte. (By construction flags 2..0x7F cannot occur;
the same arm would treat them as root.)

### 5.2 Negative flag (a key): inline tree expansion

`0x9DB56 MOV EAX,[0x5BA90]` … `0x9DB61 MOV [0x5BA8C],ESI` (save state),
`0x9DB67 MOV AL,[EDX+EAX]` = `childA[sym]`. The code then executes a
two-stage traversal, exactly as disassembled:

```
0x9DB6A XOR ECX,ECX
0x9DB6C MOV EDI,flags
0x9DB72 MOV CL,AL           ; cur = AL
0x9DB74 CMP byte [ECX+EDI],0
0x9DB78 JZ 0x9DB96          ; leaf -> emit cur (0x9DB96..0x9DB9E)
0x9DB7A MOV EAX,childA
0x9DB7F MOV AL,[ECX+EAX]    ; childA[cur]
0x9DB87 CALL 0x9D9D0        ; tree_emit_symbol(childA[cur])
0x9DB8C MOV EAX,childB
0x9DB91 MOV AL,[ECX+EAX]    ; childB[cur]
0x9DB94 JMP 0x9DB6A         ; loop with new cur
```

* **Stage A** starts at `a = childA[sym]`: while `flags[a] != 0`,
  emit `tree_emit_symbol(childA[a])` and set `a = childB[a]`; then emit
  `a` (`0x9DB96..0x9DB9E`).
* **Stage B** starts at `b = childB[sym]` (`0x9DB9F..0x9DBB5`): while
  `flags[b] != 0`, call `tree_emit_symbol(childA[b])` (`0x9DBCA`) and set
  `b = childB[b]`; then emit `b` (`0x9DBD6..0x9DBE6`) and restore the
  input cursor from `[0x5BA94]` (`0x9DBDD`). The key byte is the only
  input consumed.

`tree_emit_symbol(n)` (`0x9D9D0`):

```
0x9D9D2 XOR EDX,EDX
0x9D9D4 MOV EBX,flags
0x9D9DA MOV DL,AL           ; node = n
0x9D9DC CMP byte [EDX+EBX],0
0x9D9E0 JZ 0x9D9FE          ; leaf -> append node, advance [0x5BA8C]
0x9D9E2 MOV EAX,childA
0x9D9E7 MOV AL,[EDX+EAX]    ; childA[n]
0x9D9EF CALL 0x9D9D0        ; recurse
0x9D9F4 MOV EAX,childB
0x9D9F9 MOV AL,[EDX+EAX]    ; childB[n]  (EDX survives the call)
0x9D9FC JMP 0x9D9D2         ; loop with n = childB[n]
```

i.e.

```
emit_symbol(n):
    while flags[n] != 0:
        emit_symbol(childA[n])
        n = childB[n]
    *out++ = n
```

The inner loops test `!= 0` (any flag, including 1, means "internal"),
unlike the main loop's signed `JGE`; only the main loop reads the root
flag as positive.

**Equivalence of the inline form.** Unrolling `emit_symbol(sym)` for an
internal `sym` gives `emit_symbol(childA[sym])` followed by the same
loop restarted at `childB[sym]`, which is exactly
`emit_symbol(childB[sym])`; stage A and stage B above are those two
recursive calls with their loop bodies inlined, so
`expand_main(sym) ≡ emit_symbol(sym)`. The simulator implements the
two-stage form exactly and asserts the equality per occurrence (§9).

## 6. Output, termination and input consumption

* The loop has **no output-length bound** and **no input-end check**:
  the only termination is §5.1's root-plus-zero byte. `[ESP+0x304]`
  (declared) is initialized to 0 (`0x9DA22`), written in the header and
  read only by the return.
* Input consumption for the golden record: header+entries end at offset
  **337** (2 + 5 + 3·110); the decode stream ends with the terminator at
  offsets **2294** (root `0xFE`) / **2295** (zero); total bytes consumed
  from the record base = **2296** (measured by the simulator cursor,
  which increments exactly as `EBX` does).
* Output accounting (simulator instrumentation): 1128 literal bytes
  (`f == 0`) + 829 key expansions + 1 root/terminator event. Each
  expansion emits 2 direct leaves and 1910 recursive
  `tree_emit_symbol` bytes: `1128 + 829·2 + 1910 = 4696`.
* Stage-A/B and `tree_emit_symbol` share the global output cursor
  `[0x5BA8C]`; the main loop syncs it at `0x9DB61`/`0x9DB96`/`0x9DBA4`
  and `0x9DBD6`. Cursor bookkeeping is exact but invisible in the output
  bytes.

## 7. Return value, `out_len` and `header24`

Return is `[ESP+0x304]`, the kernel's own BE24 at the payload base
(`0x9DBFD`). For the committed record-46:

* `record-46.in.bin` starts `46 fb 00 12 58 fe 6e …`: magic 0x46FB,
  declared `0x001258` = **4696**, root `0xFE`, count `0x6E` = 110.
* The dispatcher's generic parse (`0x9E731..0x9E743`, FU-19 §1) reads
  BE24 at `stream+2`, which for the 2-byte form is the same field. Hence
  `out_len` = `header24` = 4696 here — but the decoder returns its own
  `local_18` (`0x9DBFD`), not the dispatcher's temporary. For a 0x47FB
  record the two diverge: the dispatcher would read `d[2..4]` while the
  kernel returns BE24 `d[5..7]` (§3). `record-46.json` `out_len` 4696 is
  the captured decoder return; `header24` 4696 is the dispatcher field.

## 8. Magic 0x47FB bumped-header form

Derived from `0x9DA66 CMP EDX,0x47FB` / `0x9DA6E ADD EBX,0x3`: when raw
`stream[0]<<8|stream[1] == 0x47FB`, the payload base is `stream+5`, so
the three bytes `stream[2..4]` (which the dispatcher read as `header24`)
are skipped and the kernel reads its declared length/root/count from
`stream[5..7]`, `stream[8]`, `stream[9]`. Both 0x46 and 0x47 normalize to
selector 0x46 (`0x9E759 AND AL,0xFE`), and the dispatcher's
`stream[1]==0xFB` check admits both. Status: **not exercised by any
committed vector** — FU-21 observed only raw 0x46 — so the form is
disassembly-derived only (§10.1).

## 9. Validation

Throwaway simulator `/tmp/opencode/fu25/tree_sim.py` (**not committed**)
implementing §3–§5 (exact two-stage inline expansion; recursive
`emit_symbol` implemented separately for the equivalence assert).
Decisive command:

```
$ python3 /tmp/opencode/fu25/tree_sim.py tests/golden/vgt/record-46.in.bin tests/golden/vgt/record-46.out.bin
declared=4696 written=4696 consumed=2296
stats={'lit0': 1128, 'lit_pos': 0, 'expand': 829} emit_symbol_calls=1910 emit_bytes=1910
expected_len=4696
out == expected: True
declared == expected_len: True
out sha256: 0dd1c09bb98d524ba63645cc29920a731ad5b6ee318235ec2eef687a5eb83528
exp sha256: 0dd1c09bb98d524ba63645cc29920a731ad5b6ee318235ec2eef687a5eb83528
```

Result: the simulator returns 4696 and its 4696 output bytes are
**byte-identical** to `tests/golden/vgt/record-46.out.bin` (sha256
`0dd1c09bb98d524ba63645cc29920a731ad5b6ee318235ec2eef687a5eb83528`;
input sha256
`35c43f59a96a9632575041867ae25f54edcc41a967c2af825e08cfb613a6c599`).
Every one of the 829 expansions passed the in-loop assert
`two-stage inline == tree_emit_symbol(sym)`. Instrumented header/table
state: magic 0x46FB, declared 0x1258, root 0xFE, count 110, entries end
337, root not among the keys (flags[root] = 1), 110 keys / 145
unlisted-flag symbols, terminator root at 2294 with zero at 2295.

## 10. Open legs

1. **No committed vector for the 0x47FB bumped-header form.** Its layout
   is derived from `0x9DA66..0x9DA6E` and the same field code; no asset
   in the FU-21 set used raw 0x47. Forcing record-46 through it is not
   meaningful (the three bytes there are length data, not padding).
2. **Malformed input.** No bounds checks anywhere: a truncated entry
   table or missing terminator reads past the slice; the output buffer is
   never checked against the declared length; a cyclic `childA`/`childB`
   chain would recurse indefinitely.
3. **Root listed as a key.** The seed at `0x9DAC1` is overwritten by the
   entry loop's `0xFF`, which would remove the only terminator. Derived,
   untested (record-46 root 0xFE is not listed).
4. **Positive root followed by a non-zero literal** (`0x9DBF4/5`): the
   arm is transcribed, but record-46's only root event is followed by 0
   (`lit_pos = 0`), so the literal variant is unexercised.
5. **Encoder-side construction** (how keys/children are chosen) is not
   visible in this kernel; only the decoder contract is fixed.
6. **Static globals.** `0x5BA90/0x5BA94/0x5BA98/0x5BA9C/0x5BA8C` make
   the decoder non-reentrant; not a behavioural leg for this port, noted
   for completeness.

## 11. Provenance

Ghidra program `/fifa96_le.bin` (Raw Binary, `x86:LE:32:default`, base 0,
bridge 2026-10-03; program already carrying the FU-19 names):

* `decompile_function 0x9DA14` — signature, stack arrays, header/table
  structure, loop shapes and `return local_18`.
* `disassemble_function 0x9DA14` — full 151-instruction listing quoted
  throughout (magic bump, BE24, flags clear, seed, entry fill, all three
  loop arms, both `CALL 0x9D9D0`, return `0x9DBFD`, `RET 0x9DC1A`).
* `decompile_function 0x9D9D0` and `disassemble_function 0x9D9D0` —
  recursive leaf quoted at `0x9D9D2..0x9DA10`.
* `get_function_by_address 0x9DA14` (body `0x9DA14..0x9DC1A`),
  `0x9D9D0` (body `0x9D9D0..0x9DA10`).
* `disassemble_bytes 0x9E7F8..0x9E813` — dispatcher tree arm:
  `0x9E80F MOV EAX,EBX`, `0x9E811 CALL 0x9DA14`.
* `search_instructions` second pass: `tree_47fb_decode` operand `5ba9`
  (19 refs, table/cursor globals), mnemonic `CALL` (2 calls, `0x9DB87`
  and `0x9DBCA`), `CMP 0x47FB` (single match `0x9DA66`); 
  `tree_emit_symbol` operand `5ba` (5 refs: flags/childA/childB/output
  cursor).
* Golden inputs (committed): `tests/golden/vgt/record-46.in.bin`
  (17408 B, slice from `captures/session-vgt21-3`),
  `record-46.out.bin` (4696 B), `record-46.json` (`method_raw` 70,
  `out_len` 4696, `header24` 4696, `length_contract` false).
* Simulator in `/tmp/opencode/fu25/` (intentionally not committed):
  `tree_sim.py`.

Self-review: every formula above was re-read against the printed
listings; the full 4696-byte output and return value matching, plus the
829/829 equivalence asserts, are the end-to-end evidence that no field
extraction or traversal step was misassigned. Items not covered by the
committed vector are named in §10 rather than asserted.
