/*!
   \file Gs3.c

   \brief OGSF library - loading surfaces (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois (January 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/bitmap.h>

#include <grass/ogsf.h>
/* for geoview & geodisplay in 3dview stuff */
#include "gsget.h"
/* for update_attrange - might be able to move this func now */

/*!
   \brief Used in the function Gs_update_attrange()
 */
#define INIT_MINMAX(p, nm, size, min, max, found) \
	found = 0; \
	p+=(size-1); \
	while (size--) \
	{ \
	    if (!BM_GET_BYOFFSET(nm, size)) \
	    { \
		min = max = *p; \
		found = 1; \
		break; \
	    } \
	    p--; \
	}

/*!
   \brief Used in the function Gs_update_attrange()
 */
#define SET_MINMAX(p, nm, size, min, max) \
	p+=(size-1); \
	while(size--) \
	{ \
	    if (!BM_GET_BYOFFSET(nm, size)) \
	    { \
		if (*p < min) \
		{ \
		    min = *p; \
		} \
		else if (*p > max) \
		{ \
		    max = *p; \
		}  \
	    } \
	    p--; \
	}

typedef int FILEDESC;

#define NO_DATA_COL 0xffffff

/*!
   \brief Calculates distance in METERS between two points in current projection (2D)

   Uses G_distance().

   \param from 'from' point (X, Y)
   \param to 'to' point (X, Y)

   \return distance
 */
double Gs_distance(double *from, double *to)
{
    static int first = 1;

    if (first) {
	first = 0;
	G_begin_distance_calculations();
    }

    return G_distance(from[0], from[1], to[0], to[1]);
}

/*!
   \brief Load raster map as floating point map

   Calling function must have already allocated space in buff for
   wind->rows * wind->cols floats.

   This routine simply loads the map into a 2d array by repetitve calls
   to get_f_raster_row.

   \param wind current window
   \param map_name raster map name
   \param[out] buff data buffer
   \param[out] nullmap null map buffer
   \param[out] has_null indicates if raster map contains null-data

   \return 1 on success
   \return 0 on failure
 */
int Gs_loadmap_as_float(struct Cell_head *wind, const char *map_name,
			float *buff, struct BM *nullmap, int *has_null)
{
    FILEDESC cellfile;
    const char *map_set;
    int offset, row, col;

    G_debug(3, "Gs_loadmap_as_float(): name=%s", map_name);

    map_set = G_find_raster2(map_name, "");
    if (!map_set) {
	G_warning(_("Raster map <%s> not found"), map_name);
	return 0;
    }
    *has_null = 0;

    cellfile = Rast_open_old(map_name, map_set);

    G_message(_("Loading raster map <%s>..."),
	      G_fully_qualified_name(map_name, map_set));

    for (row = 0; row < wind->rows; row++) {
	offset = row * wind->cols;
	Rast_get_f_row(cellfile, &(buff[offset]), row);

	G_percent(row, wind->rows, 2);

	for (col = 0; col < wind->cols; col++) {
	    if (Rast_is_f_null_value(buff + offset + col)) {
		*has_null = 1;
		BM_set(nullmap, col, row, 1);
	    }
	    /* set nm */
	}
    }
    G_percent(1, 1, 1);

    G_debug(4, "  has_null=%d", *has_null);

    Rast_close(cellfile);

    return (1);
}

/*!
   \brief Load raster map as integer map

   Calling function must have already allocated space in buff for
   wind->rows * wind->cols floats.

   This routine simply loads the map into a 2d array by repetitve calls
   to get_f_raster_row.

   \todo fn body of Gs_loadmap_as_float()

   \param wind current window
   \param map_name raster map name
   \param[out] buff data buffer
   \param[out] nullmap null map buffer
   \param[out] has_null indicates if raster map contains null-data

   \return 1 on success
   \return 0 on failure
 */
int Gs_loadmap_as_int(struct Cell_head *wind, const char *map_name, int *buff,
		      struct BM *nullmap, int *has_null)
{
    FILEDESC cellfile;
    const char *map_set;
    int offset, row, col;

    G_debug(3, "Gs_loadmap_as_int");

    map_set = G_find_raster2(map_name, "");
    if (!map_set) {
	G_warning(_("Raster map <%s> not found"), map_name);
	return 0;
    }
    *has_null = 0;

    cellfile = Rast_open_old(map_name, map_set);

    G_message(_("Loading raster map <%s>..."),
	      G_fully_qualified_name(map_name, map_set));

    for (row = 0; row < wind->rows; row++) {
	offset = row * wind->cols;
	Rast_get_c_row(cellfile, &(buff[offset]), row);

	G_percent(row, wind->rows, 2);

	for (col = 0; col < wind->cols; col++) {
	    if (Rast_is_f_null_value(buff + offset + col)) {
		*has_null = 1;
		BM_set(nullmap, col, row, 1);
	    }

	    /* set nm */
	}
    }
    G_percent(1, 1, 1);
    
    Rast_close(cellfile);

    return (1);
}

/*!
   \brief Get map data type

   \param filename raster map name
   \param negflag

   \return -1 if map is integer and Rast_read_range() fails
   \return data type (ARRY_*)
 */
int Gs_numtype(const char *filename, int *negflag)
{
    CELL max = 0, min = 0;
    struct Range range;
    const char *mapset;
    int shortbits, charbits, bitplace;
    static int max_short, max_char;
    static int first = 1;

    if (first) {
	max_short = max_char = 1;
	shortbits = 8 * sizeof(short);

	for (bitplace = 1; bitplace < shortbits; ++bitplace) {
	    /*1 bit for sign */
	    max_short *= 2;
	}

	max_short -= 1;

	/* NO bits for sign, using unsigned char */
	charbits = 8 * sizeof(unsigned char);

	for (bitplace = 0; bitplace < charbits; ++bitplace) {
	    max_char *= 2;
	}

	max_char -= 1;

	first = 0;
    }

    mapset = G_find_raster2(filename, "");
    if (!mapset) {
	G_warning(_("Raster map <%s> not found"), filename);
	return -1;
    }

    if (Rast_map_is_fp(filename, mapset)) {
	G_debug(3, "Gs_numtype(): fp map detected");

	return (ATTY_FLOAT);
    }

    if (-1 == Rast_read_range(filename, mapset, &range)) {
	return (-1);
    }

    Rast_get_range_min_max(&range, &min, &max);
    *negflag = (min < 0);

    if (max < max_char && min > 0) {
	return (ATTY_CHAR);
    }

    if (max < max_short && min > -max_short) {
	return (ATTY_SHORT);
    }

    return (ATTY_INT);
}

/*!
   \brief Load raster map as integer map

   Calling function must have already allocated space in buff for
   wind->rows * wind->cols shorts.  

   This routine simply loads the map into a 2d array by repetitve calls
   to get_map_row.

   \param wind current window
   \param map_name raster map name
   \param[out] buff data buffer
   \param[out] nullmap null map buffer
   \param[out] has_null indicates if raster map contains null-data

   \return 1 on success
   \return -1 on failure,
   \return -2 if read ok, but 1 or more values were too large (small)
   to fit into a short (in which case the max (min) short is used)
 */
int Gs_loadmap_as_short(struct Cell_head *wind, const char *map_name,
			short *buff, struct BM *nullmap, int *has_null)
{
    FILEDESC cellfile;
    const char *map_set;
    int *ti, *tmp_buf;
    int offset, row, col, val, max_short, overflow, shortsize, bitplace;
    short *ts;

    G_debug(3, "Gs_loadmap_as_short");

    overflow = 0;
    shortsize = 8 * sizeof(short);

    /* 1 bit for sign */
    /* same as 2 << (shortsize-1) */
    for (max_short = bitplace = 1; bitplace < shortsize; ++bitplace) {
	max_short *= 2;
    }

    max_short -= 1;

    map_set = G_find_raster2(map_name, "");
    if (!map_set) {
	G_warning(_("Raster map <%s> not found"), map_name);
	return -1;
    }
    *has_null = 0;

    cellfile = Rast_open_old(map_name, map_set);

    tmp_buf = (int *)G_malloc(wind->cols * sizeof(int));	/* G_fatal_error */
    if (!tmp_buf) {
	return -1;
    }

    G_message(_("Loading raster map <%s>..."),
	      G_fully_qualified_name(map_name, map_set));

    for (row = 0; row < wind->rows; row++) {
	offset = row * wind->cols;
	Rast_get_c_row(cellfile, tmp_buf, row);

	G_percent(row, wind->rows, 2);

	ts = &(buff[offset]);
	ti = tmp_buf;

	for (col = 0; col < wind->cols; col++) {
	    if (Rast_is_c_null_value(&tmp_buf[col])) {
		*has_null = 1;
		BM_set(nullmap, col, row, 1);
	    }
	    else {
		val = *ti;
		if (abs(val) > max_short) {
		    overflow = 1;
		    /* assign floor/ceiling value?
		     */
		    *ts = (short)(max_short * val / abs(val));
		}
		else {
		    *ts = (short)val;
		}
	    }

	    ti++;
	    ts++;
	}
    }
    G_percent(1, 1, 1);
    
    Rast_close(cellfile);

    G_free(tmp_buf);

    return (overflow ? -2 : 1);
}

/*!
   \brief Load raster map as integer map

   Calling function must have already allocated space in buff for
   wind->rows * wind->cols unsigned chars.  

   This routine simply loads the map into a 2d array by repetitve calls
   to get_map_row.

   Since signs of chars can be tricky, we only load positive chars
   between 0-255.

   \todo fn body Gs_loadmap_as_float()

   \param wind current window
   \param map_name raster map name
   \param[out] buff data buffer
   \param[out] nullmap null map buffer
   \param[out] has_null indicates if raster map contains null-data

   \return 1 on success
   \return -1 on failure
   \return -2 if read ok, but 1 or more values
   were too large (small) to fit into an unsigned char.
   (in which case the max (min) char is used)
 */
int Gs_loadmap_as_char(struct Cell_head *wind, const char *map_name,
		       unsigned char *buff, struct BM *nullmap, int *has_null)
{
    FILEDESC cellfile;
    const char *map_set;
    int *ti, *tmp_buf;
    int offset, row, col, val, max_char, overflow, charsize, bitplace;
    unsigned char *tc;

    G_debug(3, "Gs_loadmap_as_char");

    overflow = 0;
    charsize = 8 * sizeof(unsigned char);

    /* 0 bits for sign! */
    max_char = 1;

    for (bitplace = 0; bitplace < charsize; ++bitplace) {
	max_char *= 2;
    }

    max_char -= 1;

    map_set = G_find_raster2(map_name, "");
    if (!map_set) {
	G_warning(_("Raster map <%s> not found"), map_name);
	return -1;
    }
    *has_null = 0;

    cellfile = Rast_open_old(map_name, map_set);

    tmp_buf = (int *)G_malloc(wind->cols * sizeof(int));	/* G_fatal_error */
    if (!tmp_buf) {
	return -1;
    }

    G_message(_("Loading raster map <%s>..."),
	      G_fully_qualified_name(map_name, map_set));

    for (row = 0; row < wind->rows; row++) {
	offset = row * wind->cols;
	Rast_get_c_row(cellfile, tmp_buf, row);
	tc = (unsigned char *)&(buff[offset]);
	ti = tmp_buf;

	G_percent(row, wind->rows, 2);

	for (col = 0; col < wind->cols; col++) {
	    if (Rast_is_c_null_value(&tmp_buf[col])) {
		*has_null = 1;
		BM_set(nullmap, col, row, 1);
	    }
	    else {
		val = *ti;
		if (val > max_char) {
		    overflow = 1;
		    *tc = (unsigned char)max_char;
		}
		else if (val < 0) {
		    overflow = 1;
		    *tc = 0;
		}
		else {
		    *tc = (unsigned char)val;
		}
	    }

	    ti++;
	    tc++;
	}
    }
    G_percent(1, 1, 1);
    
    Rast_close(cellfile);

    G_free(tmp_buf);

    return (overflow ? -2 : 1);
}

/*!
   \brief Load raster map as integer map

   Calling function must have already allocated space in buff for
   struct BM of wind->rows & wind->cols.

   This routine simply loads the map into the bitmap by repetitve calls
   to get_map_row.  Any value other than 0 in the map will set the bitmap.
   (may want to change later to allow specific value to set)

   Changed to use null.

   \param wind current window
   \param map_name raster map name
   \param[out] buff data buffer

   \returns 1 on success
   \return -1 on failure
 */
int Gs_loadmap_as_bitmap(struct Cell_head *wind, const char *map_name,
			 struct BM *buff)
{
    FILEDESC cellfile;
    const char *map_set;
    int *tmp_buf;
    int row, col;

    G_debug(3, "Gs_loadmap_as_bitmap");

    map_set = G_find_raster2(map_name, "");
    if (!map_set) {
	G_warning(_("Raster map <%s> not found"), map_name);
	return -1;
    }

    cellfile = Rast_open_old(map_name, map_set);

    tmp_buf = (int *)G_malloc(wind->cols * sizeof(int));	/* G_fatal_error */
    if (!tmp_buf) {
	return -1;
    }

    G_message(_("Loading raster map <%s>..."),
	      G_fully_qualified_name(map_name, map_set));

    for (row = 0; row < wind->rows; row++) {
	Rast_get_c_row(cellfile, tmp_buf, row);

	for (col = 0; col < wind->cols; col++) {
	    if (Rast_is_c_null_value(&tmp_buf[col])) {
		/* no data */
		BM_set(buff, col, row, 1);
	    }
	    else {
		BM_set(buff, col, row, 0);
	    }
	}
    }

    Rast_close(cellfile);

    G_free(tmp_buf);

    return (1);
}

/*!
   \brief Build color table (256)

   Calling function must have already allocated space in buff for range of
   data (256 for now) - simply calls get_color for each cat in color range

   \param filename raster map name
   \param[out] buff data buffer

   \return 1 on success
   \return 0 on failure
 */
int Gs_build_256lookup(const char *filename, int *buff)
{
    const char *mapset;
    struct Colors colrules;
    CELL min, max, cats[256];
    int i;
    unsigned char r[256], g[256], b[256], set[256];

    G_debug(3, "building color table");

    mapset = G_find_raster2(filename, "");
    if (!mapset) {
	G_warning(_("Raster map <%s> not found"), filename);
	return 0;
    }

    Rast_read_colors(filename, mapset, &colrules);
    Rast_get_c_color_range(&min, &max, &colrules);

    if (min < 0 || max > 255) {
	G_warning(_("Color table range doesn't match data (mincol=%d, maxcol=%d"),
		  min, max);

	min = min < 0 ? 0 : min;
	max = max > 255 ? 255 : max;
    }

    G_zero(cats, 256 * sizeof(CELL));

    for (i = min; i <= max; i++) {
	cats[i] = i;
    }

    Rast_lookup_c_colors(cats, r, g, b, set, 256, &colrules);

    for (i = 0; i < 256; i++) {

	if (set[i]) {
	    buff[i] =
		(r[i] & 0xff) | ((g[i] & 0xff) << 8) | ((b[i] & 0xff) << 16);
	}
	else {
	    buff[i] = NO_DATA_COL;
	}
    }

    return (1);
}

/*!
   \brief Pack color table

   Passed an array of 32 bit ints that is converted from cell values
   to packed colors (0xbbggrr) 

   \param filename raster map name
   \param buff
   \param rows number of rows
   \param cols number of cols
 */
void Gs_pack_colors(const char *filename, int *buff, int rows, int cols)
{
    const char *mapset;
    struct Colors colrules;
    unsigned char *r, *g, *b, *set;
    int *cur, i, j;

    mapset = G_find_raster2(filename, "");
    if (!mapset) {
	G_warning(_("Raster map <%s> not found"), filename);
	return;
    }

    r = (unsigned char *)G_malloc(cols);
    g = (unsigned char *)G_malloc(cols);
    b = (unsigned char *)G_malloc(cols);
    set = (unsigned char *)G_malloc(cols);

    Rast_read_colors(filename, mapset, &colrules);

    cur = buff;

    G_message(_("Translating colors from raster map <%s>..."),
	      G_fully_qualified_name(filename, mapset));

    for (i = 0; i < rows; i++) {
	Rast_lookup_c_colors(cur, r, g, b, set, cols, &colrules);
	G_percent(i, rows, 2);

	for (j = 0; j < cols; j++) {
	    if (set[j]) {
		cur[j] =
		    (r[j] & 0xff) | ((g[j] & 0xff) << 8) | ((b[j] & 0xff) <<
							    16);
	    }
	    else {
		cur[j] = NO_DATA_COL;
	    }
	}

	cur = &(cur[cols]);
    }
    G_percent(1, 1, 1);
    
    Rast_free_colors(&colrules);

    G_free(r);
    G_free(g);
    G_free(b);

    G_free(set);

    return;
}

/*!
   \brief Pack color table (floating-point map)

   Passed a array of floats that will be converted from cell values
   to packed colors (0xbbggrr) and float to int 
   Floating point data not freed here, use: 
   gsds_free_data_buff(id, ATTY_FLOAT)

   \param filename raster map name
   \param fbuf
   \param ibuf
   \param rows number of rows
   \param cols number of cols
 */
void Gs_pack_colors_float(const char *filename, float *fbuf, int *ibuf,
			  int rows, int cols)
{
    const char *mapset;
    struct Colors colrules;
    unsigned char *r, *g, *b, *set;
    int i, j, *icur;
    FCELL *fcur;

    mapset = G_find_raster2(filename, "");
    if (!mapset) {
	G_warning(_("Raster map <%s> not found"), filename);
	return;
    }

    r = (unsigned char *)G_malloc(cols);
    g = (unsigned char *)G_malloc(cols);
    b = (unsigned char *)G_malloc(cols);
    set = (unsigned char *)G_malloc(cols);

    Rast_read_colors(filename, mapset, &colrules);

    fcur = fbuf;
    icur = ibuf;

    G_message(_("Translating colors from raster map <%s>..."),
	      G_fully_qualified_name(filename, mapset));
    
    for (i = 0; i < rows; i++) {
	Rast_lookup_f_colors(fcur, r, g, b, set, cols, &colrules);
	G_percent(i, rows, 2);

	for (j = 0; j < cols; j++) {
	    if (set[j]) {
		icur[j] =
		    (r[j] & 0xff) | ((g[j] & 0xff) << 8) | ((b[j] & 0xff) <<
							    16);
	    }
	    else {
		icur[j] = NO_DATA_COL;
	    }
	}

	icur = &(icur[cols]);
	fcur = &(fcur[cols]);
    }
    G_percent(1, 1, 1);
    
    Rast_free_colors(&colrules);

    G_free(r);
    G_free(g);
    G_free(b);
    G_free(set);

    return;
}

/*!
   \brief Get categories/labels

   Formats label as in d.what.rast -> (catval) catlabel 

   \param filename raster map name
   \param drow
   \param dcol
   \param catstr category string

   \return 1 on success
   \return 0 on failure
 */
int Gs_get_cat_label(const char *filename, int drow, int dcol, char *catstr)
{
    struct Categories cats;
    const char *mapset;
    CELL *buf;
    DCELL *dbuf;
    RASTER_MAP_TYPE map_type;
    int fd = -1;

    if ((mapset = G_find_raster2(filename, "")) == NULL) {
	G_warning(_("Raster map <%s> not found"), filename);
	return 0;
    }

    if (-1 != Rast_read_cats(filename, mapset, &cats)) {
	fd = Rast_open_old(filename, mapset);
	map_type = Rast_get_map_type(fd);

	if (map_type == CELL_TYPE) {
	    buf = Rast_allocate_c_buf();

	    Rast_get_c_row(fd, buf, drow);
	    if (Rast_is_c_null_value(&buf[dcol])) {
		sprintf(catstr, "(NULL) %s",
			Rast_get_c_cat(&buf[dcol], &cats));
	    }
	    else {
		sprintf(catstr, "(%d) %s", buf[dcol],
			Rast_get_c_cat(&buf[dcol], &cats));
	    }

	    G_free(buf);
	}

	else {
	    /* fp map */
	    dbuf = Rast_allocate_d_buf();

	    Rast_get_d_row(fd, dbuf, drow);
	    if (Rast_is_d_null_value(&dbuf[dcol])) {
		sprintf(catstr, "(NULL) %s",
			Rast_get_d_cat(&dbuf[dcol], &cats));
	    }
	    else {
		sprintf(catstr, "(%g) %s", dbuf[dcol],
			Rast_get_d_cat(&dbuf[dcol], &cats));
	    }

	    G_free(dbuf);
	}
    }
    else {
	strcpy(catstr, "no category label");
	return 0;
    }

    /* TODO: may want to keep these around for multiple queries */
    Rast_free_cats(&cats);

    if (fd >= 0)
	Rast_close(fd);

    return (1);
}

/*!
   \brief Save 3dview

   \param vname view name
   \param gv pointer to geoview struct
   \param gd pointer to geodisplay struct
   \param w current window
   \param defsurf default geosurf struct

   \return -1 on error
   \return ?
 */
int Gs_save_3dview(const char *vname, geoview * gv, geodisplay * gd,
		   struct Cell_head *w, geosurf * defsurf)
{
    const char *mapset;
    struct G_3dview v;
    float zmax, zmin;

    GS_get_zrange(&zmin, &zmax, 0);

    G_get_3dview_defaults(&v, w);
    mapset = G_mapset();

    if (mapset != NULL) {
	if (defsurf) {
	    if (defsurf->draw_mode & DM_WIRE_POLY) {
		v.display_type = 3;
	    }
	    else if (defsurf->draw_mode & DM_WIRE ||
		     defsurf->draw_mode & DM_COL_WIRE) {
		v.display_type = 1;
	    }
	    else if (defsurf->draw_mode & DM_POLY) {
		v.display_type = 2;
	    }

	    v.mesh_freq = defsurf->x_modw;	/* mesh resolution */
	    v.poly_freq = defsurf->x_mod;	/* poly resolution */
	    v.dozero = !(defsurf->nz_topo);
	    v.colorgrid = (defsurf->draw_mode & DM_COL_WIRE) ? 1 : 0;
	    v.shading = (defsurf->draw_mode & DM_GOURAUD) ? 1 : 0;
	}

	if (gv->infocus) {
	    GS_v3eq(v.from_to[TO], gv->real_to);
	    v.from_to[TO][Z] -= zmin;
	    GS_v3mult(v.from_to[TO], gv->scale);
	    v.from_to[TO][Z] *= gv->vert_exag;
	}
	else {
	    GS_v3eq(v.from_to[TO], gv->from_to[TO]);
	}

	gsd_model2real(v.from_to[TO]);

	GS_v3eq(v.from_to[FROM], gv->from_to[FROM]);
	gsd_model2real(v.from_to[FROM]);

	v.exag = gv->vert_exag;
	v.fov = gv->fov / 10.;
	v.twist = gv->twist;
	v.fringe = 0;		/* not implemented here */

	v.lightson = 1;		/* always true, curently */

	if (gv->lights[0].position[W] == 1) {
	    /* local */
	    v.lightpos[X] = gv->lights[0].position[X];
	    v.lightpos[Y] = gv->lights[0].position[Y];
	    v.lightpos[Z] = gv->lights[0].position[Z];
	    gsd_model2real(v.lightpos);
	    v.lightpos[W] = 1.0;	/* local */
	}
	else {
	    v.lightpos[X] = gv->lights[0].position[X];
	    v.lightpos[Y] = gv->lights[0].position[Y];
	    v.lightpos[Z] = gv->lights[0].position[Z];
	    v.lightpos[W] = 0.0;	/* inf */
	}

	v.lightcol[0] = gv->lights[0].color[0];
	v.lightcol[1] = gv->lights[0].color[1];
	v.lightcol[2] = gv->lights[0].color[2];

	v.ambient = (gv->lights[0].ambient[0] + gv->lights[0].ambient[1] +
		     gv->lights[0].ambient[2]) / 3.;
	v.shine = gv->lights[0].shine;

	v.surfonly = 0;		/* N/A - now uses constant color */
	strcpy((v.pgm_id), "Nvision-ALPHA!");

	return (G_put_3dview(vname, mapset, &v, w));
    }
    else {
	return (-1);
    }
}

/*!
   \brief Load 3dview

   \param vname view name
   \param gv pointer to geoview struct
   \param gd pointer to geodisplay struct
   \param w current window
   \param defsurf default geosurf struct

   \return 1
 */
int Gs_load_3dview(const char *vname, geoview * gv, geodisplay * gd,
		   struct Cell_head *w, geosurf * defsurf)
{
    const char *mapset;
    struct G_3dview v;
    int ret = -1;
    float pt[3];

    mapset = G_find_file2("3d.view", vname, "");

    if (mapset != NULL) {
	ret = G_get_3dview(vname, mapset, &v);
    }

    if (ret >= 0) {
	if (strcmp((v.pgm_id), "Nvision-ALPHA!")) {
	    G_warning(_("View not saved by this program,"
			"there may be some inconsistancies"));
	}

	/* set poly and mesh resolutions */
	v.mesh_freq = (int)(v.mesh_freq * v.vwin.ns_res / w->ns_res);
	v.poly_freq = (int)(v.poly_freq * v.vwin.ns_res / w->ns_res);

	/* Set To and FROM positions */
	/* TO */
	pt[0] = (v.from_to[TO][X] - w->west) - w->ew_res / 2.;
	pt[1] = (v.from_to[TO][Y] - w->south) - w->ns_res / 2.;
	pt[2] = v.from_to[TO][Z];
	GS_set_focus(pt);

	/* FROM */
	pt[0] = (float)v.from_to[FROM][X];
	pt[1] = (float)v.from_to[FROM][Y];
	pt[2] = (float)v.from_to[FROM][Z];
	GS_moveto_real(pt);

	if (defsurf) {
	    int dmode = 0;

	    GS_setall_drawres(v.poly_freq, v.poly_freq,
			      v.mesh_freq, v.mesh_freq);

	    while (v.display_type >= 10) {
		/* globe stuff not used */
		v.display_type -= 10;
	    }

	    /* set drawing modes */
	    if (v.colorgrid) {
		dmode |= DM_COL_WIRE;
	    }

	    if (v.shading) {
		dmode |= DM_GOURAUD;
	    }

	    switch (v.display_type) {
	    case 1:
		dmode |= DM_WIRE;

		break;
	    case 2:
		dmode |= DM_POLY;

		break;
	    case 3:
		dmode |= DM_WIRE_POLY;

		break;
	    }
	    GS_setall_drawmode(dmode);

	    /* should also set nozeros here */
	}

	/* set exaggeration */
	if (v.exag)
	    GS_set_global_exag(v.exag);

	/* Set FOV */
	if (v.fov) {
	    GS_set_fov((int)
		       (v.fov > 0 ? v.fov * 10. + 0.5 : v.fov * 10. - 0.5));
	}
	else {
	    /* TODO: do ortho */
	}

	/* Set twist */
	if (v.twist)
	    GS_set_twist((int)(v.twist > 0 ? v.twist + 0.5 : v.twist - 0.5));


	/* TODO:  OK to here - need to unravel/reverse lights stuff*** */

	if (v.lightson) {
	    /* Lights are on */

	    /* Light Position */
	    gv->lights[0].position[X] = v.lightpos[X];
	    gv->lights[0].position[Y] = v.lightpos[Y];
	    gv->lights[0].position[Z] = v.lightpos[Z];

	    /* Light Color */
	    gv->lights[0].color[0] = v.lightcol[0];
	    gv->lights[0].color[1] = v.lightcol[1];
	    gv->lights[0].color[2] = v.lightcol[2];

	    /* Light Shininess */
	    gv->lights[0].shine = v.shine;

	    /* Light Ambient */
	    gv->lights[0].ambient[0] = gv->lights[0].ambient[1] =
		gv->lights[0].ambient[2] = v.ambient * 3.;


	}			/* Done with lights */


	GS_alldraw_wire();

    }				/* Done with file */
    return (1);

}

/*!
   \brief Update no_zero ranges for attribute (actually no_null now)

   \param gs pointer to geosurf struct
   \param desc attribute id (descriptor)

   \return -1 on error
   \return 1 on success
 */
int Gs_update_attrange(geosurf * gs, int desc)
{
    long size;
    float min, max;
    typbuff *tb;
    struct BM *nm;
    int found;

    gs->att[desc].max_nz = gs->att[desc].min_nz = gs->att[desc].range_nz =
	0.0;

    if (CONST_ATT == gs_get_att_src(gs, desc)) {
	gs->att[desc].max_nz = gs->att[desc].min_nz = gs->att[desc].constant;
	min = max = gs->att[desc].constant;
	gs->att[desc].range_nz = 0.0;
    }
    else if (CF_COLOR_PACKED & gsds_get_changed(gs->att[desc].hdata)) {
	gs->att[desc].max_nz = 0xFFFFFF;
	gs->att[desc].min_nz = 0x010101;
	gs->att[desc].range_nz = 0xFFFFFF;
    }
    else {
	if (NULL == (tb = gsds_get_typbuff(gs->att[desc].hdata, 0))) {
	    return (-1);
	}

	nm = tb->nm;

	if (tb->ib) {
	    int *p;

	    size = gs->rows * gs->cols;
	    p = tb->ib;
	    INIT_MINMAX(p, nm, size, min, max, found);

	    if (!found) {
		/* all nulls! */
		return (-1);
	    }

	    size = gs->rows * gs->cols;
	    p = tb->ib;
	    SET_MINMAX(p, nm, size, min, max);
	}
	else if (tb->sb) {
	    short *p;

	    size = gs->rows * gs->cols;
	    p = tb->sb;
	    INIT_MINMAX(p, nm, size, min, max, found);

	    if (!found) {
		/* all nulls! */
		return (-1);
	    }

	    size = gs->rows * gs->cols;
	    p = tb->sb;
	    SET_MINMAX(p, nm, size, min, max);
	}
	else if (tb->cb) {
	    char *p;

	    size = gs->rows * gs->cols;
	    p = (char *)tb->cb;
	    INIT_MINMAX(p, nm, size, min, max, found);

	    if (!found) {
		/* all nulls! */
		return (-1);
	    }

	    size = gs->rows * gs->cols;
	    p = (char *)tb->cb;
	    SET_MINMAX(p, nm, size, min, max);
	}
	else if (tb->fb) {
	    float *p;

	    size = gs->rows * gs->cols;
	    p = tb->fb;
	    INIT_MINMAX(p, nm, size, min, max, found);

	    if (!found) {
		/* all nulls! */
		return (-1);
	    }

	    size = gs->rows * gs->cols;
	    p = tb->fb;
	    SET_MINMAX(p, nm, size, min, max);
	}

	gs->att[desc].max_nz = max;
	gs->att[desc].min_nz = min;
	gs->att[desc].range_nz = gs->att[desc].max_nz - gs->att[desc].min_nz;
    }

    if (ATT_TOPO == desc) {
	gs->zmin = min;
	gs->zmax = max;
	gs->zrange = gs->zmax - gs->zmin;
	gs->zminmasked = gs->zmin;
	gs->zmax_nz = gs->zmax;
	gs->zmin_nz = gs->zmin;
	gs->zrange_nz = gs->zmax_nz - gs->zmin_nz;
    }

    G_debug(3, "Gs_update_attrange(): min=%f max=%f", gs->zmin, gs->zmax);

    return (1);
}
