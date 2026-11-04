#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *planes[4];    /* Mode-X planes, 320x240 */
  size_t         stride;       /* plane stride in bytes (80) */
  const uint8_t  palette[768]; /* 8-bit RGB, DAC-expanded */
  uint32_t       flags;
} fifa96_platform_frame;

typedef struct {
  int32_t raw_code;            /* backend-neutral key/pad code */
  int32_t state;               /* 0 = release, 1 = press */
} fifa96_platform_key;

typedef struct fifa96_platform {
  int      (*init)(void *self, int w, int h, const char *title);
  void     (*shutdown)(void *self);
  void     (*destroy)(void *self);   /* frees backend state; generic destroy calls it */
  int      (*poll)(void *self, fifa96_platform_key *out, size_t cap, int *count);
  int      (*present)(void *self, const fifa96_platform_frame *frame);
  void     (*audio_open)(void *self, uint32_t rate, int channels);
  void     (*audio_submit)(void *self, const int16_t *pcm, size_t frames);
  uint64_t (*now_ns)(void *self);
  void     (*sleep_ns)(void *self, uint64_t ns);
  void     *self;
} fifa96_platform;

fifa96_platform *fifa96_platform_sdl3_create(void);
void fifa96_platform_destroy(fifa96_platform *p);
