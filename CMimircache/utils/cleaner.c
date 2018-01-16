//
//  cleaner.c
//  LRUAnalyzer
//
//  Created by Juncheng on 5/26/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#include "cleaner.h"
#include "pqueue.h"


#ifdef __cplusplus
extern "C"
{
#endif


void simple_key_value_destroyer(gpointer data) {
    free(data);
}

void simple_g_key_value_destroyer(gpointer data) {
    g_free(data);
}

void g_slist_destroyer(gpointer data){
//    if (data!=NULL)
        g_slist_free_full((GSList*)data, simple_g_key_value_destroyer);
}

void gqueue_destroyer(gpointer data) {
    g_queue_free_full(data, simple_g_key_value_destroyer);
}

void pqueue_node_destroyer(gpointer data) {
    g_free(((pq_node_t*)data)->item);
    g_free((pq_node_t*)data);
}

void ML_value_destroyer(gpointer data){
    ;

}


#ifdef __cplusplus
}
#endif