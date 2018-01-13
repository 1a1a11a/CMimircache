//
//  Score.h
//  mimircache
//
//  Created by Juncheng on 2/20/17.
//  Copyright © 2017 Juncheng. All rights reserved.
//


#include "Score.h"

#ifdef __cplusplus
extern "C"
{
#endif


/******************* priority queue structs and def **********************/

static int cmp_pri(pqueue_pri_t next, pqueue_pri_t curr){
    return (next.pri1 < curr.pri1);
}


static pqueue_pri_t get_pri(void *a){
    return ((pq_node_t *) a)->pri;
}


static void set_pri(void *a, pqueue_pri_t pri){
    ((pq_node_t *) a)->pri = pri;
}


static size_t get_pos(void *a){
    return ((pq_node_t *) a)->pos;
}


static void set_pos(void *a, size_t pos){
    ((pq_node_t *) a)->pos = pos;
}






/*************************** Score related ****************************/

void __Score_insert_element(struct_cache* Score, cache_line* cp){
    Score_params_t* Score_params = (Score_params_t*)(Score->cache_params);

    pq_node_t *node = g_new(pq_node_t, 1);
    gpointer key;
    if (cp->type == 'l'){
        key = (gpointer)g_new(guint64, 1);
        *(guint64*)key = *(guint64*)(cp->item_p);
    }
    else{
        key = (gpointer)g_strdup((gchar*)(cp->item_p));
    }

    node->data_type = cp->type;
    node->item = (gpointer)key;
    
    long long temp_pri;
    pqueue_pri_t pri = {0, 0};
    if (Score_params->hints[Score_params->hint_pos] == -1)
        temp_pri = G_MAXINT64;
    else{
//        temp_pri = Score_params->hints[Score_params->hint_pos] + Score_params->ts; // % Score->core->size;
        temp_pri = pow(4, Score_params->hints[Score_params->hint_pos]+5) + Score_params->ts; // % Score->core->size;
    }
    pri.pri1 = temp_pri<0? 0: temp_pri;
    node->pri = pri;

//    printf("[%ld_1] insert pri %lld\n", Score_params->ts, (long long)(node->pri.pri1));
    
    pqueue_insert(Score_params->pq, (void *)node);
    g_hash_table_insert (Score_params->hashtable, (gpointer)key, (gpointer)node);
}


gboolean Score_check_element(struct_cache* cache, cache_line* cp){
    return g_hash_table_contains(
                                ((Score_params_t*)(cache->cache_params))->hashtable,
                                (gconstpointer)(cp->item_p)
                                );
}


void __Score_update_element(struct_cache* Score, cache_line* cp){
    Score_params_t* Score_params = (Score_params_t*)(Score->cache_params);
    pq_node_t* node;
    node = g_hash_table_lookup(Score_params->hashtable, (gconstpointer)(cp->item_p));
//#ifdef SANITY_CHECK
//    if (node->pri.pri1 != Score_params->ts + 1){
//        printf("ERROR node pri %lu, current ts %ld\n", (unsigned long)node->pri.pri1, Score_params->ts);
//    }
//#endif
    pqueue_pri_t pri;
    pri.pri1 = G_MAXINT64;
    if (Score_params->hints[Score_params->hint_pos] != -1)
//        pri.pri1 = Score_params->hints[Score_params->hint_pos] + Score_params->ts;
        pri.pri1 = pow(4, Score_params->hints[Score_params->hint_pos]+5) +
            Score_params->ts; // % Score->core->size;

    pqueue_change_priority(Score_params->pq, pri, node);
}



void __Score_evict_element(struct_cache* Score, cache_line* cp){
    Score_params_t* Score_params = (Score_params_t*)(Score->cache_params);
    
    pq_node_t* node = (pq_node_t*) pqueue_pop(Score_params->pq);
//    printf("[%ld] evicting pri %lld, current size %u\n", Score_params->ts,
//           (long long)(node->pri.pri1), g_hash_table_size(Score_params->hashtable));
    g_hash_table_remove(Score_params->hashtable, (gconstpointer)(node->item));
}


void* __Score_evict_with_return(struct_cache* Score, cache_line* cp){
    Score_params_t* Score_params = (Score_params_t*)(Score->cache_params);
    
    void* evicted_key;
    pq_node_t* node = (pq_node_t*) pqueue_pop(Score_params->pq);
    
    if (cp->type == 'l'){
        evicted_key = (gpointer)g_new(guint64, 1);
        *(guint64*)evicted_key = *(guint64*)(node->item);
    }
    else{
        evicted_key = (gpointer)g_strdup((gchar*)(node->item));
    }

    g_hash_table_remove(Score_params->hashtable, (gconstpointer)(node->item));
    return evicted_key;
}


gint64 Score_get_size(struct_cache* cache){
    Score_params_t* Score_params = (Score_params_t*)(cache->cache_params);
    return (guint64) g_hash_table_size(Score_params->hashtable);
}



gboolean Score_add_element(struct_cache* cache, cache_line* cp){
    Score_params_t* Score_params = (Score_params_t*)(cache->cache_params);

//    if (Score_params->ts == 56935){
//        printf("existing pri: \n");
//        g_hash_table_foreach(Score_params->hashtable, printHashTable, NULL);
//    }
    
    if (Score_check_element(cache, cp)){
        __Score_update_element(cache, cp);
        (Score_params->ts) ++ ;
        (Score_params->hint_pos) ++ ;
        return TRUE;
    }
    else{
        __Score_insert_element(cache, cp);
        
        if ( (long)g_hash_table_size( Score_params->hashtable) > cache->core->size)
            __Score_evict_element(cache, cp);
        (Score_params->ts) ++ ;
        (Score_params->hint_pos) ++ ;
        return FALSE;
    }
}


void Score_destroy(struct_cache* cache){
    Score_params_t* Score_params = (Score_params_t*)(cache->cache_params);
    
    g_hash_table_destroy(Score_params->hashtable);
    pqueue_free(Score_params->pq);
    g_free(Score_params->hints);
   
    cache_destroy(cache);
}


void Score_destroy_unique(struct_cache* cache){
    /* the difference between destroy_unique and destroy 
     is that the former one only free the resources that are 
     unique to the cache, freeing these resources won't affect 
     other caches copied from original cache 
     in Score, next_access should not be freed in destroy_unique, 
     because it is shared between different caches copied from the original one.
     */
    
    Score_params_t* Score_params = (Score_params_t*)(cache->cache_params);
    
 
    g_hash_table_destroy(Score_params->hashtable);
    pqueue_free(Score_params->pq);
    g_free(cache->cache_params);
    g_free(cache->core);
    g_free(cache);
}



struct_cache* Score_init(guint64 size, char data_type, int block_size,  void* params){
    struct_cache* cache = cache_init(size, data_type, block_size);
    
    Score_params_t* Score_params = g_new0(Score_params_t, 1);
    cache->cache_params = (void*) Score_params;
    Score_init_params_t* init_params = (Score_init_params_t*)params;
    
    cache->core->type                   =   e_Score;
    cache->core->cache_init             =   Score_init;
    cache->core->destroy                =   Score_destroy;
    cache->core->destroy_unique         =   Score_destroy_unique;
    cache->core->add_element            =   Score_add_element;
    cache->core->check_element          =   Score_check_element;
    
    cache->core->__insert_element       =   __Score_insert_element;
    cache->core->__update_element       =   __Score_update_element;
    cache->core->__evict_element        =   __Score_evict_element;
    cache->core->__evict_with_return    =   __Score_evict_with_return;
    cache->core->get_size               =   Score_get_size;
    cache->core->cache_init_params      =   params;

    
    
    Score_params->ts = 0;
    Score_params->hint_pos = 0;
    
    int i;
    long filesize = 0;
    
    // read hint
    if ((init_params->hint_loc)[0] == 0)
        strcpy(init_params->hint_loc, "hint");
    if (init_params->hints == 0){
        FILE* file = fopen(init_params->hint_loc, "rb");
        filesize = (uint64_t) ftell(file);
        fseek(file, 0, SEEK_END);
        filesize = (uint64_t) ftell(file) - filesize;
        fseek(file, 0, SEEK_SET);
        init_params->hints = g_new0(hint_type, filesize/sizeof(hint_type));
        fread(init_params->hints, sizeof(hint_type), filesize/sizeof(hint_type), file);
        fclose(file);
    }
    Score_params->hints = init_params->hints;
    strcpy(Score_params->hint_loc, init_params->hint_loc); 

    // verify no hint < -1
    for (i=0; i<(int)(filesize/sizeof(hint_type)); i++){
        if (init_params->hints[i] < -1){
            printf("%s filesize %ld, found a hint at loc %d, less than -1: %d\n",
                   init_params->hint_loc,
                   filesize, i,
                   init_params->hints[i]);
            exit(1);
        }
    }
    
    
    if (data_type == 'l'){
        Score_params->hashtable =
            g_hash_table_new_full(g_int64_hash, g_int64_equal,
                                  simple_g_key_value_destroyer,
                                  simple_g_key_value_destroyer);
    }
    
    else if (data_type == 'c'){
        Score_params->hashtable =
            g_hash_table_new_full(g_str_hash, g_str_equal,
                                  simple_g_key_value_destroyer,
                                  simple_g_key_value_destroyer);
    }
    else{
        ERROR("does not support given data type: %c\n", data_type);
    }
    
    
    Score_params->pq = pqueue_init(size, cmp_pri,
                                     get_pri, set_pri, get_pos, set_pos);
    

    return cache;
    
}




#ifdef __cplusplus
}
#endif