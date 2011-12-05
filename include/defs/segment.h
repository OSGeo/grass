#ifndef GRASS_SEGMENTDEFS_H
#define GRASS_SEGMENTDEFS_H

int segment_address(const SEGMENT *, int, int, int *, int *);
int segment_address_fast(const SEGMENT *, int, int, int *, int *);
int segment_address_slow(const SEGMENT *, int, int, int *, int *);
int segment_flush(SEGMENT *);
int segment_format(int, int, int, int, int, int);
int segment_format_nofill(int, int, int, int, int, int);
int segment_get(SEGMENT *, void *, int, int);
int segment_get_row(const SEGMENT *, void *, int);
int segment_init(SEGMENT *, int, int);
int segment_pagein(SEGMENT *, int);
int segment_pageout(SEGMENT *, int);
int segment_put(SEGMENT *, const void *, int, int);
int segment_put_row(const SEGMENT *, const void *, int);
int segment_release(SEGMENT *);
int segment_seek(const SEGMENT *, int, int);
int segment_seek_fast(const SEGMENT *, int, int);
int segment_seek_slow(const SEGMENT *, int, int);
int segment_setup(SEGMENT *);

#endif /* GRASS_SEGMENTDEFS_H */
