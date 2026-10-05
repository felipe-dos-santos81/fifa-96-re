#include "fifa96_loader/fifa96_mixer.h"
#include <string.h>

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
  m->next_voice = 0;   /* DAT_000148AC is BSS-zero in the image (FU-47 §0) */
  m->sound_state = 1;  /* port default: gate open (FU-47 §4) */
}

int fifa96_mixer_start(struct fifa96_mixer *m, int voice,
                       const struct fifa96_eacs_info *info,
                       const uint8_t *payload, size_t payload_len,
                       uint8_t volume, uint64_t step) {
  if (!m || !info || !payload) return -(int)FIFA96_ERR_TRUNCATED;
  if (voice < 0 || voice >= FIFA96_MIXER_VOICES) return -(int)FIFA96_ERR_TRUNCATED;
  if (info->format == FIFA96_EACS_FMT_UNKNOWN) return -(int)FIFA96_ERR_UNSUPPORTED;
  if (info->block_size == 0) return -(int)FIFA96_ERR_TRUNCATED;
  if (info->data_off > payload_len) return -(int)FIFA96_ERR_TRUNCATED;
  if (info->data_len > payload_len - info->data_off) return -(int)FIFA96_ERR_TRUNCATED;

  int is_delta = info->f10 == 2;
  const uint8_t *data = payload + info->data_off;
  uint32_t units = info->blocks;
  struct fifa96_eacs_delta dst = {0, 0, 0, 0};
  const uint8_t *dhdr = NULL;
  uint32_t dcount = 0;
  uint8_t f9 = 0;
  if (is_delta) {
    if (info->format == FIFA96_EACS_FMT_DELTA_STEREO) {
      if (info->voice == -1) {
        /* FU-43 §2 bank stereo: voice -1 removes the arm's 0x10 flag, so there
         * is no 20-byte block header; +0x0C declares one packed (L,R) byte per
         * frame at data_off from zero state (unsigned producer 0xB8610). */
        if ((uint64_t)info->delta_units > (uint64_t)(payload_len - info->data_off))
          return -(int)FIFA96_ERR_TRUNCATED;
        units = info->delta_units;
        dcount = units;
      } else {
        /* FU-39 §2.1 signed/video producer 0xB84FE: the 20-byte block header at
         * data_off carries count + row/acc state, data follows at +0x14. The
         * arm's u32[data]-1 truncation (FU-39 §4.3) is a runtime streaming
         * detail and is not modelled by this single-payload port. */
        if (payload_len - info->data_off < 0x14u) return -(int)FIFA96_ERR_TRUNCATED;
        int rc = fifa96_eacs_delta_header(payload + info->data_off, 0x14u, &dst, &dcount);
        if (rc != FIFA96_OK) return rc;
        if ((size_t)dcount > payload_len - info->data_off - 0x14u)
          return -(int)FIFA96_ERR_TRUNCATED;
        data = payload + info->data_off + 0x14u;
        units = dcount;
        dhdr = payload + info->data_off;
      }
    } else if (info->format == FIFA96_EACS_FMT_DELTA_MONO) {
      /* FU-39 §2.2 unsigned/bank producer 0xB8610: no block header; the
       * declared nibble count starts at data_off with zero state. The bank
       * loader's in-memory data mapping stays FU-39 §7 leg 3. */
      if ((uint64_t)info->delta_units > (uint64_t)(payload_len - info->data_off) * 2u)
        return -(int)FIFA96_ERR_TRUNCATED;
      units = info->delta_units;
      dcount = units;
    } else {
      return -(int)FIFA96_ERR_UNSUPPORTED;
    }
    f9 = info->format == FIFA96_EACS_FMT_DELTA_STEREO ? 2u : 1u;
  } else if ((uint64_t)info->blocks * info->block_size > info->data_len) {
    return -(int)FIFA96_ERR_TRUNCATED;
  }

  struct fifa96_mixer_voice *ch = &m->voices[voice];
  ch->active = 1;
  ch->format = info->format;
  ch->data = data;
  ch->units = units;
  ch->volume = volume;
  ch->gain_l = volume;
  ch->gain_r = volume;
  ch->pos = 0;
  ch->step = step;
  ch->loop = 0;
  ch->loop_start = 0;
  ch->loop_end = 0;
  ch->delta = is_delta;
  ch->f9 = f9;
  ch->delta_hdr = dhdr;
  ch->delta_units = dcount;
  ch->delta_pos = 0;
  ch->delta_state = dst;
  ch->delta_l = 0;
  ch->delta_r = 0;
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

/* FUN_000a62fa @ 0xA62FA, FU-47 §1. Both phases walk the 16 voices from the
 * rotor `next_voice` with wrap (0xA6329..0xA632E), testing bit v of the mask
 * with `SHL EDX,CL; TEST EDX,ESI` (0xA6309..0xA6314). */
int fifa96_mixer_alloc_voice(struct fifa96_mixer *m, uint32_t mask,
                             uint8_t priority) {
  if (!m) return -(int)FIFA96_ERR_TRUNCATED;

  int i = m->next_voice;
  for (int n = 0; n < FIFA96_MIXER_VOICES; n++) {
    if (((mask >> i) & 1u) != 0 && !m->voices[i].active) {
      m->next_voice = (i + 1) & (FIFA96_MIXER_VOICES - 1);
      return i;
    }
    i = (i + 1) & (FIFA96_MIXER_VOICES - 1);
  }

  /* Phase 2: phase 1 consumed every free in-mask voice, so whatever remains
   * is active; no +0x16 test here, exactly like the original. The stored
   * priority compare is unsigned (0xA6367 MOVZX, 0xA636D JNC). */
  i = m->next_voice;
  for (int n = 0; n < FIFA96_MIXER_VOICES; n++) {
    if (((mask >> i) & 1u) != 0 && m->voices[i].priority <= priority) {
      m->next_voice = (i + 1) & (FIFA96_MIXER_VOICES - 1);
      return i;
    }
    i = (i + 1) & (FIFA96_MIXER_VOICES - 1);
  }
  return -1;
}

int fifa96_mixer_set_priority(struct fifa96_mixer *m, int voice,
                              uint8_t priority) {
  if (!m || voice < 0 || voice >= FIFA96_MIXER_VOICES)
    return -(int)FIFA96_ERR_TRUNCATED;
  m->voices[voice].priority = priority;
  return FIFA96_OK;
}

int fifa96_mixer_set_sound_state(struct fifa96_mixer *m, int state) {
  if (!m || state < 0 || state > 5) return -(int)FIFA96_ERR_TRUNCATED;
  m->sound_state = state;
  return FIFA96_OK;
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
      if (ch->format == FIFA96_EACS_FMT_DELTA_STEREO ||
          ch->format == FIFA96_EACS_FMT_DELTA_MONO) {
        /* FU-39 §3/§4.2: decode lazily up to the cursor unit; the state chain
         * lives in the voice and is re-initialized from the stored block
         * header on a backwards cursor (loop wrap). The bank mono arm has no
         * header and wraps with zero state (FU-39 §2.2). */
        if (unit < ch->delta_pos) {
          if (ch->delta_hdr) {
            uint32_t cnt = 0;
            fifa96_eacs_delta_header(ch->delta_hdr, 0x14u, &ch->delta_state, &cnt);
          } else {
            ch->delta_state.l_row = 0;
            ch->delta_state.r_row = 0;
            ch->delta_state.l_acc = 0;
            ch->delta_state.r_acc = 0;
          }
          ch->delta_pos = 0;
        }
        while (ch->delta_pos <= unit) {
          fifa96_eacs_delta_unit(&ch->delta_state, ch->f9, ch->data,
                                 ch->delta_pos, &ch->delta_l, &ch->delta_r);
          ch->delta_pos++;
        }
        acc_l += sar7((int32_t)ch->delta_l * (int32_t)ch->gain_l);
        acc_r += sar7((int32_t)ch->delta_r * (int32_t)ch->gain_r);
      } else if (ch->format == FIFA96_EACS_FMT_PCM16_STEREO) {
        const uint8_t *p = ch->data + (size_t)unit * 4;
        acc_l += sar7((int32_t)(int16_t)rd16le(p) * (int32_t)ch->gain_l);
        acc_r += sar7((int32_t)(int16_t)rd16le(p + 2) * (int32_t)ch->gain_r);
      } else if (ch->format == FIFA96_EACS_FMT_PCM8_STEREO) {
        /* FU-37 §A.3 volume table: 2*vol*signed8(byte), row = gain<<10. */
        const uint8_t *p = ch->data + (size_t)unit * 2;
        acc_l += 2 * (int32_t)ch->gain_l * (int32_t)(int8_t)p[0];
        acc_r += 2 * (int32_t)ch->gain_r * (int32_t)(int8_t)p[1];
      } else if (ch->format == FIFA96_EACS_FMT_PCM16_MONO) {
        int32_t s = (int16_t)rd16le(ch->data + (size_t)unit * 2);
        acc_l += sar7(s * (int32_t)ch->gain_l);
        acc_r += sar7(s * (int32_t)ch->gain_r);
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

int fifa96_mixer_pitch_ratio(int32_t pitch, uint32_t *ratio, uint32_t *shift) {
  if (!ratio || !shift) return -(int)FIFA96_ERR_TRUNCATED;
  /* 0xB86C9 ADD EBX,0x2000; 0xB86CF/0xB86D4 init 0x40D0/9; 0xB86D6..E2 walk
   * down by 0x4B0 while (signed) E < W; 0xB86E4 SUB EBX,EDX. */
  int32_t e = (int32_t)((uint32_t)pitch + 0x2000u);
  int32_t w = 0x40d0;
  uint32_t cl = 9;
  while (e < w && cl < 32) {
    w -= 0x4b0;
    cl++;
  }
  int64_t idx = (int64_t)e - (int64_t)w;
  if (cl > 31 || idx < 0 || idx >= FIFA96_MIXER_PITCH_ENTRIES)
    return -(int)FIFA96_ERR_TRUNCATED;
  *ratio = fifa96_mixer_pitch_table[idx];
  *shift = cl;
  return FIFA96_OK;
}

int fifa96_mixer_step_from_rate(uint32_t rate, uint32_t out_rate,
                                uint32_t ratio, uint32_t shift,
                                uint64_t *step) {
  /* SHRD reads CL's low 5 bits; the table head only ever produces 9..31 and
   * FU-46 §1.3 documents the reject. */
  if (!step || out_rate == 0 || shift > 31) return -(int)FIFA96_ERR_TRUNCATED;
  /* 0xB86ED MUL EDX / 0xB86EF SHRD EAX,EDX,CL: low 32 bits of the 64-bit
   * rate*ratio shifted right by CL. */
  uint32_t v = (uint32_t)(((uint64_t)rate * ratio) >> shift);
  /* 0xB86F9 DIV [0x406A0]: the unmasked first quotient. 0xB870E..0xB8722
   * multiplies the masked step_int by out_rate and DIVs rem*0x1000000; the
   * DIV's high word is rem>>8, which is >= out_rate exactly when q >= 256
   * (rem = 256*floor(q/256)*out_rate + r), so the original raises #DE for
   * every such input. The port rejects instead of inventing a masked step
   * (FU-48 §3; q < 256 cannot fault). */
  uint32_t q = v / out_rate;
  if (q >= 0x100u) return -(int)FIFA96_ERR_TRUNCATED;
  /* 0xB86FF AND EAX,0xFF is a no-op for q < 256. */
  uint32_t int_part = q & 0xFFu;
  /* 0xB8711 MUL out_rate / 0xB8717 SUB / 0xB871B MUL 0x1000000 / 0xB8722 DIV
   * / 0xB8728 SHL 8: the 8.24 fraction scaled into the 32.32 low word. */
  uint32_t rem = v - int_part * out_rate;
  uint32_t frac =
      (uint32_t)(((((uint64_t)rem << 24) / out_rate) << 8) & 0xFFFFFFFFu);
  *step = ((uint64_t)int_part << 32) | frac;
  return FIFA96_OK;
}

int fifa96_mixer_step_from_pitch(uint32_t rate, uint32_t out_rate,
                                 int32_t pitch, uint64_t *step) {
  uint32_t ratio = 0, shift = 0;
  int rc = fifa96_mixer_pitch_ratio(pitch, &ratio, &shift);
  if (rc != FIFA96_OK) return rc;
  return fifa96_mixer_step_from_rate(rate, out_rate, ratio, shift, step);
}

/* 0xA6681 MOVSX ESI,byte [ESI+0x26]; 0xA6685/0xA6695 IMUL; 0xA6688 XOR EDX;
 * 0xA668F DIV 0x7F: the 32-bit product is read unsigned, quotient kept whole.
 * A valid 0..0x7F gain stays in 0..0x7F; a >0x7F byte is negative after the
 * MOVSX, so its unsigned quotient is large (FU-48 §4). */
static uint32_t pan_scale_wide(uint32_t factor, uint8_t gain) {
  uint32_t prod = (uint32_t)((int32_t)factor * (int32_t)(int8_t)gain);
  return prod / 0x7Fu;
}

void fifa96_mixer_pan_gains_wide(uint8_t pan, uint8_t gain,
                                 uint32_t *left, uint32_t *right) {
  if (!left || !right) return;
  /* 0xA663A MOVZX EAX,[ESI+0x27]; 0xA663E..0xA664A mirror > 0x7F. */
  uint32_t p = pan > 0x7F ? (uint32_t)(0xFF - pan) : pan;
  uint32_t lf, rf;
  if (p < 0x40) {
    /* 0xA6651..0xA6659: EBX 0x7F, ECX 2p. */
    lf = 0x7F;
    rf = p * 2;
  } else if (p == 0x40) {
    /* 0xA6678: EBX 0x7F / ECX = EBX (centre). */
    lf = 0x7F;
    rf = 0x7F;
  } else {
    /* 0xA665D..0xA666D: (0x7F-p)*0x7E/0x3E (IDIV, truncating). */
    lf = ((0x7F - p) * 0x7E) / 0x3E;
    rf = 0x7F;
  }
  uint32_t ql = pan_scale_wide(lf, gain);
  uint32_t qr = pan_scale_wide(rf, gain);
  /* 0xA66A1 SHL EAX,0x10 / 0xA66A4 OR EAX,EBX; consumer 0xA6601 SHR EDI,0x10
   * and 0xA6605 AND ESI,0x7F. packed = (qr<<16)|ql, so the reader sees
   * L = ql & 0x7F and R = (packed >> 16) = qr's low 16 | ql's bits 16..31. */
  uint32_t packed = ((qr << 16) | ql);
  *left = packed & 0x7Fu;
  *right = (packed >> 16) & 0xFFFFu;
}

void fifa96_mixer_pan_gains(uint8_t pan, uint8_t gain,
                            uint8_t *left, uint8_t *right) {
  uint32_t l = 0, r = 0;
  fifa96_mixer_pan_gains_wide(pan, gain, &l, &r);
  if (left) *left = (uint8_t)l;
  if (right) *right = (uint8_t)r;
}

int fifa96_mixer_set_pan(struct fifa96_mixer *m, int voice, uint8_t pan) {
  if (!m || voice < 0 || voice >= FIFA96_MIXER_VOICES)
    return -(int)FIFA96_ERR_TRUNCATED;
  struct fifa96_mixer_voice *ch = &m->voices[voice];
  if (!ch->active) return -(int)FIFA96_ERR_TRUNCATED;
  fifa96_mixer_pan_gains_wide(pan, ch->volume, &ch->gain_l, &ch->gain_r);
  return FIFA96_OK;
}
