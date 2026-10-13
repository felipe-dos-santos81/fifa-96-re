# FU-215 — the sit-8 arming writer: the bit-8 range-word gate is structurally dead (phase-20 wave R63)

Provenance: M2 phase-20 wave **R63**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`,
max `.image::00174a2a`, **2706** functions, **15239** symbols, image size
2757479, `.image` overlay present). Every address/byte/instruction/xref-count
below is a **fresh** `read_memory` / `disassemble_bytes` / `disassemble_function`
/ `get_xrefs_to` / `get_function_by_address` / `search_instructions` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). The alias
traps are applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at
`x+3`); LE globals sit at `+0x100000` from the file offset; addresses are
`/FIFA96.EXE` runtime addresses. Engine/loader source read first-hand. **No
writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (R63 brief): extend **FU-208 / FU-214** by settling the **sit-8 arming
writer** — (1) the two range-word cells first-hand (widths, adjacency, every
reader and writer); (2) **is there any instruction that sets bit 8 (`0x100`)** of
either word? follow the whole-word writer chain if not; (3) when the hit is
armed (the native timeline, cross-citing both sit-8 producers); (4) the exact
engine extension point, and whether it is fixture-able on the M2 tape. Closes
**FU208-L1** and its sibling **FU214-L2**. Extends FU196 (§3/§6), FU194 (§3),
FU206, FU208, FU209, FU213, FU214.

**Answer in one line.** **There is no bit-8 writer.** No instruction anywhere in
the image sets bit 8 (`0x100`) of `[0x14C1D4]` or `[0x14C1D6]`: the two words are
written only by **`FUN_0001C9BC`** (zero + config-driven ORs of bits
`{0,1,2,3,4,5,6,7,9}`) and **`FUN_00024B00`** (whole word `:= 0`), and the only
indexed (table) writers that reach the words' struct (`FUN_00011620` / `FUN_00011978`,
base `0x14C1CC`) are confined to indices `{0,1}` — i.e. to the **adjacent**
words `0x14C1CC`/`0x14C1CE`, never `0x14C1D4`/`0x14C1D6` (index 4/5). So the
`TEST AH,0x1` arm at `0x4B0CB` is **structurally dead**, and native situation 8 is
armed **only** by the period-index-4 producer **`FUN_00088860`** `0x888C8`
(`[0x157AC2] == 4` at a phase-`0xC` period end). The engine already models exactly
this (`fifa96_match_run.c:2650` → `period_setup(mode 0)` → `:3161`) and never sets
`input_range & 0x100`; **no producer is to be added**, and the census does not
move. FU208-L1 / FU214-L2 close as *"dead gate — nothing to fake"*.

---

## 1. The two range-word cells (fresh)

### 1.1 Cells, widths, adjacency

Fresh `read_memory 0x14C1D0` (16 B) and `read_memory 0x14C1D4` (32 B): all bytes
**zero** (`hex 0000…`) at file-load (the two words are runtime-built, §1.3).

The sit-8 gate reads them as **two adjacent `uint16`**:

```
0x4B0BA XOR EAX,EAX
0x4B0BC MOV CX, word ptr [0x0014c1d4]   ; bytes 66 8b 0d d4 c1 14 00
0x4B0C3 MOV AX, [0x0014c1d6]            ; bytes 66 a1 d6 c1 14 00
0x4B0C9 OR  EAX,ECX                     ; bytes 09 c8
0x4B0CB TEST AH,0x1                     ; bytes f6 c4 01   <<< bit 8 (0x100)
0x4B0CE JZ  0x0004b0e5                  ; bytes 74 15
0x4B0D0 MOV EAX,0x8                     ; 0x0B0D0.. (sit 8 side 0, BX 0)
0x4B0D5 XOR EBX,EBX
0x4B0D7 XOR EDX,EDX
0x4B0D9 CALL 0x0008a938
0x4B0DE MOV byte ptr [0x00157ac2],0x4   ; period index := 4
```

(fresh `disassemble_bytes 0x4B090`, 112 B). So **word0 = `[0x14C1D4]` (side 0)**,
**word1 = `[0x14C1D6]` (side 1)**, each 2 bytes, contiguous (`0x14C1D4`/`0x14C1D5`
= word0; `0x14C1D6`/`0x14C1D7` = word1). `TEST AH,1` is unambiguously **bit 8 of
the OR** (`AH = EAX[15:8]`, `AH` bit 0 = `EAX` bit 8 = `0x100`).

### 1.2 Every writer (fresh `search_instructions` over the whole program, 235 017 instructions)

A program-wide `search_instructions` on the operand substrings `14c1d4`,
`14c1d5`, `14c1d6`, `14c1d7` returns the complete literal-reference set (21 / 1 /
12 / 1 — matching the fresh `get_xrefs_to` counts 21 / — / 12 / —). The
**write** sites are:

| # | site | function | instruction | bytes | effect |
|---|---|---|---|---|---|
| W1 | `0x1C9CD` | `FUN_0001C9BC` | `MOV word[0x14C1D6],DX` (DX=0) | `66 89 15 d6 c1 14 00` | word1 := 0 |
| W2 | `0x1C9D4` | `FUN_0001C9BC` | `MOV word[0x14C1D4],DX` (DX=0) | `66 89 15 d4 c1 14 00` | word0 := 0 |
| W3 | `0x1C9E3` | `FUN_0001C9BC` | `MOV word[0x14C1D4],0x2` | `66 c7 05 d4 c1 14 00 02 00` | word0 bit 1 |
| W4 | `0x1C9F5` | `FUN_0001C9BC` | `OR byte[0x14C1D4],0x4` | `80 0d d4 c1 14 00 04` | word0 bit 2 |
| W5 | `0x1CA05` | `FUN_0001C9BC` | `OR byte[0x14C1D4],0x1` | `80 0d d4 c1 14 00 01` | word0 bit 0 |
| W6 | `0x1CA15` | `FUN_0001C9BC` | `OR byte[0x14C1D5],0x2` | `80 0d d5 c1 14 00 02` | **word0 bit 9** (`0x200`) |
| W7 | `0x1CA25` | `FUN_0001C9BC` | `OR byte[0x14C1D7],0x2` | `80 0d d7 c1 14 00 02` | **word1 bit 9** (`0x200`) |
| W8 | `0x1CA3F` | `FUN_0001C9BC` | `OR byte[0x14C1D4],0x20` | `80 0d d4 c1 14 00 20` | word0 bit 5 |
| W9 | `0x1CA48` | `FUN_0001C9BC` | `OR byte[0x14C1D4],0x80` | `80 0d d4 c1 14 00 80` | word0 bit 7 |
| W10 | `0x1CA51` | `FUN_0001C9BC` | `OR byte[0x14C1D4],0x40` | `80 0d d4 c1 14 00 40` | word0 bit 6 |
| W11 | `0x1CA5A` | `FUN_0001C9BC` | `OR byte[0x14C1D4],0x8` | `80 0d d4 c1 14 00 08` | word0 bit 3 |
| W12 | `0x1CA74` | `FUN_0001C9BC` | `OR byte[0x14C1D6],0x20` | `80 0d d6 c1 14 00 20` | word1 bit 5 |
| W13 | `0x1CA7D` | `FUN_0001C9BC` | `OR byte[0x14C1D6],0x80` | `80 0d d6 c1 14 00 80` | word1 bit 7 |
| W14 | `0x1CA86` | `FUN_0001C9BC` | `OR byte[0x14C1D6],0x40` | `80 0d d6 c1 14 00 40` | word1 bit 6 |
| W15 | `0x1CA8F` | `FUN_0001C9BC` | `OR byte[0x14C1D6],0x8` | `80 0d d6 c1 14 00 08` | word1 bit 3 |
| W16 | `0x1CA9F` | `FUN_0001C9BC` | `OR byte[0x14C1D4],0x10` | `80 0d d4 c1 14 00 10` | word0 bit 4 |
| W17 | `0x1CAAF` | `FUN_0001C9BC` | `OR byte[0x14C1D6],0x10` | `80 0d d6 c1 14 00 10` | word1 bit 4 |
| W18 | `0x24B91` | `FUN_00024B00` | `MOV word[0x14C1D6],DX` (DX=0) | `66 89 15 d6 c1 14 00` | word1 := 0 |
| W19 | `0x24B98` | `FUN_00024B00` | `MOV word[0x14C1D4],BX` (BX=0) | `66 89 1d d4 c1 14 00` | word0 := 0 |

**Exactly two functions write the range words.** The complete `FUN_0001C9BC`
body (fresh `disassemble_function 0x1C9BC`, 54 instructions, ends `POP EDX; RET`)
targets, per word:

* **word0 bits `{0,1,2,3,4,5,6,7,9}`** — never bit 8 (`0x100`);
* **word1 bits `{3,4,5,6,7,9}`** — never bit 8.

`FUN_00024B00`'s `ESI==2` arm (fresh `disassemble_bytes 0x24B60`, 64 B):
`0x24B66 MOV EAX,4 … 0x24B80 MOV EBX,EAX / 0x24B8D XOR EDX,EDX / 0x24B8F XOR
EBX,EAX` ⇒ BX = 4 XOR 4 = **0**, DX = 0, then W18/W19 write **both words zero**.

`FUN_0001C9BC` callers (fresh `get_function_xrefs 0x1C9BC` = **2**):
`FUN_00011B7C` `0x11BA6`, `FUN_000115A0` `0x115BD`. `FUN_00024B00` callers
(fresh = **3**): `FUN_00020954` `0x2098B`, `FUN_00018680` `0x18901`,
`FUN_00032DE0` `0x32DF8`.

### 1.3 The words are part of a 646-byte per-team struct at `0x14C1CC`

Both `FUN_00011B7C` (`0x11B93..0x11B9F`) and `FUN_000115A0`
(`0x115A8..0x115B4`) **zero a 646-byte block at `0x14C1CC` immediately before
calling `FUN_0001C9BC`**:

```
0x11B93 MOV ECX,0x286            ; count = 646
0x11B98 MOV EDI,0x14c1cc         ; dest = 0x14C1CC
0x11B9D XOR EAX,EAX
0x11B9F STOSB.REP ES:EDI         ; REP STOSB -> zero 0x14C1CC..0x14C452
0x11BA6 CALL 0x0001c9bc
```

(identical block at `0x11B93` / `0x115A8`). `0x14C1CC + 0x286 = 0x14C452`. The
range words are **struct offset +8**: `0x14C1CC + 8 = 0x14C1D4` (word0) and
`+0xA = 0x14C1D6` (word1). The same struct is written by the roster/match-setup
routine `FUN_00011978` (`0x14C1E4`, `0x14C334`, `0x14C344…0x14C350`,
`0x14C3B0`, `0x14C360`, …) and read by `FUN_00028588` (`0x2891F
MOV EDX,dword[0x14C1CC]`).

### 1.4 Every reader (fresh `get_xrefs_to`)

**Word0 `0x14C1D4` — 21 refs.** Literal readers (read both words, OR, test a
bit):

| site | function | test | role |
|---|---|---|---|
| `0x4B0BC` | `FUN_0004B02C` | `TEST AH,1` = **bit 8** | the sit-8 gate (§1.1) |
| `0x73A07`/`0x73A0E` | `FUN_000736AC` | `TEST AL,2` = bit 1 | interpolation arm (jump `0x73AB4`) |
| `0x70AB1`/`0x70AB8` | `FUN_000709D0` | `TEST AL,4` = bit 2 | pan-step random walk (jump `0x70B86`/`0x70544`) |
| `0x718D0`/`0x718D6` | `FUN_0007131C` | (OR only, `XOR AH,AH`) | camera/entity use |
| `0x742F5`/`0x742FC` | *(no function)* | — | the period-end family body (`0x74244`-context) |

**Word0 indexed readers** — `MOV AX, word ptr [EAX*2 + 0x14c1d4]` (fresh xref
type `DATA`), at `0x77EDE` (`FUN_00077EAC`), `0x7A6A9` (`FUN_0007A490`),
`0x7B41F` (`FUN_0007B194`), `0x7B8F9` (`FUN_0007B878`), `0x7BD19`
(`FUN_0007B9C4`), `0x8C7A6` (`FUN_0008C758`). The index is **side (0/1)**:
fresh `disassemble_bytes 0x741B4` (the mapper they call) is
`MOV DL,[0x157ABE]; XOR EAX,EDX; AND EAX,1; RET` — i.e. `(EAX ^ [0x157ABE]) & 1`,
**always 0 or 1**. So the six indexed reads are per-side element `[side]`, never
index 4/5.

**Word1 `0x14C1D6` — 12 refs.** Literal readers: `0x4B0C3` (`FUN_0004B02C`),
`0x70AB8` (`FUN_000709D0`), `0x718D6` (`FUN_0007131C`), `0x73A0E`
(`FUN_000736AC`), `0x742FC` (no function). The same six indexed reads cover word1
at index 1.

**Verdict for §1.** Exactly two whole-word writers (`FUN_0001C9BC`,
`FUN_00024B00`); no other instruction in the 235 017 scanned writes either word,
its low byte, or its high byte with any bit other than the table of §1.2. **No
`OR`/`BTS`/`MOV`/`AND`-clear sets bit 8 of either word.**

---

## 2. The bit-8 write — the word's own writer chain (fresh)

### 2.1 No direct bit-8 writer

Bit 8 (`0x100`) is the high byte (`0x14C1D5` / `0x14C1D7`) bit **0**. The
**only** high-byte writes in the whole program are W6 (`OR byte[0x14C1D5],0x2`
= bit **9**, `0x200`) and W7 (`OR byte[0x14C1D7],0x2` = bit **9**), both in
`FUN_0001C9BC` (fresh `search_instructions` on `14c1d5` = **1** match; on
`14c1d7` = **1** match). `FUN_0001C9BC` never emits `OR byte[0x14C1D5],0x1` or
`OR byte[0x14C1D7],0x1`; its only whole-word writes are zero (W1/W2). So
`FUN_0001C9BC` cannot set bit 8 for **any** config input.

### 2.2 The whole-word writer chain, followed to its origin

`FUN_0001C9BC` (`0x1C9BC`) gates on `CALL 0x6D1B2; CMP EAX,1; JNZ 0x1CAB6` and
reads a fixed set of **config cells**, mapping each to a range-word bit:

| cell | condition | word / bit |
|---|---|---|
| `[0x10527C]` | `!= 0` | word0 bit 0 (`0x1`) |
| `[0x105278]` | `== EAX` (EAX==1) | word0 bit 1 (`0x2`) |
| `[0x105278]` | `== 2` | word0 bit 2 (`0x4`) |
| `[0x10528C]` | `1/2/3/4` | word0 bits 5/7/6/3 |
| `[0x105290]` | `== 1` | word0 bit 4 (`0x10`) |
| **`[0x105288]`** | `== 1` | **word0 bit 9 (`0x200`)** |
| **`[0x10529C]`** | `== 1` | **word1 bit 9 (`0x200`)** |
| `[0x1052A0]` | `1/2/3/4` | word1 bits 5/7/6/3 |
| `[0x1052A4]` | `== 1` | word1 bit 4 (`0x10`) |

Fresh `get_xrefs_to` of every one of these cells = **reads only, all inside
`FUN_0001C9BC`** (`0x105288` = 1, `0x10529C` = 1, `0x10527C` = 1, `0x105290` = 1,
`0x1052A0` = 1, `0x1052A4` = 1; `0x105278` = 2). The bit-9 cells
`0x105288`/`0x10529C` are the "high byte := 2" arms — the **bit-9** config that
the phase-19 docs already found. **The bit-8 slot in the mapping is simply
absent.**

The config block itself (`0x105270`-family) is written by the roster/setup
machinery, not by the sit-8 route: fresh `search_instructions 105270` =
`FUN_000156B0` `0x15835` (read), **`FUN_00024B00` `0x257CA MOV [0x105270],EAX`**
(write, only when `[0x105474]==5 && [0x105478]==4 && [0x10547C]==9`); and the
block is accessed via a base register (`FUN_0001C728` `0x1C934/0x1C941/0x1C94B
MOV dword[EDX+0x105274],…`; `FUN_0001C5D4` reads `[EAX+0x105274]`; `FUN_0001CAEC`
compares `[0x105274]` to 1/2/3). At file-load all of `0x105270..0x1052B0` is
**zero** (fresh `read_memory 0x105270`, 64 B) — the block is a runtime-edited
front-end options block on a base-register index. Its own producer chain is a
front-end leg (§6, FU215-L1) — **but even an arbitrary config value cannot set
bit 8**, because the mapping has no bit-8 arm.

### 2.3 The adjacent table's indexed writers do not reach the words

The struct base `0x14C1CC` has two indexed writers:

* `FUN_00011620` `0x1175F`: `MOV word ptr [EDX*2 + 0x14c1cc], SI` (bytes
  `66 89 34 55 cc c1 14 00`), index = its `EAX` argument.
* `FUN_00011978` `0x119C2`: `MOV word ptr [EAX*2 + 0x14c1cc], 0xffff` (bytes
  `66 c7 04 45 cc c1 14 00 ff ff`), index = its `EAX` argument.

Both **would** set bit 8 (`0xFFFF` explicitly; the `SI` value at runtime) **iff**
their index were 4 or 5 (`0x14C1CC + 4*2 = 0x14C1D4`, `+ 5*2 = 0x14C1D6`). Fresh
`search_instructions 14c1cc` yields exactly 5 sites and every caller passes
**index 0 or 1**:

* `FUN_00011978` — fresh `get_function_xrefs` = **5**, all in `FUN_00024B00`:
  `0x25B15` (EAX ← `XOR EAX,EAX` = 0), `0x25B39`/`0x25BE7`/`0x25C1B`/`0x25C4C`
  (EAX ← `MOV EAX,0x1` = 1). **Every argument is 0 or 1.**
* `FUN_00011620` — fresh `get_function_xrefs` = **5**: `FUN_00011B7C` `0x11BB6`
  (EAX ← `XOR EAX,EAX` = 0), `0x11BC5` (EAX ← `MOV EAX,1` = 1), and `FUN_00024B00`
  `0x25B4C`/`0x25B6B`/`0x25C64` (EAX = 0 or 1, per the same `MOV EAX,1` arms).
  **Every argument is 0 or 1.**

So the indexed writes land on `0x14C1CC` (index 0) and `0x14C1CE` (index 1) —
**the two words immediately before the range words** — never on
`0x14C1D4`/`0x14C1D6`. There is **no** `MOV word ptr [reg*2 + 0x14c1d4], …`
anywhere (fresh `search_instructions 14c1d4` = the 21 refs of §1.2, all reads or
the W2/W3/W18/W19 direct writes).

### 2.4 Verdict

**The bit-8 writer does not exist statically and is not an indirect/table
write.** The range words are written only by `FUN_0001C9BC` (config→bits
`{0,1,2,3,4,5,6,7,9}`) and `FUN_00024B00` (`:= 0`); the indexed table writers
target the adjacent per-team struct words at index 0/1. Therefore the `TEST AH,1`
arm at `0x4B0CB` (FU208's "producer A") **never fires**, and the disjunction that
arms situation 8 reduces to its **second** term: `[0x157AC2] == 4` at a phase-`0xC`
period end (producer B). This is the honest settlement of **FU208-L1 = FU214-L2**:
not "an unlocated producer" but a **dead gate** — do not synthesize a bit-8
writer.

---

## 3. When the hit is armed — the native timeline (fresh)

There are exactly **two** sit-8 producers (fresh `get_xrefs_to 0x8A938` = **39**;
the two `EAX=8` sites are `0x4B0D9` and `0x888C8`, cross-confirmed):

### 3.1 Producer A — the bit-8 gate (`FUN_0004B02C`) — **never armed**

`FUN_0004B02C` (the match-init / period-setup machine; fresh `get_xrefs_to
0x4B02C` = 2, both inside `FUN_000493A0` at `0x49415`/`0x4950A`) runs its common
tail on **every** match-init / period transition:

```
0x4B0B8 XOR ECX,ECX / XOR EAX,EAX
0x4B0BC MOV CX,word[0x14C1D4]
0x4B0C3 MOV AX, [0x14C1D6]
0x4B0C9 OR  EAX,ECX
0x4B0CB TEST AH,0x1
0x4B0CE JZ  0x4B0E5                ; <- ALWAYS TAKEN (bit 8 never set, §2)
0x4B0D0 MOV EAX,8 / ... / 0x4B0D9 CALL 0x8A938   ; dead
0x4B0DE MOV byte[0x157AC2],4                     ; dead
```

Because bit 8 is never written (§2), the `JZ` at `0x4B0CE` is always taken and
`0x4B0D0..0x4B0DE` is **unreachable**. Producer A arms sit 8 **never**.

### 3.2 Producer B — the period-index-4 / phase-`0xC` period end (`FUN_00088860`) — **the live path**

Fresh `disassemble_bytes 0x88860` (160 B):

```
0x88863 MOV EAX,[0x157A4A] / SAR EAX,0x18 / CMP EAX,0xC / JNZ 0x888F7
        ; gate on byte [0x157A4D] (0x157A4A>>24) == 0xC (period end)
0x888AB MOV AL,[0x157AC2] / CMP EAX,4 / JNZ 0x888E2   ; PERIOD INDEX == 4
0x888B5 CALL 0x92AC8 / AND AL,1 / MOVSX EDX,AX / EAX=8
0x888C8 CALL 0x8A938                  ; <<< SITUATION 8, side rng&1, BX 0
0x888CD.. CALL 0x8F188(0xC1,0,8)
0x888E2 EAX=1 / EDX=[0x157AAB]>>24 / CALL 0x8A938   ; else SITUATION 1
```

`FUN_00088860` has **2** callers (fresh `get_xrefs_to 0x88860` = 2): `0x4B08E`
(inside `FUN_0004B02C`'s `param_1 == 0` arm) and `0x74244` (the period-end family
body; containing function has no Ghidra function — fresh
`get_function_by_address 0x74244` = none, FU196-L6). The `0x74244` context (fresh
`disassemble_bytes 0x74220`, 48 B) is exactly a phase transition:
`CMP byte[0x14C32A],0 / JNZ 0x742E3 / 0x74244 CALL 0x88860`.

**How `[0x157AC2]` reaches 4** (fresh `get_xrefs_to 0x157AC2` = **56**; writes):

* front-end `0x38990` (`FUN_00038630`): `MOV DH,4 / MOV byte[0x157AC2],DH` —
  sets the period index **to 4** directly (an extra-time competition/setup path,
  unported — FU215-L2);
* the clock rollover `0x8B58A` (`FUN_0008AF38`): `MOV BH,[0x157AC2] / INC BH /
  MOV byte[0x157AC2],BH` — the period `INC` per period end;
* the reset `0x73F82` (`FUN_00073EE0`): `:= 0`;
* producer A `0x4B0DE`: `:= 4` (dead, §3.1);
* the act-0xC tail `0x88D6A`.

So the **live** sit-8 arming is: a completed period leaves phase `0xC`, the period
index is `4` (set by the front-end or reached by the roll), and `FUN_00088860`
(`0x74244` caller) dispatches **situation 8** → the table-`0x8A904[8] = 0x8AE08`
arm → **act 7 invoke-now** (`0x8AEAE MOV EBX,1 / 0x8AEB3 MOV EAX,7 / 0x8AEBA
CALL 0x888FC`), i.e. the phase-`0x1D` timeline (FU208 §3 / FU209), *cross-cited*.

**Co-condition** (*cross-cited* FU208 §2): sit 8 dispatches act 7 only on the
dispatcher's **direct path** (`[0x14C32A] == 0` or a situation pending); the
`0x74244` caller is itself gated on `[0x14C32A] == 0` (`0x74237 JNZ 0x742E3`).

---

## 4. Engine extension point — exactly what the port must do

### 4.1 What must **not** be added: a bit-8 producer

The native never sets bit 8 (the two located writers are config→`{0..7,9}` or
zero; the indexed table writers stop at index 1). The engine already mirrors this:
`fifa96_input_range_words` (`src/fifa96_loader/fifa96_input.c:86`) maps the eight
config cells to `w0` bits `{0,1,2,3,4,5,6,7,9}` and `w1` bits `{3,4,5,6,7,9}`
(`:94..116`) — **no `0x100`** — and `mr->input_range` is staged zero
(`fifa96_match_run.c:2012/2013`, `:1866/1867`). **Any engine change that sets
`input_range[s] & 0x100` would be a fabrication and is forbidden.**
`fifa96_match_run_period_setup`'s bit-8 gate
(`fifa96_match_run.c:3175 if ((mr->input_range[0] | mr->input_range[1]) &
0x0100u)`) is therefore **dead on the live path** — *exactly as in the native*
— and must **stay** the faithful mirror (it is not the producer).

### 4.2 The one faithful producer: the period-index-4 route (already landed)

The live sit-8 arm is producer B. The engine already carries it:

* `fifa96_match_run_phase_drive` rolls `mr->global_157ac2 = mr->state.period`
  (`src/fifa96_engine/fifa96_match_run.c:2639`) — the native `0x8B58A`;
* on `global_157ac2 == 4` it calls the mode-0 period-end selector
  (`:2650 if (mr->global_157ac2 == 4u) (void)fifa96_match_run_period_setup(mr, 0);`)
  — the native `0x74244 → 0x88860`;
* `fifa96_match_run_period_setup` (`:3145`) mode 0 reproduces `FUN_00088860`:
  `state.phase == 0x0C && global_157ac2 == 4` → `set_piece(mr, 8u, rng&1, 0u)`
  (`:3160..3164`) — the native `0x888C8`.

**So the engine extension point is already satisfied and there is nothing to
add for the bit-8 arm.** The only *native* producer still unported is the
**front-end** `[0x157AC2] = 4` writer (`0x38990`, `FUN_00038630`) that would let a
competition pre-arm extra time without a simulated period roll — a front-end
path, unported (FU215-L2), and not needed for match simulation.

### 4.3 Fixture-ability on the current M2 tape

**Dormant.** The M2 tape completes no period (it ends at the natural goal), so
`mr->state.period` never reaches 4 and `global_157ac2` never reaches 4 — the
producer B path (and therefore the whole sit-8 → act 7 → phase-`0x1D` route) is
**not reached on M2** (*cross-cited* FU214 §4 C-1/C-2). It is already proven by
**unit fixtures** (`tests/test_engine_match_handlers.c:3692
test_period_setup_mode1_extra_time_sit8`, `:3721
test_period_setup_mode0_extra_time_sit8`). A **live end-to-end** run needs a
**new scripted extra-time period** (a tape addition — an extra-time-entering
period completion) — **not** an engine-code change, and **not** a bit-8
producer. No M2 re-pin is forced by any faithful landing.

---

## 5. Census impact

**None.** No native sit-8 arming writer implies no new table row. The engine's
`fifa96_match_handlers.c:5236 {0x1D, fifa96_match_action_phase_1D, … COUNTED}`
registration and the `53/80 = 66%` headline are untouched by this slice: the
phase-`0x1D` row is already counted on the live producer-B path
(*cross-cited* FU208 §6.2 / FU214 §8.2), and the bit-8 arm was never a census
mover. `floor(100 × 53/80) = 66` stands. **No engine change; no golden re-pin.**

---

## 6. Residual legs

1. **FU215-L1 — the bit-8 gate's intent vs the bit-9 config mapping.** The gate
   tests **bit 8** (`0x4B0CB TEST AH,1`), but the only "high byte" config arms
   set **bit 9** (`0x1CA15 OR byte[0x14C1D5],0x2` from `[0x105288]`;
   `0x1CA25 OR byte[0x14C1D7],0x2` from `[0x10529C]`). Either the extra-time
   option was *intended* to map to bit 8 and the shipped code sets bit 9 (a
   possible off-by-one in the original), or the bit-8 test is vestigial. The
   intent is **not statically provable**; the *observable* fact (bit 8 never
   written) is. Settle only by a runtime trace of the config block with extra
   time on — do not assert an intent. Owner: FU196-L1 / FU208-L1.
2. **FU215-L2 — the front-end `[0x157AC2] = 4` producer.** `FUN_00038630`
   (`0x38990 MOV DH,4; MOV byte[0x157AC2],DH`) sets the period index to 4 from
   the front-end; the competition/options path that reaches it is unported, so
   extra time can enter a live engine match only via a simulated period roll.
   Owner: the front-end setup slice (FU202-L7 / FU207-L2 family).
3. **FU215-L3 — `FUN_00088860`'s live caller `0x74244`.** The period-end family
   body at `0x74244` has **no Ghidra function** (fresh
   `get_function_by_address 0x74244` = none; the enclosing range is not
   auto-created), so the live period-end invocation of producer B is
   disassembled-only, not function-attributed. Owner: FU196-L6.
4. **FU215-L4 — the M2 tape never arms sit 8.** The v18 M2 tape completes no
   period, so producer B (and the whole sit-8 → act 7 → phase-`0x1D` route) is
   dormant; a live end-to-end proof needs a **new scripted extra-time period**
   (tape change, not engine code). Owner: the tape/phase-drive slice
   (FU208 §6.2, FU214-L6).
5. **FU215-L5 — the config-block producer chain.** The `0x105270`-family block
   (the range words' value origin) is written by the roster/setup machinery
   (`FUN_00024B00` `0x257CA`, `FUN_0001C728`, `FUN_0001CAEC`) on a base-register
   index; the full front-end → config-block → `FUN_0001C9BC` chain is not
   derived here (it cannot set bit 8 regardless, §2.2). Owner: the input-options
   slice (FU-148 §12/§13 / FU1C728).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, 15239 symbols, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `read_memory` — `0x14C1D0` (16 B), `0x14C1D4` (32 B), `0x105270` (64 B).
* `disassemble_bytes` — `0x4B090` (112 B, the sit-8 gate), `0x4B000` (40 B),
  `0x1C9BC` (whole-function, 54 insns), `0x24B60` (64 B), `0x739F0` (48 B),
  `0x70AA0` (48 B), `0x718C0` (48 B), `0x77EC8` (32 B), `0x8C790` (32 B),
  `0x7A690` (32 B), `0x7B8E0` (32 B), `0x7B400` (32 B), `0x7BCF0` (48 B),
  `0x742E0` (32 B), `0x74220` (48 B), `0x88860` (160 B), `0x8B560` (176 B),
  `0x8AF38` (32 B), `0x741B4` (64 B), `0x115A0` (48 B), `0x11B90` (32 B),
  `0x115CC` (96 B), `0x11620` (whole-function), `0x11978` (whole-function),
  `0x11B7C` (whole-function), `0x257A0` (128 B), `0x38980` (32 B), `0x73F70`
  (32 B), `0x73AB4` (32 B), `0x70B86` (48 B).
* `disassemble_function` — `0x1C9BC`, `0x24B00` (truncated, huge),
  `0x11620`, `0x11978`, `0x28588`, `0x11B7C`.
* `search_instructions` (program, 235 017–235 020 scanned) — `14c1d4` (21),
  `14c1d5` (1), `14c1d6` (12), `14c1d7` (1), `14c1cc` (5), `105270` (2),
  `105274` (8), `105278` (2), `10527c` (1), `105288` (1), `10528c` (1),
  `105290` (1), `10529c` (1), `1052a0` (1), `1052a4` (1), `105268` (0).
* `get_xrefs_to` — `0x14C1D4` (**21**), `0x14C1D6` (**12**), `0x1C9BC`
  (**2** callers), `0x24B00` (**3** callers), `0x11978` (**5**), `0x11620`
  (**5**), `0x115A0` (1), `0x11B7C` (5), `0x8A938` (**39**), `0x157AC2`
  (**56**), `0x88860` (**2**), `0x105278`/`0x10527C`/`0x105288`/`0x10528C`/
  `0x105290`/`0x10529C`/`0x1052A0`/`0x1052A4` (read-only sets),
  `0x105470` (52), `0x157AC4` (7), `0x14C1CC` (7).
* `get_function_by_address` — `0x11B7C`, `0x6D1B2`, `0x8B58A` (=`FUN_0008AF38`);
  `0x74244`/`0x742F5` = **none**.

Engine/loader source read first-hand:
`src/fifa96_loader/fifa96_input.c` (`:86` `fifa96_input_range_words`, `:94..116`
the bit map — no `0x100`; `:125..131` the option-cell indices);
`src/fifa96_engine/fifa96_match_run.c` (`:460..470`
`match_run_input_bits_update`; `:2011..2013` the zero staging; `:2263` the
match-init range build; `:2636..2653` the `global_157ac2` roll + mode-0 call;
`:3145..3181` `fifa96_match_run_period_setup`); `src/fifa96_engine/
fifa96_match_handlers.c` (`:5236` the COUNTED phase-`0x1D` slot); `:5143`/`:5150`
(row `0x20`). Docs cross-cited: FU-148 §12.2/§13, FU-166, FU194 §3, FU-196
§1–§6, FU-202, FU-206, **FU-208** §2/§4.4/§6.2/§7, **FU-209**, FU-210, FU-213,
**FU-214** §3/§4/§8. No writes beyond this slice file; no
rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
untouched; no git commands.

---

## Landing note

This slice **closes FU208-L1 and FU214-L2** as an *honest dead-gate* finding:
the sit-8 bit-8 disjunct has **no writer** (not an unlocated producer), so the
`0x4B0CB TEST AH,1` arm is structurally unreachable and situation 8 is armed
**only** by the period-index-4 producer `FUN_00088860` `0x888C8` — which the
engine already models live (`fifa96_match_run.c:2650` → `period_setup(mode 0)`).
No producer is added; the engine's `input_range & 0x100` gate stays 0 (faithful);
**census `53/80 = 66%` unchanged; no golden re-pin.** FU208-L8 (the session-gate
co-condition) and FU215-L1..L5 remain the residual legs.
