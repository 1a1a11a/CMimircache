//
//  cache.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef CACHE_H
#define CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cleaner.h"
#include "reader.h"
#include "const.h"
#include "macro.h"
#include "errors.h"
#include "logging.h"

typedef enum {
  e_LRU,
  e_LFU,
  e_LFU_fast,
  e_Optimal,
  e_FIFO,
  e_LRU_K,
  e_MRU,
  e_Random,
  e_ARC,
  e_SLRU,

  e_LRUSize,
  e_FIFOSize,

  e_AMP,
  e_LRUPage,
  e_PG,

  e_mimir,
  e_Mithril,
} cache_type;

typedef struct cache_obj {
  gpointer key;
//    char key[CACHE_LINE_LABEL_SIZE];
//    void *content;
  guint64 size;
} cache_obj_t;

inline void cacheobj_destroyer(gpointer data) {
  GList *node = (GList *) data;
  cache_obj_t *cache_obj = (cache_obj_t *) node->data;
  g_free(cache_obj);
}

struct cache_core {
  cache_type type;
  long size;
  guint64 occupied_size;
  char data_type;     // l, c
  long long hit_count;
  long long miss_count;
  void *cache_init_params;
  gboolean consider_size;
  guint64 block_size;

  struct cache *(*cache_init)(guint64, char, guint64, void *);

  void (*destroy)(struct cache *);

  void (*destroy_unique)(struct cache *);

  gboolean (*add_element)(struct cache *, cache_line *);

  gboolean (*check_element)(struct cache *, cache_line *);

  // newly added 0912, may not work for all cache
  void (*__insert_element)(struct cache *, cache_line *);

  void (*__update_element)(struct cache *, cache_line *);

  void (*__evict_element)(struct cache *, cache_line *);

  gpointer (*__evict_with_return)(struct cache *, cache_line *);

  guint64 (*get_size)(struct cache *);         // get current size of used cache

  GHashTable* (*get_objmap)(struct cache *);     // get the hash map

  void (*remove_element)(struct cache *, void *);

  gboolean (*add_element_only)(struct cache *, cache_line *);

  gboolean (*add_element_withsize)(struct cache *, cache_line *);
  // only insert(and possibly evict) or update, do not conduct any other
  // operation, especially for those complex algorithm



  /** Jason: need to remove shared struct in cache and move all shared struct into reader **/
  int record_level;  // 0 not debug, 1: record evictions, 2: compare to oracle
  void *oracle;
  void *eviction_array;     // Optimal Eviction Array, either guint64* or char**
  guint64 eviction_array_len;
  guint64 evict_err;      // used for counting
  break_point_t *bp;           // break points, same as the one in reader, just one more pointer
  guint64 bp_pos;         // the current location in bp->array
  gdouble *evict_err_array;       // in each time interval, the eviction error array
};

struct cache {
  struct cache_core *core;
  void *cache_params;
};

typedef struct cache struct_cache;
typedef struct cache cache_t;

extern cache_t *cache_init(long long size, char data_type, guint64 block_size);
extern void cache_destroy(struct_cache *cache);
extern void cache_destroy_unique(struct_cache *cache);

#ifdef __cplusplus
}
#endif

#endif /* cache_h */
