# FU-120 — the snapshot's side indices and the ball record

Follow-on to FU-112 §6 legs 1–2 (the `[0x57ABE]`/`[0x57ABF]` team indices and
the 23rd snapshot slot). This slice names the side-swap writer and the ball
object behind the composer's extra record.

Result in one line: **`[0x57ABE]`/`[0x57ABF]` are the current side indices,
recomputed by `FUN_0007417c` as `([0x57AC2] ^ [0x57AC3]) & 1` and its
complement; `FUN_000741b4` tests an entity against the side; and the 23rd
snapshot slot is the ball record at `0x5880C` (x/y/z, heading `0x5885A`,
appearance `0x58866`).**

## 1. The side indices

* `FUN_0007417c` — the side swap:

  ```
  [0x57ABE] = ([0x57AC2] ^ [0x57AC3]) & 1;
  [0x57ABF] = [0x57ABE] ^ 1;
  FUN_0007412c(...);      // update the side-dependent tables
  FUN_0008c33c(); FUN_0008c33c(); FUN_00078824();
  ```

* `FUN_0007412c` — side-state refresh: `FUN_0008ce78(&0x4C35C)`,
  `FUN_0008ce78(&0x4C3D7)` (two side-dependent structures at `0x4C35C` /
  `0x4C3D7`).
* `FUN_000741b4(EAX = entity side)`: `return (EAX ^ [0x57ABE]) & 1` — maps an
  entity's stored side to the current home/away orientation.
* Readers: the snapshot composer and reset (`FUN_00036c70`/`FUN_0003705c`,
  FU-112 §1) index the team blocks `0x588A4 + idx*0x835`, plus
  `FUN_00078824`, `FUN_0008b9cc`, `FUN_000741b4`.

So the composer's two 11-player loops walk *current side A* then *side B*,
not fixed home/away blocks.

## 2. The ball record `0x5880C`

The composer's 23rd slot (`i = 0x16`) reads `0x5880C`/`0x58810`/`0x58814`
(position), `0x5885A` (heading) and `0x58866` (appearance) — the ball object.
Its live writers (`search_instructions 5880c`, 20 sites):

| site | in | effect |
|------|----|--------|
| `0x84BDD` | (unnamed) | `ADD [0x5880C],EAX` — integrate x |
| `0x8469A` | `FUN_00084630` | reads base (0x5880C) |
| `0x8873A`/`0x88746` | `FUN_000886d4` | sets `-0x720` / `0x1560` (reset/teleport) |
| `0x8B6EB` | (unnamed) | `MOV [0x5880C],EBX` |
| `0x8C26C` | `FUN_0008c24c` | `MOV [0x5880C],ECX` |
| `0x8BAF0` | `FUN_0008baf0` | compares/updates the base |

The ball pointer `[0x57A6F]` (FU-112 §3) is set by `FUN_00073e28`,
`FUN_00083428` (default `0x58767`), `FUN_00084021`, `FUN_00085498`; the
composer uses `[0x57A6F]` when set and not in the replay family, else the
fixed `0x5774C` triple — the alternate "live/held" ball source.

## 3. Closures / errata

* **FU-112 §6 leg 1 — closed.** The side indices are computed by the swap
  `FUN_0007417c` (from `[0x57AC2] ^ [0x57AC3] & 1`) and tested by
  `FUN_000741b4`.
* **FU-112 §6 leg 2 — closed.** The 23rd slot is the ball record at
  `0x5880C`, with the live updaters listed above.
* **FU-112 §3 — refined.** `0x58767` (the default `[0x57A6F]`) sits inside
  the ball cluster before `0x5880C`; its role (sprite/animation base vs
  position alias) stays open.

## 4. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x7412C, 0x7417C,
0x741B4; `search_instructions` `57abe` (7), `57abf` (7), `5880c` (20);
`search_instructions` context for the writer sites. Address mapping as FU-88.

## 5. Open legs

1. `[0x57AC2]`/`[0x57AC3]` (the side-select flags xor-ed by the swap) and
   `[0x57A83]` (FU-112 §6 leg 4).
2. The `0x4C35C`/`0x4C3D7` side-dependent structures (`FUN_0008ce78`).
3. `0x58767`'s role in the ball cluster.
