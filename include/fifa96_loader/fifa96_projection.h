#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_PROJECTION_RECIP_COUNT 1024
#define FIFA96_PROJECTION_NEAR 5

typedef struct fifa96_projection_vec {
  int32_t x;
  int32_t y;
  int32_t z;
} fifa96_projection_vec;

typedef struct fifa96_projection_point {
  int32_t x;
  int32_t y;
} fifa96_projection_point;

fifa96_err_t fifa96_projection_sincos(int32_t angle, int32_t *sin16, int32_t *cos16);

fifa96_err_t fifa96_projection_matrix(int32_t yaw, int32_t pitch, int32_t m[9]);

fifa96_err_t fifa96_projection_transform(const int32_t m[9], const fifa96_projection_vec *v,
                                         fifa96_projection_vec *out);

fifa96_err_t fifa96_projection_reciprocal(int32_t dim, int32_t *table);

fifa96_err_t fifa96_projection_screen(const int32_t *recip_x, const int32_t *recip_y,
                                      const fifa96_projection_point *center,
                                      const fifa96_projection_vec *v,
                                      fifa96_projection_point *out, uint8_t *visible);

fifa96_err_t fifa96_projection_project(const int32_t m[9], const fifa96_projection_vec *cam,
                                       const int32_t *recip_x, const int32_t *recip_y,
                                       const fifa96_projection_point *center,
                                       const fifa96_projection_vec *world,
                                       fifa96_projection_point *out, uint8_t *visible);
