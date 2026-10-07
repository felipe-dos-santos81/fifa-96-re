/* src/fifa96_engine/platform_sdl3.c — SDL3 window/renderer/audio backend.
 *
 * Converts the engine's Mode-X planar-chunky frame (fifa96_platform_frame)
 * into a streaming ARGB8888 texture with integer-scaled logical presentation,
 * and feeds the engine's 16-bit stereo PCM into an SDL audio stream bound to
 * the default playback device. Input maps keyboard scancodes and gamepad
 * buttons onto the backend-neutral FIFA96_ENGINE_KEY_* codes.
 *
 * The generic fifa96_platform_destroy (fifa96_engine.c) frees the platform
 * and calls `destroy`; this backend must therefore not define that symbol.
 */
#include <limits.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_engine/fifa96_keys.h"

#define SDL3_MAX_GAMEPADS 4

struct sdl_state {
  SDL_Window *win;
  SDL_Renderer *ren;
  SDL_Texture *tex;
  SDL_AudioStream *stream;
  SDL_AudioDeviceID audio_dev;
  int audio_channels;
  int w, h;
  uint32_t *rgba;
  uint64_t audio_frames;
  SDL_Gamepad *pads[SDL3_MAX_GAMEPADS];
  int npads;
};

static void sdl3_release(struct sdl_state *s) {
  if (s->audio_dev) {
    SDL_CloseAudioDevice(s->audio_dev);
    s->audio_dev = 0;
  }
  if (s->stream) {
    SDL_DestroyAudioStream(s->stream);
    s->stream = NULL;
  }
  for (int i = 0; i < s->npads; i++) {
    if (s->pads[i]) SDL_CloseGamepad(s->pads[i]);
    s->pads[i] = NULL;
  }
  s->npads = 0;
  if (s->tex) { SDL_DestroyTexture(s->tex); s->tex = NULL; }
  if (s->ren) { SDL_DestroyRenderer(s->ren); s->ren = NULL; }
  if (s->win) { SDL_DestroyWindow(s->win); s->win = NULL; }
  SDL_Quit();
}

static int sdl3_init(void *self, int w, int h, const char *title) {
  struct sdl_state *s = self;
  if (w <= 0 || h <= 0) return -1;
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) return -1;
  s->w = w;
  s->h = h;
  s->rgba = malloc((size_t)w * (size_t)h * sizeof *s->rgba);
  if (!s->rgba) {
    SDL_Quit();
    return -1;
  }
  if (!SDL_CreateWindowAndRenderer(title ? title : "FIFA 96", w * 3, h * 3,
                                   SDL_WINDOW_RESIZABLE, &s->win, &s->ren)) {
    free(s->rgba);
    s->rgba = NULL;
    sdl3_release(s);
    return -1;
  }
  (void)SDL_SetRenderLogicalPresentation(s->ren, w, h,
                                         SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
  s->tex = SDL_CreateTexture(s->ren, SDL_PIXELFORMAT_ARGB8888,
                             SDL_TEXTUREACCESS_STREAMING, w, h);
  if (!s->tex) {
    free(s->rgba);
    s->rgba = NULL;
    sdl3_release(s);
    return -1;
  }
  return 0;
}

static void sdl3_shutdown(void *self) {
  struct sdl_state *s = self;
  if (s) sdl3_release(s);
}

static void sdl3_destroy(void *self) {
  struct sdl_state *s = self;
  if (!s) return;
  sdl3_release(s);
  free(s->rgba);
  free(s);
}

/* Mode-X planar-chunky per the ABI: pixel (x, y) lives in plane x & 3 at byte
 * y * stride + (x >> 2). RGB comes from the DAC-expanded palette. */
static void sdl3_expand(const struct sdl_state *s, const fifa96_platform_frame *f) {
  for (int y = 0; y < s->h; y++) {
    uint32_t *row = s->rgba + (size_t)y * (size_t)s->w;
    for (int x = 0; x < s->w; x++) {
      uint8_t idx = f->planes[x & 3][(size_t)y * f->stride + (size_t)(x >> 2)];
      row[x] = 0xFF000000u | ((uint32_t)f->palette[idx * 3 + 0] << 16) |
               ((uint32_t)f->palette[idx * 3 + 1] << 8) |
               (uint32_t)f->palette[idx * 3 + 2];
    }
  }
}

static int sdl3_present(void *self, const fifa96_platform_frame *f) {
  struct sdl_state *s = self;
  if (!s->ren || !s->tex || !f) return -1;
  sdl3_expand(s, f);
  if (!SDL_UpdateTexture(s->tex, NULL, s->rgba, s->w * 4)) return -1;
  if (!SDL_RenderClear(s->ren)) return -1;
  if (!SDL_RenderTexture(s->ren, s->tex, NULL, NULL)) return -1;
  SDL_RenderPresent(s->ren);
  return 0;
}

static int32_t sdl3_map_scancode(SDL_Scancode sc) {
  switch (sc) {
    case SDL_SCANCODE_UP:     return FIFA96_ENGINE_KEY_UP;
    case SDL_SCANCODE_DOWN:   return FIFA96_ENGINE_KEY_DOWN;
    case SDL_SCANCODE_LEFT:   return FIFA96_ENGINE_KEY_LEFT;
    case SDL_SCANCODE_RIGHT:  return FIFA96_ENGINE_KEY_RIGHT;
    case SDL_SCANCODE_RETURN: return FIFA96_ENGINE_KEY_CONFIRM;
    case SDL_SCANCODE_ESCAPE: return FIFA96_ENGINE_KEY_QUIT;
    default:                  return 0;
  }
}

static int32_t sdl3_map_button(Uint8 b) {
  switch (b) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:    return FIFA96_ENGINE_KEY_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:  return FIFA96_ENGINE_KEY_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:  return FIFA96_ENGINE_KEY_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return FIFA96_ENGINE_KEY_RIGHT;
    case SDL_GAMEPAD_BUTTON_SOUTH:      return FIFA96_ENGINE_KEY_CONFIRM;
    case SDL_GAMEPAD_BUTTON_EAST:       return FIFA96_ENGINE_KEY_DECLINE;
    case SDL_GAMEPAD_BUTTON_START:      return FIFA96_ENGINE_KEY_QUIT;
    default:                            return 0;
  }
}

static void sdl3_push_key(fifa96_platform_key *out, size_t cap, int *count,
                          int32_t raw_code, int32_t state) {
  if (!out || raw_code == 0 || (size_t)*count >= cap) return;
  out[*count].raw_code = raw_code;
  out[*count].state = state;
  (*count)++;
}

static void sdl3_pad_added(struct sdl_state *s, SDL_JoystickID id) {
  if (s->npads >= SDL3_MAX_GAMEPADS) return;
  SDL_Gamepad *g = SDL_OpenGamepad(id);
  if (!g) return;
  s->pads[s->npads++] = g;
}

static void sdl3_pad_removed(struct sdl_state *s, SDL_JoystickID id) {
  for (int i = 0; i < s->npads; i++) {
    if (SDL_GetGamepadID(s->pads[i]) != id) continue;
    SDL_CloseGamepad(s->pads[i]);
    s->pads[i] = s->pads[--s->npads];
    s->pads[s->npads] = NULL;
    return;
  }
}

static int sdl3_poll(void *self, fifa96_platform_key *out, size_t cap, int *count) {
  struct sdl_state *s = self;
  if (!count) return -1;
  *count = 0;
  SDL_PumpEvents();
  SDL_Event ev;
  while (SDL_PollEvent(&ev)) {
    switch (ev.type) {
      case SDL_EVENT_QUIT:
        sdl3_push_key(out, cap, count, FIFA96_ENGINE_KEY_QUIT, 1);
        break;
      case SDL_EVENT_KEY_DOWN:
        if (!ev.key.repeat)
          sdl3_push_key(out, cap, count, sdl3_map_scancode(ev.key.scancode), 1);
        break;
      case SDL_EVENT_KEY_UP:
        sdl3_push_key(out, cap, count, sdl3_map_scancode(ev.key.scancode), 0);
        break;
      case SDL_EVENT_GAMEPAD_ADDED:
        sdl3_pad_added(s, ev.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        sdl3_pad_removed(s, ev.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        sdl3_push_key(out, cap, count, sdl3_map_button(ev.gbutton.button), 1);
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        sdl3_push_key(out, cap, count, sdl3_map_button(ev.gbutton.button), 0);
        break;
      default:
        break;
    }
  }
  return 0;
}

static void sdl3_audio_open(void *self, uint32_t rate, int channels) {
  struct sdl_state *s = self;
  if (s->audio_dev || rate == 0 || channels <= 0) return;
  SDL_AudioSpec spec;
  spec.format = SDL_AUDIO_S16;
  spec.channels = channels;
  spec.freq = (int)rate;
  s->audio_dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
  if (!s->audio_dev) return;
  s->stream = SDL_CreateAudioStream(&spec, &spec);
  if (!s->stream || !SDL_BindAudioStream(s->audio_dev, s->stream)) {
    if (s->stream) {
      SDL_DestroyAudioStream(s->stream);
      s->stream = NULL;
    }
    SDL_CloseAudioDevice(s->audio_dev);
    s->audio_dev = 0;
    return;
  }
  s->audio_channels = channels;
  (void)SDL_ResumeAudioDevice(s->audio_dev);
}

static void sdl3_audio_submit(void *self, const int16_t *pcm, size_t frames) {
  struct sdl_state *s = self;
  if (!s->stream || !pcm || frames == 0 || s->audio_channels <= 0) return;
  size_t bytes = frames * (size_t)s->audio_channels * sizeof *pcm;
  if (bytes > (size_t)INT_MAX) return;
  (void)SDL_PutAudioStreamData(s->stream, pcm, (int)bytes);
  s->audio_frames += frames;
}

static uint64_t sdl3_now_ns(void *self) {
  (void)self;
  return (uint64_t)SDL_GetTicksNS();
}

static void sdl3_sleep_ns(void *self, uint64_t ns) {
  (void)self;
  SDL_DelayNS(ns);
}

fifa96_platform *fifa96_platform_sdl3_create(void) {
  struct sdl_state *s = calloc(1, sizeof *s);
  if (!s) return NULL;
  fifa96_platform *p = calloc(1, sizeof *p);
  if (!p) {
    free(s);
    return NULL;
  }
  p->init = sdl3_init;
  p->shutdown = sdl3_shutdown;
  p->destroy = sdl3_destroy;
  p->poll = sdl3_poll;
  p->present = sdl3_present;
  p->audio_open = sdl3_audio_open;
  p->audio_submit = sdl3_audio_submit;
  p->now_ns = sdl3_now_ns;
  p->sleep_ns = sdl3_sleep_ns;
  p->self = s;
  return p;
}
