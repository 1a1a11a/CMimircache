////
////  LRFU.h
////  mimircache
////
////  Created by Juncheng on 2/12/17.
////  Copyright © 2017 Juncheng. All rights reserved.
////
//
//
//#include "LRFU.h"
//
//
///******************* priority queue structs and def **********************/
//
//static int cmp_pri(pqueue_pri_t next, pqueue_pri_t curr){
//    return (next > curr);
//}
//
//
//static pqueue_pri_t get_pri(void *a){
//    return ((pq_node_t *) a)->pri;
//}
//
//
//static void set_pri(void *a, pqueue_pri_t pri){
//    ((pq_node_t *) a)->pri = pri;
//}
//
//
//static size_t get_pos(void *a){
//    return ((pq_node_t *) a)->pos;
//}
//
//
//static void set_pos(void *a, size_t pos){
//    ((pq_node_t *) a)->pos = pos;
//}
//
//
//
//
///*************************** LRFU related ****************************/
//
//void __LRFU_insert_element(struct_cache* LRFU, cache_line* cp){
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(LRFU->cache_params);
//    
//    pq_node_t *node = g_new(pq_node_t, 1);
//    gpointer key;
//    if (cp->type == 'l'){
//        key = (gpointer)g_new(guint64, 1);
//        *(guint64*)key = *(guint64*)(cp->item_p);
//    }
//    else{
//        key = (gpointer)g_strdup((gchar*)(cp->item_p));
//    }
//    
//    node->data_type = cp->type;
//    node->item = (gpointer)key;
//    if ((gint)g_array_index(LRFU_params->next_access, gint, LRFU_params->ts) == -1)
//        node->pri = G_MAXUINT64;
//    else
//        node->pri = LRFU_params->ts + (gint)g_array_index(LRFU_params->next_access, gint, LRFU_params->ts);
//    pqueue_insert(LRFU_params->pq, (void *)node);
//    g_hash_table_insert (LRFU_params->hashtable, (gpointer)key, (gpointer)node);
//}
//
//
//gboolean LRFU_check_element(struct_cache* cache, cache_line* cp){
//    return g_hash_table_contains(
//                                 ((LRFU_params_t*)(cache->cache_params))->hashtable,
//                                 (gconstpointer)(cp->item_p)
//                                 );
//}
//
//
//void __LRFU_update_element(struct_cache* LRFU, cache_line* cp){
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(LRFU->cache_params);
//    void* node;
//    node = (void*) g_hash_table_lookup(LRFU_params->hashtable, (gconstpointer)(cp->item_p));
//    
//    if ((gint) g_array_index(LRFU_params->next_access, gint, LRFU_params->ts) == -1)
//        pqueue_change_priority(LRFU_params->pq, G_MAXUINT64, node);
//    else
//        pqueue_change_priority(LRFU_params->pq,
//                               LRFU_params->ts +
//                               (gint)g_array_index(LRFU_params->next_access, gint, LRFU_params->ts),
//                               node);
//}
//
//
//
//void __LRFU_evict_element(struct_cache* LRFU, cache_line* cp){
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(LRFU->cache_params);
//    
//    pq_node_t* node = (pq_node_t*) pqueue_pop(LRFU_params->pq);
//    if (LRFU->core->cache_debug_level == 1){
//        // save eviction
//        if (cp->type == 'l'){
//            ((guint64*)(LRFU->core->eviction_array))[LRFU_params->ts] = *(guint64*)(node->item);
//        }
//        else{
//            gchar* key = g_strdup((gchar*)(node->item));
//            ((gchar**)(LRFU->core->eviction_array))[LRFU_params->ts] = key;
//        }
//    }
//    
//    g_hash_table_remove(LRFU_params->hashtable, (gconstpointer)(node->item));
//}
//
//
//void* __LRFU_evict_with_return(struct_cache* LRFU, cache_line* cp){
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(LRFU->cache_params);
//    
//    void* evicted_key;
//    pq_node_t* node = (pq_node_t*) pqueue_pop(LRFU_params->pq);
//    
//    if (cp->type == 'l'){
//        evicted_key = (gpointer)g_new(guint64, 1);
//        *(guint64*)evicted_key = *(guint64*)(node->item);
//    }
//    else{
//        evicted_key = (gpointer)g_strdup((gchar*)(node->item));
//    }
//    
//    g_hash_table_remove(LRFU_params->hashtable, (gconstpointer)(node->item));
//    return evicted_key;
//}
//
//
//
//gboolean LRFU_add_element(struct_cache* cache, cache_line* cp){
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(cache->cache_params);
//    
//    if (LRFU_check_element(cache, cp)){
//        __LRFU_update_element(cache, cp);
//        
//        if (cache->core->cache_debug_level == 1)
//            if ((gint)g_array_index(LRFU_params->next_access, gint, LRFU_params->ts) == -1)
//                __LRFU_evict_element(cache, cp);
//        
//        (LRFU_params->ts) ++ ;
//        return TRUE;
//    }
//    else{
//        __LRFU_insert_element(cache, cp);
//        
//        if (cache->core->cache_debug_level == 1)
//            if ((gint)g_array_index(LRFU_params->next_access, gint, LRFU_params->ts) == -1)
//                __LRFU_evict_element(cache, cp);
//        
//        if ( (long)g_hash_table_size( LRFU_params->hashtable) > cache->core->size)
//            __LRFU_evict_element(cache, cp);
//        (LRFU_params->ts) ++ ;
//        return FALSE;
//    }
//}
//
//
//void LRFU_destroy(struct_cache* cache){
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(cache->cache_params);
//    
//    g_hash_table_destroy(LRFU_params->hashtable);
//    pqueue_free(LRFU_params->pq);
//    g_array_free (LRFU_params->next_access, TRUE);
//    ((struct LRFU_init_params*)(cache->core->cache_init_params))->next_access = NULL;
//    
//    cache_destroy(cache);
//}
//
//
//void LRFU_destroy_unique(struct_cache* cache){
//    /* the difference between destroy_unique and destroy
//     is that the former one only free the resources that are
//     unique to the cache, freeing these resources won't affect
//     other caches copied from original cache
//     in LRFU, next_access should not be freed in destroy_unique,
//     because it is shared between different caches copied from the original one.
//     */
//    
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(cache->cache_params);
//    g_hash_table_destroy(LRFU_params->hashtable);
//    pqueue_free(LRFU_params->pq);
//    g_free(cache->cache_params);
//    g_free(cache->core);
//    g_free(cache);
//}
//
//
//
//struct_cache* LRFU_init(guint64 size, char data_type, void* params){
//    struct_cache* cache = cache_init(size, data_type);
//    
//    LRFU_params_t* LRFU_params = g_new0(struct LRFU_params, 1);
//    cache->cache_params = (void*) LRFU_params;
//    
//    cache->core->type                   =   e_LRFU;
//    cache->core->cache_init             =   LRFU_init;
//    cache->core->destroy                =   LRFU_destroy;
//    cache->core->destroy_unique         =   LRFU_destroy_unique;
//    cache->core->add_element            =   LRFU_add_element;
//    cache->core->check_element          =   LRFU_check_element;
//    
//    cache->core->__insert_element       =   __LRFU_insert_element;
//    cache->core->__update_element       =   __LRFU_update_element;
//    cache->core->__evict_element        =   __LRFU_evict_element;
//    cache->core->__evict_with_return    =   __LRFU_evict_with_return;
//    cache->core->get_size               =   LRFU_get_size;
//    cache->core->cache_init_params      =   NULL;
//    
//    
//    if (data_type == 'l'){
//        LRFU_params->hashtable =
//        g_hash_table_new_full(g_int64_hash, g_int64_equal,
//                              simple_g_key_value_destroyer,
//                              simple_g_key_value_destroyer);
//    }
//    
//    else if (data_type == 'c'){
//        LRFU_params->hashtable =
//        g_hash_table_new_full(g_str_hash, g_str_equal,
//                              simple_g_key_value_destroyer,
//                              simple_g_key_value_destroyer);
//    }
//    else{
//        g_error("does not support given data type: %c\n", data_type);
//    }
//    
//    
//    LRFU_params->pq = pqueue_init(size, cmp_pri, get_pri, set_pri, get_pos, set_pos);
//    return cache;
//}
//
//
//uint64_t LRFU_get_size(struct_cache* cache){
//    LRFU_params_t* LRFU_params = (LRFU_params_t*)(cache->cache_params);
//    return (uint64_t)g_hash_table_size(LRFU_params->hashtable);
//}
//
