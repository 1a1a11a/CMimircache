//
//  cache.c
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//


#include "cache.h"


#ifdef __cplusplus
extern "C"
{
#endif



void cache_destroy(cache_t* cache){
    if (cache->cache_params){
        g_free(cache->cache_params);
        cache->cache_params = NULL;
    }
    
    /* cache->core->cache_init_params is on the stack default, 
     if it is on the heap, needs to be freed manually
     */
//    if (cache->core->cache_init_params){
//        g_free(cache->core->cache_init_params);
//        cache->core->cache_init_params = NULL;
//    }
    
    // This should not be freed, because it points to other's eviction_array, which should be freed only by others
//    if (cache->core->oracle){
//        g_free(cache->core->oracle);
//        cache->core->oracle = NULL;
//    }

    
    if (cache->core->eviction_array){
        
        if (cache->core->data_type == 'l')
            g_free((guint64*)cache->core->eviction_array);
        else{
            guint64 i;
            for (i=0; i<cache->core->eviction_array_len; i++)
                if ( ((gchar**)cache->core->eviction_array)[i] != 0 )
                    g_free(((gchar**)cache->core->eviction_array)[i]);
            g_free( (gchar**)cache->core->eviction_array );
        }
        cache->core->eviction_array = NULL;
    }
    
    if (cache->core->evict_err_array){
        g_free(cache->core->evict_err_array);
        cache->core->evict_err_array = NULL;
    }
    
    g_free(cache->core);
    cache->core = NULL;
    g_free(cache);
}

void cache_destroy_unique(cache_t* cache){
    if (cache->cache_params){
        g_free(cache->cache_params);
        cache->cache_params = NULL;
    }
    g_free(cache->core);
    g_free(cache);
}


cache_t* cache_init(long long size, char data_type, guint64 block_size){
    cache_t *cache = g_new0(cache_t, 1);
    cache->core = g_new0(struct cache_core, 1);
    cache->core->size = size;
    cache->core->cache_init_params = NULL;
    cache->core->data_type = data_type;
    if (block_size != 0 && block_size != -1){
        cache->core->use_block_size = TRUE;
        cache->core->block_size = block_size;
    }
    else {
        cache->core->use_block_size = FALSE;
        cache->core->block_size = 0;
    }
    return cache;
}



#ifdef __cplusplus
}
#endif
