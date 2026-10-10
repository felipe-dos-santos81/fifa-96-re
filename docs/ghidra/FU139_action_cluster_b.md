# FU-139: M2 action cluster B — ball staging/resolver, kick trajectory, possession rows

Task 6 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`).
The spec's cluster B is "Ball — staging, resolver, pairing, kick/trajectory,
possession/tackle" (design §3.3); this slice re-verifies the ball-related action
rows read-only in the authoritative program, derives the pure parts of the ball
staging block, the reception target, the event-row resolver, the kick range/
resolver path and row 07's stage-0 target, ports them into
`fifa96_ball_pairing` / `fifa96_action_handlers` with tests, and records why no
cluster-B row is wired into the FU-137 dispatch seam.

Result in one line: **rows `05`/`06`/`07`/`0F` and the possession/tackle rows
`18`/`21`/`23` re-verify against `0x1106E0`; the ball staging block
`0x158730..0x158746` (clear `FUN_0007A028`, core stage `FUN_0007A490`), the
`FUN_0007A084` camera-led reception target, the `FUN_0007AE70` event-row
resolver (class/sector/idx/band → one of four 10-byte tables: `0x1102FE`,
`0x11016E`, `0x110196`, `0x11024A`), `FUN_0007B9C4`'s negative-mode range band,
the `row[0]`/actor-action append selector (flat `0x7AE38`) and row 07's stage-0
target are ported as seven tested pure functions; no row is wired because every
cluster-B body still needs unported arms and/or the absent entity/ball pool
(FU-139 OL-29..OL-32, carrying FU-137 OL-1/OL-16), so the FU-137 action
table stays `1 × OK + 44 × UNSUPPORTED`.**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit in every call). Ghidra
  **read-only**: no renames, comments, labels, function creation, scripts or
  project saves. `/fifa96_le.bin` and the MZ loader were not used.
* Tool calls made this slice:
  * `read_memory /FIFA96.EXE 0x1106E0` (180 B) — the full action table,
    matching FU-136 §1.1/FU-137 §1.1/FU-138 §1 dword for dword;
  * `disassemble_bytes` — row `05` head `0x7F194..0x7F2D0`; row `06` head
    `0x801B4..0x80270`; row `07` `0x814B0..0x815D0`; row `0F`
    `0x82AD0..0x82B60` and `0x82B60..0x82DD0`; row `18` head
    `0x849B0..0x84A40`; row `21` head `0x85214..0x852B0`; row `23` head
    `0x82F84..0x83010`;
  * support bodies: `FUN_0007A028`/`FUN_0007A084` head
    (`0x7A028..0x7A4F0`), `FUN_0007A490` head+mid (`0x7A4F0..0x7A940`,
    `0x7A940..0x7AA30`), `FUN_0007AE70` (`0x7AE70..0x7AF20`,
    `0x7AEF0..0x7B1A0`), `FUN_0007B9C4` (`0x7BBB0..0x7BC60`,
    `0x7BE00..0x7BF10`);
  * `read_memory 0x7AE38` (56 B, the inline jump table), `read_memory
    0x1104CA` (24 B, the sector bitmask table).
* **Address mapping.** In `/FIFA96.EXE` the loader's object-4 fixups are
  already applied, so data immediates render at their resolved flat addresses
  (`[0x00158730]` = the ball block, matching FU-137/FU-138's native convention;
  FU-73/FU-76/FU-78 wrote the pre-loader `0x58730` form). The action table at
  `0x1106E0` is read directly; code addresses are identical to
  `/fifa96_le.bin` (FU-130 §6). `FIFA96.EXE` defines no functions at the table
  targets, so every row citation is `disassemble_bytes` output.
* Baseline at task start: `make check` = **98/98** (FU-138 commit `7a2c7b0`);
  after this task 98/98 with the extended library tests (no new binary).
* Evidence chain: FU-136 §2 (row inventory/tasks), FU-137 §6.1 (row classes),
  FU-73 (ball block/pairing), FU-76 §3 (kick path/resolver), FU-77 §2.5–2.7
  (rows 05/06/07), FU-78 (possession/tackle/duel family). This doc re-verifies
  the slices it ports and refines three prior claims (see Errata).

## 1. Table re-verification (`0x1106E0`)

`read_memory 0x1106E0` (180 B = 45 dwords) returns, for the cluster-B slots:

```
0x05 0x07F194  0x06 0x0801B4  0x07 0x0814B0  0x0F 0x082AD0
0x18 0x0849B0  0x21 0x085214  0x23 0x082F84
```

exactly FU-136 §1.1/FU-137 §1.1. The sole reader remains `FUN_0007D9A4
@ 0x7DA77` (`EAX = code << 2; ADD EAX,0x1106E0`), and the call contract is
`EAX = record` only (FU-137 §2).

## 2. Cluster-B per-row derivation

Windows marked ✓ were re-read first-hand this slice; the body column is the
strongest of the fresh read and the named FU derivation. Field numbers are the
native record offsets.

| code | handler | gate / entry (verified) | timer | stage / arms | record-visible body | class / open leg |
|---|---|---|---|---|---|---|
| 05 | `07F194` | ✓ `[+0x9E]=1` then phase `!=2` → `FUN_0007DAB4`, return (`0x7F19F..0x7F1BA`) | ✓ `+0x89 < 0x4B0` then `+= [0x157A64]` (`0x7F221..0x7F23A`) | ✓ `+0x92 <= 3` jump table flat `0x7F184`; stage 0 head `0x7F274..` | carrier claim (`[0x158724] != rec` → block clear + set, `0x7F1BF..0x7F205`); team `+0x7B2=rec`/`+0x7B6=0`; camera triple copy `→+0x4D..55`; stage 0 gates (`+0x6B > 0x40`, `[0x57A83]`, `+0x6B > +0x77`, release timer `0x15872D`, airborne `+0x5D`) + type-table dribble dir (`0x10F334`/`0x10F33C`, speed `0x60`/`0x30`); stages 1–3 (animation, snap, hand-off 4) per FU-78 §3.2–3.4 | not ported (partial): `possession_reset/claim/timer/dribble_dir` tested; carrier arms OL-29 |
| 06 | `0801B4` | ✓ `[+0x9E]=1`; phase `!=2` → `FUN_0007DAB4` + clear `team+0x7B2/0x7B6`, return (`0x801BF..0x80223`); ✓ `[+0x8D]==0` → same reset (`0x801E5`) | — | — | ✓ carrier gate `[0x158724]==0` or carrier `[+0x69]>>16 > 0x90` or `[0x157750] > 0x70` → `FUN_0007D9A4(rec,4,invoke-now)` (`0x8022D..0x80263`); else the 597-insn pursuit target algebra (metric/atan/vector/RNG, installs 8/9/4) per FU-77 §2.6 | ported + wired (Task 13): `fifa96_action_pursuit_step` (§11); the unmodeled record bytes/lead/swap are OL-69 |
| 07 | `0814B0` | ✓ phase `!=2` or `word[+0x81]!=0` → tail `0x81702` (`0x814B8..0x814D1`) | ✓ `+0x89 += [0x157A64]` (`0x814D9..0x814ED`) | ✓ `+0x92` 0/1/2/tail (`0x814F3..0x8150C`) | stage 0 (`0x81512..0x815B5`): `[+0x9E]=1`; gate `([+0x69]>>16 > 0x40` or `(int16)(word[+0x5D]+0x70) < [0x157750])` with `+0x89 > 0x3C` → tail else wait; target = slot word `0x60`/`0x8000` arm (camera + type-offset table bytes
`0x10F334`/`0x10F33C` << 4) or camera triple; stage 1 KICK `FUN_0007B9C4` + opponent `0x22` invoke + `[+0x89]=0,+0x92++`; tail resets and installs `4` on `[0x158730]` | not ported (partial): `kick_angle/apply` + FU-139 resolver/stage target; machine OL-31 |
| 0F | `082AD0` | ✓ phase `!=2` → `FUN_0007DAB4`, return (`0x82ADB..0x82AEF`) | ✓ `+0x89 += [0x157A64]` (`0x82AF6..0x82B0A`) | ✓ `+0x92` 0/1/2 (`0x82B10..0x82B1C`) | stage 0: active gate, `word[+0x85]!=0` → reset; metric to vector `0x157788`, pos nudge `>>0x11`, face/anim 4; stage 1: predictor `FUN_00071B9C([0x157A62]>>16)`, distance compare, first/second `FUN_0007B9C4` (slot temporarily nulled, mode from slot or RNG `0x40`/`0x20`), then `word[+0x81] = 2*word[+0x85] - word[+0x87] + 0x1E` (`0x82B61..0x82DBE`) | not ported: machine OL-31 |
| 18 | `0849B0` | ✓ none (`+0x7B=2` at entry, `0x849B7`); `[+0x89] += [0x157A64]` (`0x849BD..0x849D3`) | ✓ | ✓ `+0x92` 0/1/2 (`0x849D9..0x849E7`) | stage 0 animation abort `0x55`/`0x6A` (`0x849F4..0x84A0A`); stage 1 target `(0x900,0)` + metric `FUN_0008DCD4`; stage 2 window `0x78..0x12C` + input byte + distance `< 0x20`; resolution NSEARCH `FUN_0008DB6C` + SWAP `FUN_000786A0` (FU-78 §7) | not ported (partial): `duel_step/split` tested; resolution OL-32 |
| 21 | `085214` | ✓ phase `!=2` or `rec != [0x157A83]` → `FUN_0007DAB4`, return (`0x8521F..0x8523D`); ✓ camera triple copy (`0x85242..0x8524C`) | ✓ stage 0/1 gates (`0x85269..0x85361`) | ✓ `+0x92` 0/1/default (`0x85253..0x8525F`) | stage 0 inactive → reset (no hand-off); offset `+0x6B > 0x40` with `+0x89 > 0x3C` → reset path; else nearest `FUN_0008DE8C` + metric + angle + event `0x4A` and `+0x89=0,+0x92++`; stage 1 event flag `+0x44` or offset `+0x41` → reset path with ball-actor install 4 + `FUN_00079B58` (FU-78 §4) | not ported (partial): `action_receive_step` tested; claim arm OL-32 |
| 23 | `082F84` | ✓ phase `!=2` → `FUN_0007DAB4`, return (`0x82F8C..0x82F97`) | ✓ `+0x89 += [0x157A64]` (`0x82F9F..0x82FB3`) | ✓ `+0x92` 0/1/default (`0x82FB9..0x82FC1`) | stage 0 `[+0x9E]=1`, inactive → reset, else `+0x89=0,+0x92++`; stage 1 target arm `0x82FF3..0x83055` (camera words `0x1577EE`/`0x1577FA`/`0x157800`, slot button `0x40`, vectors `0x157794`/`0x157788`, ±0xC0 side nudge, `FUN_00079B58`); window gates + attempt `FUN_00082DD0` → install `0x0E`; success → install `0x0F` (FU-78 §6) | not ported (partial): `tackle_step/attempt` tested; target arm OL-32 |

Cross-family state shared by the table: `+0x89` action timer, `+0x92` stage
byte, `+0x8D` active flag, `+0x9E` "ran" byte, `+0x44` animation-row terminal
bit, `+0x81` event/timer latch, `[0x157A4A]>>24` phase, `[0x157A64]` frame
delta, `[0x157A83]` controlled-record pointer, `[0x158724]` event carrier,
`[0x158730..0x158746]` ball staging block.

## 3. Support-item derivations and the ported pure functions

### 3.1 Ball block clear and stage core

`FUN_0007A028` (`0x7A028..0x7A081`) is the block clear (✓ read this slice):
`[0x158730]=0` and `[0x158734]=0`; words `0x158738`, `0x15873C`, `0x15873A`,
`0x15873E`, `0x158740` = 0; then `byte 0x158743 = 2`, `byte 0x158742 = 0x20`,
bytes `0x158744`/`0x158745`/`0x158746` = 0. Ported as
`fifa96_ball_pair_clear`.

`FUN_0007A490` is the staging writer (✓ read `0x7A4D3..0x7A4EA` and the
`0x7A8D1..0x7AA2B` tail). The entry core (EAX = actor, EDX = 6-byte vector,
BX = trajectory word, CL = event code) sets `[0x158730]=actor`, copies the
6 bytes to `0x158738`, writes `[0x15873E]=BX` and `byte 0x158743=CL`. Ported as
`fifa96_ball_pair_stage`. The entry also parks a stack sub-code word and gates
`[rec+0x99]`/`[0x157A6C]` (`0x7A498..0x7A4C6`), and the code-keyed tail
(`0x7A8D1` reads `byte 0x158743` into DX; `0x7A941` sets sub-code `0x30` for
code 2, `0x31` for 1/3/6, and the **inactive** 4/5/7 arm at `0x7A97F` resets
the whole block via `0x7A987..0x7A9D9`; the **active** arm at `0x7A9DE` calls
`FUN_00079C50` for 1/2/3/6 and `FUN_0006E598` for the rest) is unported:
**OL-26**.

### 3.2 Reception `FUN_0007A084`

Already ported (FU-73 §5): the skip rule (`[+0x8E]>>24==1` or `[+0x91] ∈
{0x10,0x11,0x12}` → skip = `(int8)[+0x8D]`, else 0, `0x7A335..0x7A369`),
`FUN_0008DE8C` nearest (`0x7A376`) and the assign/clear pair
(`0x7A37F..0x7A3A2`). The special class `0x18E2` arm (`0x7A0D6..0x7A207`,
RNG/scatter/angle fold into the staging block), the action-code sound arms
(`0x7A234..0x7A2E1`) and the carrier countdown write (`0x7A40A..0x7A412`) stay
unported: **OL-32**.

### 3.3 Reception camera-led target

`0x7A2FF..0x7A331` (✓): copies the `0x157770` triple (three MOVSD = 12 bytes,
32-bit fields) and adds `([0x1577BE]>>16)<<5` to x and `([0x1577C0]>>16)<<5`
to z (arithmetic shift, so the addend is signed), y untouched. Ported as
`fifa96_ball_pair_receive_target(base, lead_x, lead_z, out)` (the port shifts
the sign-preserving bit pattern in unsigned arithmetic, reproducing the native
`SAR`+`SHL` without a UBSan-negative-shift diagnostic).

### 3.4 Kick range band

`FUN_0007B9C4` `0x7BBE4..0x7BC15` (✓): `if ((int8)byte[0x158742] >= 0) return;
x = (int16)word[0x158738]; band = x < 0x5A0 ? 0x20 : (x < 0x780 ? 0x30 :
0x10); byte[0x158742] = band`. Ported as `fifa96_action_kick_range_band`.

The rest of `FUN_0007B9C4` keeps its FU-73/FU-76 status: the row application
`0x7BCB6..0x7BE0B` is the existing `fifa96_action_kick_apply`; the tail
`0x7BE0B..0x7BEDE` (✓ reread) is `traj += row[+6]` (done), then for
`x != 0 && code != 3`: code 4 RNG branch (`0x7BE45..0x7BE99`, `FUN_00092AC8`)
and `if (divisor != 0) traj = comp_z + speed/divisor` (`0x7BE9C..0x7BEC0`,
signed `IDIV` of `[0x158736]>>16`), then the `0x460` cap. The divisor branch and
the code-4 RNG arm are unported: **OL-28**.

### 3.5 Event-row resolver `FUN_0007AE70`

Inputs EAX = actor, EDX = code, EBX = x word, ECX = z word (✓
`0x7AE70..0x7AF20` and `0x7AEF0..0x7B1A0`):

* **class** (`0x7AE79..0x7AEA2`): `code == 0x40` → 2; else `(code & 0x10) ? 0 : 1`.
* **sector** (`0x7AEA2..0x7AECA`): `x == 0 && z == 0` → `(int8)actor[+0x8E]`;
  else `((FUN_000CD474(x,z) + 0x40) & 0x3FF) >> 7` (0..7).
* **idx** (`0x7AED2..0x7AEFA`): `code == 0x40` → 0; else
  `(byte[0x1104CA + subtype] >> sector) & 1` (x86 shift-count masking).
* **d / height** (`0x7AEFD..0x7AF1A`): `d = (int16)(word[0x157750] -
  word[actor+0x5D])`; if the **dword** `actor[+0x5D] != 0` and `d < 0x38`,
  return NULL (no row).
* **table/index**:
  * height present: `table = 0x1102FE`, `index = 2*class + idx` (`0x7AF20..0x7AF3A`);
  * else fast carry (`has_slot && active && phase==2 && class==1 &&
    slot[+0x23] < 7`): `table = 0x11016E`, `index = idx` (`0x7AF3F..0x7AF96`);
  * else `has_slot && code == 0x60`: `table = 0x11016E`, `index = idx + 2`
    (`0x7AF9B..0x7AFBD`);
  * else band `= d < 0x20 ? 0 : (d < 0x70 ? 1 : 2)`, `table = active ?
    0x110196 : 0x11024A`, `index = 2*band + 6*class + idx`
    (`0x7AFBF..0x7B016`);
  * row = `table + 10*index` (`0x7B016..0x7B018`).
* **append selector** (`0x7B01A..0x7B09F`): first the **actor action** byte
  `[+0x91]` is tested — `0x11`→ring(2), `0x12`→ring(7), `0x13`/`0x20`→ring(8),
  `1`→direct(1), `0x10`→ring(3); every other value falls to the switch on
  `row[0]-1` through the 14-dword table flat `0x7AE38`
  `{0x7B0A7,0x7B0BB,0x7B0CF,0x7B0E3,0x7B0F7,0x7B10B,0x7B11F,0x7B133,
  0x7B133,0x7B07D,0x7B147,0x7B147,0x7B15B,0x7B16F}` with
  `row[0]=0`/`>0x0E` → direct(0) (✓ table read at `0x7AE38`; arm bodies
  `0x7B0A7..0x7B183`): row 1→0x0D, 2→0x0F, 3→0x11, 4→0x0A, 5→0x09, 6→0x14,
  7→0x13, 8/9→0x16, 0x0A→3, 0x0B/0x0C→4, 0x0D→0x10, 0x0E→0x0C.

Ported as `fifa96_action_kick_event_row` (class/sector/idx/band/table/index)
and `fifa96_action_kick_event_append` (selector). The ring/direct sinks
(`FUN_000928F0` 25-entry ring at `0x5B440`, `FUN_00092820` store at `0x5B650`)
are presentation-side and unported: **OL-27**.

### 3.6 Row 07 stage-0 target

`0x8154C..0x815B5` (✓): when `[rec+0x20] != 0` and the slot word `[slot+6]` is
`0x60` or `0x8000`, `out.x = [0x15774C] + (int8)table[0x10F334 + type8] << 4`
and `out.z = [0x157754] + (int8)table[0x10F33C + type8] << 4` (y untouched);
otherwise the camera triple `0x15774C..54` is copied. The native addressing is
a dword load at `0x10F331 + type8` followed by `SAR 24` (`0x81571`/`0x81577`,
`0x81591`/`0x81597`), i.e. the sign-extended byte three bytes into the record:
the x-offset base is `0x10F334` and the z-offset base `0x10F33C` — the same
bytes FU-138 §3 reads for the row-0D velocity tables, not the raw `0x10F331`/
`0x10F339` load addresses. `type8 = [rec+0x8B]>>24`. Ported as
`fifa96_action_kick_stage_target` (the caller supplies the two extracted
per-type byte tables, so the port is unaffected by the addressing). The
stage-0 gate above it (`0x81512..0x81548`) is documented in §2 but not ported
(OL-31).

### 3.7 Ported pure functions

All caller-owned, table-free where the native reads a table through an
argument, negative error on NULL `fifa96_err_t` (the FU-76/77/78/138 port
convention). Byte citations are §3's windows.

| function | native site | semantics |
|---|---|---|
| `fifa96_ball_pair_clear(&state)` | `FUN_0007A028 0x7A028..0x7A081` | zero pointers/vector/traj/angle; `flags=0x20`, `code=2`, tail bytes 0 |
| `fifa96_ball_pair_stage(&state, actor, &vector, traj, code)` | `FUN_0007A490 0x7A4D3..0x7A4EA` | record actor, copy the 6-byte vector, write traj and code; flags untouched |
| `fifa96_ball_pair_receive_target(&base, lead_x, lead_z, &out)` | `FUN_0007A084 0x7A2FF..0x7A331` | `out = base`; `x += (lead_x>>16)<<5`, `z += (lead_z>>16)<<5` |
| `fifa96_action_kick_range_band(mode, ball_x, &band)` | `FUN_0007B9C4 0x7BBE4..0x7BC15` | non-negative mode unchanged; else 0x20/0x30/0x10 by x |
| `fifa96_action_kick_event_row(&event, sector_table, &out)` | `FUN_0007AE70 0x7AE79..0x7B018` | class/sector/idx/d/band → `{found, table, index}` (four tables, 10-byte stride) |
| `fifa96_action_kick_event_append(actor_action, row0, &out)` | `FUN_0007AE70 0x7B01A..0x7B09F` + table `0x7AE38` | actor-action prefix then `row[0]-1` switch → `{direct, code}` |
| `fifa96_action_kick_stage_target(...)` | row 07 `0x8154C..0x815B5` | slot-word arm (camera + type table << 4, y untouched) or camera copy |

### 3.8 Tests

* `tests/test_ball_pairing.c`: layout `_Static_assert`s on the new structs;
  clear field-by-field; stage fill/preserve; receive-target lead shifts
  (positive/negative/sub-1 truncation); NULLs.
* `tests/test_action_handlers.c`: layout asserts; range-band boundaries
  (`0x59F`/`0x5A0`/`0x77F`/`0x780`, non-negative pass-through, negative x);
  resolver fixtures for the height table, fast carry, `code 0x60`, the three
  bands with both state tables, the angle-derived sector, the sign-extended
  `0xE1` sector mask (`& 0x1F`), the `d < 0x38` none path and NULLs; all 20
  append-selector branches (6 actor actions + 14 `row[0]` entries, plus the
  default/over-range direct paths); stage-target slot/camera arms and NULLs.
* The plan's "kick moves the ball per the derived trajectory table" is
  `test_kick_resolver_selects_row_and_moves_ball`: the resolver picks row index
  0 of the height table, the test reads that row from a synthetic 10-byte table
  and `fifa96_action_kick_apply` clamps the ball x to the row's upper bound and
  adds the row's trajectory field.

## 4. Engine wiring status (0 new rows)

No cluster-B row is wired in this task. The FU-138 rule is unchanged: a row is
wired only when its **full** record-visible body is covered by tested C
functions (row `00` was). Every cluster-B row still has unported arm support
and/or needs the absent entity/ball pool, so wiring it would silently skip
native side effects (the exact failure FU-138 §7 rejected):

| row | what is missing | leg |
|---|---|---|
| 05 | carrier stage machine (target algebra, `FUN_0007F7E0`, snap/hand-off animation) | OL-29 |
| 06 | ~~597-insn pursuit body~~ — closed in §11 (Task 13): ported as `fifa96_action_pursuit_step`, row wired | OL-30 (closed); OL-69 remainder |
| 07 | kick machine (`FUN_0007E600`, opponent `0x22` invoke, ball-actor install 4, `FUN_0007DAB4`) + full `FUN_0007B9C4` target selection | OL-31/OL-28 — **closed in §9 (Task 11); row wired** |
| 0F | second kick machine (predictor, RNG mode, timer reload) | OL-31 — **closed in §9 (Task 11); row wired** |
| 18/21/23 | resolution/claim/target arms + NSEARCH/SWAP + team/opponent records | OL-32 |
| all | 0xB2 record pool, teams, `[0x158730]` actor binding | OL-16 (carried) |

The FU-137 classification is otherwise unchanged: `1 × OK` (action `00`),
`78 × -FIFA96_ERR_UNSUPPORTED`, `1 × -FIFA96_ERR_NOT_FOUND` (phase `0x16`).
The FU-137 §6.1 cells for rows 05/06/07/0F/18/21/23 gain FU-139 cross-refs and
the new legs via the errata at the end of this document and the in-place
FU-137 errata section; their class stays `not ported (partial)`/`not ported`.

## 5. Totals

| group | rows | state |
|---|---|---|
| ported + wired | `00` (cluster A) | unchanged `FIFA96_OK` |
| ported + wired (Task 11) | `07`,`0F` | `fifa96_action_kick_machine` + `fifa96_ball_kick_target`; `FIFA96_OK` (§9) |
| ported + wired (Task 12) | `18`,`21`,`23` | `fifa96_match_action_18/_21/_23` over the pool; `FIFA96_OK` (§10) |
| ported + wired (Task 13) | `06` | `fifa96_action_pursuit_step` + `fifa96_match_action_06` over the pool; `FIFA96_OK` (§11) |
| pure part advanced (cluster B) | `05` | still `-FIFA96_ERR_UNSUPPORTED`; the tested pure functions |
| still unported | all other rows | per FU-137 §6 |

## 6. Open legs

* **OL-26 — ball staging tail.** The code-keyed sub-code/animation tail
  (`FUN_0007A490 0x7A8D1..0x7AE2F`: `0x7A941` sub-code `0x30`/`0x31`, inactive
  4/5/7 block reset `0x7A97F..0x7A9D9`, active `FUN_00079C50`/`FUN_0006E598`
  arms `0x7A9DE..0x7AA2B`, `[0x158744/45]` stack writes) is unported.
  **Status (Task 10): the bounded half `0x7A8D1..0x7AA2F` is ported as
  `fifa96_ball_pair_stage_tail` (§8.2); the residual target algebra
  `0x7AA3C..0x7AE2F` is OL-62 (the eligibility set is `{1,2,3,6,0xE}`, §8.2).**
* **OL-27 — event append sinks.** `FUN_000928F0` (25-entry ring `0x5B440`,
  stride 0x15) and `FUN_00092820` (`0x5B650`) are presentation-side and
  unported; only the selector mapping is ported.
  **Status (Task 12): both sinks are ported as `fifa96_event_ring_append` /
  `fifa96_event_sink_store` on `struct fifa96_event_queue.ring` (§10.1) with
  the 0x110F1C eligibility table and the 1-based 25-entry cursor; the ring is
  not yet driven by the engine (no derived consumer) and the 0x157758 payload
  / `[0x112E88]` stamp are caller inputs (OL-67).**
* **OL-28 — full kick path.** `FUN_0007B9C4` target selection
  `0x7BA1E..0x7BBE4` (wing target `FUN_0008DCD4`, slot `FUN_0007B878`), the
  mode-bit-0x20 arms `FUN_0007B194`/`FUN_0007B57C` (`0x7BC34..0x7BC80`), the
  code-4 RNG branch and the divisor line (`0x7BE40..0x7BEC0`), the `0x114E04`
  angle fold and the `0x158736` speed source remain unported (FU-76 §7 leg 5
  extended).
  **Status (Task 11): the whole bounded path is ported as
  `fifa96_ball_kick_target` (§9); the residuals are the shared staging-tail
  algebra OL-62 and the external-input legs OL-66 (FU-142 §6).**
* **OL-29 — row 05 carrier arms.** Stage 0 tail `0x7F3A1..0x7F57B`
  (`FUN_0007F7E0` fallback, camera target algebra, `FUN_00092820`,
  `FUN_00071C94`, `FUN_00079CCC`/`FUN_0006DA64`) and stages 1–3
  (`0x7F57C..0x7F665`: animation select, `FUN_00079B1C`/`FUN_00079C50`, ball
  actor/receiver hand-off) are unported (FU-78 §3.4/OL-2).
  **Status (Task 10): stages 0 head/1/2/3 and the stage-0 dir selection are
  ported as `fifa96_action_carrier_arm` (§8.3); the stage-0 target algebra and
  the `FUN_0007F7E0` fallback are OL-63, and the 0→1 latch edge is OL-64.**
* **OL-30 — row 06 pursuit.** The 597-instruction body (FU-77 §2.6): target
  construction, `0x114E04` folds, RNG gates, installs 8/9/4.
  **Status (Task 13): closed.** The body `0x801B4..0x809EF` is ported as
  `fifa96_action_pursuit_step` (§11) and row 06 is wired through
  `fifa96_match_action_06`; the plan's `0x81067` span end covers the row-09
  handler (`0x80A00`, the action-table slot `0x1106E0[9]`), which stays
  unwired. The residual unmodeled record/presentation inputs are OL-69.
* **OL-31 — rows 07/0F kick machines.** `FUN_0007E600` decision, the opponent
  `0x22` invoke, the ball-actor install 4, the tail `FUN_0007DAB4`, the row-0F
  predictor/RNG/timer reload, and the stage-0 gate are unported.
  **Status (Task 11): the machines are ported as `fifa96_action_kick_machine`
  and rows 07/0F are wired (§9); the unmodeled record/presentation auxiliaries
  are OL-65 and the external block inputs OL-66 (FU-142 §6).**
* **OL-32 — reception/tackle/duel arms.** `FUN_0007A084` special-class/RNG
  arms, the action-code sound arms, `FUN_0004C324` (raw bytes), NSEARCH
  `FUN_0008DB6C` + SWAP `FUN_000786A0`, the row-21 claim arm, the row-23 target
  arm and the row-18 resolution (FU-78 §12 legs 3–9).
  **Status (Task 12): the row-18/21/23 resolution arms are ported and the rows
  wired (§10): `fifa96_action_duel_search` (0x8DB6C + 0xA1860 shell sort),
  `fifa96_action_duel_swap` (0x786A0), `fifa96_action_duel_bind` (0x4C324),
  the extended `fifa96_action_duel_step`/`fifa96_action_receive_step`/
  `fifa96_action_tackle_step`/`_attempt`, and the engine handlers
  `fifa96_match_action_18/_21/_23`. The `FUN_0007A084` special-class/RNG and
  sound arms (the ball staging tail's receive arm) remain with OL-62.**
* **OL-16 (carried) — entity/record pool.** The 0xB2 pool, team blocks,
  `[0x157A83]`/`[0x158724]`/`[0x158730]` actor bindings and the `FUN_0007D9A4`
  installer remain the C8/C11 replacement for the one-record stopgap; every
  cluster-B handler binding waits on it (carries FU-137 OL-1).

## 7. Concerns

* The resolver's height-path table base `0x1102FE` was missing from FU-76 §3.4,
  and its `index` formula is `2*band + 6*class + idx` (FU-76 wrote `2*code`);
  both are corrected in the errata below. The append selector's explicit
  compares test the actor action `+0x91`, not `row[0]` — FU-76 §3.4 conflated
  them; the corrected mapping is in §3.5.
* The sub-code passed to `FUN_0007A490` comes from a stack argument
  (`[ESP+0x2E]`, `0x7A4D9`) and the tail switches on the event code byte read
  back from `0x158743` (`0x7A8D1`); FU-73 §1's "codes 1/3/4/5/6/7 clear"
  summarizes only the inactive arm. The port keeps the core (unconditional
  stage) and leaves the tail as OL-26.
* Rows 18/21/23 are FU-137 OL-11 rows, not OL-8; FU-139 re-verifies them for
  the possession/tackle half of cluster B but does not advance their port.
* `fifa96_action_kick_event_row` shifts by `sector & 0x1F` to reproduce the
  native `SAR EAX,CL` masking for the unsigned fallback sector byte; this is a
  deliberate x86-faithful detail, pinned by the tests.

## Errata (prior docs)

* **FU-76 §3.4** — "`AL = row[0]`; `AL-1` indexes the inline table" and the
  compares "intercept `row[0]`": the compares at `0x7B020..0x7B03C` test the
  **actor action** byte `[ESI+0x91]`; the `row[0]` switch at `0x7B08E` is the
  fallback (verified bytes this slice). The band formula is `2*band + 6*class
  + idx` (not `2*code`), the height-present path uses table `0x1102FE` with
  `2*class + idx` (not listed), and `d < 0x38` returns NULL only when the
  `actor[+0x5D]` dword is nonzero.
* **FU-73 §1** — the staging tail's clears (`0x7A987`) are reached on the
  **inactive** actor path with event codes 4/5/7; codes 2 and 1/3/6 set the
  sub-code `0x30`/`0x31`, and the active path calls `FUN_00079C50` (1/2/3/6)
  or `FUN_0006E598` (rest). The unconditional stage writes
  (`[0x158730]`/vector/`0x15873E`/`0x158743`) are unchanged.
* **FU-73 §3.3** — confirmed; the added detail is the 32-bit `0x157770` triple
  and the arithmetic `>>16<<5` lead (ported).

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit): `read_memory
0x1106E0` (180 B), `0x7AE38` (56 B), `0x1104CA` (24 B); `disassemble_bytes`
`0x7F194..0x7F2D0`, `0x801B4..0x80270`, `0x814B0..0x815D0`, `0x82AD0..0x82B60`,
`0x82B60..0x82DD0`, `0x849B0..0x84A40`, `0x85214..0x852B0`, `0x82F84..0x83010`,
`0x7A028..0x7A4F0`, `0x7A4F0..0x7A940`, `0x7A940..0x7AA30`,
`0x7AE70..0x7AF20`, `0x7AEF0..0x7B1A0`, `0x7BBB0..0x7BC60`,
`0x7BE00..0x7BF10`. No writes: no rename/comment/label/function/script/project
save. `/fifa96_le.bin` and `/fifa96.exe` untouched.

Repo: `make check` = **98/98** at task start and after (ASan/UBSan engine tests
included); M1 golden and pinned render hashes unchanged (no render path
touched). Port write set: `include/fifa96_loader/fifa96_ball_pairing.h`,
`src/fifa96_loader/fifa96_ball_pairing.c`, `tests/test_ball_pairing.c`,
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`, `tests/test_action_handlers.c`,
`src/fifa96_engine/fifa96_match_handlers.c` (evidence strings only),
`tests/test_engine_match_handlers.c` (comments only),
`docs/ghidra/FU137_dispatch_mechanics.md` (errata). `game/FIFAPCCD96.iso`
untouched; `fifa96.rep/**` churn not staged.

## 8. Task 10 port — ball staging tail and row-05 carrier machine (FU-142-follow / M2 arms-and-wiring T10)

Reviewed read-only in `/FIFA96.EXE` (explicit; Ghidra MCP, no writes). This
section closes the *bounded* half of OL-26 and OL-29: the `FUN_0007A490`
code-keyed staging tail and the row-05 carrier machine stages 0-3, with the
staging-tail target algebra and the `FUN_0007F7E0` fallback left as numbered
legs (OL-62..OL-64). No row is wired (see §8.6).

### 8.1 Tool calls (first-hand, read-only)

* `disassemble_bytes 0x7A8D1..0x7A9E0` (70 insns), `0x7A9DE..0x7AB20`
  (95 insns), `0x7AB1F..0x7AB60` (18 insns), `0x7AB60..0x7AD00` (124 insns) —
  the `FUN_0007A490` tail `0x7A8D1..0x7AE2F`;
* `read_memory 0x1104A0` (48 B) — the eligibility byte table `0x1104BB`
  (`... 00 01 01 01 00 00 01 00 00 00 00 00 00 00 01` for offsets
  `0x1104BB..0x1104C9`, i.e. codes `0..0xE`);
* `read_memory 0x7A458` (56 B) — the per-code jump table (used below for the
  residual leg);
* `disassemble_bytes 0x7F194..0x7F390` (127 insns), `0x7F38F..0x7F580`
  (139 insns), `0x7F57C..0x7F7D0` (203 insns) — row `05` `0x7F194..0x7F665`
  plus the helpers at `0x7F666`/`0x7F6F0`;
* `read_memory 0x7F184` (16 B) — the row-05 stage table;
* `disassemble_bytes 0x71C94..0x71D00` + `0x71D00..0x71E20` (the
  `FUN_00071C94` camera/track family), `0x79CCC..0x79D59` (`FUN_00079CCC`),
  `0x79B1C..0x79B58` (`FUN_00079B1C` snap), `0x6DA64..0x6DB40`
  (`FUN_0006DA64` block swap), `0x7F7E0..0x7F9C0` (`FUN_0007F7E0` head;
  residual).

### 8.2 The staging tail `0x7A8D1..0x7AA2F` → `fifa96_ball_pair_stage_tail`

Native flow, site-annotated (all first-hand this slice):

```
0x7A8D1  DX = (int8)byte[0x158743]              ; the staged event code
0x7A8D9  TEST DX,DX; JL 0x7A8EF                 ; negative -> receive
0x7A8E1  CMP EAX,0xF; JGE 0x7A8EF               ; >= 0xF   -> receive
0x7A8E6  CMP byte[EAX+0x1104BB],0; JNZ 0x7A8F6  ; eligibility table
0x7A8EF  CALL 0x7A084; JMP 0x7A934              ; reception (unported)
0x7A8F6  gate: (int32)[+0x69]>>16 < 0x60        ; == word[+0x6B], signed
0x7A901  +0x59 += (dword[+0x6B])>>17            ; arithmetic SAR 17
0x7A90C  +0x61 += (dword[+0x6D])>>17
0x7A91D  0x1577BE/C0/C2 = 0                     ; camera velocity
0x7A934  if (byte[+0x8D] == 0) {
0x7A941    code 2   -> byte[0x158744] = 0x30
0x7A955    code 1/3/6 -> 0x31
0x7A970    code 7/4/5 -> the whole-block reset 0x7A97F..0x7A9D9
0x7A9DE  }
0x7A9DE  if (code in {1,2,3,6})
0x7A9F5    DX = (int16)(dword[0x158738]>>16)    ; word 0x15873A
0x7AA01    BX = (int16)(dword[0x15873A]>>16)    ; word 0x15873C
0x7AA09    CALL 0x79C50                          ; face
0x7AA0E  EDX = (int8)byte[0x158744] (sub-code)
0x7AA14  EBX = (int8)byte[0x158745] (reserved45)
0x7AA1A  ECX = (int8)[+0x8B]>>24 (type8)
0x7AA2B  CALL 0x6E598                            ; animation id
0x7AA30  if ([+0x20]) CALL 0x78B00               ; slot callback
0x7AA3C..0x7AE2F                                 ; per-code target algebra
```

The `0x1104BB` bytes read this slice are
`{0,1,1,1,0,0,1,0,0,0,0,0,0,0,1}` for codes 0..0xE, so the eligibility arm is
`{1,2,3,6,0xE}` (not only `{1,2,3,6}` as the earlier FU-139 §3.1/§6 prose
implied). The inactive 4/5/7 reset is exactly the FU-73 §1 clear
(`0x158730`/`34` = 0, words `738/73A/73C/73E/740` = 0, flags `0x20`, code 2,
the three tail bytes 0) — ported by reusing `fifa96_ball_pair_clear`.

Ported: `fifa96_ball_pair_stage_tail(&state, &actor, recompute_table, &out)`
(`include/fifa96_loader/fifa96_ball_pairing.h`), with the caller-supplied
eligibility table, the nudge gate/addends, the active/inactive classification,
the clear, the face (through the tested `fifa96_arm_face`), the animation
(`fifa96_arm_anim_select`) and the slot-callback request. The `receive`,
`camera_zero` and `slot_cb` outputs are derived requests/no-ops for the
unported `FUN_0007A084`, the camera block and `FUN_00078B00` (OL-62). The
native `0x6E598` EBX input is `byte[0x158745]` (`reserved45`), read only by the
unmodeled `FUN_0006E490` frame resolve (`0x6E701..0x6E706`); it is *not* the
`byte[[rec+0x28]]` row the derived helper's `row` stands for, so the call
passes `row = 0` and the `reserved45` value is deliberately not bound
(OL-62).

### 8.3 Row `05` `0x7F194..0x7F665` → `fifa96_action_carrier_arm`

First-hand flow (site-annotated; the FU-78 §3 stage table `0x7F184` =
`{0x7F274, 0x7F57C, 0x7F5E9, 0x7F627}` re-read as bytes
`74f20700 7cf50700 e9f50700 27f60700`):

```
0x7F19F  byte[+0x9E] = 1
0x7F1A6  phase != 2 -> CALL 0x7DAB4; RET       ; reset
0x7F1BF  if ([0x58724] != rec) { block clear 0x7F1C7..0x7F205; }
0x7F20B  [rec][+0x7B2] = rec; [rec][+0x7B6] = 0
0x7F221  if (+0x89 < 0x4B0) +0x89 += zero-ext [0x157A64]
0x7F240  if (word[+0x81] != 0) RET
0x7F24E  rec+0x4D/51/55 = camera 0x15774C/50/54
0x7F259  AL = byte[+0x92]; if AL > 3 RET
0x7F274  stage 0: (int32)[+0x69]>>16 > 0x40 -> [0x157A83]=0; RET
0x7F291  [0x157A83] = rec; if word[+0x6B] > word[+0x77] RET
0x7F2A8  if (byte[0x15872D] > 0) RET
0x7F2B5  if (dword[+0x5D] != 0) RET
0x7F2BF  type arm (dword[0x157750] > 0x38): dir = 0x10F334/33C[type8], speed 0x60
0x7F304  slot arm: dir = slot[+0x20/+0x21], speed 0x30
0x7F32D  team gates (team+0x828 != 0 && team+0x7BF == 0 &&
         (team+0x829 != 0 || +0x8D != 0)) -> CALL 0x7876C (slot merge)
0x7F361  else CALL 0x7F7E0 (fallback, unported)
0x7F374  CMP type8,5 / JNZ 0x7F65C              ; only type8 == 5 continues
0x7F386  [0x5872A] = dir_x; [0x5872B] = dir_z  ; (type/slot arms fall in here)
0x7F3A1..0x7F57B  the stage-0 target algebra (unported, OL-63)
0x7F57C  stage 1: 0x92820(rec,0x26); 0x1577BE/C0/C2 = 0;
         anim = (+0x8D != 0) ? 6 : 0x30 via 0x6E598; +0x89 = 0; +0x92++
0x7F5E9  stage 2: no slot RET; CALL 0x79B1C (snap);
         CALL 0x79C50(rec, slot[+0x1D]>>24, slot[+0x1E]>>24);
         word[slot+6] == 0 -> RET; +0x92 = 0
0x7F627  stage 3: byte[+0x44] == 0 -> RET; +0x92 = 0;
         if (rec == [rec][+0x7B2]) { 0x7D9A4([0x158730], 4, 0, 0);
         0x79B58([0x158734]) }
```

Ported: `fifa96_action_carrier_arm(&state, &carrier, type_dir_x, type_dir_z,
&out)` (`include/fifa96_loader/fifa96_action_handlers.h`) with the FU-78
`possession_claim`/`_timer`/`dribble_dir` helpers, the `ran_set` latch (native
`0x7F19F` `byte[+0x9E] = 1`, the engine `ran` field, set before the phase gate)
and the 0x79B1C snap, 0x79C50 face and 0x6E598 animation folds inline (the
`fifa96_arm_helpers` library already depends on `fifa96_action_handlers`, so
reusing those symbols here would create a link cycle); the unported calls are
explicit request flags (`sink`, `slot_merge`, `fallback`, `tail`,
`camera_zero`) with the numbered legs. The stage table is honoured exactly
(0-3; >3 returns); `tail` mirrors the native continuations (type/slot arms
unconditional, fallback only at `type8 == 5`, merge request left to the merge
result).

### 8.4 The staging-block vector erratum

`0x158738` holds the `FUN_0008DCD4` out triple `{word distance, word dx, word
dz}`: `0x7F666` (`0x7F6AF..0x7F6BB`) writes it with `EAX = rec+0x59`,
`EDX = &local`, `EBX = 0x158738`, and the staging tail reads dword
`0x158738`>>16 as the target-x addend (`0x7AA75`) and dword `0x15873A`>>16 as
the target-z addend (`0x7AA8C`/`0x7AA94`). FU-139 §3.3's "x/middle/z" C field
naming is therefore semantically `{distance, dx, dz}`; the struct is unchanged
(never repurpose a field) and `fifa96_ball_pair_stage_tail` documents the
mapping.

### 8.5 Tests

* `tests/test_ball_pairing.c`: `test_stage_tail_recompute_and_receive` (the
  eligibility table domain, negative/0xF wraps, the nudge gate/addends, the
  camera-zero arm), `test_stage_tail_inactive_subcode_and_clear` (0x30/0x31
  latches, the 4/5/7 clear field-by-field, the fall-through anim), and
  `test_stage_tail_face_anim_and_slot` (octants 2/0, the zero-direction seed,
  the 0x6F clamp, the slot callback) + NULLs.
* `tests/test_action_possession.c`: `test_carrier_phase_reset` (the `ran_set`
  latch set on the early reset return), `test_carrier_claim_timer_and_timer81`,
  `test_carrier_stage0_gates` (lane 0x40/0x41, close/bound, release countdown,
  airborne, type/slot arms with `tail == 1`, the merge request with
  `tail == 0`, the fallback `type8 == 5` tail gate with `type8 == 4` vs `5`,
  + table NULLs), `test_carrier_stage1` (sink/camera/anim 6 vs 0x30, latch),
  `test_carrier_stage2_and_stage3` (snap, slot face, `word[slot+6]`, `+0x44`,
  hand-off, stage > 3) + NULLs.
* `tests/test_engine_match_handlers.c`: `test_action_05_unwired_carrier` pinned
  `fn == NULL`, the FU-139/OL-63/UNSUPPORTED evidence and `UNSUP` dispatch
  (superseded by M2 phase-9 T2: `test_action_05_wired_carrier` +
  `test_action_05_claim_dirs_and_stages` pin the wired row).

### 8.6 Wiring decision (evidence-gated; superseded by M2 phase-9 T2)

Row `05` is **not wired** in this slice. The plan's wiring gate requires
install arm + full record-visible body + pool binding; the stage-0 target
algebra (`0x7F3A1..0x7F57B`) writes the record target triple and swaps a
20-byte block via `FUN_0006DA64`, and the `FUN_0007F7E0` fallback installs a
code — all outside this slice. Per the T6/T9 reviewed precedent (conditional
gate; honest negative over an unsupported claim), `fifa96_match_action_table[0x05].fn`
stays NULL and the evidence names **OL-63**. The ported functions are
loader-level, tested symbols consumed by the future T11/T12 arms.

**M2 phase-9 T2 update (2026-10-10).** The reachable carrier producers are
now landed and the row is **wired**: `fifa96_match_action_05`
(`src/fifa96_engine/fifa96_match_handlers.c`) applies the claim
(`0x7F1FF`, the pool `ball.carrier` + the `0x158728..0x15872F` block), the
team-target bind, the capped timer, the camera triple, the stage-0
control/dir gates, the `0x7876C` merge request, the `0x79B1C` snap, the
`0x79C50` face and the stage-3 hand-off. The stage-0 target algebra and the
`FUN_0007F7E0` fallback remain the OL-63 residual (named requests
`out.tail`/`out.fallback`); the possession release countdown gets its
`0x4B163` decay and the row-06 staging reads the live `0x15872A/0x15872F`
bytes. See FU-142 OL-63 (narrowed) and ENGINE.md phase-9 T2.

### 8.7 Open legs (numbered; registered in FU-142 §6)

* **OL-62 — staging-tail residual.** `FUN_0007A490 0x7AA3C..0x7AE2F`: the
  per-code jump table flat `0x7A458` (`{0x7AACB,0x7AB83,0x7AC57,0x7AD0F,
  0x7ADA0,0x7AE2F,0x7AE2F,0x7ADBA,0x7ADEC,0x7AE2F x5}`), the
  `0x78B00` slot callback (`0x7AA37`), `FUN_0007A084`'s body, the local target
  algebra with `0x92820`/`0x8F188` sinks, `0x92AC8` RNG draws and the
  `0x157736` speed source all remain unported; the nudge's second addend
  (dword `0x15873A`>>16 == `0x15873C`) is caller-supplied. The native
  `0x6E598` EBX input (`reserved45`) feeds only the unmodeled `FUN_0006E490`
  frame resolve, so it is not bound to the derived `row` stand-in (which
  represents `byte[[rec+0x28]]`, §8.2).
* **OL-63 — row-05 residual / non-wiring.** Stage 0's target algebra
  `0x7F3A1..0x7F57B` (the camera/local target copies, the `0x8DC68` metric
  accumulator, `FUN_00092820(rec,0x26)`, `FUN_00071C94`, the `0x15872D`
  write, the `0x157A4F` gate, `FUN_00079CCC` with `+0x9A`, `FUN_0006DA64`)
  and the `FUN_0007F7E0` fallback (`0x7F7E0..0x801B2`; installs code 7/0x11,
  rotates `0x158729`, calls `0x7E528`/`0x8DCD4`/`0x92AC8`/`0x6DBCC`/`0x741B4`/
  `0x8DD70`/`0x92820`/`0x7D9A4`) are unported; **narrowed by M2 phase-9 T2**:
  the claim/team-target/control/camera/dirs/stages/hand-off are wired in
  `fifa96_match_action_05`, and only the target algebra + fallback remain this
  leg. The `0x7F19F` `+0x9E` latch is tracked through `out.ran_set` (the engine
  `ran` field) so a future wiring cannot drop it silently.
* **OL-64 — stage-0 → stage-1 edge.** Row 05's body never writes `+0x92` on
  the stage-0 path (`0x7F274..0x7F57B` has no `+0x92` store; the only stores
  are `0x7F5D9`, `0x7F616`, `0x7F630`), so the native 0→1 transition is an
  external re-install (the installer `FUN_0007D9A4` stages the BL byte into
  `+0x92`) whose caller is not statically located in this window; the port
  models the stage byte as an input.

### 8.8 Repo state (this task)

`make check` 103/103 (library tests also under ASan/UBSan); M1 golden and
pinned render hashes unchanged (no render path touched). Write set:
`include/fifa96_loader/fifa96_ball_pairing.h`,
`src/fifa96_loader/fifa96_ball_pairing.c`,
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_ball_pairing.c`, `tests/test_action_possession.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_handlers.c`, `CMakeLists.txt` (ball-pairing link adds
`fifa96_arm_helpers`), `docs/ghidra/FU139_action_cluster_b.md` (this section),
`docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 05, §7 note, Task-10
errata), `docs/ghidra/FU142_installer_arms_scope.md` (§6 leg registry).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.

## 9. Task 11 port — the `FUN_0007B9C4` kick path and the rows 07/0F machines (OL-28/OL-31)

Reviewed read-only in `/FIFA96.EXE` (explicit; Ghidra MCP, no writes). This
section closes OL-28 and OL-31: the full bounded `FUN_0007B9C4` path is ported
as `fifa96_ball_kick_target` (`fifa96_ball_pairing.{h,c}`), the row 07/0F
machines as `fifa96_action_kick_machine` (`fifa96_action_handlers.{h,c}`), and
both rows are wired through the engine handlers `fifa96_match_action_07`/`_0F`
over the pool. The residual legs are OL-62 (shared staging-tail algebra),
OL-65 (unmodeled record/presentation auxiliaries) and OL-66 (external block
inputs/tables).

### 9.1 Tool calls (first-hand, read-only)

* `disassemble_bytes` `0x7B9C4..0x7BC34` (177 insns) and `0x7BC34..0x7BF17`
  (213 insns) — the full `FUN_0007B9C4`;
* `disassemble_bytes` `0x7B194..0x7B44F` (219 insns), `0x7B44F..0x7B57C`
  (101 insns) — `FUN_0007B194`; `0x7B57C..0x7B878` (235 insns) —
  `FUN_0007B57C`; `0x7B878..0x7B9C4` (109 insns) — `FUN_0007B878`;
* `decompile_function` `0x7B9C4`, `0x7B878`, `0x7B194`, `0x795A4`, `0x92AC8`
  (cross-checked against the bytes);
* `read_memory 0x114E04` (64 B: 0, 402, 804, 1206, 1608, ... — the FU-88 sine
  table), `0x14C1D4` (all-zero per-side range words in the image);
* row 07: `disassemble_bytes` `0x814B0..0x81760`; row 0F:
  `0x82AD0..0x82B60`, `0x82B60..0x82DD0`;
* helper decompiles: `0x7DAB4`, `0x7C990`, `0x7E600`, `0x71B9C`, `0x79B1C`,
  `0x79B6C`, `0x79B58`, `0x78A84`, `0x78AA4`, `0x78B00`, `0x741B4`, `0x8DD70`,
  `0x8DC68`, `0x6DBCC`;
* `read_memory 0x1104CA` (24 B resolver mask), `0x1104A0` (48 B: the `0x1104BB`
  recompute table), `0x11016E` (40 B carry rows), `0x110196` (180 B active
  rows), `0x11024A` (180 B idle rows), `0x1102FE` (60 B height rows),
  `0x10F334`/`0x10F33C` (32 B each, the per-type kick direction bytes).

### 9.2 `FUN_0007B9C4` `0x7B9C4..0x7BF16` → `fifa96_ball_kick_target`

Entry `EAX = actor` (0 reuses `[0x158730]`), `DL = mode` stored to `0x158742`,
`EBX = 6-byte input vector or NULL`. Flow with fresh sites:

```
0x7B9D4  if (EAX) [0x158730] = EAX else EBP = [0x158730]
0x7B9E5  byte [0x158742] = DL
0x7B9EB  if (EBX) copy 6 B EBX -> 0x158738 else zero the triple
0x7BA17  word [0x15873E] = 0
0x7BA1E  if ([EBP+0x20] == 0) -> 0x7BBE4 (band)
0x7BA29  if ((int8)mode != word[slot+6]) word[slot+6] = (int16)(int8)mode
0x7BA4A  L1 = active && phase==2 && (slotword6 & 0x20) && slot[+0x23] < 7
0x7BA8A  L2 = phase==2 && (slotword6 & 0x10)
0x7BABD  if (L2 && active && |pos_x| > 0x420 && ((side==0 && pos_z>0x7B0)
         || (side==1 && pos_z<-0x7B0))) -> wing (0x7BB13):
         local = {±0xF0, 0, pos_z}; 0x8DCD4(camera, local, 0x158738);
         goto 0x7BC1A
0x7BB4B  dir_x = (int8)slot[+0x20]; dir_z = (int8)slot[+0x21]
         if (dirs nonzero) -> FUN_0007B878(dir)
         else if (!active) -> type arm
         else if ((mode & 0x30) || ball_height || pos_y) {
             if (L2 && !pos_y) -> FUN_0007B878(0,0) else type arm
         } else if (type in {0x10,0x11,0x12}) -> type arm
         else return 0
type arm dir = table 0x10F334[type8], 0x10F33C[type8]; FUN_0007B878(dir)
0x7BBE4  if ((int8)[0x158742] < 0) [0x158742] = band(x)
0x7BC1A  inactive: (flags & 0x20) -> B57C(actor, vector, 0)
         active: flags == 0x40 -> B194(actor, vector, traj)
                 else (flags & 0x20) || L1 -> B57C(actor, vector, L1)
0x7BC80  row = FUN_0007AE70(actor, (int8)flags, word[0x15873A], word[0x15873C])
0x7BCA2  row == 0 || row[0] == 0 -> return 0
0x7BCB6  code = row[0] -> 0x158743; sub = row[9] -> 0x158744
         lo = row[+2]; hi = row[+4]; add = row[+6]; divisor = row[8]
         mode 0x30: {0x1C8, 0x780, 0x30, 0x18, extend=1}
         0x14C1D4[side*2] & 0x10 && (extend || code in {1,3}) -> lo/hi 1.5x
         x < lo -> x = lo; else x > hi -> x = hi; on clamp:
            angle = FUN_000CD474(word[0x15873A], word[0x15873C]);
            word[0x15873A] = FUN_000795A4(x, sin_tab(angle));
            word[0x15873C] = FUN_000795A4(x, sin_tab(angle+0x100))
0x7BE0B  traj += add
0x7BE26  if (x != 0 && code != 3) {
             if (code == 4) mode==0x40 ? traj = 0x90 + (rng&7)*(0x10-desc15)
                                      : divisor = (rng&0x7F)+3;
             if (divisor) traj += (int16)word[0x158738]/(int16)divisor;
         }
0x7BECE  if ((int16)word[0x15873E] > 0x460) traj = 0x460
0x7BEDE  FUN_0007A490([0x158730], 0x158738, traj, code, sub); return 1
```

`FUN_0007B878` (109 insns) resolves the event row for `code = (int16)slot_word6`
and `x/z = dir_x/dir_z`, clamps `min(slot23,0x3C)^2 * (int8)row[1]` to
`row[2]..row[4]` (with the same `0x14C1D4`/slot-word 1.5x), applies `*0xB5>>8`
when both dirs are nonzero, writes `dx = dir_x*speed`, `dz = dir_z*speed` and
the `FUN_0008DC68` distance into `out[0]`; a NULL row zeroes the triple.

`FUN_0007B194` (mode 0x40, 219 insns) builds a goal-line target
`{x, 0, ±0xB40}` from 11..13 RNG draws (the no-slot phase-1/no-slot/random,
slot/mode-state clusters), runs `0x8DCD4(camera, target, vector)`, derives the
speed `((int8)rec[+4][0x10]<<7) + 0x390 + (rng&0x3F)`, halves it for
`byte[+0x99]` or applies the `0x14C1D4` 1.5x, adds the goal-side drift
`±(|0xB10-pos_z|>>7)` and the `[0x14C2F6]==1` signed adjust, then folds
`FUN_000CD474(word[0x15873A], word[0x15873C])` into
`word[0x15873A]`/`[0x15873C]` via the 0x114E04 table and `FUN_000795A4`
(`(a*b+0x8000)>>16`). `FUN_0007B57C` (235
insns) starts at the camera + the staged dx/dz (`camera_z - 0x60` for
non-zero SI), takes the nearest `FUN_0008DE8C` team record within the
`|angle2-angle1| < 0x80` gate (for SI==0) as a velocity-projected decoy, else
stages the goal-line fallback `±(0xB10 + rng&0xF)`, `±(0xD0 - rng&0x1F)`,
optionally `±0x30`, speed 0x5A0 and the same fold; `[0x14C326] > 0` returns
unchanged.

Ported surfaces: `fifa96_ball_kick_target(state, actor, slot, input, ctx, rng,
mode, out)` and the `fifa96_ball_kick_actor`/`_slot`/`_ctx`/`_out` views. The
`0x114E04` fold is implemented exactly (the 257-entry table, the bit-8 index
negation with the 0x100 wrap and the bit-9 value negation, then
`FUN_000795A4`); every native call body is either ported (`0x8DCD4` via the
tested `fifa96_arm_dist_stage`, `0x8DC68` via `fifa96_entity_distance`,
`0x8DE8C` via `fifa96_entity_find_nearest`, `0xCD474` via
`fifa96_action_kick_angle`, `0x92AC8` via `fifa96_rng_step`, `0x7AE70` via the
tested resolver, `0x7A490` via `fifa96_ball_pair_stage` + `_stage_tail`) or a
documented request (the tail's `FUN_0007A084`/`0x78B00`, OL-62). The caller
supplies the four 10-byte row tables, the `0x1104CA` sector mask, the
`0x1104BB` recompute table, the `0x10F334/33C` direction tables, the camera,
the `0x14C1D4` word and the team candidates (OL-66).

### 9.3 Rows 07/0F → `fifa96_action_kick_machine`

Row 07 (`0x814B0..0x81737`): phase != 2 or `+0x81 != 0` -> the tail
`0x81702` (`FUN_0007DAB4` then, for the team target, install 4 on the staged
ball actor and `FUN_00079B58` on the receiver); `timer89 += (uint16)delta`;
stage 0 (`0x81512..0x815CD`): `[+0x9E]=1`, the lane/`word[+0x5D]+0x70` vs
`0x157750` gate (timer > 0x3C -> tail), the tested stage target, `timer89=0`,
`+0x92++`; stage 1 (`0x815CD..0x816FC`): the SI mode word (`0x40` for the
`[team+0x7CB]` record, the slot word, `0x40` for staged code 3, else -1), the
`FUN_0007E600` decision (`0x110680` type gate, the `0x71B9C(4)` predictor in
`0x20..0x60`, the pos/predictor `0x8DCD4` distance <= 0xF0 and <= lane, the
camera x bounds, the side/pos_z bounds, the `0x8DD70` angle inside `±0x100`
(side 0) or outside (side 1) and the `|angle - word[+0x7D]| <= 0x100` gate ->
install `0x0E`), the `[0x14C32A]`/`0x15B680==4` `0x40 -> 0x20` downgrade, the
kick, the post-kick opponent gate (staged traj `word[0x15873E]` < 0x30,
timer < 5, the opponent type 6, no slot, lane `word[+0x6B]` < 0xD0, the
`FUN_0008DD70(word[+0x6D], word[+0x6F])` angle vs `word[+0x7D]` within `0x55`
-> install `0x22`), `timer89=0`, `+0x92++`; stage 2: `+0x44`.

Row 0F (`0x82AD0..0x82DCF`): phase != 2 -> reset; `timer89 += delta`; stage 0
(`0x82B21..0x82BC3`): inactive or `word[+0x85] != 0` -> reset; the
`0x8DCD4(pos, 0x157788)` distance < 0x50 advances pos by the half vector; the
`0x79B6C` re-anchor (target = pos, y = 0, the camera face; the helper's
inactive -> anim 0x26 branch is unreachable from this call — the `0x82B21`
stage-0 gate resets inactive records first), `[+0x9E]=1`, `timer89=0`,
`+0x92++`; stage 1
(`0x82BC9..0x82DA2`): the `0x79B1C` snap, `+0x44` reset, `+0x81` wait, the
`0x7876C` merge request (`team+0x828`, lane < 0xF0, `[0x1586D7]==0`), the
`0x78A84` slot backup, the lane/bound and opponent-height gates, the lane 0x30
and `pos_y+0x80` gates, the `0x78AA4` slot restore, kick 1
(`mode = word[slot+6]`), the predictor distance vs lane and the corner kick
(`0x6DBCC` code 2/3 into the camera-led vector with the slot temporarily
nulled, mode 0x40 for code 3 else 0x20 with the `0x92AC8` face gate), then
`word[+0x81] = 2*word[+0x85] - word[+0x87] + 0x1E`.

The port splits the native synchronous `FUN_0007B9C4` calls into
`kick`/`corner_kick` requests the engine runs (`kick_done` 1/2 re-enters the
machine); the reset consequences (`+0x92 = 0xFF`, `+0x89 = 0`) are applied by
the dispatcher and the requests (defender 0x0E, opponent 0x22, ball 4,
reset/merge) are drained by the FU-141 pool installer. The record bytes
`+0x44/+0x85/+0x87/+0x99/+0x9D`, the roster descriptor, the `[0x1577CA]`
exclusion, the `[0x1586D7]` gate, the `[team+0x7CB]`/`[0x7C7]` pointers and
the `0x71B9C`/`0x6DBCC` tables are staged zero / caller inputs (OL-65/OL-66).

### 9.4 Engine wiring

`fifa96_match_action_07`/`_0F` stage `mr->record` (plus the new derived
`entity_id`/`actor_type` fields) into `fifa96_action_kick`, run the machine,
run the requested kick(s) through `fifa96_ball_kick_target` on
`mr->entities.ball.pair` with the embedded EXE tables, repack
`timer89`/`timer81`/`stage92`/targets/`type`/`ran`/`install`/`helper_request`,
and resolve the opponent (`team[1-team].target` -> 0x22 install) and the
receiver timer through the pool. Rows 07/0F flip to `ported`:
`action_expect[0x07] = action_expect[0x0F] = FIFA96_OK`; dispatch counts
74 -> 72 `-UNSUPPORTED` / 5 -> 7 `FIFA96_OK` (FU-137 §6.1/§7, Task-11 errata).

### 9.5 Tests

* `tests/test_ball_pairing.c`: `test_kick_target_resolves_and_stages`
  (resolver row + staging), `test_kick_target_clamp_and_angle_fold` (the
  lower-bound clamp and the exact 0x114E04 fold values),
  `test_kick_target_bit8_fold` (the bit-8/bit-9 index negations with the
  fold values 1358/-1358),
  `test_kick_target_code4_rng_divisor` (the seed-0 divisor `x/3` traj and the
  inactive code-4 clear), `test_kick_target_negative_band_and_no_row` (the
  band + the row[0]==0 path), `test_kick_target_slot_dir_arm` (the `0x7B878`
  speed/dx/dz), `test_kick_target_invalid` (NULLs).
* `tests/test_action_handlers.c`: `test_kick_machine_07_reset_and_stage0`,
  `test_kick_machine_07_decision_and_post`,
  `test_kick_machine_07_reset_decision` (the 0x7C990 codes),
  `test_kick_machine_0F_stage0`, `test_kick_machine_0F_reload_and_corner`,
  `test_kick_machine_invalid`.
* `tests/test_engine_match_handlers.c`: `test_action_07_runs_body`,
  `test_action_0F_runs_body`, the flipped `action_expect[0x07]`/`[0x0F]`.

### 9.6 Repo state (this task)

`make check` 103/103 (engine tests under ASan/UBSan); M1 golden and pinned
render hashes unchanged. Write set: `include/fifa96_loader/fifa96_ball_pairing.h`,
`src/fifa96_loader/fifa96_ball_pairing.c`,
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`, `tests/test_ball_pairing.c`,
`tests/test_action_handlers.c`, `tests/test_engine_match_handlers.c`,
`CMakeLists.txt` (rng links), `docs/ghidra/FU139_action_cluster_b.md` (this
section), `docs/ghidra/FU137_dispatch_mechanics.md` (Task-11 errata),
`docs/ghidra/FU142_installer_arms_scope.md` (OL-65/OL-66, OL-28/OL-31 status).

### 9.7 Erratum (M2 playability-legs Task 1 fix round 1) — the kick gate byte is +0x91

First-hand re-verification on `/FIFA96.EXE`: the `0x110680` gate inside
`FUN_0007E600` reads `MOV EAX,[ESI+0x8E]; SAR EAX,0x18` (`0x7E617..0x7E620`),
i.e. the **byte at +0x91**, and `FUN_0007C990`'s carrier check reads the same
byte (`0x7C9CA..0x7C9D6`; the final install compare at `0x7CA34` likewise
`MOVSX BX,byte [EAX+0x91]`). The installer writes the action code to +0x91
(`0x7DA67`), so both gates index the record's **action code**, not the +0x8E
face octant. The engine's kick staging was feeding `record.type` (the octant
written by `fifa96_arm_face`/`match_kick_repack`) into
`fifa96_action_kick.type`; rows 07/0F are wired, so the divergence was live
whenever the octant landed in the gate's bit-0 set {0,3,4,5,6} while the
native code (7/0x0F) does not. The same byte is the ball path's
`fifa96_ball_kick_actor.type`: `0x7BB8F MOV EAX,[EBP+0x8E]; SAR 0x18` compared
to `0x12/0x10/0x11`. Fix: `struct fifa96_match_run_record` carries
`code /* native +0x91 */`, `match_run_dispatch_entity` stages it from
`e->code`, and `match_kick_from_record` sets `s.type = r->code` (previously
`s.type = r->type`); `s.type8`/`s.facing` remain the +0x8E byte/octant.
Discriminating fixtures: `test_kick_machine_07_decision_and_post` (gate code 0
vs 7), `test_kick_machine_07_reset_decision` (code 5 vs 7) and the engine
`test_action_07_decision_uses_code_byte` (staged-code SI arm; octant values
inverted). Reachability note: from a kick handler the record's code is 7/0x0F,
so the row-07/0F decision arm is refused by the native gate too; the engine's
`is_team_cb`/slot-word SI staging is separately unmodeled (OL-65), and the
0x7C990 type-5 branch has no engine-visible observable because the
team-target tail's ball install 4 overwrites `record.install`.

## 10. Task 12 port — event append sinks and rows 18/21/23 (OL-27/OL-32)

Reviewed read-only in `/FIFA96.EXE` (explicit; Ghidra MCP, no writes). This
section closes OL-27 (the native event append sinks) and OL-32 (the
reception/tackle/duel resolution arms) and wires action rows 18/21/23 through
the engine handlers `fifa96_match_action_18/_21/_23`. The residual
presentation/global inputs are the numbered legs OL-67/OL-68.

### 10.1 Tool calls (first-hand, read-only)

* `disassemble_bytes`: `0x928F0` (164 B, the ring append), `0x92820` (65 B,
  the sink), `0x8DB6C` (221 B, NSEARCH), `0x786A0` (75 B, SWAP), `0x4C324`
  (78 B, bind), `0xA1860` (the shell sort), `0x8A29F..0x8A350` (the row-18
  install arm), `0x7CFE0..0x7D057` (the row-21 install arm at `0x7D046`),
  `0x7D140..0x7D1B3` (the row-23 install arm at `0x7D1B9`), `0x82DD0` (the
  attempt), `0x849B0..0x84AE1` (row 18), `0x85214..0x8539B` (row 21),
  `0x82F84..0x83163` (row 23), `0x8DC68` (the distance), `0x8DD70` (the angle
  wrapper), `0x53DC4` (the recorder arm), `0xCB2A4` (`[0x112E88]`), `0x79B58`
  (the receiver timer), `0x7DAB4` (the shared reset), `0x45001` (the input
  byte);
* `read_memory 0x110F1C` (256 B; the deliberate 0x28-byte eligibility table),
  `0x157758` (zero in the image; the FU-71 track triple);
* `get_xrefs_to`: `0x928F0` (16 calls, `FUN_0007AE70` ring arms), `0x92820`
  (33 calls), `0x8DB6C` (6), `0x786A0` (9), `0x4C324` (17), `0x157758`
  (writers `FUN_0006FFC0`/`0x736AC`), `0x15B665` (ring clear `FUN_000927E0`);
* Ghidra read-only: no renames, comments, labels, functions or saves.

### 10.2 `FUN_000928F0` / `FUN_00092820` → `fifa96_event_ring_append` / `_sink_store`

* index cell 0x15B665, ring 0x15B440 stride 0x15, sink 0x15B650; the sink check
  is a dword load at 0x15B64D >> 24 (`0x928F9..0x92904`);
* the cursor is 1-based: `INC`/`IDIV 25` first, then the entry is written at
  the new index (`0x92913..0x92938`); the clear function `FUN_000927E0` seeds
  index 0 and zeroes all 25 code bytes;
* the ring entry and the sink both lay out `{code byte, stamp dword (from
  `FUN_000CB2A4` = `[0x112E88]`), actor dword, 12-byte 0x157758 triple}`
  (`0x9293F..0x92980`, `0x9283F..0x92856`);
* the 0x110F1C table byte bit 0 selects the extra-field fill; table[0]=0,
  table[1..0x25] bit 0 set, table[0x26]=table[0x27]=0;
* the suppression path still clears the sink byte (`0x92987`). Codes >= 0x28
  are the derived boundary (OL-67).
* Ported on `struct fifa96_event_ring` (a field of `struct
  fifa96_event_queue`) with the embedded eligibility table; tests:
  `tests/test_event_queue.c` `test_event_ring_init_and_append`,
  `test_event_ring_wraps_at_25`, `test_event_ring_sink_suppression`,
  `test_event_sink_store`.

### 10.3 `FUN_0008DB6C` (NSEARCH) → `fifa96_action_duel_search`

Eligibility (per 0xB2 record i): `[+0x20] == 0` (0x8DB89), `i != (int16)BX`
(0x8DB8F), `rec != [team+0x7BF]` (0x8DB9A), `i != 0 || byte[team+0x829] != 0`
(0x8DBA2), `[+0x9A] == 0` (0x8DBAF), `[+0x98] == 0` (0x8DBB8). The value is
`-0x8DC68(word[P] - word[rec+0x59], word[P+8] - word[rec+0x61])` (0x8DBDF..0x8DBEA);
`FUN_000A1860` (called at 0x8DC08) is a gap-sequence shell sort (gap =
n/2..1, inner `j..j+gap` compare with `JGE` = no swap on equal at `0xA18B2`)
run over the values with every swap mirrored on the index array; the chosen
record is the first index slot (`0x8DC10 IMUL [ESP],0xB2`). With no candidate
and ECX != 0, the first record with `[+0x20]==0 && [+0x9A]==0` is returned
(0x8DC1B..0x8DC41); else NULL.

### 10.4 `FUN_000786A0` / `FUN_0004C324` → `fifa96_action_duel_swap` / `_bind`

* SWAP: `from[+0x20] != 0 && to[+0x20] == 0` moves the slot pointer
  (`0x786B0..0x786B5`) and zeroes the new slot's words +4/+6/+8/+0xA/+0xC/
  +0x14/+0x16 (`0x786BF..0x786E3`); the derived surface moves the record-level
  `has_slot` flag (the slot block is unmodeled, OL-68).
* BIND: `FUN_00053DC4` recorder-arm latch (0x14E574), then
  `byte[0x157AC2] >= 4 && [0x1587D4] != 0` -> `[0x1074A4] = zero_extend(
  [[[0x1587D4]] + 0x826])` + `FUN_00036200(0)` (`0x4C347..0x4C360`), else
  `[0x1074A4] = sign_extend(dword[0x1587E3] >> 24)` (`0x4C363..0x4C370`).

### 10.5 Rows 18/21/23 bodies and the wiring gate

* **Row 18** `0x849B0..0x84AE1` (188 insns): `word[+0x7B]=2`, `timer89 +=
  delta`, the `+0x92` latch 0/1/2, the `[[rec+0x28]][0]` abort (0x55/0x6A), the
  stage-1 `(0x900,0,0)` target + `0x8DCD4`, the stage-2 window (`0x78..0x12C`,
  `0x45001 & 0xF0`, the `+0x65` metric < 0x20) and the resolution
  (`0x4C324`, `+0x9A=1`, NSEARCH+SWAP, `0x7DAB4`). Install arm
  `0x8A29F..0x8A32F`: `[0x15888F]` record, the per-player accumulator
  `[0x157B90 + 15*c + type] += byte[0x15888D]`, install 0x18 when
  `accumulator & 0x7F >= 2` (`0x8A31C..0x8A32F`).
* **Row 21** `0x85214..0x8539B` (117 insns): phase != 2 or `rec !=
  [0x157A83]` reset, the camera copy, `+0x9E=1`, the stage limits, the
  lane<=0x40 arm (`0x8DE8C` over `[[rec]+0x7A6]` with skip -1, the `0x8DCD4`
  metric and `0xCD474` angle whose two compares are dead, `0x6E598` id 0x4A),
  `timer89=0`/latch and the stage-1 `+0x44`/lane gates with the reset path's
  ball install 4 + `0x79B58`. Install arms: `0x7D010..0x7D046` (phase 2 +
  `0x110680[type]&1` -> install 0x21) and the row-05 `FUN_0007F7E0` sites
  `0x7F791`/`0x7F7B6`.
* **Row 23** `0x82F84..0x83163` (132 insns): phase reset, `timer89 += delta`,
  `+0x9E=1`, stage 0->1, the stage-1 target arm (`dword[0x1577EE]>>16 > 0x70`,
  `word[0x1577FA] < word[0x157800]`, `slot[+0x10]&0x40` -> the 0x157794 triple
  with the `side`-signed 0xC0 z nudge else the 0x157788 triple, `0x79B58`),
  the `+0x85`/timer gates, the `0x82DD0` attempt, the install-0E path, the
  install-0F gates (`slot&0x40`, `+0x99`, `+0x5D`, `word[0x1577FA] >
  word[0x1577F2]`, `rec == [team+0x7C7]`, that record's lane/bound, the
  `0x1577F8/0x157800/0x1577FE` sum and the `0x8DC68` vector distance < 0x60)
  and the `+0x6B`/`+0x77` tail reset. Install arm `0x7D174..0x7D1B9`: phase 2
  + `0x110680[type]&1` + `[+0x99]==0` + `dword[+0x5D]==0` -> install 0x23.
* The gate passes for all three: each has a bounded install arm, the full
  record-visible body is covered by the tested loader functions
  (`fifa96_action_duel_step` incl. the resolution requests,
  `fifa96_action_receive_step` incl. the nearest arm,
  `fifa96_action_tackle_step`/`_attempt` incl. the target arm) and the pool
  binding exists (records, teams, ball pair, installer). Rows 18/21/23 flip to
  `ported`; the engine handlers are `fifa96_match_action_18/_21/_23`.
* **Erratum (tackle attempt camera gate).** The pre-Task-12 port's second
  camera-x gate was inverted: first-hand `0x82E8E..0x82E96` returns only when
  `camera_x > pos_x` (`MOV EAX,[0x15774C]; CMP EAX,[ESI+0x59]; JLE
  continue`), not when `camera_x < pos_x`. Fixed in
  `fifa96_action_tackle_attempt`, pinned by the discriminating fixture
  (`pos_x = 0x211`, `camera_x = 0` installs; `camera_x = 0x300` refuses) and
  the pre-existing `pos_x = 0x211` case updated to the native result.

### 10.6 Engine wiring and dispatch

`fifa96_match_action_18` stages the record into `fifa96_action_duel_step`
(district `+0x65` metric staged, OL-60 model), runs the bind request and, on
the resolution, the `+0x9A` latch + NSEARCH/SWAP over `mr->entities` and the
shared `FUN_0007DAB4` reset. `_21` stages the receive machine (camera copy,
nearest over the team records, reset/ball-4/receiver-timer pool side). `_23`
stages the tackle machine (target arm and installs 0x0E/0x0F; the `0x1577xx`
camera/track inputs are zero or the engine camera). The shared reset applies
the `FUN_0007C990` forced-decision bounded codes {3,4,6} (FU-141 OL-44) or the
code-0 install synchronously through the pool installer; a rejected install
(occupied/same code) leaves the native `+0x92 = 0xFF`. Dispatch counts move 7
-> 10 `FIFA96_OK` / 72 -> 69 `-UNSUPPORTED` (FU-137 §7, Task-12 errata).

### 10.7 Tests (this task)

* `tests/test_event_queue.c`: the four ring/sink fixtures (fill/no-fill, the
  1-based 25-wrap, the 0x26 suppression, the sink extras preserved on
  no-fill), plus layout `_Static_assert`s.
* `tests/test_action_possession.c`: `test_duel_search` (eligibility ladder,
  tie order, skip index, record-0 gate, chosen exclusion, fallback semantics,
  NULLs), `test_duel_swap`, `test_duel_bind`; the extended
  `test_duel_step` (resolution bind/occupied, target set), `test_receive_step`
  (phase/tracked, ran latch, nearest/anim, camera arm inputs), `test_tackle_*`
  (ran latch, target arms incl. the button/side nudge and the +0x99 receiver
  gate, the camera-gate fix).
* `tests/test_engine_match_handlers.c`: `test_action_18_runs_body`,
  `test_action_21_runs_body`, `test_action_23_runs_body` and the flipped
  `action_expect[0x18]/[0x21]/[0x23]`.

### 10.8 Open legs (numbered; registered in FU-142 §6)

* **OL-67 — event ring binding.** The ring/sink are loader-tested but no
  engine path drives them yet; the `0x157758` triple (FU-71 track writers
  `FUN_0006FFC0`/`0x736AC`) and the `[0x112E88]` stamp are caller inputs, and
  codes >= 0x28 (or negative sign-extended) are outside the embedded 0x28-byte
  eligibility table (the native reads the adjacent data).
* **OL-68 — rows 18/21/23 unmodeled record/presentation inputs.** Row 18's
  `[[rec+0x28]][0]` abort byte (staged 0; shares OL-52) and the bind globals
  `[0x157AC2]`/`[0x1587D4]`/`[0x1587E3]`/`[0x1074A4]`/`[0x14E574]` (the bind
  returns the derived fallback and the stub no-op; `mr->global_157ac2` is the
  run's zero-staged global); the SWAP slot-word clears; row 21's `+0x44`
  (staged 0), the camera-velocity zero, the `0x6E598` record writes (OL-52)
  and the `[0x79B58]` +0x99 gate (the pool has no +0x99; treated open); row
  23's `[0x1577CA]` exclusion (is_tracked stand-in), the `0x71B9C` predictor
  triple (camera stand-in), the `word[+0x85]`/`+0x99`/`+0x5D` byte gates and
  the `[team+0x7C7]` record (`is_own`/`opp_*` staged 0), the slot `+0x10`
  button byte and the `word[+0x77]` bound (shared OL-65). The shared
  `FUN_0007DAB4` forced-decision predicate inputs are the pool target
  fields (the FU-141 OL-44 bounded model), and its slot callback
  `FUN_00078B00` (`0x7DAD2`, called when `[rec+0x20] != 0`) is dropped by the
  engine's `match_row_reset` (the kick machine's `kick_reset` surfaces the
  same call as `slot_callback`). The receive `out.nearest`/`out.anim` and the
  bind `bound`/`stub_36200` outputs are computed for the loader tests and
  have no engine consumer (OL-52/OL-68 scope).
## 11. Task 13 port — row 06 pursuit machine (OL-30)

Reviewed read-only in `/FIFA96.EXE` (explicit; Ghidra MCP, no writes). This
section closes OL-30: the native row-06 body `0x801B4..0x809EF` is ported as
`fifa96_action_pursuit_step` (the contract comment in
`include/fifa96_loader/fifa96_action_handlers.h`) and action row 06 is wired
through `fifa96_match_action_06` over the FU-141 pool. The residual unmodeled
record/presentation inputs are the numbered leg OL-69.

### 11.1 Tool calls (first-hand, read-only)

* `disassemble_bytes`: `0x801B4..0x803B4` (155 insns), `0x803B4..0x80534`
  (116), `0x80534..0x806B4` (124), `0x806B4..0x808B4` (138),
  `0x808B4..0x80960` (50) — row 06 RET is `0x809EF` (~597 insns); the row-09
  function starts at `0x80A00` (its prologue `PUSH ... SUB ESP,0x10`) and its
  stage jump table is flat `0x809F0` `{0x80A3F,0x80BFB,0x80FCC,0x8103A}`;
* `read_memory 0x809F0` (16 B) and `0x1106E0` (40 B; slot 9 = `0x80A00`,
  confirming the row-09 boundary);
* `decompile_function`: `0x8DCD4` (`{distance,dx,dz}` triple),
  `0x8DD70` (`JMP 0xCD474` thunk), `0x8DC68` (octagonal distance),
  `0x795A4` (`(a*b+0x8000)>>16`), `0x8DE8C` (11 x 0xB2 nearest, skip-index
  argument, unsigned min 0xFFFF), `0x79CCC` (callback nearest, signed min
  0x7FBC, first-record fallback), `0x79C20` (`target = pos + dir*0x80`, y=0),
  `0x7D3E4` (x `±0x720`, z `±0xB10`), `0x79B58` (`+0x93 = 0x10` when
  `+0x99 == 0`), `0x7D9A4` (installer EAX/DX/BL/ECX, FU-137 §2),
  `0x7DAB4` (reset), `0x741B4` (`(param ^ [0x157ABE]) & 1`), `0x6DA64`
  (the +8/+0xC/+0x10/+0x14 dword and +0x90 byte swap), `0x4B100` (frame body:
  `[0x157A4F] ^= 1` at `0x4B11A`, store `0x4B129`), `0x4B02C`
  (`[0x157A4F] = 0` at `0x4B038`);
* `get_xrefs_to`: `0x158724` (writers `FUN_0007F144`, row 05, `0x819B7`),
  `0x15872A`/`0x15872F` (only row-05 writers), `0x157ABE` (writers
  `FUN_0007417C`/`0x742B4`), `0x157A4F` (writers `FUN_0004B02C`/`FUN_0004B100`,
  readers row 05 and row 06 `0x806C7`/`0x8082B`/`0x809A2`);
* Ghidra read-only: no renames, comments, labels, functions or saves.

### 11.2 Row 06 body `0x801B4..0x809EF` (site-annotated)

```
0x801BF byte[rec+0x9E] = 1
0x801D4 phase [0x157A4A]>>24 != 2 -> 0x7DAB4, RET
0x801E5 byte[rec+0x8D] == 0 -> 0x7DAB4; clear team+0x7B2/+0x7B6 when == rec; RET
0x8022D [0x158724] == 0 || (int16)(dword[carrier+0x69]>>16) > 0x90
        || dword[0x157750] > 0x70 -> 0x7D9A4(rec, 4, staged 0, invoke 1); RET
0x8026D 0x8DCD4(camera 0x15774C, {0,0,side?0xB10:-0xB10}) -> {dist,dx,dz};
        0x8DD70(dx,dz) -> angle; V1 = camera triple; scaled = (int16)dist
        (< 0x780 ? >>3 : >>4)
0x802F7 has_slot ? the 0x802FD..0x80354 offside c0/camdist arm (pos_z-cam_z
        word sign vs V2.z; 0x8DC68(pos_x-cam_x, d) < 0x150 -> V1 = pos triple
        and skip the fold; else scaled = 0xC0) : the 0x80359..0x803F5 no-slot
        arm (side 0: V1.z < -0x5A0 && V1.z < teammate_z; side 1: > +0x5A0 &&
        > teammate_z -> flag54; actor == team+0x7B2 -> timer89 += delta word
        and scaled -= low16(timer89), then 0x803BE..0x803EC compares
        score[idx(side)] vs score[idx(side^1)] (0x741B4 = `(param ^
        [0x157ABE]) & 1`, table 0x157AC5; `0x803EC JNC` skips unless own <
        other, unsigned word) and 0x803EE subtracts low16(timer89) again;
        clamp 0x30..0x150)
0x80410 fold V1.x += 0x795A4(scaled, 0x114E04[sin(angle)]);
        V1.z += 0x795A4(scaled, sin(angle+0x100))
0x80482 actor == team+0x7B6 -> 0x8DCD4(V1, V2) -> angle2; flag54 &&
        |V1.z| < 0x930 -> speed = dword[rec+0x69] >> 18, else speed = 0x60;
        V0.x = V1.x + fold(speed, angle2); V0.z = V1.z + fold(speed,
        angle2+0x100)
0x805BC target: has_slot && (byte[slot+0x10]&0x30) -> camera triple;
        has_slot -> 0x79C20(pos, (int8)slot+0x20, (int8)slot+0x21);
        no slot && word[+0x81] != 0 -> target = pos triple, RET;
        no slot && dword[0x157750] > 0x38 -> RET; else target = V1
        (actor == team+0x7B2) or V0; flag54 -> 0x79B58(rec)
0x8064B V4: wx = word[+0x6D] + ([0x1577C0]<<2), wz = word[+0x6F] +
        ([0x1577C2]<<2) (16-bit stores); 0x8DC68(wx,wz) = metric;
        metric <= 0x60 -> install 8; metric > 0x60 && +0x99 == 0 &&
        carrier word +0x71 > 4 -> [parity && byte[0x15872F] < 2 ->
        rng; (rng&0xF) > (desc[+0xC] | +0x9D) -> target.x += byte[0x15872A]<<6]
        [metric <= 0x90 -> base = (desc[+0xE] | +0x9D) << (3 - (int8)+0x90 +
        (int16)(score_other - score_own)); > 0xF clamp; rng; (rng&0x1FF) <
        base -> install 9]
0x807A9 install != 0 -> 0x7D9A4(rec, install, staged 0, invoke 1)
0x807C6 0x7D3E4 target clamp; row_byte [[rec+0x28]][0] == 0x1C ?
        (word[+0x71] > 4 || word[+0x6B] > 0xC0 -> 0x6E598 id 2)
        : (word[+0x71] < 3 && word[+0x6B] < 0x90 -> 0x6E598 id 0x1C)
0x8082B [0x157A4F] == 0 -> RET; actor != team+0x7B2 -> 0x809A2;
        else 0x8DE8C(V1, team, skip 0) -> best != rec ? (team+0x7B6 = 0,
        team+0x7B2 = best) : (team+0x7B2 = rec; 0x808AD CMP CX,[0x8DCD4(
        carrier, V1).distance]; 0x808B2 JL 0x80903 -> self >= carrier mirrors:
        V1 += {dx,dz}; 0x8DE8C(V1, team, skip 0, +0x9A latched) ->
        team+0x7B6; self < carrier -> team+0x7B6 = 0; flag54 ->
        (second == 0 || |second.z|+0x60 < |carrier.z|) ?
        0x8DE8C({0,0,camera_z>=0?0xB10:-0xB10}, team, skip 0, latched) ->
        team+0x7B6)
0x809A2 [0x157A4F] != 0 -> 0x79CCC(pos, team, skip 0, +0x9A latched);
        best != rec && distance < 0xC0 -> 0x6DA64(best, rec); RET
```

Widths re-read from the raw bytes: the `dword[addr]>>16` idiom is the word at
`addr+2` (`[carrier+0x69]` -> `+0x6B`, `[rec+0x69]` -> `+0x6B`, `[rec+0x6F]`
-> `+0x71`, `[0x15872C]` -> `0x15872F`, `[0x158727]` -> `0x15872A`,
`0x1587E3`-style byte reads); the `CALL` targets were diffed against the `E8`
bytes (`0x8025E`/`0x807C1`->`0x7D9A4`, `0x802BE`/`0x808A8`->`0x8DCD4`,
`0x802D6`/`0x804B5`->`0x8DD70`, `0x80333`/`0x80896`->`0x8DC68`,
`0x80439`/`0x80472`/`0x80508`/`0x80571`/`0x805AC`->`0x795A4`,
`0x805F4`->`0x79C20`, `0x80646`->`0x79B58`,
`0x806F0`/`0x80791`->`0x92AC8`,
`0x80850`/`0x808EB`/`0x8098C`->`0x8DE8C`,
`0x809BE`->`0x79CCC`, `0x809E1`->`0x6DA64`).

Boundary note: the plan/brief span `0x801B4..0x81067` (and FU-142 §3's
"597 insns") covers the row-09 handler `0x80A00..0x81065` past the row-06
`RET 0x809EF`; row 09 is the action-table slot `0x1106E0[9]` and stays
unwired (OL-9). The row-06 port bounds only `0x801B4..0x809EF`.

### 11.3 The `0x114E04` fold reuse

The five row-06 fold sites (`0x80439`, `0x80472`, `0x80508`, `0x80571`,
`0x805AC`) carry the same inline byte-shift quadrant idiom (`SHL AH,7;
SBB EDX,EDX; ADD AH,AH; SBB ECX,ECX; XOR EAX,ECX; AND EAX,0xFF; SUB
EAX,ECX; MOV EAX,[EAX*4+0x114E04]; XOR EAX,EDX; SUB EAX,EDX`) as FU-139 §9's
`kick_sin`/`kick_fold`. Task 13 exposes Task 11's implementation as
`fifa96_ball_fold` (`include/fifa96_loader/fifa96_ball_pairing.h`; the
`kick_fold` uses now call it) instead of duplicating the 257-entry table in
`fifa96_action_handlers.c`. The fold speed is the native 32-bit EBX: the
camera arm passes the sign-extended `(int16)scaled` and the V0 arm passes
`dword[rec+0x69] >> 18` (native `SAR 0x12`).

### 11.4 Engine wiring

`fifa96_match_action_06` stages the record and the team mate view
(`fifa96_action_pursuit_mate`: x/z words for the searches and the full
`pos_z` dword for the `0x8092B`/`0x80943` height gate), resolves the
`[0x158724]` carrier stand-in as the pool ball carrier (`fifa96_match_entities
.ball.carrier`; the row-05 claim is unported, OL-63), the team
`+0x7B2`/`+0x7B6` identities, `teammate_z`, the run scores (`mr->score`) and
the `[0x157A4F]` parity (the run's new `pass_parity`, toggled once per granted
frame body at the FUN_0004B100 site `0x4B11A`; cleared by init/begin like
FUN_0004B02C), then drains the loader's requests: the target triple, the
`install` 4/8/9 into the pool installer, `receiver_timer` -> `timer93 = 0x10`,
the `team.target`/`team.second` index writes (SELF -> the actor id, NONE ->
`FIFA96_MATCH_ENTITY_NONE`), the `clear_*` flags and the shared
`FUN_0007DAB4` reset (`match_row_reset`). `out.anim` (the `0x6E598` call) and
`out.swap` (the `0x6DA64` metadata swap) have no derived consumer (OL-52/OL-69).

Row-06 install arms (first-hand): the carrier-gate `0x8025E` `EDX=4, ECX=1`;
the gate arms `0x807C1` with EDX = the `+0x50` word (8 or 9), ECX=1, EBX=0.
The native invoke-now semantics are the derived `record.install` request
(the FU-139 §9/§10 drain model).

Entry arm: `FUN_0007C990` (decompiled this slice) installs code 6 through
`FUN_0007D9A4(rec, 6, 0, 0)` when the record is `team+0x7B2` (or
`team+0x7B6`) and the opponent's target record holds the ball
(`byte[+0x9F] & 1`) — the same forced-decision arm the shared
`FUN_0007DAB4` reset tail calls. The derived engine's
`match_forced_decision_code`/`match_row_reset` (FU-141 OL-44 bounded model)
reproduces the predicate, so the row-06 entry is bounded; the wiring gate
(arm + full record-visible body + pool binding) passes.

### 11.5 Tests

* `tests/test_action_handlers.c`: `test_pursuit_entry_and_carrier_gate` (the
  +0x9E latch, phase/active resets, the clear-identity flags, the three
  install-4 gates, NULL args), `test_pursuit_fold_and_install_gate` (the
  hand-computed camera metric/fold target (-0xD0, 0x11, 0xB10), the install-9
  gate with the seed-0 first RNG draw 0x200 (`0x200 & 0x1FF = 0 < base`), the
  zero-base refusal, the +0x99
  block and the parity/0x15872F x-adjust draw), `test_pursuit_second_record_v0`
  (the V0 speed-0x60 arm -> (0,0,0x111)), `test_pursuit_slot_targets`
  (camera vs `0x79C20` + the `0x7D3E4` clamp), `test_pursuit_early_returns`
  (the +0x81 position copy without clamp and the >0x38 height return),
  `test_pursuit_claim_arm` (the `0x8DE8C` non-self write and the
  `0x79CCC`/`0x6DA64` swap), `test_pursuit_carrier_mirror_equal` (the
  `0x808AD` `>=` mirror and the height-gate skip),
  `test_pursuit_carrier_no_mirror` (the `0x808B2 JL` clear direction),
  `test_pursuit_height_gate_research` (the `0x80952` gate's fail/re-search
  side; the skip side is the `test_pursuit_carrier_mirror_equal` fixture),
  `test_pursuit_score_second_subtraction` (the
  `0x803BE..0x803EE` own < other unsigned branch, including the 0x8000 case),
  `test_pursuit_adjust_gate_signed` (the `(int8)0x15872F < 2` gate with 0xFF)
  and `test_pursuit_invalid`.
* `tests/test_engine_match_handlers.c`: `action_expect[0x06] = FIFA96_OK` and
  `test_action_06_runs_body` (the reset path, the carrier-gate install 4, the
  no-slot V1 target (-0xB1) + install 8, the parity claim arm and the live
  score second subtraction -> -0x91 over the pool).
* `make check` 103/103 (engine tests under ASan/UBSan); M1 golden/render pins
  unchanged.

### 11.6 Repo state (this task)

Action rows wired 10/80 -> **11/80** (`06`); dispatch 69 UNSUP / 10 OK /
1 NOTF -> **68 UNSUP / 11 OK / 1 NOTF**; action class `06` `not ported` ->
`ported`; FU-137 §7 totals move with it. Out-of-brief file: the loader header
contract, `CMakeLists.txt` (`fifa96_action_handlers` now links
`fifa96_ball_pairing` for the shared fold), `fifa96_match_run.{h,c}` (the
`pass_parity` run field) and the `fifa96_ball_fold` exposure in
`fifa96_ball_pairing.{h,c}`; all named in the Task 13 report.

### 11.7 Open legs (numbered; registered in FU-142 §6)

* **OL-69 — row 06 unmodeled record/presentation inputs.** The record bytes
  `+0x90` (the install-9 shift), `+0x99` (the gate block), `+0x9D` (the
  install-9 OR term), the roster-descriptor bytes `rec[+4][+0xC]` (the
  x-adjust gate) and `rec[+4][+0xE]` (the install-9 base), the row byte
  `[[rec+0x28]][0]` (the `0x6E598` id gate, shares OL-52), the camera-track
  lead words `0x1577C0`/`0x1577C2` and the `+0x6F` word are staged zero in
  the engine; the row-05 `0x15872A`/`0x15872F` bytes (its carrier claim
  producers, OL-63) are **live producers since M2 phase-9 T2** (row 05 is
  wired and writes them; §8.6 / FU-137 §7) — the remaining row-06 input bytes
  above stay staged zero; the `[0x157ABE]` side-mirror byte behind
  the score index is staged 0; the `[0x158724]` carrier is the pool ball
  carrier stand-in; the `0x79CCC` callback position is the record's own
  position (the `[rec+0x1C]` phase handler is unported); the `0x6DA64` swap
  moves opaque record metadata the derived pool does not model. The native
  install-9 shift leaks the descriptor-pointer upper bits when the shift
  count exceeds 16; the derived model shifts the low 16 bits (unreachable for
  the bounded shift range and the real score deltas) and records the choice
  here. No parity claim is made over the staged-zero inputs.
