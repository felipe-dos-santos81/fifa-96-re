#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_entity_update.h"

static int32_t camera_abs(int32_t value) {
  uint32_t magnitude = (uint32_t)value;
  if (value < 0) magnitude = 0u - magnitude;
  return (int32_t)magnitude;
}

int fifa96_camera_init(fifa96_camera *cam, int32_t x, int32_t y, int32_t z) {
  if (!cam) return -FIFA96_ERR_INVALID;
  cam->pos_x = x;
  cam->pos_y = y;
  cam->pos_z = z;
  cam->target_x = x;
  cam->target_y = y;
  cam->target_z = z;
  cam->anchor_x = x;
  cam->anchor_y = y;
  cam->anchor_z = z;
  cam->anchor2_x = x;
  cam->anchor2_y = y;
  cam->anchor2_z = z;
  cam->vel_x = 0;
  cam->vel_z = 0;
  cam->step = 0;
  cam->acc_x = 0;
  cam->acc_z = 0;
  cam->speed = 0;
  cam->timer = 0;
  cam->timer_limit = 0;
  cam->event_param = 0;
  cam->event_cursor = 0;
  cam->anchor_time = 0;
  cam->anchor2_time = 0;
  cam->rate_x = 0;
  cam->rate_z = 0;
  cam->paused = 0;
  return FIFA96_OK;
}

int fifa96_camera_update(fifa96_camera *cam, int16_t delta, int view_class, int input_bit2) {
  if (!cam) return -FIFA96_ERR_INVALID;
  if (cam->paused) return FIFA96_OK;
  if ((int16_t)cam->speed != 0 || (int16_t)cam->event_param != 0) {
    cam->timer = (uint16_t)(cam->timer + (uint16_t)delta);
    if ((int16_t)cam->speed != 0) {
      if (delta > 0) {
        cam->acc_x = (uint16_t)((uint16_t)cam->vel_x * (uint16_t)delta);
        cam->acc_z = (uint16_t)((uint16_t)cam->vel_z * (uint16_t)delta);
      } else {
        cam->acc_x = 0;
        cam->acc_z = 0;
      }
      cam->step = (uint16_t)fifa96_entity_distance((int16_t)cam->acc_x, (int16_t)cam->acc_z);
      cam->pos_x += (int16_t)cam->acc_x;
      cam->pos_z += (int16_t)cam->acc_z;
      if ((int16_t)cam->timer > (int16_t)cam->anchor_time) {
        cam->anchor_x = cam->pos_x;
        cam->anchor_y = cam->pos_y;
        cam->anchor_z = cam->pos_z;
        cam->anchor_time = cam->timer;
      }
      if ((int16_t)cam->timer > (int16_t)cam->anchor2_time) {
        cam->anchor2_x = cam->pos_x;
        cam->anchor2_y = cam->pos_y;
        cam->anchor2_z = cam->pos_z;
        cam->anchor2_time = cam->timer;
      }
      if ((int16_t)cam->event_param > 0x10 && (cam->rate_x != 0 || cam->rate_z != 0)) {
        int interpolate = 0;
        if (input_bit2) {
          interpolate = 1;
        } else if ((int16_t)cam->event_cursor >= (view_class ? 0x90 : 0x70)) {
          interpolate = 1;
        } else {
          cam->event_cursor = (uint16_t)(cam->event_cursor + cam->step);
        }
        if (interpolate) {
          int16_t remaining = (int16_t)(uint16_t)(cam->timer_limit - cam->timer);
          int16_t dx = (int16_t)(cam->rate_x * remaining);
          int16_t dz = (int16_t)(cam->rate_z * remaining);
          cam->vel_x = (int16_t)((uint16_t)cam->vel_x + (uint16_t)cam->rate_x);
          cam->vel_z = (int16_t)((uint16_t)cam->vel_z + (uint16_t)cam->rate_z);
          cam->target_x += dx;
          cam->anchor_x += dx;
          cam->target_z += dz;
          cam->anchor_z += dz;
          cam->event_cursor = 0;
        }
      }
    }
  }
  cam->speed = (uint16_t)fifa96_entity_distance(cam->vel_x, cam->vel_z);
  return FIFA96_OK;
}

int fifa96_camera_target(const fifa96_camera *cam, int16_t *x, int16_t *z) {
  if (!cam || !x || !z) return -FIFA96_ERR_INVALID;
  if ((int16_t)cam->timer < (int16_t)cam->anchor_time) {
    *x = (int16_t)cam->anchor_x;
    *z = (int16_t)cam->anchor_z;
    return 0;
  }
  if ((int16_t)cam->timer < (int16_t)cam->anchor2_time) {
    *x = (int16_t)cam->anchor2_x;
    *z = (int16_t)cam->anchor2_z;
    return 1;
  }
  *x = (int16_t)(cam->target_x + (int32_t)(int16_t)cam->vel_x * 32);
  *z = (int16_t)(cam->target_z + (int32_t)(int16_t)cam->vel_z * 32);
  return 2;
}

int fifa96_camera_reflect(fifa96_camera *cam, int input_bit0, int transition_active) {
  int32_t abs_x;
  int32_t abs_z;
  if (!cam) return -FIFA96_ERR_INVALID;
  abs_x = camera_abs(cam->pos_x);
  abs_z = camera_abs(cam->pos_z);
  if (!(abs_z > 0xB10 || abs_x > 0x720)) return 0;
  if (transition_active || !input_bit0) return 0;
  if (abs_x > 0x720) {
    cam->vel_x = (int16_t)(0 - (uint16_t)cam->vel_x);
    cam->pos_x = (cam->pos_x > 0 ? 0xE40 : -0xE40) - cam->pos_x;
  }
  if (abs_z > 0xB10) {
    cam->vel_z = (int16_t)(0 - (uint16_t)cam->vel_z);
    cam->pos_z = (cam->pos_z > 0 ? 0x1620 : -0x1620) - cam->pos_z;
  }
  return 1;
}

int fifa96_camera_out_of_bounds(int32_t x, int32_t z) {
  return (camera_abs(x) > 0x6C0) || (camera_abs(z) > 0xAB0);
}
