//
//  heatmap_thread.c
//  mimircache
//
//  Created by Juncheng on 5/24/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#include "heatmap.h"
#include "Optimal.h"
#include "pqueue.h"


#ifdef __cplusplus
extern "C"
{
#endif
    
    
    /**
     * thread function for computing nonLRU heatmap of type start_time_end_time
     *
     * @param data contains order+1
     * @param user_data passed in param
     */
    void hm_nonLRU_hr_st_et_thread(gpointer data, gpointer user_data){
        guint64 i, j, hit_count_all, miss_count_all, hit_count_interval, miss_count_interval;
        mt_params_hm_t* params = (mt_params_hm_t*) user_data;
        reader_t* reader_thread = clone_reader(params->reader);
        GArray* break_points = params->break_points;
        guint64* progress = params->progress;
        draw_dict* dd = params->dd;
        struct cache* cache = params->cache->core->cache_init(params->cache->core->size,
                                                              params->cache->core->data_type,
                                                              params->cache->core->block_size,
                                                              params->cache->core->cache_init_params);
        
        guint64 order = GPOINTER_TO_INT(data)-1;
        int interval_hit_ratio_b = params->interval_hit_ratio_b;
        double ewma_coefficient_lf = params->ewma_coefficient_lf;
        
        hit_count_all = 0;
        miss_count_all = 0;
        
        
        // create cache lize struct and initialization
        request_t* cp = new_req_struct();
        cp->label_type = cache->core->data_type;
        cp->block_unit_size = (size_t) reader_thread->base->block_unit_size;
        
        guint64 N = g_array_index(break_points, guint64, order);
        if (N != skip_N_elements(reader_thread, N)){
            ERROR("failed to skip %lu requests\n", (unsigned long) N);
            exit(1);
        };
        
        // this is for synchronizing ts in cache, which is used as index for access next_access array
        if (cache->core->type == e_Optimal)
            ((struct optimal_params*)(cache->cache_params))->ts = g_array_index(break_points, guint64, order);
        
        for (i=order; i<break_points->len-1; i++){
            hit_count_interval = 0;
            miss_count_interval = 0;
            for(j=0; j< g_array_index(break_points, guint64, i+1) - g_array_index(break_points, guint64, i); j++){
                read_one_element(reader_thread, cp);
                if (cache->core->add_element(cache, cp))
                    hit_count_interval++;
                else
                    miss_count_interval++;
            }
            hit_count_all += hit_count_interval;
            miss_count_all += miss_count_interval;
            if (interval_hit_ratio_b){
                if (i == order)
                    // no decay for first pixel
                    dd->matrix[order][i] = (double)(hit_count_all)/(hit_count_all + miss_count_all);
                else {
                    dd->matrix[order][i] = dd->matrix[order][i-1] * ewma_coefficient_lf +
                    (1 - ewma_coefficient_lf) * (double)(hit_count_interval)/(hit_count_interval + miss_count_interval);
                }
            }
            else
                dd->matrix[order][i] = (double)(hit_count_all)/(hit_count_all + miss_count_all);
        }
        
        
        // clean up
        g_mutex_lock(&(params->mtx));
        (*progress) ++ ;
        g_mutex_unlock(&(params->mtx));
        g_free(cp);
        
        
        close_reader_unique(reader_thread);
        cache->core->destroy_unique(cache);
    }
    
    
    /**
     * thread function for computing nonLRU heatmap of type interval_size
     *
     * @param data contains order+1
     * @param user_data passed in param
     */
    void hm_nonLRU_hr_interval_size_thread(gpointer data, gpointer user_data){
        guint64 i, j, hit_count_interval, miss_count_interval;
        mt_params_hm_t* params = (mt_params_hm_t*) user_data;
        reader_t* reader_thread = clone_reader(params->reader);
        GArray* break_points = params->break_points;
        guint64* progress = params->progress;
        draw_dict* dd = params->dd;
        
        struct cache* cache = NULL;
        request_t* cp = NULL;
        
        guint64 order = GPOINTER_TO_INT(data)-1;
        guint64 cache_size = params->bin_size * order;
        if (cache_size == 0){
            for (i=0; i<break_points->len-1; i++)
                dd->matrix[i][order] = 0;
            
        }
        else {
            cache = params->cache->core->cache_init(cache_size,
                                                    params->cache->core->data_type,
                                                    params->cache->core->block_size,
                                                    params->cache->core->cache_init_params);
            
            double ewma_coefficient_lf = params->ewma_coefficient_lf;
            
            
            // create cache lize struct and initialization
            cp = new_req_struct();
            cp->label_type = cache->core->data_type;
            cp->block_unit_size = (size_t) reader_thread->base->block_unit_size;
            
                        
            for (i=0; i<break_points->len-1; i++){
                hit_count_interval = 0;
                miss_count_interval = 0;
                for(j=0; j< g_array_index(break_points, guint64, i+1) - g_array_index(break_points, guint64, i); j++){
                    read_one_element(reader_thread, cp);
                    if (cache->core->add_element(cache, cp))
                        hit_count_interval++;
                    else
                        miss_count_interval++;
                }
                if (i == 0)
                    // no decay for first pixel
                    dd->matrix[i][order] = (double)(hit_count_interval)/(hit_count_interval + miss_count_interval);
                else {
                    dd->matrix[i][order] = dd->matrix[i-1][order] * ewma_coefficient_lf +
                    (1 - ewma_coefficient_lf) * (double)(hit_count_interval)/(hit_count_interval + miss_count_interval);
                }
            }
        }
        
        // clean up
        g_mutex_lock(&(params->mtx));
        (*progress) ++ ;
        g_mutex_unlock(&(params->mtx));
        if (cp != NULL)
            g_free(cp);
        
        close_reader_unique(reader_thread);
        if (cache != NULL)
            cache->core->destroy_unique(cache);
    }
    
    
    /**
     * thread function for computing effective size heatmap of type start_time_end_time
     *
     * @param data contains order+1
     * @param user_data passed in param
     */
    void hm_effective_size_thread(gpointer data, gpointer user_data){
        gint64 i, j;
        mt_params_hm_t* params = (mt_params_hm_t*) user_data;
        reader_t* reader_thread = clone_reader(params->reader);
        GArray* break_points = params->break_points;
        gint64 bin_size = params->bin_size;
        gboolean use_percent = params->use_percent; 
        gint64 order = GPOINTER_TO_INT(data);
        gint64 cache_size = order * bin_size;
        
        guint64* progress = params->progress;
        draw_dict* dd = params->dd;

        cache_t* cache = params->cache->core->cache_init((unsigned long) cache_size,
                                                         params->cache->core->data_type,
                                                         params->cache->core->block_size,
                                                         params->cache->core->cache_init_params);
        
        guint64 *effective_cache_size = malloc(sizeof(guint64) * reader_thread->base->total_num);
        
        
        // create request struct and initialization
        request_t* cp = new_req_struct();
        cp->label_type = cache->core->data_type;
        cp->block_unit_size = (size_t) reader_thread->base->block_unit_size;
        
        
        optimal_params_t* opt_params = cache->cache_params;
        
        
        guint64 current_effective_size = 0;
        gint64 last_ts = 0;
        guint64 cur_ts = 0;
        gpointer item;
        GHashTable* last_access_time_ght;
        
        if (cp->label_type == 'l'){
            last_access_time_ght = g_hash_table_new_full(g_int64_hash, g_int64_equal,
                                                            simple_g_key_value_destroyer,
                                                            NULL);
        }
        
        else { // (cp->label_type == 'c')
            last_access_time_ght = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                           simple_g_key_value_destroyer,
                                                           NULL);
        }


        read_one_element(reader_thread, cp);

        while (cp->valid){
            // record last access
            if (cp->label_type == 'l'){
                item = (gpointer)g_new(guint64, 1);
                *(guint64*)item = *(guint64*)(cp->label_ptr);
            }
            else{
                item = (gpointer)g_strdup((gchar*)(cp->label_ptr));
            }
            g_hash_table_insert(last_access_time_ght, item, GUINT_TO_POINTER(cur_ts+1));
            
            // add to cache
            if (cache->core->check_element(cache, cp)){
                cache->core->__update_element(cache, cp);
            }
            else{
                current_effective_size += 1;
                cache->core->__insert_element(cache, cp);
            }

            /** Optimal only
             *  find elements that will never be accessed
             */
            if (cache->core->type == e_Optimal){
                pq_node_t* node = pqueue_peek(opt_params->pq);
                if (node->pri.pri1 == G_MAXINT64){
                    cache->core->__evict_element(cache, NULL);
                    current_effective_size -= 1;
                }
            }

            /** check whether the cache is full,
             *  if full, then we need evict, however, this eviction should happen long time ago
             *  in other words, this item should not be added to cache
             *  so need to find out when this item was added and reduced the size of all time after it
             */
            while ( (long)cache->core->get_current_size(cache) > cache->core->size ){
                item = cache->core->__evict_with_return(cache, cp);
                last_ts = GPOINTER_TO_UINT(g_hash_table_lookup(last_access_time_ght, item)) - 1;
                if (last_ts < 0){
                    ERROR("last access time should be larger than 0, current %ld", (long) last_ts);
                    abort();
                }
                current_effective_size -= 1;
                for (i = last_ts; i<cur_ts; i++){
                    if (effective_cache_size[i] == 0){
                        ERROR("effective cache size at %ld is already 0, cannot decrease, "
                              "last_ts %ld, cur_ts %lu, cache_size %lu\n",
                              (long) i, (unsigned long) last_ts, (unsigned long) cur_ts, (unsigned long) cache_size);
                        abort();
                    }
                    effective_cache_size[i] -= 1;
                }
                g_free(item);
            }
            
            // not cur_ts - 1 because ts has not been increased
            effective_cache_size[cur_ts] = current_effective_size;

            if (current_effective_size > order * bin_size){
                ERROR("effective size %lu, cache size %lu\n",
                        (unsigned long) current_effective_size, (unsigned long) (order*bin_size));
                abort();
            }
            
            cur_ts += 1;
            if (cache->core->type == e_Optimal)
                (opt_params->ts) ++ ;
            
            
            read_one_element(reader_thread, cp);
        }

        // now calculate the stat information
        guint64 sum_effective_size = 0;
        for (i=0; i<break_points->len-1; i++){
            sum_effective_size = 0;
            for(j=g_array_index(break_points, guint64, i); j< g_array_index(break_points, guint64, i+1); j++){
                sum_effective_size += effective_cache_size[j];
            }
            dd->matrix[i][order-1] = (double)(sum_effective_size)/
                (g_array_index(break_points, guint64, i+1) - g_array_index(break_points, guint64, i));
            
            if (dd->matrix[i][order-1] / cache_size > 1){
                ERROR("cache size %lu, %lu th bp, effective size %lf\n",
                        (unsigned long) cache_size, (unsigned long) i, dd->matrix[i][order-1]);
                for(j=g_array_index(break_points, guint64, i); j< g_array_index(break_points, guint64, i+1); j++)
                    printf("effective size at %lu (%lu-%lu): %lu\n", (unsigned long) j,
                           (unsigned long) g_array_index(break_points, guint64, i),
                           (unsigned long) g_array_index(break_points, guint64, i+1),
                           (unsigned long) effective_cache_size[j]);
                abort(); 
            }
            
            if (use_percent)
                dd->matrix[i][order-1] = dd->matrix[i][order-1] / cache_size;
        }
        
        
        // clean up
        g_mutex_lock(&(params->mtx));
        (*progress) ++ ;
        g_mutex_unlock(&(params->mtx));
        g_free(cp);
        g_hash_table_destroy(last_access_time_ght);
        
        close_reader_unique(reader_thread);
        cache->core->destroy_unique(cache);
    }

    
    
#ifdef __cplusplus
}
#endif
