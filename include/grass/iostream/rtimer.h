/****************************************************************************
 *
 *  MODULE:     iostream
 *

 *  COPYRIGHT (C) 2007 Laura Toma
 *
 *

 *  Iostream is a library that implements streams, external memory
 *  sorting on streams, and an external memory priority queue on
 *  streams. These are the fundamental components used in external
 *  memory algorithms.

 * Credits: The library was developed by Laura Toma.  The kernel of
 * class STREAM is based on the similar class existent in the GPL TPIE
 * project developed at Duke University. The sorting and priority
 * queue have been developed by Laura Toma based on communications
 * with Rajiv Wickremesinghe. The library was developed as part of
 * porting Terraflow to GRASS in 2001.  PEARL upgrades in 2003 by
 * Rajiv Wickremesinghe as part of the Terracost project.

 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.  *
 *  **************************************************************************/

#ifndef RTIMER_H
#define RTIMER_H

#ifdef _WIN32

#include <time.h>
#include <stdio.h>
#include <string.h>
#ifdef __MINGW32__
#include <strings.h>
#endif
typedef struct {
    time_t tv1, tv2;
} Rtimer;

#define rt_start(rt)                           \
    if ((time(&(rt.tv1)) == ((time_t) - 1))) { \
        perror("time");                        \
        exit(1);                               \
    }

/* doesn't really stop, just updates endtimes */
#define rt_stop(rt)                            \
    if ((time(&(rt.tv2)) == ((time_t) - 1))) { \
        perror("time");                        \
        exit(1);                               \
    }

#define rt_u_useconds(rt) rt_w_useconds(rt)

#define rt_s_useconds(rt) rt_w_useconds(rt)

#define rt_w_useconds(rt) (1.0e6 * (rt.tv2 - rt.tv1))

#else /* __MINGW32__ */

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

typedef struct {
    struct rusage rut1, rut2;
    struct timeval tv1, tv2;
} Rtimer;

#define rt_start(rt)                              \
    if ((getrusage(RUSAGE_SELF, &rt.rut1) < 0) || \
        (gettimeofday(&(rt.tv1), NULL) < 0)) {    \
        perror("rusage/gettimeofday");            \
        exit(1);                                  \
    }

/* doesn't really stop, just updates endtimes */
#define rt_stop(rt)                               \
    if ((getrusage(RUSAGE_SELF, &rt.rut2) < 0) || \
        (gettimeofday(&(rt.tv2), NULL) < 0)) {    \
        perror("rusage/gettimeofday");            \
        exit(1);                                  \
    }

#define rt_u_useconds(rt)                          \
    (((double)rt.rut2.ru_utime.tv_usec +           \
      (double)rt.rut2.ru_utime.tv_sec * 1000000) - \
     ((double)rt.rut1.ru_utime.tv_usec +           \
      (double)rt.rut1.ru_utime.tv_sec * 1000000))

#define rt_s_useconds(rt)                          \
    (((double)rt.rut2.ru_stime.tv_usec +           \
      (double)rt.rut2.ru_stime.tv_sec * 1000000) - \
     ((double)rt.rut1.ru_stime.tv_usec +           \
      (double)rt.rut1.ru_stime.tv_sec * 1000000))

#define rt_w_useconds(rt)                                         \
    (((double)rt.tv2.tv_usec + (double)rt.tv2.tv_sec * 1000000) - \
     ((double)rt.tv1.tv_usec + (double)rt.tv1.tv_sec * 1000000))

#endif /* __MINGW32__ */

/* not required to be called, but makes values print as 0.
   obviously a hack */
#define rt_zero(rt)        bzero(&(rt), sizeof(Rtimer));

#define rt_seconds(rt)     (rt_w_useconds(rt) / 1000000)

#define rt_sprint(buf, rt) rt_sprint_safe(buf, rt)

char *rt_sprint_safe(char *buf, Rtimer rt);

#endif /* RTIMER_H */
