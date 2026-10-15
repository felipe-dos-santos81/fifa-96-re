# FU297 — R11-L4: the row-`0x1A` live/NULL decision + the census/M2 impact

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 8 (bounded, read-only derivation); draft awaiting controller promotion.

Read-only Ghidra session, authoritative program exactly **`/FIFA96.EXE`**
(case-sensitive; `get_current_program_info` this session: `watcom:LE:32:default`,
image base 0, 2706 functions, 15239 symbols, `.image` overlay). Every native
address/byte/ref-count below is a **fresh** `read_memory` / `disassemble_bytes` /
`get_xrefs_to` of **this** session unless marked *carried* (then the frozen `FU*`
slice is named); engine/test source facts cite the built tree by line, read-only.
Alias traps applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`);
CALL rel32 verified. **No writes** — no rename / comment / label / function /
script / project save; `/FIFA96.EXE` untouched; ISO never touched; no git command.
Carried input: `docs/ghidra/FU284_r11l4_row1A.md` (§1/§2.3/§4), `docs/ghidra/
FU289_contact_kind3_producer.md` (the LIVE-LEG ruling pattern), `FU273` §1.4/§3.2,
`FU272` §1.3-1.4.

> **Line-drift note.** FU284 (written a phase earlier) cites the row-`0x1A` body at
> `fifa96_match_handlers.c:3901-3940` and the action-table row at `:5517`. Fresh
> this session the body is **`:4017..4056`** and the row is **`:5633`** — a
> consistent **+116** drift (the file grew). No content change; line numbers only.
> All engine cites below are the **fresh** numbers.

---

## 1. The row entry + the sole table reference (fresh)

Fresh `read_memory 0x110748` (16 B) = `2c 66 07 00 | 28 6d 07 00 | 28 77 07 00 |
b0 4e 07 00` → action table `0x1106E0[0x1A] = 0x7662C`, `[0x1B] = 0x76D28`,
`[0x1C] = 0x77728`, `[0x1D] = 0x74EB0`. Reconciles FU284 §1 exactly. Fresh
`get_xrefs_to 0x7662C` = **1** (`0x110748` `DATA`). The action table is the row's
**sole** reference — and the table's **sole reader** is `FUN_0007D9A4` (fresh
`get_xrefs_to 0x1106E0` = **2**, both in `FUN_0007D9A4` at `0x7DA77`/`0x7DA7C`).

---

## 2. The installer is the row dispatcher (fresh) — the ladder DOES install AND call row `0x1A`

`FUN_0007D9A4(rec=EAX, row=DX, stage=BX, run=ECX)` is the row install/dispatch.
Fresh `disassemble_bytes 0x7DA57`:

```
0x7DA63  MOV AL,byte[ESP+4]        ; row id
0x7DA67  MOV byte[ESI+0x91],AL     ; rec.cur_row := row
0x7DA6D  MOV EAX,[ESP+2] ; SAR 0x10 ; SHL 2 ; ADD 0x1106E0 ; MOV EAX,[EAX]
                                   ; -> handler := action_table[row]  (0x1106E0[row])
0x7DA8F  MOV [ESI+0x18],EAX        ; rec.handler := handler
0x7DA95  MOV byte[ESI+0x92],AL     ; rec.stage := stage
0x7DAA8  MOV EAX,ESI ; CALL dword[ESI+0x18]   ; (when ECX/DI nonzero) INVOKE the handler
```

So a `FUN_0007D9A4(rec, 0x1A, 0, 1)` writes `0x1A` into `rec+0x91`, loads
`0x1106E0[0x1A] = 0x7662C`, and (non-zero `ECX`) **calls the row body**.

The ladder `FUN_00077EAC` (fresh `get_xrefs_to 0x77EAC` = **1**: `FUN_000782D0`
`0x78599`). Fresh `disassemble_bytes 0x78200`:

```
0x78205  DEC ECX ; CMP CX,0x4 ; JA 0x782c4
0x78215  JMP dword CS:[EAX*4 + 0x77E98]     ; table 0x77E98 (fresh read_memory, 20 B)
0x77E98  = {0x7826d, 0x78245, 0x7821d, 0x78294, 0x782b1}
0x7821d  ESI=0x2E ... 0x78227 MOV EDX,0x1a ; [0x157C2C]=0x2E ; CALL 0x7D9A4
0x78245  EDI=0x2D ... 0x7824f MOV EDX,0x1a ; [0x157C2C]=0x2D ; CALL 0x7D9A4
0x7826d  EAX=0x2C ... 0x78277 MOV EDX,0x1a ; [0x157C2C]=0x2C ; CALL 0x7D9A4
0x78294  MOV EDX,0x1b ; CALL 0x7D9A4        ; row 0x1B
0x782b1  MOV EDX,0x1c ; CALL 0x7D9A4        ; row 0x1C
```

Fresh table order (selector `ECX` after `DEC`): `1->0x7826d (0x2C)`, `2->0x78245
(0x2D)`, `3->0x7821d (0x2E)` — all three install **row `0x1A`** on `EBP` with
`[0x157C2C] in {0x2C,0x2D,0x2E}`, then `CALL 0x7D9A4` (which invokes the body when
`ECX=1`). *(FU284 §2.2 / FU273 §1.4 name the `[0x157C2C]` values case-1 `0x2E`,
case-2 `0x2D`, case-3 `0x2C`; the fresh table shows the values in ascending
selector order `0x2C/0x2D/0x2E` for selectors 1/2/3 — a **naming-order nuance only**,
same value set, same row.)*

**So, mechanically, the ladder's install DOES call the row body and can reach the
sit-5 emit.** The ladder is buried: fresh `get_xrefs_to 0x782D0` = 1 and
`disassemble_bytes 0x8DB20` confirms the caller chain `FUN_0008D8EC 0x8DB35 CALL
0x782D0` (with `EBX=1`), inside the unported pre-pass `FUN_0008D8EC` (fresh
`get_xrefs_to 0x8D8EC` = **4**: `0x4B2AB`/`0x4B2B2` (`FUN_0004B100`), `0x74429`/`0x74430`),
and `FUN_000782D0 0x78599 CALL 0x77EAC` is gated by `[EBP+0x8e]>>0x18 == 2`
(`0x78580`) + `[EBP+0x20]==0` + `[0x157C5D] != 0` (`0x7858e`).

---

## 3. The emit route (fresh) — the sit-5 emit lands in the unported FU-146 queue

The row's stage-4 tail is the sit-5 emit (fresh `disassemble_bytes 0x76AF0`):

```
0x76AF6  EAX=[0x157A49] ; SAR 0x18 ; CMP EAX,0x1 ; JZ 0x76b21   ; gate
0x76B03  MOV EDX,[EBP] ; MOV DL,[EDX+0x826]                      ; EDX = side
0x76B0C  MOV EBX,0x1                                             ; <<< queue flag
0x76B11  MOV EAX,0x5                                             ; sit 5
0x76B16  AND EDX,0xff
0x76B1C  CALL 0x8A938                                            ; e8173e0100 -> 0x8A938
```

Fresh `disassemble_bytes 0x8A938`: the situation dispatcher gates on `[0x14C32A]
!= 0` (`0x8A957`) and `[0x15B6C0] == 0` (`0x8A964`), then `EAX-2` selects a lane via
table `0x8A8E0` (`0x8A996`) and stores a flag at `[0x15B6B8]`. With **EAX=5, EBX=1**
the emit routes through the **situation-5 queue (id-7)** to the unported **FU-146
screen machinery** — **never** the engine's live direct-phase route (carried:
FU272 §1.3-1.4, FU269-L5; the *carried* sit-5 reach also dead-ends at the R11-L1
claim bridge `0x6E598`, FU273 §3.2).

---

## 4. Verdict (a) — the fresh dichotomy resolves to FU284's NO-GO

The ladder install mechanically reaching the body (Step 2) is **not** the deciding
fact. The deciding facts, freshly re-verified:

1. **The body is <½ ported.** Fresh `fifa96_match_action_1A`
   (`fifa96_match_handlers.c:4017-4056`) models **only** the head (`:4031-4033`
   phase-`0xD`+side → `r->controlled=1`), `timer89 += delta` (`:4034`), the
   `stage92 > 4` exit (`:4035`), and **stage 0** (`:4036-4054`) — then
   `/* 0x766FC.. the [0x157C2C] direction machine + movement/event block and stages
   1-4 -> FU186-L1. */` and `return FIFA96_OK;`. There is **no stage switch**
   (table `0x76618` = `{0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}`, fresh
   `disassemble_bytes 0x76618` = data), **no direction machine**, **no event
   block**, and **neither emit**.
2. **The emit route is unported.** Even with the ladder live, the sit-5 emit
   (Step 3, BX=1) is the **FU-146 queue id-7** route, not the engine's live route.
   Wiring the slot would be a **dead count** — exactly the FU289 live-vs-leg shape
   (a row wired but whose producer/consumer is unported is a LEG).
3. **The ladder + pre-pass are unported** (FU186-L5/L6/L7; the engine comment
   `:5634` names them).

**Verdict: FU284's NO-GO STANDS.** Row `0x1A` stays **`{0x1A, NULL}`** at
`fifa96_match_handlers.c:5633`. The ladder's `0x1A` install *would* call the row
body and *would* reach the sit-5 emit, but the emit is the unported **FU-146** queue
route and the body is half-ported — so a live install+dispatch is **not honest
census upside**. It is a LIVE-LEG (wired-but-unraised) at best, and the entry is
unresolved, so it is not even that; it is **NULL**.

---

## 5. Census delta (b) — exactly +1 action row = `59/80 → 60/80 = 73% → 75%`

Census source of truth: the pinned counts
(`tests/test_engine_m2.c:3668` and `:4043`, both `action_wired == 36u &&
phase_wired == 23u`) over the table sizes
(`include/fifa96_engine/fifa96_match_handlers.h:34/35`: action `45`, phase `35`;
total `80`). `floor(100 × wired / 80)`.

| state | action_wired | phase_wired | wired | total | `floor(100·wired/80)` | README title |
|---|---|---|---|---|---|---|
| **current** (fresh) | 36 | 23 | **59** | 80 | `floor(73.75)` = **73** | `73%` |
| row `0x1A` wired (`fn != NULL`) | **37** | 23 | **60** | 80 | `floor(75.00)` = **75** | `75%` |
| **delta** | **+1** | **0** | **+1** | 0 | **+2 pp** | `73% → 75%` |

The delta is **mechanical only**: it is produced by flipping the `NULL` slot to
`fifa96_match_action_1A` in the table (`:5633`), i.e. `action_wired 36→37` and the
two M2 census assertions `36u → 37u`. It does **not** require the producer; it is
the exact shape the "-UNSUPPORTED / rows-27/29/2C precedent" refuses (a ported body
whose entry is unresolved).

---

## 6. M2 / docs impact (c)

**M2 tape (`tests/golden/engine/m2-frames.txt`).** A **slot flip alone does NOT
move the tape**: no live engine path installs row `0x1A` (the ladder + pre-pass are
unported, `:5634`), so the null-backend frame stream is unchanged — the row is
tape-dormant. **The M2 re-pin expectation, if the slot were flipped, is the two
census asserts only** (`:3668`, `:4043`: `36u → 37u`, comment `59/80 → 60/80`), not
the tape. *However*, if the ladder were **also** ported live (install + dispatch on
the tape), the **half-ported body would diverge** (missing stages 1-4 + both emits)
wherever the ladder fires, and the tape **would** move — that is a written-reason +
frame-diff re-pin (AGENTS.md §"goldens"), never a silent upgrade.

**Four-doc update list (if the count ever changes):**

| doc | line (fresh) | change |
|---|---|---|
| `README.md` | `:1` (`# FIFA 96 (DOS, 1995) — Reverse Engineering (73%)`) | title `73%` → `75%` |
| `docs/ENGINE.md` | `:8` (`59/80 dispatch rows wired (36 action + …)`) and the census narrative (`:268`, `:4342`, `:1504/1530/1612/1665`) | `59/80` → `60/80`, `36 action` → `37 action` |
| `docs/PROGRESS.md` | status narrative | reflect the (hypothetical) row-`0x1A` closure |
| `AGENTS.md` | `:90` (`"N/M action rows wired" … (currently 59/80 → 73%)`) | `59/80 → 73%` → `60/80 → 75%` |

Plus the M2 census riders in `tests/test_engine_m2.c:3668/4043`.

**Recommendation:** these four docs **stay unchanged** — the census is *not* being
moved (NO-GO, §4).

---

## 7. Numbered open legs (with native owners)

* **FU297-L1 — the installer ladder + pre-pass.** `FUN_00077EAC` cases (fresh
  `0x7821d/0x78245/0x7826d` → row `0x1A`; `0x78294` → `0x1B`; `0x782b1` → `0x1C`)
  ← `FUN_000782D0` (`0x78599`, gated ~phase-2 + `[0x157C5D]`) ← `FUN_0008D8EC`
  (`0x8DB35`) ← `FUN_0004B100` (`0x4B2AB/0x4B2B2`) + `0x74429/0x74430`. Unported.
  Owner: FU186-L5 / FU186-L6 / FU186-L7.
* **FU297-L2 — the row-`0x1A` body remainder.** stage table `0x76618`, the
  `[0x157C2C]` direction machine + movement/event block, and **both emits**
  (sit-7 `0x76A90` BX=0; sit-5 `0x76B1C` BX=1). Unported. Owner: FU186-L1 /
  FU284-L1/L2/L3.
* **FU297-L3 — the sit-5 emit route.** `CALL 0x8A938` (EAX=5, EBX=1, gated
  `[0x157A49]>>0x18 != 1`, `[0x14C32A]`, `[0x15B6C0]`) → the situation-5 queue
  id-7 → the unported **FU-146** screen machinery; never the engine's live direct
  phase route. Native owner: `0x8A938` (FU-146). A fidelity divergence (FU269-L5).
* **FU297-L4 — the claim-bridge dead-end.** The carried sit-5 reach terminates at
  `FUN_0006E598` (the R11-L1 anim bridge) `0x76863`. Owner: FU163 R11-L1 /
  FU284-L5.
* **FU297-L5 — census is a slot-count, not a route-count.** The census counts
  `fn != NULL` (the M2 pins `tests/test_engine_m2.c:3665/3667,4040/4042`), so a wired
  half-ported body would inflate the count without executing a faithful route.
  This is the rationale the `:5634` comment already records (rows-27/29/2C). Not a
  defect, but a policy leg the census re-pin must respect.

## 8. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | fresh evidence |
|---|---|
| table `0x1106E0[0x1A]=0x7662C` (`[0x1B]=0x76D28`,`[0x1C]=0x77728`,`[0x1D]=0x74EB0`) | `read_memory 0x110748` (16 B) |
| row `0x1A` sole ref = the action table; table sole reader `FUN_0007D9A4` | `get_xrefs_to 0x7662C` = 1 (`0x110748`); `get_xrefs_to 0x1106E0` = 2 (`0x7DA77`/`0x7DA7C`) |
| `FUN_0007D9A4` installs row into `+0x91`, loads `0x1106E0[row]` into `+0x18`, calls it | `disassemble_bytes 0x7DA57` |
| stage table `0x76618 = {0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}` | `disassemble_bytes 0x76618` (data) |
| sit-5 emit `0x76B1C` (EAX=5, EBX=1, EDX=side, gate `[0x157A49]>>0x18!=1`, CALL `0x8A938`) | `disassemble_bytes 0x76AF0` (`e8173e0100`) |
| `0x8A938` gates `[0x14C32A]`/`[0x15B6C0]`, lane table `0x8A8E0` | `disassemble_bytes 0x8A938` |
| ladder `FUN_00077EAC` cases 1/2/3 → row `0x1A` + `[0x157C2C]`; table `0x77E98` | `disassemble_bytes 0x78200`; `read_memory 0x77E98` (20 B) |
| ladder sole caller `FUN_000782D0 0x78599` (field gate `[EBP+0x8e]>>0x18==2`, `[0x157C5D]`); its caller `FUN_0008D8EC 0x8DB35` | `get_xrefs_to 0x77EAC`=1; `disassemble_bytes 0x78580`, `0x8DB20` |
| pre-pass `FUN_0008D8EC` callers = `FUN_0004B100` + `0x74429/0x74430` | `get_xrefs_to 0x8D8EC` = 4 |
| engine body = head+stage-0 only, no emits, `return FIFA96_OK` | `fifa96_match_handlers.c:4017-4056` (read-only) |
| engine row `0x1A` slot NULL | `fifa96_match_handlers.c:5633` (read-only) |
| census pins `action_wired == 36u && phase_wired == 23u` | `tests/test_engine_m2.c:3668`, `:4043` (read-only) |
| table sizes action `45` + phase `35` = `80` | `include/fifa96_engine/fifa96_match_handlers.h:34/35` (read-only) |
| README `73%`, ENGINE `59/80`, AGENTS `:90` `(currently 59/80 → 73%)` | `README.md:1`; `docs/ENGINE.md:8`; `AGENTS.md:90` (read-only) |
| sit-5 BX=1 = queue id-7 → FU-146, never phase 9; claim-bridge dead-end | *carried* FU272 §1.3-1.4 / FU269-L5 / FU273 §3.2 |

**Carried docs (read, not rewritten):** FU284 §1/§2.3/§4; FU289 (the LIVE-LEG
ruling pattern); FU273 §1.4/§3.2; FU272 §1.3-1.4.

## 9. Tooling caveat

* **`get_xrefs_to 0x782D0` returned `0x78599`** (which the disassembly shows is the
  `CALL 0x77EAC` site), inconsistent with the true caller `FUN_0008D8EC 0x8DB35`
  (`disassemble_bytes 0x8DB20`). Disassembly is authoritative here; the ladder
  chain is asserted from the bytes, not the ref-count. Flagged for the MCP bridge.
