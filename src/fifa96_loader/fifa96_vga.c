#include "fifa96_loader/fifa96_vga.h"
#include "fifa96_loader/fifa96_blit.h"

uint32_t fifa96_vga_page_rows(uint32_t parity) {
  return fifa96_blit_page_rows(parity);
}

uint32_t fifa96_vga_page_start(uint32_t parity) {
  return fifa96_blit_page_start(parity);
}

uint32_t fifa96_vga_page_bytes(uint32_t parity) {
  return parity * FIFA96_MODEX_PAGE_LINEAR_OFFSET;
}

uint32_t fifa96_vga_flip(uint32_t parity) {
  return parity ^ 1u;
}

uint8_t fifa96_vga_dac_component(uint8_t component) {
  return (uint8_t)(component >> FIFA96_VGA_DAC_COMPONENT_SHIFT);
}

uint8_t fifa96_vga_crtc_start_high(uint32_t start) {
  return (uint8_t)((start >> 8) & 0xFFu);
}

uint8_t fifa96_vga_crtc_start_low(uint32_t start) {
  return (uint8_t)(start & 0xFFu);
}

int fifa96_vga_palette_upload(uint32_t first, uint32_t count, const uint8_t *rgb,
                              fifa96_vga_port_write *out, size_t out_cap) {
  if (first >= FIFA96_VGA_PALETTE_ENTRIES) return -(int)FIFA96_ERR_TRUNCATED;
  if (count > FIFA96_VGA_PALETTE_ENTRIES) return -(int)FIFA96_ERR_TRUNCATED;
  if (first + count > FIFA96_VGA_PALETTE_ENTRIES) return -(int)FIFA96_ERR_TRUNCATED;
  if (count != 0 && !rgb) return -(int)FIFA96_ERR_TRUNCATED;
  if (!out) return -(int)FIFA96_ERR_TRUNCATED;
  size_t need = 1u + (size_t)count * 3u;
  if (out_cap < need) return -(int)FIFA96_ERR_TRUNCATED;
  out[0].port = FIFA96_VGA_DAC_INDEX_PORT;
  out[0].value = (uint8_t)first;
  size_t k = 1;
  for (uint32_t i = 0; i < count; i++) {
    for (uint32_t c = 0; c < 3u; c++) {
      out[k].port = FIFA96_VGA_DAC_DATA_PORT;
      out[k].value = fifa96_vga_dac_component(rgb[(size_t)i * 3u + c]);
      k++;
    }
  }
  return (int)need;
}

int fifa96_vga_present_order(int palette_changed, fifa96_vga_step *out, size_t out_cap) {
  if (!out) return -(int)FIFA96_ERR_TRUNCATED;
  size_t need = palette_changed ? FIFA96_VGA_PRESENT_STEPS_PALETTE
                                : FIFA96_VGA_PRESENT_STEPS_PLAIN;
  if (out_cap < need) return -(int)FIFA96_ERR_TRUNCATED;
  out[0] = FIFA96_VGA_STEP_BLIT;
  out[1] = FIFA96_VGA_STEP_CRTC_START_HIGH;
  out[2] = FIFA96_VGA_STEP_CRTC_START_LOW;
  if (palette_changed) {
    out[3] = FIFA96_VGA_STEP_WAIT_VERTICAL_RETRACE;
    out[4] = FIFA96_VGA_STEP_DAC_UPLOAD;
    out[5] = FIFA96_VGA_STEP_FLIP;
  } else {
    out[3] = FIFA96_VGA_STEP_FLIP;
  }
  return (int)need;
}
