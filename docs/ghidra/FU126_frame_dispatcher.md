# FU-126 — the outer match-frame dispatcher `FUN_000495b0`

Follow-on to FU-125 §5 leg 1 (the `0x496xx` body's entry). The entry is
`0x495b0`; the body was masked by hidden data items at `0x495C0` and now runs
`0x495B0..0x49817` after repair. This slice names the frame pipeline it
drives.

Result in one line: **`FUN_000495b0` is the indirect-entry outer match-frame
body: it ticks (`FUN_00049280`), zeroes `[0x7300]`/`[0x7304]`, then loops the
view/state updates (`FUN_0004d2d4`, `FUN_0004c904` → `[0x7314]`,
`FUN_00043600` → `[0x7318]`), the audio/state gates, and — when the recorder
gate `FUN_0006400c()` is on — calls `FUN_00049b28` (the packer's match loop),
the replay-family branch (`FUN_00063fd0` → `FUN_0004a068`/`FUN_00053d7c`) and
the input gates, storing the true frame delta in `[0x7300]` at `0x496A7` and
returning when `[0x7304] != 0`.**

## 1. Shape

```
FUN_00049280(...); [0x7300] = 0; [0x7304] = 0;
do {
    if (FUN_00053d58()) FUN_0004cd70();          // replay window (FU-116 §2)
    FUN_0004d2d4();
    [0x7314] = FUN_0004c904();                   // view record ptr (FU-121)
    [0x7318] = FUN_00043600();
    FUN_00043e30(); FUN_000439d0();               // per-state audio/UI
    gate = FUN_0004382c() && FUN_0006400c() && FUN_000541d4();
    if (gate) { FUN_00043330(); FUN_00058bc0([0x7314]); }
    else if (FUN_00043608()) { FUN_000432ec(); FUN_00058d70([0x7314]); }
    else { FUN_000432ec(); FUN_00058b68([0x7314]); FUN_00049830(); FUN_000565bc(); FUN_00039180(); }
    FUN_00043b48(); FUN_000478d0();
    delta = FUN_00049280(...); [0x7300] = delta;   // the true tick (0x496A7)
    if ([0x730c] != 0) { FUN_00045d0d(); [0x730c] = 0; }
    if (FUN_0004a178() == 0) {
        if (FUN_0006400c()) {                       // recorder/match gate
            FUN_00049b28();                          // the match/packer loop (FU-111)
            ... FUN_00036c3c / [0x7304] latch ...
        }
        if (FUN_00063fd0()) { FUN_0004a068(); FUN_000635b8(); if (FUN_00053d7c()) FUN_00053bb8(); }
        if (!FUN_00037ae4() && FUN_0006400c()) FUN_0004536c();
        if ([0x72f8] && (FUN_00036bc8() || FUN_00036bc8())) FUN_00036bc0();
        FUN_0004a294();
        if (FUN_0004557c()) { if (FUN_0006d1b2() == 1) FUN_00045d0d(); else { FUN_00036bc0(); [0x7304]=0; } }
    }
    if ([0x7304] != 0) { if (FUN_0004557c()) { FUN_00036bc0(); [0x7304]=3; } return; }
} while (true);
```

## 2. The frame-state globals

| global | role |
|--------|------|
| `[0x7300]` | the shared frame tick (FU-123/FU-124); zeroed at frame start, true delta stored at `0x496A7` |
| `[0x7304]` | loop exit state: 1/2 latched from `FUN_0004b378`/`FUN_00036c3c` paths, 3 after the final `FUN_00036bc0` |
| `[0x730c]` | pending-flush flag: when set, `FUN_00045d0d` runs and the flag clears |
| `[0x7314]` | view record pointer from `FUN_0004c904` (FU-121) for this iteration |
| `[0x7318]` | `FUN_00043600` result |
| `[0x72f8]` | gate for the controller-check pair (`FUN_00036bc8`, FU-113 §2) |

The recorder gate `FUN_0006400c` is the same predicate the replay packer sites
check (FU-111 §1); when it is on, the loop calls `FUN_00049b28`, which is the
match body that snapshots (`FUN_00036c70`) and packs (`FUN_0006408c`) replay
records. So this dispatcher is the per-frame **root of the live recording
chain**.

## 3. Closures / errata

* **FU-125 §5 leg 1 — closed.** The `0x496A7` writer is inside
  `FUN_000495b0`, created and repaired this slice (body
  `0x495B0..0x49817`).
* **FU-124 §5 leg 1 — closed.** The true-delta store is the `EAX` one in this
  function; the two `EDX` stores remain the pause freeze (FU-125).
* **FU-111 §1 — refined.** `FUN_00049b28` is called from here under
  `FUN_0006400c`; its packer sites are reached once per dispatcher iteration.
* Ghidra-project change: cleared hidden data items at `0x495C0`/`0x495CC` and
  re-disassembled from `0x495C0`; created `FUN_000495b0`.

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: backward instruction walk from `0x495CC`
(boundary `RET 0x495AF`); `clearListing` `0x495C0..`, `DisassembleCommand`
`0x495C0`; `decompile_function` 0x495B0; `get_function_callers` 0x495B0 (0).
Address mapping as FU-88.

## 5. Open legs

1. The `FUN_00043xxx` gate cluster (`0x4382c`/`0x43608`/`0x43600`/`0x43608`/
   `0x43e30`/`0x439d0`/`0x43b48`) and the `FUN_00058b68`/`0x58d70`/`0x58bc0`
   audio trio.
2. `FUN_0004a178`/`0x4a294`/`0x4a068`/`0x49830`/`0x565bc`/`0x39180`.
3. `FUN_0004b378`, `[0x7304]`'s 1/2/3 semantics, `[0x730c]`'s writer.
