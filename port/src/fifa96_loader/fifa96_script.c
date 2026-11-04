#include "fifa96_loader/fifa96_script.h"
fifa96_err_t fifa96_script_table_append(fifa96_script_table_t *t, uint16_t v) {
  if (!t || !t->words || t->len >= t->cap) return FIFA96_ERR_TRUNCATED;
  t->words[t->len++] = v;
  return FIFA96_OK;
}
fifa96_err_t fifa96_script_table_terminated(const uint16_t *w, size_t n, size_t *term_at) {
  size_t i = 0;
  if (!w || !term_at) return FIFA96_ERR_TRUNCATED;
  for (i = 0; i < n; i++) {
    if (w[i] == FIFA96_SCRIPT_TERM) { *term_at = i; return FIFA96_OK; }
  }
  return FIFA96_ERR_TRUNCATED;
}
size_t fifa96_script_comment_len(const uint8_t *b, size_t n) {
  size_t i = 0;
  if (!b || n == 0 || b[0] != 0x3b) return 0;
  for (i = 1; i < n; i++) {
    if (b[i] == 0x0a) return i + 1;
  }
  return n;
}
