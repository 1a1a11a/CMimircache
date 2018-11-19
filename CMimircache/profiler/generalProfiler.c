//
//  generalProfiler.c
//  generalProfiler
//
//  Created by Juncheng on 5/24/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//



#include "generalProfiler.h"


#include "reader.h"
#include "FIFO.h"
#include "Optimal.h"
#include "LRU_K.h"
#include "LRU.h"
#include "cacheHeader.h"
#include "AMP.h"
#include "PG.h"


#ifdef __cplusplus
extern "C"
{
#endif


static void profiler_thread(gpointer data, gpointer user_data) {
    mt_param_gp_t* params = (mt_param_gp_t*) user_data;

    int order = GPOINTER_TO_UINT(data);
    guint bin_size = params->bin_size;

    struct_cache* cache = params->cache->core->cache_init(bin_size * order,
                          params->cache->core->data_type,
                          params->cache->core->block_unit_size,
                          params->cache->core->cache_init_params);
    return_res_t** result = params->result;

    reader_t* reader_thread = clone_reader(params->reader);


    // create cache line struct and initialization
    cache_line* cp = new_cacheline();
    cp->type = params->cache->core->data_type;
    cp->block_unit_size = (size_t) reader_thread->base->block_unit_size;    // this is not used, block_unit_size goes with cache
    cp->disk_sector_size = (size_t) reader_thread->base->disk_sector_size;


    guint64 hit_count = 0, miss_count = 0;
    gboolean (*add_element)(struct cache*, cache_line * cp);
    add_element = cache->core->add_element;

    read_one_element(reader_thread, cp);

    // this must happen after read, otherwise cp size and type unknown
    if (cache->core->consider_size && cp->type == 'l' && cp->disk_sector_size != 0) {    // && cp->size != 0 is removed due to trace dirty
        add_element = cache->core->add_element_withsize;
        if (add_element == NULL) {
            ERROR("using size with profiling, cannot find add_element_withsize\n");
            abort();
        }
    }

    while (cp->valid) {
        /* new 170428
         add size into consideration, this only affects the traces with cache_size column,
         currently CPHY traces are affected
         the default size for each block is 512 bytes */
        if (add_element(cache, cp))
            hit_count ++;
        else
            miss_count ++;
        read_one_element(reader_thread, cp);
    }

    result[order]->hit_count = (long long) hit_count;
    result[order]->miss_count = (long long) miss_count;
    result[order]->total_count = hit_count + miss_count;
    result[order]->hit_ratio = (double) hit_count / (hit_count + miss_count);
    result[order]->miss_ratio = 1 - result[order]->hit_ratio;


    if (cache->core->type == e_Mithril) {
        Mithril_params_t* Mithril_params = (Mithril_params_t*)(cache->cache_params);
        gint64 prefetch = Mithril_params->num_of_prefetch_Mithril;
        gint64 hit = Mithril_params->hit_on_prefetch_Mithril;

        printf("\ncache size %ld, real size: %ld, hit rate %lf, total check %lu, "
               "Mithril prefetch %lu, hit %lu, accuracy: %lf, prefetch table size %u\n",
               cache->core->size, Mithril_params->cache->core->size,
               (double)hit_count / (hit_count + miss_count),
               (unsigned long) ((Mithril_params_t*)(cache->cache_params))->num_of_check,
               (unsigned long) prefetch, (unsigned long) hit, (double)hit / prefetch,
               g_hash_table_size(Mithril_params->prefetch_hashtable));

        if (Mithril_params->sequential_type == 1) {
            gint64 prefetch2 = ((Mithril_params_t*)(cache->cache_params))->num_of_prefetch_sequential;
            gint64 hit2 = ((Mithril_params_t*)(cache->cache_params))->hit_on_prefetch_sequential;
            printf("sequential prefetching, prefetch %lu, hit %lu, accuracy %lf\n", (unsigned long) prefetch2, (unsigned long) hit2, (double)hit2 / prefetch2);
            printf("overall size %ld, hit rate %lf, efficiency %lf\n", Mithril_params->cache->core->size,
                   (double)hit_count / (hit_count + miss_count), (double)(hit + hit2) / (prefetch + prefetch2));
        }

        if (Mithril_params->cache->core->type == e_AMP) {
            gint64 prefetch2 = ((struct AMP_params*)(Mithril_params->cache->cache_params))->num_of_prefetch;
            gint64 hit2 = ((struct AMP_params*)(Mithril_params->cache->cache_params))->num_of_hit;
            printf("Mithril_AMP cache size %ld, prefetch %lu, hit %lu, accuracy: %lf, total prefetch %lu, hit %lu, accuracy: %lf\n",
                   Mithril_params->cache->core->size, (unsigned long) prefetch2, (unsigned long) hit2, (double)hit2 / prefetch2,
                   (unsigned long) (prefetch + prefetch2), (unsigned long) (hit + hit2), (double)(hit + hit2) / (prefetch + prefetch2));

        }
    }

    if (cache->core->type == e_PG) {
        PG_params_t *PG_params = (PG_params_t*)(cache->cache_params);
        printf("\n PG cache size %lu, real size %ld, hit rate %lf, prefetch %lu, "
               "hit %lu, precision %lf\n", (unsigned long)PG_params->init_size,
               PG_params->cache->core->size,
               (double)hit_count / (hit_count + miss_count),
               (unsigned long) PG_params->num_of_prefetch, (unsigned long) PG_params->num_of_hit,
               (double)(PG_params->num_of_hit) / (PG_params->num_of_prefetch));
    }

    if (cache->core->type == e_AMP) {
        gint64 prefech = ((struct AMP_params*)(cache->cache_params))->num_of_prefetch;
        gint64 hit = ((struct AMP_params*)(cache->cache_params))->num_of_hit;

        printf("\nAMP cache size %ld, hit rate %lf, prefetch %lu, hit %lu, accuracy: %lf\n\n",
               cache->core->size, (double)hit_count / (hit_count + miss_count),
               (unsigned long) prefech, (unsigned long) hit, (double)hit / prefech);
    }


    // clean up
    g_mutex_lock(&(params->mtx));
    (*(params->progress)) ++ ;
    g_mutex_unlock(&(params->mtx));

    g_free(cp);
    close_reader_unique(reader_thread);
    cache->core->destroy_unique(cache);
}


static void get_eviction_age_thread(gpointer data, gpointer user_data) {
    mt_param_gp_t* params = (mt_param_gp_t*) user_data;

    int order = GPOINTER_TO_UINT(data);
    guint bin_size = params->bin_size;

    struct_cache* cache = params->cache->core->cache_init(bin_size * order,
                          params->cache->core->data_type,
                          params->cache->core->block_unit_size,
                          params->cache->core->cache_init_params);
    return_res_t** result = params->result;

    reader_t* reader_thread = clone_reader(params->reader);

    GHashTable* last_ref_ht;
    guint64 cur_ts = 1;
    gint64 last_ref_ts = 0;
    gpointer key;
    gint64* eviction_age = g_new(gint64, reader_thread->base->total_num);
    gint64 i = 0;
    for (i=0; i<reader_thread->base->total_num; i++)
        eviction_age[i] = -1;

    if (cache->core->data_type == 'l'){
        last_ref_ht = g_hash_table_new_full(g_int64_hash, g_int64_equal,
                                                      simple_g_key_value_destroyer, NULL);
    }
    else if (cache->core->data_type == 'c'){
        last_ref_ht = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                      simple_g_key_value_destroyer, NULL);
    }


    // create cache line struct and initialization
    cache_line* cp = new_cacheline();
    cp->type = params->cache->core->data_type;
    cp->block_unit_size = (size_t) reader_thread->base->block_unit_size;    // this is not used, block_unit_size goes with cache
    cp->disk_sector_size = (size_t) reader_thread->base->disk_sector_size;


    guint64 hit_count = 0, miss_count = 0;
    void (*insert_element)(struct cache*, cache_line * cp);
    void (*update_element)(struct cache*, cache_line * cp);
    gboolean (*check_element)(struct cache*, cache_line * cp);
    gboolean (*add_element)(struct cache*, cache_line * cp);
    gpointer (*evict_element)(struct cache*, cache_line * cp);
    gint64 (*get_size)(cache_t*);
    gpointer evicted_obj;

    insert_element = cache->core->__insert_element;
    update_element = cache->core->__update_element;
    check_element = cache->core->check_element;
    add_element = cache->core->add_element_only;
    evict_element = cache->core->__evict_with_return;
    get_size = cache->core->get_size;

    read_one_element(reader_thread, cp);

    // this must happen after read, otherwise cp size and type unknown
    if (cache->core->consider_size && cp->type == 'l' && cp->disk_sector_size != 0) {    // && cp->size != 0 is removed due to trace dirty
        add_element = cache->core->add_element_only;
        if (add_element == NULL) {
            ERROR("using size with profiling, cannot find add_element_withsize\n");
            abort();
        }
    }

    while (cp->valid) {
        /* new 170428
         add size into consideration, this only affects the traces with cache_size column,
         currently CPHY traces are affected
         the default size for each block is 512 bytes */

        /* record last reference time */
        if (cache->core->data_type == 'l'){
            key = (gpointer)g_new(guint64, 1);
            *(guint64*)key = *(guint64*)(cp->item_p);
        }
        else if (cache->core->data_type == 'c'){
            key = (gpointer) g_strdup((gchar*) cp->item_p);
        }
        g_hash_table_replace(last_ref_ht, key, GINT_TO_POINTER(cur_ts));


        if (cache->core->type == e_Optimal){
            optimal_params_t* optimal_params = (optimal_params_t*)(cache->cache_params);
            (optimal_params->ts) ++ ;
        }

        if (check_element(cache, cp)){
            update_element(cache, cp);
            hit_count ++;
        }
        else{
            miss_count ++;
            insert_element(cache, cp);
            if (get_size(cache) > cache->core->size) {
                evicted_obj = evict_element(cache, cp);
                last_ref_ts = GPOINTER_TO_INT(g_hash_table_lookup(last_ref_ht, evicted_obj));
                eviction_age[last_ref_ts] = cur_ts - last_ref_ts;
                //                printf("set ts %ld req to %ld (%ld - %ld) cache size %ld\n", last_ref_ts, cur_ts-last_ref_ts, cur_ts, last_ref_ts,
                //                       cache->core->get_size(cache));
                g_free(evicted_obj);
            }
        }
        cur_ts ++;
        read_one_element(reader_thread, cp);
    }

    result[order]->hit_count = (long long) hit_count;
    result[order]->miss_count = (long long) miss_count;
    result[order]->total_count = hit_count + miss_count;
    result[order]->hit_ratio = (double) hit_count / (hit_count + miss_count);
    result[order]->miss_ratio = 1 - result[order]->hit_ratio;
    result[order]->cache_size = cache->core->size;
    result[order]->other_data = eviction_age;


    // clean up
    g_mutex_lock(&(params->mtx));
    (*(params->progress)) ++ ;
    g_mutex_unlock(&(params->mtx));
    g_free(cp);
    close_reader_unique(reader_thread);
    cache->core->destroy_unique(cache);
}


    static void get_hit_result_thread(gpointer data, gpointer user_data) {
        mt_param_gp_t* params = (mt_param_gp_t*) user_data;

        int order = GPOINTER_TO_UINT(data);
        guint bin_size = params->bin_size;

        struct_cache* cache = params->cache->core->cache_init(bin_size * order,
                                                              params->cache->core->data_type,
                                                              params->cache->core->block_unit_size,
                                                              params->cache->core->cache_init_params);
        return_res_t** result = params->result;

        reader_t* reader_thread = clone_reader(params->reader);
        gboolean *hit_result = g_new(gboolean, reader_thread->base->total_num);


        // create cache line struct and initialization
        cache_line* cp = new_cacheline();
        cp->type = params->cache->core->data_type;
        cp->block_unit_size = (size_t) reader_thread->base->block_unit_size;    // this is not used, block_unit_size goes with cache
        cp->disk_sector_size = (size_t) reader_thread->base->disk_sector_size;


        guint64 hit_count = 0, miss_count = 0;
        gboolean (*add_element)(struct cache*, cache_line * cp);
        add_element = cache->core->add_element;

        read_one_element(reader_thread, cp);

        // this must happen after read, otherwise cp size and type unknown
        if (cache->core->consider_size && cp->type == 'l' && cp->disk_sector_size != 0) {    // && cp->size != 0 is removed due to trace dirty
            add_element = cache->core->add_element_withsize;
            if (add_element == NULL) {
                ERROR("using size with profiling, cannot find add_element_withsize\n");
                abort();
            }
        }


        guint64 i = 0;

        while (cp->valid) {
            /* new 170428
             add size into consideration, this only affects the traces with cache_size column,
             currently CPHY traces are affected
             the default size for each block is 512 bytes */
            if (add_element(cache, cp)){
                hit_result[i] = TRUE;
                hit_count ++;
            }
            else{
                hit_result[i] = FALSE;
                miss_count ++;
            }
            i ++;
            read_one_element(reader_thread, cp);
        }

        result[order]->hit_count = (long long) hit_count;
        result[order]->miss_count = (long long) miss_count;
        result[order]->total_count = hit_count + miss_count;
        result[order]->hit_ratio = (double) hit_count / (hit_count + miss_count);
        result[order]->miss_ratio = 1 - result[order]->hit_ratio;
        result[order]->other_data = hit_result;


        // clean up
        g_mutex_lock(&(params->mtx));
        (*(params->progress)) ++ ;
        g_mutex_unlock(&(params->mtx));

        g_free(cp);
        close_reader_unique(reader_thread);
        cache->core->destroy_unique(cache);
    }


    static void get_evictions_thread(gpointer data, gpointer user_data) {
        mt_param_gp_t* params = (mt_param_gp_t*) user_data;

        int order = GPOINTER_TO_UINT(data);
        guint bin_size = params->bin_size;

        struct_cache* cache = params->cache->core->cache_init(bin_size * order,
                                                              params->cache->core->data_type,
                                                              params->cache->core->block_unit_size,
                                                              params->cache->core->cache_init_params);
        return_res_t** result = params->result;

        reader_t* reader_thread = clone_reader(params->reader);
        cache->core->record_level = 1;
        cache->core->eviction_array = g_new(gpointer, reader_thread->base->total_num);


        // create cache line struct and initialization
        cache_line* cp = new_cacheline();
        cp->type = params->cache->core->data_type;
        cp->block_unit_size = (size_t) reader_thread->base->block_unit_size;    // this is not used, block_unit_size goes with cache
        cp->disk_sector_size = (size_t) reader_thread->base->disk_sector_size;


        guint64 hit_count = 0, miss_count = 0;
        gboolean (*add_element)(struct cache*, cache_line * cp);
        add_element = cache->core->add_element;

        read_one_element(reader_thread, cp);

        // this must happen after read, otherwise cp size and type unknown
        if (cache->core->consider_size && cp->type == 'l' && cp->disk_sector_size != 0) {    // && cp->size != 0 is removed due to trace dirty
            add_element = cache->core->add_element_withsize;
            if (add_element == NULL) {
                ERROR("using size with profiling, cannot find add_element_withsize\n");
                abort();
            }
        }


        while (cp->valid) {
            /* new 170428
             add size into consideration, this only affects the traces with cache_size column,
             currently CPHY traces are affected
             the default size for each block is 512 bytes */
            if (add_element(cache, cp)){
                hit_count ++;
            }
            else{
                miss_count ++;
            }
            read_one_element(reader_thread, cp);
        }

        result[order]->hit_count = (long long) hit_count;
        result[order]->miss_count = (long long) miss_count;
        result[order]->total_count = hit_count + miss_count;
        result[order]->hit_ratio = (double) hit_count / (hit_count + miss_count);
        result[order]->miss_ratio = 1 - result[order]->hit_ratio;
        result[order]->other_data = cache->core->eviction_array;


        // clean up
        g_mutex_lock(&(params->mtx));
        (*(params->progress)) ++ ;
        g_mutex_unlock(&(params->mtx));

        g_free(cp);
        close_reader_unique(reader_thread);
        cache->core->destroy_unique(cache);
    }


return_res_t** profiler(reader_t* reader_in,
                        struct_cache* cache_in,
                        int num_of_threads_in,
                        int bin_size_in,
                        profiler_type_e prof_type) {
    /**
     if profiling from the very beginning, then set begin_pos=0,
     if porfiling till the end of trace, then set end_pos=-1 or the length of trace+1;
     return results do not include size 0
     **/

    long i;
    guint64 progress = 0;


    // initialization
    int num_of_threads = num_of_threads_in;
    int bin_size = bin_size_in;
    long num_of_bins = ceil((double) cache_in->core->size / bin_size) + 1;

    if (reader_in->base->total_num == -1)
        get_num_of_req(reader_in);

    // check whether profiling considering size or not
    if (cache_in->core->consider_size && reader_in->base->data_type == 'l'
            && reader_in->base->disk_sector_size != 0 &&
            cache_in->core->block_unit_size != 0) {    // && cp->size != 0 is removed due to trace dirty
        INFO("use block size %d, disk sector size %d in profiling\n",
             cache_in->core->block_unit_size,
             reader_in->base->disk_sector_size);
    }


    // create the result storage area and caches of varying sizes
    return_res_t** result = g_new(return_res_t*, num_of_bins);

    for (i = 0; i < num_of_bins; i++) {
        result[i] = g_new0(return_res_t, 1);
        result[i]->cache_size = bin_size * (i);
//        if (prof_type == e_hit_result)
//            result[i]->other_data = g_new(gboolean*, num_of_bins);
    }
    result[0]->miss_ratio = 1;


    // build parameters and send to thread pool
    mt_param_gp_t* params = g_new0(mt_param_gp_t, 1);
    params->reader = reader_in;
    params->cache = cache_in;
    params->result = result;
    params->bin_size = (guint) bin_size;
    params->progress = &progress;
    g_mutex_init(&(params->mtx));


    // build the thread pool
    GThreadPool *gthread_pool;
    if (prof_type == e_hit)
        gthread_pool = g_thread_pool_new ( (GFunc) profiler_thread,
                                          (gpointer)params, num_of_threads, TRUE, NULL);
    else if (prof_type == e_eviction_age)
        gthread_pool = g_thread_pool_new ( (GFunc) get_eviction_age_thread,
                                          (gpointer)params, num_of_threads, TRUE, NULL);
    else if (prof_type == e_hit_result)
        gthread_pool = g_thread_pool_new ( (GFunc) get_hit_result_thread,
                                          (gpointer)params, num_of_threads, TRUE, NULL);
    else if (prof_type == e_evictions)
        gthread_pool = g_thread_pool_new ( (GFunc) get_evictions_thread,
                                          (gpointer)params, num_of_threads, TRUE, NULL);
    else{
        ERROR("unknown profiler type %d\n", prof_type);
        abort();
    }

    if (gthread_pool == NULL)
        ERROR("cannot create thread pool in general profiler\n");


    for (i = 1; i < num_of_bins; i++) {
        if ( g_thread_pool_push (gthread_pool, GUINT_TO_POINTER(i), NULL) == FALSE)
            ERROR("cannot push data into thread in generalprofiler\n");
    }

    while (progress < (guint64)num_of_bins - 1) {
        fprintf(stderr, "%.2f%%\n", ((double)progress) / (num_of_bins - 1) * 100);
        sleep(1);
//            fprintf(stderr, "\033[A\033[2K\r");
    }

    g_thread_pool_free (gthread_pool, FALSE, TRUE);

    /* change hit count, now it is accumulated hit_count,
     * the returned hit count should be the increased
     * hit count from last cache size to current cache size */
    for (i = num_of_bins - 1; i >= 1; i--)
        result[i]->hit_count -= result[i - 1]->hit_count;

    // clean up
    g_mutex_clear(&(params->mtx));
    g_free(params);
    // needs to free result later

    return result;
}


static void traverse_trace(reader_t* reader, struct_cache* cache) {

    // create cache lize struct and initialization
    cache_line* cp = new_cacheline();
    cp->type = cache->core->data_type;
    cp->block_unit_size = (size_t) reader->base->block_unit_size;

    gboolean (*add_element)(struct cache*, cache_line * cp);
    add_element = cache->core->add_element;

    read_one_element(reader, cp);
    while (cp->valid) {
        add_element(cache, cp);
        read_one_element(reader, cp);
    }

    // clean up
    g_free(cp);
    reset_reader(reader);

}


//static void get_evict_err(reader_t* reader, struct_cache* cache){
//
//    cache->core->bp_pos = 1;
//    cache->core->evict_err_array = g_new0(gdouble, reader->sdata->break_points->array->len-1);
//
//    // create cache lize struct and initialization
//    cache_line* cp = new_cacheline();
//    cp->type = cache->core->data_type;
//    cp->block_unit_size = (size_t) reader->base->block_unit_size;
//
//    gboolean (*add_element)(struct cache*, cache_line* cp);
//    add_element = cache->core->add_element;
//
//    read_one_element(reader, cp);
//    while (cp->valid){
//        add_element(cache, cp);
//        read_one_element(reader, cp);
//    }
//
//    // clean up
//    g_free(cp);
//    reset_reader(reader);
//
//}





//gdouble* LRU_evict_err_statistics(reader_t* reader_in, struct_cache* cache_in, guint64 time_interval){
//
//    gen_breakpoints_realtime(reader_in, time_interval, -1);
//    cache_in->core->bp = reader_in->sdata->break_points;
//    cache_in->core->record_level = 2;
//
//
//    struct optimal_init_params* init_params = g_new0(struct optimal_init_params, 1);
//    init_params->reader = reader_in;
//    init_params->ts = 0;
//    struct_cache* optimal;
//    if (cache_in->core->data_type == 'l')
//        optimal = optimal_init(cache_in->core->size, 'l', 0, (void*)init_params);
//    else{
//        printf("other cache data type not supported in LRU_evict_err_statistics in generalProfiler\n");
//        exit(1);
//    }
//    optimal->core->record_level = 1;
//    optimal->core->eviction_array_len = reader_in->base->total_num;
//    optimal->core->bp = reader_in->sdata->break_points;
//
//    if (reader_in->base->total_num == -1)
//        get_num_of_req(reader_in);
//
//    if (reader_in->base->type == 'v')
//        optimal->core->eviction_array = g_new0(guint64, reader_in->base->total_num);
//    else
//        optimal->core->eviction_array = g_new0(gchar*, reader_in->base->total_num);
//
//    // get oracle
//    traverse_trace(reader_in, optimal);
//
//    cache_in->core->oracle = optimal->core->eviction_array;
//
//
//    get_evict_err(reader_in, cache_in);
//
//    optimal_destroy(optimal);
//
//
//    return cache_in->core->evict_err_array;
//}


#ifdef __cplusplus
}
#endif
