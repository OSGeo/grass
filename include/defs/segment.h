#ifndef GRASS_SEGMENTDEFS_H
#define GRASS_SEGMENTDEFS_H

int segment_open(SEGMENT *, char *, off_t, off_t, int, int, int, int);
int segment_close(SEGMENT *);
int segment_flush(SEGMENT *);
int segment_format(int, off_t, off_t, int, int, int);
int segment_format_nofill(int, off_t, off_t, int, int, int);
int segment_get(SEGMENT *, void *, off_t, off_t);
int segment_get_row(const SEGMENT *, void *, off_t);
int segment_init(SEGMENT *, int, int);
int segment_put(SEGMENT *, const void *, off_t, off_t);
int segment_put_row(const SEGMENT *, const void *, off_t);
int segment_release(SEGMENT *);

#endif /* GRASS_SEGMENTDEFS_H */
