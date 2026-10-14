# FU-254: the anim/sink producers — OL-27/OL-52 sinks, the OL-42 installer arm, and the FU-84 coupling

W24-7 recon slice (Task 7). Read-only derivation of the two non-zero-anim
producers that sit outside the wired action bodies: the event append sink
`FUN_00092820`/`FUN_000928F0` (OL-27) and the animation selector
`FUN_0006E598` (OL-52) with its installer caller arm (OL-42); plus the
`FUN_00036C70` scene staging and the FU-84 row/bank it feeds.

Result in one line: **`FUN_00092820(rec, code)` stores the code byte to the
flat `0x15B650` sink and, only when the fresh `0x110F1C` table byte for the
sign-extended code has bit 0 set, fills `0x15B651` (the `FUN_000CB2A4`
`[0x112E88]` tick), `0x15B655` (the actor) and the three dwords `0x15B659..`
from the flat `0x157758` track triple; its twin `FUN_000928F0` appends the
same four fields as 21-byte (`0x15`) entries into the 25-slot ring at
`0x15B440` (index `0x15B665`, 1-based `(i+1)%25`) unless the sink code is
`0x26`, then clears the sink; the selector `FUN_0006E598(rec, id16, frame16)`
clamps the id to `0..0x6E`, stores row `[0x57588]+id*9` at `[rec+0x28]`, fans
the row flags to `+0x43/+0x44/+0x45/+0x46/+0x3F`, resolves the frame via
`FUN_0006E490` and zeroes `[rec+0x32]`; its keep/reroll arm (id arg 0, phase
!= 2) is `{0, 0x62, 0x65, 0}` selected by `rng & 3` through the CS jump table
at `0x6E588` — NOT discarded; the installer arm `FUN_0007D9A4`
(`0x7D9DC..0x7DA1F`) calls it with **id `0x0F`, frame 0, phase ∉ {2,0xA,0xF}**
when `[rec+0x98]` is set and code != `0x0C`, then clears `+0x98`; and
`FUN_00036C70` stages `byte[[rec+0x28]]`/`byte[rec+0x3D]` to the render slot
bytes `0x155C20`/`0x155C37`, which the FU-84 row lookup turns into the `+8`
sprite bank for the FU-85 resolver.**

## Method

* Authoritative program **`/FIFA96.EXE`**, explicit in every Ghidra MCP call
  (`get_current_program_info` = `/FIFA96.EXE`, LE, watcom). **Read-only**: no
  rename/comment/label/function/script/project save.
* Fresh tool calls this slice: `disassemble_bytes` `0x92820` (70 B), `0x928F0`
  (180 B), `0x6E598` (280 B), `0x6E6AF` (100 B), `0x7D9DC` (72 B), `0x36C70`
  (240 B); `read_memory` `0x110F1C` (32 B), `0x6E588` (16 B); `get_xrefs_to`
  `0x92820` (33), `0x928F0` (16), `0x6E598` (84), `0x36C70` (7), `0xCB2A4`
  (118).
* **Address mapping.** In `/FIFA96.EXE` the loader fixups are applied, so data
  immediates render resolved (the sink `0x15B650`, ring `0x15B440`, triple
  `0x157758`); code addresses equal their flat forms (FU-141 §Method). The
  CS-prefixed selector table `0x6E588` reads as flat `0x0006E6xx` targets.
* Engine source read first-hand: `src/fifa96_engine/fifa96_match_entities.c`,
  `fifa96_match_handlers.c`, `fifa96_match_run.c`;
  `src/fifa96_loader/fifa96_arm_helpers.c`, `fifa96_event_queue.c`.
* No engine/loader/test/git edit; draft + report only.

## 1. `FUN_00092820` (`0x92820..0x92861`, 29 insns) — the event sink store (OL-27)

Fresh `disassemble_bytes 0x92820`:

```
0x92825  EBP = EAX                        ; EBP = actor/rec
0x92827  EAX = (int8)DL                    ; the code byte, sign-extended
0x9282a  AL = byte[EAX + 0x110f1c]         ; eligibility table
0x92830  AL &= 1
0x92835  byte[0x15b650] = DL               ; sink code (always)
0x9283d  if ((EAX & 1) == 0) goto 0x9285c  ; no-fill codes skip the extras
0x9283f  EDI = 0x15b659                     ; sink vector dest
0x92844  ESI = 0x157758                     ; the track triple source
0x92849  CALL 0xcb2a4                        ; stamp = [0x112E88] (getter)
0x9284e  [0x15b651] = EAX                    ; sink stamp
0x92853  MOVSD x3 (EDI, ESI)                 ; 0x157758 triple -> 0x15b659
0x92856  [0x15b655] = EBP                    ; sink actor
0x9285c  POP x5; RET
```

Inputs `EAX = actor`, `DL = code`. Produces the flat sink block
`{+0 code (0x15B650), +1 stamp (0x15B651), +5 actor (0x15B655),
+9 3-dword vector (0x15B659)}`; the extras are filled only when
`table[(int8)code] & 1`. The `0x15B650` byte is the "last event code" cell the
ring twin tests.

Mode is **33 `UNCONDITIONAL_CALL` sites** (fresh `get_xrefs_to`, total 33):
the possession/duel cluster (`0x7A490` ball staging `0x7AB36`/`0x7AD16`,
`0x7F7E0` fallback ×8, row 05 `0x7F4CB`, row 08 `0x81401`, `0x82A4A`),
`FUN_0006FFC0`/`FUN_000700F4`/`FUN_00070544`/`0x736AC` camera/track,
`FUN_00071DF4`, `0x752A9`, `0x75DB0`/`0x75F1A`/`0x75FEB`, `FUN_0007AE70`,
`0x773E3`, `0x77DB6`, `0x7DF3C`, `0x7E14D`, `0x7EFC6`/`0x7F0C8`, `0x80DB1`.
The eligibility table fresh `read_memory 0x110F1C` = `00 07×24 03 03 03 07 07
01 01` (indices 0..31); index 0 is the no-fill entry, `[0x26]/[0x27]` are the
`0x26` suppression pair (FU-139 §10.2).

## 2. `FUN_000928F0` (`0x928F0..0x92994`, 59 insns) — the append twin

Fresh `disassemble_bytes 0x928F0`:

```
0x928f5  EBP = EAX; BL = DL                ; actor, code
0x928f9  EAX = dword[0x15b64d] >> 24       ; byte[0x15b650] = sink code
0x92901  if (sink_code == 0x26) goto 0x92987
0x9290a  EDX = dword[0x15b662] >> 24       ; byte[0x15b665] = ring index
0x92913  EDX = EDX + 1; IDIV 0x19          ; (i+1) % 25
0x92920  byte[0x15b665] = DL               ; new 1-based index
0x92938  entry[i].code = BL                ; base 0x15b440, stride 0x15
0x92942  if ((table[(int8)code] & 1) == 0) goto 0x92987
0x92951  EBX = FUN_000cb2a4()              ; stamp
0x92976  entry[i].stamp  = EBX             ; +1
0x9297d  MOVSD x3 (entry+9, 0x157758)      ; the triple
0x92980  entry[i].actor  = EBP             ; +5
0x92987  byte[0x15b650] = 0                ; sink cleared on both paths
```

Ring base flat `0x15B440`, stride `0x15` (index `EDX + EDX*0x14`), 25 slots,
index byte `0x15B665`; `0x15B650` is the shared sink byte. **16 call sites**
(fresh `get_xrefs_to`), all inside `FUN_0007AE70` except one `FUN_0007131C`
(`0x7144A`).

Both are already ported (`fifa96_event_sink_store` / `fifa96_event_ring_append`,
`src/fifa96_loader/fifa96_event_queue.c`), but **no engine path drives them**
(OL-67). The port's `FIFA96_EVENT_RING_ELIGIBILITY 0x28` boundary and the
`0x26` suppression match the fresh bytes.

## 3. `FUN_0006E598` (`0x6E598..0x6E712`, 90 insns) — the selector (OL-52)

Fresh `disassemble_bytes 0x6E598`/`0x6E6AF` confirms FU-84 §1/§2 with one
material correction (§9). Inputs `EAX = rec`, `EDX = id16`, `EBX = frame16`.

```
0x6E59C  if (byte[rec+0x8D] == 0) {
0x6E5A9     side = ([0x57754] > 0); team = [rec];
0x6E5C8     if (side == [team+0x826]) {
               [0x57A6C] = (int16)EDX in {0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C}
            }
         }
0x6E608  if (([0x57A4A]>>24 == 2) || (int16)DX != 0) goto 0x6E68E
         ; id 0 and phase != 2 -> keep/reroll the CURRENT row byte
0x6E622  EDI = [rec+0x28]
0x6E627  if (EDI == 0) goto 0x6E659 (RNG)
0x6E629  CL = byte[EDI]
0x6E62D  EAX = (CL == 0 || CL in {0x62,0x63,0x64,0x65}) ? 0 : 1
0x6E655  if (EAX == 0) goto 0x6E687  ; EDX = byte[rec+0x28] (keep)
0x6E659  CALL 0x92AC8                 ; rng word
0x6E65e  EAX = rng & 3
0x6E66d  JMP CS:[EAX*4 + 0x6E588]
0x6E675  EDX = 0     ; case 0 and case 3 (table[0]=table[3]=0x6E675)
0x6E679  EDX = 0x62  ; case 1
0x6E680  EDX = 0x65  ; case 2
0x6E68E  if ((int16)DX < 0 || >= 0x6F) DX = 0
0x6E69D  EDX = [0x57588] + DX*9
0x6E6B1  [rec+0x28] = EDX
0x6E6B4  AL = byte[EDX+2]            ; row flags
0x6E6b7  [rec+0x45] = AL & 4 ; [rec+0x43] = AL & 2 ; [rec+0x44] = AL & 1
0x6E6d8  [rec+0x46] = 1
0x6E6e0  [rec+0x3F] = (AL&0x10) ? 2 : (AL&0x20) ? -2 : 0
0x6E6fe  EDX = &rec+0x28; EBX = (int16)BX; EAX = rec; CALL 0x6E490
0x6E70b  word[rec+0x32] = 0
```

**The reroll table** fresh `read_memory 0x6E588` (4 dwords) =
`0x0006E675, 0x0006E679, 0x0006E680, 0x0006E675`, i.e. the reroll output is
`{0, 0x62, 0x65, 0}` for `rng & 3 = {0,1,2,3}`. This is the second non-zero
anim-id producer outside the wired bodies (id `0x62`/`0x65`). FU-141 OL-52 /
FU-142 §OL-52 recorded the reroll as "`RNG & 3 -> {0, 0x62, 0x65}`"; the
fresh table additionally pins `case 3 -> 0`.

## 4. The installer anim arm (OL-42) — `FUN_0007D9A4` `0x7D9DC..0x7DA1F`

Fresh `disassemble_bytes 0x7D9DC`:

```
0x7D9DC  if (byte[rec+0x98] == 0) goto 0x7DA26   ; +0x98 latch
0x7D9E5  if ((int16)[ESP+2] == 0x0C) goto 0x7DA26  ; code != 0x0C
0x7D9F1  EAX = [0x157A4A] >> 24                   ; phase
0x7D9f9  if (phase == 2 || phase == 0x0A || phase == 0x0F) goto 0x7DA26
0x7DA08  EDX = 0x0F                                ; id
0x7DA0D  EAX = ESI (rec)
0x7DA0f  ECX = dword[rec+0x8B] >> 24               ; byte[rec+0x8E] (unused)
0x7DA15  EBX = 0                                   ; frame
0x7DA1A  CALL 0x6E598                              ; selector(rec, 0x0F, 0)
0x7DA1f  byte[rec+0x98] = 0
```

So a set `+0x98` on an install with code != `0x0C` and phase ∉ {2,`0xA`,`0xF`}
**produces anim_id `0x0F`** (id arg non-zero, so the selector takes the
`0x6E68E` path, not the reroll) and clears `+0x98`. The selector then writes
`[rec+0x28]` row `0x0F`, the `+0x43/+0x44/+0x45/+0x46/+0x3F` flags, the frame
resolve (`0x6E490`: `[rec+0x3D]=0`, `[rec+0x2C]`, `[rec+0x34]`, `[rec+0x36]`,
`[rec+0x3E]`) and `word[rec+0x32]=0`.

The engine twin `fifa96_match_entities_install` (`fifa96_match_entities.c:78`)
**keeps only the `+0x98` clear**; the `0x6E598(rec,0x0F,0)` call is unported —
this is the OL-42 gap. The other wired adapter,
`fifa96_match_run_record_anim_install` (`fifa96_match_run.c:1001`), models the
selector's store core (`0x6E68E..0x6E70B`) but is invoked only from the wired
rows, not from the installer.

## 5. `FUN_00036C70` staging + the FU-84 row/bank coupling

Fresh `disassemble_bytes 0x36C70` reconfirms the render staging:

```
0x36d44  ECX = dword[EDX+0x28]; CL = byte[ECX]      ; byte[[rec+0x28]]
0x36d49  byte[EAX + 0x155c20] = CL                   ; slot anim id
0x36d4f  CL = byte[EDX+0x3d]                         ; [rec+0x3D]
0x36d52  byte[EAX + 0x155c37] = CL                   ; slot frame index
0x36d3e  [EBP + 0x155bc4] = ((0x400 - (word[EDX+0x7b]>>16)) & 0x3FF) << 6
```

`EDX` = the `0xB2`-strided record, `EAX`/`EBP` = the slot ordinal. **7 call
sites** (fresh `get_xrefs_to`; match init `0x49528`, `0x49FF5`, `0x4A25F`,
`0x6448C`, `0x64EA9`, `0x53CAF`, `0x3706A`). The staged `0x155C20` id is the
`AL` and `0x155C37` the `DL` the draw paths feed `FUN_00078FAC` (FU-84 §7);
the resolver does `EDI = [0x57588] + id*9`, reads `byte[EDI+8]` (the row `+8`
sprite bank, observed `0..89`) and `frame_table = [EDI+4]` for the frame
record's `+4` sprite byte. So the FU-84 coupling is exactly:
**selector id -> row `[0x57588]+id*9` -> `+8` bank / `+4` frame table**; the
engine's `match_run_scene_stage` mirrors it (`fifa96_match_run.c:1657`
`e->bank_index = row.sprite_bank`), and the ball uses `appear_row` (FU-141
§1.4).

## 6. Producers outside the wired bodies (fresh census)

`get_xrefs_to 0x6E598` = **84 `UNCONDITIONAL_CALL` sites**. The ones reached
from the wired engine rows are covered by `fifa96_match_run_record_anim_install`
(handlers at `0x12`, `0x5E`, `0x11`, `0x4F`, `0x6A`/`0x55`, `0x15`/`0x03`,
`0x01`, `0x26`, the duel `choice`). The remainder are unwired arms:
`0x75000`, `0x756E8`/`0x75754`/`0x75CF7`/`0x76461`/`0x76863`/`0x76F91`/
`0x7756E`/`0x77682` (the `0x71xxx`/`0x75xxx`/`0x76xxx`/`0x77xxx` ball &
row arms), `FUN_0007131C` (7), `FUN_00088940` (2), `FUN_0006E8E8` (the aux
dispatch, 12), `0x7E472`/`0x7E4AD`/`0x7EE3C`, `0x80826`, `0x811AA`,
`0x818CB`/`0x8197B`/`0x819A0`/`0x81AFB`, `0x82BA5`, `0x845EA`/`0x846CC`,
`0x85335`/`0x85453`, `0x8695C`/`0x869F8`, `0x87603`/`0x87667`/`0x878F0`/
`0x87AFB`, `FUN_0007A490` (2), `FUN_0008CFAC` (`0x8CFD2`/`0x8CFED`), `0x8xxx`
set-piece/celebration arms. Every one writes a non-zero id or takes the id 0
reroll; none is wired (FU-137/FU-142 rows). This is the producer closure set.

## 7. Port shapes (fidelity)

| native | current port | shape to close |
|---|---|---|
| `FUN_00092820` sink store (`0x92825..0x9285c`) | `fifa96_event_sink_store` (ported, no driver) | wire a `fifa96_match_run` event call at the 33 call sites' derived sites; caller supplies actor/stamp/vector (OL-67) |
| `FUN_000928F0` ring append (`0x928f5..0x92994`) | `fifa96_event_ring_append` (ported, no driver) | driver + the `0x15B650==0x26` gate already modelled |
| `FUN_0006E598` store core `0x6E68E..0x6E70B` | `fifa96_match_run_record_anim_install` | unchanged (rows); the id/frame/flags/`+0x32` writes are correct |
| `FUN_0006E598` keep/reroll `0x6E622..0x6E680` | `fifa96_arm_anim_select(kind,row,&slot)` — non-special branch returns **0** | add the `rng & 3 -> {0,0x62,0x65,0}` table and the phase!=2 gate; the helper needs an RNG source (loader-level) — **fidelity gap: engine currently always yields 0 where the native yields 0x62/0x65** |
| installer arm `0x7D9DC..0x7DA1F` | `fifa96_match_entities_install` keeps only `+0x98 = 0` | add the `FUN_0006E598(rec,0x0F,0)` adapter (needs row tables + the entity anim_id/frame fields); pool already carries `anim_id`/`frame`/`row44` |
| `FUN_00036C70` staging | `match_run_scene_stage` (`fifa96_match_run.c:1605`) | seed `stage.anim_id/frame` from the pool; row lookup -> `bank_index = row.sprite_bank` — already correct |

## 8. Open legs

1. **FU254-L1 (OL-67 remainder, owner: event/UI task).** No engine path drives
   the two event sinks; the 33/16 call sites' derived producers (ball staging,
   possession, row arms) are not wired. The `0x157758` triple and the
   `[0x112E88]` stamp stay caller inputs.
2. **FU254-L2 (OL-52 remainder, owner: arm/anim task).** The selector's
   keep/reroll RNG branch (`0x6E659..0x6E680`) has no RNG plumbed into
   `fifa96_arm_anim_select`; the phase!=2 gate and the `0x6E588` table are
   unported. The engine's derived fallback 0 diverges from the native
   `{0,0x62,0x65}`.
3. **FU254-L3 (OL-42 remainder, owner: installer/entities task).** The
   installer `FUN_0006E598(rec,0x0F,0)` call is unported; the engine keeps
   only the `+0x98` clear, so an installed record never receives the native
   id `0x0F` from the installer arm.
4. **FU254-L4 (owner: arm/anim task).** The `0x6E598` record-side writes at
   the unwired arms' 84 call sites (the row/set-piece/celebration producers)
   remain outside the wired bodies; §6 enumerates them.
5. **FU254-L5 (owner: render task).** The `0x155C20`/`0x155C37` staged bytes
   feed `FUN_00078FAC`'s composite-id routing (FU-84 §7/OL leg 8) and the
   `0x57DE8` `+8`-bank animator records (FU-84 leg 1); their full downstream
   semantics remain legs of FU-84.
6. **FU254-L6 (owner: recon).** `FUN_0006E48C`'s conditional `[rec+0x3E]`
   write (called from the `FUN_00079B6C` kickoff tail) is cited by address
   only; not derived here.

## 9. Errata (prior docs)

* **FU-84 §1 (`0x6E598` keep/reroll) — corrected.** The text "the RNG result
  is discarded" is wrong. Fresh `disassemble_bytes 0x6E659..0x6E680` shows
  `CALL 0x92AC8; EAX = rng & 3; JMP CS:[EAX*4 + 0x6E588]`, and the fresh
  `read_memory 0x6E588` table `{0x6E675,0x6E679,0x6E680,0x6E675}` sets
  `EDX = {0,0x62,0x65,0}`. The RNG result **is** the rerolled id. This makes
  the reroll a non-zero-anim producer (id `0x62`/`0x65`); FU-84 §1's phrase is
  superseded (§3 above). FU-142 §OL-52's "`RNG & 3 -> {0,0x62,0x65}`" is
  consistent and is extended with `case 3 -> 0`.
* **FU-84 §1 method note (`0x6E724` RNG tail mis-decode).** Unchanged; the
  selector's own RNG tail here (`0x6E659..0x6E680`) is fully decoded this
  slice, so only `FUN_0006E724`'s tail remains the linear-decode leg.
* **FU-141 §3.1 `fifa96_match_entities_install` comment** — "the animation
  call is unported (OL-42)" confirmed first-hand at `0x7DA1A` (id `0x0F`), and
  the `ECX` argument is `dword[rec+0x8B]>>24` = `byte[rec+0x8E]` (the alias),
  which the selector overwrites at `0x6E5B3` and never reads.

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit):
`get_current_program_info`; `disassemble_bytes` `0x92820` (70 B), `0x928F0`
(180 B), `0x6E598` (280 B), `0x6E6AF` (100 B), `0x7D9DC` (72 B), `0x36C70`
(240 B); `read_memory` `0x110F1C` (32 B), `0x6E588` (16 B); `get_xrefs_to`
`0x92820` (33), `0x928F0` (16), `0x6E598` (84), `0x36C70` (7), `0xCB2A4`
(118). Engine/loader sources read: `src/fifa96_engine/fifa96_match_entities.c`,
`fifa96_match_handlers.c`, `fifa96_match_run.c`;
`src/fifa96_loader/fifa96_arm_helpers.c`, `fifa96_event_queue.c`;
`include/fifa96_engine/fifa96_match_entities.h`, `fifa96_match_run.h`;
`include/fifa96_loader/fifa96_event_queue.h`. Docs cross-referenced:
`docs/ENGINE.md` §OL-27/OL-52/OL-42; `docs/ghidra/FU84_animation.md`,
`FU139_action_cluster_b.md`, `FU141_action_cluster_de.md`,
`FU142_installer_arms_scope.md`. No writes: no rename/comment/label/function/
script/project save; no code/test/git/shared-doc change.
