//
//  utils.h
//  mimircache
//
//  Created by Juncheng on 6/2/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef UTILS_h
#define UTILS_h

#include "const.h" 

#include <stdio.h>
#include <math.h> 
#include <glib.h> 
#include <pthread.h>
#include <unistd.h>

#include <sched.h>
#include <sys/sysinfo.h>



int set_thread_affinity(pthread_t tid);

guint get_n_cores(void);



#endif /* UTILS_h */
