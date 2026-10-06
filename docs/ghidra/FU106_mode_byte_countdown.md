# FU-106 — the mode byte `[0x9A98]` and the countdown/camera-return path

Follow-on to FU-103 §6 legs 3/4 (the countdown helpers and `FUN_0004CD88`'s
neighbourhood). This slice maps the global mode byte the camera handlers and
the director both test, and the countdown pair that drives its camera-return
call.

Result in one line: **`[0x9A98]` is the match-mode byte (`FUN_0006400c` tests
`== 0`, `FUN_0006401c` tests `== 0x82`; writers set `0`/`0x81`/`0x82`/`0x83`)
and `FUN_00064e8c`'s reset-to-0 path calls the camera driver
`FUN_0004cef4`; the `[0x9A94]`/`[0x9AC0]` countdown (accumulator `[0x9AA8]`)
drives `FUN_00064dfc`'s tick loop, and `FUN_00047888` clears when the
`[0x8E3C]` counter expires.**

## 1. `[0x9A98]` — the mode byte

`get_xrefs_to 0x9A98` gives 29 references. The predicates the camera code
already uses:

```
FUN_0006400c() { return [0x9a98] == 0; }     ; normal mode (FU-100 §3: 100/120 switch)
FUN_0006401c() { return [0x9a98] == 0x82; }  ; the class-3 / camera-tail gate
```

Other readers: `FUN_00063fd0`, `FUN_00063fdc`, `FUN_00063ff0`, `FUN_00064ec0`
and the `FUN_000642fc` body. Writers:

| writer | value | context |
|--------|-------|---------|
| `FUN_00064e8c` (`0x64E94`) | **0** | mode reset, then the camera driver (below) |
| `FUN_00064270` (`0x64284`) | **0x81** | only when `[0x9AB0] != 0`; calls `FUN_0005865c` |
| `FUN_00064dfc` (`0x64E1F`/`0x64E67`) | **0x83 / 0x82** | the countdown's active/idle arms |
| `FUN_00063cbc` (`0x63D27`), `FUN_00063ebc` (`0x63F41`) | — | plus `FUN_000642fc`'s twelve writes and four unnamed sites (`0x64504`, `0x64CF0`, `0x63EAE`, `0x63FC5`) |

## 2. The countdown pair

* `FUN_00064e74() → [0x9AC0] <= [0x9A94]` — the expiry test.
* `FUN_00064dfc()` — the tick:
  ```
  if ([0x9a94] < [0x9ac0]) {
      [0x9a98] = 0x83; FUN_00037114(); [0x9aa8] += FUN_000492cc();
      while (0 < i && i <= [0x9aa8]) { [0x9aa8] -= i; FUN_00063cbc(); FUN_0006428c(); i = FUN_00063d34(); }
      return true;
  }
  [0x9a98] = 0x82; return false;
  ```
* the input gate `FUN_00045069` (called by the director's `FUN_000510dc`):
  reads `[0x67F3] & 0xF0` and sets `[0x67F4] = 5` when the nibble is
  nonzero.
* `FUN_00047888` — `FUN_000a1600(0, 0x100, 0x4B800)` and `[0x713C] = 0`;
  called by `FUN_00051c98` when `[0x8E3C]` reaches 0 (the director's
  camera-9 countdown, FU-103 §2).

## 3. `FUN_00064e8c` — mode reset calls the camera driver

```
DAT_00009a98 = 0;
FUN_0004afa0(param, 0);
FUN_0004cef4();          ; the camera re-select + transition-reset driver (FU-101 §4)
FUN_00036c70();
if (FUN_00053d9c() == 0) FUN_000974d8();
```

so leaving the countdown mode is what re-runs the normal camera selection —
the same driver `FUN_0004CEF4` that initialises `[0x7DD8] = 0` and calls the
selector `FUN_000505D0` (FU-100/101).

## 4. Closures / errata

* **FU-103 §6 leg 3 — partially closed.** `FUN_0004CD88` (the no-return after
  `FUN_000642fc` in `FUN_000510dc`) is still unmapped; its caller sets mode
  0x83 first (via `FUN_00064dfc`'s active arm), so it is a terminal
  transition, not a plain error path.
* **FU-103 §6 leg 4 — partially closed.** `FUN_00064e74`/`FUN_00064dfc`/
  `FUN_00064e8c`/`FUN_00045069`/`FUN_00047888` are decomposed;
  `FUN_00064270` maps the 0x81 mode.
* **Camera link — confirmed.** `FUN_00064e8c` → `FUN_0004cef4` ties the mode
  subsystem to the camera driver chain; `FUN_0006401c`'s 0x82 gate is the
  same predicate the class-3 constant switch uses.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x64270, 0x64E74,
0x64DFC, 0x64E8C, 0x45069, 0x47888, 0x6400C (0x642FC decompile timed out);
`get_function_by_address` 0x64270/0x642FC/0x64E74/0x64DFC/0x64E8C/0x45069/
0x47888; `get_xrefs_to` 0x9A98 (29). Address mapping as FU-88. Analysis-only:
no port, capture-rig, ISO, or Ghidra-project change.

## 6. Open legs

1. **`FUN_000642FC`** (`0x642FC..0x64660`, twelve `[0x9A98]` writes) — the
   mode's main body; the decompile timed out this session.
2. **`FUN_0004CD88`** (no-return terminal) and `FUN_000492CC`/`FUN_00063CBC`/
   `FUN_00063D34`/`FUN_0006428C` (the countdown loop's step/queue).
3. **The mode values 0x81/0x82/0x83 semantics** and the unnamed writers at
   `0x64504`, `0x64CF0`, `0x63EAE`, `0x63FC5`.
4. **`FUN_000A1600(0, 0x100, 0x4B800)`** — argument order/meaning (a fill?).
5. **`FUN_0005865C`** (0x81-mode action) and `FUN_0004AFA0`/`FUN_000974D8`
   (the mode-reset side effects).
