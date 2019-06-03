//
//  AMP.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//


/** since this is sequence based prefetching, we will use gint64 for block number **/



#ifndef AMP_h
#define AMP_h


#include "cache.h" 

#ifdef __cplusplus
extern "C"
{
#endif


//#define AMP_prev(cache, x) (AMP_lookup((cache), ((struct AMP_page*)x)->block_numer - 1))
//#define AMP_last(cache, x) (AMP_lookup((cache), ((struct AMP_page*)x)->last_block_numer))
//#define AMP_isLast(x) (((struct AMP_page*)x)->block_number == ((struct AMP_page*)x)->last_block_number) 

#define AMP_getPage(x) ((struct AMP_Page*) g_hashtable_lookup())



struct AMP_params{
    GHashTable *hashtable;
    GQueue *list;
    
    int APT;
    int read_size;
    int p_threshold;
    int K; 
    
    GHashTable *last_hashset;
    
    
    gint64 num_of_prefetch;
    gint64 num_of_hit;
    GHashTable *prefetched; 
};

struct AMP_page{
    gint64 block_number;
    gint64 last_block_number;
    gboolean accessed;
    gboolean tag;
    gboolean old;
    gint16 p;                   // prefetch degree
    gint16 g;                   // trigger distance 

    gint size;
};

struct AMP_init_params{
    int APT;
    int read_size;
    int p_threshold;
    int K;
};



extern gboolean     AMP_check_element_int(cache_t* cache, gint64 block);
extern gboolean     AMP_check_element(cache_t* cache, request_t* cp);

extern gboolean     AMP_add_element(cache_t* cache, request_t* cp);
extern gboolean     AMP_add_element_only(cache_t* cache, request_t* cp);
extern gboolean     AMP_add_element_only_no_eviction(cache_t* AMP, request_t* cp);
extern gboolean     AMP_add_element_no_eviction(cache_t* cache, request_t* cp);

extern struct AMP_page* __AMP_update_element_int(cache_t* AMP, gint64 block);
extern void         __AMP_update_element(cache_t* AMP, request_t* cp);

extern struct AMP_page* __AMP_insert_element_int(cache_t* AMP, gint64 block);
extern void         __AMP_insert_element(cache_t* AMP, request_t* cp);


extern void         __AMP_evict_element(cache_t* AMP, request_t* cp);
extern void*        __AMP__evict_with_return(cache_t* AMP, request_t* cp);



extern void         AMP_destroy(cache_t* cache);
extern void         AMP_destroy_unique(cache_t* cache);


cache_t*       AMP_init(guint64 size, char data_type, guint64 block_size, void* params);


extern void         AMP_remove_element(cache_t* cache, void* data_to_remove);
extern guint64    AMP_get_size(cache_t* cache);


#ifdef __cplusplus
}
#endif


#endif /* AMP_H */ 
