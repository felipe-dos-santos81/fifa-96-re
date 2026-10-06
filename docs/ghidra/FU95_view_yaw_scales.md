# FU-95 — view yaw → window projection scales (chain close-out)

Follow-on to FU-94 (open legs 1, 3, 4) and FU-93 §3. The window ratios
`0x4B0E0`/`0x4B0E4` are computed per frame from the **view object's yaw** and
the saved window dimensions, gated by settings index 4; the consumers scale
directional sprites. This closes the "who feeds `FUN_000441D8`" question and
corrects FU-88's "integer screen width" reading of `[0x146B0]`.

Result in one line: **`yaw = [[0x7314]+0x14]` → `FUN_00043E30(yaw)` →
`FUN_000441D8(yaw, window src_w, window src_h)` → ratios
`(dim/2)·sin/cos` and `(5·dim/12)·sin/cos`, which
`FUN_000590B0`/`FUN_00044394` push into `[0x146B0]`/`[0x146B4]` and
`FUN_0004C4F0` expands into the per-depth reciprocal tables.**

## 1. The call chain

`FUN_00049830` (live match update, FU-89 §1), after rendering:

```
0x49897  CALL 0x4C904 → [0x7314]          ; view record = &0x4E4E0
0x498A6  CALL 0x43600 → [0x7318]          ; window record (FU-92)
0x498AB  MOV EAX,[EBX+0x14]               ; EBX = EDX = view record
0x498AE  MOV EDX,EBX
0x498B0  CALL 0x43E30                     ; FUN_00043E30(yaw = view[0x14])
```

`FUN_0004C904` copies the view record from `[0x7DC8]`:
`0x4E4E0 ← [0x7DC8+0x10]`, `0x4E4E4 ← +0x14`, `0x4E4E8 ← +0x18`,
`0x4E4EC ← +0x58`, `0x4E4F0 ← +0x5C`, `_0x4E4F4 ← +0x4C`. The window call
site reads `[view+0x14]` = `0x4E4F4` = **`[0x7DC8+0x4C]`**: the yaw is the view
object's field `0x4C`.

`FUN_00043E30` is a 5-insn wrapper:

```
0x43E30  PUSH EBX; PUSH EDX
0x43E32  MOV EBX,[0x4B0D8]                ; window src_h (+0x34)
0x43E38  MOV EDX,[0x4B0D4]                ; window src_w (+0x30)
0x43E3E  CALL 0x441D8                     ; FUN_000441D8(EAX = yaw)
```

so `FUN_000441D8(yaw, src_w, src_h)` is the real entry; the call inside
`FUN_00043E48` (`0x43F47 MOV EAX,[0x4B0E8]`) is a cache-echo refresh with the
previous angle (data storage `0x14B0E8`, zero at rest — it does not start a
cycle: the driver is `FUN_00049830`).

`FUN_000590B0` (the entity projection consumer, FU-88 §2) then calls
`FUN_00044394` (`0x590BD`), which pushes the ratios into
`[0x146B0] = ratio1 >> 16`, `[0x146B4] = ratio2 >> 16` and rebuilds the
tables through `FUN_0004C4F0` (FU-94 §1).

## 2. `FUN_000441D8` revisited

* Cache key `(yaw, src_w, src_h)` at `0x6798`/`0x679C`/`0x67A0`, cached
  ratios at `0x67A4`/`0x67A8` (`FUN_00044394` compares `0x67AC`/`0x67B0`).
* On a miss, `FUN_000A1A60(yaw, &s, &c)` (`s` = first out = sine, `c` =
  second = cosine; FU-94 §2).
* Gate `if (0 < s < 0x10000)` — the ratio is only computed for a yaw in
  `(0°, 90°)`; otherwise the previous values are kept (cache refreshed).
* `ratio1 = ((src_w/2)·s)>>16 / c` (two-step divide + truncate to whole
  units, `0x44241..0x44284`), then `FUN_0001D940(4)` — **settings index 4**,
  default 0 (`FUN_0001DE94`), i.e. the second ratio is computed by default:
* `ratio2 = ((floor(5·src_w/12)·s)>>16 / c` (`0x44299..0x442E8`); when
  settings[4] != 0 the store falls back to `ratio1` (`0x44292 → 0x442EB`).
* `src_h` participates only in the cache key: both ratios derive from the
  window **width** and the yaw.

## 3. Consumers of the ratios

`FUN_00044320` / `0x4436C` return the two ratios; `FUN_00044344(v)` is
`(v·ratio + 0x8000)>>16`. Callers:

* `FUN_00057278` (with `FUN_00044344`/`0x4436C`): computes scaled dimensions
  `w' = ratio1·rec[8]`, `h' = ratio2·rec[8]`, writes a drawable record
  (`+8 = rec[2]>>17`, `+10 = rec[6]`) and calls the scaled blitter
  `FUN_0009B664(w'>>16, h'>>16, record, x>>16, y>>16)` — a directional
  sprite draw whose size is the view scale.
* `FUN_0005AEB8`: `atan2` of two points via `0xA2A10`, `sin` magnitude via
  `FUN_000A60A0`, blends `(0x10000-|sin|)·base + |sin|·value`, then picks
  `0x4436C` (ratio2) when `|dy| > |dx|` else `FUN_00044344` (ratio1) and
  returns the product — an **orientation-dependent aspect scale**.
* `FUN_00062120` (six calls), `FUN_000629CC` (two), `FUN_00062EBC` — the
  `0x62xxx` camera/quad band.

## 4. Errata

* **FU-88 §3.1** — `[0x146B0]`/`[0x146B4]` are not stored screen dimensions:
  they are the view-scale numerators written per frame by `FUN_00044394`
  from `FUN_000441D8`'s ratios, i.e. `floor((src_w/2)·sin(yaw)/cos(yaw))`
  and the 5/12-width variant. Their consumer (`FUN_0004C4F0`) and its
  corrected divisor are FU-94 §1.
* **FU-94 §6 legs 1/3/4** — the gate is settings index 4 (default 0 →
  ratio2 computed); the second cache key argument is the window `src_h`
  (`EBX` at the `FUN_00043E48` call, `[0x4B0D8]` at `FUN_00043E30`); the
  ratio consumers are enumerated in §3.
* **FU-93 §3** — carried: `0x4B0E8` is the cached yaw; `FUN_000A1A60` is the
  ported `fifa96_projection_sincos`.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_bytes` 0x49800 (192 B),
0x43E30 (24 B), 0x441D8 (104 B), 0x44240 (256 B); `decompile_function`
0x1D940, 0x4C904, 0x43E30, 0x57278, 0x5AEB8, 0x4C4F0, 0x44394;
`get_function_callers` 0x441D8 (`FUN_00043E30`, `FUN_00043E48`,
`FUN_000443E8`), 0x43E30 (`FUN_00049830`), 0x44394 (`FUN_000590B0`);
`get_xrefs_to` 0x4B0E0, 0x4B0E4, 0x4B0E8, 0x4F28; `read_memory` 0x14B0E0
(48 B, zero at rest), 0x106798 (24 B, zero), 0x105344 (descriptor table),
0x1052B4/0x1052CC (option arrays). Analysis-only: no port, capture-rig, ISO,
or Ghidra-project change.

## 6. Open legs

1. **Ratio geometry**: why the two reference widths are `src_w/2` and
   `floor(5·src_w/12)`, and how the resulting `[0x146B0]`/`[0x146B4]`
   numerators relate to the screen coordinates (`FUN_000590B0`'s use).
2. **View field `+0x4C`** (`[0x7DC8]`): the producer of the view yaw and its
   units.
3. **Settings index 4**: descriptor `opts`/`label` pointers resolve to
   object-1 code (`0x100D2`: `MOV [0x4F28],EAX; RET`) — the option-handler
   semantics are not derived.
4. **`FUN_0009B664`** scaled blitter internals (the magnification/scale path
   itself) remain unported; the `0x9Bxxx` sprite family is FU-86 territory.
