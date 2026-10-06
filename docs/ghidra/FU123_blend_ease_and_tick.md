# FU-123 — the blend ease and the frame tick

Follow-on to FU-122 §6 leg 1 (`FUN_0004f830`, `FUN_00049388`). Two small
closures for the view-blend driver.

Result in one line: **`FUN_0004f830(clock, duration)` is an integer
piecewise-quadratic ease built from a 32×32→64 multiply (`FUN_000a2ad8`) and
its divide (`FUN_000a2aee`), and `FUN_00049388` is just `return [0x7300]` —
the per-frame tick the match loop writes.**

## 1. The ease `FUN_0004f830`

Three branches over `[clock, duration]` (`EAX` = delta, `ECX` = clock,
`EBX` = duration):

```
clock <  duration: (clock² * delta * 2) / duration²          // accelerate
clock <= EBX     : delta - ((2*clock - duration)² * delta) / duration²*2 ...
else             : delta - ((duration² - (clock-duration)²) * delta) / duration²*2
```

(`FUN_000a2ad8` = `imul` return, `FUN_000a2aee` = the matching divide), i.e.
the classic symmetric quadratic ease-in/out. The view-blend driver calls it
once per camera field per frame with the blend clock and duration (FU-122 §2).

## 2. The frame tick

```
FUN_00049388() { return [0x7300]; }
FUN_0004937c() { return [0x7300]; }      // second getter
FUN_000492d4(EDX) { ... [0x7300] = EDX; } // setter (also returns?)
```

`search_instructions 7300`: writers `FUN_000492d4` and the per-frame sites
`0x496A7`, `0x49A5E`, `0x49AA2` (the last two in the `0x49Axx` match-loop
band, next to the packer wrappers of FU-111 §1); readers the two getters.
So `[0x7300]` is the current frame delta/tick the replay control handler
(`FUN_000510dc`, FU-116 §1) and the view blend both consume.

## 3. Closures / errata

* **FU-122 §6 leg 1 — closed.** The ease is `FUN_0004f830`; the tick source
  is `[0x7300]` (`FUN_00049388`).
* **FU-116 §1 — refined.** The control handler's `ticks = FUN_00049388()` is
  the match-loop frame delta written at `0x49A5E`/`0x49AA2`.

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x4F830, 0x49388,
0xA2AD8; `search_instructions` `7300` (6). Address mapping as FU-88.

## 5. Open legs

1. `FUN_000492d4` (the tick computation) and the orphan writers `0x496A7` /
   `0x49A5E` / `0x49AA2`.
2. `FUN_000a2aee` (the divide) and the exact rounding.
