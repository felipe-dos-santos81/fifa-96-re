# FU-259 (draft) — OL-65: the two-line staging fix and the row-07 `0x816DF`
# invoke (action `0x22`)

Provenance: M2 phase-25 wave **W25-4** (planned FU-259), 2026-10-14, **read-only**.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; standard tools only; **no** rename / comment /
label / function-create / script / project save). `get_current_program_info` this
session: name `FIFA96.EXE`, path `/FIFA96.EXE`, `watcom:LE:32:default`, image
base `0`, `min_address 0x10000`, **2706 functions / 15239 symbols**. Every
address/byte/instruction/xref below is a fresh `disassemble_bytes` /
`read_memory` / `get_xrefs_to` / `get_current_program_info` of **this session**
unless marked *cross-cited* (then the frozen FU doc is named). Engine/loader
source is read first-hand. The alias traps are applied (`dword[x]>>16` = the word
at `x+2`; `>>24` = the byte at `x+3`); the row-07 CALL rel32 is verified
byte-level. **No repo file was modified beyond this draft + the report; no git; no
subagents.**

Question (plan Task 4 / brief §Step 1-4): turn FU-250's finding — the three
native angle words are live pool fields, the engine hardcodes two of them to 0 —
into a port plan: the exact two-line staging fix, the `0x816DF` invoke condition,
whether the invoke then **fires** on a fixture/live row-07 dispatch, and the
**action-`0x22` census ruling**; all while M2 stays byte-identical.

---

## 1. Headline

- **The fix is exactly two lines**, `fifa96_match_engine/fifa96_match_handlers.c`
  `:984-985`:
  `s->opp_angle_z = opp->cam_dz6f;` (native `word[+0x6F]`) and
  `s->opp_face_word = opp->face7d;` (native `word[+0x7D]`). `opp_angle_x =
  opp->lane_z` (`:983`) **stays** — it is the correct native `word[+0x6D]`
  (FU-250 §1, re-confirmed here).
- **The invoke fires iff** `fold10(entity_angle(word[+0x6D], word[+0x6F]) −
  word[+0x7D]) < 0x55` (fresh `0x816CC CMP EAX,0x55` / `0x816CF JGE`). With the
  two fields re-sourced the gate compares the record→camera direction
  `entity_angle(cam_dx, cam_dz)` against the record facing `face7d` — a
  geometrically satisfiable comparison. **The current staging (`0/0`) makes it
  unsatisfiable** (`diff ∈ {0x100, 0x80, 0x100} > 0x55`, FU-250 §3): confirmed.
- **Reachability, first-hand:** the loader machine already has a **fixture that
  proves `out.opponent_invoke == 1`** with staged angle cells
  (`tests/test_action_handlers.c:800-814`: `opp_angle_x=1, opp_angle_z=0,
  opp_face_word=0x100` → `opponent_invoke == 1`). With the fix, the engine's
  `match_kick_from_record` feeds the same three cells from live pool fields, so
  an engine-level fixture staging the opponent record's angle cells makes the
  invoke fire identically. The invoke then installs **code `0x22`** on the
  opponent record (`match_kick_invoke_opponent` → `fifa96_match_entities_install`).
- **Dispatch** of the installed row needs the action-table slot registered; the
  slot is `{0x22, NULL}` (`handlers.c:5522`), so a full count needs the fix **and**
  the slot registration **and** a fixture proving install **and** dispatch.
- **M2 byte-identity is NOT statically provable.** The two re-sourced fields are
  consumed *only* by the row-07 post-kick gate (verified: the producers are
  `:984-985`; the sole consumers are the loader gate `fifa96_action_handlers.c`
  `:1701-1705`), so the fix is behaviorally inert *unless* the gate reaches its
  angle comparison — but the m2 tape **stages row `0x07`** (`M2_STAGE_ROWS`,
  `tests/test_engine_m2.c:2110-2111`) and drives it through a kick, so the gate
  *is* reachable on the tape. The port's acceptance is a **golden re-run**
  (`cmp` against `tests/golden/engine/m2-frames.txt`); if that moves, the gate
  fired live and the movement is itself the strongest install evidence. Recorded
  as a numbered leg (§7), **not** asserted.
- **Census ruling (row `0x22`):** uncounted in the current tree (**58/80 = 72%**);
  it becomes countable (**59/80 = 73%**) only when the two-line fix **and** the
  slot registration land together with a fixture proving install+dispatch while
  the m2 golden is confirmed unchanged. Recommendation for this wave: stay
  uncounted.

---

## 2. Step 1 — the engine's row-07 gate staging vs the native reads

### 2.1 The engine, first-hand (`fifa96_match_handlers.c:982-985`)

```
s->opp_lane_word = opp->lane_x;    /* +0x6B (<0xD0 gate) */
s->opp_angle_x   = opp->lane_z;    /* dword[+0x6D]>>16 = word[+0x6D] */
s->opp_angle_z   = 0;              /* word[+0x6F] unmodeled (OL-65) */
s->opp_face_word = 0;              /* +0x7D facing word unmodeled (OL-65) */
```

`opp` is the opposing team's target record (`oid = mr->entities.team[1u-team]
.target`, `:943`/`:968-972`); the same record the native gate reads
(`[team+0x7A6]+0x7B2`). The three loader header comments already name the native
words (`include/fifa96_loader/fifa96_action_handlers.h:914-919`).

### 2.2 The native reads, fresh bytes

Fresh `disassemble_bytes 0x81643..0x81696` (84 B) + `0x81691..0x816e3` (83 B;
25 instr) + `read_memory 0x7C776` (48 B):

| addr | bytes | instruction | meaning |
|---|---|---|---|
| `0x8169f` | `8b 56 6d` | `MOV EDX, dword[ESI+0x6D]` | **dword** at +0x6D |
| `0x816a2` | `8b 46 6b` | `MOV EAX, dword[ESI+0x6B]` | **dword** at +0x6B |
| `0x816a5` | `c1 fa 10` | `SAR EDX,0x10` | `EDX = word[+0x6F]` (0x6D+2) |
| `0x816a8` | `c1 f8 10` | `SAR EAX,0x10` | `EAX = word[+0x6D]` (0x6B+2) |
| `0x816ab` | `e8 c0 c6 00 00` | `CALL 0x8DD70` | `entity_angle(word[+0x6D], word[+0x6F])` |
| `0x816b0` | `66 2b 46 7d` | `SUB AX, word[ESI+0x7D]` | `− word[+0x7D]` |

The thunk `0x8DD70` (fresh 32 B) forwards `EAX`→first arg `x`, `DX`→second arg
`z`; fresh `get_xrefs_to 0x8DD70` = **28** (incl. our `0x816ab`). The three
native inputs are therefore `word[+0x6D]` (= pool `lane_z`), `word[+0x6F]` (=
pool `cam_dz6f`) and `word[+0x7D]` (= pool `face7d`).

### 2.3 The producers are live pool fields (first-hand)

- `lane_z`/`cam_dz6f`: the BF20 lane block `match_run.c:1459-1461` writes
  `e->lane_z = cam_dx; e->cam_dz6f = cam_dz;` every dispatched frame from
  `fifa96_action_locomotion_track(...)`. Native writer fresh at `0x7C776`:
  `MOV AX,[0x15774C]; SUB EAX,pos_x; MOV [+0x6D],AX` and `MOV AX,[0x157754];
  SUB EAX,pos_z; MOV [+0x6F],AX` → `+0x6D = cam_x − pos_x`, `+0x6F = cam_z −
  pos_z`.
- `face7d`: written by the face arms and synced record→pool each dispatch
  (`match_run.c:1378` `r->face7d = e->face7d;` / `:1514` `e->face7d =
  r->face7d;`).

### 2.4 The exact two-line fix

```
-  s->opp_angle_z   = 0;
-  s->opp_face_word = 0;
+  s->opp_angle_z   = opp->cam_dz6f;   /* word[+0x6F] */
+  s->opp_face_word = opp->face7d;     /* word[+0x7D] */
```

`opp_angle_x = opp->lane_z` is unchanged. No loader change (it already passes
`fifa96_action_kick_angle(s->opp_angle_x, s->opp_angle_z)` as `(x,z)`,
`fifa96_action_handlers.c:1701`). The two stale comments at `:984-985` are
corrected in the same edit.

---

## 3. Step 2 — the invoke condition (fresh bytes)

Full gate block, fresh (`0x8164b..0x81697`):

| addr | bytes | instruction | gate |
|---|---|---|---|
| `0x8164b` | `66 85 c0` / `0f 84 …` | `TEST AX,AX; JZ 0x816E4` | staged kick (`out.kick`) |
| `0x81654` | `a1 3c 87 15 00` `c1 f8 10` `83 f8 30` `0f 8d …` | traj word `< 0x30` | `kick_traj < 0x30` |
| `0x8166e` | `8b 95 89 00 00 00` `83 fa 05` `7d 65` | `CMP dword[EBP+0x89],5` | `timer89 < 5` |
| `0x8167f` | `85 f6` `74 61` | `TEST ESI,ESI` | opp present |
| `0x81683` | `8b 86 8e …` `c1 f8 18` `83 f8 06` `75 53` | `byte[ESI+0x91]==6` | `opp_type == 6` |
| `0x81691` | `83 7e 20 00` `75 4d` | `dword[ESI+0x20]==0` | `opp_has_slot == 0` |
| `0x81697` | `66 81 7e 6b d0 00` `7d 45` | `word[ESI+0x6B] < 0xD0` | `opp_lane_word < 0xD0` |

then the fold and threshold (§2.2): `0x816b4 AND AH,0x3` (fold to 10 bits),
`0x816ba CMP EDX,0x200 / JLE` → `0x816c2 MOV EDX,0x400 / SUB EDX,EAX` (cancel
past a half-turn), `0x816cb CWDE`, **`0x816cc CMP EAX,0x55`**, `0x816cf JGE
0x816e4` — **fire iff folded diff `< 0x55`**. This is mirrored exactly by the
loader gate `fifa96_action_handlers.c:1695-1709`.

Installer (fresh `0x816d1..0x816df`): `MOV ECX,1` (invoke-now); `MOV EDX,0x22`;
`MOV EAX,ESI` (record); `XOR EBX,EBX`; `CALL 0x7D9A4` (rel32 `0xFFFFC2C0 =
−0x3D40`; `0x816E4 − 0x3D40 = 0x7D9A4` verified). Fresh `get_xrefs_to 0x816DF`
= **0** (a CALL site, not a target).

---

## 4. Step 3 — reachability with the fix

### 4.1 The loader-level precedent (already on the tree)

`tests/test_action_handlers.c:800-814` stages a row-07 machine with
`opp_present=1, opp_type=6, opp_has_slot=0, opp_lane_word=0x80, opp_angle_x=1,
opp_angle_z=0, opp_face_word=0x100` and asserts `out.opponent_invoke == 1`
(`entity_angle(1,0) = 0x100`; folded diff `0` `< 0x55`). **The gate's arithmetic
and its firing path are fixture-proven at the loader layer today.** The only gap
is that the *engine* never hands it non-zero `opp_angle_z`/`opp_face_word`.

### 4.2 The engine route

1. `fifa96_match_action_07` (`handlers.c:1145-1169`) runs on a live row-07
   record; a kick stages (`out.kick`), `match_kick_run` sets
   `s.kick_staged = bo.staged`, `s.kick_traj = ball.pair.traj`, then the second
   `fifa96_action_kick_machine(&s, &out)` evaluates the gate (`:1162`).
2. With the §2.4 fix, `s.opp_angle_z`/`s.opp_face_word` are the **live** opponent
   cells captured at `match_kick_from_record` (top of the handler), so the gate
   compares `entity_angle(opp->lane_z, opp->cam_dz6f)` against `opp->face7d`.
3. On fire (`out.opponent_invoke != 0`), `match_kick_invoke_opponent`
   (`:1122-1132`) installs code `0x22` on the **same** opponent record
   (`team[1-team].target`) via `fifa96_match_entities_install(opp, phase, 0x22,
   0)` — the native `FUN_0007D9A4(opp, 0x22, 0, 1)` (fresh `0x7D9A4`: EAX=rec,
   `DX`=code 0x22, `BX`=staged 0, `ECX`=1 invoke).

### 4.3 What a fixture must stage (the "two records' angle cells")

For the fold to pass, an engine fixture (the `make_fixture` shape of
`tests/test_engine_match_handlers.c:133-155`) must stage:

- **the acting record** (`mr->record`, e.g. team-0 rec 0, `code = 0x07`,
  `phase = 2`, `stage92` driven to 1 so the kick runs) with the ball pair staged
  so `bo.staged != 0` and `ball.pair.traj < 0x30`, `timer89 = 0` (`< 5`);
- **the opponent record** (`mr->entities.team[1].target = <opp id>`): `code = 6`,
  `has_slot = 0`, `lane_x < 0xD0`, and the two cells
  `opp->lane_z = x`, `opp->cam_dz6f = z`, `opp->face7d = entity_angle(x, z)` — so
  `diff = 0 < 0x55`. Equivalently (the faithful BF20 route the m2 band scenario
  uses) place the render camera and the opponent position so the next BF20
  refresh yields the desired `(x,z)` and set `face7d` to match.

Then a row-07 dispatch yields `out.opponent_invoke == 1` and the opponent's
`code` becomes `0x22`. **Dispatch of the body** additionally needs the slot
registered (`{0x22, NULL}` → `{0x22, fifa96_match_action_22}`, `handlers.c:5522`)
so the opponent's next per-record dispatch runs `fifa96_match_action_22`
(`:4323`, body `0x8539C..0x85481`, fresh 233 B: phase gate `0x853A2` →
`[0x157A4A]>>24 == 2`, `timer89 += delta`, `0x79B1C` snap, stage 0/1/2).

### 4.4 The row-`0x22` body has one static reference (fresh)

Fresh `read_memory 0x110768` = `9c 53 08 00 84 2f 08 00` → slot `0x22` =
`0x8539C`. Fresh `get_xrefs_to 0x8539C` = **1** (`0x110768`, DATA) — the body's
only static caller is the `0x22` slot the row-07 installer targets. So the
install `→` dispatch chain is the sole route.

---

## 5. Step 4 — port shape (OL-65) and the M2 constraint

1. **Apply the §2.4 two-line fix** in `match_kick_from_record`.
2. **Add an engine fixture** (per §4.3) proving `out.opponent_invoke == 1` and
   `opp->code == 0x22` (install), plus — after the slot registration — that the
   next per-record dispatch reaches `fifa96_match_action_22` (dispatch). This is
   the census-rule "fixture-proven install+dispatch" witness.
3. **Register the slot** `{0x22, fifa96_match_action_22}` (`handlers.c:5522`).
4. **Re-run the m2 golden** (`./build/test_engine_m2 | cmp -
   tests/golden/engine/m2-frames.txt`). The fix touches only the row-07
   post-kick gate (the fields' **only** consumer is
   `fifa96_action_handlers.c:1701-1705`), so the expected result is byte-identical
   — **but the m2 tape stages row `0x07`** (`M2_STAGE_ROWS`) and drives it
   through a kick, so inertness is not statically decidable and this run is
   mandatory. If the golden moves, the gate fired **live** (the strongest
   possible install evidence) and the movement is re-pinned with a written
   reason + frame diff per AGENTS.md.
5. **Only then** update the ENGINE.md census line + README title in the same
   commit (census rule).
6. **Mutation-RED riders:** NULLing either re-sourced field must abort at the
   row-07 gate fixture assertion; a `NULL → fn` slot flip with a dead gate
   (fields re-zeroed) must abort the install+dispatch fixture (the FU247 §4.4
   classes 1-3).

---

## 6. Census ruling for action `0x22`

- **Today: uncounted** — slot `NULL` (`handlers.c:5522`);
  `dispatch_action(0x22)` returns `-FIFA96_ERR_UNSUPPORTED`. Census holds at
  **58/80 = 72%**.
- **The installer is statically satisfiable after the fix** (the gate is a
  geometry test, §3/§4), and the loader-level fire is fixture-proven today
  (§4.1). So OL-65 is a **producer-staging leg**, not a permanent ceiling.
- **Not countable from the fix alone**: the census rule is *install AND
  dispatch*. The fix fires the install; the `NULL` slot blocks dispatch. A count
  needs §5 steps 1-3 (fix + fixture + slot) with §5 step 4 green.
- **Ruling:** leave `0x22` NULL/uncounted this wave (**58/80**). It moves to
  **59/80 = 73%** in the commit that lands the full port (fix + slot +
  fixture-proven install+dispatch + m2 golden unchanged-or-justified).

---

## 7. Numbered legs (new)

1. **FU259-L1 — the m2 byte-identity of the two-line fix.** The fix is consumed
   only by the row-07 post-kick gate; the m2 tape stages row `0x07`
   (`M2_STAGE_ROWS`) and drives a kick, so whether the tape's gate ever reaches
   `diff < 0x55` with live cells is a runtime differential. **Acceptance:**
   `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` byte-exact
   (or a written-reason re-pin if it moves). **Owner: OL-65 / the row-`0x22`
   wiring task.**
2. **FU259-L2 — the engine-level install+dispatch fixture.** No engine test
   currently stages the opponent angle cells (`test_engine_match_handlers.c`
   row-07 tests stop at `opp` preconditions via the loader machine); the
   census-rule witness (§4.3) does not yet exist. **Owner: the row-`0x22` wiring
   task.**
3. **FU259-L3 — the live-row-07 geometry on a natural tape.** Whether a *natural*
   (un-forced) match ever produces `entity_angle(cam−opp_pos) ≈ opp->face7d`
   within `0x55` with a code-6, no-slot, `<0xD0` opponent is the FU247-L1
   differential carried forward. **Owner: OL-65.**
4. **FU250-L1/L2/L3, FU247-L1, FU185-L6/OL-65 stand unchanged** (FU-250's
   correction of FU-247 §2.3 / FU-247-L1 remains authoritative; this slice adds
   no new erratum — it re-confirms FU-250 first-hand).

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `disassemble_bytes 0x81643..0x81696` (84 B),
`0x81691..0x816e3` (83 B), `0x8dd70` (32 B), `0xcd474` (79 B), `0x7d9a4`
(92 B), `0x8539c` (233 B); `read_memory 0x110768` (8 B), `0x7C776` (48 B);
`get_xrefs_to 0x8539C` (1: `0x110768`), `0x816DF` (0), `0x8DD70` (28, incl.
`0x816AB`). Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_handlers.c` (897-1002, 1120-1169, 1540-1561,
4313-4375, 5500-5523), `fifa96_match_run.c` (1355-1527),
`fifa96_match_entities.c` (73-93), `include/fifa96_engine/
fifa96_match_entities.h` (100-193), `include/fifa96_engine/
fifa96_match_handlers.h` (186-202), `src/fifa96_loader/
fifa96_action_handlers.c` (41-46, 1620-1723), `include/fifa96_loader/
fifa96_action_handlers.h` (900-949), `tests/test_action_handlers.c` (790-841),
`tests/test_engine_match_handlers.c` (120-199, 2098-2194),
`tests/test_engine_m2.c` (2102-2111, 2401-2409, 2520-2567). Docs cross-cited:
FU-250 (§1-§8), FU-247 (§2.2/§2.3/§2.4/§4.1), FU-185 (§2/§10),
`docs/ENGINE.md` (332-355, 469-480), AGENTS.md (alias traps; census rule). No
writes beyond this draft and the workspace report; no rename/label/comment/
function/script/project save; no git; no subagents.
