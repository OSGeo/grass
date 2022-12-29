#include <grass/gis.h>
#include "vizual.h"
/*
 ** the corner of the cube 
 **
 **
 */
/* this subroutine draws polygons from dspf file using flat shading */

static float ZNexag = 1.0;

void fill_data_cube();
void get_vert_color();

void set_ZNexag(exag)
     float exag;
{
    ZNexag = exag;
}

void get_ZNexag(exag)
     float *exag;
{
    *exag = ZNexag;
}

void fdraw_polys(D_spec)
     struct dspec *D_spec;	/*structure containing interactive input */
{
    int x, y, z;
    int t, p;			/* LOOP COUNTER */
    double xadd, yadd, zadd;
    poly_info *Polyfax;		/* local pointer */
    float tmp_vect[3];
    Cube_data Cube;		/*structure containing poly info */
    cube_info *cubefax;		/* local pointer */
    int curr;
    short color[3];

    cubefax = Cube.data;
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    t = D_spec->Thresh;
    for (z = 0; z < Headfax.zdim; z++) {
	zadd = z * Headfax.ydim;
	for (y = 0; y < Headfax.ydim; y++) {
	    yadd = y * D_spec->yscale;
	    for (x = 0; x < Headfax.xdim; x++) {
		if (!read_cube(&Cube, &Headfax))
		    continue;
		if (!((x > (D_spec->B[X] - 1)) && (x < D_spec->E[X]) &&
		      (y > (D_spec->B[Y] - 1)) && (y < D_spec->E[Y]) &&
		      (z > (D_spec->B[Z] - 1)) && (z < D_spec->E[Z])))
		    continue;


		xadd = x * D_spec->xscale;

		for (t = 0; t < Cube.n_thresh; t++) {
		    if (cubefax[t].t_ndx == D_spec->Thresh) {
			curr = D_spec->Thresh;
			get_cat_color(Headfax.linefax.tvalue[curr],
				      D_spec->ctable, color);
			glColor3sv(color);

			for (p = 0; p < cubefax[t].npoly; p++) {
			    /* center data */
			    Polyfax = &(cubefax[t].poly[p]);

			    glBegin(GL_POLYGON);

			    tmp_vect[0] =
				X_sign * G_sign * (Polyfax->n1[0] / 127. -
						   1.);
			    tmp_vect[1] =
				X_sign * G_sign * (Polyfax->n1[1] / 127. -
						   1.);
			    tmp_vect[2] =
				X_sign * G_sign * (Polyfax->n1[2] / 127. -
						   1.);
			    glNormal3fv(tmp_vect);
			    tmp_vect[0] =
				Polyfax->v1[0] / 255. * D_spec->xscale + xadd;
			    tmp_vect[1] =
				Polyfax->v1[1] / 255. * D_spec->yscale + yadd;
			    tmp_vect[2] =
				Polyfax->v1[2] / 255. * D_spec->zscale + zadd;
			    glVertex3fv(tmp_vect);


			    tmp_vect[0] =
				Polyfax->v2[0] / 255. * D_spec->xscale + xadd;
			    tmp_vect[1] =
				Polyfax->v2[1] / 255. * D_spec->yscale + yadd;
			    tmp_vect[2] =
				Polyfax->v2[2] / 255. * D_spec->zscale + zadd;
			    glVertex3fv(tmp_vect);

			    tmp_vect[0] =
				Polyfax->v3[0] / 255. * D_spec->xscale + xadd;
			    tmp_vect[1] =
				Polyfax->v3[1] / 255. * D_spec->yscale + yadd;
			    tmp_vect[2] =
				Polyfax->v3[2] / 255. * D_spec->zscale + zadd;
			    glVertex3fv(tmp_vect);

			    glEnd();
			}

			break;	/* dont bother w/ other thresholds then */
		    }
		}
	    }
	}
    }
    glDisable(GL_COLOR_MATERIAL);

}

void normalize(v)
     float v[];
{
    float len;

    len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
}

/******************************** gdraw_polys *********************************/
/* this subroutine draws polygons from a dspf file using 3 different normals */


void gdraw_polys(D_spec)
     struct dspec *D_spec;
{
    int x, y, z;
    int t, p;			/* LOOP COUNTER */
    float xadd, yadd, zadd;
    poly_info *Polyfax;		/* local pointer */
    cube_info *cubefax;		/* local pointer */

    int curr;
    float xres, yres, zres, norm[3];
    Cube_data Cube;		/*structure containing poly info */
    file_info chead;
    int color_on = 0;
    int xdim = 0, ydim = 0;
    float *slice[2];
    float *tmp;
    int i;
    int level, row[2], col[2];
    short color[3];
    short data[8][3];

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    /* a color data file has been requested */
    if (D_spec->cfile != NULL) {
	chead.datainfp = D_spec->cfile;
	/* TO CHANGE
	   if (r3read_header (&chead) < 0)
	   fprintf (stderr, "Can't read color file\n");
	   else
	   color_on = 1;
	 */
    }

    if (color_on) {
	xdim = chead.xdim;
	ydim = chead.ydim;
	for (i = 0; i < 2; i++) {
	    if ((slice[i] =
		 (float *)G_malloc(sizeof(float) * xdim * ydim)) == NULL) {
		fprintf(stderr, "error in allocating memory\n");
		fprintf(stderr, "unable to use colortable\n");
		xdim = ydim = 0;
		color_on = 0;
		break;
	    }
	}
    }
    curr = D_spec->Thresh;
    if (color_on) {
	level = get_level(&Headfax, &chead, 0);
	row[1] = get_row(&Headfax, &chead, 0);
	col[1] = get_col(&Headfax, &chead, 0);
	if (level < 0 || row[1] < 0 || col[1] < 0) {
	    fprintf(stderr, "Bounds of colorfile do not match data file\n");
	    color_on = 0;
	}
	else
	    r3read_level(&chead, slice[1], level);	/*read in data */
    }
    if (!color_on) {
	get_cat_color(Headfax.linefax.tvalue[curr], D_spec->ctable, color);
	glColor3sv(color);
	glColor3ub(color[0], color[1], color[2]);
    }

    xres = D_spec->xscale;
    yres = D_spec->yscale;
    zres = D_spec->zscale * ZNexag;

    cubefax = Cube.data;



    for (z = 0; z < Headfax.zdim; z++) {
	zadd = z * D_spec->zscale;
	if (color_on) {
	    if (0 > (level = get_level(&Headfax, &chead, z + 1))) {
		fprintf(stderr, "LEVEL out of bounds for z = %d\n", z);
		/*
		   color_on = 0;
		   glColor3ub(0xff,0xff,0xff);
		 */
	    }
	    else {
		tmp = slice[0];
		slice[0] = slice[1];
		slice[1] = tmp;
		r3read_level(&chead, slice[1], level);	/*read in data */
	    }
	}
	for (y = 0; y < Headfax.ydim; y++) {
	    yadd = y * D_spec->yscale;
	    if (color_on) {
		if (y)
		    row[0] = row[1];
		else
		    row[0] = get_row(&Headfax, &chead, 0);
		if (0 > (row[1] = get_row(&Headfax, &chead, y + 1))) {
		    row[1] = row[0];
		    fprintf(stderr, " ROW out of bounds for y = %d\n", y);
		    /*
		       color_on = 0;
		       glColor3ub(0xff,0xff,0xff);
		     */
		}
	    }
	    for (x = 0; x < Headfax.xdim; x++) {
		if (color_on) {
		    if (x)
			col[0] = col[1];
		    else
			col[1] = get_col(&Headfax, &chead, 0);
		    if (0 > (col[1] = get_col(&Headfax, &chead, x + 1))) {
			col[1] = col[0];
			fprintf(stderr, " COL out of bounds for x = %d\n", x);
			/*
			   color_on = 0;
			   glColor3ub(0xff,0xff,0xff);
			 */
		    }
		}
		if (!read_cube(&Cube, &Headfax))
		    continue;

		if (!((x > (D_spec->B[X] - 1)) && (x < D_spec->E[X]) &&
		      (y > (D_spec->B[Y] - 1)) && (y < D_spec->E[Y]) &&
		      (z > (D_spec->B[Z] - 1)) && (z < D_spec->E[Z])))
		    continue;

		xadd = x * D_spec->xscale;

		for (t = 0; t < Cube.n_thresh; t++) {
		    if (cubefax[t].t_ndx == D_spec->Thresh) {

			if (D_spec->in_out == INSIDE) {
			    if (D_spec->Thresh == D_spec->low) {
				G_sign = -1;
			    }
			    else {
				G_sign = 1;
			    }
			}
			else {	/* outside */

			    if (D_spec->Thresh == D_spec->low) {
				G_sign = 1;
			    }
			    else {
				G_sign = -1;
			    }
			    /* TODO.  What happens for outside, w/ Thresh == max thresh? */
			}
			if (color_on && cubefax[t].npoly)
			    fill_data_cube(data, slice, row, col,
					   D_spec->ctable, xdim);


			for (p = 0; p < cubefax[t].npoly; p++) {
			    /* center data */
			    Polyfax = &(cubefax[t].poly[p]);

			    {
				glBegin(GL_POLYGON);

				Polyfax->v1[0] = Polyfax->v1[0] / 255.0;
				Polyfax->v1[1] = Polyfax->v1[1] / 255.0;
				Polyfax->v1[2] = Polyfax->v1[2] / 255.0;

				if (color_on) {
				    get_vert_color
					(data, Polyfax->v1, D_spec->ctable,
					 color);
				    glColor3sv(color);
				}

				norm[0] =
				    (X_sign * G_sign *
				     (Polyfax->n1[0] / 127. - 1.)) / xres;
				norm[1] =
				    (X_sign * G_sign *
				     (Polyfax->n1[1] / 127. - 1.)) / yres;
				norm[2] =
				    (X_sign * G_sign *
				     (Polyfax->n1[2] / 127. - 1.)) / zres;
				normalize(norm);
				glNormal3fv(norm);

				Polyfax->v1[0] =
				    Polyfax->v1[0] * D_spec->xscale + xadd;
				Polyfax->v1[1] =
				    Polyfax->v1[1] * D_spec->yscale + yadd;
				Polyfax->v1[2] =
				    Polyfax->v1[2] * D_spec->zscale + zadd;
				glVertex3fv(Polyfax->v1);
				Polyfax->v2[0] = Polyfax->v2[0] / 255.0;
				Polyfax->v2[1] = Polyfax->v2[1] / 255.0;
				Polyfax->v2[2] = Polyfax->v2[2] / 255.0;

				if (color_on) {
				    get_vert_color
					(D_spec, data, Polyfax->v2,
					 D_spec->ctable, color);
				    glColor3sv(color);
				}

				norm[0] =
				    (X_sign * G_sign *
				     (Polyfax->n2[0] / 127. - 1.)) / xres;
				norm[1] =
				    (X_sign * G_sign *
				     (Polyfax->n2[1] / 127. - 1.)) / yres;
				norm[2] =
				    (X_sign * G_sign *
				     (Polyfax->n2[2] / 127. - 1.)) / zres;
				normalize(norm);
				glNormal3fv(norm);

				Polyfax->v2[0] =
				    Polyfax->v2[0] * D_spec->xscale + xadd;
				Polyfax->v2[1] =
				    Polyfax->v2[1] * D_spec->yscale + yadd;
				Polyfax->v2[2] =
				    Polyfax->v2[2] * D_spec->zscale + zadd;
				glVertex3fv(Polyfax->v2);


				Polyfax->v3[1] = Polyfax->v3[1] / 255.0;
				Polyfax->v3[2] = Polyfax->v3[2] / 255.0;
				Polyfax->v3[0] = Polyfax->v3[0] / 255.0;

				if (color_on) {
				    get_vert_color
					(D_spec, data, Polyfax->v3,
					 D_spec->ctable, color);
				    glColor3sv(color);
				}
				norm[0] =
				    (X_sign * G_sign *
				     (Polyfax->n3[0] / 127. - 1.)) / xres;
				norm[1] =
				    (X_sign * G_sign *
				     (Polyfax->n3[1] / 127. - 1.)) / yres;
				norm[2] =
				    (X_sign * G_sign *
				     (Polyfax->n3[2] / 127. - 1.)) / zres;
				normalize(norm);
				glNormal3fv(norm);

				Polyfax->v3[0] =
				    Polyfax->v3[0] * D_spec->xscale + xadd;
				Polyfax->v3[1] =
				    Polyfax->v3[1] * D_spec->yscale + yadd;
				Polyfax->v3[2] =
				    Polyfax->v3[2] * D_spec->zscale + zadd;
				glVertex3fv(Polyfax->v3);



				glEnd();
			    }
			}
			break;
		    }
		}
	    }
	}
    }
    if (color_on) {
	G_free(slice[0]);
	G_free(slice[1]);
    }
    glDisable(GL_COLOR_MATERIAL);
}

int get_level(head, chead, z)
     file_info *head, *chead;
     int z;
{
    int level;

    level = (z * head->tb_res + head->bottom - chead->bottom)
	/ chead->tb_res;
    if (level < 0 || level >= chead->zdim)
	level = -1;
    return (level);
}

int get_row(head, chead, y)
     file_info *head, *chead;
     int y;
{
    int row;

    row = (y * head->ns_res + head->south - chead->south)
	/ chead->ns_res;
    if (row < 0 || row >= chead->ydim)
	row = -1;
    return (row);
}

int get_col(head, chead, x)
     file_info *head, *chead;
     int x;
{
    int col;

    col = (x * head->ew_res + head->west - chead->west)
	/ chead->ew_res;
    if (col < 0 || col >= chead->xdim)
	col = -1;
    return (col);
}

void fill_data_cube(data, slice, row, col, ctable, xdim)
     short data[8][3];
     float *slice[2];
     int row[2], col[2];
     struct color_entry *ctable;
     int xdim;
{
    int x, y, z;
    int i = 0;
    float cat, *tmp;

    for (x = 0; x < 2; x++)
	for (y = 0; y < 2; y++)
	    for (z = 0; z < 2; z++) {
		tmp = slice[z];
		cat = *(tmp + row[y] * xdim + col[x]);
		get_cat_color(cat, ctable, data[i]);
		i++;
	    }
}

void get_vert_color(data, vert, ctable, color)
     short data[8][3];
     float vert[3];
     struct color_entry *ctable;
     short color[3];
{
    short x[4][3], y[2][3];
    float dx, dy, dz;
    int i, j;

    dx = vert[0];
    dy = vert[1];
    dz = vert[2];

    if (dx == 0.0)
	for (i = 0; i < 4; i++)
	    for (j = 0; j < 3; j++)
		x[i][j] = data[i][j];
    else if (dx == 1.0)
	for (i = 0; i < 4; i++)
	    for (j = 0; j < 3; j++)
		x[i][j] = data[i + 4][j];
    else
	for (i = 0; i < 4; i++)
	    for (j = 0; j < 3; j++)
		x[i][j] = (data[i][j] * (1 - dx) + data[i + 4][j] * dx);

    if (dy == 0.0)
	for (j = 0; j < 3; j++) {
	    y[0][j] = x[0][j];
	    y[1][j] = x[1][j];
	}
    if (dy == 1.0)
	for (j = 0; j < 3; j++) {
	    y[0][j] = x[2][j];
	    y[1][j] = x[3][j];
	}
    else
	for (i = 0; i < 2; i++)
	    for (j = 0; j < 3; j++) {
		y[i][j] = (x[i][j] * (1 - dy) + x[i + 2][j] * dy);
	    }

    if (dz == 0.0)
	for (j = 0; j < 3; j++)
	    color[j] = y[0][j];
    else if (dz == 1.0)
	for (j = 0; j < 3; j++)
	    color[j] = y[1][j];
    else
	for (j = 0; j < 3; j++)
	    color[j] = (y[0][j] * (1 - dz) + y[1][j] * dz);
}

void print_color_table(ctable)
     struct color_entry ctable[];
{
    int i;

    for (i = 0; ctable[i].color[0] > 0; i++) {
	fprintf(stderr, "%f:%hd:%hd:%hd",
		ctable[i].data,
		ctable[i].color[0], ctable[i].color[1], ctable[i].color[2]);

    }
}
