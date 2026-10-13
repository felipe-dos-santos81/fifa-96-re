# FU-221 — the phase-`0x22` downstream: the `word[0x15881A]` cell (L2) and the `0x974DC(0x1E)` audio ring-push (L3) — the gate-cell alias resolved and the audio leg mapped (R69)

Provenance: R69, 2026-10-13, read-only. Authoritative program **`/FIFA96.EXE`**
(Ghidra MCP; scripts disabled; case-sensitive program argument; confirmed this
session by `get_current_program_info` — name `/FIFA96.EXE`, path
`game/hdd/FIFA96/FIFA96.EXE`, `watcom:LE:32:default`, `image_base 0`,
`min_address 0x10000`, `max_address .image::00174a2a`, 2706 functions, `.image`
overlay present). Every address/byte/xref-count/instruction below is a fresh
`disassemble_bytes` / `read_memory` / `get_xrefs_to` / `get_function_by_address`
/ `decompile_function` of **this session** unless marked *cross-cited* (then the
frozen FU doc is named). Engine/loader source read first-hand
(`src/fifa96_engine/fifa96_match_run.c`, `src/fifa96_engine/fifa96_match_act.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`src/fifa96_engine/fifa96_engine.c`,
`src/fifa96_engine/platform_null.c`, `src/fifa96_engine/platform_sdl3.c`,
`include/fifa96_engine/fifa96_match_run.h`, `include/fifa96_engine/fifa96_platform.h`,
`tests/test_engine_match_act.c`, `tests/test_engine_m2.c`, `docs/ENGINE.md`,
`README.md`). **No writes**: no rename, comment, label, function, script or
project save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git
commands. The alias traps are applied (`dword[x]>>16` = the word at `x+2`,
`>>24` = the byte at `x+3`); LE globals sit at `+0x100000` from the file offset;
addresses are `/FIFA96.EXE` runtime addresses.

Scope (R69 brief): close **FU211-L2** (the `word[0x15881A]` cell behind the
phase-`0x22` handler's stage gate) and **FU211-L3 / FU195-L3** (the
`0x974DC(0x1E)` audio ring-push). Derive first-hand: (1) the phase-`0x22` handler
entry, body, arms and every store; (2) the fresh `get_xrefs_to 0x15881A` census
(all writers + readers) and what the handler actually reads it for; (3) the exact
ring buffer + push function and the native call chain the phase-`0x22` path
reaches, and what the port lacks; (4) the other phase-`0x22` stores; (5) the
engine extension point and whether M2 reaches the row. Model: FU-211 (the phase-22
body doc). This slice also lands one quoted errata against FU-195/FU-211 (§3, the
alias trap) and one against FU-195 §2 (the `FUN_00065CC0` argument), §9.

Cross-reference at a glance: phase `0x22`'s body is the **same dword** as act
`0xC`'s slot — `0x110794[0x22] = 0x1107EC[0xC] = 0x8B900` (the FU165-L9 dual
identity; fresh §1).

---

## One-line answer

The `word[0x15881A]` leg is **dissolved**: the phase-`0x22` body's stage-1/2 gate
is `MOV EAX,[0x00158816] ; SAR EAX,0x10` (bytes `a116881500` / `c1f810`), and by
the repo alias rule `dword[0x158816]>>16 = word[0x158818]` — **the act timer the
body itself accumulates**, not the config cell `word[0x15881A]` (period limit,
60 s). The ported substitute (`mr->act.timer >= 0x3C`) is therefore **faithful**,
not a divergence: FU211-L2 / FU195-L4 are a re-propagation of the FU-165 alias
error already falsified by FU-170 §4. The audio leg **stays a leg**: the
`0x974DC(0x1E)` path pushes onto the 20-slot ring at `0x155C8C`
(`FUN_00064EF0(1,·)`) and pokes the audio handler `FUN_00065CC0` →
`FUN_000A7705`, and **none of it is ported** — the port opens an audio device but
never submits PCM, and `mr->ref_whistle` (the staged `0x1E`) has no consumer.

---

## 1. Fresh address / slot confirmation

* **Phase table `0x110794`** — fresh `read_memory` (144 B). Row `0x22` bytes at
  `0x110794 + 0x22*4 = 0x11081C`: `00 b9 08 00` → **`[0x22] = 0x08B900`** (the
  trailing row `[0x21]` at `0x110818` = `74 b8 08 00` = `0x08B874`; the dword at
  `0x110820` = `ff 07 60 00` is unrelated).
* **Act table `0x1107EC`** — `0x1107EC + 0xC*4 = 0x1107EC + 0x30 = 0x11081C` =
  the same dword (`00 b9 08 00`), so **act `0xC` = phase `0x22` = `0x8B900`**
  (the FU165-L9 dual identity; base relation `0x110794 + 0x16*4 = 0x1107EC`).
* **xrefs to `0x8B900`** — fresh `get_xrefs_to 0x8B900` = **1**, `DATA`, from
  `0x11081C` (the shared table slot). Nothing else references the body.
* **`get_function_by_address 0x8B900`** → **"No function found"** (a table-only
  target, exactly as FU-195 §3 / FU-211 §1 found).
* The 2-byte pad: fresh `disassemble_bytes 0x8B900` (204 B) ends
  `0x8B9C9 RET`, then `0x8B9CA MOV EAX,EAX` (`8bc0`) before the producer
  `FUN_0008B9CC` at `0x8B9CC`.

---

## 2. The body `0x8B900` — full timeline (fresh, first-hand)

Fresh `disassemble_bytes 0x8B900` (`0x8B900..0x8B9CB`, 204 B, **67 body
instructions** + the `MOV EAX,EAX` pad; `RET` at `0x8B9C9`). A **three-stage
act-slot timer**. `EBX/ECX/EDX/ESI` saved (no record/argument is read — the
act-tail bodies ignore `EAX`/`EDX`).

**Head (`0x8B900`, shared with the act-0xB/9/0x20 siblings):**
```
0x8B900 PUSH EBX; PUSH ECX; PUSH EDX; PUSH ESI        ; 53 51 52 56
0x8B904 MOV AX, [0x00157A64]      ; 66a1647a1500  — frame delta
0x8B90A MOV DX, word[0x00158818]  ; 668b1518881500 — DX = act timer
0x8B911 ADD EDX, EAX              ; 01c2
0x8B913 MOV AL, [0x00158829]      ; a029881500 — AL = stage byte
0x8B918 MOV word[0x00158818], DX  ; 66891518881500 — timer += delta
0x8B91F CMP AL, 1                 ; 3c01
0x8B921 JC  0x8B92E               ; 720b — stage 0 (AL<1) -> stage-0 block
0x8B923 JBE 0x8B95C               ; 7637 — stage 1 -> stage-1 block
0x8B925 CMP AL, 2                 ; 3c02
0x8B927 JZ  0x8B98F               ; 7466 — stage 2 -> stage-2 block
0x8B929 POP ESI;POP EDX;POP ECX;POP EBX; RET   ; 5e 5a 59 5b c3 — stage > 2 no-op
```

**Stage 0 (`0x8B92E`):**
```
0x8B92E TEST AL, AL               ; 84c0 — AL == 0 here (only via 0x8B921 JC)
0x8B930 JNZ 0x8B9C5               ; 0f858f000000 — dead guard (see FU211-L5)
0x8B936 MOV EAX, 0x1E             ; b81e000000
0x8B93B CALL 0x000974DC           ; e89cbb0000 — the 0x1E event/sound
0x8B940 MOV BL, [0x00158829]      ; 8a1d29881500 — BL = stage (0)
0x8B946 XOR ESI, ESI              ; 31f6
0x8B948 INC BL                    ; fec3 — BL = 1
0x8B94A MOV word[0x00158818], SI  ; 66893518881500 — timer = 0
0x8B951 MOV byte[0x00158829], BL  ; 881d29881500 — stage = 1
0x8B957 POP×4 ; RET
```

**Stage 1 (`0x8B95C`):**
```
0x8B95C MOV EAX, [0x00158816]     ; a116881500
0x8B961 SAR EAX, 0x10             ; c1f810 — alias: word[0x158818] (the act timer; §3)
0x8B964 CMP EAX, 0x3C             ; 83f83c
0x8B967 JL  0x8B9C5               ; 7c5c — < 0x3C -> return (wait)
0x8B969 MOV EAX, 0x1E ; 0x8B96E CALL 0x974DC   ; b81e.. / e869bb0000
0x8B973 MOV DH, [0x00158829]      ; 8a3529881500
0x8B979 XOR ECX, ECX              ; 31c9
0x8B97B INC DH                    ; fec6 — DH = 2
0x8B97D MOV word[0x00158818], CX  ; 66890d18881500 — timer = 0
0x8B984 MOV byte[0x00158829], DH  ; 883529881500 — stage = 2
0x8B98A POP×4 ; RET
```

**Stage 2 (`0x8B98F`, terminal):**
```
0x8B98F MOV EAX, [0x00158816] ; 0x8B994 SAR EAX, 0x10   ; a116881500/c1f810 -> word[0x158818]
0x8B997 CMP EAX, 0x3C ; 0x8B99A JL 0x8B9C5              ; 83f83c/7c29 — < 0x3C -> return
0x8B99C MOV EAX, 0x1E ; 0x8B9A1 CALL 0x974DC            ; b81e.. / e836bb0000
0x8B9A6 XOR EBX, EBX ; 0x8B9A8 XOR EDX, EDX              ; 31db/31d2
0x8B9AA MOV word[0x00158818], BX  ; 66891d18881500 — timer = 0
0x8B9B1 XOR AH, AH                ; 30e4
0x8B9B3 MOV dword[0x00158808], EDX ; 891508881500 — installed handler = 0 (slot cleared)
0x8B9B9 MOV byte[0x00158828], AH  ; 882528881500 — act id = 0
0x8B9BF MOV byte[0x00158829], AH  ; 882529881500 — stage = 0
0x8B9C5 POP ESI;POP EDX;POP ECX;POP EBX; RET
```

**Derived semantics.** A **three-stage act-slot timer**. The head adds the frame
delta to the act timer (`word[0x158818] += word[0x157A64]`, byte-identical shape
to the act-0xB `0x8B874`, act-9 `0x88F4C` and phase-0x20 `0x8B688` heads). Each
stage fires the `0x1E` event via `0x974DC`, resets the timer, and advances
(0→1→2); stages 1 and 2 **wait** on the `word[0x158818] >= 0x3C` gate (the act
timer — §3); the terminal stage 2 clears the act slot (handler/id/stage/timer =
0). `stage > 2` is the `0x8B929` POP/RET no-op.

**Exact write set of the body (all 8 stores, fresh):**

| target | role | sites |
|---|---|---|
| `word[0x158818]` | act timer | `0x8B918` (+=delta), `0x8B94A`/`0x8B97D`/`0x8B9AA` (=0) |
| `byte[0x158829]` | act stage | `0x8B951` (=1), `0x8B984` (=2), `0x8B9BF` (=0) |
| `dword[0x158808]` | installed handler | `0x8B9B3` (=0, terminal) |
| `byte[0x158828]` | act id | `0x8B9B9` (=0, terminal) |

Plus **exactly three calls**, all `0x974DC(0x1E)` (`0x8B93B`, `0x8B96E`,
`0x8B9A1`). **The body performs no `FUN_000740A0` phase write, touches no record,
and writes `[0x157AC0]` nowhere** (FU-211 §3 re-confirmed: the `0x1E` arg is the
only outward effect).

---

## 3. The gate cell — `dword[0x158816]>>16` is `word[0x158818]` (the act timer), **not** `word[0x15881A]` — FU211-L2 dissolved

The body's stage-1/2 gate loads `[0x00158816]` and shifts right 16. Raw bytes,
fresh first-hand:

```
0x8B95C: a1 16 88 15 00   MOV EAX, [0x00158816]   ; dword at 0x158816
0x8B961: c1 f8 10         SAR EAX, 0x10           ; high word = word[0x158818]
0x8B964: 83 f8 3c         CMP EAX, 0x3C
0x8B967: 7c 5c            JL  0x8B9C5
0x8B969: b8 1e 00 00 00   MOV EAX, 0x1E
```
(fresh `disassemble_bytes` **and** an independent fresh `read_memory 0x8B95C`
(16 B) = `a1 16 88 15 00 c1 f8 10 83 f8 3c 7c 5c b8 1e 00` — agrees byte for byte.)

The `SAR EAX,0x10` of the dword loaded from `0x158816` yields the **high 16
bits = the bytes at `0x158818`/`0x158819` = `word[0x158818]`**, by the repo's own
alias rule (AGENTS.md: `dword[addr]>>16` is the word at `addr+2`;
`0x158816 + 2 = 0x158818`). `word[0x158818]` is precisely the cell the head
reads (`0x8B90A`) and accumulates (`0x8B918`) as the act timer. So the stage 1/2
wait is `act_timer >= 0x3C` — a **self-contained timer gate**, ~30 granted frames
at `frame_delta = 2`.

The **sibling** bodies use the identical idiom: fresh `disassemble_bytes 0x8B8C3`
(phase-0x21 stage 1) = `a1 16 88 15 00` / `c1 f8 10` / `83 f8 3c` / `7c 29` — the
same `word[0x158818]` gate.

This is the FU-165 → FU-194/FU-195/FU-211 alias misapply **already falsified by
FU-170 §4** (and re-asserted by FU-170-L6: "the act gates do not use it
[`word[0x15881A]`]"). The phase-`0x22` body is a fresh, first-hand confirmation of
the FU-170 correction, applied to this row specifically:

* FU-195 §2/§3 and FU-211 §2 render the gate as `word[0x15881A]`
  (FU-195 §3 line 89 "alias trap: = word[0x15881A] (timeline cell)"; FU-211 §2
  lines 97/111). The alias target is `word[0x158818]` (§9 errata).
* Consequently FU-195-L4 ("the `word[0x15881A]` gate … the derived bodies
  substitute the act timer; the cell's producer is unported. Same divergence
  class as phase 0x1F/0x20") and FU-211-L2 ("the body's stage-1/2 waits gate on
  the timeline cell `word[0x15881A]` … so the derived body substitutes the act
  timer") describe a **non-existent divergence**: the native gate *is* the act
  timer, and the ported body (`mr->act.timer < 0x3C`) is exact. **FU211-L2 is
  closed as a non-leg.**

**Port consequence.** `src/fifa96_engine/fifa96_match_run.c:6273` /
`:6280` (`if (mr->act.timer < 0x3C) return FIFA96_OK;`) is faithful; the
in-source comments that call it "the `word[0x15881A]` gate (FU195-L4)"
(`fifa96_match_run.c:6223`, `:6240`, `:6259`, `:6273`, `:6280`, and the row
descriptions at `fifa96_match_handlers.c:5245`, `:5247`) carry the stale cell
name — a comment-only errata, no code change (see §9).

---

## 4. Fresh `word[0x15881A]` census — writers, readers, and what it is

Fresh `get_xrefs_to 0x15881A` = **21** references: **3 WRITE**, **18 READ**.

**Writers (all fresh):**

| site | function | fresh evidence |
|---|---|---|
| `0x375E4` | `FUN_0003749C` | `0x375E1 IMUL EAX,EDX,0x3C` (`6bc23c`, `EDX=byte[0x14C1D1]`) then `66 a3 1a 88 15 00` = `MOV [0x15881A],AX`; `0x375EA IMUL EAX,EDX,0x14`; `0x375ED MOV [0x15881C],AX` |
| `0x4B514` | `FUN_0004B508` | `0x4B50A MOV EDX,0x3C`; `0x4B50F MOV EBX,0x1E`; `66 89 15 1a 88 15 00` = `MOV word[0x15881A],DX`; `0x4B51B MOV word[0x15881C],BX` |
| `0x88710` | `FUN_000886D4` | xref `WRITE` (the match reset; FU-170 §4 records `AX = byte[0x14C1D1] * 0x3C`, `[0x15881C] = byte[0x14C1D1] * 0x14`) |

**Readers (fresh):** `0x8AFE2`/`0x8B090`/`0x8B124`/`0x8B1E3`/`0x8B235`
(`FUN_0008AF38`, the clock harness + the period-completion blocks),
`0x8EF53`/`0x8EFC9` (`FUN_0008EF38`), `0x9260B` (`FUN_00092548`), `0x4B572`
(`FUN_0004B564`), `0x4B5AE`, and the raw `0x6E0B3`/`0x8F4EC`/`0x8FC90`/
`0x8FD1F`/`0x8FF7E`/`0x8FFFA`/`0x908A1`/`0x908CD`. **Neither `0x8B95C` nor
`0x8B98F` (nor `0x8B8C3`) is in the list** — a census confirmation that the
phase-`0x22` handler does not read this cell.

**What the cell is.** A **config-derived period limit (seconds)**. `FUN_0004B508`
writes `0x3C` (60 s) into `word[0x15881A]` and `0x1E` (30 s) into
`word[0x15881C]` — the FU-213 §1/§4 map (`state.period_length` / `extra_length`)
and the engine's `FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET 60` / `EXTRA_SECONDS_RESET
30` (*cross-cited* FU-213 §4). `FUN_0003749C`/`FUN_000886D4` derive it from the
match-length byte `[0x14C1D1]`. The clock harness reads it as the completion
limit (`0x8B1E3 MOV DX,word[0x15881A]` compared against the period seconds
`[0x157AB6]`, the completion block `0x8B22F`/`0x8B235`, *cross-cited*
FU-202:235 / FU-213 §2.3).

**So:** `word[0x15881A]` is written once per match init/reset (not per frame), read
by the clock/completion code, and **never read by the phase-`0x22` handler** — the
handler reads the adjacent `word[0x158818]` (§3). The "cell producer" FU211-L2
asked for is the config writer set above (all unported as *producers*, but the
engine models the *value* via `fifa96_match_run_period_setup` / `set_period`,
FU-213 §6) — and it is **irrelevant to this row**.

---

## 5. The audio ring-push (FU211-L3 / FU195-L3) — the exact chain and the port gap

### 5.1 The native chain (all fresh)

The `0x974DC` sink the phase-`0x22` body calls (three times) is:

```
0x974DC PUSH EDX            ; 52
0x974DD MOV EDX, EAX        ; 89c2  — EDX = the event id (0x1E)
0x974DF CALL 0x00064EC0     ; e8dcd9fcff  — FUN_00064EC0()  [ring push]
0x974E4 MOV EAX, EDX        ; 89d0
0x974E6 CALL 0x00065CC0     ; e8d5e7fcff  — FUN_00065CC0(id) [audio handle]
0x974EB POP EDX ; RET
```

Fresh `get_xrefs_to 0x974DC` = **42** unconditional calls; three of them are this
body's `0x8B93B`/`0x8B96E`/`0x8B9A1`.

**(a) `FUN_00064EC0` — the ring push (fresh `decompile_function 0x64EC0`):**
```c
void FUN_00064ec0(void) {
  if ((DAT_00109a98 & 0x80) == 0) {          /* the suppression gate */
    if (FUN_00037ae4() == 0) {               /* a second probe gate */
      FUN_00064ef0(1, <EDX>);                /* EDX = the event id (0x1E) */
    }
  }
}
```
`FUN_00037ae4` (fresh `decompile_function 0x37AE4`) = `FUN_00036bc8(5)` /
`FUN_00036bc8(7)` / `FUN_00036bc8(7)` input-state probes returning `0` only when
all are clear (an input/held-key suppression probe — semantics out of scope,
FU221-L2).

**(b) `FUN_00064EF0` — the ring buffer (fresh `decompile_function 0x64EF0`):**
`param_1 == 1` pushes `param_2` onto a **20-slot (`0x14`) ring at `0x155C8C`** —
head `DAT_00109af0`, tail `DAT_00109af4`, count `DAT_00109aec`
(`(&DAT_00155c8c)[head] = param_2; head = (head+1) & wrap; count++`). `param_1 != 1`
is the pop/return path. So the phase-`0x22` event id `0x1E` is **enqueued** onto
the 20-slot ring at `0x155C8C`.

**(c) `FUN_00065CC0` — the audio handle (fresh `decompile_function 0x65CC0`):**
```c
void FUN_00065cc0(uint id) {
  if (DAT_00155ce0 != 0 && DAT_00155d30 != 0 &&
      0 < (int)DAT_00155d00 && DAT_0010a250 != 0) {
    _DAT_0010a254 = FUN_000a7705(id, DAT_00155d00);   /* the voice/engine trigger */
  }
}
```
So when the audio state is live (`[0x155CE0]`, `[0x155D30]`, `[0x155D00] > 0`,
`[0x10A250]`), `FUN_00065CC0` calls `FUN_000A7705(id, [0x155D00])` and posts the
result to `_DAT_0010A254`.

**Chain summary (phase-`0x22` → audio):** `0x8B900` stage tick → `0x974DC(0x1E)`
→ `FUN_00064EC0` (gate) → `FUN_00064EF0(1, 0x1E)` → **ring enqueue at `0x155C8C`**;
then `FUN_00065CC0(0x1E)` → `FUN_000A7705(0x1E, [0x155D00])` → `[0x10A254]`.
The ring is drained elsewhere (`FUN_00064EF0(0,·)` / the mixer) — the drain side
and `FUN_000A7705` are not traced here (FU221-L3).

### 5.2 What the port lacks — the audio subsystem is **not ported**

* **No event ring, no push, no voice/mixer.** Fresh engine grep: there is **no**
  port of `0x974DC`, `FUN_00064EC0`, `FUN_00064EF0`, `FUN_00065CC0`,
  `FUN_000A7705`, the `0x155C8C` ring, or the `[0x109A98]`/`[0x109AEC]` cells.
* **The staged `0x1E` is a write-only observation slot.** The ported bodies set
  `mr->ref_whistle = 0x1Eu` (`fifa96_match_run.c:6234`/`:6241` for phase 0x21 and
  `:6267`/`:6274`/`:6281` for phase 0x22). Fresh grep over `src/` and `tools/`:
  `ref_whistle` is **never read** (only assigned, or asserted in tests). The
  header documents it as an observation slot whose "event-queue sinks stay
  unported" (`include/fifa96_engine/fifa96_match_run.h:863..877`).
* **The platform audio layer is device-open only.** `fifa96_engine.c:138` calls
  `plat->audio_open(…, 22050u, 2)`, but a fresh grep finds **no `audio_submit`
  call site** anywhere in `src/`/`tools/` — the PCM sink (`platform_null.c:50`,
  `platform_sdl3.c:290`) is defined and never fed. The engine produces no audio
  frames.

**Verdict.** The native audio event path the phase-`0x22` row drives is wholly
unported; the port has a deliberately stubbed platform layer (open only) and a
write-only `ref_whistle` slot. This is an **honest leg to be parked**, exactly as
FU-195-L3 / FU-211-L3 framed it — unless and until the audio subsystem (event
ring + voice allocator + mixer) is in port scope, which it is not today.

---

## 6. The other phase-`0x22` stores (full enumerate) and the non-stores

From §2 the body's complete write set is four cells (act slot + timer):

1. `word[0x158818]` — the act timer (`0x8B918` accumulate; `0x8B94A`/`0x8B97D`/
   `0x8B9AA` reset).
2. `byte[0x158829]` — the act stage (`0x8B951`→1, `0x8B984`→2, `0x8B9BF`→0).
3. `dword[0x158808]` — the installed handler (`0x8B9B3`→0, terminal).
4. `byte[0x158828]` — the act id (`0x8B9B9`→0, terminal).

**Non-stores (fresh, negative):** no `FUN_000740A0` phase write (the only three
CALLs target `0x974DC`); no `[0x157AC0]` OL-85 write; no record/field touch; no
score write; no `[0x15882A]`/`[0x158822]` touch. There is **no "phase advance",
"row install" or "score" store** in this body — the only outward effects are the
act slot and the `0x974DC(0x1E)` audio event (the §5 leg).

---

## 7. The engine extension point and the tape verdict

### 7.1 Engine seam (fresh first-hand source)

* **Row registration:** `fifa96_match_phase_table[0x22] = fifa96_match_action_phase_22`
  (`src/fifa96_engine/fifa96_match_handlers.c:5246`; declaration
  `include/fifa96_engine/fifa96_match_run.h:1634`; body
  `src/fifa96_engine/fifa96_match_run.c:6263..6288`).
* **The body** is a faithful port of §2: head `mr->act.timer += state.frame_delta`
  (`:6265`); stage 0 event + timer 0 + stage 1 (`:6266..6270`); stage 1 gate
  `timer < 0x3C` then event + timer 0 + stage 2 (`:6272..6277`); stage 2 gate then
  event + slot clear (`:6279..6285`). By §3 these gates are **exact** (they are
  the act timer).
* **The act dispatch (the native `0x8BAE7 CALL 0x888FC(0xC,0,1)` mirror):**
  `fifa96_match_run_phase_drive` → `fifa96_action_phase_period_end` → the new
  `fifa96_match_act_dispatch(mr, out.act, 0u, 1)` at
  `src/fifa96_engine/fifa96_match_run.c:2634` (completion arm) and `:2676` (mid-ET
  / period-4 arm). `fifa96_match_act_dispatch` (`src/fifa96_engine/fifa96_match_act.c`)
  resolves `out.act` (0xB/0xC) → phase `0x16+act` (0x21/0x22) and runs
  `fifa96_match_phase_table[phase].fn` (guard: `fn == NULL` → `FIFA96_OK`).
* **The live producer** `[0x157AC0]` (`mr->global_157ac0`) is threaded into the
  chooser (`:2614..2622`), so a differing-score regulation/ET-2 completion selects
  `out.act = 0x0C` (the extra-time set arm), reaching the registered row.

### 7.2 Tape reachability — does M2 reach phase `0x22`?

**No — not on the golden tape.** Fresh first-hand: the M2 acceptance harness
records that "the tape reaches no period-end completion (the golden stays
`441415fc…`)" (`tests/test_engine_m2.c:3524..3525`); every phase-19 producer
"completes only past the scripted tape's path … so M2 is byte-identical at
`441415fc…`" (`:1455..1465`). The tape stages no extra-time period.

The row is exercised only by **staged completions**:

* the unit tests `test_phase_22_body_direct` / `test_phase_22_registered` /
  `test_phase_22_producer_live_via_phase_drive` (`tests/test_engine_match_act.c:766/
  795/814`) — the last drives a real `fifa96_match_run_phase_drive` with a
  hand-staged `clock_period_ended` + differing scores;
* the acceptance rider `run_phase19_riders` block 1 (`tests/test_engine_m2.c:3540..3562`),
  also a hand-staged completion.

This is the same live-path standard on which phase-19 T4 **counted** row `0x22`
(`docs/ENGINE.md:5..6` census `53/80 = 66%`; `README.md:1` title `66%` — the
derived `floor(100×53/80)`). "M2 reaches it" is therefore **false as a tape
statement** but **true as a producer-live statement**: the real chooser +
`phase_drive` act dispatch install act `0xC` and the registered row runs stage 0
invoke-now, and because the body is render-invisible (act slot + write-only
`ref_whistle`) the golden would not move even if a tape did reach it.

### 7.3 What the engine must add (extension point)

**Nothing further for the row itself** — registration, body and the act-dispatch
seam are all landed and faithful. The only genuine extension is the §5 audio
subsystem (out of current scope). If audio is ever ported, the extension point is
the `mr->ref_whistle` slot: replace the write-only staging with the native
`0x974DC` chain (gate `[0x109A98]&0x80` → ring-enqueue at a `0x155C8C` analogue
via a `FUN_00064EF0(1,·)` mirror → `FUN_00065CC0`/`FUN_000A7705` voice trigger),
and feed the drained ring into `plat->audio_submit` (which currently has no
caller). No render path reads `ref_whistle`, so this is additive.

---

## 8. Numbered legs

1. **FU221-L1 — the `0x974DC(0x1E)` audio subsystem is unported (the FU211-L3 /
   FU195-L3 remainder).** The phase-`0x22` (and every `0x974DC` caller's) only
   outward effect is the ring enqueue at `0x155C8C` (`FUN_00064EF0(1,·)`) plus the
   audio handle `FUN_00065CC0` → `FUN_000A7705` (§5). The port has **no** ring /
   voice / mixer; `plat->audio_submit` has **no call site**; `mr->ref_whistle` is
   write-only. **Recommendation: park the leg honestly** — no fake. Owner: the
   audio subsystem slice (if ever in scope). Do **not** count it toward the census
   (it is unported, not wired).
2. **FU221-L2 — the sound-suppression gate `(byte[0x109A98] & 0x80) == 0 &&
   FUN_00037ae4() == 0` (fresh `decompile_function 0x64EC0`/`0x37AE4`).**
   `FUN_00037ae4` = the `FUN_00036bc8(5)` / `FUN_00036bc8(7)` input probes. This
   gate decides whether the `0x1E` event is even enqueued; unported and governed
   by the input layer, which is out of this row's scope. Bounded: not traced
   further. Owner: the audio/input subsystem.
3. **FU221-L3 — the ring drain side and `FUN_000A7705`.** The 20-slot ring at
   `0x155C8C` is drained by `FUN_00064EF0(0,·)` callers and the voice allocator
   (`FUN_000A7705`, `_DAT_0010A254`); neither the drain call sites nor the voice
   trigger are traced here. Marked **unknown** (a data/control-flow gap, not a
   semantic claim). Owner: FU221-L1's audio slice.
4. **FU221-L4 — comment-only errata: the `word[0x15881A]` cell name in the
   landed phase-21/22 code.** `fifa96_match_run.c:6223`/`:6240`/`:6259`/`:6273`/
   `:6280` and `fifa96_match_handlers.c:5245`/`:5247` still call the gate "the
   `word[0x15881A]` gate (FU195-L4)" though the byte-exact cell is `word[0x158818]`
   (§3). No code change (the code is exact); a future comment sweep should correct
   the cell name and cite this slice. No owner needed for behaviour.

---

## 9. Errata (quoted; prior sections are not rewritten)

* **FU-195 §3 / FU-211 §2 — the gate cell (the load-bearing one).** FU-195 §3
  renders stage 1 as "`0x8B8C8 SAR EAX, 0x10  ; alias trap: = word[0x15881A]
  (timeline cell)`"; FU-211 §2 renders stage 1/2 as "`alias trap: = word[0x15881A]`"
  and "`-> word[0x15881A]`". Fresh §3: the operand is `[0x00158816]` (`a116881500`),
  so by the repo rule `dword[0x158816]>>16 = word[0x158818]` — **the act timer**,
  not `word[0x15881A]`. This is the same misapply FU-170 §4 already corrected for
  the act-9 stage-3 gate; FU-170-L6 ("the act gates do not use it") stands. Quoted
  so the frozen text is not rewritten.
* **FU-195 §3 / FU-211 §2, the "substitute the act timer" framing.** Because the
  native gate **is** the act timer, the ported `mr->act.timer >= 0x3C` is exact,
  not a derived substitute: FU-195-L4 ("the derived bodies substitute the act
  timer … the cell's producer is unported") and FU-211-L2 ("the derived body
  substitutes the act timer … same divergence class as phase 0x1F/0x20") are
  **closed** — there is no divergence for this row. (FU-188's phase-0x20 gate and
  FU-194's act-7/act-9 gates are the same `[0x158816]>>16 = word[0x158818]` idiom,
  per FU-170 §4; their "0x15881A" wording is the same stale cell name.)
* **FU-195 §2, the `FUN_00065CC0` argument.** FU-195 §2 (and FU-211 §?): "sets
  `_DAT_0010A254 = FUN_000A7705(id, [0x155D30])`". Fresh
  `decompile_function 0x65CC0`: the second argument is `DAT_00155d00` =
  `[0x155D00]`; `[0x155D30]` is one of the four **guard** cells. Corrected: the
  call is `FUN_000A7705(id, [0x155D00])`.
* **FU-211 §10 "T4 registers the phase-table slot `0x22`".** Confirmed fresh
  (`fifa96_match_handlers.c:5246`); no correction. §10's FU211-L2/L3 "stay legs"
  is **partly superseded**: L2 is dissolved by §3; L3 remains FU221-L1 (§8).
* **Determinism.** M1 `09b726b7…` unmoved; M2 `441415fc…` byte-identical (this
  slice is read-only; the row is render-invisible regardless — §7.2).

---

## 10. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, 2706
  functions, `.image` overlay present.
* `read_memory` — `0x110794` (144 B), `0x158816` (24 B), `0x8B95C` (16 B).
* `get_xrefs_to` — `0x8B900` (1, DATA), `0x15881A` (**21** = 3 WRITE + 18 READ),
  `0x974DC` (**42**).
* `get_function_by_address` — `0x8B900` (none — table-only).
* `disassemble_bytes` — `0x8B900` (204 B / 68 slots incl. pad), `0x8B8C3` (16 B,
  the phase-21 sibling gate), `0x974DC` (32 B), `0x375D0` (48 B), `0x4B500` (64 B).
* `decompile_function` — `0x64EC0`, `0x64EF0`, `0x65CC0`, `0x37AE4`.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2555..2683` the phase driver + the act dispatch at `:2634`/`:2676`;
`:6195..6288` the phase-21/22 bodies); `src/fifa96_engine/fifa96_match_act.c`
(the dispatch + the `fn == NULL` guard); `src/fifa96_engine/fifa96_match_handlers.c`
(`:5245..5247` the phase-21/22 rows); `src/fifa96_engine/fifa96_engine.c:138`;
`src/fifa96_engine/platform_null.c:47..72`;
`src/fifa96_engine/platform_sdl3.c:267..323`;
`include/fifa96_engine/fifa96_match_run.h:872..877, 1617, 1634`;
`include/fifa96_engine/fifa96_platform.h:23..24`;
`tests/test_engine_match_act.c:662..893`;
`tests/test_engine_m2.c:1455..1493, 3523..3562`; `docs/ENGINE.md:5..6`;
`README.md:1`.

Docs cross-cited: FU-165 (§3.3/§3.6/L2, the origin of the alias misapply),
FU-170 (§4, the `word[0x15881A]` erratum — the load-bearing correction; L6),
FU-194 (§2/the act-tail head), FU-195 (§2/§3/§4/§6.1/L3/L4), FU-211
(§1/§2/§3/§5/§6/§7/L2/L3/L5/§10), FU-202 (the clock-length read), FU-206 (§5,
the render-read ground truth), FU-213 (§1/§2.3/§2.4/§2.5/§4/§6, the period limit
and the chooser), FU-208/FU-209 (the period-end spine). AGENTS.md (the alias rule
and the census rule). **No writes beyond this slice file**; no
rename/label/comment/function/script/project save; no git commands.
