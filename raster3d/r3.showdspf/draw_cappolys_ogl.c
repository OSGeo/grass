#include <stdio.h>
#include "vizual.h"


#define COLOR3

int draw_cappolys(Headp, D_spec, D_Cap, poly, x, y, direction, index)
     file_info *Headp;
     struct dspec *D_spec;
     struct Cap *D_Cap;
     struct poly_info *poly;
     int x;
     int y;
     int direction;
     int index;
{
    int yloc, xloc;
    int t;
    double *vertices;
    double tmpvt[20][3];	/* these are going to be sent to v3d */
    float norm[3];
    int nverts;
    short color[3];

    /* RECONSTRUCT ACTUAL LOCATION OF POLYGONS */
    int start, stop;

    vertices = poly->verts;
    nverts = poly->vnum;


    if (!direction) {
	direction = -1;		/* was 0,1,  now  -1,1 */
	start = nverts - 1;
	stop = -1;
    }
    else {
	start = 0;
	stop = nverts;
    }
    xloc = x;
    yloc = y;

    switch (D_Cap->side) {
    case 0:
    case 1:
	norm[2] = D_Cap->side ? -1.0 : 1.0;
	norm[0] = norm[1] = 0.0;
	for (t = start; t != stop; t += direction) {
	    tmpvt[t][0] = (vertices[t << 1] + xloc) * D_spec->xscale;
	    tmpvt[t][1] = (vertices[(t << 1) + 1] + yloc) * D_spec->yscale;
	    tmpvt[t][2] = (D_Cap->z) * D_spec->zscale;
	}
	break;
    case 2:
    case 3:
	norm[0] = D_Cap->side == 2 ? 1.0 : -1.0;
	norm[1] = norm[2] = 0.0;
	for (t = start; t != stop; t += direction) {
	    tmpvt[t][0] = (D_Cap->z) * D_spec->xscale;
	    tmpvt[t][1] = (vertices[t << 1] + xloc) * D_spec->yscale;
	    tmpvt[t][2] = (vertices[(t << 1) + 1] + yloc) * D_spec->zscale;
	}
	break;
    case 4:
    case 5:
	norm[1] = D_Cap->side == 4 ? 1.0 : -1.0;
	norm[0] = norm[2] = 0.0;
	for (t = start; t != stop; t += direction) {
	    tmpvt[t][0] = (vertices[t << 1] + xloc) * D_spec->xscale;
	    tmpvt[t][1] = (D_Cap->z) * D_spec->yscale;
	    tmpvt[t][2] = (vertices[(t << 1) + 1] + yloc) * D_spec->zscale;
	}
	break;
    }

    /*now ready to draw the polygons, not going to worry about reversing
       the polygon ordering at this time */

    get_cat_color(Headp->linefax.tvalue[index], D_spec->ctable, color);

    glColor3sv(color);

    glBegin(GL_POLYGON);
    if (Headp->linefax.litmodel != 1)
	/* if not flat shade */
	glNormal3fv(norm);
#ifdef CLOCKWISE
    for (t = 0; t < nverts; t++) {
	glVertex3dv(tmpvt[t]);
    }
#else
    for (t = nverts - 1; t >= 0; t--) {
	glVertex3dv(tmpvt[t]);
    }
#endif
    glEnd();
    return;
}
