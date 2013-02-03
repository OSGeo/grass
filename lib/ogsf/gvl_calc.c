/*!
   \file gvl_calc.c

   \brief OGSF library - loading and manipulating volumes (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Tomas Paudits (February 2004)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

#include "rgbpack.h"
#include "mc33_table.h"

/*!
   \brief memory buffer for writing
 */
#define BUFFER_SIZE 1000000

/* USEFUL MACROS */

/* interp. */
#define	LINTERP(d,a,b)	(a + d * (b - a))
#define TINTERP(d,v) ((v[0]*(1.-d[0])*(1.-d[1])*(1.-d[2])) +\
			(v[1]*d[0]*(1.-d[1])*(1.-d[2])) + \
			(v[2]*d[0]*d[1]*(1.-d[2])) + \
			(v[3]*(1.-d[0])*d[1]*(1.-d[2])) + \
			(v[4]*(1.-d[0])*(1.-d[1])*d[2]) + \
			(v[5]*d[0]*(1.-d[1])*d[2]) + \
			(v[6]*d[0]*d[1]*d[2]) + \
			(v[7]*(1.-d[0])*d[1]*d[2]))

#define FOR_VAR i_for
#define FOR_0_TO_N(n, cmd) { int FOR_VAR; for (FOR_VAR = 0; FOR_VAR < n; FOR_VAR++) {cmd;} }

/*!
   \brief writing and reading isosurface data
 */
#define WRITE(c) gvl_write_char(dbuff->ndx_new++, &(dbuff->new), c)
#define READ() gvl_read_char(dbuff->ndx_old++, dbuff->old)
#define SKIP(n) dbuff->ndx_old = dbuff->ndx_old + n

/*!
   \brief check and set data descriptor
 */
#define IS_IN_DATA(att) ((isosurf->data_desc >> att) & 1)
#define SET_IN_DATA(att) isosurf->data_desc = (isosurf->data_desc | (1 << att))

typedef struct
{
    unsigned char *old;
    unsigned char *new;
    int ndx_old;
    int ndx_new;
    int num_zero;
} data_buffer;

int mc33_process_cube(int c_ndx, float *v);

/* global variables */
int Rows, Cols, Depths;
double ResX, ResY, ResZ;

/************************************************************************/
/* ISOSURFACES */

/*!
   \brief Write cube index

   \param ndx
   \param dbuff
 */
void iso_w_cndx(int ndx, data_buffer * dbuff)
{
    /* cube don't contains polys */
    if (ndx == -1) {
	if (dbuff->num_zero == 0) {
	    WRITE(0);
	    dbuff->num_zero++;
	}
	else if (dbuff->num_zero == 254) {
	    WRITE(dbuff->num_zero + 1);
	    dbuff->num_zero = 0;
	}
	else {
	    dbuff->num_zero++;
	}
    }
    else {			/* isosurface cube */
	if (dbuff->num_zero == 0) {
	    WRITE((ndx / 256) + 1);
	    WRITE(ndx % 256);
	}
	else {
	    WRITE(dbuff->num_zero);
	    dbuff->num_zero = 0;
	    WRITE((ndx / 256) + 1);
	    WRITE(ndx % 256);
	}
    }
}

/*!
   \brief Read cube index

   \param dbuff
 */
int iso_r_cndx(data_buffer * dbuff)
{
    int ndx, ndx2;

    if (dbuff->num_zero != 0) {
	dbuff->num_zero--;
	ndx = -1;
    }
    else {
	WRITE(ndx = READ());
	if (ndx == 0) {
	    WRITE(dbuff->num_zero = READ());
	    dbuff->num_zero--;
	    ndx = -1;
	}
	else {
	    WRITE(ndx2 = READ());
	    ndx = (ndx - 1) * 256 + ndx2;
	}
    }

    return ndx;
}

/*!
   \brief Get value from data input

   \param isosurf
   \param desc
   \param x,y,z
   \param[out] value

   \return 0
   \return ?
 */
int iso_get_cube_value(geovol_isosurf * isosurf, int desc, int x, int y,
		       int z, float *v)
{
    double d;
    geovol_file *vf;
    int type, ret = 1;

    /* get volume file from attribute handle */
    vf = gvl_file_get_volfile(isosurf->att[desc].hfile);
    type = gvl_file_get_data_type(vf);

    /* get value from volume file */
    if (type == VOL_DTYPE_FLOAT) {
	gvl_file_get_value(vf, (int)(x * ResX), (int)(y * ResY),
			   (int)(z * ResZ), v);
    }
    else if (type == VOL_DTYPE_DOUBLE) {
	gvl_file_get_value(vf, (int)(x * ResX), (int)(y * ResY),
			   (int)(z * ResZ), &d);
	*v = (float)d;
    }
    else {
	return 0;
    }

    /* null check */
    if (gvl_file_is_null_value(vf, v))
	ret = 0;

    /* adjust data */
    switch (desc) {
    case (ATT_TOPO):
	*v = (*v) - isosurf->att[desc].constant;
	break;
    case (ATT_MASK):
	if (isosurf->att[desc].constant)
	    ret = !ret;
	break;
    }

    return ret;
}

/*!
   \brief Get volume file values range

   \param isosurf
   \param desc
   \param[out] min
   \param[out] max
 */
void iso_get_range(geovol_isosurf * isosurf, int desc, double *min,
		   double *max)
{
    gvl_file_get_min_max(gvl_file_get_volfile(isosurf->att[desc].hfile), min,
			 max);
}

/*!
   \brief Read values for cube

   \param isosurf
   \param desc
   \param x,y,z
   \param[out] v

   \return
 */
int iso_get_cube_values(geovol_isosurf * isosurf, int desc, int x, int y,
			int z, float *v)
{
    int p, ret = 1;

    for (p = 0; p < 8; ++p) {
	if (iso_get_cube_value
	    (isosurf, desc, x + ((p ^ (p >> 1)) & 1), y + ((p >> 1) & 1),
	     z + ((p >> 2) & 1), &v[p]) == 0) {
	    ret = 0;
	}
    }

    return ret;
}

/*!
   \brief Calculate cube grads

   \param isosurf
   \param x,y,z
   \param grad
 */
void iso_get_cube_grads(geovol_isosurf * isosurf, int x, int y, int z,
			float (*grad)[3])
{
    float v[3];
    int i, j, k, p;

    for (p = 0; p < 8; ++p) {
	i = x + ((p ^ (p >> 1)) & 1);
	j = y + ((p >> 1) & 1);
	k = z + ((p >> 2) & 1);

	/* x */
	if (i == 0) {
	    iso_get_cube_value(isosurf, ATT_TOPO, i, j, k, &v[1]);
	    iso_get_cube_value(isosurf, ATT_TOPO, i + 1, j, k, &v[2]);
	    grad[p][0] = v[2] - v[1];
	}
	else {
	    if (i == (Cols - 1)) {
		iso_get_cube_value(isosurf, ATT_TOPO, i - 1, j, k, &v[0]);
		iso_get_cube_value(isosurf, ATT_TOPO, i, j, k, &v[1]);
		grad[p][0] = v[1] - v[0];
	    }
	    else {
		iso_get_cube_value(isosurf, ATT_TOPO, i - 1, j, k, &v[0]);
		iso_get_cube_value(isosurf, ATT_TOPO, i + 1, j, k, &v[2]);
		grad[p][0] = (v[2] - v[0]) / 2;
	    }
	}

	/* y */
	if (j == 0) {
	    iso_get_cube_value(isosurf, ATT_TOPO, i, j, k, &v[1]);
	    iso_get_cube_value(isosurf, ATT_TOPO, i, j + 1, k, &v[2]);
	    grad[p][1] = v[2] - v[1];
	}
	else {
	    if (j == (Rows - 1)) {
		iso_get_cube_value(isosurf, ATT_TOPO, i, j - 1, k, &v[0]);
		iso_get_cube_value(isosurf, ATT_TOPO, i, j, k, &v[1]);
		grad[p][1] = v[1] - v[0];
	    }
	    else {
		iso_get_cube_value(isosurf, ATT_TOPO, i, j - 1, k, &v[0]);
		iso_get_cube_value(isosurf, ATT_TOPO, i, j + 1, k, &v[2]);
		grad[p][1] = (v[2] - v[0]) / 2;
	    }
	}

	/* z */
	if (k == 0) {
	    iso_get_cube_value(isosurf, ATT_TOPO, i, j, k, &v[1]);
	    iso_get_cube_value(isosurf, ATT_TOPO, i, j, k + 1, &v[2]);
	    grad[p][2] = v[2] - v[1];
	}
	else {
	    if (k == (Depths - 1)) {
		iso_get_cube_value(isosurf, ATT_TOPO, i, j, k - 1, &v[0]);
		iso_get_cube_value(isosurf, ATT_TOPO, i, j, k, &v[1]);
		grad[p][2] = v[1] - v[0];
	    }
	    else {
		iso_get_cube_value(isosurf, ATT_TOPO, i, j, k - 1, &v[0]);
		iso_get_cube_value(isosurf, ATT_TOPO, i, j, k + 1, &v[2]);
		grad[p][2] = (v[2] - v[0]) / 2;
	    }
	}
    }
}

/*!
   \brief Process cube

   \param isosurf
   \param x,y,z
   \param dbuff
 */
void iso_calc_cube(geovol_isosurf * isosurf, int x, int y, int z,
		   data_buffer * dbuff)
{
    int i, c_ndx;
    int crnt, v1, v2, c;
    float val[MAX_ATTS][8], grad[8][3];
    float d, d3[3], d_sum[3], n[3], n_sum[3], tv;
    double min, max;

    if (isosurf->att[ATT_TOPO].changed) {
	/* read topo values, if there are NULL values then return */
	if (!iso_get_cube_values(isosurf, ATT_TOPO, x, y, z, val[ATT_TOPO])) {
	    iso_w_cndx(-1, dbuff);
	    return;
	}

	/* mask */
	if (isosurf->att[ATT_MASK].att_src == MAP_ATT) {
	    if (!iso_get_cube_values
		(isosurf, ATT_MASK, x, y, z, val[ATT_MASK])) {
		iso_w_cndx(-1, dbuff);
		return;
	    }
	}

	/* index to precalculated table */
	c_ndx = 0;
	for (i = 0; i < 8; i++) {
	    if (val[ATT_TOPO][i] > 0)
		c_ndx |= 1 << i;
	}
	c_ndx = mc33_process_cube(c_ndx, val[ATT_TOPO]);

	iso_w_cndx(c_ndx, dbuff);

	if (c_ndx == -1)
	    return;

	/* calc cube grads */
	iso_get_cube_grads(isosurf, x, y, z, grad);

    }
    else {
	/* read cube index */
	if ((c_ndx = iso_r_cndx(dbuff)) == -1)
	    return;
    }

    /* get color values */
    if (isosurf->att[ATT_COLOR].changed &&
	isosurf->att[ATT_COLOR].att_src == MAP_ATT) {
	iso_get_cube_values(isosurf, ATT_COLOR, x, y, z, val[ATT_COLOR]);
    }

    /* get transparency values */
    if (isosurf->att[ATT_TRANSP].changed &&
	isosurf->att[ATT_TRANSP].att_src == MAP_ATT) {
	iso_get_cube_values(isosurf, ATT_TRANSP, x, y, z, val[ATT_TRANSP]);
    }

    /* get shine values */
    if (isosurf->att[ATT_SHINE].changed &&
	isosurf->att[ATT_SHINE].att_src == MAP_ATT) {
	iso_get_cube_values(isosurf, ATT_SHINE, x, y, z, val[ATT_SHINE]);
    }

    /* get emit values */
    if (isosurf->att[ATT_EMIT].changed &&
	isosurf->att[ATT_EMIT].att_src == MAP_ATT) {
	iso_get_cube_values(isosurf, ATT_EMIT, x, y, z, val[ATT_EMIT]);
    }

    FOR_0_TO_N(3, d_sum[FOR_VAR] = 0.;
	       n_sum[FOR_VAR] = 0.);

    /* loop in edges */
    for (i = 0; i < cell_table[c_ndx].nedges; i++) {
	/* get edge number */
	crnt = cell_table[c_ndx].edges[i];

	/* set topo */
	if (isosurf->att[ATT_TOPO].changed) {
	    /* interior vertex */
	    if (crnt == 12) {
		FOR_0_TO_N(3,
			   WRITE((d3[FOR_VAR] =
				  d_sum[FOR_VAR] /
				  ((float)(cell_table[c_ndx].nedges))) *
				 255));
		GS_v3norm(n_sum);
		FOR_0_TO_N(3,
			   WRITE((n_sum[FOR_VAR] /
				  ((float)(cell_table[c_ndx].nedges)) +
				  1.) * 127));
		/* edge vertex */
	    }
	    else {
		/* set egdes verts */
		v1 = edge_vert[crnt][0];
		v2 = edge_vert[crnt][1];

		/* calc intersection point - edge and isosurf */
		d = val[ATT_TOPO][v1] / (val[ATT_TOPO][v1] -
					 val[ATT_TOPO][v2]);

		d_sum[edge_vert_pos[crnt][0]] += d;
		d_sum[edge_vert_pos[crnt][1]] += edge_vert_pos[crnt][2];
		d_sum[edge_vert_pos[crnt][3]] += edge_vert_pos[crnt][4];

		WRITE(d * 255);

		/* set normal for intersect. point */
		FOR_0_TO_N(3, n[FOR_VAR] =
			   LINTERP(d, grad[v1][FOR_VAR], grad[v2][FOR_VAR]));
		GS_v3norm(n);
		FOR_0_TO_N(3, n_sum[FOR_VAR] += n[FOR_VAR]);
		FOR_0_TO_N(3, WRITE((n[FOR_VAR] + 1.) * 127));
	    }
	}
	else {
	    /* read x,y,z of intersection point in cube coords */
	    if (crnt == 12) {
		WRITE(c = READ());
		d3[0] = ((float)c) / 255.0;
		WRITE(c = READ());
		d3[1] = ((float)c) / 255.0;
		WRITE(c = READ());
		d3[2] = ((float)c) / 255.0;
	    }
	    else {
		/* set egdes verts */
		v1 = edge_vert[crnt][0];
		v2 = edge_vert[crnt][1];

		WRITE(c = READ());
		d = ((float)c) / 255.0;
	    }

	    /* set normals */
	    FOR_0_TO_N(3, WRITE(READ()));
	}

	/* set color */
	if (isosurf->att[ATT_COLOR].changed &&
	    isosurf->att[ATT_COLOR].att_src == MAP_ATT) {
	    if (crnt == 12) {
		tv = TINTERP(d3, val[ATT_COLOR]);
	    }
	    else {
		tv = LINTERP(d, val[ATT_COLOR][v1], val[ATT_COLOR][v2]);
	    }

	    c = Gvl_get_color_for_value(isosurf->att[ATT_COLOR].att_data,
					&tv);

	    WRITE(c & RED_MASK);
	    WRITE((c & GRN_MASK) >> 8);
	    WRITE((c & BLU_MASK) >> 16);

	    if (IS_IN_DATA(ATT_COLOR))
		SKIP(3);
	}
	else {
	    if (isosurf->att[ATT_COLOR].att_src == MAP_ATT) {
		FOR_0_TO_N(3, WRITE(READ()));
	    }
	    else {
		if (IS_IN_DATA(ATT_COLOR))
		    SKIP(3);
	    }
	}

	/* set transparency */
	if (isosurf->att[ATT_TRANSP].changed &&
	    isosurf->att[ATT_TRANSP].att_src == MAP_ATT) {
	    if (crnt == 12) {
		tv = TINTERP(d3, val[ATT_TRANSP]);
	    }
	    else {
		tv = LINTERP(d, val[ATT_TRANSP][v1], val[ATT_TRANSP][v2]);
	    }

	    iso_get_range(isosurf, ATT_TRANSP, &min, &max);
	    c = (min != max) ? 255 - (tv - min) / (max - min) * 255 : 0;

	    WRITE(c);
	    if (IS_IN_DATA(ATT_TRANSP))
		SKIP(1);
	}
	else {
	    if (isosurf->att[ATT_TRANSP].att_src == MAP_ATT) {
		WRITE(READ());
	    }
	    else {
		if (IS_IN_DATA(ATT_TRANSP))
		    SKIP(1);
	    }
	}

	/* set shin */
	if (isosurf->att[ATT_SHINE].changed &&
	    isosurf->att[ATT_SHINE].att_src == MAP_ATT) {
	    if (crnt == 12) {
		tv = TINTERP(d3, val[ATT_SHINE]);
	    }
	    else {
		tv = LINTERP(d, val[ATT_SHINE][v1], val[ATT_SHINE][v2]);
	    }

	    iso_get_range(isosurf, ATT_SHINE, &min, &max);
	    c = (min != max) ? (tv - min) / (max - min) * 255 : 0;

	    WRITE(c);
	    if (IS_IN_DATA(ATT_SHINE))
		SKIP(1);
	}
	else {
	    if (isosurf->att[ATT_SHINE].att_src == MAP_ATT) {
		WRITE(READ());
	    }
	    else {
		if (IS_IN_DATA(ATT_SHINE))
		    SKIP(1);
	    }
	}

	/* set emit */
	if (isosurf->att[ATT_EMIT].changed &&
	    isosurf->att[ATT_EMIT].att_src == MAP_ATT) {
	    if (crnt == 12) {
		tv = TINTERP(d3, val[ATT_EMIT]);
	    }
	    else {
		tv = LINTERP(d, val[ATT_EMIT][v1], val[ATT_EMIT][v2]);
	    }

	    iso_get_range(isosurf, ATT_EMIT, &min, &max);
	    c = (min != max) ? (tv - min) / (max - min) * 255 : 0;

	    WRITE(c);
	    if (IS_IN_DATA(ATT_SHINE))
		SKIP(1);
	}
	else {
	    if (isosurf->att[ATT_EMIT].att_src == MAP_ATT) {
		WRITE(READ());
	    }
	    else {
		if (IS_IN_DATA(ATT_EMIT))
		    SKIP(1);
	    }
	}
    }
}

/*!
   \brief Fill data structure with computed isosurfaces polygons

   \param gvol pointer to geovol struct

   \return 1
 */
int gvl_isosurf_calc(geovol * gvol)
{
    int x, y, z;
    int i, a, read;
    geovol_file *vf;
    geovol_isosurf *isosurf;

    data_buffer *dbuff;
    int *need_update, need_update_global;

    dbuff = G_malloc(gvol->n_isosurfs * sizeof(data_buffer));
    need_update = G_malloc(gvol->n_isosurfs * sizeof(int));

    /* flag - changed any isosurface */
    need_update_global = 0;

    /* initialize */
    for (i = 0; i < gvol->n_isosurfs; i++) {
	isosurf = gvol->isosurf[i];

	/* initialize read/write buffers */
	dbuff[i].old = NULL;
	dbuff[i].new = NULL;
	dbuff[i].ndx_old = 0;
	dbuff[i].ndx_new = 0;
	dbuff[i].num_zero = 0;

	need_update[i] = 0;
	for (a = 1; a < MAX_ATTS; a++) {
	    if (isosurf->att[a].changed) {
		read = 0;
		/* changed to map attribute */
		if (isosurf->att[a].att_src == MAP_ATT) {
		    vf = gvl_file_get_volfile(isosurf->att[a].hfile);
		    read = 1;
		}
		/* changed threshold value */
		if (a == ATT_TOPO) {
		    isosurf->att[a].hfile = gvol->hfile;
		    vf = gvl_file_get_volfile(gvol->hfile);
		    read = 1;
		}
		/* initialize reading in selected mode */
		if (read) {
		    gvl_file_set_mode(vf, 3);
		    gvl_file_start_read(vf);
		}

		/* set update flag - isosurface will be calc */
		if (read || IS_IN_DATA(a)) {
		    need_update[i] = 1;
		    need_update_global = 1;
		}
	    }
	}

	if (need_update[i]) {
	    /* set data buffer */
	    dbuff[i].old = isosurf->data;
	}
    }

    /* calculate if only some isosurface changed */
    if (need_update_global) {

	ResX = gvol->isosurf_x_mod;
	ResY = gvol->isosurf_y_mod;
	ResZ = gvol->isosurf_z_mod;

	Cols = gvol->cols / ResX;
	Rows = gvol->rows / ResY;
	Depths = gvol->depths / ResZ;

	/* calc isosurface - marching cubes - start */

	for (z = 0; z < Depths - 1; z++) {
	    for (y = 0; y < Rows - 1; y++) {
		for (x = 0; x < Cols - 1; x++) {
		    for (i = 0; i < gvol->n_isosurfs; i++) {
			/* recalculate only changed isosurfaces */
			if (need_update[i]) {
			    iso_calc_cube(gvol->isosurf[i], x, y, z,
					  &dbuff[i]);
			}
		    }
		}
	    }
	}

    }
    /* end */

    /* deinitialize */
    for (i = 0; i < gvol->n_isosurfs; i++) {
	isosurf = gvol->isosurf[i];

	/* set new isosurface data */
	if (need_update[i]) {
	    if (dbuff[i].num_zero != 0)
		gvl_write_char(dbuff[i].ndx_new++, &(dbuff[i].new),
			       dbuff[i].num_zero);

	    if (dbuff[i].old == isosurf->data)
		dbuff[i].old = NULL;
	    G_free(isosurf->data);
	    gvl_align_data(dbuff[i].ndx_new, &(dbuff[i].new));
	    isosurf->data = dbuff[i].new;
	    isosurf->data_desc = 0;
	}

	for (a = 1; a < MAX_ATTS; a++) {
	    if (isosurf->att[a].changed) {
		read = 0;
		/* changed map attribute */
		if (isosurf->att[a].att_src == MAP_ATT) {
		    vf = gvl_file_get_volfile(isosurf->att[a].hfile);
		    read = 1;
		}
		/* changed threshold value */
		if (a == ATT_TOPO) {
		    isosurf->att[a].hfile = gvol->hfile;
		    vf = gvl_file_get_volfile(gvol->hfile);
		    read = 1;
		}
		/* deinitialize reading */
		if (read) {
		    gvl_file_end_read(vf);

		    /* set data description */
		    SET_IN_DATA(a);
		}
		isosurf->att[a].changed = 0;
	    }
	    else if (isosurf->att[a].att_src == MAP_ATT) {
		/* set data description */
		SET_IN_DATA(a);
	    }
	}
    }
    
    /* TODO: G_free() dbuff and need_update ??? */

    return (1);
}

/*!
   \brief ADD

   \param pos
   \param data
   \param c
 */
void gvl_write_char(int pos, unsigned char **data, unsigned char c)
{
    /* check to need allocation memory */
    if ((pos % BUFFER_SIZE) == 0) {
	*data = (char *)G_realloc(*data,
				  sizeof(char) * ((pos / BUFFER_SIZE) +
						  1) * BUFFER_SIZE);
	if (!(*data)) {
	    return;
	}

	G_debug(3,
		"gvl_write_char(): reallocate memory for pos : %d to : %d B",
		pos, sizeof(char) * ((pos / BUFFER_SIZE) + 1) * BUFFER_SIZE);
    }

    (*data)[pos] = c;
}

/*!
   \brief Read char

   \param pos position index
   \param data data buffer

   \return char on success
   \return NULL on failure
 */
unsigned char gvl_read_char(int pos, const unsigned char *data)
{
    if (!data)
	return '\0';
    
    return data[pos];
}

/*!
   \brief Append data to buffer

   \param pos position index
   \param data data buffer
 */
void gvl_align_data(int pos, unsigned char **data)
{
    unsigned char *p = *data;
    

    /* realloc memory to fit in data length */
    p = (unsigned char *)G_realloc(p, sizeof(unsigned char) * pos);	/* G_fatal_error */
    if (!p) {
	return;
    }

    G_debug(3, "gvl_align_data(): reallocate memory finally to : %d B", pos);

    if (pos == 0)
	p = NULL;
    
    *data = p;

    return;
}

/************************************************************************/
/* SLICES */

/************************************************************************/

#define DISTANCE_2(x1, y1, x2, y2)	sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2))

#define SLICE_MODE_INTERP_NO	0
#define SLICE_MODE_INTERP_YES	1

/*!
   \brief Get volume value

   \param gvl pointer to geovol struct
   \param x,y,z

   \return value
 */
float slice_get_value(geovol * gvl, int x, int y, int z)
{
    static double d;
    static geovol_file *vf;
    static int type;
    static float value;

    if (x < 0 || y < 0 || z < 0 || (x > gvl->cols - 1) || (y > gvl->rows - 1)
	|| (z > gvl->depths - 1))
	return 0.;

    /* get volume file from attribute handle */
    vf = gvl_file_get_volfile(gvl->hfile);
    type = gvl_file_get_data_type(vf);

    /* get value from volume file */
    if (type == VOL_DTYPE_FLOAT) {
	gvl_file_get_value(vf, x, y, z, &value);
    }
    else if (type == VOL_DTYPE_DOUBLE) {
	gvl_file_get_value(vf, x, y, z, &d);
	value = (float)d;
    }
    else {
	return 0.;
    }

    return value;
}

/*!
   \brief Calculate slices

   \param gvl pointer to geovol struct
   \param ndx_slc
   \param colors

   \return 1
 */
int slice_calc(geovol * gvl, int ndx_slc, void *colors)
{
    int cols, rows, c, r;
    int i, j, k, pos, color;
    int *p_x, *p_y, *p_z;
    float *p_ex, *p_ey, *p_ez;
    float value, v[8];
    float x, y, z, ei, ej, ek, stepx, stepy, stepz;
    float f_cols, f_rows, distxy, distz, modxy, modx, mody, modz;

    geovol_slice *slice;
    geovol_file *vf;

    slice = gvl->slice[ndx_slc];

    /* set mods, pointer to x, y, z step value */
    if (slice->dir == X) {
	modx = ResY;
	mody = ResZ;
	modz = ResX;
	p_x = &k;
	p_y = &i;
	p_z = &j;
	p_ex = &ek;
	p_ey = &ei;
	p_ez = &ej;
    }
    else if (slice->dir == Y) {
	modx = ResX;
	mody = ResZ;
	modz = ResY;
	p_x = &i;
	p_y = &k;
	p_z = &j;
	p_ex = &ei;
	p_ey = &ek;
	p_ez = &ej;
    }
    else {
	modx = ResX;
	mody = ResY;
	modz = ResZ;
	p_x = &i;
	p_y = &j;
	p_z = &k;
	p_ex = &ei;
	p_ey = &ej;
	p_ez = &ek;
    }

    /* distance between slice def. points */
    distxy = DISTANCE_2(slice->x2, slice->y2, slice->x1, slice->y1);
    distz = fabsf(slice->z2 - slice->z1);

    /* distance between slice def points is zero - nothing to do */
    if (distxy == 0. || distz == 0.) {
	return (1);
    }

    /* start reading volume file */
    vf = gvl_file_get_volfile(gvl->hfile);
    gvl_file_set_mode(vf, 3);
    gvl_file_start_read(vf);

    /* set xy resolution */
    modxy =
	DISTANCE_2((slice->x2 - slice->x1) / distxy * modx,
		   (slice->y2 - slice->y1) / distxy * mody, 0., 0.);

    /* cols/rows of slice */
    f_cols = distxy / modxy;
    cols = f_cols > (int)f_cols ? (int)f_cols + 1 : (int)f_cols;

    f_rows = distz / modz;
    rows = f_rows > (int)f_rows ? (int)f_rows + 1 : (int)f_rows;

    /* set x,y intially to first slice point */
    x = slice->x1;
    y = slice->y1;

    /* set x,y step */
    stepx = (slice->x2 - slice->x1) / f_cols;
    stepy = (slice->y2 - slice->y1) / f_cols;
    stepz = (slice->z2 - slice->z1) / f_rows;

    /* set position in slice data */
    pos = 0;

    /* loop in slice cols */
    for (c = 0; c < cols + 1; c++) {

	/* convert x, y to integer - index in grid */
	i = (int)x;
	j = (int)y;

	/* distance between index and real position */
	ei = x - (float)i;
	ej = y - (float)j;

	/* set z to slice z1 point */
	z = slice->z1;

	/* loop in slice rows */
	for (r = 0; r < rows + 1; r++) {

	    /* distance between index and real position */
	    k = (int)z;
	    ek = z - (float)k;

	    /* get interpolated value */
	    if (slice->mode == SLICE_MODE_INTERP_YES) {
		/* get grid values */
		v[0] = slice_get_value(gvl, *p_x, *p_y, *p_z);
		v[1] = slice_get_value(gvl, *p_x + 1, *p_y, *p_z);
		v[2] = slice_get_value(gvl, *p_x, *p_y + 1, *p_z);
		v[3] = slice_get_value(gvl, *p_x + 1, *p_y + 1, *p_z);

		v[4] = slice_get_value(gvl, *p_x, *p_y, *p_z + 1);
		v[5] = slice_get_value(gvl, *p_x + 1, *p_y, *p_z + 1);
		v[6] = slice_get_value(gvl, *p_x, *p_y + 1, *p_z + 1);
		v[7] = slice_get_value(gvl, *p_x + 1, *p_y + 1, *p_z + 1);

		/* get interpolated value */
		value = v[0] * (1. - *p_ex) * (1. - *p_ey) * (1. - *p_ez)
		    + v[1] * (*p_ex) * (1. - *p_ey) * (1. - *p_ez)
		    + v[2] * (1. - *p_ex) * (*p_ey) * (1. - *p_ez)
		    + v[3] * (*p_ex) * (*p_ey) * (1. - *p_ez)
		    + v[4] * (1. - *p_ex) * (1. - *p_ey) * (*p_ez)
		    + v[5] * (*p_ex) * (1. - *p_ey) * (*p_ez)
		    + v[6] * (1. - *p_ex) * (*p_ey) * (*p_ez)
		    + v[7] * (*p_ex) * (*p_ey) * (*p_ez);

		/* no interp value */
	    }
	    else {
		value = slice_get_value(gvl, *p_x, *p_y, *p_z);
	    }

	    /* translate value to color */
	    color = Gvl_get_color_for_value(colors, &value);

	    /* write color to slice data */
	    gvl_write_char(pos++, &(slice->data), color & RED_MASK);
	    gvl_write_char(pos++, &(slice->data), (color & GRN_MASK) >> 8);
	    gvl_write_char(pos++, &(slice->data), (color & BLU_MASK) >> 16);

	    /* step in z */
	    if (r + 1 > f_rows) {
		z += stepz * (f_rows - (float)r);
	    }
	    else {
		z += stepz;
	    }
	}

	/* step in x,y */
	if (c + 1 > f_cols) {
	    x += stepx * (f_cols - (float)c);
	    y += stepy * (f_cols - (float)c);
	}
	else {
	    x += stepx;
	    y += stepy;
	}
    }

    /* end reading volume file */
    gvl_file_end_read(vf);
    gvl_align_data(pos, &(slice->data));

    return (1);
}

/*!
   \brief Calculate slices for given volume set

   \param gvol pointer to geovol struct

   \return 1
 */
int gvl_slices_calc(geovol *gvol)
{
    int i;
    void *colors;

    G_debug(5, "gvl_slices_calc(): id=%d", gvol->gvol_id);
    
    /* set current resolution */
    ResX = gvol->slice_x_mod;
    ResY = gvol->slice_y_mod;
    ResZ = gvol->slice_z_mod;

    /* set current num of cols, rows, depths */
    Cols = gvol->cols / ResX;
    Rows = gvol->rows / ResY;
    Depths = gvol->depths / ResZ;

    /* load colors for geovol file */
    Gvl_load_colors_data(&colors, gvl_file_get_name(gvol->hfile));

    /* calc changed slices */
    for (i = 0; i < gvol->n_slices; i++) {
	if (gvol->slice[i]->changed) {
	    slice_calc(gvol, i, colors);

	    /* set changed flag */
	    gvol->slice[i]->changed = 0;
	}
    }

    /* free color */
    Gvl_unload_colors_data(colors);

    return (1);
}
