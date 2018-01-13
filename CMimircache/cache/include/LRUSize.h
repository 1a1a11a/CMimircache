//
//  LRUSize.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef LRU_SIZE_H
#define LRU_SIZE_H


#include "cache.h" 


#ifdef __cplusplus
extern "C"
{
#endif


struct LRUSize_params{
    GHashTable *hashtable;
    GQueue *list;
    gint64 ts;              // this only works when add_element is called 
};

typedef struct LRUSize_params LRUSize_params_t; 




extern gboolean LRUSize_check_element(struct_cache* cache, cache_line* cp);
extern gboolean LRUSize_add_element(struct_cache* cache, cache_line* cp);


extern void     __LRUSize_insert_element(struct_cache* LRUSize, cache_line* cp);
extern void     __LRUSize_update_element(struct_cache* LRUSize, cache_line* cp);
extern void     __LRUSize_evict_element(struct_cache* LRUSize, cache_line* cp);
extern void*    __LRUSize__evict_with_return(struct_cache* LRUSize, cache_line* cp);


extern void     LRUSize_destroy(struct_cache* cache);
extern void     LRUSize_destroy_unique(struct_cache* cache);


struct_cache*   LRUSize_init(guint64 size, char data_type, int block_size, void* params);


extern void     LRUSize_remove_element(struct_cache* cache, void* data_to_remove);
extern gint64 LRUSize_get_size(struct_cache* cache);


#ifdef __cplusplus
}
#endif


#endif	/* LRU_SIZE_H */ 
