# FU-104 — the slot-4/slot-8 camera arms and the block/bounds data

Follow-on to FU-102 §6 leg 1 (`0x4D98C`/`0x4DDA8` bodies), FU-102 §6 leg 4
(`[0x7DF0]`), and FU-97 §8 leg 3 (the `0x83CC`/`0x84BC` side blocks). This
slice decomposes the two remaining runner arms and reads the camera block
tables and bounds they use.

Result in one line: **the slot-8 arm `FUN_0004DDA8(EAX=camera, EDX=index)`
places the camera from the nine six-dword blocks at object-4 `0x866C`
(index 7/8 share entry 7 with a yaw flip, pitch clamped to `0x1324..8000`);
the slot-4 arm `FUN_0004D98C` slerps yaw/pitch with the `0x780000` RNG rate
and, when `cam[+0x00] == 4`, clamps x/y/z to the bounds at `0x7E14..0x7E28`
(`x ±0x450`, `y [0x60, 0x3C0]`, `z ±0x7F0`); `0x80B4` is exactly FU-99 array-A
preset 7, `0x872C` is `0x866C` entry 8, and the `0x83CC`/`0x84BC` side blocks
are byte-identical read-only constants.**

## 1. `FUN_0004DDA8` — the slot-8 placement arm

`FUN_0004DDA8(EAX = camera, EDX = param_2)` for `0 <= param_2 < 9` copies one
six-dword block into `cam[+0x10/+0x14/+0x18/+0x58/+0x5c/+0x4c]`:

* `param_2 ∈ 0..6` and `7`: `block = 0x866C + param_2*0x18`;
* `param_2 == 8`: the literal entry-7 block (`0x8714..0x8728`) with
  `cam[+0x18] = -cam[+0x18]` and `cam[+0x58] = 0x18000 - cam[+0x58] & 0xFFFF`
  (the same flip pair as FU-97 §4/FU-99 §2);
* for `param_2 == 7 || 8`: pitch is reset to
  `FUN_0004c77c() + 400`, clamped to `[0x1324, 8000]`; else
  `FUN_00036b34()`;
* every path scales `cam[+0x4c]` by 3/4 around `FUN_00036acc()`
  (`iVar2*3 >> 2`), and the 7/8 path clamps yaw via the `0x16A8` gate
  (`+ 0xAD0` for 8, `- 30000` for 7).

The block table `read_memory 0x10866C` (216 B, object-4 `0x866C`, 9 × `0x18`):

| idx | x | y | z | yaw | pitch | angle |
|-----|----|----|----|-----|-------|-------|
| 0 | 3 | 2800 | 5000 | 32828 | 6400 | 5376 |
| 1 | -3408 | 4816 | 3 | 49152 | 9472 | 5376 |
| 2 | -3300 | 1800 | 673 | 49152 | 7000 | 6608 |
| 3 | -3196 | 996 | 673 | 49152 | 2500 | 6608 |
| 4 | -1 | 932 | -2287 | 32768 | 0 | 3896 |
| 5 | -15 | 300 | 3650 | 32768 | 2096 | 3400 |
| 6 | -15 | 300 | -3650 | 0 | 2096 | 3400 |
| 7 | -66 | 4300 | 6700 | 32768 | 5900 | 1696 |
| 8 | -800 | 924 | -3000 | **63000 (0xF618)** | 3000 | 4608 |

Entry 8 is `0x872C` — the start block `FUN_0004CC98` uses (FU-99 §2); its yaw
is the `0xF618` constant also seen in the class-3/slot-4 pitch clamps.

## 2. `FUN_0004D98C` — the slot-4 settle arm

`FUN_0004D98C(EAX = camera)` (called from the runner slot 4):

* yaw/pitch targets from `FUN_0004c7d0`/`FUN_0004c77c(cam+0x10)`, pitch
  clamped to `[0x1800, 0xF618]`-style bands;
* yaw and pitch each slerp toward the target with
  `FUN_00049388()`-modulated 16.16 rates (mod `0x780000`; the class-3
  handler used `0x1E0000`/`0x3C0000`), then zero `+0x5a`/`+0x5e`;
* copies `cam[+0x40/+0x44/+0x48] = cam[+0x34/+0x38/+0x3C]` (the standard
  tail);
* **when `cam[+0x00] == 4`** clamps x/y/z (dwords 4/5/6) to the six bounds
  at `0x7E14..0x7E28` (`read_memory 0x107E14`, 24 B):

| bound | value | use |
|-------|-------|-----|
| `0x7E14` | -0x450 | x min |
| `0x7E18` | 0x60 | y min |
| `0x7E1C` | -0x7F0 | z min |
| `0x7E20` | 0x450 | x max |
| `0x7E24` | 0x3C0 | y max |
| `0x7E28` | 0x7F0 | z max |

This also explains FU-101 §2's `0x7E0C` cluster: dwords 0/1 are the variant
parameters (`0x340`/`0xB4`), dwords 2..7 are these six bounds — entries 2..7
are not selector variants (FU-101 §8 leg 8 is closed).

## 3. The block data cluster

* `0x80B4` (`read_memory 0x1080B4`, 24 B) =
  `{-347, 380, 1325, 62621, 3852, 4608}` — **exactly FU-99 array-A preset 7**;
  read only by `FUN_0004CC98` (`0x4FBB8`/`0x4FBBD`).
* `0x81A4` = `{-83, 320, 3631, 33732, 2203, 3712}` (the other side of the
  `FUN_0004B7D0`/`FUN_0004B6FC` pair in `FUN_0004CC98`).
* `0x83CC` = `{-755, 120, 295, 45425, 0, 4608}` and `0x84BC` is
  **byte-identical**; `get_xrefs_to` shows both are read-only (readers
  `FUN_0004CAEC`, `FUN_0004CBA0`, `FUN_0004FC2C`, `FUN_000505D0`) with no
  writer. So the "home/away" selector of FU-97 §4/FU-100 §5 has no static
  data difference in this release image — the side selection is inert in the
  file (open leg: a runtime writer would have to exist outside the enumerated
  sweep).
* `0x875C` (216 B at `0x10875C`) is the nine-block random table of
  `FUN_0004FDA4` (first entry quoted in FU-101 §4).
* `0x866C+0x18*8 = 0x872C` (§1) and `0x8714` (entry 7) are the two literals
  `FUN_0004CC98` uses.

`[0x7DF0]` (the slot-8 arm's `EDX` source, `0x4D97C`) is static 0
(`read_memory 0x107DF0`); its writer is open.

## 4. Closures / errata

* **FU-102 §6 leg 1 — closed.** Both arms are decomposed: slot 8 = block
  placement (`0x866C`), slot 4 = settle + bounds clamp.
* **FU-102 §6 leg 4 — partially closed.** `[0x7DF0]` is static 0; the arm
  degenerates to `FUN_0004DDA8(camera, 0)` until a writer appears.
* **FU-97 §8 leg 3 / FU-100 §5 — clarified.** The `0x83CC`/`0x84BC` side
  blocks are identical read-only data; the selection between them cannot
  change behavior in this image.
* **FU-101 §8 leg 8 — closed.** `0x7E0C` entries 2..7 are the position
  bounds; only entries 0/1 are variant parameters.
* **FU-99 chain — confirmed.** `0x80B4` (the cinematic target block) is
  array-A preset 7, so `FUN_0004CC98` animates toward the cinematic preset.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x4DDA8, 0x4D98C;
`read_memory` 0x10866C (216 B), 0x10875C (216 B), 0x1080B4/0x1081A4/
0x1083CC/0x1084BC (24 B each), 0x107DF0 (4 B), 0x107E14 (24 B), 0x107E0C
(32 B, FU-101); `get_xrefs_to` 0x83CC (7), 0x84BC (5), 0x80B4 (2), 0x866C
(0); `get_function_by_address` 0x4D98C/0x4DDA8. Address mapping as FU-88.
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 6. Open legs

1. **`[0x7DF0]`** writer and the slot-8 index source (the runner passes
   `EDX = [0x7DF0]`, not a per-camera field).
2. **The `0x83CC`/`0x84BC` identical-block puzzle**: whether a runtime path
   (or the CD build) differentiates them; the static sweep found no writer.
3. **`FUN_0004c77c`/`FUN_0004c7d0`/`FUN_00036b34`/`FUN_00036acc`/
   `FUN_00063fd0`** cited only (the position/pitch helpers shared with the
   class-3 handler).
4. **Blocks 1..8 of `0x866C` and 1..8 of `0x875C`** are quoted but their
   selection sources (`param_2`, `[0x8BF4]`) are only partly traced.
5. **`cam[+0x00] == 4`** (the bounds-clamp gate) — which path sets dword 0
   to 4 (FU-102 noted the `== 9` marker too).
