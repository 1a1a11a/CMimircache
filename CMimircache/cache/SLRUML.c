//
//  SLRUML.h
//  mimircache
//
//  Created by Juncheng on 2/12/17.
//  Copyright © 2017 Juncheng. All rights reserved.
//


#include "SLRUML.h"

#ifdef __cplusplus
extern "C"
{
#endif


void insert_at_segment(struct_cache* SLRUML, cache_line* cp, int segment){
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(SLRUML->cache_params);
    __LRU_insert_element(SLRUML_params->LRUs[segment], cp);
    SLRUML_params->current_sizes[segment] ++;
    
    if (SLRUML_params->num_insert_at_segments == NULL)
        SLRUML_params->num_insert_at_segments = g_new0(gint64, SLRUML_params->N_segments);
    SLRUML_params->num_insert_at_segments[segment] ++; 
    
    
    int i;
    for (i=segment; i>=1; i--){
        
        if (SLRUML_params->current_sizes[i] > SLRUML_params->LRUs[i]->core->size){
            gpointer old_itemp = cp->item_p;
            gpointer evicted = __LRU__evict_with_return(SLRUML_params->LRUs[i], cp);
            SLRUML_params->current_sizes[i] --;
            cp->item_p = evicted;
            __LRU_insert_element(SLRUML_params->LRUs[i-1], cp);
            SLRUML_params->current_sizes[i-1] ++;
            cp->item_p = old_itemp;
            g_free(evicted);
        }
    }
}

void __SLRUML_insert_element(struct_cache* SLRUML, cache_line* cp){
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(SLRUML->cache_params);
    int hint = SLRUML_params->hints[SLRUML_params->hint_pos];
    if (SLRUML_params->mode){
        if (hint > SLRUML_params->N_segments)
            return;
        else if (hint > SLRUML_params->N_segments-1){
            insert_at_segment(SLRUML, cp, SLRUML_params->N_segments-1);
//            __LRU_insert_element(SLRUML_params->LRUs[0], cp);
//            SLRUML_params->current_sizes[0] ++;
        }
        else if (hint == -1){
            ERROR("hint -1?\n");
            exit(-1);
        }
        else
            insert_at_segment(SLRUML, cp, hint);
    }
    else{
        // normal LRU mode by inserting at last segment
        insert_at_segment(SLRUML, cp, SLRUML_params->N_segments-1);
        
        long size = 0;
        int i;
        for (i=0; i<SLRUML_params->N_segments; i++)
            size += SLRUML_params->current_sizes[i];
        if (size == SLRUML->core->size){
            SLRUML_params->mode = TRUE;
//            printf("switch to non-LRU mode\n");
        }
        
    }
}

gboolean SLRUML_check_element(struct_cache* cache, cache_line* cp){
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(cache->cache_params);
    gboolean retVal = FALSE;
    int i;
    for (i=0; i<SLRUML_params->N_segments; i++)
        retVal = retVal || LRU_check_element(SLRUML_params->LRUs[i], cp);
    return retVal;
}


void __SLRUML_update_element(struct_cache* cache, cache_line* cp){
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(cache->cache_params);
    int i;
    for (i=0; i<SLRUML_params->N_segments; i++){
        if (LRU_check_element(SLRUML_params->LRUs[i], cp)){
            /* move to upper LRU 
             * first remove from current LRU 
             * then add to upper LRU, 
             * if upper LRU is full, evict one, insert into current LRU
             */
                LRU_remove_element(SLRUML_params->LRUs[i], cp->item_p);
                SLRUML_params->current_sizes[i] --;
            __SLRUML_insert_element(cache, cp);
            return;
        }
    }
}


void __SLRUML_evict_element(struct_cache* SLRUML, cache_line* cp){
    /* because insert only happens at LRU0, 
     * then eviction also can only happens at LRU0 
     */
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(SLRUML->cache_params);

    __LRU_evict_element(SLRUML_params->LRUs[0], cp);
    SLRUML_params->current_sizes[0] --;
#ifdef SANITY_CHECK
    if (LRU_get_size(SLRUML_params->LRUs[0]) != SLRUML_params->LRUs[0]->core->size){
        fprintf(stderr, "ERROR: SLRUML_evict_element, after eviction, LRU0 size %lu, "
                "full size %ld\n", (unsigned long)LRU_get_size(SLRUML_params->LRUs[0]),
                SLRUML_params->LRUs[0]->core->size);
        exit(1);
    }
    if (LRU_get_size(SLRUML_params->LRUs[0]) != SLRUML_params->current_sizes[0]){
        fprintf(stderr, "ERROR: SLRUML_evict_element, after eviction, LRU0 real size %lu, "
                "count size %lu\n", (unsigned long)LRU_get_size(SLRUML_params->LRUs[0]),
                (unsigned long)SLRUML_params->current_sizes[0]);
        exit(1);
    }
    
#endif
}


gpointer __SLRUML__evict_with_return(struct_cache* SLRUML, cache_line* cp){
    /** evict one element and return the evicted element, 
     user needs to free the memory of returned data **/
    
//    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(SLRUML->cache_params);
//    return __LRU__evict_with_return(SLRUML_params->LRUs[0], cp);
    return NULL;
}



gboolean SLRUML_add_element(struct_cache* cache, cache_line* cp){
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(cache->cache_params);

    long size = 0;
    int i;
//    if (SLRUML_params->hint_pos % 10000 == 0){
//        for (i=0; i<SLRUML_params->N_segments; i++)
//            printf("segment %d size %d(%ld)\n", i, SLRUML_params->current_sizes[i], SLRUML_params->LRUs[i]->core->size);
//        printf("\n\n\n");
//    }
    if (SLRUML_params->mode){
        for (i=0; i<SLRUML_params->N_segments; i++)
            size += SLRUML_params->current_sizes[i];
        if (size < (int)(cache->core->size * 0.9)){
            SLRUML_params->mode = FALSE;
//            printf("switch to LRU mode\n");
        }
    }

    if (SLRUML_check_element(cache, cp)){
        __SLRUML_update_element(cache, cp);
        if ( LRU_get_size(SLRUML_params->LRUs[0]) > SLRUML_params->LRUs[0]->core->size)
            __SLRUML_evict_element(cache, cp);
        SLRUML_params->hint_pos ++;
        return TRUE;
    }
    else{
        __SLRUML_insert_element(cache, cp);
        if ( LRU_get_size(SLRUML_params->LRUs[0]) > SLRUML_params->LRUs[0]->core->size)
            __SLRUML_evict_element(cache, cp);
        SLRUML_params->hint_pos ++;
        return FALSE;
    }

}




void SLRUML_destroy(struct_cache* cache){
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(cache->cache_params);
    int i;
    for (i=0; i<SLRUML_params->N_segments; i++)
        LRU_destroy(SLRUML_params->LRUs[i]);
    g_free(SLRUML_params->LRUs);
    g_free(SLRUML_params->current_sizes);
    g_free(SLRUML_params->hints);
    cache_destroy(cache);
}

void SLRUML_destroy_unique(struct_cache* cache){
    /* the difference between destroy_unique and destroy
     is that the former one only free the resources that are
     unique to the cache, freeing these resources won't affect
     other caches copied from original cache
     in Optimal, next_access should not be freed in destroy_unique,
     because it is shared between different caches copied from the original one.
     */
    
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(cache->cache_params);
    int i;
    for (i=0; i<SLRUML_params->N_segments; i++)
        LRU_destroy(SLRUML_params->LRUs[i]);
    g_free(SLRUML_params->LRUs); 
    g_free(SLRUML_params->current_sizes);
    cache_destroy_unique(cache);
}


struct_cache* SLRUML_init(guint64 size, char data_type, int block_size, void* params){
    struct_cache *cache = cache_init(size, block_size, data_type);
    cache->cache_params = g_new0(struct SLRUML_params, 1);
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(cache->cache_params);
    SLRUML_init_params_t* init_params = (SLRUML_init_params_t*) params;
    
    cache->core->type                   =   e_SLRUML;
    cache->core->cache_init             =   SLRUML_init;
    cache->core->destroy                =   SLRUML_destroy;
    cache->core->destroy_unique         =   SLRUML_destroy_unique;
    cache->core->add_element            =   SLRUML_add_element;
    cache->core->check_element          =   SLRUML_check_element;
    cache->core->__insert_element       =   __SLRUML_insert_element;
    cache->core->__update_element       =   __SLRUML_update_element;
    cache->core->__evict_element        =   __SLRUML_evict_element;
    cache->core->__evict_with_return    =   __SLRUML__evict_with_return;
    cache->core->get_size               =   SLRUML_get_size;
    cache->core->cache_init_params      =   params;
    
    SLRUML_params->mode = FALSE;
    
    int i;
    long filesize = 0;
    if ((init_params->hint_loc)[0] == 0)
        strcpy(init_params->hint_loc, "hint");
    SLRUML_params->hint_pos = 0;
    if (init_params->hints == 0){
        FILE* file = fopen(init_params->hint_loc, "rb");
        filesize = ftell(file);
        fseek(file, 0, SEEK_END);
        filesize = ftell(file) - filesize;
        fseek(file, 0, SEEK_SET);
        init_params->hints = g_new0(char, filesize);
        fread(init_params->hints, sizeof(char), filesize, file);
        fclose(file);
//        int counter[3]={0, 0, 0};
//        for (i=0; i<filesize; i++)
//            counter[init_params->hints[i]] ++;
//        
//        printf("\n%d:%d:%d\n\n", counter[0], counter[1], counter[2]);

    }

    // verify no hint < -1
    for (i=0; i<filesize; i++)
        if (init_params->hints[i] < -1){
            printf("%s filesize %ld, found a hint at loc %d, less than -1: %d\n",
                   init_params->hint_loc, filesize, i, init_params->hints[i]);
            exit(1);
        }
    
    
    
    SLRUML_params->hints = init_params->hints;
    strcpy(SLRUML_params->hint_loc, init_params->hint_loc);
    
    
    SLRUML_params->LRUs = g_new(cache_t*, 20);
    unsigned long size_temp = 4096;
    unsigned long size_left = size;
    i = 0;

    while (size_left > size_temp){
        size_left -= size_temp;
        SLRUML_params->LRUs[i++] = LRU_init(size_temp, data_type, 0, NULL);
        size_temp *= 4;
    }
//    printf("%ld %d, out %ld: %ld\n", size, i, size_left, size_temp);
    SLRUML_params->LRUs[i] = LRU_init(size_left, data_type, 0, NULL);
    
    SLRUML_params->N_segments = i+1;
    init_params->N_segments = i+1;
//    printf("%ld using %d segments: ", size, i);
//    int j;
//    for (j=0; j<i; j++)
//        printf("%ld\t", SLRUML_params->LRUs[j]->core->size);
//    printf("\n");

    SLRUML_params->current_sizes = g_new0(uint64_t, SLRUML_params->N_segments);
    
    return cache;
}


gint64 SLRUML_get_size(struct_cache* cache){
    SLRUML_params_t* SLRUML_params = (SLRUML_params_t*)(cache->cache_params);
    int i;
    uint64_t size = 0;
    for (i=0; i<SLRUML_params->N_segments; i++)
        size += SLRUML_params->current_sizes[i];
    return size;
}


#ifdef __cplusplus
extern "C"
{
#endif