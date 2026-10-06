# FU-94 — reciprocal-table divisor fix and the window→projection feed

Follow-on to FU-93 §3/§8 (open legs around `FUN_000441D8`/`0x4B0E8`) and a
correction pass over FU-88 §3.1. Two results:

1. `FUN_0004C4F0` fills entry `k` with divisor `max(k,10)`, **not**
   `max(k+1,10)` as FU-88 §3.1 states; the ported
   `fifa96_projection_reciprocal` and its pinned expectations are corrected.
2. The reciprocal tables are rebuilt from the **window and camera yaw**:
   `FUN_00043E48` → `FUN_000441D8` (yaw + width → tan-scaled ratios
   `0x4B0E0`/`0x4B0E4`) → `FUN_00044394` (→ `0x146B0`/`0x146B4`) →
   `FUN_0004C4F0` (tables `0x74C0`/`0x74C4`).

Result in one line: **the perspective divide tables are built per view from
`(window_dim << 16) / max(k,10)`, and the two window dimensions are
themselves derived from the camera yaw via `tan`-scaled half/`5/12` widths.**

## 1. The divisor — `FUN_0004C4F0`

Second loop, `disassemble_bytes 0x4C4F0`:

```
0x4C53B  EBX = 0xA                       ; divisor
0x4C540  ECX = 0x28                      ; byte offset = index 10
0x4C545  EDX = [0x146B0]; SHL EDX,0x10; EAX = EDX; SAR EDX,0x1F
0x4C54F  IDIV EBX                        ; (W<<16) / 10
0x4C553  [[0x74C0] + ECX] = EAX          ; tableX[10] = /10
0x4C562  ECX += 4
0x4C56B  INC EBX                         ; divisor 11
0x4C56C  [[0x74C4] + ECX - 4] = EAX      ; tableY[10] = /10
```

so index `k` uses divisor `max(k,10)` (entries 0..10 → 10, 11 → 11, …,
1023 → 1023); the first loop only pre-fills indices 0..9 with divisor 10.
`FUN_0004C590` then indexes the tables directly: `EBP = z*4;
IMUL EAX,[tableX + EBP]` (`0x4C5E9..0x4C5F8`) — no `+1`.

**Erratum (FU-88 §3.1)**: the formula `(W<<16)/max(k+1,10)` is off by one
from index 10 on; the correct statement is `(W<<16)/max(k,10)`. The ported
`fifa96_projection_reciprocal` used the wrong divisor
(`d = i + 1; if (d < 10) d = 10`), which made every projected coordinate for
`z' >= 10` slightly small. Corrected values (`dim = 320`):
`table[10] = 0x200000`, `[255] = 82241`, `[256] = 81920`, `[512] = 40960`,
`[1023] = 20500`; `dim = 200`: `[255] = 51400`, `[1023] = 12812`.

## 2. The window→projection feed

### 2.1 `FUN_000441D8` — yaw + width → tan-scaled ratios

Entry (`0x441D8`): `ESI = EAX` (camera yaw), `EDI = EDX` (window dimension),
`EBP = EBX` (cache key third). Cache key `(yaw, EDI, EBP)` at
`0x6798`/`0x679C`/`0x67A0`; cached ratios at `0x67A4`/`0x67A8`.

On a cache miss:

```
0x4421E  FUN_000A1A60(yaw, &s, &c)              ; (s,c) = sincos, 16.16
0x44227  if (0 < s < 0x10000):                  ; first/second quadrant
0x4423D    EBX = c
0x44246    EAX = EDI >> 1 ; IMUL (s)            ; (dim/2) * s
0x44250    +0x8000 ; SHRD 16 ; IDIV c           ; t = ((dim/2)*s)>>16, q = t/c, r = t%c
0x44274    [0x4B0E0] = (q<<16) + (r<<16)/c      ; 16.16 ratio, low bits rounded at 0x44279
0x44284    EAX = 4; CALL 0x1D940
0x4428E    if (nonzero) → skip the second ratio (0x4B0E4 keeps ratio1)
0x44299    EAX = (10*EDI)/12; SAR 1             ; floor(5*dim/12)
0x442BA    IMUL (s) ; +0x8000 ; SHRD 16 ; IDIV c
0x442E0    [0x4B0E4] = …                        ; second ratio
0x442F0..  refresh the cache words
```

Both ratios are `dim' * tan(yaw-ish)` in 16.16 with the two-step
divide/remainder sequence; the second uses `5*dim/12` instead of `dim/2`.
The `FUN_0001D940(4)` result suppresses the second ratio (open leg).

The call site `0x43F1E` passes `EAX = [0x4B0E8]`, `EDX = src_w`,
`EBX = src_h`, `ECX = src_h + src_y`, and `FUN_000441D8` writes
`[0x4B0E8] = ESI` (`0x44248`), so **`0x4B0E8` is the camera yaw angle**
(cached from the previous call).

### 2.2 `FUN_00044320` / `FUN_00044344` / `FUN_00044394`

* `FUN_00044320` (2 insns) returns `[0x4B0E0]`; the sibling at `0x4436C`
  returns `[0x4B0E4]`.
* `FUN_00044344(v)` = `(v * [0x4B0E0] + 0x8000) >> 16` — the ratio as a
  multiply helper.
* `FUN_00044394`: if `([0x67AC],[0x67B0]) != ([0x4B0E0],[0x4B0E4])`, stores
  `[0x146B0] = [0x4B0E0]>>16`, `[0x146B4] = [0x4B0E4]>>16` and calls
  `FUN_0004C4F0`. `FUN_000590B0` calls `0x44394` at `0x590BD` before
  projecting (FU-88 §2), so the tables are refreshed per frame from the
  current window/yaw.

### 2.3 Relation to the port

`fifa96_projection_reciprocal` is the ported `FUN_0004C4F0` (with the §1
fix); `[0x146B0]`/`[0x146B4]` are the `dim` arguments, `[0x146A8]`/`[0x146AC]`
the centers from `FUN_00043E48` (FU-88 §3.1). The window therefore sets both
the projection center and the reciprocal numerators.

## 3. Errata

* **FU-88 §3.1** — divisor formula, see §1.
* **FU-93 §3** — `FUN_000A1A60` is not "scaled bitmap dimensions": it is the
  ported `fifa96_projection_sincos` (angle → `(sin,cos)`; the 257-entry
  `0x14E04` table plus the 6-bit first-order correction `((a&0x3F)*0x6487E)>>9`,
  FU-88 §1.1). `FUN_000CE3B0` is its table fold (10-bit angle quadrant
  lookup, popcount-parity sign selection over the tables `0x14E04`/`0x15204`
  /`0x14A04`/`0x15604`/`0x15E04`). `0x4B0E8` is a **camera yaw angle**, not
  a bitmap id; `FUN_000441D8` computes tan-scaled view ratios for the
  projection tables.
* **FU-93 §8 leg 3** (`0x4B0E8` producer) — closed: written by
  `FUN_000441D8` from its `EAX` argument (the previous yaw), first value 0.

## 4. Port

`src/fifa96_loader/fifa96_projection.c`: `fifa96_projection_reciprocal`
divisor corrected to `i < 10 ? 10 : i`. `tests/test_projection.c`: the
pinned reciprocal values and the five projected outputs that hit `z' >= 10`
were updated (values listed in §1 and in the test). No API change; the
`test_scene` slot-project case derives its expectation from
`fifa96_projection_screen`, so it follows the fix.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x4C4F0, 0x4C590,
0x441D8, 0x44320, 0x44344, 0x44394, 0xA1A60, 0xCE3B0, 0x43E48;
`get_xrefs_to` 0x4B0E0, 0x4B0E4, 0x4B0E8; `disassemble_bytes` 0x441D8
(104 B), 0x4C4F0 (128 B), 0x4C590 (192 B). Analysis-only outside the port;
no capture-rig, ISO, or Ghidra-project change. Port write set:
`src/fifa96_loader/fifa96_projection.c`, `tests/test_projection.c`.

## 6. Open legs

1. **`FUN_0001D940(4)`**: its nonzero return suppresses the second ratio
   (`0x4B0E4 = ratio1`); the check's meaning (input/mode/debug) is not
   derived.
2. **`5*dim/12`**: why the second ratio uses 5/12 of the window width
   (a second screen line?) is not evidenced.
3. **Ratio consumers beyond the table build**: who calls `FUN_00044344` and
   `0x4436C` (and for which screen coordinate) is not enumerated.
4. **`FUN_000441D8`'s `EBP`/third cache key argument** (`EBX` at the call
   site = `src_h`): only the cache identity is proven.
