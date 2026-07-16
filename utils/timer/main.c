/****************************************************************************
 *
 * MODULE:       $ETC/current_time_s_ms
 * AUTHOR(S):    Markus Neteler
 * PURPOSE:      timer for benchmarking. Prints current time in
 *               seconds.milliseconds
 *
 * SPDX-FileCopyrightText: 2003 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/
#include <sys/time.h>
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

