//
//  a LRU module that supports different obj size
//
//
//  LRUSize.c
//  CMimircache
//
//  Created by Juncheng on 12/4/18.
//  Copyright © 2018 Juncheng. All rights reserved.
//

#include "LRUSize.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" {
#endif

void __LRUSize_insert_element(cache_t *cache, request_t *cp) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);
  cache_obj_t *cache_obj = g_new(cache_obj_t, 1);
  cache_obj->size = cp->size;
  cache->core->used_size += cp->size;

  if (cp->label_type == 'l') {
    cache_obj->key = (gpointer)g_new(guint64, 1);
    *(guint64 *)(cache_obj->key) = *(guint64 *)(cp->label_ptr);
  } else {
    cache_obj->key = (gpointer)g_strdup((gchar *)(cp->label_ptr));
  }

  GList *node = g_list_alloc();
  node->data = cache_obj;

  g_queue_push_tail_link(LRUSize_params->list, node);
  g_hash_table_insert(LRUSize_params->hashtable, cache_obj->key,
                      (gpointer)node);
}

gboolean LRUSize_check_element(cache_t *cache, request_t *cp) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);
  return g_hash_table_contains(LRUSize_params->hashtable, cp->label_ptr);
}

void __LRUSize_update_element(cache_t *cache, request_t *cp) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);
  GList *node =
      (GList *)g_hash_table_lookup(LRUSize_params->hashtable, cp->label_ptr);

  cache_obj_t *cache_obj = node->data;
  if (cache->core->used_size < ((cache_obj_t *)(node->data))->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long)cache->core->used_size,
          (unsigned long long)((cache_obj_t *)(node->data))->size);
    abort();
  }
  cache->core->used_size -= cache_obj->size;
  cache->core->used_size += cp->size;
  cache_obj->size = cp->size;

  g_queue_unlink(LRUSize_params->list, node);
  g_queue_push_tail_link(LRUSize_params->list, node);
}

void __LRUSize_evict_element(cache_t *cache, request_t *cp) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);

  cache_obj_t *cache_obj =
      (cache_obj_t *)g_queue_pop_head(LRUSize_params->list);

  if (cache->core->used_size < cache_obj->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long)cache->core->used_size,
          (unsigned long long)cache_obj->size);
    abort();
  }
  cache->core->used_size -= cache_obj->size;
  g_hash_table_remove(LRUSize_params->hashtable, (gconstpointer)cache_obj->key);
  g_free(cache_obj);
}

gpointer __LRUSize__evict_with_return(cache_t *cache, request_t *cp) {
  /** evict one element and return the evicted element,
   * needs to free the memory of returned data
   */

  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);

  cache_obj_t *cache_obj = g_queue_pop_head(LRUSize_params->list);
  if (cache->core->used_size < cache_obj->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long)cache->core->used_size,
          (unsigned long long)cache_obj->size);
    abort();
  }

  cache->core->used_size -= cache_obj->size;

  gpointer evicted_key;
  if (cp->label_type == 'l') {
    evicted_key = (gpointer)g_new(guint64, 1);
    *(guint64 *)evicted_key = *(guint64 *)(cache_obj->key);
  } else {
    evicted_key = (gpointer)g_strdup((gchar *)(cache_obj->key));
  }

  g_hash_table_remove(LRUSize_params->hashtable,
                      (gconstpointer)(cache_obj->key));
  g_free(cache_obj);
  return evicted_key;
}

gboolean LRUSize_add_element(cache_t *cache, request_t *cp) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);
  if (cp->size == 0) {
    ERROR("LRUSize get size zero for request\n");
    abort();
  }

  gboolean exist = LRUSize_check_element(cache, cp);

  if (LRUSize_check_element(cache, cp))
    __LRUSize_update_element(cache, cp);
  else
    __LRUSize_insert_element(cache, cp);

  while (cache->core->used_size > cache->core->size)
    __LRUSize_evict_element(cache, cp);

  LRUSize_params->ts++;
  cache->core->ts += 1;
  return exist;
}

void LRUSize_destroy(cache_t *cache) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);

  g_hash_table_destroy(LRUSize_params->hashtable);
  //    g_queue_free(LRUSize_params->list);
  g_queue_free_full(LRUSize_params->list, simple_g_key_value_destroyer);
  //    cache_destroy(cache);
}

void LRUSize_destroy_unique(cache_t *cache) {
  /* the difference between destroy_unique and destroy
   is that the former one only free the resources that are
   unique to the cache, freeing these resources won't affect
   other caches copied from original cache
   in Optimal, next_access should not be freed in destroy_unique,
   because it is shared between different caches copied from the original one.
   */

  LRUSize_destroy(cache);
}

cache_t *LRUSize_init(guint64 size, char data_type, guint64 block_size,
                      void *params) {
  cache_t *cache = cache_init(size, data_type, block_size);
  cache->cache_params = g_new0(struct LRUSize_params, 1);
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);

  cache->core->type = e_LRUSize;
  cache->core->cache_init = LRUSize_init;
  cache->core->destroy = LRUSize_destroy;
  cache->core->destroy_unique = LRUSize_destroy_unique;
  cache->core->add_element = LRUSize_add_element;
  cache->core->check_element = LRUSize_check_element;
  cache->core->__insert_element = __LRUSize_insert_element;
  cache->core->__update_element = __LRUSize_update_element;
  cache->core->__evict_element = __LRUSize_evict_element;
  cache->core->__evict_with_return = __LRUSize__evict_with_return;
  cache->core->get_current_size = LRUSize_get_size;
  cache->core->get_objmap = LRUSize_get_objmap;
  cache->core->remove_element = LRUSize_remove_element;
  cache->core->cache_init_params = NULL;

  if (data_type == 'l') {
    LRUSize_params->hashtable = g_hash_table_new_full(
        g_int64_hash, g_int64_equal, simple_g_key_value_destroyer, NULL);
  } else if (data_type == 'c') {
    LRUSize_params->hashtable = g_hash_table_new_full(
        g_str_hash, g_str_equal, simple_g_key_value_destroyer, NULL);
  } else {
    ERROR("does not support given data label_type: %c\n", data_type);
  }
  LRUSize_params->list = g_queue_new();

  return cache;
}

void LRUSize_remove_element(cache_t *cache, void *data_to_remove) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);

  GList *node =
      (GList *)g_hash_table_lookup(LRUSize_params->hashtable, data_to_remove);
  if (!node) {
    fprintf(stderr,
            "LRUSize_remove_element: data to remove is not in the cache\n");
    abort();
  }
  if (cache->core->used_size < ((cache_obj_t *)(node->data))->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long)cache->core->used_size,
          (unsigned long long)((cache_obj_t *)(node->data))->size);
    abort();
  }

  cache->core->used_size -= ((cache_obj_t *)(node->data))->size;
  g_queue_delete_link(LRUSize_params->list, (GList *)node);
  g_hash_table_remove(LRUSize_params->hashtable, data_to_remove);
}

guint64 LRUSize_get_size(cache_t *cache) {
  return (guint64)cache->core->used_size;
}

GHashTable *LRUSize_get_objmap(cache_t *cache) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *)(cache->cache_params);
  return LRUSize_params->hashtable;
}

#ifdef __cplusplus
extern "C" {
#endif
