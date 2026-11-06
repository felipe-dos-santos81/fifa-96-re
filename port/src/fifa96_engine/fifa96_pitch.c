#include "fifa96_engine/fifa96_pitch.h"

#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_record.h"
#include "fifa96_loader/fifa96_sprite.h"

/* FU-86 §2/§3: the match-art containers are refpack envelopes around a BIGF v2
 * directory whose entries are raw SHPI banks or nested record chains. This is
 * the same decode chain fifa96_match_run.c's match_stage_container/
 * match_stage_entry use (kept local so the pitch module stays a leaf). */
#define PITCH_DECODE_STAGES 4

static int pitch_name_eq(const char *a, const char *b) {
  while (*a && *b) {
    unsigned char ca = (unsigned char)*a;
    unsigned char cb = (unsigned char)*b;
    if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + 0x20);
    if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + 0x20);
    if (ca != cb) return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static int pitch_container_decode(const uint8_t *file, size_t file_len, uint8_t **out,
                                  size_t *out_len) {
  *out = NULL;
  *out_len = 0;
  if (file_len >= 4 && memcmp(file, "BIGF", 4) == 0) {
    uint8_t *copy = malloc(file_len);
    if (!copy) return -1;
    memcpy(copy, file, file_len);
    *out = copy;
    *out_len = file_len;
    return 0;
  }
  if (file_len >= 5 && file[1] == 0xFB) {
    size_t declared = ((size_t)file[2] << 16) | ((size_t)file[3] << 8) | (size_t)file[4];
    if (declared == 0) return -1;
    uint8_t *buf = malloc(declared);
    if (!buf) return -1;
    size_t n = 0;
    if (fifa96_record_decode(file, file_len, buf, declared, &n) != 0) {
      free(buf);
      return -1;
    }
    *out = buf;
    *out_len = n;
    return 0;
  }
  return -1;
}

static int pitch_entry_decode(const uint8_t *raw, size_t entry_len, size_t raw_len,
                              uint8_t **out, uint32_t *out_len) {
  *out = NULL;
  *out_len = 0;
  if (raw_len >= 4 && memcmp(raw, "SHPI", 4) == 0) {
    fifa96_sprite_bank bank;
    if (fifa96_sprite_bank_parse(raw, entry_len, &bank) != FIFA96_OK) return -1;
    uint8_t *copy = malloc(bank.total_size);
    if (!copy) return -1;
    memcpy(copy, raw, bank.total_size);
    *out = copy;
    *out_len = bank.total_size;
    return 0;
  }
  if (raw_len < 5 || raw[1] != 0xFB) return -1;
  const uint8_t *src = raw;
  size_t src_len = raw_len;
  uint8_t *cur = NULL;
  for (int stage = 0; stage < PITCH_DECODE_STAGES; stage++) {
    if (src_len >= 4 && memcmp(src, "SHPI", 4) == 0) {
      fifa96_sprite_bank bank;
      if (fifa96_sprite_bank_parse(src, src_len, &bank) != FIFA96_OK) {
        free(cur);
        return -1;
      }
      uint8_t *copy = malloc(bank.total_size);
      if (!copy) {
        free(cur);
        return -1;
      }
      memcpy(copy, src, bank.total_size);
      free(cur);
      *out = copy;
      *out_len = bank.total_size;
      return 0;
    }
    if (src_len < 5 || src[1] != 0xFB) {
      free(cur);
      return -1;
    }
    size_t declared = ((size_t)src[2] << 16) | ((size_t)src[3] << 8) | (size_t)src[4];
    if (declared == 0) {
      free(cur);
      return -1;
    }
    uint8_t *dst = malloc(declared);
    if (!dst) {
      free(cur);
      return -1;
    }
    size_t n = 0;
    if (fifa96_record_decode(src, src_len, dst, declared, &n) != 0) {
      free(dst);
      free(cur);
      return -1;
    }
    free(cur);
    cur = dst;
    src = dst;
    src_len = n;
  }
  free(cur);
  return -1;
}

/* Copy a frame of the requested size from a decoded SHPI bank. FplainH/V
 * carry a single 64x64 frame; STACcovr carries 32/64/128 LOD frames (A1 §3),
 * so the crowd loader asks for the 128x128 one first and falls back to 64. */
static int pitch_bank_tile_wh(const uint8_t *bank_data, uint32_t bank_len, uint8_t *dst,
                              int want_w, int want_h) {
  fifa96_sprite_bank bank;
  if (fifa96_sprite_bank_parse(bank_data, bank_len, &bank) != FIFA96_OK) return 0;
  size_t bytes = (size_t)want_w * (size_t)want_h;
  for (uint32_t i = 0; i < bank.count; i++) {
    fifa96_sprite_entry entry;
    fifa96_sprite_frame frame;
    if (fifa96_sprite_bank_entry(&bank, i, &entry) != FIFA96_OK) continue;
    if (fifa96_sprite_frame_parse(&bank, entry.offset, &frame) != FIFA96_OK) continue;
    if ((int)frame.width != want_w || (int)frame.height != want_h ||
        frame.pixel_len < bytes || !frame.pixels)
      continue;
    memcpy(dst, frame.pixels, bytes);
    return 1;
  }
  return 0;
}


/* Find a BIGF entry by name and decode its first 64x64 frame. Returns 1 on
 * success, 0 when the entry/frame is absent or undecodable. */
static int pitch_container_tile_wh(const uint8_t *file, size_t file_len, const char *name,
                                   const char *alt_name, uint8_t *dst, int want_w,
                                   int want_h) {
  uint8_t *container = NULL;
  size_t container_len = 0;
  int found = 0;
  if (pitch_container_decode(file, file_len, &container, &container_len) != 0) return 0;
  struct fifa96_bigf_info info;
  if (fifa96_bigf_parse(container, container_len, &info) == FIFA96_OK) {
    for (size_t i = 0; i < info.count && !found; i++) {
      uint32_t off = 0;
      uint32_t size = 0;
      const char *entry_name = NULL;
      if (fifa96_bigf_record(&info, i, &off, &size, &entry_name) != FIFA96_OK) continue;
      if (!entry_name ||
          (!pitch_name_eq(entry_name, name) &&
           (!alt_name || !pitch_name_eq(entry_name, alt_name))))
        continue;
      uint8_t *bank = NULL;
      uint32_t bank_len = 0;
      if (pitch_entry_decode(container + off, size, container_len - off, &bank,
                             &bank_len) != 0)
        continue;
      found = pitch_bank_tile_wh(bank, bank_len, dst, want_w, want_h);
      free(bank);
    }
  }
  free(container);
  return found;
}

static int pitch_container_tile(const uint8_t *file, size_t file_len, const char *name,
                                const char *alt_name, uint8_t *dst) {
  return pitch_container_tile_wh(file, file_len, name, alt_name, dst, FIFA96_PITCH_TILE_W,
                                 FIFA96_PITCH_TILE_H);
}

int fifa96_pitch_stage(struct fifa96_pitch *pitch, const struct fifa96_engine *eng,
                       const char *field_path, const char *stadium_path) {
  uint8_t *bytes = NULL;
  size_t len = 0;
  uint8_t *owner = NULL;
  uint8_t *tile_h;
  uint8_t *tile_v;
  uint8_t *crowd;
  uint8_t *stand;
  uint8_t *ad;
  uint8_t *banner;
  if (!pitch || !eng || !eng->assets || !field_path || !stadium_path)
    return -FIFA96_ERR_INVALID;
  owner = calloc(2u * FIFA96_PITCH_TILE_BYTES + FIFA96_PITCH_CROWD_MAX_BYTES +
                     FIFA96_PITCH_STAGE_EXTRA_BYTES,
                 1);
  if (!owner) return -FIFA96_ERR_IO;
  tile_h = owner;
  tile_v = owner + FIFA96_PITCH_TILE_BYTES;
  crowd = owner + 2u * FIFA96_PITCH_TILE_BYTES;
  stand = crowd + FIFA96_PITCH_CROWD_MAX_BYTES;
  ad = stand + FIFA96_PITCH_STAND_BYTES;
  banner = ad + FIFA96_PITCH_AD_BYTES;

  if (fifa96_asset_read(eng->assets, field_path, &bytes, &len) != FIFA96_OK) {
    free(owner);
    return -FIFA96_ERR_NOT_FOUND;
  }
  int have_h = pitch_container_tile(bytes, len, "FplainH.fsh", "FHstripH.fsh", tile_h);
  int have_v = pitch_container_tile(bytes, len, "FplainV.fsh", "FHstripV.fsh", tile_v);
  fifa96_asset_free(bytes);
  bytes = NULL;
  if (!have_h || !have_v) {
    free(owner);
    return -FIFA96_ERR_UNSUPPORTED;
  }

  if (fifa96_asset_read(eng->assets, stadium_path, &bytes, &len) != FIFA96_OK) {
    free(owner);
    return -FIFA96_ERR_NOT_FOUND;
  }
  int crowd_w = FIFA96_PITCH_CROWD_MAX;
  int crowd_h = FIFA96_PITCH_CROWD_MAX;
  int have_crowd = pitch_container_tile_wh(bytes, len, "STACcovr.fsh", NULL, crowd,
                                           crowd_w, crowd_h);
  if (!have_crowd) {
    crowd_w = FIFA96_PITCH_TILE_W;
    crowd_h = FIFA96_PITCH_TILE_H;
    have_crowd = pitch_container_tile_wh(bytes, len, "STACcovr.fsh", NULL, crowd,
                                         crowd_w, crowd_h);
  }
  /* The same GAMESTD1 container carries the stand facade (STARcovr, 64x128),
   * the EA Sports hoarding (STAScovr, 194x87) and the banner strip (STABcovr,
   * 128x16). They are optional: a container without them keeps the old
   * screen-space crowd band fallback. */
  int have_stand = pitch_container_tile_wh(bytes, len, "STARcovr.fsh", NULL, stand,
                                           FIFA96_PITCH_STAND_W, FIFA96_PITCH_STAND_H);
  int have_ad = pitch_container_tile_wh(bytes, len, "STAScovr.fsh", NULL, ad,
                                        FIFA96_PITCH_AD_W, FIFA96_PITCH_AD_H);
  int have_banner = pitch_container_tile_wh(bytes, len, "STABcovr.fsh", NULL, banner,
                                            FIFA96_PITCH_BANNER_W,
                                            FIFA96_PITCH_BANNER_H);
  fifa96_asset_free(bytes);
  if (!have_crowd) {
    free(owner);
    return -FIFA96_ERR_UNSUPPORTED;
  }

  fifa96_pitch_release(pitch);
  pitch->owner = owner;
  pitch->tile_h = tile_h;
  pitch->tile_v = tile_v;
  pitch->crowd = crowd;
  pitch->crowd_w = crowd_w;
  pitch->crowd_h = crowd_h;
  pitch->stand = have_stand ? stand : NULL;
  pitch->stand_w = have_stand ? FIFA96_PITCH_STAND_W : 0;
  pitch->stand_h = have_stand ? FIFA96_PITCH_STAND_H : 0;
  pitch->ad = have_ad ? ad : NULL;
  pitch->ad_w = have_ad ? FIFA96_PITCH_AD_W : 0;
  pitch->ad_h = have_ad ? FIFA96_PITCH_AD_H : 0;
  pitch->banner = have_banner ? banner : NULL;
  pitch->banner_w = have_banner ? FIFA96_PITCH_BANNER_W : 0;
  pitch->banner_h = have_banner ? FIFA96_PITCH_BANNER_H : 0;
  pitch->staged = 1;
  pitch->sky_index = FIFA96_PITCH_SKY_INDEX;
  pitch->line_index = FIFA96_PITCH_LINE_INDEX;
  return 0;
}

void fifa96_pitch_release(struct fifa96_pitch *pitch) {
  if (!pitch) return;
  free(pitch->owner);
  memset(pitch, 0, sizeof *pitch);
}

static int32_t pitch_abs32(int32_t v) { return v < 0 ? -v : v; }

/* The native H/V tile selector FUN_0004C9D4 (A1 §2): V for yaw inside
 * [0x2000,0x6000] or [0xA000,0xE000] (the native returns 0 there); H for the
 * complement, including (0x6000,0xA000) and the two extremes. The earlier v1
 * had the two sets swapped; Fplain H/V pixels are identical so only the
 * FHstrip fallback showed the rotation. */
static const uint8_t *pitch_tile_pick(const struct fifa96_pitch *pitch, int32_t yaw) {
  uint16_t a = (uint16_t)yaw;
  int use_v = (a >= 0x2000u && a <= 0x6000u) || (a >= 0xA000u && a <= 0xE000u);
  return use_v ? pitch->tile_v : pitch->tile_h;
}

/* v1 marking gate: a 2*FIFA96_PITCH_LINE_HALF-unit wide band around the
 * native line coordinates (A1 §4 [D]). The pitch rectangle is x ±2155,
 * z ±3102 (FUN_0006D870, A1 §5); the goal lines sit at x ±2016 and the
 * touchlines at z ±3102. */
#define FIFA96_PITCH_LINE_HALF 6
#define FIFA96_PITCH_HALF_LENGTH 0x7E0    /* goal line x = ±2016 */
#define FIFA96_PITCH_HALF_WIDTH 0xC1Eu    /* touchline z = ±3102 */
#define FIFA96_PITCH_CIRCLE_R 0x3C0       /* centre circle radius [D] */
#define FIFA96_PITCH_MAX_DEPTH 0x4000     /* native floor depth cut, A1 §2 (P10: extended for the wide live view) */

static int pitch_marking(int32_t wx, int32_t wz) {
  int32_t ax = pitch_abs32(wx);
  int32_t az = pitch_abs32(wz);
  if (ax > 0x86B || az > (int32_t)FIFA96_PITCH_HALF_WIDTH) return 0;
  if (ax <= FIFA96_PITCH_LINE_HALF) return 1;                       /* halfway */
  if (pitch_abs32(ax - FIFA96_PITCH_HALF_LENGTH) <= FIFA96_PITCH_LINE_HALF)
    return 1;                                                       /* goal lines */
  if (pitch_abs32(az - FIFA96_PITCH_HALF_WIDTH) <= FIFA96_PITCH_LINE_HALF &&
      ax <= FIFA96_PITCH_HALF_LENGTH)
    return 1;                                                       /* touchlines */
  {
    int64_t d2 = (int64_t)wx * wx + (int64_t)wz * wz;
    int64_t lo = FIFA96_PITCH_CIRCLE_R - FIFA96_PITCH_LINE_HALF;
    int64_t hi = FIFA96_PITCH_CIRCLE_R + FIFA96_PITCH_LINE_HALF;
    if (d2 >= lo * lo && d2 <= hi * hi) return 1;                   /* circle */
  }
  return 0;
}

/* The far-stand wall [D]: the native 3dwpts bowl is a set of projected stand
 * quads (P10-L2). This derived layer uses the real GAMESTD1 banks on a single
 * wall just behind the far touchline (x = +0x7E0 + 64) and inverse-projects
 * each pixel onto it. It gives the crowd, seating rows and advertising boards
 * correct perspective relative to the camera-0 sideline rig, while keeping the
 * native 3dwpts renderer a numbered open leg. */
#define FIFA96_PITCH_WALL_X (FIFA96_PITCH_HALF_LENGTH + 64)
#define FIFA96_PITCH_WALL_TOP 1500
#define FIFA96_PITCH_WALL_AD 180
#define FIFA96_PITCH_WALL_SEAT 700
#define FIFA96_PITCH_WALL_CROWD 1280

struct pitch_wall_colors {
  uint8_t blue;
  uint8_t blue_light;
  uint8_t blue_dark;
  uint8_t red;
  uint8_t red_light;
  uint8_t red_dark;
  uint8_t white;
  uint8_t grey;
  uint8_t dark;
};

static uint8_t pitch_nearest_rgb(const uint8_t *pal, int r, int g, int b) {
  int best = 0;
  int best_d = 0x7fffffff;
  for (int i = 0; i < 256; i++) {
    int dr = (int)pal[i * 3] - r;
    int dg = (int)pal[i * 3 + 1] - g;
    int db = (int)pal[i * 3 + 2] - b;
    int d = dr * dr + dg * dg + db * db;
    if (d < best_d) {
      best_d = d;
      best = i;
    }
  }
  return (uint8_t)best;
}

/* The stand banks are indexed for the live match palette, but their seat and
 * hoarding pixels were authored to read as bright rows/text rather than as
 * the raw match colours. Use a three-level tint (light row, seat colour, dark
 * shadow) so the derived wall reads as blue/red seating and a white-lettered
 * EA Sports board instead of a dark noise field. */
static uint8_t pitch_wall_tint(uint8_t idx, uint8_t tint, uint8_t tint_light,
                               uint8_t tint_dark, uint8_t white) {
  if (idx >= 240) return white;
  if (idx >= 140) return tint_light;
  if (idx >= 60) return tint;
  return tint_dark;
}

static uint8_t pitch_ad_tint(uint8_t idx, const struct pitch_wall_colors *col) {
  if (idx >= 140) return col->white;
  if (idx >= 60) return col->blue_dark;
  return col->dark;
}

static uint8_t pitch_wall_pixel(const struct fifa96_pitch *pitch, int32_t wy, int32_t wz,
                                const struct pitch_wall_colors *col) {
  const int32_t half_w = (int32_t)FIFA96_PITCH_HALF_WIDTH;
  int32_t u;
  int32_t v;
  if (pitch_abs32(wy - FIFA96_PITCH_WALL_AD) <= 4 ||
      pitch_abs32(wy - FIFA96_PITCH_WALL_SEAT) <= 4 ||
      pitch_abs32(wy - FIFA96_PITCH_WALL_CROWD) <= 4 ||
      pitch_abs32(wy - (FIFA96_PITCH_WALL_AD + FIFA96_PITCH_WALL_SEAT) / 2) <= 4)
    return col->white;
  if (wy < FIFA96_PITCH_WALL_AD) {
    if (pitch->banner && pitch->banner_w > 0 && pitch->banner_h > 0 && wy < 48) {
      u = (int32_t)(((int64_t)(wz + half_w) * pitch->banner_w) / 512);
      u %= pitch->banner_w;
      if (u < 0) u += pitch->banner_w;
      v = (48 - wy) * pitch->banner_h / 48;
      if (v < 0) v = 0;
      if (v >= pitch->banner_h) v = pitch->banner_h - 1;
      return pitch_wall_tint(
          pitch->banner[(size_t)v * (size_t)pitch->banner_w + (size_t)u],
          col->blue, col->blue_light, col->blue_dark, col->white);
    }
    if (pitch->ad && pitch->ad_w > 0 && pitch->ad_h > 0) {
      u = (int32_t)(((int64_t)(wz + half_w) * pitch->ad_w) / 1200);
      u %= pitch->ad_w;
      if (u < 0) u += pitch->ad_w;
      v = (FIFA96_PITCH_WALL_AD - wy) * pitch->ad_h / FIFA96_PITCH_WALL_AD;
      if (v < 0) v = 0;
      if (v >= pitch->ad_h) v = pitch->ad_h - 1;
      return pitch_ad_tint(
          pitch->ad[(size_t)v * (size_t)pitch->ad_w + (size_t)u], col);
    }
  }
  if (wy < FIFA96_PITCH_WALL_SEAT) {
    if (pitch->stand && pitch->stand_w > 0 && pitch->stand_h > 0) {
      const int32_t mid = (FIFA96_PITCH_WALL_AD + FIFA96_PITCH_WALL_SEAT) / 2;
      uint8_t tint = wy < mid ? col->blue : col->red;
      uint8_t tint_light = wy < mid ? col->blue_light : col->red_light;
      uint8_t tint_dark = wy < mid ? col->blue_dark : col->red_dark;
      u = (int32_t)(((int64_t)(wz + half_w) * pitch->stand_w) / 512);
      u %= pitch->stand_w;
      if (u < 0) u += pitch->stand_w;
      v = (FIFA96_PITCH_WALL_SEAT - wy) * pitch->stand_h /
          (FIFA96_PITCH_WALL_SEAT - FIFA96_PITCH_WALL_AD);
      if (v < 0) v = 0;
      if (v >= pitch->stand_h) v = pitch->stand_h - 1;
      return pitch_wall_tint(
          pitch->stand[(size_t)v * (size_t)pitch->stand_w + (size_t)u], tint,
          tint_light, tint_dark, col->white);
    }
  }
  if (wy < FIFA96_PITCH_WALL_CROWD) {
    if (pitch->crowd && pitch->crowd_w > 0 && pitch->crowd_h > 0) {
      u = (int32_t)(((int64_t)(wz + half_w) * pitch->crowd_w) / 1024);
      u %= pitch->crowd_w;
      if (u < 0) u += pitch->crowd_w;
      v = (FIFA96_PITCH_WALL_CROWD - wy) * pitch->crowd_h /
          (FIFA96_PITCH_WALL_CROWD - FIFA96_PITCH_WALL_SEAT);
      if (v < 0) v = 0;
      if (v >= pitch->crowd_h) v = pitch->crowd_h - 1;
      return pitch->crowd[(size_t)v * (size_t)pitch->crowd_w + (size_t)u];
    }
  }
  {
    /* Dark roof field with light truss uprights; the old band used the sky
     * shade here, which read as a second blue sky above the crowd. */
    uint8_t roof = col->dark;
    if (((wz + half_w) & 383) < 18) roof = col->grey;
    if (wy > FIFA96_PITCH_WALL_TOP - 24) roof = col->grey;
    return roof;
  }
}

static void pitch_wall_draw(const struct fifa96_pitch *pitch, struct fifa96_surface *s,
                            const struct fifa96_render_pos *eye, const int32_t mt[9],
                            const fifa96_render_clip *clip,
                            const fifa96_projection_point *center, int32_t fov_w,
                            int32_t fov_h) {
  if (!pitch->stand || pitch->stand_w <= 0) return;
  int32_t left = clip->left;
  int32_t top = clip->top;
  int32_t right = clip->right;
  int32_t bottom = clip->bottom;
  if (left < 0) left = 0;
  if (top < 0) top = 0;
  if (right > s->width) right = s->width;
  if (bottom > s->height) bottom = s->height;
  if (right <= left || bottom <= top) return;
  if (eye->x >= FIFA96_PITCH_WALL_X) return;
  struct pitch_wall_colors col;
  col.blue = pitch->sky_index;
  col.blue_light = pitch->line_index;
  col.blue_dark = pitch->shade ? pitch->shade[pitch->sky_index] : pitch->sky_index;
  col.red = pitch->sky_index;
  col.red_light = pitch->line_index;
  col.red_dark = col.blue_dark;
  col.white = pitch->line_index;
  col.grey = pitch->shade ? pitch->shade[pitch->line_index] : pitch->line_index;
  col.dark = 0x00;
  if (pitch->palette) {
    col.blue = pitch_nearest_rgb(pitch->palette, 40, 80, 180);
    col.blue_light = pitch_nearest_rgb(pitch->palette, 110, 160, 230);
    col.blue_dark = pitch_nearest_rgb(pitch->palette, 5, 15, 60);
    col.red = pitch_nearest_rgb(pitch->palette, 190, 40, 40);
    col.red_light = pitch_nearest_rgb(pitch->palette, 230, 150, 150);
    col.red_dark = pitch_nearest_rgb(pitch->palette, 70, 10, 15);
    col.white = pitch_nearest_rgb(pitch->palette, 245, 245, 245);
    col.grey = pitch_nearest_rgb(pitch->palette, 120, 120, 120);
    col.dark = pitch_nearest_rgb(pitch->palette, 0, 0, 0);
  }
  for (int32_t sy = top; sy < bottom; sy++) {
    int32_t dy = (int32_t)(((int64_t)center->y - ((int64_t)sy << 16)) /
                           (int64_t)fov_h);
    uint8_t *row = s->indexed + (size_t)sy * (size_t)s->width;
    for (int32_t sx = left; sx < right; sx++) {
      int32_t dx = (int32_t)((((int64_t)sx << 16) - (int64_t)center->x) /
                             (int64_t)fov_w);
      fifa96_projection_vec cam_dir = {dx, dy, 1 << 16};
      fifa96_projection_vec world_dir;
      if (fifa96_projection_transform(mt, &cam_dir, &world_dir) != FIFA96_OK) continue;
      if (world_dir.x <= 0) continue;
      int64_t t = ((int64_t)(FIFA96_PITCH_WALL_X - eye->x) << 16) / world_dir.x;
      if (t <= 0) continue;
      int64_t wy = (int64_t)eye->y + (t * world_dir.y >> 16);
      if (wy < 0 || wy >= FIFA96_PITCH_WALL_TOP) continue;
      /* If the ground ray reaches the wall plane closer than the wall, the
       * pitch is in front of the stand. */
      if (world_dir.y < 0) {
        int64_t tg = -((int64_t)eye->y << 16) / world_dir.y;
        if (tg > 0) {
          int64_t gx = (int64_t)eye->x + (tg * world_dir.x >> 16);
          if (gx < FIFA96_PITCH_WALL_X) continue;
        }
      }
      int64_t wz = (int64_t)eye->z + (t * world_dir.z >> 16);
      row[sx] = pitch_wall_pixel(pitch, (int32_t)wy, (int32_t)wz, &col);
    }
  }
}

int fifa96_pitch_draw(const struct fifa96_pitch *pitch, struct fifa96_surface *s,
                      const struct fifa96_render_pos *eye, int32_t yaw,
                      const int32_t matrix[9], const fifa96_render_clip *clip,
                      const fifa96_projection_point *center, int32_t fov_w,
                      int32_t fov_h) {
  if (!pitch || !s || !eye || !matrix || !clip || !center) return -FIFA96_ERR_INVALID;
  if (!pitch->staged || !pitch->tile_h || !pitch->tile_v || !pitch->crowd) return 0;
  if (fov_w <= 0 || fov_h <= 0) return -FIFA96_ERR_INVALID;

  int32_t left = clip->left;
  int32_t top = clip->top;
  int32_t right = clip->right;
  int32_t bottom = clip->bottom;
  if (left < 0) left = 0;
  if (top < 0) top = 0;
  if (right > s->width) right = s->width;
  if (bottom > s->height) bottom = s->height;
  if (right <= left || bottom <= top) return 0;

  int32_t mt[9];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) mt[i * 3 + j] = matrix[j * 3 + i];

  /* Horizon: the first row whose centre-column camera ray points down at the
   * ground plane. The floor is drawn below it, the sky/crowd band above. */
  int32_t horizon = bottom;
  if (eye->y > 0) {
    for (int32_t sy = top; sy < bottom; sy++) {
      int32_t dy = (int32_t)(((int64_t)center->y - ((int64_t)sy << 16)) /
                             (int64_t)fov_h);
      fifa96_projection_vec cam_dir = {0, dy, 1 << 16};
      fifa96_projection_vec world_dir;
      if (fifa96_projection_transform(mt, &cam_dir, &world_dir) != FIFA96_OK) break;
      if (world_dir.y < 0) {
        horizon = sy;
        break;
      }
    }
  }

  /* Sky, roof and the crowd band [D] (A1 §4). The band scales with the
   * surface height so the 640x480 shell and the 320x240 tape read the same. */
  int32_t band_rows = FIFA96_PITCH_CROWD_BAND_ROWS * s->height / 240;
  if (band_rows < 16) band_rows = 16;
  int32_t band_top = horizon - band_rows;
  int32_t ad_rows = 4 * s->height / 240;
  if (ad_rows < 2) ad_rows = 2;
  uint8_t roof_index = pitch->shade
                           ? pitch->shade[pitch->shade[pitch->sky_index]]
                           : (uint8_t)(pitch->sky_index >> 1);
  uint32_t crowd_u_off = (uint32_t)((yaw & 0xFFFF) >> 9);
  for (int32_t sy = top; sy < horizon; sy++) {
    uint8_t *row = s->indexed + (size_t)sy * (size_t)s->width;
    if (pitch->stand) {
      /* The projected stand wall (pitch_wall_draw) replaces the old
       * screen-space band; paint sky first and let the wall overwrite the
       * stand area. */
      for (int32_t sx = left; sx < right; sx++) row[sx] = pitch->sky_index;
      continue;
    }
    if (sy < band_top) {
      /* The native broadcast view is inside the stadium: the top of the
       * frame is the stand roof, not open sky. Draw a lighter top edge, a
       * dark roof field and periodic vertical supports so it reads as a
       * structure rather than a flat band. */
      int32_t edge = 2 * s->height / 240;
      uint8_t edge_col = pitch->shade ? pitch->shade[pitch->line_index]
                                      : pitch->line_index;
      uint8_t support = pitch->shade ? pitch->shade[roof_index] : roof_index;
      for (int32_t sx = left; sx < right; sx++) {
        uint8_t roof = roof_index;
        if (sy < edge) roof = edge_col;
        else if (sy >= band_top - edge) roof = edge_col;
        else if (((uint32_t)sx % 96u) < 2u) roof = support;
        row[sx] = roof;
      }
      continue;
    }
    int32_t band_row = sy - band_top;
    int cw = pitch->crowd_w > 0 ? pitch->crowd_w : FIFA96_PITCH_TILE_W;
    int ch = pitch->crowd_h > 0 ? pitch->crowd_h : FIFA96_PITCH_TILE_H;
    uint32_t v = ((uint32_t)band_row * (uint32_t)ch) / (uint32_t)band_rows;
    if (v >= (uint32_t)ch) v = (uint32_t)(ch - 1);
    const uint8_t *crowd_row = pitch->crowd + (size_t)v * (size_t)cw;
    if (sy >= horizon - ad_rows) {
      /* Advertising hoardings along the near edge of the stand: a dark wall
       * with a white panel every 64 source pixels. */
      uint8_t wall = pitch->shade ? pitch->shade[pitch->sky_index]
                                  : (uint8_t)(pitch->sky_index >> 1);
      for (int32_t sx = left; sx < right; sx++) {
        uint32_t panel = ((uint32_t)sx + crowd_u_off) & 63u;
        row[sx] = panel < 28u ? pitch->line_index : wall;
      }
      continue;
    }
    for (int32_t sx = left; sx < right; sx++) {
      uint32_t u = ((uint32_t)sx + crowd_u_off) % (uint32_t)cw;
      row[sx] = crowd_row[u];
    }
  }

  if (eye->y <= 0 || horizon >= bottom) return 0;
  const uint8_t *tile = pitch_tile_pick(pitch, yaw);
  for (int32_t sy = horizon; sy < bottom; sy++) {
    int32_t dy = (int32_t)(((int64_t)center->y - ((int64_t)sy << 16)) /
                           (int64_t)fov_h);
    uint8_t *row = s->indexed + (size_t)sy * (size_t)s->width;
    for (int32_t sx = left; sx < right; sx++) {
      int32_t dx = (int32_t)((((int64_t)sx << 16) - (int64_t)center->x) /
                             (int64_t)fov_w);
      fifa96_projection_vec cam_dir = {dx, dy, 1 << 16};
      fifa96_projection_vec world_dir;
      if (fifa96_projection_transform(mt, &cam_dir, &world_dir) != FIFA96_OK) continue;
      /* Above the ground plane (or behind the eye): leave the sky/band value. */
      if (world_dir.y >= 0) continue;
      int64_t t = -((int64_t)eye->y << 16) / world_dir.y;
      if (t <= 0 || t > FIFA96_PITCH_MAX_DEPTH) continue;
      int32_t wx = eye->x + (int32_t)((t * world_dir.x) >> 16);
      int32_t wz = eye->z + (int32_t)((t * world_dir.z) >> 16);
      if (pitch_marking(wx, wz)) {
        row[sx] = pitch->line_index;
      } else {
        uint32_t u = (uint32_t)wx & 63u;
        uint32_t v = (uint32_t)wz & 63u;
        uint8_t index = tile[(size_t)v * FIFA96_PITCH_TILE_W + u];
        /* Mowing stripes (derived [D]): alternate the live shade table every
         * 32 world-texel diagonal band. */
        if (pitch->shade != NULL && (((wx + wz) >> 6) & 1))
          index = pitch->shade[index];
        row[sx] = index;
      }
    }
  }
  pitch_wall_draw(pitch, s, eye, mt, clip, center, fov_w, fov_h);
  return 0;
}
