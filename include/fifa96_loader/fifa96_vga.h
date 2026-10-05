#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_VGA_DAC_INDEX_PORT 0x3C8u
#define FIFA96_VGA_DAC_DATA_PORT 0x3C9u
#define FIFA96_VGA_CRTC_INDEX_PORT 0x3D4u
#define FIFA96_VGA_CRTC_START_HIGH_INDEX 0x0Cu
#define FIFA96_VGA_CRTC_START_LOW_INDEX 0x0Du
#define FIFA96_VGA_STATUS_PORT 0x3DAu
#define FIFA96_VGA_VERTICAL_RETRACE 0x08u
#define FIFA96_VGA_PALETTE_ENTRIES 256u
#define FIFA96_VGA_DAC_COMPONENT_SHIFT 2u

#define FIFA96_VGA_PALETTE_UPLOAD_RECORDS (1u + 3u * FIFA96_VGA_PALETTE_ENTRIES)
#define FIFA96_VGA_PRESENT_STEPS_PALETTE 6u
#define FIFA96_VGA_PRESENT_STEPS_PLAIN 4u

typedef enum {
  FIFA96_VGA_STEP_BLIT = 0,
  FIFA96_VGA_STEP_CRTC_START_HIGH = 1,
  FIFA96_VGA_STEP_CRTC_START_LOW = 2,
  FIFA96_VGA_STEP_WAIT_VERTICAL_RETRACE = 3,
  FIFA96_VGA_STEP_DAC_UPLOAD = 4,
  FIFA96_VGA_STEP_FLIP = 5
} fifa96_vga_step;

typedef struct {
  uint16_t port;
  uint8_t value;
} fifa96_vga_port_write;

uint32_t fifa96_vga_page_rows(uint32_t parity);
uint32_t fifa96_vga_page_start(uint32_t parity);
uint32_t fifa96_vga_page_bytes(uint32_t parity);
uint32_t fifa96_vga_flip(uint32_t parity);
uint8_t fifa96_vga_dac_component(uint8_t component);
uint8_t fifa96_vga_crtc_start_high(uint32_t start);
uint8_t fifa96_vga_crtc_start_low(uint32_t start);
int fifa96_vga_palette_upload(uint32_t first, uint32_t count, const uint8_t *rgb,
                              fifa96_vga_port_write *out, size_t out_cap);
int fifa96_vga_present_order(int palette_changed, fifa96_vga_step *out, size_t out_cap);
