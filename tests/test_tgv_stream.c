// tests/test_tgv_stream.c — chunk walker over the committed real TGV frame.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_kvgt.h"
#include "fifa96_loader/fifa96_tgv_stream.h"

int main(void) {
  uint8_t *frame = 0;
  size_t frame_n = 0;
  assert(fifa96_file_read("tests/golden/vgt/kvgt-frame-01.bin", &frame, &frame_n) == 0);
  assert(frame_n == 5788);

  /* the committed chunk walks as exactly one kVGT frame. */
  fifa96_tgv_walk w = {frame, frame_n, 0};
  const uint8_t *chunk = 0;
  size_t chunk_n = 0;
  int is_frame = 0;
  assert(fifa96_tgv_walk_next(&w, &chunk, &chunk_n, &is_frame) == 1);
  assert(is_frame == 1);
  assert(chunk == frame && chunk_n == frame_n);
  assert(fifa96_tgv_walk_next(&w, &chunk, &chunk_n, &is_frame) == 0);

  /* synthetic stream: unknown chunk, real frame, unknown chunk. */
  size_t pad = 16;
  size_t total = pad + frame_n + pad;
  uint8_t *buf = malloc(total);
  assert(buf);
  uint8_t *q = buf;
  for (int k = 0; k < 2; k++) {
    memset(q, 0x5A, pad);
    q[0] = 0x31; q[1] = 0x53; q[2] = 0x4E; q[3] = 0x68;  /* observed companion tag */
    q[4] = (uint8_t)pad; q[5] = 0; q[6] = 0; q[7] = 0;
    q += pad;
    if (k == 0) { memcpy(q, frame, frame_n); q += frame_n; }
  }
  fifa96_tgv_walk w2 = {buf, total, 0};
  int frames = 0;
  int chunks = 0;
  while (1) {
    int r = fifa96_tgv_walk_next(&w2, &chunk, &chunk_n, &is_frame);
    assert(r >= 0);
    if (r == 0) break;
    chunks++;
    if (is_frame) {
      frames++;
      size_t got = 0;
      uint8_t *dst = malloc(9600);
      assert(dst);
      assert(fifa96_kvgt_decode(chunk, chunk_n, dst, 9600, &got, NULL) == 0);
      assert(got == 9600);
      free(dst);
    } else {
      assert(chunk_n == pad);
    }
  }
  assert(chunks == 3 && frames == 1);

  /* malformed chunks. */
  static const uint8_t short_hdr[4] = {0, 0, 0, 0};
  fifa96_tgv_walk w3 = {short_hdr, sizeof short_hdr, 0};
  assert(fifa96_tgv_walk_next(&w3, &chunk, &chunk_n, &is_frame) ==
         -(int)FIFA96_ERR_TRUNCATED);
  static const uint8_t zero_len[16] = {0x31, 0x53, 0x4E, 0x68, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0};
  fifa96_tgv_walk w4 = {zero_len, sizeof zero_len, 0};
  assert(fifa96_tgv_walk_next(&w4, &chunk, &chunk_n, &is_frame) ==
         -(int)FIFA96_ERR_TRUNCATED);
  static const uint8_t overrun[16] = {0x31, 0x53, 0x4E, 0x68, 0x20, 0, 0, 0,
                                      0, 0, 0, 0, 0, 0, 0, 0};
  fifa96_tgv_walk w5 = {overrun, sizeof overrun, 0};
  assert(fifa96_tgv_walk_next(&w5, &chunk, &chunk_n, &is_frame) ==
         -(int)FIFA96_ERR_TRUNCATED);

  /* empty buffer ends cleanly; NULL args error. */
  fifa96_tgv_walk w6 = {frame, 0, 0};
  assert(fifa96_tgv_walk_next(&w6, &chunk, &chunk_n, &is_frame) == 0);
  fifa96_tgv_walk w7 = {frame, frame_n, 0};
  assert(fifa96_tgv_walk_next(NULL, &chunk, &chunk_n, &is_frame) < 0);
  assert(fifa96_tgv_walk_next(&w7, NULL, &chunk_n, &is_frame) < 0);
  assert(fifa96_tgv_walk_next(&w7, &chunk, NULL, &is_frame) < 0);
  assert(fifa96_tgv_walk_next(&w7, &chunk, &chunk_n, NULL) < 0);

  free(buf); free(frame);
  printf("test_tgv_stream OK\n");
  return 0;
}
