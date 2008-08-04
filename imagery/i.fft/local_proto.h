#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/gis.h>

/* fft_colors.c */
int fft_colors(void);

/* orig_wind.c */
int put_orig_window(struct Cell_head *);

/* save_fft.c */
int save_fft(int, double *[2], double *, double *);

#endif /* __LOCAL_PROTO_H__ */
