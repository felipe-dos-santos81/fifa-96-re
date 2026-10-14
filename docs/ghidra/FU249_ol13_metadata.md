# FU-249 — OL-13 / FU192-L1: the per-record metadata producer `FUN_0006D920` and the `[rec+0x1C]` dispatch seam (phase-24 wave W24-2)

Provenance: M2 phase-24 wave **W24-2**, 2026-10-14, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` this session: watcom `LE:32:default`, image
base `0`, `min_address 0x10000`, 2706 funcs). Every byte/address/xref/instruction
below is a fresh `read_memory` / `disassemble_function` / `disassemble_bytes` /
`decompile_function` / `get_xrefs_to` / `search_instructions` of **this session**
unless marked *cross-cited* (then the frozen FU doc is named). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE` untouched;
no git commands. The alias traps are applied (`dword[x]>>16` = word at `x+2`,
`>>24` = byte at `x+3`; CALL rel32 verified). **This slice verifies and re-derives
only — it ports nothing.**

Scope: (1) the exact producer of `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` (FU192-L1),
fresh; (2) its loader-content chain; (3) the producer's **live writer surfaces**
(fresh xref census); (4) the `[rec+0x1C]` dispatch seam (FU175-L1) and its call
sites; (5) the consumers — the phase-`0x02`/`0x15` gate and the action-row-`03`
`[rec+0x1C] = 0x6DCC8` rewrite; (6) the **port shape + census ruling**.

**Ruling headline (stated up front, derived below):** the metadata *begin* bind is
already landed (FU-199 §7.2/§11/§12) and mirrors the native. The **exact remaining
producer** is the **per-walk / per-reset / per-single re-stage** — the native calls
`FUN_0006D920` from **four** surfaces, but the engine walks without re-binding. It
is **behaviourally idempotent** while the formation id and each record's `[rec+0x8D]`
are stable, so it is a **fidelity-only** remainder. **Census ruling `+0`**: the
phase-`0x02`/`0x15` gate is already live/counted; action row `03` stays a numbered
leg (tape-visible, FU177-L1). Nothing here moves the count.

---

## 1. The metadata producer `FUN_0006D920` (fresh `disassemble_function` + `decompile_function`)

Fresh `disassemble_function 0x6D920` (51 instructions) — byte-level (the
decompiler mis-models the `FUN_0004C384` return into an `undefined8`; the disasm
is authoritative):

```
0x6D924 EDX=EAX(rec) ; 0x6D926 EAX=[EAX]=[rec]           ; team
0x6D928 EAX=[EAX+0x7AE]                                  ; layout row ptr
0x6D92E MOVSX BX,byte[EAX]                               ; id = layout[0] (int8)
0x6D932 IMUL EBX,EBX,6                                   ; id*6
0x6D935 ESI=[EDX+0x8A] ; 0x6D93B MOVSX CX,byte[EDX+0x8D] ; rec ordinal (idx)
0x6D943 SAR ESI,0x18 ; SHL ESI,2                         ; (rec[0x8A]>>24)*4
0x6D94B MOV AX,BX ; 0x6D951 CALL 0x4C384 ; 0x6D956 ADD EAX,ESI
0x6D95E [rec+0x8] = EAX                                  ; id*6+0, + (rec[0x8A]>>24)*4
0x6D958 IMUL ESI,ECX,0x256                               ; idx*0x256
0x6D961 EAX=EBX+1 ; 0x6D969 CALL 0x4C384 ; 0x6D96E ADD EAX,ESI
0x6D970 [rec+0xC] = EAX                                  ; id*6+1, + idx*0x256
0x6D975 LEA ESI,[ECX*8] ; 0x6D97C EAX=EBX+2 ; 0x6D98C CALL 0x4C384
0x6D991 ADD EAX,ESI ; 0x6D996 [rec+0x10] = EAX           ; id*6+2, + idx*8
0x6D981 IMUL ECX,ECX,0x958 ; 0x6D993 EBX+=4 ; 0x6D99E CALL 0x4C384
0x6D9A3 ADD EAX,ECX ; 0x6D9A5 [rec+0x14] = EAX           ; id*6+4, + idx*0x958
0x6D9A8 EAX=[0x157A4A] ; 0x6D9AD SAR EAX,0x18 ; 0x6D9B0 SHL EAX,2
0x6D9B3 ADD EAX,0x110794 ; 0x6D9B8 EAX=[EAX]            ; 0x110794[byte 0x157A4D]
0x6D9BA [rec+0x1C] = EAX                                 ; the per-record phase handler
```

So, **definitively** (fresh, re-confirmed byte-for-byte against FU-199 §5.1):

| field | source (fresh) |
|---|---|
| `[rec+0x8]`  | `FUN_0004C384(id*6+0) + (rec[0x8A]>>24)*4` |
| `[rec+0xC]`  | `FUN_0004C384(id*6+1) + (int8)rec[0x8D] * 0x256` |
| `[rec+0x10]` | `FUN_0004C384(id*6+2) + (int8)rec[0x8D] * 8` |
| `[rec+0x14]` | `FUN_0004C384(id*6+4) + (int8)rec[0x8D] * 0x958` |
| `[rec+0x1C]` | `0x110794[(int8)byte[0x157A4D]]` (the phase table) |

`id = (int8)byte[[[rec]+0x7AE]]` (the layout row's first byte = the formation id);
`idx = (int8)byte[rec+0x8D]` (the record ordinal 0..10).

**`[rec+0x1C]` is staged by this same routine** at `0x6D9BA` from the **current
byte `0x157A4D`** (`[0x157A4A]>>24`), i.e. the phase handler is re-resolved on
every `FUN_0006D920` call (not once).

This pins FU192-L1's producer exactly: **`FUN_0006D920`**, keyed by the formation
id and the record ordinal. It is *not* `FUN_00011620` (that is the id + desired-order
producer one hop upstream, FU-199 §5.2).

---

## 2. The loader-content chain — `FUN_0004C384` / `FUN_0004AFB8` / `FUN_0004A6BC` (fresh)

Fresh `decompile_function`:

* `FUN_0004C384(n)` → `FUN_0004AFB8(n)` → **`return (&DAT_0014BFC0)[n];`** — the
  **loaded-resource-pointer cache**.
* `FUN_0004A6BC` (fresh `decompile`, called once from `FUN_0004AC6C`) is the
  **loader that fills `0x14BFC0`**:
  - `DAT_0010736C = FUN_0004a344("art\\gameart0.pvi")` (the archive open);
  - loop 1 fills `0x14BFC0[0 .. 0x74]` (step 4 = **30** entries) via
    `FUN_00099e9f(buf, ..., "%s.fmt")` then `(&0x14BFC0)[i] = FUN_000a2614()`
    (the per-file load);
  - loop 2 fills `0x14BFC0[0x78 .. 0x98]` via `"%s.dat"`;
  - loop 3 fills `0x14BFC0[0x9c .. 0xdc]` via `"%s_%s"` reading the
    `PTR_s_352ko_00107370` name table;
  - loop 4 fills `0x14BFC0[0xe0 .. 0xf8]` via `"%s.qfs"`;
  - then `DAT_0014C0E0 = FUN_000a2614()`.
  - teardown arm (`param_1 == 0`): `FUN_000993ec` + `FUN_0009e907(&0x14BFC0,0,0x3f)`.

Fresh `read_memory 0x107370` (96 B) and `0x101A30` (128 B): the name table is
`{ko,pl,pk,ps,freekick,sp}` per 6-name group; the four used slots
`id*6+{0,1,2,4}` are `ko`/`pl`/`pk`/`freekick`. Fresh `0x101A30` decodes to
`"352ko"`,`"352pl"`,`"352pk"`,`"352ps"`,`"freekick"`(at `0x101A50`, shared),
`"352sp"`; the `0x107370` dwords at slot 4 and slot 10 both point to `0x101A50`
(the shared `freekick`) — matches FU-199 §5.1/§7.2.

`FUN_0004AC6C` (fresh `decompile`, once-only latch `DAT_001074A0`) is the loader
entry; fresh `get_xrefs_to 0x4A6BC` = **1** (`0x4ACE5` in `FUN_0004AC6C`), and
`get_xrefs_to 0x4C384` = **6** (4 in `FUN_0006D920`, 2 in `FUN_0006D9C4`).

**The engine substitute** (first-hand `fifa96_match_run.c`): the loader is
`match_run_record_meta_bind` (`:5421`) — it reads `/ART/GAMEART0.PVI` (`:1889`),
parses the BIGF (`match_stage_container` + `fifa96_bigf_parse`), finds the id's
`.fmt` family **by name** (`match_formation_family[5][3]`, `:5395`) and `memcpy`s
the three blobs into `mr->meta_owner`. It mirrors `FUN_0006D920`'s strides in
`fifa96_match_entities_bind_record_meta` (`fifa96_match_entities.c:371`:
`idx*0x256` bytes / `idx*2` dwords / `idx*0x256` dwords — native-exact). It does
**not** model `FUN_0004A6BC` (the `.fmt/.dat/_/.qfs` construction), the `.dat`/
`_`/`.qfs` siblings, or `DAT_0014C0E0`; it also bounds the id to `0..4` (the native
has no range check) — a documented hardening.

---

## 3. The producer's live writer surfaces (fresh `get_xrefs_to`)

`[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`/`[rec+0x1C]` are all staged by `FUN_0006D920`.
Fresh xref census (this session):

| callee | fresh `get_xrefs_to` | callers |
|---|---|---|
| `FUN_0006D920` | **4** | `0x79983` (`FUN_0007997C`), `0x6DA0A` (`FUN_0006D9C4`), `0x8D107` (`FUN_0008D098`), `0x8CF73` (`FUN_0008CF60`) |
| `FUN_0006D9C4` | **2** | `FUN_0008CE78` (`0x8CE87`), `FUN_0008D098` (`0x8D0B1`) |
| `FUN_0008CE78` | **2** | both in `FUN_0007412C` (`0x7414D`, `0x74171`) |
| `FUN_0007412C` | **6** | `FUN_0004B02C` (`0x4B064`), `FUN_0007417C` (`0x74195`), `FUN_00041494` (`0x41702`), `FUN_0008EA70` (`0x8EA79`), `0x7420D`, `0x742C0` |
| `FUN_0008D098` | **2** | both in `FUN_000740A0` (`0x740C8`, `0x740DB`) |
| `FUN_000740A0` | **27** | the phase writer (FU-175 §4) |
| `FUN_0008CF60` | **2** | both in `FUN_00073E08` (`0x73E17`, `0x73E21`) |
| `FUN_00073E08` | **9** | the record-placement commit |
| `FUN_0007997C` | **1** | `FUN_0008C33C` (`0x8C34A`); `FUN_0008C33C` xrefs = **4** |

**The producer runs from four surfaces**, not one:

1. **Match begin** — `FUN_0007412C` → `FUN_0008CE78(team, [0x14C1E4/5])` →
   `FUN_0006D9C4(team, id)` → per-record `FUN_0006D920`. (The engine's bind site.)
2. **Every phase write** — `FUN_000740A0` (fresh `decompile`: writes `[0x157A4E] =
   [0x157A4D]`, `[0x157A4A]>>24 = new phase`, `[0x157AAF] = side`, then
   `FUN_0008D098(team)` twice) → `FUN_0008D098` → **`FUN_0006D9C4` at the head
   (`0x8D0B1`)** and **`FUN_0006D920` per record (`0x8D107`)**.
3. **Record reset** — `FUN_00073E08` → `FUN_0008CF60` (a per-record 11-loop:
   `FUN_0006D920(rec)` then `CALL [rec+0x1C](rec, &rec+0x4D)`, stride `0xB2`).
4. **Single record** — `FUN_0008C33C` → `FUN_0007997C` → `FUN_0006D920(rec)` then
   `CALL [rec+0x1C](rec, &rec+0x4D, -1)`.

**This is the exact remaining producer (FU249-L1).** The engine's
`fifa96_match_run_record_meta_bind` is called **once**, at the match begin
(`fifa96_match_run.c:2298`); the engine's walk mirror
`fifa96_match_run_record_phase_walk` (`:2494`) — the `FUN_0008D098` counterpart,
called twice per `match_run_write_phase` (`:2530/:2531`) — resolves + dispatches
the row but **does not re-run the bind**. The native re-stages the metadata
pointers (and re-resolves `[rec+0x1C]`) on **every** phase write. Because the
bind depends only on `id` and `(int8)rec[0x8D]`, it is **idempotent** while those
are unchanged — which is why the begin-only engine bind is M2 byte-identical and
why the remainder is **fidelity-only**. It becomes observable only if the
formation id or a record's `[rec+0x8D]` changes mid-match (the FU197-L2
substitution / order-routine producer, a separate leg).

Also fresh: `FUN_0008D098`'s head runs `FUN_0008C418(team, 0)` (`0x8D0BA`) and
clears `[team+0x7CB]/[team+0x7B6]/[team+0x7BF]` (`0x8D0C7`/`0x8D0D4`/`0x8D0E1`)
before the loop — this is the same `FUN_0008C418` block the row-03 clamp needs
(FU177-L1), reconfirming that leg's owner.

---

## 4. The `[rec+0x1C]` dispatch seam (fresh `search_instructions`)

Fresh `search_instructions mnemonic=CALL operand="0x1c]"` over the whole program =
**8** `CALL dword ptr [reg+0x1c]` sites (the `[rec+0x1C]` handler invocations):

| site | containing body | role |
|---|---|---|
| `0x8D110` | `FUN_0008D098` | the per-record **walk** (×11 records ×2 teams per phase write) |
| `0x8CF7C` | `FUN_0008CF60` | the 11-record **reset** loop (from `FUN_00073E08`) |
| `0x799A6` | `FUN_0007997C` | single-record (from `FUN_0008C33C`) |
| `0x79D10` | `FUN_00079CCC` | the **nearest-record search** (fresh `decompile`: calls the handler per record, then `FUN_0008DC68` distance) |
| `0x7E252` | action row `03` slot branch A (raw `0x7E252`) | mid-body phase call |
| `0x7E2DD` | action row `03` idle core (raw `0x7E2DD`) | mid-body phase call |
| `0x87D29` | action row `15` prologue (`0x87D29`) | mid-body phase call |
| `0x74C98` | unnamed raw region (action body) | mid-body phase call |

The handler ABI is the walk's two-arg watcall (`EAX=rec`, `EDX=&rec+0x4D`,
`EBX=-1`), as FU-175 §5 established. The engine mirror is
`fifa96_match_dispatch_phase_record(mr, phase, e, -1)` (per-record; lands via the
FU-191 walk cursor, `fifa96_match_handlers.c:5649`) plus `match_row_phase_call`
(`:4555`) for the action-body mid-calls.

**FU175-L1 status:** the seam is **landed** for both the walk shape
(`fifa96_match_dispatch_phase_record`, phase-17 T1) and the action bodies
(`match_row_phase_call` in `fifa96_match_action_03`/`_15`). The engine walks the
8 surfaces that matter; the only unmodelled surface is the **re-bind** (§3,
FU249-L1).

---

## 5. The consumers — the phase-`0x02`/`0x15` gate and the action-row-`03` rewrite (fresh)

Fresh `read_memory 0x110794` (140 B) = the phase table, byte-identical to FU-192 §1:

```
[00]=06DE34 [01]=06E1D0 [02]=06DCC8 [03]=06DE44 [04]=06DE44 [05]=06E05C
[06]=06DD9C [07]=06DE44 [08]=06DD6C [09]=06DD6C [0A]=06DE34 [0B]=06DE34
[0C]=06DF4C [0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8
[12]=06E1D0 [13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000 (INT3) …
```

* **Phase `0x02` and `0x15` both resolve to `0x6DCC8`** (`0x110794[2]` and
  `[0x15]`). Fresh `get_xrefs_to 0x6DCC8` = **5** (2 DATA: `0x11079C`=[02],
  `0x1107E8`=[15]; 3 CALL: `0x6DD91`=`0x6DD6C`, `0x6DF37`=`0x6DE44`, `0x6DD61`=the
  orphan `0x6DD3C`). Fresh `decompile 0x6DCC8`:
  `if (param_3 < 0) idx = word[0x1577DE + side*4]>>16; else idx = param_3;
  pcVar1 = [rec+0xC] + idx*2; target[0]=byte0*±0x26; target[1]=0;
  target[2]=byte1*±0x21` — **the gate is `[rec+0xC]` (`param_1[3]`)**.
* **Phase `0x03` resolves to `0x6DE44`** (`0x110794[3]`), gate `[rec+0x14]`
  (fresh `get_xrefs_to 0x6DE44` = 3 DATA: `0x1107A0`=[03], `0x1107A4`=[04],
  `0x1107B0`=[07]).
* **The action-row-`03` body `0x7E1A4`** (fresh `disassemble_bytes`): the
  phase-`0x13/0x14` special arm at `0x7E1CE..0x7E223`, then the slot gate at
  `0x7E224` (`[rec+0x20] != 0 && phase != 0xC`), then the `[rec+0x1C]` sub-gate:
  `EDX = [0x157A49]>>24 (= byte 0x157A4C, the engine `mr->global_157a4c`);
  if (byte == 1 || phase == 6 || phase == 0x15) → 0x7E248 LEA EDX,[ESI+0x4D];
  MOV EBX,-1; MOV EAX,ESI; CALL dword ptr [ESI+0x1c]` (**`0x7E252`**) — the
  **row-03 `[rec+0x1C]` = `0x6DCC8` rewrite** (for phase 2 the handler is
  `0x110794[2] = 0x6DCC8`); else the `0x7E25F` slot branch B. The idle core
  (`0x7E2D3..0x7E308`) also calls `[rec+0x1C]` (**`0x7E2DD`**) unconditionally.

**Engine state (first-hand `fifa96_match_handlers.c` / `tests/test_engine_m2.c`):**

* Phase row `0x02` = `fifa96_match_phase_02` (`:5555`), phase `0x15` shares it
  (`:5593`), phase `0x03` = `fifa96_match_phase_04` (`:5557`); the three bodies
  (`:4579` `match_phase_dcc8_apply`, `:4604` `fifa96_match_phase_02`, `:4646`
  `fifa96_match_phase_04`) gate on `rec->phase_meta`/`phase_meta14` NULL
  (`:4595`/`:4671`). **Both `0x02` and `0x15` are already wired/counted**
  (`phase_wired == 23`, `test_engine_m2.c:3668`; `fifa96_match_dispatch_phase_record(&mr,
  0x02/0x15, e, -1) == FIFA96_OK`, `:3945`/`:3947`). The `[rec+0xC]` gate is open
  via the landed bind (FU-199 §7.2/§11).
* Action row `0x03` = **`{0x03, NULL, …}`** (`:5455`) — **NULL/uncounted**;
  `fifa96_match_dispatch_action(&mr, 0x03u) == -FIFA96_ERR_UNSUPPORTED`
  (`test_engine_m2.c:4082`), `action_expect[0x03] == UNSUP`
  (`test_engine_match_handlers.c:1156`). The body `fifa96_match_action_03`
  (`:4846`) is ported and its `[rec+0x1C]` mid-call is faithful (`:4896`
  `global_157a4c == 1 || phase == 6 || phase == 15 → match_row_phase_call`;
  `:4938` idle core), but wiring the slot moved M2 at transcribed frame 29
  (controller R7) — so it stays a leg (**FU177-L1**), and row `03` remains a live
  force-decision code (`match_run.c:960`), i.e. its uncounted status is honest.

---

## 6. Port shape + census ruling

**Port shape (fidelity-only).** The begin bind is landed; the only unmodelled
native producer is the **re-stage on the walk/reset/single surfaces** (§3). The
minimal faithful port is a `match_run_record_meta_rebind(mr)` that mirrors
`FUN_0006D9C4(team, id)` + per-record `FUN_0006D920(rec)`, folded into
`fifa96_match_run_record_phase_walk` (`fifa96_match_run.c:2494`, before the
per-record dispatch) and the record-reset path (`FUN_00073E08`'s engine mirror).
It re-derives `id` from `mr->formation[team]` and `idx` from `e->active` and
re-stages `phase_meta`/`phase_meta10`/`phase_meta14` in place. **No new engine
field is needed** — the `[rec+0x1C]` slot itself is not modelled as a stored
pointer; `fifa96_match_dispatch_phase_record(mr, phase, e, -1)` already resolves
`0x110794[phase]` per record at call time, which is observationally identical to
the native staging **provided `mr->state.phase` equals the native byte `0x157A4D`**
(it does — the walk is invoked from `match_run_write_phase` after the phase write).
**The port is a no-op on the M2 tape** (idempotent while `formation`/`active` are
stable); it is a **fidelity** gain for a future mid-match formation change, and
should be paired with the FU197-L2 order-routine producer or explicitly deferred
as FU249-L1.

**Census ruling — the phase-`0x02`/`0x15` gate and row `03`.**

* **Phase `0x02`/`0x15`: go live — already live, `+0`.** Both rows are wired and
  counted; the `[rec+0xC]` gate is opened by the landed bind. Porting the
  re-stage changes no byte of the phase table and no count. **No census movement.**
* **Action row `03`: stays a numbered leg, `+0`.** Wiring its slot makes the
  phase-`0x13/0x14` special arm (`0x7E1CE`) and the `[rec+0x1C]` rewrite run on
  the on-tape phase-`0x13/0x14` frames and moved M2 at frame 29; it is not
  tape-inert. Owner **FU177-L1** (the row-03 rewrite chain), with the
  `FUN_0008C418` team-bound block (fresh-reconfirmed at `FUN_0008D098` `0x8D0BA`)
  as the clamp-input leg.

**Net ruling: `+0`.** FU249 closes no row and re-pins nothing; the phase-`0x02`/
`0x15` gate is already live and action row `03` is honestly gated. The slice's
real finding is the **four-surface producer** and the **idempotent re-stage**
remainder.

---

## 7. Numbered legs

1. **FU249-L1 — the per-surface re-stage is unmodelled (the exact remaining
   producer).** The native calls `FUN_0006D920` from **four** surfaces (begin
   `FUN_0007412C`, walk `FUN_0008D098`, reset `FUN_0008CF60`, single
   `FUN_0007997C`); the engine binds **only at match begin**
   (`fifa96_match_run.c:2298`) and the walk (`:2494`) does not re-bind. It is
   idempotent while `mr->formation`/`e->active` are stable, so the remainder is
   **fidelity-only**; it becomes observable only with a mid-match formation
   change (owner: the FU197-L2 order-routine producer). Port shape §6.
2. **FU249-L2 — the loader `FUN_0004A6BC` content model.** The engine substitutes
   a by-name BIGF lookup for the native `sprintf("%s.fmt"/"%s.dat"/"%s_%s"/
   "%s.qfs")` + `FUN_000a2614` load; the `.dat`/`_`/`.qfs` siblings and
   `DAT_0014C0E0` are unmodelled (supersedes/extends FU199-L7). Not gate-relevant.
3. **FU249-L3 — the team-level `id*6+3`/`id*6+5` slots.** `FUN_0006D9C4` also
   sets `[team+0x7DB] = FUN_0004C384(id*6+3)` and `[team+0x7DF] =
   FUN_0004C384(id*6+5)` (fresh disasm `0x6DA3E`/`0x6DA51`); these are *team*
   pointers, not the record metadata, and are not bound by
   `fifa96_match_entities_bind_record_meta` (carries FU200-L6).
4. **FU249-L4 — the `[rec+0x8]` (`id*6+0` = `ko`) semantics.** The engine binds
   only the three non-`ko` fields; the native still stages `[rec+8]` (the by-value
   formation seed's blob). If the seed is ever switched to the pointer model this
   field matters (FU199 §7.2 item 2 note).
5. **FU249-L5 — the unnamed `[rec+0x1C]` call site `0x74C98`.** A raw-region
   action body invokes the handler; its containing body is not a defined function
   and was not identified here (not a phase/action-table slot target).
6. **FU249-L6 — `FUN_00079CCC`'s handler-driven distance pick.** The
   nearest-record search calls `[rec+0x1C]` per record then `FUN_0008DC68`; its
   engine counterpart (if any) is unverified here.
7. **FU249-L7 — the row-03 `[rec+0x1C]` tape-visible chain.** Restates
   **FU177-L1** with the fresh `0x7E252`/`0x7E2DD` bytes and the
   `global_157a4c`/phase-`{1,6,0x15}` sub-gate; owner the row-03 rewrite chain.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (program `/FIFA96.EXE`, watcom LE, image base 0).
* `disassemble_function` — `0x6D920` (51), `0x6D9C4` (52), `0x8CE78` (23),
  `0x8D098` (583).
* `decompile_function` — `0x6D920`, `0x4C384`, `0x4AFB8`, `0x4A6BC`, `0x7412C`,
  `0x8CF60`, `0x7997C`, `0x79CCC`, `0x740A0`, `0x6DCC8`, `0x4AC6C`.
* `disassemble_bytes` — `0x7E1A4` (200 B).
* `read_memory` — `0x110794` (140 B, the phase table), `0x107370` (96 B, the name
  table), `0x101A30` (128 B, the name strings), `0x157A48` (32 B).
* `search_instructions` — `mnemonic=CALL operand="0x1c]"` (8 sites).
* `get_xrefs_to` — `0x6D920` (4), `0x6D9C4` (2), `0x4C384` (6), `0x4A6BC` (1),
  `0x7412C` (6), `0x8CE78` (2), `0x8D098` (2), `0x740A0` (27), `0x8CF60` (2),
  `0x7997C` (1), `0x73E08` (9), `0x8C33C` (4), `0x6DCC8` (5), `0x6DE44` (3),
  `0x157A4A` (128).
* `get_function_by_address` — `0x79D10` (`FUN_00079CCC` `0x79CCC..0x79D59`),
  `0x73E08` (`FUN_00073E08` `0x73E08..0x73E25`); `0x74C98` = **no function**.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`:4547..5010` the seam + the per-record bodies, `:5012` the action/phase table,
`:5448` action table, `:5550` phase table), `src/fifa96_engine/fifa96_match_run.c`
(`:1889` the bank, `:1915` seed, `:1944` slot bind, `:2280..2302` begin order,
`:2490..2534` the walk + `match_run_write_phase`, `:2494`
`fifa96_match_run_record_phase_walk`, `:5379..5477` the meta bind),
`src/fifa96_engine/fifa96_match_entities.c` (`:363..383` the bind),
`include/fifa96_engine/fifa96_match_entities.h` (`:178..192` the metadata fields),
`tests/test_engine_m2.c` (`:3663..3668` `action_wired==35 && phase_wired==23`,
`:3945..3947` phase-`0x02`/`0x15` OK, `:4035` the gated set, `:4082` action-`0x03`
UNSUP), `tests/test_engine_match_handlers.c` (`:1156` `action_expect[0x03]`).

Docs cross-cited: FU-170, FU-175 (§4/§5/§7), FU-177 (§2/§3/§7), FU-191 (§4/§6),
FU-192 (§1..§3, §4 erratum), FU-196, FU-197, FU-199 (§5/§7.2/§8/§11/§12), FU-200,
FU-205, ENGINE.md (phase-17 T1/T3, phase-18 T1/T2/T5). The alias traps
(`[0x157A4A]>>24` = byte `0x157A4D`; `[0x157A49]>>24` = byte `0x157A4C`;
`dword[x]>>16`) are applied.

No writes beyond this slice file and the workspace report. `/FIFA96.EXE`, the ISO
and `game/hdd/` untouched.

---

## 9. Ruling / handoff

**FU249 = `+0` (fidelity-only).** The `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`
producer is **`FUN_0006D920`** (id-keyed `.fmt` family over the `FUN_0004C384`
cache); the begin bind is landed. The **exact remaining producer** is the
**per-walk/per-reset/per-single re-stage** — idempotent, so no census and no
re-pin. The `[rec+0x1C]` seam is **landed**; the phase-`0x02`/`0x15` gate is
**already live/counted**; action row `03` stays a numbered leg (tape-visible,
FU177-L1). Promote to `docs/ghidra/FU249_ol13_metadata.md`; hand FU249-L1 to the
FU197-L2 order-routine producer and FU249-L2/L3 to the loader-content frontier.
