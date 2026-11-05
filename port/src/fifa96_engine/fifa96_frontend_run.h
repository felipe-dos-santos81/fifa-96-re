#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_lang.h"
#include "fifa96_engine/fifa96_menu_model.h"
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_frontend.h"

#define FIFA96_FRONTEND_TEAM_NAME_MAX 16

/* Engine-level front-end driver: maps platform keys to the game codes the
 * fifa96_frontend state machine understands, queues them, and steps the
 * library once per rendered frame.
 *
 * P2: when `native_menu` is set (asset-backed interactive boot) the same key
 * queue drives `menu` instead of the legacy FU-135 list; the library state
 * machine stays parked in FRONTEND. The deterministic headless M1/M2 tapes
 * never enable it, so their pinned frames are untouched. */
struct fifa96_frontend_run {
  struct fifa96_frontend frontend;  /* engine/library state machine */
  struct fifa96_surface *surface;
  uint32_t phase;                   /* mirrors fifa96_frontend.phase */
  uint32_t entry_state;             /* FU-65 state table index */
  int      selected_row;            /* menu highlight, FU135 row window */
  int      queue[8];                /* pending mapped key codes */
  int      queue_len;
  uint64_t frames;                  /* frames rendered since init */
  int      quit_requested;          /* FIFA96_ENGINE_KEY_QUIT seen since init */
  int      match_start;             /* FU-66 match-start exit classification */

  /* P2 native menu (A3): Game Select / Team Select state and the handoff
   * payload the bridge stages after fifa96_match_run_begin. */
  struct fifa96_lang lang;          /* decoded LANG000 + HELV12 */
  struct fifa96_menu_model menu;
  int      native_menu;             /* 1 = route input/draw through `menu` */
  uint32_t team_id[2];              /* LANG000 id of the chosen team (A3, no
                                     * roster consumer yet: P2-L4) */
  char     team_name[2][FIFA96_FRONTEND_TEAM_NAME_MAX];  /* full selected name */
  char     team_code[2][4];         /* derived 3-letter HUD code (P2-L4) */
  uint8_t  formation[2];            /* native [0x14C1E4]/[0x14C1E5] ids */
  int      team_selected;           /* 1 = the native menu supplies the current
                                     * team payload (the bridge stages it) */
  uint8_t  key_down[16];            /* F1: raw engine key codes currently held
                                     * (native edge latch; 1..10 in use) */
  int      input_armed;             /* F1: 0 = drop held input until a neutral
                                     * poll; 1 = edge-latch fresh presses */
};

/* Initialise a fresh (zeroed or uninitialised) front-end run. This clears the
 * struct, so it must not be called a second time on a live run: that would
 * drop the decoded LANG/font copies without freeing them. Use
 * fifa96_frontend_run_reset to re-enter Game Select after a match. */
int fifa96_frontend_run_init(struct fifa96_frontend_run *fr, struct fifa96_surface *s);
int fifa96_frontend_run_input(struct fifa96_frontend_run *fr, const fifa96_platform_key *keys,
                              size_t count);
int fifa96_frontend_run_step(struct fifa96_frontend_run *fr, struct fifa96_surface *s, int *quit);

/* FU-343 (P4): reset the interactive front-end after a match ends, without
 * discarding the decoded language/font or the native-menu enable flag. The
 * library state machine, menu model, queue, match-start classification and
 * team handoff are re-seeded exactly like fifa96_frontend_run_init, so the
 * next step draws Game Select and a second match can start. `fr->lang` and
 * `fr->native_menu` are preserved (the assets are not re-read). Returns 0, or
 * -1 for NULL arguments. */
int fifa96_frontend_run_reset(struct fifa96_frontend_run *fr, struct fifa96_surface *s);

/* P2 test/engine seam: switch between the legacy FU-135 render (0) and the
 * native Game Select model (1). The engine enables it only for the
 * asset-backed interactive boot; tests may call it directly. Enabling the
 * native path disarms the F1 input latch, so a key already held when control
 * is handed over is ignored until it is released. */
void fifa96_frontend_run_enable_native(struct fifa96_frontend_run *fr, int enable);

/* F1: disarm the native input edge latch and forget held keys. The next
 * native poll with no key down re-arms it; only a fresh press after that is
 * queued. Used on the INTRO -> FRONTEND handoff so the key that skipped the
 * intro cannot activate the menu (and after a post-match reset). */
void fifa96_frontend_run_disarm_input(struct fifa96_frontend_run *fr);
