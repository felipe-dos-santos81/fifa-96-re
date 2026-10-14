# FU-231 — the `FUN_0008BAF0` end machine (phase-22 Stage-1 recon, wave W22-1)

Provenance: M2 phase-22 Stage-1 recon, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, `memory_size 2757479`, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `disassemble_function` /
`get_function_by_address` / `get_xrefs_to` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named). The alias traps are applied:
`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3` (so the phase
byte `[0x157A4A]>>24` = **`byte[0x157A4D]`**); LE globals sit at `+0x100000`
from the file offset; addresses are `/FIFA96.EXE` runtime addresses. **No
writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (W22-1 brief): map **`FUN_0008BAF0`** `0x8BAF0..0x8C065` (wave-1 slice
W22-1; canonical `docs/ghidra/FU231_end_machine.md`) and its **exact gated
terminal decision**. This extends the predecessor **FU-228** (*the post-period
lifecycle and the end-of-match word*, `docs/ghidra/FU228_post_period_lifecycle.md`)
— read first; it established the phase dispatch `{6,2,0x0C,0x13,0x14}`, the
`0x960`/`0x780` camera sinks, the two `[0x158822]=2` sites and the `0x8BCD9`
setter. FU-231 re-reads the whole body first-hand, decomposes the shared camera
tail, and adds the **third terminal arm** (`[0x105FFC]=5` via `0x37F54`) and the
clock-side flag producers that FU-228 left conditional (**FU228-L6**).

---

## Answer in one line

**`FUN_0008BAF0` is a per-frame **camera/entity controller** on the struct at
`0x1587EC` — a phase dispatch (`byte[0x157A4D] ∈ {6,2,0x0C,0x13,0x14}`) feeding a
shared camera tail — whose *only* match-terminal outputs are the end word
`word[0x158822]` (`=1` at `0x8BCD9` for a **flag-clear regulation end**; `=2` at
`0x8BD8A`/`0x8BDD4` for an **ET-half terminal**) plus a **third arm** FU-228 did
not record: the no-ET screen `[0x105FFC]=5` written by `0x37F54` on the phase-`0xC`
**tie latch** `[0x10F378]!=0 path.** The terminal decision is a small state
machine over `{the ET flag byte[0x157AC0], the tie latch [0x10F378], the ET-over
latch [0x10F360], the end timer word[0x158818], the phase byte}` — so the minimal
port is to **reproduce that decision** (gate the engine's `mark_over` on it), not
to port the camera body.**

---

## 1. The whole body (fresh)

Fresh `get_function_by_address 0x8BAF0` = `FUN_0008baf0`, body
`0x8BAF0..0x8C065`; fresh `disassemble_function 0x8BAF0` = **379** instructions.
Fresh `get_xrefs_to 0x8BAF0` = **1** — a single `UNCONDITIONAL_CALL` from
`0x8B664` inside `FUN_0008AF38` (the per-frame clock), confirming FU-228 §2.3.

The body is a **dispatcher → five phase prologues → one shared camera/entity
tail → epilogue**. Every phase prologue either falls through or `JMP 0x8BDE7`
(the shared tail); the tail runs once per call.

### 1.1 Dispatch and prologue (fresh bytes `0x8BAF0` 32 B)

```
0x8BAF6 MOV EBP,0x1587EC                  ; the camera/entity struct base
0x8BAFB MOV EAX,[0x157A4A]
0x8BB05 SAR EAX,0x18                       ; EAX = byte[0x157A4D] (CURRENT phase)
0x8BB08 MOV dword[0x15884E],0x5            ; default speed/mode cell = 5
0x8BB0E CMP EAX,6  ; JNZ 0x8BB55           ; phase 6 prologue
0x8BB55 CMP EAX,2  ; JNZ 0x8BC01           ; phase 2 prologue
0x8BC01 CMP EAX,0xC ; JNZ 0x8BCE7          ; phase 0x0C prologue (the end arm)
0x8BCE7 CMP EAX,0x13 ; JZ 0x8BCF5
0x8BCEC CMP EAX,0x14 ; JNZ 0x8BDE7         ; phase 0x13/0x14 prologue
```

The phase byte is **`[0x157A4D]`**: fresh `FUN_000740A0` (the phase setter)
first-hand is `MOV AH,[0x157A4D]; MOV [0x157A4E],AH; MOV [0x157A4D],AL` — it
saves the current phase to `[0x157A4E]` and writes the new phase to the **byte
`[0x157A4D]`**. So `MOV EAX,[0x157A4A]; SAR EAX,0x18` reads `byte[0x157A4D]`
(the `0x157A4A` dword's high byte) — the alias trap applied. A phase outside
`{6,2,0x0C,0x13,0x14}` is a **no-op** (straight to `0x8BDE7`).

**Phase 6 (`0x8BB13..0x8BB50`)** — a fixed camera-offset block: `[0x158838]:=0x930`,
`[0x158830]:=0x4B0`; if `dword[0x157754] < 0` then `[0x158838]:=0xFFFFF6D0`
(`−0x930`); if `dword[0x15880C] < 0` then `NEG [0x158830]`; `JMP tail`.
(`0x8BB23 MOV [0x158838],EBX` bytes `89 1d 38 88 15 00`.)

**Phase 2 (`0x8BB55..0x8BBFC`)** — a dynamic camera block keyed on the period
index: `AL=byte[0x157AC2]`; if `> 3` propagate `dword[0x15880C]` into the
`0x158830` triple (`MOVSD`×3); else compute an angle via `CALL 0x8DC68`
(`MOVSX EAX,DI / MOVSX EDX,SI / CALL 0x8DC68 / CWDE`), clamp to `±0x120`, and
`MOV dword[0x15884E],0x7` (`0x8BBE6`); `JMP tail`.

### 1.2 The shared camera tail (`0x8BDE7..0x8C065`)

Every prologue converges on `0x8BDE7`, which is **also** the fall-through from
the phase-`0x13`/`0x14` block:

```
0x8BDE7 MOV EBX,0x15883C ; EDX=0x158830 ; EAX=0x15880C
0x8BDF6 CALL 0x8DCD4                        ; camera vector transform -> [0x15883C]
0x8BDFB CMP word[0x15883C],0 ; JZ 0x8BE20
0x8BE18 CALL 0x8DD70 ; CWDE                  ; angle = atan2(word[0x15883E],[0x158840])
0x8BE25 MOV [0x15885E],EAX                   ; store the aim angle
0x8BE2A CMP byte[0x158881],0 ; JZ 0x8BF0F
0x8BE37..  the aim/steer interpolation (mod 0x400, ±clamp)
0x8BF0F MOV EAX,[EBP+0x4E] ...               ; [0x1587EC] struct fields
0x8C030 CALL 0x8DC68 / 0x8C052 the entity velocity integrate
0x8C055 MOV EAX,0x1587EC ; CALL 0x6FE84      ; publish the entity
0x8C065 RET
```

This tail is **camera/entity steering math** (the `[0x1587EC]` struct, its
`[+0x20]/[+0x28]/[+0x6A]/[+0x72]/[+0x76]` fields, table `0x114E04`, the
`0x8DC68`/`0x8DCD4` signed-16 helpers, and `0x6FE84(0x1587EC)`); it contains
**no** end-of-match decision. Helpers (fresh signatures): `FUN_0008DCD4(short*,
short*, short*)` (`0x8DCD4..0x8DD5B`) a 3-point delta/weighted-average transform;
`FUN_0008DC68(int,int)` (`0x8DC68..0x8DCD2`) a 2-value interpolation;
`FUN_0008DD70` (`0x8DD70..0x8DD82`) a `CALL 0x000CD474` wrapper. See **FU231-L5**.

---

## 2. The phase-`0xC` block — the regulation-end terminal decision (fresh)

Fresh `0x8BC01..0x8BCE2` (`disassemble_function`), and fresh `read_memory
0x8BCB0` 64 B confirm the decision bytes. The block:

```
0x8BC0A CMP dword[0x158808],0 ; JNZ 0x8BC20   ; if no "act" installed:
0x8BC13 MOV AX,[0x157A64]                      ;   delta
0x8BC19 ADD word[0x158818],AX                  ;   <<< the END TIMER += delta
0x8BC20..0x8BC52  camera := 0x960 triple; CALL 0x36200 (EAX=3); CALL 0x8DCD4
0x8BC57 MOV EAX,[0x158816] ; SAR EAX,0x10      ; = word[0x158818]
0x8BC5F CMP EAX,0x78 ; JLE 0x8BDE7             ; T <= 0x78 -> hold
```

`FUN_00036200` first-hand is `MOV [0x105FC4],EAX; RET`, so the `EAX=3` call
writes **`[0x105FC4]=3`** (the camera/view mode; fresh `0x36200` 32 B). The gate
`0x8BC57` reads **`word[0x158818]`** (`[0x158816]>>16`) — the timer this same
block accumulates — **not** `[0x15881A]` (FU-228-L2 erratum direction
confirmed; see §4.2).

Fresh `read_memory 0x8BCB0` (bytes `00 c1 f8 10 3d 68 01 00 00 0f 8e 28 01 00 00
80 3d c0 7a 15 00 00 74 11 b8 13 00 00 00 31 d2 e8 cc 83 fe ff e9 0e 01 00 00
66 c7 05 22 88 15 00 01 00 ...`) decodes:

```
0x8BCAC MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x168 ; JLE 0x8BDE7
0x8BCBF CMP byte[0x157AC0],0 ; JZ 0x8BCD9          ; <<< the EXTRA-TIME FLAG
0x8BCC8 MOV EAX,0x13 ; XOR EDX,EDX ; CALL 0x740A0   ; flag SET -> phase 0x13
0x8BCD9 MOV word[0x158822],0x1                       ; flag CLEAR -> MATCH OVER (1)
```

Bytes verified: `0x8BCBF 80 3d c0 7a 15 00 00` (CMP byte `[0x157AC0]`,0);
`0x8BCC8 b8 13 00 00 00` (MOV EAX,0x13); `0x8BCCF e8 cc 83 fe ff` → rel32
`0xFFFE83CC` = `−0x17C34` → target `0x8BCD4 − 0x17C34 = 0x740A0` (verified by
hand); `0x8BCD9 66 c7 05 22 88 15 00 01 00` (MOV word `[0x158822]`,1).

**The full decision (`P := FUN_00045001()`):** fresh `FUN_00045001`
(`0x45001..0x45024`) is `MOV AL,[0x1067EC]; AND AL,0xF0; RET` — a **byte
predicate** (`P != 0` iff the high nibble of `[0x1067EC]` is set). The block:

Each row's guard is evaluated **only** once `T > 0x78` (the `0x8BC57` exit gates
every later row):

| line reached | guard (`T = word[0x158818]`, `P := FUN_00045001()`) | effect |
|---|---|---|
| `0x8BC57` | `T <= 0x78` | hold (tail) |
| `0x8BC80` reset | `T > 0x78` **and** `(P!=0 or T>0xB4)` **and** `[0x10F378]!=0` | `CALL 0x37F54` → **`[0x105FFC]=5`**; `[0x10F378]:=0`; `word[0x158818]:=0`; tail — **third arm** (§5) |
| `0x8BCA3` re-entry | (`T > 0x78` and) `[0x10F378]==0` fallthrough **or** `P==0 and T<=0xB4` (skip) | re-test P/timer below |
| `0x8BCBF` decide | `T > 0x78` **and** `[0x10F378]==0` **and** `(P!=0 or T>0x168)` | `byte[0x157AC0]!=0` → **phase `0x13`** (`0x740A0`); `==0` → **`word[0x158822]:=1`** |

So the **regulation-end terminal decision** is:

```
if T <= 0x78: hold
elif [0x10F378] != 0 and (P or T > 0xB4):        # tie latch -> no extra time
        CALL 0x37F54        ; [0x105FFC] = 5       # no-ET "match over" screen
        [0x10F378] = 0 ; word[0x158818] = 0
elif [0x10F378] == 0 and (P or T > 0x168):
        if byte[0x157AC0] != 0:  phase := 0x13   # extra time entry
        else:                    word[0x158822] := 1   # match over (no ET)
```

The `[0x10F378]` no-ET reset path (`0x8BC89 CALL 0x37F54`) is the arm FU-228
§2.2 recorded only as "the no-ET latch"; §5 below shows it writes `[0x105FFC]=5`.

---

## 3. The phase-`0x13`/`0x14` blocks — the extra-time terminals (fresh)

Fresh `0x8BCE7..0x8BDDD`; fresh `read_memory 0x8BD80` 96 B verifies every byte.
`0x8BCE7 CMP EAX,0x13; JZ 0x8BCF5` / `0x8BCEC CMP EAX,0x14; JNZ tail`:

```
0x8BCF5 CMP dword[0x158808],0 ; JNZ 0x8BD0B
0x8BCFE MOV AX,[0x157A64] ; ADD word[0x158818],AX     ; ET-half timer += delta
0x8BD0B MOV ESI,0x780                                  ; camera hold
0x8BD18 MOV dword[0x158830],0x780                      ; <<< the 0x780 camera hold
0x8BD10 MOV EDX,[0x158814] ; ... TEST EDX,EDX ; NEG if <0 ; CMP EAX,0x60
0x8BD3B MOV dword[0x158830],0x61A8                     ; if |[0x158814]| < 0x60
0x8BD45 MOV EAX,[0x157A4A] ; SAR EAX,0x18 ; CMP EAX,0x14 ; JNZ 0x8BD95
```

**Phase `0x14` arm (`0x8BD52..0x8BD93`):**
```
0x8BD52 MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x4B0 ; JG 0x8BD81
0x8BD61 CALL 0x45001 ; TEST AL,AL ; JZ tail           ; if !P ...
0x8BD6E MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x258 ; JLE tail
0x8BD81 CMP dword[0x10F360],0 ; JNZ 0x8BDDD            ; the ET-over latch
0x8BD8A MOV word[0x158822],0x2                          ; <<< ET OVER (2)
0x8BD93 JMP 0x8BDDD
```

**Phase `0x13` arm (`0x8BD95..0x8BDD4`):**
```
0x8BD95 MOV AX,[0x157AC5] ; CMP AX,[0x157AC7] ; JNZ tail   ; scores DIFFER -> hold
0x8BDA4 MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x4B0 ; JG 0x8BDCB
0x8BDB3 CALL 0x45001 ; TEST AL,AL ; JZ tail
0x8BDBC MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x12C ; JLE tail
0x8BDCB CMP dword[0x10F360],0 ; JNZ 0x8BDDD
0x8BDD4 MOV word[0x158822],0x2                          ; <<< ET OVER (2)
0x8BDDD MOV dword[0x10F360],1                           ; <<< the latch
```

Fresh byte checks: `0x8BD81 83 3d 60 f3 10 00 00` (CMP dword `[0x10F360]`,0);
`0x8BD8A 66 c7 05 22 88 15 00 02 00`; `0x8BD95 66 a1 c5 7a 15 00` (MOV AX
`[0x157AC5]`); `0x8BD9B 66 3b 05 c7 7a 15 00` (CMP AX `[0x157AC7]`);
`0x8BDD4 66 c7 05 22 88 15 00 02 00`; `0x8BDDD c7 05 60 f3 10 00 01 00 00 00`
(MOV dword `[0x10F360]`,1).

So an **ET-half terminal** is gated on: the ET-half timer `word[0x158818]` past
its gate (`> 0x4B0`, or the `P`/`0x258` resp. `0x12C` paths), **and** the ET-over
latch `[0x10F360]==0`; it writes `word[0x158822]=2` and then sets
`[0x10F360]:=1` (one-shot). The phase-`0x13` (first ET half) terminal **also**
requires the scores to be **equal** (`0x8BD95`); a differing score holds. The
phase-`0x14` (second ET half) arm has no score test.

---

## 4. The cell producers — the clock side (fresh)

### 4.1 The ET flag `byte[0x157AC0]` and the tie latch `[0x10F378]`

Fresh `get_xrefs_to 0x157AC0` = **19** (writers include `0x8B2AC`, `0x8B2C1`,
`0x73F58`, `0x389A4`); fresh `get_xrefs_to 0x10F378` = **4**
(`0x887F8 WRITE FUN_000886D4`, `0x8BC80 READ`, `0x8BC92 WRITE FUN_0008BAF0`,
`0x8B2DA WRITE FUN_0008AF38`). The producer (fresh `0x8B280` 128 B) is inside
the same per-frame clock `FUN_0008AF38`:

```
0x8B295 MOV AX,[0x157AC5] ; MOV DX,[0x157AC7] ; MOV ECX,1
0x8B2A7 CMP AX,DX ; JZ 0x8B2B8
0x8B2AC MOV byte[0x157AC0],1                 ; scores DIFFER -> ET flag set
0x8B2B3 JMP 0x8AFB7
0x8B2B8 CMP dword[0x14C2EE],0 ; JZ 0x8B2CD
0x8B2C1 MOV byte[0x157AC0],1                 ; [0x14C2EE] force-ET -> flag set
0x8B2C8 JMP 0x8AFB7
0x8B2CD CMP byte[0x14C1D0],0 ; JNZ 0x8AFB7    ; [0x14C1D0]!=0 -> leave
0x8B2DA MOV dword[0x10F378],1                ; tie, no force -> the TIE LATCH
```

Bytes: `0x8B2AC c6 05 c0 7a 15 00 01` (MOV byte `[0x157AC0]`,1);
`0x8B2C1 c6 05 c0 7a 15 00 01`; `0x8B2DA c7 05 78 f3 10 00 01 00 00 00`
(MOV dword `[0x10F378]`,1). **This resolves FU228-L6's core:** the ET flag is set
by a **differing score** (`0x8B2AC`) or an explicit `[0x14C2EE]` **force-ET**
(`0x8B2C1`); a **tie** with no force and `[0x14C1D0]==0` sets the **tie latch**
`[0x10F378]=1` (which routes the phase-`0xC` block to the `[0x105FFC]=5` arm, not
to `[0x158822]=1`). Fresh `read_memory 0x157AC0` (16 B) is all zero at file load.

### 4.2 The period-limit cells are `[0x15881A]`/`[0x15881C]`, not the end timer

The clock's own limit selection (fresh `0x8AFE2` / `0x8AFEB`) reads
`word[0x15881A]` (period index `<= 1`) resp. `word[0x15881C]` (index `2..3`):
`0x8AFE2 66 8b 35 1a 88 15 00`. The **end timer** the phase-`0xC`/`0x13`/`0x14`
blocks accumulate is `word[0x158818]`. These are distinct cells. `FUN_000886D4`
(fresh `0x88710`) seeds `MOV [0x15881A],AX` from `[0x14C1D1]×0x3C` and (fresh
`0x88716`) `IMUL EAX,EDX,0x14` — i.e. `[0x15881A]`/`[0x15881C]` are the FU-62
period-limit seconds, while `0x8871B` clears the end word
(`66 89 1d 22 88 15 00`). This confirms **FU228-L2**'s erratum direction.

### 4.3 The match-reset clear set `FUN_000886D4`

Fresh `0x886F0`/`0x887D0` (in `FUN_000886D4`, body `0x886D4..0x8885F`):
`0x886FA MOV word[0x158818],DX` (DX=0), `0x8871B MOV word[0x158822],BX`
(BX=0), `0x887F2 MOV dword[0x10F360],EDI` (EDI=0), `0x887F8 MOV
dword[0x10F378],EDI` (EDI=0). So a **new match/reset clears all four terminal
cells**: the end timer, the end word, the ET-over latch, the tie latch.

---

## 5. The three end-word setters + the third arm (fresh)

`word[0x158822]` is a **word**; fresh `get_xrefs_to 0x158822` = **10** — exactly
FU-228 §2.1's census (5 clears `0x4B0E7`/`0x4B29B`/`0x8871B`/`0x74342`/`0x88D42`;
3 setters all inside `FUN_0008BAF0`; 2 readers `0x4B106` in `FUN_0004B100` and
`0x8B678` in `FUN_0008AF38`). The three setters:

| site | phase | gate | value |
|---|---|---|---|
| `0x8BCD9` | `0x0C` | `byte[0x157AC0]==0` and `[0x10F378]==0` and timeline | **`1`** (regulation over) |
| `0x8BD8A` | `0x14` | ET2 timeline, `[0x10F360]==0` | **`2`** (ET over) |
| `0x8BDD4` | `0x13` | ET1 timeline, scores equal, `[0x10F360]==0` | **`2`** (ET over) |

**The third terminal arm.** Fresh `0x37F54` (body `0x37F54..0x37F8F`) — the
target of the phase-`0xC` no-ET path (`0x8BC89`) — is:

```
0x37F58 MOV dword[0x14B028],0
0x37F5E CALL 0x37B34 ; TEST EAX,EAX ; JL 0x37F8D
0x37F67 MOV EAX,0x13 ; CALL 0x396E4
0x37F71 MOV EAX,0x5 ; XOR EBX,EBX ; CALL 0x36BC0   ; <<< [0x105FFC] = 5
0x37F7D MOV dword[0x14AF70],EBX ; CALL 0x53DC4 ; CALL 0x54028
```

Fresh `0x37F71 b8 05 00 00 00` (MOV EAX,5) + `e8 43 ec ff ff` (CALL, rel32 →
`0x36BC0`). Fresh `0x36BC0` first-hand is `MOV [0x00105FFC],EAX; RET` (bytes
`a3 fc 5f 10 00`) — so the store target is the **LE-image flat runtime address
`0x105FFC`**, not `0x5FFC` (one hex digit): fresh `get_xrefs_to 0x105FFC` = **7**,
`get_xrefs_to 0x5FFC` = **0**. Cross-cited FU-64 §2.1 renders the same cell as
`[0x5FFC]`; because LE globals sit at `+0x100000` from the file offset, FU-64's
flat form is the `0x105FFC` cell (the `+0x100000` delta). So `FUN_00036BC0(x)`
writes `[0x105FFC]=x`; the FU-228 lifecycle screens are `2/3/4`. The tie/no-ET
arm writes a **fifth screen `[0x105FFC]=5`** — a terminal distinct from the end
word. This is **new** relative to FU-228 and is what the clock's tie latch
(`0x8B2DA`) ultimately reaches. (Screen-5 semantics: **FU231-L2**.)

---

## 6. Verdict — the exact minimal-port terminal decision

### 6.1 The native terminal decision, stated exactly

The match reaches a terminal through **three** mutually-exclusive arms, all
driven by the same phase dispatch and the cells
`{byte[0x157AC0], [0x10F378], [0x10F360], word[0x158818], phase}`:

1. **No-ET / tie terminal (`[0x105FFC]=5`).** phase-`0xC`, `word[0x158818] > 0x78`
   and `[0x10F378]!=0` and `(P != 0 or T > 0xB4)` → `0x37F54` sets `[0x105FFC]=5`,
   clears `[0x10F378]`, resets `word[0x158818]`. **No end word.** The clock sets
   `[0x10F378]=1` on a **tie** (`0x8B2DA`).
2. **Regulation match-over (`[0x158822]=1`).** phase-`0xC`, timeline reached,
   `[0x10F378]==0`, `byte[0x157AC0]==0` → `0x8BCD9`. The flag is **clear** only
   when the clock never set it (no differ, no force-ET).
3. **Extra time / ET terminal.** phase-`0xC`, timeline, `[0x10F378]==0`,
   `byte[0x157AC0]!=0` → **phase `0x13`** (`0x8BCC8`); then phase-`0x13`/`0x14`
   with the ET-half timeline and `[0x10F360]==0` → **`[0x158822]=2`**
   (`0x8BDD4`/`0x8BD8A`), latched by `[0x10F360]:=1`.

The **exact gated terminal decision** the frame loop consumes is therefore the
end word `word[0x158822] ∈ {0,1,2}` (read at `0x8B678`, consumed by
`FUN_0004B100` at `0x4B106`/`0x4B1AB`, FU-228 §2.3), **plus** the orthogonal
screen `[0x105FFC]=5` arm the end word does not signal.

### 6.2 The minimal-port shape (phase-22)

* **Do not port the camera body.** §1's prologues and the shared tail
  (`0x8BDE7..0x8C065`, the `0x1587EC` struct, `0x8DC68`/`0x8DCD4`/`0x8DD70`,
  `0x6FE84`, the `0x960`/`0x780`/`0x930` camera sinks) are camera/entity work;
  the terminal decision is **not** in them.
* **Port the decision, not the machine.** The minimal shape is the three-arm
  state machine of §6.1 over five cells. The engine already holds the inputs or
  their near-equivalents: `byte[0x157AC0]` (the ET flag — engine cell per
  FU-228 §4.2/§4.3), `word[0x158818]` (the end timer — currently unmodelled),
  `[0x10F378]`/`[0x10F360]` (the two latches — unmodelled), and the phase byte.
  The honest Stage-2 shape is to derive the terminal condition from the
  FU-62 clock + the ET flag and gate the lifecycle `mark_over`/exit on it.
* **The engine's current `mark_over` trigger is period-agnostic and wrong**
  (FU-228 §1.3 / §4.2: `fifa96_match_run.c:4161`), and — new here — the native
  has **three** terminal outcomes, not one: the engine today has no cell for
  `[0x158822]`, no no-ET screen, and no ET-over screen. A faithful phase-22
  landing must distinguish **regulation-over (1)** vs **ET-over (2)** vs
  **no-ET screen (5)**; the no-ET arm is the one FU-228 did not record.
* **Cross-slice (not owned here).** The half-time restart, the ET-entry route,
  the choice of `0x13`/`0x14`, and the champion/penalty continuation are sibling
  W22 slices; this slice fixes only the end-machine's decision surface. The
  FU-228 conditional (**FU228-L6**) is **partially resolved**: the flag is set
  by differ/force (`0x8B2AC`/`0x8B2C1`); a tie takes the `[0x10F378]` no-ET arm
  — but the *configuration* polarity of `[0x14C2EE]`/`[0x14C1D0]` remains open
  (**FU231-L3**).

### 6.3 Census impact

**None.** No new match-action row is wired; this is a re-count/re-read of the
`FUN_0008BAF0` decision surface. `floor(100 × 55/80) = 68` stands; no golden
re-pin.

---

## 7. Residual legs

1. **FU231-L1 — `FUN_00045001` is an un-named byte predicate.** It returns
   `byte[0x1067EC] & 0xF0` (fresh `0x45009..0x45024`); the phase-`0xC` and
   ET-half timeline gates all branch on it, but the cell's producer and meaning
   are unmapped (fresh `get_xrefs_to 0x1067EC` not taken). Owner: the HUD/menu
   or FU-64 screen slice.
2. **FU231-L2 — the `[0x105FFC]=5` no-ET terminal screen is unmapped.** The arm is
   fresh (`0x37F54 → 0x37F71 EAX=5; CALL 0x36BC0`), and FU-228 only knew the
   screens `2/3/4` (FU-64 §5). The screen-5 render/logic and the `0x396E4`,
   `0x53DC4`, `0x54028` side-effects are not decomposed. Owner: the FU-64 screen
   machine.
3. **FU231-L3 — the ET configuration flags `[0x14C2EE]` / `[0x14C1D0]` are
   unmapped.** Fresh reads at `0x8B2B8`/`0x8B2CD` show they select differ/force
   vs no-ET; their producers and polarity are unknown. This is the residue of
   **FU228-L6**. Owner: the match-config / FU-224 slice.
4. **FU231-L4 — the per-frame reachability of the score-compare is not settled.**
   The `0x8B295` comparison is entered from `0x8B249 JZ` on `word[0x157ABA]==0`,
   itself reached from the `0x8B3AE` block gated by the period timer
   `[0x157AC1] >= 0x3C` (`0x8AFBE CMP EAX,0x3C`). The `0x8B3AE` block is not
   decomposed, so whether the flag/latch are set once at period end or
   repeatedly per frame is unproven. Owner: the clock slice (`FUN_0008AF38`).
5. **FU231-L5 — the camera helpers and the shared tail are recorded, not
   decomposed.** `FUN_0008DC68` (`0x8DC68..0x8DCD2`), `FUN_0008DCD4`
   (`0x8DCD4..0x8DD5B`), `FUN_0008DD70` (`0x8DD70..0x8DD82`, wraps
   `CALL 0x000CD474`), `FUN_0006FE84` (`0x6FE84..0x6FFB7`), and the `0x1587EC`
   struct fields used in `0x8BDE7..0x8C065` are only located, not semantically
   named. Not required for the terminal decision. Owner: the camera slice.
6. **FU231-L6 — the phase-`6`/`2` prologues' semantics are unproven.** Fresh
   bytes give the constants (`0x930`/`0x4B0` fixed offsets; the period-index
   `> 3` propagation vs the `0x8DC68` angle + `[0x15884E]=7`), but which phase
   is which (goal celebration / replay / kickoff) is not established. Owner: the
   camera/phase slice.
7. **FU231-L7 — the raw `[0x158822]` writers `0x74342` / `0x88D42` remain
   function-less (FU228-L3 carries).** Both clear the word (`CX=0` / `SI`) in
   reset blocks; `get_function_by_address 0x74340` / `0x88D42` = none. Owner:
   FU-196-L6 / the match-reset slice.
8. **FU231-L8 — `word[0x157A64]` (the timeline delta) is recorded by role, not
   mapped.** Fresh xrefs show it written by `FUN_0004B02C` (`0x4B052`) and
   `FUN_0004B100` (`0x4B151`) and read as the `word[0x158818]` increment here
   (`0x8BC13`/`0x8BCFE`). Owner: the frame-loop/FU-62 slice.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `image_base 0`, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `disassemble_function` — `0x8BAF0` (379 insns, the whole body), `0x740A0`.
* `disassemble_bytes` (start, len B) — `0x8BAF0` (32, `read_memory`), `0x8B280`
  (128), `0x8AFA8` (64), `0x8B240` (80), `0x8AF38` (192), `0x36200` (32),
  `0x45001` (48), `0x8DD70` (24), `0x8DCD4` (140), `0x8DC68` (112), `0x6FE84`
  (64), `0x37F54` (60), `0x887D0` (96), `0x886F0` (48).
* `read_memory` — `0x8BCB0` (64), `0x8BD80` (96), `0x8BAF0` (32),
  `0x158808` (96, all zero), `0x157AC0` (16, all zero).
* `get_function_by_address` — `0x8BAF0` (`FUN_0008baf0`, `0x8BAF0..0x8C065`),
  `0x8AF38` (`FUN_0008af38`, `0x8AF38..0x8B687`), `0x36200` (`FUN_00036200`),
  `0x45001` (`FUN_00045001`), `0x37F54` (`FUN_00037f54`),
  `0x740A0` (`FUN_000740a0`), `0x8DCD4` (`FUN_0008dcd4`),
  `0x8DC68` (`FUN_0008dc68`), `0x8DD70` (`FUN_0008dd70`),
  `0x6FE84` (`FUN_0006fe84`), `0x886D4` (`FUN_000886d4`, `0x886D4..0x8885F`).
* `get_xrefs_to` — `0x8BAF0` (**1**), `0x158822` (**10**), `0x10F378` (**4**),
  `0x10F360` (**5**), `0x157AC0` (**19**), `0x158818` (**83**),
  `0x157AC5` (**41**), `0x157AC7` (**95**), `0x157A4A` (**128**),
  `0x158808` (**22**), `0x15884E` (**3**), `0x45001` (**11**),
  `0x740A0` (**27**), `0x8AF38` (**2**), `0x14C2EE` (**5**),
  `0x14C1D0` (**19**), `0x105FFC` (**7**), `0x5FFC` (**0** — the bare form does
  not exist; the cell is the flat `0x105FFC`). Re-verify: `disassemble_bytes
  0x36BC0` (16 B) = `MOV [0x00105FFC],EAX; RET` (`a3 fc 5f 10 00`).

Docs cross-cited: **FU-228** (the predecessor — §2.1 cell census, §2.2 body
sketch, §2.3 caller/consumer, §4 verdict, legs L2/L3/L6), FU-64 §2.1/§5/§6
(the `[0x105FFC]` screens — FU-64's flat `[0x5FFC]` form plus the `+0x100000`
delta — and `0x36BC0`), FU-213 §5 (the end-word census). No
writes beyond this slice file; no rename/label/comment/function/script/project
save; `/FIFA96.EXE` and the ISO untouched; no git commands.
