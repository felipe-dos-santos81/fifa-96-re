# Open-items closure, Slice A — live replay/audio seams and inert proofs

Date: 2026-11-12
Status: draft for approval
Campaign: close the remaining open/blocked items after the recorded-frame
replay ring. Slice A closes the live small legs and the provably inert items;
Slice B (the 19-site `0x8ED40` camera/stats chain, B2-L1 and RR-L7) and
Slice C (the `FUN_0003BB1C` match-screen UI loop) are separate follow-ons.

## 1. Intent and why

The ring landed (`125/125`, M1/M2 exact). What remains is a mixed register:
some legs are live in the port's current contract, some are provably inert in
the analyzed image, and two are large unported paths. Treating them as one
"finish everything" campaign would either fake the inert items or move the
tape on the camera/UI paths without a separate decision.

Slice A draws that line explicitly:

- close the live replay/audio seams that the port can reach today;
- close the inert items with fresh evidence, append-only;
- hand the 19-site camera/stats chain and the UI loop to B/C with their
  evidence and gates already named.

No behavior outside the replay/audio seams changes; no golden is re-pinned.

## 2. Goals

- Close RR-L10's real half: `FUN_000974DC` is not a no-op; model its
  replay-family gate and its observable.
- Close C4-L1: prove the native `0x2B` channel identity and the restore, or
  record the adapter with the evidence that the native mapping is unproved.
- Close RR-L9 by proof: `0x15887B` has no static writer, so the port's staged
  0 is the only defensible value.
- Close the 23rd remap-pool slot by proof: `0x14BF9C` has no static xref.
- Close the `FUN_0001CAEC` option-UI / A4 branch by proof: the two option
  cells have no static store and the non-default branch is team-editor
  preview (out of contract).
- Close or honestly re-label RR-L1/L2/L6/RR-L8 against the embedded-ring
  design and the FU-111 phase model.
- Keep `make check` 125/125, M1/M2 byte-exact, smoke exit 124, and the
  completion numbers unchanged.

## 3. Non-goals and anti-goals

Non-goals:

- The 19 `0x8ED40` sites in the camera/announce chain, the row-1C tail and
  the `0x8A938` `[0x157A9B]` arm — **Slice B**. RR-L3 (camera band/queue
  producers) and RR-L4 (appearance-byte mapping) are B too; they are the
  chain's producer/view facts and have no live seam in A.
- B2-L1 (the non-zero `side_swap` flip) — **Slice B**, because every live
  writer (`0x4B0DE` in `FUN_0004B02C`, `0x4BA20/31` in `FUN_0004BA00`,
  `0x8B58A/0x8B61B` in `FUN_0008AF38`) is in the camera/period cluster.
- RR-L7 (mode-7 request/timed exit) — **Slice B**.
- The `0x20` replay camera record cycle / matrix precedence (RR-L5) —
  **Slice B**, because it consumes the camera-record selection B ports.
- The full `FUN_0003BB1C` match-screen loop and its screen table — **Slice C**.
- The team-editor preview.

Anti-goals:

- Inventing a producer for `0x15887B`, `0x14BF9C`, `0x105284`/`0x105298`, or
  `[0x14E574]` to make a leg look closed.
- Re-pinning M1/M2 to absorb an audio/announce seam.
- Moving a completion number or adding a dispatch body/row.

## 4. Evidence (fresh Ghidra, `/FIFA96.EXE`, 2026-11-12)

### 4.1 The remaining `0x8ED40` sites are B

Fresh `get_xrefs_to 0x736AC` = 2: `0x4B193` in `FUN_0004B100` and `0x743EF`
inside the chain. `FUN_0004B100` is called twice from the match loop
`FUN_000493A0`; the camera chain is therefore live, not inert. The chain is
`0x736AC → 0x7131C → 0x70DE0` and `0x736AC → 0x72AC4 → 0x72478`
(`get_xrefs_to` each = 1). The 14 camera sites are live behind it; the four
row-1C tail sites (`0x77DFD/0x77E38/0x77E4E/0x77E61`) sit in the region
Ghidra has not created as a function (`get_function_by_address 0x779DE` =
none), and `0x8AD54` is the `[0x157A9B]` arm in `FUN_0008A938`.

Fresh `get_xrefs_to 0x157A9B`: 3 WRITE in `FUN_00088940`, READ in
`FUN_0004BB5C` and `FUN_0008A938`; the decompiled case 6 shows
`FUN_0008ECC4(piVar3,iVar5)` when `[0x157A9B] == NULL`, else
`FUN_0008ED40(piVar2,-0x96)`. The arm is live on a goal event, so it belongs
to B with the other 18.

### 4.2 The UI loop is C

Fresh `get_xrefs_to 0x3BB1C` = 3: `0x38004` (live event dispatch, called from
`0x49F14` in `FUN_00049B28`), `0x38630` (front-end) and `0x3B708`. The
`0x8FF30` third-selection consumer is a branch in the screen-table code at
`0x8FF30..0x8FF57` (`CMP dword[0x5B358],0 / JZ`, then announce id `0x38`);
the following entry `0x8FF58` is a function pointer at `0x110C70`. That table
is the match-screen loop's, so the consumer belongs to C.

### 4.3 The inert items are provable

- `get_xrefs_to 0x14BF9C` = **0**. The 23rd remap-pool block has no static
  consumer; the live tables are reached through the runtime pointer pair
  `0x14BF60`/`0x14BF8C` (FU-348 §8).
- `0x105284`/`0x105298` (the `FUN_0001CAEC` case-9/-D option cells) have only
  the four `CMP` reads each and image value 0; the non-default A4 branch is
  the team-editor preview (`FUN_000481EC` writer, `0x372B4` path), out of
  contract by the small-legs design §3.
- `get_xrefs_to 0x15887B` = 1 READ (`0x36E38` in `FUN_00036C70`), no writer.
  RR-L9's "staged 0" is exact for the analyzed image.
- `0x974D8` is a one-byte `RET` (`disassemble_bytes 0x974D0` shows
  `MOV EAX,1; LEA EAX,[EAX]; RET`); the port's `replay_transition` no-op is
  faithful.
- `0x974DC` is **not** a no-op: it calls `FUN_00064EC0` and `FUN_00065CC0`.
  `FUN_00064EC0` gates on `([0x109A98] & 0x80) == 0` and calls
  `FUN_00064EF0(1, ...)`; `FUN_00065CC0` updates `[0x10A254]` from the mixer
  when `[0x155CE0]/[0x155D30]/[0x155D00]/[0x10A250]` are non-zero. In the
  replay family the first gate is false, so the observable is the mixer cell.

### 4.4 The remaining ring legs

- RR-L1 (allocator): the embedded 150 × `0x118` ring is the chosen design;
  the native allocator lifecycle is out of contract. Close the leg as a
  design decision, not a missing behavior.
- RR-L2 (overlay gating): `[0x14E574]` is the presentation-overlay latch
  (`FUN_00053DC4`/`FUN_00053A48`). It gates a presentation-overlay recorder,
  not the live match frame; the port records every live ACTIVE frame while
  the mode-window budget allows. Re-read the two functions and record the
  out-of-contract closure.
- RR-L6 (tick source): `FUN_000492CC` = `[0x10732C]`; the port uses 1 per
  granted replay frame and `state.frame_delta` for the view's `+0x1CE`.
  Record the mapping; the exact native frame-rate reconciliation is
  observation-only and stays a note, not a code leg.
- RR-L8 (phase mapping): the port's `state.phase` is the `[0x157A4A] >> 24`
  model; the increment/reset sets are FU-111's. Record the mapping and the
  one rule: if a live phase is re-mapped, the budget gate follows the mode
  byte.

## 5. Approaches

1. **Proof-first closure (one slice).** Census, append-only proof closures,
   no code. Fastest, but leaves the two real live seams (RR-L10, C4-L1)
   unported.
2. **Full native path (3–4 slices).** Port the camera chain, mode 7, the UI
   loop, and wire all 19 sites. True closure, but camera/UI tape movement and
   a golden re-pin.
3. **Two-track (chosen).** Slice A closes the live small legs and the inert
   proofs; Slice B takes the camera/stats chain; Slice C takes the UI loop.
   Each gets its own design and gate.

## 6. The design (Slice A)

### 6.1 RR-L10 — the `0x974DC` resync seam

The port keeps `replay_transition` (the `FUN_000974D8` call site) as the
faithful one-byte `RET` no-op. `replay_resync` is no longer a no-op:

- Add a run cell `replay_resync_pending` (or reuse the existing replay-cell
  naming) set by `replay_resync(mr)` after each countdown step.
- Model the `FUN_00064EC0` gate: when the replay family is **not** set,
  set `replay_resync_pending = 1` (the `FUN_00064EF0(1,...)` request); in the
  replay family the gate is false and the cell is not set.
- Model the `FUN_00065CC0` observable as a run cell only if the mixer cell
  `[0x10A254]` has a ported consumer; otherwise record it as a numbered
  observability leg with the fresh evidence. Do not invent a mixer write.
- Tests: in `PLAYING`, one countdown step sets no `replay_resync_pending`
  while the family is set; a direct `replay_resync` on a non-family run sets
  it; `init`/`begin`/`end` clear it. Keep `test_replay_step_machine` green.
- The `0x974D8` RET is pinned by the disassembly evidence, not by code.

### 6.2 C4-L1 — the `0x2B` channel identity

- Fresh-read the native mixer channel map around `0x2B` (the stop call and
  the restore path). **Decision:** native fidelity replaces the adapter only
  when **both** the `0x2B` identity and the native restore are proved. If
  either is unproved, the crowd-loop adapter stays and C4-L1 is recorded as
  a documented leg with the evidence; audible behavior never changes on an
  unproved channel.
- Either way the engine behavior is unchanged: stop the crowd on arm, do not
  re-arm while ducked, restore on release. The test surface stays
  `test_engine_audio`/`test_engine_match_bridge`; add a fixture assertion for
  the exact channel only if both halves are proved.
- No golden movement: the headless/no-bank path stays silent.

### 6.3 Proof closures (append-only)

Append a closure section to the owning records:

- FU-348 §8 / FU-350 §7: the 23rd slot has no static xref; the runtime pair
  is the only reach. Close A1.
- FU-348 §8 / small-legs design §10: the option cells have no store; the A4
  non-default branch is team-editor preview. Close FU348-L2d as
  out-of-contract.
- FU-108/FU-109/FU-112/RR records: RR-L1, RR-L2, RR-L6, RR-L8, RR-L9 and the
  `0x974D8` half of RR-L10 as above. Keep RR-L5/RR-L7 for B and the UI loop
  for C.
- `port/ENGINE.md` and `docs/PROGRESS.md`: one closure entry with the fresh
  evidence and the B/C handoff.

### 6.4 What moves to B and C

- **B:** 19 `0x8ED40` sites (14 camera chain, 4 row-1C tail, 1 `0x8A938`
  `[0x157A9B]` arm), B2-L1, RR-L3 (the camera band/queue producers), RR-L4
  (the appearance-byte mapping), RR-L5 (`0x20` cycle/matrix precedence), and
  RR-L7 (mode-7 request/timed exit). RR-L3/RR-L4 are camera-chain/composer
  facts; B is their owner because B ports the chain and the view consumer.
- **C:** `FUN_0003BB1C`, its screen table (`0x110C70`), and the `0x8FF30`
  third-selection consumer.

## 7. Tests and gates

- `make check` 125/125, strict-clean.
- M1/M2 `cmp` byte-exact; no re-pin.
- Windowed smoke exit 124 with a clean log.
- New tests only for the RR-L10 cell and (if proved) the C4 channel; the
  proof closures are docs-only.
- No README/port-table/census/ENGINE headline/completion-number movement.

## 8. What is left to the builder

- The exact run-cell name for the resync observable and whether the mixer
  cell needs a consumer in this slice.
- Whether the `0x2B` channel identity is provable from the fresh mixer read;
  if not, the adapter stays and the closure record says so.
- The append-only wording for each owning record; the design gives the facts,
  not the prose.

## 9. Provenance

Fresh Ghidra MCP on `/FIFA96.EXE` (2026-11-12): `get_xrefs_to` 0x736AC (2),
0x7131C (1), 0x72AC4 (1), 0x72478 (1), 0x3BB1C (3), 0x14BF9C (0),
0x157A9B (9), 0x157AC2 (56), 0x15887B (1), 0x974D8 (18), 0x974DC (42),
0x8FF58 (1 DATA at 0x110C70); `get_function_by_address` 0x8FF30 (none),
0x779DE/0x77E8B (none), 0x8A938, 0x974D8, 0x974DC, 0x8F0C4;
`disassemble_bytes` 0x8FED0 (160 B), 0x8FF58 (64 B), 0x974D0 (40 B);
`decompile_function` 0x8A938, 0x974DC, 0x64EC0, 0x65CC0; `get_xrefs_to`
0x64EC0 (1), 0x65CC0 (46). Prior records: FU-348 §8, FU-350 §7, FU72,
FU-351 §26/§27, design §7.8/§7.9.
