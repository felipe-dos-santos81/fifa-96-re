#include "fifa96_loader/fifa96_vgt_player.h"

#include <string.h>

#include "fifa96_loader/fifa96_fvgt.h"
#include "fifa96_loader/fifa96_kvgt.h"

#define VGT_TAG_FVGT 0x54475666u
#define VGT_TAG_KVGT 0x5447566Bu
#define VGT_TAG_REWIND 0xFFFFFFFFu
#define VGT_TAG_SKIP 0xFFFFFFFDu

static uint32_t vgt_le16(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t vgt_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void vgt_frame_clear(fifa96_vgt_player *player, fifa96_vgt_frame *frame) {
  frame->pixels = 0;
  frame->pixels_len = 0;
  frame->width = player->width;
  frame->height = player->height;
  frame->palette = player->palette;
  frame->palette_changed = 0;
}

int fifa96_vgt_player_init(fifa96_vgt_player *player, uint32_t width, uint32_t height,
                           uint8_t *canvas, size_t canvas_cap,
                           uint8_t *scratch, size_t scratch_cap,
                           fifa96_vgt_audio_fn on_audio, void *audio_user) {
  if (!player || !canvas || !scratch) return -(int)FIFA96_ERR_TRUNCATED;
  if (width == 0 || height == 0) return -(int)FIFA96_ERR_TRUNCATED;
  if (canvas == scratch) return -(int)FIFA96_ERR_TRUNCATED;
  uint64_t need = (uint64_t)width * height;
  if (need > canvas_cap || need > scratch_cap) return -(int)FIFA96_ERR_TRUNCATED;
  memset(player, 0, sizeof *player);
  player->front = canvas;
  player->back = scratch;
  player->front_cap = canvas_cap;
  player->back_cap = scratch_cap;
  player->width = width;
  player->height = height;
  player->on_audio = on_audio;
  player->audio_user = audio_user;
  return FIFA96_OK;
}

void fifa96_vgt_player_feed(fifa96_vgt_player *player, const uint8_t *data, size_t size) {
  if (!player) return;
  player->walk.data = data;
  player->walk.size = size;
  player->walk.pos = 0;
  player->ended = 0;
}

int fifa96_vgt_player_step(fifa96_vgt_player *player, fifa96_vgt_frame *frame) {
  if (!player || !frame) return -(int)FIFA96_ERR_TRUNCATED;
  vgt_frame_clear(player, frame);
  if (player->ended) return 0;
  for (;;) {
    const uint8_t *chunk = 0;
    size_t chunk_len = 0;
    int is_frame = 0;
    int r = fifa96_tgv_walk_next(&player->walk, &chunk, &chunk_len, &is_frame);
    if (r < 0) {
      player->ended = 1;
      return r;
    }
    if (r == 0) {
      player->ended = 1;
      return 0;
    }
    uint32_t tag = vgt_le32(chunk);
    if (tag == VGT_TAG_REWIND) {
      if (player->walk.pos - chunk_len != 0) player->walk.pos = 0;
      continue;
    }
    if (tag == VGT_TAG_SKIP) {
      player->ended = 1;
      return 0;
    }
    if (tag == VGT_TAG_KVGT) {
      if (chunk_len < 0x14) {
        player->ended = 1;
        return -(int)FIFA96_ERR_TRUNCATED;
      }
      uint64_t need = (uint64_t)vgt_le16(chunk + 8) * vgt_le16(chunk + 10);
      if (need > player->front_cap || need > player->back_cap) {
        player->ended = 1;
        return -(int)FIFA96_ERR_TRUNCATED;
      }
      struct fifa96_kvgt_info info;
      size_t out_len = 0;
      r = fifa96_kvgt_decode(chunk, chunk_len, player->front, player->front_cap, &out_len, &info);
      if (r < 0) {
        player->ended = 1;
        return r;
      }
      player->width = info.width;
      player->height = info.height;
      memcpy(player->palette, info.palette, FIFA96_VGT_PALETTE_SIZE);
      frame->pixels = player->front;
      frame->pixels_len = out_len;
      frame->width = player->width;
      frame->height = player->height;
      frame->palette = player->palette;
      frame->palette_changed = 1;
      return 1;
    }
    if (tag == VGT_TAG_FVGT) {
      uint64_t need = (uint64_t)player->width * player->height;
      if (need > player->front_cap || need > player->back_cap) {
        player->ended = 1;
        return -(int)FIFA96_ERR_TRUNCATED;
      }
      size_t out_len = 0;
      r = fifa96_fvgt_decode(chunk, chunk_len, player->width, player->height,
                             player->front, player->back, player->back_cap, &out_len);
      if (r < 0) {
        player->ended = 1;
        return r;
      }
      uint8_t *front = player->front;
      player->front = player->back;
      player->back = front;
      size_t cap = player->front_cap;
      player->front_cap = player->back_cap;
      player->back_cap = cap;
      frame->pixels = player->front;
      frame->pixels_len = out_len;
      frame->width = player->width;
      frame->height = player->height;
      frame->palette = player->palette;
      frame->palette_changed = 0;
      return 1;
    }
    if (player->on_audio) player->on_audio(player->audio_user, chunk, chunk_len);
  }
}

int fifa96_vgt_player_ended(const fifa96_vgt_player *player) {
  if (!player) return 1;
  return player->ended;
}
