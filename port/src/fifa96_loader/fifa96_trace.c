#include "fifa96_loader/fifa96_trace.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint16_t k_site_addr[FIFA96_TRACE_NSITES] = {
  0x5aa1, 0x5aa8, 0x5f6e, 0x5f89, 0x5f4b
};

static uint16_t rd16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

static uint32_t rd32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

typedef struct {
  char *p;
  size_t n;
  size_t cap;
  int oom;
} sbuf;

static int sb_init(sbuf *b, size_t cap) {
  b->p = (char *)malloc(cap ? cap : 1u);
  if (!b->p) return -1;
  b->cap = cap ? cap : 1u;
  b->n = 0;
  b->p[0] = '\0';
  b->oom = 0;
  return 0;
}

static int sb_reserve(sbuf *b, size_t extra) {
  if (b->n + extra + 1u <= b->cap) return 0;
  size_t cap = b->cap;
  while (cap < b->n + extra + 1u) {
    if (cap > ((size_t)-1) / 2u) return -1;
    cap *= 2u;
  }
  char *np = (char *)realloc(b->p, cap);
  if (!np) return -1;
  b->p = np;
  b->cap = cap;
  return 0;
}

static void sb_puts(sbuf *b, const char *s) {
  if (b->oom) return;
  size_t l = strlen(s);
  if (sb_reserve(b, l)) { b->oom = 1; return; }
  memcpy(b->p + b->n, s, l);
  b->n += l;
  b->p[b->n] = '\0';
}

static void sb_printf(sbuf *b, const char *fmt, ...) {
  if (b->oom) return;
  va_list ap;
  va_start(ap, fmt);
  for (;;) {
    size_t avail = b->cap - b->n;
    va_list ap2;
    va_copy(ap2, ap);
    int need = vsnprintf(b->p + b->n, avail, fmt, ap2);
    va_end(ap2);
    if (need < 0) { b->oom = 1; break; }
    if ((size_t)need < avail) { b->n += (size_t)need; break; }
    if (sb_reserve(b, (size_t)need + 1u)) { b->oom = 1; break; }
  }
  va_end(ap);
}

static void sb_hex(sbuf *b, const uint8_t *p, size_t len) {
  for (size_t i = 0; i < len; i++) sb_printf(b, "%02x", (unsigned)p[i]);
}

static void site_str(char *dst, size_t cap, unsigned id) {
  if (id < FIFA96_TRACE_NSITES)
    snprintf(dst, cap, "%04x", (unsigned)k_site_addr[id]);
  else
    snprintf(dst, cap, "?0x%02x", id & 0xffu);
}

static const char *type_name(uint8_t t) {
  switch (t) {
    case FIFA96_TRACE_TYPE_HEADER: return "HEADER";
    case FIFA96_TRACE_TYPE_FILE: return "FILE";
    case FIFA96_TRACE_TYPE_CODEC: return "CODEC";
    case FIFA96_TRACE_TYPE_HEARTBEAT: return "HEARTBEAT";
    case FIFA96_TRACE_TYPE_PATCH_SKIP: return "PATCH_SKIP";
    case FIFA96_TRACE_TYPE_END: return "END";
    case FIFA96_TRACE_TYPE_PATCH_OK: return "PATCH_OK";
    default: return "?";
  }
}

static const char *skip_reason(unsigned r) {
  switch (r) {
    case 0: return "bad-signature";
    case 1: return "already-patched";
    case 2: return "site-out-of-range";
    default: return "?";
  }
}

static int header_ok(const uint8_t *data, size_t len) {
  if (len < 11u) return 0;
  if (data[0] != FIFA96_TRACE_TYPE_HEADER) return 0;
  if (rd16(data + 3) != 6u) return 0;
  return memcmp(data + 5, "FCAP", 4) == 0;
}

static int frame_at(const uint8_t *data, size_t len, size_t pos,
                    uint8_t *t, uint16_t *seq, uint16_t *plen) {
  if (pos + 5u > len) return 0;
  uint8_t ty = data[pos];
  if (ty < FIFA96_TRACE_TYPE_HEADER || ty > FIFA96_TRACE_TYPE_PATCH_OK) return 0;
  uint16_t l = rd16(data + pos + 3);
  if ((size_t)l > len - pos - 5u) return 0;
  if (ty == FIFA96_TRACE_TYPE_HEADER) {
    if (l != 6u) return 0;
    if (memcmp(data + pos + 5, "FCAP", 4) != 0) return 0;
  }
  *t = ty;
  *seq = rd16(data + pos + 1);
  *plen = l;
  return 1;
}

#define FIFA96_TRACE_MAX_NAMES 32

typedef struct {
  char name[16];
  unsigned opens;
  unsigned reads;
  unsigned long long bytes;
} fby_entry;

typedef struct {
  char handle_name[256][16];
  uint8_t handle_set[256];
  fby_entry fby[FIFA96_TRACE_MAX_NAMES];
  int nfby;
  unsigned opens, reads, writes, other;
  unsigned patch_ok, patch_skip;
  unsigned codec_hits[FIFA96_TRACE_NSITES];
  unsigned lost, seqgaps;
  int saw_end;
  uint8_t have_seq[8];
  uint16_t next_seq[8];
} walker;

static fby_entry *fby_find(walker *w, const char *name) {
  for (int i = 0; i < w->nfby; i++)
    if (strcmp(w->fby[i].name, name) == 0) return &w->fby[i];
  return NULL;
}

static fby_entry *fby_get(walker *w, const char *name) {
  fby_entry *e = fby_find(w, name);
  if (e) return e;
  if (w->nfby >= FIFA96_TRACE_MAX_NAMES) return NULL;
  e = &w->fby[w->nfby++];
  memset(e, 0, sizeof(*e));
  snprintf(e->name, sizeof(e->name), "%s", name);
  return e;
}

static void set_handle_name(walker *w, unsigned h, const uint8_t *p, size_t dlen) {
  if (h > 255u) return;
  size_t l = dlen < 15u ? dlen : 15u;
  memcpy(w->handle_name[h], p, l);
  w->handle_name[h][l] = '\0';
  w->handle_set[h] = 1;
}

static void render_file(sbuf *b, walker *w, const uint8_t *p, uint16_t plen) {
  if (plen < 18u) { sb_puts(b, "FILE malformed\n"); return; }
  unsigned ah = p[0];
  unsigned bx = rd16(p + 1);
  unsigned cx = rd16(p + 3);
  unsigned ax_after = rd16(p + 9);
  unsigned flags = p[11];
  uint32_t hash = rd32(p + 12);
  unsigned dlen = rd16(p + 16);
  const uint8_t *data = p + 18;
  if ((size_t)dlen > (size_t)plen - 18u) dlen = (unsigned)(plen - 18u);
  const char *name = "-";
  int have_name = 0;
  char tmp[16];
  if (flags & 0x02u) {
    size_t l = dlen < 15u ? dlen : 15u;
    memcpy(tmp, data, l);
    tmp[l] = '\0';
    set_handle_name(w, ax_after, data, dlen);
    name = tmp;
    have_name = 1;
    fby_entry *e = fby_get(w, tmp);
    if (e) e->opens++;
  } else if (w->handle_set[bx & 0xffu]) {
    name = w->handle_name[bx & 0xffu];
    have_name = 1;
  }
  if (ah == 0x3Fu && have_name && (flags & 0x01u)) {
    fby_entry *e = fby_find(w, name);
    if (e) { e->reads++; e->bytes += ax_after; }
  }
  if (ah == 0x3Du && (flags & 0x02u)) w->opens++;
  else if (ah == 0x3Fu && (flags & 0x01u)) w->reads++;
  else if (ah == 0x40u) w->writes++;
  else w->other++;
  sb_printf(b, "FILE ah=%02X h=0x%04x x=%u got=%u name=%s hash=",
            ah, bx, cx, ax_after, name);
  if (flags & 0x01u) sb_printf(b, "%08x", (unsigned)hash);
  else sb_puts(b, "-");
  sb_printf(b, " len=%u head=", dlen);
  sb_hex(b, data, dlen < 64u ? dlen : 64u);
  sb_puts(b, "\n");
}

static void render_codec(sbuf *b, walker *w, const uint8_t *p, uint16_t plen) {
  if (plen < 39u) { sb_puts(b, "CODEC malformed\n"); return; }
  unsigned site = p[0];
  unsigned ax = rd16(p + 1), bx = rd16(p + 3), cx = rd16(p + 5), dx = rd16(p + 7);
  unsigned si = rd16(p + 9), di = rd16(p + 11), bp = rd16(p + 13), sp = rd16(p + 15);
  unsigned ds = rd16(p + 17), es = rd16(p + 19), ss = rd16(p + 21), ip = rd16(p + 23), cs = rd16(p + 25);
  uint32_t hash = rd32(p + 29);
  unsigned dseg = rd16(p + 33), doff = rd16(p + 35), dlen = rd16(p + 37);
  const uint8_t *data = p + 39;
  if ((size_t)dlen > (size_t)plen - 39u) dlen = (unsigned)(plen - 39u);
  if (site < FIFA96_TRACE_NSITES) w->codec_hits[site]++;
  char st[16];
  site_str(st, sizeof(st), site);
  sb_printf(b, "CODEC site=%s ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x "
               "bp=%04x sp=%04x ds=%04x es=%04x ss=%04x ip=%04x cs=%04x "
               "hash=%08x seg=%04x off=%04x len=%u head=",
            st, ax, bx, cx, dx, si, di, bp, sp, ds, es, ss, ip, cs,
            (unsigned)hash, dseg, doff, dlen);
  sb_hex(b, data, dlen < 64u ? dlen : 64u);
  sb_puts(b, "\n");
}

static int trace_impl(const uint8_t *data, size_t len, char **out, int raw) {
  *out = NULL;
  sbuf b;
  if (sb_init(&b, 512u)) return FIFA96_ERR_TRUNCATED;
  if (!header_ok(data, len)) {
    sb_puts(&b, "BAD-HEADER\n");
    if (b.oom) { free(b.p); return FIFA96_ERR_TRUNCATED; }
    *out = b.p;
    return FIFA96_ERR_BAD_MAGIC;
  }
  walker w;
  memset(&w, 0, sizeof(w));
  const uint8_t *hp = data + 5;
  if (raw) {
    sb_printf(&b, "RAW type=%02x seq=%u len=%u ", (unsigned)data[0],
              (unsigned)rd16(data + 1), 6u);
    sb_hex(&b, hp, 6u);
    sb_puts(&b, "\n");
  } else {
    sb_printf(&b, "HEADER magic=%c%c%c%c version=%u patches=%u\n",
              hp[0], hp[1], hp[2], hp[3], (unsigned)hp[4], (unsigned)hp[5]);
  }
  size_t pos = 11u;
  while (pos < len) {
    uint8_t t;
    uint16_t sq, pl;
    if (!frame_at(data, len, pos, &t, &sq, &pl)) {
      w.lost++;
      sb_printf(&b, "LOST-SYNC off=0x%04x\n", (unsigned)pos);
      do { pos++; } while (pos < len && !frame_at(data, len, pos, &t, &sq, &pl));
      if (pos >= len) break;
    }
    const uint8_t *pay = data + pos + 5u;
    if (t == FIFA96_TRACE_TYPE_FILE || t == FIFA96_TRACE_TYPE_CODEC ||
        t == FIFA96_TRACE_TYPE_HEARTBEAT) {
      if (w.have_seq[t] && sq != w.next_seq[t]) {
        w.seqgaps++;
        sb_printf(&b, "SEQ-GAP type=%s expected=%u got=%u\n",
                  type_name(t), (unsigned)w.next_seq[t], (unsigned)sq);
      }
      w.have_seq[t] = 1;
      w.next_seq[t] = (uint16_t)(sq + 1u);
    }
    if (raw) {
      sb_printf(&b, "RAW type=%02x seq=%u len=%u", (unsigned)t,
                (unsigned)sq, (unsigned)pl);
      if (pl) { sb_puts(&b, " "); sb_hex(&b, pay, pl); }
      sb_puts(&b, "\n");
    } else {
      switch (t) {
        case FIFA96_TRACE_TYPE_HEADER:
          sb_printf(&b, "HEADER magic=%c%c%c%c version=%u patches=%u\n",
                    pay[0], pay[1], pay[2], pay[3], (unsigned)pay[4], (unsigned)pay[5]);
          break;
        case FIFA96_TRACE_TYPE_FILE:
          render_file(&b, &w, pay, pl);
          break;
        case FIFA96_TRACE_TYPE_CODEC:
          render_codec(&b, &w, pay, pl);
          break;
        case FIFA96_TRACE_TYPE_HEARTBEAT:
          if (pl >= 4u) sb_printf(&b, "HEARTBEAT files=%u\n", (unsigned)rd32(pay));
          else sb_puts(&b, "HEARTBEAT malformed\n");
          break;
        case FIFA96_TRACE_TYPE_PATCH_SKIP:
          if (pl >= 2u) {
            char st[16];
            site_str(st, sizeof(st), pay[0]);
            sb_printf(&b, "PATCH_SKIP site=%s reason=%s\n", st, skip_reason(pay[1]));
            w.patch_skip++;
          }
          break;
        case FIFA96_TRACE_TYPE_END:
          w.saw_end = 1;
          sb_puts(&b, "END\n");
          break;
        case FIFA96_TRACE_TYPE_PATCH_OK:
          if (pl >= 6u) {
            char st[16];
            site_str(st, sizeof(st), pay[0]);
            sb_printf(&b, "PATCH_OK site=%s target=0x%08x siglen=%u\n",
                      st, (unsigned)rd32(pay + 1), (unsigned)pay[5]);
            w.patch_ok++;
          }
          break;
        default:
          break;
      }
    }
    pos += 5u + (size_t)pl;
  }
  sb_puts(&b, "SUMMARY files-by-name:\n");
  for (int i = 0; i < w.nfby; i++)
    sb_printf(&b, "  %s opens=%u reads=%u bytes=%llu\n",
              w.fby[i].name, w.fby[i].opens, w.fby[i].reads, w.fby[i].bytes);
  sb_printf(&b, "SUMMARY opens=%u reads=%u writes=%u other=%u\n",
            w.opens, w.reads, w.writes, w.other);
  sb_printf(&b, "SUMMARY patch: ok=%u skip=%u\n", w.patch_ok, w.patch_skip);
  for (int i = 0; i < FIFA96_TRACE_NSITES; i++)
    if (w.codec_hits[i]) {
      char st[16];
      site_str(st, sizeof(st), (unsigned)i);
      sb_printf(&b, "SUMMARY codec: site=%s hits=%u\n", st, w.codec_hits[i]);
    }
  sb_printf(&b, "SUMMARY end=%s lost=%u seqgaps=%u\n",
            w.saw_end ? "END" : "no-END", w.lost, w.seqgaps);
  if (b.oom) { free(b.p); return FIFA96_ERR_TRUNCATED; }
  *out = b.p;
  return FIFA96_OK;
}

int fifa96_trace_format(const uint8_t *data, size_t len, char **out) {
  return trace_impl(data, len, out, 0);
}

int fifa96_trace_raw(const uint8_t *data, size_t len, char **out) {
  return trace_impl(data, len, out, 1);
}
