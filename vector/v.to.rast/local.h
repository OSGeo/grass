#ifndef __LOCAL_H__
#define __LOCAL_H__

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>


#undef DEBUG

/* 2 * PI */
#define M_2PI 6.283185307179586232

#define ISNULL(x)   G_is_c_null_value (x)
#define ISDNULL(x)  G_is_d_null_value (x)
#define SETNULL(x)  G_set_c_null_value (x, 1)
#define SETDNULL(x) G_set_d_null_value (x, 1)

/* cell type */
#define USE_CELL  1
#define USE_DCELL 2

/* value type */
#define USE_ATTR  1
#define USE_CAT   2
#define USE_VAL   3
#define USE_Z     4
#define USE_D     5


/* do_areas.c */
int do_areas(struct Map_info *, struct line_pnts *, dbCatValArray *, int, int,
	     int, double, int);
int sort_areas(struct Map_info *, struct line_pnts *, int);

/* do_lines.c */
int do_lines(struct Map_info *, struct line_pnts *, dbCatValArray *, int, int,
	     int, double, int, int, int *);

/* raster.c */
int begin_rasterization(int, int);
int output_raster(int);
int set_cat(CELL);
int set_dcat(DCELL);

/* support.c */
int update_hist(char *, char *, char *, long);
int update_colors(char *);
int update_dbcolors(char *, char *, int, char *, int, char *);
int update_labels(char *, char *, int, char *, int, int, char *);
int update_cats(char *);
int update_fcolors(char *raster_name);

/* vect2rast.c */
int vect_to_rast(char *, char *, int, char *, int, int, double, int, char *,
		 char *, int);

#endif
