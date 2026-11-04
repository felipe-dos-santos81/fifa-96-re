# FU-239 — the extra-time re-arm and the live ET-half phase

Provenance: M2 phase-22 **Stage-2 unblock recon**, 2026-10-13, **read-only**, dispatched
after FU-238 §2.3's assumption failed the spec §5 T1 feasibility probe (the ET half did
not play). Ghidra project `fifa96`, program **`/FIFA96.EXE`** (LE image; globals at
`+0x100000` from file offset). All claims cite fresh disassembly/raw bytes. Numbered legs
at the end; nothing unprovable is asserted. Corrects FU-213 §11, FU-233 §3, and FU-238 §2.3.

## 0. The problem this slice closes

The M2 phase-22 probe (the T1 fact sheet) measured: the engine enters extra time at phase
`0x13` and **stalls to the cap** (`exit=-1`) — the ET clock never ticks, because `0x13`/`0x14`
are **class 0** (`fifa96_match_state.c:4-8`; native table `0x1106AD`). FU-213-L2 cited the
class-0→class-1 re-arm as `FUN_00088860`/`FUN_0004B02C`. This slice identifies the actual
re-arm chain and the phase the live ET half runs in.

## 1. The period-over re-arm chain (identified)

```
FUN_0004B02C(mode 0)            ; the period-over / match-init re-arm machine
  0x4B08E  CALL 0x88860         ; -> the situation dispatcher
FUN_00088860                    ; 0x88860..0x888FA
  0x8886B  CMP EAX,0xC          ; GATE: current phase [0x157A4D] == 0x0C
  0x888B0  CMP [0x157AC2],4     ; DISCRIMINATOR: the period index
  0x888B3  JNZ -> 0x888F2       ; != 4  -> sit-1 (half-time)
  0x888C8  -> sit-8             ; == 4  -> sit-8 (extra time)
  (sit-1) 0x8ABCF CALL 0x888FC(1,0,1)   -> act 1
  (sit-8) 0x8AEBA CALL 0x888FC(7,0,1)   -> act 7
```

- `FUN_000888FC` (`0x888FC`) installs an **act**: row-`0x17` slot `0x1107F0 = 0x88DC8`
  (act 1) and row-`0x1D` slot `0x110808 = 0x89868` (act 7).
- The act bodies write the **kickoff** phase: act-1 → `0x88E82 CALL 0x740A0` writes phase
  `1`; act-7 → `0x898DD` writes phase `0x10`. Both are **class 0** (a kickoff staging frame).
- The raw kickoff restart then reaches **phase `2`** (class 1) — see §3.

**FU-213 §11 erratum.** `FUN_00088860` is **not** the re-arm itself: it writes only
`[0x157AAF] := [0x157AAE]` (`0x8887E`, bytes `a2 af 7a 15 00`) and **no** phase/period/clock
cell. It is the phase-`0xC`-gated **situation dispatcher**. Fresh `get_xrefs_to 0x88860` = **2**:
`0x4B08E` (`FUN_0004B02C`, the mode-0 arm) and `0x74244` (the unnamed frame dispatch
`0x741C4`, only when `[0x14C32A]==0`).

## 2. `FUN_0004B02C` — the re-arm machine (mode 0 / mode 1)

Body `0x4B02C..0x4B0FE` (61 insns), parameter-switched (`0` = period-over, `1` = match-init).
Writes (fresh disasm): `byte[0x157AB3]:=0` (`0x4B032`); `byte[0x157A4F]:=0` (`0x4B038`);
`dword[0x157A5C]:=0` (`0x4B045`); `word[0x157A64]:=0` (`0x4B052`); `byte[0x157AC2]:=4`
(`0x4B0DE`, **bit-8 gated** — `FUN_0008A938(8)`; FU-216-L2); **`word[0x158822]:=0`**
(`0x4B0E7`) — the end word is **cleared at period-over**; `word[0x157A68]:=0` (`0x4B0EE`).
No phase write. Fresh `get_xrefs_to 0x4B02C` = **2**, both in `FUN_000493A0`: `0x49415`
(match-init) and `0x4950A` (the do-while loop head `0x494A6`). It is **not** a per-frame
consumer inside the frame loop `FUN_0004B100`.

## 3. The live ET half runs in phase `2` (derived)

- The phase cell is `byte[0x157A4D]` (= `[0x157A4A]>>24`). Fresh `get_xrefs_to 0x157A4D` = **6**;
  three writers: `FUN_000740A0` (`0x740AC`, the only general writer, **27** call sites),
  `FUN_00073E28` (`0x73E4E` → 0), `FUN_00073D10` (`0x73D2C`, memset → 0).
- Decoding all **27** `FUN_000740A0` sites: values written = `{0,1,2,3,4,5,8,9,A,B,C,D,F,10,
  11,13,14,15}` (plus 2 register-derived). **No site writes `0x17/0x1A/0x1C/0x1D/0x1E`.**
- The class table `0x1106AD`: class-1 = `{0x02,0x17,0x1A,0x1C,0x1D,0x1E}`; `[0x13]=[0x14]=0`.
  **The only class-1 phase with any static writer is `2`** — so the clock can tick only in
  phase `2`. The phase-`2` writer is `FUN_0008A938` **situation `0xB`** (jump-table idx 11
  `0x8A930` → `0x8AEF6` → `0x8AEFA MOV EAX,2`; `0x8AF02 CALL 0x740A0`); its callers are the raw
  restart handlers `0x7546E`/`0x75B58`/`0x76072`.

**Derivation (not a guess):** a class-0 phase cannot advance the clock, so the live ET half
**must** be a class-1 phase; the only class-1 phase with any static writer is `2`. Therefore
the ET half plays (and clocks) in **phase `2`**. Consequently `0x13`/`0x14` are **not** the
playing half — see §4.

## 4. `0x13`/`0x14` are the ET-over screen (class-0 dwell), not the ET half

The ET chooser `FUN_0008B9CC` (`0x8B574 CALL 0x8B9CC`) writes phase `0x13`/`0x14`
(`0x8BAC1 CALL 0x740A0`) then installs act `0xC` (`0x8BAE7 CALL 0x888FC(0xC)`, handler
`0x8B900`), at the period-`1`→`2` roll `0x8B58A`. `0x13`/`0x14` are class-0; the end-machine
arm on them is a **dwell**: `word[0x158818] > 0x4B0` (≈ 20 s) → `word[0x158822] = 2`.

**FU-238 §2.3 erratum.** "The ET halves then each complete like the regulation halves" is
**false of the native**: the ET half completes by the §1 re-arm into class-1 phase `2`; the
`0x13`/`0x14` phases are the ET-**over** dwell. The engine's current model (ET half parked at
`0x13`/`0x14`, FU-238-L2) is a screen/running-phase mistranslation — and is exactly the T1
stall the probe measured.

## 5. The ET-over terminal

`word[0x158822] := 2` at **`0x8BD8A`** (phase-`0x14` arm; bytes `66 c7 05 22 88 15 00 02 00`)
and **`0x8BDD4`** (phase-`0x13`, score-equal arm); latch `MOV dword[0x10F360],1` at `0x8BDDD`
(bytes `c7 05 60 f3 10 00 01 00 00 00`). Fresh `get_xrefs_to 0x10F360` = **5** (cleared by
init `0x7D989` / reset `0x887F2`; read `0x8BD81` / `0x8BDCB`; set `0x8BDDD`). Enclosing gates:
phase `[0x157A4D] ∈ {0x13,0x14}`; timer `word[0x158818] > 0x4B0` (or P-set `>0x258`/`>0x12C`);
latch `[0x10F360] == 0`; phase-`0x13` additionally needs `[0x157AC5] == [0x157AC7]`. So the
ET-over fires from the `0x13`/`0x14` dwell once the timer passes its gate.

## 6. Open legs (do not fake)

- **FU239-L1 (UB6-L4, the route leg):** no direct xref ties the ET chooser's `0x13`/`0x14`
  write / the period-roll `0x8B58A` to the `0x0C → sit-8 → phase 2` re-write — the situation
  dispatch is **indirect** (jump-table, in the function-undef'd `0x74xxx–0x76xxx` region). The
  *target* (phase `2`) is derived (§3); the exact route is not statically pinned. Alternative
  not excluded: the ET half in `0x13`/`0x14` with the end-machine dwell as its timer (ruled out
  by the class-0 physics, but not by a direct xref). Owner: the period/lifecycle slice.
- **FU239-L2:** the `sit-0xB → phase 2` jump-table entry lives in the function-undef'd region;
  the three caller addresses (`0x7546E`/`0x75B58`/`0x76072`) are unresolved bodies.
- **FU239-L3:** the `[0x157AC2]=4` ET discriminator is written under the bit-8 gate (`0x4B0DE`,
  FU-216-L2); whether the M2 tape's forced-ET path sets bit-8 (vs the phase-`0x13` chooser) is
  not reconciled here.
- **FU239-L4:** the `0x13`-vs-`0x14` ET-half order is score/team-driven (FU-237-L1); the timer
  units for the `>0x4B0` gate are derived by role, not decomposed.

## 7. Evidence commands (reproducible)

```
# the re-arm dispatch (fresh, same session)
#   FUN_0004B02C   -> CALL 0x88860 at 0x4B08E
#   FUN_00088860   -> 0x8886B CMP EAX,0xC ; 0x888B0 CMP [0x157AC2],4 ; 0x888B3 JNZ
#   FUN_0008A938   -> sit-1 0x8ABCF CALL 0x888FC(1,..) ; sit-8 0x8AEBA CALL 0x888FC(7,..)
#                   -> sit-0xB 0x8AEFA MOV EAX,2 ; 0x8AF02 CALL 0x740A0
# the terminal
#   0x8BD8A  66 c7 05 22 88 15 00 02 00    word[0x158822]:=2
#   0x8BDDD  c7 05 60 f3 10 00 01 00 00 00 dword[0x10F360]:=1
# the phase cell
#   get_xrefs_to 0x157A4D == 6 ; the general writer FUN_000740A0 (0x740AC), 27 sites
```
