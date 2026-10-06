#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_projection.h"

#define FIFA96_SCENE_POSITION_DWORDS 3
#define FIFA96_SCENE_SLOT_STRIDE 8
#define FIFA96_SCENE_HIDDEN_Y (-10000)
#define FIFA96_SCENE_LATERAL_MAX 0x8E0
#define FIFA96_SCENE_CLAMP_MIN 0x78

typedef struct fifa96_scene_slot {
  fifa96_projection_point clean;
  fifa96_projection_point jitter;
  uint8_t clean_visible;
  uint8_t jitter_visible;
} fifa96_scene_slot;

fifa96_err_t fifa96_scene_build_keys(uint32_t count, const uint32_t *list,
                                     const int32_t *positions, uint32_t position_count,
                                     int32_t *keys);

fifa96_err_t fifa96_scene_sort(uint32_t count, int32_t *keys, uint32_t *values);

fifa96_err_t fifa96_scene_slot_gate(int32_t threshold, int32_t key, int32_t staged_y,
                                    int32_t lateral, uint8_t *visible);

fifa96_err_t fifa96_scene_clip_edges(int32_t left, int32_t top, int32_t right, int32_t bottom,
                                     int32_t clean_y, int32_t jitter_x, int32_t jitter_y,
                                     int32_t *jitter_y_out, uint8_t *draw);

fifa96_err_t fifa96_scene_threshold(int32_t field, int32_t reference, int32_t fallback,
                                    int32_t limit, int32_t *out);

fifa96_err_t fifa96_scene_reproject(const int32_t prev[3], const int32_t pos[3],
                                    uint8_t replay_gate, const int32_t cached[6],
                                    const int32_t angles[2], const int32_t other[4],
                                    uint8_t *reproject);

fifa96_err_t fifa96_scene_slot_project(const int32_t *recip_x, const int32_t *recip_y,
                                       const fifa96_projection_point *center,
                                       const fifa96_projection_vec *clean_rot,
                                       const fifa96_projection_vec *jitter_rot,
                                       fifa96_scene_slot *slot);
