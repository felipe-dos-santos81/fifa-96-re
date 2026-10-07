# FU-135: front-end menu render — entry state table, asset path, row geometry, cursor blits

Task-9 evidence pass for the native-engine front-end renderer. The authored
target is the fixup-applied `/FIFA96.EXE` program in the open Ghidra project
(not the flat `/fifa96_le.bin`): its code operands already carry the
`+0x100000` data base of FU-129, so every address below is a real link
address as Ghidra displays it. All claims are read-only tool output from the
native program in this slice (`read_memory`, `disassemble_bytes`,
`decompile_function`, `search_instructions`, `get_xrefs_to`); no project
write was made.

Result in one line: **state 17 (`0x14AAA`) is the front-end entry screen and
builds two widget records through `FUN_00013BD8`; the front-end loop loads
`"options.inv"` (`0x1009C4`, load at `0x1E3E1`) whose surface records at
`0x147C4C..0x147C98` are BSS at rest; the visible menu is 8 of 20 rows at UI
`x=0x85`, `y=0xB8+0x21*k`, width `0x1BC` in a `0x280x0x1E0` canvas, painted
by `FUN_0001E154` with the highlighted row via `FUN_00016A3C` and two cursor
widgets `DAT_001492E0`/`DAT_001492E4` animated by `FUN_0001DF10`; the actual
blits are `FUN_00012D7C`/`FUN_00012F58` inside `FUN_00015D40`.**

## 1. Entry-state table (native, fixups applied)

`read_memory 0x143DC` (80 bytes) returns the 20 populated cells as real
handler addresses:

| state | cell | handler | state | cell | handler |
|------:|------|---------|------:|------|---------|
| 0 | `0x143DC` | `0x14451` | 10 | `0x14404` | `0x14742` |
| 1 | `0x143E0` | `0x144A8` | 11 | `0x14408` | `0x14798` |
| 2 | `0x143E4` | `0x14479` | 12 | `0x1440C` | `0x147EE` |
| 3 | `0x143E8` | `0x144D0` | 13 | `0x14410` | `0x14882` |
| 4 | `0x143EC` | `0x144FF` | 14 | `0x14414` | `0x1491F` |
| 5 | `0x143F0` | `0x14554` | 15 | `0x14418` | `0x149B3` |
| 6 | `0x143F4` | `0x145A9` | 16 | `0x1441C` | `0x14A50` |
| 7 | `0x143F8` | `0x14648` | 17 | `0x14420` | `0x14AAA` |
| 8 | `0x143FC` | `0x146DC` | 18 | `0x14424` | `0x14B01` |
| 9 | `0x14400` | `0x1470E` | 19 | `0x14428` | `0x14B28` |

`disassemble_function 0x1442C` confirms the dispatcher bounds and table jump
(`0x14440 CMP ESI,0x13`, `0x14443 JA 0x14B56`, `0x14449 JMP dword ptr
CS:[ESI*4+0x143DC]`).

State 17 is the front-end entry state; both front-end loops dispatch it:

```
0x1EB21 CALL 0x14C90          ; VIV reload
0x1EB26 MOV EAX,0x11          ; state 17
0x1EB2B CALL 0x1442C
```

(`disassemble_bytes 0x1EB20`; same in the panel loop at `0x1F8DF CALL
0x14C90` / `0x1F8E4 MOV EAX,0x11` / `0x1F8E9 CALL 0x1442C`,
`disassemble_bytes 0x1F8DC`). State 16 (`0x14A50`) is the code-8 exit
screen (`0x1EDF8 MOV EAX,0x10` / `0x1EDFD CALL 0x1442C`, FU-66).

### 1.1 State-17 handler `0x14AAA`

`disassemble_function 0x1442C` shows the handler as two widget records and
the shared tail:

```
0x14AAA PUSH -1 / PUSH 0xFFFFF9C0 / PUSH 0 / PUSH -1 / PUSH -1 /
        PUSH 0x1E0 / PUSH 0x65 / PUSH 0
        MOV ECX,-1; MOV EDX,0x76; PUSH 0; XOR EAX,EAX; MOV EBX,ECX
        CALL 0x13BD8
0x14AD5 PUSH -1 / PUSH 0 / PUSH 0xFFFFF9C0 / PUSH -1 / PUSH -1 /
        PUSH 0x1E0 / PUSH 0x280 / PUSH 0
        MOV ECX,-1; MOV EDX,0x6E; PUSH 0; MOV EAX,0x6F; MOV EBX,ECX
        CALL 0x13BD8
        JMP 0x14B4F            ; MOV EBX,ECX; CALL 0x13BD8 + shared pump
```

`decompile_function 0x13BD8` stores thirteen Watcom args (EAX,EDX,EBX,ECX
then stack, right-to-left) into one widget record of `DAT_00146560`
records at `0x146350`, stride `0x58` bytes, and increments the count
`[0x146560]`; the two state-17 records are therefore
`(p1..p13) = (0,0x76,-1,-1,0,0x65,0x1E0,-1,-1,0,-0x640,0,-1)` and
`(0x6F,0x6E,-1,-1,0,0x280,0x1E0,-1,-1,0,-0x640,0,-1)`. The selector
meanings are not bound to assets (open leg 6). No draw call occurs in the
handler itself: the tail runs the generic event pump at `0x14B56`.

## 2. Background/sprite asset path

The front-end setup `FUN_0001E3A8` loads **`"options.inv"`** before building
the widget list:

* `read_memory 0x1009B0` — the string block holds `options.dat` at `0x1009B8`
  and `"options.inv"` at `0x1009C4` (NUL at `0x1009CF`).
* `disassemble_bytes 0x1E3A8`: `0x1E3E1 MOV EAX,0x1009C4` /
  `0x1E3E6 MOV EBX,0x5` / `0x1E3EB CALL 0x19ABC`.
* `decompile_function 0x19ABC` = `FUN_000659F8(2, ...)` + `FUN_000A157C`;
  `get_xrefs_to 0x1009C4` = `{0x1F608 (FUN_0001F5CC), 0x1E3E1 (FUN_0001E3A8),
  0x1FA68}`, so the panel loop loads the same file.
* After the load, `FUN_00013888(DAT_00147D14/18/0C/10, w, h)`
  (`0x1E3F5..0x1E432`) writes width/height words into surface records and
  `FUN_0001B7E4` (`0x1E46A`) stores the eight handles `DAT_00147C90/7C8C/7D18/
  7C84/7C98/7D0C/7D10/7D14` into `0x149100..0x14911C`
  (`decompile_function 0x1B7E4`).

The menu surface slots themselves — `DAT_00147C30/34/38/3C/40/44/48/4C/
50/54/58/5C/60/64/68/6C/70/74/78/7C/80/84/88/8C/90/94/98` — are **BSS at
rest**: `read_memory 0x147C30` (128 B) and `read_memory 0x147D00` (96 B)
are all zero, so the row/panel/cursor surfaces exist only after the
`options.inv` loader runs. The runtime entry → surface binding is not
statically decodable from the image (open leg 2).

Immediate blits in the setup use ids `0x69`/`0x6A`:
`search_instructions CALL operand 12d7c in FUN_0001E3A8` = five sites
(`0x1E93F`, `0x1E95D`, `0x1E97B`, `0x1E999` with `0x69`/`0x6A`, and
`0x1EADD`), calling `FUN_00012D7C(id, w, h, DAT_00147DC0/34/38/3C, 1)`
(decompile `FUN_0001E3A8`), plus `FUN_0001325C(0x37, x, y+0x78, ...)` at
`0x1E9E6`.

## 3. Menu row geometry

`search_instructions CALL operand 15960 in FUN_0001E3A8` returns 12
`FUN_00015960` widget creations; `disassemble_bytes 0x1E4F0` and
`0x1E7A0` supply the arguments (Watcom order: `EAX,EDX,EBX,ECX`, then the
stack pushes right-to-left; `param_3`=width, `param_5`=x, `param_6`=y,
`param_9/10/11`=source surfaces, `param_16`=widget-id pair `0xN0000000N`):

| call site | widget id | x | y | source surface | stored at |
|---|---:|---:|---:|---|---|
| `0x1E54E` | 0 | surface-derived | surface-derived | `DAT_00147C4C/50/70` | `0x1492EC` |
| `0x1E5AD` | 1 | `0x85` | `0xB8` | `DAT_00147C4C/54/70` | — |
| `0x1E606` | 2 | `0x85` | `0xD9` | `DAT_00147C4C/58/70` | — |
| `0x1E660` | 3 | `0x85` | `0xFA` | `DAT_00147C4C/5C/70` | — |
| `0x1E6B9` | 4 | `0x85` | `0x11B` | `DAT_00147C4C/60/70` | — |
| `0x1E713` | 5 | `0x85` | `0x13C` | `DAT_00147C4C/64/70` | — |
| `0x1E76C` | 6 | `0x85` | `0x15D` | `DAT_00147C4C/68/70` | — |
| `0x1E7C6` | 7 | `0x85` | `0x17E` | `DAT_00147C4C/6C/70` | `0x1492E8` |
| `0x1E825` | 8 | `0xE1` | `0x1BD` | `DAT_00147C4C/74/70` | `0x1492E0` |
| `0x1E87E` | 9 | `-1` | `-1` | `DAT_00147C78/7C/80` | `0x1492E4` |
| `0x1E8CF` | 0xA | `-1` | `-1` | `DAT_00147C84/88/8C` | — |
| `0x1E91A` | 0xB | `-1` | `-1` | `DAT_00147C90/94/98` | — |

Row constants (immediates): x `0x1E5A5 PUSH 0x85`, y `0x1E59B PUSH 0xB8`,
`0x1E5ED PUSH 0xD9`, `0x1E64E PUSH 0xFA`, `0x1E6A0 PUSH 0x11B`,
`0x1E701 PUSH 0x13C`, (row 6 `0x15D`, row 7 `0x1E7B4 PUSH 0x17E`), width
`0x1E584/0x1E5FF/... MOV EBX,0x1BC`; the cursor widget's `x=0xE1`/`y=0x1BD`
are at `0x1E81C`/`0x1E815`. Step = `0x21` = 33 (0xB8, 0xD9, 0xFA, 0x11B,
0x13C, 0x15D, 0x17E).

The list is **20 entries showing 8 rows**:

* `disassemble_bytes 0x1E9D0`: `0x1EA5F MOV ESI,0x13` /
  `0x1EA75 MOV [0x14932C],ESI` → the list bound used by the painter's
  window clamp (`0x1E1AB..0x1E1BC`), i.e. at most 20 rows (indices
  0..0x13); `0x1EA04/0x1EA0A` zero `DAT_00149354` (first visible row) and
  `DAT_00149358` (selected slot).
* `disassemble_function 0x1E154` (`FUN_0001E154`, the list painter):
  `0x1E200 CMP ESI,0x8 / JL 0x1E1CD` = 8 visible rows;
  `0x1E1D2 MOV EAX,[ESI+EAX+0x105427]` / `0x1E1DB SAR EAX,0x18` reads the
  row-id byte (MSB of the dword at `0x105427+first+i`, i.e. the byte table
  at `0x10542A`; `read_memory 0x105427` = `0,0,0,0,0x12,1,0x11,2,0x13,3,4,
  6,7,0x15,9,10,...`), then `CALL 0x1D948` selects the sprite and
  `FUN_00016C68(DAT_001492EC+i, frame, ...)` writes it;
  `0x1E205..0x1E212` calls `FUN_00016A3C(DAT_001492EC+DAT_00149358)` to
  highlight the selected row, then `FUN_0001DFB8`.
* Window scrolling (`0x1E160..0x1E1BC`) keeps `DAT_00149354` in
  `[0, 0x13-8]` and `DAT_00149358` a slot in `[0,7]`.

Panel widget id 0 is created without explicit x/y/size: `FUN_00015960`
falls back to the header dimensions of `param_10=DAT_00147C50`
(`decompile_function 0x15960`, the `param_3 == 0xFFFFFFFF` / `param_5 ==
0xFFFFFFFF` arms). Its on-screen bounds are therefore runtime data, not
statically provable (open leg 3).

The `0x280`/`0x1E0` canvas follows from the handler blocks (state 16/17
push `0x640`/`0x280`/`0x1E0`, §1.1) and from the row width `0x1BC=444`
which cannot fit a 320-pixel canvas. The 320x240 mode-X presentation is
the ported surface model (FU-56), but the exact UI-canvas → mode-X mapping
is not evidenced in this slice (open leg 4).

## 4. Cursor and highlight blit calls

The two cursor widgets are id 8 (`DAT_001492E0`, at `x=0xE1,y=0x1BD`) and
id 9 (`DAT_001492E4`, surface-derived position); their frames are the
sprite handles / animation selectors `0xAA` and `0x120` produced by
`FUN_0001771C` (`0x1E7DE`/`0x1E83E`). `decompile_function 0x1DF10`
(`FUN_0001DF10`, the selection-change handler) shows the cursor update
flow:

```
if ([0x104F98] != [0x149284]) {            ; selection changed
  FUN_000175B8([0x149284]);
  FUN_0001DA58(2, [0x149284] == 0, ...); FUN_0001DFB8();
  FUN_0001E154([0x149354] + [0x149358]);   ; repaint list, highlight row
  FUN_00016C68([0x1492E4], FUN_0001771C(0x120), [0x147E60], -1, -1);
  FUN_00016C68([0x1492E0], FUN_0001771C(0xAA),  [0x147E60], -1, -1);
}
```

(`disassemble_bytes 0x1DF10`: `0x1DF64 CALL 0x1E154`, `0x1DF7B..0x1DF85`
cursor-widget 9, `0x1DF8C MOV EAX,0xAA` cursor-widget 8.)

The actual blits happen in `FUN_00015D40` (widget draw, mode in EDX),
decompiled/disassembled at `0x15D40..0x15EBF`:

* mode 3: `FUN_000196B4` + `FUN_000CBBCC` (`0x15D64`, `0x15D8E`);
* plain sprite: `FUN_00012D7C(list_id, x, y, surface, 1)` at `0x15E52`
  and `0x15E6B` (both surfaces of a two-surface widget), and `0x15EB3`;
* stretched/anchored sprite: `FUN_00012F58(list_id, x, y, surface, 1, dw,
  dh, tex, ...)` at `0x15E2A` and `0x15E9F`;
* list ids: `0x12C + 2*id` for mode 0, `0x15E` for modes 1/2
  (`0x15D48..0x15DE5`).

`FUN_0001E154`'s highlight reaches the same path:
`FUN_00016A3C(panel+selected)` → `FUN_00015D40(id, 2)` via
`decompile_function 0x16A3C` (`0x16A4F..`; the `[0x1465E4] != 0` arm).
So the cursor is: two animated widget sprites plus the selected row's
highlight fill, all drawn through `FUN_00012D7C`/`FUN_00012F58` on the
widget list.

## 5. Port mapping (`fifa96_menu_art`)

| original | port |
|---|---|
| state 17 entry (`0x1EB26`, `0x1F8E4`) | `fifa96_menu_state.entry_state` (recorded; the fallback draws one layout for all entry states because states 0–6/12–15/18–19 have no proven screen identity, FU-58 §8) |
| 8-of-20 row window (`0x1EA5F`, `0x1E200`, `0x1E154`) | `FIFA96_MENU_VISIBLE_ROWS 8`, `FIFA96_MENU_ENTRY_COUNT 20`, `selected_row` clamped to the window |
| row geometry (`0x1E5A5` x=0x85, `0x1E59B` y=0xB8, step 0x21, width 0x1BC) | panel/row rectangles at `x = 0x85*W/0x280`, `y = (0xB8 + k*0x21)*H/0x1E0`, width `0x1BC*W/0x280` (UI canvas 640x480, `0x280*W/0x280` = /2 for 320) |
| cursor widgets `0x1492E0`/`0x1492E4` and `FUN_0001DF10` (`0xAA`/`0x120` frames) | `cursor_on` flag toggles a cursor block at the selected row |
| `options.inv` load (`0x1E3E1` → `0x19ABC`) | `fifa96_menu_art_init`: basename `OPTIONS.INV` lookup, `fifa96_asset_read`, `fifa96_sprite_bank_parse`/`_entry`/`_frame`, nearest-neighbour scale into the background buffer, optional `fifa96_sprite_chunk_palette` |
| failure of any asset step | deterministic procedural fallback (below) |
| `FIFA96_MENU_FALLBACK_HASH` | FNV-1a of `fifa96_surface_hash` for `init(NULL)` + `draw(320x240, {entry_state=0, selected_row=0, cursor_on=1})` = `0x1be5e1aaceefaf09` |

Fallback frame: solid background (index 1), a top bar (width x `rh/2`,
index 6), the panel rectangle (index 2), the eight row bars (index 3,
selected row index 4) and the cursor block (4x8, index 5) at the selected
row. All coordinates derive from the UI constants above; panel bounds and
the UI→mode-X scale are the recorded open legs.

## 6. Open legs

1. **States 0–6, 12–15 (and the selector ids of 16/17) have no proven
   screen identity** (FU-58 §8, FU-65 §8). The renderer uses one generic
   menu layout.
2. **`options.inv` container identity and entry binding are not decoded.**
   The byte format behind `FUN_000659F8`/`FUN_000A157C` is not derived; no
   static evidence ties an entry to `DAT_00147C50` (panel), the row surfaces
   or the cursor surfaces. The port attempts an SHPI parse and falls back.
3. **Panel bounds are runtime data**: widget id 0 takes its x/y/w/h from the
   `options.inv` surface record (BSS at rest, §2).
4. **UI canvas → mode-X mapping**: the widget canvas is `0x280x0x1E0`
   (640x480); the engine surface is 320x240. The `/2` mapping used by the
   fallback is inferred, not statically evidenced.
5. **Row-id byte table semantics**: `0x10542A` values (0,0x12,1,0x11,2,
   0x13,3,4,6,7,0x15,9..0x10,0x42) are read as sprite-frame selectors via
   `FUN_0001D948`; the frame meanings are not derived.
6. **State-17 widget selectors** `0x76`/`0x65`/`0x6E`/`0x6F`/`0x280`/
   `0x1E0`/`-0x640` are stored but not bound to assets (`FUN_00013BD8`'s
   record consumers are not decomposed).
7. **Cursor sprite frames** `0xAA`/`0x120` and the `FUN_0001771C` selector
   semantics are not derived; only the call sites are.
8. **Mouse input path** to the menu navigation codes is a runtime
   input-binding question (FU-65 §4.3), untouched here.

## Provenance

Ghidra MCP on `/FIFA96.EXE` (native watcom LE, fixups applied; read-only):

* `read_memory`: `0x143DC` (80 B), `0x1009B0` (48 B), `0x105427` (64 B),
  `0x147C30` (128 B), `0x147D00` (96 B).
* `disassemble_function`: `0x1442C` (dispatcher + all 20 handlers),
  `0x1E154` (list painter).
* `disassemble_bytes`: `0x1E3A8` (200 B), `0x1EB20` (48 B), `0x1F8DC`
  (16 B), `0x1E4F0` (560 B), `0x1E7A0` (240 B), `0x1E9D0` (176 B),
  `0x1DF10` (128 B), `0x15D40` (384 B).
* `decompile_function`: `0x13BD8`, `0x1E3A8`, `0x15960`, `0x1E154`,
  `0x1DF10`, `0x15D40`, `0x16A3C`, `0x19ABC`, `0x12C88`, `0x13888`,
  `0x1B7E4`.
* `search_instructions`: CALL operand `15960` in `FUN_0001E3A8` (12),
  MOV operand `001493` in `FUN_0001E3A8` (22).
* `get_xrefs_to`: `0x1009C4` (3: `0x1E3E1`, `0x1F608`, `0x1FA68`).

Port write set: `include/fifa96_engine/fifa96_menu_art.h`,
`src/fifa96_engine/fifa96_menu_art.c`,
`src/fifa96_engine/fifa96_frontend_run.c`,
`include/fifa96_engine/fifa96_frontend_run.h`,
`tests/test_engine_menu_art.c`, `tests/test_engine_frontend.c`,
`CMakeLists.txt`. Analysis-only otherwise; no Ghidra/project/ISO write.

## 7. Erratum (M1 gate fix wave, 2026-10-06): no `OPTIONS.INV` in the ISO

The M1 gate review enumerated the retail image and found **no OPTIONS-like
path anywhere in the 798 files**, so the `options.inv` load recorded in §2
does not resolve to a top-level ISO file. The engine's menu art therefore
always takes the procedural fallback at M1 (as §5 already allows), and the
real front-end art asset remains unresolved (open legs 2 and 3 carry the
container-identity question unchanged).

Enumeration command and output (repo tool, read-only; full path list):

```
$ ./build/fifa96_play auto game/FIFAPCCD96.iso --list | head -1
auto game/FIFAPCCD96.iso iso9660 bytes=476033024 files=798

$ ./build/fifa96_play auto game/FIFAPCCD96.iso --list | grep -c '^iso '
798

$ ./build/fifa96_play auto game/FIFAPCCD96.iso --list | grep -ci 'options'
0

$ ./build/fifa96_play auto game/FIFAPCCD96.iso --list | grep -ci '\.inv'
0
```

(`grep -i` makes both checks case-insensitive; the walk is the same
`fifa96_iso9660_walk` used by the engine's asset table.) The three `0x1009C4`
xrefs in §2 (`0x1E3E1`, `0x1F608`, `0x1FA68`) are unchanged and still real:
the string is loaded and passed to `FUN_000659F8(2, ...)`/`FUN_000A157C`, so
the file must come from a resolver outside the plain ISO9660 namespace (e.g.
a virtual/overlay name, a different disc, or a code-generated buffer). That
resolution — and the entry-to-surface binding from open leg 2 — is the
remaining front-end art open leg; nothing in §1–§6 is retracted.

This erratum is append-only: earlier rows were not rewritten.
