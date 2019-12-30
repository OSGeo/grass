
/****************************************************************************
 *
 * MODULE:       $ETC/current_time_s_ms
 * AUTHOR(S):    Markus Neteler
 * PURPOSE:      timer for benchmarking. Prints current time in seconds.milliseconds
 *
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <string.h>
#include <stdio.h>

int main()
{
    struct timeval t;

    if (gettimeofday(&t, NULL) == -1) {
	fprintf(stderr, "gettimeofday error");
	return 1;
    }
    fprintf(stdout, "%li.%li\n", t.tv_sec, t.tv_usec);
    fflush(stdout);

    return 0;
}
