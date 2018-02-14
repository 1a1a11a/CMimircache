//
//  const.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//




#ifndef CONST_H
#define CONST_H


#ifdef __cplusplus
extern "C" {
#endif


#define cache_line_label_size 128
#define CACHE_LINE_LABEL_SIZE 128
#define FILE_LOC_STR_SIZE 1024

#define KRESET  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"


#ifndef _GNU_SOURCE
    #define _GNU_SOURCE             /* for sched in utils.h */
#endif






#define NORMAL_REUSE_DISTANCE                           1
#define FUTURE_REUSE_DISTANCE                           2
#define NORMAL_DISTANCE                                 3
#define REUSE_TIME                                      4

//#define DEFAULT_SECTOR_SIZE                             512

#define SUPPRESS_FUNCTION_NO_USE_WARNING(f) (void)f



//#define SANITY_CHECK 1
//#define _DEBUG
#define ML
#undef ML

#undef __DEBUG__
#undef _DEBUG


#ifdef __cplusplus
    }
#endif

#endif
