# FU-63: the presentation/event pump — `FUN_00091DD8` and the `0x5B380` queue

Roadmap slice S6 of FU-60 §6 ("presentation event queue"). Derives the pump
called at the end of each drained match frame, its three-state/6-slot queue,
the enqueue/dequeue plumbing and the dispatch actions, and ports the queue
state machine as `fifa96_event_queue`. `FUN_00091DD8` decompilation had died
in FU-60; this slice reads the full disassembly and resolves the queue.

Result in one line: **`FUN_00091DD8` (sole caller `0x49FCA`, after
`FUN_0004B100`/`FUN_00036208`) first gates on `FUN_00037AE4()==0`, starts
music `FUN_000652B8`, runs the FU-49 ambience machine `FUN_00091F60` only when
`[0x4C32A]==0`, and then, if `[0x4C312]!=0`, re-arms a quantised phase-2 event
trigger (`[0x57AB6]/5 + 0x2D` slots of `0x1C` past `[0x5B35C]`) before running
a 3-state machine at `[0x5B330]`: state 0 (flush `[0x5B36C]` → clear the
6-slot queue, enqueue `0x43`, drain command ring `0x40`, dequeue one record;
one-shot `[0x5B374]` → command ring `0x20`; otherwise `FUN_00066E70()==0` and
the `0xDA` timer expired → dequeue one record, or schedule a delay of
`rand()%60 + 180` and enter state 1), state 1 (any of `[0x5B370]/[0x5B36C]/
[0x5B374]` → command ring 8 plus dequeue; else after the delay re-arm
`+0xB4` and enter state 2), state 2 (past the delay, fire one of the four
input one-shots for codes 1/1/2/5, drain command ring if pending, dequeue one
record), then the tail clears the four flags. The queue is the 6×`0x1C` array
at `0x5B380` (`+0` id, `+0xC` `[0x5B368]` value, `+0x14` command-ring entry
pointer, `+0x18` param) fed by `FUN_0008F0C4` and consumed one record per
pump by `FUN_00092548`, which applies a 600-tick per-id cooldown, a `0x21`
team gate, and dispatches by id to the audio/commentary calls
`FUN_00066840`/`FUN_000668CC`/`FUN_00066724`/`FUN_0006690C`/`FUN_000669C0`,
sets `[0x5B330]=0` on success and records the event param in the
`0x5B33C/0x5B340/0x5B344` triple.** The `0x5B33C` getter `FUN_00092194` is
consumed by the entity tracker `FUN_00072AC4` (postings `0xD8`/code `0x21`).

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58/FU-59/FU-60/FU-61/
  FU-62). All instructions quoted below were read back from Ghidra this slice
  (`disassemble_function`/`disassemble_bytes`/`read_memory`).
* **`FUN_00091DD8` disassembly (FU-60 closure):** FU-60 reported the
  decompiler process died on `0x91DD8`. This slice re-ran
  `disassemble_function 0x91DD8`: 498 instructions, `0x91DD8..0x924E7`,
  decoded cleanly. The only pruned window is the state-0 scheduling arm
  `0x92330..0x9235D`, which falls through into the state-1 code at `0x9235E`
  (the listing jumps from `0x92330 CALL 0xCBC4C` straight to `0x9235E`). It
  was decoded from raw bytes (`read_memory 0x92330 48`, quoted in §3.2).
  Another listing quirk: two consecutive `JZ` at `0x9221B`/`0x9221D`
  (`74 68`/`74 0A`); the second can never be taken because `0x92219 TEST
  EBX,EBX` already proved `EBX != 0`. Behaviour is unaffected.
* **Gap helpers:** `get_function_by_address 0x924E8` returns "No function
  found"; the bytes there are an uncalled duplicate of the drain block, and
  `0x9252C` an uncalled `FUN_00066724` wrapper (0 xrefs to `0x924E8`). The
  `0x8F32F/0x8F34C/0x8F37C` gap after `FUN_0008F2C4` holds three uncalled
  enqueue wrappers (0 xrefs to `0x8F32F`), quoted in §4.5.
* **Decompiler artifact corrected:** `decompile_function 0x92548` prints the
  triple store as `uRam0005b33c = 0xf9e80c24; uRam0005b340 = 0xfffe37e9`.
  The disassembly (`0x926AD..0x926B9`, `0x92780..0x92798`) shows the actual
  instructions are `[0x5B33C] = param; [0x5B340]++` (see §5.4, errata 1).
* All numeric claims (queued offsets, thresholds, deadlines) are quoted from
  the listings; semantic labels beyond what the instructions do are not
  asserted.

## 1. `FUN_00091DD8` — identity, order and gates

Sole caller: `0x49FCA` inside `FUN_00049B28`, between the `0x200`-scaled
motion step (`0x49FC0 CALL 0x36208`) and the post-frame housekeeping
(`get_xrefs_to 0x91DD8`; FU-60 §3). The FU-60 ordering claim is confirmed by
the raw-byte tail read in FU-62 §2 (`0x49FCA CALL 0x91DD8`).

Head (`0x91DD8..0x91DFB`):

```
0x91DD8  CALL 0x37AE4
0x91DDD  TEST EAX,EAX
0x91DDF  JNZ 0x91DD5              ; menu/suspend gate -> return
0x91DE1  CALL 0x652B8             ; music start (FU-52 caller)
0x91DE6  CMP byte [0x4C32A],0
0x91DED  JNZ 0x91DF4
0x91DEF  CALL 0x91F60             ; FU-49 ambience machine
0x91DF4  CMP dword [0x4C312],0
0x91DFB  JZ 0x91DD5               ; presentation body disabled -> return
```

* The FU-60 §4.2 order is exact: `FUN_00037AE4` gate → `FUN_000652B8` →
  `FUN_00091F60` iff `[0x4C32A]==0` → `[0x4C312]` gate. `[0x4C32A]` is the
  same flag that gates `FUN_0008F188` (§4.4) and selects the `FUN_00049B28`
  event chains (FU-62 §2).
* `[0x4C312]` still has **no static writer** (`get_xrefs_to 0x4C312` → reads
  `0x8F19F`, `0x91DF4`), so it is a runtime/indirect "presentation active"
  dword: open leg. The port exposes it as the `enabled` tick argument.
* The body between the gate and the state switch is the scheduling block
  (§2). The body after the switch is the state machine plus a common tail
  that clears `[0x5B374]`, `[0x5B36C]`, `[0x5B370]`, `[0x5AAD8]`
  (`0x924C7..0x924E1`); every post-gate path reaches it.

## 2. The phase-2 scheduling block (`0x91E00..0x921EC`)

```
0x91E06  XOR EDX,EDX / MOV DX,[0x57AB6]   ; period seconds (word)
0x91E0F  MOV EBX,5 / IDIV EBX             ; [0x57AB6] / 5
0x91E1B  LEA EDX,[EAX + 0x2D]             ; slot
0x91E1E  MOV EAX,EDX / SHL EAX,4 / SUB EAX,EDX / SHL EAX,2
0x91E25  MOV ESI,[0x5B35C] / ADD ESI,EAX  ; deadline = [0x5B35C] + slot*0x1C
0x91E30  CALL 0xCB2A4                     ; now
0x91E35  CMP ESI,EAX / JGE 0x921ED        ; deadline >= now -> skip block
0x91E3D  MOV EAX,[0x57A4A] / SAR EAX,0x18 / CMP EAX,2 / JNZ 0x921ED
0x91E4E  MOV EDX,[0x57754] / TEST / JL -> NEG
0x921BB  CMP EAX,0x2D0 / JGE 0x921ED
0x921C2  CALL 0x8EF38                     ; code 0..0xB (FU-62/FU-60)
0x921CC  CWDE / CMP EAX,5 / JG 0x921E3
0x921D2  MOV EBX,4 / MOV EAX,0x8D / XOR EDX,EDX / CALL 0x8F188
0x921E3  CALL 0xCB2A4                     ; now
0x921E8  MOV [0x5B35C],EAX                ; re-arm last-run stamp
```

* The deadline is quantised on the period-seconds counter: slot index
  `period_seconds/5 + 0x2D` (45) times `0x1C` (28) after the last-run stamp.
* The re-arm stamp at `0x921E8` runs when the phase is 2 and `|[0x57754]| <
  0x2D0`, whether or not the enqueue fires (code `<= 0` or `> 5` jumps to the
  stamp). When the phase is not 2, or the distance is out of range, or the
  deadline has not passed, the stamp is **not** updated.
* `FUN_0008EF38` (decompile) returns `0..0xB` from `[0x57AC2]`, `[0x57AB6]`,
  `[0x57ABA]`, `[0x5881A]`; only `1..5` enqueues. The enqueue call is
  `FUN_0008F188(id=0x8D, param=0, code=4)` — the command-ring enqueue, not
  the `0x5B380` record queue (§4.4). Code 4 sets none of the three state
  flags; the ring entry is later drained by state 2's `FUN_0008F2C4(0)` or
  by a lower threshold.

## 3. The 3-state machine (`0x921ED..0x924E7`)

Dispatch: `0x921ED MOV EAX,[0x5B330]`; case 0 (`<1`) at `0x9220B`, case 1 at
`0x9235E`, case 2 at `0x923EF`, default `0x92206 JMP 0x924C7` (tail).

### 3.1 State 0 (`0x9220B..0x9235D`)

```
0x9220B  TEST EAX,EAX / JNZ tail
0x92213  MOV EBX,[0x5B36C] / TEST / JZ 0x92285
0x9221F  MOV [0x5B334],EAX / MOV [0x5B338],EAX   ; clear queue
0x92229  MOV EAX,0x43 / XOR EDX,EDX / CALL 0x8F0C4
0x92235  MOV EAX,0x40 / CALL 0x8F2C4            ; drain ring threshold 0x40
0x9223F..0x92268  count > index -> index++, CALL 0x92548(&slot[index])
0x92272..0x92280  else count = index = 0
0x92285  CMP [0x5B374],0 / JZ 0x9229D
0x9228E  MOV EAX,0x20 / CALL 0x8F2C4   / JMP tail ; one-shot arm
0x9229D  CALL 0x66E70 / TEST / JNZ tail
0x922AA  CMP [0x5B348],0 / JZ inactive
0x922B3  CALL now / CMP EAX,[0x5B34C] / SETLE / JNZ ...
0x922C8  inactive: EAX=1 ; expired = (timer == 0) || (now > [0x5B34C])
0x922D9  XOR EBP,EBP
0x922DB  MOV EDX,[0x5B334]
0x922E1  MOV [0x5B428],EBP             ; current record pointer = 0
0x922E7  MOV [0x5B348],EBP             ; timer flag = 0
0x922ED  TEST EDX,EDX / JZ 0x92330
0x922F1..0x9231A  count > index -> drain one
0x9231F..0x9232B  else count = index = 0
0x92330  scheduling arm (raw bytes below) -> state = 1, falls into state 1
```

So state 0 is a priority chain: **flush flag** (clear both indices, enqueue
`0x43`/param 0, command-ring threshold `0x40`, then exactly one dequeue) →
**one-shot flag** (command-ring threshold `0x20`, no dequeue) →
**`FUN_00066E70()==0`** and *timer inactive or expired* (clear the current
record pointer and the timer; dequeue one record if any; otherwise schedule,
state 1). If `FUN_00066E70()` is nonzero or the timer is still ahead, the
tick does nothing but the tail clear.

`FUN_00066E70` decompiles as `return [0x14A98] != 0` (the audio/dialog "busy"
flag; the same test guards `FUN_00092548`'s `FUN_00066DFC` call).

Raw bytes at `0x92330..0x9235D` (`read_memory 0x92330 48`):

```
0x92330  CALL 0xCBC4C              ; rand(), EAX
0x92335  MOV EBX,0x3C / XOR EDX,EDX / DIV EBX
0x9233E  MOV ESI,EDX               ; rand() % 60
0x92340  CALL 0xCB2A4              ; now
0x92345  ADD EAX,ESI
0x92347  MOV EBX,[0x5B330]
0x9234D  ADD EAX,0xB4              ; + 180
0x92353  MOV [0x5B364],EAX / INC EBX / MOV [0x5B330],EBX
```

so the schedule is `[0x5B364] = now + rand()%0x3C + 0xB4; [0x5B330] = 1`,
whose state-1 fallthrough then tests `[0x5B370]/[0x5B36C]/[0x5B374]` (the
first can be set) and `now > [0x5B364]` (just set, false).

### 3.2 State 1 (`0x9235E..0x923EE`)

```
0x9235E  if ([0x5B370] || [0x5B36C] || [0x5B374]) {
0x92379     FUN_0008F2C4(8)
0x92383     count > index -> index++, FUN_00092548(&slot[index])
0x923B6     else count = index = 0
0x923C4     JMP tail
         }
0x923C9  CALL now / CMP EAX,[0x5B364] / JLE tail
0x923DA  CALL now / ADD EAX,0xB4 / MOV [0x5B364],EAX
0x923E9  INC [0x5B330]              ; -> 2, falls into state 2
```

### 3.3 State 2 (`0x923EF..0x924C6`)

```
0x923EF  CALL now / CMP EAX,[0x5B364] / JLE 0x92474
0x92400  [0x5AFAE]==0 && FUN_0008EF38()==1 -> FUN_00091528(); [0x5AFAE]=1
0x92423  [0x5AFAF]==0 && FUN_0008EF38()==1 -> [0x5AFAF]=1
0x92440  [0x5AFAC]==0 && FUN_0008EF38()==2 -> FUN_00091630()
0x9245B  [0x5AFAD]==0 && FUN_0008EF38()==5 -> FUN_00091774()
0x92474  if ([0x5AAD8] != 0) FUN_0008F2C4(0)
0x92484  count > index -> index++, FUN_00092548(&slot[index])
0x924B9  else count = index = 0
```

The four input one-shots form a fall-through chain (at most one per tick).
Codes 1 and 1 set their flags (`0x5AFAE` after the action call, `0x5AFAF`
before); codes 2 and 5 call the action without setting `0x5AFAC`/`0x5AFAD`
(those two flags are written elsewhere or self-set — open leg). The
dequeue runs whether or not the deadline passed; only the input arm is
deadline-gated. `[0x5B364]` is not advanced in state 2, so once the delay
expires the input arm and the command-ring check run on every subsequent
pump tick until a dequeue resets the machine (or the flags change).

### 3.4 Tail (`0x924C7..0x924E1`)

`[0x5B374] = [0x5B36C] = [0x5B370] = 0` and `[0x5AAD8] = 0`, then the
register pops and `RET`. Every post-gate path ends here.

## 4. Queue mechanics

### 4.1 Layout of `0x5B380`

6 slots, stride `0x1C`, index arithmetic `idx*8 - idx` (`*7`) then `*4`:

| offset | address | width | writer | reader |
|---|---|---|---|---|
| `+0` | `0x5B380 + i*0x1C` | id | `FUN_0008F0C4 0x8F0DE` (`MOV [EAX*4+0x5B380],EBX`) | `FUN_00092548 0x92550` (`MOV EDX,[EAX]`) |
| `+0xC` | `+0x5B38C` | value from `[0x5B368]` | `FUN_0008F0C4 0x8F135` | `FUN_00092548 0x92649`, `0x9268C`, `0x9275F` (`in_EAX[3]`) |
| `+0x14` | `+0x5B394` | command-ring entry pointer | `FUN_0008F0C4 0x8F119` | `FUN_00092548 0x92574` (`in_EAX[5]`, reads `+0xC`, writes `+8=1` at `0x927A0`) |
| `+0x18` | `+0x5B398` | param | `FUN_0008F0C4 0x8F0F4` | `FUN_00092548 0x92582/0x92634` (`in_EAX[6]`) |

`+4`, `+8`, `+0x10` are never written by the enqueue; the dispatcher never
reads them.

### 4.2 Enqueue `FUN_0008F0C4` (`0x8F0C4..0x8F144`)

```
0x8F0C6  EBX = EAX (id), ECX = EDX (param)
0x8F0CA  EDX = [0x5B334] (count)
0x8F0D0  CMP EDX,6 / JGE return        ; full: silently dropped (void)
0x8F0DE  queue[count].+0  = EBX
0x8F0F4  queue[count].+0x18 = ECX
0x8F101  EBX = [0x5AADC]*0x20 + 0x5AAE0  ; current command-ring entry
0x8F119  queue[count].+0x14 = EBX
0x8F12F  queue[count].+0xC  = [0x5B368]
0x8F13C  [0x5B334]++
```

`FUN_0008F0C4` has 217 call sites (the whole `0x8Fxxx..0x9xxxx` match-event
module); the pump's flush arm is the only caller inside `FUN_00091DD8`
(`0x92230`). `[0x5B368]` is zero at every live site (init `0x91D2E`; only
the uncalled gap helpers below write it), so the `+0xC` value is 0 in
practice.

### 4.3 Reset `FUN_0008F178`

`0x8F17B [0x5B334]=0; 0x8F181 CALL 0x66DFC`. Callers (`get_xrefs_to`):
`FUN_0001B070` (`0x1B0B2`), `FUN_00051270` (`0x51273`), `FUN_00063734`
(`0x6386D`), unnamed `0x5137C`. Note it clears the **count only**; the
current index `[0x5B338]` is left as-is until the next drain resets it.

### 4.4 Signal/enqueue `FUN_0008F188` and command drain `FUN_0008F2C4`

`FUN_0008F188(EAX=id, EDX=param, EBX=code)` is the **command-ring** enqueue
(not the `0x5B380` queue): gates `[0x4C32A]==0 && [0x4C312]!=0`
(`0x8F192/0x8F19F`), maps the code to the three state flags
(`0x8F1AC..0x8F1E4`): `code==8` → `[0x5B370]=1`; `code in {0x20,0x21}` →
`[0x5B374]=1`; `code==0x40` → `[0x5B36C]=1`; other codes set none. It then
inserts a `0x20`-byte entry at `([0x5AADC]+9) % 10` in `0x5AAE0`: `+0` id,
`+4` now, `+8` 0, `+0xC` code, `+0x10/+0x14/+0x18` = `param+0x59` when param
is nonzero, `+0x1C` param; `[0x5AAD8]++`; `[0x5AADC]` = new index
(`0x8F1E6..0x8F2B9`). ~107 call sites; the pump calls it at `0x921DE`
(`0x8D`/code 4) and `FUN_00072AC4` calls it for `0xD8`/`0xD9`/`0x21` (§6).

`FUN_0008F2C4(threshold)` is called **only** from `FUN_00091DD8` (thresholds
`0x40`, `0x20`, `8`, `0`) — `get_xrefs_to 0x8F2C4` returns exactly the four
pump sites. It walks `[0x5AAD8]` down to `-1`, and for each entry
`(cursor+count)%10` with `id < 0xDD` and `threshold <= entry code` and a
nonzero callback at `0x10BB4[id*4]`, calls the callback
(`0x8F2D2..0x8F325`). The post-drain count is `-1` and the
`[0x5AAD8] != 0` guard in state 2 (`0x92474`) is therefore true after any
drain; the count encoding is not fully decoded — the port models a boolean
"command pending" instead (open leg).

### 4.5 Uncalled gap helpers (evidence)

* `0x924E8..0x9252B` (no function, 0 xrefs): a verbatim duplicate of the
  drain block — `if (count > index) { index++; FUN_00092548(&slot[index-1]); }
  else count = index = 0` (`0x924EA..0x92523`).
* `0x9252C..0x92544` (0 xrefs): `if (FUN_00066E70()==0) FUN_00066724(EAX, 0)`.
* `0x8F32F/0x8F34C/0x8F37C` (0 xrefs): three enqueue wrappers around
  `FUN_0008F0C4` with `EDX = ring[cursor].+0x1C`; `0x8F34C` brackets an
  `id=0xDC` enqueue with `[0x5B368]=1` then `[0x5B368]=0`, so the `+0xC`
  record value is 1 for that event.
* `FUN_0009219C` (`0x9219C..0x921B6`, 0 xrefs):
  `[0x5B334]==0 && FUN_00066E70()==0` boolean.

FU-60's `FUN_00092194` "zero callers" was refined: see §6.

## 5. Dispatch `FUN_00092548` (`0x92548..0x927D4`)

Called only from the four drain blocks with `EAX = &queue[index]`.

### 5.1 Gates

```
0x92550  EDX = record[+0]
0x92552  CMP EDX,0xDC / JZ 0x92574       ; id 0xDC skips cooldown
0x9255A  EDI = [0x5AC38 + id*4] + 0x258  ; 600-tick cooldown
0x92567  CALL now / CMP EDI,EAX / JG return
0x92574  EAX = record[+0x14]             ; command-ring entry
0x92577  CMP [EAX+0xC],0x21 / JNZ 0x9258B
0x9257D  EAX = [0x57A83]; CMP record[+0x18] / JNZ return  ; team gate
```

The index has already been advanced by the caller, so a gated record is
consumed (skipped) and the pump state is left unchanged (no tail).

### 5.2 Record pointer and busy hook

`0x9258B [0x5B428] = record`; `0x92591 CALL 0x66E70`; if nonzero
`CALL 0x66DFC` (closes the audio/dialog prompt).

### 5.3 Per-id dispatch

| id | site | action |
|---|---|---|
| `0xDA` | `0x925A9..0x925C0` | `[0x5B348]=1`; `[0x5B34C]=now+0x1E`; **return** (no tail, state unchanged) |
| `0xC6`/`0x8D` | `0x925D5..0x925F8` | `[0x5B35C]=now`; `FUN_00066840(id, [0x57AC7], [0x57AC5])`; tail |
| `0x79`/`0x7A` | `0x92607..0x9262F` | `FUN_000668CC(id, ([0x5881A]-[0x57AB6])/0x3B)`; tail |
| value `==0`, id `< 0xD8` | `0x92647` | `FUN_00066724(id, record[+0xC])`; tail |
| value `==0`, id `>= 0xD8` | `0x92641` | tail only |
| value `!=0`, id `0xDB` | `0x92674` | tail only |
| value `!=0`, id `0xDC` | `0x92682..0x9269D` | `FUN_000741B4(*(record[+0x18])[+0x826 byte])`, `FUN_00011E0C(*(param+4), result, record[+0xC])`, `FUN_0006690C(result, ret)`; then the triple check |
| value `!=0`, other | `0x926C3..0x9276B` | RNG calls when `*(param+4)` low word is `0x18DA`/`0x18DE`; `FUN_000741B4(...)`, `FUN_00011E0C(...)`, `FUN_000669C0(...)`; triple check when `FUN_000669C0 != 0` |

### 5.4 Success tail and the `0x5B33C` triple

```
0x9279D  record[+0x14].+0x8 = 1      ; mark the command-ring entry consumed
0x927A7  id = record[+0]
0x927B3  [0x5B330] = 0               ; any successful dispatch restarts at state 0
0x927B9  [0x5AFB8 + id*4]++          ; per-id dispatch counter
0x927C0  [0x5AC38 + id*4] = now      ; per-id cooldown stamp
```

Triple update (both `0xDC` and the generic `value!=0` arm, after the
dialogue call):

```
0x9269D/0x92774  if ([0x5B37C] == record[+0x18]) {
0x926AD/0x92780      [0x5B340]++
0x926B3/0x92787      [0x5B33C] = record[+0x18]
0x92793/0x92798      [0x5B344] = now
                 }
```

`[0x5B37C]` is initialised to `[0x57A83]` (the tracked entity) by
`FUN_00091BC4 0x91CD7`; `[0x5B33C]` is exposed by the getter `FUN_00092194`
(`MOV EAX,[0x5B33C]; RET`, `0x92194..0x92199`) and `[0x5B340]/[0x5B344]`
have no other readers (open leg). Initialisation: `FUN_00091BC4` zeroes
`[0x5B330]`, `[0x5B334]`, `[0x5B33C]`, `[0x5B340]`, `[0x5B36C]`,
`[0x5B370]`, `[0x5B374]`, `[0x5B368]`, `[0x5B428]`, sets
`[0x5B37C]=[0x57A83]`, `[0x5B364]=now+0xB4` (`0x91D0E`),
`[0x5B35C]=now+0x708` (`0x91D29`), and fills the command ring
(`0x91BC4..0x91D95`; FU-49 §1.10).

## 6. `FUN_00092194` consumer — the triple is read by the entity tracker

`get_xrefs_to 0x92194` returns one call, inside `FUN_00072AC4` at `0x73665`
(FU-62 §5's entity tracker). That address is reached by the conditional
jumps at `0x73631`/`0x7363D` (the `0x73622 RET` is the other arm's tail):

```
0x73623  EAX = [0x57A83]; EAX = [EAX]; CMP byte [EAX+0x826],0
0x73631  JNZ 0x73665
0x73633  CMP [0x57754],0x660 / JLE 0x73665
0x7363F  EAX=8 / EBX=0x21 / FUN_000651F0 / EAX=0xD9 / EDX=[0x57A83] / FUN_0008F188
0x73664  RET
0x73665  CALL 0x92194                 ; EAX = [0x5B33C]
0x7366A  ECX = [0x57A83]
0x73670  CMP EAX,ECX
0x73672  JZ 0x736A3                   ; same entity already had an event -> done
0x73674  EBX=0x21 / EAX=0xD8 / EDX=ECX / CALL 0x8F188
```

So `[0x5B33C]` is the entity param of the last dispatched presentation
event: when the tracker switches to an entity that has not had one, it posts
`id=0xD8` with `code=0x21`, which sets `[0x5B374]=1` and makes the next pump
state 0 emit command-ring threshold `0x20` (§4.4). FU-60 §9's "zero callers
/ role undetermined" is refined accordingly; the triple's count/time fields
remain unread.

## 7. `[0x4E574]` writers (task 3)

`FUN_00051AB8` is the getter (`return [0x4E574]`); `FUN_00049B28` skips the
`FUN_0004C394`/`FUN_0004B100`/`FUN_00036208`/`FUN_00091DD8` chain when it is
nonzero (`0x49FA4..0x49FB1`, FU-60 §3). 27 xrefs total; the writers with a
defined function:

| writer | value | adjacent calls (decompile) |
|---|---|---|
| `FUN_00053DC4 0x53DD8` | 1, only if it was 0 | `FUN_00064074()` first |
| `FUN_00053DE0 0x53DF1` | 0, only if it was nonzero | `FUN_00064080()`, then `FUN_00045D0D()` |
| `FUN_000537F8 0x5381A`/`0x53888` | clears the `0x4E570..` block first; sets 1 only when `FUN_0004B380()` (phase) `== 0x10` | match-screen reset |
| `FUN_00052030 0x520C8` | 0 on menu transitions | `FUN_00064080()` + `FUN_00045D0D()` |
| `FUN_00052D68 0x52DCA` | writes/reads in a menu-state pair | — |

Unnamed write sites: `0x51435`, `0x524D0`, `0x5296F`, `0x52BC7`, `0x52C3E`,
`0x52E12`. Evidenced role: a frame-suspend/pause flag (set when entering the
pause/dialog path, cleared on exit, with `FUN_00064074`/`FUN_00064080` as
the paired enter/leave calls); no further label is asserted.

## 8. Port: `fifa96_event_queue`

`include/fifa96_loader/fifa96_event_queue.h` +
`src/fifa96_loader/fifa96_event_queue.c` (caller-owned struct, no globals,
no comments, `-fifa96_err_t` for invalid arguments). Scope: the six-slot
record queue plus the three-state machine, its flags/timers, the input
one-shot chain and the schedule trigger.

| original | port |
|---|---|
| `[0x4C312]` body gate (`0x91DF4`) | `fifa96_event_queue_tick(q, enabled)` |
| `[0x5B380 + i*0x1C]` record, count `[0x5B334]`, index `[0x5B338]` | `slot[6]`, `count`, `index` |
| `FUN_0008F0C4` (silently drops when full) | `fifa96_event_queue_enqueue` (returns `-FIFA96_ERR_FULL`) |
| `FUN_0008F178` `[0x5B334]=0` | `fifa96_event_queue_reset` (clears count only, as the original) |
| `FUN_0008F188` flag mapping + ring insert, gated on `[0x4C32A]`/`[0x4C312]` | `fifa96_event_queue_signal` (sets `flush`/`ring_flag`/`one_shot`, `ring_pending`, calls `backend->post`); the two gates stay with the caller |
| state 0 flush arm (`0x92213..0x92280`) | `flush` field; `FIFA96_EVENT_QUEUE_FLUSH_ID`, command threshold `0x40` |
| state 0 one-shot arm (`0x92285`) | `one_shot`; command threshold `0x20` |
| state 0 schedule arm (`0x92330`, raw bytes) | `next_time = now + rand()%0x3C + 0xB4; state = WAIT`; fallthrough re-runs state 1 |
| state 0 timer gate (`0x922AA..0x922E7`) | `timer`/`timer_deadline`; `expired = timer == 0 \|\| now > deadline` |
| state 1 flags/timer (`0x9235E..0x923E9`) | command threshold `8`; `next_time = now + 0xB4; state = ACTIVE` |
| state 2 one-shots (`0x92400..0x9246F`) | `input_shot[4]`, `backend->input`, `backend->input_action` |
| state 2 `[0x5AAD8]!=0` (`0x92474`) | `ring_pending`; command threshold `0` |
| drain block (4 inline copies) | `queue_drain`; `backend->dispatch` return != 0 models the `FUN_00092548` success tail (`state = IDLE`), 0 models its early returns |
| schedule pre-block (`0x91E00..0x921EC`) | `fifa96_event_queue_schedule` (deadline `last_run + (period_seconds/5 + 0x2D)*0x1C`, phase 2, `|distance| < 0x2D0`, code `1..5` → `signal(0x8D, 0, 4)`, then `last_run = now`) |
| tail clears (`0x924C7`) | tick tail clears the three flags and `ring_pending` |
| `FUN_00092548` gates/dispatch | `backend->dispatch` only; cooldown, team gate, per-id actions not ported |
| `FUN_000652B8`, `FUN_00091F60`, `FUN_00037AE4` | not ported (FU-52/FU-49/other) |

Not ported (open legs): the command-ring buffer and callback table
`0x10BB4`; the per-id cooldown/counter arrays `0x5AC38`/`0x5AFB8`; the `0x21`
team gate; the `FUN_00092548` dialog/audio callees; the `0x5B33C` triple;
the uncalled gap helpers.

Tests (`tests/test_event_queue.c`, suite 51 → **52**): init zeroing and
`now+0xB4`/`now+0x708` timers; enqueue to the 6-slot boundary and
`-FIFA96_ERR_FULL`; reset clearing count only; `enabled == 0` making no
callback calls and preserving flags; NULL/partial-backend error paths; the
`busy` gate blocking state 0 without clearing state; idle scheduling
(`rand()%0x3C + 0xB4`) and the strict `now > next_time` transition to
state 2 (`+0xB4`); the state-2 input one-shot chain (code 1 fires then sets
its flag, the second code-1 branch sets its flag without firing, codes 2/5
fire without setting `input_shot[2]/[3]`); one-record-per-tick dequeue
order, count/index exhaustion reset and the dispatch-return state contract;
the `0xDA`-style timer gate; the flush arm (clear, enqueue `0x43`, threshold
`0x40`, dispatch `0x43`); the one-shot arm (threshold `0x20`, no dequeue);
state 1 flag drain (threshold 8) and the schedule→state-1 fallthrough;
state 2 command threshold 0; signal code mapping `8`/`0x20`/`0x21`/`0x40`/
other; `schedule` deadline/phase/distance/code gates, posting `0x8D`/4 and
the `last_run` stamp, plus the NULL and missing-`post` errors. ASan+UBSan
build clean (`cc -fsanitize=address,undefined
-Iinclude tests/test_event_queue.c
src/fifa96_loader/fifa96_event_queue.c`).

## 9. Errata (quoted)

* FU-60 §4.3: "the `id == -0x617F3DC` arm additionally writes the
  `0x5B33C/0x5B340/0x5B344` triple (`0xF9E80C24`/`0xFFFE37E9`/now)" —
  **corrected**: those two constants are a decompiler artifact
  (`decompile_function 0x92548`); the listing writes `[0x5B33C] = param` and
  `[0x5B340]++` (`0x926B3`/`0x92787`), in the `id==0xDC` arm and the generic
  `value!=0` arm, gated by `[0x5B37C] == record[+0x18]`; there is no
  `-0x617F3DC` comparison.
* FU-60 §9 "getter `FUN_00092194` with zero callers; role undetermined" —
  **refined**: called from `FUN_00072AC4` at `0x73665` (reachable through
  `0x73631`/`0x7363D`); compares `[0x5B33C]` against `[0x57A83]` and posts
  `0xD8`/code `0x21` when they differ (§6). `[0x5B340]`/`[0x5B344]` remain
  unread.
* FU-60 §4.2 "enqueue `0x8D`/param 4 … the sound-event enqueue in
  `FUN_0008F188`" — **clarified**: `FUN_0008F188` is the command-ring enqueue
  (§4.4); the `0x5B380` record queue is fed by `FUN_0008F0C4`. The
  `[0x5B35C]=now` stamp also runs when the code is outside `1..5` (§2).
* FU-60 §4.3 "state 0 … `FUN_0008F2C4(0x43,0)`" — **corrected**:
  `0x43` is enqueued into the `0x5B380` queue by `FUN_0008F0C4`; the command
  ring call is `FUN_0008F2C4(0x40)`.
* FU-60 §9 "`FUN_0008F0C4`'s and `FUN_0008F178`'s callers beyond `0x91DD8`
  are not enumerated" — **closed**: `FUN_0008F0C4` 217 call sites,
  `FUN_0008F178` 4 (`0x1B0B2`, `0x51273`, `0x6386D`, `0x5137C`),
  `FUN_0008F188` ~107, `FUN_0008F2C4` exactly the four pump sites.
* FU-60 §9 "`FUN_00092548`'s inner dispatches … not decomposed" — **partly
  advanced**: the id/cooldown/team/tail structure is §5; the callees
  themselves (`FUN_00066724`/`0x66840`/`0x668CC`/`0x6690C`/`0x669C0`,
  `FUN_00011E0C`, `FUN_000741B4`) remain open.
* FU-60 Method "decompiler died" on `0x91DD8` — **closed**: 498-instruction
  clean `disassemble_function` this slice, one pruned arm decoded from raw
  bytes (§Method). The FU-60 hand-decoded `0x92330` arm matches the bytes.
* Listing artifact: the redundant `0x9221D JZ` (see Method) and the missing
  `0x92330..0x9235D` in `disassemble_function`; use `read_memory 0x92330`.

## 10. Open legs

* **Command-ring count** `[0x5AAD8]`: init 1, `INC` per `FUN_0008F188`, drain
  decrements to `-1` and leaves it; the state-2 guard `!= 0` is then true and
  a second drain would compute a negative ring index. The count encoding is
  not decoded; the port uses a boolean `ring_pending`.
* **Dispatcher gates/actions**: the 600-tick cooldown (`0x5AC38`), per-id
  counter (`0x5AFB8`), `0x21` team gate (`[0x57A83]`) and the
  `FUN_00066724`/`0x66840`/`0x668CC`/`0x6690C`/`0x669C0`/`0x11E0C`/`0x741B4`
  callees are not decomposed or ported.
* `[0x5B340]`/`[0x5B344]` have no readers; `[0x5B33C]`'s only consumer is the
  tracker posting at §6. `[0x5B350]/[0x5B354]/[0x5B358]/[0x5B37A]/[0x5B42C]`
  are initialised only (`FUN_00091BC4`) and not derived.
* `[0x5AFAC]`/`[0x5AFAD]` are read but never written by the pump; the writers
  of those one-shot flags (and of `[0x5AFAE]`/`[0x5AFAF]` outside the pump)
  were not located.
* `[0x4C312]` (presentation gate) and `[0x4C302]` (clock halt, FU-62) have no
  static writer; `[0x4E574]`'s unnamed write sites were not classified
  beyond the §7 table.
* The uncalled gap helpers (`0x924E8`, `0x9252C`, `0x8F32F`, `0x8F34C`,
  `0x8F37C`, `0x9219C`) are dead/fixup-reached — not decided.
* `FUN_00091528`/`0x91630`/`0x91774` input actions are only characterised as
  call targets; `FUN_0008EF38`'s code map is quoted from FU-60/FU-62.
* Object-base classification of each quoted global (FU-59 errata) was not
  re-run per address; addresses are quoted as Ghidra displays them.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`disassemble_function` 0x91DD8, 0x8F0C4, 0x8F178, 0x8F188, 0x8F2C4, 0x91BC4,
0x92548;
`disassemble_bytes` 0x924E8 (96 B), 0x92210 (40 B), 0x8F32F (96 B), 0x73600
(112 B), 0x73640 (48 B), 0x73665 (80 B), 0x92330 (48 B);
`read_memory` 0x8F2C4 (107 B), 0x92330 (48 B);
`decompile_function` 0x92548, 0x8F2C4, 0x66E70, 0x51AB8, 0x53DC4, 0x53DE0,
0x537F8, 0x52030, 0x72AC4, 0x66DFC, 0x91528, 0x8EF38;
`get_xrefs_to` 0x4E574, 0x5B330, 0x5B334, 0x5B338, 0x5B33C, 0x5B340, 0x5B344,
0x5B348, 0x5B35C, 0x5B368, 0x5B36C, 0x5B370, 0x5B374, 0x5B37C, 0x5B428,
0x4C312 (via FU-60), 0x8F0C4, 0x8F178, 0x8F188, 0x8F2C4, 0x92194, 0x924E8,
0x8F32F, 0x73665, 0x5AAD4, 0x5AAD8;
`get_function_by_address` 0x924E8; `get_current_program_info`.

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change. Port write
set: `include/fifa96_loader/fifa96_event_queue.h`,
`src/fifa96_loader/fifa96_event_queue.c`, `tests/test_event_queue.c`,
`CMakeLists.txt` (one library/test block). `make test`: 51/51 before,
**52/52 after**; ASan+UBSan `test_event_queue` clean
(`cc -fsanitize=address,undefined -Iinclude tests/test_event_queue.c
src/fifa96_loader/fifa96_event_queue.c`). `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
