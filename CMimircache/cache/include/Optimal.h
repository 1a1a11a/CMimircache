//
//  optimal.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef optimal_h
#define optimal_h


#include "cache.h"
#include "pqueue.h"
#include "profilerUtils.h"



#ifdef __cplusplus
extern "C"
{
#endif
    
    
    struct optimal_params{
        GHashTable *hashtable;
        pqueue_t *pq;
        GArray* next_access;
        guint64 ts;       // virtual time stamp
        reader_t* reader;
    };
    
    
    struct optimal_init_params{
        reader_t* reader;
        GArray* next_access;
        guint64 ts;
    };
    
    typedef struct optimal_params       optimal_params_t;
    typedef struct optimal_init_params  optimal_init_params_t;
    
    
    
    
    extern  void __optimal_insert_element(cache_t* optimal, request_t* cp);
    
    extern  gboolean optimal_check_element(cache_t* cache, request_t* cp);
    
    extern  void __optimal_update_element(cache_t* optimal, request_t* cp);
    
    extern  void __optimal_evict_element(cache_t* optimal, request_t* cp);
    extern  void* __optimal_evict_with_return(cache_t* cache, request_t* cp);
    
    extern  gboolean optimal_add_element(cache_t* cache, request_t* cp);
    extern  gboolean optimal_add_element_only(cache_t* cache, request_t* cp);
    
    extern  void optimal_destroy(cache_t* cache);
    extern  void optimal_destroy_unique(cache_t* cache);
    
    cache_t* optimal_init(guint64 size, char data_type, guint64 block_size, void* params);
    
    
#ifdef __cplusplus
}
#endif


#endif
