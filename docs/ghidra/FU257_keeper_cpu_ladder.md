# FU-257 — the keeper CPU ladder (`FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC`) → action `0x1A`

Provenance: M2 phase-25 W25-2 recon (read-only), 2026-10-14. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument;
`get_current_program_info` = `FIFA96.EXE`, watcom LE 32, image base 0, one overlay;
plain-hex reads resolve to the default physical space). Every address/byte below is a
fresh `read_memory` / `disassemble_bytes` / `get_xrefs_to` / `get_function_by_address`
of this session unless marked *cross-cited* (FU-186, named). Engine/loader source read
first-hand. **No writes**: no rename, comment, label, function, script or project save;
no git. Alias traps applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`; here
`[rec+0x8E]>>24` = the type byte at `+0x91`).

Scope: the per-frame keeper pre-pass (`FUN_0008D8EC`), the record-0 machine
(`FUN_000782D0`) and the CPU decision ladder (`FUN_00077EAC`) whose **only** `0x1A`
installers are the three arms `0x78227`/`0x7824F`/`0x78277`; the `[0x157C2C]` direction
stamp; the record gates (`+0x89`/`+0x92`); and the **honest reach** from the ported
keeper chain (rows `0x1F`/`0x1C`) — hence the census ruling for action row `0x1A`.

Result in one line: the ladder's **only** entry is `FUN_000782D0` (`0x78599`), whose
**only** entry is `FUN_0008D8EC` (`0x8DB35`); the engine ports `FUN_0008D8EC`'s *body*
(as `fifa96_match_entities_team_update`) but **not** `FUN_000782D0`'s CPU-decision tail
(`0x7857B → 0x78599`) nor `FUN_00077EAC` — so the `0x1A` install is reached by **no**
port; row `0x1A` stays **NULL/uncounted** (rows-27/29/2C precedent).

---

## 1. The pre-pass — `FUN_0008D8EC` → `FUN_000782D0`

### 1.1 `FUN_0008D8EC` — the per-frame/team driver (`0x8D8EC..0x8DB6A`)

Fresh `get_function_by_address 0x8D8EC` → body `0008d8ec..0008db6a`; fresh
`get_xrefs_to 0x8D8EC` = **4**: `0x4B2AB`/`0x4B2B2` (in `FUN_0004B100`) and
`0x74429`/`0x74430` (an **unnamed** routine — fresh `get_function_by_address 0x74429`
= *no function*). Fresh `disassemble_function 0x8D8EC` (179 instructions):

```
0x8D8F7  INC byte[EBP+0x82C]                      ; per-team walk counter
0x8D90D  CMP EAX,0xB; JL ...; else byte[+0x82C]=0 ; rollover 0..0xA
0x8D919  EAX=[0x157A4A]>>24 (phase); [0x1586D0]=0 ; phase read, mailbox clear
0x8D929  CMP EAX,0x2; JNZ 0x8D9BD                 ; the phase-2 nearest/intercept select
...      (0x8D9AA CALL 0x8DE8C; 0x8D9B7 store [EBP+0x7B2])
0x8DAF3  team timer decay (word[+0x820] -= delta)
0x8DB2E  MOV EAX,EBP
0x8DB30  MOV EBX,0x1
0x8DB35  CALL 0x782D0                             ; -> the record-0 machine (EAX=rec, EBX=1)
0x8DB3A  LEA EDX,[EBP+0xB2]                       ; then the outfield walk
0x8DB42  CMP byte[EDX+0x9A],0; JNZ skip
0x8DB4D  CALL 0x7CA54                             ; records 1..10, stride 0xB2
0x8DB59  CMP EAX,0xB; JL 0x8DB42
```

So `0x8DB35 CALL 0x782D0` runs once per team before the `FUN_0007CA54` record walk.
It is the **only** caller of `FUN_000782D0` (fresh §4 xref).

### 1.2 `FUN_000782D0` — the record-0 machine (`0x782D0..0x785D0`)

Fresh `get_function_by_address 0x782D0` → body `000782d0..000785d0`; fresh
`get_xrefs_to 0x782D0` = **1** (`0x8DB35`, in `FUN_0008D8EC`). Fresh
`disassemble_function 0x782D0` (224 instructions). The prologue gates:

```
0x782D8  CMP byte[0x157823],0; JZ 0x78336   ; latch gate
0x782E1  CMP EBP,[0x157A83];   JNZ 0x78336  ; must be the controlled record
0x782E9  EAX=[0x157A4A]>>24; CMP 0x5; JZ 0x78336 ; phase != 5
0x7830C  CALL 0x741B4(side); INC word[EAX*2+0x157AD0]
0x78329  CALL 0x741B4(side^1); INC word[EAX*2+0x157ACC]
```

then the input-row table dispatch `0x783A8..0x784D1` (selects `0x110970`/`0x110978`
etc. by `[0x157750]`/`[rec+0x5D]`; `0x7845D CALL dword[EDX+4]` = `FUN_00076130`,
the *computed* keeper input handler), then the **action install + CPU-decision gate**:

```
0x784EF  DX=word[EBP+0x81]; if DX!=0 -> EAX=0 (skip install)
0x78507  phase==2 && EBP==[team+0x7B2] && ... : select EAX=4 (0x7854C) else 0x19 (0x78553)
0x78558  DX=(int8)[EBP+0x91]; if AX != DX:
0x7856C    byte[0x157AB2]=1; 0x78576 CALL 0x7D9A4(EAX, rec, ECX=0, EBX=0) ; install 4 / 0x19
0x7857B  phase==2 && [EBP+0x20]==0 && byte[0x157C5D]!=0:
0x78599    CALL 0x77EAC                                               ; -> the CPU ladder
0x7859E  byte[0x157C5D]=0                                             ; latch clear
0x785A8  CALL dword[EBP+0x18]                                         ; staged handler
0x785AD  CALL 0x6E8E8; if EBP==[0x157AA7] copy pos(+0x59)->target(+0x4D)
0x785C5  CALL 0x7BF20
```

The record-0 machine therefore installs the record's *own* action (`4`/`0x19`) and,
**only** when `phase==2 && [rec+0x20]==0 && [0x157C5D]!=0`, calls the CPU ladder
`FUN_00077EAC` (`0x78599`). That call is the ladder's **sole** entry (fresh §4 xref).

---

## 2. The ladder — `FUN_00077EAC` (`0x77EAC..0x782CD`)

Fresh `get_function_by_address 0x77EAC` → body `00077eac..000782cd`; fresh
`get_xrefs_to 0x77EAC` = **1** (`0x78599`, in `FUN_000782D0`). Fresh
`disassemble_bytes 0x77EAC` (200 B) — the gate head:

```
0x77EB5  EBP = rec
0x77EB7  EAX = [rec+0x8E]>>24  = type byte[rec+0x91]; CMP 0x19; JNZ 0x782C4   ; type 0x19 only
0x77ECB  AL = team[rec[0]+0x826]; CALL 0x741B4(side)
0x77EDE  AX = word[0x14C1D4 + side*2]; AND AH,0x2; JNZ 0x782C4                ; per-side gate word
0x77EF6  CMP byte[rec+0x9A],0; JNZ 0x782C4                                    ; must be inactive
0x77F03  EAX=[rec+0x28]; AL=byte[EAX]; CMP 0x54; JZ 0x782C4                   ; row byte != 0x54
0x77F16  word[rec+0x81] != 0 ? copy pos(+0x59)->target(+0x4D), exit : fall
```

The selector body (`0x77F16..0x781E1`, fresh `0x77F50`/`0x7816A`) builds a lateral
`ECX` ∈ {1,4,5,EDX} from the camera triple `0x15774C`, the in-box/lane buckets
(`0x10E1CE` table at `0x78193`; `0x157C4C` at `0x781C1`), then:

```
0x781E1  TEST CX,CX; JZ 0x782C4                ; ECX==0 -> no install
0x781EC  CALL 0x765C4 (EAX=EBP)                ; FUN_000765C4 tracked-teammate predicate
0x781F1  TEST AX,AX; JZ 0x78205
0x781F6  ECX=1; EDX=4; JMP 0x782BB             ; -> install 0x1C (via 0x782BF)
0x78205  DEC ECX; CMP CX,4; JA 0x782C4
0x78215  JMP dword CS:[EAX*4 + 0x77E98]        ; the 5-arm ladder table
```

Fresh `read_memory 0x77E98` (24 B) = `6d820700 45820700 1d820700 94820700 b1820700`
= **`{0x7826D, 0x78245, 0x7821D, 0x78294, 0x782B1}`**. Fresh `disassemble_bytes
0x78200` (260 B) — the arms (each `ECX=1` invoke-now, `EBX=0`, `EAX=EBP`,
`CALL 0x7D9A4`):

| `ECX` | table index | arm site | `[0x157C2C]` stamp | install `EDX` | call |
|---|---|---|---|---|---|
| 1 | [0] | `0x7826D` | `MOV EAX,0x2C; [0x157C2C]=EAX` (`0x7827E`) | **`0x1A`** (`0x78277`) | `0x78285` |
| 2 | [1] | `0x78245` | `MOV EDI,0x2D; [0x157C2C]=EDI` (`0x78258`) | **`0x1A`** (`0x7824F`) | `0x7825E` |
| 3 | [2] | `0x7821D` | `MOV ESI,0x2E; [0x157C2C]=ESI` (`0x78230`) | **`0x1A`** (`0x78227`) | `0x78236` |
| 4 | [3] | `0x78294` | — | `0x1B` (`0x78299`) | `0x782A2` |
| 5 | [4] | `0x782B1` | — | `0x1C` (`0x782B6`) | `0x782BF` |
| (pred) | `0x782BB` | from `0x78200` (`0x765C4`≠0) | — | `0x1C` | `0x782BF` |

Every arm's call is `CALL 0x7D9A4` (`e8`-relative to `0x78236`/`0x7825E`/`0x78285`/
`0x782A2`/`0x782BF`; all in the fresh `get_xrefs_to 0x7D9A4` = **77** list). The
ladder's **install set is `{0x1A ×3, 0x1B, 0x1C}`**, all invoke-now, and it stamps
`[0x157C2C] ∈ {0x2C,0x2D,0x2E}` immediately before each `0x1A` install — the same
global the row-`0x1A` stage bodies read back (§3). The **only** `0x1A` immediates in
the image that feed `CALL 0x7D9A4` are `0x78227`/`0x7824F`/`0x78277` (cross-cited
FU-186 §3; the FU-182 §5 “installers” `0x773DC`/`0x70DAF`/`0x80814`/`0x9E665` are
mailbox/anim arguments and a constant getter, not installers).

---

## 3. The gates — record cells `+0x89`/`+0x92` and the ladder's inputs

Fresh `disassemble_bytes 0x7662C` (240 B) — the row-`0x1A` body that the ladder's
`0x1A` install lands on:

```
0x76634  EAX=[0x157A4A]>>24; CMP 0xD; JNZ 0x76662
0x76641    team[rec[0]+0x826] == [0x157AAC]>>24 -> MOV [0x157A83]=EBP   ; side bind
0x76662  EDX=dword[EBP+0x89]; AX=word[0x157A64]; ADD EDX,EAX; [+0x89]=EDX ; +0x89 += delta
0x76672  AL=byte[EBP+0x92]
0x7667E  CMP AL,0x4; JA 0x76B21                                          ; +0x92 > 4 -> exit
0x7668B  JMP dword CS:[EAX*4 + 0x76618]                                  ; 5-arm stage table
0x76693  (stage 0) CALL 0x765C4; TEST AX; JZ 0x766BE
0x7669F    ECX=1; EDX=4; 0x766AB CALL 0x7DAB4; 0x766B4 CALL 0x7D9A4      ; install 4 invoke-now
0x766BE    byte[+0x9E]=1; [+0x89]=0; AH=byte[+0x92]; AH++; [+0x92]=AH    ; +0x92 += 1
0x766DD    DL=byte[+0x92]; [+0x89]=0; DL++; [+0x92]=DL                  ; +0x92 += 1 (stage 1 entry)
0x766F5    CALL 0x79B1C                                                  ; pos -> target snap
0x766FC    CMP [0x157C2C],0x2D ...                                       ; direction machine (FU186-L1)
```

Producers/consumers (fresh):

- **`+0x89`** is a **dword** timer, incremented by the body head (`0x76662`) and zeroed
  by the stage-0/lane arms (`0x766BE`/`0x766DD`). It is the row's own frame timer.
- **`+0x92`** is the **stage byte**, table-dispatched (`0x7668B`, table `0x76618` =
  `{0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}`); the zero arm advances it by **2**
  (two `INC`s: `0x766D5` at 0x766BE and `0x766ED` at 0x766DD), matching
  `fifa96_keeper_reposition_a_gate`'s `stage_advance = 2` (fresh `fifa96_keeper.c:246`).
- **Ladder record gates** (all exit before any install): `type == 0x19`
  (`byte[rec+0x91]`, `0x77EC2`); `word[0x14C1D4 + side*2] & 0x200 == 0` (`0x77EDE`);
  `byte[rec+0x9A] == 0` (`0x77EF6`); `byte[[rec+0x28]] != 0x54` (`0x77F03`);
  `word[rec+0x81] == 0` (`0x77F16`).
- **The record-0 machine's CPU-decision gate**: `phase==2 && [rec+0x20]==0 &&
  byte[0x157C5D]!=0` (`0x7857B..0x78595`). `[0x157C5D]`'s **producer** is the row-`0x19`
  keeper-hold prologue (`0x74707`; engine `mr->global_157c5d = 1`, fresh
  `fifa96_match_handlers.c:3971`); its **consumer** is exactly this unported gate.
- Fresh `get_xrefs_to 0x765C4` = **2**: `0x781EC` (the ladder) and `0x76695` (row-`0x1A`
  stage 0). The predicate body (`0x765C4..0x76615`, fresh `disassemble_bytes`) reads
  the tracked record `[0x1577CA]`, compares team bytes `[+0x826]`, then
  `byte[rec+0x8E]>>24 == 5` → 1, else `[0x158740]>>24 != 4` → 1, else 0.

---

## 4. The reaches — is the ladder live from the ported keeper chain?

Fresh `get_xrefs_to` this session (the whole chain, each link a single edge):

| target | fresh xref count | sources |
|---|---|---|
| `FUN_00077EAC` | **1** | `0x78599` (`FUN_000782D0`) |
| `FUN_000782D0` | **1** | `0x8DB35` (`FUN_0008D8EC`) |
| `FUN_0008D8EC` | **4** | `0x4B2AB`/`0x4B2B2` (`FUN_0004B100`), `0x74429`/`0x74430` (unnamed `0x743xx`) |
| `FUN_0004B100` | **3** | `0x4951E` (`FUN_000493A0`), `0x49FBB` (`FUN_00049B28`), `0x4A25A` (`FUN_0004A228`) |
| `FUN_000765C4` | **2** | `0x781EC` (ladder), `0x76695` (row `0x1A`) |

**The ported-engine coverage** (first-hand source):

- `fifa96_match_entities_team_update` (`fifa96_match_entities.c:659`) ports the *body* of
  `FUN_0008D8EC`: the update counter (`:670`, native `0x8D8F7`), the phase-2
  target/intercept select (`:672`, `0x8D929`), the team timer decay (`:681`, `0x8DAF3`),
  and the record walk (`:696`), with the record-0 special case reduced to **one line** —
  `if (i == 0) e->timer7b = e->timer79; /* 0x782D0 entry */` (`:698`). The
  outfield walk `0x8DB42` maps to the `FUN_0007CA54` seam (`fifa96_match_run.c:671`).
- **The `FUN_000782D0` tail is not reproduced**: the prologue gates (`0x782D8`/`0x782E1`/
  `0x782E9`), the record-0 action install (`0x78576`), the input-row table dispatch
  (`0x7845D` → `FUN_00076130`), the `0x7857B` phase-2/no-slot/`[0x157C5D]` gate, the
  **`0x78599 CALL 0x77EAC`**, the `0x785A8 CALL [rec+0x18]`, `0x6E8E8`, `0x7BF20`.
- **`FUN_00077EAC` is unported** entirely.
- The `[0x157C5D]` latch is **write-only** in the engine: `mr->global_157c5d` is set by
  row `0x19` (`:3971`) and cleared at match init (`fifa96_match_run.c:2046`/`:2167`), but
  the header itself labels it *“(row-0x19 CPU-keeper latch, **write-only**)”*
  (`fifa96_match_run.h:672`). Its only consumer in the image is the unported `0x7857B`
  gate.

**The ported keeper chain never emits `0x1A`.** Row `0x1F`'s wired stage-3 tail drains
`fifa96_keeper_input_decide` = `FUN_00076130`, whose install set is
`{4, 0x19, 0x1B, 0x1C}` (`0x762A4`, `0x7634B`; cross-cited FU-186 §1.4, fresh
`get_xrefs_to 0x76130` this session matches) — **no `0x1A`**. Row `0x1C`'s wired
installer is the same `FUN_00076130` `0x76291`→`0x762A4` route. So the ladder is
reached **only** behind the unported `FUN_000782D0` tail on the `FUN_0008D8EC` pre-pass
whose callers (`FUN_0004B100` + the unnamed `0x743xx`) are not the keeper chain.

**Answer:** the ladder is **not** reached live from the ported keeper chain; it sits
behind an unported machine (`FUN_000782D0`'s CPU-decision tail + `FUN_00077EAC`).

---

## 5. Verdict + census ruling

**Port shape.** `fifa96_match_action_1A` (`fifa96_match_handlers.c:3891`) is the bounded
head + stage-0 slice of `0x7662C..` (fresh `0x7662C` confirms it: the phase-`0xD` +
`[0x157AAC]>>24` side bind, `+0x89 += delta`, the `+0x92 > 4` tail, the
`fifa96_keeper_reposition_a_gate` install-4 arm, and the `+0x9E=1`/`+0x89=0`/`+0x92+=2`
arm + `0x79B1C` snap). It is exported and directly tested but its action-table slot is
**NULL** (`fifa96_match_handlers.c:5506`; `fifa96_match_handlers.h:204..217`). The body
is honest; the **entry is unresolved**.

**Honest port-ability.** The ladder is port-able *in principle* — every input is
traceable (the `[0x14C1D4+side*2]` gate word, the `ECX` selector, the `[0x157C2C]`
stamps) — but it is **not** a small increment: it requires the whole `FUN_000782D0`
record-0 tail (prologue gates, the input-row table, the `0x7857B` gate) *and*
`FUN_00077EAC` *and* the consumer wiring of `[0x157C5D]`. That is a standalone slice, not
a co-scoped body port. No fake producer is introduced.

**Census ruling for action row `0x1A`:**
- The criterion is **install AND dispatch**. The engine dispatches row `0x1A` only if the
  slot is set, and it is NULL. Its **only** installer is `FUN_00077EAC`'s three arms
  (`0x78227`/`0x7824F`/`0x78277`); fresh `get_xrefs_to 0x77EAC` = **1** (into the record-0
  machine) and `get_xrefs_to 0x782D0` = **1** (into the pre-pass), neither of which the
  engine ports to the `0x78599` call. The ported keeper chain (rows `0x1F`/`0x1C`) emits
  `{4,0x19,0x1B,0x1C}`, never `0x1A`.
- **Ruling: row `0x1A` stays `NULL` / UNCOUNTED** — the rows-27/29/2C precedent (a ported
  body with no live engine installer). **Census unchanged `58/80 = 72%`; README title
  stays 72%.** Counting it would require a *fake* installer, which is forbidden.
- The instant the ladder lands (owner below), row `0x1A` becomes countable **iff** the
  ported ladder installs `0x1A` and then dispatches it, and the install is proved
  tape-neutral (M2 byte-identical) or an earned re-pin.

### Numbered legs (owners)

1. **FU257-L1 — the CPU ladder `FUN_00077EAC`** (`0x77EAC..0x782CD`): the `ECX` selector
   body (`0x77F16..0x781E1`) and the `ECX → arm` mapping. **The `0x1A` installer; row
   `0x1A` cannot count without it.** Carries FU186-L5 / OL-36. Owner: keeper-ladder slice.
2. **FU257-L2 — `FUN_000782D0` record-0 tail** (`0x782D0..` incl. the prologue gates, the
   input-row table `0x110950..` + `CALL [EDX+4]` `0x7845D`, the `0x78576` install, the
   `0x7857B` gate → `0x78599 CALL 0x77EAC`, `0x785A8 CALL [rec+0x18]`, `0x785AD CALL
   0x6E8E8`, `0x785C5 CALL 0x7BF20`). Carries FU186-L6. Owner: keeper-ladder slice.
3. **FU257-L3 — `FUN_0008D8EC` driver pre-pass** (`0x8D8EC..0x8DB6A`): body ported as
   `fifa96_match_entities_team_update`; the callers (`FUN_0004B100` `0x4B2AB`/`0x4B2B2`;
   unnamed `0x743xx` `0x74429`/`0x74430`) not traced. Carries FU186-L7. Owner: frame-chain
   slice.
4. **FU257-L4 — `FUN_000765C4`** tracked-teammate predicate (`0x765C4..0x76615`). Carries
   FU186-L4. Owner: keeper-chain slice.
5. **FU257-L5 — the `[0x157C2C]` direction machine + movement/event block** and stages
   1-4 (`0x766FC..0x76AE8`). Carries FU186-L1 / OL-34. Owner: keeper-chain slice.
6. **FU257-L6 — the `[0x157C5D]` consumer wiring**: the engine latch is write-only; the
   pending consumer is the `0x7857B` gate (FU257-L2). Carries a new sub-leg. Owner:
   keeper-ladder slice.
7. **FU257-L7 — the tape question**: whether a ported ladder installs `0x1A` on M2
   (`[0x157C5D]` set by the row-`0x19` body during a live keeper-hold frame) → determines
   byte-identical vs an earned re-pin. Owner: the ladder slice that lands FU257-L1/L2.
8. **FU257-L8 — `FUN_0009E420`** constant-code getter (`0x9E665` returns `EAX=0x1A`) —
   ruled out as an installer. Carries FU186-L9. Owner: none (closeable).

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `get_function_by_address` `0x8D8EC`, `0x782D0`, `0x77EAC`,
`0x4B100`, `0x74429` (none), `0x743F0` (none); `get_xrefs_to` `0x8D8EC` (4), `0x77EAC` (1),
`0x782D0` (1), `0x4B100` (3), `0x493A0` (3), `0x765C4` (2), `0x7D9A4` (77), `0x741B4`
(>100, truncated); `disassemble_function` `0x8D8EC` (179 insn), `0x782D0` (224 insn);
`disassemble_bytes` `0x78200` (260 B), `0x7662C` (240 B), `0x77EAC` (200 B), `0x77F50`
(230 B), `0x7816A` (160 B), `0x765C4` (90 B); `read_memory` `0x77E98` (24 B), `0x76618`
(cross-cited FU-186). Engine/loader source read first-hand:
`fifa96_match_handlers.c` (`3782..3930`, `5504..5512`), `fifa96_match_entities.c`
(`659..708`), `fifa96_match_entities.h` (`20..37`), `fifa96_match_run.c`
(`621..671`, `4092..4209`, `2042..2168`), `fifa96_match_run.h` (`664..684`),
`include/fifa96_engine/fifa96_match_handlers.h` (`204..217`), `fifa96_keeper.c`
(`235..263`). Docs cross-cited: FU-186, FU-179, FU-180, FU-182. No writes.
