#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "fifa96_loader/fifa96_vga.h"

static void test_pages(void) {
  assert(fifa96_vga_page_rows(0) == 0);
  assert(fifa96_vga_page_rows(1) == 240);
  assert(fifa96_vga_page_rows(2) == 480);
  assert(fifa96_vga_page_start(0) == 0);
  assert(fifa96_vga_page_start(1) == 0x4B00u);
  assert(fifa96_vga_page_start(2) == 0x9600u);
  assert(fifa96_vga_page_bytes(0) == 0);
  assert(fifa96_vga_page_bytes(1) == 0x12C00u);
  assert(fifa96_vga_page_bytes(2) == 0x25800u);
}

static void test_flip(void) {
  assert(fifa96_vga_flip(0) == 1);
  assert(fifa96_vga_flip(1) == 0);
  assert(fifa96_vga_flip(2) == 3);
  assert(fifa96_vga_flip(0xFFFFFFFFu) == 0xFFFFFFFEu);
}

static void test_dac(void) {
  assert(fifa96_vga_dac_component(0) == 0);
  assert(fifa96_vga_dac_component(3) == 0);
  assert(fifa96_vga_dac_component(0x3F) == 0x0F);
  assert(fifa96_vga_dac_component(0x40) == 0x10);
  assert(fifa96_vga_dac_component(0x7F) == 0x1F);
  assert(fifa96_vga_dac_component(0x80) == 0x20);
  assert(fifa96_vga_dac_component(0xFF) == 0x3F);
}

static void test_crtc_split(void) {
  assert(fifa96_vga_crtc_start_high(0x4B00) == 0x4B);
  assert(fifa96_vga_crtc_start_low(0x4B00) == 0x00);
  assert(fifa96_vga_crtc_start_high(0x12C00) == 0x2C);
  assert(fifa96_vga_crtc_start_low(0x12C00) == 0x00);
  assert(fifa96_vga_crtc_start_high(0x1234) == 0x12);
  assert(fifa96_vga_crtc_start_low(0x1234) == 0x34);
}

static void test_palette(void) {
  uint8_t rgb[768];
  fifa96_vga_port_write rec[FIFA96_VGA_PALETTE_UPLOAD_RECORDS];
  for (size_t i = 0; i < sizeof rgb; i++) rgb[i] = (uint8_t)i;

  {
    const uint8_t two[6] = {0, 1, 2, 252, 253, 254};
    memset(rec, 0xA5, sizeof rec);
    int n = fifa96_vga_palette_upload(0, 2, two, rec, 7);
    assert(n == 7);
    assert(rec[0].port == FIFA96_VGA_DAC_INDEX_PORT && rec[0].value == 0);
    assert(rec[1].port == FIFA96_VGA_DAC_DATA_PORT && rec[1].value == 0);
    assert(rec[2].port == FIFA96_VGA_DAC_DATA_PORT && rec[2].value == 0);
    assert(rec[3].port == FIFA96_VGA_DAC_DATA_PORT && rec[3].value == 0);
    assert(rec[4].port == FIFA96_VGA_DAC_DATA_PORT && rec[4].value == 63);
    assert(rec[5].port == FIFA96_VGA_DAC_DATA_PORT && rec[5].value == 63);
    assert(rec[6].port == FIFA96_VGA_DAC_DATA_PORT && rec[6].value == 63);

    memset(rec, 0xA5, sizeof rec);
    assert(fifa96_vga_palette_upload(0, 2, two, rec, 6) == -(int)FIFA96_ERR_TRUNCATED);
    assert(rec[0].port == 0xA5A5u && rec[0].value == 0xA5);
  }

  {
    int n = fifa96_vga_palette_upload(255, 1, rgb, rec, 4);
    assert(n == 4);
    assert(rec[0].port == FIFA96_VGA_DAC_INDEX_PORT && rec[0].value == 255);
    assert(rec[1].value == fifa96_vga_dac_component(rgb[0]));
    assert(rec[2].value == fifa96_vga_dac_component(rgb[1]));
    assert(rec[3].value == fifa96_vga_dac_component(rgb[2]));
  }

  {
    int n = fifa96_vga_palette_upload(7, 0, NULL, rec, 1);
    assert(n == 1);
    assert(rec[0].port == FIFA96_VGA_DAC_INDEX_PORT && rec[0].value == 7);
  }

  {
    int n = fifa96_vga_palette_upload(0, 256, rgb, rec, FIFA96_VGA_PALETTE_UPLOAD_RECORDS);
    assert(n == (int)FIFA96_VGA_PALETTE_UPLOAD_RECORDS);
    assert(rec[1].value == fifa96_vga_dac_component(rgb[0]));
    assert(rec[768].port == FIFA96_VGA_DAC_DATA_PORT);
    assert(rec[768].value == fifa96_vga_dac_component(rgb[767]));
  }

  assert(fifa96_vga_palette_upload(256, 1, rgb, rec, 4) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_vga_palette_upload(200, 57, rgb, rec, 172) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_vga_palette_upload(0, 257, rgb, rec, 772) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_vga_palette_upload(0, 1, NULL, rec, 4) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_vga_palette_upload(0, 1, rgb, NULL, 4) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_vga_palette_upload(0, 0, NULL, NULL, 0) == -(int)FIFA96_ERR_TRUNCATED);
}

static void test_order(void) {
  fifa96_vga_step steps[6];
  memset(steps, 0, sizeof steps);

  int n = fifa96_vga_present_order(1, steps, 6);
  assert(n == 6);
  assert(steps[0] == FIFA96_VGA_STEP_BLIT);
  assert(steps[1] == FIFA96_VGA_STEP_CRTC_START_HIGH);
  assert(steps[2] == FIFA96_VGA_STEP_CRTC_START_LOW);
  assert(steps[3] == FIFA96_VGA_STEP_WAIT_VERTICAL_RETRACE);
  assert(steps[4] == FIFA96_VGA_STEP_DAC_UPLOAD);
  assert(steps[5] == FIFA96_VGA_STEP_FLIP);

  n = fifa96_vga_present_order(0, steps, 6);
  assert(n == 4);
  assert(steps[0] == FIFA96_VGA_STEP_BLIT);
  assert(steps[1] == FIFA96_VGA_STEP_CRTC_START_HIGH);
  assert(steps[2] == FIFA96_VGA_STEP_CRTC_START_LOW);
  assert(steps[3] == FIFA96_VGA_STEP_FLIP);

  assert(fifa96_vga_present_order(1, steps, 5) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_vga_present_order(0, NULL, 4) == -(int)FIFA96_ERR_TRUNCATED);
}

int main(void) {
  test_pages();
  test_flip();
  test_dac();
  test_crtc_split();
  test_palette();
  test_order();
  return 0;
}
