# FU-111 — the replay arming callers, the input queue and the HUD cluster

Follow-on to FU-110 §6 legs 1–4. This slice fixes the queue's direction,
names its real producers/consumers, decomposes the two packer call sites, and
separates the `0x55C78..0x55C8C` HUD button cluster from the queue storage.

Result in one line: **the packer call sites are `FUN_00049b28`'s loop tail
(`0x4A040`) and the mode-gated `FUN_00049ad0` (`0x49B1E`, created); the
"not-in-replay" hook `FUN_00064ec0` is a *push* (EAX=1) fed by `FUN_000974dc`
← `FUN_0006428c` + seven event sites, `FUN_00064ee4` is the *pop* (EAX=0)
whose value `FUN_00036c70` stores into the record lane `+0xF3`; the queue is
20 dwords at `0x55C90..0x55CDC` (not at `0x55C8C`, which is HUD state), and
the buffer free is the orphan teardown tail `0x4AEEE..0x4AF1D`.**

## 1. The two packer call sites (FU-110 §6 leg 1)

`FUN_00049b28` — the per-frame match/entity loop, `get_xrefs_to 0x6408C`
finds its call at `0x4A040`:

```
0x49FD8 CALL 0x36C3C            ; (after the 0x49FD6 JNZ 0x49FE1)
0x49FDF JZ 0x49FF5              ; the shared mode-gate block
0x49FF5 CALL 0x36C70            ; live replay-block snapshot
0x49FFA CALL 0x4B380            ; mode = [0x57A4A] >> 24
0x49FFF CMP EAX,0x3 ...         ; reset modes {2,9,0xD..0x12,>=0x15},
0x4A024 INC [0x7310]            ;   others increment the frame budget
0x4A02C MOV [0x7310],ECX        ; reset
0x4A032 CMP [0x7310],0x5A       ; 90-frame budget
0x4A039 JGE 0x4A045
0x4A040 CALL 0x6408C            ; pack one replay record (EAX=1 set at 0x4A03B)
```

`FUN_00049ad0` (created this slice, `0x49AD0..0x49B24`) is the same gate as a
standalone wrapper:

```
FUN_00036c70();                 // snapshot
switch (mode = FUN_0004b380()): // 0,1,3..8,10,11,12,0x13,0x14 -> [0x7310]++
                                // 2,9,0xD..0x12,>=0x15        -> [0x7310]=0
if ([0x7310] < 0x5A) FUN_0006408c(EAX=1);   // call at 0x49B1E
```

Both entries are indirect (`get_function_callers` 0x49B28 / 0x49AD0 = none).
`FUN_0004b380` is `[0x57A4A] >> 24` — the mode byte; `FUN_0004b454` is
`[0x4C32A]` (read by the teardown tail below).

The arming site (FU-110) is `FUN_0004a228`: it zeroes `[0x9044]`/`[0x9040]`,
calls `FUN_0005f598`, `FUN_000609ec`, `FUN_00056cf4`, branches on mode `0x10`
(`FUN_0004c394`/`FUN_0004b100`), calls the snapshot `FUN_00036c70`, then
`FUN_000537f8` (camera state init, FU-101 §5), `FUN_0004cd0c`, `FUN_0004cba0`,
`FUN_00064030` (ring reset), `FUN_00064080` (recorder enable) and
`FUN_00091bc4`; no static caller (table-driven entry, leg open).

## 2. The buffer free tail (FU-110 §6 leg 1, the 0x4AFxx caller)

The `FUN_00063ebc` xref at `0x4AF18` is a **tail JMP with EAX=0** (the free
path), at the end of an orphan teardown block whose entry is not statically
fixed (no xref to `0x4AEEE`):

```
0x4AEEE CALL 0x43D94   ; if [0x6740]!=0 -> FUN_0009a16c, clear [0x6740]
0x4AEF5 CALL 0x4A830   ; EAX=0: free [0x748C]/[0x7490], clear 0x4BFC0..0x4C11B
0x4AEFA CALL 0x4B454   ; [0x4C32A]
0x4AEFF TEST EAX,EAX
0x4AF01 JZ 0x4AF08
0x4AF03 CALL 0x92F04   ; [0x4C32A]!=0 -> no-return FUN_00018f04(0)
0x4AF08 XOR EAX,EAX
0x4AF0A CALL 0x49138   ; EAX=0: free model buffers [0x7100]/[0x7104]/[0x7290]
0x4AF0F XOR EAX,EAX
0x4AF11 CALL 0x58E00   ; EAX=0: free scene buffer [0x543E8]
0x4AF16 XOR EAX,EAX
0x4AF18 JMP 0x63EBC    ; free the replay buffer ([0x9AB0] and cursors)
```

so the `0x4AFxx` "allocator caller" FU-110 listed is the replay teardown's
last act, not a ring allocator. (`FUN_00063ebc`'s alloc branch is reached
dynamically — no static xref with a nonzero EAX.)

## 3. The queue, corrected (FU-110 §6 leg 2)

`FUN_00064ef0` (full decompile, EAX = direction):

```
push (EAX==1):
    if (count > 0x13) { tail=(tail+1) wrap 0x13; count--; }      // evict
    head = head+1;  count++;
    (&0x55C8C)[head] = param_2;                                  // store
    if (head > 0x13) head = 0;                                   // wrap after store
pop (EAX!=1):
    if (count < 1) return 0;
    tail = (tail+1) wrap 0x13;  count--;
    return *(int *)(&0x55C90 + old_tail*4) + 1;                  // value+1, 0=empty
```

with count `[0x9AEC]`, head `[0x9AF0]`, tail `[0x9AF4]`. `search_instructions`
`9af0`/`9aec` finds those three globals only inside `FUN_00064ef0`; the queue
storage is `0x55C90 + 4*i` for `i = 0..0x13` (`0x55C90..0x55CDC`). Index 0 of
the base `0x55C8C` is never written by the push (the store happens before the
wrap, and head never equals 0 at store time) — `0x55C8C` belongs to the HUD
(§5).

* **Producers.** `FUN_00064ec0` is the push, not the "not-in-replay pop"
  (FU-109 §4 / FU-110 §2):

  ```
  0x64EC0 PUSH EDX; MOV EDX,EAX        ; word in EAX -> EDX
  0x64EC3 MOV EAX,[0x9A98]; AND EAX,0x80; JNZ ret   ; not the replay family
  0x64ECF CALL 0x37AE4; TEST EAX,EAX; JNZ ret       ; no controller buttons
  0x64ED8 MOV EAX,1; CALL 0x64EF0                   ; push(EDX)
  ```

  Its caller `FUN_000974dc` is `MOV EDX,EAX; CALL 0x64EC0; MOV EAX,EDX;
  CALL 0x65CC0` (push + device-layer notify). `FUN_000974dc`'s callers:
  `FUN_0006428c` (`if ([0x55C4C] >> 24 != 0) FUN_000974dc();`) and seven
  event sites `FUN_00070de0`, `FUN_0007131c`, `FUN_00077728`, `FUN_0007a084`,
  `FUN_0007e7c8`, `FUN_00084021`, `FUN_00088940`.
* **Consumer.** `FUN_00064ee4` is the pop:
  `PUSH EDX; XOR EDX,EDX; XOR EAX,EAX; CALL 0x64EF0; POP EDX; RET`.
  Its caller `FUN_00036c70` (the live replay-block snapshot builder) stores
  the popped byte into the record lane the packer writes at `+0xF3`:

  ```
  DAT_00055c4c._3_1_ = FUN_00064ee4(uVar6);   // [0x55C4F]
  ```

  `FUN_00065cc0` is the device-layer notify: if `[0x55CE0]`, `[0x55D30]`,
  `[0x55D00]` and `[0xA250]` are set, `[0xA254] = FUN_000a7705()`.
* **Direction errata.** FU-109 §4 wrote `FUN_00064ec0` as the "not in replay
  per-frame hook" and FU-110 §2 read its `FUN_00064ef0` call as a pop; the
  disassembly above shows EAX=1 at `0x64ED8` (push), and the pop wrapper is
  `FUN_00064ee4` (EAX=0). The "20-slot ring at `0x55C8C`" is 20 dwords at
  `0x55C90`, with `0x55C8C` excluded.

## 4. The replay control handlers

* `FUN_000642fc` (`0x642FC..0x64660`) — the replay mode machine (EAX = input
  word). Sets `[0x9AA4]=0`; `0x80→0x81`; the `0x81` intro arm (`FUN_00053d7c`;
  `0x478fc`/`0x44d7c`; reset `[0x9A94]`, `FUN_00063cbc`, `[0x9A8C]=0`,
  `FUN_0004d134`, mode `0x82`, `[0x9AA8]=[0x9AC8]=0`); otherwise
  `FUN_00064aa4` (the input word, FU-108 §3) ORed with `FUN_00045025()&0xFF`,
  edge suppression through `[0x9AC8]`, modes `0x84..0x87 → 0x82`; the exit
  path at `0x64460` (`FUN_000543d4`, `FUN_00015594`, mode 0, `FUN_0004afa0`,
  `FUN_0004cef4`, `FUN_00036c70`, `FUN_00053d9c`→`FUN_000974d8`, OR
  `[0x9AA4]=0x80`); button-bit decode (`4`/`8`/`1`/`2`/`40` → `FUN_0004ca08`/
  `FUN_0004ca40` pan) and the jump table `0x542E8` on mode-`0x82`:
  * `0x82` — reset `[0x9A9C]`;
  * `0x83` — drain: `[0x9AA8] += EDI`; while `stamp = FUN_00063d34()` in
    `(0, [0x9AA8]]` → `FUN_00063cbc` + `FUN_0006428c`;
  * `0x84` — `FUN_00063e54` (step back);
  * `0x85` — `FUN_00063cbc` (append/step forward);
  * `0x86` — rewind: `[0x9A9C] > 0 ? [0x9A9C]-- : FUN_00063cbc + [0x9A9C]=[0x9AA0]`.
* `FUN_00064dfc` — play-forward helper: if `[0x9A94] < *[0x9AC0]` → mode
  `0x83`, `FUN_00037114`, `[0x9AA8] += FUN_000492cc()`, drain the stamp loop
  (`FUN_00063d34`/`FUN_00063cbc`/`FUN_0006428c`), return 1; else mode `0x82`
  return 0. Caller `FUN_000510dc`.
* `FUN_00064e8c` — replay exit: mode 0, `FUN_0004afa0`, `FUN_0004cef4`,
  `FUN_00036c70`, `FUN_00053d9c()==0 → FUN_000974d8`.
* `FUN_00064e74` — `return [0x9A94] >= *[0x9AC0]` (index vs count).
* `FUN_00064270` — `if ([0x9AB0] != 0) { FUN_0005865c(); [0x9A98] = 0x81; }`
  (arm replay).
* `FUN_000510dc` — the control handler (callers `FUN_00051270`/`FUN_000512b0`,
  the pause-menu input handlers). With state `[0x8E0C]`/`[0x8E10]` and timer
  `[0x4E58C]`: `in_EAX==1` → `[0x4E58C]=1`, `FUN_00064270`, `FUN_00037114`,
  `[0x8E0C]=1`; per-frame: state 1 → `FUN_000642fc`; states >1 accumulate to
  `0xF0`; state 0 → `FUN_00064e74` else `FUN_00064dfc`; exit when
  `[0x4E58C] <= ticks → FUN_00064e8c` (with `0x168` the hard cap).
  `FUN_000512b0` sets the pause flags `[0x4E688]`/`[0x4E598]`/`[0x4E538]`
  around it.

## 5. The `0x55C78..0x55C8C` cluster is the HUD, not the queue

* `FUN_00064690` (created, `0x64690..0x64A82`) — the replay HUD button
  builder. `[0x55C80] = FUN_000a15e0(0x3F0000)` (`FUN_000a0980(0xFC0000)` —
  the nearest-palette-index lookup, i.e. the bar colour red); `FUN_00015864`,
  `FUN_00016c00`, `FUN_00015594`, `FUN_00012280`, `FUN_00078f00` setup;
  `FUN_0004afc0` must return nonzero; `FUN_00018c10(&0x55A54)` loads the
  button descriptors. Seven descriptor blocks build `FUN_00015960` draw
  results, stored:
  * `0x55A78/7C` → `[0x55C88]`
  * `0x55A80/84` → `[0x55C78]`
  * `0x55A88/8C` → `[0x55C8C]`
  * `0x55A98/9C` → `[0x55C7C]`
  * `0x55A54`, `0x55A70/74`, `0x55A68/6C` results discarded.
* `FUN_00064664` (created, `0x64664..0x6468E`) — setter for `[0x55C84]`:
  if value != 1, `[0x9ACC]==0` and value != current → notify `FUN_00065cc0`
  (code `0x17`), then store.
* `FUN_00064c35` (created, `0x64C35..0x64DFA`) — the HUD display handler.
  Formats a string selected by `[0x9A8C]` (`0x2110`/`0x2118`/`0x2120`) via
  `FUN_00099e9f` and draws it (`FUN_00012fe4`, `FUN_00016c10`); switch on
  `[0x9A98]` (table `0x54C20`): `0x82→[0x55C8C]`, `0x83→[0x55C88]`,
  `0x84→[0x55C7C]`, `0x85→[0x55C78]`, else the icon, drawn by
  `FUN_00015d40`. The progress bar is `FUN_000642b0() (= [0x9A94]*100 /
  *[0x9AC0])` scaled by the `FUN_0003773c` helper and `/100` (`0x64D9D`..
  `0x64DAF`), drawn by `FUN_0009f7f0`.
* Calibration fix: `FUN_00064690`'s flow was cut at `0x646E2` by a
  pre-existing mis-disassembled `LES` at `0x646E4`; the data item
  (`undefined4 FECC5FE8h`) at `0x49FD8` cut `FUN_00049b28`'s loop tail the
  same way. Both were cleared and re-disassembled with `followFlow`
  (`0x646E3`: `MOV [0x9AC4],EAX`/`TEST`/`JZ 0x64A7C`; `0x49FD8`:
  `CALL 0x36C3C`/`TEST`/`JZ 0x49FF5`), and the function bodies now run
  `0x64690..0x64A82` and `0x49B28..0x4A064` (contiguous) respectively.

## 6. Closures / errata

* **FU-110 §6 leg 1 — closed.** Both packer call sites decomposed: `0x4A040`
  is `FUN_00049b28`'s own tail; `0x49B1E` is the new `FUN_00049ad0`; the
  `0x4AFxx` site is the orphan teardown tail (free, EAX=0).
* **FU-110 §6 leg 2 — closed.** Producers `FUN_00064ec0`/`FUN_000974dc`
  (event sites + `FUN_0006428c`); consumer `FUN_00036c70` → record byte
  `[0x55C4F]` (`+0xF3`); no other writers of `[0x9AF0]`/`[0x9AEC]`/`[0x9AF4]`.
* **FU-109 §4 / FU-110 §2 — corrected.** `FUN_00064ec0` pushes (EAX=1),
  `FUN_00064ee4` pops (EAX=0); the queue storage is `0x55C90 + 4*i` (20
  dwords), `0x55C8C` is HUD button state.
* **FU-108 §5 / FU-109 §3 — corrected.** `FUN_000974d8` is a single `RET`
  (empty stub), not a re-sync.
* **FU-110 §6 leg 3 — refined.** `FUN_00064f70`'s device block also gates
  `FUN_00065cc0`; six devices via `FUN_0006504c(1,k+1)`.
* **FU-110 §6 leg 4 — still open.** `FUN_0004a228` has no static caller.
* Ghidra-project changes: created `FUN_00049AD0`, `FUN_00064664`,
  `FUN_00064690`, `FUN_00064C35`; cleared the stray data item at `0x49FD8`
  and re-disassembled `0x49FD8`/`0x49FE6`; cleared the `LES` at `0x646E4`
  and re-disassembled `0x646E3`.

## 7. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x49AD0, 0x49B28,
0x64664, 0x64690, 0x64C35, 0x64EF0, 0x64EC0, 0x64EE4 (disasm), 0x36C70,
0x64DFC, 0x64E8C, 0x64E74, 0x64270, 0x510C0, 0x510DC, 0x51270, 0x512B0,
0x974D8, 0x974DC (disasm), 0x37AE4, 0x4B380, 0x4B454, 0x43D94, 0x4A830,
0x49138, 0x58E00, 0x92F04, 0xA15E0, 0xA0980, 0x642B0, 0x642FC (disasm);
`get_function_callers` 0x64EF0, 0x64EE4, 0x64EC0, 0x974DC, 0x36C70, 0x49B28,
0x49AD0, 0x64690, 0x64C35, 0x510DC, 0x64DFC, 0x64E8C, 0x642FC; 
`search_instructions` 9af0/9aec/55c8/55c9; `search_byte_patterns`
`28 a2 04 00`/`fc 42 06 00`/`be 3e 06 00` (none). Address mapping as FU-88.

## 8. Open legs

1. The orphan teardown tail's entry (`0x4AEEE..0x4AF1D`) — no xref; the bytes
   before it are not established as its prologue.
2. `FUN_0004a228`'s entry path (indirect) and the four gate-set sites'
   arguments (FU-110 §6 leg 4).
3. `FUN_000510dc`'s `[0x8E0C]`/`[0x8E10]`/`[0x4E58C]` semantics (the
   `0xF0`/`0x168` enter/exit timing).
4. The pushed words' values at the seven event sites and `FUN_00037AE4`'s
   gate meaning (`FUN_00036bc8` ×3).
5. The HUD button descriptor writers (`0x55A54..0x55A9C`, loaded by
   `FUN_00018c10`) and `[0x55C84]`'s consumers (`FUN_00064664` has no static
   caller).
6. The device layer identities (`FUN_00064f70`, `FUN_0006504c`,
   `FUN_00068cfc`, `FUN_000a6265`, `FUN_000a6a03`, `FUN_000cbbe8`).
