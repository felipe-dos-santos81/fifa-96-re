/* fifa96_pitch.h — P1 (FU-343): the visible-match pitch renderer.
 *
 * Derived v1 of the native field/stadium pass (recon A1):
 *  - the field container /ART/GAMEFLD3.PVI carries `FplainH.fsh`/`FplainV.fsh`
 *    (64x64, SHPI, 4096-byte tiles, A1 §1/§2). The native floor is the
 *    per-scanline Mode-X caster FUN_0005F684 (`0x5F684..0x5FEA9`, A1 §2); this
 *    module instead inverse-projects one ray per pixel through the transpose
 *    of fifa96_projection_matrix and samples the real 64x64 tile. The native
 *    world-units-per-texel scale (`>>12`, A1 leg 4) is a numbered open leg;
 *    v1 uses one world unit per texel.
 *  - the stadium container /ART/GAMESTD1.PVI carries `STACcovr.fsh` (9 crowd
 *    frames, 32/64/128 px, A1 §3). v1 fills the crowd band above the horizon
 *    with the 64x64 STACcovr frame; the native 3dwpts stand-point projection
 *    (FUN_000610C4) is a numbered open leg.
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

/* PALsys-index choices for the v1 band/markings (the match palette staged by
 * fifa96_match_run_stage is the PALsys frame-2 chunk; index 0x3C is the
 * blue-grey sky entry and 0xFE is set to white by the live kit stage). */
#define FIFA96_PITCH_SKY_INDEX 0x3Cu
#define FIFA96_PITCH_LINE_INDEX 0xFEu

struct fifa96_pitch {
  uint8_t *owner;              /* owned 3-tile arena, or NULL */
  const uint8_t *tile_h;       /* FplainH.fsh frame 0, 64x64 */
  const uint8_t *tile_v;       /* FplainV.fsh frame 0, 64x64 */
  const uint8_t *crowd;        /* STACcovr.fsh 64x64 frame */
  int staged;                  /* all three tiles present */
  uint8_t sky_index;           /* PALsys index of the sky rows */
  uint8_t line_index;          /* PALsys index of the white markings */
};

/* Stage the two derived match-art containers through the engine asset table:
 * `field_path` (default /ART/GAMEFLD3.PVI) and `stadium_path` (default
 * /ART/GAMESTD1.PVI). The new tiles are committed only when all three are
 * decoded; on failure the previous staging is left untouched and a negative
 * fifa96_err_t is returned. NULL arguments -> -FIFA96_ERR_INVALID. */
int fifa96_pitch_stage(struct fifa96_pitch *pitch, const struct fifa96_engine *eng,
                       const char *field_path, const char *stadium_path);

/* Free the owned tiles and return the module to its unstaged state. */
void fifa96_pitch_release(struct fifa96_pitch *pitch);

/* Draw the v1 pitch into the indexed surface, clipped to `clip`. `eye` is the
 * render eye (live: focus + camera-0 offset (-1920,160,30)); `matrix` is the
 * already-built fifa96_projection_matrix(yaw,pitch); `center` is the same
 * 16.16 projection centre the sprite pass uses; `fov_w`/`fov_h` are the
 * projection focal dimensions (live: the wide lens; non-live never draws the
 * pitch). Draw order inside: sky rows,
 * the 56-row crowd band, the inverse-projected floor, then the white markings
 * (markings are folded into the floor sample). Returns 0, or
 * -FIFA96_ERR_INVALID on NULL arguments; an unstaged pitch is a no-op. */
int fifa96_pitch_draw(const struct fifa96_pitch *pitch, struct fifa96_surface *s,
                      const struct fifa96_render_pos *eye, int32_t yaw,
                      const int32_t matrix[9], const fifa96_render_clip *clip,
                      const fifa96_projection_point *center, int32_t fov_w,
                      int32_t fov_h);
