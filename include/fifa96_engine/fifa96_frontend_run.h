#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_frontend.h"

#define FIFA96_ENGINE_KEY_UP      1
#define FIFA96_ENGINE_KEY_DOWN    2
#define FIFA96_ENGINE_KEY_LEFT    3
#define FIFA96_ENGINE_KEY_RIGHT   4
#define FIFA96_ENGINE_KEY_CONFIRM 5
#define FIFA96_ENGINE_KEY_DECLINE 6
#define FIFA96_ENGINE_KEY_QUIT    7

/* Engine-level front-end driver: maps platform keys to the game codes the
 * fifa96_frontend state machine understands, queues them, and steps the
 * library once per rendered frame. */
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
};

int fifa96_frontend_run_init(struct fifa96_frontend_run *fr, struct fifa96_surface *s);
int fifa96_frontend_run_input(struct fifa96_frontend_run *fr, const fifa96_platform_key *keys,
                              size_t count);
int fifa96_frontend_run_step(struct fifa96_frontend_run *fr, struct fifa96_surface *s, int *quit);
