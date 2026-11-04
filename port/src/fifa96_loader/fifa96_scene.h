#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_projection.h"

#define FIFA96_SCENE_POSITION_DWORDS 3
#define FIFA96_SCENE_SLOT_STRIDE 8
#define FIFA96_SCENE_HIDDEN_Y (-10000)
#define FIFA96_SCENE_LATERAL_MAX 0x8E0
#define FIFA96_SCENE_CLAMP_MIN 0x78

/* FU-89 §11 / OL-T11-8 (M2 visible-match Task 1): the resource-loaded
 * formation records. `FUN_0006D920` sets each record's formation pointer
 * `[rec+8] = FUN_0004AFB8(6*formation_id) + (byte[rec+0x8D] << 2)`, so the
 * file is 11 four-byte records indexed by the record block position; the
 * phase cell `FUN_0006E1D0` (`0x6E1D0`) then writes the target triple
 * `+0x4D/+0x51/+0x55` from the pair at `[rec+8]+2` when the record's team is
 * the controlled side (`[0x157AAC]>>24`) and from `[rec+8]+0` otherwise, as
 * `x = (int8)pair[0] * 0x26`, `y = 0`, `z = (int8)pair[1] * 0x21`, negated
 * when the team side (`team+0x826`) is non-zero. */
#define FIFA96_SCENE_FORMATION_RECORDS 11u
#define FIFA96_SCENE_FORMATION_STRIDE 4u
#define FIFA96_SCENE_FORMATION_BYTES \
  (FIFA96_SCENE_FORMATION_RECORDS * FIFA96_SCENE_FORMATION_STRIDE)
#define FIFA96_SCENE_FORMATION_X_SCALE 0x26
#define FIFA96_SCENE_FORMATION_Z_SCALE 0x21

typedef struct fifa96_scene_formation {
  uint8_t bytes[FIFA96_SCENE_FORMATION_BYTES];
  uint8_t loaded;
} fifa96_scene_formation;

/* Load the named formation record from a resource container: a raw BIGF
 * directory or the record-compressed form the game containers use
 * (`file[1] == 0xFB`, decoded through `fifa96_record_decode`, the same decode
 * the engine's match art staging applies). `name` is the
 * BIGF entry name (e.g. "352ko.fmt", formation id 0's kickoff file). On
 * FIFA96_OK the 11 records are copied into `formation` and `loaded` is set;
 * a missing entry is FIFA96_ERR_NOT_FOUND, a wrong-container form
 * FIFA96_ERR_UNSUPPORTED, a short entry FIFA96_ERR_TRUNCATED, NULL arguments
 * -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_scene_formation_load(const uint8_t *file, size_t file_len,
                                         const char *name,
                                         fifa96_scene_formation *formation);

/* The `FUN_0006E1D0` placement for `formation` record `index` (0..10) of the
 * team with side `side` (0/1) when `controlled_side` (0/1) owns the ball:
 * writes the derived target triple. A not-loaded formation writes nothing and
 * returns FIFA96_ERR_NOT_FOUND; NULL pointers/arguments or an out-of-range
 * index return -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_scene_formation_place(const fifa96_scene_formation *formation,
                                          uint32_t index, uint8_t side,
                                          uint8_t controlled_side, int32_t *x,
                                          int32_t *y, int32_t *z);

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
