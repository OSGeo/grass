/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *               Tomas Zigo <tomas zigo slovanet sk> (adding the option
 *               to read width, depth values from vector map table columns)
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#ifndef __ENFORCE_H__
#define __ENFORCE_H__

#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/bitmap.h>
#include <grass/vector.h>


#define APP_VERSION 1.0
#define MAX_PTS     10000

/* 2x2 determinat */
#define DET2_2(a,b,c,d) ((a*d) - (b*c))

#define LINTERP(a,b,r)  ((a)+(r) * ((b)-(a)))
#define SQR(x) (x * x)


typedef double Point2[2];

typedef struct
{
    Point2 pnts[MAX_PTS];
    int npts;
    double sum_x, sum_y, sum_xy, sum_x_sq, slope, yinter;
} PointGrp;


struct parms
{
    struct Option *inrast, *invect, *outrast, *outvect, *width_col,
      *depth_col, *field;
    RASTER_MAP_TYPE raster_type;
    double swidth, sdepth;
    int wrap, noflat;
};

struct sql_statement
{
    dbString *sql;
    int ncats;
    struct vect_id_cat_map *id_cat_map;
};

struct vect_id_cat_map
{
    int id;
    int cat;
};

struct ptr
{
    enum Type {
                 P_INT,
                 P_DOUBLE,
                 P_CHAR,
                 P_DBSTRING,
                 P_VECT_ID_CAT_MAP,
    } type;
    union {
        int *p_int;
        double *p_double;
        char *p_char;
        dbString *p_dbString;
        struct vect_id_cat_map *p_vect_id_cat_map;
    };
};

typedef enum
{
   WIDTH,
   DEPTH,
} value_type;

/* enforce_ds.c */
extern void enforce_downstream(int /*infd */ , int /*outfd */ ,
                               struct Map_info * /*Map */ ,
                               struct Map_info * /*outMap */ ,
                               struct parms * /* parm */,
                               struct field_info * /* Fi */,
                               int * /* width_col_posw */,
                               int * /* depth_col_pos */,
                               char *[2] /* columns[2] */,
                               dbDriver * /* driver */);
extern void adjust_swidth(struct Cell_head *win, double *value);
extern void adjust_sdepth(double *value);

/* lobf.c */
extern Point2 *pg_getpoints(PointGrp *);
extern Point2 *pg_getpoints_reversed(PointGrp *);
extern double pg_y_from_x(PointGrp *, const double);
extern void pg_init(PointGrp *);
extern void pg_addpt(PointGrp *, Point2);

/* raster.c */
void *read_raster(void *, const int, const RASTER_MAP_TYPE);
void *write_raster(void *, const int, const RASTER_MAP_TYPE);

/* support.c */
extern void update_rast_history(struct parms *);
extern void check_mem_alloc(struct ptr *pointer);

/* vect.c */
extern int open_new_vect(struct Map_info *, char *);
extern int close_vect(struct Map_info *, const int);


#endif /* __ENFORCE_H__ */
