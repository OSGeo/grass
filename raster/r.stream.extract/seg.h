#ifndef __SEG_H__
#define __SEG_H__

#include <grass/raster.h>
#include <grass/segment.h>

#define CSEG struct _c_s_e_g_
CSEG {
    SEGMENT seg;		/* segment structure */
    int fd;			/* fd for reading/writing segment file */
    char *filename;		/* name of segment file */
    char *name;			/* raster map read into segment file */
    char *mapset;
};

#define DSEG struct _d_s_e_g_
DSEG {
    SEGMENT seg;		/* segment structure */
    int fd;			/* fd for reading/writing segment file */
    char *filename;		/* name of segment file */
    char *name;			/* raster map read into segment file */
    char *mapset;
};

#define BSEG struct _b_s_e_g_
BSEG {
    SEGMENT seg;		/* segment structure */
    int fd;			/* fd for reading/writing segment file */
    char *filename;		/* name of segment file */
    char *name;			/* raster map read into segment file */
    char *mapset;
};

#define SSEG struct _s_s_e_g_
SSEG {
    SEGMENT seg;		/* segment structure */
    int fd;			/* fd for reading/writing segment file */
    char *filename;		/* name of segment file */
};

/* bseg.c */
int bseg_close(BSEG *);
int bseg_get(BSEG *, char *, int, int);
int bseg_open(BSEG *, int, int, int);
int bseg_put(BSEG *, char *, int, int);
int bseg_put_row(BSEG *, char *, int);
int bseg_read_raster(BSEG *, char *, char *);
int bseg_write_raster(BSEG *, char *);

/* cseg.c */
int cseg_close(CSEG *);
int cseg_get(CSEG *, CELL *, int, int);
int cseg_open(CSEG *, int, int, int);
int cseg_put(CSEG *, CELL *, int, int);
int cseg_put_row(CSEG *, CELL *, int);
int cseg_read_raster(CSEG *, char *, char *);
int cseg_write_raster(CSEG *, char *);

/* dseg.c */
int dseg_close(DSEG *);
int dseg_get(DSEG *, double *, int, int);
int dseg_open(DSEG *, int, int, int);
int dseg_put(DSEG *, double *, int, int);
int dseg_put_row(DSEG *, double *, int);
int dseg_read_raster(DSEG *, char *, char *);
int dseg_write_raster(DSEG *, char *);

/* seg.c */
int seg_close(SSEG *);
int seg_get(SSEG *, char *, int, int);
int seg_open(SSEG *, int, int, int, int, int, int, int);
int seg_put(SSEG *, char *, int, int);
int seg_put_row(SSEG *, char *, int);
int seg_get(SSEG *, char *, int, int);
int seg_get_row(SSEG *, char *, int);
int seg_flush(SSEG *);

#endif /* __SEG_H__ */
