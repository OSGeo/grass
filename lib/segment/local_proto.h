#ifndef SEGMENT_LOCAL_H
#define SEGMENT_LOCAL_H

#include <grass/segment.h>

/* address.c */
int segment_address(const SEGMENT *, off_t, off_t, int *, int *);
int segment_address_fast(const SEGMENT *, off_t, off_t, int *, int *);
int segment_address_slow(const SEGMENT *, off_t, off_t, int *, int *);

/* pagein.c */
int segment_pagein(SEGMENT *, int);

/* pageout.c */
int segment_pageout(SEGMENT *, int);

/* seek.c */
int segment_seek(const SEGMENT *, int, int);
int segment_seek_fast(const SEGMENT *, int, int);
int segment_seek_slow(const SEGMENT *, int, int);

/* setup.c */
int segment_setup(SEGMENT *);


#endif /* SEGMENT_LOCAL_H */

