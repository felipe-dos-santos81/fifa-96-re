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
  /* planar-chunky Mode-X: every pixel's full index byte lands in plane x & 3. */
  for (int p = 0; p < 4; p++)
    for (size_t i = 0; i < 80u * 240u; i++)
      assert(f.planes[p][i] == 0x11u);
  assert(f.stride == 80u);
  fifa96_surface_clear(s, 0x00);
  fifa96_surface_plane(s, &f);
  assert(f.planes[0][0] == 0x00u && f.planes[3][0] == 0x00u);

  /* Non-uniform pixels distinguish the planes and the byte column. */
  fifa96_surface_clear(s, 0xA5);
  s->indexed[0] = 0x10;  /* (0,0) -> plane 0, column 0 */
  s->indexed[1] = 0x20;  /* (1,0) -> plane 1, column 0 */
  s->indexed[2] = 0x30;  /* (2,0) -> plane 2, column 0 */
  s->indexed[3] = 0x40;  /* (3,0) -> plane 3, column 0 */
  fifa96_surface_plane(s, &f);
  assert(f.planes[0][0] == 0x10u && f.planes[1][0] == 0x20u);
  assert(f.planes[2][0] == 0x30u && f.planes[3][0] == 0x40u);
  assert(f.planes[0][1] == 0xA5u);  /* (4,0) -> plane 0, column 1 */

  fifa96_surface_destroy(s);
  puts("test_engine_surface OK");
  return 0;
}
