#include "fifa96_loader/fifa96_mixer.h"
#include <string.h>

#define FIFA96_MIXER_VOLUME_MAX 0x7Fu

static uint16_t rd16le(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* floor(v / 128), matching the readers' IMUL + SAR 7 without relying on
 * implementation-defined signed right shift. */
static int32_t sar7(int32_t v) {
  return v >= 0 ? v / 128 : ~((~v) / 128);
}

/* Output converter 0xB9E53: clamp both bounds to 32767 (FU-37 §A.4). */
static int16_t mix_clamp(int32_t v) {
  if (v > 32767) return 32767;
  if (v < -32767) return -32767;
  return (int16_t)v;
}

void fifa96_mixer_init(struct fifa96_mixer *m) {
  if (!m) return;
  memset(m, 0, sizeof *m);
}

int fifa96_mixer_start(struct fifa96_mixer *m, int voice,
                       const struct fifa96_eacs_info *info,
                       const uint8_t *payload, size_t payload_len,
                       uint8_t volume, uint64_t step) {
  if (!m || !info || !payload) return -(int)FIFA96_ERR_TRUNCATED;
  if (voice < 0 || voice >= FIFA96_MIXER_VOICES) return -(int)FIFA96_ERR_TRUNCATED;
  if (volume > FIFA96_MIXER_VOLUME_MAX) return -(int)FIFA96_ERR_TRUNCATED;
  /* f10 == 2 needs the second-cursor nibble decoder and its 4x sample
   * accounting; both are FU-37 §A.5/§6.1 open legs. */
  if (info->f10 == 2) return -(int)FIFA96_ERR_UNSUPPORTED;
  if (info->format == FIFA96_EACS_FMT_UNKNOWN) return -(int)FIFA96_ERR_UNSUPPORTED;
  if (info->block_size == 0) return -(int)FIFA96_ERR_TRUNCATED;
  if (info->data_off > payload_len) return -(int)FIFA96_ERR_TRUNCATED;
  if (info->data_len > payload_len - info->data_off) return -(int)FIFA96_ERR_TRUNCATED;
  if ((uint64_t)info->blocks * info->block_size > info->data_len)
    return -(int)FIFA96_ERR_TRUNCATED;

  struct fifa96_mixer_voice *ch = &m->voices[voice];
  ch->active = 1;
  ch->format = info->format;
  ch->data = payload + info->data_off;
  ch->units = info->blocks;
  ch->volume = volume;
  ch->pos = 0;
  ch->step = step;
  ch->loop = 0;
  ch->loop_start = 0;
  ch->loop_end = 0;
  /* Retail videos force-write loop start -1 / length 0 (FU-37 §A.5); arm
   * only a well-formed window inside the buffer. */
  if (info->loop_len != 0 && info->loop_start >= 0) {
    uint64_t end = (uint64_t)(uint32_t)info->loop_start + info->loop_len;
    if (end <= ch->units && (uint32_t)info->loop_start < end) {
      ch->loop = 1;
      ch->loop_start = (uint32_t)info->loop_start;
      ch->loop_end = (uint32_t)end;
    }
  }
  return FIFA96_OK;
}

void fifa96_mixer_stop(struct fifa96_mixer *m, int voice) {
  if (!m || voice < 0 || voice >= FIFA96_MIXER_VOICES) return;
  m->voices[voice].active = 0;
}

int fifa96_mixer_voice_active(const struct fifa96_mixer *m, int voice) {
  if (!m || voice < 0 || voice >= FIFA96_MIXER_VOICES) return 0;
  return m->voices[voice].active != 0;
}

void fifa96_mixer_render(struct fifa96_mixer *m, int16_t *out, size_t frames) {
  if (!m || (!out && frames)) return;
  for (size_t f = 0; f < frames; f++) {
    int32_t acc_l = 0, acc_r = 0;
    for (int v = 0; v < FIFA96_MIXER_VOICES; v++) {
      struct fifa96_mixer_voice *ch = &m->voices[v];
      if (!ch->active) continue;
      uint64_t end32 = (uint64_t)ch->units << 32;
      if (ch->loop) {
        /* Producer 0xB832D: wrap to loop_start with zero fraction once the
         * next fetch would leave the loop window. The original counts frames
         * with the truncated 8.24 step; the full 32.32 cursor is used here
         * (identical for exact steps, FU-37 §A.5). */
        uint64_t loop_end32 = (uint64_t)ch->loop_end << 32;
        if (ch->pos >= loop_end32 || loop_end32 - ch->pos < ch->step)
          ch->pos = (uint64_t)ch->loop_start << 32;
      } else if (ch->pos >= end32) {
        ch->active = 0;
        continue;
      }
      uint32_t unit = (uint32_t)(ch->pos >> 32);
      if (unit >= ch->units) {
        ch->active = 0;
        continue;
      }
      if (ch->format == FIFA96_EACS_FMT_PCM16_STEREO) {
        const uint8_t *p = ch->data + (size_t)unit * 4;
        acc_l += sar7((int32_t)(int16_t)rd16le(p) * ch->volume);
        acc_r += sar7((int32_t)(int16_t)rd16le(p + 2) * ch->volume);
      } else if (ch->format == FIFA96_EACS_FMT_PCM8_STEREO) {
        /* FU-37 §A.3 volume table: 2*vol*signed8(byte). */
        const uint8_t *p = ch->data + (size_t)unit * 2;
        acc_l += 2 * ch->volume * (int32_t)(int8_t)p[0];
        acc_r += 2 * ch->volume * (int32_t)(int8_t)p[1];
      } else if (ch->format == FIFA96_EACS_FMT_PCM16_MONO) {
        int32_t s = (int16_t)rd16le(ch->data + (size_t)unit * 2);
        acc_l += sar7(s * ch->volume);
        acc_r += sar7(s * ch->volume);
      } else {
        continue; /* start() rejects every other format */
      }
      ch->pos += ch->step;
    }
    if (out) {
      out[2 * f] = mix_clamp(acc_l);
      out[2 * f + 1] = mix_clamp(acc_r);
    }
  }
}

int fifa96_mixer_step_from_rate(uint32_t rate, uint32_t out_rate,
                                uint32_t ratio, uint32_t shift,
                                uint64_t *step) {
  if (!step || out_rate == 0 || shift > 63) return -(int)FIFA96_ERR_TRUNCATED;
  uint64_t v = ((uint64_t)rate * ratio) >> shift;
  uint64_t int_part = v / out_rate;
  uint64_t rem = v - int_part * out_rate;
  uint64_t frac = (rem << 32) / out_rate;
  *step = ((int_part & 0xFFu) << 32) | frac;
  return FIFA96_OK;
}
