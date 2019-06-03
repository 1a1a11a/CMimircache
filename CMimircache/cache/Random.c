//
//  Random.c
//  mimircache
//
//  Random cache replacement policy
//
//  Created by Juncheng on 8/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#include "Random.h"

#ifdef __cplusplus
extern "C" {
#endif

void __Random_insert_element(cache_t *Random, request_t *cp) {
  struct Random_params *Random_params =
      (struct Random_params *)(Random->cache_params);

  gpointer key;
  if (cp->label_type == 'l') {
    key = (gpointer)g_new(guint64, 1);
    *(guint64 *)key = *(guint64 *)(cp->label_ptr);
  } else {
    key = (gpointer)g_strdup((gchar *)(cp->label_ptr));
  }

  if ((long)g_hash_table_size(Random_params->hashtable) < Random->core->size) {
    // no eviction, just insert
    g_array_append_val(Random_params->array, key);
    g_hash_table_add(Random_params->hashtable, key);
  } else {
    // replace randomly chosen element
    guint64 pos = rand() % (Random->core->size);
    /* the line below is to ensure that the randomized index can be large
    *enough,
    **   because RAND_MAX is only guaranteed to be 32767
    */
    pos = (rand() % (Random->core->size) * pos) % (Random->core->size);
    gpointer evict_key = g_array_index(Random_params->array, gpointer, pos);
    g_hash_table_remove(Random_params->hashtable, evict_key);
    g_array_index(Random_params->array, gpointer, pos) = key;
    g_hash_table_add(Random_params->hashtable, (gpointer)key);
  }
}

gboolean Random_check_element(cache_t *cache, request_t *cp) {
  return g_hash_table_contains(
      ((struct Random_params *)(cache->cache_params))->hashtable,
      (gconstpointer)(cp->label_ptr));
}

void __Random_update_element(cache_t *cache, request_t *cp) { ; }

void __Random_evict_element(cache_t *cache, request_t *cp) { ; }

gboolean Random_add_element(cache_t *cache, request_t *cp) {
  gboolean retval;
  if (Random_check_element(cache, cp)) {
    retval = TRUE;
  } else {
    __Random_insert_element(cache, cp);
    retval = FALSE;
  }
  cache->core->ts += 1;
  return retval;
}

void Random_destroy(cache_t *cache) {
  struct Random_params *Random_params =
      (struct Random_params *)(cache->cache_params);

  g_hash_table_destroy(Random_params->hashtable);
  g_array_free(Random_params->array, TRUE);
  cache_destroy(cache);
}

void Random_destroy_unique(cache_t *cache) {
  /* the difference between destroy_unique and destroy
   is that the former one only free the resources that are
   unique to the cache, freeing these resources won't affect
   other caches copied from original cache
   in Random, next_access should not be freed in destroy_unique,
   because it is shared between different caches copied from the original one.
   */
  Random_destroy(cache);
}

cache_t *Random_init(guint64 size, char data_type, guint64 block_size,
                     void *params) {
  cache_t *cache = cache_init(size, data_type, block_size);
  struct Random_params *Random_params = g_new0(struct Random_params, 1);
  cache->cache_params = (void *)Random_params;

  cache->core->type = e_Random;
  cache->core->cache_init = Random_init;
  cache->core->destroy = Random_destroy;
  cache->core->destroy_unique = Random_destroy_unique;
  cache->core->add_element = Random_add_element;
  cache->core->check_element = Random_check_element;
  cache->core->add_element_only = Random_add_element;

  if (data_type == 'l') {
    Random_params->hashtable = g_hash_table_new_full(
        g_int64_hash, g_int64_equal, simple_g_key_value_destroyer, NULL);
  }

  else if (data_type == 'c') {
    Random_params->hashtable = g_hash_table_new_full(
        g_str_hash, g_str_equal, simple_g_key_value_destroyer, NULL);
  } else {
    ERROR("does not support given data label_type: %c\n", data_type);
  }

  Random_params->array =
      g_array_sized_new(FALSE, FALSE, sizeof(gpointer), (guint)size);

  time_t t;
  srand((unsigned)time(&t));
  return cache;
}

#ifdef __cplusplus
}
#endif
