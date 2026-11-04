#include "fifa96_detect.h"
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_crd.h"
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_envelope.h"
#include "fifa96_loader/fifa96_pog.h"
#include "fifa96_loader/fifa96_qfs.h"
#include "fifa96_loader/fifa96_tgv.h"
#include "fifa96_loader/fifa96_viv.h"

fifa96_detect_kind_t fifa96_detect_kind(const uint8_t *b, size_t n) {
  if (!b) return FIFA96_DETECT_UNKNOWN;
  if (n >= 22u * 2048u && b[16u * 2048u] == 1 &&
      memcmp(b + 16u * 2048u + 1, "CD001", 5) == 0)
    return FIFA96_DETECT_ISO9660;
  if (n >= 2 && b[0] == 0x10 && b[1] == 0xfb) return FIFA96_DETECT_ENVELOPE;
  if (n >= 4 && memcmp(b, "kVGT", 4) == 0) return FIFA96_DETECT_TGV;
  if (n >= 4 && memcmp(b, "BIGF", 4) == 0) return FIFA96_DETECT_BIGF;
  if (n >= 4 && memcmp(b, "SHPI", 4) == 0) return FIFA96_DETECT_SHPI;
  if (n >= 4 && memcmp(b, "CRDF", 4) == 0) return FIFA96_DETECT_CRD;
  if (n >= 4 && memcmp(b, "EACS", 4) == 0) return FIFA96_DETECT_EACS;
  if (n >= 0x200u) {
    struct fifa96_bnk_info bank;
    if (fifa96_bnk_parse(b, n, &bank) == FIFA96_OK && bank.entry_count > 0)
      return FIFA96_DETECT_BNK;
  }
  if (n >= 8) {
    uint32_t off0 = 0, off1 = 0;
    if (fifa96_viv_entry_at(b, n, 0, &off0) == FIFA96_OK && off0 == 0 &&
        fifa96_viv_entry_at(b, n, 1, &off1) == FIFA96_OK && off1 > 0)
      return FIFA96_DETECT_VIV;
  }
  return FIFA96_DETECT_UNKNOWN;
}

const char *fifa96_detect_kind_name(fifa96_detect_kind_t kind) {
  switch (kind) {
    case FIFA96_DETECT_ISO9660: return "iso9660";
    case FIFA96_DETECT_ENVELOPE: return "envelope";
    case FIFA96_DETECT_QFS: return "qfs";
    case FIFA96_DETECT_POG: return "pog";
    case FIFA96_DETECT_TGV: return "tgv";
    case FIFA96_DETECT_VIV: return "viv";
    case FIFA96_DETECT_BIGF: return "bigf";
    case FIFA96_DETECT_BNK: return "bnk";
    case FIFA96_DETECT_CRD: return "crd";
    case FIFA96_DETECT_EACS: return "eacs";
    case FIFA96_DETECT_SHPI: return "shpi";
    default: return "unknown";
  }
}

static void try_envelope(const uint8_t *b, size_t n, const char *indent) {
  fifa96_envelope_hdr_t h;
  if (fifa96_envelope_parse_hdr(b, n, &h) == FIFA96_OK)
    printf("%senvelope: magic=0x%04x word_a=0x%04x word_b=0x%04x tag='%.4s' tail_off=%zu "
           "tail_len=%zu\n",
           indent, h.magic, h.word_a, h.word_b, h.tag, h.tail_off, h.tail_len);
}

static void try_qfs(const uint8_t *b, size_t n, const char *indent) {
  fifa96_qfs_hdr_t h;
  if (fifa96_qfs_parse_hdr(b, n, &h) == FIFA96_OK)
    printf("%sqfs: magic=0x%02x%02x dec_len=0x%08x tag='%.4s'\n", indent, h.magic[0], h.magic[1],
           h.dec_len, h.tag);
}

static void try_pog(const uint8_t *b, size_t n, const char *indent) {
  fifa96_pog_hdr_t h;
  if (fifa96_pog_parse_hdr(b, n, &h) == FIFA96_OK)
    printf("%spog: magic=0x%02x%02x w1=0x%04x w2=0x%08x\n", indent, h.magic[0], h.magic[1], h.w1,
           h.w2);
}

static void try_tgv(const uint8_t *b, size_t n, const char *indent) {
  fifa96_tgv_hdr_t h;
  if (fifa96_tgv_parse_hdr(b, n, &h) == FIFA96_OK)
    printf("%stgv: magic='%.4s' v0=0x%08x\n", indent, h.magic, h.v0);
}

static void try_viv(const uint8_t *b, size_t n, const char *indent) {
  uint32_t off0 = 0, off1 = 0;
  if (n >= 8 && fifa96_viv_entry_at(b, n, 0, &off0) == FIFA96_OK && off0 == 0 &&
      fifa96_viv_entry_at(b, n, 1, &off1) == FIFA96_OK && off1 > 0)
    printf("%sviv: entry[0]=0x%08x entry[1]=0x%08x\n", indent, off0, off1);
}

static void try_bigf(const uint8_t *b, size_t n, const char *indent) {
  struct fifa96_bigf_info info;
  if (n >= 4 && memcmp(b, "BIGF", 4) == 0 && fifa96_bigf_parse(b, n, &info) == FIFA96_OK)
    printf("%sbigf: size=%u count=%u table_end=0x%x\n", indent, info.size, info.count,
           info.table_end);
}

static void try_bnk(const uint8_t *b, size_t n, const char *indent) {
  struct fifa96_bnk_info info;
  if (n >= 0x200u && fifa96_bnk_parse(b, n, &info) == FIFA96_OK && info.entry_count > 0)
    printf("%sbnk: entries=%u\n", indent, info.entry_count);
}

static void try_crd(const uint8_t *b, size_t n, const char *indent) {
  struct fifa96_crd crd;
  if (n >= 4 && memcmp(b, "CRDF", 4) == 0 && fifa96_crd_parse(b, n, &crd) == FIFA96_OK)
    printf("%scrd: version=%u tracks=%u events=%u\n", indent, crd.header.version,
           crd.header.track_count, crd.header.event_count);
}

static void try_eacs(const uint8_t *b, size_t n, const char *indent) {
  struct fifa96_eacs_info info;
  if (n >= 4 && memcmp(b, "EACS", 4) == 0 && fifa96_eacs_parse(b, n, &info) == FIFA96_OK)
    printf("%seacs: rate=%u f8=%u f9=%u f10=%u blocks=%u\n", indent, info.rate, info.f8, info.f9,
           info.f10, info.blocks);
}

void fifa96_detect_summary(const uint8_t *b, size_t n, const char *indent) {
  fifa96_detect_kind_t kind = fifa96_detect_kind(b, n);
  if (kind == FIFA96_DETECT_ISO9660) printf("%siso9660: pvd sector 16 tag=CD001\n", indent);
  if (kind == FIFA96_DETECT_ENVELOPE) try_envelope(b, n, indent);
  try_qfs(b, n, indent);
  try_pog(b, n, indent);
  try_tgv(b, n, indent);
  try_bigf(b, n, indent);
  try_bnk(b, n, indent);
  try_crd(b, n, indent);
  try_eacs(b, n, indent);
  if (kind == FIFA96_DETECT_VIV || kind == FIFA96_DETECT_UNKNOWN) try_viv(b, n, indent);
}
