// tools/fifa96_trace.c — CLI: parse a capture-rig trace.bin to text.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_trace.h"

int main(int argc, char **argv) {
  int raw = 0, argi = 1;
  if (argc > 1 && strcmp(argv[1], "--raw") == 0) { raw = 1; argi = 2; }
  if (argi >= argc) { fprintf(stderr, "usage: %s [--raw] FILE\n", argv[0]); return 2; }
  uint8_t *data = 0; size_t len = 0;
  fifa96_err_t re = fifa96_file_read(argv[argi], &data, &len);
  if (re != FIFA96_OK) {
    fprintf(stderr, "read failed (%d)\n", re); return 1;
  }
  char *text = 0;
  int rc = raw ? fifa96_trace_raw(data, len, &text)
               : fifa96_trace_format(data, len, &text);
  if (rc != FIFA96_OK) {
    if (text) fputs(text, stdout);
    free(text);
    fprintf(stderr, "parse failed (%d)\n", rc);
    fifa96_file_free(data);
    return 1;
  }
  fputs(text, stdout);
  free(text); fifa96_file_free(data);
  return 0;
}
