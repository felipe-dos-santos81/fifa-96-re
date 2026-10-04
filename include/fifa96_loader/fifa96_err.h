#pragma once
typedef enum {
  FIFA96_OK = 0,
  FIFA96_ERR_NOT_FOUND = 1,
  FIFA96_ERR_SHORT_READ = 2,
  FIFA96_ERR_BAD_MAGIC = 3,
  FIFA96_ERR_TRUNCATED = 4,
  FIFA96_ERR_CRC_MISMATCH = 5,  /* reserved: crcvals.dat slice deferred (FU-2) — no producer yet */
  FIFA96_ERR_IO = 6,
  FIFA96_ERR_UNSUPPORTED = 7  /* selector known but its decoder is not ported yet */
} fifa96_err_t;
