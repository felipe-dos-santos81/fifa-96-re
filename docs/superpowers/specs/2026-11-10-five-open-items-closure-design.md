# Five open items closure — design

Date: 2026-11-10
Status: draft for approval
Campaign: close the five remaining engine-level open items after the set-piece
target chain (`5206bc7`).

**Open scope question (replay).** The design closes the replay trigger, state
machine, camera selection and overlay, and leaves the recorded-frame ring
advance (`0x63D34`/`0x63CBC`/`0x6428C`) and the live-match freeze as a separate
leg. Recommendation: keep that split; a recorded-frame playback is a distinct
feature, not the “UI trigger” leg.

## 1. Intent and why

The set-piece target chain closed the live path builder, interpolation, input
tail, camera override and delivery clears. Five items remain in the engine
record:

1. **FU275-L6 / `FUN_000786A0`** — the slot-block move is not modeled; row 18
   and the long-ball hand-off still leave `mr->slot.entity` and the slot dirs
   on the donor (FU315-L2, OL-68).
2. **Row 29** — the body is ported, but no live entry has been found
   (FU-142f/OL-48).
3. **`FUN_0008ED40`** — the weighted activity accumulator is ported as pure
   helpers, but its call sites are not wired, so replay source-4 selection only
   sees goal bumps.
4. **`0x515A6`/`0x3BB1C` replay UI trigger** — the replay state machine, camera
   selector and overlay exist, but nothing in the engine enters replay.
5. **`FUN_00047298`** — the full 23-descriptor roster/appearance pool; the port
   draws an 11-per-side projection.

Closing them makes the live match state internally consistent (exactly one
slot holder, one slot block), makes replay selection see native activity
counts, makes replay reachable, and makes the per-player appearance pass
native-exact.

## 2. Goals

- Model `FUN_000786A0` once and use it at every ported call site; after a move,
  `has_slot`, `mr->slot.entity`, the slot dirs and the merge tail agree.
- Settle row 29 with a fresh, first-hand census: either a wired entry and a
  test, or a documented negative closure of the same rigor as FU-142f.
- Wire `FUN_0008ED40` at every call site whose enclosing body is ported, with
  the native weight, table layout and max publish.
- Make replay enterable from the live match, driven by the native trigger
  state, with the existing `replay_step`/`replay_camera_set`/`replay_target`
  machine and the engine's pad input.
- Implement the native 23-block `FUN_00047298` remap pass for the live draw,
  including the template/pattern bands and the per-player skin/hair classes.

## 3. Non-goals and anti-goals

Non-goals:

- Porting the whole `FUN_0003BB1C` match-screen loop (menus, scoreboard, audio
  mix, goal screen). The replay call at `0x3BFED` is covered by the existing
  camera selector; the surrounding loop stays a separate UI slice.
- The three native `FUN_000786A0` callers with no engine body (`0x74CD2`,
  `0x75451`, `0x76053`) and the `FUN_00093890` slot↔record walk (`0x938F0`).
  The helper is ready for them; the caller bodies are not invented.
- The four-slot native array (`0x157C64`, stride `0x25`). The engine models the
  single human slot (player 0); the other three slots stay behind the existing
  FU-70 boundary.
- New stand-ins for behavior the native code does not prove.

Anti-goals (would make the campaign a failure even if tests pass):

- Two records holding the slot, or `mr->slot.entity` naming a record without
  `has_slot`.
- A stats table whose keys do not match the replay roster ids, so activity
  silently never reaches the selector.
- A replay trigger that bypasses the native state transitions (0x81/0x82/
  0x83/0x86/0x80).
- A 47298 port that only recolors the 11 visible players and leaves the
  pattern/template pass as an unmarked approximation.
- Moving M1/M2 goldens.

## 4. Constraints

- Work stays on `main`, one slice per spec/plan, commit and push per slice.
- Every slice ends with `make check`, M1 and M2 `cmp`, and the windowed smoke.
- `port/tests/golden/engine/m1-frames.txt` is immutable; `m2-frames.txt` only
  re-pins with a written reason and a frame diff.
- Native evidence is first-hand Ghidra (`/FIFA96.EXE`, standard tools). No
  scripts, no stale `/tmp` image, no caller-count claims without a fresh
  `get_xrefs_to`.
- Anything not statically proved is a numbered leg, never an asserted table.
- The engine layer must stay byte-identical on the non-live tapes. New state
  that is only read by live paths is safe; new state that reaches the draw or
  the clock must be live-gated.

## 5. Approaches considered

**A — Five sequential slices, live-first (chosen).** Each item gets its own
design section, plan, implementation, tests and commit. Order: slot move,
row-29 census, stats wiring, replay trigger, roster pool. Small blast radius,
independent verification, the live divergence lands first.

**B — Two grouped campaigns.** Match-state (slot + stats + row-29) then
presentation (replay + roster pool). Fewer design docs, but the first campaign
would change the slot model, a stats table and several handler bodies in one
review; regressions are harder to bisect.

**C — One foundation refactor.** Build a unified per-record state (slot block,
stats table, roster descriptors) once, then wire consumers. Removes three
refactors but has the largest blast radius and the weakest proof against the
goldens.

A is chosen because the repo's evidence and golden discipline rewards small,
provable slices, and because the slot move is the only item fixing a known live
gameplay divergence.

## 6. Slice 1 — slot-block move (`FUN_000786A0`)

### 6.1 Native contract

`FUN_000786A0` (`0x786A0..0x786EB`, 24 instructions; fresh disassembly):

```
0x786A2  EBX = [EAX+0x20]          ; from's slot block
0x786A5  if (EBX == 0) return
0x786A9  ECX = [EDX+0x20]          ; to must be free
0x786AC  if (ECX != 0) return
0x786B0  [EDX+0x20] = EBX          ; move the pointer
0x786B3  [EBX] = EDX               ; slot back-pointer follows
0x786B5  [EAX+0x20] = ECX          ; clear from
0x786BF  word[slot+4/6/8/0xA/0xC/0x14/0x16] = 0
```

Fresh `get_xrefs_to 0x786A0` = **9**:

| site | enclosing body | engine body |
|---|---|---|
| `0x74CD2` | `0x74CB8` dead helper (0 xrefs) | none |
| `0x75451` | keeper restart body (`0x7541C`/`0x7542E`/`0x7546E`) | not ported at this arm |
| `0x76053` | keeper restart body (`0x76030`/`0x76053`) | not ported at this arm |
| `0x7CDB4` | row `0x7CD60` long-ball handler | `match_input_handler` |
| `0x7D336` | `FUN_0007D1D4` control switch | `fifa96_match_run_select_step` |
| `0x838EF` | `FUN_00083428` set-piece receiver cut | `match_sp_receiver_cut` / probe |
| `0x83AA0` | `FUN_00083428` second receiver branch | `match_sp_*` (evidence task) |
| `0x84AD0` | row 18 duel hand-off | `fifa96_match_action_18` |
| `0x938F0` | `FUN_00093890` slot↔record walk | none |

`FUN_00078B00` (`0x78B00..0x78B14`) additionally clears slot `+0xA`, `+0x16`
and byte `+0x24`; `+0xA`/`+0x16` are already cleared by `0x786A0`, so the only
new effect is `+0x24`. The `0x7D1D4` binder calls it; the other sites do not.

### 6.2 Current engine state

- `struct fifa96_control_slot` (`fifa96_control.h`) is the single slot block:
  `entity`, input words, `map_select`, animation bytes, `byte24`.
- `mr->slot` is one block bound to player 0 (`fifa96_match_run.h:925`).
- `has_slot` is a per-record byte; `dir_x`/`dir_z` are per-record staging
  copies of `slot.anim_b`/`anim_c`.
- `fifa96_action_duel_swap` (`fifa96_action_handlers.c:971`) swaps only
  `has_slot`.
- Row 18 (`fifa96_match_handlers.c:2759`) swaps only `has_slot`.
- Row `0x7CD60` (`match_input_handler`) swaps `has_slot` and rebinds
  `slot.entity`, but does not run the tail or move the dirs.
- `fifa96_match_run_select_step` swaps, rebinds, runs the tail and clears
  `byte24`, but only when `slot.entity == id && has_slot != 0`.
- The set-piece cut does not move the slot.

### 6.3 Required behavior

One engine helper owns the move:

```c
int fifa96_match_run_slot_move(struct fifa96_match_run *mr,
                               int32_t from_id, int32_t to_id,
                               int clear_byte24);
```

Semantics:

- `-FIFA96_ERR_INVALID` on NULL; `0` on a native no-op (from is not the slot
  holder, to is already the holder or `to.has_slot != 0`, self move, or ids out
  of range); `1` on a move.
- **Authority:** `mr->slot.entity` is the native `[rec+0x20]` pointer;
  `has_slot` is the record-visible projection. The helper tests the pointer
  (`slot.entity == from_id`) and the destination's projection
  (`to.has_slot == 0 && slot.entity != to_id`), not the source's stale
  projection. On a move it sets `slot.entity = to_id`, `from.has_slot = 0`,
  `to.has_slot = 1`, so the two representations cannot disagree after the
  call. If `mr->record` names either record, update its `has_slot` and dirs
  too; the staged record copy must not outlive the move.
- On a move: keep `slot.anim_b`/`anim_c` (the block moves wholesale), run
  `fifa96_control_slot_merge_reset`, and clear `byte24` only when
  `clear_byte24`.
- Keep the per-record projection coherent: `to.dir_x/dir_z = slot.anim_b/c`
  when the moved block is the engine slot; `from.dir_x/dir_z = 0`.
- The helper does not create a second block: the engine has one human slot, and
  the native four-slot array is outside this slice.
- The live dispatch's forced `has_slot = 1` when `slot.entity == id` is a read
  projection, not a second owner. The helper heals it; the dispatch staging
  stays as is.

Use it at every ported call site:

- Row 18 replaces its local swap block (and must not keep a second
  `has_slot` write).
- Row `0x7CD60` replaces its swap + entity rebind; `clear_byte24 = 0`.
- `fifa96_match_run_select_step` replaces its swap + rebind + tail +
  `byte24` write; `clear_byte24 = 1`.
- The set-piece receiver cut (`0x838EF`) moves the taker's slot to the picked
  receiver before the `0x83937` camera-target write; `clear_byte24 = 0`.
- The second `FUN_00083428` site (`0x83AA0`) is wired only if the engine's
  corresponding branch exists; otherwise it is recorded with the other
  unported callers.

The three unported caller bodies (`0x74CD2`, `0x75451`, `0x76053`) and
`0x938F0` get no invented code. The spec records that the helper is the full
`0x786A0` body and that those callers remain outside the ported engine subset.

### 6.4 Tests and gates

- New engine test: move a slot from record A to B; assert `has_slot`, the
  `mr->slot.entity` back-pointer, the dirs, the merge reset (`pressed`,
  `released`, `held`, the reserved words) and `byte24`.
- No-op cases: from free, to held, self move, out-of-range ids.
- Row 18 integration: after the hand-off, the slot names the target and the
  donor is free; the follow/pan sees the moved dirs.
- Set-piece cut integration: the picked receiver holds the slot, the taker is
  free, and the camera target still uses the receiver.
- Select-step regression: the existing `0x7D1D4` tests still pass with the
  helper, including the no-op edge.
- **M2 rule:** the helper is called unconditionally at the native call sites,
  because the native action is not live-gated. The consumers that would move
  the M2 draw (camera follow, dir staging) are already live-gated. If the M2
  tape still moves, stop and report; do not re-pin it for this slice. The
  campaign rule “state reaching the draw/clock must be live-gated” applies to
  the consumer, not to the native state move.
- `make check`, M1/M2 exact, smoke.

### 6.5 Closure (2026-11-10) — **closed**

Slice 1 is closed. `fifa96_match_run_slot_move` is the full `0x786A0` body and
is adopted at row 18 (`0x84AD0`), `0x7CD60`, `0x7D1D4` (`clear_byte24 = 1`) and
the `0x838EF` set-piece receiver cut (`clear_byte24 = 0`);
`test_slot_move_helper` plus the row-18, `0x7D1D4` and receiver-cut assertions
pin the move, the no-op edges, the dirs and `byte24`. The three unported
callers (`0x74CD2`, `0x75451`, `0x76053`) are recorded here and get no code;
`0x938F0` (`FUN_00093890`) stays FU-70 and the second `FUN_00083428` site
`0x83AA0` has no distinct engine branch — none of them is counted as wired.

Commits: `45090c0` (helper), `034045d` (row 18 / `0x7CD60` / `0x7D1D4`),
`e9469f1` (receiver cut), plus this record commit. `make check` 124/124, M1/M2
byte-exact, windowed smoke clean. Record: FU-275 §4, the FU315 closure section,
`port/ENGINE.md` FU-351 §24.

### 6.5.1 Final-review closure / erratum (2026-11-11)

The closure above is retained as the phase record, but its “four adopted” and
“`0x83AA0` has no distinct engine branch” statements are superseded. Fresh
review evidence identifies `0x83AA0` as the return-to-taker hand-off. The
`b0 == 0` / `released & 0x20` / previous-low-nibble-zero branch now calls
`fifa96_match_run_slot_move(mr, mr->sp_recv_1587ac, mr->record.entity_id, 0)`
after the b0/b6 stores. The slot-block slice is therefore **5 adopted / 4
unwired**: adopted `0x84AD0`, `0x7CDB4`, `0x7D336`, `0x838EF`, `0x83AA0`;
unwired `0x74CD2`, `0x75451`, `0x76053`, `0x938F0` (FU-70).

The final wave also makes the helper single-owner (all other record projections
and `slot_merge` are cleared after a move), stores the receiver as a global id,
and repairs the post-cut receiver gate and `0x157AB0` outfield skip lifecycle.
The redundant row-18 staged `has_slot` write is removed. Tests cover the
receiver delivery `2`, the return-to-taker state, global team-1 ids, no-op
sentinel preservation, stale holders, pending merges, `clear_byte24 = 0`, and a
staged destination. Exact gate output is recorded in the phase-113 final-review
closure and the task report.

## 7. Slice 2 — row-29 entry closure

### 7.1 Native contract

- Action table `0x1106E0[0x29] = 0x874E4`; body `0x874E4..0x87738` (187
  instructions), ported as `fifa96_arm_29_step`.
- The body self-installs code `3` (`0x8753C MOV EDX,0x3`), never `0x29`.
- The only reference to `0x874E4` is the action-table slot; the only
  `MOV EDX,0x29` in the image is `0x1F53C` inside the non-match
  `FUN_0001F440`.
- FU-142f's census of all 77 `FUN_0007D9A4` call sites finds no `0x29`
  invocation.

### 7.2 Task

Run a fresh, first-hand negative/positive census:

- `get_xrefs_to 0x874E4` and `0x110784` (the table slot).
- `search_instructions` for `0x29` in every `FUN_0007D9A4` caller; scan for
  computed installs (`MOV EDX, [table]`, register-derived code).
- Re-read `FUN_0001F440` and the non-match installer paths.
- Re-read the row-29 body's self-install to confirm it writes `3`, not `0x29`.

If a live entry exists, wire it and add a dispatch test. If none exists, close
row 29 as **confirmed unreachable** with the evidence above, keep the body, and
state that it stays outside the wired count. No code change is expected on the
negative path; the deliverable is the evidence section and the closure record.

### 7.3 Tests and gates

- Negative: a spec section with the fresh calls and the same rigor as FU-142f;
  no test change required.
- Positive (if found): a dispatch test that reaches `0x874E4` and a census
  update (`wired` +1).
- `make check`, M1/M2 exact.

## 8. Slice 3 — `FUN_0008ED40` activity wiring

### 8.1 Native contract

`FUN_0008ED40(entity, value)` (71 instructions; FU-67 §3.6):

- Weight: `AL = (int8)[entity+0x8D]`, `AH = (int8)[team+0x9]`; if
  `AL > AH`, then `value += value >> 2` when
  `([entity+0x8A] >> 24) <= ([team+6] >> 24) + ([team+0xD] >> 24)`, else
  unchanged; if `AL <= AH`, `value *= 2`.
- Block: `xor = (side ^ [0x57ABE]) & 1`.
- Table: `0x5A998`, 2 blocks × 16 slots, stride `0xA`; key `[[entity+4]]`,
  word counter `+4`, dword value `+6`.
- On a hit: add the scaled value; if the sign-extended low word exceeds
  `[0x5AFB0]`, publish `[0x5AFB0]`/`[0x5AFB4] = [[entity+4]]` and
  `[0x5B434] = (word)xor`.
- The table is initialized by `FUN_00091BC4` from `[[0x4C360 + i*4]]` /
  `[[0x4C3DB + i*4]]`; the image keys are zero, so the runtime identity is the
  descriptor dword.

### 8.2 Current engine state

- `fifa96_stats_accumulate` and `fifa96_stats_weight` are ported and tested
  (`fifa96_ring_stats.c`), but no engine code calls them.
- The run has no stats table. `fifa96_match_run_replay_target` builds
  `activity.id[side][slot]` from `roster_order` and `activity.count` from
  `render.replay.activity_count`; only goal events bump those counts.
- The team struct starts at `+0x826`; the native team header bytes `+0x9` and
  `+0x10` (the weight thresholds) are not modeled. The native reads them as
  the high bytes of dwords `+0x6` and `+0xD`, i.e. `[team+6] >> 24` = byte
  `+0x9` and `[team+0xD] >> 24` = byte `+0x10`; the rank compare uses byte
  `+0x9`, and the class gate uses `+0x9 + +0x10`. The two-byte view is the
  same data as the dword view, not a contradiction.

### 8.3 Required behavior

- **Canonical id.** The native key is the per-record descriptor dword
  `[[rec+4]]`. The engine's canonical id is the **current roster slot id** —
  the same value `roster_order[side][pitch]` the replay target builds. The
  goal ring must store that id, not `desc_d` (`[[rec+4]+0xD]`, a placement-tier
  byte); this fixes an existing mismatch. After a substitution the pitch slot's
  id changes with the incoming player: the stats table is re-keyed from the
  current order on the substitution rebuild (the native `FUN_00047298` reads
  the current roster descriptor on every pass), and past goal-ring entries keep
  the id they were scored under. The first task of the slice is to confirm this
  lifecycle against `FUN_00091BC4`/`FUN_0004C00C`; if the native table keeps the
  starting key instead, follow the native and record the choice.
- Add a run-level stats table. Decision: index it by **side/roster slot** and
  keep the native `key` field per slot; initialize the keys from the same
  `roster_order` ids the replay target uses. This avoids depending on the
  native pointer identity while keeping `fifa96_stats_accumulate` usable.
- Add the two team threshold bytes (native `team+0x9`, `team+0x10`) to
  `struct fifa96_match_team`, or record them as an explicit staged value if
  their writer cannot be located. The first task of the slice is the writer
  census; a zero stage is allowed only with a numbered leg.
- Add an engine helper `match_run_stats_add(mr, entity_id, delta)` that:
  resolves the side/slot, computes `rank_above` and `class_in_range` from the
  record/team fields, calls `fifa96_stats_weight`, and accumulates into the
  side/slot table (and updates the max triple).
- Wire the call sites whose enclosing body is ported. The native sites are:
  `0x70F44/0x70FCE` (`FUN_00070DE0`), six in `FUN_0007131C`
  (`0x714BE/0x7157C/0x715DD/0x716C7/0x71732/0x717E3`), two in
  `FUN_00072AC4` (`0x72DE3/0x72ED1`), four in `FUN_00072478`
  (`0x72649/0x72660/0x7270F/0x72A52`), `0x77451/0x77493/0x7749F/0x774B2/
  0x77613`, `0x77DFD/0x77E38/0x77E4E/0x77E61`, `0x80D6C/0x80D99/0x813CD/
  0x813F5/0x81BBE`, and `0x8AD54`. The plan maps each to its ported handler
  (`fifa96_match_action_09`, `_1A`, `_1B`, `_0C`, `_05`, `_02`, `_0E`, the
  camera/announce helpers) and records the ones whose enclosing body is not
  ported.
- Mirror the accumulated values into `render.replay.activity_count` (or make
  the replay target read the stats table) so source-4 selection sees them.
- Model the max publish as three run cells; it has no reader besides reset in
  the native, so it is observability only.

### 8.4 Tests and gates

- Pure `fifa96_stats_weight`/`accumulate` tests remain.
- Engine fixture: a positive `+0x64` event and a negative `-0x64` event on a
  known record; assert the side/slot value, the 2x/1x/1.25x choice, the max
  triple, and that `fifa96_match_run_replay_target` sees the bumped count.
- A non-live tape check: stats writes must not change M1/M2.
- `make check`, M1/M2 exact, smoke.

## 9. Slice 4 — replay trigger (`0x515A6`/`0x3BB1C`)

### 9.1 Native contract

- Replay state `[0x109A98]`: `0` live, `0x81` armed, `0x82` play, `0x83`
  pause, `0x84` step back, `0x85` step forward, `0x86` slow, `0x87` wait.
- The arm is `FUN_00064270` → `[0x109A98] = 0x81` (`0x64284`). The native
  entry chain is `FUN_0004B100` mode 7 → `FUN_000513EC`/`FUN_00053BB8` →
  `FUN_00051270`/`0x51370`/`0x512B0` → `FUN_000510DC` → `FUN_00064270`.
- The replay camera selector is `0x515A6` state 4 (`0x5179F` calls
  `FUN_0004BCC4`); the engine already has the selector
  (`fifa96_camera_replay_choice_select`) and the target writer.
- `FUN_0003BB1C` is the match-screen loop; its sole replay camera call is
  `0x3BFED FUN_0004CF7C`, which the engine already covers with
  `fifa96_match_run_replay_camera_set`.

### 9.2 Current engine state

- `struct fifa96_match_run_replay` has the state, cursor, camera index,
  captions, goal ring and activity counts.
- `fifa96_match_run_replay_step` implements the reachable `FUN_000642FC`
  subset (0x80→0x81, 0x81 arm, button mapping, exit) but has **no production
  caller**.
- `fifa96_match_run_replay_begin` does not exist; nothing sets 0x80/0x81.
- The renderer draws the replay overlay when the state family is set.

### 9.3 Required behavior

- Add `fifa96_match_run_replay_begin(mr, trigger)` that:
  - is called from the native trigger path (the event/mode-7 request) and
    from a host key for the live runner;
  - sets the state to `0x81` (the `FUN_00064270` arm) and stages the cursor,
    camera index and target through the existing helpers;
  - does not invent a `0x80` store (the native has no `MOV [0x109A98],0x80`).
- Call `fifa96_match_run_replay_step` once per granted frame with the pad edge
  word when the state is in the replay family. Map the native buttons: `1`
  play/pause, `0x20` camera cycle, `8` step forward, `4` step back, `2` slow,
  `0x80` exit. The host runner exposes these keys.
- Exit restores source 0 and the live state.
- The full `FUN_0003BB1C` menu/scoreboard/audio loop stays a separate UI
  slice; the replay trigger, state machine, camera selection and overlay are
  closed here.

### 9.4 Tests and gates

- Engine fixture: trigger → 0x81; first step → 0x82 and the target writer;
  button `1` toggles pause; `0x20` cycles the camera; `8/4/2` enter the step/
  slow states; `0x80` exits and restores source 0.
- Render fixture: the replay overlay row/family appears only in the replay
  state.
- Non-live tapes: no replay trigger, M1/M2 unchanged.
- `make check`, M1/M2 exact, smoke.

## 10. Slice 5 — `FUN_00047298` 23-descriptor pool

### 10.1 Native contract

`FUN_00047298` (`0x47298..0x47813`) builds **23** remap blocks of `0x8E`
bytes. It:

- calls `FUN_0004BF04(1)` (the edited-lineup flag `[0x1074B8]`), then
  `FUN_0004C098(side, slot)` (hair class) and `FUN_0004C00C(side, slot)`
  (skin class) for the 11 slots per side;
- copies the 11 template rows from `0x106C00` (stride `0x30`) and applies the
  pattern bands at `0x106B00`/`0x106B70`/`0x106B74`/`0x106B78` through the
  source map `0x1068E4`;
- writes the hair ramp from `0x1070D4` and the skin base from `0x1070E4`;
- gates the skin/hair blanking with `FUN_0001CAEC(9)` (home) and
  `FUN_0001CAEC(0xD)` (away);
- writes the final table through `FUN_000CD390(0x100, ...)` — **256 bytes per
  block**, not `0x8E`. `0x8E` is the template/source stride; the skin/hair
  writes extend to `0xFD`, so the full 256-byte table is the native output.

`FUN_0004C00C`/`0x4C098` read the lineup order from `[0x14C360]` or
`[0x157BD2]` (the edited flag), then the descriptor byte `+0x1A`/`+0x1B`
(skin/hair ASCII) and map it to 0..3.

### 10.2 Current engine state

- `match_run_apply_player_remap` builds a 256-byte remap from kit bands +
  skin/hair classes for 11 players per side.
- `match_run_build_entity_remaps` overlays `roster_hair_code`/`skin_code` from
  the PCCD roster and composes the team table.
- There is no 23rd block, no template/pattern pass, and the native `0x8E`
  block length is not represented.

### 10.3 Required behavior

- Add a native-block builder that produces a **256-byte table per slot**:
  the `0x8E`-byte template row (11 per side), the pattern bands, the
  skin/hair writes at `0xD7..0xFD`, and the blanking gates. Do not collapse
  the table to `0x8E` and do not identity-fill `0x8E..0xFF`; the native
  writes the full 256 bytes.
- Build 23 blocks: 11 home, 11 away, and the 23rd block identified by the
  fresh evidence pass. If the 23rd block's identity/consumer cannot be
  proved, implement it as the native loop does (a default block) and record a
  numbered leg for its consumer.
- Keep the current kit-band/skin-hair behavior as the base; the native template
  pass replaces the approximation, not the kit selection.
- Live-only: the remap is only installed for the live match; M1/M2 never run
  this path.

### 10.4 Tests and gates

- Live render fixture: assert the template/pattern bands and the skin/hair
  ramp for home/away slots; assert the 23rd block exists and is stable.
- Substitution rebuild: the remap still follows `sub_lineup`/`roster_order`.
- M1/M2 exact; `make check`; smoke.

## 11. Campaign acceptance

- Each slice is green on `make check`, M1/M2 `cmp` and the windowed smoke
  before commit/push.
- No golden re-pin. If a golden moves, stop and report.
- The census is updated only when a body/row is wired (row 29 positive, or a
  new dispatch slot). The slot move, stats wiring and remap pass are body
  internals and do not change the 78/78 body count.
- Docs: one spec section per slice (`FU-275`/`FU-315` for the slot move,
  `FU-142` for row 29, `FU-72` for stats, `FU-351`/`FU-270` for replay,
  `FU-350`/`FU-348` for the pool), plus `ENGINE.md` and `docs/PROGRESS.md`.
- Only `main` locally and remotely.

## 12. Risks and open evidence tasks

- **Slot move:** the engine's single-block model vs the native four-slot
  array. Decision: close the human slot; the other three remain FU-70 legs.
  Risk: a hidden reader of `dir_x/dir_z` outside the dispatch staging. The
  plan starts with a reader census.
- **Row 29:** the expected outcome is negative; if the fresh census finds no
  entry, the closure is documentary. Risk: a computed installer not covered by
  the 77-site census; the plan scans the table writers too.
- **Stats:** the team `+0x9`/`+0x10` writers are not yet located. Decision:
  evidence task first; if unlocated, stage zero with a numbered leg and keep
  the weight formula native. Risk: the 2x/1x/1.25x choice is wrong until the
  writers land.
- **Replay:** the native entry is a mode-7 UI flow. Decision: wire the trigger
  and state machine, leave the `0x3BB1C` menu loop as a separate slice. Risk:
  the host key trigger is an engine convenience, not a native screen; document
  it as the live-runner binding of the native arm.
- **47298:** the 23rd block and the pattern tables need a fresh evidence pass.
  Decision: first task of the slice is the table/loop reconstruction; if a
  table is unreadable, record a leg and keep the current approximation for that
  band.
- **Cross-slice:** stats wiring and the roster pool both touch the live replay
  and render paths; land stats before replay so the trigger sees the counts,
  and the pool last so the render changes are isolated.

## 13. What is left to the builder

- The exact per-site mapping from the 29 `FUN_0008ED40` call sites to the
  ported handler bodies, and which enclosing bodies are still unported.
- The team `+0x9`/`+0x10` writer census and the decision to port or stage.
- The native replay trigger's exact event path from `0x92820(rec,0x0B)` /
  mode-7 entry to `FUN_00064270`; the engine hook point (frame body vs host
  input) is a plan decision once the path is traced.
- The 23rd `FUN_00047298` block identity and its consumer.
- The precise native `0x8E` block layout (`0x1068E4` map, template rows,
  pattern bands) and how each band maps into the engine's 256-byte remap.
- The row-29 positive/negative verdict.

These are evidence tasks, not product decisions; the design fixes the outcome
and the boundaries, and the plan fixes the order and the tests.
