# FU-242 (W23-3, anchor): the installer mechanism + the exhaustive per-row census

Recon slice for the Phase-23 census/ceiling probe (task 3 of 8).
Promotes to `docs/ghidra/FU242_installer_census.md`.

Result in one line: **the action-table installer is `FUN_0007D9A4`
(EAX=rec / EDX=code / EBX=stage / ECX=invoke) with **77** direct call sites
(fresh), but a BA/66BA/C7C2 literal scan is NOT exhaustive — the *parametric*
bulk installer `FUN_0008CEB8(rec,i0,i1,code,skip)` carries the code in **ECX
(B9)** and installs rows `0`,`3`,`0x15`,`0x19`,`0x25`,`0x26`,`0x28` across a
record range; re-running the census over all 77 sites + the parametric route
finds an installer for row `15` (missed) and row `25` (missed) — the prior
FU-142f "no installer" verdicts for `25` (and the FU-186 `1A` bucket) are
incomplete. Rows `27`,`29`,`2B`,`2C` still have **no** installer in either
mechanism; row `0A`'s only site lives in an **xref-dead** function `0x7CDD8`.**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit `program` argument every
  call; case-sensitive). `get_current_program_info` this session: 2706 fns /
  15239 syms, `watcom:LE:32:default`, image base 0, `.image` overlay present.
* Ghidra **read-only**: no renames/comments/labels/function creation/scripts/saves.
* Tool calls this slice (all on `/FIFA96.EXE`):
  * `read_memory` — `0x1106E0`(180 B), `0x110794`(140 B), `0x1107EC`(56 B),
    `0x110870`(40 B), `0x110950`(160 B).
  * `get_xrefs_to` — `0x7D9A4` (**77**), `0x8D098`(2), `0x8CEB8`(15),
    `0x888FC`(11), `0x740A0`(27), `0x157A4A`(128), `0x157A4D`(6),
    `0x77EAC`(1), `0x7997C`(1), `0x782D0`(1), `0x8D8EC`(4), `0x8C33C`(6),
    `0x7CD60`(2), `0x7CFD0`(2), `0x4C374`(19), `0x157A68`(-).
  * `disassemble_bytes` — the 77-site windows (16–64 B each, `restrict_to_execute_memory=false`),
    `0x7D9A4`, `0x7CDD0`, `0x88CC0`, `0x88DF0`, `0x73D20`, `0x73E40`, the
    27 `FUN_000740A0` call windows, the 11 `FUN_000888FC` call windows.
  * `decompile_function` — `0x8D098`, `0x77EAC`, `0x7997C`, `0x782D0`,
    `0x8D8EC`, `0x7DAB4`, `0x8CEB8`, `0x6D920`, `0x888FC`, `0x740A0`,
    `0x88860`, `0x73E28`, `0x4C374`, `0x8C33C`.
  * `search_byte_patterns` — every NULL code in `BA`/`66BA`/`C7C2` (see §5.2).
* Engine ground truth read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
  (slot comments `5097..5259`), `fifa96_match_run.c`, `fifa96_match_act.c`,
  `include/fifa96_engine/fifa96_match_handlers.h`.
* Baseline: HEAD `4b98748`, `make check` 110/110, wired **55/80 = 68%**,
  M1 `09b726b7…`, M2 `b281a9b2…`. No file outside this slice's draft/report
  written; ISO / `game/hdd/` untouched.

## 1. The three tables (fresh `read_memory`, byte-for-byte)

### 1.1 Action table `0x1106E0[45]` (codes `0x00..0x2C`)

```
00 7DB10  01 7DBC0  02 7DFCC  03 7E1A4  04 7E7C8  05 7F194  06 801B4
07 814B0  08 81068  09 80A00  0A 81738  0B 81908  0C 81C90  0D 8251C
0E 82710  0F 82AD0  10 855F0  11 85DE4  12 83D68  13 84B00  14 8784C
15 87CD0  16 84630  17 84730  18 849B0  19 746E4  1A 7662C  1B 76D28
1C 77728  1D 74EB0  1E 7550C  1F 76380  20 84EEC  21 85214  22 8539C
23 82F84  24 86510  25 880CC  26 866F4  27 86820  28 870E8  29 874E4
2A 86A34  2B 87738  2C 84598
```
`[0x0A]=0x81738` is the row-07 tail/RET; `[0x2B]=0x87738` is the shared
row-29 epilogue RET (FU-142f dead verdict re-verified: `0x87738` body is
`ADD ESP,0x14 / POP…RET`).

### 1.2 Phase table `0x110794[35]` (phases `0x00..0x22`)

```
00 6DE34 01 6E1D0 02 6DCC8 03 6DE44 04 6DE44 05 6E05C 06 6DD9C 07 6DE44
08 6DD6C 09 6DD6C 0A 6DE34 0B 6DE34 0C 6DF4C 0D 6DE34 0E 6DE34 0F 6DE34
10 6E004 11 6E1C8 12 6E1D0 13 6E244 14 6E244 15 6DCC8 16 00000 17 88DC8
18 8922C 19 89FA4 1A 89620 1B 890EC 1C 89110 1D 89868 1E 8A798 1F 88F4C
20 8B688 21 8B874 22 8B900
```
`[0x16]=0` is the loader INT3 stub (`FIFA96_MATCH_PHASE_INT3_SLOT`).

### 1.3 Act table `0x1107EC[13]` (acts `0..0xC`)

```
0 0000000  1 88DC8  2 8922C  3 89FA4  4 89620  5 890EC  6 89110  7 89868
8 8A798   9 88F4C  A 8B688  B 8B874  C 8B900
```
`0x1107EC[k] == 0x110794[0x16 + k]` **byte-for-byte** (13 dwords), and the
next 4 B `0x006007FF` are unrelated data (the read was 56 B). The act slot is
confirmed by `FUN_000888FC` (§4.3).

## 2. The action installer `FUN_0007D9A4` (fresh)

`get_function_by_address 0x7D9A4` → `FUN_0007d9a4`, body `0x7D9A4..0x7DAB2`.
Fresh disassembly (73 instructions) gives the **Watcom register ABI**
(`EAX=rec`, `DX=code`, `BX=stage`, `ECX=invoke`):

```
7D9A9 MOV ESI,EAX            ; rec
7D9AB MOV word[ESP+4],DX     ; code (16-bit)
7D9B0 MOV word[ESP],BX       ; stage
7D9B4 MOV EDI,ECX            ; invoke flag
7D9CB MOVSX AX,byte[ESI+0x91]; current action code
7D9D3 CMP AX,DX / 7D9D6 JZ   ; no-op if already this code
7DA6D MOV EAX,dword[ESP+2]
7DA71 SAR EAX,0x10           ; EAX = (int32)(int16)DX  (code, sign-extended)
7DA74 SHL EAX,0x2
7DA77 ADD EAX,0x1106E0       ; action-table base
7DA7C MOV EAX,[EAX]          ; handler
7DA8F MOV dword[ESI+0x18],EAX; stage handler at rec+0x18 (== rec+0x1C-4)
7DA63 MOV AL,byte[ESP+4] / 7DA67 MOV byte[ESI+0x91],AL   ; rec action code
7DA92 MOV AL,byte[ESP] / 7DA95 MOV byte[ESI+0x92],AL     ; rec stage
7DA9B MOV AX,word[ESI+0x79] / 7DA9F MOV word[ESI+0x7B],AX
7DAA3 TEST DI,DI / 7DAA6 JZ   ; ECX (DI) == 0 -> no invoke
7DAA8 MOV EAX,ESI / 7DAAA CALL dword[ESI+0x18]  ; invoke-now
```

Notes (fresh, correcting common aliases):
* The code is read from `DX`, not `EDX`; `MOV EDX,imm32` with a zero high word
  is one way to set it, but so is `MOV DX,imm16`.
* `dword[ESP+2]>>16` is the **word at ESP+4** (i.e. DX) — the alias trap
  stated in the brief; verified against the raw disassembly.
* The handler is written to `[rec+0x18]` (the phase walk reads `[rec+0x1C]`;
  §4.2 — these are distinct slots: `+0x18` = action handler, `+0x1C` = phase
  handler).

### 2.1 The 77 direct call sites (fresh `get_xrefs_to 0x7D9A4`)

`get_xrefs_to 0x7D9A4` returns **77** `UNCONDITIONAL_CALL` references
(fresh, first-hand — matches the brief's expected 77). Every site was
disassembled and the code argument classified (§5.1). Encodings seen:

| form | bytes | example |
|---|---|---|
| `MOV EDX,imm32` | `BA` | `0x78299 BA1B000000` (row 1B) |
| `MOV DX,imm16` | `66 BA` | (none found at a site) |
| `MOV EDX,imm32` | `C7 C2` | (none found at a site) |
| `MOV EAX,imm32` + `MOVSX EDX,AX` | `B8` + `0F BF D0` | `0x79A97/0x79A9E/0x79AA3` |
| **`MOV ECX,imm32` → `FUN_0008CEB8`** | **`B9`** | `0x8D44B B915000000` (row 15) |

The last row is the census blind spot (§3).

## 3. The parametric bulk installer `FUN_0008CEB8` (the blind spot)

Fresh decompile `0x8CEB8`:

```c
void FUN_0008ceb8(int rec, short i0, short i1, short code, short skip) {
  if (-1 < i0 && i0 < 0xb) {
    if (10 < i1) i1 = 0xb;
    p = rec + i0*0xb2;
    for (i = i0; i <= i1; i++, p += 0xb2)
      if (byte[p+0x9a] == 0 && byte[p+0x91] != skip) {
        short c = code;
        if (i == 0 && code == 3) c = 0x19;      /* index-0 special case */
        FUN_0007d9a4(p, c, 0, 0);               /* bulk install */
      }
  }
}
```

Its 5th template parameter (the **action code**) is in **ECX** (Watcom 4th
reg param), so a `BA`/`66BA`/`C7C2` scan for `MOV EDX,code` cannot see it.
`get_xrefs_to 0x8CEB8` = 15 (all inside `FUN_0008D098`; FU-142 already noted
this helper for rows 0x26/0x28/0x2A). The `FUN_0008D098` situation machine
calls it with codes `{0, 3, 0x15, 0x25, 0x26, 0x28}` — see §6.

## 4. The phase dispatcher + the act invoker

### 4.1 The phase/situation byte `0x157A4D` (aliasing, fresh)

`[0x157A4A]>>24` **is** the byte at `0x157A4D`. `get_xrefs_to 0x157A4D` = 6:
three WRITEs and three READs. So the per-record phase index and the
`FUN_0008D098` situation switch are the **same byte**:

* WRITE `0x740AC` (`FUN_000740A0`), `0x73E4E` (`FUN_00073E28`), `0x73D2C`
  (`FUN_00073D10`).
* READ `0x740A0`, `0x8D178` (`FUN_0008D098`), `0x8316E`.

No `MOV byte[0x157A4D],imm8` (`C6 05 4D 7A 15 00`) and no other
`MOV [0x157A4D],AL` (`A2 4D 7A 15 00`) exists. **Only three writers.**

### 4.2 The `[rec+0x1C]` walk (`FUN_0006D920` + `FUN_0008D098`)

`FUN_0006D920` (fresh decompile) ends
`*(…+0x1C) = (&PTR_LAB_00110794)[ram0x00157a4a >> 0x18];` — i.e.
`[rec+0x1C] = phase_table[byte 0x157A4D]`. `FUN_0008D098` (the 1928-byte
per-record driver) then does `FUN_0006d920(rec); (*(code**)(rec+0x1C))(rec, rec+0x4D);`
for each non-hidden record (11 records, `0xB2` stride), inside a
`switch (DAT_00157a4d)` on the situation byte. `FUN_0008D098` is called only
from `FUN_000740A0` (`0x740C8`/`0x740DB`).

`FUN_000740A0` (fresh decompile) is the situation writer/entry:
`DAT_00157a4e = DAT_00157a4d; ram0x00157a4a = CONCAT13(param_1, …); … FUN_0008d098(...)`.
`get_xrefs_to 0x740A0` = **27**.

### 4.3 The act invoker `FUN_000888FC` (fresh)

Fresh decompile:
```c
void FUN_000888fc(short act, u8 stage, short invoke) {
  if (act != [0x158828]) {                      /* latch */
    [0x158828] = act;
    [0x158808] = (*(code**)(&DAT_001107EC + act*4));  /* act table */
    [0x158818] = 0; [0x158829] = stage;
    if (invoke != 0 && [0x158808] != 0) (*[0x158808])();
  }
}
```
`get_xrefs_to 0x888FC` = **11** sites; fresh act args: `0x88EE8`→4, `0x8A6FE`→3,
`0x8A789`→6, `0x8AB57`→4, `0x8AB8E`→0xA, `0x8ABCF`→1, `0x8AEBA`→7,
`0x8AED2`→2, `0x8AEEA`→2, `0x8BAE7`→0xB, plus `0x8AF1A`→9. **No site passes act 5.**

## 5. The exhaustive installer census

### 5.1 All 77 `FUN_0007D9A4` call sites (fresh, code per site)

`R*` = register-derived compound; `D` = derived from a stack param.

| # | site | container | code(s) |
|---|---|---|---|
| 1 | `6DCBC` | `FUN_0006DC88` | 7 |
| 2 | `75B67` | (0x75B·) | 5 |
| 3 | `762A4` | `FUN_00076130` | 1C |
| 4 | `7634B` | `FUN_00076130` | 1B/1C (`ADD EAX,0x1B`) |
| 5 | `7655C` | (row-1F body) | 1B |
| 6 | `766B4` | (row-1B body) | D (EDX preserved from `0x7DAB4`) |
| 7 | `77719` | (row-1C body) | 19 |
| 8 | `78236` | `FUN_00077EAC` | **1A** |
| 9 | `7825E` | `FUN_00077EAC` | **1A** |
| 10 | `78285` | `FUN_00077EAC` | **1A** |
| 11 | `782A2` | `FUN_00077EAC` | 1B |
| 12 | `782BF` | `FUN_00077EAC` | 1C/4 |
| 13 | `78576` | `FUN_000782D0` | 19/4 |
| 14 | `79AAC` | `FUN_0007997C` | **3**/19 (`[rec+0x8D]`) |
| 15 | `7CA48` | `FUN_0007C990` | 3/4/6 |
| 16 | `7CD24` | `FUN_0007CA54` | 8 |
| 17 | `7CE1A` | `FUN_0007CDD8` (**xref-dead**) | **0A** |
| 18 | `7CE93` | (0x7CDD8 cluster) | 0B |
| 19 | `7CF05` | (0x7CDD8 cluster) | 8 |
| 20 | `7CFB2` | (0x7CDD8 cluster) | 9 |
| 21 | `7D046` | `FUN_0007D054` | 21 |
| 22 | `7D149` | `FUN_0007D110` | 7 |
| 23 | `7D1B9` | `FUN_0007D174` | 23 |
| 24 | `7DB03` | `FUN_0007DAB4` | 0 |
| 25 | `7DBA3` | `FUN_0007DAB4` | 19/3 |
| 26 | `7E05A` | (row-02 body) | 4 |
| 27 | `7E5C9` | `FUN_0007E528` | 7 |
| 28 | `7E7B2` | `FUN_0007E600` | 0E |
| 29 | `7E85C` | (row-04 body) | 4 |
| 30 | `7E8CA` | (row-04 body) | 19 |
| 31 | `7EB5C` | (row-04 body) | 0F |
| 32 | `7ED6A` | (row-04 body) | 0B |
| 33 | `7EF33` | (row-04 body) | 7 |
| 34 | `7F120` | (row-05 body) | 6 |
| 35 | `7F133` | (row-05 body) | 5 |
| 36 | `7F64D` | `FUN_0007F6F0` | 4 |
| 37 | `7F791` | `FUN_0007F6F0` | 21 |
| 38 | `7F7B6` | `FUN_0007F6F0` | 21 |
| 39 | `7F928` | `FUN_0007F7E0` | 7 |
| 40 | `7F970` | `FUN_0007F7E0` | 7 |
| 41 | `80190` | `FUN_0007F7E0` | 7 |
| 42 | `8025E` | (row-06 body) | 4 |
| 43 | `807C1` | (row-06 RNG arm) | D (~9) |
| 44 | `80EFA` | (row-09 body) | 0C |
| 45 | `80F6A` | (row-09 arm 1) | **0D** |
| 46 | `816DF` | (row-07 body) | **22** |
| 47 | `81722` | (row-07 body) | 4 |
| 48 | `81BEB` | (row-0B body) | 0C |
| 49 | `83145` | (row-23 body) | 0F |
| 50 | `82F70` | `FUN_00082DD0` | 0E |
| 51 | `844E7` | (row-12 body) | 4 |
| 52 | `84DAB` | (row-16 body) | 13 |
| 53 | `85383` | (row-21 body) | 4 |
| 54 | `85CA1` | (row-10 body) | 4 |
| 55 | `86436` | (row-11 body) | 4 |
| 56 | `8753C` | (row-28 body) | **3** |
| 57 | `89740` | (act-4 stage 0) | **17** |
| 58 | `8990A` | (act-7 body) | 19 |
| 59 | `8994C` | (act-7 body) | **3** |
| 60 | `899AB` | (act-7 body) | 20 |
| 61 | `899CD` | (act-7 body) | 1F |
| 62 | `8A0A0` | (act-3 body) | 16 |
| 63 | `8A32F` | (act-4 body) | 18 |
| 64 | `8B750` | (act-0xA builder) | 24 |
| 65 | `8B78B` | (act-0xA builder) | 24 |
| 66 | `8CF3F` | `FUN_0008CEB8` | **param4** ∈ {0,3,15,19,25,26,28} |
| 67–77 | `8D200`,`8D238`,`8D291`,`8D349`,`8D421`,`8D475`,`8D546`,`8D569`,`8D5C7`,`8D62C`,`8D807` | `FUN_0008D098` | 1,2,4,10,11,14,13,1F,12,1D/1E,2A |

Count: 1–65 (65) + 66 + 11 (67–77) = **77**. ✓

### 5.2 The literal encoding scan (NULL codes)

`search_byte_patterns` for each NULL action code in the three direct encodings.
Default-space (non-`.image`) hits only; `.image` hits are overlay false
positives (FU-189 §6.2):

| code | `BA<cc>000000` | `66BA<cc>00` | `C7C2<cc>000000` | installer-site hits (default) |
|---|---|---|---|---|
| 03 | many | none | none | `0x79A9E`(via B8), `0x7CA2F`, `0x87531`, `0x89941`, `0x8ABE7`(via EAX) |
| 0A | `0x7CE11` | none | none | **`0x7CE11`** (dead container) |
| 0D | `0x80F61` | none | none | **`0x80F61`** |
| 15 | many | none | none | none with `BA` (parametric only, ECX) |
| 17 | `0x89736` | none | none | **`0x89736`** |
| 1A | `0x78227`,`0x7824F`,`0x78277` | none | none | **3 sites** |
| 22 | `0x816D6` | none | none | **`0x816D6`** |
| 25 | `0x1F527`,`0x22033`,`0x7E460`,`0xA9B41` | none | none | **none** (FU-189 re-verified) |
| 27 | `0x1F52E`,`0x756D5` | none | none | **none** |
| 29 | `0x1F53C` | none | none | **none** |
| 2B | `0x1F535` | none | none | **none** |
| 2C | `0x3035E`,`0x40A8E`,`0x40C5C` | none | none | **none** |

`66 BA` and `C7 C2` direct forms occur **nowhere** for these codes (and none
at any of the 77 sites). The literal scan therefore confirms `25/27/29/2B/2C`
have **no direct immediate installer** — but `15` and `25` have a
**parametric** installer (`FUN_0008CEB8`, code in ECX) that the scan misses.

### 5.3 Phase value writers (fresh, `FUN_000740A0` 27 callers + 2 resets)

EAX arg (the new situation byte `0x157A4D`) per `FUN_000740A0` caller:

```
88FB8=15  89372=0A  895AC=D(EBX)  898DD=10  8A091=0F
93DB2=0  94040=0  941FF=0  944A3=0  94681=0  94877=0
87C71=1   8AA89=0   88E82=1   8915B=0A   8972F=0B   88C82=0C
8AB9F=11  8ABE7=3   8AC1C=4   8ADB4=5    8ADCC=8    8ADE4=9
8ADFC=0D  8AF02=2   8BAC1=0C  8BCCF=13
```

Value set written: **{0,1,2,3,4,5,8,9,A,B,C,D,F,10,11,13,15}** plus one derived
(`0x895AC`, from a stack param). **Not written by any of the 27:** `6, 7, E,
12, 14, 16`. `FUN_00073E28`/`FUN_00073D10` are resets (write 0). The
`MOV EAX,0x12`/`MOV EAX,0x14` at `0x88CD8`/`0x88E0E` are `CALL 0x4C374`
(`FUN_0004C374` = `[0x157A68]=0`, EAX ignored) — **not** situation writes
(the phase-1E `0x4C374` decoy, FU-194/FU-212).

## 6. Per-row installer verdict (the 24 NULL rows)

Verdict vocabulary: **live** (installer reachable from a wired engine path),
**unported** (installer exists; its producer/container is not ported),
**gated** (installer reachable but its gate is structurally false in-engine),
**tape-visible** (installer reachable and would move the tape), **absent**
(no installer exists after the exhaustive scan), **deliberate** (withheld by
design), **leg** (writer/invoker unresolved).

### 6.1 Action rows

| row | installer chain (fresh) | verdict |
|---|---|---|
| **03** | `FUN_0007997C 0x79AAC` (dx=19 if `[rec+0x8D]==0` else 3) ← `FUN_0008C33C` (`0x8C34A`, called from `FUN_0008D098 0x8D0BA` + match-init `0x4B0AC`); also `FUN_0007C990 0x7CA48`, `FUN_0007DAB4 0x7DBA3`, `0x8753C`, `FUN_0008CEB8` param-3 (`0x8CF3F`) | **tape-visible** — live installer; engine withholds (M2 moved at frame 29, FU-177 R7) |
| **0A** | only site `0x7CE1A` (`MOV EDX,0xA`) inside function entry `0x7CDD8`, which has **0 xrefs** (no caller, no jump-table/data ref) and follows a `RET` at `0x7CDD5` | **absent** — dead installer (prior "0x7CDD8 has no xrefs" re-confirmed) |
| **0D** | only site `0x80F6A` (`MOV EDX,0xD`) inside row-09 arm 1 (`0x80BFB..0x80FCB`) | **unported** (row-09 arm 1 = FU178-L1) |
| **15** | `FUN_0008CEB8(rec,1,10,0x15,-1)` at `0x8D45C` inside `FUN_0008D098` case 5 (`situation==5`) | **unported** (body "head mis-decoded" OL-14; parametric installer freshly located) |
| **17** | `0x89740` (`MOV EDX,0x17`) act-4 stage 0 (`0x89620`); act-4 invokers `0x8AB57` (sit-2..4 arm) + `0x88EE8` | **gated** (sit-2..4 formation-order residual is 0, FU-201 R49) |
| **1A** | `FUN_00077EAC 0x78236/0x7825E/0x78285` (`MOV EDX,0x1A`) ← `FUN_000782D0 0x78599` ← `FUN_0008D8EC 0x8DB35` (live at match-init `0x4B2AB`) | **unported** (CPU-ladder pre-pass) |
| **22** | `0x816DF` (`MOV EDX,0x22`) row-07 body | **gated** (opponent angle inputs unmodelled OL-65, FU185-L6) |
| **25** | `FUN_0008CEB8(rec,0,10,0x25,-1)` at `0x8D79F`/CALL `0x8D7AD` inside `FUN_0008D098` case `0x13` (tie-break `word[0x157AC5] != word[0x157AC7]`) | **unported** — **prior "no installer" verdict refuted** (parametric route) |
| **27** | none (literal scan + parametric code set) | **absent** |
| **29** | none (action-table slot `0x110784` is the only body ref) | **absent** |
| **2B** | none (`0x87738` = shared row-29 RET) | **absent** |
| **2C** | none (`0x84598` body; slot is the only ref) | **absent** |

### 6.2 Phase rows

| phase | writer / invoker (fresh) | verdict |
|---|---|---|
| **01** | situation 1 written at `0x88E82`/`0x87C71`; body `0x6E1D0` = formation place, already realized by the seeding path | **gated** (double-wire; engine keeps NULL) |
| **0B** | only writer `0x8972F` (`MOV EAX,0xB`) inside the act-4 body (`0x89620`) | **unported** |
| **0D** | only writer `0x8ADFC` (`MOV EAX,0xD`) inside the act-4 body | **unported** |
| **0E** | no writer in the 27 `FUN_000740A0` callers (`FUN_0008D098` handles `situation 0x0B/0x0E` cases but nothing writes 0x0E) | **absent** |
| **11** | situation `0x11` written at `0x8AB9F` (FUN_0008A938); body `0x6E1C8` = 8-byte shim into `0x6E1D0` | **gated** (double-wire, FU192-L6/L7) |
| **12** | no writer in the 27 callers | **absent** |
| **13** | situation `0x13` written at `0x8BCCF` (`MOV EAX,0x13`, `FUN_0008BAF0`); body `0x6E244` consumes RNG `0x92AC8` ×2 + `0x10F364/68` | **gated** (unported scatter producer, FU192-L2) |
| **14** | no writer located in the 27 callers (engine asserts on-tape 40 frames) | **leg** (writer unresolved — OL-242-L1) |
| **1A** | act-4 body; invoker `0x8AB57`/`0x88EE8` | **gated** (no live act-4 producer, FU-201 R49) |
| **1B** | act-5 body; `get_xrefs_to 0x888FC` = 11, **none passing EAX=5** | **absent** (no invoker, FU195-L1 re-verified) |
| **1C** | act-6 body; invoker `0x8A789` (`MOV EAX,6`) exists in `FUN_0008A43C` | **unported** (engine does not dispatch act 6, FU193-L4) |
| **1E** | act-8 body; compressed model retained (render-invisible, tape-dormant) | **deliberate** (FU212-L1) |

### 6.3 Counts

* **tape-visible: 1** (action `03`)
* **gated: 6** (action `17`,`22`; phase `01`,`11`,`13`,`1A`)
* **unported: 7** (action `0D`,`15`,`1A`,`25`; phase `0B`,`0D`,`1C`)
* **absent: 8** (action `0A`,`27`,`29`,`2B`,`2C`; phase `0E`,`12`,`1B`)
* **deliberate: 1** (phase `1E`)
* **leg: 1** (phase `14`, writer unresolved)

Total **24**. Action NULL: 4 unported, 2 gated, 1 tape-visible, 5 absent.
Phase NULL: 3 unported, 4 gated, 3 absent, 1 deliberate, 1 leg.

## 7. Open legs (numbered, with owners)

* **OL-242-L1** (owner: this slice / FU-242 follow-up). The native writer for
  phases **06/07/0E/12/14** is not among the 27 `FUN_000740A0` callers nor the
  `FUN_0008A938` arms; only 3 writers exist for `0x157A4D`. Phases `06`/`07`
  are nonetheless counted (`fn != NULL`), and the engine asserts phase `14`
  on-tape — so either an indirect writing path (not `MOV [0x157A4D]`-shaped)
  or a mis-modelled `0x157A4A` write exists. Unprovable at recon level.
* **OL-242-L2** (owner: FU-189/FU-142 follow-up). The prior **row-25** verdict
  ("no installer exists") is refuted at the raw installer level by the
  parametric `FUN_0008CEB8(rec,0,10,0x25,-1)` path (`FUN_0008D098` case
  `0x13`). Whether `case 0x13` is reachable on the engine's wired situation
  machine is a separate question (the phase-0x13 writer is `0x8BCCF`); the
  row-25 *body* (`0x880CC`) is still unported, so the row stays NULL — but the
  reason must be "body unported / route gated", not "no installer".
* **OL-242-L3** (owner: FU-186 follow-up). The same parametric blind spot
  applies to row **15** (`FUN_0008CEB8` param `0x15`); the prior "stub bucket"
  classification is incomplete.
* **OL-242-L4** (owner: FU-177/FU-198 follow-up). The row-03 installer chain
  (`FUN_0007997C` ← `FUN_0008C33C`) is native-live and has **six** site
  variants (incl. `FUN_0008CEB8` param-3); a future re-arm must reproduce the
  `[rec+0x8D]` code choice (19 vs 3) and the index-0 `3→19` special case.
* **OL-242-L5** (owner: this slice). The `0x7CDD8` cluster (`0x7CDD8..0x7CF1F`)
  contains four `FUN_0007D9A4` sites (codes `0A`,`0B`,`8`,`9`) in
  Ghidra-**unanalysed** code; `0x7CDD8` has zero xrefs. Whether the cluster is
  reached by any computed dispatch (the `FUN_000782D0` handler tables at
  `0x110950` do **not** list it) is unresolved; the four site codes are
  documented but their reachability is not proven.
* **OL-242-L6** (owner: FU-213/FU-208 follow-up). The 27-caller value set omits
  `0x895AC`'s EBX source (a stack param) — a fully derived situation write
  whose possible values are unbounded by the immediate scan.

## Provenance

* Every address/byte above is a fresh first-hand Ghidra MCP read (program
  `/FIFA96.EXE`, case-sensitive) taken this slice: `read_memory`,
  `get_xrefs_to`, `disassemble_bytes`, `decompile_function`,
  `search_byte_patterns`, `get_function_by_address`.
* Counts asserted first-hand: `FUN_0007D9A4` xrefs **77**;
  `FUN_000888FC` xrefs **11**; `FUN_000740A0` xrefs **27**; `FUN_0008CEB8`
  xrefs **15**; `0x157A4D` xrefs **6** (3 W / 3 R); `0x157A4A` xrefs **128**.
* Tables `0x1106E0`/`0x110794`/`0x1107EC` read fresh and decoded above.
* No file outside this slice's draft `w23-3-installer-census.md` and
  `task-3-report.md` was written; Ghidra state untouched; ISO / `game/hdd/`
  untouched. Prior doc sections untouched (errata land here as quoted text).
