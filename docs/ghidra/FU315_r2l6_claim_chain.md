# FU315 — the possession/claim chain (the row-04 pan's `has_slot` / `+0x20` state)

**Phase/Provenance:** M2 phase-31 **recon-ahead** wave, **Task 2 (`FU315`)**, 2026-10-22. Read-only Ghidra MCP session, authoritative program **`/FIFA96.EXE`** (case-sensitive; `watcom:LE:32:default`, image base 0, `.image` overlay; confirmed with `get_current_program_info` at session start). No writes, no renames, no comments, no scripts, no project save; the ISO and the repo were not modified. Every address/byte below is a **fresh** `disassemble_bytes` / `disassemble_function` / `decompile_function` / `read_memory` / `get_xrefs_to` of this session unless marked *carried* (source named). Engine/test source was read only; one existing test binary was run for measurement (§4.3). No git. This is **not** production code.

---

## 1. The question

Can **natural** play produce the claim state that the row-04 half-line pan needs — `has_slot` (`record+0x20`), the lane (`+0x6B`) and the bound (`+0x77`) — or is **the P3 possession producer** (the phase-28 register's P3 cell, `dword[rec+0x20]`) the missing link?

Scope: the ball-pair possession/claim machine (`FU148` §16, `FU153`), the claim bind (`0x73B65`), the record `+0x20`, the P3 possession producer (phase-28 register), the pass/claim/kick routes (`FU163`). Port: `fifa96_ball_pairing.c`, `fifa96_match_run.c` (`ball_receive`, the pair state), `fifa96_match_entities.c` (record slots / `has_slot`), the keeper bind's P3 cell.

## 2. Result in one line

**GO — a natural claim exists and is already test-pinned live** (kickoff → held direction → row-01 → row-02 → row-04 half-line coda → code-5 install → row-05 claim: `ball.carrier` = the slot record, `[0x157A83]` = the claimer, dirs written; §4.3). **The P3 cell is not the missing link** — P3 is `dword[rec+0x20]`, it is already bound live from `has_slot` (`fifa96_match_entities.c:768`), and the "P3 possession unported" line is a **stale phase-28/source comment** (`fifa96_match_entities.c:741`). What *is* missing is narrower: (a) the row-05 claim's own `FUN_00071C94` event/tracked bind (`0x7F4E7`, OL-63) is **unported**; (b) the engine projects the slot pointer onto a per-record byte + per-record dirs, so the **slot dirs do not follow a `FUN_000786A0` hand-off** (§4.2). Neither gap blocks the row-04 pan: at `camY == 0` and no slot the pan event falls back to the record's velocity words (`0x7F056..0x7F0A2`) — so the natural pan's binding input remains FU314's **lane admission**, not the claim.

---

## 3. Evidence — native

### 3.1 The record `+0x20` is a **control-slot pointer**, not the ball possession

Fresh `FUN_000785E0` tail (`0x78600..0x78644`):

```
0x7860B  EDI = 0x157C64 + (slot index)*0x25      ; the 4-slot control array
0x7862F  MOV [EAX + 0x20], EDI                   ; the chosen record's slot pointer
0x78632  MOV [EDI], EAX                          ; the slot's bound-record back-pointer
0x78637  MOV byte[EDI + 0x1C], slot_index
0x7863E  MOV byte[EDI + 0x1E], map_select
```

Fresh `FUN_000786A0` (24 insns) — the **direct hand-off**:

```
0x786A2  EBX = [EAX + 0x20]        ; from's slot
0x786A5  TEST EBX,EBX; JZ 0x786E9
0x786A9  ECX = [EDX + 0x20]        ; to's slot must be null
0x786B0  MOV [EDX + 0x20], EBX     ; the pointer moves
0x786B3  MOV [EBX], EDX            ; the slot's back-pointer follows
0x786B5  MOV [EAX + 0x20], ECX     ; from is cleared
0x786BF..0x786E3  word[slot+4/6/8/0xA/0xC/0x14/0x16] = 0
```

Fresh `FUN_0007876C` (decompile) — the **claim/merge request**: a requester with `[rec+0x20] == 0` and `byte[team+0x828] != 0` asks the team's slot-holders for the slot (largest `FUN_0008DC68` metric wins), takes the slot pointer, updates the slot's back-pointer, clears the donor and calls `FUN_00078670`. So the **natural claim of the control slot** is this function, not the ball machine.

Fresh setup clear (`0x78847..0x78871`): `ESI = 0`, and for each of records `0..10` of each team `0x78863 MOV dword[EDX - 0x92], ESI` = `[rec+0x20] = 0`. **Erratum to FU-70:** FU-70 §1.4 reads this as `[team + i*0xB2 + 0x20] = team`; the fresh bytes store the zeroed `ESI`, i.e. the setup **clears** every `+0x20`, then `FUN_000785E0` binds the one human slot. That is why a slotless AI record is the native norm.

Fresh slot dir bytes (`FUN_00078950`): `0x78A33 MOV DL, byte[0x10E1EC + anim] ; 0x78A39 MOV [ECX], DL` (slot `+0x20`) and `0x78A40 MOV DL, byte[0x10E1F5 + anim] ; 0x78A46 MOV [ESI], DL` (slot `+0x21`). The **dirs are a property of the slot block**, updated from the controller input.

### 3.2 What the pan and the follow read through `+0x20`

Row-04 half-line event (`0x7EFEE` branch), fresh:

```
0x7EFEE  CMP EBP,[0x1577CA] ; JZ 0x7F0D6        ; the tracked-record exclusion
0x7EFFA  CMP dword[0x157750],0 ; JZ 0x7F035     ; camY == 0 ?
0x7F035  MOV ESI,[EBP+0x20] ; TEST ESI,ESI ; JZ 0x7F056
0x7F03C  MOVSX AX, byte[ESI+0x20] ; SHL EAX,6   ; slot dir x -> event vector x
0x7F04C  MOVSX AX, byte[EAX+0x21] ; SHL EAX,6   ; slot dir z -> event vector z
0x7F056  ... else |cam.x| <= 0x630 && |cam.z| <= 0xA20 -> word[rec+0x73/0x75]<<2
```

So the pan **dereferences** `[rec+0x20]` to the slot's `+0x20/+0x21` bytes, and falls back to the record's velocity words when there is no slot. The slot is an **optional vector source**, not a gate.

The follow (fresh, `FUN_000736AC`/`FUN_00071DF4`):

```
0x73B5B  EBP = [0x1577CA] ; TEST ; JZ 0x73B70
0x73B65  CMP dword[EBP+0x20],0 ; JZ 0x73B70     ; tracked has a slot
0x73B6B  CALL 0x71DF4
0x71E1C  CMP dword[EAX+0x20],0 ; JZ 0x71EF0     ; the first arm re-tests the slot
0x71E4E  EDX = [0x1577CA] ; 0x71E54 EDX = [EDX+0x20]   ; the slot pointer
0x71E5B  MOVSX AX, [EDX+0x20] ; IMUL EAX,0xF ; [0x1577C0] = AX
0x71E69  MOVSX AX, [EDX+0x21] ; IMUL EAX,0xF ; [0x1577C2] = AX
```

`0x73B65` is a **read gate** (the tracked record holds a slot); the dirs come from the *slot block* it points to. The engine mirrors the gate as `e->has_slot` (`fifa96_match_run.c:3984`) and the dirs as the record's `dir_x/dir_z` (`match_run.c:3987`) — a projection of the pointer that is only faithful while the record is the current slot owner (§4.2).

Fresh `get_xrefs_to 0x1577CA` = **88 refs, exactly 3 writes**: `0x71D27` (`FUN_00071C94` sets `[0x1577CA] = rec`), `0x70258` (`FUN_000700F4` clear), `0x70054` (`FUN_0006FFC0` clear). The tracked bind is therefore the event setter, not the claim.

### 3.3 The P3 cell and the P3/P6/P8/P12 register rows

Fresh `0x7857B..0x785A8` (the keeper record-0 machine):

```
0x7857B  EAX = [0x157A4A]>>0x18 ; CMP EAX,2 ; JNZ 0x7859E   ; phase == 2
0x78588  CMP dword[EBP+0x20],0 ; JNZ 0x7859E                ; **P3** (queued20 == 0)
0x7858E  CMP byte[0x157C5D],0 ; JZ 0x7859E                  ; P4 interlock
0x78599  CALL 0x77EAC                                       ; the ladder
```

P3 is `dword[rec+0x20] == 0` — the **same control-slot pointer** as §3.1. It is *not* the ball possession (`0x58724` / `+0x9B`). The ladder opens when the record holds **no** slot.

The other phase-28 register rows, fresh:

| row | native site (fresh) | reads | claim relevance |
|---|---|---|---|
| P6 | `0x77EE0..0x77EF0` `AND AH,2 ; JNZ 0x782C4` on the `[0x14C1D4]` prepass word | per-side input range | **none** (keeper ladder) |
| P8 | `0x77F03..0x77F10` `byte[[rec+0x28]] == 0x54 ('T')` | the anim-row descriptor | **none** |
| P12 | `0x765C4..0x765DF` `EDX=[0x1577CA]`, side compare at `+0x826` | the tracked record + its side/code | **adjacent** (tracked presence, not the slot) |

### 3.4 The ball possession claim (the carrier block)

Fresh `0x7F1C0..0x7F248` (row 05 head, `FUN_0007F194`):

```
0x7F1BF  if ([0x158724] != rec) {
0x7F1C7     clear the whole 0x158724..0x15872F block
0x7F1FF     MOV [0x158724], EBP            ; the **claim**: carrier = rec
0x7F205     byte[0x15872B] = 0 }
0x7F20B  [[rec]+0x7B2] = rec ; [[rec]+0x7B6] = 0
0x7F221  rec[+0x89] += delta (cap 0x4B0)
0x7F240  if (word[rec+0x81] != 0) return
```

Fresh `get_xrefs_to 0x158724` = 9: writes `0x7F149` (`FUN_0007F144` reset), `0x7F1CD` (claim clear), `0x7F1FF` (claim set), `0x819B7` (row-0B **clear**, `ECX=0`); reads `0x7A40A`, `0x7F1BF`, `0x8022D`.

Fresh row-04 tail (`0x7F0D6..0x7F133`) — the carrier/chaser split:

```
0x7F0D6  if (byte[rec+0x8D] && other) {
0x7F0E7     ocode = opps[other].code
0x7F0F0     if (ocode == 4 || ocode == 5) {
0x7F106        if (lane > opps[other].lane) install(self, 6) ; return
0x7F120        install(other, 6) }
0x7F125  ECX = 1 ; 0x7F12A EDX = 5 ; 0x7F131 EBX = 0
0x7F133  CALL 0x7D9A4                      ; install(self, 5, invoke-now)
```

So **any** row-04 dispatch that reaches the tail ends by installing the carrier row `5` on itself. The claim next frame is `0x7F1FF`. (The native install is invoke-now, so the native claim can run in the same frame; the engine only re-stages `e->code` and the row-05 body runs on the **next** dispatch — the `test_live_carrier_claim_and_kick` asserts exactly that one-frame lag, `ball.carrier == NONE` at the install frame. That is the invoke-now reconciliation, FU296-L8, not a claim-producer gap.)

The keeper's claim is separate and live-capable: fresh `0x755C0 MOV byte[EAX+0x9B],1` and `0x755CF MOV [0x157A83],EAX` (the row-1E keeper claim — ported as `fifa96_keeper_claim_step`, FU147 §3.6).

### 3.5 The claim tail — the missing claim bind (`0x7F4E7`)

Fresh `0x7F4D0..0x7F54E` (row-05 stage-0 tail):

```
0x7F4D0  CMP dword[0x157750],0x10 ; SETG AL          ; camY > 0x10
0x7F4DE  EDX = &vec (stack)
0x7F4E2  MOVSX ECX, AX
0x7F4E7  CALL 0x71C94                                ; **the claim's camera event + tracked bind**
0x7F4EC  CMP dword[EBP+0x20],0 ; JZ 0x7F4F9
0x7F4F2  EAX = 4                                     ; slot -> release countdown 4
0x7F4F9  else EAX = word[ESP+0xA] >> 0x12
0x7F500  if (dword[0x157750] != 0) AX += word[0x1577F8]
0x7F513  if (AX > 0x3C) AX = 0x3C
0x7F51D  MOV [0x15872D], AL                          ; release countdown
```

This is the **claim bind** the slice looks for: `FUN_00071C94(rec = the carrier, vector = the stage-0 target algebra)` sets the camera event **and** `[0x1577CA] = the carrier` (`0x71D27`), and then `dword[rec+0x20]` selects the release countdown. Fresh `get_xrefs_to 0x71C94` = **11**, including `0x7F4E7` (this claim tail), `0x7EFCF`/`0x7F0D1` (row-04), `0x7A219` (the receive arm), `0x75DC4` (row-1E stage 5), `0x70DD1` (the BF20 record tail).

### 3.6 The pass and the receive arm

Fresh `FUN_0007A084` (decompile) + `0x7A370..0x7A418`:

```
0x7A219  FUN_00071C94(actor, &0x158738, height = 0x15873C.hi) ; event + tracked = actor
0x7A227  [0x157A83] = 0                                       ; release the actor
0x7A2F0  [0x158746] = 1                                       ; ack
0x7A37F  [0x158734] = receiver ; 0x7A384 [[team]+0x7B2] = receiver
0x7A38E  [[team]+0x7B6] = 0 ; 0x7A3A2 the opponent's 0x7B2/0x7B6 = 0
0x7A412  if (actor == [0x158724]) byte[0x15872D] = 0x14         ; the carrier-kicker release
```

The receive arm binds `tracked = the kicker`, **not** the receiver, and writes the receiver only into the team target. The claimer's own bind is the row-05 tail (§3.5), so the natural chain is: kick → receive (tracked = kicker, team target = receiver) → the receiver's chase row → row-04 coda → row-05 claim (tracked = the receiver *if* the claim tail were ported; otherwise the row-04 pan already bound it).

---

## 4. Evidence — port (read-only; cited)

### 4.1 The claim state is live

| native cell | engine cell | where | status |
|---|---|---|---|
| `rec+0x20` (slot ptr) | `entity.has_slot` (byte) + `mr->slot` (single slot) | `fifa96_match_entities.h:116`; `match_run.c:2059-2072` | **LIVE** — setup bind at `match_run.c:2516` (`bind_slot`, `entities.c:250-280`); one slot (player 0) |
| the merge/claim (`0x7876C`) | `fifa96_match_entities_merge_slot` | `entities.c:293-320`; drained at `match_run.c:1647` (`mr->slot.entity = merged`) | **LIVE** — requested by rows 01/02/04/1F/05 (`handlers.c:283,357,2812,3659,2937`) |
| slot dirs `slot+0x20/+0x21` | `mr->slot.anim_b/anim_c` → `r->dir_x/dir_z` | `match_run.c:1406-1408`; slot update `match_run.c:4403` | **LIVE** while `mr->slot.entity == id` |
| `[0x1577CA]` tracked | `render.camera.tracked` | set by `fifa96_camera_set_tracked` (receive `match_run.c:4043`; row-04 `handlers.c:2788`) | **LIVE** |
| `0x58724` carrier | `entities.ball.carrier` | row-05 claim `handlers.c:2909`; reset `entities.c:52` | **LIVE** (claim test, §4.3) |
| `rec+0x9B` possession | `entity.has_ball` | row-1E claim (ported) + reset `entities.c:209` | **LIVE** |
| `0x5872D` release countdown | `ball.pos_release` | receive `match_run.c:4094`; claim tail **unported** | receive live / claim tail leg |
| P3 `dword[rec+0x20]` | `kc_rec.queued20 = e->has_slot ? 1 : 0` | `entities.c:768` | **LIVE** (the `entities.c:741` comment is stale) |
| `ball_owner`/`owner_rel_5` (keeper install tree) | — | memset at `entities.c:744` | **STAGED 0** → FU282-L3 |
| P6 `[0x14C1D4]` | `mr->input_range` live, **not copied** into `kc_rec.input_range` | `match_run.c:484`; `keeper_cpu.c:262` | table live / **glue leg** |
| P8 `'T'` descriptor | `kc_rec.is_T` | memset | **STAGED 0** |
| P12 tracked side/code | `kc_rec.tracked_present/tracked_same_side/tracked_code` | memset | **STAGED 0** |

The `+0x7D` face word (`word[rec+0x7D]`) that the row-04 half-line angle gate reads (`0x7EE5C`) is staged 0 in the row-04 handler (`handlers.c:2646`, OL-72) — a pan-admission input, not a claim cell.

### 4.2 The claim-state **split** (the one real port delta)

The native slot is a **block** whose pointer lives at `rec+0x20`; the dirs live in the block (`+0x20/+0x21`, §3.1) and the block is moved wholesale by `FUN_000786A0` (§3.1). The engine models the slot as `has_slot` (byte, on the record) + `mr->slot` (one block) + per-record `dir_x/dir_z` copied only during that record's dispatch (`match_run.c:1406-1408`).

* On a **merge** (`FUN_0007876C`): `merge_slot` moves `has_slot` and the drain sets `mr->slot.entity = merged` (`match_run.c:1647`) — consistent.
* On a **row-0x18 hand-off** (`FUN_000786A0`, native `0x84AD0 CALL 0x786A0`): `fifa96_match_action_18` swaps only the boolean (`handlers.c:2394-2400` via `fifa96_action_duel_swap`, `action_handlers.c:877-885`); `mr->slot.entity` and the dirs are **not** moved. After that the pool says the target holds the slot while the follow/pan read the old record's dirs and `mr->slot.entity` still names the donor.
* On a **row-0B clear** (`0x819B7 MOV [0x158724],0`): the port never clears `ball.carrier` on that path.

### 4.3 The natural claim is already measured live

`tests/test_engine_match_frame.c:2259` `test_live_carrier_claim_and_kick` (read this session; the test binary was **run** from the repo root with `game/FIFAPCCD96.iso` present and exited 0, i.e. the test ran and passed):

```
kickoff + KICK -> phase 2   (slot record id 9, rec->has_slot == 1, ball.carrier == NONE)
hold UP                     -> rec->code becomes 5 on the real dispatch path
next granted frame          -> ball.carrier == id            (0x7F1FF)
                               entities.controlled == id      (0x7F291)
                               ball.pos_dir_x/z == rec->dir_x/z
                               rec->ran == 1                  (0x7F19F)
```

The same file's pass/long-ball tests (`:2333-2450`; the `:2428` note) confirm the receive arm runs live on the real path and *"at camY 0 every other record's row-04 half-line admission is open too, and their events/merge requests move the control slot (native behavior)"* — i.e. the natural slot churn is the native one, not a fixture.

The natural probe (`tests/test_engine_m2.c:3809`) samples only the 16 camera cells; its 3 598 granted frames keep `tracked == -1` because the tape never claims. That is a **drive** limitation, not a claim-producer failure.

---

## 5. The bounded port shape

1. **Port the row-05 claim tail (`0x7F3A1..0x7F57B`, the `out.tail` stub today)** — the smallest honest claim→camera link. It already has a scaffold: `fifa96_action_carrier_arm` sets `out->tail` (`action_handlers.c:500-516`) and the engine has **no consumer** (`fifa96_match_action_05`, `handlers.c:2902`). Bounded content: at `out.tail != 0`, `fifa96_camera_event_set(cam, vec_x, vec_z, height, 1)` + `fifa96_camera_event_dir(cam, has_slot, dir_x, dir_z)` + `fifa96_camera_set_tracked(cam, id)`, and `ball.pos_release = has_slot ? 4 : clamp((dist>>0x12) + (camY ? F8 : 0), 0x3C)`. The open variable is the vector (the stage-0 target algebra `0x7F3A1..0x7F4CB`, leg **FU315-L1**); the setter/tracked primitives already exist (`fifa96_camera.h:318-335`).
2. **Make the dirs slot-owned.** Keep `mr->slot.anim_b/anim_c` as the single dir source (already the dispatch projection) and **move `mr->slot.entity` with the slot on the row-0x18 hand-off** (`handlers.c:2394-2400`), mirroring `0x786B0..0x786B5`; do not add per-record dir copies. This is the only change the follow (`0x71E5B`) and the pan (`0x7F03C`) need to stay pointer-faithful across hand-offs (leg **FU315-L2**).
3. **Do not port a "P3 possession producer".** P3 is bound (`entities.c:768`). If the keeper's possession lane is wanted, port **FU282-L3** (`ball_owner`/`owner_rel_5`) — a different cell, zero effect on the row-04 pan.
4. **Keep the single-slot model.** One human controller is the native natural configuration (the AI team has no slots; the setup clear `0x78863` + the one `FUN_000785E0` bind). The 4-slot array only matters for 2P/AI-slot modes (leg **FU315-L5**).

**Acceptance implication for Stage-2:** the phase-31 witness does **not** need a new possession producer. It needs (a) FU314's lane admission (drive the record under the camera), and (b) if the claim-time tracked bind is desired on the claim frame (for the follow), the `0x7F4E7` tail. The row-04 pan fires from the velocity fallback with `has_slot == 0` too (`0x7F056..0x7F0A2`), so a no-claim natural pan is possible — FU314 must decide which witness is honest.

---

## 6. Numbered open legs

- **FU315-L1 (owner: `fifa96_match_action_05` / `fifa96_action_carrier_arm`, OL-63).** The row-05 stage-0 tail: the `0x7F3A1..0x7F4CB` target algebra (the claim vector), the `0x7F4E7` `FUN_00071C94` claim event/tracked bind, the `0x7F4EC` slot-selected release countdown (`[0x15872D]`), the `0x92820`/`0x79CCC`/`0x6DA64` sinks. `out.tail` is emitted but has no engine consumer.
- **FU315-L2 (owner: the FU-70 slot model / `fifa96_match_action_18`).** The slot-pointer projection: after `FUN_000786A0` (`0x84AD0`) the native pointer (and its dirs) move; the port swaps only `has_slot` and leaves `mr->slot.entity` + the dirs on the donor. Same class on any merge whose drain is skipped.
- **FU315-L3 (owner: the possession cluster / FU282).** The keeper install-tree possession inputs `ball_owner` (`rec == [[rec]+0x7B2]`) and `owner_rel_5` are staged 0 (FU282-L3); the keeper's `code 4/5` target selection can never fire on the ported path.
- **FU315-L4 (owner: `fifa96_match_action_0B`, FU-169).** The row-0B carrier clear (`0x819B7 MOV [0x158724],0`) is unported; a duel exit can leave a stale `entities.ball.carrier`.
- **FU315-L5 (owner: FU-70 `0x4C1E0` mode rows).** The native 4-slot array `0x57C64` (stride `0x25`) vs the single engine slot; only 2P/AI-slot configurations are affected.
- **FU315-L6 (owner: FU314 / the record facing producer).** The row-04 half-line angle gate reads `word[rec+0x7D]` (`0x7EE5C`) and the port stages it 0 (`handlers.c:2646`, OL-72); it is a pan-admission input, not a claim cell.
- **FU315-L7 (owner: docs/ENGINE.md phase-28 register + `entities.c:741`).** The "P3 possession unported" line is stale: P3 = `dword[rec+0x20]` is bound live from `has_slot` (`entities.c:768`). The actual unported possession cell in the keeper bind is FU282-L3. Land as an erratum, not a code change.

**Errata recorded here (fresh bytes, this session):**
1. FU-70 §1.4's setup reset `[team + i*0xB2 + 0x20] = team` — the fresh bytes store the zeroed `ESI` (`0x78847 XOR ESI,ESI`; `0x78863 MOV dword[EDX-0x92],ESI`), i.e. a **clear**.
2. FU-78 §7's `0x84AC5 FUN_000786A0(rec, found)` — fresh: `0x84AC5 MOV EDX,[ESI]`; `0x84AC7 CALL 0x8DB6C` (NSEARCH); `0x84AD0 CALL 0x786A0` (hand-off).

---

## 7. GO / NO-GO — a natural claim

**GO.** Natural play produces the claim state, with no new possession producer:

**The exact natural chain (test-pinned, §4.3):**

```
kickoff reset (all rec+0x20 = 0, 0x78863)
  -> setup bind FUN_000785E0 (rec = 9, rec+0x20 = slot, [0x157A83] = 9)
  -> KICK -> phase 2
  -> held direction: the outfield machine installs code 4 (chase)
  -> row 04 (body span `0x7E7C8..0x7F141`, carried FU252 §2; the cited sites fresh):
       0x7ED89 (0x40 - timer81 >= lane) -> 0x7EDCE (lane <= bound)
       -> 0x7EDD8 (camY <= 0x50) -> 0x7EE4B (angle <= 0xAB)
       -> 0x7EFEE (rec != tracked) -> 0x7F0D1 FUN_00071C94 (half-line pan, tracked = rec)
       -> tail 0x7F0D6..0x7F133: install code 5 on self (invoke-now)
  -> next frame row 05 (0x7F194): 0x7F1FF claim ([0x158724] = rec)
       + 0x7F20B team target = rec, 0x7F291 controlled = rec (carried FU78 §3.2),
         0x7F386/0x7F397 ball dir bytes = the slot dirs (+0x20/+0x21; carried FU78 §3.2)
       + [0x7F4E7] claim event/tracked bind   <- the unported tail (FU315-L1)
```

**Pass route (the same claim, delivered by a kick):** row 07 → `match_kick_run` → receive arm `0x7A084` (`0x7A219` event + `tracked = kicker`; `0x7A384` `team+0x7B2 = receiver`) → the receiver's chase row → row 04 → code 5 → row 05 claim. **Loose-ball route:** any row-04 dispatch in the lane window ends in the code-5 install (`0x7F133`), so the next row-05 dispatch claims.

**What the claim state supplies, and what it does not:**

* `has_slot` (`+0x20`) / the slot dirs: **live** for the one human slot, and **optional** for the pan — at `camY == 0` the pan uses the slot dirs if present, else `word[rec+0x73/0x75]<<2` (`0x7F056..0x7F0A2`).
* lane (`+0x6B`) / bound (`+0x77`): **live**, refreshed every dispatched frame by the BF20 lane block (`match_run.c:1504-1507`), independent of any claim.
* P3: **not a missing producer** (§3.3, §4.1).

**NO-GO conditions (narrow):** (i) a fixture that *stages* `has_slot`, lane, bound or the slot dirs is not a natural claim (and must not be the Layer-1 witness); (ii) relying on the **claim frame** to bind `camera.tracked` without porting `0x7F4E7` — the claim does not bind tracked on the ported path; the row-04 pan does. Neither condition blocks a natural claim, so the slice verdict is **GO**, with FU315-L1/L2 as the bounded Stage-2 tasks if the claim-time bind / hand-off fidelity is wanted.

---

## 8. Provenance (this session)

| # | read | object |
|---|---|---|
| 1 | `get_current_program_info` | `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image` overlay |
| 2 | `disassemble_bytes 0x7ED40..0x7EEC0`, `0x7EF40..0x7F141` | the row-04 half-line gate, the pan event arms, the carrier/chaser tail |
| 3 | `disassemble_bytes 0x73B40..0x73B80`, `0x71DF4..0x71E77` | `0x73B65` slot gate; `0x71E1C/0x71E54/0x71E5B/0x71E69` slot-dir deref |
| 4 | `disassemble_bytes 0x78570..0x785B0`, `0x77EE0..0x77F20`, `0x765C4..0x765DF` | P3 gate, P6/P8 sites, P12 predicate |
| 5 | `disassemble_bytes 0x78600..0x78645`, `0x78840..0x78880`, `0x78A2A..0x78A48` | the slot bind, the setup clear, the dir tables |
| 6 | `disassemble_function`+`decompile_function 0x786A0` | the hand-off; `decompile_function 0x7876C` | the claim/merge request |
| 7 | `disassemble_bytes 0x7EC28..0x7EC90`, `0x7DF58..0x7DF80`, `0x7E025..0x7E048`, `0x76390..0x763B0` | the merge-request sites (row 04/01/02/1F) |
| 8 | `disassemble_bytes 0x7F1C0..0x7F250`, `0x7F4D0..0x7F550` | the code-5 claim and the claim tail (`0x7F4E7`) |
| 9 | `decompile_function 0x7A084`; `disassemble_bytes 0x7A370..0x7A418` | the receive arm's event/tracked/team-target writes |
| 10 | `disassemble_bytes 0x755A0..0x755E0`, `0x84A90..0x84AE5`, `0x81990..0x819D0` | the keeper claim `+0x9B`, the row-18 hand-off (`0x84AD0`), the row-0B carrier clear |
| 11 | `get_xrefs_to 0x71C94`, `0x1577CA`, `0x158724`, `0x157750` | 11 / 88 (3 writes) / 9 / the pan cell |
| 12 | engine/test source reads | `src/fifa96_engine/fifa96_match_entities.c`, `fifa96_match_run.c`, `fifa96_match_handlers.c`, `src/fifa96_loader/fifa96_outfield.c`, `fifa96_action_handlers.c`, `fifa96_ball_pairing.c`, `include/fifa96_engine/fifa96_match_entities.h`, `include/fifa96_loader/fifa96_control.h`, `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c` |
| 13 | `./build/test_engine_match_frame` (repo root, ISO present) | exit 0 — the natural carrier-claim witness ran and passed |

Program stayed `/FIFA96.EXE`; no repo file was created or changed except this draft; no project DB save; no tool touched `game/FIFAPCCD96.iso` or `/tmp/opencode/fifa96_le.bin`.

*(End of draft.)*

## Review erratum (wave review, 2026-10-22)

**Quote (§2):** "Neither gap blocks the row-04 pan ... so the natural pan's binding input remains FU314's **lane admission**, not the claim." **Fresh:** verified, and stronger: the measured natural drive fires the pan **before** the claim — event `0x1D` at f=1513, first motion f=1514, `ball.carrier=2` at f=1514 (`review_rec2`, `probe3`; `pair=0/0`). The pan's seed is the existing control-slot dir (`slot.entity=2`, `dir=(−1,0)`; `has_slot=1` at the gate), not a new possession. The phase's premise "the possession/claim chain arms the row-04 pan" is **inverted**: row 04's tail installs row 5, and the row-05 claim is the pan's downstream. Stage-2 must not scope the possession chain as the pan's producer; it is the follow/claim-frame bind producer only.

**Quote (§2/§4.1):** "P3 is `dword[rec+0x20]`, it is already bound live from `has_slot` (`fifa96_match_entities.c:768`), and the 'P3 possession unported' line is a stale ... comment (`entities.c:741`)." **Fresh:** `0x78588 CMP dword[EBP+0x20],0 / JNZ 0x7859E` re-read; `entities.c:768` binds `queued20 = e->has_slot ? 1 : 0`; `keeper_cpu.c:81` tests `queued20 == 0`, so the gate is the native "no slot" arm. The comment at `entities.c:741` is stale; land the erratum. `FU282 §1.4`'s label "no queued action" for the same cell is the same misnomer (optional same erratum).

**Quote (§3.4):** "So **any** row-04 dispatch that reaches the tail ends by installing the carrier row `5` on itself." **Fresh:** false as written. `0x7F106..0x7F111` (`JLE`/row-6-self) jumps to `0x7F131` and installs **row 6** on self (no invoke), skipping `0x7F125`'s row-5 install. The measured witness took the row-5 path (`code 5` at f=1513), but a pan is not guaranteed to hand off to the claim. Stage-2: the claim/re-arm model must carry the row-6 diversion.

**Quote (§5.1):** "at `out.tail != 0`, `fifa96_camera_event_set` ... + `fifa96_camera_event_dir` ... + `fifa96_camera_set_tracked`." **Fresh:** `out.tail` is emitted in `fifa96_action_carrier_arm` (`action_handlers.c:500`) before the native `0x7F3A1..0x7F4E7` gates; the native event also requires the `0x7F3A1` mode word, the `lane>0x20` camera/record triple copy, `camera.speed > word[+0x71]` (`0x7F3D7..0x7F3DA`) and the slot word (`0x7F472`). `out.tail` alone is an over-approximation. Stage-2: if the claim-time bind is wanted, port the gate tail with `0x7F3A1..0x7F4E7` (FU315-L1) or leave it out; do not fire on `out.tail` alone.

**Stage-2 consequence:** FU315's GO stands (natural claim at f=1514), but the claim is not the pan's producer. FU315-L1/L2 are claim/follow fidelity, out of phase-31's pan acceptance; the P3 line is a docs erratum, not code.
