#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_camera {
  int32_t pos_x, pos_y, pos_z;
  int32_t target_x, target_y, target_z;
  int32_t anchor_x, anchor_y, anchor_z;
  int32_t anchor2_x, anchor2_y, anchor2_z;
  int16_t vel_x, vel_z;
  uint16_t step;
  uint16_t acc_x, acc_z;
  uint16_t speed;
  uint16_t timer;
  uint16_t timer_limit;
  uint16_t event_param;
  uint16_t event_cursor;
  uint16_t anchor_time, anchor2_time;
  int8_t rate_x, rate_z;
  uint8_t paused;
} fifa96_camera;

int fifa96_camera_init(fifa96_camera *cam, int32_t x, int32_t y, int32_t z);
int fifa96_camera_update(fifa96_camera *cam, int16_t delta, int view_class, int input_bit2);
int fifa96_camera_target(const fifa96_camera *cam, int16_t *x, int16_t *z);
int fifa96_camera_reflect(fifa96_camera *cam, int input_bit0, int transition_active);
int fifa96_camera_out_of_bounds(int32_t x, int32_t z);
