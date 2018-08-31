#ifndef Segment_LOCAL_H
#define Segment_LOCAL_H

#include <grass/segment.h>

/* internal functions */

/* address.c */
int seg_address(const SEGMENT *, off_t, off_t, int *, int *);
int seg_address_fast(const SEGMENT *, off_t, off_t, int *, int *);
int seg_address_slow(const SEGMENT *, off_t, off_t, int *, int *);

/* pagein.c */
int seg_pagein(SEGMENT *, int);

/* pageout.c */
int seg_pageout(SEGMENT *, int);

/* seek.c */
int seg_seek(const SEGMENT *, int, int);
int seg_seek_fast(const SEGMENT *, int, int);
int seg_seek_slow(const SEGMENT *, int, int);

/* setup.c */
int seg_setup(SEGMENT *);

#endif /* Segment_LOCAL_H */

