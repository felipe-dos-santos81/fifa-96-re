# FU-341 — action row `0x03` arm fidelity: dx gate, `+0x99`, counter, `+0x7C3`, slot branch B

**Scope/provenance.** Phase-51 Task 2
(`docs/superpowers/plans/2026-11-04-fifa96-m2-phase51-producers.md`), spec
§2.2/§7. The implementation landed in `66fe985`; the phase-51 T5 close-out
re-read the key ranges with the Ghidra MCP on **`/FIFA96.EXE`** this session
(`disassemble_bytes 0x7E1CE`, `0x7E25F`, `0x7E2D3`, `0x7E3A6`, `0x7E3D5`,
`0x7C730`, `0x8C38D`, `0x79B58`, `0x79C20`, `0x740A0`, `0x8DCD4`). The T2
recon reads are R6 (the dx gate) and R11 (the clamp inputs); the clamp-bound
ruling is R11 §1.6. Prior records are not rewritten: FU-177 owns the row
map/legs, FU-198 the clamp helpers, FU-340 the phase-`0x13` writer/cadence
(FU-340 is the reference for phase `0x13`; this record covers only row `0x03`).

**Verdict in one line:** the already-ported `0x7E1CE..0x7E223` special arm and
the row's latent inputs are now faithful and unit-pinned, but the action-table
slot stays `NULL`: the `FUN_0008C418` phase-2 clamp bounds are `PCCD.PDB`
data-blocked (R11 §1.6), so wiring is the carried **FU177-L1** leg.

---

## 1. The phase-`0x13`/`0x14` arm `0x7E1CE..0x7E223`

Fresh bytes this session (T2 read the same range; `0x7E1CE` 86 B):

| address | bytes | instruction |
|---|---|---|
| `0x7E1CE` | `a1 4a 7a 15 00` | `MOV EAX,[0x157A4A]` (the phase dword) |
| `0x7E1D3` | `c1 f8 18` | `SAR EAX,0x18` (byte `0x157A4D`) |
| `0x7E1D6` | `83 f8 13` | `CMP EAX,0x13` |
| `0x7E1DB` | `83 f8 14` | `CMP EAX,0x14` |
| `0x7E1DE` | `75 44` | `JNZ 0x7E224` (the normal slot gate) |
| `0x7E1E0` | `c7 46 55 00 00 00 00` | `[rec+0x55] = 0` (`target_z`) |
| `0x7E1E9` | `8b 46 55` | `EAX = [rec+0x55]` |
| `0x7E1EC` | `8d 56 4d` | `EDX = &rec[0x4D]` (`target_x`) |
| `0x7E1EF` | `89 46 51` | `[rec+0x51] = EAX` (`target_y = 0`) |
| `0x7E1F2` | `8d 46 59` | `EAX = &rec[0x59]` (`pos_x`) |
| `0x7E1F5` | `c7 46 4d 80 07 00 00` | `[rec+0x4D] = 0x780` |
| `0x7E1FC` | `e8 d3 fa 00 00` | `CALL 0x8DCD4` |
| `0x7E201` | `8b 04 24` | `MOV EAX,[ESP]` |
| `0x7E204` | `c1 f8 10` | `SAR EAX,0x10` |
| `0x7E207` | `83 f8 30` | `CMP EAX,0x30` |
| `0x7E20A` | `7d 07` | `JGE 0x7E213` |
| `0x7E20C` | `c7 46 4d e0 0a 00 00` | `[rec+0x4D] = 0xAE0` |
| `0x7E213` | `89 f0` | `EAX = ESI` (rec) |
| `0x7E215` | `e8 3e b9 ff ff` | `CALL 0x79B58` |
| `0x7E21A` | `83 c4 08` | `ADD ESP,0x8` (the 8-byte scratch) |
| `0x7E21D..0x7E222` | `5d 5f 5e 5a 59 5b` | pops |
| `0x7E223` | `c3` | `RET` |

**The dx gate.** `FUN_0008DCD4` is called with `EAX = &rec[0x59]` (pos_x),
`EDX = &rec[0x4D]` (the `0x780` target) and the out buffer at `ESP`
(`0x7E1E7 MOV EBX,ESP`). Fresh `0x8DCD4` bytes store the x delta as a word at
`[ECX+2]` (`0x8DCEF 66 89 51 02` = `target_x - pos_x`) and the z delta at
`[ECX+4]` (`0x8DCF3 66 89 59 04`). `0x7E201 MOV EAX,[ESP]; SAR EAX,0x10` is
the **`dword[x]>>16` alias**, i.e. `out[1] = dx = (int16_t)(0x780 - pos_x)`.
It is a 1-D test: `pos_z` does not participate, and the sign-extended 16-bit
window wraps (`pos_x = 0x7000` gives `dx = -0x6880 < 0x30`). The pre-FU-341
port compared the 2-D metric (`out[0]`), which selected a different record set
and never took `0xAE0` for the fixture's far-z record.

**The `+0x99` guard.** Fresh `0x79B58..0x79B68`:

```
80 b8 99 00 00 00 00   CMP byte[EAX+0x99],0
75 07                  JNZ 0x79B68
c6 80 93 00 00 00 10   MOV byte[EAX+0x93],0x10
c3                     RET
```

The helper is inlined at **both** sites: guard A (`0x7E1C4`, the
`rec == [team+0x7BA]` interception target) and the special arm (`0x7E215`).
The engine's `+0x99` is `skip_99`; the write is now guarded at both sites.

## 2. The placement counter `0x7E2D3..0x7E308`

Fresh bytes at the idle core:

| address | bytes | instruction |
|---|---|---|
| `0x7E2D3` | `8d 56 4d` | `EDX = &rec[0x4D]` |
| `0x7E2D6` | `bb ff ff ff ff` | `EBX = -1` |
| `0x7E2DB` | `89 f0` | `EAX = rec` |
| `0x7E2DD` | `ff 56 1c` | `CALL [rec+0x1C]` (the phase handler) |
| `0x7E2E0` | `8b 46 63` | `EAX = [rec+0x63]` (the mover attribute) |
| `0x7E2E3` | `c1 f8 16` | `SAR EAX,0x16` |
| `0x7E2E6` | `40` | `INC EAX` |
| `0x7E2E7` | `66 8b 56 79` | `DX = word[rec+0x79]` |
| `0x7E2EB` | `66 89 46 7b` | `word[rec+0x7B] = AX` |
| `0x7E2EF` | `66 39 d0` | `CMP AX,DX` |
| `0x7E2F2` | `76 04` | `JBE 0x7E2F8` |
| `0x7E2F4` | `66 89 56 7b` | `word[rec+0x7B] = DX` (limit) |
| `0x7E2F8` | `a1 4a 7a 15 00` | `EAX = [0x157A4A]` |
| `0x7E2FD` | `c1 f8 18` | `SAR EAX,0x18` |
| `0x7E300` | `83 f8 02` | `CMP EAX,2` |
| `0x7E303` | `0f 85 9d 00 00 00` | `JNZ 0x7E3A6` (skip the clamp) |

`+0x63`'s high word is the mover block-A distance (`0x8E273`/`0x8E278`, the
`0x8DC68` metric over the low-word target-minus-pos), so the native
`([+0x63] >> 0x16) + 1` is `min((distance >> 6) + 1, word[+0x79])`. The port
passes `distance << 16` to `fifa96_action_locomotion_placement_counter`; the
literal-0 stand-in is gone.

**Timepoint leg (FU341-L1).** The native read is the value staged by the
*previous* frame's mover pass; the port recomputes the distance from the
current `target`/`pos` after the `[rec+0x1C]` call. When that call rewrites the
target, the port can lead the native by one pass. Latent while the slot is
`NULL`; it must be named before wiring.

## 3. The `+0x7C3` last-defender cell and the clamp selector

**Writer `0x7C741..0x7C770`** (the `FUN_0007BF20` tail after the z
integration; fresh bytes):

| address | bytes | instruction |
|---|---|---|
| `0x7C741` | `80 bd 8d 00 00 00 00` | `CMP byte[rec+0x8D],0` (active only) |
| `0x7C748` | `74 2c` | `JZ 0x7C776` |
| `0x7C74A` | `8b 45 00` | `EAX = [rec]` (the team block) |
| `0x7C74D` | `8b 90 c3 07 00 00` | `EDX = [team+0x7C3]` (the incumbent pointer) |
| `0x7C753` | `85 d2` | `TEST EDX,EDX` |
| `0x7C755` | `74 19` | `JZ 0x7C770` (store) |
| `0x7C757` | `8b 5d 61` | `EBX = [rec+0x61]` (self pos_z) |
| `0x7C75A` | `3b 5a 61` | `CMP EBX,[EDX+0x61]` |
| `0x7C75D` | `0f 9c c2` | `SETL DL` |
| `0x7C760/0x7C762` | `31 db / 88 d3` | `EBX = (self_z < incumbent_z)` |
| `0x7C764/0x7C766` | `31 d2 / 8a 90 26 08 00 00` | `EDX = byte[team+0x826]` (side) |
| `0x7C76C` | `31 da` | `EDX ^= EBX` |
| `0x7C76E` | `74 06` | `JZ 0x7C776` (keep) |
| `0x7C770` | `89 a8 c3 07 00 00` | `[team+0x7C3] = EBP` (the record pointer) |

Store rule: `side != (self_z < incumbent_z)`; side 0 keeps the minimum and
keeps the incumbent on an equal z, side 1 keeps the maximum and replaces on an
equal z (the native tie asymmetry). **Reset `0x8C38D..0x8C399`**:
`8b 86 b2 07 00 00` (`EAX = [team+0x7B2]`) then
`89 86 c3 07 00 00` (`[team+0x7C3] = EAX`) — the `FUN_0008C33C` reset writes
the same pick.

**Reader `0x7E360`/`0x7E383`.** After the bound arms
(`0x7E309..0x7E348`, the `FUN_0008C418` block), the side gate is
`0x7E357 CMP byte[team+0x826],0; JNZ 0x7E383`. Side 0 reads
`0x7E360 a1 9c 98 15 00` (`EAX = [0x15989C]`, the side-1 team block) then
`0x7E368 8b 68 61` (`EBP = [EAX+0x61]`, `[rec+0x61]` = pos_z); side 1 mirrors
with `0x7E383 a1 67 90 15 00` / `0x7E38B 8b 78 61`. The clip is
`target_z > opponent_z -> opponent_z - 0x60` (`0x7E36D`/`0x7E373`) or
`target_z < opponent_z -> opponent_z + 0x60` (`0x7E390`/`0x7E396`), gated by
`[0x14C32A] == 0` (`0x7E348`).

Engine: `struct fifa96_match_team.last_defender7c3` (team-relative index;
`NONE` = the native `NULL`), the writer
`fifa96_match_entities_last_defender_step` from `match_run_dispatch_entity`
(active gate + post-mover `pos_z`), the reset
`fifa96_match_entities_last_defender_reset` in `reset_lane`, and the reader
`fifa96_match_entities_opponent_defender_z` feeding the row-03 clamp. An unset
cell is the declared no-clip anchor (`INT32_MAX`/`INT32_MIN` by side); the
native pointer is guaranteed, so this is a documented engine default.

## 4. Slot branch B `0x7E25F..0x7E27C`

The slot gate is `0x7E224 MOV EBX,[rec+0x20]; TEST; JZ 0x7E281; CMP EAX,0xC;
JZ 0x7E281`. Branch A is `0x7E230..0x7E25E` (the `[0x157A4C] == 1`, phase 6
or phase `0x15` `CALL [rec+0x1C]`). Branch B, fresh bytes:

| address | bytes | instruction |
|---|---|---|
| `0x7E25F` | `89 da` | `EDX = EBX` |
| `0x7E261` | `89 f0` | `EAX = ESI` (rec) |
| `0x7E263` | `8b 5b 1e` | `EBX = [EBX+0x1E]` (slot dir_z dword) |
| `0x7E266` | `8b 52 1d` | `EDX = [EDX+0x1D]` (slot dir_x dword) |
| `0x7E269` | `c1 fb 18` | `SAR EBX,0x18` |
| `0x7E26C` | `c1 fa 18` | `SAR EDX,0x18` |
| `0x7E26F` | `e8 ac b9 ff ff` | `CALL 0x79C20` |
| `0x7E274` | `66 8b 46 79` | `AX = word[rec+0x79]` |
| `0x7E278` | `66 89 46 7b` | `word[rec+0x7B] = AX` |
| `0x7E27C` | `e9 25 01 00 00` | `JMP 0x7E3A6` |

`FUN_00079C20` (`0x79C20..0x79C4D`) writes `target = pos + dir*0x80`, forces
`target_y = 0` (`0x79C3C c7 40 04 00 00 00 00`), then calls `0x7D3E4`, whose
bytes clamp x to `±0x720` (`0x7D3E8..0x7D400`) and z to `±0xB10`
(`0x7D409..0x7D423`). The engine body is `fifa96_action_move_target`.

The `0x7E27C` jump skips the idle hold/idle core and the phase-2 clamp and
runs the shared tail: `0x7E3A6 CALL 0x79F3C` (camera place), then
`0x7E3AD..0x7E3B8` (`[0x157A4C]>>24 == 1` -> `word[rec+0x7B] = 1`, return) and
`0x7E3CA..0x7E417` (phase 1 only: side 0 floors target_z at `-0x20`
(`0x7E3E2`/`0x7E3EC`), side 1 floors at `+0x20` (`0x7E3FD`/`0x7E407`)). The
engine models this as `tail_only = 1`.

**Limit-word stand-in.** The native copies `word[rec+0x79]`; the engine's
staging record has no `+0x79` cell, so `timer7b` takes the pool entity's
`timer79`. Pre-existing and documented, unchanged here.

## 5. Tests

`port/tests/test_engine_match_handlers.c`:

- `test_action_03_arm_dx_gate` — `pos_x = 0x751`, `pos_z = 0x4000` -> `0xAE0`
  (the metric form fails: the 2-D octagon is ~0x4000); the `0x780` twin; the
  `0x750` boundary -> `0x780`; `0x600`; the signed-16 window `0x7000` ->
  `0xAE0`; the phase-`0x14` twin.
- `test_action_03_skip_99_guard` — `skip_99 = 1` no-write twins at the arm and
  guard A; `skip_99 = 0` writes; `+0x93` sentinels `0x55`/`0x22`.
- `test_action_03_counter_move_attr` — `d = 0x180 -> 7`, `0x100 -> 5`,
  `0x1000` limited by `timer79 = 0x10`, and the signed `target_z = -0x100 ->
  5`.
- `test_action_03_slot_branch_b` — phase 2: `pos (0x700,-0xB00)`,
  `dir (1,-1)` -> `(0x720, 0, -0xB10)`, `timer7b = timer79 = 0x12` (not the
  idle counter); phase 1: the branch-B move target `-0xB10` is raised to
  `+0x20` by the side-1 floor, so a `return` after `timer7b` leaves `-0xB10`
  and fails (T5 precision).
- `test_action_03_phase2_opponent_clip` — sibling `+0x7C3 = record 3` with
  `pos_z = 0` clips `0x200` to `-0x60`; the `NONE` twin leaves `0x200` (T5
  precision; term isolation).
- `test_last_defender7c3_dispatch_site` — one granted live frame moves the
  team-0 cell off `NONE` and it names an active record.

`port/tests/test_engine_match_entities.c::test_last_defender7c3` pins the
writer rule (NULL store, side-0 min/tie-keep, side-1 max/tie-replace), the
reset decode (id / `NONE` / sibling id), the opponent read (sibling block,
`-0x9A`) and the guards. `test_reset_lane_pass` carries the two `0x8C399`
picks (records 5/6).

Measured mutation-RED (T2): restore the metric gate -> the dx fixture fails;
drop both `skip_99` guards -> the guard fixture fails; counter literal 0 ->
the counter fixture fails; branch-B stub (`timer7b` + return) -> the branch-B
fixture fails; flip the side compare -> the entity fixture fails; remove the
dispatch-site call -> the dispatch fixture fails.

## 6. Carried legs

1. **FU341-L1 — the counter timepoint.** The native reads the previous mover
   pass's `+0x63`; the port recomputes the current distance. One-pass lead;
   latent while the slot is `NULL`.
2. **FU341-L2 — the clamp bounds are `PCCD.PDB` data-blocked.** `FUN_0008C418`
   sources the role-band line percentages from the `PCCD.POG` `PCDB` payload
   (R11 §1.6); there is no payload codec. The bound words stay the declared 0;
   the engine must not fake them. The portable sub-inputs landed here (the
   opponent selector and branch B); the block itself is FU177-L1.
3. **FU341-L3 — the `reset_lane` begin seed.** `0x8C399` (`+0x7C3 := +0x7B2`)
   sits in `fifa96_match_entities_reset_lane`, which has no live caller
   (FU295-L1). The cell is seeded `NONE` by init and the live value comes from
   the dispatch writer; the native begin seed is a carry.
4. **FU341-L4 — row-`0x03` wiring is FU177-L1.** The slot stays `NULL` until
   the `FUN_0008C418` block has real data; the arm/body work here removes the
   fidelity blockers but does not wire the row.

Related pre-existing notes: the branch-B `timer79` stand-in (above) and the
`reset_lane` `tracker7c7` encoded-id vs team-relative-index mismatch reported
in the T2 review. Both are out of this record's change set.

## 7. Census and goldens

The action row `0x03` slot is still `NULL`, so no census or golden moves:
`78/78` bodies, `58/80` natural, `64/80` install-live, `6` blocked, action
`38/45`, phase `26/35`; M1 `09b726b7…` and M2 v27 `54f44987…` stay
byte-identical (`cmp`).
