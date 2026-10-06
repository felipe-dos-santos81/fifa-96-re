#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_blit.h"
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_record.h"
#include "fifa96_loader/fifa96_sprite.h"
#include "fifa96_loader/fifa96_tgv_stream.h"
#include "fifa96_loader/fifa96_vgt_player.h"

#define PLAY_MAX_PIXELS ((uint64_t)4096u * 4096u)
#define PLAY_DEFAULT_DIR "build/play"
#define PLAY_TAG_KVGT 0x5447566Bu

typedef struct {
  uint32_t h[8];
  uint64_t bytes;
  uint8_t buf[64];
  size_t len;
} play_sha256;

static const uint32_t play_sha256_k[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u,
    0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu,
    0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu,
    0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
    0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u,
    0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u,
    0xc67178f2u,
};

static uint32_t play_rotr(uint32_t x, unsigned n) {
  return (x >> n) | (x << (32u - n));
}

static void play_sha256_block(play_sha256 *c, const uint8_t *p) {
  uint32_t w[64];
  for (int i = 0; i < 16; i++) {
    w[i] = ((uint32_t)p[i * 4] << 24) | ((uint32_t)p[i * 4 + 1] << 16) |
           ((uint32_t)p[i * 4 + 2] << 8) | (uint32_t)p[i * 4 + 3];
  }
  for (int i = 16; i < 64; i++) {
    uint32_t s0 = play_rotr(w[i - 15], 7) ^ play_rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = play_rotr(w[i - 2], 17) ^ play_rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  uint32_t a = c->h[0], b = c->h[1], cc = c->h[2], d = c->h[3];
  uint32_t e = c->h[4], f = c->h[5], g = c->h[6], h = c->h[7];
  for (int i = 0; i < 64; i++) {
    uint32_t s1 = play_rotr(e, 6) ^ play_rotr(e, 11) ^ play_rotr(e, 25);
    uint32_t ch = (e & f) ^ (~e & g);
    uint32_t t1 = h + s1 + ch + play_sha256_k[i] + w[i];
    uint32_t s0 = play_rotr(a, 2) ^ play_rotr(a, 13) ^ play_rotr(a, 22);
    uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
    uint32_t t2 = s0 + maj;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = cc;
    cc = b;
    b = a;
    a = t1 + t2;
  }
  c->h[0] += a;
  c->h[1] += b;
  c->h[2] += cc;
  c->h[3] += d;
  c->h[4] += e;
  c->h[5] += f;
  c->h[6] += g;
  c->h[7] += h;
}

static void play_sha256_init(play_sha256 *c) {
  static const uint32_t iv[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
  memcpy(c->h, iv, sizeof iv);
  c->bytes = 0;
  c->len = 0;
}

static void play_sha256_update(play_sha256 *c, const uint8_t *p, size_t n) {
  c->bytes += n;
  while (n) {
    size_t take = 64 - c->len;
    if (take > n) take = n;
    memcpy(c->buf + c->len, p, take);
    c->len += take;
    p += take;
    n -= take;
    if (c->len == 64) {
      play_sha256_block(c, c->buf);
      c->len = 0;
    }
  }
}

static void play_sha256_final(play_sha256 *c, uint8_t out[32]) {
  uint64_t bits = c->bytes * 8u;
  uint8_t pad[72];
  size_t padlen = (c->len < 56) ? 56 - c->len : 120 - c->len;
  memset(pad, 0, sizeof pad);
  pad[0] = 0x80;
  for (int i = 0; i < 8; i++) pad[padlen + (size_t)i] = (uint8_t)(bits >> (56 - 8 * i));
  play_sha256_update(c, pad, padlen + 8);
  for (int i = 0; i < 8; i++) {
    out[i * 4] = (uint8_t)(c->h[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(c->h[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(c->h[i] >> 8);
    out[i * 4 + 3] = (uint8_t)c->h[i];
  }
}

static void play_sha256_hex(const uint8_t *p, size_t n, char out[65]) {
  static const char hex[] = "0123456789abcdef";
  uint8_t d[32];
  play_sha256 c;
  play_sha256_init(&c);
  play_sha256_update(&c, p, n);
  play_sha256_final(&c, d);
  for (int i = 0; i < 32; i++) {
    out[i * 2] = hex[d[i] >> 4];
    out[i * 2 + 1] = hex[d[i] & 15];
  }
  out[64] = '\0';
}

static uint32_t play_le16(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t play_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t play_be24(const uint8_t *p) {
  return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
}

static void errmsg(const char *fmt, ...) {
  va_list ap;
  fputs("fifa96_play: error: ", stderr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

static void usage(FILE *fp) {
  fputs("usage: fifa96_play MODE [OPTIONS] [FILE]\n"
        "\n"
        "modes:\n"
        "  video FILE [--max-frames N] [--frame K] [--out PATH] [--modex] [--print-summary]\n"
        "  audio FILE [--out PATH] [--print-summary]\n"
        "  audio --bnk FILE --id N [--out PATH] [--print-summary]\n"
        "  audio --viv FILE (--name NAME | --index N) [--out PATH] [--print-summary]\n"
        "  sprite FILE [--entry N | --name NAME] [--frame K] [--out PATH] [--palette FILE]\n"
        "              [--print-summary]\n"
        "\n"
        "video decodes a raw .TGV chunk stream and writes each selected frame as a P6 PPM;\n"
        "--modex writes the raw 4-plane Mode-X image instead. --out is a directory for a\n"
        "sequence (default build/play) and a file for --frame K. --max-frames and --frame\n"
        "are exclusive. audio decodes an EACS payload, a .BNK entry or a BIGF .VIV entry\n"
        "to a 16-bit PCM RIFF WAV (default build/play/audio.wav). sprite decodes a BIGF\n"
        ".pvi sprite container (optionally codec-wrapped) to the SHPI bank in entry N or\n"
        "NAME (default entry 0), renders frame K (default 0) to a P6 PPM (grayscale, or\n"
        "RGB with a 768-byte --palette FILE; default build/play/sprite.ppm). --print-summary\n"
        "prints one geometry/format/sha256 line per written output.\n",
        fp);
}

static int arg_error(const char *fmt, ...) {
  va_list ap;
  fputs("fifa96_play: error: ", stderr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  fputs("try 'fifa96_play --help'\n", stderr);
  return 2;
}

static int parse_u32(const char *s, uint32_t *v) {
  if (!s || !*s || *s == '-') return 0;
  errno = 0;
  char *end = NULL;
  unsigned long x = strtoul(s, &end, 10);
  if (errno != 0 || !end || *end != '\0') return 0;
  if (x > 0xFFFFFFFFul) return 0;
  *v = (uint32_t)x;
  return 1;
}

static void *xmalloc(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p) {
    errmsg("out of memory");
    exit(1);
  }
  return p;
}

static int is_dir(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int ensure_dir(const char *path) {
  char tmp[4096];
  size_t n = strlen(path);
  if (n == 0 || n >= sizeof tmp) return -1;
  memcpy(tmp, path, n + 1);
  for (size_t i = 1; i < n; i++) {
    if (tmp[i] != '/') continue;
    tmp[i] = '\0';
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST) return -1;
    tmp[i] = '/';
  }
  if (mkdir(tmp, 0777) != 0 && errno != EEXIST) return -1;
  return 0;
}

static int ensure_parent(const char *path) {
  const char *slash = strrchr(path, '/');
  if (!slash || slash == path) return 0;
  char dir[4096];
  size_t n = (size_t)(slash - path);
  if (n == 0 || n >= sizeof dir) return -1;
  memcpy(dir, path, n);
  dir[n] = '\0';
  return ensure_dir(dir);
}

static int write_file(const char *path, const uint8_t *buf, size_t len) {
  FILE *fp = fopen(path, "wb");
  if (!fp) {
    errmsg("cannot open '%s' for writing: %s", path, strerror(errno));
    return 1;
  }
  if (len && fwrite(buf, 1, len, fp) != len) {
    errmsg("short write to '%s'", path);
    fclose(fp);
    return 1;
  }
  if (fclose(fp) != 0) {
    errmsg("cannot close '%s': %s", path, strerror(errno));
    return 1;
  }
  return 0;
}

static int scan_canvas(const uint8_t *data, size_t n, uint32_t *cap) {
  fifa96_tgv_walk walk;
  walk.data = data;
  walk.size = n;
  walk.pos = 0;
  uint64_t maxpix = 320u * 240u;
  for (;;) {
    const uint8_t *chunk = NULL;
    size_t len = 0;
    int is_frame = 0;
    int r = fifa96_tgv_walk_next(&walk, &chunk, &len, &is_frame);
    if (r < 0) return r;
    if (r == 0) break;
    if (len >= 0x14 && play_le32(chunk) == PLAY_TAG_KVGT) {
      uint64_t pix = (uint64_t)play_le16(chunk + 8) * (uint64_t)play_le16(chunk + 10);
      if (pix > maxpix) maxpix = pix;
    }
  }
  if (maxpix > PLAY_MAX_PIXELS) return -(int)FIFA96_ERR_UNSUPPORTED;
  *cap = (uint32_t)maxpix;
  return 0;
}

static int emit_video_frame(const fifa96_vgt_frame *f, uint32_t index, const char *out, int modex,
                            int have_frame, int summary) {
  const char *ext = modex ? "modex" : "ppm";
  char *path = NULL;
  if (have_frame && out && !is_dir(out)) {
    size_t n = strlen(out) + 1;
    path = xmalloc(n);
    memcpy(path, out, n);
    if (ensure_parent(path) != 0) {
      errmsg("cannot create parent directory of '%s'", path);
      free(path);
      return 1;
    }
  } else {
    const char *dir = out ? out : PLAY_DEFAULT_DIR;
    if (ensure_dir(dir) != 0) {
      errmsg("cannot create output directory '%s'", dir);
      return 1;
    }
    size_t n = strlen(dir) + 32;
    path = xmalloc(n);
    snprintf(path, n, "%s/frame-%04u.%s", dir, index, ext);
  }

  uint8_t *buf = NULL;
  size_t len = 0;
  if (modex) {
    size_t plane_cap = (size_t)f->height * FIFA96_MODEX_PLANE_STRIDE;
    if (!f->pixels || (uint64_t)f->width * f->height > f->pixels_len) {
      errmsg("frame %u: decoded %zu bytes for %ux%u", index, f->pixels_len, f->width, f->height);
      free(path);
      return 1;
    }
    len = plane_cap * FIFA96_MODEX_PLANES;
    buf = xmalloc(len);
    memset(buf, 0, len);
    fifa96_modex_image img;
    for (uint32_t p = 0; p < FIFA96_MODEX_PLANES; p++) img.planes[p] = buf + (size_t)p * plane_cap;
    img.plane_cap = plane_cap;
    fifa96_blit_clip clip = {0, 0, (int32_t)f->width, (int32_t)f->height};
    int r = fifa96_blit_modex(&img, 0, 0, f->pixels, f->pixels_len, f->width, f->height, &clip);
    if (r != FIFA96_OK) {
      errmsg("frame %u: modex blit failed (%d)", index, -r);
      free(buf);
      free(path);
      return 1;
    }
  } else {
    size_t pixels = (size_t)f->width * (size_t)f->height;
    if (!f->pixels || !f->palette || f->pixels_len < pixels) {
      errmsg("frame %u: decoded %zu bytes for %ux%u", index, f->pixels_len, f->width, f->height);
      free(path);
      return 1;
    }
    buf = xmalloc(64 + pixels * 3);
    int hn = snprintf((char *)buf, 64, "P6\n%u %u\n255\n", f->width, f->height);
    if (hn < 0 || hn >= 64) {
      errmsg("frame %u: cannot format PPM header", index);
      free(buf);
      free(path);
      return 1;
    }
    for (size_t i = 0; i < pixels; i++) {
      const uint8_t *rgb = f->palette + (size_t)f->pixels[i] * 3;
      buf[(size_t)hn + i * 3] = rgb[0];
      buf[(size_t)hn + i * 3 + 1] = rgb[1];
      buf[(size_t)hn + i * 3 + 2] = rgb[2];
    }
    len = (size_t)hn + pixels * 3;
  }

  int rc = write_file(path, buf, len);
  if (rc == 0 && summary) {
    char hex[65];
    play_sha256_hex(buf, len, hex);
    printf("frame %u: %ux%u %s sha256=%s\n", index, f->width, f->height, modex ? "modex" : "ppm",
           hex);
  }
  free(buf);
  free(path);
  return rc;
}

static int cmd_video(int argc, char **argv) {
  const char *in = NULL;
  const char *out = NULL;
  int modex = 0;
  int summary = 0;
  int have_frame = 0;
  int have_max = 0;
  uint32_t frame = 0;
  uint32_t max_frames = 0;
  for (int i = 2; i < argc; i++) {
    const char *a = argv[i];
    if (strcmp(a, "--modex") == 0) {
      modex = 1;
    } else if (strcmp(a, "--print-summary") == 0) {
      summary = 1;
    } else if (strcmp(a, "--frame") == 0) {
      if (++i >= argc || !parse_u32(argv[i], &frame))
        return arg_error("--frame needs a non-negative frame index");
      have_frame = 1;
    } else if (strcmp(a, "--max-frames") == 0) {
      if (++i >= argc || !parse_u32(argv[i], &max_frames) || max_frames == 0)
        return arg_error("--max-frames needs a count >= 1");
      have_max = 1;
    } else if (strcmp(a, "--out") == 0) {
      if (++i >= argc) return arg_error("--out needs a path");
      out = argv[i];
    } else if (a[0] == '-') {
      return arg_error("unknown option '%s'", a);
    } else if (!in) {
      in = a;
    } else {
      return arg_error("unexpected argument '%s'", a);
    }
  }
  if (!in) return arg_error("video needs an input FILE");
  if (have_frame && have_max) return arg_error("--frame and --max-frames are exclusive");

  uint8_t *data = NULL;
  size_t n = 0;
  fifa96_err_t fe = fifa96_file_read(in, &data, &n);
  if (fe != FIFA96_OK) {
    errmsg("cannot read '%s' (%d)", in, (int)fe);
    return 1;
  }
  uint32_t cap = 0;
  int sr = scan_canvas(data, n, &cap);
  if (sr < 0) {
    errmsg("malformed TGV stream '%s' (%d)", in, -sr);
    fifa96_file_free(data);
    return 1;
  }
  uint8_t *front = xmalloc(cap);
  uint8_t *back = xmalloc(cap);
  fifa96_vgt_player player;
  int r = fifa96_vgt_player_init(&player, 320, 240, front, cap, back, cap, NULL, NULL);
  if (r != FIFA96_OK) {
    errmsg("player init for '%s' failed (%d)", in, -r);
    free(front);
    free(back);
    fifa96_file_free(data);
    return 1;
  }
  fifa96_vgt_player_feed(&player, data, n);

  uint32_t seen = 0;
  uint32_t written = 0;
  int rc = 0;
  for (;;) {
    fifa96_vgt_frame f;
    r = fifa96_vgt_player_step(&player, &f);
    if (r < 0) {
      errmsg("malformed TGV stream '%s' at frame %u (%d)", in, seen, -r);
      rc = 1;
      break;
    }
    if (r == 0) break;
    if (have_frame) {
      if (seen == frame) {
        rc = emit_video_frame(&f, frame, out, modex, 1, summary);
        if (rc == 0) written = 1;
        break;
      }
    } else {
      if (have_max && written >= max_frames) break;
      rc = emit_video_frame(&f, written, out, modex, 0, summary);
      if (rc != 0) break;
      written++;
    }
    seen++;
  }
  if (rc == 0) {
    if (have_frame && !written)
      rc = (errmsg("frame %u not present (stream has %u frames)", frame, seen), 1);
    else if (!have_frame && written == 0)
      rc = (errmsg("no frames decoded from '%s'", in), 1);
  }
  free(front);
  free(back);
  fifa96_file_free(data);
  return rc;
}

struct play_pcm {
  int16_t *samples;
  uint32_t frames;
  uint32_t channels;
  uint32_t rate;
};

static int decode_eacs(const uint8_t *base, size_t base_len, const struct fifa96_eacs_info *info,
                       struct play_pcm *pcm) {
  if ((uint64_t)info->data_off + info->data_len > base_len) {
    errmsg("EACS data region crosses the source buffer");
    return 1;
  }
  const uint8_t *data = base + info->data_off;
  memset(pcm, 0, sizeof *pcm);
  pcm->rate = info->rate;
  if (info->format == FIFA96_EACS_FMT_PCM16_STEREO) {
    pcm->frames = info->blocks;
    pcm->channels = 2;
    pcm->samples = xmalloc((size_t)pcm->frames * 4);
    memcpy(pcm->samples, data, (size_t)pcm->frames * 4);
    return 0;
  }
  if (info->format == FIFA96_EACS_FMT_PCM16_MONO) {
    pcm->frames = info->blocks;
    pcm->channels = 1;
    pcm->samples = xmalloc((size_t)pcm->frames * 2);
    memcpy(pcm->samples, data, (size_t)pcm->frames * 2);
    return 0;
  }
  if (info->format == FIFA96_EACS_FMT_PCM8_STEREO) {
    pcm->frames = info->blocks;
    pcm->channels = 2;
    pcm->samples = xmalloc((size_t)pcm->frames * 4);
    for (uint32_t i = 0; i < pcm->frames; i++) {
      pcm->samples[i * 2] = (int16_t)((int16_t)(int8_t)data[i * 2] << 8);
      pcm->samples[i * 2 + 1] = (int16_t)((int16_t)(int8_t)data[i * 2 + 1] << 8);
    }
    return 0;
  }
  if (info->format == FIFA96_EACS_FMT_DELTA_STEREO) {
    struct fifa96_eacs_delta st;
    memset(&st, 0, sizeof st);
    uint32_t units = 0;
    const uint8_t *d = data;
    if (info->voice >= 0) {
      if (info->data_len < 0x14u) {
        errmsg("EACS delta block header is truncated");
        return 1;
      }
      int r = fifa96_eacs_delta_header(data, 0x14, &st, &units);
      if (r != FIFA96_OK) {
        errmsg("EACS delta block header is malformed (%d)", -r);
        return 1;
      }
      if ((uint64_t)units > (uint64_t)info->data_len - 0x14u) {
        errmsg("EACS delta block count exceeds its payload");
        return 1;
      }
      d = data + 0x14;
    } else {
      if ((uint64_t)info->delta_units > info->data_len) {
        errmsg("EACS delta frame count exceeds its payload");
        return 1;
      }
      units = info->delta_units;
    }
    pcm->frames = units;
    pcm->channels = 2;
    pcm->samples = xmalloc((size_t)units * 4);
    for (uint32_t u = 0; u < units; u++) {
      int16_t l = 0, r = 0;
      if (fifa96_eacs_delta_unit(&st, 2, d, u, &l, &r) != FIFA96_OK) {
        errmsg("EACS delta decode failed at frame %u", u);
        free(pcm->samples);
        return 1;
      }
      pcm->samples[u * 2] = l;
      pcm->samples[u * 2 + 1] = r;
    }
    return 0;
  }
  if (info->format == FIFA96_EACS_FMT_DELTA_MONO) {
    if ((uint64_t)info->delta_units > (uint64_t)info->data_len * 2u) {
      errmsg("EACS delta nibble count exceeds its payload");
      return 1;
    }
    uint32_t units = info->delta_units;
    struct fifa96_eacs_delta st;
    memset(&st, 0, sizeof st);
    pcm->frames = units;
    pcm->channels = 1;
    pcm->samples = xmalloc((size_t)units * 2);
    for (uint32_t u = 0; u < units; u++) {
      int16_t l = 0;
      if (fifa96_eacs_delta_unit(&st, 1, data, u, &l, NULL) != FIFA96_OK) {
        errmsg("EACS delta decode failed at frame %u", u);
        free(pcm->samples);
        return 1;
      }
      pcm->samples[u] = l;
    }
    return 0;
  }
  errmsg("unsupported EACS format (f8=%u f9=%u f10=%u)", info->f8, info->f9, info->f10);
  return 1;
}

static void wr_u16le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void wr_u32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static int write_wav(const char *path, const struct play_pcm *pcm, int summary) {
  size_t data_bytes = (size_t)pcm->frames * pcm->channels * 2;
  size_t len = 44 + data_bytes;
  uint8_t *buf = xmalloc(len);
  memcpy(buf, "RIFF", 4);
  wr_u32le(buf + 4, (uint32_t)(36 + data_bytes));
  memcpy(buf + 8, "WAVE", 4);
  memcpy(buf + 12, "fmt ", 4);
  wr_u32le(buf + 16, 16);
  wr_u16le(buf + 20, 1);
  wr_u16le(buf + 22, (uint16_t)pcm->channels);
  wr_u32le(buf + 24, pcm->rate);
  wr_u32le(buf + 28, pcm->rate * pcm->channels * 2u);
  wr_u16le(buf + 32, (uint16_t)(pcm->channels * 2u));
  wr_u16le(buf + 34, 16);
  memcpy(buf + 36, "data", 4);
  wr_u32le(buf + 40, (uint32_t)data_bytes);
  for (size_t i = 0; i < (size_t)pcm->frames * pcm->channels; i++)
    wr_u16le(buf + 44 + i * 2, (uint16_t)pcm->samples[i]);

  int rc = write_file(path, buf, len);
  if (rc == 0 && summary) {
    char hex[65];
    play_sha256_hex(buf, len, hex);
    printf("audio: frames=%u rate=%u channels=%u sha256=%s\n", pcm->frames, pcm->rate,
           pcm->channels, hex);
  }
  free(buf);
  return rc;
}

static int cmd_audio(int argc, char **argv) {
  const char *in = NULL;
  const char *bnk = NULL;
  const char *viv = NULL;
  const char *name = NULL;
  const char *out = NULL;
  int summary = 0;
  int have_id = 0, have_name = 0, have_index = 0;
  uint32_t id = 0, index = 0;
  for (int i = 2; i < argc; i++) {
    const char *a = argv[i];
    if (strcmp(a, "--bnk") == 0) {
      if (++i >= argc) return arg_error("--bnk needs a file");
      bnk = argv[i];
    } else if (strcmp(a, "--viv") == 0) {
      if (++i >= argc) return arg_error("--viv needs a file");
      viv = argv[i];
    } else if (strcmp(a, "--id") == 0) {
      if (++i >= argc || !parse_u32(argv[i], &id)) return arg_error("--id needs a number");
      have_id = 1;
    } else if (strcmp(a, "--name") == 0) {
      if (++i >= argc) return arg_error("--name needs a value");
      name = argv[i];
      have_name = 1;
    } else if (strcmp(a, "--index") == 0) {
      if (++i >= argc || !parse_u32(argv[i], &index)) return arg_error("--index needs a number");
      have_index = 1;
    } else if (strcmp(a, "--out") == 0) {
      if (++i >= argc) return arg_error("--out needs a path");
      out = argv[i];
    } else if (strcmp(a, "--print-summary") == 0) {
      summary = 1;
    } else if (a[0] == '-') {
      return arg_error("unknown option '%s'", a);
    } else if (!in) {
      in = a;
    } else {
      return arg_error("unexpected argument '%s'", a);
    }
  }
  if (bnk && viv) return arg_error("--bnk and --viv are exclusive");
  if (bnk && in) return arg_error("--bnk does not take a positional FILE");
  if (viv && in) return arg_error("--viv does not take a positional FILE");
  if (bnk && !have_id) return arg_error("--bnk needs --id N");
  if (bnk && (have_name || have_index)) return arg_error("--id is exclusive with --name/--index");
  if (viv && have_id) return arg_error("--viv does not take --id");
  if (viv && have_name == have_index) return arg_error("--viv needs exactly one of --name/--index");
  if (!bnk && !viv && !in) return arg_error("audio needs an input FILE or --bnk/--viv");
  if (!bnk && !viv && (have_id || have_name || have_index))
    return arg_error("--id/--name/--index need --bnk or --viv");

  uint8_t *data = NULL;
  size_t n = 0;
  const uint8_t *base = NULL;
  size_t base_len = 0;
  struct fifa96_eacs_info info;
  fifa96_err_t e;
  if (bnk) {
    e = fifa96_file_read(bnk, &data, &n);
    if (e != FIFA96_OK) {
      errmsg("cannot read '%s' (%d)", bnk, (int)e);
      return 1;
    }
    struct fifa96_bnk_info bank;
    e = fifa96_bnk_parse(data, n, &bank);
    if (e != FIFA96_OK) {
      errmsg("'%s' is not a valid .BNK (%d)", bnk, (int)e);
      fifa96_file_free(data);
      return 1;
    }
    struct fifa96_bnk_entry entry;
    e = fifa96_bnk_entry(&bank, id, &entry);
    if (e != FIFA96_OK) {
      errmsg("id %u is not present in '%s' (%d)", id, bnk, (int)e);
      fifa96_file_free(data);
      return 1;
    }
    e = fifa96_eacs_parse_bank(data, n, entry.eacs_off, entry.payload_len, &info);
    if (e != FIFA96_OK) {
      errmsg("id %u in '%s' has no decodable EACS header (%d)", id, bnk, (int)e);
      fifa96_file_free(data);
      return 1;
    }
    base = data;
    base_len = n;
  } else if (viv) {
    e = fifa96_file_read(viv, &data, &n);
    if (e != FIFA96_OK) {
      errmsg("cannot read '%s' (%d)", viv, (int)e);
      return 1;
    }
    struct fifa96_bigf_info bigf;
    e = fifa96_bigf_parse(data, n, &bigf);
    if (e != FIFA96_OK) {
      errmsg("'%s' is not a valid BIGF .VIV (%d)", viv, (int)e);
      fifa96_file_free(data);
      return 1;
    }
    uint32_t off = 0, size = 0;
    const char *rec_name = NULL;
    if (have_name) {
      int found = 0;
      for (size_t i = 0; i < bigf.count; i++) {
        if (fifa96_bigf_record(&bigf, i, &off, &size, &rec_name) != FIFA96_OK) break;
        if (strcmp(rec_name, name) == 0) {
          found = 1;
          break;
        }
      }
      if (!found) {
        errmsg("record '%s' is not present in '%s'", name, viv);
        fifa96_file_free(data);
        return 1;
      }
    } else {
      e = fifa96_bigf_record(&bigf, index, &off, &size, &rec_name);
      if (e != FIFA96_OK) {
        errmsg("index %u is not present in '%s' (%d)", index, viv, (int)e);
        fifa96_file_free(data);
        return 1;
      }
    }
    e = fifa96_eacs_parse(data + off, size, &info);
    if (e != FIFA96_OK) {
      errmsg("record '%s' in '%s' has no decodable EACS header (%d)",
             rec_name ? rec_name : "", viv, (int)e);
      fifa96_file_free(data);
      return 1;
    }
    base = data + off;
    base_len = size;
  } else {
    e = fifa96_file_read(in, &data, &n);
    if (e != FIFA96_OK) {
      errmsg("cannot read '%s' (%d)", in, (int)e);
      return 1;
    }
    e = fifa96_eacs_parse(data, n, &info);
    if (e != FIFA96_OK) {
      errmsg("'%s' has no decodable EACS header (%d)", in, (int)e);
      fifa96_file_free(data);
      return 1;
    }
    base = data;
    base_len = n;
  }

  struct play_pcm pcm;
  if (decode_eacs(base, base_len, &info, &pcm) != 0) {
    fifa96_file_free(data);
    return 1;
  }
  const char *path = out ? out : PLAY_DEFAULT_DIR "/audio.wav";
  if (ensure_parent(path) != 0) {
    errmsg("cannot create parent directory of '%s'", path);
    free(pcm.samples);
    fifa96_file_free(data);
    return 1;
  }
  int rc = write_wav(path, &pcm, summary);
  free(pcm.samples);
  fifa96_file_free(data);
  return rc;
}

static int sprite_leaf(const uint8_t *src, size_t n, uint8_t **owned, const uint8_t **out,
                       size_t *out_len) {
  uint8_t *buf = NULL;
  for (int depth = 0; depth < 8; depth++) {
    if (n >= 4 && (memcmp(src, "SHPI", 4) == 0 || memcmp(src, "BIGF", 4) == 0)) {
      *owned = buf;
      *out = src;
      *out_len = n;
      return 0;
    }
    if (n < 5 || src[1] != 0xFB) {
      free(buf);
      return -(int)FIFA96_ERR_BAD_MAGIC;
    }
    size_t cap = play_be24(src + 2);
    uint8_t *next = xmalloc(cap ? cap : 1);
    size_t len = 0;
    int r = fifa96_record_decode(src, n, next, cap, &len);
    if (r != 0) {
      free(next);
      free(buf);
      return r;
    }
    free(buf);
    buf = next;
    src = next;
    n = len;
  }
  free(buf);
  return -(int)FIFA96_ERR_UNSUPPORTED;
}

static int cmd_sprite(int argc, char **argv) {
  const char *in = NULL;
  const char *out = NULL;
  const char *palpath = NULL;
  const char *name = NULL;
  int summary = 0;
  int have_entry = 0;
  uint32_t entry = 0;
  uint32_t frame = 0;
  for (int i = 2; i < argc; i++) {
    const char *a = argv[i];
    if (strcmp(a, "--entry") == 0) {
      if (++i >= argc || !parse_u32(argv[i], &entry))
        return arg_error("--entry needs a non-negative index");
      have_entry = 1;
    } else if (strcmp(a, "--name") == 0) {
      if (++i >= argc) return arg_error("--name needs a value");
      name = argv[i];
    } else if (strcmp(a, "--frame") == 0) {
      if (++i >= argc || !parse_u32(argv[i], &frame))
        return arg_error("--frame needs a non-negative frame index");
    } else if (strcmp(a, "--out") == 0) {
      if (++i >= argc) return arg_error("--out needs a path");
      out = argv[i];
    } else if (strcmp(a, "--palette") == 0) {
      if (++i >= argc) return arg_error("--palette needs a file");
      palpath = argv[i];
    } else if (strcmp(a, "--print-summary") == 0) {
      summary = 1;
    } else if (a[0] == '-') {
      return arg_error("unknown option '%s'", a);
    } else if (!in) {
      in = a;
    } else {
      return arg_error("unexpected argument '%s'", a);
    }
  }
  if (!in) return arg_error("sprite needs an input FILE");
  if (have_entry && name) return arg_error("--entry and --name are exclusive");

  uint8_t *data = NULL;
  size_t n = 0;
  fifa96_err_t fe = fifa96_file_read(in, &data, &n);
  if (fe != FIFA96_OK) {
    errmsg("cannot read '%s' (%d)", in, (int)fe);
    return 1;
  }

  uint8_t *outer_owned = NULL;
  const uint8_t *leaf = NULL;
  size_t leaf_len = 0;
  int r = sprite_leaf(data, n, &outer_owned, &leaf, &leaf_len);
  if (r != 0) {
    errmsg("'%s' does not decode to a BIGF/SHPI container (%d)", in, -r);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }

  uint8_t *bank_owned = NULL;
  const uint8_t *bank_src = NULL;
  size_t bank_len = 0;
  uint32_t entries = 1;
  uint32_t sel_entry = 0;
  const char *sel_name = "-";
  if (memcmp(leaf, "SHPI", 4) == 0) {
    bank_src = leaf;
    bank_len = leaf_len;
  } else {
    struct fifa96_bigf_info bigf;
    if (fifa96_bigf_parse(leaf, leaf_len, &bigf) != FIFA96_OK) {
      errmsg("'%s' is not a valid BIGF container", in);
      free(outer_owned);
      fifa96_file_free(data);
      return 1;
    }
    entries = bigf.count;
    uint32_t off = 0, size = 0;
    const char *rec_name = NULL;
    if (name) {
      int found = 0;
      for (uint32_t i = 0; i < bigf.count; i++) {
        if (fifa96_bigf_record(&bigf, i, &off, &size, &rec_name) != FIFA96_OK) break;
        if (strcmp(rec_name, name) == 0) {
          found = 1;
          sel_entry = i;
          break;
        }
      }
      if (!found) {
        errmsg("entry '%s' is not present in '%s'", name, in);
        free(outer_owned);
        fifa96_file_free(data);
        return 1;
      }
    } else {
      fifa96_err_t be = fifa96_bigf_record(&bigf, entry, &off, &size, &rec_name);
      if (be != FIFA96_OK) {
        errmsg("entry %u is not present in '%s' (%d)", entry, in, (int)be);
        free(outer_owned);
        fifa96_file_free(data);
        return 1;
      }
      sel_entry = entry;
    }
    sel_name = rec_name;
    r = sprite_leaf(leaf + off, size, &bank_owned, &bank_src, &bank_len);
    if (r != 0 || memcmp(bank_src, "SHPI", 4) != 0) {
      errmsg("entry '%s' is not a decodable SHPI sprite bank (%d)", sel_name, r ? -r : -1);
      free(bank_owned);
      free(outer_owned);
      fifa96_file_free(data);
      return 1;
    }
  }

  fifa96_sprite_bank bank;
  r = fifa96_sprite_bank_parse(bank_src, bank_len, &bank);
  if (r != 0) {
    errmsg("entry '%s' is not a valid SHPI sprite bank (%d)", sel_name, -r);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }
  if (bank.count == 0) {
    errmsg("entry '%s' has no frames", sel_name);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }
  if (frame >= bank.count) {
    errmsg("frame %u not present (entry '%s' has %u frames)", frame, sel_name, bank.count);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }
  fifa96_sprite_entry fe_entry;
  r = fifa96_sprite_bank_entry(&bank, frame, &fe_entry);
  if (r != 0) {
    errmsg("entry '%s' frame %u is not addressable (%d)", sel_name, frame, -r);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }
  fifa96_sprite_frame sf;
  r = fifa96_sprite_frame_parse(&bank, fe_entry.offset, &sf);
  if (r != 0) {
    errmsg("entry '%s' frame %u has no valid header (%d)", sel_name, frame, -r);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }
  uint64_t pixels = (uint64_t)sf.width * sf.height;
  if (!sf.pixels || sf.pixel_len < pixels || pixels > PLAY_MAX_PIXELS) {
    errmsg("entry '%s' frame %u: decoded %u bytes for %ux%u", sel_name, frame, sf.pixel_len,
           sf.width, sf.height);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }

  uint8_t pal[768];
  if (palpath) {
    uint8_t *pdata = NULL;
    size_t pn = 0;
    fifa96_err_t pe = fifa96_file_read(palpath, &pdata, &pn);
    if (pe != FIFA96_OK || pn < sizeof pal) {
      errmsg("palette '%s' must be at least 768 bytes (%zu)", palpath, pn);
      fifa96_file_free(pdata);
      free(bank_owned);
      free(outer_owned);
      fifa96_file_free(data);
      return 1;
    }
    memcpy(pal, pdata, sizeof pal);
    fifa96_file_free(pdata);
  } else {
    for (uint32_t i = 0; i < 256; i++) {
      pal[i * 3] = (uint8_t)i;
      pal[i * 3 + 1] = (uint8_t)i;
      pal[i * 3 + 2] = (uint8_t)i;
    }
  }

  const char *path = out ? out : PLAY_DEFAULT_DIR "/sprite.ppm";
  if (ensure_parent(path) != 0) {
    errmsg("cannot create parent directory of '%s'", path);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }
  uint8_t *buf = xmalloc(64 + (size_t)pixels * 3);
  int hn = snprintf((char *)buf, 64, "P6\n%u %u\n255\n", sf.width, sf.height);
  if (hn < 0 || hn >= 64) {
    errmsg("entry '%s' frame %u: cannot format PPM header", sel_name, frame);
    free(buf);
    free(bank_owned);
    free(outer_owned);
    fifa96_file_free(data);
    return 1;
  }
  for (uint64_t i = 0; i < pixels; i++) {
    const uint8_t *rgb = pal + (size_t)sf.pixels[i] * 3;
    buf[(size_t)hn + i * 3] = rgb[0];
    buf[(size_t)hn + i * 3 + 1] = rgb[1];
    buf[(size_t)hn + i * 3 + 2] = rgb[2];
  }
  int rc = write_file(path, buf, (size_t)hn + (size_t)pixels * 3);
  if (rc == 0 && summary) {
    char hex[65];
    play_sha256_hex(buf, (size_t)hn + (size_t)pixels * 3, hex);
    printf("sprite: entries=%u entry=%u name=%s frames=%u frame=%u %ux%u ppm sha256=%s\n",
           entries, sel_entry, sel_name, bank.count, frame, sf.width, sf.height, hex);
  }
  free(buf);
  free(bank_owned);
  free(outer_owned);
  fifa96_file_free(data);
  return rc;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(stderr);
    return 2;
  }
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    usage(stdout);
    return 0;
  }
  if (strcmp(argv[1], "video") == 0) return cmd_video(argc, argv);
  if (strcmp(argv[1], "audio") == 0) return cmd_audio(argc, argv);
  if (strcmp(argv[1], "sprite") == 0) return cmd_sprite(argc, argv);
  return arg_error("unknown mode '%s'", argv[1]);
}
