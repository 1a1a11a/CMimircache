//
//  MRU.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef MRU_h
#define MRU_h

#include <stdio.h>
#include "cache.h"


#ifdef __cplusplus
extern "C"
{
#endif


struct MRU_params{
    GHashTable *hashtable;
};





extern  void __MRU_insert_element(cache_t* MRU, request_t* cp);

extern  gboolean MRU_check_element(cache_t* cache, request_t* cp);

extern  void __MRU_update_element(cache_t* MRU, request_t* cp);

extern  void __MRU_evict_element(cache_t* MRU, request_t* cp);

extern  gboolean MRU_add_element(cache_t* cache, request_t* cp);

extern  void MRU_destroy(cache_t* cache);
extern  void MRU_destroy_unique(cache_t* cache);

cache_t* MRU_init(guint64 size, char data_type, guint64 block_size, void* params);


#ifdef __cplusplus
}
#endif

 
#endif /* MRU_h */
