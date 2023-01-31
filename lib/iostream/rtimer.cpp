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

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
// #include <rtimer.h>
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
#include <grass/iostream/rtimer.h>

#define BUFMAX 256

char *rt_sprint_safe(char *buf, Rtimer rt)
{
    if (rt_w_useconds(rt) == 0) {
        snprintf(buf, BUFMAX, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
                 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    }
    else {
        snprintf(buf, BUFMAX, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
                 rt_u_useconds(rt) / 1000000,
                 100.0 * rt_u_useconds(rt) / rt_w_useconds(rt),
                 rt_s_useconds(rt) / 1000000,
                 100.0 * rt_s_useconds(rt) / rt_w_useconds(rt),
                 rt_w_useconds(rt) / 1000000,
                 100.0 * (rt_u_useconds(rt) + rt_s_useconds(rt)) /
                     rt_w_useconds(rt));
=======
// #include <rtimer.h>
#include <grass/iostream/rtimer.h>

=======
// #include <rtimer.h>
#include <grass/iostream/rtimer.h>

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
#include <grass/iostream/rtimer.h>

#define BUFMAX 256

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
char *rt_sprint_safe(char *buf, Rtimer rt)
{
    if (rt_w_useconds(rt) == 0) {
        snprintf(buf, BUFMAX, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
                 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    }
    else {
<<<<<<< HEAD
        sprintf(buf, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
                rt_u_useconds(rt) / 1000000,
                100.0 * rt_u_useconds(rt) / rt_w_useconds(rt),
                rt_s_useconds(rt) / 1000000,
                100.0 * rt_s_useconds(rt) / rt_w_useconds(rt),
                rt_w_useconds(rt) / 1000000,
                100.0 * (rt_u_useconds(rt) + rt_s_useconds(rt)) /
                    rt_w_useconds(rt));
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
=======
=======
#include <grass/iostream/rtimer.h>

#define BUFMAX 256

char *rt_sprint_safe(char *buf, Rtimer rt)
{
    if (rt_w_useconds(rt) == 0) {
        snprintf(buf, BUFMAX, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
                 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    }
    else {
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
        snprintf(buf, BUFMAX, "[%4.2fu (%.0f%%) %4.2fs (%.0f%%) %4.2f %.1f%%]",
                 rt_u_useconds(rt) / 1000000,
                 100.0 * rt_u_useconds(rt) / rt_w_useconds(rt),
                 rt_s_useconds(rt) / 1000000,
                 100.0 * rt_s_useconds(rt) / rt_w_useconds(rt),
                 rt_w_useconds(rt) / 1000000,
                 100.0 * (rt_u_useconds(rt) + rt_s_useconds(rt)) /
                     rt_w_useconds(rt));
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }
    return buf;
}
