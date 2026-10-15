# FU287 — the phase-6 arm + `FUN_0008D098` (the row-`0x1F` producer): port shape

**Phase/Provenance:** M2 phase-26c recon-ahead, Task 7 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP, case-sensitive; `watcom:LE:32:default`, image base 0, `.image` overlay, 2706 functions — confirmed this session). Every native address/byte below is a **fresh** `disassemble_bytes` / `disassemble_function` / `read_memory` / `get_xrefs_to` of this session unless marked *carried* (source named). Alias traps applied (`dword[x]>>0x18` = byte `x+3`; `>>0x10` = word `x+2`; CALL rel32 bytes verified). Read-only: no rename / comment / label / function-create / script / save; ISO untouched; no git. One draft.

## Question

Derive the **port shape** of the phase-6 arm inside `FUN_0008D098` (reached from `FUN_000740A0`), the row-`0x1F` installer it drives, the `[0x157C5C]` latch link to row `0x1F`, and the engine's phase-6 state — then rule whether the ported FU-150 chain already carries a **LIVE** phase-6 producer or whether the leg stays **T2b-L2**.

---

## 1. Port shape

**Native (fresh).** `FUN_000740A0` runs `FUN_0008D098` **once per team** — `0x740c8 CALL 0x8D098` with `EAX = 0x1588A4 + side*0x835`, then `0x740c5 XOR DL,0x1` / `0x740db CALL 0x8D098` for the other team; `EBP` inside `FUN_0008D098` is the **team base**. The phase-6 block is reached at `0x8D4A2`:

```
0x8D4A2  PUSH -1 ; MOV ECX,3 ; MOV EBX,0xA ; MOV EAX,EBP ; XOR EDX,EDX
0x8D4B2  CALL 0x0008CEB8                       ; install code 3 over records 0..0xA
0x8D4B7  MOV EDX,dword[0x00157AAC]            ; EDX = side_controlled word
0x8D4BF  SAR EDX,0x18                          ;   >>0x18  -> the side byte
0x8D4C2  MOV AL,byte[EBP+0x826]               ; team side byte
0x8D4C8  CMP EAX,EDX ; 0x8D4CA JNZ 0x8D558    ; team side != controlled -> 0x8D558
```

* **Controlled team falls through** (`0x8D4CA..0x8D558`, fresh): copies the `0x157740` block (0x3A dwords) onto the stack, `CALL 0x73DC4`, calls the penalty-spot probe `0x700F4`, then `CALL 0x79CCC` on `0x157740`; stores the result in `[EBP+0x7B2]`, `MOV EDX,0x13` (`0x8D532`), `MOV EAX,[EBP+0x7B2]`, and `0x8D546 CALL 0x7D9A4` — **install code `0x13` (penalty-spot reset) on the record returned by `0x79CCC`**.
* **Non-controlled team** (`0x8D558`, fresh): `MOV EDX,0x1F ; MOV EAX,EBP ; XOR ECX,ECX ; XOR EBX,EBX ; MOV dword[EBP+0x7B2],EBP ; 0x8D569 CALL 0x7D9A4` — **install row `0x1F` on record 0 (the team base `EBP`)**, and pin `[EBP+0x7B2] = EBP`.

So the phase-6 arm produces **row `0x1F` only for the non-controlled team's record 0**; the controlled team takes the penalty-spot reset (code `0x13`). This matches the task brief byte-for-byte.

**Row-`0x1F` → the `[0x157C5C]` latch (fresh).** The action-table entry `0x11075C` = `0x76380` (fresh `read_memory` = `80 63 07 00`), so the row-`0x1F` body is `0x76380` (no Ghidra function defined there — unrecovered). Fresh `disassemble_bytes`:

* Head `0x76388 CMP dword[EAX+0x20],0` → `0x76399 CALL 0x7876C` (an `[rec+0x20]`/`[rec+0x828]` sub-branch).
* `0x763A0 MOV EAX,dword[EBP+0x8F] ; SAR 0x18` (state index = byte `+0x92`); `0x763AB MOV byte[0x157C5C],DH` (`XOR DH,DH` first) — **the row-`0x1F` body clears `[0x157C5C]`**; `CMP EAX,2` selects the pos-reset tail.
* Tail `0x76580..0x765C2`: copies `rec.pos` (`[EBP+0x59]`) into `[EBP+0x4D]`, then if `[EBP+0x20]!=0 && word[[EBP+0x20]+0x4]!=0` → `0x76599 CALL 0x76130`; then if `[rec+0x8E]>>0x18 == 0x1F && [0x157A4C]==0` → `0x765B7 CALL 0x7DAB4`.

**`FUN_00076130` — the shared keeper aim helper (fresh; `0x76130..0x7636C`, 191 insns).** It reads `[rec+0x8E]>>0x18` (action code; `0x1F` short-circuits the `0x110680`-table gate at `0x761D1`), gates on `[EBP+0x20]`, phase 2 (`0x7615B`, `[0x15774C]` band ±`0x420`), phase 1 (`0x761A5`), the actor/side skip `[0x157A83]`, then installs keeper rows via `FUN_0007D9A4` — `0x76256 MOV EDX,0x19` (with the `0x15774C` triple copied to `[EBP+0x4D]`), `0x76291 MOV EDX,0x1C`, and the `0x7633F ADD EAX,0x1B` (`0x1B`/`0x1C` by `[EBP+0x63]>>0x10 >= 0x70`) — and **clears `[0x157C5C]` at `0x76318`** (`XOR BH,BH` then `MOV byte[0x157C5C],BH`).

Fresh `get_xrefs_to 0x157C5C` = **6**: `0x7802D` WRITE + `0x780D7` WRITE (`FUN_00077EAC`, the ladder — `0x7802B XOR AL,AL` → clear, and `0x780D7` from `BH`), `0x76318` WRITE (`FUN_00076130`), `0x763AB` WRITE (row-`0x1F` body `0x76380`), `0x76FF4` READ + `0x77003` WRITE (row-`0x1B` body `0x76D28..0x77728`). *(The carried FU273 §1.3 quoted "= 6" but enumerated only the three ladder/`0x76130` sites; the fresh set adds the row-`0x1B` read/write and the row-`0x1F` clear.)* `FUN_00076130` has 4 fresh xrefs: `0x7845D` COMPUTED_CALL in **`FUN_000782D0`** (the record-0 machine's function-pointer table; DATA `0x110954`/`0x11097C`) and `0x76599` UNCONDITIONAL_CALL in the row-`0x1F` body.

**Interplay.** Row `0x1F` is the phase-6 arm's product; its body (a) clears `[0x157C5C]`, (b) sets the keeper pos from `[EBP+0x59]`, and (c) calls `FUN_00076130`, which may install the follow-on keeper row (`0x19`/`0x1B`/`0x1C`) from the `0x15774C` camera triple and again clears `[0x157C5C]`. The **same** `FUN_00076130` is reached from the record-0 machine `FUN_000782D0` via its dispatch table. So `[0x157C5C]` is the **shared keeper-action latch** coupling the ladder (`FUN_00077EAC`), the row-`0x1F`/`0x76130` install path, and the row-`0x1B` body.

---

## 2. Engine state (fresh tree read)

* `fifa96_match_run_phase_arm` (`fifa96_match_run.c:2763`) — the `phase == 6u` block (`:2838-2853`) installs `0x1F` on `records[0]` for the non-controlled team (`:2850`), and the controlled team runs the penalty-spot reset + `0x13` (`:2841-2847`: `fifa96_camera_init(&camera,0,0,±0x8D0)`, `match_run_phase_pick`, install `0x13`). The `phase == 8u || phase == 9u` block (`:2854-2862`) installs `0x1D`/`0x1E` on the controlled team's record 0. This mirrors the native arm (a) exactly (the engine keys the record on `team->side` vs `side_controlled`) and (d).
* **The only phase-6 WRITER** is `match_run_ref_step_restart` (`:5984`, stage-3): it derives `phase = 7u`, flips to `6u` when `foul_kind != 3 && |incident_x| < 0x420 && incident_z ∈ [-0xB10,-0x7B0] (side 0) / [0x7B0,0xB10] (side 1)` (`:6008-6019`), then `match_run_write_phase(mr, phase)` (`:6024`) and `fifa96_match_run_phase_arm(mr, phase)` (`:6026`). It is dispatched on the **sit-9/sit-0xA → act-2** route (`match_run_situation_side` `:2897` → `fifa96_match_action_phase_18` → `match_run_ref_step_restart` at `:6058`) — the **FU-150 foul/ref chain**.

**Ruling.** There **is** a ported, structurally-live phase-6 producer in the FU-150 chain: `match_run_ref_step_restart` writes `phase 6` and immediately arms the phase-6 reducer, and the route is dispatched by the act-2/situation machinery (not a test-only or unwired call). It is therefore a **LIVE-LEG**, not a dead leg — but end-to-end liveness (a granted-frame ISO scenario that actually reaches `foul_kind != 3` + the incident band and installs row `0x1F`) is **unproven**, exactly as FU271 §4.1 labelled it. So the leg **stays T2b-L2** (LIVE-LEG; carried and re-confirmed, not closed). **NO-GO** to promote T2b-L2 to proven-live or to retire it; no stand-in producer is warranted (no-fakes rail).

---

## 3. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| phase-6 block: code-3 sweep, `>>0x18` side compare, `JNZ 0x8D558` | `disassemble_bytes 0x8D4A2` (`e801faffff`→`0x8CEB8`; `0f8588000000`→`0x8D558`) |
| controlled fall-through = penalty-spot reset + install `0x13` on `[EBP+0x7B2]` | `disassemble_bytes 0x8D4A2` (`0x8D526 CALL 0x73DC4`, `0x8D511 CALL 0x700F4`, `0x8D51D CALL 0x79CCC`, `0x8D532 MOV EDX,0x13`, `0x8D546 CALL 0x7D9A4`) |
| `0x8D558` installs row `0x1F` on record 0 (`EAX=EBP`, `[EBP+0x7B2]=EBP`) | `disassemble_bytes 0x8D4A2` (`0x8D558 ba1f000000`, `0x8D563 89adb2070000`, `0x8D569 CALL 0x7D9A4`) |
| `FUN_000740A0` runs `FUN_0008D098` once per team (`EAX=0x1588A4+side*0x835`) | `disassemble_function 0x740A0` (`0x740c8`/`0x740db CALL 0x8D098`) |
| row `0x1F` action-table entry = `0x76380` | `read_memory 0x11075C` = `80 63 07 00` |
| row-`0x1F` body clears `[0x157C5C]` (`0x763AB`) and calls `FUN_00076130` (`0x76599`) | `disassemble_bytes 0x76380`; `disassemble_bytes 0x76580` |
| `FUN_00076130` clears `[0x157C5C]` (`0x76318`), installs `0x19`/`0x1C`/`0x1B` via `0x7D9A4` | `disassemble_function 0x76130` (`0x76256`, `0x76291`, `0x7633F`, `0x76318`) |
| `[0x157C5C]` fresh refs = 6 | `get_xrefs_to 0x157C5C` = `0x7802D`/`0x780D7` (W, `FUN_00077EAC`), `0x76318` (W, `FUN_00076130`), `0x763AB` (W, row `0x1F`), `0x76FF4` (R)/`0x77003` (W, row `0x1B`) |
| ladder writes `[0x157C5C]` (`0x7802D` clear via `XOR AL,AL`; `0x780D7` from `BH`) | `disassemble_bytes 0x78025`; `disassemble_bytes 0x780D0` |
| `FUN_00076130` callers = record-0-machine table (`0x7845D`) + row `0x1F` (`0x76599`) | `get_xrefs_to 0x76130` = 4 (`0x7845D` COMPUTED_CALL, `0x76599`, DATA `0x110954`/`0x11097C`); `disassemble_bytes 0x78440` |
| engine phase-6 arm installs `0x1F` (non-controlled rec 0) / reset+`0x13` (controlled) | `fifa96_match_run.c:2838-2853` |
| engine phase-8/9 arm installs `0x1D`/`0x1E` | `fifa96_match_run.c:2854-2862` |
| engine's only phase-6 writer = `match_run_ref_step_restart` (FU-150) | `fifa96_match_run.c:5984-6030` (`:6015`/`:6017` phase 6, `:6024` write, `:6026` arm); dispatched `:6058` via `:2897-2913`; carried `FU271 §4.1` |

**Carried docs (read, not rewritten):** `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md` §2.4/§2.5/§5 (T2b-L2); FU271 §4.1; FU269 §4.3.

## 4. Numbered open legs

* **T287-L1 — the controlled-path record identity.** The native `0x13` install is keyed on the `CALL 0x79CCC` return (`[EBP+0x7B2]`), while the engine substitutes `match_run_phase_pick(mr, t, 0, pz)` (`fifa96_match_run.c:2845`). Whether `0x79CCC` is exactly the nearest-record pick is **unproven**; the record the penalty-spot reset lands on is a derived equivalence. Owner: `FUN_0008D098` (`0x8D517 CALL 0x79CCC`) / `FUN_00079CCC`.
* **T287-L2 (carried, LIVE-LEG) — end-to-end liveness of the FU-150 phase-6 producer.** The ported producer is wired (`match_run_ref_step_restart` → `phase 6` → arm), but no granted-frame ISO scenario proves the precondition set (`foul_kind != 3`, `|incident_x| < 0x420`, incident band) is reached; the ball-row `[0x158882]` term is unmodelled (FU166-L6), so the stage-3 gate can pass early. Owner: the FU-150 foul chain + `FUN_0008D098` phase arm. **Next hop: the RED-first reach test** (FU273 §4.4).
* **T287-L3 — `FUN_00076130` semantics (the shared keeper aim/steer).** It installs `0x19`/`0x1B`/`0x1C` from the `0x15774C` triple and `[EBP+0x63]>>0x10`, gated on phase 1/2 and the actor/side skip, and clears `[0x157C5C]`; called from both `FUN_000782D0` (table) and row `0x1F`. The exact install arm ordering is **unported**. Owner: FU180-L3 / FU186 (keeper cluster).
* **T287-L4 — the `[0x157C5C]` latch ordering (ladder ↔ row `0x1F` ↔ row `0x1B`).** Six fresh refs; the set/clear/read ordering that couples the ladder (`FUN_00077EAC`), the row-`0x1F` body, and row `0x1B` is **not pinned** (the ladder's `[0x157C5D]` gate vs `[0x157C5C]` latch is a distinct pair; FU273 §1.3). Owner: `FUN_00077EAC` + `FUN_00076130` + row `0x1B` (`0x76D28`).
