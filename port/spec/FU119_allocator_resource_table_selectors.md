# FU-119 — closeout: the ring allocator's real signature, the resource table and two write-only globals

Follow-on to FU-114 §6 leg 1 (`FUN_00063ebc`'s caller), FU-116 §6 leg 4 (the
`[0x4BFC0]` codec chain) and FU-118 §5 legs 2–3 (`[0x8E1C]`). This slice
records three bounded results and two honest negatives.

Result in one line: **`FUN_00063ebc` allocates the fixed ring
(`FUN_0004a448(EAX=0x20DC, EDX=0xA41C, EBX=0x20)` — the "size parameter" FU-108
quoted was the constant `0xA41C = 0x96*0x118 + 0xC`); `[0x4BFC0]` is the
resource-handle table with getter `FUN_0004afb8` (11 callers) whose filler is
not statically located; and `[0x8E1C]`/`[0x55C84]` are write-only globals
chosen from `FUN_000566d0()` (a 160-bit counter) at enumerated scope.**

## 1. `FUN_00063ebc` — the real allocator call

Disassembly of the alloc arm:

```
0x63EBF TEST EAX,EAX
0x63EC1 JZ  0x63F01                        ; EAX==0 -> free path
0x63EC3 MOV EBX,0x20                       ; 3rd arg
0x63EC8 MOV EDX,0xA41C                     ; 2nd arg = size
0x63ECD MOV EAX,0x20DC                     ; 1st arg (tag/type)
0x63ED2 CALL 0x4A448
0x63ED7 MOV [0x9AB0],EAX
... lays out [0x9AB8]=+0, [0x9ABC]=+4, [0x9AC0]=+8, [0x9AB4]=+0xC
```

so the allocator signature is `FUN_0004a448(EAX, EDX, EBX)` and the ring size
is the **constant** `0xA41C` = `0x96 * 0x118 + 0xC` (150 records × 0x118 B +
12 B header) — FU-108 §1's `FUN_00063ebc(size)` and `FUN_0004a448(size,
&0xA41C)` were decompiler renderings of the three register arguments
(`0x20DC`, `0xA41C`, `0x20`). The free arm is unchanged (FU-108 §1).

`search_byte_patterns` for `bc 3e 06 00` and `search_instructions` for
`63ebc` still find **no** reference to the function; `get_xrefs_to` is empty
(FU-114 §3). The allocator's entry remains indirect/loader-side (open).

## 2. The `[0x4BFC0]` resource-handle table

* `FUN_0004afb8(EAX = index) { return [0x4BFC0 + index*4]; }` — the getter;
  callers: `FUN_000379b8`, `FUN_000395cc`, `FUN_0004808c`, `FUN_00048104`,
  `FUN_00048b60`, `FUN_00048ed8`, `FUN_0004c384`, `FUN_00058e00`, `FUN_0005ff24`,
  `FUN_00060400`, `FUN_00078f1c`.
* `FUN_0004a830(0)` clears entries at `0x4BFC0 + 0xFC .. 0x118` (eight dwords,
  FU-114 §2).
* The HUD decode chain (FU-116 §3) indexes the same table via `FUN_0004afc0`
  and feeds `decode_size_probe` / `FUN_00098c38` / `decode_record_strict`
  (the named ported codec entry points at `0x9E890`, `0x9E3xx`, `0x9E860`).
* No write with an absolute `0x4BFC0` operand exists besides the clear, so the
  **filler is register-based or indirect** — not statically located at slice
  time (negative; the getter and clear are the attributable facts).

## 3. The write-only selector globals

* `[0x8E1C]` is written only by `FUN_000513bc` (`0x1D` when its input is 1 or
  `FUN_000566d0() & 1`, else `0x1C`); `search_instructions 8e1c` finds no
  reader. So the 0x1C/0x1D screen variant is consumed by an indirect handler.
* `FUN_000566d0` is a 160-bit counter: five 16-bit limbs at `0x8FD0..0x8FE4`
  incremented with carry propagation (and a final increment of `[0x8FD0]` on
  the all-zero-limb event). It is the `& 1` source for the selector above —
  a timer/random-driven idle variant.
* `[0x55C84]` (the HUD button setter `FUN_00064664`, FU-111 §5) is likewise
  write-only at enumerated scope (FU-116 §3).

## 4. Closures / errata

* **FU-108 §1 / FU-109 §3 — corrected.** The ring allocator takes no size
  parameter: the size is the constant `0xA41C`, passed as `EDX`;
  `FUN_0004a448(0x20DC, 0xA41C, 0x20)`.
* **FU-114 §3 — unchanged.** `FUN_00063ebc` still has no static caller.
* **FU-116 §6 leg 4 — refined.** `[0x4BFC0]` is a resource-handle table with
  a named getter and 11 consumers; only its filler is missing.
* **FU-118 §5 leg 2 — answered negatively.** `[0x8E1C]` has no static reader;
  `FUN_000566d0` is a 160-bit counter, not a flag source.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_function` 0x63EBC (45 insns,
body `0x63EBC..0x63F4C`); `decompile_function` 0x4AFB8, 0x4A6BC, 0x566D0;
`get_function_callers` 0x4AFB8 (11), 0x4A6BC (0), 0x513BC (0);
`search_instructions` `a41c` (3), `9ab0` (7), `4bfc0` (2), `8e1c` (4);
`search_byte_patterns` `bc 3e 06 00` (earlier, 0). Address mapping as FU-88.

## 6. Open legs

1. `FUN_00063ebc`'s entry (indirect/loader-side) — unchanged.
2. The `[0x4BFC0]` filler (register-based/indirect).
3. The consumers of `[0x8E1C]` (0x1C/0x1D) and of `[0x55C84]`.
4. `FUN_000566d0`'s limbs' writer/seed (`0x8FD0..0x8FE4`).
