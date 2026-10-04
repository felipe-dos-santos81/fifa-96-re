#pragma once
#include <stdint.h>
// FU-37 Part B — video pacing. The original clock FUN_000cb2a4 @ 0xCB2A4
// reads the 100 Hz centisecond counter 0x12E88 (PIT divisor 0x2E9C,
// FUN_0009F754 @ 0x9F754); vgt_stream_poll @ 0x67BA8 paces video with
// progress = clock()*15/100 (15 fps) when the sound card is absent, and a
// catch-up bound of frames displayed + 2 when it is audio-clocked.

/* Progress units due after `ticks` centisecond ticks (0x67E41). */
uint32_t fifa96_pacing_frames_due(uint32_t ticks);

// 1 if the player should decode the next frame immediately: the on-time
// bound is frames_before + 2 (0x563f0 = 0x563dc + 2, 0x67CC5), compared
// after the frame counter increment (0x67E60, 0x67E66..0x67E8C), so
// `frames_before` is the count at frame start. 0 = return and wait.
int fifa96_pacing_catch_up(uint32_t frames_before, uint32_t progress);
