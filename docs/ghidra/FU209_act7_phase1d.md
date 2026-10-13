# FU-209 — act 7 / phase `0x1D` (native `0x89868`) full derivation + the phase-`0x10` writer (phase-19 wave R57)

Provenance: M2 phase-19 wave **R57**, 2026-10-13, read-only. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument;
this session `get_current_program_info` = `FIFA96.EXE`, `watcom:LE:32:default`,
`image_base 0`, `min_address 0x10000`, `max_address .image::00174a2a`, 2706
functions). Every address/byte/xref/instruction below is a fresh `read_memory` /
`disassemble_bytes` / `disassemble_function` / `get_xrefs_to` /
`get_current_program_info` of **this** session unless marked *cross-cited* (then
the frozen FU doc is named). LE-image globals sit at `+0x100000`; the addresses
here are `/FIFA96.EXE` program (runtime) addresses. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands.

Scope (the R57 brief): fully derive native **act 7 / phase `0x1D`** — the function
at `0x89868` (verify the address/entry first-hand); timeline the body (arm 0,
**every** stage/gate, the coupled action-row `0x20` installer `0x899A2` — which
function it calls and what it installs, the row-`0x19` and phase-`0x10` installs,
and the exit); state whether the whole act is live-reachable in the ported flow
and what the engine must add (function, install order); and derive the
phase-`0x10` writer that phase-18 left **Tier-3** (`docs/ghidra/FU206_census_audit_phase18.md`
§4 Tier-3 #12). This slice unblocks the extra-time / act-7-8 cohort.

---

## 1. Fresh anchors — entry, act table, phase table, the seam

### 1.1 The entry is phase slot `0x1D` (verified first-hand)

* `README`-independent entry check: fresh `read_memory 0x110808` (4 B) =
  `68 98 08 00` = **`0x00089868`**, and `0x110808 = 0x110794 + 0x1D*4` is phase
  slot **`0x1D`**. Fresh `get_xrefs_to 0x89868` = **1**, type DATA, from
  `0x110808` — the **only** static reference; there is no direct `CALL 0x89868`.
  So the sole dispatch route is the phase seam (the act machine's indirect
  `CALL dword[0x158808]`), exactly as FU-194 §1 / FU-189 §3.1 / FU-205 §2.1 hold.
* Fresh `read_memory 0x110794` (120 B) re-confirms the phase table byte-for-byte
  (`[10]=06E004`, `[1D]=089868`):
  `00:06DE34 01:06E1D0 02:06DCC8 03:06DE44 04:06DE44 05:06E05C 06:06DD9C
  07:06DE44 08:06DD6C 09:06DD6C 0A:06DE34 0B:06DE34 0C:06DF4C 0D:06DE34
  0E:06DE34 0F:06DE34 10:06E004 11:06E1C8 12:06E1D0 13:06E244 14:06E244
  15:06DCC8 16:00000000 17:088DC8 18:08922C 19:089FA4 1A:089620 1B:0890EC
  1C:089110 1D:089868 1E:08A798 …` — matches FU-206 §1 exactly.

### 1.2 The act table binds act 7 to the same handler (dual identity)

Fresh `read_memory 0x1107EC` (32 B): `0, 0x88DC8, 0x8922C, 0x89FA4, 0x89620,
0x890EC, 0x89110, 0x89868, 0x8A798, …` → **`0x1107EC[7] = 0x89868`**. The base is
`0x110794 + 0x16*4`, so **act 7 → phase `0x16+7 = 0x1D`** — the FU165-L9
act/phase dual identity (re-confirmed byte-for-byte vs FU-194 §1 / FU-206 §1).

### 1.3 The dispatch seam (`FUN_000888FC` + the tick) — fresh

Fresh `disassemble_function 0x888FC` (20 insns):
`0x888FD MOVSX CX,byte[0x158828]; 0x88905 CMP AX,CX; 0x88908 JZ 0x8893D`
(the latch no-op) → `0x8890A MOV [0x158828],AL` (act id) → `0x88910 SHL EAX,2;
0x88913 ADD EAX,0x1107EC; 0x88918 MOV byte[0x158829],DL` (stage) →
`0x88920 MOV EAX,[EAX]` (handler) → `0x88922 MOV word[0x158818],DX` (timer=0) →
`0x88929 MOV [0x158808],EAX` (act slot) → `0x8892E TEST BX,BX; 0x88931 JZ epi;
0x88933 TEST EAX,EAX; 0x88935 JZ epi; 0x88937 CALL dword[0x158808]`
(invoke-now iff `BX!=0`). The clock tail (`0x8B669 CMP dword[0x158808],0 /
0x8B672 CALL dword[0x158808]`, *cross-cited* FU-175 §2) ticks the installed
handler once per granted frame. So **every act reaches its stage-0 arm on the
dispatch frame (invoke-now) or the next granted-frame tick** (FU-194 §2). The
frame delta is `word[0x157A64]`.

### 1.4 The 10-arm table `0x89840` — fresh

Fresh `read_memory 0x89840` (40 B) = `b4980800 ee990800 4e9a0800 7a9a0800
ea9a0800 169b0800 509b0800 949b0800 b69b0800 6e9e0800` =
`{0x898B4, 0x899EE, 0x89A4E, 0x89A7A, 0x89AEA, 0x89B16, 0x89B50, 0x89B94,
0x89BB6, 0x89E6E}` (stage 0..9) — byte-for-byte FU-194 §3.1 / FU-189 §3.2.

### 1.5 Head (`0x89868..0x898AC`) — fresh `disassemble_bytes 0x89868` (1608 B, 427 insns, ends `0x89EAF`)

```
0x89868 PUSH EBX/ECX/EDX/ESI/EDI/EBP ; SUB ESP,0x14     ; prologue
0x89871 EAX = [0x1587D4]                                ; the ACTOR record pointer
0x89876 [ESP+0x8] = EAX                                 ; save actor rec
0x8987a EAX = [EAX]                                     ; actor's team base
0x8987c EAX = [EAX+0x7A6]                               ; team ball record
0x89889 [ESP+0xc] = EAX                                 ; save ball rec
0x89882 DX = word[0x158818] ; 0x8988d DX += word[0x157A64] ; timer += frame delta
0x89899 word[0x158818] = DX
0x89894 AL = [0x158829]                                 ; STAGE byte
0x898a0 CMP AL,9 ; 0x898a2 JA 0x89EA6                   ; > 9 -> epilogue
0x898a8 XOR EDX,EDX ; 0x898aa MOV DL,AL
0x898ac JMP dword CS:[EDX*4 + 0x89840]                  ; 10-arm dispatch
```

Two record pointers are captured up front: **actor** `[0x1587D4]` (`[ESP+8]`) and
the actor team's **ball** record `[[actor]+0x7A6]` (`[ESP+0xc]`). The actor
pointer's producer is unported (FU194-L8); it is the target of the coupled
row-`0x20` install (§3).

---

## 2. The body timeline — every arm and gate

**Structural fact (fresh, load-bearing):** this handler is mostly a **fall-through
cascade**, not a set of RETURN-terminated arms. Arms 0/1/2/4/5/6/7/8 each end by
advancing `[0x158829]` (`stage = N+1`) and **fall through** into the arm N+1 body;
arm 3 is the exception — it advances to stage 4 and then **returns** (via the
`0x89AB3` tail `RET` or the `0x89AAE JMP 0x89EA6` epilogue), so arm 4 runs on the
next call. A call therefore runs arm 0, then as many successive arms as the
intervening gates allow, and stops at the first gate that fails (or at the shared
epilogue `0x89EA6`, which pops the frame and returns). Every "wait" gate is
therefore a re-entry gate, not a terminator.

### 2.1 Arm 0 (`0x898B4..0x899ED`) — the install frame

```
0x898b4 XOR CL,CL
0x898b6 EAX = [0x157A4A]                                ; phase dword
0x898bb MOV byte[0x15882A],CL                           ; [0x15882A] = 0
0x898c1 SAR EAX,0x18                                    ; = byte[0x157A4D] (CURRENT phase)
0x898c4 MOV byte[0x157AB1],CL                           ; [0x157AB1] = 0 (unmodelled flag)
0x898ca CMP EAX,0x10 ; 0x898cd JZ 0x898e2               ; if phase == 0x10 skip the write
0x898cf EDX = [0x157AAC] ; 0x898da SAR EDX,0x18         ; DL = byte[0x157AAF] (side)
0x898d5 EAX = 0x10
0x898dd CALL 0x740A0                                    ; <<< PHASE-0x10 WRITE (see §5)
; --- team loop: 2 teams, EBP = 0x1588A4, stride 0x835, counter [ESP+0x10] ---
0x898f3 MOV EDX,0x19 ; EDI=1 ; EAX=EBP ; ECX=0 ; EBX=0
0x89903 MOV byte[EBP+0x9B],0                            ; records[0].+0x9B = 0
0x8990a CALL 0x7D9A4                                    ; <<< ROW-0x19 INSTALL on records[0]
0x8990f MOV EDX,0x26 ; 0x89914 EAX=EBP
0x89916 MOV ECX,[EBP+0x8B] ; 0x8991e SAR ECX,0x18
0x89923 CALL 0x6E598                                    ; records[0] anim 0x26
0x89928 JMP 0x89966
; --- record loop (records[1..10]): reads records[i].+0x9A, installs row 3 on records[i] ---
0x8992a MOV byte[ESI+0x14D],0                           ; records[i].+0x9B = 0  (ESI=records[i-1])
0x89931 AL = byte[ESI+0x14C]                            ; = records[i].+0x9A
0x89937 ADD ESI,0xB2                                    ; ESI = records[i]
0x8993f JNZ 0x89965                                     ; if +0x9A != 0 skip
0x89941 EDX=3 ; 0x8994c CALL 0x7D9A4                    ; row 3 on records[i] (installer 3->0x19, §4)
0x89960 CALL 0x6E598                                    ; records[i] anim 0
0x89965 INC EDI ; 0x89966 MOVSX ECX,DI ; CMP ECX,0xB ; JL 0x8992A
0x8997e CMP word[ESP+0x10],2 ; JL 0x898F3               ; 2 teams
; --- after both teams ---
0x8998e EAX=[ESP+8] ; EDX=[ESP+8] ; EAX=[EAX]
0x8999c MOV [EAX+0x7B2],EDX                             ; [actor-team+0x7B2] = actor rec
0x89998 XOR ECX,ECX ; 0x8999a XOR EBX,EBX               ; invoke = 0
0x899a2 MOV EDX,0x20                                    ; <<< ROW-0x20 CODE
0x899a7 MOV EAX,[ESP+8]                                 ; actor rec
0x899ab CALL 0x7D9A4                                    ; <<< ROW-0x20 INSTALL (deferred)
0x899b0 EAX=[ESP+0xc] ; EDX=[ESP+0xc] ; EAX=[EAX]
0x899be MOV [EAX+0x7B2],EDX                             ; [ball-team+0x7B2] = ball rec
0x899c4 MOV EDX,0x1F ; 0x899c9 MOV EAX,[ESP+0xc]
0x899cd CALL 0x7D9A4                                    ; <<< ROW-0x1F INSTALL on ball rec (deferred)
0x899d2 CALL 0x7A028                                    ; referee/incident block reset (see §6.3)
0x899d7 CH=[0x158829] ; 0x899df INC CH ; 0x899e1 word[0x158818]=0 ; 0x899e8 [0x158829]=CH
; --- falls through into arm 1 ---
```

Fresh call bytes: `0x8990a = e8 95 40 ff ff` → `0x7D9A4` (row `0x19`),
`0x8994c = e8 53 40 ff ff` → `0x7D9A4` (record loop, `3`), `0x899a2 = ba 20 00 00
00`, `0x899ab = e8 f4 3f ff ff` → `0x7D9A4` (row `0x20`), `0x899cd = e8 d2 3f ff
ff` → `0x7D9A4` (row `0x1F`), `0x899d2 = e8 51 06 ff ff` → `0x7A028`,
`0x898dd = e8 be a7 fe ff` → `0x740A0` — all re-computed to their targets and
agreeing with Ghidra.

### 2.2 Arm 1 (`0x899EE..0x89A4D`) — the whistle wait

```
0x899ee XOR AH,AH ; 0x899f0 MOV byte[0x15882A],AH        ; [0x15882A] = 0
0x899fa EAX = word[[actor]+0x65]  (MOV [actor+0x63]; SAR 0x10)
0x89a00 CMP EAX,0x30 ; 0x89a03 JG 0x89EA6                ; actor word[+0x65] > 0x30 -> epi
0x89a0d EAX = word[[ball]+0x65] (same idiom)
0x89a13 CMP EAX,0x30 ; 0x89a16 JG 0x89EA6                ; ball word[+0x65] > 0x30 -> epi
0x89a1c EAX = [0x158816] ; SAR 0x10                      ; = word[0x15881A] (shared timeline)
0x89a24 CMP EAX,0x78 ; 0x89a27 JL 0x89EA6                ; < 0x78 -> epi (wait)
0x89a2d EAX=0x1E ; 0x89a32 DL=1
0x89a34 CALL 0x974DC                                    ; whistle 0x974DC(0x1E)
0x89a39 MOV byte[0x15882A],DL                           ; [0x15882A] = 1
0x89a41 word[0x158818]=0 ; 0x89a48 INC byte[0x158829]   ; timer=0; stage=2
; --- falls through into arm 2 ---
```

### 2.3 Arm 2 (`0x89A4E..0x89A79`) — the actor/referee contact wait

```
0x89a4e EAX=[ESP+8] (actor rec)
0x89a52 CMP EAX,[0x158730] ; 0x89a58 JNZ 0x89EA6         ; actor rec != [0x158730] -> epi
0x89a5e CMP byte[0x158746],0 ; 0x89a65 JZ 0x89EA6       ; [0x158746] == 0 -> epi
0x89a6b word[0x158818]=0 ; 0x89a74 INC byte[0x158829]   ; timer=0; stage=3
; --- falls through into arm 3 ---
```

`[0x158730]`/`[0x158746]` are exactly two of the cells `0x7A028` **zeroes** in
arm 0 (§6.3): arm 2 is therefore armed by an external referee/contact producer
that (re)sets them, and not by act 7 itself.

### 2.4 Arm 3 (`0x89A7A..0x89AB2` + the `0x89AB3` tail) — the restart-line gate

```
0x89a7a CMP byte[0x15781D],0 ; 0x89a81 JZ 0x89AB3       ; [0x15781D] == 0 -> 0x89AB3
0x89a83 CMP byte[0x15781E],0 ; 0x89a8a JNZ 0x89AB3      ; [0x15781E] != 0 -> 0x89AB3
0x89a8c EAX=6 ; 0x89a91 CALL 0x651F0                    ; 0x651F0(6)
0x89a96 EAX=0x5DC ; 0x89a9b CALL 0x974F0                ; 0x974F0(0x5DC)
0x89aa0 word[0x158818]=0 ; 0x89aa8 INC byte[0x158829]   ; timer=0; stage=4
0x89aae JMP 0x89EA6                                     ; epilogue
0x89ab3 CMP word[0x1577BE],0 ; 0x89abb JNZ 0x89acb      ; either flag nonzero -> advance
0x89abd CMP word[0x1577F0],0 ; 0x89ac5 JZ 0x89EA6       ; both zero -> epi (wait)
0x89acb AL=stage ; 0x89ad2 word[0x158818]=0 ; 0x89adb stage=AL ; 0x89ae0 epilogue RET
```

Arm 3 is compound: with `[0x15781D]!=0 && [0x15781E]==0` it fires the
`0x651F0(6)`/`0x974F0(0x5DC)` pair and advances; otherwise it arrows to the
`0x89AB3` tail, which advances only once `word[0x1577BE]` or `word[0x1577F0]` is
nonzero (a two-phase "flag high then low" wait — see §2.5).

### 2.5 Arm 4 (`0x89AEA..0x89B15`) — the complementary wait

```
0x89aea BX=word[0x1577BE]; 0x89af1 TEST BX,BX; 0x89af4 JNZ 0x89EA6   ; !=0 -> epi
0x89afa CMP word[0x1577F0],0 ; 0x89b02 JNZ 0x89EA6                   ; !=0 -> epi
0x89b08 INC AL ; 0x89b0a word[0x158818]=0 ; 0x89b11 [0x158829]=AL   ; stage=5
; --- falls through into arm 5 ---
```

Arm 4 advances only while **both** `word[0x1577BE]` and `word[0x1577F0]` are
zero — the inverse of the `0x89AB3` tail. So the pair `[0x1577BE]`/`[0x1577F0]`
is a raise-then-lower handshake across arms 3/4.

### 2.6 Arm 5 (`0x89B16..0x89B4F`) — the timeline gate

```
0x89b16 EAX=word[0x15881A] ; 0x89b1e CMP EAX,0x3C ; 0x89b21 JL 0x89EA6  ; < 0x3C -> epi
0x89b27 CMP EAX,0x78 ; 0x89b2a JGE 0x89B39
0x89b2c CALL 0x45001 ; 0x89b31 TEST AL,AL ; 0x89b33 JZ 0x89EA6         ; predicate false -> epi
0x89b39 word[0x158818]=0 ; 0x89b4a INC byte[0x158829]                  ; timer=0; stage=6
; --- falls through into arm 6 ---
```

### 2.7 Arm 6 (`0x89B50..0x89B93`) — the code/ball gate

```
0x89b50 EAX = byte[actor+0x91] (MOV [ESP+8]; [EAX+0x8E]; SAR 0x18)
0x89b5d CMP EAX,0x20 ; 0x89b60 JZ 0x89EA6              ; actor action code == 0x20 -> epi
0x89b66 EAX=[ESP+0xc] (ball rec)
0x89b6a CMP byte[EAX+0x44],0 ; 0x89b6e JZ 0x89EA6      ; ball byte[+0x44] == 0 -> epi
0x89b74 EAX=0x1A ; 0x89b79 CALL 0x4C374                ; 0x4C374(0x1A) = [0x157A68] clear (FU194-L3)
0x89b88 word[0x158818]=0 ; 0x89b8e INC byte[0x158829]  ; timer=0; stage=7
; --- falls through into arm 7 ---
```

### 2.8 Arm 7 (`0x89B94..0x89BB5`) — the wait gate

```
0x89b94 CALL 0x4BEC8 ; 0x89b99 TEST AX,AX ; 0x89b9c JNZ 0x89EA6  ; 0x4BEC8()!=0 -> epi
0x89baa word[0x158818]=0 ; 0x89bb0 INC byte[0x158829]            ; timer=0; stage=8
; --- falls through into arm 8 ---
```

### 2.9 Arm 8 (`0x89BB6..0x89E6D`) — the selection block (structure only)

```
0x89bb6 EDI=-1 ; EAX=[0x1587E4] ; BH=0xFF ; [0x1587E9]=BH
0x89bcb CMP EAX,0x5 ; 0x89bce JGE 0x89CA8              ; the two rank/side arms
… (0x89BD4..0x89D0D: the team-side/record-rank comparisons, the [0x1587E3..E9]
   state, the [0x897F4]/[0x741B4] helpers, the EDI>=0 winner vs the EDI<0
   reset-to-stage-0 branch — FU194-L7, cited not semantically resolved)
0x89e43 EAX=0xBA ; 0x89e48 CALL 0x8F188                ; event 0x8F188(EAX=0xBA, EBX=0x20, EDX=rec)
0x89e4d EAX=7 ; 0x89e52 CALL 0x4C374                   ; 0x4C374(7) = [0x157A68] clear
0x89e61 word[0x158818]=0 ; 0x89e68 INC byte[0x158829]  ; timer=0; stage=9
; --- falls through into arm 9 ---
```

The full selection block (the `[0x1587E3..E9]` state machine and its
`0x741B4`/`0x897F4` helpers) is the FU194-L7 leg; only its head, its `0xBA`
event emit, its `0x4C374(7)` clear and its stage advance are derived here.

### 2.10 Arm 9 (`0x89E6E..0x89EA5`) — the terminal exit

```
0x89e6e CALL 0x4BEC8 ; 0x89e73 TEST AX,AX ; 0x89e76 JNZ 0x89EA6  ; 0x4BEC8()!=0 -> epi
0x89e7a word[0x158818]=0                              ; timer=0
0x89e84 byte[0x158828]=0                              ; act id = 0
0x89e8c byte[0x157AC0]=1                              ; <<< EXTRA-TIME FLAG = 1
0x89e92 dword[0x158808]=0                             ; act slot clear
0x89e9a byte[0x157A4C]=0                              ; goal-side flag = 0
0x89ea0 byte[0x158829]=0                              ; stage = 0
0x89ea6 ADD ESP,0x14 ; POP EBP/EDI/ESI/EDX/ECX/EBX ; RET   ; epilogue
```

**The exit is a pure termination — no situation re-dispatch** (unlike act 8
`0x8A798` and act 0xA `0x8B688`, which re-dispatch a stored situation). Act 7
instead **sets `[0x157AC0] = 1`**, the extra-time flag whose only other engine
producer is the unported OL-85 cell (FU195-L2 / FU202-L1). This is an act-7 tail
producer the engine's ported arm 9 omits (see §6.4).

---

## 3. The coupled action-row `0x20` installer `0x899A2`

### 3.1 Which function it calls, and what it installs

`0x899A2` calls the **installer `FUN_0007D9A4(rec, code, stage, invoke)`** with
`EAX = [ESP+8] = [0x1587D4]` (the **actor** record), `EDX = 0x20` (code),
`ECX = 0`, `EBX = 0` (stage 0, invoke 0). Fresh `disassemble_function 0x7D9A4`
(the full 73-insn contract):

```
guard:  rec==0 -> ret;  byte[rec+0x9A]!=0 -> ret;  code == byte[rec+0x91] -> ret
pre:    if byte[rec+0x98]!=0 && code!=0xC && [0x157A4D]not in {2,0xA,0xF}: anim 0xF, [rec+0x98]=0
        if byte[rec+0x8D]==0 && code==3: code = 0x19          ; <<< the 3->0x19 rewrite
        code==5 || code==0x21 -> [rec+0x9F] |= 1  else  &= 0xFE
write:  byte[rec+0x91]=code ; [rec+0x89]=0 ; byte[rec+0x9E]=0
        [rec+0x18] = action_table[code]   (0x1106E0 + code*4)
        byte[rec+0x92]=stage ; word[rec+0x7B]=word[rec+0x79]
invoke: if DI(invoke)!=0: CALL [rec+0x18]                     ; invoke-now
```

* The action table `0x1106E0` slot `0x20` = **`0x84EEC`** (fresh
  `read_memory 0x110760` = `ec 4e 08 00`; §1.2's 136-B read). So the installer
  writes `[actor+0x91] = 0x20` and `[actor+0x18] = 0x84EEC` (the row-`0x20`
  body).
* `ECX = 0` at `0x89998`, so the install is **deferred** (no invoke-now): the
  `0x84EEC` body runs only when the **per-record action machine** next calls
  `[actor+0x18]` (*cross-cited* FU-189 §3.3 / FU-194 §3.2 / FU-205 §2.2 — the
  engine mirrors it at `fifa96_match_dispatch_action`, `match_run.c:1406/1410`).
* This resolves FU189-L1's "who installs row `0x20`": the act-7 arm 0, and the
  gate is the **act-7 / phase-`0x1D` route**, not the installer.

### 3.2 The install-order neighbourhood (arm 0 tail)

The arm-0 tail is a fixed native order (verified instruction-by-instruction):
`[actor-team+0x7B2] = actor rec` → **row-`0x20` install on the actor rec** →
`[ball-team+0x7B2] = ball rec` → **row-`0x1F` install on the ball rec**
(`0x899CD`, action slot `0x1F = 0x76380`) → `0x7A028` → advance. Both installs are
deferred; the two `+0x7B2` writes set the per-team "active record" pointer.

---

## 4. The row-`0x19` and record-loop row-`3` installs

* **Base install `0x8990A`**: `0x7D9A4(records[0], code=0x19, stage=0, invoke=0)`
  — action slot `0x19 = 0x746E4` (fresh `read_memory 0x1106E0`:
  `[0x19]=0746E4`). One install **per team** on the team base (`EBP` =
  `records[0]`), outside the 11-record loop (the FU-194 erratum, §Erratum below).
* **Record-loop install `0x8994C`**: `0x7D9A4(records[i], code=3, 0, 0)` for
  `i = 1..10` when `records[i].+0x9A == 0`. The installer rewrites `code 3 → 0x19`
  when `records[i].+0x8D == 0` (§3.1), so an **inactive** record receives `0x19`
  while an **active** record receives `3`. `records[0]` gets the direct `0x19`
  (base) and is not read in the loop.
* Both are followed by `0x6E598(rec, anim, …)` (the animation sink; anim `0x26`
  on `records[0]`, anim `0` on the loop records), and the loop clears
  `records[i].+0x9B`.

**Indexing (fresh, load-bearing).** The loop enters with `ESI = EBP = records[0]`
and reads `byte[ESI+0x14C]`/clears `byte[ESI+0x14D]` *before* `ESI += 0xB2`; since
`records[i].+0x9A = EBP + i*0xB2 + 0x9A = EBP + 0x14C + (i-1)*0xB2`, the offsets
`+0x14C`/`+0x14D` at `ESI = records[i-1]` address `records[i].+0x9A`/`+0x9B`. The
read is records[i]'s and the install is records[i]'s (after the advance). Net: the
base install covers `records[0]`; the loop covers `records[1..10]` — all 11
records.

---

## 5. The phase-`0x10` writer (the phase-18 Tier-3 leg)

### 5.1 The native write site

Arm 0 (`0x898B6..0x898DD`) writes phase `0x10` when the **current** phase is not
already `0x10`:

```
0x898b6 EAX = [0x157A4A] ; 0x898c1 SAR EAX,0x18    ; = byte[0x157A4D] (current phase)
0x898ca CMP EAX,0x10 ; 0x898cd JZ 0x898e2          ; phase == 0x10 -> skip
0x898cf EDX = [0x157AAC] ; 0x898da SAR EDX,0x18    ; DL = byte[0x157AAF] (side)
0x898d5 EAX = 0x10
0x898dd CALL 0x740A0                               ; 0x740A0(phase=0x10, side)
```

### 5.2 The writer contract `FUN_000740A0` — fresh `disassemble_function`

```
0x740a0 AH=[0x157A4D] ; 0x740a6 byte[0x157A4E]=AH   ; save the OLD phase
0x740ac [0x157A4D]=AL                               ; write the NEW phase byte
0x740b1 EAX=DX(side) ; *0x835 ; 0x740ba byte[0x157AAF]=DL ; + 0x1588A4
0x740c8 CALL 0x8D098                                ; per-record walk on team[side]
0x740c5 XOR DL,1 ; 0x740d... CALL 0x8D098           ; walk on team[side^1]
0x740e0 EAX=[0x157A4A]>>24 ; CMP EAX,2 ; JNZ 0x7410D
0x740ed CALL 0x4C380 ; [0x15781D]=0 ; [0x157AB2]=1 ; [0x157A73]=0x15774C
0x7410d RET
```

So `0x740A0(phase, side)`: stores the old phase to `[0x157A4E]`, writes the new
phase to `[0x157A4D]`, records the side to `[0x157AAF]`, **runs the per-record
walk `0x8D098` on both teams**, and (only when the new phase is `2`) runs the
`0x4C380`/`[0x15781D]`/`[0x157AB2]`/`[0x157A73]` phase-2 block.

### 5.3 The per-record walk `0x8D098` dispatches the new phase row

Fresh `disassemble_function 0x8D098` (583 insns): head binds the team roster
(`0x6D9C4`) and refreshes (`0x8C418`); then the 11-record loop
(`0x8D0F7 CMP byte[ECX+0x9A],0; JNZ skip` → `0x8D107 CALL 0x6D920` (stage the
row pointer) → `0x8D110 CALL dword[ECX+0x1C]` (the phase body) →
`0x8D11E CALL 0x8DCD4`); then the `[0x14C32A]` gate (`0x937DC`); then the switch
on `[0x157A4D]` (`0x8D178`).

The stager `0x6D920` resolves the phase from `[0x157A4A]>>24` (*cross-cited*
FU-206 §1: `0x6D9A8 MOV EAX,[0x157A4A]; SAR 0x18; SHL 2; ADD 0x110794;
MOV EAX,[EAX]; MOV [EDX+0x1C],EAX`). Because `0x740A0` writes `[0x157A4D]`
**before** calling `0x8D098`, the walk dispatches the **new** phase row — so the
phase-`0x10` write itself runs the phase-`0x10` body `0x6E004` on every active
record.

### 5.4 The phase-`0x10` body `0x6E004` — fresh `disassemble_bytes`

`0x6E004` (`BL=[rec+0x8D]`; `BL==0` → `target = (0x3C0, 0, 0x840)`; else
`k = BL-1`, index the signed-byte pair table, `<<5`, flip `target[2]` when
`[team+0x826]!=0`, `target[1]=0`) — byte-for-byte the engine's
`fifa96_match_phase_10` variant table. Fresh `read_memory 0x1105E0` (40 B) gives
the pair base `0x1105EA` stride 2:
`(-8,-2)(-4,-2)(0,-2)(4,-2)(8,-2)(-5,-5)(0,-5)(5,-5)(-3,-7)(2,-7)(-16,0)` —
**exactly** the engine's `variant[11][2]` (`handlers.c:4807-4809`). The
out-of-range `k=-1` native read is the documented FU192-L1 leg (the engine bounds
`k` to `[0,10]`).

### 5.5 The engine model and why it is Tier-3

* **Writer**: the engine already models the native write as
  `match_run_write_phase(mr, 0x10u)` at **`match_run.c:6007`**, inside
  `fifa96_match_action_phase_1D` arm 0 (line `:6006` holds the
  `state.phase != 0x10` gate). `match_run_write_phase` (`match_run.c:2504`) sets
  `state.phase`/`phase_machine` and, **iff the phase row is wired**, runs the
  per-team walk (`match_run_record_phase_walk`, `:2518-2522`).
* **Why it never runs**: the writer sits inside the phase-`0x1D` body, whose
  engine slot is **NULL** (`handlers.c:5117`); and the phase-`0x10` row itself is
  **NULL** (`handlers.c:5091`) because the *sole* writer (§5.1) is inside that
  unwired body — the exact Tier-3 statement of FU-206 §4 #12 / FU-194-L9. So the
  phase-`0x10` body is ported-but-unregistered and the writer is a structural
  no-op until phase `0x1D` is dispatched.

---

## 6. Reachability in the ported flow + what the engine must add

### 6.1 The native producer chain for act 7 (fresh source trace)

The only native producer of act 7 is **situation 8**
(`fifa96_action_phase_situations[8] = {phase=NONE, act=7, stage=0}`,
`fifa96_action_handlers.c:1142`; *cross-cited* FU-194 §3.4). The chain:

`FUN_0004B02C` (period/half transition) → `FUN_0008A938(8,0,0)` (the table-2
sit-8 arm `0x8AEAE`, `MOV EAX,7; BX=1`) → `FUN_000888FC(7,0,BX=1)` (invoke-now,
arm 0 runs the same frame). The engine mirrors this in
`fifa96_match_run_period_setup` (`match_run.c:3063`) and
`match_run_situation_side` (`:2765` → `fifa96_match_act_dispatch(mr, 7, 0, 0)`,
`:2807`).

**The two situation-8 gates are both inert in the ported flow:**

1. **the bit-8 extra-time gate** (`match_run.c:3093`):
   `if ((input_range[0]|input_range[1]) & 0x100) { set_piece(8,0,0);
   global_157ac2 = 4; }`. Fresh first-hand: `fifa96_input_range_words`
   (`fifa96_input.c:86..120`) **never ORs `0x100`** — the bits it can set are
   `1,2,4,8,0x10,0x20,0x40,0x80,0x200`. `mr->input_range` is written only by that
   function (`match_run.c:484`, `:2259`) and zeroed at reset (`:1866`, `:2012`).
   So `(input_range & 0x100) == 0` **always**.
2. **the phase-`0x0C` extra-time arm** (`match_run.c:3079`):
   `if (state.phase == 0x0C) if (global_157ac2 == 4) set_piece(8, …)`. Since
   `global_157ac2` is only ever set to `4` by gate (1) (`:3095`) and is otherwise
   reset to `0` (`:2034`, `:2152`), it is never `4` in the ported flow.

Additionally, even were sit 8 emitted, `set_piece(8, …)` (`match_run.c:2983`)
routes to the **queue arm** (`situation_id = 0xA`, no act dispatch) when
`session_gate_14c32a && !situation_pending` (FU205-L3).

### 6.2 Does the act-7 body run? — **No**

Two independent blocks, both fresh:

* The **phase-`0x1D` engine slot is NULL** (`handlers.c:5117`), so the act tick
  bails at `fifa96_match_act.c:63` (`if (fifa96_match_phase_table[phase].fn ==
  NULL) return FIFA96_OK;`) before `[0x158808]` is ever invoked.
* The **situation-8 producer never fires** (§6.1), so act 7 is never even
  latched (`fifa96_match_act_dispatch(mr, 7, …)` is never reached live).

Therefore **the whole act 7 / phase-`0x1D` body is NOT live-reachable in the
ported flow**; the ported `fifa96_match_action_phase_1D` body runs only under the
direct unit tests (`tests/test_engine_match_act.c:521/542/563`). This confirms and
sharpens FU-205 §3 / §T5.2 and FU-206 §5 PR-7/PR-8 (tape-dormant).

### 6.3 Why arm 0's `0x7A028` and arm 2 matter (fresh)

Arm 0 calls `0x7A028` (fresh `disassemble_function`: zeroes
`[0x158730..0x158746]`, then sets `byte[0x158743]=2`, `byte[0x158742]=0x20`,
`byte[0x158744..746]=0`). `[0x158730]` and `[0x158746]` are precisely the cells
arm 2 reads (§2.3), so arm 2 is armed only by the external referee/contact
producer that (re)sets them — not by act 7. The engine has no `[0x158730]`
cell; the arm-2 gate is not modelled.

### 6.4 What the engine must add (function, install order)

To make act 7 live the engine needs **four** landings, in this order:

1. **Register the body**:
   `fifa96_match_phase_table[0x1D].fn = fifa96_match_action_phase_1D`
   (`handlers.c:5117`); the body already exists (`match_run.c:5998`). Without
   this the act tick early-returns regardless of the producer.
2. **Complete arm 0**: the coupled **row-`0x20` install on the actor record**
   (`0x899A2`), the **row-`0x1F` install on the ball record** (`0x899CD`), and the
   **`0x7A028` reset** (`match_run.c:6017-6019` currently records these as
   FU194-L7/L8 legs; the arm-0 port stops after the base row-`0x19` install).
   The actor `[0x1587D4]` and ball-record producers have no engine cell (a fake
   install is forbidden — FU-205 §3(c)).
3. **Port the row-`0x20` body** `0x84EEC` as `fifa96_match_action_20`
   (`handlers.c:5030` is NULL): landing the phase-`0x1D` install makes row `0x20`
   *installed*, but the per-record action machine would dispatch a NULL body as
   UNSUPPORTED. The `0x20` body is the **coupled pair** (FU-189 §6).
4. **Provide a live situation-8 producer**: either set the extra-time input bit
   (`0x100`) through `fifa96_input_range_words`/the config, **or** add the genuine
   `global_157ac2` producer (FU205-L4 / OL-85). Until one lands, act 7 cannot be
   latched.

**Arm-0 install order** (native, to mirror exactly): clear `[0x15882A]`/
`[0x157AB1]` → phase-`0x10` write (`0x740A0`) → per team (`records[0]`:
row-`0x19` + anim `0x26`; `records[1..10]`: `+0x9B` clear, row-`3`→`0x19` on
inactive + anim) → `[actor-team+0x7B2] = actor rec` → **row-`0x20` install on
actor rec** → `[ball-team+0x7B2] = ball rec` → **row-`0x1F` install on ball rec** →
`0x7A028` → stage 1. The engine's port already does the first three steps
(`match_run.c:6003..6016`) and must be extended with the last five.

---

## 7. Numbered legs

1. **FU209-L1 — the act-7 producer is inert.** Both situation-8 gates are
   provably closed in the port (`fifa96_input_range_words` never sets bit
   `0x100`, `fifa96_input.c:86`; hence `global_157ac2` never `4`). This slice
   proves the closure statically and *does not* add the true producer. Owner:
   FU205-L4 / OL-85 (the `global_157ac2` writer) and the input-config producer.
   Do not count phase `0x1D` or row `0x20` until one lands.
2. **FU209-L2 — the arm-0 coupled install is unported.** The row-`0x20`
   (`0x899A2`), row-`0x1F` (`0x899CD`) and `0x7A028` are FU194-L7/L8 legs
   (the actor `[0x1587D4]` / ball-record producers have no engine cell). Landing
   phase `0x1D` without them leaves row `0x20` un-installed; landing them without
   a live producer is inert. Owner: FU194-L8 / FU-205 §T5.2.
3. **FU209-L3 — the row-`0x20` body `0x84EEC`.** Unported (`handlers.c:5030`
   NULL); the coupled landing (FU-189 §3.3/L3, OL-9). Required for the installed
   row to run without `-UNSUPPORTED`. Owner: FU-189.
4. **FU209-L4 — arm 8 (the selection block).** `0x89BB6..0x89E6D` is derived
   structurally only (the `[0x1587E3..E9]` state, the `0x741B4`/`0x897F4`
   helpers, the `EDI>=0` winner vs the `EDI<0` reset-to-stage-0 branch). Owner:
   FU194-L7 (unchanged).
5. **FU209-L5 — the act-7 terminal `[0x157AC0]=1` is an extra-time-flag
   producer.** Arm 9 writes `[0x157AC0]=1` (`0x89E8C`); the engine's ported arm 9
   omits it (`match_run.c:6032-6038`), and `[0x157AC0]` is the OL-85 extra-time
   source that FU195-L2 lists as unproduced. Recorded as a coupling note (the
   engine has no `[0x157AC0]` cell wired to act 7). Owner: FU195-L2 / FU209-L1.
6. **FU209-L6 — the arm-2 contact gate `[0x158730]`/`[0x158746]`.** Arm 0's
   `0x7A028` zeroes the exact cells arm 2 reads; the engine models neither the
   cells nor the external referee/contact producer that arms them. Owner: FU194-L7
   (the unported record-pair producer).

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base
  `0`, min `0x10000`.
* `read_memory`: `0x110808` (4 B, phase slot `0x1D`), `0x110794` (120 B, the
  phase table), `0x1107EC` (32 B, the act table), `0x110760` (8 B, action slots
  `0x20`/`0x21`), `0x1106E0` (136 B, the action table), `0x89840` (40 B, the arm
  table), `0x1105E0` (40 B, the phase-`0x10` variant table).
* `get_xrefs_to`: `0x89868` (**1**, DATA from `0x110808`).
* `disassemble_bytes`: `0x89868` (1608 B = the full 427-insn body to `0x89EAF`),
  `0x6E004` (92 B, the phase-`0x10` body), `0x6D920` (48 B, the phase stager).
* `disassemble_function`: `0x7D9A4` (73 insns, the installer contract),
  `0x740A0` (25 insns, the phase writer), `0x888FC` (20 insns, the act seam),
  `0x7A028` (20 insns, the referee-block reset), `0x8D098` (583 insns, the
  per-record walk), `0x974DC` (7 insns, the whistle sink).

Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_run.c` (`:2504-2524` `match_run_write_phase`;
`:2765-2812` `match_run_situation_side`; `:2977-3052` `set_piece` + the queue
trap; `:3063-3099` `period_setup` (the `:3079` phase-0x0C arm, the `:3093`
bit-8 gate, the `:3095` `global_157ac2=4` write); `:2217` `period_setup_live=1`
and `:2408` the begin call; `:5979-6044` `fifa96_match_action_phase_1D`);
`src/fifa96_engine/fifa96_match_handlers.c` (`:5030` action row `0x20` NULL;
`:5016`/`:5028` rows `0x19`/`0x1F`; `:5091` phase `0x10` NULL; `:5117` phase
`0x1D` NULL; `:4806-4836` `fifa96_match_phase_10`); `src/fifa96_engine/fifa96_match_act.c`
(`:38-65`, the act seam + the NULL-row tick guard); `src/fifa96_loader/fifa96_action_handlers.c`
(`:1129-1147`, the situation table; situation 8 = `{NONE,7,0}` at `:1142`);
`src/fifa96_loader/fifa96_input.c` (`:86-120`, `fifa96_input_range_words` — no
`0x100` bit). Docs cross-cited: FU-175 (§1/§2), FU-189 (§1/§3/§6), FU-194
(§1/§2/§3/§6.1), FU-195 (L2), FU-205 (§2/§3/§4/T5.2), FU-206 (§1/§4 Tier-3
#12/§5 PR-7/PR-8), the phase-18 plan. No writes beyond this slice file and the
workspace report; no rename/label/comment/function/script/project save; no git.

---

**Erratum (vs FU-194 §3.2).** FU-194 §3.2 diagrams the arm-0 tail as
`0x899a2 MOV EDX,0x20; 0x899a7 MOV EAX,[ESP+8]; 0x899ab CALL 0x7D9A4`, and its
closing erratum already corrects the row-`0x19` install to "one install per team
on `records[0]`". This slice confirms both first-hand and adds the precise loop
indexing: the record loop's `+0x14C`/`+0x14D` reads/clears at `ESI = records[i-1]`
address `records[i]`'s bytes, so the loop covers `records[1..10]` (not
`records[1..10]` off-by-one); the base `0x8990A` covers `records[0]`. No prior
section is rewritten.

---

## Landing (M2 phase-19 T2, 2026-10-13)

The phase-`0x1D` slot is REGISTERED (`fifa96_match_handlers.c`, `{0x1D, NULL}` →
`fifa96_match_action_phase_1D`), so the act-7 route derived here now dispatches
the body from the clock-tail act seam. §6.2 ("does the act-7 body run? — No") is
superseded for the `[0x157AC2] == 4` entry: the T1/FU-213 period-index roll feeds
the mode-0 sit-8 selector in `fifa96_match_run_phase_drive`, and the body's arm 0
runs (phase-0x10 write + row-0x19 team-base install). The remaining §6.4 landings
are still legs: the arm-0 coupled row-`0x20` install (FU209-L2, T3), the row-`0x20`
body `0x84EEC` (FU209-L3, T3), the arm-8 selection block (FU209-L4), the arm-9
`[0x157AC0]` write-back (FU209-L5) and the arm-2 contact gate (FU209-L6). Census
`+1` (phase `0x1D`) → `52/80 = 65%`; FU209-L1 (the act-7 producer inert) is
resolved for the `global_157ac2 == 4` arm, the bit-8 arm stays FU208-L1.
