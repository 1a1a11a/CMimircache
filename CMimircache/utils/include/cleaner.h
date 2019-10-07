//
//  cleaner.h
//  LRUAnalyzer
//
//  Created by Juncheng on 5/26/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef cleaner_h
#define cleaner_h

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "cache.h"


#ifdef __cplusplus
extern "C"
{
#endif


void simple_key_value_destroyer(gpointer data);
void simple_g_key_value_destroyer(gpointer data);
void cacheobj_destroyer(gpointer data);
void g_slist_destroyer(gpointer data);
void gqueue_destroyer(gpointer data);
void pqueue_node_destroyer(gpointer data);


#ifdef __cplusplus
}
#endif


#endif /* cleaner_h */
