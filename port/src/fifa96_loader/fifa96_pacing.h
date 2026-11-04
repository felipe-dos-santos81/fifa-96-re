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

// FU-48 §6 — the PIT ISR tick model. FUN_0009F754 programs PIT channel 0
// with divisor bytes 0x9C/0x2E (0x9F799..0x9F7B0, `OUT 0x40`) = 0x2E9C, so
// 1193182/0x2E9C = 100.0 Hz. The ISR at 0x9F5E4 increments [0x12E88] once
// per interrupt (`MOV EDX,[0x12E88]; INC EDX; MOV [0x12E88],EDX`
// 0x9F5F5..0x9F601), then `MOV EBX,5; IDIV EBX; TEST EDX,EDX` (0x9F5FC..
// 0x9F60E): every 5th tick (20 Hz) increments [0x12E8C] and far-calls the
// saved handler, otherwise the ISR sends EOI directly. The game-side
// consumers are pure functions over the counter: FUN_000cb2aa (0xCB2AA) is
// `now - then` (32-bit wrap) and 0xCB2EF returns `(int32)(now-deadline) >= 0`
// (SUB/SBB/INC at 0xCB2EF..0xCB2FE, the matching wait loop at 0xCB2E1 is
// signed). Existing pacing APIs are unchanged; this struct is the ISR model.
struct fifa96_pacing_clock {
  uint32_t ticks;    /* [0x12E88]: 100 Hz centisecond counter */
  uint32_t ticks20;  /* [0x12E8C]: 20 Hz callback counter */
};

/* Zero both counters (the image is BSS). */
void fifa96_pacing_clock_init(struct fifa96_pacing_clock *c);

// One PIT interrupt (0x9F5F5..0x9F61F): ticks++, and on a tick that is a
// multiple of 5 (the IDIV remainder is zero; divisibility is the same for
// the original's signed IDIV and an unsigned modulo) ticks20++ and the
// 20 Hz callback fires. Returns 1 when it fired, else 0. NULL -> 0.
int fifa96_pacing_clock_isr(struct fifa96_pacing_clock *c);

/* 0xCB2AA: elapsed = now - then with 32-bit wrap. */
uint32_t fifa96_pacing_clock_elapsed(uint32_t then, uint32_t now);

/* 0xCB2EF: 1 once now >= deadline under the original's signed comparison. */
int fifa96_pacing_deadline_reached(uint32_t deadline, uint32_t now);
