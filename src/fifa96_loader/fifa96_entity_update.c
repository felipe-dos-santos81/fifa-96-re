#include "fifa96_loader/fifa96_entity_update.h"

/* `FUN_000CD474`'s 257-byte ratio table, flat `0x14072C` (moved down from
 * `fifa96_action_handlers.c`; the values are unchanged). */
static const uint8_t entity_atan[257] = {
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0A,
    0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x12, 0x12, 0x13, 0x14,
    0x14, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1E,
    0x1E, 0x1F, 0x1F, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x26, 0x27, 0x27,
    0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x30, 0x30, 0x31,
    0x31, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39, 0x39, 0x3A,
    0x3A, 0x3B, 0x3C, 0x3C, 0x3D, 0x3D, 0x3E, 0x3E, 0x3F, 0x3F, 0x40, 0x41, 0x41, 0x42, 0x42, 0x43,
    0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x48, 0x48, 0x49, 0x4A, 0x4A, 0x4B, 0x4B,
    0x4C, 0x4C, 0x4D, 0x4D, 0x4E, 0x4E, 0x4F, 0x4F, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53,
    0x54, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x57, 0x57, 0x58, 0x58, 0x59, 0x59, 0x5A, 0x5A, 0x5B,
    0x5B, 0x5B, 0x5C, 0x5C, 0x5D, 0x5D, 0x5E, 0x5E, 0x5F, 0x5F, 0x60, 0x60, 0x60, 0x61, 0x61, 0x62,
    0x62, 0x63, 0x63, 0x63, 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x66, 0x67, 0x67, 0x68, 0x68, 0x68,
    0x69, 0x69, 0x6A, 0x6A, 0x6A, 0x6B, 0x6B, 0x6C, 0x6C, 0x6C, 0x6D, 0x6D, 0x6E, 0x6E, 0x6E, 0x6F,
    0x6F, 0x70, 0x70, 0x70, 0x71, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x73, 0x74, 0x74, 0x74, 0x75,
    0x75, 0x76, 0x76, 0x76, 0x77, 0x77, 0x77, 0x78, 0x78, 0x78, 0x79, 0x79, 0x79, 0x7A, 0x7A, 0x7A,
    0x7B, 0x7B, 0x7B, 0x7C, 0x7C, 0x7C, 0x7D, 0x7D, 0x7D, 0x7E, 0x7E, 0x7E, 0x7F, 0x7F, 0x7F, 0x80,
    0x80,
};

/* `0x114E04`, the 257-entry sine table (moved down from `fifa96_ball_pairing.c`
 * where it backed `kick_sin`; the values are unchanged). */
static const int32_t entity_sine_table[257] = {
    0, 402, 804, 1206, 1608, 2010, 2412, 2814,
    3215, 3617, 4018, 4420, 4821, 5222, 5622, 6023,
    6423, 6823, 7223, 7623, 8022, 8421, 8819, 9218,
    9616, 10013, 10410, 10807, 11204, 11600, 11995, 12390,
    12785, 13179, 13573, 13966, 14359, 14751, 15142, 15533,
    15923, 16313, 16702, 17091, 17479, 17866, 18253, 18638,
    19024, 19408, 19792, 20175, 20557, 20938, 21319, 21699,
    22078, 22456, 22833, 23210, 23586, 23960, 24334, 24707,
    25079, 25450, 25820, 26189, 26557, 26925, 27291, 27656,
    28020, 28383, 28745, 29105, 29465, 29824, 30181, 30538,
    30893, 31247, 31600, 31952, 32302, 32651, 32999, 33346,
    33692, 34036, 34379, 34721, 35061, 35400, 35738, 36074,
    36409, 36743, 37075, 37406, 37736, 38064, 38390, 38716,
    39039, 39361, 39682, 40002, 40319, 40636, 40950, 41263,
    41575, 41885, 42194, 42501, 42806, 43110, 43412, 43712,
    44011, 44308, 44603, 44897, 45189, 45480, 45768, 46055,
    46340, 46624, 46906, 47186, 47464, 47740, 48015, 48288,
    48558, 48828, 49095, 49360, 49624, 49886, 50145, 50403,
    50659, 50914, 51166, 51416, 51665, 51911, 52155, 52398,
    52639, 52877, 53114, 53348, 53581, 53811, 54040, 54266,
    54491, 54713, 54933, 55152, 55368, 55582, 55794, 56004,
    56212, 56417, 56621, 56822, 57022, 57219, 57414, 57606,
    57797, 57986, 58172, 58356, 58538, 58718, 58895, 59070,
    59243, 59414, 59583, 59749, 59913, 60075, 60235, 60392,
    60547, 60700, 60850, 60998, 61144, 61288, 61429, 61568,
    61705, 61839, 61971, 62100, 62228, 62353, 62475, 62596,
    62714, 62829, 62942, 63053, 63162, 63268, 63371, 63473,
    63571, 63668, 63762, 63854, 63943, 64030, 64114, 64197,
    64276, 64353, 64428, 64501, 64571, 64638, 64703, 64766,
    64826, 64884, 64939, 64992, 65043, 65091, 65136, 65179,
    65220, 65258, 65294, 65327, 65358, 65386, 65412, 65436,
    65457, 65475, 65491, 65505, 65516, 65524, 65531, 65534,
    65536,
};

int32_t fifa96_entity_sine(int32_t angle) {
  uint32_t u = (uint32_t)angle;
  int32_t idx = (int32_t)(u & 0xFFu);
  int32_t bit8 = (int32_t)((u >> 8) & 1u);
  int32_t bit9 = (int32_t)((u >> 9) & 1u);
  int32_t v;
  idx = (int32_t)(((uint32_t)idx ^ (0u - (uint32_t)bit8)) & 0xFFu);
  idx += bit8;   /* 0x7BDA9 SUB EAX,ECX with ECX = -bit8 */
  v = entity_sine_table[idx];
  return bit9 ? -v : v;
}

int fifa96_entity_angle(int32_t x, int32_t z, int32_t *angle) {
  uint32_t flags = 0;
  uint32_t ux, uz, mn = 0, mx = 0;
  int32_t value = -1;
  if (!angle) return -FIFA96_ERR_INVALID;
  ux = (uint32_t)x;
  uz = (uint32_t)z;
  if (x < 0) {
    flags |= 16;
    ux = 0u - ux;
  }
  if (z < 0) {
    flags |= 8;
    uz = 0u - uz;
  }
  if (ux >= uz) {
    if (ux == uz) {
      value = 0x80;
    } else {
      flags |= 4;
      mx = ux;
      mn = uz;
    }
  } else {
    mx = uz;
    mn = ux;
  }
  if (value < 0) {
    uint64_t quotient = ((uint64_t)mn << 32) / mx;
    uint32_t index = (uint32_t)(quotient >> 24) + (uint32_t)((quotient >> 23) & 1u);
    value = entity_atan[index];
  }
  switch (flags) {
    case 0: break;
    case 4: value = 0x100 - value; break;
    case 8: value = 0x200 - value; break;
    case 12: value = value + 0x100; break;
    case 16: value = -value; break;
    case 20: value = value - 0x100; break;
    case 24: value = value - 0x200; break;
    default: value = -value - 0x100; break;
  }
  *angle = value;
  return FIFA96_OK;
}

int fifa96_entity_intercept_bind(int32_t actor_x, uint8_t actor_side,
                                 int32_t nearest_x, int32_t nearest_z,
                                 fifa96_entity_intercept_target *out) {
  int64_t nz;
  int32_t half;
  int32_t edge;
  if (!out) return -FIFA96_ERR_INVALID;
  (void)nearest_x;   /* read only by the native's dead |nearest.x| test */
  out->y = 0;
  nz = nearest_z;
  if (nz < 0) nz = -nz;
  half = (int32_t)((0xB10 - nz) / 2);
  edge = 0xB10 - (half + 0x120);
  out->z = actor_side != 0 ? -edge : edge;
  if (actor_x < 0 ? -(int64_t)actor_x >= 0x180 : actor_x >= 0x180) {
    int64_t base = (int64_t)actor_x + 0x180;
    out->x = (int32_t)(base / 3) + 0xC0;   /* IDIV truncates toward zero */
  } else if (actor_x > 0) {
    out->x = actor_x - 0x240;
  } else {
    out->x = 0x240 - actor_x;
  }
  return FIFA96_OK;
}

int fifa96_entity_intercept_band(int32_t pos_x, int32_t pos_z,
                                 int32_t target_x, int32_t target_z,
                                 fifa96_entity_intercept_band_out *out) {
  int16_t dx;
  int16_t dz;
  int32_t a;
  int32_t divisor;
  uint32_t magnitude;
  if (!out) return -FIFA96_ERR_INVALID;
  dx = (int16_t)(uint16_t)((uint16_t)target_x - (uint16_t)pos_x);
  dz = (int16_t)(uint16_t)((uint16_t)target_z - (uint16_t)pos_z);
  out->dx = dx;
  out->dz = dz;
  if (fifa96_entity_angle(dx, dz, &a) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if (a < 0) a = -a;
  if (a > 0x100) a = 0x200 - a;
  divisor = a > 0x80 ? fifa96_entity_sine(a) : fifa96_entity_sine(a + 0x100);
  magnitude = (uint32_t)(dx < 0 ? -dx : dx);
  out->band = (int16_t)(uint16_t)((magnitude << 16) / (uint32_t)divisor);
  return FIFA96_OK;
}

static int32_t fifa96_entity_abs_word(int32_t value) {
  if ((int16_t)value < 0) value = -value;
  return value;
}

int32_t fifa96_entity_distance(int32_t dx, int32_t dy) {
  int32_t a = fifa96_entity_abs_word(dx);
  int32_t b = fifa96_entity_abs_word(dy);
  int32_t a16 = (int16_t)a;
  int32_t b16 = (int16_t)b;
  int32_t q;
  if (a16 < b16) {
    q = a16 >> 2;
    if (a16 > (b16 >> 1)) q = ((a16 >> 1) + q) >> 1;
    return q + b;
  }
  if (a16 > b16) {
    q = b16 >> 2;
    if (b16 > (a16 >> 1)) q = ((b16 >> 1) + q) >> 1;
    return q + a;
  }
  q = (a16 >> 2) + (a16 >> 1);
  return (q >> 1) + (int16_t)b;
}

int fifa96_entity_find_nearest(const fifa96_entity_candidate *candidates,
                               uint32_t count, uint32_t skip_index,
                               int16_t target_x, int16_t target_y,
                               int16_t *best_distance) {
  uint16_t best;
  int best_index = -1;
  uint32_t i;
  if (!candidates || !best_distance) return -FIFA96_ERR_INVALID;
  best = 0xFFFFu;
  for (i = 0; i < count; i++) {
    uint16_t d;
    if (i == (uint32_t)(uint16_t)skip_index) continue;
    if (candidates[i].skip_98 != 0 || candidates[i].skip_9a != 0) continue;
    d = (uint16_t)fifa96_entity_distance((int16_t)(target_x - candidates[i].x),
                                         (int16_t)(target_y - candidates[i].y));
    if (d < best) {
      best = d;
      best_index = (int)i;
    }
  }
  *best_distance = (int16_t)best;
  return best_index;
}
