#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_TRACE_TYPE_HEADER    0x01
#define FIFA96_TRACE_TYPE_FILE      0x02
#define FIFA96_TRACE_TYPE_CODEC     0x03
#define FIFA96_TRACE_TYPE_HEARTBEAT 0x04
#define FIFA96_TRACE_TYPE_PATCH_SKIP 0x05
#define FIFA96_TRACE_TYPE_END       0x06
#define FIFA96_TRACE_TYPE_PATCH_OK  0x07

#define FIFA96_TRACE_VERSION 1
#define FIFA96_TRACE_NSITES  5

int fifa96_trace_format(const uint8_t *data, size_t len, char **out);
int fifa96_trace_raw(const uint8_t *data, size_t len, char **out);
