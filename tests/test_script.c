// tests/test_script.c — synthetic mechanism vectors cite motivating instructions; golden vectors (if any) cite xxd + Task-1 attribution
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_script.h"
int main(void) {
  uint16_t w[4]; fifa96_script_table_t t = { w, 0, 4 };
  size_t at = 0;
  /* table mechanics: terminator 0xffff per MOV word ptr [SI],0xffff at 11bd:5d18 */
  assert(fifa96_script_table_append(&t, 0x0043) == 0);   /* 'C' dispatch byte 0x43 at 11bd:5cb7 */
  assert(fifa96_script_table_append(&t, 0x0045) == 0);   /* 'E' dispatch byte 0x45 at 11bd:5cbd */
  assert(fifa96_script_table_append(&t, 0x004d) == 0);   /* 'M' dispatch byte 0x4d at 11bd:5cc0 */
  assert(t.len == 3);
  assert(fifa96_script_table_append(&t, 0xffff) == 0);
  assert(fifa96_script_table_terminated(w, t.len, &at) == 0 && at == 3);
  assert(fifa96_script_table_append(&t, 0x0001) == FIFA96_ERR_TRUNCATED); /* cap 4 full: limit idiom CMP AX,SI at 11bd:5d4d */
  /* negatives */
  assert(fifa96_script_table_append(0, 1) == FIFA96_ERR_TRUNCATED);
  { fifa96_script_table_t n = { 0, 0, 0 }; assert(fifa96_script_table_append(&n, 1) == FIFA96_ERR_TRUNCATED); }
  assert(fifa96_script_table_terminated(0, 1, &at) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_script_table_terminated(w, 3, 0) == FIFA96_ERR_TRUNCATED);
  { static const uint16_t u[] = { 0x15e8, 0x0001 }; assert(fifa96_script_table_terminated(u, 2, &at) == FIFA96_ERR_TRUNCATED); } /* no 0xffff in range */
  /* comment skip (ONLY if Task-1 ';' row is CONFIRMED-skip; delete this block otherwise) */
  { static const uint8_t c[] = { 0x3b, 0x41, 0x0a, 0x42 }; assert(fifa96_script_comment_len(c, 4) == 3); } /* ';A\nB': SUB AX,0x3c at 11bd:5be9 */
  { static const uint8_t c[] = { 0x41, 0x0a }; assert(fifa96_script_comment_len(c, 2) == 0); }
  assert(fifa96_script_comment_len(0, 4) == 0);
  { static const uint8_t c[] = { 0x3b, 0x41 }; assert(fifa96_script_comment_len(c, 2) == 2); } /* ';' runs to EOF with no newline: skip-to-end models 5c8b EOF edge (OR/JGE at 11bd:5d70/5d72 skipping 0xffff store) */
  /* golden vectors (ONLY files Task 1 attributed; bytes verbatim, each line cites xxd + attribution) */
  printf("test_script OK\n");
  return 0;
}
