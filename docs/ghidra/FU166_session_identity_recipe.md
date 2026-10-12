# FU-166 — played-session identity (`[0x14C32A]`, FU-155 L155.1) and the runtime recipe

> Phase-13 second recon wave R14 (read-only), 2026-10-11. Program `/FIFA96.EXE`
> (case-sensitive), Ghidra MCP, scripts disabled; LE flat-image addresses;
> globals at the flat address. No repo file was written besides this slice and
> `r14-report.md`; no git command was run; the ISO was only read.
>
> **This slice closes FU-155 leg L155.1** (which front-end session is the
> played match / what `[0x14C32A]` carries there) with first-hand static +
> working-tree frame evidence, and supplies the DOSBox-X runtime recipe that
> confirms it (exact addresses, commands, expected evidence; one tooling leg).

**Answer in one line.** `[0x14C32A] = ([0x14C1D0] == 4)` is a **per-GAME-SELECT
session flag, not a played-match constant**: all six `FUN_0001B7B8` callers
pass the modes `{5, 0, 1, 2, 3, 4}` in that order, and **only the Practise
item** (hub case 8 → `FUN_00032DE0`, mode 4) yields byte **1**; **every
ordinary played match** — Friendly (mode 0), Leagues (1), Tournaments (2),
Playoffs (3), the Modem path (0), the boot/idle demo (5) — yields byte **0**.
The polarity pair (`0x4B198` `!=0` → scheduler; `0x8B65B` `==0` → ball machine)
is therefore consistent *per session*: the machine family belongs to byte-0
sessions (all regular matches), the queued goal/screen family belongs to the
single byte-1 session (Practise). The project's live model (engine seed 1 +
machine every frame) models the Practise session's scheduler while driving the
regular session's machine — a composite; assessment in §7, recipe in §6.

## 1. Method (all first-hand this session)

* Fresh `disassemble_function 0x1B7B8`, `0x32DE0`, `0x24B00`; decompiles
  `0x1B7B8`, `0x32DE0`, `0x24B00`, `0x1FC50`, `0x16350`, `0x4B454`.
* Fresh `search_instructions` for the call censuses: `1b7b8` (6 sites, exact
  bytes quoted §3), `24b00` (3 sites), `18644` (table jump: 1 site),
  `1472de`/`1472dd` (widget active-byte writers: `FUN_00016350` only).
* Disassembly windows: `0x18860..0x188D0` (the hub selector + table jump),
  `0x188CF..0x18978` (all eleven case blocks), `0x18254`, `0x24BDE`,
  `0x28A98`, `0x2FADC`, `0x31BDC`, `0x32DE0..0x32E1C`, `0x186B8`.
* `read_memory 0x18644` (the 11-cell dispatch table), `0x1472F0`-family
  (string-pointer table: zero in the static image), `0x1472D0` (widget state:
  zero), `inspect_memory_content 0x100A9C` (the `ftmsel.inv`/`ptmsel.inv`
  debug names).
* Fresh `get_xrefs_to`: `0x32DE0` (1: `0x18943`), `0x18644` (1), `0x105090`
  (14), `0x104FB8` (8), `0x105094` (40), `0x104FC4`/`0x104FC0` (2 each),
  `0x100A9C`/`0x100AA8` (1 each), `0x4B454` (decompile: returns the byte).
* Repo tooling, read-only: `tools/fifa96_patch.py --print-overwrite` for the
  probe targets, `tools/fifa96_probe.py`, `tools/trace_probe.sh`,
  `run-fifa96-capture.sh`, `tools/keys/*`.
* Working-tree capture frames read directly (first-hand pixels; `captures/` is
  git-ignored, present in the working tree):
  * `captures/session-probe-mode-1/frames/fifa96_004-0005.png` — GAME SELECT;
  * `captures/session-probe-s8-2/frames/fifa96_004-0024.png` — FRIENDLY screen;
  * `captures/session-probe-14c18-v4/frames/fifa96_004-0108.png` — PRACTISE
    screen.
  The committed FU-15/FU-16/FU-18 docs describe the same screens from the same
  sessions; they are the durable record.

## 2. The byte model (re-verified byte-exact)

`FUN_0001B7B8` (`disassemble_function`, 14 instructions):

```
0x1B7B8 PUSH EDX
0x1B7B9 MOV [0x14C1D0],AL        ; companion session byte
0x1B7BE AND EAX,0xff
0x1B7C3 CMP EAX,0x4
0x1B7C6 JNZ 0x1B7D1
0x1B7C8 MOV byte [0x14C32A],1
0x1B7CF POP EDX / RET
0x1B7D1 XOR AH,AH / XOR EDX,EDX
0x1B7D5 MOV byte [0x14C32A],AH   ; 0
0x1B7DB MOV dword [0x15B680],EDX ; 0  (screen leg reset)
0x1B7E1 POP EDX / RET
```

So `[0x14C32A] = ([0x14C1D0] == 4)` exactly (FU-155 §3 stands); the mode is a
plain argument, and the only value that produces byte 1 is 4. The third
writer (`0x32DFF MOV byte [0x14C32A],1`) is redundant with the `FUN_0001B7B8(4)`
that follows it two instructions later (§3, row 6).

## 3. The six `FUN_0001B7B8` call sites, first-hand (fresh census)

`search_instructions 0x0001b7b8` = **6** sites (instructions scanned 234,954);
bytes and the mode loaded into EAX immediately before each CALL:

| # | site | containing fn | bytes at the site | mode | launch after |
|---|---|---|---|---|---|
| 1 | `0x18261` | `FUN_00018108` | `b8 05 00 00 00` MOV EAX,5; `e8 52 35 00 00` | **5** | `MOV EBP,1` → `0x18357 CALL 0x493A0` (selector 1) |
| 2 | `0x24BED` | `FUN_00024B00` | `31 c0` XOR EAX,EAX; `e8 c6 6b ff ff` | **0** | the screen returns 4 → caller launches |
| 3 | `0x28AB0` | unfunctioned competition cluster | `b8 01 00 00 00`; `31 f6`; `e8 03 2d ff ff` | **1** | `0x28AB5 CALL 0x18014` |
| 4 | `0x2FAEB` | unfunctioned (`FUN_0002F9DC`+0x114 region) | `b8 02 00 00 00`; `e8 c8 bc fe ff` | **2** | `0x2FAF0 CALL 0x18014` |
| 5 | `0x31BF0` | `FUN_00031AA4` | `b8 03 00 00 00`; `e8 c3 9b fe ff` | **3** | `0x31BF5 CALL 0x18014` |
| 6 | `0x32E0B` | `FUN_00032DE0` | `c6 05 2a c3 14 00 01` store 1; `b8 04 00 00 00`; `e8 a8 89 fe ff` | **4** | `0x32E10 CALL 0x18014` |

(Every call's rel32 was decoded and lands on `0x1B7B8`; e.g.
`0x18943 e8 98 a4 01 00` → `0x18948+0x1A498 = 0x32DE0`.)

`FUN_00032DE0` full body (`disassemble_function`, 18 instructions) — the only
byte-1 producer path:

```
0x32DE0 MOV EAX,1
0x32DE5 CMP EAX,1 / JC 0x32E17 / JBE 0x32DF3 / CMP EAX,4 / JZ 0x32DFF / JMP 0x32E17
0x32DF3 MOV EAX,3 ; CALL 0x24B00            ; team-select loop (param 3)
0x32DFF MOV byte [0x14C32A],1               ; explicit byte-1 store
0x32E06 MOV EAX,4 ; CALL 0x1B7B8            ; mode 4
0x32E10 CALL 0x18014                        ; run the match
0x32E15 XOR EAX,EAX
0x32E17 TEST EAX,EAX / JNZ 0x32DE5 / RET    ; loop while the screen returns nonzero
```

So `FUN_00032DE0` = *enter the param-3 team-select screen; when it returns 4
(user confirmed Play), set mode 4 / byte 1 and run the match*. Its sole caller
is `0x18943` (fresh `get_xrefs_to 0x32DE0` = 1).

`FUN_00024B00` (site 2) sets mode 0 up-front, first-hand disassembly:

```
0x24BDF XOR EAX,EAX ; 0x24BE1 MOV [0x105478],EAX ; 0x24BE6 CALL 0x10020
0x24BEB XOR EAX,EAX ; 0x24BED CALL 0x1B7B8      ; <- mode 0
```

Its three callers (fresh census) pass params {1, 2, 3}: `0x18901`
(`FUN_00018680`, ECX=1), `0x2098B` (`FUN_00020954`; `MOV EAX,2` then
`CMP EAX,4 / JNZ`, `CALL 0x18014` on 4), `0x32DF8` (`FUN_00032DE0`, param 3).
Its decompile shows the INV switch: params 1/2 load `ftmsel.inv`
(`s_ftmsel_inv`), param 3 loads `ptmsel.inv` (`s__ptmsel_inv`, code passes
`+1`); fresh xrefs: `0x100A9C` ← `0x24C81` only, `0x100AA8` ← `0x24D35` only,
both inside `FUN_00024B00`.

## 4. The hub dispatch — how each mode is selected (GAME SELECT)

`FUN_00018680`'s tail loop: selector at `0x18865..0x188C7`, table at
`0x18644` (11 link cells, read first-hand):

```
selector (0x18865): XOR EAX,EAX; TEST ESI,ESI; JNZ exit
  EDX = [0x105090]            ; screen-state re-entry: 1->EAX=5, 2->6, 3->7
  [0x105094]!=0 -> EAX=3      ; exit marker
  [0x104FB8]!=0 -> EAX=4      ; team-select re-entry
  EAX<=0xA; JMP CS:[EAX*4+0x18644]
table: 0x188CF 0x188D9 0x18927 0x1894A 0x188E3 0x1892E 0x18935 0x1893C
       0x18943 0x18961 0x1894E
case blocks: 0x188CF CALL 0x1FC50; JMP 0x18961   ; case 0
             0x188D9 CALL 0x1DD40                 ; case 1 (outer driver)
             0x18927 CALL 0x206FC                 ; case 2
             0x1894A MOV ESI,ECX; JMP 0x18961     ; case 3 (exit hub)
             0x188E3 sub-dispatch (0x18670)       ; case 4
             0x1892E CALL 0x28960                 ; case 5
             0x18935 CALL 0x2F9DC                 ; case 6
             0x1893C CALL 0x31AA4                 ; case 7
             0x18943 CALL 0x32DE0                 ; case 8
             0x18961 TEST ESI,ESI; JZ selector    ; case 9 (loop)
             0x1894E CALL 0x17F80; 0x18953 CALL 0x18108; 0x1895A CALL 0x17DF0
loop return: 0x1895F MOV EAX,EBX(0)  (case 0/3 JMP 0x18961, EAX untouched)
```

Case 4 (`0x188E3`) is the team-select sub-dispatch: `MOV EAX,ECX(1)`, and when
`[0x105094]==0`, `CALL 0x24B00` (**param 1**, the FRIENDLY screen); on its
return 4, `EDX=EAX-1=3` → `0x1891A CALL 0x18014` (play). Cases 5/6/7/8 select
`FUN_00028960` / `FUN_0002F9DC` / `FUN_00031AA4` / `FUN_00032DE0`, whose
case-body regions contain the mode writers of §3 rows 3–6 (the mode-1/mode-2
sites sit in the unfunctioned case-body regions of the first two, first-hand
bytes §3); case 8 is the only one that can produce byte 1.

**Which item selects which case.** `FUN_0001FC50` (GAME SELECT, the case-0
screen) registers nine `FUN_00015960` widgets whose packed ids carry the codes
`4,5,6,7,8,9,1,2,3` (in registration order; strings
`0x81,0x1BF,0x1BB,0x1BD,0xEA,0xB3,0xD8,0xC2,0xF3`), returns the activated
widget's code through `FUN_00016350` (`LAB_000201c0` returns `iVar8` for every
non-negative code), and sets `iVar8=10` on a 3-second input-idle timer. The
case-0 block `JMP 0x18961` leaves EAX = that code in place for the selector,
which keeps it when `[0x105090]` is not 1/2/3 and `[0x105094]/[0x104FB8]` are
0 → **the GAME SELECT item code indexes the hub table directly** (this also
resolves FU-66 §9's "no straight-line path to index 1/2/8/9/10": they are
reached through the case-0 return, which the FU-66 §3 decode did not model).

The identity of the two byte-relevant items is **observed, not inferred from
the code assignment**:

* The first right-column item (initial highlight) opens the FRIENDLY screen
  (frame `mode-1/004-0005`: right column `Friendly, Leagues, Tournaments,
  Playoffs, Practise`; `s8-2/004-0024`: title `FRIENDLY`, Home/Visitor rows,
  Brazil–Italy, Play). Its screen is `ftmsel.inv` → `FUN_00024B00` param 1/2 →
  mode 0 (`0x24BEB XOR EAX,EAX`) → the match runs with `[0x14C32A]==0`.
  (Param 2 comes from `FUN_00020954` ← hub case 2 only; either param writes 0.)
* The fifth right-column item (`down`×4; FU-15 run 4, committed doc
  "TOURNAMENT → PLAYOFF → PRACTISE … practise team select") opens the PRACTISE
  screen (frame `14c18-v4/004-0108`: title `PRACTISE`, header `Practise Team`,
  single International/Brazil row, Play). Its screen is `ptmsel.inv` →
  `FUN_00024B00` **param 3** — whose *only* caller is `FUN_00032DE0`
  (hub case 8) → mode 4 → `[0x14C32A]==1` for the match.
* The reverse assignment (Friendly=case 8) is **falsified**: case 8's screen is
  the single-team PRACTISE layout, while the item the game enters first shows
  the two-column FRIENDLY layout.

The three other right-column items (Leagues/Tournaments/Playoffs) run cases
5/6/7 → modes 1/2/3 (§3 rows 3–5) → byte 0. Case 8 is the **only** byte-1
match launcher in the image (its only caller is the table cell; `FUN_0001B7B8`
mode 4 has no other site), so regardless of any remaining intra-column code
permutation, **the only byte-1 match session is Practise**. The demo paths
round it out: the 3-second GAME SELECT idle timeout returns 10 → case 10 →
`FUN_00017F80` + `FUN_00018108` (mode 5) — the looping attract demo match; and
at boot `FUN_00018680` calls `FUN_00018108` directly when `[0x104FC4]==0`
(first-hand `0x186B9 CMP [0x104FC4],0 / JNZ / CALL 0x18108`; the two flag
writers are `FUN_000183A0` only). Both demo paths are byte 0.

## 5. Conclusion — session identity and polarity consistency

| session (GAME SELECT item) | hub case | launcher | mode | `[0x14C32A]` |
|---|---|---|---|---|
| Friendly (1st right) | 4 sub-dispatch | `FUN_00024B00(1)` → `FUN_00018014` | 0 | **0** |
| Leagues (2nd right) | 5 | competition cluster (`0x28AB0`, the `FUN_00028960` case-body region) → `FUN_00018014` | 1 | **0** |
| Tournaments (3rd right) | 6 | `FUN_0002F9DC` case-body region (`0x2FAEB`) → `FUN_00018014` | 2 | **0** |
| Playoffs (4th right) | 7 | `FUN_00031AA4` → `0x31BF0` → `FUN_00018014` | 3 | **0** |
| **Practise (5th right)** | **8** | `FUN_00032DE0` → `FUN_00024B00(3)` → `FUN_00018014` | **4** | **1** |
| Load Game (1st left) | 9 + `FUN_000314B4(0)` dialog | — (no match) | — | 0 (unchanged) |
| Options (2nd left) | 1 | outer driver `FUN_0001DD40` | — | 0 |
| Modem Setup (3rd left) | 2 | `FUN_000206FC` → `FUN_00020954` → `FUN_00024B00(2)` | 0 | **0** |
| Quit to DOS (4th left) | 3 | exit | — | 0 |
| idle 3 s / boot demo | 10 / `[0x104FC4]==0` | `FUN_00018108` | 5 | **0** |

1. **The ordinary played match carries `0`.** Friendly (the game's first,
   initial-highlight item) and all three competitive items run byte-0 sessions;
   the goal scan's byte-0 arm (`0x8A957 JZ 0x8AA7B`, FU-155 §4.2) is the direct
   goal path in every one of them, and the ball machine (`0x8B65B ==0` →
   `0x8B664`) is their per-frame ball system.
2. **Byte 1 is the Practise session only** — the single-team `ptmsel` flow with
   the redundant explicit store at `0x32DFF`. Its per-frame system is the
   queued one: the scheduler site `0x4B198 !=0` → `FUN_000948AC` and the
   situation queue (`0x8A957` queued arm); the ball machine is natively dead
   there (the byte-1 ball producers are FU-155 L155.2's event writers).
3. **The polarity pair is consistent per session, not per frame.** Because the
   byte's writers are front-end only (FU-155 §5), each session fixes one value
   and exactly one of {scheduler, ball machine} runs. There is no value that
   makes both systems run natively; the engine's composite (§7) does exactly
   that.

## 6. The DOSBox-X runtime recipe (settle/confirm at the live frame)

Everything needed already exists in-repo: Stages 1/2/4 use the proven plain
trampoline as-is; only the optional direct byte read (§6.3, leg FU166-L1) needs
a small new cave variant. The staged plan, cheapest first:

### 6.1 Stage 1 — mode probe at the writer entry (existing tooling)

Patch `CALL 0x1B7B8`'s *entry* with the plain capture-eax trampoline: first-hand
`python3 tools/fifa96_patch.py --iso game/FIFAPCCD96.iso --target 0x1B7B8
--print-overwrite` → `6` (the displaced prefix `52 a2 d0 c1 14 00` = `PUSH EDX`
+ `MOV [0x14C1D0],AL`, no branch; resume `0x1B7BE`). Every caller sets full EAX
(`MOV EAX,imm32`/`XOR EAX,EAX`, §3), so the packed high half is the clean mode.

```
CAPTURE_EAX=1 KEYS_FILE=<friendly.keys> sh tools/trace_probe.sh 0x1B7B8 21 probe-166-friendly
python3 tools/fifa96_probe.py captures/session-probe-166-friendly/trace.bin \
    --target-link 0x1B7B8 --overwrite 6 --expect-site 21 --capture-eax
```

*Keys:* the FU-16 run-2 timeline (verified in `captures/session-probe-mode-2`
and committed in `docs/ghidra/FU16_mode_map.md`): `enter@10 enter@18 down@23
down@28 down@33 down@38 right@43 right@46 enter@49` (match starts; the extra
`enter@60/75/90/105` were in-match panels). Reconstruct that single AUTOTYPE
line as a keys file (`tools/fifa96_keys.py --check` first).

*Expected evidence (friendly):* `T_PROBE site=21 mode=0` frames (the team-select
entry at caller `0x24BED`/return `0x24BF2`, and no writer between the screen
and the match), plus any boot-demo `mode=5` frames before the keys land; **no
`mode=4` frame at any point**. *Practice run:* the same probe with the FU-15
run-4 pattern (`enter@10 … down×4 … enter` reaching the PRACTISE screen, keys
in leg FU166-L2) plus the Play press → a `mode=4` frame with the caller near
`0x32E10` immediately before the match.

### 6.2 Stage 2 — system-presence probes (existing tooling, decisive)

The gate's *effect* is directly observable through its two per-frame consumers,
and both function entries have branch-free, relocation-free 5-byte prologues
(first-hand: `0x8BAF0` = `53 51 52 56 57` = PUSH EBX/ECX/EDX/ESI/EDI; `0x948AC`
identical; `--print-overwrite` = 5 both), so the *existing* plain trampoline
patches them safely (the cave re-executes the five pushes and resumes at +5):

```
KEYS_FILE=<friendly.keys> sh tools/trace_probe.sh 0x8BAF0 22 probe-166-machine
KEYS_FILE=<friendly.keys> sh tools/trace_probe.sh 0x948AC 23 probe-166-sched
python3 tools/fifa96_probe.py captures/session-probe-166-machine/trace.bin \
    --target-link 0x8BAF0 --overwrite 5 --expect-site 22
python3 tools/fifa96_probe.py captures/session-probe-166-sched/trace.bin \
    --target-link 0x948AC --overwrite 5 --expect-site 23
```

*Expected evidence:* `0x8BAF0` is the ball machine (the `0x8B664` call target)
and `0x948AC` the scheduler (the `0x4B1A1`/`0x743EA` call target). In the
friendly match: `site=22` frames on the granted-frame cadence (machine runs)
and **zero `site=23` frames** (the scheduler is never called) → the byte was 0
for the whole match. In the practise match the mirror image (`site=23` frames,
no `site=22`) → byte 1. A zero-keys run is the cheapest control: the boot
attract demo (§4) plays a match unprompted → `site=22` frames with no input at
all.

### 6.3 Stage 3 — direct byte watch at the gates (new cave variant, leg FU166-L1)

To read the literal byte at the frame the systems run: patch a trampoline over
`0x8B65B` (`CMP byte [0x14C32A],0`; overwrite **7**, resume `0x8B662` — the
`JNZ` stays in place) and, as a cross-check, over `0x4B198` (same 7-byte shape,
resume `0x4B19F`, `JZ`). **Hazard (new, first-hand):** the displaced prefix
`80 3d 2a c3 14 00 00` contains an absolute operand that the LE loader
relocates *in place*; a byte-for-byte copy into the cave would read the
unrelocated link address `0x14C32A`, so the cave must **replace** the CMP with
a delta-correct one and hand the resumed `JNZ`/`JZ` the same ZF (the tool's
`overwrite_len` rejects branch opcodes but does not detect absolute operands).
Add a `capture_byte` variant to `tools/fifa96_patch.py`:

```
bits 32
org {cave:#x}                              ; default cave 0x6728D (0x103 B)
probe_byte:
        pushad
        pushfd
        push es
        mov  ax, ss
        mov  es, ax
        mov  ebp, [esp+40]                 ; call's return (runtime) = target+5+delta
        add  ebp, {adjust}                 ; EBP = resume runtime (target+overwrite+delta)
        movzx eax, byte [ebp + {0x14C32A - resume:#x}]  ; gate byte (delta-correct)
        shl  eax, 0x10
        or   eax, strict dword {site:#x}
        mov  esi, eax                      ; site word = byte<<16 | site
        mov  ebx, [esp+44]                 ; caller split (attribution only)
        mov  edx, ebx
        shr  edx, 16
        and  ebx, 0xFFFF
        sub  esp, 48
        mov  edi, esp
        xor  eax, eax
        mov  ecx, 12
        cld
        rep  stosd
        mov  edi, esp                      ; ES:EDI -> DPMI reg struct
        mov  [esp+0x04], esi
        mov  [esp+0x08], ebp
        mov  [esp+0x10], ebx
        mov  [esp+0x14], edx
        mov  ax, 0x0300
        mov  bx, 0x0061
        xor  cx, cx
        int  0x31
        add  esp, 48
        pop  es
        ; the displaced CMP is REPLACED, never re-executed (absolute operand):
        cmp  byte [ebp + {0x14C32A - resume:#x}], 0  ; ZF for the resumed JNZ/JZ
        pushfd                             ; park the CMP flags below the popad frame
        lea  esp, [esp + 8]                ; skip the parked flags -> pushad frame
        popad                              ; restore every game register
        lea  esp, [esp - 40]               ; back to the parked flags
        popfd                              ; ZF restored (lea/push/pop keep flags)
        lea  esp, [esp + 40]               ; drop parked flags + pushfd + call return
        jmp  {resume:#x}                   ; the JNZ/JZ sees the correct ZF
```

Notes (first-hand against `CAVE_TEMPLATE`/`FVGT_ENTRY_TEMPLATE`): delta
recovery is the proven FVGT-entry pattern (`[esp+4]`/`sub {call_ret}`) — here
the byte address is `EBP + (0x14C32A − resume)`, an immediate-only operand, so
the cave itself needs no relocation; the post-INT `cmp` reproduces the
displaced CMP and the `pushfd/lea/popad/lea/popfd/lea` chain keeps ZF intact
across `popad` without touching any game register (`push`/`pop`/`lea`/`mov`
do not modify flags); the TSR's INT-61 dump maps the site word's low half to
`site` and the high half to `mode` (`tools/fifa96_probe.py --capture-eax`
prints it), so `mode` = the byte (0/1). No branch lies in the 7-byte displaced
prefix (branch rejection was verified live: `--print-overwrite 0x8B664`
refuses with "branch opcode E8 in overwrite prefix" — hence the gate sites,
not the CALLs, are the probe targets). Decode:

```
KEYS_FILE=<friendly.keys> sh tools/trace_probe.sh 0x8B65B 22 probe-14c32a-f1
python3 tools/fifa96_probe.py captures/session-probe-14c32a-f1/trace.bin \
    --target-link 0x8B65B --overwrite 7 --expect-site 22 --capture-eax
```

*Expected evidence:* the gate is inside `FUN_0008AF38`, reached from the match
frame body `FUN_0004B100` at `0x4B1A6` once per granted frame, so the friendly
run should emit a dense burst of `T_PROBE site=22 mode=0` over the whole match
and **never** `mode=1`; the practice run the mirror image (`mode=1`, never 0).
Run the same patch at `0x4B198` as the scheduler-side cross-check (friendly:
`mode=0` per frame, i.e. the scheduler is skipped; practice: `mode=1`). The
`0x743CF` block (`MOV AL,[0x14C32A]`, overwrite 5, resume `0x743D4`) is an
alternative byte-watch target — its displaced prefix is *followed* by three
absolute-operand stores that the resume executes normally, but the cave must
reproduce the `MOV AL` result (merge: `(game_EAX & ~0xFF) | byte`) because AL
feeds the `0x743E6 CMP BL,AL` gate; the block runs only in the setup/reset
path (FU-155 L155.4), so it is a cross-check, not the primary watch.

### 6.4 Stage 4 — launch-path presence probes (optional, existing tooling)

Entry patches with branch-free prefixes work today: `--print-overwrite` = 5 at
`0x32DE0`, `0x18108`, `0x24B00`, `0x2F9DC`, `0x31AA4`. A plain trampoline at
`0x32DE0` fires **only** if hub case 8 ran (the Practise flow); at `0x18108`
only for the boot/idle demo. These corroborate §4's reachability model (no
register/memory reads needed).

### 6.5 Reused rig facts

`tools/trace_probe.sh TARGET SITE SESSION` (with `CAPTURE_EAX=1` for the EAX
pack), `run-fifa96-capture.sh` (`HEADLESS=1 TIMEOUT=120`, `KEYS_FILE=`,
`CAPTURE_VIDEO=1`), `tools/fifa96_keys.py` (validate + emit the one AUTOTYPE
line), `tools/fifa96_frames.py --fps 1` for screen verification; the TSR
(`make tsr`, `tsr/fifa96_capture.asm`) has 5 site slots and the DPMI probe dump
(the FU-16 §"positive control" run exercised capture-eax end-to-end: 145
frames). Patched ISOs go to `build/fifa96-trace-<site>.iso`; the retail ISO is
never written.

## 7. The engine's composite — benignness under each session value

Engine state (first-hand repo reads): `session_gate_14c32a` seeds 1 in
`fifa96_match_run_begin` (`fifa96_match_run.c:1821`, comment "live-session gate
1"), 0 in init/teardown (`:1497`, `:1740`); the per-frame scheduler runs iff the
gate (`:3270` → `fifa96_match_run_screen_schedule`), the goal queue routes on
the gate (`:2366`), the referee foul arm ORs `foul_kind==3` with the gate
(`:4773`); the ball machine runs unconditionally (`:3291`, disclosure comment
`:3034..3046`, L14.8.d). The gate also flows to handlers as `s->session_gate`
(`fifa96_match_handlers.c:584`) where the byte-0 arms are guarded off when it
is 1 (`:1407`, `:1687`, `:1721`, `:1837`).

**Under byte 0 (the ordinary played match — the statically settled value):**
the machine-every-frame half is native-correct; the unfaithful halves are
(a) the scheduler + queued goal routing, which native byte-0 sessions never
run (their goals take `0x8A957 JZ 0x8AA7B` → the direct `add_goal`/phase arm,
so the engine's score timing/`score_last_event`/`situation_id`/
`screen_leg`-timer state step through a machine that is natively idle), and
(b) the byte-0 handler arms, which native runs and the engine skips. Bounded:
every divergence is run-object state (no memory, no indirection, no safety
effect), deterministic, and centralised where the disclosures already are;
the fixtures/goldens pin the composite (M1 immutable; M2 re-pin only with
reason + diff), so a later faithfulness flip (seed 0 for live matches) is a
deliberate, reviewable re-pin — not a silent behaviour change.

**Under byte 1 (the Practise session — now known to be a different session):**
the scheduler/queue half is native-correct and the byte-0 arms are correctly
off, but the machine every frame is natively dead — the engine then advances
the visible ball from a system that only exists in byte-0 sessions (the byte-1
ball producers of L155.2 stay unported). Same bounding argument.

**Verdict.** `session_gate_14c32a = 1` + machine-every-frame is **not a native
window under either value** (FU-155 §6.3 disclosed this; this slice identifies
which half is wrong under each). It is benign for the engine's own contract —
bounded, deterministic, test-visible, disclosed under L14.8.d — but it is the
*Practise* session's scheduler bolted to the *regular* session's ball machine,
so neither the live-match goal path nor the live-match ball path corresponds to
one native session. If a later slice makes the engine's live match native-
faithful, the settled choice is **seed 0** (ordinary match: machine + direct
goal arm), with the Practise byte-1 semantics reserved for a practise session
model; that change is implementation work, not this recon's.

## 8. Numbered legs (FU-166)

* **FU166-L1 — byte-watch cave in `tools/fifa96_patch.py`.** The §6.3
  `capture_byte` variant is drafted but not implemented; implement + a positive
  control run (friendly → `mode=0` burst at `0x8B65B`; practise → `mode=1`)
  to close the runtime leg. Static identity (§5) does not depend on it.
* **FU166-L2 — Practise Play keys.** The "Practise Team" screen's exact keys to
  reach Play are not in a committed keys file (FU-15 run 4 reached the screen
  only; `/tmp/opencode/fu15/v4.keys` is gone). Discovery run: replay FU-15 #4's
  pattern with `CAPTURE_VIDEO=1` + `fifa96_frames.py`, then walk to Play
  (bottom-right, frame `14c18-v4/004-0108`) with down/right/enter and confirm
  the match entry by frames.
* **FU166-L3 — runtime mode-sequence confirmation.** Optional run of §6.1 with
  the committed-friendly keys and a Practise key file; expected `mode=0`
  (friendly) and `mode=4` (practise) hits, no cross-contamination.
* **FU166-L4 — cases 1/2/9 screen bodies.** The Options/Modem/Load item flows'
  internals (outer driver `FUN_0001DD40`, `FUN_000206FC`, `FUN_000314B4`) are
  labelled here but not decomposed; no match-mode impact (no mode writers in
  their code paths, fresh census §3).

## 9. Errata observations (first-hand; prior sections not rewritten)

* **FU-155 §3 "Modes the call sites pass: 0 (menu), 2, 3, 4 (setup walk), 5
  (intro loop)"** — corrected: the six sites pass `5, 0, 1, 2, 3, 4` (site
  order §3 here); their identities are GAME SELECT items (table §5), not a
  "setup walk". Mode 5 is the boot/idle **demo match** path `FUN_00018108`
  (which launches `0x493A0(1)`), not an intro loop.
* **FU-155 §6.2 / §7 L155.1 and the project assumption** ("the played match is
  the byte == 1 session"): the played match is byte **0**; byte 1 is the
  **Practise** session (`ptmsel`, `FUN_00032DE0`). L155.1's static half is
  closed; L155.2 (byte-1 ball producers) now scopes to the Practise session.
* **FU-66 §9 "the decoded selector has no straight-line path to index 1 (or to
  2/8/9/10)"** — resolved: the case-0 screen's return value (the GAME SELECT
  item code, or 10 on the idle timer) is preserved by the selector when
  `[0x105090]` is not 1/2/3 and `[0x105094]`/`[0x104FB8]` are 0, so the item
  code *is* the table index (§4); index 10 is the 3-second idle demo.
* **FU-68 §4.1/FU-142 L.5 "setup entry `FUN_00032DE0`"** — refined: `FUN_00032DE0`
  is the Practise launch loop (mode 4 / byte 1); its type-4 "competition arm"
  default settings (FU-68 §4.2) are the Practise session's 1-minute preset.
* **FU-148 §7.5's byte-!=0 HUD re-label** (string ids `0x1EA`/`0x1E9`,
  `FUN_0004B454` returns the raw byte — decompile re-verified) is the Practise
  session's HUD; the mode is therefore "the practise/extra-time presentation
  family", not a per-match setting of ordinary matches.

## 10. Evidence index (all first-hand this session)

* `disassemble_function`: `0x1B7B8`, `0x32DE0`.
* `disassemble_bytes`: `0x18860` (112 B; selector+table jump), `0x188CF`
  (170 B; case blocks), `0x18254`, `0x24BDE`, `0x28A98`, `0x2FADC`, `0x31BDC`,
  `0x2097C`, `0x186B8`, `0x314E8`, `0x1C5B8`, `0x32BC4`, `0x2AD48`, `0x329B8`,
  `0x31514`, `0x30E24`, `0x30F39`, `0x2DC00`.
* `decompile_function`: `0x1B7B8`, `0x32DE0`, `0x24B00`, `0x1FC50`, `0x16350`,
  `0x15864`, `0x4B454`.
* `search_instructions`: `0x0001b7b8` (6), `0x00024b00` (3), `18644` (1),
  `1472de` (3), `1472dd` (5), `1465a0` (23), `146620` (12), `105090` set (14
  xrefs), `104fb8` (8), `105094` (40).
* `get_xrefs_to`: `0x32DE0` (1), `0x18644` (1), `0x105090`, `0x104FB8`,
  `0x105094`, `0x104FC4` (2), `0x104FC0` (2), `0x100A9C` (1), `0x100AA8` (1).
* `read_memory`: `0x18644` (44 B), `0x1472F0`/`0x1474F4`/`0x1479B0` (zero in
  the image), `0x1472D0` (zero). `inspect_memory_content 0x100A9C`.
* Repo tooling: `tools/fifa96_patch.py --print-overwrite` →
  `0x1B7B8`=6, `0x8B65B`=7, `0x4B198`=7, `0x32DE0`=5, `0x18108`=5, `0x24B00`=5,
  `0x2F9DC`=5, `0x31AA4`=5; `0x8B664` refused (branch `E8` in prefix).
* Working-tree frames (first-hand pixels): `mode-1/004-0005`,
  `s8-2/004-0024`, `14c18-v4/004-0108` (paths under `captures/session-*`).
* Engine: `src/fifa96_engine/fifa96_match_run.c:1497/1740/1821/2366/3270/3291/4773`
  and `:3034..3046`; `fifa96_match_handlers.c:584/1407/1687/1721/1837`;
  `include/fifa96_engine/fifa96_match_run.h:604/615`.

---

## Status append — T3 implementation (phase-13, 2026-10-11)

**The engine seed is settled to 0 (R14).** `fifa96_match_run_begin` now seeds
`mr->session_gate_14c32a = 0` — the ordinary played match's value (all
GAME-SELECT items but Practise pass mode 0; byte 1 is the `ptmsel` Practise
session). Consequences:
- the per-frame scheduler stays off and `fifa96_match_run_set_piece` takes the
  native byte-0 direct arm (the `0x8AA7B` path) for the engine's live match;
- the goal-consumer/scheduler fixtures that exercise the **byte-1 family**
  stage `session_gate_14c32a = 1` explicitly (`test_goal_chain_pan_fixture`,
  `test_goal_consumer_chain_fixture`, `test_camera_pan_event_chain`,
  `test_row04_live_pan_arms_camera`, `test_view_pose_feed_arms_camera`,
  `test_natural_goal_end_to_end`); the fallback fixture settles its machine
  explicitly under the byte-0 default;
- the **M2 acceptance tape** stages gate 1 at the kickoff directive so it
  keeps pinning the ported queue/scheduler chain — M2 stays v13 byte-exact
  (`5f7e7692…`), M1 unmoved (`09b726b7…`).

**New leg FU166-L5:** re-derive the M2 acceptance tape under the byte-0
default (the natural pan would take the direct goal arm: score increment +
table-2 situation-6/phase-5 instead of the queue -> scheduler -> post chain).
T4 scope (the acceptance slice owns the tape). FU166-L1..L4 unchanged.

**Fix round 1 (T3 review, 2026-10-11): the act-2 restart leg re-derived.**
The referee act-2 hand-off (`match_run_ref_step_restart`) had compressed the
native decision gate into `foul_kind == 3 || session gate` (leg). Fresh
disassembly resolves it:

* `0x8947A MOV AL,[0x14C32A]; TEST AL,AL; JZ 0x89615` — the end of the
  stage-2 body: **a byte-0 session aborts the hand-off for every contact
  kind** (the act-2 family is the Practise session's);
* `0x89494/0x89499 CMP EAX,6; JZ; CMP EAX,0x10; JNZ 0x89615` — the caller's
  decision word must be 6 or 0x10;
* `0x894A2 CMP byte[0x158882],0; JNZ` + `0x894AB kind==3` +
  `0x894B7 [0x14C32A]==0 -> abort` — the stage-3 gate
  `[0x158882] != 0 || kind == 3 || [0x14C32A] != 0`. **`[0x158882]` is not
  an unwritten cell**: fresh `decompile_function 0x6E724` shows the ball
  block's `+0x96` byte (base `0x1587EC`) is written by `FUN_0006E724` as
  `row_flags & 1` — the ball record's current anim-row **terminal flag**
  (the reset installs row 3, flags `0x07` -> 1; the act-2 body itself
  re-installs row 0/0x49 at `0x8941D`/`0x89464` right before the read).

Because the engine's session gate is binary, the `!= 0` term always holds
once `0x8947A` passes, so the effective native condition is **abort iff the
session gate is 0**; the old compression wrongly let kind-3 contacts reach
the phase-7/6 fork under byte 0 (where the native aborts at `0x8947A`). The
engine now models the byte-0 abort for all kinds (`fifa96_match_run.c`
`match_run_ref_step_restart`, fresh comment) and
`test_engine_referee_restart_default_gate_aborts` pins the default-gate
behavior (soft + kind-3); the byte-1 fork stays covered by
`test_engine_referee_contact_fk`.

**New leg FU166-L6 — the act-2 multi-stage compression.** The native act-2
dispatch (`0x89214` 6-entry jump table, stages 0..5) runs the whistle/phase
0xA (stage 0), the `FUN_0004BEC8` (RET 0) check + `[0x158818]`/`[0x158829]`
stage machinery (stage 1), the kind dispatch + the ball-row re-installs +
the decision-word check (stage 2) and the `[0x158882]`/kind/gate decision
(stage 3) with camera-word gates (`word[0x15883A].hi > 0x20`,
`word[0x158816].hi < 0x78`, `word[0x158848] != 0` aborts). The engine
compresses stages 2/3 into one step and does not model the decision-word
check or the camera gates.

**Erratum (re-review round 2, 2026-10-11) — the round-1 byte-0 conclusion.**
Quoted from the round-1 act-2 paragraph above:

> a byte-0 session aborts the hand-off for every contact kind ... the engine
> now aborts iff the gate is 0 (`match_run_ref_step_restart`, fresh comment)
> and `test_engine_referee_restart_default_gate_aborts` pins the default-gate
> behavior

**Corrected: the abort is an armed one-call split.** Fresh bytes: the abort
(`0x89487`/`0x894B7` `JZ 0x89615`) skips the `0x895E6` teardown
(`[0x15882A]=1`, `[0x158828]=0`, `[0x158808]=0`, `[0x158829]=0`); the
dispatcher reads the **stage byte from memory** (`0x892FE MOV AL,[0x158829]`,
table `0x89214`); the aborting body incremented the stage before the gate
(`0x89478/0x8947F` -> 3), so the next driver call (`FUN_0008AF38`
`0x8B669 CMP dword[0x158808],0` / `0x8B672 CALL dword[0x158808]`) re-enters
at `table[3] = 0x894A2` and bypasses `0x89487`. The stage-3 gate
`[0x158882] != 0 || kind == 3 || [0x14C32A] != 0` then decides; the stage-2
`FUN_0006E724(0x1587EC, 0, ...)` install at `0x8941D` keeps/rerolls the ball
row to 0/0x62/0x63/0x64/0x65 (flags 0x07 -> `[0x158882] = 1`), so **kind == 3
completes under byte 0 on the re-entry call** (the old
`foul_kind == 3 || gate` rule was net-right); kind != 3 installs row 0x49 at
`0x89464` (flags 0x00) and holds armed at stage 3. The engine models the
split (`ref_restart_stage = 2`);
`test_engine_referee_restart_default_gate_split` supersedes the round-1 test.

**FU166-L6 extension — the re-entry semantics and the ball-base writers.**
The ball block (base `0x1587EC`) `+0x96` byte `[0x158882]` is set by every
`FUN_0006E724(base, row, ...)` install as `row_flags & 1`:
`0x887D5` (`FUN_000886D4`, row 3, the match reset -> 1), `0x8C2CE`
(`FUN_0008C24C`, row 0, the kickoff commit -> 1), `0x8A18C` (row 0x48), the
`FUN_0006FE84` appearance-machine pairs (`0x6FF37`/`0x6FF85`), and the act-2
body's own `0x8941D` (keep/reroll -> rows 0/0x62/0x63/0x64/0x65, all flags 0x07 -> 1)
and `0x89464` (row 0x49 -> 0). The engine does not model the byte; a future
port should track the ball row-terminal flag to reproduce the stage-3 gate
without the kind proxy.

---

## T4 status — the acceptance decision + the residuals fold (phase-13, 2026-10-12)

**FU166-L5 decided: keep the M2 acceptance tape's staged byte-1 session (no
re-derivation).** The tape's purpose is to pin the deepest ported chain
end-to-end — the queue -> scheduler -> post -> writer goal family, which is
natively the byte-1 (Practise) session's. The engine's byte-0 default stays
pinned where it belongs (the `fifa96_match_run_begin` seed, asserted at match
start on the tape) and is covered end-to-end by
`test_engine_match_frame::test_natural_goal_fallback_arm` (the direct `0x8AA7B`
arm: score + table-2 situation 6 / phase 5). Re-deriving the tape under byte 0
would replace the deepest chain with the shallower direct arm on the
acceptance tape and re-pin a v14 for no coverage gain; the staged value is a
real native session, not a fabricated one. Recorded in `tests/test_engine_m2.c`
(the v13 acceptance-completion block) and `docs/ENGINE.md` (T4 section).

**FU166-L6 residuals folded (from the T3 round-2 re-review).** The four named
compressions, made explicit here:
- the **kind proxy** — the engine's stage-3 gate passes kind 3 via
  `foul_kind == 3` where the native passes via the ball-row flag
  `[0x158882] = 1` left by the keep/reroll install (`0x8941D`); the future
  port tracks the ball row-terminal flag (the writer catalogue above);
- the **decision-word check** (`0x89494/0x89499`: the caller's decision word
  must be 6 or 0x10) is not modeled;
- the **camera-word gates** (`word[0x15883A].hi > 0x20`,
  `word[0x158816].hi < 0x78`, `word[0x158848] != 0` aborts) are not modeled;
- the **teardown poke** — the `0x895E6` teardown's `[0x15882A] = 1` poke and
  the abort-skip split are modeled only through `ref_restart_stage`; the
  `[0x158828]` machine-kind / `[0x158808]` step-pointer stores stay
  unmodeled.

**Keep/reroll set sweep (T4).** The round-2 text's `0/0x62/0x65` is corrected
in place to `0/0x62/0x63/0x64/0x65` (all five sites: this doc x2, FU-150,
`docs/ENGINE.md`, the `fifa96_match_run.c` comment and the frame test). Fresh
`decompile_function 0x6E724` this task confirms the keep set
(`{'\0','b','c','d','e'}` = `{0,0x62,0x63,0x64,0x65}`) and the reroll switch
(`& 3`: 0 -> the param, 1 -> 99 = 0x63, 2 -> 100 = 0x64, 3 -> 0x65), so the
full outcome set is `{0,0x62,0x63,0x64,0x65}`; all five rows carry flags
`0x07`, so the derived `[0x158882] = row_flags & 1 = 1` is unchanged.

**Quote-rewording marker (T4).** The round-2 erratum's blockquote above
condenses its source without marking the rewording: the verbatim round-1
sentences are "Because the engine's session gate is binary, the `!= 0` term
always holds once `0x8947A` passes, so the effective native condition is
**abort iff the session gate is 0**" and "The engine now models the byte-0
abort for all kinds"; the quoted "the engine now aborts iff the gate is 0" is
a rewording of those two (the blockquote's ellipsis marks the omission, not
the rewording). The conclusion stands — only the quote form is marked here.
