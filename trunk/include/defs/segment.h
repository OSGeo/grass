#ifndef GRASS_SEGMENTDEFS_H
#define GRASS_SEGMENTDEFS_H

int Segment_open(SEGMENT *, char *, off_t, off_t, int, int, int, int);
int Segment_close(SEGMENT *);
int Segment_flush(SEGMENT *);
int Segment_format(int, off_t, off_t, int, int, int);
int Segment_format_nofill(int, off_t, off_t, int, int, int);
int Segment_get(SEGMENT *, void *, off_t, off_t);
int Segment_get_row(const SEGMENT *, void *, off_t);
int Segment_init(SEGMENT *, int, int);
int Segment_put(SEGMENT *, const void *, off_t, off_t);
int Segment_put_row(const SEGMENT *, const void *, off_t);
int Segment_release(SEGMENT *);

#endif /* GRASS_SEGMENTDEFS_H */
