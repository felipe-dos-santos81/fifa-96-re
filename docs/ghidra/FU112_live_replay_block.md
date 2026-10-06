# FU-112 — the live replay block: the composer and the reset

Follow-on to FU-109 §7 leg 1 (the `0x55AA4` live block's field map and its
writer). This slice decomposes `FUN_00036c70` — the composer that snapshots
the live match model into the block the packer serializes — and its companion
reset `FUN_0003705c`.

Result in one line: **`FUN_00036c70` fills `0x55AA4` (ball x/y/z), the 23
entity slots at `0x55AB0 + 0xC*i`, the packed headings at `0x55BC4 + 4*i`, the
appearance bytes at `0x55C20`/`0x55C37`, and the camera/input cluster
`0x55C4C..0x55C72` (band, matrix, speed gear, popped queue byte); the sources
are the team blocks `0x588A4 + {[0x57ABE],[0x57ABF]}*0x835` (0xB2-stride
player records) plus the ball object; `FUN_0003705c` is the hide-all reset;
`FUN_0006408c` packs the block and `FUN_00063b80` applies a record back into
it.**

## 1. The composer `FUN_00036c70`

Head:

```
src = ([0x57A6F] != 0 && !FUN_00063fd0()) ? [0x57A6F] : &0x5774C;
[0x55AA4/8/C] = src[0..2];                       // ball x, y, z
clamp |x|,|z| <= 6000 else (x,z) = -4000, y = 0;  clamp 0 <= y <= 16000 else y = 0;
```

Entities (two 11-player loops + one special, 23 slots):

| block | source | value |
|-------|--------|-------|
| `0x55AB0 + 0xC*i` | `puVar7+0x59/+0x5D/+0x61` | x/y/z dwords |
| `0x55BC4 + 4*i` | `(0x400 - (*(int*)(puVar7+0x7B) >> 16) & 0x3FF) << 6` | packed heading |
| `0x55C20 + i` | `***(byte**)(puVar7+0x28)` | appearance byte |
| `0x55C37 + i` | `puVar7[0x3D]` | byte |
| hidden (`puVar7[0x9A] != 0`) | — | y := `0xFFFFD8F0` (-10000) |

with `puVar7 = 0x588A4 + [0x57ABE]*0x835` (11 players, `+= 0xB2`), then
`0x588A4 + [0x57ABF]*0x835` (11 more), then the 23rd slot `i = 0x16` from
`0x5880C`/`0x58810`/`0x58814` (position), `0x5885A` (heading),
`0x58866` (appearance); its `0x55C37+0x16` byte is written 0. Note the 23rd
`[0x55C37+0x16]` = `0x55C4D` lands inside the camera dword `0x55C4C` and is
written before the camera fill, which rewrites bytes 2/3 — an observed
overlap at slice time.

Camera/input cluster:

* six bytes at `0x9A88..0x9A8D` start `0xFF`; four slots from
  `0x57C64 + i*0x25`, when the slot is non-null, its `+3` byte is >= 0 and
  `[slot+0x20] != 0`, take `[slot+0x8D]`, `+0x0B` when the slot's team byte
  (at `0x57C83 + i*0x25`, byte 3) equals `[0x57ABF]`;
* `[0x9A88+4] = FUN_0004ba44()` and `[0x9A88+5] = FUN_0004ba00()` (created
  `FUN_0004ba44`, `0x4BA44..0x4BAD5`; player-selection helpers keyed on
  `[0x590CC]` per-team flags and `[0x57ABE]`/`[0x57ABF]`; semantics open);
* `[0x55C4C]` byte 2: if bit `0x40` was clear, arm it and
  `[0x55C50] = FUN_000627f4(pitch, [0x55AA8])` (camera band from ball height;
  pitch thresholds ±0x46, ball-Y thresholds 0x35/0x6A, result 1..0xF), then
  `[0x55C51] = 1`; the `0x80` bit is set when `[0x57822] != 0`, and the
  low nibble increments when `[0x577C2]`/`[0x577C0]` are set;
* `[0x55C4C]` byte 3 = popped queue value (`FUN_00064ee4`, FU-111 §3);
* `[0x55C70] = FUN_000974d0()` (a `return 1` stub); `FUN_00063b20()` clamps
  its `EAX` to `[0, 0x6A4]` and sets the `[0x97BC]` speed gear 1..4
  (`<0x47`/`<0x83`/`<0xB5`/else); `[0x55C72] = FUN_000492cc()` (word);
* `[0x55C52..0x55C69]` = six dwords copied from `FUN_0004c904()` =
  `[0x7DC8]+0x10/0x14/0x18/0x58/0x5C/0x4C` staged at `0x4E4E0..0x4E4F4`
  (the active view object's matrix — this is the packer's `+0xF6` six dwords).

## 2. The reset `FUN_0003705c`

```
if (EAX < 0) FUN_00036c70();                     // keep the composer path
else {
    [0x55AA4]=-10000; [0x55AA8]=0; [0x55AAC]=-10000;
    for i in 0..22: [0x55AB0+0xC*i]=0xFFFFB1E0 (-20000); [+4]=0; [+8]=&0x4E20;
    FUN_00036fd0(0, EAX == 0 ? 0xB : 0x16);
}
```

`FUN_00036fd0(EAX=start, EDX=end)` re-copies only the visible records
(`[base+0x9A] == 0`) from a register-passed player base into the entity
arrays — the same field writes as the composer's loops. So the reset hides
the block and then restores one team (11) or both (22) from the live model.

## 3. Writers of the block

`search_instructions 55aa4`: `FUN_00036c70` (composer), `FUN_0003705c`
(reset), `FUN_00063cbc`/`FUN_00063e54` (the playback drivers, passing
`0x55AA4` as EAX to the apply at `0x63CAC`/`0x63CFA`/`0x63E9B`), and
`FUN_0006408c` (the packer, reading it). So the pair is exactly:
**compose (live) → pack (`FUN_0006408c`) → record → apply (`FUN_00063b80`) →
compose**. The ball source pointer `[0x57A6F]` is set by the live sites
`FUN_00073e28`, `FUN_00083428` (defaults to `0x58767`), `FUN_00084021`,
`FUN_00085498`.

## 4. Closures / errata

* **FU-109 §7 leg 1 — closed.** The `0x55AA4` block's field map and its
  writers: `FUN_00036c70` (live) and `FUN_00063b80` (replay apply), with
  `FUN_0003705c` the hide-all reset and `FUN_00036fd0` the partial restore.
* **FU-108 §1 — refined.** The record's `+0x0C..+0x2E` word loop sources are
  the entity arrays mapped in §1 (0xC-stride x/y/z), not a separate buffer.
* **FU-109 §2 — refined.** The `+0xF2/+0xF3` camera bytes are the composer's
  band byte (`[0x55C4C]` byte 2, armed by `FUN_000627f4`) and the popped queue
  byte (byte 3); byte 1 carries the 23rd entity's `[0x55C37+0x16]` write.
* **FU-111 §8 leg 5 — partially closed.** `[0x55C84]`'s consumers remain
  open, but the HUD cluster's camera-side neighbours (`0x55C50`, `0x55C70`,
  `0x55C72`) are now named.
* Ghidra-project change: created `FUN_0004ba44` (a data item blocked the
  entry; cleared and re-disassembled `0x4BA44`).

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x36C70, 0x3705C,
0x36FD0, 0x4BA44 (created), 0x4BA00, 0x4BE44, 0x4C904, 0x627F4, 0x63B20,
0x974D0; `search_instructions` `55aa4` (10), `588a4` (39), `57a6f` (7);
`search_instructions` in `FUN_00036c70` (`4ba44`/`4ba00` call sites at
`0x36EDA`/`0x36EE8`). Address mapping as FU-88.

## 6. Open legs

1. `[0x57ABE]`/`[0x57ABF]` (team indices) and `[0x57A6F]` (ball pointer)
   writers on the live side.
2. The 23rd slot's object (`0x5880C..0x58866`) — the ball or referee record.
3. The `0x55C37+0x16` / `0x55C4C` byte-1 overlap (intentional aliasing or
   off-by-one in the 23rd write).
4. `FUN_0004ba44`/`FUN_0004ba00`/`FUN_0004be44` semantics (`[0x590CC]`
   per-team flags, `[0x57A83]`, `[0x57AC2]`).
5. `FUN_0004c904`'s `[0x7DC8]` view object identity.
