#include "fifa96_loader/fifa96_sfx.h"
#include <stdint.h>
#include <string.h>

static uint32_t sfx_u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ── FUN_000cbc4c RNG and its seeders (FU-47 §2) ─────────────────────────── */

/* Image 0x112E68 = FUN_000cbcb8(0). */
static const uint32_t sfx_rng_seed0[6] = {
  0xF22D0E56u, 0x883126E9u, 0xC624DD2Fu, 0x0702C49Cu, 0x9E353F7Du, 0x6FDF3B64u
};

/* FUN_000cbcb8 @ 0xCBCB8: cumulative constants (0xCBCC3..0xCBCF5). */
static const uint32_t sfx_rng_seed_c[6] = {
  0xF22D0E56u, 0x96041893u, 0x3DF3B646u, 0x40DDE76Du, 0x97327AE1u, 0xD1A9FBE7u
};

/* 0x4C698: the six bytes read by MOVSX from image 0x101D98. */
static const int8_t sfx_rng_key[6] = {'A', 'r', 'C', 'a', 'D', 'e'};

void fifa96_sfx_rng_init(struct fifa96_sfx_rng *rng) {
  if (!rng) return;
  memcpy(rng->w, sfx_rng_seed0, sizeof sfx_rng_seed0);
}

void fifa96_sfx_rng_seed(struct fifa96_sfx_rng *rng, uint32_t seed) {
  if (!rng) return;
  uint32_t e = seed;
  for (int i = 0; i < 6; i++) {
    e += sfx_rng_seed_c[i];
    rng->w[i] = e;
  }
}

void fifa96_sfx_rng_seed_arcade(struct fifa96_sfx_rng *rng, uint32_t seed) {
  if (!rng) return;
  for (int i = 0; i < 6; i++)
    rng->w[i] = (seed << 25) + (uint32_t)sfx_rng_key[i];
}

uint32_t fifa96_sfx_rng_next(struct fifa96_sfx_rng *rng) {
  if (!rng) return 0;
  /* 0xCBC4C..0xCBC83: ADD EAX,[0x12E7C]; then ADC EAX,[w] for
   * w = 0x12E78, 0x12E74, 0x12E70, 0x12E6C, 0x12E68, storing each running
   * sum back into w. EAX ends as the new w[0], the returned word. */
  uint32_t eax = rng->w[5];
  uint32_t carry = 0;
  for (int i = 4; i >= 0; i--) {
    uint64_t s = (uint64_t)eax + rng->w[i] + carry;
    rng->w[i] = (uint32_t)s;
    eax = (uint32_t)s;
    carry = (uint32_t)(s >> 32);
  }
  uint32_t ret = eax;
  /* 0xCBC88: INC w[5]; if it wraps, INC w[4]..w[0] until one does not
   * (0xCBC90..0xCBCB0); if all wrap, INC EAX (0xCBCB6). The last INC keeps
   * EAX unchanged, hence `ret` mirrors the original's return register. */
  if (++rng->w[5] == 0) {
    int i = 4;
    for (; i >= 0; i--) {
      if (++rng->w[i] != 0) break;
    }
    if (i < 0) ret++;
  }
  return ret;
}

uint32_t fifa96_sfx_rng_default(void *ctx) {
  return fifa96_sfx_rng_next((struct fifa96_sfx_rng *)ctx);
}

/* One randomization draw: FUN_000cbc4c's return, top 16 bits (SHR EAX,0x10 at
 * 0xA78B6/0xA78E6/0xA793D). The derived generator is the default provider
 * (fifa96_sfx_rng_default); the hook stays injectable for tests. */
static int sfx_draw16(const struct fifa96_sfx_opts *o, uint32_t *r16) {
  if (!o->rand) return -(int)FIFA96_ERR_UNSUPPORTED;
  *r16 = o->rand(o->rand_ctx) >> 16;
  return FIFA96_OK;
}

/* record+0x25: descriptor +0x19 base plus the +0x1A span
 * (0xA78D4..0xA7922): MOVSX span, IMUL rand, SHR 15, SUB span, ADD base,
 * clamp to 0..0x7F. All 32-bit arithmetic, so the negative (s8) span walks
 * the same logical-shift path as the original. */
static int sfx_desc_volume(const uint8_t *d, const struct fifa96_sfx_opts *o,
                           uint8_t *out) {
  if (d[0x1A] == 0) { *out = d[0x19]; return FIFA96_OK; }
  uint32_t r;
  int rc = sfx_draw16(o, &r);
  if (rc != FIFA96_OK) return rc;
  int32_t span = (int8_t)d[0x1A];
  uint32_t prod = r * (uint32_t)span;
  uint32_t v = (prod >> 15) - (uint32_t)span + (uint32_t)(int8_t)d[0x19];
  int32_t sv = (int32_t)v;
  if (sv > 0x7F) sv = 0x7F;
  if (sv < 0) sv = 0;
  *out = (uint8_t)sv;
  return FIFA96_OK;
}

/* record+0xC: pitch base +0x10 plus the +0x0C numerator randomization
 * (0xA789D..0xA78C8; record+0x04 takes the +0x08 range, unused here). The
 * condition is +0x10 != 0 or +0x0C != 0, else the record pitch is zeroed. */
static int sfx_desc_pitch(const uint8_t *d, const struct fifa96_sfx_opts *o,
                          int32_t *out) {
  uint32_t base = sfx_u32le(d + 0x10);
  uint32_t num = sfx_u32le(d + 0x0C);
  if (base == 0 && num == 0) { *out = 0; return FIFA96_OK; }
  uint32_t r;
  int rc = sfx_draw16(o, &r);
  if (rc != FIFA96_OK) return rc;
  uint32_t prod = r * num;
  *out = (int32_t)(base + (prod >> 15) - num);
  return FIFA96_OK;
}

/* record+0x27 (0xA7925..0xA796F): caller pan wins; else the +0x18 fallback;
 * else the +0x1D span path. That path draws once for the bounds check and,
 * when the value stays <= 0x7F, draws again (FUN_000a79c1, 0xA795D) and
 * stores the second value unchecked as a byte. */
static int sfx_desc_pan(const uint8_t *d, int32_t caller_pan,
                        const struct fifa96_sfx_opts *o, uint8_t *out) {
  if (caller_pan != -1) { *out = (uint8_t)caller_pan; return FIFA96_OK; }
  if (d[0x1D] == 0) { *out = d[0x18]; return FIFA96_OK; }
  uint32_t span = d[0x1D];    /* MOVZX: unsigned */
  uint32_t r;
  int rc = sfx_draw16(o, &r);
  if (rc != FIFA96_OK) return rc;
  int32_t p = 0x40 + (int32_t)((r * span) >> 15) - (int32_t)span;
  if (p > 0x7F) { *out = 0x7F; return FIFA96_OK; }
  rc = sfx_draw16(o, &r);
  if (rc != FIFA96_OK) return rc;
  p = 0x40 + (int32_t)((r * span) >> 15) - (int32_t)span;
  *out = (uint8_t)p;
  return FIFA96_OK;
}

/* record+0x26 (FUN_000b9fdd @ 0xB9FDD): (s8)volume * (s8)caller *
 * (s8)master, CDQ; IDIV 0x3F01, `MOV [EBX+0x26],AL` (0xBA007). No clamp: a
 * negative product stores the low byte (0x80..0xFF), which is the negative
 * gain byte FUN_000a662c MOVSXes (FU-48 §4). Retail 0..0x7F operands keep
 * the quotient in 0..0x7F. */
static uint8_t sfx_gain(uint8_t volume, uint8_t caller, uint8_t master) {
  int32_t g = ((int32_t)(int8_t)volume * (int32_t)(int8_t)caller *
               (int32_t)(int8_t)master) / 0x3F01;
  return (uint8_t)g;
}

/* FUN_000a6717 @ 0xA6717: split a caller pan/volume into the two voices'
 * volumes and the pan both voices use. The return is
 * local<<16 | left<<8 | right (0xA67E0..0xA67E8) and FUN_000a7728 unpacks
 * eax&0xFF for id+1 and (eax>>8)&0xFF for id with AND 0xFF (0xA777C/0xA7782/
 * 0xA7796), so each split volume is a raw byte, not a 0..0x7F value. The
 * table is DAT_000148e0 (image 0x1148E0); the invalid-pan branch reads ESI
 * left by the caller, which FUN_000a7728 leaves as the id (0xA772E ->
 * 0xA67B0), so `id` seeds its table lookup. id < 5 with the default pan -1
 * scales left by table[id]/100 and can exceed 0x7F (id 0, volume 0x7F ->
 * 150*127/100 = 190 = 0xBE): the arm stores the byte and FUN_000b9fdd
 * MOVSXes it, the signed read-back FU-48 §2 ports. */
static void sfx_split(int32_t pan, uint8_t volume, uint32_t id,
                      uint8_t *pan_out, uint8_t *left, uint8_t *right) {
  static const uint32_t table[5] = {150, 140, 130, 120, 110};
  uint32_t esi, local, l, r;
  if (pan < 0 || pan >= 0x100) {
    local = 0x40; l = volume; r = 0; esi = id;
  } else if (pan >= 0x80) {
    uint32_t t = 0xFF - (uint32_t)pan;
    local = t; esi = t;
    if (t > 0x7A) esi = 0x7F - t;
    r = volume;
    if (esi < 5) r = (esi + 5) * volume / 10;
    l = volume - r;
  } else {
    local = (uint32_t)pan; esi = (uint32_t)pan;
    if (esi > 0x7A) esi = 0x7F - esi;
    l = volume;
    if (esi < 5) l = (esi + 5) * volume / 10;
    r = volume - l;
  }
  if (esi < 5) {
    l = table[esi] * l / 100;
    r = table[esi] * r / 100;
  }
  /* The original packs each 32-bit value with SHL 8/16 + OR; the callers'
   * AND 0xFF keeps the low byte (FU-48 §2). */
  *pan_out = (uint8_t)local;
  *left = (uint8_t)l;
  *right = (uint8_t)r;
}

struct sfx_resolved {
  uint8_t volume;  /* record+0x25 */
  uint8_t pan;     /* record+0x27 */
  uint8_t caller;  /* record+0x24 */
  uint8_t gain;    /* record+0x26 */
  int32_t pitch;   /* record+0xC */
};

static int sfx_resolve(const struct fifa96_mixer *m,
                       const struct fifa96_bnk_info *bank, uint32_t id,
                       int32_t caller_pan, uint8_t caller_volume,
                       const struct fifa96_sfx_opts *o,
                       struct fifa96_bnk_entry *e, struct fifa96_eacs_info *ei,
                       struct sfx_resolved *p) {
  fifa96_err_t rc = fifa96_bnk_entry(bank, id, e);
  if (rc != FIFA96_OK) return -(int)rc;
  /* The original checks the EACS tag (0xA7840) before it fills the record,
   * so a bad magic consumes no randomization draw. */
  int prc = fifa96_eacs_parse_bank(bank->src, bank->src_len, e->eacs_off,
                                   e->payload_len, ei);
  if (prc != FIFA96_OK) return prc;
  /* FU-47 §3 sound-state gate [0x15FC8] (0xA7852..0xA7869): signed 1..5 only,
   * original -4, checked before the first draw at 0xA789D. */
  if (m->sound_state < 1 || m->sound_state > 5)
    return -(int)FIFA96_ERR_TRUNCATED;
  const uint8_t *d = bank->src + e->desc_off;
  int rc2 = sfx_desc_pitch(d, o, &p->pitch);
  if (rc2 != FIFA96_OK) return rc2;
  rc2 = sfx_desc_volume(d, o, &p->volume);
  if (rc2 != FIFA96_OK) return rc2;
  rc2 = sfx_desc_pan(d, caller_pan, o, &p->pan);
  if (rc2 != FIFA96_OK) return rc2;
  p->caller = caller_volume;
  p->gain = sfx_gain(p->volume, caller_volume, o->master);
  return FIFA96_OK;
}

static int sfx_start_one(struct fifa96_mixer *m, int voice,
                         const struct fifa96_bnk_info *bank,
                         const struct fifa96_bnk_entry *e,
                         const struct fifa96_eacs_info *ei,
                         const struct sfx_resolved *p,
                         struct fifa96_sfx_voice *out) {
  uint64_t step = 0;
  /* FU-46 §1: FUN_000b86C8 head+tail over the record+0xC pitch (retail 0 ->
   * table[0]/0x10000 shift 16, exactly the old retail constant). */
  if (fifa96_mixer_step_from_pitch(ei->rate, FIFA96_SFX_OUT_RATE, p->pitch,
                                   &step) != FIFA96_OK)
    return -(int)FIFA96_ERR_TRUNCATED;
  int rc = fifa96_mixer_start(m, voice, ei, bank->src, bank->src_len,
                              p->gain, step);
  if (rc != FIFA96_OK) return rc;
  /* FU-46 §2: FUN_000a6579 converts record+0x27 pan and record+0x26 gain
   * through FUN_000a662c before the arm; centre pan leaves both reader gains
   * at the combined gain. */
  rc = fifa96_mixer_set_pan(m, voice, p->pan);
  if (rc != FIFA96_OK) {
    fifa96_mixer_stop(m, voice);
    return rc;
  }
  if (out) {
    const struct fifa96_mixer_voice *ch = &m->voices[voice];
    out->id = e->id;
    out->voice = voice;
    out->volume = p->volume;
    out->pan = p->pan;
    out->caller = p->caller;
    out->gain = p->gain;
    out->pitch = p->pitch;
    out->loop = ch->loop;
    out->loop_start = ch->loop_start;
    out->loop_end = ch->loop_end;
  }
  return voice;
}

/* FUN_000a780e one-shot with the FUN_000a62fa allocator: allocate from the
 * descriptor's +0x00 mask and +0x14 priority (0xA781F..0xA7825), then resolve
 * and start on the chosen voice, storing the priority after the start
 * (0xA788A). Returns the voice index or a negated fifa96_err_t. */
static int sfx_arm_alloc_one(struct fifa96_mixer *m,
                             const struct fifa96_bnk_info *bank, uint32_t id,
                             int32_t caller_pan, uint8_t caller_volume,
                             const struct fifa96_sfx_opts *opts,
                             struct fifa96_sfx_voice *out) {
  struct fifa96_bnk_entry e;
  fifa96_err_t erc = fifa96_bnk_entry(bank, id, &e);
  if (erc != FIFA96_OK) return -(int)erc;
  const uint8_t *d = bank->src + e.desc_off;
  int v = fifa96_mixer_alloc_voice(m, sfx_u32le(d), d[0x14]);
  if (v < 0) return -(int)FIFA96_ERR_NO_VOICE;
  struct fifa96_eacs_info ei;
  struct sfx_resolved p;
  int rc = sfx_resolve(m, bank, id, caller_pan, caller_volume, opts, &e, &ei,
                       &p);
  if (rc != FIFA96_OK) return rc;
  rc = sfx_start_one(m, v, bank, &e, &ei, &p, out);
  if (rc < 0) return rc;
  (void)fifa96_mixer_set_priority(m, v, d[0x14]);
  return v;
}

int fifa96_sfx_arm(struct fifa96_mixer *m, int voice,
                   const struct fifa96_bnk_info *bank, uint32_t id,
                   const struct fifa96_sfx_opts *opts,
                   struct fifa96_sfx_voice out[2]) {
  /* FUN_000a76ea wrapper defaults. */
  static const struct fifa96_sfx_opts defaults = {-1, 0x7F, 0x7F, NULL, NULL};
  if (!opts) opts = &defaults;
  if (!m || !bank || !bank->src) return -(int)FIFA96_ERR_TRUNCATED;
  if (voice < 0 || voice >= FIFA96_MIXER_VOICES) return -(int)FIFA96_ERR_TRUNCATED;
  if (id >= FIFA96_BNK_IDS) return -(int)FIFA96_ERR_TRUNCATED;
  if (opts->volume > FIFA96_SFX_VOLUME_MAX || opts->master > FIFA96_SFX_VOLUME_MAX)
    return -(int)FIFA96_ERR_TRUNCATED;
  /* The wrappers pass a byte pan; FUN_000a6717 mirrors 0x80..0xFF. */
  if (opts->pan != -1 && (opts->pan < 0 || opts->pan > 0xFF))
    return -(int)FIFA96_ERR_TRUNCATED;

  struct fifa96_bnk_entry e0;
  fifa96_err_t erc = fifa96_bnk_entry(bank, id, &e0);
  if (erc != FIFA96_OK) return -(int)erc;

  if (((bank->src[e0.desc_off + 0x1C]) & 1u) == 0) {
    struct fifa96_eacs_info ei;
    struct sfx_resolved p;
    int rc = sfx_resolve(m, bank, id, opts->pan, opts->volume, opts, &e0, &ei, &p);
    if (rc != FIFA96_OK) return rc;
    rc = sfx_start_one(m, voice, bank, &e0, &ei, &p, out ? &out[0] : NULL);
    if (rc < 0) return rc;
    /* The arm stores the descriptor priority for later steals (0xA788A). */
    (void)fifa96_mixer_set_priority(m, voice, bank->src[e0.desc_off + 0x14]);
    return rc;
  }

  /* Two-voice path (FU-43 §3.2): the split pans/volumes feed the arms for id
   * and id+1; the original allocates a second record (FUN_000a62fa) while
   * this port uses voice+1. Resolve both before starting, so an error leaves
   * no voice armed. */
  if (voice + 1 >= FIFA96_MIXER_VOICES) return -(int)FIFA96_ERR_TRUNCATED;
  uint8_t span, lv, rv;
  sfx_split(opts->pan, opts->volume, id, &span, &lv, &rv);
  struct fifa96_bnk_entry e1;
  struct fifa96_eacs_info ei0, ei1;
  struct sfx_resolved p0, p1;
  int rc = sfx_resolve(m, bank, id, span, lv, opts, &e0, &ei0, &p0);
  if (rc != FIFA96_OK) return rc;
  rc = sfx_resolve(m, bank, id + 1, span, rv, opts, &e1, &ei1, &p1);
  if (rc != FIFA96_OK) return rc;
  rc = sfx_start_one(m, voice, bank, &e0, &ei0, &p0, out ? &out[0] : NULL);
  if (rc < 0) return rc;
  (void)fifa96_mixer_set_priority(m, voice, bank->src[e0.desc_off + 0x14]);
  rc = sfx_start_one(m, voice + 1, bank, &e1, &ei1, &p1, out ? &out[1] : NULL);
  if (rc < 0) {
    fifa96_mixer_stop(m, voice);  /* port atomicity; the original's cleanup
                                     passes the id, 0xA77E2 */
    return rc;
  }
  (void)fifa96_mixer_set_priority(m, voice + 1, bank->src[e1.desc_off + 0x14]);
  return ((voice + 1) << 16) | voice;
}

int fifa96_sfx_arm_alloc(struct fifa96_mixer *m,
                         const struct fifa96_bnk_info *bank, uint32_t id,
                         const struct fifa96_sfx_opts *opts,
                         struct fifa96_sfx_voice out[2]) {
  static const struct fifa96_sfx_opts defaults = {-1, 0x7F, 0x7F, NULL, NULL};
  if (!opts) opts = &defaults;
  if (!m || !bank || !bank->src) return -(int)FIFA96_ERR_TRUNCATED;
  if (id >= FIFA96_BNK_IDS) return -(int)FIFA96_ERR_TRUNCATED;
  if (opts->volume > FIFA96_SFX_VOLUME_MAX || opts->master > FIFA96_SFX_VOLUME_MAX)
    return -(int)FIFA96_ERR_TRUNCATED;
  if (opts->pan != -1 && (opts->pan < 0 || opts->pan > 0xFF))
    return -(int)FIFA96_ERR_TRUNCATED;

  struct fifa96_bnk_entry e0;
  fifa96_err_t erc = fifa96_bnk_entry(bank, id, &e0);
  if (erc != FIFA96_OK) return -(int)erc;
  const uint8_t *d0 = bank->src + e0.desc_off;

  /* FUN_000a780e 0xA781F..0xA7828 runs the allocator before the EACS tag
   * (0xA7840) and the gate (0xA7852), so a blocked arm still moves the rotor
   * and writes the chosen record's +0x12. */
  int v0 = fifa96_mixer_alloc_voice(m, sfx_u32le(d0), d0[0x14]);
  if (v0 < 0) return -(int)FIFA96_ERR_NO_VOICE;

  if ((d0[0x1C] & 1u) == 0)
    return sfx_arm_alloc_one(m, bank, id, opts->pan, opts->volume, opts,
                             out ? &out[0] : NULL);

  /* Two-voice: FUN_000a7728 arms id then id+1, each allocating from its own
   * descriptor (0xA77A3/0xA77D6). Resolve both before starting so an error
   * leaves no voice armed. */
  if (id + 1 >= FIFA96_BNK_IDS) return -(int)FIFA96_ERR_NOT_FOUND;
  uint8_t span, lv, rv;
  sfx_split(opts->pan, opts->volume, id, &span, &lv, &rv);
  struct fifa96_bnk_entry e1;
  erc = fifa96_bnk_entry(bank, id + 1, &e1);
  if (erc != FIFA96_OK) return -(int)erc;
  struct fifa96_eacs_info ei0, ei1;
  struct sfx_resolved p0, p1;
  int rc = sfx_resolve(m, bank, id, span, lv, opts, &e0, &ei0, &p0);
  if (rc != FIFA96_OK) return rc;
  rc = sfx_resolve(m, bank, id + 1, span, rv, opts, &e1, &ei1, &p1);
  if (rc != FIFA96_OK) return rc;
  const uint8_t *d1 = bank->src + e1.desc_off;
  int v1 = fifa96_mixer_alloc_voice(m, sfx_u32le(d1), d1[0x14]);
  if (v1 < 0) return -(int)FIFA96_ERR_NO_VOICE;
  rc = sfx_start_one(m, v0, bank, &e0, &ei0, &p0, out ? &out[0] : NULL);
  if (rc < 0) return rc;
  (void)fifa96_mixer_set_priority(m, v0, d0[0x14]);
  rc = sfx_start_one(m, v1, bank, &e1, &ei1, &p1, out ? &out[1] : NULL);
  if (rc < 0) {
    fifa96_mixer_stop(m, v0);
    return rc;
  }
  (void)fifa96_mixer_set_priority(m, v1, d1[0x14]);
  return (v1 << 16) | v0;
}

int fifa96_sfx_play_id(struct fifa96_mixer *m,
                       const struct fifa96_sfx_id_table *ids, int32_t id,
                       const struct fifa96_sfx_opts *opts,
                       struct fifa96_sfx_voice out[2]) {
  static const struct fifa96_sfx_opts defaults = {-1, 0x7F, 0x7F, NULL, NULL};
  if (!opts) opts = &defaults;
  if (!m || !ids) return -(int)FIFA96_ERR_TRUNCATED;
  /* 0xA7738..0xA7742: signed id bound, original -0x13. */
  if (id < 0 || id >= (int32_t)FIFA96_BNK_IDS)
    return -(int)FIFA96_ERR_NOT_FOUND;
  /* 0xA774D..0xA775D: DAT_00061c14[id] == 0 -> -0x13. */
  const struct fifa96_bnk_info *bank0 = ids->bank[id];
  if (!bank0) return -(int)FIFA96_ERR_NOT_FOUND;
  if (opts->volume > FIFA96_SFX_VOLUME_MAX ||
      opts->master > FIFA96_SFX_VOLUME_MAX)
    return -(int)FIFA96_ERR_TRUNCATED;
  if (opts->pan != -1 && (opts->pan < 0 || opts->pan > 0xFF))
    return -(int)FIFA96_ERR_TRUNCATED;

  struct fifa96_bnk_entry e0;
  if (fifa96_bnk_entry(bank0, (uint32_t)id, &e0) != FIFA96_OK)
    return -(int)FIFA96_ERR_NOT_FOUND;
  if ((bank0->src[e0.desc_off + 0x1C] & 1u) == 0)
    return sfx_arm_alloc_one(m, bank0, (uint32_t)id, opts->pan, opts->volume,
                             opts, out ? &out[0] : NULL);

  /* 0xA775F..0xA77F4: FUN_000a6717 split, arm id, require the next table
   * slot's descriptor ([0x61C18 + id*4] = table[id+1], 0xA77BA; possibly a
   * different bank), arm id+1. The original returns -0x13 without cleanup
   * when id+1 is absent (0xA77C4), so the first voice stays armed; a second
   * arm failure runs FUN_000a6cdc(id) (0xA77E2). */
  uint8_t span, lv, rv;
  sfx_split(opts->pan, opts->volume, (uint32_t)id, &span, &lv, &rv);
  int v0 = sfx_arm_alloc_one(m, bank0, (uint32_t)id, span, lv, opts,
                             out ? &out[0] : NULL);
  if (v0 < 0) return v0;
  if (id + 1 >= (int32_t)FIFA96_BNK_IDS)
    return -(int)FIFA96_ERR_NOT_FOUND;
  const struct fifa96_bnk_info *bank1 = ids->bank[id + 1];
  if (!bank1) return -(int)FIFA96_ERR_NOT_FOUND;
  int v1 = sfx_arm_alloc_one(m, bank1, (uint32_t)id + 1, span, rv, opts,
                             out ? &out[1] : NULL);
  if (v1 < 0) {
    fifa96_mixer_stop(m, v0);
    return v1;
  }
  return (v1 << 16) | v0;
}
