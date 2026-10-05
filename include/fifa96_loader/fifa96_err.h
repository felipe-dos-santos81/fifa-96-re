#pragma once
typedef enum {
  FIFA96_OK = 0,
  FIFA96_ERR_NOT_FOUND = 1,
  FIFA96_ERR_SHORT_READ = 2,
  FIFA96_ERR_BAD_MAGIC = 3,
  FIFA96_ERR_TRUNCATED = 4,
  FIFA96_ERR_CRC_MISMATCH = 5,  /* reserved: crcvals.dat slice deferred (FU-2) — no producer yet */
  FIFA96_ERR_IO = 6,
  FIFA96_ERR_UNSUPPORTED = 7, /* selector known but its decoder is not ported yet */
  FIFA96_ERR_NO_VOICE = 8,    /* FU-47: FUN_000a62fa matched no voice (original -0x14) */
  FIFA96_ERR_VOICE_HANDLE = 11, /* FU-50: handle outside 0..0xF (original -0xB) */
  FIFA96_ERR_VOICE_PAN = 12,    /* FU-50: pan outside 0..0x7F (original -0xC) */
  FIFA96_ERR_VOICE_STATE = 13,  /* FU-50: record state 0 (original -0xD) */
  FIFA96_ERR_VOICE_VOLUME = 14, /* FU-50: volume outside 0..0xFF (original -0xE) */
  FIFA96_ERR_VOICE_SLIDE = 1    /* FU-50: slide state != 1 (original -1; same value as NOT_FOUND) */
} fifa96_err_t;
