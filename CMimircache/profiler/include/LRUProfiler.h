//
//  LRUAnalyzer.h
//  LRUAnalyzer
//
//  Created by Juncheng on 5/24/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef LRUAnalyzer_h
#define LRUAnalyzer_h

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <math.h>
#include "splay.h"
#include "reader.h"
#include "cleaner.h"
#include "const.h"
#include "pqueue.h"
#include "murmur3.h"


#ifdef __cplusplus
extern "C"
{
#endif


guint64* get_hit_count_seq   (reader_t* reader,
                              gint64 size);
double* get_hit_ratio_seq     (reader_t* reader,
                              gint64 size);
double* get_miss_rate_seq    (reader_t* reader,
                              gint64 size);
gint64* get_reuse_dist_seq   (reader_t* reader);

gint64* get_future_reuse_dist(reader_t* reader);

gint64* get_dist_to_last_access(reader_t* reader);

gint64* get_reuse_time(reader_t* reader);

guint64* get_hit_count_seq_shards(reader_t* reader,
                                  gint64 size,
                                  double sample_ratio);

double* get_hit_ratio_seq_shards(reader_t* reader,
                                gint64 size,
                                double sample_ratio);

guint64* get_hit_count_phase(reader_t* reader,
                            gint64 current_phase,
                            gint64 num_phases);

double* get_hit_ratio_phase(reader_t* reader,
                           gint64 current_phase,
                           gint64 num_phases);

guint64* get_hitcount_withsize_seq(reader_t* reader,
                                   gint64 size,
                                   int block_size);
double* get_hitrate_withsize_seq(reader_t* reader,
                                 gint64 size,
                                 int block_size);

void cal_save_future_reuse_dist(reader_t* reader,
                                char* save_file_loc);

void cal_save_reuse_dist(reader_t * const reader,
                         const char *const save_file_loc,
                         const int reuse_type);


void load_reuse_dist(reader_t * const reader,
                     const char * const load_file_loc,
                     const int reuse_type);


#ifdef __cplusplus
}
#endif


#endif /* LRUAnalyzer_h */
