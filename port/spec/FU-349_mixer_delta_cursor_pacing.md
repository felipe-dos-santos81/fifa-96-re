# FU-349: mixer adaptive-delta cursor reset and live pacing

**Slice:** live gameplay pacing regression, found while profiling the windowed
match (2026-11-09). No new native body; this fixes the port's lazy-delta model
inside the already-ported mixer.

## 1. Symptom

Windowed live play ran slower than wall time in episodes: the HUD clock and
granted frames lagged the 100 Hz PIT counter, especially across goal/restart
phases. A 640x480 release run held RIGHT for three seconds, then sampled the
`FIFA96_CAMERA_TRACE` sink once per second:

```
t+1s  f=143 clock=00:04 tick=480
t+2s  f=167 clock=00:05 tick=560
...
t+8s  f=320 clock=00:10 tick=1070
t+15s f=519 clock=00:16 tick=1734
```

`tick` is the registered 100 Hz callback counter, so the 5 s → 16 s clock
window spans 1174 ticks (11.7 s of simulation) over ~14 s of wall time. The
step then recovered to 100 ticks/s, which pointed at a phase-dependent
per-step stall rather than a constant wrong pace constant.

## 2. Measurement

Temporary step instrumentation (not committed) bracketed the engine step:

```
slow-step mode=3 phase=7 ticks=10 raw=153232480 cap=100000000
          clock=0 poll=0 run=0 render=9 audio=145 present=0 total=155
```

- `render` = `fifa96_match_run_render` (the pitch/scene composer): **9 ms**.
- `run` = `fifa96_match_run_step`: **0 ms** (the frame body runs on the
  100 Hz tick trampoline, not in `run_step`).
- `audio` = `fifa96_match_audio_step`: **110–145 ms**, the whole stall.
- The match frame-body timer (`fifa96_match_run_frame`) stayed **below 5 ms**
  on every granted frame, so the simulation body was not the cost.

Instrumenting `fifa96_mixer_render` found the real work:

```
mixer-slow frames=441 voices=1 delta_calls=2013820 ms=5
```

A 441-frame chunk (20 ms of 22.05 kHz output) decoded ~2.0 million delta units,
about **4 500 units per output frame**. The single active voice was the
CHN_GEN1 crowd loop (`id=55`, `rate=16000`, bank stereo, `step=3116529408` =
0.7256 units/output frame, loop `[10753,108426)` over `units=116736`).

## 3. Root cause

`struct fifa96_mixer_voice.delta_pos` is the **next** unit to decode: after
decoding unit `U`, `delta_pos == U + 1` (`fifa96_mixer.h`). The lazy render
path reset the decoder whenever `unit < delta_pos`:

```c
if (unit < ch->delta_pos) { reset header/state; delta_pos = 0; }
while (ch->delta_pos <= unit) { decode one unit; delta_pos++; }
```

At a 0.7256 source-unit step, the resampler cursor stays on the same source
unit for roughly 27% of output frames. On such a hold, `unit == U` while
`delta_pos == U + 1`, so the old test reset the chain and replayed units
`0..U` from zero **on every held frame**. The decode cost therefore grew with
the cursor position (O(n²) over the loop), and the 6.1 s crowd loop's audio
pump consumed 100–145 ms per engine step. When a step exceeded the engine's
100 ms catch-up cap, PIT ticks were dropped and the match clock lagged.

## 4. Native evidence

The native loop reader `0xB832D` wraps only when the remaining loop window is
**shorter than one resample step**:

```
0xB832D  MOV EDX,[ESI+0x20]        ; loop end
0xB8330  SUB EDX,[ESI+0x14]        ; current cursor
0xB8333  TEST EDX,0xffffffff
0xB8339  JNS 0xB8340               ; clamp negative to 0
0xB833B  MOV EDX,0
0xB8340  MOV EAX,0
0xB8345  SHRD EAX,EDX,0x8
0xB8349  SHR EDX,0x8
0xB834C  DIV dword ptr [ESI+0x2c]  ; remaining frames / step
0xB834F  CMP EAX,0
0xB8352  JNZ 0xB8363               ; at least one frame left -> no wrap
0xB8354  MOV EAX,[ESI+0x1c]        ; loop_start
0xB8357  MOV [ESI+0x14],EAX        ; cursor := loop_start
0xB835A  MOV [ESI+0x18],0          ; fraction := 0
0xB8361  JMP 0xB832D               ; re-check the wrapped window
```

The per-format reader `0xB8610` consumes the cursor at `[ESI+0x38]`, calls the
delta decoder `[ESI+0x58]` at that cursor, and advances the cursor by the
decoded count (`0xB86B6 ADD [ESI+0x38],EAX`). It never replays the chain from
unit 0 to serve a held output frame. A same-unit hold is therefore **not** a
wrap in the native reader; only the `remaining < step` condition at `0xB834F`
is.

Addresses above are first-hand `disassemble_bytes 0xB832D` /
`0xB8610` reads in the `fifa96` Ghidra project (`/FIFA96.EXE`). The port's
lazy one-unit decoder is a derived model (FU-39 §3/§4.2); its reset adapter
must follow the same boundary.

## 5. Fix

`fifa96_mixer_delta_cursor_reset(unit, decoded)` returns 1 only for a true
backward jump:

```c
return decoded != 0u && unit < decoded - 1u;
```

The render path calls the predicate before the replay. A cursor parked on the
last decoded unit (`unit + 1 == decoded`) keeps the cached sample and the
state chain; a loop wrap still resets.

Tests:
- `test_mixer.c::test_delta_fractional_step` renders the 3-unit bank-stereo
  fixture at step 0.5 and asserts the held samples
  `(0,1),(0,1),(2,5),(2,5),(6,12),(6,12)` and `delta_pos == 3`.
- `test_mixer.c::test_delta_cursor_reset` pins the predicate boundary:
  same-unit holds and forward moves return 0; a backward jump returns 1. With
  the pre-fix `unit < decoded` predicate restored, this test aborts at the
  `(0,1)` hold case.

## 6. Verification

With the temporary step/mixer instrumentation still present, the same drive
emitted **no** `slow-step` or `mixer-slow` line (the pre-fix run emitted them
continuously). The final clean build, without instrumentation, sampled the
trace once per second through the same goal/restart window:

```
f=150 clock=00:05 tick=504
f=180 clock=00:05 tick=604
f=210 clock=00:06 tick=704
...
f=481 clock=00:15 tick=1607
```

That is exactly 100 ticks and 30 granted frames per wall second, with the
clock advancing 1 s per second. `make check` is green; M1/M2 goldens are
byte-exact. No score, phase, entity, ball, camera, render or census cell
changes; the fix is inside the mixer's delta cursor state only.

## 7. Open legs

None added. The native reader decodes fixed blocks from the cursor while the
port decodes lazily per output frame; that modelling difference is already
recorded in FU-39 §7 and is unchanged by this fix. The native signed-arm
one-unit truncation (FU-39 §4.3) and the unsigned bank loader data mapping
(FU-39 §2.2/§7 leg 3) remain where they were.
