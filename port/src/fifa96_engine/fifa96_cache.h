#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_asset.h"

/* On-demand byte cache over an asset table. Entries are read from the table on
 * first request and kept until fifa96_cache_destroy; returned pointers are
 * stable for the lifetime of the cache. The asset table is borrowed, never
 * owned: destroying the cache leaves the table usable and unmounting the table
 * must happen after the cache is destroyed. */
struct fifa96_cache;

struct fifa96_cache *fifa96_cache_create(struct fifa96_asset_table *t);
const uint8_t *fifa96_cache_get(struct fifa96_cache *c, const char *path, size_t *len);
void fifa96_cache_destroy(struct fifa96_cache *c);
