#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum {
  FIFA96_DETECT_UNKNOWN = 0,
  FIFA96_DETECT_ISO9660,
  FIFA96_DETECT_ENVELOPE,
  FIFA96_DETECT_QFS,
  FIFA96_DETECT_POG,
  FIFA96_DETECT_TGV,
  FIFA96_DETECT_VIV,
  FIFA96_DETECT_BIGF,
  FIFA96_DETECT_BNK,
  FIFA96_DETECT_CRD,
  FIFA96_DETECT_EACS,
  FIFA96_DETECT_SHPI
} fifa96_detect_kind_t;

fifa96_detect_kind_t fifa96_detect_kind(const uint8_t *b, size_t n);
const char *fifa96_detect_kind_name(fifa96_detect_kind_t kind);
void fifa96_detect_summary(const uint8_t *b, size_t n, const char *indent);
