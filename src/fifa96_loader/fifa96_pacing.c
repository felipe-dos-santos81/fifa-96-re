#include "fifa96_loader/fifa96_pacing.h"

uint32_t fifa96_pacing_frames_due(uint32_t ticks) { return ticks * 15u / 100u; }

int fifa96_pacing_catch_up(uint32_t frames_before, uint32_t progress) {
  return progress >= frames_before + 2u;
}

void fifa96_pacing_clock_init(struct fifa96_pacing_clock *c) {
  if (!c) return;
  c->ticks = 0;
  c->ticks20 = 0;
}

/* ISR 0x9F5E4: CALL 0x9FFDC (DS reload); STI; CLD; MOV EDX,[0x12E88];
 * INC EDX; MOV EBX,5; MOV [0x12E88],EDX; MOV EAX,EDX; SAR EDX,0x1F;
 * IDIV EBX; TEST EDX,EDX; JNZ <EOI>. The remainder is zero iff the counter
 * is divisible by 5 (signed and unsigned divisibility agree), then
 * INC [0x12E8C] and CALLF [0x12E90]. */
int fifa96_pacing_clock_isr(struct fifa96_pacing_clock *c) {
  if (!c) return 0;
  c->ticks++;
  if (c->ticks % 5u != 0) return 0;
  c->ticks20++;
  return 1;
}

/* FUN_000cb2aa @ 0xCB2AA: MOV EAX,[0x12E88]; SUB EAX,ECX (arg). */
uint32_t fifa96_pacing_clock_elapsed(uint32_t then, uint32_t now) {
  return now - then;
}

/* FUN_000cb2ef @ 0xCB2EF: MOV EBX,[0x12E88]; SUB EBX,[0x127D0];
 * SBB EAX,EAX; INC EAX -> 1 when now - deadline has no borrow (>= 0), 0 when
 * now < deadline. The matching wait loop at 0xCB2E1 uses JS, i.e. the signed
 * difference. */
int fifa96_pacing_deadline_reached(uint32_t deadline, uint32_t now) {
  return (int32_t)(now - deadline) >= 0;
}
