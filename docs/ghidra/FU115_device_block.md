# FU-115 — the six-slot device block above the replay queue

Follow-on to FU-110 §6 leg 3 and FU-111 §8 leg 6 (the `FUN_00064f70` device
layer). This slice decomposes the `0x55CE0` block, its six slots and the
driver-open path, and resolves the "six devices" question: they are
**audio/MIDI device ports**, not player input.

Result in one line: **`FUN_00064f70` opens up to six driver devices
(`FUN_000a6265(0..5)` via a `0x14`-stride table at `0x14830`, current device
`[0x15FC8]`), keeps per-slot arrays `0x55CE8`/`0x55D00`/`0x55D18` and a handler
table at `0xA0AC`, resets sixteen channels (`FUN_000a6505` →
`FUN_000ba00e(i)`, `i < 0x10`), and the block's notify hooks
(`FUN_00065cc0`, `FUN_000651f0`) drive note writes (`FUN_000a7728`) and the
`FUN_000a72e7` sequencer/period trigger — the same layer the replay queue
notifies.**

## 1. Initialisation `FUN_00064f70`

```
if ([0x55CE0] == 0) {
    [0x55CE4] = FUN_00068cfc();                  // device count/flags
    if (0 < [0x55CE4]) {
        i = FUN_000a6265(-1);                    // open/probe
        [0x55CE0] = (i >= 0);
        if (i < 0) { FUN_000a6a03(...); FUN_000cbbe8(...); }   // fallback
        for (k = 0; k < 6*4; k += 4) {           // six slots
            [0x55D18+k] = 1; [0x55D00+k] = 0; [0x55CE8+k] = 0;
        }
        for (d = 0; d < 6; d++) FUN_0006504c(1, d+1);
    }
}
```

* `FUN_0006504c(EAX = slot)`: `[0x55D18+4s] = 1`,
  `[0x55D00+4s] = &[0x55CE8+4s]` (the slot's data area), and if the handler
  pointer `[0xA0AC+4s] != 0` calls it with `param_2`.
* `FUN_00068cfc(EAX = i)`: `i > 4 → 0`, else
  `return [0x56400 + i + 1] >> 24` — the per-slot device-present byte.
* Slot arrays: values `0x55CE8 + 4s`, pointers `0x55D00 + 4s`, flags
  `0x55D18 + 4s` for `s = 0..5` (the init loop writes `0..0x17`).

## 2. The driver-open path `FUN_000a6265`

```
FUN_000a6265(index):                  // in_EAX = 0..5
    if (index < 0 || 5 < index) { [0x15FC8] = 0x2E0C1D0; return -4; }
    FUN_000a6505(...); FUN_000cbda0(...);          // once each
    r = (**(code **)(&0x14830 + index*0x14))();    // per-device entry
    if (r < 0) { [0x15FC8] = 0; return r; }
    [0x15FC8] = index; FUN_000a64ac(...);
```

`FUN_000a6505` resets the driver: `FUN_0009f684(0xa6a8f)`, `[0x15FC8] = 0`,
`FUN_000ba00e(i)` for `i = 0..0xF` (sixteen channels/notes), then
`FUN_000a765f`. The per-index table at `0x14830` (stride `0x14`) dispatches
the open; `[0x15FC8]` is the current device index (or `0` on failure), and
`0x2E0C1D0` is the out-of-range error value. `FUN_000cbda0` chains to
`FUN_000b25df` — the same `0xCxxxx` band as the file/stream helpers
(`FUN_000cbbe8`, FU-114).

## 3. The notify hooks and what they drive

* `FUN_00065cc0`: if `[0x55CE0] != 0 && [0x55D30] != 0 && [0x55D00] > 0 &&
  [0xA250] != 0` → `[0xA254] = FUN_000a7705()`. This is the hook
  `FUN_000974dc` calls after a queue push (FU-113 §3).
* `FUN_000a7705` = `FUN_000a7728(0x40)` — a driver write:
  index `< 0x80` into the per-channel handler table `[0x61C14]`; the
  `+0x1c` bit-0 path splits the write across the channel and channel+1
  (`FUN_000a6717` then `FUN_000a780e` twice), else a single
  `FUN_000a780e(port, index, value)`; `0xFFFFFFED` when the channel is
  unbound. `FUN_000a76ea` is the same `FUN_000a7728(0x40)` write.
* `FUN_000651f0(code)` (called before the `0x77B9E` queue push in
  `FUN_00077728`, FU-113 §1): if `[0x55CE0] && [0x55D38] && [0x55D08] > 0` →
  `[0x55D50] = code`, `FUN_000a72e7()`.
* `FUN_000a72e7` is the **sixteen-channel sequencer trigger**: `in_EAX` must
  be `0..0xF`; it indexes `0x14`-stride tables at `0x5DA54`/`0x5DA58`/
  `0x5DA5C`/`0x5DA64`, arms `[0x5D820]`/`[0x5D828]` window state and calls
  `FUN_000a7040()` (period) and `FUN_000a76ea()` (the `0x40` write).
* `FUN_000a7040` computes `[0x5DB94] = abs(([0x5DA58+i*0x14] << 16) /
  ([0x5D844 + [0x5DA60+i*0x14]*8] * 100 >> 16))` — a frequency/period divider
  for the current device, with the rate table at `0x5D844`.

So the block is the **music/audio driver interface**: six logical devices
(opened 0..5), sixteen channels (`0x5DAxx` tables), frequency/period math, and
a note/port write path. No static strings name the driver (`AIL`/`MIDI`/`AdLib`
search: none); the identity of the hardware target stays open.

## 4. Closures / errata

* **FU-110 §6 leg 3 — closed.** The "six devices" are six audio/MIDI device
  slots opened by `FUN_000a6265`, reset to sixteen channels by
  `FUN_000a6505`, not player input devices. `FUN_00068cfc`,
  `FUN_000a6265`, `FUN_000a6a03`, `FUN_000cbbe8` are all in that path.
* **FU-111 §8 leg 6 — partially closed.** The slot arrays and handler table
  are mapped; `FUN_000a6a03` (a very large fallback, not decomposed) and the
  `0x55D08`/`0x55D30`/`0x55D38`/`0x55D50` field roles stay open.
* **FU-113 §3 — refined.** The queue's notify chain terminates in driver
  writes (`FUN_00065cc0` → `FUN_000a7705`; `FUN_000651f0` → `FUN_000a72e7`),
  so a recorded event byte can re-trigger a note/period when applied.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x64F70, 0x6504C,
0x68CFC, 0xA6265, 0xA6505, 0x9F64C, 0xCBBE8, 0xCBDA0, 0xA72E7, 0xA7040,
0xA76EA, 0xA7705, 0xA7728; `search_strings` `(AIL|MIDI|AdLib|Sound Blaster|
General MIDI|MSS)` (no driver strings; memory-manager messages only).
Address mapping as FU-88.

## 6. Open legs

1. The hardware driver identity (no static name) and the `0x61C14`/
   `0x14830` handler tables' real targets.
2. `FUN_000a6a03` (fallback/initialisation; enormous — not decomposed) and
   `FUN_000cbbe8`/`FUN_000cd8b4`/`FUN_000cda1e`.
3. The `0x55D08`/`0x55D28`/`0x55D30`/`0x55D38`/`0x55D50` fields (slot-2/…
   views of the same block).
4. `FUN_000a780e`/`FUN_000a6717` (the write primitives) and `FUN_000a765f`.
