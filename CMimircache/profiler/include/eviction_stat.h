//
//  eviction_stat.h
//  eviction_stat
//
//  Created by Juncheng on 5/24/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef eviction_stat_h
#define eviction_stat_h

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include "reader.h"
#include "glib_related.h"
#include "cache.h" 
#include "const.h"



#ifdef __cplusplus
extern "C"
{
#endif


typedef enum{
    evict_reuse_dist,
    evict_freq,
    evict_freq_accumulatve,
    
    evict_data_classification,
    other
    

}evict_stat_type;






gint64* eviction_stat(reader_t* reader_in, struct_cache* cache, evict_stat_type stat_type);


#ifdef __cplusplus
}
#endif


#endif /* eviction_stat_h */
