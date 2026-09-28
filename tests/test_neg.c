// tests/test_neg.c — negative-path coverage for all decoder entry points (no I/O)
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_pog.h"
#include "fifa96_loader/fifa96_qfs.h"
#include "fifa96_loader/fifa96_tgv.h"
#include "fifa96_loader/fifa96_viv.h"
#include "fifa96_loader/fifa96_tables.h"
int main(void) {
  uint8_t z[16] = {0};
  size_t cnt = 0; uint32_t v = 0; char name[16];
  fifa96_pog_hdr_t ph; fifa96_qfs_hdr_t qh; fifa96_tgv_hdr_t th;

  /* ---- fifa96_pog_parse_hdr ---- */
  assert(fifa96_pog_parse_hdr(NULL, 8, &ph) == FIFA96_ERR_TRUNCATED);   /* (a) NULL buffer */
  assert(fifa96_pog_parse_hdr(z, 8, NULL) == FIFA96_ERR_TRUNCATED);     /* (a) NULL out */
  assert(fifa96_pog_parse_hdr(z, 3, &ph) == FIFA96_ERR_TRUNCATED);      /* (b) too short (n=3) */
  assert(fifa96_pog_parse_hdr(z, 12, &ph) == FIFA96_ERR_BAD_MAGIC);     /* (c) bad first bytes */

  /* ---- fifa96_qfs_parse_hdr ---- */
  assert(fifa96_qfs_parse_hdr(NULL, 12, &qh) == FIFA96_ERR_TRUNCATED);  /* (a) NULL buffer */
  assert(fifa96_qfs_parse_hdr(z, 12, NULL) == FIFA96_ERR_TRUNCATED);    /* (a) NULL out */
  assert(fifa96_qfs_parse_hdr(z, 3, &qh) == FIFA96_ERR_TRUNCATED);      /* (b) too short (n=3) */
  assert(fifa96_qfs_parse_hdr(z, 16, &qh) == FIFA96_ERR_BAD_MAGIC);     /* (c) bad first bytes */

  /* ---- fifa96_tgv_parse_hdr ---- */
  assert(fifa96_tgv_parse_hdr(NULL, 8, &th) == FIFA96_ERR_TRUNCATED);   /* (a) NULL buffer */
  assert(fifa96_tgv_parse_hdr(z, 8, NULL) == FIFA96_ERR_TRUNCATED);     /* (a) NULL out */
  assert(fifa96_tgv_parse_hdr(z, 3, &th) == FIFA96_ERR_TRUNCATED);      /* (b) too short (n=3) */
  assert(fifa96_tgv_parse_hdr(z, 16, &th) == FIFA96_ERR_BAD_MAGIC);     /* (c) bad first bytes */

  /* ---- fifa96_viv_entry_at ---- */
  assert(fifa96_viv_entry_at(NULL, 4, 0, &v) == FIFA96_ERR_TRUNCATED);  /* (a) NULL buffer */
  assert(fifa96_viv_entry_at(z, 4, 0, NULL) == FIFA96_ERR_TRUNCATED);   /* (a) NULL out */
  assert(fifa96_viv_entry_at(z, 3, 0, &v) == FIFA96_ERR_TRUNCATED);     /* (b) too short (n=3) */
  assert(fifa96_viv_entry_at(z, 8, 2, &v) == FIFA96_ERR_TRUNCATED);     /* (d) index out of range */

  /* ---- fifa96_tables_entry_count ---- */
  assert(fifa96_tables_entry_count(NULL, 12, &cnt) == FIFA96_ERR_TRUNCATED); /* (a) NULL buffer */
  assert(fifa96_tables_entry_count(z, 12, NULL) == FIFA96_ERR_TRUNCATED);    /* (a) NULL out */
  assert(fifa96_tables_entry_count(z, 4, &cnt) == FIFA96_ERR_TRUNCATED);     /* (b) non-divisible (n=4) */

  /* ---- fifa96_tables_name_at ---- */
  assert(fifa96_tables_name_at(NULL, 12, 0, name) == FIFA96_ERR_TRUNCATED);  /* (a) NULL buffer */
  assert(fifa96_tables_name_at(z, 4, 0, name) == FIFA96_ERR_TRUNCATED);      /* (b) non-divisible (n=4) */
  assert(fifa96_tables_name_at(z, 12, 1, name) == FIFA96_ERR_TRUNCATED);     /* (d) index out of range */

  /* ---- fifa96_tables_length_at ---- */
  assert(fifa96_tables_length_at(NULL, 4, 0, &v) == FIFA96_ERR_TRUNCATED);   /* (a) NULL buffer */
  assert(fifa96_tables_length_at(z, 4, 0, NULL) == FIFA96_ERR_TRUNCATED);    /* (a) NULL out */
  assert(fifa96_tables_length_at(z, 3, 0, &v) == FIFA96_ERR_TRUNCATED);      /* (b) too short (n=3) */
  assert(fifa96_tables_length_at(z, 4, 1, &v) == FIFA96_ERR_TRUNCATED);      /* (d) index out of range */

  printf("test_neg OK\n");
  return 0;
}
