//
//  LRU.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef LRU_h
#define LRU_h


#include "cache.h" 


#ifdef __cplusplus
extern "C"
{
#endif


struct LRU_params{
    GHashTable *hashtable;
    GQueue *list;
    gint64 ts;              // this only works when add_element is called 
};

typedef struct LRU_params LRU_params_t; 




extern gboolean LRU_check_element(cache_t* cache, request_t* cp);
extern gboolean LRU_add_element(cache_t* cache, request_t* cp);


extern void     __LRU_insert_element(cache_t* LRU, request_t* cp);
extern void     __LRU_update_element(cache_t* LRU, request_t* cp);
extern void     __LRU_evict_element(cache_t* LRU, request_t* cp);
extern void*    __LRU__evict_with_return(cache_t* LRU, request_t* cp);


extern void     LRU_destroy(cache_t* cache);
extern void     LRU_destroy_unique(cache_t* cache);


cache_t*   LRU_init(guint64 size, char data_type, guint64 block_size, void* params);


extern void     LRU_remove_element(cache_t* cache, void* data_to_remove);
extern guint64 LRU_get_size(cache_t* cache);


#ifdef __cplusplus
}
#endif


#endif	/* LRU_H */
