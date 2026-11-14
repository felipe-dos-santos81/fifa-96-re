# Replay chain B7–B8 leg census

Date: 2026-11-13. Scope: the replay recorder/queue/arm/camera legs handed to
Slice B (B7/B8) in `2026-11-12-open-items-closure-slice-b-design.md`. Every
address/byte is a fresh Ghidra MCP read on `/FIFA96.EXE` (flat LE image);
sources are the four evidence files
`.superpowers/sdd/2026-11-13-replay-chain-b7-b8/evidence-{A,B,C,D}.md`, which
are the authority over any stale wording in the B7/B8 brief.

The timed exit is folded into RR-L7 and the resync into RR-L10, so the table
has seven rows.

| leg | native body/address | exact gate/store | engine cell or OPEN | test |
|---|---|---|---|---|
| RR-L2 | `FUN_00053DC4` (`0x53DC5 CMP [0x14E574],0; 0x53DD3 CALL 0x64074; 0x53DD8 [0x14E574]=1`); `FUN_00053A48` state-5→6 (`0x53A8E [0x14E578]=6`); `FUN_00064074`/`FUN_00064080`; `FUN_00064030` (`0x64030..0x64061`) | latch `[0x14E574]` has 35 xrefs across **nine** named functions (`FUN_000513EC`, `FUN_00051AB8`, `FUN_000537F8`, `FUN_00053DC4`, `FUN_00053DE0`, `FUN_00053BB8`, `FUN_00052030`, `FUN_00053A48`, `FUN_00052D68`) plus **five** orphan xrefs (`0x51485`, `0x51535`, `0x51805`, `0x518D5`, `0x51A49`) in the un-named `0x51441..0x51AB7` region (not "eight functions"). Re-enable call conditional on the latch nonzero: `0x53A95 CMP ESI,[0x14E574]; 0x53A9B JZ 0x53AAD; 0x53A9D CALL 0x64080; 0x53AA2 [0x14E574]=0`. `FUN_00064074`/`FUN_00064080` write `[0x109A90]` = 0/1. `FUN_00064030` writes `*[0x109AB8]`, `*[0x109ABC]`, `[0x109A94]`, `[0x109AAC]`, `*[0x109AC0]` (all zero); it does **not** write `[0x109A90]` or `[0x107310]`. | `enabled` (`[0x109A90]`) and `budget` (`[0x107310]`) are engine cells; a gate-preserving reset is native, the engine's budget zeroing is an adapter. The `[0x14E574]` latch has no engine cell — OPEN. | `test_engine_replay_ring.c` — reset preserves the recorder gate; the gate is toggled only by `64074`/`64080`; the budget is a separate adapter. |
| RR-L3 | `FUN_00036C70` (`0x36E79`/`0x36E7F` queue6 `0xFF` init; `0x36EB7` store `obj+0x8D`; `0x36ECE` add `0x0B`; `0x36EE5`/`0x36EF9` store `FUN_0004BA44`/`FUN_0004BA00`); `FUN_000627F4` band; `0x36F89`/`0x36F8E` pop → `[0x155C4F]` | queue6 base `0x155C6A..0x155C6F` (`[0x109A88]` points at it), native init `0xFF`; per-object `queue[i]=obj[0x8D]` gated `obj!=0 && byte[0x157C86]>=0 && [obj+0x20]!=0`, `queue[i] += 0x0B` when `byte[0x157C86]==byte[0x157ABF]`; `queue[4]=FUN_0004BA44()`, `queue[5]=FUN_0004BA00()`. Band `0x155C50 = FUN_000627F4(focus_x, focus_y)` gated `[0x15781C]!=0 && (0x155C4E & 0x40)==0` (x>0x46→1, x<−0x46→4, y<0x6A→|8, y>0x35→|2, all +1); flag `0x155C51=1`; queue-pop `0x155C4F = FUN_00064EE4()` (pop, returns value+1) outside the band gate. | composer stages queue/band/flag as 0 — OPEN (`0x157C64` object producer and the band/flag/pop producers are unported). | `test_engine_replay_ring.c` — queue6 `0xFF` init, band classifier thresholds. |
| RR-L4 | `FUN_00036C70` — player `0x36D44 MOV ECX,[EDX+0x28]; 0x36D47 MOV CL,[ECX]`; ball `0x36E20 MOV ECX,[0x158866]; 0x36E2D MOV CL,[ECX]` | one pointer load + one byte load (offset 0) → `**(byte**)(rec+0x28)` and `*[0x158866]`, both `row[0]`; not three dereferences. | the port's composed `stage.anim_id` (a row id) is the identity, not a new mapping (OL-80). | `test_engine_replay_ring.c` / bridge — appearance identity. |
| RR-L5 | `FUN_0004D134` (`0x4D13C CMP EAX,0x6; 0x4D13F JA 0x4D2AB`); jump table `0x4D118`; POSE `0x4D14D..0x4D165`; `FUN_000642FC` (`0x644B2..0x644C6`); six-field copy `FUN_0004C904` | index > 6 → epilogue `0x4D2AB`, which still stores `[0x107DDC]=index` at `0x4D2C4`; out-of-range changes no selection (no wrap in the selector). Table: 0→POSE `0x107968`, 1/2/3/6→VIEW `0x107CE8` (camera_sub 5/6/7/8), 4→ACTION `0x107B98`, 5→BALL `0x107B28`. POSE base = `0x107968 + ([0x107DD0]*8 − [0x107DD0])*0x10` (a `0x70` stride), not the bare constant. The `0x20` wrap is in `FUN_000642FC`, not the selector. | engine models VIEW/ACTION; POSE/BALL are fed elsewhere (partial — RR-L5 residuals). | `test_engine_replay_ring.c` / bridge — cycle, out-of-range no-op, POSE stride. |
| RR-L7 | `FUN_00051270`/`FUN_000512B0` → `FUN_000510DC` → `FUN_00064270`; `0x511D5 CALL 0x64E74; 0x511DC JNZ 0x51222`; `FUN_00064E74`; `FUN_00064DFC`; `FUN_00064E8C` | `FUN_000510DC` ranges — `0x510EB..0x51119` tick floor (`EDX=max(tick,1)`) + early return (`0x51114` returns 1 when `param_1==0 && 0x6400C()!=0`); `0x51122..0x5114A` request arm (`[0x14E58C]=1`, `CALL 0x64270`, `CALL 0x37114(4)`, `[0x108E0C]=1`, `[0x108E10]=0`); `0x5114F..0x5118A` `[0x108E0C]==1` arm; `0x5118B..0x511B4` mode advance; `0x511B5..0x51221` ramp (`0xF0`/`0x168`/clamp `0x169`, `CALL 0x64DFC` when phase>0xF0); `0x511D5`/`0x511DC` complete gate; `0x51222..0x51249` countdown (`tick>=phase` → `CALL 0x64E8C`, return 1). `FUN_00064270` only stores `[0x109A98]=0x81` (gated `[0x109AB0]!=0`) and calls `0x5865C`; it does **not** write mode/acc/phase. `FUN_0004B100` is **not** the request source. | mode-7 request/timed exit; the arm's `[0x14E58C]`/`[0x108E0C]`/`[0x108E10]` writes live in the caller `0x510DC` (engine `replay_begin` stores `0x81` only) — OPEN (request cells/timed exit). | `test_engine_replay_ring.c` / bridge — request chain, conditional timed exit. |
| RR-L10 | `FUN_00064EF0` (`0x64EF0..0x64F65`); `FUN_00064EC0` (`0x64EC0..0x64EE3`); `FUN_00064EE4`; `FUN_000974DC`; `FUN_00065CC0`; `FUN_0006428C` (`0x6428C..0x64299`); `FUN_00037AE4`/`FUN_00036BC8` | FIFO selector `EAX==1` push / `0` pop, value `EDX`; push store base `0x155C8C + head*4` (`0x64F21`), pop base `0x155C90 + tail*4` (`0x64F3B`); push **pre-increments** head (`0x64F1F`), evicts the tail when `count==20` (`0x64F10`), pop returns `value+1` (`0x64F4C`); count/head/tail `[0x109AEC]/[0x109AF0]/[0x109AF4]`. Drain gate `0x64EC0`: `[0x109A98] & 0x80` (`0x64EC3..0x64ECD`) → drain; else `0x37AE4`/`0x36BC8` compare `[0x105FFC]` vs 5 and 7; on pass sets `EAX=1` and calls `0x64EF0` with the caller's value in `EDX`. `0x974DC` calls the gate then `0x65CC0`; `0x65CC0` reads only globals (`[0x155CE0]/[0x155D30]/[0x155D00]/[0x10A250]`) and writes `[0x10A254]` via `0xA7705` — "audio" is inferred, not proven. Resync `0x6428C`: `MOV EAX,[0x155C4C]; SAR EAX,0x18` (byte `0x155C4F`); nonzero → `DEC EAX`, `JMP 0x974DC`. | the C ring's conventional indexing preserves order, not the address offset; the `0x974DC` drain half is a B handoff. | `test_engine_replay_ring.c` / bridge — FIFO push/pop/evict/wrap, drain gate, resync decrement. |
| C4R-L1 | `FUN_0005865C` (`0x58666 [0x109014]=0; 0x5866C CALL 0x56B88(0x2B); 0x58671 [0x14E9B8]>=0x80 → IDIV 3 over 23 dwords; 0x586B6 CALL 0x56894`); `FUN_000585E4` (`0x585F2 [0x109014]=1`) | `[0x109014]=0` is a **shared** projection-active flag (writer `0x58666`; `FUN_000585E4` writes 1 at `0x585F2`; five consumer functions incl. read-write `FUN_000586C0`) — NOT presentation-only. `FUN_00056B88(0x2B)`, the `/3` loop (23 dwords), and `FUN_00056894` are presentation-only. | engine models the flag (init 0, arm 0); the three projection bodies are legs — OPEN. | `test_engine_replay_ring.c` / bridge — view reset flag. |

## Fresh-read corrections

1. `[0x14E574]` has 35 xrefs: nine named functions plus five xrefs in the
   un-named `0x51441..0x51AB7` region. The brief's "eight functions" is wrong;
   the ninth is `FUN_00051AB8`. The `FUN_00053A48` re-enable call is conditional
   on the latch being nonzero (`0x53A95 CMP ESI,[0x14E574]; 0x53A9D CALL 0x64080`).
2. `FUN_00064030` writes `*[0x109AB8]`, `*[0x109ABC]`, `[0x109A94]`,
   `[0x109AAC]`, `*[0x109AC0]` (all zero). It does not write `[0x109A90]` or
   `[0x107310]`.
3. `0x64EC0` is the drain gate: state `[0x109A98] & 0x80`, then
   `0x37AE4`/`0x36BC8` compare `[0x105FFC]` against 5 and 7; on pass it sets
   `EAX=1` (push selector) and calls `0x64EF0` with the caller's value in `EDX`.
   `0x974DC` calls the gate then `0x65CC0`; `0x65CC0` reads only globals
   (`[0x155CE0]/[0x155D30]/[0x155D00]/[0x10A250]`) and writes `[0x10A254]` via
   `0xA7705`; calling it audio is inferred, not proven.
4. The `0x510DC` ranges: `0x510EB..0x51119` tick floor + early return;
   `0x51122..0x5114A` request arm (`0x14E58C=1`, `0x64270`, `0x37114(4)`, mode 1,
   acc 0); `0x5114F..0x5118A` `[0x108E0C]==1` arm; `0x5118B..0x511B4` mode
   advance; `0x511B5..0x51221` ramp; `0x511D5`/`0x511DC` complete gate;
   `0x51222..0x51249` countdown. `FUN_00064270` only stores `[0x109A98]=0x81`
   (gated `[0x109AB0]!=0`) and calls `0x5865C`; it does not write mode/acc/phase.
5. Camera selector: index > 6 jumps to the epilogue at `0x4D2AB`, which still
   stores `[0x107DDC]=index` at `0x4D2C4`; out-of-range changes no selection.
   POSE computes base `0x107968 + ([0x107DD0]*8 − [0x107DD0])*0x10` (a `0x70`
   stride), not the bare constant. The `0x20` wrap is in `FUN_000642FC`
   (`0x644B2..0x644C6`), not the selector.
6. The exact FIFO semantics: selector `EAX=1` push / `0` pop, value `EDX`; push
   store base `0x155C8C + head*4`, pop base `0x155C90 + tail*4`; pre-increment
   head, evict tail when `count==20`, pop returns `value+1`. The C ring's
   conventional indexing preserves order, not the address offset.
