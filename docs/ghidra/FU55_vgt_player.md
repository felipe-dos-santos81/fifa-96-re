# FU-55 — TGV stream player (`vgt_stream_poll` 0x67BA8)

Date: 2026-10-05. Program `/fifa96_le.bin` (FU-4 linear image, flat link
addresses, bridge 2026-10-04). Result: the player loop that ties
`fifa96_tgv_stream`, `fifa96_kvgt` and `fifa96_fvgt` together is derived
and ported (`fifa96_vgt_player`), with the committed vectors as tests.
The Mode-X blit and the sound-subsystem decode stay out of scope; the port
stops at the canvas/palette/audio-chunk boundary.

## 1. Entry chain, callers, state

`vgt_stream_poll` (0x67BA8, `get_function_callers`: `FUN_00068108` /
`FUN_00068194` / `FUN_0006847C`) is one iteration of the movie player.
`FUN_000679F4` (stream open, 0x679F4) allocates the 0x344-byte decode
context (`FUN_000AE490` 0xAE490, zero-filled), opens the frame stream
`[0x563D4] = FUN_00094B3E(...)` (0x67A83) and the companion stream
`[0x563F4] = FUN_00095064(0x31,0xFF,...)` (0x67AA8), resets
`[0x563CC]`, `[0x563FC]`, `[0x563E0]`, `[0x563DC]`, `[0x563EC]`,
`[0x563C8]` and sets `[0x563E8] = ctx` (0x67A26). `FUN_00067E94`
(closes) frees both (`FUN_00094E03`, `FUN_000AE51C`).

Player-loop globals (xrefs from `get_bulk_xrefs`; FU-37 §B.3 also lists
them):

| addr | role | citations |
|---|---|---|
| `0x563C0` | current chunk tag | write `0x67C34` |
| `0x563C4` | frame target = frames drawn at frame start | write `0x67CBD`, read `0x67E3B` |
| `0x563C8` | on-schedule return counter | write `0x67E86` |
| `0x563CC` | `vgt_dispatch` result (surface pointer) | write `0x67C6A`, read `0x67CAC/0x67E81` |
| `0x563D0` | sound-present flag (`FUN_00064F70`, then `FUN_000A7CA4`) | write `0x67A3A`, reads `0x67D46/0x67D89/0x67DD4/0x67E10` |
| `0x563D4` | frame stream context | write `0x67A83` |
| `0x563D8` | current chunk pointer | write `0x67BD5/0x67CD7/0x67CF0` |
| `0x563DC` | frames displayed | write `0x67B3B`, inc `0x67E6C` |
| `0x563E0` | audio-EOF latch | write `0x67D2E`, read `0x67CD1` |
| `0x563E4` | progress (frames or dequeued audio chunks) | write `0x67E41`, read `0x67E60` |
| `0x563E8` | decode context | write `0x67A26`, reads `0x67C55/0x67C5B/0x67C78` |
| `0x563EC` | audio-started latch | write `0x67D8E/0x67DDA` |
| `0x563F0` | `[0x563DC]+2` catch-up bound | write `0x67CC5`, read `0x67E66` |
| `0x563F4` | companion/audio stream context | write `0x67AA8` |
| `0x563F8` | progress time base | write `0x67DFF`, read `0x67E33` |
| `0x563FC` | kVGT palette-copy flag | write `0x67C49`, read `0x67C61` (hosts clear) |

## 2. Player loop (disassembly, 211 insns)

Entry 0x67BA8..0x67BBA: `EBP=1`, `EDI=0`, `ESI = clock()+0x1F4`
(`FUN_000CB2A4` = `[0x12E88]`, the 100 Hz counter, FU-37/FU-48).

**Inner loop A — frame chunks, 0x67BC0..0x67CB2:**

1. `[0x563CC] = 0` (0x67BC7).
2. `chunk = FUN_00095CB3([0x563D4])` (0x67BC0/0x67BCD), stored at
   `[0x563D8]` (0x67BD5).
3. Stream-event check: `FUN_00095EB8()` (0x67BDA, returns
   `PTR_DAT_00011178`) and `FUN_000949F8()` (0x67BE3, returns
   `[DAT_0005B7E4+0x1C] == -2`; `DAT_0005B7E4` is the stream object
   installed by `FUN_00094A48`). If either fires, `FUN_00095ED4` clears
   `0x11178`, `FUN_00094A2D` clears `[+0x1C]`, and the poll returns 0
   (0x67BF6..0x67C02).
4. `if (chunk == (int*)-1) return 0` (0x67C0D/0x67C10) — the walker's
   sentinel return (§4).
5. `if (chunk == 0)` skip dispatch (0x67C16/0x67C18).
6. tag = LE32(chunk) (0x67C25..0x67C34; the `SHR EAX,CL` with `CL=0` is
   the compiler's LE dword load), stored `[0x563C0]`.
7. Dispatch only for tag `0x54475666` (`fVGT`, 0x67C39/0x67C3E/0x67C40)
   and `0x5447566B` (`kVGT`, 0x67C42/0x67C47); on kVGT set
   `[0x563FC] = 1` (0x67C49). Other tags, including `eVGT` 0x54475665,
   fall through to the release at 0x67C8E.
8. `[0x563CC] = vgt_dispatch([0x563E8], chunk)` (0x67C4F..0x67C6A).
9. `if ([0x563FC] != 0)` copy 0x300 bytes `ctx+0x44 -> 0x560C0`
   (0x67C73..0x67C86; `FUN_000CD390` copies its first arg to its second,
   FU-19 §5 lines 411-415).
10. `FUN_00095DD2([0x563D4], chunk)` marks the chunk consumed
    (`*chunk = 0xFFFFFFFE`, 0x67C8E..0x67C9B; `FUN_00095DD2` decompile).
11. Loop A repeats while `clock() <= frame_start+0x1F4` and
    `[0x563CC] == 0` (0x67CA3..0x67CB2). `ESI` is reloaded at the outer
    loop top (`0x67BB5` clock, `0x67BBA LEA ESI,[EAX+0x1F4]`, re-entered
    by the catch-up jump `0x67E74 JGE 0x67BB5`), so the bound is a 5 s
    watchdog from the start of the current frame, not a per-chunk timeout.

**Frame boundary, 0x67CB8..0x67CC5:** `[0x563C4] = [0x563DC]`,
`[0x563F0] = [0x563DC]+2`.

**Inner loop B — companion stream + pacing, 0x67CD1..0x67E54:**

1. `[0x563D8] = 0`; if `[0x563E0] == 0`, fetch
   `chunk = FUN_00095CB3([0x563F4])` (0x67CE1/0x67CE8/0x67CF0).
2. Same stream-event abort as A (0x67CF5..0x67D23), returning 0.
3. `chunk == -1` (walker sentinel) sets `[0x563E0] = 1`
   (0x67D2E/0x67D34): the companion stream is never read again.
4. Otherwise (and `chunk != 0`): sound present → `FUN_000A7D3C(chunk)`
   (0x67D52/0x67D57); no sound → `FUN_00095DD2([0x563F4], chunk)`
   (0x67DB5..0x67DC7). The first dequeued audio (> 0x20000 bytes queued
   via `FUN_000A7E31(0)`, 0x67D6A..0x67D79) starts playback with
   `FUN_000A7CE4`, stores `[0xA2A8]` and latches `[0x563EC]=1`
   (0x67D7F..0x67D8E), setting the progress base `[0x563F8]` to
   `clock()*15/100` or `FUN_000A7EBC()` (0x67D9F..0x67DFF).
5. Wait: `if (clock() > fetch_time + 0x1E) break` (30 ticks = 300 ms,
   0x67E04..0x67E0E); else `[0x563E4] = now_progress - [0x563F8]`
   (0x67E10..0x67E41) and loop while `[0x563E4] < [0x563C4] ||
   [0x563EC] == 0` (0x67E46..0x67E54).

**Frame return / catch-up, 0x67E5A..0x67E92:** `[0x563DC]++`; if
`[0x563E4] >= [0x563F0]` jump back to loop A immediately (0x67E72/0x67E74,
the catch-up path, `fifa96_pacing_catch_up`); else `[0x563C8]++` and
return `[0x563CC]` (0x67E7A..0x67E8C). The return value is the dispatch
result, i.e. the surface decoded this frame, or 0 (end).

## 3. Context, surfaces, geometry

* Context (`FUN_000AE490`, 0xAE490): 0x344 bytes, zero-filled. Slots
  used here: `ctx[0]` pitch/width, `ctx[1]` height, `ctx[0xB]` pre
  surface, `ctx[0xC]` block ids, `ctx[0xD]` row table, `ctx[0x10]` post
  surface, `ctx+0x44` palette (0x300 = 256 RGB triples).
* `vgt_dispatch` (0xAE4BC) routes `fVGT` -> `vgt_decode_f` (0xADEFC) and
  `kVGT` -> `vgt_decode_k` (0xAE218), and returns `ctx[10]`.
* kVGT (`vgt_decode_k` decompile): `ctx[0] = LE16(+8)`, `ctx[1] =
  LE16(+10)` (**keyframe geometry**; FU-29 errata / FU-33 §4); frees and
  re-allocates both surfaces as `((w*h+3)&~3)+0x10`, header byte0
  `|=0x7B`, LE16 width at +4, LE16 height at +6 (0xAE3EB/0xAE3F5/0xAE400
  per FU-19 §5; FU-19's "BE16" wording is the same little-endian store the
  host's `dword[+2]>>16` read at 0x681DD confirms), pixels at +0x10;
  decodes the record into `ctx[10]+0x10`
  only (0xAE467/0xAE474), leaving `ctx[0xB]` zeroed; rebuilds
  `ctx[0xD][k] = k*w - w*h` for `2*h` entries (0xAE432..0xAE458).
* fVGT (`vgt_decode_f`, FU-33 §4/§6/§8) never writes `ctx[0]/[1]/[0xD]`;
  it uses the **inherited keyframe geometry** (`count = pitch*height/16`,
  0xAE19B..0xAE1A5), swaps `ctx[10]`/`ctx[0xB]` at entry
  (0xADF25..0xADF31), composites from post-swap `ctx[0xB]+0x10` to
  post-swap `ctx[10]+0x10`, so the returned `ctx[10]` is the new canvas
  and the previous canvas is untouched by the decoder (FU-32).

## 4. Sentinel semantics (`FUN_00095CB3`, 0x95CB3, 92 insns)

Stream fields (displacement from the decompiled `param_1[i]`):
`+0x00` base, `+0x04` buffer end, `+0x0C` data end, `+0x10` cursor,
`+0x14` tracked -3 mark, `+0x1C` state. Both open helpers
(`FUN_00094A48`/`FUN_00094F85`) point base/end/data-end/cursor/mark at the
same address; the frame stream additionally sets state 7
(`param_1[7] = 7`, `FUN_00094A48`), the companion stream flags
`param_1[0xF] = 1` with `param_1[0x14]/[0x15]` (`FUN_00094F85`).

* `state == 1 || state == 2` -> return 0 (0x95CC6..0x95CD3).
* `data_end == cursor` -> return 0 (0x95CE4..0x95CED).
* **tag `-1` = rewind/loop.** `if (*(int*)cursor == -1)` then
  `cursor = base` (0x95CFE..0x95D0E); if `data_end == cursor` return 0
  (0x95D14..0x95D1D). Otherwise the fall-through reads `len =
  [cursor+4]`, bounds-checks it against the linear/wrapped window
  (0x95D2B..0x95D73), returns `cursor` as the chunk and advances
  `cursor += len` (0x95D7E..0x95D8D). So a `-1` chunk is skipped and the
  chunk at the buffer base is returned instead — a loop point. A `-1`
  already at base falls through and is returned like any chunk.
* **tag `-3` = terminal/skip marker.** Advance `cursor += len` first
  (0x95D87/0x95D8D), then `if (*(int*)chunk == -3)`: if the chunk is the
  tracked mark `[+0x14]`, the mark is advanced past it (0x95DA3..0x95DAC),
  otherwise the chunk header is overwritten with `-2`
  (`*chunk = 0xFFFFFFFE`, 0x95DB1..0x95DB4); either way the walker
  returns `(int*)-1` (0x95DBA/0x95DC9).
* Otherwise returns the chunk pointer (0x95DC3..0x95DC6); a short or
  wrapping-past length returns 0 (need refill).

The player turns the `-1` return into an end: frame stream -> return 0
(0x67C0D/0x67C10); companion stream -> `[0x563E0]=1` (0x67D2E). Retail
assets contain **no** sentinel tags (`FU-37` validation: 0/73 TGV files;
the committed `vid_game.tgv` walk also has none), so only the committed
synthetic tests exercise this path.

## 5. Palette publishing

The kVGT decoder copies `palette_count` RGB triples into `ctx+0x44`
(0xAE2B3..0xAE2CE, unbounded in the original; the port caps at 256).
After dispatch the player copies the full 0x300 bytes to the global
buffer `0x560C0` when `[0x563FC]` is set (0x67C73..0x67C86); `[0x563FC]`
is set on every kVGT tag and only cleared by the hosts after they program
the VGA DAC: `FUN_00068194` at 0x68255..0x6827C (`FUN_000CE8D7`,
`FUN_000CE754(0, 0x100, 0x560C0)`) and `FUN_00068108` (`FUN_000AE760`,
then `[0x563FC]=0`). Deltas never touch the palette, so the host keeps the
last keyframe's palette.

## 6. Dispatch/blit boundary and audio

* The poll returns the raw surface pointer; `FUN_00068194` reads its
  width/height back from the header (`dword[+2]>>16`, `dword[+4]>>16`,
  0x681DD/0x681F6) and centers it: `FUN_000AE7F0(surface,
  (0x12ABC-w)/2 & ~3, (0x12AC0-h)/2 & ~3)` (0x68200..0x68224). The
  blitter writes VGA `0xA0000` in 4 planes with stride 0x140 = 320
  (`FUN_000AE7F0` decompile: `(y+h)*0x140`, `FUN_000D064B(2,
  1<<(x&3))`, `FUN_000BAA48`); its internals are out of scope.
  `FUN_00068108` presents the surface via `FUN_000A6040`; `FUN_0006847C`
  via `FUN_0009AA00`.
* Frame-stream non-VGT chunks are only released (`FUN_00095DD2`,
  0x67C8E); the companion chunks are read through the second stream
  `[0x563F4]` and routed to the sound dispatcher `FUN_000A7D3C`
  (0x67D57; tags `1SNd`/`1SNh`/`1SNe`/`1SNl` per the FU-30 errata and the
  `FUN_000A7D3C` decompile). The sound scheme runs on its own record
  (FU-35/FU-39); the player itself never decodes audio. Whether the two
  stream contexts page the same TGV file or two files is an open leg
  (`FUN_000679F4` opens them through different helpers).

## 7. Pacing

FU-37 §B.3 documents the model; the load-bearing citations are
0x67D9F..0x67DB3 (`clock()*15/100`, 15 fps), 0x67D98/0x67DE4/0x67E18
(calls to `FUN_000A7EBC`, the dequeued-chunk counter used as master
clock), 0x67E04..0x67E0E
(300 ms wait bound), 0x67CC5 (bound `frames+2`) and 0x67E72/0x67E74
(catch-up). `fifa96_pacing_frames_due` is the `*15/100` term and
`fifa96_pacing_catch_up(frames_before, progress)` is the `>= frames+2`
test. The port's `step` is one outer iteration without the wait loops;
pacing stays host-side.

## 8. Port mapping

`include/fifa96_loader/fifa96_vgt_player.h` +
`src/fifa96_loader/fifa96_vgt_player.c`:

| original | port |
|---|---|
| `FUN_000679F4` open/alloc ctx | `fifa96_vgt_player_init(p,w,h,front,front_cap,back,back_cap,on_audio,user)` |
| `FUN_00067E94` close | caller drops the struct |
| feeding a chunk stream | `fifa96_vgt_player_feed(p, data, size)` |
| one poll (loops A+B) | `fifa96_vgt_player_step(p, frame)` -> 1 frame / 0 end / negated `fifa96_err_t` |
| `vgt_dispatch`/`vgt_decode_k` | `fifa96_kvgt_decode` into `front` |
| `vgt_decode_f` + surface swap | `fifa96_fvgt_decode(pre=front, dst=back)` then swap |
| return `ctx[10]` | `frame->pixels` |
| `ctx[0]`/`ctx[1]` geometry | `p->width`/`p->height` (init, updated by kVGT) |
| `ctx+0x44 -> 0x560C0` + `[0x563FC]` | `frame->palette` + `frame->palette_changed` on kVGT frames |
| companion `FUN_000A7D3C` call | `on_audio(user, chunk, chunk_len)` for non-frame chunks |
| walker `-1` return / `-3` tag | `p->ended` (step -> 0) |
| walker `-1` tag | `walk.pos = 0` and keep scanning (no rewind at offset 0) |
| walker `-2` mark | not modeled (read-only input) |

Divergences: the original allocates/frees surfaces per keyframe
(unbounded), the port's caller buffers are fixed and a too-large frame is
`FIFA96_ERR_TRUNCATED`; the original writes the `-2` mark into its ring
buffer, the port never mutates input; the original's decoder failures
leave the stale `ctx[10]` visible, the port returns the error; the
original preserves stale palette bytes past `palette_count`, the port
zero-fills (the existing `fifa96_kvgt_decode` choice); the host-side
stream-event abort (`FUN_00095EB8`/`FUN_000949F8`) is not modeled.

## 9. Port tests (`tests/test_vgt_player.c`, suite 44 -> 45)

* kVGT end-to-end: `kvgt-frame-01.bin` -> 96x100, 9600 bytes identical to
  `fifa96_kvgt_decode`, palette = frame `+0x14` (256 triples),
  `palette_changed`, then end.
* fVGT end-to-end: canvas primed with `fvgt-01.pre.bin`, feed
  `fvgt-01.in.bin[0..9972)` -> `fvgt-01.out.bin` byte-exact, 320x240,
  `palette_changed == 0`, buffers swapped.
* Geometry change: committed 96x100 keyframe followed by a synthetic
  320x200 tree keyframe (`record-46`); capacity error when the keyframe
  does not fit; init rejection for zero dims/overlap.
* Sentinels: `-3` ends; `-1` loops back to the base frame; a `-1` at the
  base is skipped; malformed/truncated chunks return the negated error
  and latch `ended`.
* Audio: a companion chunk inside the sequence fires the callback once
  with the exact pointer/length and is not decoded; audio-only streams
  end after notifying.

## 10. Open legs

1. **Blit internals** (`FUN_000AE7F0`, `FUN_000D064B`, `FUN_000BAA48`) and
   the `FUN_0009AA00`/`FUN_000A6040` present path — out of scope.
2. **Two-stream plumbing**: whether `[0x563D4]` and `[0x563F4]` page the
   same file, and how the ring-buffer refill (`FUN_000950B6`,
   `PTR_DAT_00011178` writers at 0x960B0+) feeds the walker, is not
   traced; the port takes contiguous buffers.
3. **`[+0x14]` tracked mark** semantics (which `-3` avoids the `-2` mark)
   is only derivable with an injected sentinel; retail assets have none.
4. **Stream-event abort** `FUN_00095EB8`/`FUN_000949F8` (error/exit
   condition) is host-side and not modeled.
5. **Palette tail** past `palette_count` in the original is stale context
   bytes; the port zero-fills via the existing decoder.

## 11. Provenance

* Ghidra `/fifa96_le.bin`: disassembled `0x67BA8` (211 insns, quoted
  address-by-address in §2), `0x68194` (103 insns), `0x95CB3` (92 insns);
  decompiled `0x67BA8`, `0x95CB3`, `0x95DD2`, `0x95EB8`, `0x95ED4`,
  `0x949F8`, `0x94A2D`, `0xCD390`, `0xCB2A4`, `0x679F4`, `0x67E94`,
  `0x950B6`, `0xAE490`, `0xAE4BC`, `0xAE760`, `0xAE218`, `0xAE7F0`,
  `0x68108`, `0x68194`, `0x6847C`, `0x94A48`, `0x94F85`, `0xA7D3C`;
  xrefs: `get_function_callers 0x67BA8` (3), `get_bulk_xrefs` for the
  16 loop globals; memory `0xA2C8`/`0xA2D0` (BSS at rest).
* Assets: `tests/golden/vgt/kvgt-frame-01.bin` (5788 B, sha256
  `92a18ccc…dd14a27`), `fvgt-01.{in,pre,out}.bin` (9972/76800/76800,
  FU-31/FU-32), `record-46.{in,out}.bin`; a full walk of
  `tests/golden/vid_game.tgv` (1165 chunks, no sentinels) for §4/§6.
* `make test` 44/44 before; 45/45 after; ASan/UBSan clean.
