# FU-108 — the replay ring buffer

Follow-on to FU-107 §7 leg 2 (the replay-queue helpers) and FU-106 §2 (the
`[0x9A94]`/`[0x9AC0]` countdown pair). This slice decomposes the 150-record
ring the mode driver records into and steps through.

Result in one line: **`FUN_00063ebc` allocates/frees the replay buffer
(`[0x9AB0]`, header `+0`/`+4`/`+8`/`+0xC` → `[0x9AB8]`/`[0x9ABC]`/`[0x9AC0]`/
`[0x9AB4]`), `FUN_00063cbc` appends a record at
`([0x9A94] + *[0x9AB8]) mod 0x96` into the `0x118`-byte slot array at
`[0x9AB4]` (mode 0x82 when full), `FUN_00063d34` reads the current record's
`+0x116` word, `FUN_00063e54` steps back, and `FUN_00063b80` packs the live
state into the record.**

## 1. The buffer (`FUN_00063ebc`)

```
FUN_00063ebc(EAX = size):
  if (size == 0):
      if ([0x9ab0] != 0) { FUN_000993ec([0x9ab0]); [0x9ab0]=0; [0x9ab8]=0; [0x9ab4]=0;
                           [0x9a94]=0; [0x9abc]=0; [0x9aac]=0; [0x9a98]=0; [0x9ac0]=0; }
  else:
      [0x9ab0] = FUN_0004a448(size, &0xA41C);
      if ([0x9ab0] != 0) { [0x9ab8] = [0x9ab0]; [0x9abc] = +4; [0x9ac0] = +8; [0x9ab4] = +0xC; }
```

so the header at `[0x9AB0]`: `+0` start index (`*[0x9AB8]`), `+4` `[0x9ABC]`,
`+8` record count/limit (`*[0x9AC0]`), `+0xC` the `0x118`-byte record array
(`[0x9AB4]`). `[0x9A94]` is the current index, `[0x9AAC]` an extra field.

## 2. The record engine

* **append — `FUN_00063cbc`**:
  ```
  if ([0x9a94] < *[0x9ac0]) {
      i = [0x9a94] + *[0x9ab8]; if (0x95 < i) i -= 0x96;      // 150-slot ring
      FUN_00063b80(EAX = live state, [0x9ab4] + i*0x118);      // pack one record
      [0x9a94]++;
      if (*[0x9ac0] <= [0x9a94]) { FUN_000974d8(); [0x9a98] = 0x82; }  // full -> idle
  }
  ```
* **current stamp — `FUN_00063d34`**: returns the word at
  `[0x9ab4] + i*0x118 + 0x116` (0 when past the count) — the value the mode-0x83
  countdown compares against `[0x9AA8]` (FU-107 §4).
* **step back — `FUN_00063e54`**: while `[0x9a94] > 0` decrements the index
  and re-applies the record slot; at `< 1` sets mode 0x82.
* **packing — `FUN_00063b80(EAX = dest? , EDX = ring slot)`**: copies the
  first three dwords, the bytes at source `+0x0A/+0x66/+0x96/+0x31/+0xDB`,
  six dwords from `+0xF6`, the word `+0x116`, six bytes from `+0x10E`, and a
  per-field 16-bit high-word pass (`>>0x10` of `+0x0A`, `+0x66`, `+0x38*4`).
  The source/destination roles (EAX vs EDX) are only partly resolved (open
  leg) — one of the pair is the live camera/entity state, the other the
  `0x118` record.
* **`FUN_0006428c`**: `if ([0x55C4C] >> 24 != 0) FUN_000974dc();` — a re-sync
  hook called by the mode-0x83 loop.

## 3. `FUN_00064AA4` — the replay input word

The decompiler truncated its jump table, but the head is clear:

```
FUN_00065920(); FUN_00065920(); FUN_000125c4(); FUN_000136b4();
uVar3 = FUN_00016350(...);
uVar1 = 0;
if ([0x9acc] == 1) uVar1 = 8;      // the FU-107 pan bits
if ([0x9acc] == 2) uVar1 = 4;
if ((int)uVar3 >= 0) { FUN_00065cc0(); ... }
switch (uVar3) { ... }             // open (jump table lost)
if ([0x9a8c] == 5) {               // the FU-107 six-frame cursor
    FUN_00037778()/FUN_00037748()×2/FUN_0003773c()×2 -> OR 4 / OR 0x40
}
return uVar1;
```

so the replay input combines `[0x9ACC]` (1→bit 3, 2→bit 2), the button
helpers `FUN_0003773c`/`FUN_00037748`/`FUN_00037778`, and the mode's own
`[0x9A8C] == 5` frame — matching the `[0x9AA4]` bit map FU-107 §3 deduced.

## 4. Closures / errata

* **FU-107 §7 leg 2 — closed** (the queue step/reset calls are
  `FUN_00063cbc`/`FUN_00063d34`/`FUN_00063e54`, all ring-slot operations).
* **FU-106 §2 — refined.** The `[0x9A94] < *[0x9AC0]` test is *index < record
  count* (not a time countdown); `[0x9AA8]` is the tick accumulator the
  mode-0x83 loop drains in `FUN_00063d34` stamp units.
* **FU-107 §7 leg 1 — partially closed.** `FUN_00064AA4`'s input sources are
  named; its switch is lost to the decompiler (open).
* **Mode-0x82 writes** — both the full-buffer path (`FUN_00063cbc`) and the
  step-back empty path (`FUN_00063e54`) return mode 0x82, consistent with
  FU-107's state table.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x63CBC, 0x63D34,
0x63E54, 0x63EBC, 0x6428C, 0x64AA4, 0x63B80; `search_functions`
`FUN_00063`/`FUN_00064` (23 + 18 functions). Address mapping as FU-88.
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 6. Open legs

1. **`FUN_00063b80` source/destination roles** and the record field map
   (`0x118` bytes; reads `+0x0A/+0x31/+0x66/+0x96/+0xDB/+0xF6/+0x10E/+0x116`).
2. **`FUN_0004a448`** (allocator) and the `&0xA41C` descriptor;
   `FUN_000993ec` (free), `FUN_000974D8`/`FUN_000974DC` (re-sync).
3. **`FUN_00064AA4`'s jump table** and `FUN_00065920`/`FUN_000125C4`/
   `FUN_000136B4`/`FUN_00016350`/`FUN_00065CC0` (input layer).
4. **`[0x9AB8]`'s first field** (`*[0x9AB8]`, the start index) and
   `[0x9ABC]`/`[0x9AAC]` writers.
5. **The other `0x63xxx` functions** (`FUN_00063fd0`/`0x63fdc`/`0x63ff0`
   `[0x9A98]` readers, `FUN_00063734`, the `0x631xx..0x63xxx` band).
