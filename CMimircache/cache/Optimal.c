//
//  optimal.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#include "Optimal.h"

#ifdef __reqlusplus
extern "C" {
#endif

/******************* priority queue structs and def **********************/

static int cmp_pri(pqueue_pri_t next, pqueue_pri_t curr) {
  return (next.pri1 < curr.pri1);
}

static pqueue_pri_t get_pri(void *a) { return ((pq_node_t *)a)->pri; }

static void set_pri(void *a, pqueue_pri_t pri) { ((pq_node_t *)a)->pri = pri; }

static size_t get_pos(void *a) { return ((pq_node_t *)a)->pos; }

static void set_pos(void *a, size_t pos) { ((pq_node_t *)a)->pos = pos; }

/*************************** OPT related ****************************/

void __optimal_insert_element(cache_t *optimal, request_t *req) {
  optimal_params_t *optimal_params =
      (optimal_params_t *)(optimal->cache_params);

  pq_node_t *node = g_new(pq_node_t, 1);
  gpointer key;
  if (req->label_type == 'l') {
    key = (gpointer)g_new(guint64, 1);
    *(guint64 *)key = *(guint64 *)(req->label_ptr);
  } else {
    key = (gpointer)g_strdup((gchar *)(req->label_ptr));
  }

  gint64 cur_ts = optimal_params->ts - 1;
  node->data_type = req->label_type;
  node->item = (gpointer)key;
  if ((gint)g_array_index(optimal_params->next_access, gint, cur_ts) == -1)
    node->pri.pri1 = G_MAXINT64;
  else
    node->pri.pri1 =
        cur_ts + (gint)g_array_index(optimal_params->next_access, gint, cur_ts);
  pqueue_insert(optimal_params->pq, (void *)node);
  g_hash_table_insert(optimal_params->hashtable, (gpointer)key, (gpointer)node);
}

gboolean optimal_check_element(cache_t *cache, request_t *req) {
  return g_hash_table_contains(
      ((optimal_params_t *)(cache->cache_params))->hashtable,
      (gconstpointer)(req->label_ptr));
}

void __optimal_update_element(cache_t *optimal, request_t *req) {
  optimal_params_t *optimal_params =
      (optimal_params_t *)(optimal->cache_params);
  void *node;
  node = (void *)g_hash_table_lookup(optimal_params->hashtable,
                                     (gconstpointer)(req->label_ptr));
  pqueue_pri_t pri;

  gint64 cur_ts = optimal_params->ts - 1;
  if ((gint)g_array_index(optimal_params->next_access, gint, cur_ts) == -1)
    pri.pri1 = G_MAXINT64;
  else
    pri.pri1 =
        cur_ts + (gint)g_array_index(optimal_params->next_access, gint, cur_ts);

  pqueue_change_priority(optimal_params->pq, pri, node);
}

void __optimal_evict_element(cache_t *optimal, request_t *req) {
  optimal_params_t *optimal_params =
      (optimal_params_t *)(optimal->cache_params);
  gint64 cur_ts = optimal_params->ts - 1;

  pq_node_t *node = (pq_node_t *)pqueue_pop(optimal_params->pq);
  if (optimal->core->record_level == 1) {
    // save eviction
    if (req->label_type == 'l') {
      ((guint64 *)(optimal->core->eviction_array))[cur_ts] =
          *(guint64 *)(node->item);
    } else {
      gchar *key = g_strdup((gchar *)(node->item));
      ((gchar **)(optimal->core->eviction_array))[cur_ts] = key;
    }
  }

  g_hash_table_remove(optimal_params->hashtable, (gconstpointer)(node->item));
}

void *__optimal_evict_with_return(cache_t *optimal, request_t *req) {
  optimal_params_t *optimal_params =
      (optimal_params_t *)(optimal->cache_params);

  void *evicted_key;
  pq_node_t *node = (pq_node_t *)pqueue_pop(optimal_params->pq);

  if (req->label_type == 'l') {
    evicted_key = (gpointer)g_new(guint64, 1);
    *(guint64 *)evicted_key = *(guint64 *)(node->item);
  } else {
    evicted_key = (gpointer)g_strdup((gchar *)(node->item));
  }

  g_hash_table_remove(optimal_params->hashtable, (gconstpointer)(node->item));
  return evicted_key;
}

guint64 optimal_get_size(cache_t *cache) {
  optimal_params_t *optimal_params = (optimal_params_t *)(cache->cache_params);
  return (guint64)g_hash_table_size(optimal_params->hashtable);
}

gboolean optimal_add_element(cache_t *cache, request_t *req) {
  optimal_params_t *optimal_params = (optimal_params_t *)(cache->cache_params);
  gboolean retval;
  (optimal_params->ts)++;

  if (optimal_check_element(cache, req)) {
    __optimal_update_element(cache, req);

    if (cache->core->record_level == 1)
      if ((gint)g_array_index(optimal_params->next_access, gint,
                              optimal_params->ts - 1) == -1)
        __optimal_evict_element(cache, req);

    retval = TRUE;
  } else {
    __optimal_insert_element(cache, req);

    if (cache->core->record_level == 1) {
      if ((gint)g_array_index(optimal_params->next_access, gint,
                              optimal_params->ts - 1) == -1)
        __optimal_evict_element(cache, req);
      else if ((long)g_hash_table_size(optimal_params->hashtable) >
               cache->core->size)
        __optimal_evict_element(cache, req);
    } else {
      if ((long)g_hash_table_size(optimal_params->hashtable) >
          cache->core->size)
        __optimal_evict_element(cache, req);
    }
    retval = FALSE;
  }
  cache->core->ts += 1;
  return retval;
}

gboolean optimal_add_element_withsize(cache_t *cache, request_t *req) {
  ERROR("optimal does not support size now\n");
  abort();

  int i, n = 0;
  gint64 original_lbn = *(gint64 *)(req->label_ptr);
  gboolean ret_val;

  if (cache->core->block_size != 0) {
    *(gint64 *)(req->label_ptr) =
        (gint64)(*(gint64 *)(req->label_ptr) * req->disk_sector_size /
                 cache->core->block_size);
    n = (int)ceil((double)req->size / cache->core->block_size);
  }
  ret_val = optimal_add_element(cache, req);

  if (cache->core->block_size != 0) {
    for (i = 0; i < n - 1; i++) {
      (*(guint64 *)(req->label_ptr))++;
      optimal_add_element_only(cache, req);
    }
  }

  *(gint64 *)(req->label_ptr) = original_lbn;
  return ret_val;
}

gboolean optimal_add_element_only(cache_t *cache, request_t *req) {
  optimal_params_t *optimal_params = (optimal_params_t *)(cache->cache_params);
  (optimal_params->ts)++; // do not move
  gboolean retval;

  if (optimal_check_element(cache, req)) {
    __optimal_update_element(cache, req);
    retval = TRUE;
  } else {
    __optimal_insert_element(cache, req);
    if ((long)g_hash_table_size(optimal_params->hashtable) > cache->core->size)
      __optimal_evict_element(cache, req);
    retval = FALSE;
  }
  cache->core->ts += 1;
  return retval; 
}

void optimal_destroy(cache_t *cache) {
  optimal_params_t *optimal_params = (optimal_params_t *)(cache->cache_params);

  g_hash_table_destroy(optimal_params->hashtable);
  pqueue_free(optimal_params->pq);
  g_array_free(optimal_params->next_access, TRUE);
  ((struct optimal_init_params *)(cache->core->cache_init_params))
      ->next_access = NULL;

  cache_destroy(cache);
}

void optimal_destroy_unique(cache_t *cache) {
  /* the difference between destroy_unique and destroy
   is that the former one only free the resources that are
   unique to the cache, freeing these resources won't affect
   other caches copied from original cache
   in Optimal, next_access should not be freed in destroy_unique,
   because it is shared between different caches copied from the original one.
   */

  optimal_params_t *optimal_params = (optimal_params_t *)(cache->cache_params);
  g_hash_table_destroy(optimal_params->hashtable);
  pqueue_free(optimal_params->pq);
  g_free(cache->cache_params);
  g_free(cache->core);
  g_free(cache);
}

cache_t *optimal_init(guint64 size, char data_type, guint64 block_size,
                      void *params) {
  cache_t *cache = cache_init(size, data_type, block_size);

  optimal_params_t *optimal_params = g_new0(optimal_params_t, 1);
  cache->cache_params = (void *)optimal_params;

  cache->core->type = e_Optimal;
  cache->core->cache_init = optimal_init;
  cache->core->destroy = optimal_destroy;
  cache->core->destroy_unique = optimal_destroy_unique;
  cache->core->add_element = optimal_add_element;
  cache->core->check_element = optimal_check_element;

  cache->core->__insert_element = __optimal_insert_element;
  cache->core->__update_element = __optimal_update_element;
  cache->core->__evict_element = __optimal_evict_element;
  cache->core->__evict_with_return = __optimal_evict_with_return;
  cache->core->get_current_size = optimal_get_size;
  cache->core->add_element_only = optimal_add_element_only;
  cache->core->add_element_withsize = optimal_add_element_withsize;

  optimal_params->ts = ((struct optimal_init_params *)params)->ts;

  reader_t *reader = ((struct optimal_init_params *)params)->reader;
  optimal_params->reader = reader;

  if (data_type == 'l') {
    optimal_params->hashtable = g_hash_table_new_full(
        g_int64_hash, g_int64_equal, simple_g_key_value_destroyer,
        simple_g_key_value_destroyer);
  }

  else if (data_type == 'c') {
    optimal_params->hashtable = g_hash_table_new_full(
        g_str_hash, g_str_equal, simple_g_key_value_destroyer,
        simple_g_key_value_destroyer);
  } else {
    ERROR("does not support given data label_type: %c\n", data_type);
  }

  optimal_params->pq =
      pqueue_init(size, cmp_pri, get_pri, set_pri, get_pos, set_pos);

  if (((struct optimal_init_params *)params)->next_access == NULL) {
    if (reader->base->total_num == -1)
      get_num_of_req(reader);
    optimal_params->next_access = g_array_sized_new(
        FALSE, FALSE, sizeof(gint), (guint)reader->base->total_num);
    GArray *array = optimal_params->next_access;
    GSList *list = get_last_access_dist_seq(reader, read_one_element_above);
    if (list == NULL) {
      ERROR("error getting last access distance in optimal_init\n");
      exit(1);
    }
    GSList *list_move = list;

    gint dist = (GPOINTER_TO_INT(list_move->data));
    g_array_append_val(array, dist);
    while ((list_move = g_slist_next(list_move)) != NULL) {
      dist = (GPOINTER_TO_INT(list_move->data));
      g_array_append_val(array, dist);
    }
    g_slist_free(list);

    ((struct optimal_init_params *)params)->next_access =
        optimal_params->next_access;
  } else
    optimal_params->next_access =
        ((struct optimal_init_params *)params)->next_access;

  cache->core->cache_init_params = params;

  return cache;
}

#ifdef __reqlusplus
}
#endif
