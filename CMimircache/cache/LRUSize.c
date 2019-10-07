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



void __LRUSize_insert_element(cache_t *cache, request_t *req) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *) (cache->cache_params);
  cache_obj_t *cache_obj = g_new(cache_obj_t, 1);
  cache_obj->size = req->size;
  cache_obj->extra_data = req->extra_data;
#ifdef TRACK_ACCESS_TIME
    cache_obj->access_time = cache->core->ts;
#endif
  cache->core->used_size += req->size;

  if (req->label_type == 'l') {
    cache_obj->key = (gpointer) g_new(guint64, 1);
    *(guint64 *) (cache_obj->key) = *(guint64 *) (req->label_ptr);
  } else {
    cache_obj->key = (gpointer) g_strdup((gchar *) (req->label_ptr));
  }

  GList *node = g_list_alloc();
  node->data = cache_obj;

//  g_hash_table_foreach(LRUSize_params->hashtable, _objmap_aux, GUINT_TO_POINTER(cache->core->ts));

  g_queue_push_tail_link(LRUSize_params->list, node);
  g_hash_table_insert(LRUSize_params->hashtable, cache_obj->key,
                      (gpointer) node);
}

gboolean LRUSize_check_element(cache_t *cache, request_t *req) {
  LRUSize_params_t *LRUSize_params = (struct LRUSize_params *) (cache->cache_params);
  return g_hash_table_contains(LRUSize_params->hashtable, req->label_ptr);
}

void __LRUSize_update_element(cache_t *cache, request_t *req) {
  LRUSize_params_t *LRUSize_params = (struct LRUSize_params *) (cache->cache_params);
  GList *node = (GList *) g_hash_table_lookup(LRUSize_params->hashtable, req->label_ptr);

  cache_obj_t *cache_obj = node->data;
  if (cache->core->used_size < ((cache_obj_t *) (node->data))->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long) cache->core->used_size,
          (unsigned long long) ((cache_obj_t *) (node->data))->size);
    abort();
  }
  cache->core->used_size -= cache_obj->size;
  cache->core->used_size += req->size;
  cache_obj->size = req->size;
  // we can't update extra_data here, otherwise, the old extra_data will be (memory) leaked
  // cache_obj->extra_data = req->extra_data;
#ifdef TRACK_ACCESS_TIME
  cache_obj->access_time = cache->core->ts;
#endif
  g_queue_unlink(LRUSize_params->list, node);
  g_queue_push_tail_link(LRUSize_params->list, node);
}

void LRUSize_update_cached_data(cache_t* cache, request_t* req, void* extra_data){
  LRUSize_params_t *LRUSize_params = (struct LRUSize_params *) (cache->cache_params);
  GList *node = (GList *) g_hash_table_lookup(LRUSize_params->hashtable, req->label_ptr);
  cache_obj_t *cache_obj = node->data;
  if (cache_obj->extra_data != NULL)
    g_free(cache_obj->extra_data);
  cache_obj->extra_data = extra_data;
}

void* LRUSize_get_cached_data(cache_t *cache, request_t *req) {
  LRUSize_params_t *LRUSize_params = (struct LRUSize_params *) (cache->cache_params);
  GList *node = (GList *) g_hash_table_lookup(LRUSize_params->hashtable, req->label_ptr);
  cache_obj_t *cache_obj = node->data;
  return cache_obj->extra_data;
}

void __LRUSize_evict_element(cache_t *cache, request_t *req) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *) (cache->cache_params);

  cache_obj_t *cache_obj =
      (cache_obj_t *) g_queue_pop_head(LRUSize_params->list);

  if (cache->core->used_size < cache_obj->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long) cache->core->used_size,
          (unsigned long long) cache_obj->size);
    abort();
  }

#ifdef TRACK_EVICTION_AGE
  gint last_real_ts = GPOINTER_TO_INT(g_hash_table_lookup(
      LRUSize_params->last_access_rtime_map, cache_obj->key));
  gint last_logical_ts = GPOINTER_TO_INT(g_hash_table_lookup(
      LRUSize_params->last_access_vtime_map, cache_obj->key));
  gint eviction_age_realtime = req->real_time - last_real_ts;
  gint eviction_age_logical_time = LRUSize_params->ts - last_logical_ts;
  fprintf(LRUSize_params->eviction_age_ofile, "%llu: %lld: %d: %d\n",
          *(guint64 *)cache_obj->key, LRUSize_params->ts,
          eviction_age_logical_time, eviction_age_realtime);
#endif

  cache->core->used_size -= cache_obj->size;
  g_hash_table_remove(LRUSize_params->hashtable, (gconstpointer) cache_obj->key);
  g_free(cache_obj);
}

gpointer __LRUSize__evict_with_return(cache_t *cache, request_t *req) {
  /** evict one element and return the evicted element,
   * needs to free the memory of returned data
   */

  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *) (cache->cache_params);

  cache_obj_t *cache_obj = g_queue_pop_head(LRUSize_params->list);
  if (cache->core->used_size < cache_obj->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long) cache->core->used_size,
          (unsigned long long) cache_obj->size);
    abort();
  }

  cache->core->used_size -= cache_obj->size;

  gpointer evicted_key;
  if (req->label_type == 'l') {
    evicted_key = (gpointer) g_new(guint64, 1);
    *(guint64 *) evicted_key = *(guint64 *) (cache_obj->key);
  } else {
    evicted_key = (gpointer) g_strdup((gchar *) (cache_obj->key));
  }

  g_hash_table_remove(LRUSize_params->hashtable,
                      (gconstpointer) (cache_obj->key));
  g_free(cache_obj);
  return evicted_key;
}

gboolean LRUSize_add_element(cache_t *cache, request_t *req) {
  LRUSize_params_t *LRUSize_params = (struct LRUSize_params *) (cache->cache_params);
  if (req->size == 0) {
    ERROR("LRUSize get size zero for request\n");
    abort();
  }

#ifdef TRACK_EVICTION_AGE
  gpointer key = (gpointer)g_new(guint64, 1);
  gpointer key2 = (gpointer)g_new(guint64, 1);
  *(guint64 *)(key) = *(guint64 *)(req->label_ptr);
  *(guint64 *)(key2) = *(guint64 *)(req->label_ptr);
  g_hash_table_insert(LRUSize_params->last_access_rtime_map, key,
                      GINT_TO_POINTER(req->real_time)+1);
//  printf("real time %lld\n", req->real_time);
  g_hash_table_insert(LRUSize_params->last_access_vtime_map, key2,
                      GINT_TO_POINTER(LRUSize_params->ts));
#endif


  gboolean exist = LRUSize_check_element(cache, req);


  if (req->size <= cache->core->size){
    if (exist)
      __LRUSize_update_element(cache, req);
    else
      __LRUSize_insert_element(cache, req);

    while (cache->core->used_size > cache->core->size)
      __LRUSize_evict_element(cache, req);
  }

  LRUSize_params->ts++;
  cache->core->ts += 1;


//  if (cache->core->ts > 540){
//    guint64 key = 502;
//    GList *node = (GList *) g_hash_table_lookup(LRUSize_params->hashtable, &key);
//    cache_obj_t *cache_obj = node->data;
//    printf("502 access_time %llu %llu %llu %p\n", cache_obj->access_time, *(guint64*)(cache_obj->key), cache_obj->size, cache_obj);
//  }

  return exist;
}

void LRUSize_destroy(cache_t *cache) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *) (cache->cache_params);

  g_hash_table_destroy(LRUSize_params->hashtable);
//  g_queue_free_full(LRUSize_params->list, simple_g_key_value_destroyer);
  g_queue_free_full(LRUSize_params->list, cacheobj_destroyer);
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
      (struct LRUSize_params *) (cache->cache_params);

  cache->core->type = e_LRUSize;
  cache->core->cache_init = LRUSize_init;
  cache->core->destroy = LRUSize_destroy;
  cache->core->destroy_unique = LRUSize_destroy_unique;
  cache->core->add_element = LRUSize_add_element;
  cache->core->check_element = LRUSize_check_element;
  cache->core->get_cached_data = LRUSize_get_cached_data;
  cache->core->update_cached_data = LRUSize_update_cached_data;

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

#ifdef TRACK_EVICTION_AGE
  LRUSize_params->eviction_age_ofile = fopen((char *)params, "w");
  LRUSize_params->last_access_rtime_map = g_hash_table_new_full(
      g_int64_hash, g_int64_equal, simple_g_key_value_destroyer, NULL);
  LRUSize_params->last_access_vtime_map = g_hash_table_new_full(
      g_int64_hash, g_int64_equal, simple_g_key_value_destroyer, NULL);
#endif

  return cache;
}

void LRUSize_remove_element(cache_t *cache, void *data_to_remove) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *) (cache->cache_params);

  GList *node =
      (GList *) g_hash_table_lookup(LRUSize_params->hashtable, data_to_remove);
  if (!node) {
    fprintf(stderr,
            "LRUSize_remove_element: data to remove is not in the cache\n");
    abort();
  }
  if (cache->core->used_size < ((cache_obj_t *) (node->data))->size) {
    ERROR("occupied cache size %llu smaller than object size %llu\n",
          (unsigned long long) cache->core->used_size,
          (unsigned long long) ((cache_obj_t *) (node->data))->size);
    abort();
  }

  cache->core->used_size -= ((cache_obj_t *) (node->data))->size;
  g_queue_delete_link(LRUSize_params->list, (GList *) node);
  g_hash_table_remove(LRUSize_params->hashtable, data_to_remove);
}

guint64 LRUSize_get_size(cache_t *cache) {
  return (guint64) cache->core->used_size;
}

GHashTable *LRUSize_get_objmap(cache_t *cache) {
  LRUSize_params_t *LRUSize_params =
      (struct LRUSize_params *) (cache->cache_params);
  return LRUSize_params->hashtable;
}

#ifdef __reqlusplus
extern "C" {
#endif
