# FU-251: OL-63 residual — the row-05 stage-0 tail and the `FUN_0007F7E0` fallback

Task 4 (W24-4) of the phase-24 producer-closure recon-ahead plan
(`docs/superpowers/plans/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead.md`).
Read-only re-derivation in the authoritative program **`/FIFA96.EXE`** of the two
row-05 legs FU-139 §8.7 / FU-142 OL-63 left after M2 phase-9 T2 wired
`fifa96_match_action_05`: the stage-0 target algebra `0x7F3A1..0x7F57B` and the
`FUN_0007F7E0` fallback (`0x7F7E0..0x801B2`).

Result in one line: **the stage-0 tail is a bounded, fully linear target algebra
(seed → early-out → speed/angle build → `0x92820`/`0x71C94`/`0x79CCC`/`0x6DA64`
sinks) that is portable as a tested helper; the `FUN_0007F7E0` fallback is a
2.5 KB teammate-selection machine (its own `0x7F7CC` jump table, code-7 install,
team gates `0x826/0x82a/0x82b/0x82d/0x82e`) and belongs to a **separate** slice,
not the row-05 tail.** Row 05 is already wired, so the census is unchanged.

## Method

* Program **`/FIFA96.EXE`** (case-sensitive, live). Ghidra **read-only**: no
  rename / comment / label / function / script / save; standard MCP tools only.
* Tool calls made this slice:
  * `disassemble_bytes 0x7F274` (301 B) — the stage-0 head through the tail
    entry;
  * `disassemble_bytes 0x7F3A1` (476 B) — the stage-0 tail algebra
    `0x7F3A1..0x7F57B`;
  * `disassemble_bytes 0x7F194` (224 B) — the row-05 prologue + dispatch;
  * `disassemble_bytes 0x7F7E0` (160 B) + `0x7F880` (700 B) + `0x7FB3B` (700 B)
    — the fallback head, `0x7F7CC`-dispatched body and delegation cluster;
  * `read_memory 0x7F184` (16 B) — the stage table; `read_memory 0x7F7B0`
    (48 B) — the fallback `0x7F7CC` jump table; `read_memory 0x10F334` (24 B) —
    the dribble dir tables;
  * `get_function_by_address 0x7F7E0` (body `0x7F7E0..0x801B2`);
  * `get_function_xrefs 0x7F7E0` (fresh) — **count 1, from `0x7F36F`**.
* Every CALL rel32 below was byte-verified (`e8`/`e9` + rel32 target of the
  following instruction).
* **Address form.** Data immediates render at their resolved flat addresses
  (`[0x0015872A]`, `[0x0015774C]`), matching the FU-137/FU-139 native
  convention. The engine documents the same sites in the pre-loader `0x5872A`
  short form.

## 1. Re-verified anchors

* Stage table `0x7F184` = `{0x7F274, 0x7F57C, 0x7F5E9, 0x7F627}` (fresh
  `read_memory`, bytes `74f20700 7cf50700 e9f50700 27f60700`); `> 3` returns at
  `0x7F65C`. Stage 0 = `0x7F274`.
* The row-05 prologue `0x7F194..` (fresh) confirms: `byte[+0x9E]=1` (`0x7F19F`);
  `phase != 2` → `0x7DAB4` reset + RET (`0x7F1A6..0x7F1BA`); the claim block
  `0x7F1BF..0x7F205`; `[team+0x7B2]=rec`, `[team+0x7B6]=0` (`0x7F20B/0x7F214`);
  the `+0x89 < 0x4B0` capped add (`0x7F221..0x7F23A`); `word[+0x81]!=0` RET
  (`0x7F240`); the camera-triple copy to `+0x4D/+0x51/+0x55`
  (`0x7F24E..0x7F258`, source `0x15774C`). Nothing here is new; the anchors
  align with FU-139 §8.3 and the loader contract.
* Stage-0 head `0x7F274..0x7F386` (fresh) re-confirms the loader's gates: lane
  `[+0x69]>>16 > 0x40` → `[0x157A83]=0` + RET; else `[0x157A83]=rec` and
  `word[+0x6B] > word[+0x77]` RET; `byte[0x15872D] > 0` RET; `dword[+0x5D] != 0`
  RET; the type arm (`dword[0x157750] > 0x38`: dirs = `0x10F334/0x10F33C[octant]`,
  speed `0x60`, `0x7F2BF..0x7F2FF`); the slot arm (dirs = `slot[+0x20/+0x21]`,
  speed `0x30`, `0x7F304..0x7F32B`); the team gates → `0x7876C` merge
  (`0x7F32D..0x7F35F`); else the `0x7F7E0` fallback call (`0x7F36F`); the
  `code == 5` continuation gate `0x7F374` (`MOV EAX,[EBP+0x8E]; SAR 0x18` = byte
  `+0x91`); and the dir-byte writes `[0x15872A]/[0x15872B]` at
  `0x7F386/0x7F397`.
* Dir tables `0x10F334` (x) and `0x10F33C` (z) are offset-overlapping 32-byte
  tables indexed by the **face octant** `byte[+0x8E]` (`[EBP+0x8B]>>24`), matching
  the engine `match_kick_dir_x/z` and the loader `type8` field.

## 2. The stage-0 tail `0x7F3A1..0x7F57B` (fresh disassembly)

Entered from the `0x7F386` dir-byte writes. After them, `ESI = S24` (the
dir-x word) and:

```
0x7F39C  TEST SI,SI ; JNZ 0x7F3FC          ; dir_x != 0 -> skip the zero-seed
0x7F3A1  CMP word[ESP+0x14],0 ; JNZ 0x7F3FC ; dir_z != 0 -> skip
0x7F3A9  EAX = (int32)word[+0x6B] (lane)
0x7F3AF  lane > 0x20 ? copy [0x15774C..54] (camera triple) : copy [EBP+0x59] (pos triple) -> S00/04/08
0x7F3C5  S00/04/08 -> [EBP+0x4D/+0x51/+0x55]   ; target = seed triple
0x7F3CD  word[0x1577BE] <= word[+0x71] ? RET (0x7F65C)
0x7F3E0  S18 = (word[+0x71] << 2) & 0xFFFF      ; speed local re-seeded
0x7F3EA  S24 = low word of S1c (type dir_x); S14 = low word of S20 (type dir_z)
0x7F3FC  EBX = S18 ; AX = word[0x157750] (ball height) ; S18 = EBX + AX
0x7F40E  S18 == 0 ? RET
0x7F414  word[S24] != word[S1c] ? jmp 0x7F48E
0x7F41F  word[S14] != word[S20] ? jmp 0x7F48E
0x7F42A  [EBP+0x4D] = [0x15774C] + (int32)word[+0x6B]     ; target_x = camera_x + lane
0x7F437  [EBP+0x55] = [0x157754] + (int32)word[+0x6F]     ; target_z = camera_z + word[+0x6F]
0x7F44A  (int32)word[+0x6B] > 0x28 ? RET
0x7F459  word[+0x6B] > word[+0x77] ? RET
0x7F467  if (S24 && S14) S18 += ((word[+0x71]<<3) * 0xB5) >> 8
0x7F48E  S0e = (S18 * S24) & 0xFFFF          ; ox = speed * dir_x
0x7F4A0  S10 = (S18 * S14) & 0xFFFF          ; oz = speed * dir_z
0x7F4AC  CALL 0x8DC68(S0c >> 16, S0e >> 16) ; (S0c reader — see leg L2)
0x7F4C4  S0c = AX                            ; angle accumulator
0x7F4CB  CALL 0x92820(rec, 0x26)             ; OL-27 sink
0x7F4D0  AL = (dword[0x157750] > 0x10)
0x7F4E7  CALL 0x71C94(rec, &S0c, 0, (int16)AL)
0x7F4EC  EAX = slot(+0x20)!=0 ? 4 : ([S0a] >> 18)
0x7F500  dword[0x157750]!=0 ? AX += word[0x1577F8]
0x7F510  AX > 0x3C ? AX = 0x3C
0x7F51D  byte[0x15872D] = AL                  ; release countdown, clamped 0x3C
0x7F522  byte[0x157A4F] == 0 ? RET
0x7F52F  byte[+0x9A]=1; CALL 0x79CCC([rec], rec+0x59, &S18, 0); byte[+0x9A]=0
0x7F54E  result == rec ? RET
0x7F556  S18 >= 0xC0 ? RET                    ; [S18 = speed]
0x7F569  CALL 0x6DA64(rec, result)
0x7F572  epilogue (ADD ESP,0x28; pops; RET)
```

**Reading.** The tail is a stage-0 "settle the target and arm the release" arm:
(1) when both dir bytes are zero it seeds the target triple from the camera
(lane > 0x20) or the record's own position; (2) it builds a speed scalar from
`word[+0x71]` and the ball height `[0x157750]`; (3) for the type-arm direction it
overrides the target with a lane/`word[+0x6F]` camera-relative offset; (4) it
folds `speed × dir` into the `0x8DC68` metric, fires the `0x92820(rec,0x26)`
event, runs the `0x71C94` (rec, angle, height-sign) call, writes the clamped
`0x15872D` release countdown, and — only when `[0x157A4F]` is set — runs the
`0x79CCC` slot selection and, on success with speed < 0xC0, the `0x6DA64`
20-byte block swap. Every sink is either ported (`0x15872D` decay already in
`fifa96_match_entities_update`) or an already-numbered request (OL-27, OL-52).

### 2.1 Stack-locals note (the one real ambiguity)

The dir/speed locals are words packed into the `SUB ESP,0x28` frame
(`S14`, `S18`, `S24`, plus the type-table dword copies `S1c`, `S20`). Two reads
straddle word boundaries and are not written within the tail:

* `S0c` (dword, read at `0x7F4b0` and written at `0x7F4c4`) has no producer in
  `0x7F274..0x7F57B`;
* `[ESP+0x0a]` (read at `0x7F4f9`, `>>18`) overlaps the seed-Z high word and the
  `S0c` low word.

Both are Watcom stack-slot aliasing; their exact producers are outside the
disassembled window. Flagged as legs **L1**/**L2** below — they do not change the
portable shape (the result feeds only the `0x8DC68` argument and a counter
seed).

## 3. The `FUN_0007F7E0` fallback (`0x7F7E0..0x801B2`; sole caller `0x7F36F`)

Called when the record's dribble dir is unresolved (`dir.resolved == 0`), the
team gates at `0x7F32D` are false **or** the `0x7876C` merge returned no slot.
Signature (Watcom register args): `param_1 = rec` (EAX), `param_2 = &S24`
(dir_x, EDX), `param_3 = &S14` (dir_z, EBX), `param_4 = &S18` (speed, ECX).

**Head / entry conditions (`0x7F7E0..0x7F889`).**

```
0x7F7F4  [rec+0x4D/+0x51/+0x55] = [0x15774C..54]   ; target = camera triple
0x7F7FF  *param_3 = 0                               ; dir_z = 0
0x7F807  *param_2 = *param_3 (0)                    ; dir_x = 0
0x7F80A  *param_4 = 0                               ; speed = 0
0x7F827  team = [rec]; EBX = [team+0x7A6]; S38 = (word)byte[team+0x826]
0x7F82D  byte[rec+0x8D]==0 ?
           -> EDX=5; CALL 0x92820(rec,5); EBX=0x780; 0x92AC8 RNG; DIV EBX;
              S18 = 0x3C0 - RNG%0x780; JMP 0x7FB9B        ; inactive arm
           else CMP byte[team+0x7E7]; JZ 0x7F889; nonzero -> CALL 0x7E528
```

So the fallback **always writes the target triple from the camera and zeros the
caller's dir/speed slots**, then branches on the record's `+0x8D` (active) and
the team's `+0x7E7`. The `0x82A`-keyed jump on `[team+0x82a]` = 1 vs 2
(`0x7F8B2`/`0x7F949`) installs **code 7** via `0x7D9A4` after the `0x92820(rec,
0xF)` / `0x92820(rec,0x11)` events and clears `[team+0x7bf]`/`[team+0x82a]`
(both arms RET inside `FUN_0007F7E0` — they never reach the caller's tail).

**The `0x7F7CC` jump table (fresh `read_memory 0x7F7B0`, 48 B).** Keyed on
`byte[team+0x82d] - 1`, `> 4` default: `{0x7FAA5, 0x7FB0D, 0x7FC1D, 0x7FC2E,
0x7FC3D}`. Cases read/rotate the team's `+0x7ba` chosen record, `+0x7be`,
`+0x7bf`, `+0x826`, `+0x827`, `+0x82b`, `+0x82d`, and call `0x6DBCC`
(corner/score table), `0x7F6F0`, `0x8DD70`/`0x8DC68` (angle/metric), `0x92AC8`
(RNG), `0x71C94`, `0x92820` (events 2/3/0xE/0x11), `0x7D9A4` (install), and the
`0x1105FE` score table — a teammate-selection / support-run decision, not a
target algebra. It shares the `0x158738` staging triple and the `0x157A4F` gate
with the tail but is otherwise a distinct machine.

**Bound.** `body 0x7F7E0..0x801B2` = 2 514 bytes, one static caller, its own jump
table and a 7-site event/install cluster. It is **not** a row-05 tail; it is a
whole AI decision slice.

## 4. Engine posture (first-hand: `fifa96_match_handlers.c`,
`fifa96_action_handlers.c`, `.h`)

* `fifa96_match_action_05` (`src/fifa96_engine/fifa96_match_handlers.c:2744`)
  binds `fifa96_action_carrier_arm` over `mr->record` + the FU-141 pool and
  applies: `ran_set`, `timer89`, `out.stage`/`out.reset`, the claim block, the
  team-target bind, `set/clear_control`, `target_camera`, the dir bytes
  (`out.dirs`), the `0x7876C` merge (`helper_request`), the `0x79B1C` snap, the
  `0x79C50` face and the stage-3 hand-off. The comment at
  `fifa96_match_handlers.c:2738` explicitly leaves `0x7F3A1..0x7F57B` as
  `out.tail` and `FUN_0007F7E0` as `out.fallback` unconsumed.
* `fifa96_action_carrier_arm` (`src/fifa96_loader/fifa96_action_handlers.c:439`)
  raises `out.tail` in the stage-0 dir-resolved / merge / `code==5` continuation
  arms (`:494/:500/:507/:515`) and `out.fallback` in the `FUN_0007F7E0` arm
  (`:514`), but performs **no** target algebra, no `0x92820`/`0x71C94`/`0x79CCC`/
  `0x6DA64`, no `0x15872D` write and no `FUN_0007F7E0` call. This exactly matches
  the derived gate structure (the `code==5` mask is the `0x7F374` byte-`+0x91`
  gate).
* The header contract (`include/fifa96_loader/fifa96_action_handlers.h:243..255`)
  documents the same split and the OL-63 legs.

**What the tail adds (fidelity):** the record-visible target triple writes
(`+0x4D/+0x51/+0x55` from the camera/pos seed and the lane/`word[+0x6F]`
override), the `0x15872D` release-countdown clamp, the `0x92820(rec,0x26)` /
`0x71C94` / `0x79CCC` / `0x6DA64` requests, and — on the fallback arm — code 7
install plus the team-state rotation. None of these currently runs in the port.

## 5. Port shape + verdict

**Verdict — the tail is a separate, portable slice; the fallback is not tail.**

1. **Tail (FU-251a).** Port `0x7F3A1..0x7F57B` as a bounded pure helper (e.g.
   `fifa96_action_carrier_tail_step`) taking the staged dir/speed locals plus the
   record target/pos/camera/ball-height inputs, returning the target triple, the
   `0x15872D` release write, and request bits for `0x92820(0x26)`, `0x71C94`,
   `0x79CCC` and `0x6DA64`. Bind it in `fifa96_action_carrier_arm` stage 0 (the
   existing `out.tail` set-points) and consume the requests in
   `fifa96_match_action_05`. Tests: the zero-dir camera-vs-pos seed (lane
   0x20 boundary), the `[0x1577BE] <= [+0x71]` early-out, the speed/height build,
   the type-dir camera-relative override, the `lane > 0x28` / `close > bound`
   returns, and the `[0x157A4F]` / `speed >= 0xC0` gates. **Resolve legs L1/L2
   first** (the `S0c`/`S0a` producer) — otherwise the `0x8DC68` argument and the
   fallback-counter seed stay stand-ins.
2. **Fallback (FU-251b / new leg).** `FUN_0007F7E0` is a standalone teammate-
   selection machine. Keep it a numbered leg and schedule it as its own slice;
   do not fold it into the row-05 tail. Its engine consumer would be the
   `out.fallback` arm with the team-block fields (`0x7ba/0x7be/0x7bf/0x826/
   0x827/0x82a/0x82b/0x82d/0x82e`) staged from the pool.

**Census.** Row 05 is already wired (M2 phase-9 T2); `out.tail`/`out.fallback`
are fidelity gaps on a wired row. **No census angle — the wired count stays
58/80 = 72%.** The README title must not move for this slice.

## 6. Open legs (numbered; owner = this FU-251 unless noted)

* **L1 — the `S0c` angle accumulator producer.** `[ESP+0x0c]` is read at
  `0x7F4b0` (`CALL 0x8DC68` arg) and written at `0x7F4c4`; it is not written in
  `0x7F274..0x7F57B`. Owner: FU-251a (needs `FUN_0008DC68` calling-convention +
  stack-slot analysis).
* **L2 — the `[ESP+0x0a]` fallback-counter seed** (`0x7F4f9`, `>>18`): overlaps
  the seed-Z high word and the `S0c` low word. Owner: FU-251a.
* **L3 — the `0x8DC68` / `0x8DD70` / `0x6DBCC` / `0x7F6F0` bodies.** Metric /
  angle / corner-score tables. Owner: FU-251a (tail) and FU-251b (fallback).
* **L4 — the `0x92820(rec,0x26)` sink** (`0x7F4CB`). Already OL-27 (FU-139).
* **L5 — the `0x71C94` body** (`0x7F4E7`; rec, &angle, 0, height-sign). OL-T11
  callers register (FU-142 §13.4).
* **L6 — the `0x79CCC` slot-selection body** (`0x7F52F`; team, pos, &speed, 0,
  with the `+0x9A` latch). Owner: FU-251a.
* **L7 — the `0x6DA64` 20-byte block swap** (`0x7F569`). Owner: FU-251a.
* **L8 — `FUN_0007F7E0` (fallback).** `0x7F7E0..0x801B2`, the `0x7F7CC` jump
  table, `0x7E528`/`0x7F6F0`/`0x8DCD4`/`0x92AC8`/`0x6DBCC`/`0x8DD70`/`0x92820`/
  `0x7D9A4`, the team-block gates and the code-7/0x11 install. Owner: FU-251b
  (own slice). Carries the FU-139 §8.7 OL-63 residue for the fallback.
* **L9 — `word[+0x71]`, `word[+0x6F]`, word `[0x1577BE]`, `[0x1577F8]`,
  `[0x157A4F]` semantics** as used by the tail (bearing/mask/facing words). Owner:
  FU-251a; cross-ref FU-148 §13.

## 7. Errata / corrections

* None against prior sections. FU-139 §8.7's sub-part list is confirmed
  first-hand; the `FUN_0007F7E0` span `0x7F7E0..0x801B2` and the "installs code
  7" claim are re-confirmed (`0x7F928`/`0x7F96C` `CALL 0x7D9A4` with `EDX=7`).
  The fresh `get_function_xrefs 0x7F7E0` count (1, from `0x7F36F`) supports the
  "sole static caller" claim.
