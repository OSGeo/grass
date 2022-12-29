#include <grass/raster.h>
#include <grass/glocale.h>
#include "pv.h"

/*
 * Use centers of GRASS CELLS as vertexes for grid. 
 * Currently, grid space is "unitized" so that the 
 * largest dimension of the current region in GRASS == 1.0 
 */

void vrml_put_grid(FILE * vout,
		   struct Cell_head *w,
		   int elevfd, int colorfd,
		   struct Colors *colr,
		   int color_ok, int rows, int cols, int shh)
{
    char str[512];

    FCELL *tf;
    FCELL *dbuf;

    dbuf = (FCELL *) G_malloc(cols * sizeof(FCELL));

#ifdef VRML2
    fprintf(vout, "grid\n");
    vrml_putline(0, vout, "grid");
#else
    vrml_putline(0, vout, "Separator");
    vrml_putline(1, vout, OCB);


    /* write grid vertices */
    {
	double coordx, coordy, coordz;
	int row, col;

	if (!shh)
	    G_message(_("Writing vertices..."));

	vrml_putline(0, vout, "Coordinate3");
	vrml_putline(1, vout, OCB);
	vrml_putline(0, vout, "point");
	vrml_putline(1, vout, OSB);

	for (row = 0; row < rows; row++) {
	    tf = dbuf;

	    if (!shh)
		G_percent(row, rows - 1, 10);

	    Rast_get_f_row(elevfd, tf, row);
	    coordz = Rast_row_to_northing((double)row, w);
	    do_coordcnv(&coordz, 'z');

	    /* write a row */
	    for (col = 0; col < cols; col++) {
		coordx = Rast_col_to_easting((double)col, w);
		do_coordcnv(&coordx, 'x');

		/* HACK: no nulls in vrml grid */
		if (Rast_is_f_null_value(tf))
		    *tf = 0.0;
		coordy = *tf;
		do_coordcnv(&coordy, 'y');
		sprintf(str, "%f %f %f,", coordx, coordy, coordz);
		vrml_putline(0, vout, str);
		tf++;
	    }
	    /* end a row */

	}
	vrml_putline(-1, vout, CSB);	/* end point */
	vrml_putline(-1, vout, CCB);	/* end Coordinate3 */
    }

    if (color_ok)
	/* write material color */
    {
	int row, col;
	unsigned char *red, *green, *blue, *set;

	if (!shh)
	    G_message(_("Writing color file..."));

	vrml_putline(0, vout, "Material");
	vrml_putline(1, vout, OCB);
	vrml_putline(0, vout, "diffuseColor");
	vrml_putline(1, vout, OSB);

	/* allocate buffers */
	red = G_malloc(cols);
	green = G_malloc(cols);
	blue = G_malloc(cols);
	set = G_malloc(cols);

	tf = dbuf;
	for (row = 0; row < rows; row++) {

	    if (!shh)
		G_percent(row, rows - 1, 5);

	    Rast_get_f_row(colorfd, tf, row);
	    Rast_lookup_f_colors(tf, red, green, blue, set, cols, colr);

	    for (col = 0; col < cols; col++) {
		sprintf(str, "%.3f %.3f %.3f,",
			red[col] / 255., green[col] / 255., blue[col] / 255.);
		vrml_putline(0, vout, str);
	    }
	}

	vrml_putline(-1, vout, CSB);	/* end diffuseColor */
	vrml_putline(-1, vout, CCB);	/* end Material */

	vrml_putline(0, vout, "MaterialBinding");
	vrml_putline(1, vout, OCB);
	vrml_putline(0, vout, "value PER_VERTEX_INDEXED");
	vrml_putline(-1, vout, CCB);	/* end MaterialBinding */

	G_free(red);
	G_free(green);
	G_free(blue);
	G_free(set);
    }

    /* write face set indices */
    {
	int row, col, c1, c2;

	vrml_putline(0, vout, "IndexedFaceSet");
	vrml_putline(1, vout, OCB);
	vrml_putline(0, vout, "coordIndex");
	vrml_putline(1, vout, OSB);

	/* write indexes */
	for (row = 0; row < rows - 1; row++) {
	    for (col = 0; col < cols - 1; col++) {
		c1 = row * cols + col;
		c2 = c1 + cols + 1;
		sprintf(str, "%d, %d, %d, -1, %d, %d, %d, -1,",
			c1, c1 + cols, c1 + 1, c2, c2 - cols, c2 - 1);
		vrml_putline(0, vout, str);
	    }
	}

	vrml_putline(-1, vout, CSB);	/* end coordIndex */
	vrml_putline(-1, vout, CCB);	/* end IndexedFaceSet */
    }


    vrml_putline(-1, vout, CCB);	/* end Separator */

#endif


    /*
       G_free(ibuf);
     */
    G_free(dbuf);

}
