# FU-276 — the contact-gate invoker, the offside receive arm, and the L10 byte

Promoted canonical slice — phase-26b (FU276); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 4 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its ruling are unchanged by promotion.

# W26b-4 / FU276 — the contact-gate invoker, the offside receive arm, and the L10 byte

Read-only Ghidra recon slice — phase-26b recon-ahead wave **Task 4** (FU-276);
authoritative program **`/FIFA96.EXE`** (Ghidra MCP, explicit case-sensitive
program argument; scripts disabled; standard read-only tools only). Every
address/byte claim below is a **fresh** `disassemble_function` /
`disassemble_bytes` / `get_xrefs_to` / `get_function_by_address` of **this
session** unless marked *cross-cited* (then the frozen FU doc is named).
Engine/loader source cites are read-only reads. LE-image globals are flat
addresses (`+0x100000` maps a file offset). Alias traps applied:
`dword[x] >> 16` = the word at `x+2`, `>> 24` = the byte at `x+3`, so
`dword[rec+0x8E]>>24` = `byte[rec+0x91]`. Program identity re-confirmed this
session (`get_current_program_info`): `FIFA96.EXE`, `/FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions, `.image` overlay present. **No writes**:
no rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git command; the ISO/HDD untouched.

**Scope (brief).** Step 1 the **contact gate** — `fifa96_match_run_contact`
(`match_run.c:6489`, called from row `0x0C` `match_handlers.c:3329` → act-3
invoke-now `run.c:6530`); the live path that raises the contact precondition;
the pad `want 0x60` handler. Step 2 the **offside** entry —
`fifa96_match_run_offside_reception` (`match_run.c:6544`, no engine caller
today), the receive arm `FUN_0007A084 0x7A448`, the offside-enable producer for
`field_4c2f2`. Step 3 the **`FU268-L10` byte divergence** (native `code`/`+0x91`
vs engine `type`/`+0x8E`, `match_run.c:6597`). Step 4 port shapes + the
live-trigger ruling.

Extends / first-hand re-verifies FU-268 (`docs/ghidra/FU268_fouls_offside_extratime.md`),
FU-150 (`FU150_fouls_referee_offside.md`), FU-271 (`FU271_live_trigger_census.md`)
and FU-169 (`FU169_row5e_live_installer.md`).

**Result in one line.** The contact gate is **producer-wired live but unraised
in the smoke** (pad `0x7CE38` want-0x60 and the row-`0x04` AI duel each install
row `0x0B` invoke → row-`0x0B` arm-1 → row-`0x0C` → `fifa96_match_run_contact`;
neither fires in any test/tape/smoke) → **LIVE-LEG (FU276-L1)**; the offside
check is **double-dormant** — the engine's *live* receive arm
(`fifa96_match_run_ball_receive` = native `FUN_0007A084`, driven by the live
kick path) **drops** the `0x7A448` offside call as a declared leg, and the
offside entry `fifa96_match_run_offside_reception` + its enable `field_4c2f2`
(front-end-only, default 0) have no live producer → **LIVE-LEG (FU276-L2/L3)**;
**`FU268-L10` is confirmed**: the native reads `code`/`+0x91` at both
`0x79EF7`/`0x79F00` and at the arm gate `0x7A419`, but the engine's `!= 0x11`
disjunct reads `own->type` (native `+0x8E`, the 0–7 octant) so the native
`code != 0x11` test is dropped → fix `own->code != 0x11u` (**FU276-L4**).

---

## Step 1 — the contact gate and its live invoker

### 1.1 The engine entry and its one native-backed caller (fresh)

`fifa96_match_run_contact` (`match_run.c:6489`) is the engine's contact entry.
Its only engine caller is the row-`0x0C` body (`match_sp_action_0c_foul`,
`match_handlers.c:3329`), which is the port of the native row-`0x0C` re-call
block. The native block is `0x81E8A..0x81ED4` — fresh `disassemble_bytes
0x81E80` this session:

```
0x81E8A CMP dword[0x14C306],0 ; JZ 0x81ECD   ; <<< the settings-0x0A gate
0x81E93 CMP dword[0x15888F],0 ; JZ 0x81ECD   ; rec_first != NULL
0x81E9C MOV EBX,[0x158893] ; CMP EBP,EBX ; JNZ 0x81ECD   ; EBP == rec_second
0x81EA6 CALL 0x92AC8 ; TEST AL,0x7 ; JZ 0x81ECD   ; the 1-in-8 rehearsal skip
0x81EAF MOV ECX,0x158897                      ; point = the stored contact triple
0x81EB4 MOV EAX,0x1                           ; kind 1
0x81EB9 MOV EDX,[0x15888F]                    ; rec A
0x81EBF CALL 0x8A43C                          ; <<< the foul decision (fifa96_ref_foul_decide)
0x81EC4 MOV byte[0x15888E],1                  ; the re-call-consumed flag
0x81ECD XOR DL,DL ; MOV byte[0x15888E],DL
```

So the "contact precondition" is: **`[0x14C306] != 0`** (settings `0x0A`, the
fouls master) AND `rec_first != NULL` AND `EBP == rec_second` (the row-`0x0C`
body is running *on* the second contact record) AND the RNG `(AL & 7) != 0`.
`fifa96_match_run_contact` mirrors the `[0x14C306]` gate (`run.c:6503`) and the
1-in-8 draw (`run.c:6508`).

The `[0x14C306]` cell itself has **one writer producer only** — fresh
`get_xrefs_to 0x14C306` = **5**: writers `0x3754F`/`0x37581` in
`FUN_0003749c`, readers `0x8A47A`/`0x8A4DC` in `FUN_0008a43c` and `0x81E8A`
(this block). Fresh `disassemble_bytes 0x37540`:

```
0x37545 MOV EAX,0xa ; 0x3754a CALL 0x1d940 ; 0x3754f MOV [0x14C306],EAX   ; settings get(0x0A)
0x37578 TEST DL,DL (=[0x14C32A]) ; JZ 0x375a9
0x37581 MOV dword[0x14C306],ECX   ; zeroed when the session gate is set
```

i.e. `field_4c306` is a **front-end settings-translator cell**
(`FUN_0003749c`, get setting `0x0A`; zeroed when `[0x14C32A] != 0`). It has **no
in-match producer**. Under the FU-68 defaults setting `0x0A = 2`
(`fifa96_settings.c:22`), `fifa96_settings_handoff` binds
`config->field_4c306 = competition ? 0 : settings->value[0x0A]`
(`fifa96_settings.c:85`), so for `match_type 0` `field_4c306 = 2` — the gate is
**ON by default**, so the settings cell is not itself the blocker.

### 1.2 The live invoker — the pad `want 0x60` handler and the row-`0x04` duel

The row-`0x0C` body is only *reached* when a row-`0x0B` duel installs it. The
row-`0x0B` arm-1 (fresh `disassemble_bytes 0x81BA0`) registers contact **kind 2**
and installs row `0x0C` with no invoke:

```
0x81BC3 LEA ECX,[ESI+0x59]           ; point = found.pos
0x81BC6 MOV EAX,0x2                  ; kind 2
0x81BCB MOV EBX,ESI                  ; rec B = found opponent
0x81BCD MOV EDX,EBP                  ; rec A = challenger
0x81BCF CALL 0x8A3FC                 ; registrar (fifa96_ref_contact_register)
0x81BD4 MOV EDX,0xC ; MOV AL,byte[EBP+0x8E] ; XOR ECX,ECX ; XOR EBX,EBX
0x81BE3 MOV byte[ESI+0x8E],AL
0x81BE9 MOV EAX,ESI ; 0x81BEB CALL 0x7D9A4   ; ACTION 0x0C install on ESI(opponent), invoke=0
```

Row `0x0B` is installed **live** by two paths, both fresh this session:

**(a) The pad `want 0x60` handler `0x7CE38`.** Fresh `get_xrefs_to 0x7CE38` = 1
(a `DATA` slot at `0x110824`, i.e. an entry of the pressed row table). Fresh
`disassemble_bytes 0x7CDF0` + `0x7CE60`:

```
0x7CE3E MOV EAX,[0x157a4a]; SAR 0x18; CMP 2; JNZ 0x7CE98   ; phase 2 gate
0x7CE4B MOV EAX,[ESI+0x8e]; SAR 0x18; MOV AL,[EAX+0x110680]; AND 1   ; type bits
0x7CE63 CMP byte[ESI+0x99],0 ; JNZ 0x7CE98                 ; the +0x99 gate
0x7CE6C MOV EAX,[ESI+0x28]; MOV AL,[EAX]; CMP {0xC,0x59,0x5E} ; anim gate
0x7CE85 MOV ECX,0x1 ; MOV EDX,0xb ; MOV EAX,ESI ; XOR EBX,EBX
0x7CE93 CALL 0x7D9A4                                        ; <<< install 0x0B, invoke (ECX=1)
0x7CE98 MOV EAX,[ESI+0x8e]; SAR 0x18; CMP 0xb; SETZ AL      ; returns code==0x0B
```

The engine ports this as `match_input_handler` case `0x7CE38u`
(`fifa96_match_run.c:693-700`: "pressed code-0 want 0x60: install 0x0B
(invoke)"), ported with the `+0x99` gate staged 0 (OL-72; the case body has no
`+0x99` test — `fifa96_match_run.c:653-656`).

**(b) The row-`0x04` AI duel.** Fresh `disassemble_bytes 0x7ED20`:

```
0x7ED50 CALL 0x92AC8 ; AND EAX,0x1f ; CMP EAX,EDX ; JNC 0x7ED89   ; the (r2 & 0x1F) < thresh gate
0x7ED5C MOV ECX,0x1 ; 0x7ED61 MOV EDX,0xb ; 0x7ED66 MOV EAX,EBP ; 0x7ED68 XOR EBX,EBX
0x7ED6A CALL 0x7D9A4                                              ; <<< install 0x0B, invoke
0x7ED6F ADD ESP,0x10                                               ; the return tail
```

The engine binds this as `row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF,
0x0B, 0, 1)` (`fifa96_outfield.c:598-600`, a comment tagged `/* 0x7ED6F */`).
**Erratum (fresh):** `0x7ED6F` is the `ADD ESP,0x10` *after* the install; the
actual install CALL is `0x7ED6A` (`MOV EDX,0xB` at `0x7ED61`, `MOV ECX,1` at
`0x7ED5C`). The engine comment's `0x7ED6F` is the return site, not the install.

This row-`0x04` AI is **live**: it runs on the per-record outfield machine
(`match_run.c:1393-1409`, `match_run_outfield_input` → the `0x0B` install); the
native analog is the outfield record machine (`FUN_0007CA54`/`0x7E7C8..0x7F141`)
that calls row-04 for records 1..10. The duel arm is RNG/position/score gated
(`fifa96_outfield.c:583-604`: `(r & 0x3F) == 0` at `0x7ECB4`, metric `d <= 0x50`
at `0x7ECD4`, the score / `3*team_7d7 < opp_7d7` term at `0x7ED1B`, then
`(r2 & 0x1F) < thresh` at `0x7ED58`), so a live tackle needs a 1-in-64 draw plus
conditions.

### 1.3 Live-vs-leg ruling (Step 1)

**LIVE-LEG.** The contact gate is **producer-wired live**: the pad handler
`0x7CE38` (want 0x60) and the row-`0x04` AI duel each install row `0x0B` with
invoke, and the ported chain row-`0x0B` arm-1 → row-`0x0C` →
`fifa96_match_run_contact` already exists and is unit-proven. But **no headless
granted-frame test, tape frame or smoke raises it**: the row-`0x04` duel is gated
by a 1-in-64 RNG draw + the metric/score/threshold terms, and the pad wants the
`0x60` button (the smoke's held-direction input does not press it). ENGINE.md
`:1754` still records "the FU-150 entries have no live producer; … tape v10 pins
`ref_machine == REF_NONE` with no whistle/speech/decision cells" — the accurate
statement is *producer-wired, unraised*. **Payoff: Layer 1** (no visible foul
outcome; the audio sinks are unported, FU-150 leg 3).

### 1.4 Numbered open legs (Step 1)

* **FU276-L1 — the live contact invoker (the sit-9/0xA foul producers) is not
  raised by any test/tape/smoke.** The pad handler `0x7CE38` (want 0x60,
  `0x7CE93 install 0x0B invoke`) and the row-`0x04` AI duel (`0x7ED61/0x7ED6A
  install 0x0B invoke`, engine `fifa96_outfield.c:599`) are wired live but
  never fire in the smoke. Owner: the row-`0x04`/row-`0x0B` foul-chain wiring
  (`fifa96_outfield.c:599`; `fifa96_match_run.c:699`/`fifa96_match_run.c:693`); needs a
  headless granted-frame proof. (= FU268-L1.)
* **FU276-L5 — the `0x80EE7` severity-RNG registrar caller.** Fresh
  `disassemble_bytes 0x80ED0`: `CALL 0x92AC8; TEST EAX,EDX; SETNZ AL; CWDE;
  CALL 0x8A3FC` (kind 0/1) then `MOV EDX,0xC; EAX=ESI; ECX=1; XOR EBX,EBX;
  CALL 0x7D9A4` (row `0x0C` invoke-now) — a second registrar caller in the
  `0x80Exx` foul band, not the row-`0x0B`/`0x0C` bodies. Owner: the `0x80Exx`
  foul body (FU-178/FU-190 band). (= FU268-L3.)
* **FU276-L6 — `field_4c306` has no in-match producer.** Fresh
  `get_xrefs_to 0x14C306` = 5; the only writers are `0x3754F`/`0x37581` in the
  front-end settings translator `FUN_0003749c`. Owner: front-end settings
  (`FUN_0003749c`). Note the gate is ON by default (setting `0x0A = 2`), so this
  is not the smoke blocker.

---

## Step 2 — the offside entry and the receive arm

### 2.1 The offside check `FUN_00079D5C` (fresh `disassemble_function`, 138 insns)

Fresh caller census: `get_xrefs_to 0x79D5C` = **1** — only `0x7A448`
(`FUN_0007A084`). The pre-gates and the record-state eligibility gate:

```
0x79D68 MOV EAX,[0x157A4A]; SAR 0x18; CMP 2; JNZ 0x79F30     ; phase 2 only
0x79D79 CMP word[0x157A6A],0 ; JG 0x79F30                    ; [0x157A6A] suppression
0x79D87 CMP dword[0x14C2F2],0 ; JZ 0x79F30                   ; <<< settings 0x10 (offside enable)
...
0x79E3C CALL 0x92AC8 ; AND EAX,0x3F                          ; the 6-bit tolerance draw
...
0x79EDD TEST ESI,ESI ; 0x79EE8 CMP EAX,0x3c0 ; 0x79EEF CMP ESI,[0x158777]
0x79EF7 MOV EAX,[ESI+0x8E]; SAR EAX,0x18                    ; <<< byte[ESI+0x91] = code
0x79F00 MOVSX DX,byte[ESI+0x91]                             ; <<< code again
0x79F08 CMP EAX,0x11 ; JZ 0x79F30                           ; code != 0x11
0x79F0D MOVSX ECX,DX ; CMP ECX,0x10 ; JZ 0x79F30            ; code not in
0x79F15 CMP ECX,0x1d ; JZ 0x79F30                           ;   {0x10,0x1D,0x1E}
0x79F1A CMP ECX,0x1e ; JZ 0x79F30
0x79F1F MOV EAX,0x3 ; LEA ECX,[ESI+0x59] ; XOR EBX,EBX ; MOV EDX,ESI
0x79F2B CALL 0x8A43C                                        ; kind-3 decision (fifa96_ref_offside_event)
```

Both eligibility reads (`0x79EF7`, `0x79F00`) test the **`code` byte `+0x91`**
— `dword[+0x8E] >> 24` is `byte[+0x91]` (the house alias trap). The
`[0x14C2F2]` gate at `0x79D87` is the offside-enable settings cell.

### 2.2 The receive arm `FUN_0007A084` and `0x7A448` (fresh)

`get_xrefs_to 0x7A448` = **0** (it is an interior call site, not an entry);
`get_xrefs_to 0x7A084` = **3** — `0x6F56F` (`FUN_0006E8E8`, the per-record
animation-event machine), `0x7A8C7` and `0x7A8EF` (`FUN_0007A490`, the
kick/ball tail). `get_function_by_address 0x7A084`: body
`0x7A084..0x7A456`. Fresh `disassemble_bytes 0x7A084` + `0x7A3C0` + `0x7A410`:

```
0x7A08D MOV EBP,[0x158730]                       ; the receiving record
0x7A0A4 MOV EAX,[0x158740]; SAR 0x18; CMP byte[EAX+0x1104bb],0 ; entry gate
...
0x7A40A CMP EBP,[0x158724] ; JNZ 0x7A419         ; the possession carrier?
0x7A412 MOV byte[0x15872d],0x14                  ;   -> release countdown = 0x14
0x7A419 MOV EAX,[EBP+0x8E]; SAR 0x18             ; <<< byte[EBP+0x91] = code
0x7A422 MOVSX DX,byte[EBP+0x91]                  ; <<< code again
0x7A42A CMP EAX,0x11 ; JZ 0x7A44D                ; code != 0x11
0x7A432 CMP EAX,0x10 / 0x1d / 0x1e ; JZ 0x7A44D  ; code not in {0x10,0x1D,0x1E}
0x7A441 MOV EDX,0x158738 ; 0x7A446 MOV EAX,EBP
0x7A448 CALL 0x79D5C                             ; <<< the offside check
```

So the receive arm runs its **own identical `code`-eligibility gate** at
`0x7A419..0x7A43F` and only then calls `FUN_00079D5C` at `0x7A448` (which
re-tests the same gate). The native arm is reached from two live native paths:
the kick/ball tail `FUN_0007A490` (`0x7A8C7`/`0x7A8EF`) and the per-record
animation-event machine `FUN_0006E8E8` (`0x6F56F`, gated by
`byte[0x158743] >= 0`, `[0x158740]>>24 < 0xF`, `byte[EAX+0x1104bb] != 0`).

### 2.3 Engine reachability (fresh reads)

**The engine the receive-arm body as `fifa96_match_run_ball_receive`
(`match_run.c:3769`) and it IS live.** Its header comment
(`match_run.c:3762-3768`) is explicit:

> "the FUN_0007A084 derived receive arm … the 0x18E2-class vector rebuild, the
> sound-id selection/sink, the timer81 loop, the `[team+0x7E7]` clear and the
> **FUN_00079D5C foul body are the numbered legs**."

It is called from `match_kick_run` (`match_handlers.c:1118-1124`) when
`out->receive != 0`, and `match_kick_run` is driven by the live kick rows
`fifa96_match_action_07` (`:1164`), `_0F` (`:1188`/`:1199`) and the corner taker
`:1568` — the phase-11 T1 live receive path (ENGINE.md `:1750-1751`). **But the
live receive arm does NOT call the offside check**: the `0x7A448` call is a
declared leg and is absent from `fifa96_match_run_ball_receive` (`match_run.c`
stops at the team-target writes, `:3846-3854`).

**The offside entry `fifa96_match_run_offside_reception` (`match_run.c:6544`)
has no caller.** Fresh `grep` of `src/`: only the definition (`match_run.c:6544`)
and the handler-registry doc-comment (`match_handlers.c:5626`). All other
matches are unit tests (`tests/test_engine_match_frame.c:4236,4272,4276,4279,
4283,4285,4287`). Its own header contract (`match_run.h:1479-1487`) states the
native caller is `FUN_0007A084 0x7A448` **(unported)** and that it stages the
gate as "**type != 0x11**, code not in {0x10,0x1D,0x1E}" — the L10 divergence,
documented in the engine's own comment.

### 2.4 The offside-enable `field_4c2f2` (fresh)

Fresh `get_xrefs_to 0x14C2F2` = **6**: writers `0x3755E`/`0x37587` in
`FUN_0003749c`; readers `0x4B069` (`FUN_0004b02c`, match-init), `0x79D87`
(the offside check), `0x8A73A` (the kind-3 arm of `FUN_0008a43c`), `0x74212`
(`FUN_0007412c`-caller band). Fresh `disassemble_bytes 0x37540`:

```
0x37554 MOV EAX,0x10 ; 0x37559 CALL 0x1d940 ; 0x3755e MOV [0x14C2F2],EAX  ; settings get(0x10)
0x37587 MOV dword[0x14C2F2],ECX   ; zeroed when [0x14C32A] != 0
```

So `field_4c2f2` has **one writer producer only** — the front-end settings
translator `FUN_0003749c` (get setting `0x10`; zeroed when the session gate is
set). **No in-match producer.** Under the FU-68 defaults setting `0x10 = 0`
(`fifa96_settings.c:23`), `fifa96_settings_handoff` binds
`config->field_4c2f2 = competition ? 0 : settings->value[0x10]`
(`fifa96_settings.c:86`) → default **0 (offside off)**. The engine's first
offside pre-gate is `if (mr->config.field_4c2f2 == 0) return 0;`
(`match_run.c:6570`).

### 2.5 Live-vs-leg ruling (Step 2)

**LIVE-LEG (double-dormant).** The native receive arm `FUN_0007A084` is reached
live (kick tail + animation-event machine) and calls the offside check at
`0x7A448`, but (a) the engine's *live* port of that arm
(`fifa96_match_run_ball_receive`) deliberately **drops** the `0x7A448` offside
call (declared leg, `match_run.c:3768`), and (b) the offside check+event is
bound in `fifa96_match_run_offside_reception`, which has **no engine caller**
(grep: definition + tests only). Additionally the enable `field_4c2f2` is
front-end-only and defaults 0. So the offside chain is not raised by any live
beat. **Payoff: Layer 1** (no visible offside outcome; the sink/speech are
staged). No fake producer may be invented — the honest landing is to port the
`0x7A448` call into the live receive arm behind `field_4c2f2`.

### 2.6 Numbered open legs (Step 2)

* **FU276-L2 — the live receive arm drops the `0x7A448` offside call.**
  `FUN_0007A084`'s receive body is ported live (`fifa96_match_run_ball_receive`,
  `match_run.c:3769`, driven by `match_kick_run`), but the `0x7A448 →
  FUN_00079D5C` offside call is a declared leg (`match_run.c:3768`) and
  `fifa96_match_run_offside_reception` (`match_run.c:6544`) has no caller.
  Owner: the `match_kick_run`/`fifa96_match_run_ball_receive` seam
  (`match_handlers.c:1118`, `match_run.c:3854`) + the `field_4c2f2` enable.
* **FU276-L3 — the offside enable `field_4c2f2` has no in-match producer.**
  Fresh: writers only `0x3755E`/`0x37587` in `FUN_0003749c` (front-end settings
  get `0x10`; zeroed on the session gate); default 0 (`fifa96_settings.c:23,86`).
  Owner: the front-end settings producer `FUN_0003749c` / a config path.
* **FU276-L7 — the `FUN_0006E8E8` (`0x6F56F`) receive-arm caller is unported.**
  The native arm is also reached from the per-record animation-event machine
  `FUN_0006E8E8`; the engine's `fifa96_match_run_ball_receive` is called only
  from `match_kick_run`. Owner: the `FUN_0006E8E8` event-machine seam (FU-153).
* **FU276-L8 — the arm's receive tail legs (carried OL-88/FU-142).** The
  `0x18E2`-class vector rebuild, the sound-id selection/sink, the `timer81`
  loop, the `[team+0x7E7]` clear (`0x7A3AC..0x7A3B9`) stay unported
  (`match_run.c:3767-3768`). Owner: the receive arm tail.

---

## Step 3 — the `FU268-L10` byte divergence (reconciliation)

### 3.1 The two native reads vs the engine read

Native (fresh, `FUN_00079D5C`, both sites read **`code` = `+0x91`**):

```
0x79EF7 MOV EAX,dword[ESI+0x8E] ; SAR EAX,0x18    ; = byte[ESI+0x91]
0x79F08 CMP EAX,0x11                              ; code != 0x11
0x79F00 MOVSX DX,byte[ESI+0x91]                   ; = the same code byte
0x79F0D MOVSX ECX,DX ; CMP ECX,0x10/0x1d/0x1e     ; code not in {0x10,0x1D,0x1E}
```

and again at the receive-arm gate (fresh `FUN_0007A084`): `0x7A419
MOV EAX,[EBP+0x8E]; SAR 0x18` + `0x7A422 MOVSX DX,byte[EBP+0x91]` — **both
`+0x91`**.

Engine (`match_run.c:6597`, fresh read):

```c
rec.eligible = (own->type != 0x11u && own->code != 0x10u &&
                own->code != 0x1Du && own->code != 0x1Eu) ? 1u : 0u;
```

Field map (fresh `fifa96_match_entities.h`): `code` = native `+0x91` (`:65`),
`type` = native `+0x8E` (`:76`, "the one face octant"). The `!= 0x11` disjunct
therefore reads the **wrong byte** — native reads `code` (`+0x91`), the engine
reads `type` (`+0x8E`). Because `type` is a 0–7 octant it can never be `0x11`,
so `own->type != 0x11u` is **always true** and the native `code != 0x11` test is
**silently dropped**.

### 3.2 Reconciliation (verdict)

**`FU268-L10` is confirmed, fresh.** Reconcile by changing the disjunct to read
the action code:

```c
own->code != 0x11u && own->code != 0x10u && own->code != 0x1Du && own->code != 0x1Eu
```

The engine's own header contract (`match_run.h:1487`) already mislabels the gate
as "type != 0x11", so that comment must be corrected in the same edit. This is a
**correctness fix ready to apply**; it does not change any live tape (the offside
chain is dormant), so it implies **no M1/M2 re-pin**. Owner:
`fifa96_match_run_offside_reception` (`match_run.c:6597`), with
`fifa96_match_entities.h:65/76` as the field authority.

---

## Step 4 — the port shapes + the live-trigger ruling (hand-off)

### 4.1 Port shapes (native → engine)

| native | engine (existing) | status |
|---|---|---|
| row-`0x0C` re-call `0x81E8A..0x81ED4` | `match_sp_action_0c_foul` → `fifa96_match_run_contact` (`match_handlers.c:3329`, `match_run.c:6489`) | ported; entry wired to row `0x0C` |
| row-`0x0B` arm-1 `0x81BCF`/`0x81BEB` | `fifa96_match_action_0B` (`match_handlers.c:3253/3258`) | ported |
| pad `0x7CE38` (want 0x60) | `match_input_handler` case `0x7CE38u` (`fifa96_match_run.c:693`) | ported & live |
| row-`0x04` duel `0x7ED61/0x7ED6A` | `fifa96_outfield.c:599` install `0x0B` | ported & live |
| `FUN_0008A3FC` | `fifa96_ref_contact_register` | ported |
| `FUN_0008A43C` normal / kind-3 | `fifa96_ref_foul_decide` / `fifa96_ref_offside_event` | ported |
| `FUN_00079D5C` | `fifa96_ref_offside_check` | ported; entry `fifa96_match_run_offside_reception` |
| `FUN_0007A084` receive body | `fifa96_match_run_ball_receive` (`match_run.c:3769`) | ported & live (kick path) |
| `FUN_0007A084` `0x7A448` offside call | — (declared leg, `match_run.c:3768`) | **LEG (FU276-L2)** |
| `FUN_0007A084` receive tail | — (OL-88 legs) | **LEG (FU276-L8)** |
| `FUN_0006E8E8` `0x6F56F` receive call | — (caller-side unported) | **LEG (FU276-L7)** |
| `[0x14C306]` | `config.field_4c306` (`match_run.c:6503`) | ported; **front-end producer only** (FU276-L6) |
| `[0x14C2F2]` | `config.field_4c2f2` (`match_run.c:6570`) | ported; **front-end producer only, default 0** (FU276-L3) |
| `0x80EE7` registrar caller | — | **LEG (FU276-L5)** |

### 4.2 Live-trigger ruling (the wave's core question)

* **Contact gate:** **LIVE-LEG (FU276-L1).** Producer-wired live (pad `0x7CE38`
  / row-`0x04` duel → row-`0x0B` → row-`0x0C` → `fifa96_match_run_contact`);
  **unraised in the smoke** (1-in-64 duel RNG + the `0x60` button). No fake
  producer.
* **Offside:** **LIVE-LEG (FU276-L2/L3).** The *live* receive arm drops the
  `0x7A448` offside call; the offside entry `fifa96_match_run_offside_reception`
  is callerless; the enable `field_4c2f2` is front-end-only, default 0.
* **`FU268-L10`:** **confirmed** — fix `own->type != 0x11u` →
  `own->code != 0x11u` at `match_run.c:6597` (+ the `match_run.h:1487` comment);
  no live tape, **no re-pin**.

### 4.3 Hand-off

The contact cluster's realistic Layer-1 landing is **FU276-L1**: raise the
contact gate through the existing row-`0x04` AI duel install (or the pad `0x7CE38`
handler) with a **headless granted-frame proof** — the row-`0x0B` → row-`0x0C` →
`fifa96_match_run_contact` chain already exists and is unit-proven. The offside
chain (FU276-L2/L3) needs the `0x7A448` call ported **into the live receive arm**
(`fifa96_match_run_ball_receive`) behind a raised `field_4c2f2`, plus an offside
enable producer; wiring `fifa96_match_run_offside_reception` alone would remain
callerless. The L10 fix (FU276-L4) is a ready correctness edit. No tape re-pin is
implied for any producer (the tape performs no tackle and no offside).

---

## Numbered open legs (consolidated)

| leg | statement | native owner |
|---|---|---|
| **FU276-L1** | the live contact invoker (pad `0x7CE38` want 0x60 / row-`0x04` duel `0x7ED6A`) is wired but not raised by any test/tape/smoke | `fifa96_outfield.c:599` / `fifa96_match_run.c:699` / `fifa96_match_run.c:693` |
| **FU276-L2** | the live receive arm `fifa96_match_run_ball_receive` drops the `0x7A448 → FUN_00079D5C` offside call; `fifa96_match_run_offside_reception` has no caller | the receive-arm offside call `FUN_0007A084 0x7A448` (`match_run.c:3768/6544`) |
| **FU276-L3** | the offside enable `field_4c2f2` has no in-match producer (front-end `FUN_0003749c` only; default 0) | `FUN_0003749c` (`0x3755E`/`0x37587`) |
| **FU276-L4** | the L10 byte divergence: the engine's `!= 0x11` disjunct reads `type` (`+0x8E`) where native reads `code` (`+0x91`) | `match_run.c:6597` (+ `match_run.h:1487`) |
| **FU276-L5** | the `0x80EE7` severity-RNG registrar caller's body is unnamed; no engine port | the `0x80Exx` foul body (FU-178/FU-190) (= FU268-L3) |
| **FU276-L6** | `field_4c306` has no in-match producer (front-end `FUN_0003749c`; ON by default) | `FUN_0003749c` (`0x3754F`/`0x37581`) |
| **FU276-L7** | the `FUN_0006E8E8` (`0x6F56F`) receive-arm caller is unported | the `FUN_0006E8E8` event-machine seam (FU-153) |
| **FU276-L8** | the `FUN_0007A084` receive tail (vector rebuild / sound sink / timer81 / `[team+0x7E7]`) stays unported | the receive-arm tail (`match_run.c:3767-3768`, OL-88) |

---

## Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `.image` overlay.
* `get_xrefs_to`: `0x7A448` (0), `0x79D5C` (1), `0x7A084` (3), `0x7A490` (9),
  `0x8A43C` (2), `0x8A3FC` (2), `0x7CE38` (1, DATA `0x110824`), `0x14C2F2` (6),
  `0x14C306` (5).
* `get_function_by_address`: `0x7A084` (body `0x7A084..0x7A456`), `0x7A490`
  (body `0x7A490..0x7AE37`), `0x6E8E8` (body `0x6E8E8..0x6FE83`).
* `disassemble_function`: `0x79D5C` (138 insns).
* `disassemble_bytes` (start, length B): `0x7A410` (96), `0x7A3C0` (96),
  `0x7A084` (96), `0x7A490` (80), `0x7A860` (120), `0x6F540` (64), `0x81E80`
  (96), `0x81BA0` (96), `0x7CDF0` (112), `0x7CE60` (160), `0x7ED20` (96),
  `0x80ED0` (64), `0x37540` (96), `0x74200` (48), `0x4B040` (64).
* Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
  (`:1380-1419` the outfield machine dispatch, `:3762-3855`
  `fifa96_match_run_ball_receive`, `:6489-6685` contact / offside_reception /
  referee_step), `src/fifa96_engine/fifa96_match_handlers.c` (`:620-740` the
  input-row dispatch incl. `0x7CE38`, `:1039-1126` `match_kick_run` + the
  receive-arm call, `:3240-3414` row-`0x0B` arm-1 / row-`0x0C` body,
  `:5463-5479` the registry rows 04/0B/0C, `:5620/5626` the phase-19/1C
  comments), `src/fifa96_loader/fifa96_outfield.c` (`:560-624` the row-04 AI
  duel install), `src/fifa96_loader/fifa96_settings.c` (`:14-30`, `:72-100`),
  `include/fifa96_engine/fifa96_match_entities.h` (`:61-92` `code`/`type`),
  `include/fifa96_engine/fifa96_match_run.h` (`:1441-1519` the FU-150 P2
  contract incl. `:1487`).
* Tests cross-read: `tests/test_engine_match_frame.c` (`:4220-4295` the offside
  chain, test-only callers).
* Docs cross-cited (not rewritten): `docs/ENGINE.md` (`:1754` the fouls/offside
  known-gaps row), `docs/ghidra/FU268_fouls_offside_extratime.md` (L1/L2/L10),
  `FU271_live_trigger_census.md` (§1.4), `FU150_fouls_referee_offside.md`,
  `FU169_row5e_live_installer.md`.

No writes beyond this draft; no rename/label/comment/function/script/project
save; `/FIFA96.EXE` untouched; no git command.
