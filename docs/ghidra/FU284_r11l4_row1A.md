# FU284 — R11-L4: the row-`0x1A` body remainder + the sit-5 emit (port shape)

**Phase/Provenance:** M2 phase-26c recon-ahead, Task 4 (bounded, read-only derivation); draft awaiting controller promotion.

Read-only Ghidra session, authoritative program **`/FIFA96.EXE`** (case-sensitive;
confirmed this session: `watcom:LE:32:default`, image base 0, `.image` overlay,
2706 functions). Every native address/byte below is **fresh** this session
(`read_memory` / `disassemble_bytes` / `get_xrefs_to`) unless marked *carried*
(the frozen `docs/ghidra/FU*` slice is named). Alias traps applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`); CALL rel32 verified.
No rename / comment / label / function / script / project-save; `/FIFA96.EXE`
untouched; ISO never touched; no git command. Engine facts cite the built tree by
line (read-only). Carried input: `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md`
§2.1/§2.2/§2.5, `docs/ghidra/FU272_r11l4_derivation.md` §2.2.

---

## 1. The row entry (reconcile)

Fresh `read_memory 0x110748` (16 B) = `2c 66 07 00 | 28 6d 07 00 | 28 77 07 00 |
b0 4e 07 00` → action table `0x1106E0[0x1A] = 0x7662C`, `[0x1B] = 0x76D28`,
`[0x1C] = 0x77728`, `[0x1D] = 0x74EB0`. This reconciles FU273 §2.1 and FU272 §2.1
exactly. Fresh `get_xrefs_to 0x7662C` = **1** (`0x110748` `DATA`) — the action
table is the row's **sole** reference; the entry is dispatched through the table,
never called directly.

---

## 2. The row-`0x1A` body (port shape)

Fresh `disassemble_bytes 0x7662C` (208 B) / `0x766FC` (400 B) / `0x7688B` (620 B)
/ `0x76AF0` (32 B) / `0x76B00` (64 B); fresh `read_memory 0x76618` (20 B).

### 2.1 Head + stage dispatch (the ported slice)

```
0x76632  MOV EBP,EAX
0x76634  EAX=[0x157A4A]>>0x18 ; CMP EAX,0xd ; JNZ 0x76662     ; phase == 0xD
0x76641  EAX=[EBP]; AL=[EAX+0x826]; EDX=[0x157AAC]>>0x18
0x76658  CMP EDX,EAX ; JNZ 0x76662
0x7665c  MOV dword[0x157A83],EBP                              ; actor := rec (side==controlled)
0x76662  EDX=[EBP+0x89]; AX=word[0x157A64]; EDX+=EAX          ; timer89 += [0x157A64]
0x76672  AL=byte[EBP+0x92]; MOV [EBP+0x89],EDX
0x7667e  CMP AL,0x4 ; JA 0x76b21                              ; stage92 > 4 -> exit
0x76686  AND EAX,0xff ; JMP dword CS:[EAX*4 + 0x76618]        ; <-- stage table
```

The stage table `0x76618` (fresh `read_memory`, 5 dwords) is
`{0x76693, 0x766DD, 0x766F5, 0x76AAD, 0x76AE8}` — **stages 0..4**:

| stage | arm | what |
|---|---|---|
| 0 | `0x76693` | `FUN_000765C4` predicate; if nonzero → `0x7DAB4` reset + install **code 4** (`0x7D9A4`) then exit `0x76B21`; else `+0x9E=1`, `timer89=0`, `stage92 += 2`, snap `0x79B1C`, fall into the direction machine |
| 1 | `0x766DD` | `timer89=0`, `stage92 += 1`, snap `0x79B1C`, fall into the direction machine |
| 2 | `0x766F5` | snap `0x79B1C` only, fall into the direction machine |
| 3 | `0x76AAD` | `CMP byte[EBP+0x44],0; JZ 0x76AD2` — the `+0x44` test → actor-copy or `stage92 += 1` + RET |
| 4 | `0x76AE8` | snap `0x79B1C` + `0x7DAB4` reset, then the **sit-5 emit** (`0x76B1C`) |

### 2.2 The `[0x157C2C]` direction machine + movement/event block (`0x766FC..0x76AE7`, FU186-L1)

The fall-through from stages 0/1/2 at `0x766FC` is a **state dispatch on the
`[0x157C2C]` cell** (the code the FU273 §1.4 ladder arms write: case-1 `0x2E`,
case-2 `0x2D`, case-3 `0x2C`):

* **`==0x2D`** (`0x766FC`): gates `word[0x1577EE] < 0x60`, `word[0x1577BC] > 0x12`,
  `word[0x1577F8] < 0x1E`, `|[EBP+0x61]| < 0xA90`; on pass
  `[0x157C2C] := 0x5C` + `0x651F0(6)`.
* **`==0x2E`** (`0x76757`): gates `word[0x1577BC] > 0x14`, `word[0x1577F8] < 0x1C`,
  `[0x157750] < 0x90` + a ball-code compare (`0x92AC8`); the shoot pass sets
  `[0x157C2C] := 0x54` + `0x651F0(6)`; else the camera-Y clamp block
  (`[0x157750]-0x70`, clamp `0x60`, `0x702F8`, `+0x87=0`, `+0x85=AX`).
* **`==0x54`** (`0x767E7`): gate `word[EBP+0x69] > 0x40`; if `[EBP+0x89] <= 0xF`
  exit, else `0x7DAB4` reset.
* **else** (`0x76816`): the **movement block** — `0x79C50(rec, -[0x1577C0],
  -[0x1577C2])` place/heading; `+0x6D`/`+0x6B` push → `0xCD474`; `[EBP+0x7F]=AX`;
  `ECX=[EBP+0x8B]>>0x18`; `0x6E598` (**the R11-L1 claim-anim bridge**, `0x76863`);
  then the `[EBP+0x28] == 0x54` (`'T'`) branch: `'T'` → `0x92AC8` + `0x8DC68` +
  the `[0x15774C]`/`[0x157754]` camera add + `0x7DAB4` reset; else → the event block.

The **event block** (`0x7690C..0x76A3C`) folds `[0x157750]`/`[0x1577BC]`/phase/
`[0x157C2C]`/`[0x1577EE]` into a situation/act id (`5 / 4 / 9 / 1 / 0xA / 7 / 0`),
calls `0x8ED40` on some arms, then `0x76A3E CALL 0x8F188(id, 0x20, EBP)` — a
distinct act/event sink. It then selects the anim code (`0xF/1/0xE`) via
`0x651F0` and reaches the **sit-7 emit**.

### 2.3 The two emits (fresh; the port contract)

Both are `CALL 0x8A938` (the FU272 §1.1 situation dispatcher) with `EDX` = the
record side byte (`[EBP]` → `+0x826`, `AND 0xff`) and a queue flag in `EBX`:

**sit-7 — the stage-path tail (`0x76A90`, BX=0):**
```
0x76A83  MOV EAX,0x7
0x76A88  AND EDX,0xff
0x76A8E  XOR EBX,EBX
0x76A90  CALL 0x8A938          ; e8 a3 3e 01 00 -> 0x76A95 + 0x13EA3 = 0x8A938
```
then `timer89=0`, `stage92 += 1` (`0x76A95..0x76AA7`), the `+0x44` test
(`0x76AAD`), and either the actor-copy (`0x76AD2` `MOVSD ×3 [EBP+0x59]→[EBP+0x4D]`
+ `[0x157A83]=EBP`) or `stage92 += 1` + RET. *(Carried cross-ref: FU272 §2.2
quotes the same `0x76A83/0x76A88/0x76A8E/0x76A90`; fresh-matched here.)*

**sit-5 — the stage-4 tail (`0x76B1C`, BX=1; the FU273 §2.2 emit):**
```
0x76AF6  EAX=[0x157A49]>>0x18 ; CMP EAX,0x1 ; JZ 0x76b21     ; gate
0x76B03  MOV EDX,[EBP]
0x76B06  MOV DL,[EDX+0x826]                                  ; EDX = side
0x76B0C  MOV EBX,0x1
0x76B11  MOV EAX,0x5
0x76B16  AND EDX,0xff
0x76B1C  CALL 0x8A938          ; e8 17 3e 01 00 -> 0x76B21 + 0x13E17 = 0x8A938
0x76B21  POP EBP; POP EDI; ...; RET                          ; exit
```
i.e. stage-4: snap `0x79B1C` + row reset `0x7DAB4`, then — when
`[0x157A49]>>0x18 != 1` — emit **situation 5 with BX=1** (the queue / act-8
route, FU272 §1.4 / FU269-L5). CALL bytes verified (`e8173e0100`).

---

## 3. The ENGINE delta (fresh tree read)

* **Body** `fifa96_match_action_1A` — `src/fifa96_engine/fifa96_match_handlers.c:3901-3940`.
  Models **only** the head (`:3915-3917` phase-`0xD`+side → `r->controlled=1`),
  `timer89 += delta` (`:3918`), the `stage92 > 4` exit (`:3919`), the **stage-0**
  gate + install-4 + snap (`:3920-3935`), and then a comment
  `/* 0x766FC.. the [0x157C2C] direction machine + movement/event block and stages
  1-4 -> FU186-L1. */` (`:3936-3937`) and `return FIFA96_OK;` (`:3939`). There is
  **no stage switch** (table `0x76618` unmodeled), **no direction machine**, **no
  event block**, and **neither emit** (sit-7 `0x76A90` / sit-5 `0x76B1C`).
* **Action table row** `{0x1A, NULL, "…"}`
  — `fifa96_match_handlers.c:5517` (**fresh**; FU273 §2.5 / FU272 §2.4 cite
  `:5512` — a **+5 line drift**; see erratum E1). Slot **NULL**, uncounted; the
  comment itself names the row's only installer as the unported
  `FUN_00077EAC` ladder (FU186-L5) on the unported pre-pass, the
  `[0x157C2C]` direction machine + stages 1-4 as FU186-L1, and tags `OL-34;
  -UNSUPPORTED`.

**What wiring the row live implies.** Setting the slot to `fifa96_match_action_1A`
is a one-line census change, but it would be a **dead count**: (i) the body is
<½ ported — stages 1-4, the direction machine, the event block and **both emits**
are absent (FU186-L1); (ii) the row's **only installer** is the unported
`FUN_00077EAC` ladder (`0x78227/0x7824F/0x78277`, FU186-L5) on the unported
`FUN_0008D8EC→FUN_000782D0` pre-pass (FU186-L6/L7) — nothing ever installs
`0x1A`, so the handler never runs; (iii) even if it ran, the native sit-5 emit is
**BX=1**, which routes through the **situation-5 queue id-7** to the unported
FU-146 screen machinery — **never phase 9** (FU272 §1.3-1.4); the engine's direct
`fifa96_match_run_situation(mr, 5)` route is a disclosed divergence (FU269-L5);
(iv) the sit-5 reach still terminates at the **R11-L1 claim bridge** dead-end
(FU273 §3.2).

---

## 4. GO / NO-GO

**NO-GO — wiring row `0x1A` live is not honest census upside.** The row is a
half-ported body (`handlers.c:3901-3940`); its only installer (the `FUN_00077EAC`
ladder + pre-pass) is unported; and its sit-5 emit takes the native BX=1 queue
route that never reaches phase 9. Counting it would inflate the wired count with
a row that can neither install nor emit on any live path — exactly the
rows-27/29/2C precedent the `NULL` slot cites (`:5517`, `-UNSUPPORTED`). Keep it
`NULL` until FU186-L1 + FU186-L5/L6/L7 land. This is bounded-slice negative for
the same reason as FU273 §4.3 (the keeper CPU cluster is phase-scale).

---

## 5. Numbered open legs (with native owners)

* **FU284-L1 — the `[0x157C2C]` direction machine + movement/event block**
  (`0x766FC..0x7690B`; the `0x79C50` place, `0xCD474`, `0x92AC8`, `0x8DC68`, the
  `'T'(0x54)` branch, the `0x8ED40`/`0x8F188` sink family). Unported. Owner:
  FU186-L1.
* **FU284-L2 — the stage 1-4 dispatch** (table `0x76618` =
  `{0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}`, fresh) — the engine body has no
  stage switch. Owner: FU186-L1.
* **FU284-L3 — the two emits** (sit-7 `0x76A90` BX=0; sit-5 `0x76B1C` BX=1; both
  `CALL 0x8A938`). Unported; the native BX=1 sit-5 route is the queue id-7 →
  FU-146 screen machinery, **never phase 9** (a fidelity divergence to the
  engine's direct route). Owner: FU186-L1 / FU269-L5.
* **FU284-L4 — the installer** `FUN_00077EAC` ladder cases 1/2/3
  (`0x78227/0x7824F/0x78277`; `[0x157C2C] := 0x2E/0x2D/0x2C`) + the
  `FUN_0008D8EC→FUN_000782D0` pre-pass. Unported. Owner: FU186-L5 / FU186-L6 /
  FU186-L7 (fresh xref census in FU273 §1.1).
* **FU284-L5 — the claim bridge inside the row body** `FUN_0006E598` called at
  `0x76863` (movement block) — the R11-L1 anim bridge; unported. Owner: FU163
  R11-L1.
* **FU284-L6 — the act/event sink identity** `CALL 0x8F188(id, 0x20, EBP)` at
  `0x76A3E` (and the `0x8ED40` family) — named here only descriptively; its
  consumer is unported. Owner: FU186-L1.

## 6. Doc errata candidates

* **E1 — action-table line drift.** FU273 §2.5 and FU272 §2.4 cite
  `fifa96_match_handlers.c:5512/5514/5516` for rows `0x1A`/`0x1B`/`0x1C` and
  FU273 §2.5 cites `:5512` for row `0x1A`. Fresh reads: row `0x1A` = **`:5517`**,
  `0x1B` = **`:5519`**, `0x1C` = **`:5521`** (a consistent **+5** drift). The
  body cite `:3897` is fresh **`:3901`**. No content change; line numbers only.

## 7. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| table `0x1106E0[0x1A]=0x7662C` (`[0x1B]=0x76D28`,`[0x1C]=0x77728`,`[0x1D]=0x74EB0`) | `read_memory 0x110748` (16 B) = `2c660700 286d0700 28770700 b04e0700` |
| row `0x1A` sole xref = the action table | `get_xrefs_to 0x7662C` = 1 (`0x110748` DATA) |
| head: phase-`0xD`+side actor bind; `timer89 += [0x157A64]`; `stage92>4` exit; stage table `0x76618` | `disassemble_bytes 0x7662C` (208 B) |
| stage table `0x76618 = {0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}` | `read_memory 0x76618` (20 B) |
| `[0x157C2C]` direction machine (`0x2D/0x2E/0x54` arms) + movement/event block | `disassemble_bytes 0x766FC` (400 B); `0x7688B` (620 B) |
| sit-7 emit `0x76A90` (EAX=7, BX=0, CALL `0x8A938`) | `disassemble_bytes 0x76A80` (48 B) |
| sit-5 emit `0x76B1C` (EAX=5, BX=1, EDX=side, gated `[0x157A49]>>0x18!=1`, CALL `0x8A938`) | `disassemble_bytes 0x76B00` (64 B), `0x76AF0` (32 B) |
| engine body = head+stage-0 only, FU186-L1 comment, no emits | `fifa96_match_handlers.c:3901-3940` |
| engine row `0x1A` slot NULL/uncounted | `fifa96_match_handlers.c:5517` |
| sit-5 queue id-7 never reaches phase 9; BX=1 native vs engine direct route | *carried* FU272 §1.3-1.4 / FU269-L5 |
| installer = `FUN_00077EAC` ladder, unported | *carried* FU273 §1.4 (fresh xref census §1.1) |

**Carried docs (read, not rewritten):** FU273 §1.1/§1.4/§2.1/§2.2/§2.5/§3.2/§4.1,
FU272 §2.1/§2.2/§2.4.
