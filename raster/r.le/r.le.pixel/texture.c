/*
 ************************************************************
 * MODULE: r.le.pixel/texture.c                             *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze pixel-scale landscape properties     *
 *         texture.c calculates the measures for r.le.pixel *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/gis.h>
#include <grass/config.h>
#include <grass/raster.h>
#include "pixel.h"


					/* set the maximum number of categories
					   that can be in the input map */

#define NO_OF_CATEGORIES cats.num

					/* set external pointers and routines */

extern struct CHOICE *choice;
extern int g_scale, g_unit;

					/* define a structure used to read in
					   info about categories in the map */

struct Categories cats;


					/* declare a counter for the number
					   of pixels with non-null values
					   (total) in cal_edge and a flag for
					   pixels with null attribute in 
					   cal_divers */

int total, nullflag;




					/* MOVING WINDOW ANALYSIS DRIVER */

void mv_texture(int nrows, int ncols, double **buf, double **null_buf,
		double **value, int index, double *rich, int cnt,
		int cntwhole)
{
    int lc;
    register int i, j;
    double *atts, *edgeatts, attr[4], diver[4], edge[4], tex[5], **weight,
	**edgemat;

    /* set the contents of the arrays used
       used to stored the results of the
       diversity, texture, and edge 
       calculations to zero */

    attr[0] = attr[1] = attr[2] = attr[3] = 0;
    diver[0] = diver[1] = diver[2] = diver[3] = 0;
    tex[0] = tex[1] = tex[2] = tex[3] = tex[4] = 0;
    edge[0] = edge[1] = edge[2] = edge[3] = 0;


					/*************************/
    /* The weight matrix in  */
    /* "r.le.para/weight"    */
    /* must have the follow- */
    /* ing format: a, b, c   */
    /*    = category values  */
    /*                       */
    /*      a     b   c      */
    /* a    0.0 0.1 0.1      */
    /* b    0.1 0.1 0.1      */
    /* c    0.2 0.2 0.3      */

					/*************************/

    /* If juxtaposition is to be 
       calculated, then dynamically 
       allocate memory for the atts
       array and the weight matrix */

    if (choice->jux[0]) {
	atts = (double *)G_calloc(cntwhole, sizeof(double));
	weight = (double **)G_calloc(cntwhole, sizeof(double *));
	for (i = 0; i < cntwhole; i++)
	    weight[i] = (double *)G_calloc(cntwhole, sizeof(double));

	/* read in the weight matrix */

	read_weight(cntwhole, atts, weight);
	total = 0;
    }


					/*************************/
    /* The edge matrix in    */
    /* "r.le.para/edge"      */
    /* must have the follow- */
    /* ing format: a, b, c   */
    /*    = category values  */
    /*                       */
    /*      a b c            */
    /* a    0 1 1            */
    /* b    1 0 1            */
    /* c    1 1 0            */

					/*************************/

    /* If edge by type is to be
       calculated, then dynamically 
       allocate memory for the edgeatts
       array and the edgemat matrix */


    if (choice->edg[2]) {
	edgeatts = (double *)G_malloc(cntwhole * sizeof(double));
	edgemat = (double **)G_malloc(cntwhole * sizeof(double *));
	for (i = 0; i < cntwhole; i++)
	    edgemat[i] = (double *)G_malloc(cntwhole * sizeof(double));

	/* read in the edge matrix */

	read_edge(cntwhole, edgeatts, edgemat);
    }

    /* main calculation loop.  Do the
       calculations for each pixel in
       the sampling area */

    for (i = 1; i < nrows + 1; i++) {
	for (j = 1; j < ncols + 1; j++) {

	    /* find the sequence number of the
	       attribute in the richness array */

	    if ((buf[i][j] || buf[i][j] == 0.0) && null_buf[i][j] == 0.0)
		lc = check_order(buf[i][j], rich);


	    /* based on the choices made, call
	       the appropriate calc. routine */

	    if (choice->att[0])
		cal_att(buf, null_buf, i, j, nrows, ncols, attr);

	    if (choice->te2[0])
		cal_tex(buf, null_buf, i, j, nrows, ncols, lc, rich, cnt,
			tex);

	    if (choice->edg[0] || choice->jux[0])
		cal_edge(buf, null_buf, i, j, nrows, ncols, lc, edge,
			 cntwhole, atts, weight, edgeatts, edgemat, 0, 0);

	    if (choice->div[0])
		cal_divers(buf, null_buf, i, j, nrows, ncols, lc, cnt, diver);

	}
    }

    if (choice->jux[0]) {
	G_free(atts);
	for (i = 0; i < cntwhole; i++)
	    G_free(weight[i]);
	G_free(weight);
    }

    if (choice->edg[2]) {
	G_free(edgeatts);
	for (i = 0; i < cntwhole; i++)
	    G_free(edgemat[i]);
	G_free(edgemat);
    }
    /* put the calculated value for the
       selected measure into the 
       corresponding pixel in the output
       map */

    if (choice->att[0]) {	/* ATTRIBUTE MEASURES */
	if (choice->att[1])
	    value[index][0] = attr[0];	/* Mean */
	if (choice->att[2])
	    value[index][1] = attr[1];	/* St. dev. */
	if (choice->att[3])
	    value[index][2] = attr[2];	/* Min. */
	if (choice->att[4])
	    value[index][3] = attr[3];	/* Max. */
    }

    if (choice->div[0]) {	/* DIVERSITY MEASURES */
	if (choice->div[1])
	    value[index][4] = diver[0];	/* Richness */
	if (choice->div[2])
	    value[index][5] = diver[1];	/* Shannon */
	if (choice->div[3])
	    value[index][6] = diver[2];	/* Dominance */
	if (choice->div[4])
	    value[index][7] = diver[3];	/* Inv. Simpson */
    }

    if (choice->te2) {		/* TEXTURE MEASURES */
	if (choice->te2[1])	/* Contagion */
	    value[index][8] = tex[0];
	if (choice->te2[2])	/* ASM */
	    value[index][9] = tex[1];
	if (choice->te2[3])	/* IDM */
	    value[index][10] = tex[2];
	if (choice->te2[4])	/* Entropy */
	    value[index][11] = tex[3];
	if (choice->te2[5])	/* Contrast */
	    value[index][12] = tex[4];
    }

    if (choice->jux[0]) {	/* JUXTA. MEASURES */
	if (choice->jux[1])
	    value[index][13] = edge[0];	/* Mean jux. */
	if (choice->jux[2])
	    value[index][14] = edge[2];	/* St.dev. jux. */
    }

    if (choice->edg[0]) {	/* EDGE MEASURES */
	if (choice->edg[1])
	    value[index][15] = edge[1];	/* Sum of edges */
	if (choice->edg[2])
	    value[index][16] = edge[3];	/* Sum of edges 
					   by type */
    }

    return;
}




					/* WHOLE MAP, UNITS, OR REGIONS 
					   DRIVER */

void df_texture(int nrows, int ncols, double **buf, double **null_buf,
		double *rich, int cnt, int cntwhole)
{
    FILE *fp0, *fp1, *fp2, *fp3, *fp4, *fp5;
    int lc, fd, *edge1, *edge2, fc, **edgenull;
    register int i, j;
    double *atts, *edgeatts, attr[4], diver[4], edge[4], tex[5], **weight,
	**edgemat;
    CELL **edgemap_c, *edge_buf_c;
    FCELL **edgemap_f, *edge_buf_f;
    DCELL **edgemap_d, *edge_buf_d, *zscor_buf;
    RASTER_MAP_TYPE data_type;

    data_type = Rast_map_type(choice->fn, G_mapset());

    /* set the contents of the arrays
       used to stored the results of the
       diversity, texture, and edge 
       calculations to zero */

    attr[0] = attr[1] = attr[2] = attr[3] = 0;
    diver[0] = diver[1] = diver[2] = diver[3] = 0;
    edge[0] = edge[1] = edge[2] = edge[3] = 0;
    tex[0] = tex[1] = tex[2] = tex[3] = tex[4] = 0;

					/*************************/
    /* The weight matrix in  */
    /* "r.le.para/weight"    */
    /* must have the follow- */
    /* ing format: a, b, c   */
    /*    = category values  */
    /*                       */
    /*      a     b   c      */
    /* a    0.0 0.1 0.1      */
    /* b    0.1 0.1 0.1      */
    /* c    0.2 0.2 0.3      */

					/*************************/

    /* If juxtaposition is to be 
       calculated, then dynamically 
       allocate memory for the atts
       array and the weight matrix */

    if (choice->jux[0]) {
	atts = (double *)G_calloc(cntwhole, sizeof(double));
	weight = (double **)G_calloc(cntwhole, sizeof(double *));
	for (i = 0; i < cntwhole; i++)
	    weight[i] = (double *)G_calloc(cntwhole, sizeof(double));

	/* read in the weight matrix */

	read_weight(cntwhole, atts, weight);
	total = 0;
    }


					/*************************/
    /* The edge matrix in    */
    /* "r.le.para/edge"      */
    /* must have the follow- */
    /* ing format: a, b, c   */
    /*    = category values  */
    /*                       */
    /*      a b c            */
    /* a    0 1 1            */
    /* b    1 0 1            */
    /* c    1 1 0            */

					/*************************/

    /* If edge by type is to be 
       calculated, then dynamically 
       allocate memory for the edgeatts
       array and the edgemat matrix */


    if (choice->edg[2]) {
	edgeatts = (double *)G_calloc(cntwhole, sizeof(double));
	edgemat = (double **)G_calloc(cntwhole, sizeof(double *));
	for (i = 0; i < cntwhole; i++) {
	    edgemat[i] = (double *)G_calloc(cntwhole, sizeof(double));
	}

	/* read in the edge matrix */

	read_edge(cntwhole, edgeatts, edgemat);
    }


    /* dynamically allocate storage for the
       buffer that will hold the map of the
       edges and for the edgenull array */

    if (choice->edgemap) {
	switch (data_type) {
	case (CELL_TYPE):
	    edgemap_c = (CELL **) G_calloc(nrows + 3, sizeof(CELL *));
	    for (i = 0; i < nrows + 3; i++)
		edgemap_c[i] = (CELL *) G_calloc(ncols + 3, sizeof(CELL));
	    break;
	case (FCELL_TYPE):
	    edgemap_f = (FCELL **) G_calloc(nrows + 3, sizeof(FCELL *));
	    for (i = 0; i < nrows + 3; i++)
		edgemap_f[i] = (FCELL *) G_calloc(ncols + 3, sizeof(FCELL));
	    break;
	case (DCELL_TYPE):
	    edgemap_d = (DCELL **) G_calloc(nrows + 3, sizeof(DCELL *));
	    for (i = 0; i < nrows + 3; i++)
		edgemap_d[i] = (DCELL *) G_calloc(ncols + 3, sizeof(DCELL));
	    break;
	}
	edgenull = (int **)G_calloc(nrows + 3, sizeof(int *));
	for (i = 0; i < nrows + 3; i++) {
	    edgenull[i] = (int *)G_calloc(ncols + 3, sizeof(int));
	}
	for (i = 1; i < nrows + 1; i++)
	    for (j = 1; j < ncols + 1; j++)
		*(*(edgenull + i) + j) = 1;
    }

    /* main calculation loop.  Do the
       calculations for each pixel in
       the map */

    for (i = 1; i < nrows + 1; i++) {
	for (j = 1; j < ncols + 1; j++) {

	    /* find the sequence number of the
	       attribute in the richness array */

	    if ((buf[i][j] || buf[i][j] == 0.0) && null_buf[i][j] == 0.0)
		lc = check_order(buf[i][j], rich);

	    /* based on the choices made, call
	       the appropriate calc. routine */

	    if (choice->att[0])
		cal_att(buf, null_buf, i, j, nrows, ncols, attr);

	    if (choice->div[0])
		cal_divers(buf, null_buf, i, j, nrows, ncols, lc, cnt, diver);

	    if (choice->jux[0] || choice->edg[0]) {
		edge1 = edge2 = 0;
		cal_edge(buf, null_buf, i, j, nrows, ncols, lc, edge,
			 cntwhole, atts, weight, edgeatts, edgemat, &edge1,
			 &edge2);

		if (choice->edgemap) {
		    if (edge1) {
			switch (data_type) {
			case (CELL_TYPE):
			    *(*(edgemap_c + i) + j) = *(*(buf + i) + j);
			    *(*(edgemap_c + i + 1) + j) =
				*(*(buf + i + 1) + j);
			    break;
			case (FCELL_TYPE):
			    *(*(edgemap_f + i) + j) = *(*(buf + i) + j);
			    *(*(edgemap_f + i + 1) + j) =
				*(*(buf + i + 1) + j);
			    break;
			case (DCELL_TYPE):
			    *(*(edgemap_d + i) + j) = *(*(buf + i) + j);
			    *(*(edgemap_d + i + 1) + j) =
				*(*(buf + i + 1) + j);
			    break;
			}
			*(*(edgenull + i) + j) = 0;
			*(*(edgenull + i + 1) + j) = 0;
		    }
		    if (edge2) {
			switch (data_type) {
			case (CELL_TYPE):
			    *(*(edgemap_c + i) + j) = *(*(buf + i) + j);
			    *(*(edgemap_c + i) + j + 1) =
				*(*(buf + i) + j + 1);
			    break;
			case (FCELL_TYPE):
			    *(*(edgemap_f + i) + j) = *(*(buf + i) + j);
			    *(*(edgemap_f + i) + j + 1) =
				*(*(buf + i) + j + 1);
			    break;
			case (DCELL_TYPE):
			    *(*(edgemap_d + i) + j) = *(*(buf + i) + j);
			    *(*(edgemap_d + i) + j + 1) =
				*(*(buf + i) + j + 1);
			    break;
			}
			*(*(edgenull + i) + j) = 0;
			*(*(edgenull + i) + j + 1) = 0;
		    }
		}
	    }

	    if (choice->te2[0])
		cal_tex(buf, null_buf, i, j, nrows, ncols, lc, rich, cnt,
			tex);
	}
    }

    if (choice->jux[0]) {
	G_free(atts);
	for (i = 0; i < cntwhole; i++)
	    G_free(weight[i]);
	G_free(weight);
    }

    if (choice->edg[2]) {
	G_free(edgeatts);
	for (i = 0; i < cntwhole; i++)
	    G_free(edgemat[i]);
	G_free(edgemat);
    }

    /* if the edge map was requested */

    if (choice->edgemap) {
	fc = Rast_open_new("edge", data_type);
	switch (data_type) {
	case (CELL_TYPE):
	    edge_buf_c = Rast_allocate_buf(CELL_TYPE);
	    for (i = 1; i < nrows + 1; i++) {
		Rast_zero_buf(edge_buf_c, CELL_TYPE);
		Rast_set_null_value(edge_buf_c, ncols + 1, CELL_TYPE);
		for (j = 1; j < ncols + 1; j++) {
		    if (*(*(edgenull + i) + j) == 0)
			*(edge_buf_c + j - 1) = edgemap_c[i][j];
		}
		Rast_put_row(fc, edge_buf_c, CELL_TYPE);
	    }
	    break;
	case (FCELL_TYPE):
	    edge_buf_f = Rast_allocate_buf(FCELL_TYPE);
	    for (i = 1; i < nrows + 1; i++) {
		Rast_zero_buf(edge_buf_f, FCELL_TYPE);
		Rast_set_null_value(edge_buf_f, ncols + 1, FCELL_TYPE);
		for (j = 1; j < ncols + 1; j++) {
		    if (*(*(edgenull + i) + j) == 0)
			*(edge_buf_f + j - 1) = edgemap_f[i][j];
		}
		Rast_put_row(fc, edge_buf_f, FCELL_TYPE);
	    }
	    break;
	case (DCELL_TYPE):
	    edge_buf_d = Rast_allocate_buf(DCELL_TYPE);
	    for (i = 1; i < nrows + 1; i++) {
		Rast_zero_buf(edge_buf_d, DCELL_TYPE);
		Rast_set_null_value(edge_buf_d, ncols + 1, DCELL_TYPE);
		for (j = 1; j < ncols + 1; j++) {
		    if (*(*(edgenull + i) + j) == 0)
			*(edge_buf_d + j - 1) = edgemap_d[i][j];
		}
		Rast_put_row(fc, edge_buf_d, DCELL_TYPE);
	    }
	    break;
	}
	switch (data_type) {
	case (CELL_TYPE):
	    G_free(edge_buf_c);
	    for (i = 0; i < nrows + 3; i++)
		G_free(edgemap_c[i]);
	    G_free(edgemap_c);
	    break;
	case (FCELL_TYPE):
	    G_free(edge_buf_f);
	    for (i = 0; i < nrows + 3; i++)
		G_free(edgemap_f[i]);
	    G_free(edgemap_f);
	    break;
	case (DCELL_TYPE):
	    G_free(edge_buf_d);
	    for (i = 0; i < nrows + 3; i++)
		G_free(edgemap_d[i]);
	    G_free(edgemap_d);
	    break;
	}
	for (i = 0; i < nrows + 3; i++)
	    G_free(edgenull[i]);
	G_free(edgenull);
	Rast_close(fc);
    }

    /* if the zscore map was requested */

    if (choice->z) {
	fd = Rast_open_new("zscores", DCELL_TYPE);
	zscor_buf = Rast_allocate_buf(DCELL_TYPE);
	for (i = 1; i < nrows + 1; i++) {
	    Rast_zero_buf(zscor_buf, DCELL_TYPE);
	    Rast_set_null_value(zscor_buf, ncols + 1, DCELL_TYPE);
	    for (j = 1; j < ncols + 1; j++) {
		if (attr[1] > 0.0)
		    if ((buf[i][j] || buf[i][j] == 0) &&
			null_buf[i][j] == 0.0)
			*(zscor_buf + j - 1) =
			    (buf[i][j] - attr[0]) / attr[1];
	    }
	    Rast_put_row(fd, zscor_buf, DCELL_TYPE);
	}
	G_free(zscor_buf);
	Rast_close(fd);
    }

    /* open the output files and 
       save the calculated values */

    if (choice->att[0]) {
	fp0 = fopen0("r.le.out/b1-4.out", "a");
	fprintf(fp0, "%5d%5d", g_scale, g_unit);
	fprintf(fp0, "     %10.3f  %10.3f  %10.3f  %10.3f\n",
		attr[0], attr[1], attr[2], attr[3]);
	fclose(fp0);
    }

    if (choice->div[0]) {
	fp1 = fopen0("r.le.out/d1-4.out", "a");
	fprintf(fp1, "%5d%5d", g_scale, g_unit);
	fprintf(fp1, "     %10.3f  %10.3f  %10.3f  %10.3f\n",
		diver[0], diver[1], diver[2], diver[3]);
	fclose(fp1);
    }

    if (choice->te2[0]) {
	fp2 = fopen0("r.le.out/t1-5.out", "a");
	fprintf(fp2, "%5d%5d", g_scale, g_unit);
	fprintf(fp2, "     %10.3f  %10.3f  %10.3f  %10.3f  %10.3f\n",
		tex[0], tex[1], tex[2], tex[3], tex[4]);
	fclose(fp2);
    }

    if (choice->jux[0]) {
	fp3 = fopen0("r.le.out/j1-2.out", "a");
	fprintf(fp3, "%5d%5d", g_scale, g_unit);
	fprintf(fp3, "     %10.3f  %10.3f\n", edge[0], edge[2]);
	fclose(fp3);
    }

    if (choice->edg[1]) {
	fp4 = fopen0("r.le.out/e1.out", "a");
	fprintf(fp4, "%5d%5d", g_scale, g_unit);
	fprintf(fp4, "     %10.0f\n", edge[1]);
	fclose(fp4);
    }

    if (choice->edg[2]) {
	fp5 = fopen0("r.le.out/e2.out", "a");
	fprintf(fp5, "%5d%5d", g_scale, g_unit);
	fprintf(fp5, "     %10.0f\n", edge[3]);
	fclose(fp5);
    }

    return;
}





					/* ATTRIBUTE CALC. */

void cal_att(double **buf, double **null_buf, int i0, int j0, int nr, int nc,
	     double *attr)
{

    static int count;
    static double mini, maxi;
    double mean, stdv;
    static double sum, sum2;

    if (i0 == 1 && j0 == 1) {
	sum = 0.0;
	sum2 = 0.0;
	count = 0;
	maxi = 0.0;
	mini = BIG;
	mean = 0.0;
	stdv = 0.0;
    }

    if ((buf[i0][j0] || buf[i0][j0] == 0.0) && null_buf[i0][j0] == 0.0) {
	count++;
	sum += buf[i0][j0];
	sum2 += buf[i0][j0] * buf[i0][j0];

	if (buf[i0][j0] > maxi)
	    maxi = buf[i0][j0];

	if (buf[i0][j0] < mini)
	    mini = buf[i0][j0];
    }

    if (i0 == nr && j0 == nc) {

	/* calc. b1 = mean pixel attr. */

	attr[0] = mean = sum / count;

	/* calc. b2 = st. dev. pixel attr. */

	stdv = sum2 / count - mean * mean;
	if (stdv > 0)
	    attr[1] = sqrt(stdv);

	/* calc. b3 = min. pixel attr. */

	attr[2] = mini;

	/* calc. b4 = max. pixel attr. */

	attr[3] = maxi;
    }

    return;
}




					/* DIVERSITY CALC. */

void cal_divers(double **buf, double **null_buf, int i0, int j0, int nr,
		int nc, int lc, int cnt, double *diver)
{
    int tot;
    static int *density;
    register int i;
    double p, entr;

    /* if this is the first pixel, 
       dynamically allocate memory for
       the density array */

    if (1 == i0 && 1 == j0)
	density = (int *)G_calloc(cnt, sizeof(int));


    /* if the pixel has a non-null 
       attribute */

    if ((buf[i0][j0] || buf[i0][j0] == 0) && null_buf[i0][j0] == 0.0) {

	/* increment the density count for
	   the right element of the density
	   array */

	density[lc]++;

    }

    /* if this is the last pixel in the
       sampling area, do the calc. */

    if (i0 == nr && j0 == nc) {

	/* initialize the counter for the
	   total number of pixels */

	tot = 0;
	diver[0] = cnt;		/* richness */
	if (cnt > 1)
	    entr = log((double)(cnt));
	else
	    entr = 0.0;

	/* calculate the total # of pixels */

	for (i = 0; i < cnt; i++) {
	    /*printf("i=%d tot=%d density[%d]=%d\n",i,tot,i,density[i]);  */
	    tot = tot + density[i];
	}

	for (i = 0; i < cnt; i++) {
	    if (density[i] > 0 && tot > 0) {
		p = density[i] / (double)(tot);
		diver[1] += -(p * log(p));	/* Shannon */
		/*printf("i=%d p=%f shann=%f\n", i, p, diver[1]);  */
		diver[3] += p * p;
	    }
	}
	diver[2] = entr - diver[1];	/* dominance */

	diver[3] = 1 / diver[3];	/* recip. Simpson */
	G_free(density);

    }

    return;
}







					/* TEXTURE CALC. */

void cal_tex(double **buf, double **null_buf, int i0, int j0, int nr, int nc,
	     int lc, double *rich, int cnt, double *tex)
{

    int r, ln;
    double p;
    static int **GLCM;
    register int i, j;
    int GLCM_sum = 0;


    /* setup the GLCM matrix */

    if ((i0 == 1 && j0 == 1)) {
	GLCM = (int **)G_calloc(cnt, sizeof(int *));
	for (i = 0; i < cnt; i++)
	    GLCM[i] = (int *)G_calloc(cnt, sizeof(int));
    }


    /* calculate the GLCM, using
       the appropriate one of the
       seven texture methods
       (parameter te1) */

    if ((buf[i0][j0] || buf[i0][j0] == 0.0) && null_buf[i0][j0] == 0.0) {
	if (i0 > 1) {
	    if (choice->tex == 3 || choice->tex == 5 || choice->tex == 7) {
		if ((buf[i0 - 1][j0] ||
		     buf[i0 - 1][j0] == 0.0) && null_buf[i0 - 1][j0] == 0.0) {
		    ln = check_order(buf[i0 - 1][j0], rich);
		    GLCM[lc][ln]++;
		}
	    }

	    if (j0 > 1 && (choice->tex == 4 ||
			   choice->tex == 6 || choice->tex == 7)) {
		if ((buf[i0 - 1][j0 - 1] ||
		     buf[i0 - 1][j0 - 1] == 0.0) &&
		    null_buf[i0 - 1][j0 - 1] == 0.0) {
		    ln = check_order(buf[i0 - 1][j0 - 1], rich);
		    GLCM[lc][ln]++;
		}
	    }

	    if (j0 < nc && (choice->tex == 2 ||
			    choice->tex == 6 || choice->tex == 7)) {
		if ((buf[i0 - 1][j0 + 1] ||
		     buf[i0 - 1][j0 + 1] == 0.0) &&
		    null_buf[i0 - 1][j0 + 1] == 0.0) {
		    ln = check_order(buf[i0 - 1][j0 + 1], rich);
		    GLCM[lc][ln]++;
		}
	    }
	}

	if (i0 < nr) {
	    if (choice->tex == 3 || choice->tex == 5 || choice->tex == 7) {
		if ((buf[i0 + 1][j0] ||
		     buf[i0 + 1][j0] == 0.0) && null_buf[i0 + 1][j0] == 0.0) {
		    ln = check_order(buf[i0 + 1][j0], rich);
		    GLCM[lc][ln]++;
		}
	    }

	    if (j0 > 1 && (choice->tex == 2 ||
			   choice->tex == 6 || choice->tex == 7)) {
		if ((buf[i0 + 1][j0 - 1] ||
		     buf[i0 + 1][j0 - 1] == 0.0) &&
		    null_buf[i0 + 1][j0 - 1] == 0.0) {
		    ln = check_order(buf[i0 + 1][j0 - 1], rich);
		    GLCM[lc][ln]++;
		}
	    }

	    if (j0 < nc && (choice->tex == 4 ||
			    choice->tex == 6 || choice->tex == 7)) {
		if ((buf[i0 + 1][j0 + 1] ||
		     buf[i0 + 1][j0 + 1] == 0.0) &&
		    null_buf[i0 + 1][j0 + 1] == 0.0) {
		    ln = check_order(buf[i0 + 1][j0 + 1], rich);
		    GLCM[lc][ln]++;
		}
	    }
	}

	if (j0 > 1 && (choice->tex == 1 ||
		       choice->tex == 5 || choice->tex == 7)) {
	    if ((buf[i0][j0 - 1] ||
		 buf[i0][j0 - 1] == 0.0) && null_buf[i0][j0 - 1] == 0.0) {
		ln = check_order(buf[i0][j0 - 1], rich);
		GLCM[lc][ln]++;
	    }
	}

	if (j0 < nc && (choice->tex == 1 ||
			choice->tex == 5 || choice->tex == 7)) {
	    if ((buf[i0][j0 + 1] ||
		 buf[i0][j0 + 1] == 0.0) && null_buf[i0][j0 + 1] == 0.0) {
		ln = check_order(buf[i0][j0 + 1], rich);
		GLCM[lc][ln]++;
	    }
	}
    }


    /* once the end of the buffer
       has been reached, sum up the
       contents of the GLCM */


    if (i0 == nr && j0 == nc) {

	for (i = 0; i < cnt; i++) {
	    /*printf("%3d %3d %3d %3d %3d\n",GLCM[i][0],GLCM[i][1],GLCM[i][2],
	       GLCM[i][3],GLCM[i][4]);  */
	    for (j = 0; j < cnt; j++)
		GLCM_sum = GLCM_sum + GLCM[i][j];
	}

	r = GLCM_sum;

	/* for each category, compute the
	   Pij and the measures */

	for (i = 0; i < cnt; i++)
	    for (j = 0; j < cnt; j++)
		if (p = GLCM[i][j] / (double)(r)) {
		    tex[3] += p * log(p);
		    tex[1] += p * p;	/* ASM */
		    tex[2] += p / (1 + (rich[i] - rich[j]) * (rich[i] - rich[j]));	/* IDM */
		    tex[4] += p * (rich[i] - rich[j]) * (rich[i] - rich[j]);	/* Contrast */
		}


	if (tex[3])
	    tex[3] = -1.0 * tex[3];	/* Entropy */
	tex[0] = 2 * log((double)(cnt)) - tex[3];	/* Contagion */

	for (i = 0; i < cnt; i++)
	    G_free(GLCM[i]);
	G_free(GLCM);
    }

    return;
}



					/* EDGE, JUXTAPOSITION CALC. */

void cal_edge(double **buf, double **null_buf, int i0, int j0, int nr, int nc,
	      int lc, double *edge, int cntwhole, double *atts,
	      double **weight, double *edgeatts, double **edgemat, int *edge1,
	      int *edge2)
{

    int ln, lr, cnt, fr, to;
    double juxta, sum, stdv;
    static double sum2 = 0;


    /* VARIABLES:
       atts =               an array of the attributes; read from weight file.
       weight =     weight from the weight file.
       edgeatts =   
       edgemat =    
       cntwhole =   richness (total number of attributes)
       ln =         sequence number, for the neighbor pixel, in the atts 
       array
       lc =         sequence number for an attribute in the richness 
       array (NO LONGER USED)
       lr =         sequence number, for the current pixel, in the atts
       array
       edge1 =              sum of edges requested
       edge2 =              sum of edges by type requested 
       edge[0] =    running total of juxtaposition values
       edge[1] =    sum of edges
       edge[2] =    st. dev. juxtaposition
       edge[3] =    sum of edges by type
     */

    /* initialize variables */

    sum = cnt = 0;

    /* juxtaposition calc. step 1: 
       each 4 line loop checks first
       that the pixel has non-null
       attribute, second finds the
       sequence number for the 
       pixel's attribute in the 
       attribute array, third sums
       the quantity and weight, and
       fourth sums the correct cnt */

    /* if this pixel has a non-null
       attribute, do the calculations */

    if ((buf[i0][j0] || buf[i0][j0] == 0) && null_buf[i0][j0] == 0.0) {	/*1 */

	/* increment the count of non-zero
	   pixels */


	total++;
	if (choice->jux[0])
	    lr = find_loc(cntwhole, atts, buf[i0][j0]);

	/* if this pixel is not on the
	   edge of the map in the first
	   row, and if juxt. was chosen */

	if (i0 > 1 && choice->jux[0]) {


	    /* neighbor 1: i0-1,j0 */

	    if ((buf[i0 - 1][j0] || buf[i0 - 1][j0] == 0.0) &&
		null_buf[i0 - 1][j0] == 0.0) {
		ln = find_loc(cntwhole, atts, buf[i0 - 1][j0]);
		sum += 2 * weight[lr][ln];
		cnt += 2;
	    }

	    /* if this pixel is not on the
	       edge of the map in the first
	       col */

	    if (j0 > 1) {

		/* neighbor 2: i0-1,j0-1 */

		if ((buf[i0 - 1][j0 - 1] || buf[i0 - 1][j0 - 1] == 0.0) &&
		    null_buf[i0 - 1][j0 - 1] == 0.0) {
		    ln = find_loc(cntwhole, atts, buf[i0 - 1][j0 - 1]);
		    sum += weight[lr][ln];
		    cnt++;
		}
	    }

	    /* if this pixel is not on the
	       edge of the map in the last
	       col */

	    if (j0 < nc) {

		/* neighbor 3: i0-1,j0+1 */

		if ((buf[i0 - 1][j0 + 1] || buf[i0 - 1][j0 + 1] == 0.0) &&
		    null_buf[i0 - 1][j0 + 1] == 0.0) {
		    ln = find_loc(cntwhole, atts, buf[i0 - 1][j0 + 1]);
		    sum += weight[lr][ln];
		    cnt++;
		}
	    }
	}

	/* if this pixel is not on the
	   edge of the map in the last
	   row and if juxta. was chosen */

	if (i0 < nr) {
	    if (choice->jux[0]) {

		/* neighbor 4: i0+1,j0 */

		if ((buf[i0 + 1][j0] || buf[i0 + 1][j0] == 0.0) &&
		    null_buf[i0 + 1][j0] == 0.0) {
		    ln = find_loc(cntwhole, atts, buf[i0 + 1][j0]);
		    sum += 2 * weight[lr][ln];
		    cnt += 2;
		}

		/* if this pixel is not on the
		   edge of the map in the first
		   col */

		if (j0 > 1) {

		    /* neighbor 5: i0+1,j0-1 */

		    if ((buf[i0 + 1][j0 - 1] || buf[i0 + 1][j0 - 1] == 0.0) &&
			null_buf[i0 + 1][j0 - 1] == 0.0) {
			ln = find_loc(cntwhole, atts, buf[i0 + 1][j0 - 1]);
			sum += weight[lr][ln];
			cnt++;
		    }
		}

		/* if this pixel is not on the
		   edge of the map in the last
		   col */

		if (j0 < nc) {

		    /* neighbor 6: i0+1,j0+1 */

		    if ((buf[i0 + 1][j0 + 1] || buf[i0 + 1][j0 + 1] == 0.0) &&
			null_buf[i0 + 1][j0 + 1] == 0.0) {
			ln = find_loc(cntwhole, atts, buf[i0 + 1][j0 + 1]);
			sum += weight[lr][ln];
			cnt++;
		    }
		}
	    }
	    /* if the i0+1, j0 pixel is different,
	       and both pixels have non-null
	       attributes */

	    if (choice->edg[0] && (buf[i0][j0] != buf[i0 + 1][j0]) &&
		(buf[i0 + 1][j0] || buf[i0 + 1][j0] == 0.0) &&
		null_buf[i0 + 1][j0] == 0.0) {

		/* then increment edge[1] to 
		   indicate that an edge has been 
		   found if sum of edges requested */

		if (choice->edg[1])
		    edge[1]++;

		/* then increment edge[2] to 
		   indicate that an edge has been
		   found if the edge is one of the
		   types requested and if sum of 
		   edges by type requested */

		if (choice->edg[2]) {
		    fr = find_edge(cntwhole, edgeatts, buf[i0][j0]);
		    to = find_edge(cntwhole, edgeatts, buf[i0 + 1][j0]);
		    if (edgemat[fr][to]) {
			edge[3]++;
			if (choice->edgemap)
			    *edge1 = 1;
		    }
		}
	    }
	}


	/* regardless which row this pixel
	   is in (even if edge row); if 
	   this pixel is not on the edge
	   of the map in the first col, and
	   if juxta. was chosen */

	if (j0 > 1 && choice->jux[0]) {

	    /* neighbor 7: i0,j0-1 */

	    if ((buf[i0][j0 - 1] || buf[i0][j0 - 1] == 0.0) &&
		null_buf[i0][j0 - 1] == 0.0) {
		ln = find_loc(cntwhole, atts, buf[i0][j0 - 1]);
		sum += 2 * weight[lr][ln];
		cnt += 2;
	    }
	}

	/* regardless which row this pixel
	   is in (even if edge row); if 
	   this pixel is not on the edge
	   of the map in the last col, and
	   if juxta. was chosen */


	if (j0 < nc) {
	    if (choice->jux[0]) {

		/* neighbor 8: i0,j0+1 */

		if ((buf[i0][j0 + 1] || buf[i0][j0 + 1] == 0.0) &&
		    null_buf[i0][j0 + 1] == 0.0) {
		    ln = find_loc(cntwhole, atts, buf[i0][j0 + 1]);
		    sum += 2 * weight[lr][ln];
		    cnt += 2;
		}
	    }


	    /* if the i0, j0+1 pixel is different,
	       and both pixels have non-zero
	       attributes */

	    if (choice->edg[0] && (buf[i0][j0] != buf[i0][j0 + 1]) &&
		(buf[i0][j0 + 1] || buf[i0][j0 + 1] == 0.0) &&
		null_buf[i0][j0 + 1] == 0.0) {

		/* then increment edge[1] to 
		   indicate that an edge has been 
		   found if sum of edges requested */

		if (choice->edg[1])
		    edge[1]++;

		/* then increment edge[2] to 
		   indicate that an edge has been
		   found if the edge is one of the
		   types requested and if sum of 
		   edges by type requested */

		if (choice->edg[2]) {
		    fr = find_edge(cntwhole, edgeatts, buf[i0][j0]);
		    to = find_edge(cntwhole, edgeatts, buf[i0][j0 + 1]);
		    if (edgemat[fr][to]) {
			edge[3]++;
			if (choice->edgemap)
			    *edge2 = 1;
		    }
		}
	    }
	}
    }

    /* calculate juxtaposition and 
       add it to the running total
       in edge[0] */

    if (choice->jux[0]) {
	if (cnt)
	    juxta = sum / cnt;
	else
	    juxta = 0.0;
	edge[0] += juxta;
	sum2 += juxta * juxta;
    }


    /* if this is the last pixel in 
       the sampling area and juxta-
       position was chosen */

    if (choice->jux[0] && i0 == nr && j0 == nc) {

	edge[0] /= total;

	stdv = (double)sum2 / total - edge[0] * edge[0];
	if (stdv > 0)
	    edge[2] = sqrt(stdv);

	sum2 = 0;
    }

    return;
}




					/* READ THE WEIGHT FILE */

void read_weight(int richcount, double atts[], double **weight, int *attcnt)
{
    FILE *fp;
    register int i, j;
    float tmp;

    /* open the weight file */

    fp = fopen2("r.le.para/weight", "r");

    /* read the attributes into 
       the atts array */

    for (i = 0; i < richcount; i++) {
	fscanf(fp, "%f", &tmp);
	atts[i] = tmp;
    }
    while (fgetc(fp) != '\n')
	if (fgetc(fp) != ' ') {
	    fprintf(stdout, "\n");
	    fprintf(stdout,
		    "   *************************************************\n");
	    fprintf(stdout,
		    "    The weight file (r.le.para/weight) is incorrect \n");
	    fprintf(stdout,
		    "       since more/less than the %d attributes found \n",
		    richcount);
	    fprintf(stdout,
		    "       in the map are listed in the weight file     \n");
	    fprintf(stdout,
		    "   *************************************************\n");
	    exit(1);
	}
    /* read the weights, skipping
       the first column, which
       contains the attribute again */

    for (i = 0; i < richcount; i++) {
	fscanf(fp, "%f", &tmp);
	for (j = 0; j < i; j++) {
	    weight[i][j] = weight[j][i];
	}
	for (j = 0; j < richcount; j++) {
	    fscanf(fp, "%f", &tmp);
	    weight[i][j] = tmp;
	}
	while (fgetc(fp) != '\n') ;
    }
    fclose(fp);

    return;
}






					/* READ THE EDGE FILE */

void read_edge(int richcount, double atts[], double **edge)
{
    FILE *fp;
    register int i, j;
    float tmp;

    /* open the edge file */

    fp = fopen3("r.le.para/edge", "r");

    /* read the attributes into
       the atts array */

    for (i = 0; i < richcount; i++) {
	fscanf(fp, "%f", &tmp);
	atts[i] = tmp;
    }
    while (fgetc(fp) != '\n')
	if (fgetc(fp) != ' ') {
	    fprintf(stdout, "\n");
	    fprintf(stdout,
		    "   *************************************************\n");
	    fprintf(stdout,
		    "    The edge file (r.le.para/edge) is incorrect     \n");
	    fprintf(stdout,
		    "       since more/less than the %d attributes found \n",
		    richcount);
	    fprintf(stdout,
		    "       in the map are listed in the edge file       \n");
	    fprintf(stdout,
		    "   *************************************************\n");
	    exit(1);
	}

    /* read the edge weights, skipping
       the first column, which 
       contains the attribute again */

    for (i = 0; i < richcount; i++) {
	fscanf(fp, "%f", &tmp);
	for (j = 0; j < i; j++) {
	    edge[i][j] = edge[j][i];
	}
	for (j = 0; j < richcount; j++) {
	    fscanf(fp, "%f", &tmp);
	    edge[i][j] = tmp;
	}
	while (fgetc(fp) != '\n') ;
    }
    fclose(fp);

    return;
}






					/* FIND THE SEQUENCE NUMBER FOR
					   AN ATTRIBUTE IN THE ATTRIBUTE
					   ARRAY WHICH IS READ FROM THE
					   WEIGHT FILE */

int find_loc(int richcount, double atts[], double test)
{
    register int i;

    G_sleep_on_error(0);
    for (i = 0; i < richcount; i++) {
	if (test == atts[i])
	    return i;
    }
    G_fatal_error("The weight file in r.le.para is incorrect, exit\n");

    return (0);
}



					/* FIND THE SEQUENCE NUMBER FOR
					   AN ATTRIBUTE IN THE ATTRIBUTE
					   ARRAY WHICH IS READ FROM THE
					   EDGE FILE */

int find_edge(int richcount, double atts[], double test)
{
    register int i;

    G_sleep_on_error(0);
    for (i = 0; i < richcount; i++) {
	if (test == atts[i]) {
	    return i;
	}
    }
    G_fatal_error("The edge file in r.le.para is incorrect, exit\n");

    return (0);
}




					/* FIND THE SEQUENCE NO. OF AN
					   ATTRIBUTE IN THE RICHNESS ARRAY */

int check_order(double att, double *rich)
{
    int i = 0;

    while (att != rich[i])
	i++;
    return i;

}
