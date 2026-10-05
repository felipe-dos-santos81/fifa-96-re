// tests/test_blit.c — Mode-X blitter: hand-checked planar interleave, clip
// windows, page offsets, capacity errors and the fVGT-01 full frame.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_blit.h"
#include "fifa96_loader/fifa96_file.h"

#define ROW 80u
#define CAP 0x9600u

static uint8_t planes_buf[4][CAP];
static fifa96_modex_image img;

static void reset(void) {
  memset(planes_buf, 0, sizeof planes_buf);
  for (int p = 0; p < 4; p++) img.planes[p] = planes_buf[p];
  img.plane_cap = CAP;
}

static fifa96_blit_clip full_clip(void) {
  fifa96_blit_clip c;
  c.left = 0;
  c.top = 0;
  c.right = (int32_t)FIFA96_MODEX_WIDTH;
  c.bottom = 480;
  return c;
}

static uint64_t fnv1a64(const uint8_t *p, size_t n) {
  uint64_t h = 14695981039346656037ull;
  for (size_t i = 0; i < n; i++) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}

static void blit_ok(int32_t x, int32_t y, const uint8_t *src, size_t src_len,
                    uint32_t w, uint32_t h, fifa96_blit_clip clip) {
  assert(fifa96_blit_modex(&img, x, y, src, src_len, w, h, &clip) == FIFA96_OK);
}

static void all_zero_but(const size_t *offs, const uint8_t *vals, size_t n) {
  for (int p = 0; p < 4; p++) {
    for (size_t i = 0; i < CAP; i++) {
      size_t off = (size_t)p * CAP + i;
      int hit = 0;
      for (size_t j = 0; j < n; j++)
        if (offs[j] == off) {
          assert(planes_buf[p][i] == vals[j]);
          hit = 1;
        }
      if (!hit) assert(planes_buf[p][i] == 0);
    }
  }
}

int main(void) {
  reset();

  {
    const uint8_t src[4] = {1, 2, 3, 4};
    blit_ok(0, 0, src, sizeof src, 4, 1, full_clip());
    assert(planes_buf[0][0] == 1);
    assert(planes_buf[1][0] == 0 && planes_buf[2][0] == 0 && planes_buf[3][0] == 0);
    const size_t offs[1] = {0};
    const uint8_t vals[1] = {1};
    all_zero_but(offs, vals, 1);
  }

  reset();
  {
    const uint8_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    blit_ok(0, 0, src, sizeof src, 8, 1, full_clip());
    assert(planes_buf[0][0] == 1 && planes_buf[0][1] == 5);
    assert(planes_buf[1][0] == 2 && planes_buf[2][0] == 3);
    assert(planes_buf[3][0] == 4);
    assert(planes_buf[0][2] == 0 && planes_buf[1][1] == 0);
    const size_t offs[5] = {0, 1, CAP, 2 * CAP, 3 * CAP};
    const uint8_t vals[5] = {1, 5, 2, 3, 4};
    all_zero_but(offs, vals, 5);
  }

  reset();
  {
    const uint8_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    blit_ok(2, 0, src, sizeof src, 8, 1, full_clip());
    assert(planes_buf[2][0] == 1 && planes_buf[2][1] == 5);
    assert(planes_buf[3][0] == 2 && planes_buf[0][1] == 3);
    assert(planes_buf[1][1] == 4);
    assert(planes_buf[3][1] == 0 && planes_buf[2][2] == 0);
    const size_t offs[5] = {2 * CAP, 2 * CAP + 1, 3 * CAP, CAP + 1, 1};
    const uint8_t vals[5] = {1, 5, 2, 4, 3};
    all_zero_but(offs, vals, 5);
  }

  reset();
  {
    const uint8_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    blit_ok(0, 0, src, sizeof src, 4, 2, full_clip());
    assert(planes_buf[0][0] == 1 && planes_buf[0][ROW] == 5);
    assert(planes_buf[1][0] == 0 && planes_buf[3][ROW] == 0);
    const size_t offs[2] = {0, ROW};
    const uint8_t vals[2] = {1, 5};
    all_zero_but(offs, vals, 2);
  }

  reset();
  {
    const uint8_t one[1] = {9};
    blit_ok(0, 0, one, sizeof one, 1, 1, full_clip());
    for (int p = 0; p < 4; p++) assert(planes_buf[p][0] == 0);
  }
  {
    const uint8_t src[6] = {1, 2, 3, 4, 5, 6};
    blit_ok(0, 0, src, sizeof src, 6, 1, full_clip());
    assert(planes_buf[0][0] == 1 && planes_buf[1][0] == 2 && planes_buf[2][0] == 3);
    assert(planes_buf[3][0] == 0 && planes_buf[0][1] == 0);
  }

  reset();
  {
    uint8_t src[16];
    for (int i = 0; i < 16; i++) src[i] = (uint8_t)(i + 1);
    blit_ok(0, -2, src, sizeof src, 4, 4, full_clip());
    assert(planes_buf[0][0] == 9 && planes_buf[0][ROW] == 13);
    for (int p = 1; p < 4; p++) assert(planes_buf[p][0] == 0 && planes_buf[p][ROW] == 0);
    assert(planes_buf[0][2 * ROW] == 0);
  }

  reset();
  {
    const uint8_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    blit_ok(-3, 0, src, sizeof src, 8, 1, full_clip());
    assert(planes_buf[0][0] == 4 && planes_buf[1][0] == 5);
    assert(planes_buf[2][0] == 0 && planes_buf[3][0] == 0);
    assert(planes_buf[0][1] == 0);
  }

  reset();
  {
    const uint8_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    blit_ok(316, 0, src, sizeof src, 8, 1, full_clip());
    assert(planes_buf[0][79] == 1);
    assert(planes_buf[1][79] == 0 && planes_buf[2][79] == 0 && planes_buf[3][79] == 0);
    assert(planes_buf[0][78] == 0);
  }

  reset();
  {
    uint8_t src[16];
    for (int i = 0; i < 16; i++) src[i] = (uint8_t)(i + 1);
    blit_ok(0, 238, src, sizeof src, 4, 4, full_clip());
    assert(planes_buf[0][238 * ROW] == 1 && planes_buf[0][239 * ROW] == 5);
    for (int p = 1; p < 4; p++) assert(planes_buf[p][238 * ROW] == 0);
    assert(planes_buf[0][239 * ROW + 1] == 0);
  }

  reset();
  {
    uint8_t src[64];
    for (int i = 0; i < 64; i++) src[i] = (uint8_t)(i + 1);
    fifa96_blit_clip c;
    c.left = 1;
    c.top = 1;
    c.right = 7;
    c.bottom = 5;
    blit_ok(0, 0, src, sizeof src, 8, 8, c);
    assert(planes_buf[1][ROW] == 10 && planes_buf[2][ROW] == 11);
    assert(planes_buf[3][ROW] == 12 && planes_buf[0][ROW + 1] == 0);
    assert(planes_buf[1][2 * ROW] == 18 && planes_buf[2][2 * ROW] == 19);
    assert(planes_buf[3][2 * ROW] == 20 && planes_buf[1][4 * ROW] == 34);
    assert(planes_buf[2][4 * ROW] == 35 && planes_buf[3][4 * ROW] == 36);
    assert(planes_buf[1][3 * ROW] == 26 && planes_buf[0][5 * ROW] == 0);
    const size_t offs[12] = {CAP + ROW, 2 * CAP + ROW, 3 * CAP + ROW,
                             CAP + 2 * ROW, 2 * CAP + 2 * ROW, 3 * CAP + 2 * ROW,
                             CAP + 3 * ROW, 2 * CAP + 3 * ROW, 3 * CAP + 3 * ROW,
                             CAP + 4 * ROW, 2 * CAP + 4 * ROW, 3 * CAP + 4 * ROW};
    const uint8_t vals[12] = {10, 11, 12, 18, 19, 20, 26, 27, 28, 34, 35, 36};
    all_zero_but(offs, vals, 12);
  }

  reset();
  {
    const uint8_t src[4] = {1, 2, 3, 4};
    fifa96_blit_clip zero;
    zero.left = 0;
    zero.top = 0;
    zero.right = 0;
    zero.bottom = 0;
    blit_ok(0, 0, src, sizeof src, 4, 1, zero);
    blit_ok(400, 0, src, sizeof src, 4, 1, full_clip());
    blit_ok(0, 500, src, sizeof src, 4, 1, full_clip());
    for (int p = 0; p < 4; p++)
      for (size_t i = 0; i < CAP; i++) assert(planes_buf[p][i] == 0);
  }

  assert(fifa96_blit_page_rows(0) == 0 && fifa96_blit_page_rows(1) == 240);
  assert(fifa96_blit_page_rows(2) == 480);
  assert(fifa96_blit_page_start(0) == 0);
  assert(fifa96_blit_page_start(1) == 0x4B00u);
  assert(fifa96_blit_page_start(2) == 0x9600u);

  reset();
  {
    const uint8_t src[4] = {9, 8, 7, 6};
    blit_ok(0, (int32_t)fifa96_blit_page_rows(1) + 2, src, sizeof src, 4, 1, full_clip());
    assert(planes_buf[0][242 * ROW] == 9);
    assert(planes_buf[1][242 * ROW] == 0 && planes_buf[2][242 * ROW] == 0);
    assert(planes_buf[3][242 * ROW] == 0 && planes_buf[0][0] == 0);
  }

  memset(planes_buf, 0xA5, sizeof planes_buf);
  img.plane_cap = 240;
  {
    uint8_t src[16];
    memset(src, 7, sizeof src);
    fifa96_blit_clip c = full_clip();
    assert(fifa96_blit_modex(&img, 0, 0, src, sizeof src, 4, 4, &c) == -(int)FIFA96_ERR_TRUNCATED);
  }
  img.plane_cap = CAP;
  {
    const uint8_t src[4] = {1, 2, 3, 4};
    fifa96_blit_clip c = full_clip();
    assert(fifa96_blit_modex(&img, 0, 0, src, 3, 4, 1, &c) == -(int)FIFA96_ERR_TRUNCATED);
    assert(fifa96_blit_modex(&img, 0, 0, NULL, 4, 4, 1, &c) == -(int)FIFA96_ERR_TRUNCATED);
    assert(fifa96_blit_modex(&img, 0, 0, src, sizeof src, 4, 1, NULL) == -(int)FIFA96_ERR_TRUNCATED);
    assert(fifa96_blit_modex(NULL, 0, 0, src, sizeof src, 4, 1, &c) == -(int)FIFA96_ERR_TRUNCATED);
    img.planes[2] = NULL;
    assert(fifa96_blit_modex(&img, 0, 0, src, sizeof src, 4, 1, &c) == -(int)FIFA96_ERR_TRUNCATED);
    img.planes[2] = planes_buf[2];
  }
  for (int p = 0; p < 4; p++)
    for (size_t i = 0; i < CAP; i++) assert(planes_buf[p][i] == 0xA5);

  {
    uint8_t *frame = NULL;
    size_t frame_n = 0;
    assert(fifa96_file_read("tests/golden/vgt/fvgt-01.out.bin", &frame, &frame_n) == 0);
    assert(frame && frame_n == 76800);
    static const uint64_t want0[4] = {
        0xcc222176698ac56full, 0x9861d34fec010a88ull,
        0xa5c038e050b6dae3ull, 0x6e442ac8dd2d0359ull};
    static const uint64_t want1[4] = {
        0x3e6e8ca5317e0d6full, 0x2be2afbf8f200688ull,
        0x0d9f9eb6fbea32e3ull, 0xab01cf45ee341359ull};

    reset();
    blit_ok(0, 0, frame, frame_n, 320, 240, full_clip());
    for (int p = 0; p < 4; p++) assert(fnv1a64(planes_buf[p], CAP) == want0[p]);
    assert(fnv1a64((const uint8_t *)planes_buf, 4 * CAP) == 0xfb746f9ffb867ac8ull);
    static const uint8_t spot_p2_row57[8] = {5, 5, 1, 5, 1, 2, 8, 22};
    assert(memcmp(planes_buf[0], (const uint8_t[]){1, 1, 5, 5, 5, 5, 5, 5}, 8) == 0);
    assert(memcmp(planes_buf[1], (const uint8_t[]){1, 5, 5, 5, 5, 5, 5, 5}, 8) == 0);
    assert(memcmp(planes_buf[2] + 57 * ROW, spot_p2_row57, 8) == 0);
    static const uint8_t spot_p0_row128[4] = {0x18, 0x1e, 0x1e, 0x1e};
    assert(memcmp(planes_buf[0] + 128 * ROW, spot_p0_row128, 4) == 0);
    for (uint32_t y = 0; y < 240; y++) {
      for (uint32_t x = 0; x < 320; x++) {
        uint8_t got = planes_buf[x & 3][y * ROW + (x >> 2)];
        if (x < 317) assert(got == frame[y * 320 + x]);
        else assert(got == 0);
      }
    }

    reset();
    blit_ok(0, (int32_t)fifa96_blit_page_rows(1), frame, frame_n, 320, 240, full_clip());
    for (int p = 0; p < 4; p++) assert(fnv1a64(planes_buf[p], CAP) == want1[p]);
    assert(fnv1a64((const uint8_t *)planes_buf, 4 * CAP) == 0x28aee86fd93a76c8ull);
    for (int p = 0; p < 4; p++)
      for (size_t i = 0; i < 0x4B00u; i++) assert(planes_buf[p][i] == 0);
    for (uint32_t y = 0; y < 240; y++) {
      for (uint32_t x = 0; x < 320; x++) {
        uint8_t got = planes_buf[x & 3][(y + 240) * ROW + (x >> 2)];
        if (x < 317) assert(got == frame[y * 320 + x]);
        else assert(got == 0);
      }
    }

    free(frame);
  }

  printf("test_blit: ok\n");
  return 0;
}
