/* tests/test_engine_surface.c */
#include <assert.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_surface.h"

int main(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  uint8_t pal6[768] = {0};
  pal6[0x11 * 3 + 0] = 0x3F;  /* entry 0x11 = bright red */
  fifa96_surface_set_palette6(s, pal6);
  fifa96_surface_clear(s, 0x11);
  assert(s->palette[0x11 * 3 + 0] == 0xFF);  /* 6-bit 0x3F -> 8-bit */

  fifa96_platform_frame f;
  fifa96_surface_plane(s, &f);
  /* 0x11 = 0b00010001 -> plane 0 bit set; planes 1..3 clear (bit 4 is not a plane). */
  for (size_t i = 0; i < 80u * 240u; i++) {
    assert(f.planes[0][i] == 0xFFu);
    assert(f.planes[1][i] == 0x00u);
    assert(f.planes[2][i] == 0x00u);
    assert(f.planes[3][i] == 0x00u);
  }
  fifa96_surface_clear(s, 0x00);
  fifa96_surface_plane(s, &f);
  assert(f.planes[0][0] == 0x00u && f.planes[3][0] == 0x00u);

  fifa96_surface_destroy(s);
  puts("test_engine_surface OK");
  return 0;
}
