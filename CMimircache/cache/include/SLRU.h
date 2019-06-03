//
//  SLRU.h
//  mimircache
//
//  Created by Juncheng on 2/12/17.
//  Copyright © 2017 Juncheng. All rights reserved.
//

#ifndef SLRU_h
#define SLRU_h


#include "cache.h" 
#include "LRU.h" 


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct SLRU_params{
    cache_t** LRUs;
    int N_segments;
    uint64_t *current_sizes; 
}SLRU_params_t;


typedef struct SLRU_init_params{
    int N_segments;
}SLRU_init_params_t;




extern gboolean SLRU_check_element(cache_t* cache, request_t* cp);
extern gboolean SLRU_add_element(cache_t* cache, request_t* cp);


extern void     __SLRU_insert_element(cache_t* SLRU, request_t* cp);
extern void     __SLRU_update_element(cache_t* SLRU, request_t* cp);
extern void     __SLRU_evict_element(cache_t* SLRU, request_t* cp);
extern void*    __SLRU__evict_with_return(cache_t* SLRU, request_t* cp);


extern void     SLRU_destroy(cache_t* cache);
extern void     SLRU_destroy_unique(cache_t* cache);


cache_t*   SLRU_init(guint64 size, char data_type, guint64 block_size, void* params);


extern void     SLRU_remove_element(cache_t* cache, void* data_to_remove);
extern guint64 SLRU_get_size(cache_t* cache);


#ifdef __cplusplus
}
#endif


#endif	/* SLRU_H */ 
