#pragma once

/* Engine-level key codes shared by the front-end and match run drivers. A
 * fifa96_platform_key carries one of these as raw_code with state == 1 for a
 * press (fifa96_platform.h). QUIT is consumed by fifa96_engine_step and never
 * reaches a run's input path. */
#define FIFA96_ENGINE_KEY_UP      1
#define FIFA96_ENGINE_KEY_DOWN    2
#define FIFA96_ENGINE_KEY_LEFT    3
#define FIFA96_ENGINE_KEY_RIGHT   4
#define FIFA96_ENGINE_KEY_CONFIRM 5
#define FIFA96_ENGINE_KEY_DECLINE 6
#define FIFA96_ENGINE_KEY_QUIT    7

/* Match action keys (no front-end meaning). */
#define FIFA96_ENGINE_KEY_KICK 8
#define FIFA96_ENGINE_KEY_PASS 9
