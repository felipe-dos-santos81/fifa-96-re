#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_competition.h"
#include "fifa96_loader/fifa96_settings.h"

/* P13 — derived ten-slot save files.
 *
 * The native loader uses SGAME%.2d.DAT with a per-module body (FU-166/FU-65);
 * that body is not ported. This module writes a distinct F96SAVE%.2d.DAT
 * record (magic/version/length/CRC32) so native saves are never overwritten.
 * The caller supplies the directory; the engine uses data/game/hdd when the
 * ISO is mounted and disables file writes for headless/test runs. */

#define FIFA96_SAVE_SLOT_COUNT 10
#define FIFA96_SAVE_SUMMARY_MAX 80
#define FIFA96_SAVE_PATH_MAX 512

struct fifa96_save_info {
  int valid;
  uint8_t kind;
  uint8_t state;
  uint8_t user_team;
  uint8_t team_count;
  uint32_t crc;
  char summary[FIFA96_SAVE_SUMMARY_MAX];
};

/* Build `<dir>/F96SAVE%02d.DAT`. Empty/NULL dir means ".". Returns 0, -1 on
 * invalid slot/path. */
int fifa96_save_path(char *buf, size_t cap, const char *dir, int slot);

/* Atomically write a slot. Returns 0, -1 on invalid arguments or I/O failure. */
int fifa96_save_write(const char *dir, int slot,
                      const struct fifa96_competition *comp,
                      const struct fifa96_settings *settings);

/* Read a slot. On success fills both structs and returns 0. Missing/corrupt/
 * wrong-version files return -1 and leave outputs untouched. */
int fifa96_save_read(const char *dir, int slot,
                     struct fifa96_competition *comp,
                     struct fifa96_settings *settings);

/* Read only the header/summary for the Load Game list. Returns 0 when valid,
 * -1 otherwise; an invalid slot is still reported through info.valid=0. */
int fifa96_save_inspect(const char *dir, int slot, struct fifa96_save_info *info);

/* Human-readable slot label: "Empty" or the competition summary. */
int fifa96_save_slot_label(const char *dir, int slot, char *buf, size_t cap);
