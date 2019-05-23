//
//  a FIFO module that supports different obj size
//
//
//  FIFOSize.c
//  CMimircache
//
//  Created by Juncheng on 12/4/18.
//  Copyright © 2018 Juncheng. All rights reserved.
//


#include "cache.h"
#include "FIFOSize.h"

#ifdef __cplusplus
extern "C"
{
#endif

void __FIFOSize_insert_element(struct_cache *cache, cache_line *cp) {
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);
    cache_obj_t *cache_obj = g_new(cache_obj_t, 1);
    cache_obj->size = cp->size;
    cache->core->occupied_size += cp->size;

    if (cp->type == 'l') {
        cache_obj->key = (gpointer) g_new(guint64, 1);
        *(guint64 *) (cache_obj->key) = *(guint64 *) (cp->item_p);
    } else {
        cache_obj->key = (gpointer) g_strdup((gchar *) (cp->item_p));
    }

    GList *node = g_list_alloc();
    node->data = cache_obj;

    g_queue_push_tail_link(FIFOSize_params->list, node);
    g_hash_table_insert(FIFOSize_params->hashtable, cache_obj->key, (gpointer) node);

}

gboolean FIFOSize_check_element(struct_cache *cache, cache_line *cp) {
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);
    return g_hash_table_contains(FIFOSize_params->hashtable, cp->item_p);
}


void __FIFOSize_update_element(struct_cache *cache, cache_line *cp) {
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);
    GList *node = (GList *) g_hash_table_lookup(FIFOSize_params->hashtable, cp->item_p);

    cache_obj_t *cache_obj = node->data;
    if (cache_obj->size != cp->size){
        ERROR("updating in FIFO is impossible, it will only result in a new entry");
    }
}


void __FIFOSize_evict_element(struct_cache *cache, cache_line *cp) {
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);

    cache_obj_t *cache_obj = (cache_obj_t *) g_queue_pop_head(FIFOSize_params->list);

    if (cache->core->occupied_size < cache_obj->size) {
        ERROR("occupied cache size %llu smaller than object size %llu\n",
              (unsigned long long) cache->core->occupied_size, (unsigned long long) cache_obj->size);
        abort();
    }
    cache->core->occupied_size -= cache_obj->size;
    g_hash_table_remove(FIFOSize_params->hashtable, (gconstpointer) cache_obj->key);
    g_free(cache_obj);
}


gpointer __FIFOSize__evict_with_return(struct_cache *cache, cache_line *cp) {
    /** evict one element and return the evicted element,
     * needs to free the memory of returned data
     */

    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);

    cache_obj_t *cache_obj = g_queue_pop_head(FIFOSize_params->list);
    if (cache->core->occupied_size < cache_obj->size) {
        ERROR("occupied cache size %llu smaller than object size %llu\n",
              (unsigned long long) cache->core->occupied_size, (unsigned long long) cache_obj->size);
        abort();
    }

    cache->core->occupied_size -= cache_obj->size;

    gpointer evicted_key;
    if (cp->type == 'l') {
        evicted_key = (gpointer) g_new(guint64, 1);
        *(guint64 *) evicted_key = *(guint64 *) (cache_obj->key);
    } else {
        evicted_key = (gpointer) g_strdup((gchar *) (cache_obj->key));
    }

    g_hash_table_remove(FIFOSize_params->hashtable, (gconstpointer) (cache_obj->key));
    g_free(cache_obj);
    return evicted_key;
}


gboolean FIFOSize_add_element(struct_cache *cache, cache_line *cp) {
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);
    if (cp->size == 0) {
        ERROR("FIFOSize get size zero for request\n");
        abort();
    }

    gboolean exist = FIFOSize_check_element(cache, cp);

    if (FIFOSize_check_element(cache, cp))
        __FIFOSize_update_element(cache, cp);
    else
        __FIFOSize_insert_element(cache, cp);

    while (cache->core->occupied_size > cache->core->size)
        __FIFOSize_evict_element(cache, cp);

    FIFOSize_params->ts++;
    return exist;
}


void FIFOSize_destroy(struct_cache *cache) {
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);

    g_hash_table_destroy(FIFOSize_params->hashtable);
//    g_queue_free(FIFOSize_params->list);
    g_queue_free_full(FIFOSize_params->list, simple_g_key_value_destroyer);
    cache_destroy(cache);
}

void FIFOSize_destroy_unique(struct_cache *cache) {
    /* the difference between destroy_unique and destroy
     is that the former one only free the resources that are
     unique to the cache, freeing these resources won't affect
     other caches copied from original cache
     in Optimal, next_access should not be freed in destroy_unique,
     because it is shared between different caches copied from the original one.
     */

    FIFOSize_destroy(cache);
}


struct_cache *FIFOSize_init(guint64 size, char data_type, guint64 block_size, void *params) {
    struct_cache *cache = cache_init(size, data_type, block_size);
    cache->cache_params = g_new0(struct FIFOSize_params, 1);
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);

    cache->core->type = e_FIFOSize;
    cache->core->cache_init = FIFOSize_init;
    cache->core->destroy = FIFOSize_destroy;
    cache->core->destroy_unique = FIFOSize_destroy_unique;
    cache->core->add_element = FIFOSize_add_element;
    cache->core->check_element = FIFOSize_check_element;
    cache->core->__insert_element = __FIFOSize_insert_element;
    cache->core->__update_element = __FIFOSize_update_element;
    cache->core->__evict_element = __FIFOSize_evict_element;
    cache->core->__evict_with_return = __FIFOSize__evict_with_return;
    cache->core->get_size = FIFOSize_get_size;
    cache->core->get_objmap = FIFOSize_get_objmap;
    cache->core->remove_element = FIFOSize_remove_element;
    cache->core->cache_init_params = NULL;

    if (data_type == 'l') {
        FIFOSize_params->hashtable = g_hash_table_new_full(g_int64_hash, g_int64_equal, simple_g_key_value_destroyer, NULL);
    } else if (data_type == 'c') {
        FIFOSize_params->hashtable = g_hash_table_new_full(g_str_hash, g_str_equal, simple_g_key_value_destroyer, NULL);
    } else {
        ERROR("does not support given data type: %c\n", data_type);
    }
    FIFOSize_params->list = g_queue_new();


    return cache;
}


void FIFOSize_remove_element(struct_cache *cache, void *data_to_remove) {
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);

    GList *node = (GList *) g_hash_table_lookup(FIFOSize_params->hashtable, data_to_remove);
    if (!node) {
        fprintf(stderr, "FIFOSize_remove_element: data to remove is not in the cache\n");
        abort();
    }
    if (cache->core->occupied_size < ((cache_obj_t *) (node->data))->size) {
        ERROR("occupied cache size %llu smaller than object size %llu\n",
              (unsigned long long) cache->core->occupied_size,
              (unsigned long long) ((cache_obj_t *) (node->data))->size);
        abort();
    }

    cache->core->occupied_size -= ((cache_obj_t *) (node->data))->size;
    g_queue_delete_link(FIFOSize_params->list, (GList *) node);
    g_hash_table_remove(FIFOSize_params->hashtable, data_to_remove);
}

guint64 FIFOSize_get_size(struct_cache *cache) {
    return (guint64) cache->core->occupied_size;
}

GHashTable* FIFOSize_get_objmap(struct_cache *cache){
    struct FIFOSize_params *FIFOSize_params = (struct FIFOSize_params *) (cache->cache_params);
    return FIFOSize_params->hashtable;
}


#ifdef __cplusplus
extern "C"
{
#endif

