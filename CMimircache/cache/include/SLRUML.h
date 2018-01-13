//
//  SLRUML.h
//  mimircache
//
//  Created by Juncheng on 2/12/17.
//  Copyright © 2017 Juncheng. All rights reserved.
//

#ifndef SLRUML_h
#define SLRUML_h


#include <stdio.h>
#include "cache.h" 
#include "LRU.h" 


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct SLRUML_params{
    cache_t** LRUs;
    int N_segments;
    uint64_t *current_sizes;
    char *hints;
    uint64_t hint_pos;
    gboolean mode;          // TRUE: use hint, FALSE: use LRU to insert
    char hint_loc[128];
    gint64* num_insert_at_segments;
}SLRUML_params_t;


typedef struct SLRUML_init_params{
    int N_segments;
    char* hints;
    char hint_loc[128];
}SLRUML_init_params_t;




extern gboolean SLRUML_check_element(struct_cache* cache, cache_line* cp);
extern gboolean SLRUML_add_element(struct_cache* cache, cache_line* cp);


extern void     __SLRUML_insert_element(struct_cache* SLRUML, cache_line* cp);
extern void     __SLRUML_update_element(struct_cache* SLRUML, cache_line* cp);
extern void     __SLRUML_evict_element(struct_cache* SLRUML, cache_line* cp);
extern void*    __SLRUML__evict_with_return(struct_cache* SLRUML, cache_line* cp);


extern void     SLRUML_destroy(struct_cache* cache);
extern void     SLRUML_destroy_unique(struct_cache* cache);


struct_cache*   SLRUML_init(guint64 size, char data_type, int block_size, void* params);


extern void     SLRUML_remove_element(struct_cache* cache, void* data_to_remove);
extern gint64 SLRUML_get_size(struct_cache* cache);

extern void insert_at_segment(struct_cache* SLRUML, cache_line* cp, int segment); 


#ifdef __cplusplus
}
#endif


#endif
