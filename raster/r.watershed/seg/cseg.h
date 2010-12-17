#ifndef __CSEG_H__
#define __CSEG_H__

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

/* bseg_close.c */
int bseg_close(BSEG *);

/* bseg_get.c */
int bseg_get(BSEG *, char *, int, int);

/* bseg_open.c */
int bseg_open(BSEG *, int, int, int);

/* bseg_put.c */
int bseg_put(BSEG *, char *, int, int);
int bseg_put_row(BSEG *, char *, int);

/* bseg_read.c */
int bseg_read_cell(BSEG *, char *, char *);

/* bseg_write.c */
int bseg_write_cellfile(BSEG *, char *);

/* cseg_close.c */
int cseg_close(CSEG *);

/* cseg_get.c */
int cseg_get(CSEG *, CELL *, int, int);

/* cseg_open.c */
int cseg_open(CSEG *, int, int, int);

/* cseg_put.c */
int cseg_put(CSEG *, CELL *, int, int);
int cseg_put_row(CSEG *, CELL *, int);

/* cseg_read.c */
int cseg_read_cell(CSEG *, char *, char *);

/* cseg_write.c */
int cseg_write_cellfile(CSEG *, char *);

/* dseg_close.c */
int dseg_close(DSEG *);

/* dseg_get.c */
int dseg_get(DSEG *, double *, int, int);

/* dseg_open.c */
int dseg_open(DSEG *, int, int, int);

/* dseg_put.c */
int dseg_put(DSEG *, double *, int, int);
int dseg_put_row(DSEG *, double *, int);

/* dseg_read.c */
int dseg_read_cell(DSEG *, char *, char *);

/* dseg_write.c */
int dseg_write_cellfile(DSEG *, char *);

/* seg_close.c */
int seg_close(SSEG *);

/* seg_get.c */
int seg_get(SSEG *, char *, int, int);

/* seg_open.c */
int seg_open(SSEG *, int, int, int, int, int, int);

/* seg_put.c */
int seg_put(SSEG *, char *, int, int);
int seg_put_row(SSEG *, char *, int);

/* sseg_close.c */
int seg_close(SSEG *);

/* sseg_get.c */
int seg_get(SSEG *, char *, int, int);
int seg_get_row(SSEG *, char *, int);
int seg_flush(SSEG *);

/* sseg_open.c */
int seg_open(SSEG *, int, int, int, int, int, int);

/* sseg_put.c */
int seg_put(SSEG *, char *, int, int);


#endif /* __CSEG_H__ */
