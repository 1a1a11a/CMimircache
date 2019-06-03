//
//  PG.h
//  mimircache
//
//  Created by Juncheng on 11/20/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#include "PG.h"
#include "FIFO.h"
#include "LRU.h"
#include "Optimal.h"
#include "cache.h"

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

/****************************** UTILS ************************************/
static void graphNode_destroy(gpointer data) {
  graphNode_t *graphNode = (graphNode_t *)data;
  g_hash_table_destroy(graphNode->graph);
  pqueue_free(graphNode->pq);
  g_free(graphNode);
}

static inline void __PG_add_to_graph(cache_t *PG, request_t *req) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  int i;
  guint64 block, current_block = 0;
  char req_lbl[REQ_LABEL_MAX_LEN], current_req_lbl[REQ_LABEL_MAX_LEN];
  graphNode_t *graphNode = NULL;

  if (PG->core->data_type == 'l') {
    current_block =
        get_Nth_past_request_l(PG_params, PG_params->past_request_pointer);
    if (current_block)
      graphNode =
          (graphNode_t *)g_hash_table_lookup(PG_params->graph, &current_block);
  }
  if (PG->core->data_type == 'c') {
    get_Nth_past_request_c(PG_params, PG_params->past_request_pointer,
                           current_req_lbl);
    if (current_req_lbl[0])
      graphNode =
          (graphNode_t *)g_hash_table_lookup(PG_params->graph, current_req_lbl);
  }

  // now update past requests
  if (PG->core->data_type == 'l') {
    set_Nth_past_request_l(PG_params, PG_params->past_request_pointer++,
                           *(guint64 *)(req->label_ptr));
  }

  if (PG->core->data_type == 'c') {
    set_Nth_past_request_c(PG_params, PG_params->past_request_pointer++,
                           (req->label_ptr));
  }
  PG_params->past_request_pointer =
      PG_params->past_request_pointer % PG_params->lookahead;

  if (!(current_req_lbl[0] || current_block))
    // this is the first request
    return;

  if (graphNode == NULL) {
    if (!PG_params->stop_recording) {
      // current block is not in graph, insert
      gpointer key;
      graphNode = g_new0(graphNode_t, 1);

      if (req->label_type == 'l') {
        key = (gpointer)g_new(guint64, 1);
        *(guint64 *)key = current_block;
        graphNode->graph = g_hash_table_new_full(g_int64_hash, g_int64_equal,
                                                 simple_g_key_value_destroyer,
                                                 simple_g_key_value_destroyer);
      } else {
        key = (gpointer)g_strdup(current_req_lbl);
        graphNode->graph = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                 simple_g_key_value_destroyer,
                                                 simple_g_key_value_destroyer);
      }
      graphNode->pq =
          pqueue_init(2, cmp_pri, get_pri, set_pri, get_pos, set_pos);
      graphNode->total_count = 0;
      g_hash_table_insert(PG_params->graph, key, graphNode);
      PG_params->meta_data_size += (8 + 8 * 3);
    } else
      // no space for meta data
      return;
  }

  for (i = 0; i < PG_params->lookahead; i++) {
    graphNode->total_count++;
    if (PG->core->data_type == 'l') {
      block = get_Nth_past_request_l(PG_params, i);
      if (block == 0)
        break;

      pq_node_t *pq_node =
          (pq_node_t *)g_hash_table_lookup(graphNode->graph, &block);
      if (pq_node) {
        // relation already exists
        pq_node->pri.pri1++;
        pqueue_change_priority(graphNode->pq, pq_node->pri, pq_node);

#ifdef SANITY_CHECK
        if (*((gint64 *)(pq_node->label)) != (gint64)block)
          ERROR("pq node content not equal block\n");
#endif
      } else {
        // there is no probability between current_block->block
        if (!PG_params->stop_recording) {
          gpointer key = (gpointer)g_new(guint64, 1);
          *(guint64 *)key = block;

          pq_node_t *pq_node = g_new0(pq_node_t, 1);
          pq_node->data_type = 'l';
          pq_node->item = key;
          pq_node->pri.pri1 = 1;
          pqueue_insert(graphNode->pq, pq_node);
          g_hash_table_insert(graphNode->graph, key, pq_node);
          PG_params->meta_data_size += (8 + 8 * 3);
        } else
          return;
      }
    }
    if (PG->core->data_type == 'c') {
      get_Nth_past_request_c(PG_params, i, req_lbl);
      if (req_lbl[0] == 0)
        break;

      pq_node_t *pq_node =
          (pq_node_t *)g_hash_table_lookup(graphNode->graph, req_lbl);
      if (pq_node) {
        // relation already exists
        pq_node->pri.pri1++;
        pqueue_change_priority(graphNode->pq, pq_node->pri, pq_node);

        // sanity check
        if (strcmp((gchar *)(pq_node->item), req_lbl) != 0)
          printf("ERROR pq node content not equal req\n");
      } else {
        // there is no probability between current_block->block
        if (!PG_params->stop_recording) {
          gpointer key = (gpointer)g_strdup((gchar *)(req_lbl));

          pq_node_t *pq_node = g_new0(pq_node_t, 1);
          pq_node->data_type = 'c';
          pq_node->item = key;
          pq_node->pri.pri1 = 1;
          pqueue_insert(graphNode->pq, pq_node);
          g_hash_table_insert(graphNode->graph, key, pq_node);
          PG_params->meta_data_size += (8 + 8 * 3);
        } else
          return;
      }
    }
  }

  if (PG_params->init_size * PG_params->max_meta_data <=
      PG_params->meta_data_size / PG_params->block_size)
    PG_params->stop_recording = TRUE;

  PG->core->size = PG_params->init_size -
                   PG_params->meta_data_size / 1024 / PG_params->block_size;
  PG_params->cache->core->size =
      PG_params->init_size - PG_params->meta_data_size / PG_params->block_size;
}

static inline GList *PG_get_prefetch(cache_t *PG, request_t *req) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  GList *list = NULL;
  graphNode_t *graphNode = g_hash_table_lookup(PG_params->graph, req->label_ptr);

  if (graphNode == NULL)
    return list;

  GList *pq_node_list = NULL;
  while (1) {
    pq_node_t *pqNode = pqueue_pop(graphNode->pq);
    if (pqNode == NULL)
      break;
    if ((double)(pqNode->pri.pri1) / (graphNode->total_count) >
        PG_params->prefetch_threshold) {
      list = g_list_prepend(list, pqNode->item);
      pq_node_list = g_list_prepend(pq_node_list, pqNode);
    } else {
      //            printf("threshold %lf\n",
      //            (double)(pqNode->pri)/(graphNode->total_count));
      break;
    }
  }

  if (pq_node_list) {
    GList *node = pq_node_list;
    while (node) {
      pqueue_insert(graphNode->pq, node->data);
      node = node->next;
    }
  }
  g_list_free(pq_node_list);

  return list;
}

// static void printHashTable (gpointer key, gpointer value, gpointer
// user_data){
//    printf("key %s, value %p\n", (char*)key, value);
//}

/************************** probability graph ****************************/

void __PG_insert_element(cache_t *PG, request_t *req) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  PG_params->cache->core->__insert_element(PG_params->cache, req);
}

gboolean PG_check_element(cache_t *PG, request_t *req) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  return PG_params->cache->core->check_element(PG_params->cache, req);
}

void __PG_update_element(cache_t *PG, request_t *req) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  PG_params->cache->core->__update_element(PG_params->cache, req);
}

void __PG_evict_element(cache_t *PG, request_t *req) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  g_hash_table_remove(PG_params->prefetched, req->label_ptr);

  PG_params->cache->core->__evict_element(PG_params->cache, req);
}

gpointer __PG__evict_with_return(cache_t *PG, request_t *req) {
  /** evict one element and return the evicted element, needs to free the memory
   * of returned data **/
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  g_hash_table_remove(PG_params->prefetched, req->label_ptr);
  return PG_params->cache->core->__evict_with_return(PG_params->cache, req);
}

gboolean PG_add_element(cache_t *PG, request_t *req) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  __PG_add_to_graph(PG, req);

  if (g_hash_table_contains(PG_params->prefetched, req->label_ptr)) {
    PG_params->num_of_hit++;
    g_hash_table_remove(PG_params->prefetched, req->label_ptr);
    if (g_hash_table_contains(PG_params->prefetched, req->label_ptr))
      fprintf(stderr, "ERROR found prefetch\n");
  }

  gboolean retval;

  // original add_element
  if (PG_check_element(PG, req)) {
    __PG_update_element(PG, req);
    retval = TRUE;
  } else {
    __PG_insert_element(PG, req);
    while (PG_get_size(PG) > PG->core->size)
      __PG_evict_element(PG, req);
    retval = FALSE;
  }

  // begin prefetching
  GList *prefetch_list = PG_get_prefetch(PG, req);
  if (prefetch_list) {
    GList *node = prefetch_list;
    while (node) {
      req->label_ptr = node->data;
      if (!PG_check_element(PG, req)) {
        PG_params->cache->core->__insert_element(PG_params->cache, req);
        PG_params->num_of_prefetch += 1;

        gpointer item_p;
        if (req->label_type == 'l') {
          item_p = (gpointer)g_new(guint64, 1);
          *(guint64 *)item_p = *(guint64 *)(req->label_ptr);
        } else
          item_p = (gpointer)g_strdup((gchar *)(req->label_ptr));

        g_hash_table_insert(PG_params->prefetched, item_p, GINT_TO_POINTER(1));
      }
      node = node->next;
    }

    req->label_ptr = req->label;
    while (PG_get_size(PG) > PG->core->size)
      __PG_evict_element(PG, req);
    g_list_free(prefetch_list);
  }
  PG->core->ts += 1;
  return retval;
}

gboolean PG_add_element_only(cache_t *PG, request_t *req) {
  gboolean retval;
  if (PG_check_element(PG, req)) {
    __PG_update_element(PG, req);
    retval = TRUE;
  } else {
    __PG_insert_element(PG, req);
    while (PG_get_size(PG) > PG->core->size)
      __PG_evict_element(PG, req);
    retval = FALSE;
  }
  PG->core->ts += 1;
  return retval;
}

gboolean PG_add_element_withsize(cache_t *cache, request_t *req) {
  int i;
  gboolean ret_val;

  *(gint64 *)(req->label_ptr) =
      (gint64)(*(gint64 *)(req->label_ptr) * req->disk_sector_size /
               cache->core->block_size);
  ret_val = PG_add_element(cache, req);

  int n = (int)ceil((double)req->size / cache->core->block_size);

  for (i = 0; i < n - 1; i++) {
    (*(guint64 *)(req->label_ptr))++;
    PG_add_element_only(cache, req);
  }
  *(gint64 *)(req->label_ptr) -= (n - 1);
  cache->core->ts += 1;
  return ret_val;
}

void PG_destroy(cache_t *PG) {
  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  PG_params->cache->core->destroy(PG_params->cache);
  g_hash_table_destroy(PG_params->graph);
  g_hash_table_destroy(PG_params->prefetched);

  if (PG->core->data_type == 'c') {
    int i;
    for (i = 0; i < PG_params->lookahead; i++)
      g_free(((char **)(PG_params->past_requests))[i]);
  }
  g_free(PG_params->past_requests);
  cache_destroy(PG);
}

void PG_destroy_unique(cache_t *PG) {
  /* the difference between destroy_unique and destroy
   is that the former one only free the resources that are
   unique to the cache, freeing these resources won't affect
   other caches copied from original cache
   in Optimal, next_access should not be freed in destroy_unique,
   because it is shared between different caches copied from the original one.
   */

  PG_params_t *PG_params = (PG_params_t *)(PG->cache_params);
  PG_params->cache->core->destroy(PG_params->cache);
  g_hash_table_destroy(PG_params->graph);
  g_hash_table_destroy(PG_params->prefetched);

  if (PG->core->data_type == 'c') {
    int i;
    for (i = 0; i < PG_params->lookahead; i++)
      g_free(((char **)(PG_params->past_requests))[i]);
  }
  g_free(PG_params->past_requests);
  g_free(PG->cache_params);
  g_free(PG->core);
  g_free(PG);
}

cache_t *PG_init(guint64 size, char data_type, guint64 block_size,
                 void *params) {

  cache_t *cache = cache_init(size, data_type, block_size);
  cache->cache_params = g_new0(PG_params_t, 1);
  PG_params_t *PG_params = (PG_params_t *)(cache->cache_params);
  PG_init_params_t *init_params = (PG_init_params_t *)params;

  cache->core->type = e_PG;
  cache->core->cache_init = PG_init;
  cache->core->destroy = PG_destroy;
  cache->core->destroy_unique = PG_destroy_unique;
  cache->core->add_element = PG_add_element;
  cache->core->check_element = PG_check_element;
  cache->core->__insert_element = __PG_insert_element;
  cache->core->__update_element = __PG_update_element;
  cache->core->__evict_element = __PG_evict_element;
  cache->core->__evict_with_return = __PG__evict_with_return;
  cache->core->add_element_only = PG_add_element_only;
  cache->core->add_element_withsize = PG_add_element_withsize;

  cache->core->get_current_size = PG_get_size;
  cache->core->cache_init_params = params;

  PG_params->lookahead = init_params->lookahead;
  PG_params->prefetch_threshold = init_params->prefetch_threshold;
  PG_params->max_meta_data = init_params->max_meta_data;
  PG_params->init_size = size;
  PG_params->block_size = init_params->block_size;
  PG_params->stop_recording = FALSE;

  if (strcmp(init_params->cache_type, "LRU") == 0)
    PG_params->cache = LRU_init(size, data_type, block_size, NULL);
  else if (strcmp(init_params->cache_type, "FIFO") == 0)
    PG_params->cache = fifo_init(size, data_type, block_size, NULL);
  else if (strcmp(init_params->cache_type, "Optimal") == 0) {
    struct optimal_init_params *optimal_init_params =
        g_new(struct optimal_init_params, 1);
    optimal_init_params->reader = NULL;
    PG_params->cache = NULL;
  } else {
    fprintf(stderr, "can't recognize cache label_type: %s\n",
            init_params->cache_type);
    PG_params->cache = LRU_init(size, data_type, block_size, NULL);
  }

  if (data_type == 'l') {
    PG_params->graph =
        g_hash_table_new_full(g_int64_hash, g_int64_equal,
                              simple_g_key_value_destroyer, graphNode_destroy);
    PG_params->prefetched = g_hash_table_new_full(
        g_int64_hash, g_int64_equal, simple_g_key_value_destroyer, NULL);
    PG_params->past_requests = g_new0(guint64, PG_params->lookahead);
  } else if (data_type == 'c') {
    PG_params->graph =
        g_hash_table_new_full(g_str_hash, g_str_equal,
                              simple_g_key_value_destroyer, graphNode_destroy);
    PG_params->prefetched = g_hash_table_new_full(
        g_str_hash, g_str_equal, simple_g_key_value_destroyer, NULL);
    PG_params->past_requests = g_new0(char *, PG_params->lookahead);
    int i;
    for (i = 0; i < PG_params->lookahead; i++)
      ((char **)(PG_params->past_requests))[i] =
          g_new0(char, REQ_LABEL_MAX_LEN);
  } else {
    ERROR("does not support given data label_type: %c\n", data_type);
  }

  return cache;
}

guint64 PG_get_size(cache_t *cache) {
  PG_params_t *PG_params = (PG_params_t *)(cache->cache_params);
  return (guint64)PG_params->cache->core->get_current_size(PG_params->cache);
}

#ifdef __reqlusplus
}
#endif
