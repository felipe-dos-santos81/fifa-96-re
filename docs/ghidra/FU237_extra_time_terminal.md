# FU-237 — the extra-time terminal + the shootout entry (phase-22 wave W22-7)

Provenance: M2 phase-22 Stage-1 recon, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, executable
`/home/felipe/code/fifa96-reversed/game/hdd/FIFA96/FIFA96.EXE`,
`watcom:LE:32:default`, image base `0`, min `0x10000`, max `.image::00174a2a`,
**2706** functions, **15239** symbols, memory_size `2757479`, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`disassemble_bytes` / `read_memory` / `get_xrefs_to` / `search_strings` of
**this** session unless marked *cross-cited* (then the frozen FU doc is named).
The alias traps are applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the
byte at `x+3`); LE globals sit at `+0x100000` from the file offset; addresses
are `/FIFA96.EXE` runtime addresses. Engine/loader source read first-hand. **No
writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (W22-7 brief; spec §4 slice 7 / §10): derive the **extra-time terminal's
winner/loser decision** and pin the **(project-)phase-23 penalty-shootout entry
point**. Steps: (1) the phase-`0x13`/`0x14` equal-score test
(`0x8BD95 MOV AX,[0x157AC5]; CMP AX,[0x157AC7]; JNZ hold`) → `[0x158822]=2`
(`0x8BD8A`/`0x8BDD4`) and the `[0x10F360]` latch; (2) the tie→shootout boundary —
what the native does on a **tied ET end**, a definitive in/out verdict + entry
point; (3) verdict + legs. Extends FU-228 (§2.2), FU-231 (the end machine),
FU-232 (the frame-loop consumer), FU-233 (the ET-half rows), FU-236 (the
regulation-word route), FU-64/FU-213/FU-207.

---

## Answer in one line

**The ET terminal is a word-writer, not a shootout gate.** `FUN_0008BAF0`'s
phase-`0x13`/`0x14` block sets the end word `[0x158822]=2` on a **score-equal**
phase-`0x13` or on **any** phase-`0x14` (after a per-phase timeline gate) and
latches `[0x10F360]=1`; it makes **no winner/loser decision** and stores **no
winner**. The word `2`'s **sole** consumer, the frame loop `FUN_0004B100`'s
`AX==2` arm (`0x4B257..0x4B2A8`), re-selects the camera target and **clears the
word** — it does **not** enter the match-over screen (`[0x5FFC]=2` is the
`AX==1`/past-half-time path at `0x4B244`) and calls **no** shootout. There is
**no penalty-shootout entry point anywhere on the end spine** (no `penalty`/
`shootout` string, no phase/act row, no decider call in the end machine or its
consumers): the spec §2/§7.4 premise *"a tied ET end enters the phase-23
shootout"* is **not supported** by the frozen end machine. **In/out verdict:
OUT — the phase-23 shootout leg inherits NO entry point from the ET terminal.**
The only native "penalty" artifacts are the **in-match penalty kick** (action
row `0x13` `0x84B00..0x84EEB`) and a **HUD "extra-time/penalties display"** mode
(`[0x14C32A] != 0`; FU-148 §5) — neither is a shootout and neither is reachable
from this terminal.

---

## 1. The ET terminal — `FUN_0008BAF0`, the phase-`0x13`/`0x14` block (fresh)

`FUN_0008BAF0` (`0x8BAF0..0x8C065`) is the end machine; its **sole** caller is
`0x8B664`, a single `UNCONDITIONAL_CALL` inside the per-frame clock
`FUN_0008AF38` (fresh `get_xrefs_to 0x8BAF0` = **1**; agrees FU-228 §2.2 /
FU-231). It dispatches on the phase byte `[0x157A4A]>>24` (read at `0x8BAFB`,
re-read at `0x8BD45`) and handles phases `{6, 2, 0x0C, 0x13, 0x14}`; anything
else is a no-op (fresh `0x8BCEC CMP EAX,0x14 / 0x8BCEF JNZ 0x8BDE7`).

Fresh `disassemble_bytes 0x8BCE7` (280 B) — the phase-`0x13`/`0x14` block, byte
exact:

```
0x8BCE7 CMP EAX,0x13            ; 83f813
0x8BCEA JZ  0x0008bcf5          ; 7409
0x8BCEC CMP EAX,0x14            ; 83f814
0x8BCEF JNZ 0x0008bde7          ; 0f85f2000000      ; not 0x13/0x14 -> common tail

0x8BCF5 CMP dword[0x158808],0   ; 833d0888150000    ; act installed?
0x8BCFC JNZ 0x0008bd0b          ; 750d               ; yes -> skip the timer
0x8BCFE MOV AX,[0x157A64]       ; 66a1647a1500       ; frame delta
0x8BD04 ADD word[0x158818],AX   ; 66010518881500     ; END/ACT TIMER += delta
0x8BD0B MOV ESI,0x780           ; be80070000         ; the 0x780 camera hold
0x8BD10 MOV EDX,[0x158814]      ; 8b1514881500
0x8BD16 XOR EDI,EDI             ; 31ff
0x8BD18 MOV [0x158830],ESI      ; 893530881500       ; = 0x780
0x8BD1E MOV [0x158834],EDI      ; 893d34881500       ; = 0
0x8BD24 MOV [0x158838],EDI      ; 893d38881500       ; = 0
0x8BD2A TEST EDX,EDX            ; 85d2
0x8BD2C JL  0x0008bd32          ; 7c04
0x8BD2E MOV EAX,EDX             ; 89d0
0x8BD30 JMP 0x0008bd36          ; eb04
0x8BD32 MOV EAX,EDX             ; 89d0
0x8BD34 NEG EAX                 ; f7d8
0x8BD36 CMP EAX,0x60            ; 83f860
0x8BD39 JGE 0x0008bd45          ; 7d0a
0x8BD3B MOV dword[0x158830],0x61a8 ; c70530881500a8610000  ; |[0x158814]| < 0x60 -> 0x61A8

0x8BD45 MOV EAX,[0x157A4A]      ; a14a7a1500         ; reload phase
0x8BD4A SAR EAX,0x18            ; c1f818
0x8BD4D CMP EAX,0x14            ; 83f814
0x8BD50 JNZ 0x0008bd95          ; 7543               ; phase != 0x14 -> the 0x13 path

; --- phase == 0x14 path ---
0x8BD52 MOV EAX,[0x158816]      ; a116881500         ; dword -> word[0x158818] (alias)
0x8BD57 SAR EAX,0x10            ; c1f810
0x8BD5A CMP EAX,0x4b0           ; 3db0040000
0x8BD5F JG  0x0008bd81          ; 7f20
0x8BD61 CALL 0x00045001         ; e89b92fbff
0x8BD66 TEST AL,AL              ; 84c0
0x8BD68 JZ  0x0008bde7          ; 0f8479000000       ; probe false -> tail (hold)
0x8BD6E MOV EAX,[0x158816]      ; a116881500
0x8BD73 SAR EAX,0x10            ; c1f810
0x8BD76 CMP EAX,0x258           ; 3d58020000
0x8BD7B JLE 0x0008bde7          ; 0f8e66000000       ; <= 0x258 -> tail (hold)
0x8BD81 CMP dword[0x10F360],0   ; 833d60f3100000     ; THE LATCH
0x8BD88 JNZ 0x0008bddd          ; 7553               ; already latched -> tail
0x8BD8A MOV word[0x158822],2    ; 66c705228815000200 ; phase 0x14 -> ET OVER (2)
0x8BD93 JMP 0x0008bddd          ; eb48

; --- phase == 0x13 path ---
0x8BD95 MOV AX,[0x157AC5]       ; 66a1c57a1500       ; score A
0x8BD9B CMP AX,[0x157AC7]       ; 663b05c77a1500     ; vs score B
0x8BDA2 JNZ 0x0008bde7          ; 7543               ; DIFFER -> tail (hold, no write)
0x8BDA4 MOV EAX,[0x158816]      ; a116881500         ; equal: same timeline gate
0x8BDA9 SAR EAX,0x10            ; c1f810
0x8BDAC CMP EAX,0x4b0           ; 3db0040000
0x8BDB1 JG  0x0008bdcb          ; 7f18
0x8BDB3 CALL 0x00045001         ; e84992fbff
0x8BDB8 TEST AL,AL              ; 84c0
0x8BDBA JZ  0x0008bde7          ; 742b
0x8BDBC MOV EAX,[0x158816]      ; a116881500
0x8BDC1 SAR EAX,0x10            ; c1f810
0x8BDC4 CMP EAX,0x12c           ; 3d2c010000
0x8BDC9 JLE 0x0008bde7          ; 7e1c
0x8BDCB CMP dword[0x10F360],0   ; 833d60f3100000
0x8BDD2 JNZ 0x0008bddd          ; 7509
0x8BDD4 MOV word[0x158822],2    ; 66c705228815000200 ; phase 0x13 + EQUAL -> ET OVER (2)

0x8BDDD MOV dword[0x10F360],1   ; c70560f3100001000000  ; THE LATCH SET
0x8BDE7 ...                     ; the common tail (camera sinks only, §3)
```

**Decision, precisely:**

* **phase `0x14`** (`0x8BD52`): after the timeline gate (`word[0x158818] > 0x4B0`
  **OR** (`0x45001()` true **AND** `word[0x158818] > 0x258`)) → iff the latch
  `[0x10F360]==0`, write `[0x158822]=2`. **No score test.**
* **phase `0x13`** (`0x8BD95`): **`[0x157AC5] == [0x157AC7]`** (scores equal) —
  after the analogous gate (`… > 0x4B0` **OR** (`0x45001()` **AND** `… > 0x12C`))
  → iff `[0x10F360]==0`, write `[0x158822]=2`; **scores differ → the `JNZ 0x8BDE7`
  tail, no write (hold).**
* **latch** (`0x8BDDD`): both write paths — and only they — set
  `[0x10F360]=1`, so at most one ET-over event per match.

The alias trap is live: the gate loads the **dword** `[0x158816]` and `SAR
EAX,0x10`, i.e. the high word = `word[0x158818]` (the same end/act timer the
block accumulates at `0x8BCFE`), **not** `word[0x15881A]` (the config period
limit). This is the FU-170 §4 / FU-221 §3 idiom, re-confirmed here.

**The terminal makes no winner/loser decision.** `[0x157AC5]`/`[0x157AC7]` are
only tested for **equality** (it decides *whether to write the end word*, not
*who won*); there is no winner cell, no score store, no team index, and no
ball-record touch in the block. The only stores in the whole block are
`word[0x158818]` (timer), `[0x158830/0x158834/0x158838]` (the 0x780 camera
hold), `word[0x158822]` (`=2`), and `[0x10F360]` (`=1`).

---

## 2. The end word `[0x158822]` and its **sole** consumer (fresh)

### 2.1 The cell census (fresh)

Fresh `get_xrefs_to 0x158822` = **10** — **8 writes / 2 reads**, identical site
set to FU-228 §2.1 / FU-231:

| kind | site | function | instruction |
|---|---|---|---|
| READ (guard) | `0x4B106` | `FUN_0004B100` | `CMP word[0x158822],0` |
| READ (return) | `0x8B678` | `FUN_0008AF38` | `MOV AX,word[0x158822]` |
| WRITE (=1) | `0x8BCD9` | `FUN_0008BAF0` | phase-`0xC` flag-clear → regulation over |
| WRITE (=2) | `0x8BD8A` | `FUN_0008BAF0` | **phase `0x14` → ET over** |
| WRITE (=2) | `0x8BDD4` | `FUN_0008BAF0` | **phase `0x13` + equal → ET over** |
| WRITE (=0) | `0x4B0E7` | `FUN_0004B02C` | reset |
| WRITE (=0) | `0x4B29B` | `FUN_0004B100` | the `AX==2` arm's clear (§2.2) |
| WRITE (=0) | `0x8871B` | `FUN_000886D4` | match reset |
| WRITE | `0x74342` | *(no function)* | reset block |
| WRITE | `0x88D42` | *(no function)* | reset block |

So the word has exactly **two nonzero producers**, both the ET terminal's `=2`
sites; `=1` is the FU-236 regulation route. **The only `==2` consumer is the
frame loop.**

### 2.2 The consumer `FUN_0004B100` — the whole branch (fresh)

Fresh `disassemble_bytes 0x4B100`/`0x4B180`/`0x4B2BF` (`0x4B100..0x4B2E7`):

```
0x4B106 CMP word[0x158822],0 ; JZ 0x4B11A ; MOV EAX,1 ; JMP 0x4B2E1
        ; head guard: word != 0 -> the WHOLE frame body is skipped (return 1)
0x4B1A6 CALL 0x8AF38                 ; AX = the harness end word
0x4B1AB CMP AX,1 ; JC  0x4B2A9       ; AX == 0  -> ongoing
0x4B1B5 JBE 0x4B1C6                  ; AX == 1  -> regulation branch
0x4B1B7 CMP AX,2 ; JZ  0x4B257       ; AX == 2  -> THE ET-OVER ARM
0x4B1C1 JMP 0x4B2A9
...
0x4B244 MOV EAX,2 ; CALL 0x36BC0     ; [0x5FFC]=2  (mark_over)  <-- AX==1 + index>=2 ONLY
...
0x4B257 CALL 0x53DC4                 ; AX==2 arm: a generic period-end screen-enable
0x4B25C CALL 0x4B678                 ;          returns (period >= 4)
0x4B261 TEST EAX,EAX ; JZ 0x4B28A
0x4B265 MOV EDI,[0x1587D4] ; … MOV [0x1074A4],EAX   ; camera target select
0x4B283 CALL 0x36200                 ;          camera sink(0)
0x4B28A … MOV [0x1074A4],[0x1587E3]>>24
0x4B297 XOR ESI,ESI
0x4B299 XOR EAX,EAX
0x4B29B MOV word[0x158822],SI        ; <<< CLEARS the word
0x4B2A2 … RET
```

**Structure:** the `AX==2` arm is the **same shape** as the `AX==1` half-time
block (`0x4B1D2`: `CALL 0x513EC` / `CALL 0x53DC4` / `CALL 0x4B678`) minus the
`0x513EC` call — i.e. a **period-transition / camera re-select**, **not** a
match-over. It writes **no `[0x5FFC]`** and calls **nothing
shootout-shaped**. The match-over screen (`[0x5FFC]=2`) is entered **only** on
the `AX==1` **and** `[0x157AC2] >= 2` path (`0x4B1C6 CMP EAX,2 ; JGE 0x4B220` …
`0x4B244`), i.e. the **regulation** over.

Consequence: on a tied ET end the terminal writes `word=2`, the frame loop's
`AX==2` arm consumes it and **clears it** — the end word returns to `0` and the
run continues. **The ET terminal does not by itself enter the match-over screen,
and it enters no shootout.**

### 2.3 The latch `[0x10F360]` (fresh)

Fresh `get_xrefs_to 0x10F360` = **5**:

| kind | site | function |
|---|---|---|
| WRITE | `0x7D989` | `FUN_0007D8D0` (session/team init; also clears `0x10F368`/`0x10F364`/`0x10F36C`) |
| WRITE | `0x887F2` | `FUN_000886D4` (the match reset) |
| READ | `0x8BD81` | `FUN_0008BAF0` (phase-`0x14` latch test) |
| READ | `0x8BDCB` | `FUN_0008BAF0` (phase-`0x13` latch test) |
| WRITE | `0x8BDDD` | `FUN_0008BAF0` (**the** set) |

So `[0x10F360]` is a per-match **one-shot "ET-over already fired"** latch,
cleared only by match/session init (`FUN_0007D8D0`, `FUN_000886D4`) and set only
by the ET terminal. Fresh `disassemble_bytes 0x7D989` confirms the sibling
clears (`MOV dword[0x10F360],EBX` / `[0x10F368]` / `[0x10F364]` / `[0x10F36C]`,
EBX=0). The neighbours `[0x10F364]`/`[0x10F368]` are read by the phase-`0x13`/
`0x14` body (§4) and written by `FUN_0008D098`; their semantics are not derived
here (**FU237-L6**).

---

## 3. The end machine's tail and the shootout hunt (fresh)

The ET block's non-terminal exits and the terminal both land at the **common
tail** `0x8BDE7`; fresh `disassemble_bytes 0x8BDE7` (320 B) shows the tail is
**presentation only** — the camera sinks `0x8DCD4(0x15880C,0x158830,0x15883C)`
(`0x8BDF6`), `0x8DD70` (`0x8BE18`/`0x8BE7B`), the `[0x15885A]`/`[0x158852]`
camera-angle integrator and the `[0x1104CF]` table read (`0x8BF01`). There is
**no CALL to a phase write (`0x740A0`), no act dispatch (`0x888FC`), and no
shootout-shaped call.**

Independent negative checks (fresh this session):

* `search_strings "hoot"` = **0**; `search_strings "pen"` returns only
  `openhandle`/`openblockhandle`/`Suspenders`/`xPenny Lee`/etc. — **no
  `penalty`/`shootout` string.**
* The **phase table** `0x110794` (fresh `read_memory` 148 B) maps phases
  `0x00..0x22` and has **no** decider row; rows `0x13`/`0x14` both resolve to
  `0x0006E244` (a per-record update; §4). The **act base** is `0x1107EC`
  (= `0x110794 + 0x16*4`, FU-221 §1) — likewise no shootout act.
* `get_xrefs_to 0x158822` = 10 (§2.1); the only `==2`-specific handling is
  `FUN_0004B100`'s `0x4B1B7 JZ 0x4B257`, which does not shoot.

**The only native "penalty" artifacts** (fresh, repo-wide): the **in-match
penalty kick** — action row `0x13` `0x84B00..0x84EEB`, the FK/penalty hand-off,
*port* `fifa96_match_action_13` (`src/fifa96_engine/fifa96_match_handlers.c:5135`,
`:2025`) — and a **HUD mode** keyed on `[0x14C32A] != 0` that FU-148 §5 calls
"extra-time / **penalties display**" (string-table ids `0x1EA`/`0x1E9` as
team-name substitutes, native `0x55CCD..0x55D25`). Neither is a shootout; the
former is a single penalty awarded in play, the latter is a HUD label mode whose
own entry is unproven (FU148-L5).

---

## 4. The phase-`0x13`/`0x14` bodies and the engine posture (fresh)

The native phase table row `0x13` and row `0x14` **share** the body
`0x0006E244` (fresh `read_memory 0x110794`: row `0x13` at `0x110794+0x4C =
0x1107E0` = `44 e2 06 00` = `0x0006E244`; row `0x14` at `0x1107E4` = the same).
Fresh `disassemble_bytes 0x6E244` shows a per-record update: it compares a
record side byte `[EAX+0x826]` against `[0x157AAC]>>24` (`0x6E25F`), reads the
period `[0x157AC2]` (`0x6E281`) and the sibling cells `[0x10F368]` (`0x6E267`)
and `[0x10F364]` (`0x6E274`), draws the RNG (`0x92AC8`, `0x6E2B3`/`0x6E302`)
and writes `[ECX+8]`/`[ECX]` (a heading/timer). It is **not** a terminal and
touches **no** end word, phase, score or shootout cell.

Engine posture (fresh source): the engine's **phase table rows `0x13` and `0x14`
are `NULL`** (`src/fifa96_engine/fifa96_match_handlers.c:5227`/`:5229`) — the
native bodies are **unported**. The engine only special-cases the phase in the
FU-142a arm (`fifa96_match_run.c:4124` → `fifa96_match_phase_machine_step`, the
`FUN_0008D098` state-`0x13`/`0x14` block) and the action row-03 phase-`0x13`/
`0x14` target arm (`fifa96_match_handlers.c:4878`). Crucially, the engine's
match-over proxy **intentionally excludes** the ET-over event: fresh
`fifa96_match_run.c:4185`
`int end_word_decided = (mr->state.phase == 0x0Cu && mr->global_157ac0 == 0u);`
— with the in-source note "`end_word_decided` … intentionally EXCLUDES the `==2`
extra-time-over event (`0x8BD8A`/`0x8BDD4`)" (`:4177..4180`). So the **engine
already agrees** that the ET terminal's `word=2` is **not** the match end.

---

## 5. Verdict (steps 2–3)

### 5.1 The ET-end decision (step 1/3)

**What the native does on a tied ET end:** at phase `0x13` with
`[0x157AC5] == [0x157AC7]` (and after the `word[0x158818]` timeline gate), the
end machine **writes `[0x158822]=2`** and **latches `[0x10F360]=1`**. That is the
*entire* tie-specific consequence. The winner/loser is **not** decided by the
terminal (equality is only a write-gate), and the word `2` is then consumed by
the frame loop's `AX==2` arm as a **period-transition/camera** event that
**clears** the word. The ET terminal therefore does **not** end the match by
itself, and for the tape it is at most an "ET-half over" transition (the exact
interpretation of the `AX==2` arm is FU-232's; **FU237-L3**).

### 5.2 The tie→shootout boundary — **in/out = OUT** (step 2)

**There is no penalty-shootout entry point reachable from the ET terminal, nor
anywhere on the end spine.** Evidence: (a) the terminal's tie branch only gates
the end-word write (§1); (b) the word's **sole** consumer is the frame loop's
`AX==2` camera/period arm, which clears the word and calls no shootout (§2.2);
(c) the end machine's tail is camera sinks (§3); (d) no `penalty`/`shootout`
string, no phase/act row, no decider cell; (e) the only "penalties" artifacts
are the in-match penalty kick and a HUD display mode (§3).

Therefore spec §2 / §7.4's premise *"a tied ET end enters the phase-23
shootout"* is **not supported by the frozen end machine** — there is **no entry
point to pin**. The shootout is **out** of this spine:
it is, if it exists at all, in the **front-end/competition layer** (the binary
carries `WORLD_CUP teams` / `WORLD_CUP matches` / `cup1.inv` / `cup2.inv` data
strings — a cup path outside this spine), or it is a HUD-only label. **The
phase-23 shootout subsystem inherits no entry from FU237** and must be located
from the competition/front-end side (FU-65) or declared absent.

### 5.3 Hand-off

* **FU-232 (Task 2)** owns the `AX==2` arm's exact semantics (period transition
  vs ET-over screen) — the fork that decides whether `word=2` is "an ET half
  over" or "the ET over" (**FU237-L3**).
* **FU-233 (Task 3)** owns the phase-`0x13`/`0x14` time-order/semantics (which
  ET half each names; why `0x13` is equality-gated and `0x14` unconditional)
  (**FU237-L1**, **FU237-L5**).
* **phase-23** takes the shootout as a **separate subsystem with no entry pin**
  (**FU237-L4**).

---

## 6. Census impact

**None.** No action/phase row is wired or unwired by this read-only slice; the
phase-`0x13`/`0x14` rows stay `NULL`. `floor(100 × 55/80) = 68` stands; no
golden re-pin. M1 `09b726b7…` and M2 `0b87fd18…` (v21) untouched.

---

## 7. Residual legs

1. **FU237-L1 — the phase-`0x13`-vs-`0x14` time-order/semantics.** Phase `0x14`
   writes `word=2` unconditionally; phase `0x13` writes it only on a **score
   equality**. Which ET half each names (and why the tie is gated on `0x13` but
   not `0x14`) is **not** statically proven here. Owner: **FU-233** (Task 3, the
   ET-half rows) / FU-213.
2. **FU237-L2 — the `[0x157AC5]`/`[0x157AC7]` score-cell identity/polarity is
   inherited.** The equal test's *meaning* depends on these being the two teams'
   scores; that identity is FU-207's/FU-224's (recorded by FU-230-L2 as
   inherited, not independently corroborated). Re-confirm before porting the
   terminal. Owner: **FU-224/FU-207/FU-236**.
3. **FU237-L3 — the `AX==2` arm's semantics (the fork).** `0x4B257..0x4B2A8`
   re-selects the camera and clears `word=2`; whether this is an "ET-half
   transition → next period" or the "ET-over screen" (spec §3 diagram) governs
   whether the ET terminal is a *match* terminal at all. Owner: **FU-232**
   (Task 2).
4. **FU237-L4 — no shootout entry exists on the end spine (this verdict).** The
   phase-23 shootout subsystem inherits **no** entry point from FU237; if a
   shootout exists it lies in the front-end/competition layer (the
   `WORLD_CUP`/`cup*.inv` data path) or is a HUD-only label (the `[0x14C32A]`
   "penalties display", FU148-L5). Owner: **phase-23** (+ the front-end slice,
   FU-65).
5. **FU237-L5 — the phase-`0x13`/`0x14` bodies are unported.** Native rows
   `0x13`/`0x14` share the per-record body `0x6E244`; the engine's rows are
   `NULL` (`fifa96_match_handlers.c:5227`/`:5229`). Only the FU-142a
   `FUN_0008D098` state-`0x13`/`0x14` arm and the action row-03 target arm are
   partial ports. Whether the terminal path needs any of this body is a
   Stage-2 decision. Owner: **FU-233** / FU-142a.
6. **FU237-L6 — the `0x10F3xx` cluster.** The latch `[0x10F360]` co-resets with
   `[0x10F364]`/`[0x10F368]`/`[0x10F36C]` at session init (`FUN_0007D8D0`) and
   match reset (`FUN_000886D4`); the semantics of `0x10F364`/`0x10F368` (read by
   the phase-`0x13`/`0x14` body and written by `FUN_0008D098`) are not derived
   here. Owner: **FU-233** / the match-reset slice.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, 15239 symbols, image base 0, min
  `0x10000`, max `.image::00174a2a`, `.image` overlay.
* `disassemble_bytes` (start, length) — `0x8BCE7` (280 B, the ET block),
  `0x8BAF0` (280 B, the dispatch head), `0x8BDE7` (320 B, the tail),
  `0x4B100` (140 B), `0x4B180` (320 B, the AX branch), `0x4B2BF` (60 B),
  `0x8B3A0` (560 B, the period-completion region), `0x6E244` (120 B),
  `0x6E2BB` (120 B), `0x6DF4C` (120 B), `0x7D8D0` (150 B), `0x7D989` (60 B),
  `0x886D4` (80 B), `0x6E326` (180 B), `0x8D098` (140 B), `0x4B678` (120 B),
  `0x53DC4` (90 B).
* `read_memory` — `0x110794` (148 B, the phase table), `0x157A40` (112 B),
  `0x157AB0` (48 B).
* `get_xrefs_to` — `0x8BAF0` (**1**), `0x158822` (**10**), `0x10F360` (**5**),
  `0x10F364` (**5**), `0x10F368` (**6**), `0x157AC5` (**95**), `0x157AC7`
  (**41**), `0x157A4A` (128 total), `0x1015FC` (**1**), `0x100C28` (**1**).
* `search_strings` — `hoot` (**0**), `pen` (7, none a shootout), `cup` (5,
  incl. `cup1.inv`/`cup2.inv`/`WORLD_CUP teams`/`WORLD_CUP matches`), `final`
  (**0**), `extra` (**0**).
* `get_function_by_address` — `0x8BAF0` = `FUN_0008baf0` (FU-228 cross-cited
  body `0x8BAF0..0x8C065`).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:4124` the FU-142a phase arm, `:4183..4191` the `end_word_decided` proxy),
`src/fifa96_engine/fifa96_match_handlers.c` (`:4878` the action row-03
phase-`0x13`/`0x14` arm, `:5135`/`:2025` the penalty-taker row 0x13,
`:5227`/`:5229` the NULL phase rows 0x13/0x14, `:5235` the phase-0x17 row).
Docs cross-cited: **FU-228** §2.1/§2.2/§2.3, FU-231 (the end machine), FU-232
(the frame loop), FU-233 (the ET-half rows), FU-236 (word-1 route), FU-213,
FU-207, FU-224, FU-230-L2, FU-221 §1/§3, FU-148 §5, FU-170 §4, FU-64. Spec:
`docs/superpowers/specs/2026-10-13-fifa96-m2-phase22-end-spine-design.md` §2/§4
(slice 7)/§7.4/§10. No writes beyond this slice file; no
rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
untouched; no git commands.

---

## Landing note

The ET terminal (`FUN_0008BAF0` phase-`0x13`/`0x14`) is a **gated end-word
writer**: `[0x158822]=2` on a **tied** phase-`0x13` or **any** phase-`0x14`,
latched once by `[0x10F360]`; it decides **no** winner. The word `2`'s only
consumer (`FUN_0004B100` `AX==2`, `0x4B257`) is a camera/period-transition arm
that **clears** the word and **does not** reach the match-over screen or any
shootout. **The shootout has no entry point on the end spine (verdict OUT)**;
the spec's tie→shootout premise is unsupported by the frozen native and is
recorded as FU237-L4 (owner phase-23 / the front-end slice). No engine change is
mandated by this read-only slice; FU237-L1..L6 are the residual legs.
