//
//  Score.h
//  mimircache
//
//  Created by Juncheng on 2/20/17.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef Score_h
#define Score_h


#include "cache.h"
#include "pqueue.h"
#include "math.h" 
#include <stdio.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef gchar hint_type;



typedef struct{
    GHashTable *hashtable;
    pqueue_t *pq;
    guint64 ts;       // virtual time stamp

//    gint32 *hints;

    hint_type *hints;
    uint64_t hint_pos;
    char hint_loc[128];

}Score_params_t;


typedef struct Score_init_params{
//    READER* reader;
//    GArray* next_access;
    char hint_loc[128];
//    gint32 *hints;
    hint_type* hints;
    guint64 ts;
    
}Score_init_params_t;






extern  void __Score_insert_element(struct_cache* Score, cache_line* cp);

extern  gboolean Score_check_element(struct_cache* cache, cache_line* cp);

extern  void __Score_update_element(struct_cache* Score, cache_line* cp);

extern  void __Score_evict_element(struct_cache* Score, cache_line* cp);

extern  gboolean Score_add_element(struct_cache* cache, cache_line* cp);

extern  void Score_destroy(struct_cache* cache);
extern  void Score_destroy_unique(struct_cache* cache);

struct_cache* Score_init(guint64 size, char data_type, int block_size, void* params);


#ifdef __cplusplus
}
#endif


#endif
