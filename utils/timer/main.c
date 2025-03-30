/****************************************************************************
 *
 * MODULE:       $ETC/current_time_s_ms
 * AUTHOR(S):    Markus Neteler
 * PURPOSE:      timer for benchmarking. Prints current time in
 *               seconds.milliseconds
 *
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#ifdef _MSC_VER
#include <windows.h>
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <string.h>
#include <stdio.h>

int main(void)
{
    struct timeval t;

    if (gettimeofday(&t, NULL) != 0) {
        fprintf(stderr, "gettimeofday error");
        return 1;
    }
    fprintf(stdout, "%li.%li\n", t.tv_sec, (long)t.tv_usec);
    fflush(stdout);

    return 0;
}
