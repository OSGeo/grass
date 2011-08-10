#include <stdlib.h>
#include <math.h>
#include "vizual.h"
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include "local_proto.h"


#define XDIMYDIM (Headfax.xdim*Headfax.ydim)

float TEMP_VERT[13][3];
float TEMP_NORM[13][3];

static float DATA[8];


/* function prototypes */
static void percent(int z, int zloop);
static void calc_cube_info(float *data[], int z1);
static void xings_fnorm(int c_ndx, int t_ndx);
static void calc_fnorm(int c_ndx);
static void xings_grad(float *data[], int c_ndx, int x1, int y1,
		       int z1, int t_ndx);
static void normalize(float n[3]);


void viz_iso_surface(void *g3map, RASTER3D_Region * g3reg,
		     cmndln_info * linefax, int quiet)
{
    float *data[4];		/* will hold 4 slices of xy data */
    int zloop;
    int z;			/* looping variables */
    int slice;
    float *temp;

    zloop = Headfax.zdim - 1;	/*crop to permit use of gradients */

    /* for gradient shading processing 4 slices of xy data at a time */

    for (z = 0; z < zloop; z++) {	/*dpg */
	if (!quiet)
	    percent(z, zloop);

	if (!z) {		/* first time thru need to read in four slices of data */
	    for (slice = 0; slice < 4; slice++) {
		data[slice] = (float *)G_malloc(sizeof(float) * XDIMYDIM);

		if (slice)
		    /*read in data */
		    r3read_level(g3map, g3reg, &Headfax, data[slice],
				 slice - 1);
	    }
	}
	else {
	    temp = data[0];
	    data[0] = data[1];
	    data[1] = data[2];
	    data[2] = data[3];
	    data[3] = temp;
	    if (z < zloop - 1)
		r3read_level(g3map, g3reg, &Headfax, data[3], z + 2);

	}
	calc_cube_info(data, z);
    }
}


/************************  percent *******************************************/
static void percent(int z, int zloop)
{
    int percent;
    static int flag = 1;

    if (flag) {
	fprintf(stderr, "display file completed:");
	flag = 0;
    }
    percent = (z * 100) / zloop;
    fprintf(stderr, "  %3d %%", percent);
    fprintf(stderr, "\b\b\b\b\b\b\b");
}


/************************ calc_cube_info  ************************************/
static void calc_cube_info(float *data[], int z1)
{
    int x1, y1;
    int x2, y2;
    int xloop, yloop;
    int y1dist, y2dist;
    int c_ndx;			/*index into cell table */
    int t_ndx = 0;		/*index into array of thresholds */
    int a = 0;			/*keeps track of how many thresholds are contained in a cell */
    cmndln_info *linefax;
    cube_info *CUBEFAX;
    int vnum;			/* index to loop through vertices of cube */

    CUBEFAX = CUBE.data;	/* make old code fit new structure */

    linefax = &Headfax.linefax;
    xloop = (Headfax.xdim);
    yloop = (Headfax.ydim);

    /* the following commands turn on appropriate bits in the c_ndx variable
     *  based on vertex value.
     */
    /* loop down in y */
    for (y1 = 0; y1 < yloop - 1; y1++) {	/* dpg */
	y2 = y1 + 1;
	y1dist = y1 * (Headfax.xdim);
	y2dist = y2 * (Headfax.xdim);

	/* loop across in x */
	/* loop for each threshold for each cube */
	for (x1 = 0; x1 < xloop - 1; x1++) {	/* dpg */
	    a = 0;		/* set to zero for each cell */
	    for (t_ndx = 0; t_ndx < (linefax->nthres); t_ndx++) {
		x2 = x1 + 1;

		DATA[0] = data[1][y2dist + x1];
		DATA[1] = data[1][y2dist + x2];
		DATA[2] = data[1][y1dist + x2];
		DATA[3] = data[1][y1dist + x1];
		DATA[4] = data[2][y2dist + x1];
		DATA[5] = data[2][y2dist + x2];
		DATA[6] = data[2][y1dist + x2];
		DATA[7] = data[2][y1dist + x1];
		c_ndx = 0;

		for (vnum = 0; vnum < 8; vnum++) {
		    if (DATA[vnum] >= linefax->tvalue[t_ndx])
			c_ndx |= 1 << vnum;
		}
		/* currently masks in grid3 files are in xy-plane, so check for cube with one
		   or more (vertical) edges masked out & don't draw polys. This gets rid of
		   walls around masked surfaces, but leaves edges jagged */
#ifdef OLDSTUFF
		if (!DATA[0] && !DATA[4])
		    c_ndx = 0;
		else if (!DATA[1] && !DATA[5])
		    c_ndx = 0;
		else if (!DATA[2] && !DATA[6])
		    c_ndx = 0;
		else if (!DATA[3] && !DATA[7])
		    c_ndx = 0;
#endif
		/* CHANGED: use NULLS instead of zeros - if any are null, polygons
		   are undefined - or else need to define polygons which exclude nulls under
		   various combinations e.g., give each null a tetrahedron of influence */
		if (Rast_is_f_null_value((FCELL *) (DATA + 0)) ||
		    Rast_is_f_null_value((FCELL *) (DATA + 1)) ||
		    Rast_is_f_null_value((FCELL *) (DATA + 2)) ||
		    Rast_is_f_null_value((FCELL *) (DATA + 3)) ||
		    Rast_is_f_null_value((FCELL *) (DATA + 4)) ||
		    Rast_is_f_null_value((FCELL *) (DATA + 5)) ||
		    Rast_is_f_null_value((FCELL *) (DATA + 6)) ||
		    Rast_is_f_null_value((FCELL *) (DATA + 7)))
		    c_ndx = 0;

		/* c_ndx numbers (1 - 254) contain polygons */
		/*if (c_ndx > 0 && c_ndx < 254) this is the hole bug */
		if (c_ndx > 0 && c_ndx < 255) {	/* -dpg */
		    CUBEFAX[a].t_ndx = t_ndx;
		    NTHRESH = a;
		    a++;
		    switch (linefax->litmodel) {
		    case 1:
			xings_fnorm(c_ndx, t_ndx);
			break;
		    case 2:
		    case 3:
			xings_grad(data, c_ndx, x1, y1, z1, t_ndx);
			break;
		    }
		    fill_cfax(&CUBE, linefax->litmodel, c_ndx,
			      TEMP_VERT, TEMP_NORM);
		}
	    }
	    if (!a)
		CUBEFAX[0].npoly = 0;	/* sets 'empty' flag */

	    CUBE.n_thresh = a;
	    write_cube(&CUBE, x1, &Headfax);
	}
    }
}


/***********************************  xings_fnorm****************************/
/*  This subroutine is called once for each cell(cube) at given threshold value.
 **  This is the subroutine that is called if flat shading is to be used. 
 **  Subroutine vertices are determined for the polygons that will be 
 **  saved in a display file as well as the normal to the polygon
 */
static void xings_fnorm(int c_ndx, int t_ndx)
{
    cmndln_info *linefax;
    register int i;		/* loop count variable incremented to to examine each edge */

    /* listed for c_ndx of cube being examined */

    linefax = &Headfax.linefax;
    for (i = 0; i < cell_table[c_ndx].nedges; i++) {
	switch (cell_table[c_ndx].edges[i]) {
	case 1:
	    TEMP_VERT[1][0] =
		LINTERP(DATA[0], DATA[1], linefax->tvalue[t_ndx]);
	    TEMP_VERT[1][1] = 255;
	    TEMP_VERT[1][2] = 0;
	    break;

	case 2:
	    TEMP_VERT[2][0] = 255;
	    TEMP_VERT[2][1] =
		LINTERP(DATA[2], DATA[1], linefax->tvalue[t_ndx]);
	    TEMP_VERT[2][2] = 0;
	    break;

	case 3:
	    TEMP_VERT[3][0] =
		LINTERP(DATA[3], DATA[2], linefax->tvalue[t_ndx]);
	    TEMP_VERT[3][1] = 0;
	    TEMP_VERT[3][2] = 0;
	    break;

	case 4:
	    TEMP_VERT[4][0] = 0;
	    TEMP_VERT[4][1] =
		LINTERP(DATA[3], DATA[0], linefax->tvalue[t_ndx]);
	    TEMP_VERT[4][2] = 0;
	    break;

	case 5:
	    TEMP_VERT[5][0] =
		LINTERP(DATA[4], DATA[5], linefax->tvalue[t_ndx]);
	    TEMP_VERT[5][1] = 255;
	    TEMP_VERT[5][2] = 255;
	    break;

	case 6:
	    TEMP_VERT[6][0] = 255;
	    TEMP_VERT[6][1] =
		LINTERP(DATA[6], DATA[5], linefax->tvalue[t_ndx]);
	    TEMP_VERT[6][2] = 255;
	    break;

	case 7:
	    TEMP_VERT[7][0] =
		LINTERP(DATA[7], DATA[6], linefax->tvalue[t_ndx]);
	    TEMP_VERT[7][1] = 0;
	    TEMP_VERT[7][2] = 255;
	    break;

	case 8:
	    TEMP_VERT[8][0] = 0;
	    TEMP_VERT[8][1] =
		LINTERP(DATA[7], DATA[4], linefax->tvalue[t_ndx]);
	    TEMP_VERT[8][2] = 255;
	    break;

	case 9:
	    TEMP_VERT[9][0] = 0;
	    TEMP_VERT[9][1] = 255;
	    TEMP_VERT[9][2] =
		LINTERP(DATA[0], DATA[4], linefax->tvalue[t_ndx]);
	    break;

	case 10:
	    TEMP_VERT[10][0] = 255;
	    TEMP_VERT[10][1] = 255;
	    TEMP_VERT[10][2] =
		LINTERP(DATA[1], DATA[5], linefax->tvalue[t_ndx]);
	    break;

	case 11:
	    TEMP_VERT[11][0] = 0;
	    TEMP_VERT[11][1] = 0;
	    TEMP_VERT[11][2] =
		LINTERP(DATA[3], DATA[7], linefax->tvalue[t_ndx]);
	    break;

	case 12:
	    TEMP_VERT[12][0] = 255;
	    TEMP_VERT[12][1] = 0;
	    TEMP_VERT[12][2] =
		LINTERP(DATA[2], DATA[6], linefax->tvalue[t_ndx]);
	}
    }
    calc_fnorm(c_ndx);
}


/*************************** calc_fnorm  ************************************/
/* this routine calculates the normal to a polygon for flat shading */
static void calc_fnorm(int c_ndx)
{
    float x1, y1, z1, x2, y2, z2, x3, y3, z3;
    int i = 0;
    int inref;
    int poly_num = 0;		/*the polygon number */
    double r2x, r2y, r2z, r1x, r1y, r1z;

    /* cell_table structure included in .h file */
    while (i < cell_table[c_ndx].npolys * 3) {
	/* indirect referencing into the TEMP_VERT array */
	inref = cell_table[c_ndx].polys[i];
	x1 = TEMP_VERT[inref][0];
	y1 = TEMP_VERT[inref][1];
	z1 = TEMP_VERT[inref][2];
	i++;

	inref = cell_table[c_ndx].polys[i];
	x2 = TEMP_VERT[inref][0];
	y2 = TEMP_VERT[inref][1];
	z2 = TEMP_VERT[inref][2];
	i++;

	inref = cell_table[c_ndx].polys[i];
	x3 = TEMP_VERT[inref][0];
	y3 = TEMP_VERT[inref][1];
	z3 = TEMP_VERT[inref][2];
	i++;

	/* Cramer's rule used to calculate the coefficients */
	r2x = x1 - x2;
	r2y = y1 - y2;
	r2z = z1 - z2;
	r1x = x3 - x2;
	r1y = y3 - y2;
	r1z = z3 - z2;

	/* assign the unit vector normal to the polygon for use in lighting */
	TEMP_NORM[poly_num][0] = (float)(r1y * r2z - r1z * r2y);
	TEMP_NORM[poly_num][1] = (float)(r1z * r2x - r1x * r2z);
	TEMP_NORM[poly_num][2] = (float)(r1x * r2y - r1y * r2x);
	normalize(TEMP_NORM[poly_num]);
	poly_num += 1;
    }
}


/*****************************   xings_grad   ********************************/
/*  this subroutine is called once for each cell(cube) at given threshold tvalue 
 **  vertices are determined for the polygons that will be written to a display
 **  file.  Gradients for each vertex are determined to be used as normals
 **  in lighting calculations.
 **  gradients are stored in temporary variables that are also written to display**  file. 
 */
static void xings_grad(float *data[], int c_ndx, int x1, int y1, int z1,
		       int t_ndx)
{
    cmndln_info *linefax;
    register int i;		/* loop count variable incremented to examine each edge */

    /* listed for c_ndx of cube being examined */
    int nedges;			/* number of edges as listed for given c_ndx into cell_table */
    int crnt_edge;		/* number of the current edge being examined */

    int x0, y0, z0;		/* used to calculate desired location in data array */
    int x2, y2, z2;
    int x3, y3, z3;		/* used to calculate desired location in data array */

    int a = 0, b = 0;		/* the x,y,z components of the gradient vector */
    float delta = 0;

    /* the following variables are used to calculate gradients in the x,y,Z
     ** direction.  gradients are going to be used to calculate relative positions of
     ** vertices to the light source. though not specifically necessary, the offset
     ** variables facilitate error checking. 
     */
    float Data1xoffset, Data1yoffset, Data1zoffset;
    float Data2xoffset, Data2yoffset, Data2zoffset;
    float Data3xoffset, Data3yoffset, Data3zoffset;
    float Data4xoffset, Data4yoffset, Data4zoffset;
    float Data5xoffset, Data5yoffset, Data5zoffset;
    float Data6xoffset, Data6yoffset, Data6zoffset;
    float Data7xoffset, Data7yoffset, Data7zoffset;
    float Data8xoffset, Data8yoffset, Data8zoffset;

    /*  HOLDS THE GRADIENT FOR EACH VERTEX OF THE CELL BEING EXAMINED */
    float Data_Grad[9][3];

    linefax = &Headfax.linefax;

    x0 = x1 - 1;
    y0 = y1 - 1;
    z0 = z1 - 1;

    x2 = x1 + 1;
    y2 = y1 + 1;
    z2 = z1 + 1;

    x3 = x2 + 1;
    y3 = y2 + 1;
    z3 = z2 + 1;

    if (x0 >= 0) {
	Data1xoffset = data[1][y2 * Headfax.xdim + x0];
	Data4xoffset = data[1][y1 * Headfax.xdim + x0];
	Data5xoffset = data[2][y2 * Headfax.xdim + x0];
	Data8xoffset = data[2][y1 * Headfax.xdim + x0];
    }
    else {
	Data1xoffset = 2.0 * data[1][y2 * Headfax.xdim + x1]
	    - data[1][y2 * Headfax.xdim + x2];
	Data4xoffset = 2.0 * data[1][y1 * Headfax.xdim + x1]
	    - data[1][y1 * Headfax.xdim + x2];
	Data5xoffset = 2.0 * data[2][y2 * Headfax.xdim + x1]
	    - data[2][y2 * Headfax.xdim + x2];
	Data8xoffset = 2.0 * data[2][y1 * Headfax.xdim + x1]
	    - data[2][y1 * Headfax.xdim + x2];
    }

    if (x3 < Headfax.xdim) {
	Data2xoffset = data[1][y2 * Headfax.xdim + x3];
	Data3xoffset = data[1][y1 * Headfax.xdim + x3];
	Data6xoffset = data[2][y2 * Headfax.xdim + x3];
	Data7xoffset = data[2][y1 * Headfax.xdim + x3];
    }
    else {
	Data2xoffset = 2.0 * data[1][y2 * Headfax.xdim + x2]
	    - data[1][y2 * Headfax.xdim + x1];
	Data3xoffset = 2.0 * data[1][y1 * Headfax.xdim + x2]
	    - data[1][y1 * Headfax.xdim + x1];
	Data6xoffset = 2.0 * data[2][y2 * Headfax.xdim + x2]
	    - data[2][y2 * Headfax.xdim + x1];
	Data7xoffset = 2.0 * data[2][y1 * Headfax.xdim + x2]
	    - data[2][y1 * Headfax.xdim + x1];
    }

    if (y0 >= 0) {
	Data3yoffset = data[1][y0 * Headfax.xdim + x2];
	Data4yoffset = data[1][y0 * Headfax.xdim + x1];
	Data7yoffset = data[2][y0 * Headfax.xdim + x2];
	Data8yoffset = data[2][y0 * Headfax.xdim + x1];
    }
    else {
	Data3yoffset = 2.0 * data[1][y1 * Headfax.xdim + x2]
	    - data[1][y2 * Headfax.xdim + x2];
	Data4yoffset = 2.0 * data[1][y1 * Headfax.xdim + x1]
	    - data[1][y2 * Headfax.xdim + x1];
	Data7yoffset = 2.0 * data[2][y1 * Headfax.xdim + x2]
	    - data[2][y2 * Headfax.xdim + x2];
	Data8yoffset = 2.0 * data[2][y1 * Headfax.xdim + x1]
	    - data[2][y2 * Headfax.xdim + x1];
    }

    if (y3 < Headfax.ydim) {
	Data1yoffset = data[1][y3 * Headfax.xdim + x1];
	Data2yoffset = data[1][y3 * Headfax.xdim + x2];
	Data5yoffset = data[2][y3 * Headfax.xdim + x1];
	Data6yoffset = data[2][y3 * Headfax.xdim + x2];
    }
    else {
	Data1yoffset = 2.0 * data[1][y2 * Headfax.xdim + x1]
	    - data[1][y1 * Headfax.xdim + x1];
	Data2yoffset = 2.0 * data[1][y2 * Headfax.xdim + x2]
	    - data[1][y1 * Headfax.xdim + x2];
	Data5yoffset = 2.0 * data[2][y2 * Headfax.xdim + x1]
	    - data[2][y1 * Headfax.xdim + x1];
	Data6yoffset = 2.0 * data[2][y2 * Headfax.xdim + x2]
	    - data[2][y1 * Headfax.xdim + x2];
    }

    if (z0 >= 0) {
	Data1zoffset = data[0][y2 * Headfax.xdim + x1];
	Data2zoffset = data[0][y2 * Headfax.xdim + x2];
	Data3zoffset = data[0][y1 * Headfax.xdim + x2];
	Data4zoffset = data[0][y1 * Headfax.xdim + x1];
    }
    else {
	Data1zoffset = 2.0 * data[1][y2 * Headfax.xdim + x1]
	    - data[2][y2 * Headfax.xdim + x1];
	Data2zoffset = 2.0 * data[1][y2 * Headfax.xdim + x2]
	    - data[2][y2 * Headfax.xdim + x2];
	Data3zoffset = 2.0 * data[1][y1 * Headfax.xdim + x2]
	    - data[2][y1 * Headfax.xdim + x2];
	Data4zoffset = 2.0 * data[1][y1 * Headfax.xdim + x1]
	    - data[2][y1 * Headfax.xdim + x1];
    }

    if (z3 < Headfax.zdim) {
	Data5zoffset = data[3][y2 * Headfax.xdim + x1];
	Data6zoffset = data[3][y2 * Headfax.xdim + x2];
	Data7zoffset = data[3][y1 * Headfax.xdim + x2];
	Data8zoffset = data[3][y1 * Headfax.xdim + x1];
    }
    else {
	Data5zoffset = 2.0 * data[2][y2 * Headfax.xdim + x1]
	    - data[1][y2 * Headfax.xdim + x1];
	Data6zoffset = 2.0 * data[2][y2 * Headfax.xdim + x2]
	    - data[1][y2 * Headfax.xdim + x2];
	Data7zoffset = 2.0 * data[2][y1 * Headfax.xdim + x2]
	    - data[1][y1 * Headfax.xdim + x2];
	Data8zoffset = 2.0 * data[2][y1 * Headfax.xdim + x1]
	    - data[1][y1 * Headfax.xdim + x1];
    }

    Data_Grad[1][0] = (DATA[1] - Data1xoffset) / 2;
    Data_Grad[1][1] = (Data1yoffset - DATA[3]) / 2;
    Data_Grad[1][2] = (DATA[4] - Data1zoffset) / 2;

    Data_Grad[2][0] = (Data2xoffset - DATA[0]) / 2;
    Data_Grad[2][1] = (Data2yoffset - DATA[2]) / 2;
    Data_Grad[2][2] = (DATA[5] - Data2zoffset) / 2;

    Data_Grad[3][0] = (Data3xoffset - DATA[3]) / 2;
    Data_Grad[3][1] = (DATA[1] - Data3yoffset) / 2;
    Data_Grad[3][2] = (DATA[6] - Data3zoffset) / 2;

    Data_Grad[4][0] = (DATA[2] - Data4xoffset) / 2;
    Data_Grad[4][1] = (DATA[0] - Data4yoffset) / 2;
    Data_Grad[4][2] = (DATA[7] - Data4zoffset) / 2;

    Data_Grad[5][0] = (DATA[5] - Data5xoffset) / 2;
    Data_Grad[5][1] = (Data5yoffset - DATA[7]) / 2;
    Data_Grad[5][2] = (Data5zoffset - DATA[0]) / 2;

    Data_Grad[6][0] = (Data6xoffset - DATA[4]) / 2;
    Data_Grad[6][1] = (Data6yoffset - DATA[6]) / 2;
    Data_Grad[6][2] = (Data6zoffset - DATA[1]) / 2;

    Data_Grad[7][0] = (Data7xoffset - DATA[7]) / 2;
    Data_Grad[7][1] = (DATA[5] - Data7yoffset) / 2;
    Data_Grad[7][2] = (Data7zoffset - DATA[2]) / 2;

    Data_Grad[8][0] = (DATA[6] - Data8xoffset) / 2;
    Data_Grad[8][1] = (DATA[4] - Data8yoffset) / 2;
    Data_Grad[8][2] = (Data8zoffset - DATA[3]) / 2;

    nedges = cell_table[c_ndx].nedges;	/*ASSIGNED A VALUE FROM CELL_TABLE */

    /*  loop for number of edges determined by cell c_ndx, specific edge numbeR
     **  is located in the cell_table[c_ndx].edges[#].  for each edge listed, the
     **  polygon vertices are calculated and stored in temporary array temp_vert[][],
     **  gradients for x,y,z components of these vertices are calculated next as 
     **  a,b,c the length l of this gradient vector is computed and the normalizeD
     **  gradient vector (a/l, b/l, c/l) are stored in the temp_grad[][].
     */
    for (i = 0; i < nedges; i++) {
	crnt_edge = cell_table[c_ndx].edges[i];
	switch (crnt_edge) {
	case 1:
	    delta = (linefax->tvalue[t_ndx] - DATA[0]) / (DATA[1] - DATA[0]);
	    TEMP_VERT[1][0] = delta * 255;
	    TEMP_VERT[1][1] = 255;
	    TEMP_VERT[1][2] = 0;
	    a = 2;
	    b = 1;

	    break;

	case 2:
	    delta = (linefax->tvalue[t_ndx] - DATA[2]) / (DATA[1] - DATA[2]);
	    TEMP_VERT[2][0] = 255;
	    TEMP_VERT[2][1] = delta * 255;
	    TEMP_VERT[2][2] = 0;
	    a = 2;
	    b = 3;

	    break;

	case 3:
	    delta = (linefax->tvalue[t_ndx] - DATA[3]) / (DATA[2] - DATA[3]);
	    TEMP_VERT[3][0] = delta * 255;
	    TEMP_VERT[3][1] = 0;
	    TEMP_VERT[3][2] = 0;
	    a = 3;
	    b = 4;

	    break;

	case 4:
	    delta = (linefax->tvalue[t_ndx] - DATA[3]) / (DATA[0] - DATA[3]);
	    TEMP_VERT[4][0] = 0;
	    TEMP_VERT[4][1] = delta * 255;
	    TEMP_VERT[4][2] = 0;
	    a = 1;
	    b = 4;

	    break;

	case 5:
	    delta = (linefax->tvalue[t_ndx] - DATA[4]) / (DATA[5] - DATA[4]);
	    TEMP_VERT[5][0] = delta * 255;
	    TEMP_VERT[5][1] = 255;
	    TEMP_VERT[5][2] = 255;
	    a = 6;
	    b = 5;

	    break;

	case 6:
	    delta = (linefax->tvalue[t_ndx] - DATA[6]) / (DATA[5] - DATA[6]);
	    TEMP_VERT[6][0] = 255;
	    TEMP_VERT[6][1] = delta * 255;
	    TEMP_VERT[6][2] = 255;
	    a = 6;
	    b = 7;

	    break;

	case 7:
	    delta = (linefax->tvalue[t_ndx] - DATA[7]) / (DATA[6] - DATA[7]);
	    TEMP_VERT[7][0] = delta * 255;
	    TEMP_VERT[7][1] = 0;
	    TEMP_VERT[7][2] = 255;
	    a = 7;
	    b = 8;

	    break;

	case 8:
	    delta = (linefax->tvalue[t_ndx] - DATA[7]) / (DATA[4] - DATA[7]);
	    TEMP_VERT[8][0] = 0;
	    TEMP_VERT[8][1] = delta * 255;
	    TEMP_VERT[8][2] = 255;
	    a = 5;
	    b = 8;

	    break;

	case 9:
	    delta = (linefax->tvalue[t_ndx] - DATA[0]) / (DATA[4] - DATA[0]);
	    TEMP_VERT[9][0] = 0;
	    TEMP_VERT[9][1] = 255;
	    TEMP_VERT[9][2] = delta * 255;
	    a = 5;
	    b = 1;

	    break;

	case 10:
	    delta = (linefax->tvalue[t_ndx] - DATA[1]) / (DATA[5] - DATA[1]);
	    TEMP_VERT[10][0] = 255;
	    TEMP_VERT[10][1] = 255;
	    TEMP_VERT[10][2] = delta * 255;
	    a = 6;
	    b = 2;

	    break;

	case 11:
	    delta = (linefax->tvalue[t_ndx] - DATA[3]) / (DATA[7] - DATA[3]);
	    TEMP_VERT[11][0] = 0;
	    TEMP_VERT[11][1] = 0;
	    TEMP_VERT[11][2] = delta * 255;
	    a = 8;
	    b = 4;

	    break;

	case 12:
	    delta = (linefax->tvalue[t_ndx] - DATA[2]) / (DATA[6] - DATA[2]);
	    TEMP_VERT[12][0] = 255;
	    TEMP_VERT[12][1] = 0;
	    TEMP_VERT[12][2] = delta * 255;

	    a = 7;
	    b = 3;
	    break;
	}
	TEMP_NORM[crnt_edge][0] = delta * (Data_Grad[a][0] - Data_Grad[b][0])
	    + Data_Grad[b][0];
	TEMP_NORM[crnt_edge][1] = delta * (Data_Grad[a][1] - Data_Grad[b][1])
	    + Data_Grad[b][1];
	TEMP_NORM[crnt_edge][2] = delta * (Data_Grad[a][2] - Data_Grad[b][2])
	    + Data_Grad[b][2];

	normalize(TEMP_NORM[crnt_edge]);
    }
}


static void normalize(float n[3])
{
    float l;

    l = sqrt((n[0] * n[0]) + (n[1] * n[1]) + (n[2] * n[2]));
    if (!l)
	l = 1;

    n[0] /= l;
    n[1] /= l;
    n[2] /= l;
}
