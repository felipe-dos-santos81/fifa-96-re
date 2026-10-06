# FU-116 — the replay control timeline and the HUD button handles

Follow-on to FU-111 §8 legs 3 and 5 and FU-114 §6 leg 2. This slice maps the
replay control handler's private state, the shared `[0x4E58C]` phase counter
with its helper predicates, and the resource/name chain that fills the HUD
button descriptor block.

Result in one line: **`FUN_000510dc` is the replay control/phase handler
(private state `[0x8E0C]`/`[0x8E10]`, shared phase counter `[0x4E58C]` started
at 1 and exiting at `0x168`), the `0x53d58`/`0x53d7c`/`0x53d84` helpers read
the phase window `0xF1..0x168`, and `FUN_00018c10` fills the `0x55A54..`
button descriptors by resolving runtime-formatted names against the resource
decoded by `FUN_0004afc0` from the `0x4BFC0` handle table.**

## 1. The control handler `FUN_000510dc`

Callers: `FUN_00051270`/`FUN_000512b0` (pause-menu input arms; `FUN_000512b0`
sets the pause flags `[0x4E688]`/`[0x4E598]`/`[0x4E538]`).

```
if (in_EAX != 0) {                       // button edge
    [0x4E58C] = 1;
    FUN_00064270(...);                   // arm replay: [0x9AB0]!=0 -> FUN_0005865C; mode = 0x81
    FUN_00037114();
    [0x8E0C] = 1; [0x8E10] = 0;
} else {                                 // per-frame
    ticks = FUN_00049388(); if (ticks < 1) ticks = 1;
    FUN_00045069(ticks);
    if ([0x8E0C] == 1) { FUN_000642fc(); FUN_0004cd88(); }   // mode driver + ???
    if (1 < [0x8E0C] && [0x8E10] < 0xF0) { [0x8E10] += ticks; [0x8E0C]++; return; }
    if (0 < [0x8E0C]) { [0x8E10] = 0; [0x8E0C] = 0; }
    if ([0x8E0C] == 0) {
        if (FUN_00064e74(...) /* index >= count */ == 0) {
            if ([0x4E58C] > 0xF0) FUN_00064dfc(...);          // play-forward helper
            if ([0x4E58C] < 0x168) { [0x4E58C] += ticks; return; }
            [0x4E58C] = 0x169; return;
        }
        if ([0x4E58C] <= (int)ticks) { FUN_00064e8c(...); [0x4E58C] = 0; return 1; }
        [0x4E58C] -= ticks;
    }
}
```

`[0x8E0C]` / `[0x8E10]` appear **only** in this function (`search_instructions`
`8e0c`/`8e10`: no other writer/reader) — the handler's private enter/step
state. The constants are the phase thresholds: `0xF0` (start advancing),
`0x168` (hard stop), with `ticks = FUN_00049388()` (frame delta, min 1) and
`FUN_00045069(ticks)` (a per-frame accumulator).

## 2. The shared phase counter `[0x4E58C]`

`search_instructions 4e58c`: written by `FUN_000537f8` (camera state init,
FU-101 §5) and the control handler above; read by the camera helpers:

* `FUN_00053d58` — `return ([0x4E58C]-0xF1 < 0x78) && [0x4E58C] != 0` — the
  active replay window predicate (`0xF1..0x168`, 120 frames).
* `FUN_00053d7c` — getter, `return [0x4E58C]`.
* `FUN_00053d84` — `return [0x4E58C] - 0xF1` (window offset).
* `FUN_00053d9c` — `return [0x4E578] != 0 && [0x4E578] != 6` (camera-mode
  predicate, used by the replay exit `FUN_00064e8c`).

So `[0x4E58C]` is the replay/camera phase counter: 1 on arm, ramping through
the `0xF1..0x168` window while the replay advances, and 0 when idle.

## 3. The HUD button handles

`FUN_00018c10(EAX = resource, param_2 = dest, EBX = count)`:

```
for (i = 0; i < count; i++) {
    FUN_00099e9f((int)local, &DAT_00000600);        // sprintf a name
    *dest++ = FUN_000cbde7(resource, local);        // name -> handle
}
```

`FUN_000cbde7(resource, name)` looks up a key/offset pair table at
`resource+0x10` (count at `resource+8`) and returns `resource + offset` (0 on
miss). In the HUD builder (`FUN_00064690`, FU-111 §5) the resource is the
return of `FUN_0004afc0` — a decoded record:

```
FUN_0004afc0(i) = decode_size_probe([0x4BFC0 + i*4]);
                  size != 0 -> FUN_00098c38(0x1C5C, size+0x10, ...);
                  decode_record_strict([0x4BFC0 + i*4], ...);
```

`[0x4BFC0..]` is the handle table cleared by `FUN_0004a830(0)` (FU-114 §2's
real callee, `0x4BFC0 + 0xFC..0x11B`). The builder calls
`FUN_00018c10(..., &0x55A54)` and then reads the seven descriptor groups
(FU-111 §5) — so `0x55A54..0x55A9C` is the per-button handle array, and the
mode → highlight-slot map from FU-111 §5 is the display's fixed choice:

| mode | highlight slot | builder descriptor |
|------|----------------|--------------------|
| `0x82` (idle) | `[0x55C8C]` | `0x55A88/8C` |
| `0x83` (play) | `[0x55C88]` | `0x55A78/7C` |
| `0x84` (step back) | `[0x55C7C]` | `0x55A98/9C` |
| `0x85` (step forward) | `[0x55C78]` | `0x55A80/84` |
| else (`0x86`) | icon | — |

`[0x55C84]` (the `FUN_00064664` setter) has **no static reader**; the queue's
push never writes index 0 of `0x55C8C` (head wraps after the store), so the
slot belongs to the HUD alone. The `DAT_00000600` format and `DAT_0000195C`/
`DAT_00001964` captions are zero in the static image (runtime-filled).

## 4. Closures / errata

* **FU-111 §8 leg 3 — closed.** `[0x8E0C]`/`[0x8E10]` are private to
  `FUN_000510dc`; `[0x4E58C]` is the replay phase counter with the
  `0xF1..0x168` window (`FUN_00053d58`/`0x53d7c`/`0x53d84`).
* **FU-111 §8 leg 5 — partially closed.** The descriptor writer chain is
  `FUN_0004afc0` (decode) → `FUN_00018c10` (names) → `0x55A54..`; `[0x55C84]`
  is write-only at enumerated scope.
* **FU-114 §6 leg 2 — refined.** `FUN_0004afc0` is the HUD resource decoder
  over the `0x4BFC0` table, not the `FUN_0003dbb0` screen's.
* **FU-112 §1 — refined.** The `0x55C78..0x55C8C` slots are the highlight
  targets for the four replay modes, not generic button rectangles.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x510DC, 0x51270,
0x512B0, 0x53D58, 0x53D7C, 0x53D84, 0x53D9C, 0x537F8 (xref), 0x18C10,
0xCBDE7, 0x4AFC0; `search_instructions` `8e0c` (7, all in 0x510DC), `8e10`
(4 + 3 code false positives), `4e58c` (14), `55c84` (2, both the setter),
`55c78`/`55c8c` (builder/display); `read_memory` 0x5F0 (zero), 0x600
(zero). Address mapping as FU-88.

## 6. Open legs

1. `FUN_0004CD88` (the no-return call after `FUN_000642fc`) and
   `FUN_00049388`/`FUN_00045069` (the tick source/accumulator).
2. `[0x4E578]` (camera mode 0/6) and the `FUN_000537f8` camera-state layout.
3. The button semantics (which play/step/rewind action each slot is) and the
   `DAT_00000600` name format / caption strings.
4. The `FUN_00098C38`/`decode_size_probe`/`decode_record_strict` codec trio
   over `[0x4BFC0]`.
