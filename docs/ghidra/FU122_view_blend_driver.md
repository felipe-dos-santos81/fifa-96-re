# FU-122 — the view-blend driver `FUN_0004f8c8`

Follow-on to FU-121 §5 leg 2 (the `(10, tblA, 0xF)` setup). This slice
decomposes the routine that interpolates the active view object's six-field
camera matrix.

Result in one line: **`FUN_0004f8c8` is a three-mode blend driver —
query (`EAX==0` → `elapsed >= duration`), start (`param_2 != 0` → copy the six
fields into the view object and the origin globals `0x4E4F8..0x4E50C`, compute
the delta block `0x8BD4..0x8BE8`, set `[0x7DE4]=10*param_1`,
`[0x7DE8]=10*param_3`), and step (`[0x7DE0] += FUN_00049388()` then add
`FUN_0004f830(clock, duration)`-scaled deltas per field) — over the same six
camera fields the replay record packs at `+0xF6`.**

## 1. The blend state

| global | role |
|--------|------|
| `[0x7DE0]` | elapsed ticks |
| `[0x7DE4]` | start offset = `10 * param_1` |
| `[0x7DE8]` | duration = `10 * param_3` |
| `[0x7DD8]` | done/idle flag |
| `0x4E4F8..0x4E50C` | six-dword blend origin (copy of the start matrix) |
| `0x8BD4..0x8BE8` | six per-field deltas (`tblB - view`), angles wrapped to ±0x8000 |

## 2. The three modes

```
EAX = view object (e.g. 0x78F8), param_1 = 10, param_2 = tblA, EBX = tblB, param_3 = 0xF

EAX == 0:  return (10*param_1 + 10*param_3 <= elapsed);          // query
param_2 != 0:                                                    // start
    view+0x10/14/18/58/5C/4C = tblA[0..5];
    0x4E4F8..0x4E50C = tblA[0..5];
    if (tblB) { deltas 0x8BD4.. = tblB - view (angles wrapped); 
                [0x7DE4]=10*param_1; [0x7DE0]=0; [0x7DE8]=10*param_3; }
    return false;
else:                                                            // step
    if (duration + offset <= elapsed) { [0x7DD8]=0; return true; }
    [0x7DE0] += FUN_00049388() (clamped to duration+offset);
    for each of the six fields:
        view+off = origin + FUN_0004f830([0x7DE0], [0x7DE4]);
    view+0x5A = 0; view+0x5E = 0;   // 16-bit high halves of the angle pairs
    return false;
```

The step adds the tick delta `FUN_00049388()` (the same source the replay
control handler uses, FU-116 §1) and the per-field value
`FUN_0004f830([0x7DE0], [0x7DE4])` — an ease/cur-offset of the delta. The
fields are exactly the camera matrix FU-112 §1 reads from `[0x7DC8]` into the
record (`+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C` → record `+0xF6..`), so the
replay records the blended view, not just the raw object.

## 3. Context

* `FUN_0004cc98` (FU-121 §2) starts the blend from `0x78F8` with
  `tblA = 0x872C[0..5]` and `tblB` one of `0x80B4`/`0x81A4[0..5]`, after the
  `[0x9A70]+8 < 0` axis mirror (`x -> 0x18000 - x` on the angle lanes).
* `0x872C`/`0x80B4`/`0x81A4` are zero in the static image (runtime-filled
  orientation tables).
* `[0x9A70]` is a shared state object read by the view band
  (`FUN_000365b0`, `FUN_0004cba0`, `FUN_0004cc80`, `FUN_0004d04c`,
  `FUN_0004ff54`, `FUN_00058a44`, `FUN_00062588`, `FUN_00062620`); its `+8`
  word sign selects the mirrored axes.

## 4. Closures / errata

* **FU-121 §5 leg 2 — closed.** `FUN_0004f8c8` is a blend driver (query /
  start / step) over the six camera fields with origin `0x4E4F8` and deltas
  `0x8BD4`, clocked by `FUN_00049388`.
* **FU-112 §1 — refined.** The composer's camera matrix is mid-blend whenever
  a view transition is active, so records capture interpolated views.
* **FU-116 §1 — refined.** `FUN_00049388` drives both the replay control
  timeline and the view blend.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x4F8C8;
`read_memory` 0x872C / 0x80B4 (24 B, zero); `search_instructions` `9a70`
(22 sites in the view band). Address mapping as FU-88.

## 6. Open legs

1. `FUN_0004f830` (the ease function) and `FUN_00049388` (the tick source).
2. `[0x9A70]`'s identity (its `+8` sign selects the axis mirror).
3. The `0x872C`/`0x80B4`/`0x81A4` table fillers.
