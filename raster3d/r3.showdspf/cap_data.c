#include "vizual.h"

/********************************  cap_data  **********************************/

void draw_cap_side(D_spec, Headp, G3header, D_Cap, type)
     struct dspec *D_spec;
     file_info *Headp;
     file_info *G3header;
     struct Cap *D_Cap;
     int type;
{
    int t, y, z;
    int xdim, ydim, zdim;
    int xysize;
    FILE *fp;
    float *DB;
    int old = 0;
    int yloop, zloop;
    int ystart, zstart;
    int xrc, yrc, zrc;
    int offset;

    *(D_Cap->D_buff) = '\0';
    fp = G3header->datainfp;

    xdim = G3header->xdim;
    ydim = G3header->ydim;
    zdim = G3header->zdim;
    xysize = xdim * ydim;

    build_thresh_arrays(D_spec, Headp);

    /*
     **      Old DSPF Cubes are 3 less than the number of data points across
     **      in each dimension.  Thus to make Grid3 match up with
     **      Cubes, we have to throw away the outer points of each side 
     **      Check difference in dimensions to see if we have an old 
     **      or new file.
     */


    if ((xdim - Headp->xdim > 1) ||
	(ydim - Headp->ydim > 1) || (zdim - Headp->zdim > 1))
	old = 1;


    if (old) {
	yloop = ydim - 1;
	ystart = 1;
	zloop = zdim - 1;
	zstart = 1;
	xrc = xdim - 2;
	yrc = ydim - 2;
	zrc = zdim - 2;
	offset = 1;
    }
    else {
	yloop = ydim;
	ystart = 0;
	zloop = zdim;
	zstart = 0;
	xrc = xdim;
	yrc = ydim;
	zrc = zdim;
	offset = 0;
    }

    for (t = 0; t < 6; t++) {

	if (!(type & (1 << t)))
	    continue;

	DB = D_Cap->D_buff;
	D_Cap->side = t;

	switch (t) {
	case 0:		/*xyplane  z = zdim */
	    D_Cap->reverse = 1;
	    D_Cap->minx = D_spec->B[X];
	    D_Cap->miny = D_spec->B[Y];
	    D_Cap->maxx = D_spec->E[X];
	    D_Cap->maxy = D_spec->E[Y];

	    D_Cap->z = D_spec->E[Z];

	    D_Cap->Rows = yrc;
	    D_Cap->Cols = xrc;
	    for (y = ystart; y < yloop; y++) {
		G_fseek(fp, D_offset +
		      (xysize * (D_spec->E[Z]) + y * xdim + offset) *
		      sizeof(float), 0);
		fread(DB, xrc, sizeof(float), fp);
		DB += xrc;
	    }
	    break;
	case 1:		/*xyplane z = 0 */
	    D_Cap->reverse = 0;
	    D_Cap->minx = D_spec->B[X];
	    D_Cap->miny = D_spec->B[Y];
	    D_Cap->maxx = D_spec->E[X];
	    D_Cap->maxy = D_spec->E[Y];

	    D_Cap->z = D_spec->B[Z];

	    D_Cap->Rows = yrc;
	    D_Cap->Cols = xrc;
	    for (y = ystart; y < yloop; y++) {
		G_fseek(fp, D_offset +
		      (xysize * (D_spec->B[Z] + offset) + y * xdim + offset) *
		      sizeof(float), 0);
		fread(DB, xrc, sizeof(float), fp);
		DB += xrc;
	    }
	    break;
	case 2:		/*yzplane x=xdim */
	    D_Cap->reverse = 0;
	    D_Cap->minx = D_spec->B[Y];
	    D_Cap->miny = D_spec->B[Z];
	    D_Cap->maxx = D_spec->E[Y];
	    D_Cap->maxy = D_spec->E[Z];

	    D_Cap->z = D_spec->E[X];
	    D_Cap->Rows = zrc;
	    D_Cap->Cols = yrc;

	    for (z = zstart; z < zloop; z++) {
		for (y = ystart; y < yloop; y++) {
		    G_fseek(fp, D_offset +
			  (xysize * z + xdim * y + (D_spec->E[X] + offset)) *
			  sizeof(float), 0);
		    fread(DB++, sizeof(float), 1, fp);
		}
	    }
	    break;
	case 3:		/*yzplane  x = 0 */
	    D_Cap->reverse = 1;
	    D_Cap->minx = D_spec->B[Y];
	    D_Cap->miny = D_spec->B[Z];
	    D_Cap->maxx = D_spec->E[Y];
	    D_Cap->maxy = D_spec->E[Z];

	    D_Cap->z = D_spec->B[X];

	    D_Cap->Rows = zrc;
	    D_Cap->Cols = yrc;

	    for (z = zstart; z < zloop; z++) {
		for (y = ystart; y < yloop; y++) {
		    G_fseek(fp, D_offset +
			  (xysize * z + xdim * y + (D_spec->B[X] + offset)) *
			  sizeof(float), 0);
		    fread(DB++, sizeof(float), 1, fp);
		}
	    }
	    break;
	case 4:		/*xzplane y = ydim */
	    D_Cap->reverse = 0;
	    D_Cap->minx = D_spec->B[X];
	    D_Cap->miny = D_spec->B[Z];
	    D_Cap->maxx = D_spec->E[X];
	    D_Cap->maxy = D_spec->E[Z];
	    D_Cap->z = D_spec->E[Y];
	    D_Cap->Rows = zrc;
	    D_Cap->Cols = xrc;
	    for (z = zstart; z < zloop; z++) {
		/* fill in the buff one line at a time */
		G_fseek(fp, D_offset +
		      (xysize * z + xdim * (D_spec->E[Y] + offset) + offset) *
		      sizeof(float), 0);
		fread(DB, sizeof(float), xrc, fp);
		DB += xrc;
	    }
	    break;
	case 5:		/*xzplane y = 0 */
	    D_Cap->reverse = 1;
	    D_Cap->minx = D_spec->B[X];
	    D_Cap->miny = D_spec->B[Z];
	    D_Cap->maxx = D_spec->E[X];
	    D_Cap->maxy = D_spec->E[Z];
	    D_Cap->z = D_spec->B[Y];
	    D_Cap->Rows = zrc;
	    D_Cap->Cols = xrc;
	    for (z = zstart; z < zloop; z++) {
		/* fill in the buff one line at a time */
		G_fseek(fp, D_offset +
		      (xysize * z + xdim * (D_spec->B[Y] + offset) + offset) *
		      sizeof(float), 0);
		fread(DB, sizeof(float), xrc, fp);

		DB += xrc;
	    }
	    break;

	}
	draw_cap(Headp, D_spec, D_Cap);
    }
}
