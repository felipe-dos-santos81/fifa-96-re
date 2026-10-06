#include "fifa96_loader/fifa96_projection.h"

static const int32_t fifa96_projection_sin_table[257] = {
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

static int32_t fifa96_projection_sar(int32_t v, int shift) {
  return (int32_t)((int64_t)v >> shift);
}

static int32_t fifa96_projection_neg32(int32_t v) {
  return (int32_t)(0u - (uint32_t)v);
}

fifa96_err_t fifa96_projection_sincos(int32_t angle, int32_t *sin16, int32_t *cos16) {
  if (!sin16 || !cos16) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  uint16_t a = (uint16_t)(angle >> 6);
  int32_t v = a & 0xFF;
  int32_t sb;
  int32_t cb;
  switch ((a >> 8) & 3) {
    case 0:
      sb = fifa96_projection_sin_table[v];
      cb = fifa96_projection_sin_table[256 - v];
      break;
    case 1:
      sb = fifa96_projection_sin_table[256 - v];
      cb = -fifa96_projection_sin_table[v];
      break;
    case 2:
      sb = -fifa96_projection_sin_table[v];
      cb = -fifa96_projection_sin_table[256 - v];
      break;
    default:
      sb = -fifa96_projection_sin_table[256 - v];
      cb = fifa96_projection_sin_table[v];
      break;
  }
  int32_t k = fifa96_projection_sar((angle & 0x3F) * 0x6487E, 9);
  *sin16 = sb + fifa96_projection_sar(fifa96_projection_sar(cb, 2) * k, 21);
  *cos16 = cb - fifa96_projection_sar(fifa96_projection_sar(sb, 2) * k, 21);
  return FIFA96_OK;
}

fifa96_err_t fifa96_projection_matrix(int32_t yaw, int32_t pitch, int32_t m[9]) {
  if (!m) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  int32_t s2, c2, s1, c1;
  fifa96_projection_sincos(yaw, &s2, &c2);
  fifa96_projection_sincos(fifa96_projection_neg32(pitch), &s1, &c1);
  const int32_t m2[9] = {c2, 0, -s2, 0, 0x10000, 0, s2, 0, c2};
  const int32_t m1[9] = {0x10000, 0, 0, 0, c1, s1, 0, -s1, c1};
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      int64_t acc = (int64_t)m2[i * 3] * m1[j] + (int64_t)m2[i * 3 + 1] * m1[3 + j] +
                    (int64_t)m2[i * 3 + 2] * m1[6 + j];
      m[i * 3 + j] = (int32_t)(acc >> 16);
    }
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_projection_transform(const int32_t m[9], const fifa96_projection_vec *v,
                                         fifa96_projection_vec *out) {
  if (!m || !v || !out) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  int64_t x = (int64_t)v->x * m[0] + (int64_t)v->y * m[3] + (int64_t)v->z * m[6];
  int64_t y = (int64_t)v->x * m[1] + (int64_t)v->y * m[4] + (int64_t)v->z * m[7];
  int64_t z = (int64_t)v->x * m[2] + (int64_t)v->y * m[5] + (int64_t)v->z * m[8];
  out->x = (int32_t)(x >> 16);
  out->y = (int32_t)(y >> 16);
  out->z = (int32_t)(z >> 16);
  return FIFA96_OK;
}

fifa96_err_t fifa96_projection_reciprocal(int32_t dim, int32_t *table) {
  if (!table || dim <= 0 || dim > 0x7FFF) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  for (int i = 0; i < FIFA96_PROJECTION_RECIP_COUNT; i++) {
    int32_t d = i + 1;
    if (d < 10) d = 10;
    table[i] = (int32_t)(((int64_t)dim << 16) / d);
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_projection_screen(const int32_t *recip_x, const int32_t *recip_y,
                                      const fifa96_projection_point *center,
                                      const fifa96_projection_vec *v,
                                      fifa96_projection_point *out, uint8_t *visible) {
  if (!recip_x || !recip_y || !center || !v || !out || !visible)
    return (fifa96_err_t)-FIFA96_ERR_INVALID;
  *visible = 0;
  if (v->z < FIFA96_PROJECTION_NEAR) return FIFA96_OK;
  int32_t z = v->z;
  int shift = 0;
  if (z >= 0x10000) {
    z = fifa96_projection_sar(z, 8);
    shift += 8;
  }
  if (z >= 0x1000) {
    z = fifa96_projection_sar(z, 4);
    shift += 4;
  }
  if (z >= 0x400) {
    z = fifa96_projection_sar(z, 2);
    shift += 2;
  }
  if (z >= FIFA96_PROJECTION_RECIP_COUNT) return FIFA96_OK;
  int32_t px = (int32_t)((uint32_t)v->x * (uint32_t)recip_x[z]);
  int32_t py = (int32_t)((uint32_t)v->y * (uint32_t)recip_y[z]);
  out->x = (int32_t)((uint32_t)center->x + (uint32_t)fifa96_projection_sar(px, shift));
  out->y = (int32_t)((uint32_t)center->y - (uint32_t)fifa96_projection_sar(py, shift));
  *visible = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_projection_project(const int32_t m[9], const fifa96_projection_vec *cam,
                                       const int32_t *recip_x, const int32_t *recip_y,
                                       const fifa96_projection_point *center,
                                       const fifa96_projection_vec *world,
                                       fifa96_projection_point *out, uint8_t *visible) {
  if (!m || !cam || !recip_x || !recip_y || !center || !world || !out || !visible)
    return (fifa96_err_t)-FIFA96_ERR_INVALID;
  fifa96_projection_vec rel;
  rel.x = (int32_t)((uint32_t)world->x - (uint32_t)cam->x);
  rel.y = (int32_t)((uint32_t)world->y - (uint32_t)cam->y);
  rel.z = (int32_t)((uint32_t)world->z - (uint32_t)cam->z);
  fifa96_projection_vec rot;
  fifa96_projection_transform(m, &rel, &rot);
  return fifa96_projection_screen(recip_x, recip_y, center, &rot, out, visible);
}
