# FU-309 — the live event source (`[0x1577EE].hi > 0xF0`) + the pan writers + the visible acceptance

**Phase/Provenance:** M2 phase-29 recon-ahead wave (Stage-1, task 4/4), 2026-10-21, read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument; this session `get_current_program_info` = `watcom:LE:32:default`, image base 0, `.image` overlay, **2708 functions** — the brief's frozen count says 2706; two extra functions exist in the project DB before this session, no function was created/renamed/saved by it). Every address/byte below is a fresh `disassemble_bytes` / `get_xrefs_to` / `read_memory` of **this** session unless marked *carried* (the frozen doc is named). The alias traps are applied (`dword[x]>>16` = the word at `x+2`; `>>24` = the byte at `x+3`). No writes: no rename/comment/label/function/script/save; `/FIFA96.EXE` untouched; the ISO untouched; no git.

---

## 1. Result and verdict

**The every-frame auto-camera arm is port-exact and already wired live; the gap is the event, not the arm.** `fifa96_match_run_camera_follow` (`match_run.c:3878`) is the 0x739C6..0x73B6B call site plus `fifa96_camera_rate_event` (`camera.c:527`) = the 0x71E1C..0x71EEF first arm; its height gate reads the **word at 0x1577F0** through `dword[0x1577EE] >> 16` (fresh 0x71E26/0x71E2B and 0x739DC/0x739E1, both `SAR 0x10`), signed `> 0xF0` in the arm and `> 0x10` at the call site. The pan writers are also port-exact and live: the 64-pair rate table (0x71F26..0x71F53) → `camera.rate_x/rate_z`, the every-frame pan push (0x73B0C/B13/B30/B36/B42/B48) → the velocity/anchor cells, and the pan step (0x70A3F..0x70A90) + the ramp re-run (`FUN_00070544`). The remaining live input is the **event height**: no engine producer raises `event_param` above `0xF0` in open play today. The only wired setter that can pass `> 0xF0` is the receive arm's trajectory (`fifa96_match_run_ball_receive` → `fifa96_camera_event_set(..., traj, 0)`, `match_run.c:3946-3955`, native 0x7A207..0x7A219 fresh), and its faithful open-play row needs a jumping kicker (`pos_y != 0`) with `camY − pos_y >= 0x38` (the height-table selector) or a grounded band-2 kick (`camY − pos_y >= 0x70`). **FU307's mid-band Y is the prerequisite**; the record-event chaser (FU306) is the only per-frame re-arm. The row-0x0E pan arm's own event is structurally `<= 0xAE < 0xF0` (fresh 0x82A56/0x82A5D/0x82A61/0x82A67) and can never wake the follow.

**Verdict: GO** for the Stage-2 event-source slice with FU307's route as the named dependency. If FU307 returns NO-GO, Layer-2 is waived (spec §6) and the Layer-1 fixture below remains the landing (the staged receive + keeper-release chain is a legitimate witness, spec §8.2). The Layer-2 acceptance procedure is fixed in §5.5 with the **relative** AE gate (pan AE > the same-session still control), the same-commit cell assertions as the attribution, and the honest remainder named.

---

## 2. The arm, the alias, and the call-site gates (fresh)

### 2.1 `[0x1577EE].hi` is `word[0x1577F0]` — width and offset verified

The dword at `0x1577EE` covers bytes `EE EF F0 F1`; `SAR 16` therefore selects the **word at `0x1577F0`** (the event height), while the low word `0x1577EE` is the event cursor. Fresh body (`disassemble_bytes 0x71DF4`, 289 B):

```
0x71DF8  MOV EDX,[0x1577CA]                    ; tracked
0x71E03  TEST EDX,EDX / JZ 0x7200B
0x71E0B  EAX=EDX; EDX=[EDX+4]; CMP dword[EDX],0x18D8; JNZ 0x71EF0   ; class
0x71E1C  CMP dword[EAX+0x20],0; JZ 0x71EF0     ; slot pointer
0x71E26  MOV EAX,[0x001577EE]
0x71E2B  SAR EAX,0x10                          ; = word[0x1577F0] (height)
0x71E2E  CMP EAX,0xF0 / JLE 0x71EF0            ; the >0xF0 arm gate
0x71E4E  EDX=[0x1577CA]; EDX=[EDX+0x20]
0x71E5B  MOVSX AX,[EDX+0x20]; IMUL EAX,EAX,0xF; MOV [0x1577C0],AX
0x71E69  MOVSX AX,[EDX+0x21]; IMUL EAX,EAX,0xF; MOV [0x1577C2],AX
0x71E77  EAX=[0x001577BE]; SAR 16; CMP 0xF;   ...   ; the ±15 clamps
0x71ED5  EAX=[0x001577BE]; SAR 16
0x71EE0  CALL 0x8DC68                           ; distance
0x71EE5  MOV [0x001577BE],AX                    ; speed := |vel|
```

Alias check, fresh: `0x71E77 MOV EAX,[0x1577BE]; SAR 16` reads **word 0x1577C0** (the `vel_x` clamp), `0x71EA3 MOV EAX,[0x1577C0]; SAR 16` reads **word 0x1577C2** (the `vel_z` clamp), `0x71ECF EDX=[0x1577C0]; SAR 16` and `0x71ED5 EAX=[0x1577BE]; SAR 16` feed the distance — exactly the FU171 §3.1 clamps. The engine's `fifa96_camera_rate_event` (`camera.c:527-535`) clamps `dirs * 15` to ±15 and writes `speed`. **Port-exact.**

### 2.2 The every-frame call site (`0x739B0..0x73B80`, 107 insns, fresh)

```
0x739BB  DL = byte[0x157821]                     ; GATE 1 pan counter
0x739C6  TEST DL,DL / JNZ 0x73B70
0x739CE  CMP word[0x1577BE],0 / JZ 0x73B70       ; GATE 2 speed != 0
0x739DC  MOV EAX,[0x001577EE]; SAR 16
0x739E4  CMP EAX,0x10 / JLE 0x73B70              ; GATE 3 height > 0x10
0x739ED  CMP byte[0x157816],0 / JNZ 0x73A03      ; GATE 4 rate bytes
0x739F6  CMP byte[0x157817],0 / JZ 0x73B5B
0x73B5B  EBP=[0x1577CA]; TEST / JZ 0x73B70
0x73B65  CMP dword[EBP+0x20],0 / JZ 0x73B70      ; tracked has a slot
0x73B6B  CALL 0x71DF4
```

Two thresholds on the same word: `> 0x10` opens the call site; `> 0xF0` opens the first arm. Between them the call site takes the table arm (rate path). The engine gates 1–5 (`match_run.c:3885-3895`) are the exact set; the tracked/slot pair and the `CALL` order match. Gate 2 reads the **pre-tail** `[0x1577BE]` value the engine captures as `follow_speed` (`camera.c:85/99/149`; FU171 §3.2/T2 carried) — the keeper ladder's CX=1 read of the same cell is FU308's seam, not this gate's.

The rate-nonzero branch `0x73A03..0x73B59` is the **interpolation/pan-push arm** (its own tracked/class/height jiggle at 0x73A1E..0x73A7B can `JMP 0x73B5B` into the follow; its 0x73A80..0x73B59 path does the pan push, clears the cursor at 0x73B4E, calls 0x703E8 at 0x73B54, and skips to 0x73B70). The engine models the pan push inside `fifa96_camera_update_walk` (`camera.c:124-145`) and treats a nonzero rate pair as gate-4 refusal; see leg **FU309-L3** for the unreachability argument.

### 2.3 The integrator admission (`FUN_000736AC` 0x73760..0x737EF, fresh)

```
0x7376B  CMP word[0x1577BE],0 / JNZ 0x73783
0x73775  CMP word[0x1577F0],0 / JZ 0x73B70       ; no speed and no event -> skip
0x73783  BX=word[0x157A64]; DX=word[0x1577FA]; EDX+=EBX; MOV [0x1577FA],DX
0x737A1  CX=word[0x1577F0]; TEST CX,CX / JZ 0x737B9
0x737A6  EAX=[0x1577F8]>>16; CALL 0x70B94; CWDE; MOV [0x157750],EAX
0x737B9  AX=[0x1577FA]; CMP AX,[0x1577F4]; JG 0x737DA
0x737C8  CMP dword[0x15775C],0 / JLE 0x737DF    ; prevY > 0
0x737D1  CMP dword[0x157750],0 / JG 0x737DF     ; camY <= 0 -> descent step
0x737DA  CALL 0x709D0                            ; the pan step
```

The engine `fifa96_camera_update_walk` (`camera.c:74-151`) is the exact port (timer += delta, the arc store, the `timer > limit` admission; the descent arm remains outside, FU171-L4/T3 carried).

---

## 3. The sustained window and the pan writers (fresh)

### 3.1 The window (`FUN_000709D0` 0x70A30.., fresh)

```
0x70A3F  EAX=[0x0015781E]; SAR 0x18              ; = byte 0x157821 (alias)
0x70A47  CMP EAX,0x64 / JGE 0x70A6B              ; the cap
0x70A4C  CMP byte[0x157821],0 / JNZ 0x70A5F
0x70A55  CALL 0xCB2A4; MOV [0x1575C3],EAX        ; (unmodeled side cell)
0x70A5F  INC byte[0x1575CB]                      ; (unmodeled side cell)
0x70A65  INC byte[0x157821]                      ; THE pan counter
0x70A6B  EAX=[0x1577EE]>>16 (height); DL=byte[0x157819]
0x70A7B  IMUL EAX,EDX; PUSH 0x20; ...            ; height = h*decay/0x20
0x70A90  MOV [0x1577F0],AX                       ; the decay store
```

The clear is `0x70153 MOV byte[0x157821],AH` with `AH=0` (`0x70144 XOR AH,AH`) inside `FUN_000700F4`, reached by every setter (`0x71CC3`). So: **a setter clears the counter and opens the follow; the first pan step increments and locks it out until the next setter**. Fresh `get_xrefs_to 0x157821` = 11 (read 0x739BB, read/write 0x70A65, write 0x70153, the ladder reads 0x7812E/0x78144, the ramp fast/slow selector 0x7063D). The engine ports the counter (`camera.c:968`), the clear (`camera.c:934`) and the decay (`camera.c:970-972`).

**Window length is a function of camY.** F2 = `ramp(height)`, F4 = `2*F2`, F6 = `F2 − ramp(height − target_y)`, F8 = `F4 − F6` (fresh `0x7058B` F2, `0x7059A` F4, `0x705C4` F6, `0x705E8` F8, `0x705E1` timer). For the k=7 `0x100` event: F2 = `0x94 − T[0x640−0x100 >> 2 = 0x150] = 148 − 89 = 59`, F4 = 118. At camY `0x70`: F6 = 59 − ramp(0x90) = 59 − 45 = 14 → **(118 − 14)/2 = 52 frames** (the FU171-L1 pin, engine test `tests/test_engine_match_frame.c:2323-2328`). At the keeper-release camY `0x50`: ramp(0xB0) = 148 − T[0x164 = 99] = 49 → F6 = 10 → **(118 − 10)/2 = 54 frames**. Stage-2 must derive the expected window from the fixture's camY, not hard-code 52.

### 3.2 The pan writers

Four writers move the camera after the event (all fresh bytes):

| writer | native | engine |
|---|---|---|
| follow rate | `0x71E63 [0x1577C0] = dir_x*0xF`, `0x71E71 [0x1577C2] = dir_z*0xF`, `0x71EE5 [0x1577BE] = distance` | `camera_rate_event` (`camera.c:531-533`) |
| table rate | `0x71F26` compass, `0x71F4D/0x71F53` → `[0x157816]/[0x157817]`; gates `0x71EF0` dir byte ≥ 0, `0x71EFD..0x71F08` `[0x1577F8]>>16 = word[0x1577FA] <= 0x1E` | `fifa96_camera_rate_table` (`camera.c:1180+`) → `rate_x/rate_z` |
| every-frame pan push | `0x73B0C [0x1577C0] += rate_x`, `0x73B13 [0x1577C2] += rate_z`; `0x73B30/0x73B36` anchor `0x157770` / anchor2 `0x157788` += `rate_x*(F4−timer)`; `0x73B42/0x73B48` the z pair | `camera.c:124-145` (`target_x += dx; anchor_x += dx;` — see leg **FU309-L2**) |
| pan step | `0x70A65` counter, `0x70A90` height decay, `0x70B86` re-runs `0x70544(0)` over `[0x1577BA/BC]` | `fifa96_camera_pan_step` (`camera.c:958-989`) |

The FU293-L1 pan cells are now the engine's `camera.speed`/`vel_x`/`vel_z` (the phase-28 final fix wave's one-pair unification), and FU293-L3's pan-push delta is `timer_limit − timer` — see the erratum in §7.1.

---

## 4. The live event source: what can raise the height

### 4.1 Fresh census (31 refs, 10 writes)

`get_xrefs_to 0x1577F0` = **31** this session; writes: `0x71CD4/0x71CE5/0x71CFA` (setter), `0x70575/0x705FE` (ramp clamps), `0x70A90` (pan decay), `0x70D17/0x70D3A/0x70D97` (record-event chaser), `0x70159` (reset zero). **No other producer.** The three live wake routes (FU171 §4.1, confirmed by the fresh writer set):

1. a setter caller passing `height_arg > 0xF0`;
2. a setter caller with `camY > 0xF0` (the setter's clamp-up at 0x71CDB..0x71CE5 makes camY the floor);
3. the chaser `FUN_00070C08` ramp/reflect (`0x70D17/0x70D3A/0x70D97`), whose ramp crosses `0xF0` only for `rec.pos_y + 0x5D > 0x80` (carried FU171 §4.4, FU306's slice).

Route 2 needs an already-tall camY; route 3 needs a genuinely high jumping record. **Route 1 is the only wired open-play route, and its one caller is the receive arm.**

### 4.2 The receive arm's event set (fresh `0x7A1E0..0x7A23F`)

```
0x7A207  EDX=0x158738                            ; the staged vector
0x7A20C  EBX=[0x0015873C]; SAR 16                ; = word[0x15873E] (traj)
0x7A212  EAX=EBP; XOR ECX,ECX
0x7A219  CALL 0x71C94                            ; the event (height = traj)
```

Engine (`match_run.c:3946-3955`): `fifa96_camera_event_set(cam, vector->height, vector->z, traj, 0)` + `event_dir` + `set_tracked(actor_id)`. The traj's open-play producer is the code-4 mode-0x40 draw `0x90 + (r&7)*(0x10 − desc_15)` (`fifa96_ball_pairing.c:783`); the `k=7` draw yields `0x100`. Two selectors admit the row:

* **grounded band-2**: `d = camY − pos_y >= 0x70` (the `d < 0x20 / < 0x70 / else` table, `fifa96_action_handlers.c:126-127`; the ISO chain pins `camY = 0x70` at the kick);
* **airborne height-table**: `pos_y != 0 && d >= 0x38` (`fifa96_action_handlers.c:113-117`; the FU171 §6.3 route — a row-0x5E jump plus a mid-band Y is enough).

So the event-source port shape is a **producer chain**, not a camera-port change: row-0x5E jump (or ground band 2) + FU307's Y + the already-wired receive arm.

### 4.3 The pan arm cannot be the follow source

Fresh `0x82A4F..0x82A6C` (the row-0x0E stage-2 else arm): `CALL 0x92AC8` (RNG), `AL &= 0x3F`, `DX = word[0x157750]` (camY), `EAX += EDX`, `MOVSX EBX,AX`, `CALL 0x71C94`. The arm's own gate is `0x20 < camY < 0x70`, so its event height = `(rng & 0x3F) + camY <= 0x3F + 0x6F = 0xAE < 0xF0`. **The pan is a one-shot at mid-band, never a follow wake** (matches FU168 §7.3 and the engine's `match_sp_pan_arm`, `handlers.c:1328`).

### 4.4 The chaser and the table arm (carried/adjacent)

* The record-event chaser `FUN_00070C08` is ported and called per record (`match_run.c:576-621`, call site `:1508`); its `ball == 0` gate (`0x7C81B CMP dword[0x157750],0 / JZ 0x7C901`, fresh `disassemble_bytes 0x7C810`) hard-stops it on today's live chain — FU306 owns the ramp/reflect reach.
* `fifa96_match_run_camera_follow`'s table fall-through is the rate path; the keeper event gate `0x71F80..0x71F8D` re-reads the **height** (`dword[0x1577EE]>>16 > 0xC0`) and `0x71F93..0x71FAA` checks `|[0x157770]| < 0x240`; the `0x92998(1,2,-1)` ring result is staged 0 (OL-27), leg **FU309-L8**.

---

## 5. The port shape (Stage-2 contract)

### 5.1 A. The arm: no camera-port change is owed

Keep `fifa96_match_run_camera_follow` → `fifa96_camera_rate_event` as is. The RED proof that the *arm* is not the gap: with `event_param = 0x100`, `tracked` + slot + dirs nonzero and `pan_counter == 0`, the frame path already returns 1 and moves `pos_x` by `vel*delta` (the phase-14 T2 window test, `tests/test_engine_match_frame.c:2285-2349`, and `test_auto_camera_follows_tracked_slot:7000` pinning every gate). Stage-2's new RED must ride on the **producer**, not on a re-implementation of the arm.

### 5.2 B. The event source (the producer chain)

1. **The jump**: put the kicker on the verified jump-aux **anim** row `0x5E` (`fifa96_match_run_record_anim_install(rec, 0x5E, 0)`, the FU-153/T1 seam — 0x5E is the anim row, not an action code) and step until `rec->pos_y != 0` (the height-table selector).
2. **The Y**: FU307's mid-band producer must hold `0x20 < camera.pos_y < 0x70` across the kick/receive frame (the row-0x0E pan gate) and `camera.pos_y − kicker->pos_y >= 0x38` (the height-table selector). If FU307 returns NO-GO, this chain is not live — Layer-2 waives (spec §6).
3. **The receive**: the frame-path ball pairing must stage code 4 + flags `0x40` and draw `k=7` → `traj = 0x100` → `fifa96_match_run_ball_receive` runs the setter, binds `tracked`, clears `controlled`, acks (`match_run.c:3946-3961`).
4. **The order**: the setter runs in the entity chain (after `update_walk`); the first follow frame is the **next granted frame**. The setter's `speed` must be nonzero (seed pair / F8) or gate 2 (`0x739CE`) refuses forever — the fixture should assert `camera.speed != 0` at the bind.

### 5.3 C. The pan writers

No new writer. The FU293-L1/L3 cells are the unified `camera.speed/vel_x/vel_z`, `event_peak`/`ramp_divisor`, and the pan push uses `timer_limit − timer` (fresh Block F). The only fidelity fix candidate is the anchor pair (leg **FU309-L2**).

### 5.4 D. The Layer-1 fixture shape (spec §8.2)

Extend `test_keeper_installer_live_reach` (`tests/test_engine_match_frame.c:3660-3718`) — the real frame body, producer inputs only, which already yields `camera.pos_y` `0x38 → 0x50` and `camera.tracked == 11`:

1. keep the keeper-release chain (the mid-band `0x50` and the id-11 bind);
2. install row `0x5E` on the kicker and step until `kicker->pos_y != 0` (the airborne selector);
3. `fifa96_match_run_ball_receive(&mr, kicker_id, &vec, 0x100)` (the real setter producer; the staged vector/traj inputs are allowed): assert `event_param == 0x100`, `tracked == kicker_id`, `pan_counter == 0`, `speed != 0`;
4. granted-frame loop: assert `vel == dirs*15` and `pos_x == prev + vel*delta` while `pan_counter == 0`; assert the window = `(F4 − F6)/2` for the fixture's camY (54 at `0x50`, 52 at `0x70`); assert `pan_counter 0 → 1`, the lockout (`fifa96_match_run_camera_follow == 0`), and that a new setter re-opens;
5. the pan arm: a row-`0x0E` record with `stage92 == 2`, `jump_base == 0`, `lane_x < 0x60`, camY mid-band → `match_sp_pan_arm` fires; assert the setter's height = `(rng&0x3F) + camY` (`<= 0xAE`), the tracked bind and `stage92 == 3`, and the pos delta through `rate/vel`;
6. **mutation-RED riders** on each new producer: the height gate (`camera.c:530`), the counter INC (`camera.c:968`), the setter clear (`camera.c:934`), the receive bind (`match_run.c:3954`), the pan band (`handlers.c:1328`), the jump selector (`action_handlers.c:113`). Each must turn its assert RED.
7. **RED bar**: the *live-route* assertions (1–5 through the producer chain) fail on the phase-28 baseline; the directly-staged receive assertions already pass there and are corroboration, not the RED.

### 5.5 E. The Layer-2 acceptance procedure (spec §8.3, same commit as the cell assertions)

Host: ISO present, SDL3 build, `DISPLAY=:1`; `make game` (`./build/fifa96`, 960×720 = 320×240 ×3).

1. **Walkthrough**: intro RETURN skip → front-end BACKSPACE (panel) → RETURN (panel confirm → match); let the clock start.
2. **Drive the receive/long-ball route** (the only `>0xF0` source): KICK press+release to reach phase 2; `V` (LONG_BALL) press+release; hold `UP` until the claim; release; `Z` (KICK) press+release; keep a direction held while the tracked record moves (its slot dirs are the follow rate — a stationary tracked record gives `vel = 0`, leg **FU309-L7**). The follow window is ~1.7 s (52–54 frames).
3. **Pan pair**: with the window open, capture `docs/screens/p29-l2-pan-a.png` and `…-pan-b.png` at the **same clock second** (two `import` calls back-to-back, or a 2 s lossless clip and two frames inside one HUD second). Crop both to the scene band `960x591+0+0` (the HUD/clock sits below `y = 591`).
4. **Still control**: no key held, same clock second, same crop → `p29-l2-still-a/b.png`.
5. **AE gate** (relative, binding): `AE_pan > AE_still`. Record both numbers. Measured fresh this session on the phase-28 pairs: pan **1582**, still **2052** (whole-image == crop for both, i.e. the HUD band contributed 0). Guidance, not a floor: a real whole-scene camera shift lands near the phase-9 magnitudes (`~2.9e4–4.6e4`, ≈14–22× the still control); a pan AE in the low thousands is a fail-signal to investigate.
6. **Attribution**: the same-commit Layer-1 assertions (§5.4) are the primary witness — they prove the camera cells (`pos_x/pos_z` deltas, `pan_counter`, `event_param`, `tracked`, the pan-arm cells) moved through the real frame path. The AE is corroboration only; it cannot separate camera translation from entity animation (**FU305-L6** carried). The fresh p28 pan/still numbers (1582/2052) are exactly that limit: the pan pair had *less* AE than the still control because no camera ran.
7. **Honest remainder**: if FU307's Y route is not live, or the tracked record's slot dirs are zero through the window, Layer-2 is a numbered waiver (spec §6), not a widened fixture; the Layer-1 cell witness remains the landing.

Exact capture/AE commands (the tools are present on this host: `import`, `xdotool`, `convert`, `compare`, `ffmpeg`):

```
WID=$(xdotool search --name '^FIFA 96$' | head -1)
import -window "$WID" docs/screens/p29-l2-pan-a.png
  # … second capture into p29-l2-pan-b.png (same clock second) …
convert docs/screens/p29-l2-pan-a.png -crop 960x591+0+0 +repage /tmp/opencode/p29-pan-a.png
convert docs/screens/p29-l2-pan-b.png -crop 960x591+0+0 +repage /tmp/opencode/p29-pan-b.png
compare -metric AE /tmp/opencode/p29-pan-a.png /tmp/opencode/p29-pan-b.png null: 2>&1
```

(The crop can also be done inline with `compare -metric AE -crop 960x591+0+0 …`; both reproduce the fresh 1582/2052.)

---

## 6. Evidence table (this session unless *carried*)

| # | claim | evidence |
|---|---|---|
| 1 | program `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image`; **2708** fns observed (brief: 2706) | `get_current_program_info` |
| 2 | `[0x1577EE] >> 16` = word `0x1577F0`; signed `> 0xF0` arm gate | `disassemble_bytes 0x71DF4` (289 B): `0x71E26 MOV EAX,[0x1577EE]`, `0x71E2B SAR 0x10`, `0x71E2E CMP 0xF0`, `0x71E33 JLE` |
| 3 | call-site gates: pan counter, speed, height `> 0x10`, rate bytes, tracked/slot, `CALL 0x71DF4` | `disassemble_bytes 0x739B0` (464 B): `0x739BB/0x739C6`, `0x739CE`, `0x739DC..E7`, `0x739ED/F6`, `0x73B5B..6B` |
| 4 | first-arm write set: `dir_x*15`, `dir_z*15`, ±15 clamps (alias reads), `speed = distance` | `0x71E54/0x71E63/0x71E69/0x71E71/0x71E77/0x71EA3/0x71EE5` |
| 5 | integrator: timer += delta, arc store `[0x157750]`, pan step on `timer > limit`, descent admission | `disassemble_bytes 0x73760` (144 B) |
| 6 | pan counter: `[0x15781E]>>24` = byte `0x157821`, cap `0x64`, `INC` at `0x70A65`, decay at `0x70A90` | `disassemble_bytes 0x70A30` (112 B) |
| 7 | setter clear `0x70153`; ramp F2/F4/F6/F8 stores | `disassemble_bytes 0x70130` (69 B), `0x70544` (208 B) |
| 8 | table arm: dir byte gate, timer (`[0x1577F8]>>16` = word `0x1577FA`) `<= 0x1E`, rate pair `0x157816/17`, height `> 0xC0`, anchor `|[0x157770]| < 0x240` | `disassemble_bytes 0x71EF0` (293 B) |
| 9 | pan push cells: `0x73B0C/B13` velocity, `0x73B30/B36` anchor `0x157770`/anchor2 `0x157788`, `0x73B42/B48` z | `disassemble_bytes 0x739B0` (same 464 B window) |
| 10 | pan-arm event height = `(rng & 0x3F) + word[0x157750]` (`<= 0xAE`) | `disassemble_bytes 0x82A3E` (56 B) |
| 11 | receive setter: `EDX=0x158738`, `EBX=[0x15873C]>>16` = `word[0x15873E]`, `CALL 0x71C94` | `disassemble_bytes 0x7A1E0` (96 B) |
| 12 | `0x1577F0` writer census: 31 refs, 10 writes (setter ×3, ramp ×2, decay, reset, chaser ×3) | `get_xrefs_to 0x1577F0` |
| 13 | `0x1577EE` writers are word stores (cursor low): `0x73B4E`, `0x709AC`, `0x70239`; height reads at `0x71E26/0x71F80/0x739DC/0x73A33` | `get_xrefs_to 0x1577EE` = 27; `disassemble_bytes 0x70228` |
| 14 | `0x157821` = 11 refs; `0x1577BE` = 70 refs | fresh `get_xrefs_to` |
| 15 | cell defaults zero at load (`0x1577EE` 16 B) | `read_memory 0x1577EE` |
| 16 | arm + gates ported live; frame-path call order `update_walk` → `camera_follow` | `match_run.c:3878-3904`, `:4324`, `:4337`; `camera.c:527-535`, `:74-151`, `:957-989` |
| 17 | receive arm's setter + bind | `match_run.c:3946-3955` (source) |
| 18 | 52-frame window pin; direct-call follow tests | `tests/test_engine_match_frame.c:2291-2349`, `:7000-7079` |
| 19 | phase-28 Layer-1 fixture (0x38→0x50, tracked 11) | `tests/test_engine_match_frame.c:3660-3718` |
| 20 | p28 pan/still AE = 1582/2052, whole-image == crop | fresh `compare -metric AE` on `docs/screens/p28-l2-*.png` |
| 21 | chaser/table arm context | *carried* FU171 §4.1/§4.4, FU306 (the chaser slice), FU303/FU294-L2 |

---

## 7. Errata (quoted, fresh)

1. **FU293-L3's "pan-vector pair `[0x1577F2]`/`[0x1577F8]`".** Fresh Block F (`0x7733F MOV EDX,[0x1577F2]` / `0x77345 MOV EAX,[0x1577F8]` / `0x7734A/4D SAR 0x10` / `0x77350 SUB`) shows the shifted words are **`0x1577F4` (timer limit) − `0x1577FA` (timer)**, i.e. the remaining window — not F2/F8. The engine's `camera.c:134` (`timer_limit − timer`) is the correct semantic; the frozen label is the misread.
2. **FU171 §3.3's 52-frame figure is the camY `0x70` pin.** At the keeper-release camY `0x50` the same formula gives 54 frames (F6 = 10, §3.1). Stage-2 must not hard-code 52 across fixtures.
3. **The function count.** The constraints expect 2706; `get_current_program_info` reports 2708. No function was created by this session; the two-function delta predates it and does not touch any window above.
4. **FU152's cell-map line (`0x15774C/50/54` = `target_*`).** Fresh `0x7054B..0x70557` copies `0x15774C..` → `0x157764..` (the origin/target triple) and `0x707F6/0x70955` read `0x157764`/`0x15776C` as the origin; `0x15774C` is the camera position (the setter pushes it at `0x71CB6`). Load-bearing only for the anchor-pair leg below; the engine's `pos_*` mapping is the correct one.

---

## 8. Numbered legs (each with its native owner)

1. **FU309-L1 — the live `>0xF0` producer chain is not yet proven live.** The receive arm's traj is the only wired route; its open-play row needs the row-0x5E airborne selector (`pos_y != 0 ∧ camY − pos_y >= 0x38`) or the grounded band 2 (`>= 0x70`), and FU307's mid-band Y. Owner: `FUN_0007A084 0x7A207..0x7A219` + `0x15873E`/the code-4 draw (`fifa96_ball_pairing.c:783`) + `match_run.c:3946`; decided with FU307 (and FU306 for re-arm).
2. **FU309-L2 — the pan-push anchor pair.** The engine adds `dx/dz` to `target_*` + `anchor_*` (`camera.c:139-142`); fresh `0x73B30/0x73B36/0x73B42/0x73B48` add to **anchor `0x157770` and anchor2 `0x157788`** only (no target store). Owner: `0x73B00..0x73B48`; fix site `fifa96_camera.c:139-142`.
3. **FU309-L3 — the rate-nonzero call-site arm is not modelled as a branch.** Native `0x73A03..0x73B59` can `JMP 0x73B5B` into the follow when tracked/class/height `>0xF0` hold (the 0x73A42..0x73A7B timer/peak jiggle); the engine's gate 4 returns 0 unconditionally and does the pan push in `update_walk`. Structurally unreachable today (every setter clears the rates at `0x7099A/0x709A0`; only the table arm sets them; no non-setter height writer exists), so no live wrong motion — pinned by the fresh `0x1577F0` writer set. Owner: `0x73A03/0x73B5B` + `camera.c:124-145/3888`.
4. **FU309-L4 — the event-cursor step word.** The follow's fail path adds `word[0x1577C4]` (`0x73AA2`); the native reset/ramp write it (`0x70240`, `0x709B3`), while the engine's `step = distance(acc)` (`camera.c:109`). Not hit by the first-arm follow window; the equivalence is carried FU-152 §2.9. Owner: `0x70240/0x709B3/0x73AA2`.
5. **FU309-L5 — the live camera-cell trace (Layer-2 corroboration) does not exist.** The phase-28 precedent is a read-only `gdb` sample; the same-commit Layer-1 assertions are the attribution. An env-gated, default-off per-granted-frame camera log in `match_run.c` would let the captures be aligned to the window without moving M1/M2. Owner: the acceptance procedure / `fifa96_match_run_frame`.
6. **FU309-L6 — the still-control AE is host/state dependent.** Fresh p28 re-measure: pan 1582, still 2052 (whole-image == crop). The relative gate must re-measure the still pair in the same session; there is no absolute floor (FU305 precedent is guidance). Owner: the acceptance procedure (FU305-L6 carried).
7. **FU309-L7 — the follow rate needs a moving tracked record.** `vel = slot dirs × 15` (`0x71E54..0x71E71`); a stationary tracked record yields `speed == 0` and no visible motion even with a live `>0xF0` event (gate 2 refuses). The live acceptance must hold a direction or observe the AI mover's dirs; the fixture asserts `rec->dir_x != 0`. Owner: the entity pool's dir producer + the acceptance step.
8. **FU309-L8 — the table arm's keeper event ring.** `FUN_00092998(1,2,-1)` is staged 0 (OL-27) in `match_run.c:3901-3903`; the rate write is live, the `0x1D/0x1E` `0x8F188` sink stays a leg (only matters when the table arm's rate path runs). Owner: FUN_00092998 / OL-27, carried FU303/FU294-L2.

---

## 9. Provenance (this session)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled): `get_current_program_info`; `disassemble_bytes` `0x71DF4` (0x121 B), `0x739B0` (0x1D0 B), `0x73760` (0x90 B), `0x70A30` (0x70 B), `0x71C94` (0x160 B), `0x70130` (0x45 B), `0x70544` (0xD0 B), `0x70630` (0xE0 B), `0x707B0` (0x80 B), `0x708F0` (0xC0 B), `0x709A0` (0x40 B), `0x71EF0` (0x125 B), `0x7A1E0` (0x60 B), `0x82A3E` (0x38 B), `0x7C7C0` (0x50 B), `0x7721B` (0xF8 B), `0x77313` (0x88 B), `0x70228` (0x30 B), `0x7C810` (0x30 B); `get_xrefs_to` `0x1577F0` (31), `0x1577EE` (27), `0x1577BE` (70), `0x157821` (11); `read_memory` `0x1577EE` (16 B). Engine/test source read: `src/fifa96_engine/fifa96_match_run.c` (540-621, 1380-1418, 3878-3904, 3946-3969, 4260-4370), `src/fifa96_loader/fifa96_camera.c` (20-199, 515-535, 640-955, 957-1109), `src/fifa96_engine/fifa96_match_handlers.c` (1274-1400), `src/fifa96_loader/fifa96_action_handlers.c` (95-135), `src/fifa96_loader/fifa96_ball_pairing.c` (770-800), `tests/test_engine_match_frame.c` (127-201, 1144-1151, 2140-2352, 3626-3718, 7000-7241), `docs/screens/p28-l2-*.png` (`compare` fresh). Carried docs: FU171 (the gates/window/census), FU168 (the pan arm and its mid-band window), FU305 (the acceptance contract, FU305-L5/L6), FU293 (L1/L3), the phase-28 register (`docs/ENGINE.md:1924-2233`). No writes: no rename/comment/label/function/script/save; no git; no ISO write.
