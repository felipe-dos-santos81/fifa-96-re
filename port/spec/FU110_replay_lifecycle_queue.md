# FU-110 — the replay lifecycle drivers and the 20-slot command queue

Follow-on to FU-109 §7 legs 3/4 (the not-in-replay body and the ring
producer) and FU-106 §3 (`FUN_0004a228`'s role). This slice traces who
allocates, arms and fills the replay ring, and decomposes the small command
queue its frame hook returns through.

Result in one line: **the packer `FUN_0006408c` is called from the match band
(`FUN_00049b28` at `0x49B1E`, an unnamed `0x4A040` site), the allocator
`FUN_00063ebc` from `0x4AF18`, the recorder gate `[0x9a90]` is set by
`FUN_00064080` (from `FUN_0004a228`/`FUN_00052030`/`FUN_00052d68`/
`FUN_00053de0`) and cleared by `FUN_00064074` (via `FUN_00053dc4`), and
`FUN_00064ef0` is a 20-slot command ring (head `[0x9af0]`, tail `[0x9af4]`,
count `[0x9aec]`) driven from `FUN_00064ec0`'s not-in-replay path.**

## 1. The ring's producers (xrefs)

| symbol | called from | in |
|--------|-------------|----|
| packer `FUN_0006408c` | `0x49B1E`, `0x4A040` | `FUN_00049b28`, an unnamed `0x4A0xx` function |
| allocator `FUN_00063ebc` | `0x4AF18` | an unnamed `0x4AFxx` function |
| gate set `FUN_00064080` | `FUN_0004a228`, `FUN_00052030`, `FUN_00052d68`, `FUN_00053de0` | match phases |
| gate clear `FUN_00064074` | `FUN_00053dc4` (from `FUN_00037f54`, `FUN_0004b100`) | |
| ring reset `FUN_00064030` | `FUN_0004a228` | |

`FUN_0004a228` is the aggregator: it also calls `FUN_000537f8` (the camera
state init, FU-101 §5) and `FUN_00064080`, and `get_function_callers` finds
no caller — an indirect-entry phase handler. So the replay ring and the
camera-state block are armed together from the match flow.

## 2. `FUN_00064ef0` — the 20-slot command ring

```
in_EAX == 1 (push):
    if ([0x9aec] > 0x13) { if (tail+1 > 0x13) tail = 0; [0x9aec]--; [0x9af4] = tail'; }
    [0x9af0] = ([0x9af0] + 1) wrap 0x13;
    [0x9aec]++;
    (&0x55C8C)[[0x9af0]] = param_2;
else (pop):
    if ([0x9aec] < 1) return 0;
    tail = ([0x9af4] + 1) wrap 0x13;
    [0x9aec]--;
    return (*(int *)(&0x55C90 + [0x9af4]*4)) + 1;   // value + 1
```

so it is a 20-entry (`0x13`) ring of dwords at `0x55C8C`/`0x55C90` with
head `[0x9AF0]`, tail `[0x9AF4]` and count `[0x9AEC]`; the pop returns
`value + 1` (0 = empty). `FUN_00064ec0` (FU-109 §4) pops from it each frame
while the mode is not the replay family — the queue carries pending replay
commands from live play into the mode machine.

## 3. `FUN_00064f70` — the device/session init (context)

`FUN_00064f70` (the next function) initialises the `0x55CE0` block: if
`[0x55CE0] == 0` it probes `FUN_00068cfc` (a count), `FUN_000a6265(-1)`,
`FUN_000a6a03`, `FUN_000cbbe8`, zeroes six `0x55CE8`/`0x55D18`/`0x55D00`
slots and calls `FUN_0006504c(1, k+1)` for six devices. It is the audio/input
device layer above the replay queue, cited for context only.

## 4. Closures / errata

* **FU-109 §7 leg 3 — closed.** `FUN_00064ef0` is the not-in-replay command
  ring; `FUN_00064ec0`'s hook pops it.
* **FU-109 §7 leg 4 — partially closed.** The packer's callers are in the
  `0x49B28`/`0x4A0xx` band; the ring eviction (`*[0x9ab8]++`,
  `*[0x9ac0]--`) is the recorder's own full-ring path (FU-109 §1).
* **FU-106 §3 — refined.** `FUN_0004a228` is the arming aggregator
  (camera init + ring reset + recorder enable); its entry is indirect.
* **FU-106 §7 leg 1 — confirmed.** The `[0x9a98] & 0x80` family hooks
  (`FUN_00063ff0`/`FUN_00064ec0`) bracket this queue.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_function_callers` 0x6408C (0),
0x63EBC (0), 0x64030 (1), 0x64074 (1), 0x64080 (4), 0x4A228 (0), 0x53DC4
(2); `get_xrefs_to` 0x6408C (2), 0x63EBC (1), 0x9A90 (3);
`decompile_function` 0x64EF0, 0x64F70. Address mapping as FU-88.
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 6. Open legs

1. **The `0x49B28`/`0x4A0xx`/`0x4AFxx` caller bodies** (which match phases
   arm and fill the ring; `FUN_00049b28` also calls the camera director
   helpers, FU-103 §3).
2. **The command ring's producers** (`[0x9AF0]`/`[0x9AEC]` writers besides
   `FUN_00064ef0`) and the consumers of the popped values (the mode machine
   dispatch?).
3. **`FUN_00064f70`'s device layer** (`FUN_0006504c(1..6)`, `FUN_00068cfc`,
   `FUN_000a6265`, `FUN_000cbbe8`) — likely the six-player input/audio
   devices, not replay.
4. **`FUN_0004a228`'s entry path** (no static caller) and the four gate-set
   call sites' arguments.
