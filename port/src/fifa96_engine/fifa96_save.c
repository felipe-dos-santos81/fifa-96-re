#include "fifa96_engine/fifa96_save.h"

#include <stdio.h>
#include <string.h>

#define SAVE_MAGIC "F96SAVE1"
#define SAVE_VERSION 1u
#define SAVE_HEADER 20u
#define SAVE_FILE_MAX 4096u

static uint32_t save_crc32(const uint8_t *p, size_t n) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; i++) {
    crc ^= p[i];
    for (int b = 0; b < 8; b++)
      crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
  }
  return ~crc;
}

static void save_put_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static uint32_t save_get_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

int fifa96_save_path(char *buf, size_t cap, const char *dir, int slot) {
  if (!buf || cap == 0 || slot < 0 || slot >= FIFA96_SAVE_SLOT_COUNT) return -1;
  const char *d = (dir && dir[0]) ? dir : ".";
  int n = snprintf(buf, cap, "%s/F96SAVE%02d.DAT", d, slot);
  return (n > 0 && (size_t)n < cap) ? 0 : -1;
}

static int save_build_payload(const struct fifa96_competition *comp,
                              const struct fifa96_settings *settings,
                              uint8_t *payload, size_t cap, size_t *len) {
  uint8_t record[FIFA96_COMPETITION_SERIAL_MAX];
  size_t record_len = 0;
  if (!comp || !settings || !payload || !len) return -1;
  if (fifa96_competition_serialize(comp, record, sizeof record, &record_len) != 0)
    return -1;
  size_t need = 2u + record_len + FIFA96_SETTINGS_COUNT * 8u;
  if (need > cap) return -1;
  size_t p = 0;
  payload[p++] = (uint8_t)record_len;
  payload[p++] = (uint8_t)(record_len >> 8);
  memcpy(payload + p, record, record_len);
  p += record_len;
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++) {
    save_put_u32(payload + p, (uint32_t)settings->value[i]);
    p += 4;
  }
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++) {
    save_put_u32(payload + p, (uint32_t)settings->max[i]);
    p += 4;
  }
  *len = p;
  return 0;
}

int fifa96_save_write(const char *dir, int slot,
                      const struct fifa96_competition *comp,
                      const struct fifa96_settings *settings) {
  char path[FIFA96_SAVE_PATH_MAX];
  char tmp[FIFA96_SAVE_PATH_MAX];
  uint8_t payload[SAVE_FILE_MAX];
  uint8_t file[SAVE_FILE_MAX];
  size_t payload_len = 0;
  if (fifa96_save_path(path, sizeof path, dir, slot) != 0) return -1;
  if (save_build_payload(comp, settings, payload, sizeof payload, &payload_len) != 0)
    return -1;
  if (SAVE_HEADER + payload_len > sizeof file) return -1;
  memcpy(file, SAVE_MAGIC, 8);
  save_put_u32(file + 8, SAVE_VERSION);
  save_put_u32(file + 12, (uint32_t)payload_len);
  save_put_u32(file + 16, save_crc32(payload, payload_len));
  memcpy(file + SAVE_HEADER, payload, payload_len);
  if (snprintf(tmp, sizeof tmp, "%s.tmp", path) <= 0) return -1;
  FILE *f = fopen(tmp, "wb");
  if (!f) return -1;
  size_t wrote = fwrite(file, 1, SAVE_HEADER + payload_len, f);
  int failed = wrote != SAVE_HEADER + payload_len;
  if (fclose(f) != 0) failed = 1;
  if (failed) {
    remove(tmp);
    return -1;
  }
  if (rename(tmp, path) != 0) {
    remove(tmp);
    return -1;
  }
  return 0;
}

static int save_read_file(const char *dir, int slot, uint8_t *file,
                          size_t cap, size_t *len) {
  char path[FIFA96_SAVE_PATH_MAX];
  if (!file || !len || fifa96_save_path(path, sizeof path, dir, slot) != 0)
    return -1;
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return -1;
  }
  long n = ftell(f);
  if (n < 0 || (size_t)n > cap) {
    fclose(f);
    return -1;
  }
  rewind(f);
  size_t got = fread(file, 1, (size_t)n, f);
  int extra = fgetc(f);
  fclose(f);
  if (got != (size_t)n || extra != EOF) return -1;
  *len = got;
  return 0;
}

static int save_parse(const uint8_t *file, size_t len,
                      struct fifa96_competition *comp,
                      struct fifa96_settings *settings) {
  size_t payload_len;
  uint16_t record_len;
  if (!file || !comp || !settings || len < SAVE_HEADER) return -1;
  if (memcmp(file, SAVE_MAGIC, 8) != 0) return -1;
  if (save_get_u32(file + 8) != SAVE_VERSION) return -1;
  payload_len = save_get_u32(file + 12);
  if (payload_len > len - SAVE_HEADER) return -1;
  if (save_crc32(file + SAVE_HEADER, payload_len) != save_get_u32(file + 16))
    return -1;
  const uint8_t *p = file + SAVE_HEADER;
  if (payload_len < 2u) return -1;
  record_len = (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
  if (record_len > payload_len - 2u) return -1;
  size_t need = 2u + record_len + FIFA96_SETTINGS_COUNT * 8u;
  if (need > payload_len) return -1;
  if (fifa96_competition_deserialize(comp, p + 2, record_len) != 0) return -1;
  p += 2 + record_len;
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++) {
    settings->value[i] = (int32_t)save_get_u32(p);
    p += 4;
  }
  for (uint32_t i = 0; i < FIFA96_SETTINGS_COUNT; i++) {
    settings->max[i] = (int32_t)save_get_u32(p);
    p += 4;
  }
  return 0;
}

int fifa96_save_read(const char *dir, int slot,
                     struct fifa96_competition *comp,
                     struct fifa96_settings *settings) {
  uint8_t file[SAVE_FILE_MAX];
  size_t len = 0;
  struct fifa96_competition c;
  struct fifa96_settings s;
  if (!comp || !settings) return -1;
  if (save_read_file(dir, slot, file, sizeof file, &len) != 0) return -1;
  if (save_parse(file, len, &c, &s) != 0) return -1;
  *comp = c;
  *settings = s;
  return 0;
}

int fifa96_save_inspect(const char *dir, int slot, struct fifa96_save_info *info) {
  uint8_t file[SAVE_FILE_MAX];
  size_t len = 0;
  struct fifa96_competition c;
  struct fifa96_settings s;
  if (!info) return -1;
  memset(info, 0, sizeof *info);
  if (save_read_file(dir, slot, file, sizeof file, &len) != 0) return -1;
  if (save_parse(file, len, &c, &s) != 0) return -1;
  info->valid = 1;
  info->kind = c.kind;
  info->state = c.state;
  info->user_team = c.user_team;
  info->team_count = c.team_count;
  info->crc = save_get_u32(file + 16);
  if (fifa96_competition_summary(&c, info->summary, sizeof info->summary) != 0)
    snprintf(info->summary, sizeof info->summary, "%s",
             fifa96_competition_kind_name(c.kind));
  return 0;
}

int fifa96_save_slot_label(const char *dir, int slot, char *buf, size_t cap) {
  struct fifa96_save_info info;
  if (!buf || cap == 0) return -1;
  if (fifa96_save_inspect(dir, slot, &info) != 0 || !info.valid) {
    snprintf(buf, cap, "Empty");
    return 0;
  }
  snprintf(buf, cap, "%s", info.summary);
  return 0;
}
