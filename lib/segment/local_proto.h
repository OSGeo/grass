#ifndef Segment_LOCAL_H
#define Segment_LOCAL_H

#include <grass/segment.h>

/* address.c */
int Segment_address(const SEGMENT *, off_t, off_t, int *, int *);
int Segment_address_fast(const SEGMENT *, off_t, off_t, int *, int *);
int Segment_address_slow(const SEGMENT *, off_t, off_t, int *, int *);

/* pagein.c */
int Segment_pagein(SEGMENT *, int);

/* pageout.c */
int Segment_pageout(SEGMENT *, int);

/* seek.c */
int Segment_seek(const SEGMENT *, int, int);
int Segment_seek_fast(const SEGMENT *, int, int);
int Segment_seek_slow(const SEGMENT *, int, int);

/* setup.c */
int Segment_setup(SEGMENT *);


#endif /* Segment_LOCAL_H */

