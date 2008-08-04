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

/* bseg_close.c */
int bseg_close(BSEG *);

/* bseg_get.c */
int bseg_get(BSEG *, CELL *, int, int);

/* bseg_open.c */
int bseg_open(BSEG *, int, int, int);

/* bseg_put.c */
int bseg_put(BSEG *, CELL *, int, int);

/* bseg_read.c */
int bseg_read_cell(BSEG *, char *, char *);

/* bseg_write.c */
int bseg_write_cellfile(BSEG *, char *);

/* cseg_close.c */
int cseg_close(CSEG *);

/* cseg_get.c */
int cseg_get(CSEG *, int, int, CELL *);

/* cseg_open.c */
int cseg_open(CSEG *, int, int, int);

/* cseg_put.c */
int cseg_put(CSEG *, int, int, CELL);

/* cseg_read.c */
int cseg_read_cell(CSEG *, char *, char *);

/* cseg_write.c */
int cseg_write_cellfile(CSEG *, char *);
