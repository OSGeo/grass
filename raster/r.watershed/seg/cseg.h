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
int bseg_get(BSEG *, char *, GW_LARGE_INT, GW_LARGE_INT);

/* bseg_open.c */
int bseg_open(BSEG *, int, int, int);

/* bseg_put.c */
int bseg_put(BSEG *, char *, GW_LARGE_INT, GW_LARGE_INT);
int bseg_put_row(BSEG *, char *, GW_LARGE_INT);

/* bseg_read.c */
int bseg_read_cell(BSEG *, char *, char *);

/* bseg_write.c */
int bseg_write_cellfile(BSEG *, char *);

/* cseg_close.c */
int cseg_close(CSEG *);

/* cseg_get.c */
int cseg_get(CSEG *, CELL *, GW_LARGE_INT, GW_LARGE_INT);

/* cseg_open.c */
int cseg_open(CSEG *, int, int, int);

/* cseg_put.c */
int cseg_put(CSEG *, CELL *, GW_LARGE_INT, GW_LARGE_INT);
int cseg_put_row(CSEG *, CELL *, GW_LARGE_INT);

/* cseg_read.c */
int cseg_read_cell(CSEG *, char *, char *);

/* cseg_write.c */
int cseg_write_cellfile(CSEG *, char *);

/* dseg_close.c */
int dseg_close(DSEG *);

/* dseg_get.c */
int dseg_get(DSEG *, double *, GW_LARGE_INT, GW_LARGE_INT);
int dseg_flush(DSEG *);

/* dseg_open.c */
int dseg_open(DSEG *, int, int, int);

/* dseg_put.c */
int dseg_put(DSEG *, double *, GW_LARGE_INT, GW_LARGE_INT);
int dseg_put_row(DSEG *, double *, GW_LARGE_INT);

/* dseg_read.c */
int dseg_read_cell(DSEG *, char *, char *);

/* dseg_write.c */
int dseg_write_cellfile(DSEG *, char *);

/* sseg_close.c */
int seg_close(SSEG *);

/* sseg_get.c */
int seg_get(SSEG *, char *, GW_LARGE_INT, GW_LARGE_INT);
int seg_get_row(SSEG *, char *, GW_LARGE_INT);
int seg_flush(SSEG *);

/* sseg_open.c */
int seg_open(SSEG *, GW_LARGE_INT, GW_LARGE_INT, int, int, int, int);

/* sseg_put.c */
int seg_put(SSEG *, char *, GW_LARGE_INT, GW_LARGE_INT);
int seg_put_row(SSEG * sseg, char *value, GW_LARGE_INT);


#endif /* __CSEG_H__ */
