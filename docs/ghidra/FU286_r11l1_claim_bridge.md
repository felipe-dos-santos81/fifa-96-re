# W26c-T6 / FU286 — R11-L1 claim anim-selection bridge (`FUN_0006E598`): port shape + GO/NO-GO

**Phase/Provenance:** M2 phase-26c recon-ahead, Task 6 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP, case-sensitive; scripts disabled; read tools only). Confirmed this session: `/FIFA96.EXE`, `watcom:LE:32:default`, image base 0, `.image` overlay present, 2706 functions. Every native address/byte below is a **fresh** `disassemble_bytes` / `disassemble_function` / `decompile_function` / `read_memory` / `get_xrefs_to` of *this* session unless marked *carried* (frozen `docs/ghidra/FU*` slice named). Alias traps applied (`dword[x]>>0x18` = byte at `x+3`; `>>0x10` = word at `x+2`; CALL rel32 verified). Engine facts cite the built tree by line (read-only). No rename/comment/label/function/script/save; `/FIFA96.EXE` untouched; no git command.

Read first (carried): `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md` §3.1/§3.2.

**Result in one line.** **GO — bounded port.** The `FUN_0006E598` *store core* is **already ported** as `fifa96_match_run_record_anim_install` (`src/fifa96_engine/fifa96_match_run.c:1002-1027`); the only gap is **wiring**: the keeper claim **produces** the selected anim id in `out->event` (`fifa96_keeper.c:698-700`) but `match_keeper_claim_apply` (`fifa96_match_handlers.c:476-533`) **never consumes it** — nothing in the engine reads `out->event` (grep: zero readers). Add the consume, plus R11-L2 (pause flag) and R11-L3 (sector offset-table read). Small.

## 1. Port shape (the bridge in ≤10 lines)

1. `FUN_0006E598(rec, id, frame)`: clamp `id` to `0..0x6E` (`0x6E68E..0x6E69B`); `row = *(0x157588) + id*9` (stride **9**); write `rec+0x28 := row`.
2. Derived row-flag bytes: `rec+0x44 := row[2]&1`, `rec+0x43 := row[2]&2`, `rec+0x45 := row[2]&4`, `rec+0x46 := 1`, `rec+0x3F := {2 | 0xFFFFFFFE | 0}` from `row[2]&0x10 / &0x20`.
3. `FUN_0006E490(rec, &rec+0x28, frame)`: clamp `frame` to `row[1]` (last frame), `frame_ptr = dword[row+4] + frame*5`, store frame index + frame word; `word[rec+0x32] := 0`.
4. Engine already implements 1-3 as `fifa96_match_run_record_anim_install` (id clamp, row lookup, `e->anim_id=id`, `e->row44=row.flags&1`, `word[+0x32]=0`).
5. Missing: at claim-apply, `record.anim_id := out->event` **via** `fifa96_match_run_record_anim_install(mr,e,out->event,0)` when `out->event != 0`.
6. Anim table `*(0x157588)` is **runtime-populated** (image bytes are zeros) → engine must keep using its compiled `match_run_anim_rows` (row `0x45` present, `fifa96_match_run.c:117`).
7. R11-L2: emit the place-pause flag (`1`) at the two row-`0x1E` place sites (`0x7559E`, `0x7595F`).
8. R11-L3: `record.place_offset_x/z := 0x10F334[sector] / 0x10F33C[sector]` (the adjacent 8-byte signed tables).
9. Stage 5 (`0x75D24 CMP byte[[rec+0x28]],0x45`) then passes; release `[0x157750]:=0x50` (`0x75D69`) is reachable.
10. Broader **R11-L4 remains NO-GO** (FU273) — this bridge is only its second gate, not the installer cluster.

## 2. Evidence table (fresh unless marked)

| # | Fact | Evidence (this session) |
|---|------|-------------------------|
| a1 | `FUN_0006E598(rec,id,frame)`: `row=*(0x157588)+id*9` | `disassemble_function 0x6E598`: `0x6E6A0 LEA EAX,[EDX*8+0]`; `0x6E6A7 ADD EAX,EDX` (→ `EDX*9`); `0x6E6A9 MOV EDX,[0x157588]`; `0x6E6B1 MOV [ESI+0x28],EDX` |
| a2 | stride 9, index clamp `0..0x6E` | id → `0x6E68E TEST DX,DX; JL 0x6E69B`; `0x6E696 CMP EAX,0x6f; JL`; `0x6E69B XOR EDX,EDX` |
| a3 | rec+0x28 = row pointer (byte[[rec+0x28]] = live id) | `0x6E6B1 MOV [ESI+0x28],EDX`; engine `handlers.c:464 s->anim_row = mr->record.anim_id` (OL-80 byte[[rec+0x28]]) |
| a4 | flag bytes `+0x44/43/45/46/3F` from `row[2]` | `0x6E6B4 MOV AL,[EDX+2]`; `0x6E6BC [ESI+0x45],AH(&4)`; `0x6E6C4 +0x43(&2)`; `0x6E6CC +0x44(&1)`; `0x6E6D8 +0x46:=1`; `0x6E6E0/E6F2/E6FB +0x3F` |
| a5 | index fallback when `id==0` | `0x6E608 MOV EAX,[0x157a4a]; SAR 0x18; CMP 2`; `0x6E659 CALL 0x92ac8; &3` → `0/0x62/0x65` |
| b1 | helper `FUN_0006E490` store core | `decompile_function 0x6E490`: frame clamp to `byte[row+1]`; `puVar3=dword[row+4]+frame*5`; `word[rec+0x32]=0` |
| b2 | `FUN_0006E490` fan-in (4 sites) | `get_xrefs_to 0x6E490` = `0x6E706` (in `FUN_0006E598`), `0x6FD72/0x6EE86/0x6F6D5` (in `FUN_0006E8E8`) |
| b3 | stage-4 selects id `{0x44,0x2F,0x45}` from vec_band tiers then calls the bridge | `disassemble_bytes 0x75CA6`: `CMP EAX,0x5a0→ECX=0x44`, `0x780→0x2f`, else `0x45`; `0x75CF7 CALL 0x6E598` (rec in EAX, id in DX, BX=0) |
| b4 | `FUN_0006E598` callers (84) incl. keeper region | `get_xrefs_to 0x6E598`: `0x75000,0x756E8,0x75754,0x75CF7,0x76461,...`; `0x75CF7` is the stage-4 site |
| c1 | R11-L2 place-pause flag `1` at `0x7559E` | `disassemble_bytes 0x75580`: `0x7559E PUSH 0x1` (`6a01`), after `place [0x157750]` staging |
| c2 | R11-L2 place-pause flag `1` at `0x7595F` | `disassemble_bytes 0x75940`: `0x7595F PUSH 0x1` (`6a01`); `0x75964 MOV [0x157754],EAX` |
| d1 | R11-L3 offset tables | `read_memory 0x10F334` (32B): `0x10F334[8]={0,1,1,1,0,-1,-1,-1}`, `0x10F33C[8]={1,1,0,-1,-1,-1,0,1}` |
| d2 | table refs | `get_xrefs_to 0x10F334` (26) / `0x10F33C` (26); engine models them `fifa96_match_locomotion_tables.c:25-26` (FU-161) |
| e1 | stage-5 frame gate | `disassemble_bytes 0x75D30`: `0x75D36 CMP EAX,0x3; 0x75D39 JL 0x760DF` (byte `[rec+0x3a]>>0x18`) |
| e2 | release pos copy | `0x75D42 MOV EDI,0x15774c; 0x75D47 LEA ESI,[ESI+0x59]; 0x75D52 MOVSD x3` (→ `0x15774C/50/54 := rec.pos`) |
| e3 | `camY:=0x50` byte-exact | `0x75D69 MOV [0x00157750],ECX` bytes `890d50771500`; `ECX` set `0x75D4A MOV ECX,0x50` |
| e4 | stage-5 gate byte | `disassemble_bytes 0x75D00`: `0x75D1A MOV EAX,[EAX+0x28]; 0x75D1D MOV AL,[EAX]; 0x75D24 CMP EAX,0x45; 0x75D27 JNZ 0x75DD8` |
| f1 | claim produces `out->event` | `fifa96_keeper.c:698-700`: `out->event = 0x44 / 0x2F / 0x45` (vec_band tiers) |
| f2 | stage-5 gate on `anim_row==0x45` | `fifa96_keeper.c:709-722`: `if (s->anim_row == 0x45u){ if(frame<3) exit; … cam_y=0x50; out->scenario=1; out->released=1; }` |
| f3 | apply never consumes `out->event` | `fifa96_match_handlers.c:476-533` read inline: no `out->event`; grep `out->event` in `src/` = only writer `fifa96_keeper.c`; **zero readers** |
| f4 | store core already ported | `fifa96_match_run.c:1002-1027` `fifa96_match_run_record_anim_install` (FU-159 §5.4 / FU-160 §1.4); row `0x45` in `match_run_anim_rows` at `:117` |
| f5 | claim reads offsets from record (unfed) | `handlers.c:467-468 s->offset_x/z = mr->record.place_offset_x/z`; only write `match_run.c:1363-1364 r->place_offset_x = e->place_offset_x` |

## 3. Why stage 5 is currently dead

`keeper.c:698-700` writes the selected anim id into `out->event`; `f2` shows stage 5 only
releases when `s->anim_row` (which `handlers.c:464` seeds from `record.anim_id`) equals `0x45`.
`f3` shows `match_keeper_claim_apply` ignores `out->event` entirely — `record.anim_id` is never
set from the claim path, so `s->anim_row` keeps its pre-claim value and the `0x75D24` gate
(`f2`/`e4`) fails. The store core that *would* set it (`f4`) is already ported but unreached from
the keeper apply. Hence the whole `0x75D52..0x75D69` release (`e2`/`e3`) is unreachable even when
row `0x1E` dispatches and the sit-5 emit has run.

## 4. GO / NO-GO

**GO — the R11-L1 bridge is a bounded port.** Rationale: (i) the `FUN_0006E598` store core is
already ported (`f4`); (ii) the anim table is runtime-populated so no native data extraction is
needed — the engine's compiled `match_run_anim_rows` substitutes (`a`, `f4`); (iii) R11-L2 is two
`PUSH 1` pause sites (`c1`, `c2`) and R11-L3 is two adjacent 8-byte sector tables (`d1`, `d2`)
already present engine-side; (iv) the fix is the one missing consume (`f3`→`f4`) plus the pause
flag and the sector offset read. **Scope:** R11-L1/L2/L3 only. The broader **R11-L4 remains
NO-GO** (FU273 §4, phase-scale) — this bridge is the second gate on the same release, not the
unported installer/predicate cluster. Open legs below are carried owners, not blockers.

## 5. Open legs

1. **FU286-L1 (carried).** `*(0x157588)` (the anim table) is image-zero — the 111 row bytes are
   runtime-populated and not statically derivable in scope. Engine substitutes `match_run_anim_rows`.
   Owner FU-159/FU-160.
2. **FU286-L2 (carried).** The exact mapping of `row[2]` flag bits → engine fields (`row44 = bits&1`,
   FU-159 §4.1) was not re-derived fresh; `a4` records the native writes only. Owner FU-159.
3. **FU286-L3.** `FUN_0006E330` (called by `FUN_0006E490` when `byte[&rec+0x28+7] != 0`) was not
   decompiled this session — its frame-word selector remains unverified. Owner FU-159.
4. **FU286-L4 (carried, FU273 §3.2).** No engine write of `record.place_offset_x/z` from
   `0x10F334/0x10F33C[sector]` exists (`f5`); the offsets are always 0 until R11-L3 lands. Owner FU-180/FU-162.
5. **FU286-L5.** The stage-4 nearest-mate gate (`0x75C86` `0x8DCD4`) was not re-walked; carried from FU-273.
6. **FU286-L6.** `FUN_0006E598`'s 84 callers beyond the keeper region were not exhausted; the keeper
   region sites (`0x75000/0x756E8/0x75754/0x75CF7/0x76461`) are the relevant ones. Owner: dispatch census.
