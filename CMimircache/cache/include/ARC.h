//
//  ARC.h
//  mimircache
//
//  Created by Juncheng on 2/12/17.
//  Copyright © 2017 Juncheng. All rights reserved.
//

#ifndef ARC_h
#define ARC_h


#include "cache.h"
#include "LRU.h" 
#include "LFU.h"


#ifdef __cplusplus
extern "C"
{
#endif


// by default, the ghost list size is 10 times the orginal cache size

typedef struct ARC_params{
    cache_t* LRU1;         // normal LRU segment
    cache_t* LRU1g;        // ghost list for normal LRU segment
    cache_t* LRU2;         // normal LRU segement for items accessed more than once
    cache_t* LRU2g;        // ghost list for normal LFU segment
    gint32 ghost_list_factor;  // size(ghost_list)/size(cache),
                                // by default, the ghost list size is
                                // 10 times the orginal cache size
    gint64 size1;             // size for segment 1
    gint64 size2;             // size for segment 2
}ARC_params_t;


typedef struct ARC_init_params{
    gint32 ghost_list_factor;
} ARC_init_params_t;



extern gboolean ARC_check_element(cache_t* cache, request_t* cp);
extern gboolean ARC_add_element(cache_t* cache, request_t* cp);


extern void     __ARC_insert_element(cache_t* ARC, request_t* cp);
extern void     __ARC_update_element(cache_t* ARC, request_t* cp);
extern void     __ARC_evict_element(cache_t* ARC, request_t* cp);
extern void*    __ARC__evict_with_return(cache_t* ARC, request_t* cp);


extern void     ARC_destroy(cache_t* cache);
extern void     ARC_destroy_unique(cache_t* cache);


cache_t*   ARC_init(guint64 size, char data_type, guint64 block_size, void* params);


extern void     ARC_remove_element(cache_t* cache, void* data_to_remove);
extern guint64 ARC_get_size(cache_t* cache);


#ifdef __cplusplus
}
#endif


#endif  /* ARC_H */ 
