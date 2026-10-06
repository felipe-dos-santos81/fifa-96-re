# FU-132 — dispatcher state producers

Cycle 2, Task 3. FU-127 §5 leg 2 left the producers of the dispatcher's four
state globals unknown. This slice finds the static writers, records the guard
that sets each state, and names the `FUN_000435xx` cluster that drives the
special-render sub-state.

All probes are on the authoritative program **`/FIFA96.EXE`** (native Watcom LE
loader, fixups applied; FU-131), at the **real** addresses. The operands render
with MCP leading zeros (`[0x0010677c]`), so the substring searches use
`10677c` / `106780` / `109090` / `109094`.

Result in one line: **`[0x10677C]` is the match/sub-state machine written by
`FUN_00044f24`(1)/`FUN_000436e4`(2)/`FUN_0004372c`(3) and reset by
`FUN_000443e8`, `FUN_00044d7c`(→1 on match start) and `FUN_00044e3c`;
`[0x106780]` is the special/held-render flag written by `FUN_00051270`(1),
`FUN_000512b0`(0), `FUN_00058d70`(2) and cleared by `FUN_00044d7c`; the two
counters `[0x109090]`/`[0x109094]` have single static producers — `FUN_00058bc0`
(inc/reset) and `FUN_000590b0` (camera delta) — and every `FUN_000435xx` xref
comes only from the special-render path `FUN_00058bc0`.**

## 1. Search evidence

`ghidra_search_instructions(program="/FIFA96.EXE", operand_pattern=…)`:

| pattern | hits | writers (store into the global) |
|---|---|---|
| `10677c` | 13 | `FUN_000436e4`@`0x43701`, `FUN_0004372c`@`0x437d6`,`0x4381b`, orphan@`0x438e5`, `FUN_000443e8`@`0x444de`, `FUN_00044d7c`@`0x44dd0`,`0x44e0f`, `FUN_00044e3c`@`0x44ef0`, `FUN_00044f24`@`0x44f46` |
| `106780` | 5 | `FUN_00043610`@`0x43610` (the setter), `FUN_00044d7c`@`0x44e2b`; reads in `FUN_00043608`, `FUN_00043b48`@`0x43d1f`, `FUN_00043f68`@`0x43f80` |
| `109090` | 3 | `FUN_00058bc0` only (`INC`@`0x58cc7`, `MOV`@`0x58cd6`); `CMP 3`@`0x58cae` |
| `109094` | 4 | `FUN_000590b0` only (`MOV EBX`@`0x59142`, `MOV EDX`@`0x59189`); reads in `FUN_00058bc0`@`0x58bdd`, `FUN_00058d70`@`0x58d80` |

`ghidra_get_bulk_xrefs(program="/FIFA96.EXE", addresses="0x10677c")` returns 5
`WRITE` xrefs typed `FUN_000443e8`/`FUN_00044d7c`×2/`FUN_00044e3c`/`FUN_00044f24`
plus the `0x438e5` write, matching the search.

## 2. Producers

`state | writer | trigger (guard) | consumers`

| state | writer | trigger (guard) | consumers |
|---|---|---|---|
| `[0x10677C]` | `FUN_00044f24` @`0x44f46` | `[0x106768]!=0 && param_3 != [0x106790]` → set `=1`, latch `[0x106790]=param_3`, `FUN_0008eb78(1)`, `FUN_000603ec(1)` (mode/selection change) | `FUN_0004382c` getter (callers `FUN_000495b0`@`0x4960a`, `FUN_00058bc0`@`0x58bd3`); direct `CMP==3` in `FUN_00043b48`@`0x43ba4`; `>0` test in `FUN_000436e4`@`0x436f2`; `==3` in `FUN_0004372c`@`0x43744` |
| `[0x10677C]` | `FUN_000436e4` @`0x43701` | `[0x106768]!=0 && [0x10677C]>0` → set `=2`; `FUN_000ce6f0`, `FUN_0009a9e4`, `FUN_000439d0` (special/replay step) | `FUN_00058bc0` (special-render path, calls it @`0x58c46`/`0x58c83`) |
| `[0x10677C]` | `FUN_0004372c` @`0x437d6`,`0x4381b` | `[0x106768]!=0` (after `FUN_000439d0`; when `[0x10677C]==3 && [0x10678C]>0 && [0x106788]!=0` drains `[0x10678C]-2` events) → set `=3` | `FUN_00058bc0` @`0x58cdb` |
| `[0x10677C]` | `FUN_00044d7c` @`0x44dd0`,`0x44e0f` | match start (`[0x106770]==0`): sets `=1`, clears `[0x106780]=0`, `[0x10678C]=0`, `[0x106788]=0`, `[0x106790]=-1`, `[0x106770]=1` | dispatcher via getter (FU-126 §1) |
| `[0x10677C]` | `FUN_00044e3c` @`0x44ef0` | match teardown (`[0x106770]!=0`): resets `=0`, clears `[0x106770]`, sets `[0x10676C]=1`, flushes `[0x106768]` | dispatcher via getter |
| `[0x10677C]` | `FUN_000443e8` @`0x444de` | setup/init (`param_1` clamped to `0..2`, index into `[0x1067B4]`/`[0x1067BC]`) → set `=0`; also clears `[0x10678C]`, `[0x106788]`, `[0x106790]=-1` | dispatcher via getter |
| `[0x10677C]` | orphan `0x438e5` | no function/xref (see open leg 1): after `[0x106768]!=0` cleanup, `XOR ESI,ESI` → set `=0` | none static |
| `[0x106780]` | `FUN_00043610` @`0x43610` | the generic setter (`[0x106780]=EAX`); every below writer funnels here or stores directly | `FUN_00043608` getter (callers `FUN_000495b0`@`0x4963c`, `FUN_00058d70`@`0x58d89`); `CMP==2` in `FUN_00043b48`@`0x43d1f`; read in `FUN_00043f68`@`0x43f80` |
| `[0x106780]` | `FUN_00044d7c` @`0x44e2b` | match start: `=0` (same block as `[0x10677C]=1`) | as above |
| `[0x106780]` | `FUN_00051270` @`0x5128d` | `FUN_0008f178()!=0` → `FUN_00043610(1)`, `FUN_0004cd70`, `FUN_00063734(1)` (start held mode) | as above |
| `[0x106780]` | `FUN_000512b0` @`0x512dd` | `FUN_00063734`/`FUN_000510dc` path returns 0 → `FUN_00043610(0)` (stop held mode) | as above |
| `[0x106780]` | `FUN_00058d70` @`0x58d98`,`0x58df5` | held/special render path sets `=2` (both at entry when `[0x109094]==0 && FUN_00043608()!=1`, and at exit) | as above |
| `[0x106780]` | orphan `0x51396` | no function/xref (open leg 2): mirror of `FUN_00051270`, `MOV EAX,1; CALL FUN_00043610` → set `=1` | none static |
| `[0x109090]` | `FUN_00058bc0` @`0x58cc7`,`0x58cd6` | fast/special branch (`[0x109094]!=0 \|\| [0x10677C]==1`): `INC` each call, or reset `=0` when `FUN_00063fd0()==0` leaves the state; `CMP >3` → `FUN_00012c60` | `FUN_00058bc0` (own threshold test @`0x58cae`) |
| `[0x109094]` | `FUN_000590b0` @`0x59142`,`0x59189` | `delta = [0x15426C]-param_2[0]`; if `delta==0 && [0x154270]==param_2[1] && [0x154274]==param_2[2] && FUN_0005903c(param_2)!=0` → store `delta`, else store `1` | `FUN_00058bc0`@`0x58bdd` (gate), `FUN_00058d70`@`0x58d80` (gate) |

Only `[0x109090]` and `[0x109094]` have a single static producer; `[0x10677C]`
has seven write sites (six functions + one orphan) and `[0x106780]` has five
(one generic setter + three direct stores + one orphan).

## 3. The `FUN_000435xx` cluster

All xrefs to the cluster targets come **only** from `FUN_00058bc0` (the
special-render path), evidence:

```
ghidra_get_bulk_xrefs(addresses="0x43568,0x435c0,0x435d0,0x436e4,0x4372c")
0x43568: 0x58c28, 0x58cfc   0x435c0: 0x58ce0     0x435d0: 0x58d37, 0x58d46
0x436e4: 0x58c46, 0x58c83   0x4372c: 0x58cdb
```

| function | role |
|---|---|
| `FUN_00043568` | predicate over `[0x14B018]`: returns `1` unless `[0x14B018] ∈ {5,6,9,10}`; gates the `FUN_00043568`-tested branches (after `FUN_0006d1b2`/`FUN_0006d1e5`/`FUN_000377a0`) |
| `FUN_000435c0` | predicate: `return [0x14B018] == 2` |
| `FUN_000435d0` | predicate: `return [0x14B018] ∈ {0xB,0xC,0xD,0x19}` |
| `FUN_000436e4` | sub-state producer: writes `[0x10677C]=2` (see §2) |
| `FUN_0004372c` | sub-state producer: writes `[0x10677C]=3` (see §2) |

So the cluster is a phase-classifier (`FUN_00043568`/`0x435c0`/`0x435d0`, all
keyed on the match-phase global `[0x14B018]`) plus the two sub-state producers,
and the special-render path is its only caller. In `FUN_00058bc0` the slow
branch calls `FUN_0004372c` first (`0x58cdb`), then `FUN_000435c0` (`0x58ce0`),
then `FUN_000435d0` (`0x58d37`/`0x58d46`), and `FUN_00043568`
(`0x58c28`/`0x58cfc`) decides whether the match is in a normal phase;
`FUN_000436e4` (`0x58c46`/`0x58c83`) is called in both the fast and slow
branches.

`[0x14B018]`'s value set (`{2}`, `{5,6,9,10}`, `{0xB,0xC,0xD,0x19}`) is the
match-phase enum consumed by the cluster; its own producer is outside this
slice (open leg 3).

## 4. Ghidra project changes

Program `/FIFA96.EXE` (only), via `ghidra_set_comment(type="plate")` on the
producer functions and the four globals, then `ghidra_save_program`:

* Functions annotated: `FUN_00043568`, `FUN_000435c0`, `FUN_000435d0`,
  `FUN_000436e4`, `FUN_0004372c`, `FUN_000443e8`, `FUN_00044d7c`,
  `FUN_00044e3c`, `FUN_00044f24`, `FUN_000590b0`.
* Globals `0x10677C`/`0x106780`/`0x109090`/`0x109094`: FU-131 plate comments
  extended with the producer summary.
* No labels renamed; no function bodies created/repaired (the two orphan
  writers at `0x438e5`/`0x51396` remain inside undefined output, see legs).
* `/fifa96_le.bin` was not modified.

## 5. Provenance

Ghidra MCP on `/FIFA96.EXE`: `search_instructions` (`10677c`, `106780`,
`109090`, `109094`), `get_bulk_xrefs` (`0x10677c`, `0x4382c,0x43608,0x43610`,
`0x43568,0x435c0,0x435d0,0x436e4,0x4372c`), `get_xrefs_to` (`0x10677c`,
`0x438d0`, `0x438c4`, `0x51370`), `get_function_by_address`
(`0x438e5`,`0x43834`,`0x4960a`,`0x5128d`,`0x512dd`,`0x51396`,`0x58d70`),
`decompile_function` (`0x43608`,`0x43610`,`0x436e4`,`0x4372c`,`0x443e8`,
`0x44d7c`,`0x44e3c`,`0x44f24`,`0x43568`,`0x435c0`,`0x435d0`,`0x58bc0`,
`0x58d70`,`0x590b0`,`0x495b0`,`0x51270`,`0x512b0`), `disassemble_bytes`
(`0x438b8`,`0x438d0`,`0x51360`), `batch_get_comments`, `set_comment`,
`save_program`. Run via `ghidra_list_open_programs`; `FIFA96.EXE` was current.

## 6. Open legs

1. **Orphan writer `0x438e5` (sets `[0x10677C]=0`).** Bytes at
   `0x438c4..0x438ef` form a complete routine (`PUSH`/`TEST [0x106768]`/`CALL
   FUN_0009a16c`/clear `[0x106768]`/clear `[0x10677C]`/`RET`) immediately after
   `FUN_00043834` (body ends `0x438c3`), but Ghidra has no function there and
   `get_xrefs_to 0x438c4`/`0x438d0` are empty. Its entry is not statically
   reachable; whether it is dead code or an entry reached only via a runtime
   pointer is unresolved.
2. **Orphan writer `0x51396` (sets `[0x106780]=1`).** The block
   `0x51370..0x513b1` mirrors `FUN_00051270` (calls `FUN_00043610(1)`,
   `FUN_0004cd70`), sits just past `FUN_000512b0` (body ends `0x51367`), and has
   no function or xref (`get_xrefs_to 0x51370` empty). Same unresolved status.
3. **`[0x14B018]` producer/values.** The `FUN_000435xx` predicates classify it
   as a match-phase enum, but its writer(s) and exact value names are not
   identified here (it is the phase variable from earlier slices, e.g. FU-83).
4. **`[0x106768]`/`[0x106788]`/`[0x10678C]`/`[0x106790]` semantics.** The
   producers read/write these adjacent dwords (`[0x106768]` = a handle flushed
   by `FUN_0009a16c`; `[0x106790]` = last-selected mode; `[0x10678C]` = event
   count drained by `FUN_0004372c`), but their full meaning is left to a
   match-lifecycle slice.
5. **Runtime reachability of the direct stores.** The stores in
   `FUN_00044d7c`/`FUN_00044e3c`/`FUN_00044f24` are confirmed statically; the
   callers of those functions are not traced here, so the exact game event that
   invokes each is inferred from its guard, not from a call-site.
