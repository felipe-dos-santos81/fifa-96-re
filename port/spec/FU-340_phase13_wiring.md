# FU-340 — phase `0x13` wiring: the slot and the once-per-write scatter

**Scope/provenance.** Phase-51 T1 (`docs/superpowers/plans/2026-11-04-fifa96-m2-phase51-producers.md`),
spec §2.1. Fresh Ghidra MCP reads of **`/FIFA96.EXE`** this session
(`disassemble_bytes 0x6E244` 232 B, `disassemble_bytes 0x8D693` 160 B,
`get_xrefs_to 0x8D098`); the tape numbers are R5's instrumented replays and
the in-tree re-pin below. The phase-51 plan allocated this record as FU-339;
that number already belongs to the phase-43 live-leg audit
(`port/spec/FU339_phase43_live_leg_audit.md`), so the record is numbered
**FU-340**. No prior section is rewritten.

**Verdict in one line:** the ported phase-`0x13` body is now wired to the live
walk, and the `FUN_0008D098` case-`0x13/0x14` scatter is called at the native
once-per-write cadence in `fifa96_match_run_write_phase`; the M2 golden re-pins
to v27 with a hash-only tail (first differing line 3185, 3338/6522), and phase
`0x14` stays `NULL` (no live writer).

---

## 1. The body `0x6E244..0x6E32A` (fresh 232-byte disassembly)

Entry ABI: `EAX = rec`, `EDX = &rec+0x4D` (stored to `ECX` at `0x6E24A`).

- `0x6E24C MOV EDX,dword[0x157AAC]` / `0x6E256 SAR EDX,0x18` — the controlled
  side `[0x157AAF]` (`phase_machine.side_controlled`).
- `0x6E252 MOV EAX,[EAX]` (the record's team dword) / `0x6E259 MOV BL,
  byte[EAX+0x826]` — the team side; `0x6E25F CMP EBX,EDX` (`39d3`) /
  `0x6E261 JNZ 0x6E326` (`0f85bf000000`) — the side gate returns with no
  write and no draw.
- `0x6E267 MOV BX,word[0x10F368]` (z cell) and `0x6E274 MOV SI,word[0x10F364]`
  (x cell); `0x6E27B TEST DL,DL` / `0x6E27D JNZ 0x6E2AC` (`752d`) selects the
  active arm.
- Inactive arm (`0x6E281..0x6E2A4`): `[0x157AC2] < 4 && side == 0` negates the
  z cell (`0x6E298 NEG EAX`), then `0x6E2A4 MOV [ECX+8],EAX` = `target_z`.
  It jumps to `0x6E321` and writes `0x6E324 MOV [ECX],EAX` = `target_x`.
- Active arm: `0x6E2B3 CALL 0x92AC8` / `0x6E2B8 DIV 0x90` / `0x6E2BA CALL
  0x92AC8` / `0x6E2BF TEST AL,1` → z magnitude/sign; `0x6E2CD..0x6E2F8`
  adds the int16 cell, applies the same `ac2 < 4 && side == 0` negation, and
  stores `target_z` (`894108`). Then `0x6E302 CALL 0x92AC8` / `0x6E307 DIV
  0x90` / `0x6E309 CALL 0x92AC8` / `0x6E30E TEST AL,1` → x magnitude/sign;
  `0x6E31C MOVSX ESI,SI` / `0x6E31F ADD ESI,EAX` / `0x6E321 MOVSX EAX,SI` /
  `0x6E324 MOV [ECX],EAX` (`8901`) stores `target_x`, never negated.
- `0x6E326..0x6E32A` is the shared `POP EBP/EDI/ESI/ECX; RET`.

So: **four `0x92AC8` draws per active served record, z before x**; two writes
only, `+0x4D` (`target_x`) and `+0x55` (`target_z`); `+0x51` (`target_y`) is
untouched. The port's `fifa96_match_phase_13` already matched this shape; this
slice only changes its dispatch status.

## 2. The producer `0x8D693..0x8D722` and its cadence

Fresh disassembly bytes: `0x8D693 B8 0D 00 00 00` (`MOV EAX,0xD`),
`0x8D698 E8 53 7B FD FF` (`CALL 0x651F0`, the unported sink),
`0x8D69D 31 C0` / `0x8D69F A0 C2 7A 15 00` (`MOV AL,[0x157AC2]`),
`0x8D6A4 83 F8 04` (`CMP EAX,4`) / `0x8D6A7 7D 5A` (`JGE 0x8D703`).

- `ac2 < 4` (`0x8D6B0..0x8D6FB`): calls `0x92AC8`, `DIV 0x1E0`, sign from the
  next draw's bit 0, stores `[0x10F364]`; then `z = 0x5A0` plus the same
  x-then-z pair into `[0x10F368]` — **four draws, x before z**.
- `ac2 >= 4` (`0x8D703..0x8D722`): no draws; `x = side ? +0x2D0 : -0x2D0`
  (`0x8D713 B8 D0 02 00 00` / `0x8D70C B8 30 FD FF FF`), `z = 0x5A0`, stored
  to the same two cells.
- The cells are global (not per record); the second team's call overwrites the
  first's values, so the last call before the next body run wins.

**Cadence.** `get_xrefs_to 0x8D098` returns exactly two references, both
`UNCONDITIONAL_CALL` inside `FUN_000740a0`: `0x740C8` and `0x740DB` — one call
per team, immediately after the `[0x157A4D]` phase write. There is no per-frame
xref. The port mirrors this by calling `fifa96_match_phase_machine_scatter`
after each team's walk inside `fifa96_match_run_write_phase`:

```
walk team 0 -> scatter team 0 -> walk team 1 -> scatter team 1   (0x13/0x14)
```

The `0x8D728` install tail remains in `fifa96_match_phase_machine_step` (the
per-frame divergence predates this slice and is unchanged).

## 3. What moves on M2

The tape carries **three live `write_phase(0x13)` calls** at engine steps
2502 / 3402 / 4302 (transcribed lines 2503 / 3403 / 4303; R5's gdb breakpoint
replay), and zero `write_phase(0x14)` calls. Per write the walk serves 20
records (team 0: 9 of 11 after the `skip_9a` exclusions; team 1: 11, all
side-gated), of which **8 team-0 records are active** (1, 2, 3, 4, 5, 7, 8, 10)
and take the four-draw arm: **32 body draws per write, 96 per run**. At
`ac2 < 4` the two producer calls add four draws each: **8 producer draws per
write, 120 shared draws per run**.

The in-tree re-pin (regenerated in this commit) is:

- sha256 `54f4498755a74f0945266c66c59164a239ebf1f5fd60d0f78e416fe5316d44fd`
  (old v26 `6f402095beeb7d50923964cd918874059f03b874e6a134c9c331881209cecf30`);
- first differing line **3185**; **3338 of 6522** lines differ, all hash-only
  (the `state=` tags are byte-identical and keep the v26 counts: 304/953/2193/
  303/4/2640/20/100 across the ten tags);
- length unchanged, **295087 bytes**;
- M1 stays `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`
  (`cmp` exact).

## 4. The rejected variants (R5 measurements)

| variant | first diff | lines | sha |
|---|---|---|---|
| slot only, no scatter | 3209 | 3314 | `c231a549…` |
| **slot + once-per-write scatter (landed)** | **3185** | **3338** | `54f44987…` |
| scatter in the per-frame `machine_step` | 852 | 5671 | `09642d40…` |

The per-frame variant is rejected on native evidence: `FUN_0008D098` has only
the two `FUN_000740A0` xrefs, so the scatter cannot run on every granted frame.
Measured, it re-pins the forced kickoff window itself (first diff 852), which
would break the tape's role as the frame-for-frame FU-142a staging pin. The
slot-only variant is rejected because the second and third body runs would read
`(0,0)` instead of `(-229,1398)` / `(-411,1380)`, and a later scatter slice
would force a second re-pin.

## 5. Tests

- `port/tests/test_engine_match_handlers.c`: `phase_expect[0x13]` is `FIFA96_OK`.
- `port/tests/test_engine_match_completion.c::test_phase_13_14_rng_gate`: the
  ET chooser route still lands phase `0x13`/act `0xC`; the slot is
  `fifa96_match_phase_13`; a forced write with the draw-free `ac2 >= 4` branch
  advances `mr.rng` by exactly `4 × active` (the body); the tape-shaped
  `ac2 < 4` write advances it by `4 × active + 8` (the two producers); phase
  `0x14` stays `NULL`/`UNSUPPORTED`. Nulling the slot drops both counts to zero.
- `port/tests/test_engine_m2.c::run_phase17_t3_riders` block 3: the tape-shaped
  fixture (eight active team-0 records, `ac2 >= 4`) measures exactly **32**
  draws across one `write_phase(0x13)`, the cells land on the team-1 fixed
  `+0x2D0/0x5A0`, and the `ac2 < 4` replay measures **40** (32 + 8).
- `port/tests/test_engine_match_act.c::test_phase_13_write_seam`: with
  `ac2 = 4`, the second write's body reads the first write's cells (the
  zeroed-cell replay differs by exactly `+0x2D0` / `+0x5A0`).
- `port/tests/test_engine_match_port_census.c`: wired `58/80`, install-live
  `64/80`, action `38/45`, phase `26/35`, bodies `78/78`, blocked `6`.

## 6. Residual legs

- **`0x651F0(0xD)` sink** (`0x8D693/0x8D698`): still unported (R1a-L3).
- **FU340-L2 — the native side-first walk order** (owner
  `fifa96_match_run_write_phase`). Fresh `0x740A0..0x740E0` bytes:
  `0x740B1 MOVSX EAX,DX` takes the caller's side, `0x740BA` stores it to
  `[0x157AAF]`, `0x740C0` builds `0x1588A4 + side*0x835`, `0x740C5 XOR DL,0x1`,
  then `0x740C8 CALL 0x8D098` walks team[side] and `0x740DB CALL 0x8D098`
  walks team[side^1]. The port's `write_phase` has no side argument and walks
  team 0 then team 1. M2's three writes carry `side_controlled = 0`, so the
  tape order coincides; a side-1 `write_phase(0x13)` swaps the cell/install
  order — a real off-tape divergence, no M2 impact.
- **Phase `0x14` writer**: no live engine producer. The ET chooser
  `FUN_0008B9CC` → `0x8BAC1` is fed by the probe cells `0x142FE0`/`0x142FE8`,
  which the engine models as `mr->probe_a/probe_b`; `fifa96_match_run_set_probe`
  has no live caller and ordinary setup zeroes them (`match_run.c:2221-2224`,
  `:2406-2409`). The slot stays `NULL`; the write seam is already 0x14-shaped,
  so a future competition-setup producer needs no further wiring.
- **`0x157A4E` consumers** (FU240-L3, the `0x87159`/`0x87161` readers in
  FU240-L4) are unchanged.
- **The action-row per-record seam** (`match_row_phase_call`, row 15 `0x87D29`)
  can dispatch the phase-`0x13` body inside a dwell. It does not on M2; a future
  `0x13` dwell with a dispatched row 03/15 adds four draws per record.

## Errata (phase-51 T6, 2026-11-04)

Append-only corrections from the Task 2/5 review round; no prior section is
rewritten.

1. **The phase-`0x14` seam is gated on the slot too.** Quoted from §6:

   > The slot stays `NULL`; the write seam is already 0x14-shaped,
   > so a future competition-setup producer needs no further wiring.

   **Corrected.** The walk/scatter calls sit inside the
   `fifa96_match_phase_table[phase].fn != NULL` gate in
   `fifa96_match_run_write_phase` (`match_run.c:2853`). A future phase-`0x14`
   producer must therefore **also wire the `0x14` slot** (`fn =
   fifa96_match_phase_14` or the derived body); feeding `probe_a/probe_b`
   alone leaves the seam a structural no-op. The `0x14` slot remains `NULL`
   today because the probe cells have no live competition-setup writer.

2. **The native walk order is side-first, not 0-then-1.** Quoted from §2:

   > ```
   > walk team 0 -> scatter team 0 -> walk team 1 -> scatter team 1   (0x13/0x14)
   > ```

   **Corrected.** Fresh `0x740A0..0x740E0` bytes show `0x740B1 MOVSX EAX,DX`
   (the caller's side), `0x740BA` stores it to `[0x157AAF]`, `0x740C0` builds
   `0x1588A4 + side*0x835`, `0x740C5 XOR DL,0x1`, then the two calls
   `0x740C8 CALL 0x8D098` (team[side]) and `0x740DB CALL 0x8D098`
   (team[side^1]). The engine's `fifa96_match_run_write_phase` has no side
   argument and walks team 0 then team 1. M2's three `write_phase(0x13)`
   calls carry `side_controlled = 0`, so the tape order coincides and the
   golden is unaffected; a future side-1 write would swap the cell/install
   order. **Carried leg FU340-L2** (off-tape, no M2 impact).

3. **The riders pin 40 per write; 120 is the three-write total.** Quoted from
   §3/§5:

   > `8 producer draws per write, 120 shared draws per run`
   > ... the `ac2 < 4` replay measures **40** (32 + 8).

   **Corrected.** The in-tree m2 rider measures **32 body draws** and **40
   shared draws for one tape-shaped write** (`ac2 < 4`). It does **not** count
   120: 120 is `3 × 40` for the tape's three writes and is pinned by the v27
   golden sha, not by a rider. The docs state both numbers; the "riders pin
   40/120" shorthand overstates the rider coverage.
