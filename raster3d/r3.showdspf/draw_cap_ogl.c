#include "vizual.h"


/* LOCALLY Global structures */
static file_info *Headp;
static struct dspec *B_spec;
static struct Cap *Cap;
static struct poly_info Polys[3 * (MAXTHRESH + 1)];
static int x, y;

static double Xlinterp();

#define ANTICLOCKWISE 0
#define CLOCKWISE     1

/**************************************  get_min_max  *************************/
/* get min and max values from the original data */
static get_min_max(minmax, size, array)
     double *minmax;
     int size;
     double array[4];
{
    register int i;
    double min, max;

    min = max = *array;

    for (i = 0; i < size; i++) {
	if (array[i] > max)
	    max = array[i];
	else if (array[i] < min)
	    min = array[i];
    }

    minmax[0] = min;
    minmax[1] = max;
}

/*************************************** side_to_xy  **************************/
static side_to_xy(xyray, pre, zz, side)
     double *xyray;
     struct poly_info *pre;
     double zz;
     int side;
{
    double x1, x2;
    double y1, y2;
    int side2;
    double interp;

    side2 = next_higher(side, pre->vnum);

    x1 = pre->verts[side << 1];
    y1 = pre->verts[(side << 1) + 1];
    x2 = pre->verts[side2 << 1];
    y2 = pre->verts[(side2 << 1) + 1];
    /*
       debugf("%lf x1  %lf y1\n",x1,y1);
       debugf("%lf x2  %lf y2\n",x2,y2);
     */

    interp = Xlinterp(pre->data[side], zz, pre->data[side2]);

    if (x1 != x2)
	xyray[0] = x1 + (interp * (x2 - x1));
    else
	xyray[0] = x1;
    if (y1 != y2)
	xyray[1] = y1 + (interp * (y2 - y1));
    else
	xyray[1] = y2;

}

/**********************************  split_poly   *****************************/
/* recursively subdivides polygons until no more threshold divide
 ** them then draws the polygon and falls thru to left over polygon
 ** code */

void split_poly(ta, pnum, t)
     int pnum;			/* index to current polygon */
     int ta;			/* current threshold array */
     int t;			/* current threshold index */
{
    double minmax[2];
    double zz;			/* current threshold */
    int num;
    int hits;
    int xing[4];		/* a list of the sides with hits */
    int tnum;
    int i;
    int new_pnum;
    int side;
    static int first;
    int n_verts;
    cmndln_info *tp;
    int drawn;

    num = 0;
    hits = 0;
    drawn = 0;

    tp = &(B_spec->threshes[ta]);

    if (t >= tp->nthres)
	return;

    zz = (double)tp->tvalue[t];

    if (t) {			/* don't draw below 1st threshold */
	if (B_spec->in_out == INSIDE)
	    tnum = t + B_spec->low;
	else if (ta == 0)
	    /* tnum == t; *//* fix ? MN 2001 */
	    tnum = t;
	else
	    tnum = t + B_spec->hi;
    }

    /* calc min/max for each new poly */
    get_min_max(minmax, Polys[pnum].vnum, Polys[pnum].data);

    if (zz > (double)minmax[1]) {	/* if entirely contained */
	if (t) {
	    draw_cappolys(Headp, B_spec, Cap, &Polys[pnum], x, y, 1, tnum);
	}

	/*and we are done */
	return;
    }
    /* thresh in square */
    else if (WITHIN((double)minmax[0], zz, (double)minmax[1])) {

	/* this code from contour program:  determine number of crossings (hits)
	 ** if non-zero and even, then have 1 or 2 valid contours in cell
	 */
	for (i = 0; i < Polys[pnum].vnum; i++) {
	    if ((Polys[pnum].data[i] <= zz &&
		 zz < Polys[pnum].data[(i + 1) % Polys[pnum].vnum]) ||
		(Polys[pnum].data[i] > zz &&
		 zz >= Polys[pnum].data[(i + 1) % Polys[pnum].vnum])) {
		hits |= (1 << i);	/* record which lines were crossed */
		xing[num] = i;
		num++;
	    }
	}


	if (!num || num & 0x01) {	/* zero or odd  no contour in cell */
	    /* just go on to next thresh */
	    split_poly(ta, pnum, t + 1);
	}


	/* else split into two polygons */
	else {
	    /* NUMBER OF XINGS = 2 */
	    /* split the polygon and the reiterate */
	    side = xing[0];
	    /* first is set to true if the first split polygon is to be drawn */
	    first = (zz > Polys[pnum].data[side]) ? 0 : 1;

	    new_pnum = pnum + 1;
	    n_verts = 0;

	    /* 1st vert will be the location where the first xing occurs */
	    /* not only are the vertices stored in the poly_info variable
	     ** but also the data values at these vertices must be stored */
	    Polys[new_pnum].data[n_verts] = zz;
	    side_to_xy(&(Polys[new_pnum].verts[n_verts << 1]), &(Polys[pnum]),
		       zz, side);
	    n_verts++;
	    side = next_higher(side, Polys[pnum].vnum);

	    while (side != xing[1]) {
		Polys[new_pnum].data[n_verts] = Polys[pnum].data[side];
		Polys[new_pnum].verts[n_verts << 1] =
		    Polys[pnum].verts[side << 1];
		Polys[new_pnum].verts[(n_verts << 1) + 1] =
		    Polys[pnum].verts[(side << 1) + 1];
		n_verts++;

		side = next_higher(side, Polys[pnum].vnum);
	    }
	    Polys[new_pnum].data[n_verts] = Polys[pnum].data[side];
	    Polys[new_pnum].verts[n_verts << 1] =
		Polys[pnum].verts[side << 1];
	    Polys[new_pnum].verts[(n_verts << 1) + 1] =
		Polys[pnum].verts[(side << 1) + 1];
	    n_verts++;

	    Polys[new_pnum].data[n_verts] = zz;
	    side_to_xy(&(Polys[new_pnum].verts[n_verts << 1]), &(Polys[pnum]),
		       zz, side);
	    n_verts++;

	    Polys[new_pnum].vnum = n_verts;
	    Polys[new_pnum].data[n_verts] = Polys[new_pnum].data[0];

	    if (first) {
		if (t)
		    draw_cappolys(Headp, B_spec, Cap, &Polys[new_pnum], x, y,
				  1, tnum);
		drawn = 1;
	    }
	    else {
		split_poly(ta, new_pnum, t + 1);
	    }

/*============================================================================*/
	    /* NOW DO THE SECOND HALF OF THE SPLIT */
	    side = xing[1];
	    n_verts = 0;

	    /* 1st vert will be the location that xing occurs */
	    Polys[new_pnum].data[n_verts] = zz;
	    side_to_xy(&Polys[new_pnum].verts[n_verts << 1], &(Polys[pnum]),
		       zz, side);
	    n_verts++;

	    side = next_higher(side, Polys[pnum].vnum);
	    while (side != xing[0]) {
		Polys[new_pnum].data[n_verts] = Polys[pnum].data[side];
		Polys[new_pnum].verts[n_verts << 1] =
		    Polys[pnum].verts[side << 1];
		Polys[new_pnum].verts[(n_verts << 1) + 1] =
		    Polys[pnum].verts[(side << 1) + 1];
		n_verts++;

		side = next_higher(side, Polys[pnum].vnum);
	    }
	    Polys[new_pnum].data[n_verts] = Polys[pnum].data[side];
	    Polys[new_pnum].verts[n_verts << 1] =
		Polys[pnum].verts[side << 1];
	    Polys[new_pnum].verts[(n_verts << 1) + 1] =
		Polys[pnum].verts[(side << 1) + 1];
	    n_verts++;

	    Polys[new_pnum].data[n_verts] = zz;
	    side_to_xy(&Polys[new_pnum].verts[n_verts << 1], &(Polys[pnum]),
		       zz, side);
	    n_verts++;

	    Polys[new_pnum].vnum = n_verts;
	    Polys[new_pnum].data[n_verts] = Polys[new_pnum].data[0];

	    if (!first) {
		if (t) {
		    draw_cappolys(Headp, B_spec, Cap, &Polys[new_pnum], x, y,
				  1, tnum);
		}
		/* mark a flag */
		drawn = 1;
	    }
	    else {
		split_poly(ta, new_pnum, t + 1);
	    }
	}
    }				/*end WITHIN code */


    /* if no hits then ready to fall thru to left over node
       code which will be the tail end of the split_poly routine */

    if ((t < tp->nthres) && !drawn) {
	split_poly(ta, pnum, t + 1);
    }
    return;
}


/**************************************  Xlinterp  ****************************/
/* rescale zz to a number between 0. and 1. */
static double Xlinterp(zmin, zz, zmax)
     double zmin, zz, zmax;
{
    if (zmin == zmax)		/* div by zero.  should never get here */
	return zmin;
    return (zz - zmin) / (zmax - zmin);
}

/*******************************   next_higher  *******************************/
/* given sides 0,1,2,3, return number of next side in clockwise order */
int next_higher(side, verts)
     int side, verts;
{
    return ((side + 1) % verts);
}

int draw_cap(XHeadp, XB_spec, XCap)
     file_info *XHeadp;
     struct dspec *XB_spec;
     struct Cap *XCap;
{
    int row, xdim;
    int ta;
    cmndln_info *tp;
    int org_tnum;

    /* if flat shading */
    Headp = XHeadp;
    if (Headp->linefax.litmodel == 1)
	glDisable(GL_COLOR_MATERIAL);
    else {
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
    }
    B_spec = XB_spec;
    Cap = XCap;

    xdim = Cap->Cols;
    for (y = Cap->miny; y < Cap->maxy; y++) {	/* for each cell */
	row = y * Cap->Cols;	/*shouldn't this be Rows? JCM   NO. DPG */
	for (x = Cap->minx; x < Cap->maxx; x++) {
	    Polys[0].data[0] = Polys[0].data[4] =
		(double)Cap->D_buff[row + xdim + x];
	    /*BL*/ Polys[0].data[1] = (double)Cap->D_buff[row + xdim + x + 1];	/* BR */
	    Polys[0].data[2] = (double)Cap->D_buff[row + x + 1];	/* TR */
	    Polys[0].data[3] = (double)Cap->D_buff[row + x];	/* TL */

	    /* for each threshold array */
	    for (ta = 0; ta < 2 && (tp = &(B_spec->threshes[ta]))->nthres;
		 ta++) {

		{		/* build initial poly */
		    Polys[0].verts[0] = 0.0;
		    Polys[0].verts[1] = 1.0;
		    Polys[0].verts[2] = 1.0;
		    Polys[0].verts[3] = 1.0;
		    Polys[0].verts[4] = 1.0;
		    Polys[0].verts[5] = 0.0;
		    Polys[0].verts[6] = 0.0;
		    Polys[0].verts[7] = 0.0;
		    Polys[0].verts[8] = 0.0;
		    Polys[0].verts[9] = 1.0;
		    Polys[0].vnum = 4;

		}
		/* First time */
		if (B_spec->in_out == INSIDE)
		    org_tnum = B_spec->low;
		else
		    org_tnum = ta == 0 ? 0 : B_spec->hi;

		split_poly(ta, 0, 0);
	    }
	}
    }
    glDisable(GL_COLOR_MATERIAL);
}
