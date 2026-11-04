#include "fifa96_loader/fifa96_window.h"

int fifa96_window_init(fifa96_window *win, int32_t screen_w, int32_t screen_h) {
  if (!win) return -FIFA96_ERR_INVALID;
  win->screen_w = screen_w;
  win->screen_h = screen_h;
  win->box.x0 = 0;
  win->box.y0 = 0;
  win->box.x1 = 0;
  win->box.y1 = 0;
  win->box.w = 0;
  win->box.h = 0;
  win->box.x0_fix = 0;
  win->box.y0_fix = 0;
  win->box.x1_fix = 0;
  win->box.y1_fix = 0;
  win->src_x = 0;
  win->src_y = 0;
  win->src_w = 0;
  win->src_h = 0;
  win->cx_fix = 0;
  win->cy_fix = 0;
  return FIFA96_OK;
}

int fifa96_window_set(fifa96_window *win, int32_t x, int32_t y, int32_t w, int32_t h) {
  if (!win) return -FIFA96_ERR_INVALID;
  int32_t cw = w < FIFA96_WINDOW_MIN ? FIFA96_WINDOW_MIN
                                     : (w > win->screen_w ? win->screen_w : w);
  int32_t ch = h < FIFA96_WINDOW_MIN ? FIFA96_WINDOW_MIN
                                     : (h > win->screen_h ? win->screen_h : h);
  int32_t cx = x < 0 ? 0 : (x > win->screen_w - cw ? win->screen_w - cw : x);
  int32_t cy = y < 0 ? 0 : (y > win->screen_h - ch ? win->screen_h - ch : y);
  fifa96_window_box *box = &win->box;
  box->x0 = cx;
  box->y0 = cy;
  box->x1 = cx + cw;
  box->y1 = cy + ch;
  box->w = cw;
  box->h = ch;
  box->x0_fix = cx << 16;
  box->y0_fix = cy << 16;
  box->x1_fix = (cx + cw) << 16;
  box->y1_fix = (cy + ch) << 16;
  win->src_x = x;
  win->src_y = y;
  win->src_w = w;
  win->src_h = h;
  win->cx_fix = (x + (w >> 1)) << 16;
  win->cy_fix = (y + (h >> 1)) << 16;
  return FIFA96_OK;
}

int fifa96_window_define_full(fifa96_window *win, int32_t w, int32_t h) {
  if (!win) return -FIFA96_ERR_INVALID;
  win->screen_w = w;
  win->screen_h = h;
  return fifa96_window_set(win, 0, 0, w, h);
}

int fifa96_window_expand(const fifa96_window *win, fifa96_window_box *saved,
                         const int32_t *pts, int count) {
  if (!win || !saved) return -FIFA96_ERR_INVALID;
  if (count < 0 || count > FIFA96_WINDOW_POINTS_MAX) return -FIFA96_ERR_INVALID;
  if (count > 0 && !pts) return -FIFA96_ERR_INVALID;
  *saved = win->box;
  if (count == 0) return FIFA96_OK;
  int32_t min_x = pts[0];
  int32_t max_x = pts[0];
  int32_t min_y = pts[1];
  int32_t max_y = pts[1];
  for (int i = 1; i < count; i++) {
    int32_t px = pts[i * 2];
    int32_t py = pts[i * 2 + 1];
    if (px < min_x) min_x = px;
    if (px > max_x) max_x = px;
    if (py < min_y) min_y = py;
    if (py > max_y) max_y = py;
  }
  if (saved->x0 < min_x) saved->x0 = min_x;
  if (saved->x1 < max_x) saved->x1 = max_x;
  if (saved->y0 < min_y) saved->y0 = min_y;
  if (saved->y1 < max_y) saved->y1 = max_y;
  return FIFA96_OK;
}

int fifa96_window_scale(const fifa96_window *win, int32_t *scale_x, int32_t *scale_y) {
  if (!win || !scale_x || !scale_y) return -FIFA96_ERR_INVALID;
  *scale_x = (int32_t)(((int64_t)win->box.w << 16) / 320);
  *scale_y = (int32_t)(((int64_t)win->box.h << 16) / 200);
  return FIFA96_OK;
}

int fifa96_window_zoomed(const fifa96_window *win, int *zoomed) {
  if (!win || !zoomed) return -FIFA96_ERR_INVALID;
  int32_t scale_x, scale_y;
  int rc = fifa96_window_scale(win, &scale_x, &scale_y);
  if (rc != FIFA96_OK) return rc;
  *zoomed = scale_x < 0x10000;
  return FIFA96_OK;
}

/* The native fixed-point scale `(value*scale + 0x8000) >> 16` (signed
 * 64-bit, 0x53240's CONCAT/SHRD sequence). */
static int32_t window_strip_scale(int32_t value, int32_t scale) {
  return (int32_t)(((int64_t)value * scale + 0x8000) >> 16);
}

int fifa96_window_strip_layout(const fifa96_window *win, int32_t frame5_height,
                               int32_t name_width, int wide,
                               fifa96_window_strip *out) {
  int32_t scale_x, scale_y;
  if (!win || !out) return -FIFA96_ERR_INVALID;
  if (fifa96_window_scale(win, &scale_x, &scale_y) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  for (int side = 0; side < 2; side++) {
    out->row[side].y0 = win->box.y0;                              /* 0x53E6A/0x53E71 */
    out->row[side].height = wide ? frame5_height * 2 : frame5_height;
    out->row[side].y_text = win->box.y0 + window_strip_scale(12, scale_y);
  }
  out->row[0].x = win->box.x0 + window_strip_scale(4, scale_x);
  out->row[1].x = win->box.x1 - window_strip_scale(4, scale_x) -
                  window_strip_scale(name_width, scale_x);
  return FIFA96_OK;
}
