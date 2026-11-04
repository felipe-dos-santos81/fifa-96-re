#include "fifa96_loader/fifa96_render.h"
#include <string.h>

fifa96_err_t fifa96_render_camera_stage(const fifa96_render_pos *src, fifa96_render_pos *cam) {
  if (!src || !cam) return -FIFA96_ERR_INVALID;
  cam->x = src->x;
  cam->y = src->y;
  cam->z = src->z;
  if (cam->x > FIFA96_RENDER_CAM_BOUND || cam->x < -FIFA96_RENDER_CAM_BOUND ||
      cam->z > FIFA96_RENDER_CAM_BOUND || cam->z < -FIFA96_RENDER_CAM_BOUND) {
    cam->x = FIFA96_RENDER_CAM_RESET;
    cam->y = 0;
    cam->z = FIFA96_RENDER_CAM_RESET;
  }
  if (cam->y < 0 || cam->y > FIFA96_RENDER_CAM_Y_MAX) cam->y = 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_render_slot_stage(const fifa96_render_entity *e, fifa96_render_slot *slot) {
  if (!e || !slot) return -FIFA96_ERR_INVALID;
  slot->pos = e->pos;
  slot->angle = (int32_t)((((0x400 - (int32_t)(e->heading >> 16)) & 0x3FF) << 6));
  slot->anim_id = e->anim_id;
  slot->frame = e->frame;
  slot->hidden = e->hidden;
  if (e->hidden) slot->pos.y = FIFA96_RENDER_HIDDEN_Y;
  return FIFA96_OK;
}

const uint8_t *fifa96_render_bank_at(const fifa96_render_bank *bank, int32_t index) {
  if (!bank || !bank->base || !bank->offsets || index < 0 || (uint32_t)index >= bank->count)
    return NULL;
  return bank->base + bank->offsets[index * 2];
}

static const uint8_t *bank_frame(const fifa96_render_bank *bank, int32_t offset) {
  return fifa96_render_bank_at(bank, offset);
}

static fifa96_err_t resolve_bumped(fifa96_render_frame *out, const fifa96_render_bank *banks,
                                   uint32_t bank_count, uint32_t bank_index, int32_t dir,
                                   int32_t sprite, int32_t bump) {
  uint32_t used = bank_index;
  if (dir >= bump) {
    used++;
    dir -= bump;
  }
  if (used >= bank_count) return -FIFA96_ERR_INVALID;
  out->offset = banks[used].step * dir + sprite;
  out->sprite = bank_frame(&banks[used], out->offset);
  return FIFA96_OK;
}

static fifa96_err_t resolve_flat(fifa96_render_frame *out, const fifa96_render_bank *banks,
                                 uint32_t bank_count, uint32_t bank_index, int32_t dir,
                                 int32_t sprite) {
  if (dir >= 5) {
    dir = 8 - dir;
    out->mirrored = 1;
  }
  return resolve_bumped(out, banks, bank_count, bank_index, dir, sprite, 3);
}

static fifa96_err_t resolve_pair(fifa96_render_frame *out, const fifa96_render_bank *banks,
                                 uint32_t bank_count, uint32_t bank_index, uint8_t id,
                                 int32_t dir, int32_t sprite, uint8_t flip_lo,
                                 uint8_t flip_hi, uint8_t pair_lo, uint8_t pair_hi,
                                 const uint8_t *mirror) {
  uint32_t used;
  int32_t d2;
  int32_t raw;
  uint8_t m;
  if (id == flip_lo || id == flip_hi) {
    dir = (8 - dir) & 7;
    out->mirrored = 1;
  }
  used = bank_index + (dir >= 4 ? 1u : 0u);
  d2 = dir >= 4 ? dir - 4 : dir;
  if (used >= bank_count) return -FIFA96_ERR_INVALID;
  out->offset = banks[used].step * d2 + sprite;
  out->sprite = bank_frame(&banks[used], out->offset);
  if (id == pair_lo || id == pair_hi) {
    uint32_t second = bank_index + 2u;
    if (second >= bank_count) return -FIFA96_ERR_INVALID;
    raw = banks[second].step * dir + sprite;
    m = (mirror && raw >= 0) ? mirror[raw] : 0;
    if (m != 0) {
      out->overlay_offset = (int32_t)m - 1;
      out->overlay = bank_frame(&banks[second], out->overlay_offset);
    } else {
      out->overlay_offset = raw;
      out->overlay = NULL;
    }
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_render_resolve(uint8_t anim_id, int32_t frame_index, int32_t direction,
                                   const uint8_t *frames, uint8_t bank_index,
                                   const fifa96_render_bank *banks, uint32_t bank_count,
                                   const fifa96_render_bank *fixed60,
                                   const fifa96_render_bank *fixed61, const uint8_t *mirror,
                                   fifa96_render_frame *out) {
  int8_t sid;
  int8_t fi;
  uint8_t id;
  int32_t dir;
  int32_t sprite;
  uint32_t used;
  if (!out) return -FIFA96_ERR_INVALID;
  out->sprite = NULL;
  out->overlay = NULL;
  out->offset = 0;
  out->overlay_offset = 0;
  out->mirrored = 0;
  sid = (int8_t)anim_id;
  if (sid >= 0x6F) return FIFA96_OK;
  if (!frames || !banks || bank_count == 0) return -FIFA96_ERR_INVALID;
  if (bank_index >= bank_count) return -FIFA96_ERR_INVALID;
  fi = (int8_t)(uint8_t)frame_index;
  if (fi < 0) return -FIFA96_ERR_INVALID;
  sprite = frames[(int32_t)fi * 5 + 4];
  id = anim_id;
  dir = direction & 7;
  if (id == 0x2F || id == 0x4A || id == 0x4F || id == 0x51 || id == 0x54 || id == 0x56 ||
      id == 0x5A || id == 0x67 || id == 0x6B)
    return resolve_flat(out, banks, bank_count, bank_index, dir, sprite);
  if (id >= 0x34 && id <= 0x37)
    return resolve_pair(out, banks, bank_count, bank_index, id, dir, sprite, 0x36, 0x37, 0x35,
                        0x37, mirror);
  if (id >= 0x38 && id <= 0x3B)
    return resolve_pair(out, banks, bank_count, bank_index, id, dir, sprite, 0x3A, 0x3B, 0x39,
                        0x3B, mirror);
  if (id == 0x3D || id == 0x3E) {
    if (id == 0x3E) {
      dir = (8 - dir) & 7;
      out->mirrored = 1;
    }
    return resolve_bumped(out, banks, bank_count, bank_index, dir, sprite, 4);
  }
  if (id >= 0x3F && id <= 0x42) {
    uint32_t second;
    if (id == 0x41 || id == 0x42) {
      dir = (8 - dir) & 7;
      out->mirrored = 1;
    }
    out->offset = banks[bank_index].step * dir + sprite;
    out->sprite = bank_frame(&banks[bank_index], out->offset);
    if (id == 0x40 || id == 0x42) {
      second = bank_index + 1u;
      if (second >= bank_count) return -FIFA96_ERR_INVALID;
      out->overlay_offset = banks[second].step * dir + sprite;
      out->overlay = bank_frame(&banks[second], out->overlay_offset);
    }
    return FIFA96_OK;
  }
  if (id == 0x1B) return resolve_bumped(out, banks, bank_count, bank_index, dir, sprite, 4);
  if (id == 0x1A) {
    out->mirrored = 1;
    if (dir >= 5) {
      dir = 8 - dir;
      out->mirrored = 0;
    }
  } else if (dir >= 5) {
    dir = 8 - dir;
    out->mirrored = 1;
  }
  used = bank_index;
  out->offset = banks[used].step * dir + sprite;
  out->sprite = bank_frame(&banks[used], out->offset);
  if (id == 0x61 && fixed61) {
    out->overlay_offset = fixed61->step * dir + sprite;
    out->overlay = bank_frame(fixed61, out->overlay_offset);
  } else if (id == 0x60 && fixed60) {
    out->overlay_offset = fixed60->step * dir + sprite;
    out->overlay = bank_frame(fixed60, out->overlay_offset);
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_render_place(int32_t sprite_w, int32_t sprite_h, int32_t pivot_x,
                                 int32_t pivot_y, int32_t dest_w, int32_t dest_h, int32_t x,
                                 int32_t y, int32_t *out_x, int32_t *out_y) {
  int64_t sx;
  int64_t sy;
  if (sprite_w <= 0 || sprite_h <= 0 || !out_x || !out_y) return -FIFA96_ERR_INVALID;
  sx = ((int64_t)dest_w * 65536) / sprite_w;
  sy = ((int64_t)dest_h * 65536) / sprite_h;
  if (sx >= 0)
    *out_x = x - (int32_t)((sx * pivot_x) >> 16);
  else
    *out_x = x + (int32_t)((sx * (sprite_w - pivot_x)) >> 16);
  if (sy >= 0)
    *out_y = y - (int32_t)((sy * pivot_y) >> 16);
  else
    *out_y = y + (int32_t)((sy * (sprite_h - pivot_y)) >> 16);
  return FIFA96_OK;
}

fifa96_err_t fifa96_render_cover_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                                      int32_t sprite_w, int32_t sprite_h,
                                      const fifa96_render_clip *clip, fifa96_render_cover *out,
                                      uint8_t *visible) {
  int32_t aw;
  int32_t ah;
  int32_t sdx;
  int32_t sdy;
  int32_t over;
  if (sprite_w <= 0 || sprite_h <= 0 || !clip || !out || !visible) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  *visible = 0;
  if (w == 0 || h == 0) return FIFA96_OK;
  aw = w < 0 ? -w : w;
  ah = h < 0 ? -h : h;
  if (x >= clip->right || y >= clip->bottom) return FIFA96_OK;
  if (x + aw <= clip->left || y + ah <= clip->top) return FIFA96_OK;
  sdx = (int32_t)(((int64_t)sprite_w * 65536) / w);
  sdy = (int32_t)(((int64_t)sprite_h * 65536) / h);
  out->mirrored_x = w < 0;
  out->mirrored_y = h < 0;
  out->src_x = sdx / 2 + (out->mirrored_x ? (int32_t)((uint32_t)sprite_w << 16) : 0);
  out->src_y = sdy / 2 + (out->mirrored_y ? (int32_t)((uint32_t)sprite_h << 16) : 0);
  out->dst_x = x;
  out->dst_y = y;
  out->dst_w = aw;
  out->dst_h = ah;
  if (y < clip->top) {
    over = clip->top - y;
    out->src_y += (int32_t)((uint32_t)over * (uint32_t)sdy);
    out->dst_h -= over;
    out->dst_y = clip->top;
  }
  if (out->dst_y + out->dst_h > clip->bottom) out->dst_h = clip->bottom - out->dst_y;
  if (x < clip->left) {
    over = clip->left - x;
    out->src_x += (int32_t)((uint32_t)over * (uint32_t)sdx);
    out->dst_w -= over;
    out->dst_x = clip->left;
  }
  if (out->dst_x + out->dst_w > clip->right) out->dst_w = clip->right - out->dst_x;
  out->src_dx = sdx;
  out->src_dy = sdy;
  if (out->dst_w <= 0 || out->dst_h <= 0) return FIFA96_OK;
  *visible = 1;
  return FIFA96_OK;
}
