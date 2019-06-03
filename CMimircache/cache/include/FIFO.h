//
//  FIFO.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef FIFO_H
#define FIFO_H


#include "cache.h" 

/* need add support for p and c label_type of data 
 
 */


#ifdef __cplusplus
extern "C"
{
#endif


struct FIFO_params{
    GHashTable *hashtable;
    GQueue *list;
};




extern gboolean fifo_check_element(cache_t* cache, request_t* cp);
extern gboolean fifo_add_element(cache_t* cache, request_t* cp);

extern void     __fifo_insert_element(cache_t* fifo, request_t* cp);
extern void     __fifo_update_element(cache_t* fifo, request_t* cp);
extern void     __fifo_evict_element(cache_t* fifo, request_t* cp);
extern void*    __fifo__evict_with_return(cache_t* fifo, request_t* cp);


extern void     fifo_destroy(cache_t* cache);
extern void     fifo_destroy_unique(cache_t* cache);
extern guint64 fifo_get_size(cache_t *cache);


cache_t* fifo_init(guint64 size, char data_type, guint64 block_size, void* params);


#ifdef __cplusplus
}
#endif


#endif	/* FIFO_H */
