# Open-items closure, Slice A — reachability proofs and misattribution corrections

Date: 2026-11-12
Status: draft for approval
Campaign: close the remaining open/blocked items after the recorded-frame
replay ring. Slice A is the **proof/erratum slice**: it closes the inert or
misattributed items and names the live code paths for Slice B (the camera/
stats chain, B2-L1, RR-L3/L4/L5/L7 and RR-L10's real hook) and Slice C (the
`FUN_0003BB1C` match-screen UI loop and its screen table).

## 1. Intent and why

The ring landed (`125/125`, M1/M2 exact). The remaining register is not one
kind of work. Some items are inert in the analyzed image; one is a
misattribution; the rest are live code paths that need the camera chain or
the UI loop. Porting the live paths in the same slice as the proofs would
either move the tape or fake the inert items.

Slice A draws that line explicitly:

- close the inert items with fresh evidence, append-only;
- correct the C4 audio misattribution with the decompiled call chain;
- reconcile the replay tick source (RR-L6) if the live writers prove it, or
  keep it as a numbered leg;
- hand every live camera/UI path to B/C with its evidence and gate named.

No engine behavior changes; no golden is re-pinned.

## 2. Goals

- Close C4-L1 as a **misattribution**: `FUN_0005865C` (called from the replay
  arm `FUN_00064270` at `0x6427F`) is a screen/projection reset, not a mixer
  channel stop.
- Close RR-L9 by proof: `0x15887B` has one static READ (`FUN_00036C70`) and
  no static writer.
- Close the 23rd remap-pool slot by proof: `0x14BF9C` has zero static xrefs.
- Close the `FUN_0001CAEC` option-UI / A4 branch by proof: the two option
  cells have four READs each in `FUN_0001CAEC` and no static store; the
  non-default branch is the team-editor preview (out of contract).
- Close the `0x974D8` half of RR-L10 by proof (a one-byte `RET`).
- Re-label RR-L1 (embedded ring by design), RR-L2 (presentation-overlay
  latch) and RR-L8 (phase mapping) with the fresh xref census.
- Reconcile RR-L6 against the live `0x10732C` writers (`0x49B28`/`0x4A068`);
  if the value is not proved, RR-L6 stays a numbered leg.
- Keep `make check` 125/125, M1/M2 byte-exact, smoke exit 124, and the
  completion numbers unchanged.

## 3. Non-goals and anti-goals

Non-goals (all **Slice B** unless noted):

- The 19 `0x8ED40` sites: 14 in the `0x736AC` camera chain, 4 in the row-1C
  tail, 1 the `0x8A938` `[0x157A9B]` arm.
- B2-L1 (the non-zero `side_swap` flip), RR-L3 (camera band/queue
  producers), RR-L4 (appearance-byte mapping), RR-L5 (`0x20` replay camera
  cycle/matrix precedence), RR-L7 (mode-7 request/timed exit).
- RR-L10's `0x974DC` half: the hook pushes the `0x155C8C` queue and updates
  the `0x10A254` scalar through `FUN_00065CC0`; its semantics are unproved
  and it is a camera/UI-path fact.
- The replay arm's `0x5865C` screen/projection reset (`FUN_00056B88(0x2B)`
  plus `FUN_00056894`); it is the actual native behavior at the arm, newly
  named by this slice.
- **Slice C:** `FUN_0003BB1C`, its screen table (`0x110C70`), and the
  `0x8FF30` third-selection consumer.
- The team-editor preview.

Anti-goals:

- Inventing a producer for `0x15887B`, `0x14BF9C` or `0x105284`/`0x105298`
  to make a leg look closed.
- Re-pinning M1/M2 to absorb a proof/erratum slice.
- Moving a completion number or adding a dispatch body/row.

## 4. Evidence (fresh Ghidra, `/FIFA96.EXE`, 2026-11-12)

### 4.1 The camera chain is live; the 19 sites are B

Fresh `get_xrefs_to 0x4B100` = 3: `0x4951E` in the match loop
`FUN_000493A0`, `0x49FBB` in the frame event dispatch `FUN_00049B28`, and
`0x4A25A` in the match setup `FUN_0004A228`. `get_xrefs_to 0x4B02C` = 2:
`0x49415`/`0x4950A`, both in `FUN_000493A0`. The chain is
`0x736AC → 0x7131C → 0x70DE0` and `0x736AC → 0x72AC4 → 0x72478`
(`get_xrefs_to` each = 1). The bodies run in the native match path; the
port's mode-7/replay subset is RR-L7, and the per-site reachability inside
the chain is B's census, not an A claim.

The four row-1C tail addresses (`0x77DFD/0x77E38/0x77E4E/0x77E61`) have no
Ghidra function (`get_function_by_address` = none for each), so the tail is
B's port target. `0x8AD54` is the `[0x157A9B]` arm in `FUN_0008A938`:
`get_xrefs_to 0x157A9B` shows 3 WRITE in `FUN_00088940`, READ in
`FUN_0004BB5C` and `FUN_0008A938`; the decompiled case 6 calls
`FUN_0008ECC4` when `[0x157A9B] == NULL`, else `FUN_0008ED40(-0x96)`.
That arm is live and belongs to B.

### 4.2 The UI loop is C

Fresh `get_xrefs_to 0x3BB1C` = 3: `0x38004` (live event dispatch, called
from `0x49F14` in `FUN_00049B28`), `0x38630` (front-end) and `0x3B708`.

The `0x8FF30` third-selection consumer is a branch at
`0x8FF30..0x8FF57` (`CMP dword[0x5B358],0 / JZ`, then announce id `0x38`);
the next entry `0x8FF58` is the target of the single DATA xref at
`0x110C70`. The table's dispatcher is **not** statically referenced
(`get_xrefs_to 0x110C70` = 0), so ownership of the table is unproved. Slice C
owns the table census; the consumer is assigned there by screen-table
proximity, not asserted as the `FUN_0003BB1C` loop's.

### 4.3 The inert items are provable

- `get_xrefs_to 0x14BF9C` = **0**. The 23rd remap-pool block has no static
  consumer; the live tables are reached through the runtime pointer pair
  `0x14BF60`/`0x14BF8C` (FU-348 §8, prior record).
- `get_xrefs_to 0x105284` = 4 READs and `0x105298` = 4 READs, all in
  `FUN_0001CAEC`; no store. The non-default A4 branch is the team-editor
  preview (`FUN_000481EC` writer, `0x372B4` path), out of contract by the
  small-legs design §3 (prior record).
- `get_xrefs_to 0x15887B` = 1 READ (`0x36E38` in `FUN_00036C70`), no
  static writer. RR-L9's staged 0 is exact for the analyzed image; an
  indirect/runtime writer remains unproved and is not invented.

### 4.4 C4-L1 is a misattribution (fresh decompile)

`FUN_0005865C` is called from the replay arm `FUN_00064270` at `0x6427F`.
Its body sets `[0x109014] = 0`, calls `FUN_00056B88(0x2B)`, and when
`[0x14E9B8] > 0x7F` divides the 0x5C-byte array at `0x14E9B8` by 3 and calls
`FUN_00056894`.

Fresh `decompile_function 0x56B88`: it writes a 12-dword table at
`0x1543A4` from `param_1` and `-param_1`, copies it to `0x14E838`, adjusts
the six `0x14E8xx` cells, and calls `FUN_0004C478` over
`0x14E868..0x14E9B8`. Fresh `decompile_function 0x56894`: it computes a
12-value matrix from `param_2/param_3 << 8` and the `FUN_000A1A60` outputs
and writes the `0x1543A0` array. These are screen/projection transforms, not
mixer channels. The C4 record's "`FUN_00056B88(0x2B)` (a channel stop)" and
"0x5C-byte channel-volume array" are errata.

Consequence: the engine audio duck is an **adapter** with no native audio
gate at the replay arm. C4-L1 is closed as a misattribution, not as a proved
`0x2B` channel. The arm's real `0x5865C` screen/projection reset is a new
B leg (the replay view setup).

### 4.5 RR-L10: one half proved, one half B

- `FUN_000974D8` is a one-byte `RET` (`disassemble_bytes 0x974D0` shows
  `MOV EAX,1; LEA EAX,[EAX]; RET`); the port's `replay_transition` no-op is
  exact.
- `FUN_000974DC` calls `FUN_00064EC0` and `FUN_00065CC0`.
  `FUN_00064EC0` gates on `([0x109A98] & 0x80) == 0` **and**
  `FUN_00037AE4() == 0`; `FUN_00037AE4` returns 1 when the screen id
  `[0x105FFC]` is 5 or 7 (`FUN_00036BC8` is `param == [0x105FFC]`). On the
  gate it calls `FUN_00064EF0(1, ...)`, a push into the 0x14-entry `0x155C8C`
  queue (`0x109AF0/0x109AEC/0x109AF4`).
  `FUN_00065CC0` updates `[0x10A254]` through `FUN_000A7705(param_1,
  [0x155D00])` when `[0x155CE0]/[0x155D30]/[0x155D00]/[0x10A250]` are
  non-zero.
  In the replay family the first gate is false, so the hook is the queue/
  scalar path, not an audio stop. The exact semantics stay unproved; B owns
  the camera/UI consumer.

### 4.6 RR-L6 has live writers

`get_xrefs_to 0x10732C` = 10. `FUN_000492CC` reads it (the port's
`+0x1CE` stand-in); the writers are `0x492FD` in `FUN_000492E8`, `0x49B86`
and `0x49BC2` in `FUN_00049B28`, and `0x4A08A`/`0x4A0C9` in
`FUN_0004A068`; readers sit in the same three functions. The port's
"1 per granted replay frame, `frame_delta` in the view" is a model, not yet
a reconciliation. The slice reads those writes; if they prove the model, it
is pinned with a test; if not, RR-L6 stays a numbered leg.

### 4.7 RR-L1/RR-L2/RR-L8

- RR-L1: the embedded 150 × `0x118` ring is the chosen design; the native
  allocator lifecycle is out of contract, not a missing behavior.
- RR-L2: `get_xrefs_to 0x14E574` = 35, spanning
  `FUN_000513EC`, `FUN_000537F8`, `FUN_00053DC4`, `FUN_00053DE0`,
  `FUN_00052030`, `FUN_00053A48`, `FUN_00052D68` and `FUN_00053BB8`. It is
  the presentation-overlay latch cluster; the slice re-reads the two named
  functions before deciding whether it gates the live match recorder. No
  "out of contract" assertion is made before that read.
- RR-L8: the port's `state.phase` is the `[0x157A4A] >> 24` model and the
  increment/reset sets are FU-111's. Re-label it as a documented mapping,
  with the rule that a re-mapped live phase follows the mode byte, not the
  phase table.

## 5. Approaches

1. **Proof-first closure (one slice).** Census, append-only proofs and the
   C4 erratum; no engine code. Fastest, but leaves the live camera/UI paths
   unported.
2. **Full native path (3–4 slices).** Port the camera chain, mode 7, the UI
   loop and wire all 19 sites. True closure, but camera/UI tape movement and
   a golden re-pin.
3. **Two-track (chosen).** Slice A closes the inert/misattributed items and
   reconciles the tick source; Slice B takes the camera/stats chain; Slice C
   takes the UI loop. Each gets its own design and gate.

## 6. The design (Slice A)

### 6.1 C4-L1 — the misattribution erratum (docs-only)

Append a quoted erratum to the C4 record (FU-351 §26 and the small-legs
design §11.4/§11.5):

- `FUN_0005865C` is called from the replay arm, but it is a screen/projection
  reset: `FUN_00056B88(0x2B)` writes the `0x1543A4`/`0x14E838` 12-dword
  table and the `0x14E868..0x14E9B8` block; `FUN_00056894` computes the
  `0x1543A0` 12-value matrix; the `0x14E9B8` 0x5C-byte array is divided by
  3, not a channel-volume array.
- The engine audio duck is an adapter, not a native `0x2B` channel. C4-L1 is
  closed as a misattribution; no code changes.
- The actual native arm behavior (the `0x5865C` view reset) is a new B leg,
  named `C4R-L1` in the B design.

### 6.2 RR-L9 — the ball frame byte (docs-only)

`0x15887B` has one static READ (the composer `FUN_00036C70`) and no static
writer. Append the proof and close RR-L9: the port's staged 0 is exact for
the analyzed image; an indirect writer remains unproved.

### 6.3 The 23rd slot and the option UI (docs-only)

- `0x14BF9C` has zero xrefs. Append the proof to FU-348 §8/FU-350 §7 and
  close the 23rd-slot leg (small-legs A1).
- `0x105284`/`0x105298` have four READs each and no store. Append the proof
  and close FU348-L2d as the team-editor preview (out of contract).

### 6.4 RR-L1/RR-L2/RR-L8 (docs-only)

- RR-L1: close as "embedded ring, out of contract", not a fidelity claim.
- RR-L2: re-read `FUN_00053DC4`/`FUN_00053A48` and the 0x14E574 census;
  record whether the latch gates the live match recorder. If it does, the
  item moves to B; if it is presentation-only, close it.
- RR-L8: re-label as the documented phase mapping with the mode-byte rule.

### 6.5 RR-L6 — the tick source (read; code only if proved)

- Read the `0x10732C` writes in `FUN_00049B28` (`0x49B86/0x49BC2`) and
  `FUN_0004A068` (`0x4A08A/0x4A0C9`) and the `FUN_000492E8` write.
- If they prove the port's 1/frame + `frame_delta` model, add one test
  pinning the replay tick and close RR-L6.
- If not, append the evidence and keep RR-L6 as a numbered leg. Do not
  invent a tick.

### 6.6 RR-L10 (docs-only for A)

- Append the `0x974D8` `RET` proof; the port's `replay_transition` no-op is
  exact.
- The `0x974DC` queue/scalar hook moves to B with the camera/UI consumer.
  No code in A.

### 6.7 B/C handoff

- **B:** 19 `0x8ED40` sites; B2-L1; RR-L3/L4/L5/L7; RR-L10's `0x974DC`
  half; the arm's `0x5865C` view reset (C4R-L1); **RR-L2 only if** the
  re-read proves the `0x14E574` latch gates the live match recorder (if it
  is presentation-only, it closes in A).
- **C:** `FUN_0003BB1C`, its screen table (`0x110C70`) and the `0x8FF30`
  third-selection consumer.

## 7. Tests and gates

- `make check` 125/125, strict-clean.
- M1/M2 `cmp` byte-exact; no re-pin.
- Windowed smoke exit 124 with a clean log.
- New tests only if RR-L6 is proved (one tick test). The C4/ball/slot/option
  closures are docs-only.
- No README/port-table/census/ENGINE headline/completion-number movement.

## 8. What is left to the builder

- The RR-L6 read: whether the `0x10732C` writes prove 1/frame plus
  `frame_delta`, or leave a numbered leg.
- The RR-L2 re-read: whether the latch gates the live match recorder.
- The append-only wording for each owning record; the design gives the
  facts, not the prose.

## 9. Provenance

Fresh Ghidra MCP on `/FIFA96.EXE` (2026-11-12): `get_xrefs_to` 0x4B100 (3),
0x4B02C (2), 0x736AC (2), 0x7131C (1), 0x72AC4 (1), 0x72478 (1), 0x3BB1C (3),
0x14BF9C (0), 0x157A9B (9), 0x157AC2 (56), 0x15887B (1), 0x974D8 (18),
0x974DC (42), 0x8FF58 (1 DATA at 0x110C70), 0x105284 (4), 0x105298 (4),
0x14E574 (35), 0x10732C (10), 0x64EC0 (1), 0x65CC0 (46), 0x56B88 (3),
0x56894 (3); `get_function_by_address` 0x8FF30 (none), 0x779DE/0x77E8B/
0x77DFD/0x77E38/0x77E4E/0x77E61 (none), 0x8A938, 0x974D8, 0x974DC, 0x8F0C4;
`disassemble_bytes` 0x8FED0 (160 B), 0x8FF58 (64 B), 0x974D0 (40 B), 0x4B100
(48 B), 0x4B02C (32 B); `decompile_function` 0x8A938, 0x974DC, 0x64EC0,
0x64EF0, 0x65CC0, 0xA7705, 0x37AE4, 0x36BC8, 0x56B88, 0x56894, 0x4C478,
0x5865C. Prior records: FU-348 §8, FU-350 §7, FU72, FU-351 §26/§27, design
§7.8/§7.9.
