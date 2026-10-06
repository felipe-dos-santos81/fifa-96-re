# FU-121 — the active view object `[0x7DC8]`

Follow-on to FU-112 §6 leg 5 (`FUN_0004c904`'s `[0x7DC8]` view object). This
slice names the pointer's targets and the snapshot routine the replay composer
reads through it.

Result in one line: **`[0x7DC8]` is the active view/camera object pointer —
its static bases are the zero-initialised objects at `0x78F8` (default) and
`0x7818` — and the replay composer's six-field camera matrix
(`+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C`, FU-112 §1) is read from whichever
base is active.**

## 1. The pointer and its bases

`search_instructions 7dc8` finds 131 sites in the `0x4C9xx..0x510xx` view
band. The bases appear as immediates:

```
FUN_0004cc98: if ([0x7DC8] != &0x78F8) [0x7DC8] = &0x78F8;
FUN_0004cef4/0x4cf7c/0x4fd50/0x4fda4/0x4ff24: [0x7DC8] = 0x78F8 ...
FUN_0004fe20 / 0x4f2c8:                        [0x7DC8] = 0x7818 ...
```

Both `0x7818` and `0x78F8` are zero in the static image (BSS-like view
records); the pointer is swapped among them and the other view objects by
`FUN_0004ce34`, `FUN_0004d488`, `FUN_0004d498`, `FUN_0004d04c`.

## 2. The view snapshot `FUN_0004cc98`

```
save = [0x7DC8];
if ([0x7DC8] != &0x78F8) [0x7DC8] = &0x78F8;
copy:  +0x10 <- save+0x10;  +0x14 <- save+0x14;  +0x18 <- save+0x18;
       +0x58 <- save+0x58;  +0x5C <- save+0x5C;  +0x4C <- save+0x4C;
tblA = 0x872C[0..5];
tblB = (FUN_0004b7d0() || FUN_0004b6fc() != 0) ? 0x81A4[0..5] : 0x80b4[0..5];
if ([0x9A70] + 8 < 0) { mirror: tblA[3] = (0x18000 - tblA[3]) & 0xffff; tblA[2] = -tblA[2];
                        tblB[2] = -tblB[2]; tblB[3] = (0x18000 - tblB[3]) & 0xffff; }
FUN_0004f8c8(10, tblA, 0xF);
```

So the six fields `FUN_0004c904` copies for the replay record
(`+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C`, FU-112 §1) are the **view object's
camera matrix**, snapshotted from the previous object into `0x78F8`; two
six-dword orientation tables (`0x872C`, and `0x80B4`/`0x81A4` selected by
`FUN_0004b7d0`/`FUN_0004b6fc`) are mirrored around `0x18000` when
`[0x9A70]+8 < 0` (the pitch/roll sign flip) and handed to
`FUN_0004f8c8(10, tblA, 0xF)`.

`FUN_0004ce34` gates the same swap on `FUN_00053d9c()` (the replay/camera
mode predicate, FU-116 §2) and the match state `FUN_00053d50()` (values
`<0x13` or in `0x14..0x1c`/`>0x1d` admitted), re-running
`FUN_0004d04c`/`FUN_0001da58`/`FUN_0004cef4` when the current object is a
`*piVar1 == 7` record.

## 3. Closures / errata

* **FU-112 §6 leg 5 — closed.** `[0x7DC8]` is the active view/camera object
  pointer (bases `0x7818`/`0x78F8`, both BSS), and the replay record's camera
  matrix comes from its `+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C` fields via
  `FUN_0004c904`.
* **FU-116 §6 leg 2 — refined.** `FUN_0004ce34` re-uses the `FUN_00053d9c`
  camera-mode predicate to gate the view swap.

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `search_instructions` `7dc8` (131 sites,
bases `0x7818`/`0x78F8`); `read_memory` 0x7818 / 0x78F8 (48 B each, zero);
`decompile_function` 0x4CC98, 0x4CE34. Address mapping as FU-88.

## 5. Open legs

1. The other `[0x7DC8]` targets swapped in by `FUN_0004d488`/`0x4d498`/
   `0x4d04c` (view-object pool).
2. `FUN_0004f8c8` (the `(10, tblA, 0xF)` blend/setup) and the `0x872c`/
   `0x80b4`/`0x81a4` tables.
3. `[0x9A70]` (the sign-flip gate) and `FUN_00053d50` (the state value).
