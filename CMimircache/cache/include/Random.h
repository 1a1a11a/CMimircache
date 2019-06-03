//
//  Random.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef Random_h
#define Random_h

#include "cache.h"



#ifdef __cplusplus
extern "C"
{
#endif


struct Random_params{
    GHashTable *hashtable;
    GArray* array;
};





extern  void __Random_insert_element(cache_t* Random, request_t* cp);

extern  gboolean Random_check_element(cache_t* cache, request_t* cp);

extern  void __Random_update_element(cache_t* Random, request_t* cp);

extern  void __Random_evict_element(cache_t* Random, request_t* cp);

extern  gboolean Random_add_element(cache_t* cache, request_t* cp);

extern  void Random_destroy(cache_t* cache);
extern  void Random_destroy_unique(cache_t* cache);

cache_t* Random_init(guint64 size, char data_type, guint64 block_size, void* params);


#ifdef __cplusplus
}

#endif


#endif /* Random_h */
