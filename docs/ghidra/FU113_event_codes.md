# FU-113 — the event codes the input queue carries

Follow-on to FU-111 §8 leg 4 (the pushed words at the seven event sites) and
FU-111 §3 (the queue's producer/consumer). This slice enumerates every
`FUN_000974dc` call site, the code each one pushes and the gate that admits
them, and links the consumer side back into the replay drain.

Result in one line: **the queue carries gameplay event codes, not raw input —
10 sites push `1, 2, 4, 8, 0xB, 0xC, 0xE, 0x1E` under mode/state guards; the
push is gated by `FUN_00037AE4` = three `EAX == [0x5FFC]` device-null tests;
the drain (`FUN_0006428c`) re-notifies the applied record's event byte through
`FUN_000974dc`, which notifies `FUN_00065cc0`/`FUN_000651f0` but cannot
re-enqueue inside the replay family.**

## 1. The call sites and their codes

| site | in | code (`EAX`) | guard |
|------|----|--------------|-------|
| `0x710C1` | `FUN_00070de0` | `8` | `[ESP+0x14] = ±0xAD4` staged by the sign of EDI |
| `0x7121D` | `FUN_00070de0` | `0xE` | `[0x5781C] == 0` and `[0x577BC] >> 16 >= 0xA` |
| `0x7123D` | `FUN_00070de0` | `8` | after the `0xE` path (`FUN_000974f0(0x3E8)`) |
| `0x71B5B` | `FUN_0007131c` | `1` or `2` | `EAX = (v > 8) + 1`, `CWDE`/`INC` |
| `0x77B9E` | `FUN_00077728` | `1` | after `FUN_000651f0(6)` and a `AX > SI` test |
| `0x7A2E1` | `FUN_0007a084` | `0xB` or `0xC` | `EAX <= 0x3C0` → `0xB`, else `0xC` |
| `0x7EF5F` | `FUN_0007e7c8` | `4` | entity byte `[[EBP+0x28]] == 0x13`, `EBX = 4` too |
| `0x84085` | `FUN_00084021` | `0x1E` | `[0x4C32A] != 0` (after `FUN_0004c324`) |
| `0x88B69` | `FUN_00088940` | `0x1E` | mode `([0x57A4A] >> 24) == 2` |
| `0x88BDE` | `FUN_00088940` | `0x1E` | same guard, second arm of the function |

`search_instructions` in each function found 3 sites in `FUN_00070de0` and 2
in `FUN_00088940`; the ten listed are all `CALL 0x974dc` instructions in the
seven functions FU-111 named. The codes are event identifiers fed to the
input queue; their per-code semantics (which match event each triggers) stay
open.

## 2. The gate (`FUN_00037AE4`)

```
FUN_00037AE4() = FUN_00036bc8() || FUN_00036bc8() || FUN_00036bc8()
FUN_00036bc8() = (in_EAX == [0x5FFC])
```

Three device-slot tests against the single null/idle code `[0x5FFC]`;
`FUN_00064ec0` pushes only when the gate returns 0, i.e. when at least one
slot differs from the null code. So the queue records frames that carry a real
device state, not idle ones.

## 3. The notify arms and the drain

* `FUN_00065cc0` (the push's notify): if `[0x55CE0]`, `[0x55D30]`,
  `[0x55D00]` and `[0xA250]` are set → `[0xA254] = FUN_000a7705()`.
* `FUN_000651f0(code)` (called before the `0x77B9E` push): if `[0x55CE0]`,
  `[0x55D38]`, `[0x55D08]` are set → `[0x55D50] = code`, `FUN_000a72e7()`.
* `FUN_000974f0(EAX)` = `FUN_00063b20()` — the `[0x97BC]` speed gear setter
  (FU-112 §1) invoked with `0x3E8` at `0x71227`.
* Drain hook `FUN_0006428c` (`if ([0x55C4C] >> 24 != 0) FUN_000974dc();`): the
  mode-`0x83` drain calls it after `FUN_00063cbc` (FU-111 §4), so an applied
  record whose event byte (`[0x55C4F]`, packed at `+0xF3`) is nonzero goes
  back through `FUN_000974dc` → `FUN_00064ec0` (blocked by the
  `[0x9A98] & 0x80` replay-family test) → `FUN_00065cc0` notify. Live sites
  take the same path but enqueue; replay only notifies.

## 4. The live cadence (caller repair)

With FU-111's body repairs, `get_function_callers 0x36C70` now returns
`FUN_0003705c`, `FUN_00049ad0`, **`FUN_00049b28`**, `FUN_0004a228`,
`FUN_000642fc`, `FUN_00064e8c`. Both packer wrappers call the composer
immediately before the packer (`FUN_00049b28` at the repaired `0x49FF5`,
`FUN_00049ad0` at `0x49AD1`), so the live record cadence is
**snapshot (`FUN_00036c70`, which pops one queued event into `[0x55C4F]`) →
pack (`FUN_0006408c`)**, one event per recorded frame at most.

## 5. Closures / errata

* **FU-111 §8 leg 4 — partially closed.** The pushed values are event codes
  and all ten sites are enumerated with their guards; the per-code names
  (which event `1, 2, 4, 8, 0xB, 0xC, 0xE, 0x1E` denote) remain open.
* **FU-111 §3 — refined.** The producer gate is a three-slot null-code test
  against `[0x5FFC]`; the consumer's `FUN_0006428c` re-entry is explained
  (notify-only inside the replay family).
* **FU-112 §1 — refined.** The composer is called from both packer wrappers,
  so the popped event byte is per recorded frame.

## 6. Provenance

Ghidra MCP on `/fifa96_le.bin`: `search_instructions` `974dc` scoped to
`FUN_00070de0` (3), `FUN_0007131c` (1), `FUN_00077728` (1), `FUN_0007a084`
(1), `FUN_0007e7c8` (1), `FUN_00084021` (1), `FUN_00088940` (2) and
instruction context for each; `decompile_function` 0x36BC8, 0x37AE4, 0x651F0,
0x974F0, 0x974DC, 0x6428C; `get_function_callers` 0x36C70 (6, including
`FUN_00049b28` after the FU-111 repair). Address mapping as FU-88.

## 7. Open legs

1. The per-code event names (`1, 2, 4, 8, 0xB, 0xC, 0xE, 0x1E`) — the sites
   are gameplay-state driven; the consumer arms (`FUN_000a7705`,
   `FUN_000a72e7`, `[0x55D50]`, `[0xA254]`) are the event layer.
2. The device slot identities behind the three `FUN_00036bc8` calls and
   `[0x5FFC]`.
3. The `±0xAD4` staged value at `0x710AA`/`0x710B4` (its consumer).
