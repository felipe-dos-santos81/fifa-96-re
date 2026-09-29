// tools/fifa96_dump.c — CLI driver: detect and print FIFA96 CD container headers.
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_envelope.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_pog.h"
#include "fifa96_loader/fifa96_qfs.h"
#include "fifa96_loader/fifa96_tgv.h"
#include "fifa96_loader/fifa96_viv.h"

static void dump_hex(const uint8_t *b, size_t n) {
  size_t i;
  for (i = 0; i < n && i < 16; i++) {
    printf("%02x%s", b[i], (i % 4 == 3) ? " " : "");
  }
  printf("\n");
}

static void try_envelope(const uint8_t *b, size_t n) {
  fifa96_envelope_hdr_t h;
  if (fifa96_envelope_parse_hdr(b, n, &h) == FIFA96_OK) {
    printf("  envelope: magic=0x%04x word_a=0x%04x word_b=0x%04x tag='%.4s' tail_off=%zu tail_len=%zu\n",
           h.magic, h.word_a, h.word_b, h.tag, h.tail_off, h.tail_len);
  }
}

static void try_qfs(const uint8_t *b, size_t n) {
  fifa96_qfs_hdr_t h;
  if (fifa96_qfs_parse_hdr(b, n, &h) == FIFA96_OK) {
    printf("  qfs:      magic=0x%02x%02x dec_len=0x%08x tag='%.4s'\n",
           h.magic[0], h.magic[1], h.dec_len, h.tag);
  }
}

static void try_pog(const uint8_t *b, size_t n) {
  fifa96_pog_hdr_t h;
  if (fifa96_pog_parse_hdr(b, n, &h) == FIFA96_OK) {
    printf("  pog:      magic=0x%02x%02x w1=0x%04x w2=0x%08x\n",
           h.magic[0], h.magic[1], h.w1, h.w2);
  }
}

static void try_tgv(const uint8_t *b, size_t n) {
  fifa96_tgv_hdr_t h;
  if (fifa96_tgv_parse_hdr(b, n, &h) == FIFA96_OK) {
    printf("  tgv:      magic='%.4s' v0=0x%08x\n", h.magic, h.v0);
  }
}

static void try_viv(const uint8_t *b, size_t n) {
  uint32_t off = 0;
  if (n >= 8 && fifa96_viv_entry_at(b, n, 0, &off) == FIFA96_OK && off == 0) {
    uint32_t off1 = 0;
    if (fifa96_viv_entry_at(b, n, 1, &off1) == FIFA96_OK && off1 > 0) {
      printf("  viv:      entry[0]=0x%08x entry[1]=0x%08x\n", off, off1);
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s FILE\n", argv[0]);
    return 2;
  }
  uint8_t *b = 0;
  size_t n = 0;
  fifa96_err_t e = fifa96_file_read(argv[1], &b, &n);
  if (e != FIFA96_OK) {
    fprintf(stderr, "read failed (%d)\n", e);
    return 1;
  }
  printf("%s: %zu bytes\n", argv[1], n);
  if (n >= 2 && b[0] == 0x10 && b[1] == 0xfb) {
    try_envelope(b, n);
    try_qfs(b, n);
    try_pog(b, n);
  } else if (n >= 4 && memcmp(b, "kVGT", 4) == 0) {
    try_tgv(b, n);
  } else {
    dump_hex(b, n);
    try_viv(b, n);
  }
  fifa96_file_free(b);
  return 0;
}
