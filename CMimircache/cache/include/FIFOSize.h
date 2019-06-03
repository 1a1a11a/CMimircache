//
//  FIFOSize.h
//  mimircache
//
//  Created by Juncheng on 5/08/19.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef FIFO_SIZE_H
#define FIFO_SIZE_H


#include "cache.h" 


#ifdef __cplusplus
extern "C"
{
#endif


struct FIFOSize_params{
    GHashTable *hashtable;
    GQueue *list;
    gint64 ts;              // this only works when add_element is called
};


typedef struct FIFOSize_params FIFOSize_params_t;




extern gboolean FIFOSize_check_element(cache_t* cache, request_t* cp);
extern gboolean FIFOSize_add_element(cache_t* cache, request_t* cp);


extern void     __FIFOSize_insert_element(cache_t* FIFOSize, request_t* cp);
extern void     __FIFOSize_update_element(cache_t* FIFOSize, request_t* cp);
extern void     __FIFOSize_evict_element(cache_t* FIFOSize, request_t* cp);
extern void*    __FIFOSize__evict_with_return(cache_t* FIFOSize, request_t* cp);


extern void     FIFOSize_destroy(cache_t* cache);
extern void     FIFOSize_destroy_unique(cache_t* cache);


cache_t*   FIFOSize_init(guint64 size, char data_type, guint64 block_size, void* params);


extern void     FIFOSize_remove_element(cache_t* cache, void* data_to_remove);
extern guint64   FIFOSize_get_size(cache_t* cache);
extern GHashTable* FIFOSize_get_objmap(cache_t *cache);


#ifdef __cplusplus
}
#endif


#endif	/* FIFO_SIZE_H */ 
