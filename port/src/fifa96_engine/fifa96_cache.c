#include "fifa96_engine/fifa96_cache.h"
#include <stdlib.h>
#include <string.h>

struct fifa96_cache_entry {
  struct fifa96_cache_entry *next;
  char *path;
  uint8_t *bytes;
  size_t len;
};

struct fifa96_cache {
  struct fifa96_asset_table *table;
  struct fifa96_cache_entry *entries;
};

static int cache_upper(int c) {
  return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

static int cache_path_eq(const char *a, const char *b) {
  while (*a == '/') a++;
  while (*b == '/') b++;
  while (*a && *b) {
    if (cache_upper((unsigned char)*a) != cache_upper((unsigned char)*b)) return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

struct fifa96_cache *fifa96_cache_create(struct fifa96_asset_table *t) {
  if (!t) return NULL;
  struct fifa96_cache *c = (struct fifa96_cache *)calloc(1, sizeof *c);
  if (!c) return NULL;
  c->table = t;
  return c;
}

const uint8_t *fifa96_cache_get(struct fifa96_cache *c, const char *path, size_t *len) {
  if (!len) return NULL;
  *len = 0;
  if (!c || !path) return NULL;

  for (struct fifa96_cache_entry *e = c->entries; e; e = e->next) {
    if (cache_path_eq(e->path, path)) {
      *len = e->len;
      return e->bytes;
    }
  }

  uint8_t *bytes = NULL;
  size_t n = 0;
  if (fifa96_asset_read(c->table, path, &bytes, &n) != FIFA96_OK) return NULL;

  size_t path_n = strlen(path);
  struct fifa96_cache_entry *e = (struct fifa96_cache_entry *)malloc(sizeof *e);
  char *copy = (char *)malloc(path_n + 1u);
  if (!e || !copy) {
    free(e);
    free(copy);
    fifa96_asset_free(bytes);
    return NULL;
  }
  memcpy(copy, path, path_n + 1u);
  e->path = copy;
  e->bytes = bytes;
  e->len = n;
  e->next = c->entries;
  c->entries = e;

  *len = n;
  return bytes;
}

void fifa96_cache_destroy(struct fifa96_cache *c) {
  if (!c) return;
  struct fifa96_cache_entry *e = c->entries;
  while (e) {
    struct fifa96_cache_entry *next = e->next;
    free(e->path);
    fifa96_asset_free(e->bytes);
    free(e);
    e = next;
  }
  free(c);
}
