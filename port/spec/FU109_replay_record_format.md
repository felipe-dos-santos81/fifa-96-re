# FU-109 — the replay record format and the pack/apply pair

Follow-on to FU-108 §6 leg 1 (the `FUN_00063b80` roles). The call-site
disassembly resolves the ambiguity: `FUN_0006408c` is the **packer**
(live state → ring record) and `FUN_00063b80` is the **apply**
(ring record → live state). This slice quotes the record map.

Result in one line: **`FUN_0006408c` appends a `0x118`-byte record at the
write cursor `*[0x9abc]` from the live block `0x55AA4`, stamping the tick
`[0x9aac]` at `+0x116`; `FUN_00063b80(EAX = 0x55AA4, EDX = ring slot)` applies
a record back into the live block, which is what the playback drivers
`FUN_00063cbc`/`FUN_00063e54` call; the cursors are `*[0x9abc]` (write),
`*[0x9ab8]` (read), `*[0x9ac0]` (count) and `[0x9a94]` (play index).**

## 1. The pair, by call site

`FUN_00063cbc` and `FUN_00063e54` both end their slot computation with:

```
0x63cfa MOV EAX,0x55aa4                  ; live replay block
0x63cff CALL 0x00063b80                  ; EDX = [0x9ab4] + index*0x118
```

and the decompiled `FUN_00063b80` copies **from `param_2` (EDX) to `in_EAX`
(EAX)** (first statement `*in_EAX = *param_2`, block moves `param_2+0xf6 →
in_EAX+0x1ae`, `param_2+0x10e → in_EAX+0x1c6`). So `FUN_00063b80` is the
**record → `0x55AA4` apply**, and the playback drivers call it after moving
the play index `[0x9a94]`.

`FUN_0006408c(EAX = ticks)` is the mirror image: gated by `[0x9a90]` and
`[0x9ab4] != 0`, it accumulates `[0x9aac] += ticks` (min 1 when `ticks == 0`),
writes a record at the **write cursor** `[0x9ab4] + *[0x9abc]*0x118`, then
advances `*[0x9abc]` (wrap at `0x95`, i.e. mod `0x96`), `*[0x9ac0]++`, and
resets `[0x9aac] = 0`.

So the ring is a two-cursor structure: write `*[0x9abc]`, read
`([0x9a94] + *[0x9ab8]) mod 0x96`, count `*[0x9ac0]` (freed by
`*[0x9ab8]++` and `*[0x9ac0]--` when the count exceeds `0x95`).

## 2. The record map (from the packer)

`FUN_0006408c` writes (`puVar2` = the record, source = `0x55AA4` and the
`0x55C4C..` cluster):

| record | source | value |
|--------|--------|-------|
| `+0x00/+0x04/+0x08` | `[0x55AA4]/[0x55AA8]/[0x55AAC]` | first three dwords |
| `+0x0C`, `+0x3A`, `+0x68`, `+0x96` | words via `+0x0A`, `+0x14`, `+0x10`, `+0x120`-stride loop | per-entity words |
| `+0x31`, `+0xDB` | `+0x5F`, `+0x193` byte lanes | per-entity bytes |
| `+0xF2/+0xF3` | `[0x55C4C]` bytes 2/3 | camera state |
| `+0xF4` | `[0x55C50]` byte | |
| `+0xF5` | `[0x55C51]` byte | |
| `+0xF6..+0x10D` | six dwords at `[0x55C52]` | camera matrix |
| `+0x114` | `[0x55C70]` word | |
| `+0x116` | `[0x9AAC]` word | the tick stamp (FU-108 §2) |
| `+0x10E..+0x113` | `memmove` from `0x55C6A`, 6 bytes | |
| `+0x0C..+0x2E` | the word loop | 16-bit high-word pass |

The apply (`FUN_00063b80`) is the inverse subset: first three dwords, bytes
`+0xF2/+0xF3/+0xF4/+0xF5`, six dwords `+0xF6`, the `+0x116` word, the six
bytes `+0x10E`, and the same word loop (`+0x0A/+0x66/+0x96/+0x31/+0xDB`
source lanes).

## 3. The lifecycle helpers

| function | effect |
|----------|--------|
| `FUN_00063ebc(size)` | alloc (`FUN_0004a448(size, &0xA41C)`) / free (`FUN_000993ec`); lays out `[0x9ab8]=base`, `[0x9abc]=+4`, `[0x9ac0]=+8`, `[0x9ab4]=+0xC` (FU-108 §1) |
| `FUN_00064030` | resets `*[0x9ab8]`, `*[0x9abc]`, `[0x9a94]`, `[0x9aac]`, `*[0x9ac0]` |
| `FUN_00064074`/`FUN_00064080` | `[0x9a90] = 0` / `1` — the recorder enable |
| `FUN_0006408c` | the gated packer (§1) |

## 4. The `[0x9a98]` readers (the mode predicates)

```
FUN_00063fd0() { return [0x9a98] & 0x80; }                 ; special/replay mode
FUN_00063fdc() { return [0x9a98] == 0x81; }                ; the intro arm
FUN_00063ff0() { return ([0x9a98] & 0x80) && FUN_00053d7c() == 0; }
FUN_00064ec0() { if (!([0x9a98] & 0x80)) { if (FUN_00037ae4() == 0) FUN_00064ef0(); } }
```

so bit 0x80 groups modes `0x80..0x87` (the replay/photo family of
FU-106/FU-107) and `FUN_00064ec0` is the "not in replay" per-frame hook.

## 5. Closures / errata

* **FU-108 §6 leg 1 — closed, with correction.** `FUN_00063b80` is the
  **apply** (record → `0x55AA4`), not the pack; the packer is `FUN_0006408c`
  (the FU-108 text is corrected by this section; its record offsets stand).
* **FU-108 §6 leg 4 — closed.** `*[0x9ab8]` is the read cursor (advanced by
  the recorder's eviction), `*[0x9abc]` the write cursor.
* **FU-106/FU-107 mode family — refined.** `[0x9a98] & 0x80` is the family
  bit; `FUN_00063ff0`/`FUN_00064ec0` are the two mode-aware per-frame hooks.
* **FU-107 §7 leg 1 — partially closed.** `FUN_00064ef0` is the
  not-in-replay frame body (still open).

## 6. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_function` 0x63CBC (38 insns),
0x63E54 (31); `decompile_function` 0x63B80, 0x63FD0, 0x63FDC, 0x63FF0,
0x64EC0, 0x64030, 0x64074, 0x64080, 0x6408C. Address mapping as FU-88.
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 7. Open legs

1. **The `0x55AA4` live block** field map (`+0x00..+0x1CE`, the per-entity
   word loop's `0x120`/`0x96` strides) and its writer(s) during live play.
2. **`FUN_0004a448`/`&0xA41C`** (allocator descriptor) and `FUN_000993ec`.
3. **`FUN_00064EF0`** (the not-in-replay frame body) and
   `FUN_00053D7C`/`FUN_00037AE4` (the mode companions).
4. **The eviction path** (`*[0x9ab8]++`/`*[0x9ac0]--` when `> 0x95`) — the
   producer that fills the ring before the recorder runs.
5. **The `+0x0C..+0x2E` word loop's exact per-lane mapping** (the packer and
   applier iterate it; the stride expression `puVar1 += 2`/`puVar1 += 1`
   mixes word and byte lanes).
