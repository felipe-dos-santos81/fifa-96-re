#include <stdlib.h>
#include <string.h>

#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_record.h"
#include "fifa96_loader/fifa96_scene.h"

fifa96_err_t fifa96_scene_build_keys(uint32_t count, const uint32_t *list,
                                     const int32_t *positions, uint32_t position_count,
                                     int32_t *keys) {
  if (!list || !positions || !keys) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  for (uint32_t i = 0; i < count; i++) {
    uint32_t value = list[i];
    if (value >= position_count) return (fifa96_err_t)-FIFA96_ERR_INVALID;
    keys[i] = positions[(uint64_t)value * FIFA96_SCENE_POSITION_DWORDS + 2];
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_scene_sort(uint32_t count, int32_t *keys, uint32_t *values) {
  if (count > 1 && (!keys || !values)) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  for (uint32_t gap = count >> 1; gap > 0; gap >>= 1) {
    for (uint32_t i = gap; i < count; i++) {
      int32_t j = (int32_t)(i - gap);
      while (j >= 0) {
        if (keys[j] >= keys[j + gap]) break;
        int32_t k = keys[j];
        keys[j] = keys[j + gap];
        keys[j + gap] = k;
        uint32_t v = values[j];
        values[j] = values[j + gap];
        values[j + gap] = v;
        j -= (int32_t)gap;
      }
    }
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_scene_slot_gate(int32_t threshold, int32_t key, int32_t staged_y,
                                    int32_t lateral, uint8_t *visible) {
  if (!visible) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (threshold > key || staged_y == FIFA96_SCENE_HIDDEN_Y ||
      lateral > FIFA96_SCENE_LATERAL_MAX) {
    *visible = 0;
    return FIFA96_OK;
  }
  *visible = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_scene_clip_edges(int32_t left, int32_t top, int32_t right, int32_t bottom,
                                     int32_t clean_y, int32_t jitter_x, int32_t jitter_y,
                                     int32_t *jitter_y_out, uint8_t *draw) {
  if (!jitter_y_out || !draw) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (bottom <= jitter_y || clean_y <= top) {
    *jitter_y_out = bottom;
    *draw = 0;
    return FIFA96_OK;
  }
  int32_t delta = (int32_t)((uint32_t)clean_y - (uint32_t)jitter_y);
  int32_t candidate = (int32_t)((uint32_t)right + ((uint32_t)delta << 1));
  int32_t alternative = (int32_t)((uint32_t)left - ((uint32_t)delta << 1));
  if (candidate > jitter_x && alternative < jitter_x) {
    *jitter_y_out = jitter_y;
    *draw = 1;
    return FIFA96_OK;
  }
  *jitter_y_out = bottom;
  *draw = 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_scene_threshold(int32_t field, int32_t reference, int32_t fallback,
                                    int32_t limit, int32_t *out) {
  if (!out) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  int32_t value;
  if (reference < limit) {
    value = fallback;
  } else {
    int32_t divisor = (int32_t)((uint32_t)field * 2u);
    if (divisor == 0) return (fifa96_err_t)-FIFA96_ERR_INVALID;
    int32_t quotient = 0x14 / divisor;
    int32_t remainder = 0x14 % divisor;
    int64_t fraction = ((int64_t)remainder * 0x10000) / divisor;
    value = (int32_t)((int64_t)quotient * 0x10000 + fraction);
  }
  if (value < FIFA96_SCENE_CLAMP_MIN) value = FIFA96_SCENE_CLAMP_MIN;
  *out = value;
  return FIFA96_OK;
}

fifa96_err_t fifa96_scene_reproject(const int32_t prev[3], const int32_t pos[3],
                                    uint8_t replay_gate, const int32_t cached[6],
                                    const int32_t angles[2], const int32_t other[4],
                                    uint8_t *reproject) {
  if (!prev || !pos || !cached || !angles || !other || !reproject)
    return (fifa96_err_t)-FIFA96_ERR_INVALID;
  int32_t dx = (int32_t)((uint32_t)prev[0] - (uint32_t)pos[0]);
  int32_t dy = (int32_t)((uint32_t)prev[1] - (uint32_t)pos[1]);
  int32_t dz = (int32_t)((uint32_t)prev[2] - (uint32_t)pos[2]);
  if (dx == 0 && dy == 0 && dz == 0 && replay_gate && cached[0] == angles[0] &&
      cached[1] == angles[1] && cached[2] == other[0] && cached[3] == other[1] &&
      cached[4] == other[2] && cached[5] == other[3]) {
    *reproject = 0;
  } else {
    *reproject = 1;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_scene_slot_project(const int32_t *recip_x, const int32_t *recip_y,
                                       const fifa96_projection_point *center,
                                       const fifa96_projection_vec *clean_rot,
                                       const fifa96_projection_vec *jitter_rot,
                                       fifa96_scene_slot *slot) {
  if (!recip_x || !recip_y || !center || !clean_rot || !jitter_rot || !slot)
    return (fifa96_err_t)-FIFA96_ERR_INVALID;
  fifa96_err_t err =
      fifa96_projection_screen(recip_x, recip_y, center, clean_rot, &slot->clean,
                               &slot->clean_visible);
  if (err != FIFA96_OK) return err;
  return fifa96_projection_screen(recip_x, recip_y, center, jitter_rot, &slot->jitter,
                                  &slot->jitter_visible);
}

/* FU-89 §11 / OL-T11-8: the resource-container decode is the same chain the
 * engine's match art staging uses (FU-86): a raw BIGF directory or a
 * `file[1] == 0xFB` record whose decoded bytes are the BIGF. */
fifa96_err_t fifa96_scene_formation_load(const uint8_t *file, size_t file_len,
                                         const char *name,
                                         fifa96_scene_formation *formation) {
  uint8_t *decoded = NULL;
  const uint8_t *container = file;
  size_t container_len = file_len;
  if (!file || !name || !formation) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (file_len >= 5 && file[1] == 0xFB) {
    size_t declared = ((size_t)file[2] << 16) | ((size_t)file[3] << 8) | (size_t)file[4];
    size_t used = 0;
    if (declared == 0) return FIFA96_ERR_TRUNCATED;
    decoded = malloc(declared);
    if (!decoded) return FIFA96_ERR_UNSUPPORTED;
    if (fifa96_record_decode(file, file_len, decoded, declared, &used) != 0) {
      free(decoded);
      return FIFA96_ERR_TRUNCATED;
    }
    container = decoded;
    container_len = used;
  } else if (!(file_len >= 4 && memcmp(file, "BIGF", 4) == 0)) {
    return FIFA96_ERR_UNSUPPORTED;
  }
  struct fifa96_bigf_info info;
  fifa96_err_t err = fifa96_bigf_parse(container, container_len, &info);
  if (err != FIFA96_OK) {
    free(decoded);
    return err;
  }
  for (size_t i = 0; i < info.count; i++) {
    uint32_t off = 0;
    uint32_t size = 0;
    const char *entry = NULL;
    err = fifa96_bigf_record(&info, i, &off, &size, &entry);
    if (err != FIFA96_OK) {
      free(decoded);
      return err;
    }
    if (strcmp(entry, name) != 0) continue;
    if (size < FIFA96_SCENE_FORMATION_BYTES) {
      free(decoded);
      return FIFA96_ERR_TRUNCATED;
    }
    memcpy(formation->bytes, container + off, FIFA96_SCENE_FORMATION_BYTES);
    formation->loaded = 1;
    free(decoded);
    return FIFA96_OK;
  }
  free(decoded);
  return FIFA96_ERR_NOT_FOUND;
}

fifa96_err_t fifa96_scene_formation_place(const fifa96_scene_formation *formation,
                                          uint32_t index, uint8_t side,
                                          uint8_t controlled_side, int32_t *x,
                                          int32_t *y, int32_t *z) {
  const uint8_t *pair;
  int32_t px;
  int32_t pz;
  if (!formation || !x || !y || !z || index >= FIFA96_SCENE_FORMATION_RECORDS)
    return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (!formation->loaded) return FIFA96_ERR_NOT_FOUND;
  /* 0x6E1F1: the controlled side reads the record's +2 pair, the opponent the
   * +0 pair. 0x6E1F4..0x6E211 / 0x6E20C..0x6E21A: x = (int8)byte0 * 0x26,
   * z = (int8)byte1 * 0x21. 0x6E21C..0x6E227: a non-zero team side negates
   * both. */
  pair = formation->bytes + (size_t)index * FIFA96_SCENE_FORMATION_STRIDE +
         ((side == controlled_side) ? 2u : 0u);
  px = (int32_t)(int8_t)pair[0] * FIFA96_SCENE_FORMATION_X_SCALE;
  pz = (int32_t)(int8_t)pair[1] * FIFA96_SCENE_FORMATION_Z_SCALE;
  if (side != 0) {
    px = -px;
    pz = -pz;
  }
  *x = px;
  *y = 0;
  *z = pz;
  return FIFA96_OK;
}
