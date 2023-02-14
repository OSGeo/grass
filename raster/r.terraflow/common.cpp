/****************************************************************************
 *
 *  MODULE:        r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#include <sys/types.h>
#ifdef USE_LARGEMEM
#include <sys/mman.h>
#endif
#include <ctype.h>

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || defined(_MSC_VER)
#include <ostream>
#else
#include <ostream.h>
#endif

#include <iostream>

#include "common.h"

/* globals */

statsRecorder *stats = NULL;
userOptions *opt = NULL;
struct Cell_head *region = NULL;
dimension_type nrows = 0, ncols = 0;

size_t parse_number(const char *s)
{
    size_t n, mult = 1;
    int len = strlen(s);
    if (isalpha(s[len - 1])) {
        switch (s[len - 1]) {
        case 'M':
            mult = 1 << 20;
            break;
        case 'K':
            mult = 1 << 10;
            break;
        default:
            cerr << "bad number format: " << s << endl;
            exit(-1);
            break;
        }
        /* s[len-1] = '\0'; not needed, as it will stop at first invalid char */
    }
    n = atol(s);
    return n * mult;
}

/* ---------------------------------------------------------------------- */
/* is anybody using this?? DELETE ! */

#ifdef USE_LARGEMEM

void *LargeMemory::ptr[LM_HIST];
size_t LargeMemory::len[LM_HIST];
int LargeMemory::next = 0;

#ifndef MAP_ANON
#define MAP_ANON 0
#endif

#ifdef __alpha
#undef MAP_FAILED
#define MAP_FAILED (caddr_t) - 1L
#endif

void *LargeMemory::alloc(size_t leng)
{
    assert(next < LM_HIST);
    void *p = mmap(0, leng, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    len[next] = leng;
    ptr[next] = p;
    next++;
    if (stats) {
        char buf[BUFSIZ], buf2[32];
        snprintf(buf, BUFSIZ, "allocated large memory: %s 0x%lX",
                 formatNumber(buf2, leng), (unsigned long)p);
        stats->comment(buf);
    }
    return p;
}

void LargeMemory::free(void *p)
{
    int z;
    int i;
    for (i = next - 1; i >= 0; i--) {
        if (ptr[i] == p)
            break;
    }
    assert(i < next && i >= 0); /* must have been allocated before */

#if (defined sun && defined sparc)
    z = munmap((caddr_t)p, len[i]);
#else
    z = munmap(p, len[i]);
#endif
    if (z < 0) {
        perror("munmap");
    }

    if (stats) {
        char buf[BUFSIZ], buf2[32];
        snprintf(buf, BUFSIZ, "freed large memory: %s 0x%lX",
                 formatNumber(buf2, len[i]), (unsigned long)p);
        stats->comment(buf);
    }

    next--;
    if (next) {
        ptr[i] = ptr[next];
        len[i] = len[next];
    }
}

#endif /*  USE_LARGEMEM */
