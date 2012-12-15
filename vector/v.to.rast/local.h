#ifndef __LOCAL_H__
#define __LOCAL_H__

#include <grass/raster.h>
#include <grass/dbmi.h>
#include <grass/vector.h>


/* 2 * PI */
#define M_2PI 6.283185307179586232

#define ISNULL(x)   Rast_is_c_null_value (x)
#define ISDNULL(x)  Rast_is_d_null_value (x)
#define SETNULL(x)  Rast_set_c_null_value (x, 1)
#define SETDNULL(x) Rast_set_d_null_value (x, 1)

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
int do_areas(struct Map_info *, struct line_pnts *, dbCatValArray *, int,
	     int, double, int);
int sort_areas(struct Map_info *, struct line_pnts *, int, struct cat_list *);

/* do_lines.c */
int do_lines(struct Map_info *, struct line_pnts *, dbCatValArray *, int, int,
	     struct cat_list *, int, double, int, int, int *);

/* raster.c */
int begin_rasterization(int, int);
int output_raster(int);
int set_cat(CELL);
int set_dcat(DCELL);

/* support.c */
int update_hist(const char *, const char *, long);
int update_colors(const char *);
int update_dbcolors(const char *, const char *, int, const char *, int, const char *);
int update_labels(const char *, const char *, int, const char *, int, int, const char *);
int update_cats(const char *);
int update_fcolors(const char *);

/* vect2rast.c */
int vect_to_rast(const char *, const char *, const char *, const char *, int, int,
		 double, int, const char *, const char *, int, char *, char *);

#endif
