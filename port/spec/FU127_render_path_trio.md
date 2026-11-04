# FU-127 — the frame-state getters and the render-path trio

Follow-on to FU-126 §5 leg 1 (the `0x43xxx` gate cluster and the
`0x58xxx` trio). This slice names the gates and the three render/update paths
the dispatcher selects.

Result in one line: **`FUN_0004382c` = `[0x677C]` and `FUN_00043608`/
`FUN_00043610` = get/set of `[0x6780]` are the dispatcher's state gates;
`FUN_00043600` returns the view bounding-box structure at `0x4B0A4` and
`FUN_00043618` merges a 4-pair rect list into its min/max fields
`0x4B07C/0x4B080/0x4B084/0x4B088`; the trio `FUN_00058b68` (normal),
`FUN_00058d70` (gated by `[0x6780]`), `FUN_00058bc0` (gated by the
replay/special condition) runs a common render core over the `0x54324`
table.**

## 1. The gates

| function | effect |
|----------|--------|
| `FUN_0004382c` | `return [0x677C]` (match state) |
| `FUN_00043608` | `return [0x6780]` |
| `FUN_00043610` | `[0x6780] = EAX` |
| `FUN_00043600` | `return &0x4B0A4` (the view bbox accumulator) |
| `FUN_00043618` | bbox merge: copy 10 dwords `0x4B0A4 -> 0x4B07C`, then for an EAX-passed 4-pair rect list (start index EBX) accumulate min/max into `0x4B07C`/`0x4B080`/`0x4B084`/`0x4B088` |
| `FUN_00043608` reads, `FUN_00043610` writes | the `[0x6780]` sub-state used by `FUN_00058d70` |

The dispatcher's selection (FU-126 §1):

```
if ([0x677C] && FUN_0006400c() && FUN_000541d4())  FUN_00058bc0([0x7314]);   // replay/special
else if ([0x6780] != 0)                            FUN_00058d70([0x7314]);   // paused/held
else                                               FUN_00058b68([0x7314]);   // normal
```

## 2. The render paths

All three take the view record `[0x7314]` (from `FUN_0004c904`, FU-121) and
share the core sequence around the table `0x54324`:

```
FUN_00058a44(record);
FUN_00062b40(&0x54324); FUN_0006331c(&0x54324);
FUN_0005feac(record); FUN_000610c4(record); FUN_00058ac4(record);
FUN_00058538(); FUN_000565bc(); FUN_00039180();
```

* `FUN_00058b68` — the normal path: the core plus `FUN_0006331c`.
* `FUN_00058d70` — the held path: if `[0x9094] == 0` and
  `FUN_00043608() != 1`, it skips the core and runs `FUN_00043610` /
  `FUN_000638b0` / `FUN_00043618` instead; otherwise the core.
* `FUN_00058bc0` — the special path: takes the fast branch when
  `[0x9094] != 0` or `[0x677C] == 1` or the replay family
  (`FUN_00063fd0() != 0`), else the slow branch with
  `FUN_00043568`/`FUN_000435d0`/`FUN_000435c0`/`FUN_0004372c`,
  `FUN_0008eb70`/`FUN_0008eb78`, `FUN_000603a8`/`FUN_00060344`/
  `FUN_000603ec`, the `func_0x00057754` indirect call and
  `FUN_00056fa4(&0x54324)`; it advances the `[0x9090]` frame counter and
  resets it when leaving the special state.

## 3. Closures / errata

* **FU-126 §5 leg 1 — closed.** The gates are `[0x677C]`/`[0x6780]` getters
  and the bbox accumulator `0x4B0A4`; the trio are the normal/held/special
  render paths, not the audio trio.
* **FU-126 §2 — refined.** `[0x7318]` is the `0x4B0A4` bbox pointer, and
  `FUN_00043618` is what feeds `FUN_00043600`'s structure.
* **FU-126 §1 — refined.** `FUN_000541d4` and `FUN_0006400c` gate the
  special path selection.

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x58B68, 0x58D70,
0x58BC0, 0x4382C, 0x43608, 0x43600, 0x43610, 0x43618. Address mapping as
FU-88.

## 5. Open legs

1. The render core (`FUN_00058a44`, `FUN_00062b40`, `FUN_0006331c`,
   `FUN_0005feac`, `FUN_000610c4`, `FUN_00058ac4`, `FUN_00058538`,
   `FUN_000565bc`, `FUN_00039180`) and the `0x54324` table layout.
2. `[0x677C]`/`[0x6780]`/`[0x9090]`/`[0x9094]` producers and the special-path
   `FUN_000435xx` cluster.
3. The `func_0x00057754` indirect call.
