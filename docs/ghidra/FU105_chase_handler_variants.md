# FU-105 — the chase-camera handler variants (classes 1 and 2)

Follow-on to FU-100 §3 (the class 1/2 bodies left open), FU-102 §3 (the
runner passes `EDX` = the type descriptor) and FU-104 §2 (the C-record
fields). This slice decompiles `FUN_0004e3a8` and `FUN_0004ec9c` and shows all
three chase handlers share one skeleton, differing in the descriptor fields
they read and the state they write.

Result in one line: **all three handlers (`FUN_0004e834`, `FUN_0004e3a8`,
`FUN_0004ec9c`) integrate the z step `FUN_0004e248(...)` into the descriptor
`+0x34/+0x38/+0x3C`, move the camera through `FUN_0004d698`, slew yaw/pitch
and copy `+0x40..+0x48`; class 1 mirrors class 0 with the inverted
`desc[2] == 1` test and its own scale constants (`0xE3B`/`0xAD3` at
`0x4E4DC`/`0x4E4D0`), and class 2 adds a `sincos(desc[3])` C-record rescale
and writes the descriptor behavior state `desc[2] = 3/4`.**

## 1. The shared skeleton

`param_2` is the type descriptor (FU-102 §3), `in_EAX` the camera, `param_1`
an integer fed to the step helper. Every handler runs:

```
uVar3 = FUN_0004e248(param_2[0x10], param_1, cam[+0x3c]);   // z step
param_2[0xe] = uVar3;
param_2[0xd] += uVar3;                       // target-z offset,
  clamped to ±param_2[0xf]                   //   its own bound,
  clamped to cam[+0x3c] ± 0xB10;             //   and the camera window
... per-class yaw-sector correction ...
scale = FUN_000a2ad8(-0xb, *param_2) >> 3 + C1  ; C1 = 0xE09 (class 0) / 0xE3B (1,2)
scale2 = FUN_000a2ad8(-0xd, *param_2) >> 3 + C2 ; C2 = 0xAA1 (class 0) / 0xAD3 (1,2)
if (desc[2] == X) { if (cam[+0x3c] >= 0) FUN_000a2ad8(scale - scale2,  cam[+0x3c]); }
else              { if (cam[+0x3c] <  0) FUN_000a2ad8(scale - scale2, -cam[+0x3c]); }
cam[+0x60] = FUN_0004df34(cam[+0x4c]);
cam[+0x58] = (cam[+0x58] + cam[+0x60]) & 0xFFFF;            // yaw slew
... yaw clamp through FUN_0004c7d0 and the quadrant thresholds ...
cam[+0x64] = func_0x4e05c(...);
cam[+0x5c] = (cam[+0x5c] + cam[+0x64]) & 0xFFFF;            // pitch slew
... x/z through FUN_0004d698, y and angle eased 16.16 through FUN_0004d668 ...
cam[+0x40/+0x44/+0x48] = cam[+0x34/+0x38/+0x3c];            // tail copy
```

The scale scratch slots differ per class: class 0 writes `0x4E4D8`/`0x4E4D4`
(class 0 used `0xE09`/`0xAA1`), class 1 `0x4E4DC`/`0x4E4D0` and class 2
`0x4E4D8`/`0x4E4D4` (both `0xE3B`/`0xAD3`).

## 2. `FUN_0004e3a8` — class 1

Same head as class 0 (`PUSH ESI/EDI/EBP; SUB ESP,0x18; ESI=EAX; EDI=EDX;
EDX=ECX`; early-out on `param_2 == NULL` to `0x4E82C`), with two differences:

* the z integration gains an extra proportional term toward the window edge
  (`FUN_000a2ad8(iVar4 - *piVar1, cam[+0x34])` → `FUN_000a2aee(...)`) whose
  result is not stored by the decompiler (open leg: register-held store);
* the descriptor test is **`param_2[2] == 1`** (inverted polarity versus
  class 0's `== 3`), with the `∓0x361`/`+0x35f` sign tests on `cam[+0x34]`
  and the yaw quadrant `[0x4000, 0xC000)` deciding whether `FUN_0004c7d0()`
  ± `cam[+0x4c]` is the reference.

The y target is `param_2[4] = [param_2[0x14] + 0x10]`; the rest of the tail is
the shared skeleton.

## 3. `FUN_0004ec9c` — class 2

The distinctive head (before the shared skeleton):

```
FUN_000a1a60(param_2[3], &local_18, &local_1c);      // sincos(desc[3])
[param_2[0x14] + 0x18] = abs([C+0x14] * sin >> 16);
param_2[4]              = abs([C+0x14] * cos >> 16);
```

so class 2 rotates a C-record radius (`[desc+0x50]+0x14`) by the descriptor's
third dword — `0xA7F8` (type 2) / `0xA21C` (type 5), the only descriptors with
a nonzero `desc[3]` (FU-100 §5). The shared z integration then runs, and the
yaw sector (`desc[3] < 0x4000 || > 0xC000`, then `< 0x8000`) **writes the
behavior state**:

```
if (desc[3] < 0x4000 || desc[3] > 0xC000) { desc[2] = 4; ... } else { desc[2] = 3; ... }
```

which explains the static `desc[2]` values (1/3) and class 0's `== 4` branch:
class 2 promotes the descriptor between states 3 and 4. The yaw clamp here
uses the `[0x2000, 0xA000]` band instead of class 1's `[0x4000, 0xC000)`.

## 4. Descriptor fields as handler state

| field | source | use |
|-------|--------|-----|
| `desc[0]` | `FUN_0004F1C8` (variant) | x scale input to `FUN_000a2ad8(-0xb, ·)` |
| `desc[1]` | static (0xFA0/0x1200/0xD48) | target angle for the `FUN_0004d668` ease |
| `desc[2]` | static 1/3; written 3/4 by class 2 | behavior selector (`==1` class 1, `==3`/`==4` class 0) |
| `desc[3]` | static 0 / 0xA7F8 / 0xA21C | class-2 sincos angle and yaw sector |
| `desc[0x0d..0x0f]` | runtime | z-offset integrator (`+0xF` its bound) |
| `desc[0x10]` | runtime | `FUN_0004e248` argument |
| `desc[0x14]` | `FUN_0004F1C8` | C record (`+0x00/+0x04` x/z, `+0x08/+0x0c` bounds, `+0x10` y, `+0x14` radius, `+0x18` scaled) |

## 5. Closures / errata

* **FU-100 §3 — closed.** All three chase handlers are decomposed; the class
  labels hold (`FUN_0004e834` class 0, `FUN_0004e3a8` class 1,
  `FUN_0004ec9c` class 2).
* **FU-100 §3/§8 leg 8 — refined.** The handler common helpers are
  `FUN_0004e248`, `FUN_0004d698`, `FUN_0004df34`, `FUN_0004c7d0`,
  `FUN_0004d668`, `func_0x4e05c`, `FUN_000a2ad8`/`FUN_000a2aee`; the four
  scale scratch globals are `0x4E4D0..0x4E4DC`.
* **FU-100 §3 "tracked entity" — corrected.** `param_2[0x14]` is the C
  record, so the x/z bounds and the rotated radius are *descriptor*
  parameters, not per-entity state.
* **FU-104 §2 — confirmed.** `[C+0x10]` is the y goal class 1 uses; class 2
  writes `[C+0x18]` from the radius.

## 6. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x4E3A8, 0x4EC9C
(both resolved in-session; `create_function` reported "already exists", so no
function was created and no project change was made), 0x4E834 (FU-100),
0x4DDA8/0x4D98C (FU-104); `disassemble_bytes` 0x4E3A8 (224 B, head window).
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 7. Open legs

1. **Class 1's discarded proportional term** (`FUN_000a2ad8`/`FUN_000a2aee`
   result not stored in the decompile) — which camera/descriptor field it
   feeds.
2. **`func_0x4e05c`** (pitch integrator, called by all three) and
   `FUN_0004d668` (ease factor, also used by the transition path).
3. **The `desc[3]` semantics** — a yaw angle (0xA7F8 ≈ 235°, 0xA21C ≈ 227°)
   used as a sincos input *and* an eight-sector selector.
4. **The `desc[2] = 3/4` flip** in class 2 versus the static 1/3 values —
   when the type-1/4 descriptors (state 1) switch to the other handlers.
