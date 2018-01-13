//
//  LRUSize.c
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//


#include "cache.h" 
#include "LRUSize.h"

#ifdef __cplusplus
extern "C"
{
#endif



void __LRUSize_insert_element(struct_cache* LRUSize, cache_line* cp){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(LRUSize->cache_params);
    
    gpointer key;
    if (cp->type == 'l'){
        key = (gpointer)g_new(guint64, 1);
        *(guint64*)key = *(guint64*)(cp->item_p);
    }
    else{
        key = (gpointer)g_strdup((gchar*)(cp->item_p));
    }
    
    GList* node = g_list_alloc();
    node->data = key;
    
    
    g_queue_push_tail_link(LRUSize_params->list, node);
    g_hash_table_insert(LRUSize_params->hashtable, (gpointer)key, (gpointer)node);
    
}

gboolean LRUSize_check_element(struct_cache* cache, cache_line* cp){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(cache->cache_params);
    return g_hash_table_contains( LRUSize_params->hashtable, cp->item_p );
}


void __LRUSize_update_element(struct_cache* cache, cache_line* cp){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(cache->cache_params);
    GList* node = (GList* ) g_hash_table_lookup(LRUSize_params->hashtable, cp->item_p);
    g_queue_unlink(LRUSize_params->list, node);
    g_queue_push_tail_link(LRUSize_params->list, node);
}


void __LRUSize_evict_element(struct_cache* LRUSize, cache_line* cp){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(LRUSize->cache_params);

    gpointer data = g_queue_pop_head(LRUSize_params->list);
    g_hash_table_remove(LRUSize_params->hashtable, (gconstpointer)data);
}


gpointer __LRUSize__evict_with_return(struct_cache* LRUSize, cache_line* cp){
    /** evict one element and return the evicted element, 
     * needs to free the memory of returned data 
     */
    
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(LRUSize->cache_params);
    
    gpointer data = g_queue_pop_head(LRUSize_params->list);
    
    gpointer evicted_key;
    if (cp->type == 'l'){
        evicted_key = (gpointer)g_new(guint64, 1);
        *(guint64*)evicted_key = *(guint64*)(data);
    }
    else{
        evicted_key = (gpointer)g_strdup((gchar*)data);
    }

    g_hash_table_remove(LRUSize_params->hashtable, (gconstpointer)data);
    return evicted_key;
}




gboolean LRUSize_add_element(struct_cache* cache, cache_line* cp){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(cache->cache_params);
    if (cp->size !=0 && cp->type != 'l'){
        ERROR("LRU size get non-zero size, but data type is %c\n", cp->type);
        exit(1);
    }
    int i, n = (int)(cp->size / 512);
    guint64 original_block = *(guint64*)(cp->item_p);
    gboolean exist = LRUSize_check_element(cache, cp);
    
    for (i=0; i<n; i++){
        if (LRUSize_check_element(cache, cp)){
            __LRUSize_update_element(cache, cp);
        }
        else{
            __LRUSize_insert_element(cache, cp);
            if ((long)g_hash_table_size(LRUSize_params->hashtable) > cache->core->size)
                __LRUSize_evict_element(cache, cp);
        }

        
        (*(guint64*)(cp->item_p)) ++;
    }
    LRUSize_params->ts ++;
    *(guint64*)(cp->item_p) = original_block;
    return exist; 
}




void LRUSize_destroy(struct_cache* cache){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(cache->cache_params);

//    g_queue_free(LRUSize_params->list);                 // Jason: should call g_queue_free_full to free the memory of node content
    // 0921
    g_queue_free(LRUSize_params->list);
    g_hash_table_destroy(LRUSize_params->hashtable);
    cache_destroy(cache);
}

void LRUSize_destroy_unique(struct_cache* cache){
    /* the difference between destroy_unique and destroy
     is that the former one only free the resources that are
     unique to the cache, freeing these resources won't affect
     other caches copied from original cache
     in Optimal, next_access should not be freed in destroy_unique,
     because it is shared between different caches copied from the original one.
     */
    
    LRUSize_destroy(cache);
}


struct_cache* LRUSize_init(guint64 size, char data_type, int block_size, void* params){
    struct_cache *cache = cache_init(size, data_type, block_size);
    cache->cache_params = g_new0(struct LRUSize_params, 1);
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(cache->cache_params);
    
    cache->core->type                   =   e_LRUSize;
    cache->core->cache_init             =   LRUSize_init;
    cache->core->destroy                =   LRUSize_destroy;
    cache->core->destroy_unique         =   LRUSize_destroy_unique;
    cache->core->add_element            =   LRUSize_add_element;
    cache->core->check_element          =   LRUSize_check_element;
    cache->core->__insert_element       =   __LRUSize_insert_element;
    cache->core->__update_element       =   __LRUSize_update_element;
    cache->core->__evict_element        =   __LRUSize_evict_element;
    cache->core->__evict_with_return    =   __LRUSize__evict_with_return;
    cache->core->get_size               =   LRUSize_get_size;
    cache->core->remove_element         =   LRUSize_remove_element; 
    cache->core->cache_init_params      =   NULL;

    if (data_type == 'l'){
        LRUSize_params->hashtable = g_hash_table_new_full(g_int64_hash, g_int64_equal, simple_g_key_value_destroyer, NULL);
    }
    else if (data_type == 'c'){
        LRUSize_params->hashtable = g_hash_table_new_full(g_str_hash, g_str_equal, simple_g_key_value_destroyer, NULL);
    }
    else{
        ERROR("does not support given data type: %c\n", data_type);
    }
    LRUSize_params->list = g_queue_new();
    
    
    return cache;
}




void LRUSize_remove_element(struct_cache* cache, void* data_to_remove){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(cache->cache_params);
    
    gpointer data = g_hash_table_lookup(LRUSize_params->hashtable, data_to_remove);
    if (!data){
        fprintf(stderr, "LRUSize_remove_element: data to remove is not in the cache\n");
        exit(1);
    }
    g_queue_delete_link(LRUSize_params->list, (GList*) data);
    g_hash_table_remove(LRUSize_params->hashtable, data_to_remove);
}

gint64 LRUSize_get_size(struct_cache* cache){
    struct LRUSize_params* LRUSize_params = (struct LRUSize_params*)(cache->cache_params);
    return (guint64) g_hash_table_size(LRUSize_params->hashtable);
}


#ifdef __cplusplus
extern "C"
{
#endif

