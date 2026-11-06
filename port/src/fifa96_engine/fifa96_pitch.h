/* fifa96_pitch.h — P1 (FU-343): the visible-match pitch renderer.
 *
 * Derived v1 of the native field/stadium pass (recon A1):
 *  - the plain field container /ART/GAMEFLD1.PVI carries
 *    `FplainH.fsh`/`FplainV.fsh` (64x64, SHPI, 4096-byte tiles, A1 §1/§2).
 *    The retail field binding is GAMEFLD3 (FU-2), but its entries are the
 *    stripped `FHstripH/V` names + `3dwpts.dat` (A1 §1, measured); the
 *    windowed bridge therefore tries GAMEFLD1 first and falls back to the
 *    GAMEFLD3 FHstrip pair. The native floor is the per-scanline Mode-X
 *    caster FUN_0005F684 (`0x5F684..0x5FEA9`, A1 §2); this module instead
 *    inverse-projects one ray per pixel through the transpose of
 *    fifa96_projection_matrix and samples the real 64x64 tile. The native
 *    world-units-per-texel scale (`>>12`, A1 leg 4) is a numbered open leg;
 *    v1 uses one world unit per texel.
 *  - the stadium container /ART/GAMESTD1.PVI carries `STACcovr.fsh` (9 crowd
 *    frames, 32/64/128 px, A1 §3), `STARcovr.fsh` (64x128 seating rows),
 *    `STAScovr.fsh` (194x87 EA Sports hoarding) and `STABcovr.fsh` (128x16
 *    banners). The live path inverse-projects a derived far-stand wall at the
 *    far touchline using these banks; the native 3dwpts point/quad projection
 *    (FUN_000610C4) and the goal-net pass remain the P10-L2 open leg.
 *  - the white markings (goal lines x = ±0x7E0 = 2016, touchlines z = ±0xC1E =
 *    3102, halfway line x = 0, centre circle r ~ 0x3C0) are drawn
 *    procedurally (A1 §4 [D]); the native markings are not in the Fplain tile.
 *
 * The module owns decoded tile copies; fifa96_match_run_stage_pitch feeds it
 * from the engine asset table and fifa96_match_run_render draws it only on the
 * live_start path, so the M1/M2 non-live renders stay byte-identical.
 */
#pragma once
#include <stdint.h>

#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_render.h"

struct fifa96_engine;
struct fifa96_surface;

#define FIFA96_PITCH_TILE_W 64
#define FIFA96_PITCH_TILE_H 64
#define FIFA96_PITCH_TILE_BYTES (FIFA96_PITCH_TILE_W * FIFA96_PITCH_TILE_H)
#define FIFA96_PITCH_CROWD_BAND_ROWS 56
#define FIFA96_PITCH_CROWD_MAX 128
#define FIFA96_PITCH_CROWD_MAX_BYTES (FIFA96_PITCH_CROWD_MAX * FIFA96_PITCH_CROWD_MAX)
/* Native GAMESTD1 stand banks used by the derived far-stand wall. STARcovr is
 * the seating row texture, STAScovr the EA Sports hoarding and STABcovr the
 * banner strip; the native 3dwpts bowl remains the P10-L2 open leg. */
#define FIFA96_PITCH_STAND_W 64
#define FIFA96_PITCH_STAND_H 128
#define FIFA96_PITCH_STAND_BYTES (FIFA96_PITCH_STAND_W * FIFA96_PITCH_STAND_H)
#define FIFA96_PITCH_AD_W 194
#define FIFA96_PITCH_AD_H 87
#define FIFA96_PITCH_AD_BYTES (FIFA96_PITCH_AD_W * FIFA96_PITCH_AD_H)
#define FIFA96_PITCH_BANNER_W 128
#define FIFA96_PITCH_BANNER_H 16
#define FIFA96_PITCH_BANNER_BYTES (FIFA96_PITCH_BANNER_W * FIFA96_PITCH_BANNER_H)
#define FIFA96_PITCH_STAGE_EXTRA_BYTES \
  (FIFA96_PITCH_STAND_BYTES + FIFA96_PITCH_AD_BYTES + FIFA96_PITCH_BANNER_BYTES)

/* PALsys-index choices for the v1 band/markings. The live match palette is
 * the PALsys frame 0 base with the PALteam band overlay (the P1 measured
 * correction; the non-live PALsys frame-2 chunk never reaches the pitch
 * path); index 0x3C is the blue-grey sky entry and 0xFE is set to white by
 * the live kit stage. */
#define FIFA96_PITCH_SKY_INDEX 0x3Cu
#define FIFA96_PITCH_LINE_INDEX 0xFEu

struct fifa96_pitch {
  uint8_t *owner;              /* owned 3-tile arena, or NULL */
  const uint8_t *tile_h;       /* FplainH.fsh frame 0, 64x64 */
  const uint8_t *tile_v;       /* FplainV.fsh frame 0, 64x64 */
  const uint8_t *crowd;        /* STACcovr.fsh frame (128x128 preferred) */
  int crowd_w, crowd_h;        /* selected crowd frame dimensions */
  const uint8_t *stand;        /* STARcovr.fsh 64x128 seat rows, or NULL */
  int stand_w, stand_h;
  const uint8_t *ad;           /* STAScovr.fsh 194x87 EA Sports board, or NULL */
  int ad_w, ad_h;
  const uint8_t *banner;       /* STABcovr.fsh 128x16 banner strip, or NULL */
  int banner_w, banner_h;
  int staged;                  /* all three tiles present */
  uint8_t sky_index;           /* PALsys index of the sky rows */
  uint8_t line_index;          /* PALsys index of the white markings */
  const uint8_t *shade;        /* live palette shade table (or NULL) */
  const uint8_t *palette;      /* live RGB palette for stand tinting, or NULL */
};

/* Stage the two derived match-art containers through the engine asset table:
 * `field_path` (the bridge supplies /ART/GAMEFLD1.PVI and a GAMEFLD3
 * FHstrip fallback) and `stadium_path` (/ART/GAMESTD1.PVI). The floor tiles
 * and the crowd are required; the stand/ad/banner banks are optional. On
 * failure of the required set the previous staging is left untouched and a
 * negative fifa96_err_t is returned. NULL arguments -> -FIFA96_ERR_INVALID. */
int fifa96_pitch_stage(struct fifa96_pitch *pitch, const struct fifa96_engine *eng,
                       const char *field_path, const char *stadium_path);

/* Free the owned tiles and return the module to its unstaged state. */
void fifa96_pitch_release(struct fifa96_pitch *pitch);

/* Draw the v1 pitch into the indexed surface, clipped to `clip`. `eye` is the
 * render eye (live: focus + camera-0 offset (-1920,160,30)); `matrix` is the
 * already-built fifa96_projection_matrix(yaw,pitch); `center` is the same
 * 16.16 projection centre the sprite pass uses; `fov_w`/`fov_h` are the
 * projection focal dimensions (live: the native full surface lens; non-live
 * never draws the pitch). Draw order inside: sky/fallback band, the
 * inverse-projected floor and markings, then the derived far-stand wall.
 * Returns 0, or -FIFA96_ERR_INVALID on NULL arguments; an unstaged pitch is a
 * no-op. */
int fifa96_pitch_draw(const struct fifa96_pitch *pitch, struct fifa96_surface *s,
                      const struct fifa96_render_pos *eye, int32_t yaw,
                      const int32_t matrix[9], const fifa96_render_clip *clip,
                      const fifa96_projection_point *center, int32_t fov_w,
                      int32_t fov_h);
