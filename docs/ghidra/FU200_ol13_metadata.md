# FU-200 — the OL-13 metadata writers/consumers + the 7 dispatched-but-gated rows (phase-18 wave R48)

Provenance: M2 phase-18 wave **R48**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; fresh `get_current_program_info` this session: watcom `LE:32:default`,
image base `0`, `min_address 0x10000`). Every byte/address/xref/instruction
below is a fresh `read_memory` / `disassemble_bytes` / `disassemble_function` /
`decompile_function` / `get_xrefs_to` / `get_function_by_address` of **this
session** unless marked *cross-cited* (then the frozen FU doc is named). No
writes: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; no git commands. The alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`; here
`dword[rec+0x8A]>>24` = byte at `rec+0x8D`).

Scope (the R48 brief): for each of the **7 dispatched-but-gated per-record
rows** (`0x03`/`0x04`/`0x06`/`0x07`/`0x08`/`0x09`/`0x15`) + phase `0x02`,
derive the exact metadata read (`[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` via the
engine's `phase_meta`/`phase_meta10`/`phase_meta14` NULL gate); derive the
**writer** (`FUN_0006D920`) and its **source**; prove whether the R47 setup would
make each row genuinely live; state the **tape-dormancy** verdict per row; give
the T1/T5 un-gate rec per row; number the legs.

**R47 (FU-199) landed mid-session** (wave R47–R54 runs concurrently: at session
start `docs/ghidra/FU199_*` did not exist; it appeared after the R48 derivation
below was complete). FU-199 is therefore *reconciled* in §5.1/§7: its
independent derivation of `FUN_0006D920`/`FUN_0004C384`/`FUN_0004A6BC` agrees
with this slice's (§1/§2), and its tape verdict agrees with §6 — and it adds the
concrete T1 rec (its §7) that closes this slice's FU200-L1.

---

## 1. The metadata writer `FUN_0006D920` (fresh `decompile_function` + `disassemble_function`, 51 insns)

`FUN_0006d920(int *param_1)`, `param_1` = the record. Fresh
`get_xrefs_to 0x6D920` = **4** (`0x79983` `FUN_0007997C`, `0x6DA0A`
`FUN_0006D9C4`, `0x8D107` `FUN_0008D098` — the walk, `0x8CF73` `FUN_0008CF60`).
The five stores, byte-level:

```
0x6D926 MOV EAX,[EAX]              ; EAX = [rec]   (team back-pointer)
0x6D928 MOV EAX,[EAX+0x7AE]        ; [team+0x7AE]
0x6D92E MOVSX BX,byte[[team+0x7AE]]; form = (int8) byte[[team+0x7AE]]   ; the formation row id
0x6D932 IMUL EBX,EBX,0x6           ; form*6
0x6D935 MOV ESI,[rec+0x8A]         ; idx source (dword)
0x6D93B MOVSX CX,byte[rec+0x8D]    ; idx = (int8) byte[rec+0x8D]         ; the active/anim index
0x6D943 SAR ESI,0x18               ; dword[rec+0x8A]>>24 == (int8) byte[rec+0x8D]
0x6D95E MOV [rec+0x8],  EAX  ; = FUN_0004C384(form*6  ) + idx*4
0x6D970 MOV [rec+0xC],  EAX  ; = FUN_0004C384(form*6+1) + idx*0x256
0x6D996 MOV [rec+0x10], EAX  ; = FUN_0004C384(form*6+2) + idx*8
0x6D9A5 MOV [rec+0x14], EAX  ; = FUN_0004C384(form*6+4) + idx*0x958
0x6D9A8 MOV EAX,[0x00157A4A]; SAR EAX,0x18; ADD EAX,0x110794; MOV EAX,[EAX]
0x6D9BA MOV [rec+0x1C],EAX   ; = 0x110794[byte 0x157A4D]  (the phase handler)
```

So the four OL-13 pointers are, exactly:

| field | formula | per-record variation |
|---|---|---|
| `[rec+8]`   | `FUN_0004C384(form*6)   + idx*4`     | `idx*4` (4 bytes/record) |
| `[rec+0xC]` | `FUN_0004C384(form*6+1) + idx*0x256` | `idx*0x256` (598 bytes/record) |
| `[rec+0x10]`| `FUN_0004C384(form*6+2) + idx*8`     | `idx*8` (8 bytes/record) |
| `[rec+0x14]`| `FUN_0004C384(form*6+4) + idx*0x958` | `idx*0x958` (2392 bytes/record) |

with `form ∈ {0..4}` (the 5 formation ids) and `idx = (int8) byte[rec+0x8D]`
(the record's active/anim index — the engine's `records[].active`). Note the
**index stride 3 is skipped** (`form*6+3` unused on this path); `+4` is the
fourth blob.

**Errata (FU-191 §6 / FU-192 §3):** those docs gloss `[rec+8]`'s offset as
`[rec+0x8A]>>24` and the rest as `[rec+0x8D]>>24`. Both are the **same byte**
(`dword[rec+0x8A]>>24` = byte at `rec+0x8D`); the fresh disassembly loads it as
a **signed byte** (`MOVSX CX,byte[rec+0x8D]`), not a dword shift. The
`form*6` term is `(int8) byte[[team+0x7AE]] * 6`, not a per-record value — all
records of a team share the same 6·form base; only `idx` varies per record.

---

## 2. The metadata source — a **runtime art-resource table**, not static data

`FUN_0006D920` does not embed the metadata; it points into
`FUN_0004C384(k)`. Fresh `decompile_function 0x4C384`:
`FUN_0004c384(param) { FUN_0004afb8(param & 0xffff); }` — a tail call. Fresh
`decompile_function 0x4AFB8`: `return (&DAT_0014bfc0)[param];` — **a dword
pointer table at `0x14BFC0`**.

Fresh `read_memory 0x14BFC0` (64 B) = **all zero**; fresh `list_segments` puts
`0x14BFC0` in `.object4` (`0x100000..0x16AA4F`, initialized, writable) ⇒ a BSS
slot zero at load, **runtime-populated**. Fresh `get_xrefs_to 0x14BFC0` = **5**:
the two readers (`0x4AFB8`, `0x4AFC0`) and the **writer `FUN_0004A6BC`**
(`0x4A711` WRITE, `0x4A807` the teardown PARAM).

Fresh `decompile_function` + `disassemble_function 0x4A6BC` (117 insns): the
populator loads `DAT_0010736C = FUN_0004a344("art/gameart0.pvi")` (fresh
`read_memory 0x101C6C` = `"art/gameart0.pvi"`), then four loops fill
`DAT_0014BFC0[0..0x3E]` by `FUN_00099e9f(name-buf, "<fmt>", name[k])` +
`FUN_000A2614(...)` (the resource load). Fresh `read_memory` at the exact
pushed addresses gives the format strings:

```
0x101C80 "%s.fmt"   (loop 1: entries 0..0x77/4 = 0..29)
0x101C88 "t%s.dat"  (loop 2: entries 30..38)
0x101C94 "%s.%s"    (loop 3: entries 39..55; arg1 = 0x101C90 "lfsh")
0x101C9C "%s.qfs"   (loop 4: entries 56..62)
```

Each loop's `%s` is `[0x107370 + k*4]`, a name pointer (fresh `read_memory
0x107370` = 48 B, 12 pointers; fresh `read_memory 0x101A30` = the name strings
`"352ko"`, `"352pl"`, `"352pk"`, `"352ps"`, `"freekick"`, …). The teardown arm
(`FUN_0004A6BC(0)`) frees and zeroes `0x3f` dwords (`0x4A81E MOV ECX,0x3f`) —
63 entries.

**The four metadata slots are all loop 1.** `id ∈ {0..4}` ⇒ `id*6+{0,1,2,4}`
maxes at `24+4 = 28 < 30`, so every slot the writer reads is loaded by the
`"%s.fmt"` loop — the **`.fmt` family** (id 0: `352ko.fmt`, `352pl.fmt`,
`352pk.fmt`, `freekick.fmt`, slot 3 `352ps.fmt` skipped). The `.dat`/`.%s`/
`.qfs` loops (indices ≥ 30) are never read by `FUN_0006D920`. (Erratum to this
slice's own first draft: an earlier reading misplaced the loop-1 format as
`"lay%s.fmt"`; the fresh `read_memory 0x101C80` is `"%s.fmt"`. This matches
FU-199 §5.1's `sprintf("%s.fmt", table[n])`.)

**Finding:** the OL-13 metadata pointers source from the **`gameart0` `.fmt`
family** (loader-loaded at runtime into `DAT_0014BFC0`), **not** from static LE
data. `DAT_0014BFC0` and the four pointer formulas are the OL-13 producer. The
engine `grep`s (`14bfc0`, `4a6bc`, `4afb8`, `4c384`) find **no** model of this
table — the metadata is entirely unported.

**Only `[rec+8]` is partly modelled today**, as the formation `.fmt` file:
`fifa96_scene_formation` (`include/fifa96_loader/fifa96_scene.h`) holds the 11×4
= 44-byte `<name>.fmt` blob (e.g. `352ko.fmt`) and its header comment records the exact
native producer `[rec+8] = FUN_0004AFB8(6*formation_id) + (byte[rec+0x8D] << 2)`
— the fresh §1 derivation. `fifa96_scene_formation_place`
(`fifa96_scene.c:181`) is the `0x6E1D0` row-01/12 consumer.

### 2.1 The pre-pass `FUN_0006D9C4` (fresh `decompile_function`)

`FUN_0006d9c4(team, form)`: `[team+0x7AE] = &DAT_0011033a + form*0x1d`
(fresh `read_memory 0x11033A` = the 5-row × 0x1D formation layout table, byte 0
of each row = the formation id), then **4 groups**; per group
`count = (char)group[1]`, entries `group[2+k]` = a record index →
`rec = team + idx*0xB2`; `byte[rec+0x90] = group[0]` (the role byte); and
`FUN_0006D920(rec)`. So the pre-pass stages **all four metadata pointers** per
listed record before the walk. Fresh `get_xrefs_to 0x6D9C4` = **2** (`0x8CE87`
`FUN_0008CE78`, `0x8D0B1` the walk prologue, *cross-cited* FU-191 §5).

---

## 3. The per-row gate + exact metadata read (fresh disassembly)

The engine gate is the NULL check inside each ported body
(`fifa96_match_handlers.c`, the R48 brief's sites). Fresh bodies:

### 3.1 Rows `0x02`/`0x15` — body `0x6DCC8` (T1-owned; `fifa96_match_phase_02`)

Fresh `get_function_by_address 0x6DCC8` = **defined**
(`FUN_0006dcc8`, `0x6DCC8..0x6DD3B`); fresh `disassemble_bytes` (42 insns):
```
0x6DCCD MOV EAX,[EAX+0xC]                   ; meta = [rec+0xC]
0x6DCD0 TEST BX,BX ; JGE explicit
  (BX<0) 0x6DCD7 BL=[team+0x826]; 0x6DCE3 EBX=word[0x1577DE+side*4]>>16  ; per-side default idx
  (BX>=0) 0x6DCEF MOVSX EBX,BX              ; explicit idx
0x6DCF2 ADD EBX,EBX ; 0x6DCF4 EAX += EBX   ; meta + idx*2
0x6DCF6 MOVSX BX,byte[EAX]      ; target[0] = bx*0x26   (SHL5+SHL2+add)
0x6DD0A MOVSX BX,byte[EAX+1]    ; target[2] = bx*0x21
0x6DD18..0x6DD24 side!=0 -> NEG both  ; 0x6DD29 target[1]=0
```
**Exact read:** `[rec+0xC]` (two signed bytes at `idx*2`), the per-side default
`word[0x1577DE+side*4]>>16` (the `BX<0` arm), the side cell `[team+0x826]`.
Engine gate (`handlers.c:4594`): `meta = rec->phase_meta; if (meta == NULL)
return;` — `phase_meta` is the `[rec+0xC]` field (`entities.h:179`). The engine
also stages `phase_dir_default[side]` 0 for the `edge<0` default
(`handlers.c:4590`) — the `[0x1577DE]` cell is **also** unported.

### 3.2 Rows `0x08`/`0x09` — body `0x6DD6C` (wrapper → `0x6DCC8`)

Fresh `get_function_by_address 0x6DD6C` = **not defined** (raw code). Fresh
`disassemble_bytes 0x6DD6C..0x6DD98` (*cross-cited* FU-192 §2.2 for the body;
fresh here): `idx = (side == ([0x157AAC]>>24)) ? 0xBC : 0x6F; CALL 0x6DCC8`.
**Exact read:** via `0x6DCC8` → `[rec+0xC]` (`rec->phase_meta`), plus the
controlled-side byte `[0x157AAC]>>24` (`phase_machine.side_controlled`). Engine
gate (`handlers.c:4748` → `match_phase_dcc8_apply:4594`): `phase_meta == NULL`.

### 3.3 Row `0x06` — body `0x6DD9C` (`fifa96_match_phase_06`)

Fresh `get_function_by_address 0x6DD9C` = **not defined**. Fresh
`disassemble_bytes` (53 insns):
```
0x6DDA2 CL=[team+0x826] ; 0x6DDA8 MOV EAX,[EAX+0x10]      ; meta = [rec+0x10]
0x6DDAD side!=0 -> 0x6DDE1: idx = ([0x157754]<0)          ; else idx = ([0x157754]>0)
0x6DDC2 EAX += idx*4                                        ; meta + idx*4
0x6DDC4 MOVSX ECX,word[EAX]      ; target[0] = w + (w>>2)   (W+(W>>2))
0x6DDD0 MOV EAX,[EAX]; SAR 0x10  ; target[2] = d + (d>>4)   (D+(D>>4))
0x6DDFE side!=0 -> NEG both
0x6DE15 [0x157754]>0 ? SUB target[2],0x60 : ADD target[2],0x60
0x6DE28 target[1]=0
```
**Exact read:** `[rec+0x10]` (a dword pair at `idx*4`), the sign cell
`[0x157754]`, the side cell `[team+0x826]`. Engine gate (`handlers.c:4729`):
`p = rec->phase_meta10; if (p == NULL) return;` — `phase_meta10` is `[rec+0x10]`
(`entities.h:186`). `cell_157754` staged 0 (`handlers.c:4715`).

### 3.4 Rows `0x03`/`0x04`/`0x07` — body `0x6DE44` (`fifa96_match_phase_04`)

Fresh `get_function_by_address 0x6DE44` = **not defined**. Fresh
`disassemble_bytes` (89 insns): the `DX ≥ 0` arm reads
```
0x6DE57 EBX=[0x157AAC]>>24 ; SHL2                          ; gside*4
0x6DE65 DX = word[0x1577D8 + gside*4]                       ; the per-side selector (sign = arm)
0x6DE75 EDX = dword[0x1577D6 + gside*4] >> 16              ; idx
0x6DE7E EAX = [rec+0x14]                                    ; meta14
0x6DE81 SHL EDX,3 ; 0x6DE88 EAX += idx*8
0x6DE8A DX = (int8)[0x157AAF] ; CMP DX,side ; 0x6DE97 JZ -> else EAX += 4   ; the +4 off
0x6DEA2 MOVSX EBP,word[EAX]     ; target[0] = W + (W>>2)
0x6DEAE MOV EAX,[EAX]; SAR 0x10 ; target[2] = D + (D>>4)
0x6DEC2 side!=0 -> NEG both     ; 0x6DF3C target[1]=0
```
The `DX < 0` arm (`0x6DEE5..0x6DF3A`, base `[0x15774C]` ±0x1E0, `CALL 0x6D870`,
`CALL 0x6DCC8`) is *cross-cited* FU-192 §2.4/FU192-L3. **Exact read:**
`[rec+0x14]` (a dword pair at `idx*8 + off*4`), the side tables
`[0x1577D6+gside*4]`/`[0x1577D8+gside*4]`, the side byte `[0x157AAF]`
(= `[0x157AAC]>>24` after the phase writer's side store), the cell
`[0x157AAC]>>24`. Engine gate (`handlers.c:4668`): `p = rec->phase_meta14; if (p
== NULL) return;` — `phase_meta14` is `[rec+0x14]` (`entities.h:187`). The DX<0
arm is short-circuited by `sel_1577d8[] = {0,0}` (`handlers.c:4648`, staged 0 —
the real `[0x1577D8]` is runtime).

### 3.5 The metadata/index cells are all runtime BSS

Fresh `read_memory`: `0x1577D0` (32 B, covers `[0x1577D6]`/`[0x1577D8]`/
`[0x1577DE]`) = **zero**; `0x157750` 16 B (`[0x157754]`) = **zero**; `0x157AA8`
16 B (`[0x157AAC]`/`[0x157AAF]`) = **zero**. So even with the four metadata
pointers set, the engine's **index selectors** for rows `0x02`/`0x15`
(`[0x1577DE]`), `0x06` (`[0x157754]`) and `0x03/0x04/0x07` (`[0x1577D6]`) are
staged 0 → the body would run with `idx = 0` (not the native default). This is a
second OL-13 half (leg FU200-L3).

---

## 4. The engine gate sites (first-hand source, the R48 brief's lines)

`src/fifa96_engine/fifa96_match_handlers.c`:

* `:4594` — `match_phase_dcc8_apply`: `meta = rec->phase_meta; if (meta == NULL) return;`
  the `[rec+0xC]` gate (rows `0x02`/`0x15` and, via the `0x6DD6C` wrapper,
  `0x08`/`0x09`).
* `:4603` — `fifa96_match_phase_02`: the `phase_walk_active`/`phase_walk_rec`
  guard, then `match_phase_dcc8_apply(mr, rec, mr->phase_walk_edge)`.
* `:4668` — `fifa96_match_phase_04`: `p = rec->phase_meta14; if (p == NULL) return;`
  the `[rec+0x14]` gate (rows `0x03`/`0x04`/`0x07`).
* `:4729` — `fifa96_match_phase_06`: `p = rec->phase_meta10; if (p == NULL) return;`
  the `[rec+0x10]` gate (row `0x06`).
* `:4748` — `fifa96_match_phase_08`: the `0x6DD6C` wrapper, then
  `match_phase_dcc8_apply` (the `phase_meta` gate).

The walk that dispatches them: `fifa96_match_run_record_phase_walk`
(`fifa96_match_run.c:2418`) → `fifa96_match_dispatch_phase_record`
(`handlers.c:5154`) → `row->fn(mr)`. The walk is invoked only inside
`match_run_write_phase` (`fifa96_match_run.c:2452`) **iff
`fifa96_match_phase_table[phase].fn != NULL`**. The walk does **not** stage the
metadata (contrast the native `0x8D107 CALL 0x6D920`); the only writer of
`rec->phase_meta*` in the engine today is a test fixture (`entities.h:178`
"`NULL` always means unported").

---

## 5. Does R47 make each row "genuinely live"?

"Genuinely live" = **dispatched** (the walk fires the phase on the tape) **and**
**un-gated** (the metadata field is non-NULL, so the body runs).

* **Un-gate.** Each row's only structural gate is the NULL check on its field
  (§3). So R47 (the T1 setup writing `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]`,
  plan Task 1) opens the gate for: `phase_meta` → rows `0x02`/`0x08`/`0x09`/
  `0x15`; `phase_meta10` → row `0x06`; `phase_meta14` → rows `0x03`/`0x04`/
  `0x07`. **But** the pointer must point at the *loader blob data* (§2) — R47
  must supply the `gameart0` `.fmt`/`.dat`/`.qfs` bytes, or stage an equivalent
  finite table. If R47 stages NULL (as today) nothing changes. **Unproven from
  R47's output (not landed) — leg FU200-L1.**
* **Dispatch.** The walk fires only for phases written through
  `match_run_write_phase` with a wired row. On the M2 tape (§6) that is
  `0x02`/`0x04`/`0x15`; `0x03`/`0x06`/`0x07`/`0x08`/`0x09` are never written.
* **Native-faithfulness.** Even un-gated, the index selectors are staged 0
  (§3.5), so the body runs at `idx = 0`, not the native default → a live but
  **non-faithful** result until the selector cells land. **Leg FU200-L3.**

So R47 **can** only make the *on-tape* rows genuinely live; the five off-tape
rows stay honestly dormant regardless.

### 5.1 Reconciliation with FU-199 (R47, landed mid-session)

R47's independent result (its §5.1/§5.2) is **identical** to this slice's §1/§2:
the pointers are written at match begin by `FUN_0006D920` via
`FUN_0007412C → FUN_0008CE78 → FUN_0006D9C4 → FUN_0006D920`, keyed by the
formation id, from `FUN_0004C384(id*6 + {0,1,2,4})` = the `.fmt` family; and R47
§7 explicitly says the landing site is `match_run_record_meta_bind` at match
begin, staging `rec->phase_meta`/`phase_meta10`/`phase_meta14`. R47 §8's
per-write table matches §6: `[rec+0xC]` (rows 0x02/0x15) and `[rec+0x14]`
(row 0x04) are **NOT tape-dormant**, `[rec+0x10]`/`[rec+8]` are dormant — and R47
concludes "landing the metadata producer **moves M2** and must carry a documented
re-pin; it also **un-gates phase rows 0x02/0x15**". So the §6 on-tape risk is
**confirmed by R47**, and FU200-L1 is closed by R47 §7 (the concrete bind rec).
The remaining R48 legs (FU200-L2 the cmp, FU200-L3 the index selectors, L4–L7)
stand — R47 §7.2 notes only the pointer staging, not the index cells or the
`0x6D870` arm.

---

## 6. Tape-dormancy verdict (per row)

Grounds: the M2 tape's phase census (*cross-cited* FU-192 §4, fresh
`grep 'state='` 792 lines): visited phases = `{0x00, 0x02, 0x04, 0x13, 0x14,
0x15}`. Fresh `match_run_write_phase` call sites (`fifa96_match_run.c`
`:2392,2447,2628,2650,2817,3062,5229,5302,5335`, *cross-cited* FU-191 §10) and
the M2 test's own forcing (`tests/test_engine_m2.c`):

* phase `0x13`/`0x14` are **forced** via `fifa96_match_state_set_phase(&mr->state,
  0x13|0x14)` (`:1954`/`:1966`) — **bypassing the walk**;
* the first phase-`2` window is forced via `fifa96_match_state_set_phase(&mr->state,
  2)` (`:2130`) — **bypassing the walk**; the post-restart phase `2` comes from
  situation `0xB` (`fifa96_action_phase_situations[0xB].phase = 2`,
  `fifa96_action_handlers.c:1145`) through `match_run_situation_side` →
  `match_run_write_phase` (`run.c:2713`) — the walk **fires**;
* phase `0x04` comes from situation `3` (`.phase = 4`, `action_handlers.c:1136`,
  the corner, `corner_increment = 1`) → `match_run_write_phase` — the walk
  **fires**;
* phase `0x15` is written by the act-9 producer `match_run_write_phase(mr, 0x15u)`
  (`run.c:2735`) — the walk **fires**.

**Render-visibility of the R47 metadata write in isolation:** render-invisible.
`match_run_scene_stage` (`run.c:1595..1607`) reads only positions
`+0x59/+0x5D/+0x61`, `anim_id +0x28`, `frame +0x3D`, `hidden +0x9A`; the four
metadata pointers and `[rec+0x1C]` are absent, and the staged metadata is written
only by `FUN_0006D920`/the walk. (The *consequence* is not neutral — below.)

| row | body | field | on-tape dispatch? | R47 un-gate? | genuinely live | **tape-dormancy verdict** |
|---|---|---|---|---|---|---|
| `0x02` | `0x6DCC8` | `[rec+0xC]` | **yes** (post-restart sit-0xB) | yes | **yes** | **NOT render-invisible** — the body writes the target triple |
| `0x03` | `0x6DE44` | `[rec+0x14]` | no (phase 3 never written) | yes | no | **dormant by non-dispatch** |
| `0x04` | `0x6DE44` | `[rec+0x14]` | **yes** (303 frames, sit-3 corner) | yes | **yes** | **NOT render-invisible** |
| `0x06` | `0x6DD9C` | `[rec+0x10]` | no (phase 6 never written on-tape) | yes | no | **dormant by non-dispatch** |
| `0x07` | `0x6DE44` | `[rec+0x14]` | no (phase 7 never written on-tape) | yes | no | **dormant by non-dispatch** |
| `0x08` | `0x6DD6C` | `[rec+0xC]` | no (phase 8 never written) | yes | no | **dormant by non-dispatch** |
| `0x09` | `0x6DD6C` | `[rec+0xC]` | no (phase 9 never written) | yes | no | **dormant by non-dispatch** |
| `0x15` | `0x6DCC8` | `[rec+0xC]` | **yes** (100 frames, act-9) | yes | **yes** | **NOT render-invisible** |

**Why the on-tape rows are not neutral:** the un-gated bodies write the target
triple `[rec+0x4D/+0x51/+0x55]` (rule `target_x/target_z`). The target triple
**is consumed**: `fifa96_match_entities_place` (`entities.c:324`) commits
`pos := target`; the staging↔pool sync `e->target_x = r->target_x`
(`run.c:1289`) feeds the FU-77 shared mover, which "integrate[s] into velocity
and position" per the M2 tape's own v4.1 provenance
(`tests/test_engine_m2.c:143..160`). So a one-shot target write at the
`0x02`/`0x04`/`0x15` transitions can propagate to positions → the render. The
current tape-green state relies **precisely** on the metadata gate keeping these
bodies no-ops (the phase table rows `0x02`/`0x04`/`0x15` already say so,
`handlers.c:5061/5065/5099`). This is a **structural risk, not statically
proven neutral** (leg FU200-L2). The forced phase-`2` entry (`set_phase`)
bypasses the walk, so the *first* phase-2 window is not a walk surface; the
post-restart phase-2 window is.

The five off-tape rows are the clean class: the walk never fires their phase, so
opening the gate cannot move M2 (nothing dispatches them) — the FU-192 §4
"tape-dormant by construction" result, re-confirmed here via the fresh
phase-census + write-site analysis.

---

## 7. T1/T5 un-gate rec per row

| row | rec |
|---|---|
| `0x03`, `0x06`, `0x07`, `0x08`, `0x09` | un-gate freely: never dispatched on the tape ⇒ provably tape-neutral. Wire the R47 metadata write for their fields and count them the moment the pointer data exists. |
| `0x04`, `0x15`, `0x02` (the post-restart window) | **do not un-gate blind.** The R47 metadata write itself is render-invisible (render reads no metadata point), but it enables a render-visible **target** write on an on-tape phase. Either (a) prove the target write render-neutral with an M2 `cmp` (re-pin authorized only if native-faithful), or (b) keep these three rows gated (their ports stay exported for unit tests, the `0x6DCC8`/`0x6DE44` bodies already are) and count only the off-tape five. **The controller rules.** |

**Census note.** R48 is read-only and changes no engine file; the census stays
`48/80 = 60%` (31 action + 17 phase; the phase-17 T3 cohort already counts
`0x00/0x03/0x04/0x05/0x06/0x07/0x08/0x09/0x0A/0x0C/0x0F/0x15`,
`docs/ENGINE.md:264..266`). No README/ENGINE edit is warranted by this slice.

---

## 8. Numbered legs

1. **FU200-L1 (owner T1) — the metadata pointer *data* is unported.** The four
   fields point into `DAT_0014BFC0` (§2), a runtime-loaded `gameart0` `.fmt`
   table populated by `FUN_0004A6BC`; only `[rec+8]`'s `.fmt` (44 B) is modelled
   (`fifa96_scene_formation`). R47 (FU-199 §7) landed the **rec** (a
   `match_run_record_meta_bind` loading `FUN_0004C384(id*6+{0,1,2,4})` over
   `GAMEART0.PVI`) but not the loader itself; whether the three blob loads are
   ported (or staged as a finite stand-in) is T1's open half.
2. **FU200-L2 (owner T1/T5) — the on-tape un-gate is not proven render-invisible.**
   Rows `0x02`/`0x04`/`0x15` are dispatched on the tape and, if un-gated, write
   the target triple, which the mover/place chain consumes
   (`entities.c:324`, `run.c:1289`, the M2 v4.1 provenance). Pin with an M2 `cmp`
   before wiring; the metadata write alone is render-invisible.
3. **FU200-L3 (owner R47/T1) — the index selectors are unported.** Even with the
   metadata set, `[0x1577DE]` (rows `0x02`/`0x15`), `[0x157754]` (row `0x06`),
   `[0x1577D6]`/`[0x1577D8]` (rows `0x03/0x04/0x07`) are staged 0 (§3.5), so the
   body runs at `idx = 0`, not the native default. "Genuinely live" ⇒ also port
   these cells.
4. **FU200-L4 — the `0x6D870` cell-map arm (`0x6DE44` `DX<0`).** Base
   `[0x15774C]` ±0x1E0, `CALL 0x6D870`/`0x6DCC8`; FU192-L3. The M2 phase 4 tape
   takes the `DX≥0` arm (the selector is staged 0), so this arm is off-tape — but
   a faithful port needs it.
5. **FU200-L5 — the phase-`2` walk surface.** The M2 tape's first phase-2 window
   is forced via `fifa96_match_state_set_phase` (`test_engine_m2.c:2130`) and so
   bypasses the walk; only the post-restart phase-2 (sit-0xB) fires it. Whether
   the *natural* run (the `run_natural_probe` path) writes phase 2 through the
   walk with the same team order is unproven (a separate run, not the golden).
6. **FU200-L6 — the `form*6+3` slot.** `FUN_0006D920` reads `form*6+{0,1,2,4}`,
   skipping `+3`. What `DAT_0014BFC0[form*6+3]` holds (an unused `.fmt`) and
   whether any other reader uses it is not derived here.
7. **FU200-L7 — the `FUN_0006D9C4` `[rec+0x90]` role byte.** The pre-pass writes
   `byte[rec+0x90] = group[0]`; the pool has no `[rec+0x90]` model (FU191-L1).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info`, `list_segments`, `get_function_by_address` —
  `0x6DCC8` (defined `0x6DCC8..0x6DD3B`), `0x6DE44`/`0x6DD9C`/`0x6DD6C`
  (not defined — raw code), `0x4AFB8` (defined).
* `decompile_function` — `0x6D920`, `0x6D9C4`, `0x4C384`, `0x4AFB8`, `0x4A6BC`,
  `0x4AFC0`.
* `disassemble_function` — `0x6D920` (51), `0x4A6BC` (117).
* `disassemble_bytes` — `0x6DCC8` (115 B/42 insns), `0x6DD9C` (149 B/53),
  `0x6DE44` (262 B/89).
* `read_memory` — `0x14BFC0` (64 B, zero), `0x11033A` (96 B, the formation
  table), `0x1577D0` (32 B, zero), `0x157750` (16 B, zero), `0x157AA8` (16 B,
  zero), `0x110794` (92 B, the phase table), `0x101C6C` (64 B, the resource-name
  strings).
* `get_xrefs_to` — `0x6D920` (4), `0x6D9C4` (2), `0x14BFC0` (5), `0x4A6BC` (1).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(4560..4763 the gated bodies + `4670`/`4648`/`4715`/`4590` the staged cells;
4964..5126 the phase table; 5128..5179 the dispatch seam),
`src/fifa96_engine/fifa96_match_run.c` (2411..2458 the walk + `match_run_write_phase`,
2700..2746 the situation/act-9 writers, 5405..5540 the ref-apply/restart writers),
`include/fifa96_engine/fifa96_match_entities.h` (171..188 the `phase_meta*`
fields), `src/fifa96_engine/fifa96_match_entities.c` (324..340 the place commit),
`include/fifa96_loader/fifa96_scene.h` (the formation header comment),
`src/fifa96_loader/fifa96_scene.c` (181..207 `fifa96_scene_formation_place`),
`src/fifa96_loader/fifa96_action_handlers.c` (1120..1161 the situation table),
`tests/test_engine_m2.c` (1914..2160 the directive forcing, 2680..2749 the phase
observations, 140..160 the v4.1 target-integration provenance). Docs cross-cited:
FU-89, FU-177, FU-190, FU-191, FU-192, FU-198; AGENTS.md census rule.

No writes beyond this slice file and the workspace report.
