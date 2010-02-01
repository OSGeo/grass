/*
 ************************************************************
 * MODULE: r.le.pixel/driver.c                              *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze pixel-scale landscape properties     *
 *         driver.c opens the input and output files and    *
 *         then calls the moving window, unit, and whole    *
 *         map drivers                                      *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/


#include <grass/gis.h>
#include "pixel.h"
#include <grass/config.h>
#include <grass/raster.h>


int finput, g_scale = 1, g_unit = 1;
extern struct CHOICE *choice;
char cmdbuf[100];
RASTER_MAP_TYPE data_type;

/*
   Variables:
   GLOBAL:
   finput =      the raster map to be analyzed
   g_scale =     the number of sampling scales
   g_unit =      the number of sampling units
   data_type =   the type of raster map: integer, floating point, or double
 */


					/* RUN R.LE.PIXEL IN FOREGROUND */

void texture_fore()
{
    FILE *fp0, *fp1, *fp2, *fp3, *fp4, *fp5;

    fprintf(stdout, "\nR.LE.PIXEL IS WORKING....;\n\n");

    /* check for input raster map */

    if (0 > (finput = Rast_open_old(choice->fn, G_mapset()))) {
	fprintf(stdout, "\n");
	fprintf(stdout,
		"   ********************************************************\n");
	fprintf(stdout,
		"    The raster map you specified with the 'map=' parameter \n");
	fprintf(stdout,
		"    was not found in your mapset.                          \n");
	fprintf(stdout,
		"   ********************************************************\n");
	exit(1);
    }

    /* determine whether the raster map is integer
       (CELL_TYPE),floating point (FCELL_TYPE), or
       double (DCELL_TYPE) and make globally available */

    else
	data_type = Rast_map_type(choice->fn, G_mapset());

    /* if using a moving window, get the parameters,
       and start the moving window driver */

    if (choice->wrum == 'm')
	mv_driver();

    /*  if using the whole raster map as the sampling
       area or sampling with units or regions, open 
       the output files */

    else {

	if (choice->att[0]) {
	    fp0 = fopen0("r.le.out/b1-4.out", "w");
	    fprintf(fp0,
		    "                 MEAN        ST. DEV.    MINIMUM     MAXIMUM\n");
	    fprintf(fp0,
		    "Scale Unit       PIXEL ATT.  PIXEL ATT.  PIXEL ATT.  PIXEL ATT.\n");
	    fclose(fp0);
	}

	if (choice->div[0]) {
	    fp1 = fopen0("r.le.out/d1-4.out", "w");
	    fprintf(fp1,
		    "                                                     INVERSE\n");
	    fprintf(fp1,
		    "Scale Unit       RICHNESS    SHANNON     DOMINANCE   SIMPSON\n");
	    fclose(fp1);
	}

	if (choice->te2[0]) {
	    fp2 = fopen0("r.le.out/t1-5.out", "w");
	    fprintf(fp2,
		    "                             ANGULAR     INVERSE\n");
	    fprintf(fp2,
		    "Scale Unit       CONTAGION   SEC. MOM.   DIFF. MOM.  ENTROPY     CONTRAST\n");
	    fclose(fp2);
	}

	if (choice->jux[0]) {
	    fp3 = fopen0("r.le.out/j1-2.out", "w");
	    fprintf(fp3, "                 MEAN        ST. DEV..\n");
	    fprintf(fp3, "Scale Unit       JUXTAPOS.   JUXTAPOS.\n");
	    fclose(fp3);
	}

	if (choice->edg[0]) {
	    if (choice->edg[1]) {
		fp4 = fopen0("r.le.out/e1.out", "w");
		fprintf(fp4, "                 SUM\n");
		fprintf(fp4, "Scale Unit       OF EDGES\n");
		fclose(fp4);
	    }
	    if (choice->edg[2]) {
		fp5 = fopen0("r.le.out/e2.out", "w");
		fprintf(fp5, "                 SUM\n");
		fprintf(fp5, "Scale Unit       OF EDGES\n");
		fclose(fp5);
	    }
	}

	if (choice->wrum == 'w' || choice->wrum == 'r')
	    whole_reg_driver();
	else if (choice->wrum == 'u')
	    unit_driver();
    }

    Rast_close(finput);

    fputs("R.LE.PIXEL IS DONE;  ", stderr);

    if (choice->wrum != 'm')
	fputs("OUTPUT FILES IN SUBDIRECTORY \"r.le.out\"\n", stderr);

    return;
}




					/* MOVING WINDOW DRIVER */

void mv_driver()
{
    register int i, j;
    int nr, nc, u_w, u_l, x0, y0, d, fmask, m, p, cntwhole = 0, b, *row_buf;
    char *nul_buf, *nulltmp;
    int *tmp;
    float *ftmp;
    double *dtmp;
    double *tmp_buf, *tmp_buf2, **buff = NULL, *richwhole;
    int b1, b2, b3, b4, d1, d2, d3, d4, t1, t2, t3, t4, t5, j1, j2, e1, e2;
    long finished_time;
    float radius;
    struct Cell_head wind;

    /* variables: 
       nc = #cols. in search area minus (1/2 width of mov. wind. + 1) =
       number of cols with values in out map
       nr = #rows in search area minus (1/2 height of mov. wind. + 1) =
       number of rows with values in out map
       x0 = starting column for upper L corner of search area
       y0 = starting row for upper L corner of search area
       u_w = width of mov. wind. in cells
       u_l = width of mov. wind. in cells
       x0 = starting column for upper L corner of mov. wind.
       y0 = starting row for upper L corner of mov. wind.
       row = row for moving-window center
       col = column for moving-window center
       *row_buf = temporary array that holds one row of the
       MASK if there is one
       *tmp_buf = temporary array that holds one moving wind.
       measure for a single row
       **buff = temporary array that holds the set of chosen 
       measures for a row
       radius = radius of the sampling unit, if circles are used
     */


    /* open the appropriate output
       maps */

    if (choice->att[1]) {
	if (G_find_raster("b1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=b1,b1bak");
	    system(cmdbuf);
	}
	b1 = Rast_open_new("b1", DCELL_TYPE);
    }
    if (choice->att[2]) {
	if (G_find_raster("b2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=b2,b2bak");
	    system(cmdbuf);
	}
	b2 = Rast_open_new("b2", DCELL_TYPE);
    }
    if (choice->att[3]) {
	if (G_find_raster("b3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=b3,b3bak");
	    system(cmdbuf);
	}
	b3 = Rast_open_new("b3", DCELL_TYPE);
    }
    if (choice->att[4]) {
	if (G_find_raster("b4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=b4,b4bak");
	    system(cmdbuf);
	}
	b4 = Rast_open_new("b4", DCELL_TYPE);
    }

    if (choice->div[1]) {
	if (G_find_raster("d1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=d1,d1bak");
	    system(cmdbuf);
	}
	d1 = Rast_open_new("d1", DCELL_TYPE);
    }
    if (choice->div[2]) {
	if (G_find_raster("d2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=d2,d2bak");
	    system(cmdbuf);
	}
	d2 = Rast_open_new("d2", DCELL_TYPE);
    }
    if (choice->div[3]) {
	if (G_find_raster("d3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=d3,d3bak");
	    system(cmdbuf);
	}
	d3 = Rast_open_new("d3", DCELL_TYPE);
    }
    if (choice->div[4]) {
	if (G_find_raster("d4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=d4,d4bak");
	    system(cmdbuf);
	}
	d4 = Rast_open_new("d4", DCELL_TYPE);
    }

    if (choice->te2[1]) {
	if (G_find_raster("t1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=t1,t1bak");
	    system(cmdbuf);
	}
	t1 = Rast_open_new("t1", DCELL_TYPE);
    }
    if (choice->te2[2]) {
	if (G_find_raster("t2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=t2,t2bak");
	    system(cmdbuf);
	}
	t2 = Rast_open_new("t2", DCELL_TYPE);
    }
    if (choice->te2[3]) {
	if (G_find_raster("t3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=t3,t3bak");
	    system(cmdbuf);
	}
	t3 = Rast_open_new("t3", DCELL_TYPE);
    }
    if (choice->te2[4]) {
	if (G_find_raster("t4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=t4,t4bak");
	    system(cmdbuf);
	}
	t4 = Rast_open_new("t4", DCELL_TYPE);
    }
    if (choice->te2[5]) {
	if (G_find_raster("t5", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=t5,t5bak");
	    system(cmdbuf);
	}
	t5 = Rast_open_new("t5", DCELL_TYPE);
    }

    if (choice->jux[1]) {
	if (G_find_raster("j1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=j1,j1bak");
	    system(cmdbuf);
	}
	j1 = Rast_open_new("j1", DCELL_TYPE);
    }
    if (choice->jux[2]) {
	if (G_find_raster("j2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=j2,j2bak");
	    system(cmdbuf);
	}
	j2 = Rast_open_new("j2", DCELL_TYPE);
    }
    if (choice->edg[1]) {
	if (G_find_raster("e1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=e1,e1bak");
	    system(cmdbuf);
	}
	e1 = Rast_open_new("e1", DCELL_TYPE);
    }
    if (choice->edg[2]) {
	if (G_find_raster("e2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=e2,e2bak");
	    system(cmdbuf);
	}
	e2 = Rast_open_new("e2", DCELL_TYPE);
    }


    /* get the moving window parameters */

    read_mwind(&u_w, &u_l, &nc, &nr, &x0, &y0, &radius);

    /* check for an unacceptable 
       moving-window size */

    if (nc < 1 || nr < 1) {
	fprintf(stdout, "\n");
	fprintf(stdout,
		"   *******************************************************\n");
	fprintf(stdout,
		"    The moving window size specified in file r.le.para/   \n");
	fprintf(stdout,
		"    move_wind is less than 1 row or column.  Check this   \n");
	fprintf(stdout,
		"    file or redefine the moving window using r.le.setup.  \n");
	fprintf(stdout,
		"   *******************************************************\n");
	exit(1);
    }

    /* check for an unacceptable
       search area */

    G_get_set_window(&wind);
    if (wind.rows < nr + y0 || wind.cols < nc + x0) {
	fprintf(stdout, "\n");
	fprintf(stdout,
		"   *******************************************************\n");
	fprintf(stdout,
		"    Moving window search area in file r.le.para/move_wind \n");
	fprintf(stdout,
		"    does not match the dimensions of the current region.  \n");
	fprintf(stdout,
		"    You must either rerun r.le.setup to make a new        \n");
	fprintf(stdout,
		"    r.le.para/move_wind file or reset the region to match \n");
	fprintf(stdout,
		"    the r.le.para/move_wind file 		               \n");
	fprintf(stdout,
		"   *******************************************************\n");
	exit(1);
    }


    /* begin main moving window loop 
       section */

    /* set the d parameter for the 
       performance meter */

    if (nr * nc > 10000)
	d = nr * nc / 1000;
    else if (nr * nc > 2500)
	d = nr * nc / 100;
    else
	d = 10;
    /* return a value > 0 to fmask if
       there is a MASK present */

    fprintf(stdout,
	    "If a MASK is not present (see r.mask) a beep may sound and a\n");
    fprintf(stdout,
	    "   warning may be printed or appear in a window; ignore this warning.\n");
    fprintf(stdout, "If a MASK is present there will be no warning.\n");
    fmask = Rast_open_old("MASK", G_mapset());
    fprintf(stdout, "\n");

    /* allocate memory for the buffer */

    buff = (double **)G_calloc(nc + 1, sizeof(double *));

    /* allocate memory for each of 17 measures */

    for (p = 0; p < nc + 1; p++)
	buff[p] = (double *)G_calloc(17, sizeof(double));


    /* allocate memory for a row buffer if
       there is a mask */

    if (fmask > 0)
	row_buf = Rast_allocate_buf(CELL_TYPE);


    if (choice->edg[2] || choice->jux[0]) {

	/* dynamically allocate memory for
	   the richness array */

	richwhole = (double *)G_calloc(MAX, sizeof(double));

	/* initialize the richness array
	   elements with the value -999 */

	for (i = 0; i < MAX; i++)
	    richwhole[i] = -999.0;

	/* allocate memory to store a row of the
	   raster map, depending on the type of
	   input raster map; keep track of the
	   name of the buffer for each raster type */

	switch (data_type) {
	case CELL_TYPE:
	    tmp = Rast_allocate_buf(CELL_TYPE);
	    break;
	case FCELL_TYPE:
	    ftmp = Rast_allocate_buf(FCELL_TYPE);
	    break;
	case DCELL_TYPE:
	    dtmp = Rast_allocate_buf(DCELL_TYPE);
	    break;
	}

	nul_buf = Rast_allocate_null_buf();

	/* go through the search area
	   pixel by pixel */

	for (i = 0; i < nr; i++) {

	    switch (data_type) {
	    case (CELL_TYPE):
		Rast_zero_buf(tmp, CELL_TYPE);
		Rast_get_row(finput, tmp, i, CELL_TYPE);
		break;
	    case (FCELL_TYPE):
		Rast_zero_buf(ftmp, FCELL_TYPE);
		Rast_get_row(finput, ftmp, i, FCELL_TYPE);
		break;
	    case (DCELL_TYPE):
		Rast_zero_buf(dtmp, DCELL_TYPE);
		Rast_get_row(finput, dtmp, i, DCELL_TYPE);
		break;
	    }

	    Rast_get_null_value_row(finput, nul_buf, i);

	    for (j = 0; j < nc; j++) {

		/* if the raster value is not null,
		   call get_rich_whole to tally up the
		   number of different attributes
		   in the search area and fill
		   the richness array with those
		   attributes */
		switch (data_type) {
		case (CELL_TYPE):
		    if ((*(tmp + j) ||
			 *(tmp + j) == 0) && *(nul_buf + j) == 0.0)
			get_rich_whole((double)*(tmp + j), richwhole,
				       &cntwhole);
		    break;
		case (FCELL_TYPE):
		    if ((*(ftmp + j) ||
			 *(ftmp + j) == 0) && *(nul_buf + j) == 0.0)
			get_rich_whole((double)*(ftmp + j), richwhole,
				       &cntwhole);
		    break;
		case (DCELL_TYPE):
		    if ((*(dtmp + j) ||
			 *(dtmp + j) == 0) && *(nul_buf + j) == 0.0)
			get_rich_whole(*(dtmp + j), richwhole, &cntwhole);
		    break;
		}
	    }
	}

	switch (data_type) {
	case (CELL_TYPE):
	    G_free(tmp);
	    break;
	case (FCELL_TYPE):
	    G_free(ftmp);
	    break;
	case (DCELL_TYPE):
	    G_free(dtmp);
	    break;
	}
	G_free(nul_buf);
	G_free(richwhole);
    }

    /* main loop for clipping & measuring 
       using the moving-window; index i
       refers to which moving window, not
       the row of the original map */

    for (i = 0; i < nr; i++) {

	/* zero the buffer before filling it
	   again */

	for (m = 0; m < nc + 1; m++) {
	    for (p = 0; p < 17; p++)
		*(*(buff + m) + p) = 0.0;
	}

	/* if there is a MASK, then read in
	   a row of MASK - this part skips 
	   cells with the value "0" in the 
	   MASK to speed up the moving window
	   process */

	if (fmask > 0) {
	    Rast_zero_buf(row_buf, CELL_TYPE);
	    Rast_get_row_nomask(fmask, row_buf, y0 + i + u_l / 2,
				    CELL_TYPE);

	    /* for each cell whose value is "1"
	       in MASK */

	    for (j = 0; j < nc; j++) {

		/* display #cells left to do */

		if (i == 0 && j == 0)
		    fprintf(stdout, "TOTAL WINDOWS = %8d\n", nr * nc);
		meter2(nr * nc, (i * nc + (j + 1)), d);

		/* call the cell clip driver */

		if (row_buf[x0 + j + u_w / 2])
		    cell_clip_drv(x0 + j, y0 + i, u_w, u_l, buff, j, cntwhole,
				  radius);
	    }
	}

	/* if there is no MASK, then clip
	   and measure at every cell */

	else {

	    for (j = 0; j < nc; j++) {

		/* display #cells left to do */

		if (i == 0 && j == 0)
		    fprintf(stdout, "TOTAL WINDOWS = %8d\n", nr * nc);
		meter2(nr * nc, (i * nc + (j + 1)), d);

		/* call the cell clip driver.  This routine will clip
		   the rectangle at x0 + j, y0 + i and u_w X u_l wide
		   (or in a circle with radius), and put the results
		   for each chosen moving windown measure in buff;
		   note that the center of the moving window is not
		   at x0 + j, y0 + i, but at x0 + j + u_w/2, y0 + i +
		   u_l/2 */

		cell_clip_drv(x0 + j, y0 + i, u_w, u_l, buff, j, cntwhole,
			      radius);
	    }
	}

	/* copy the chosen measures into a temporary row
	   buffer which is then fed into the chosen output
	   maps; the map location is adjusted to the center
	   of the moving window */

	tmp_buf = Rast_allocate_buf(DCELL_TYPE);
	nulltmp = Rast_allocate_null_buf();

	if (choice->att[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 0) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 0);
		}
		Rast_put_d_row(b1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 1) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 1);
		}
		Rast_put_d_row(b2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 2) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 2);
		}
		Rast_put_d_row(b3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 3) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 3);
		}
		Rast_put_d_row(b4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(b4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->div[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 4) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 4);
		}
		Rast_put_d_row(d1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->div[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 5) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 5);
		}
		Rast_put_d_row(d2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->div[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 6) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 6);
		}
		Rast_put_d_row(d3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->div[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 7) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 7);
		}
		Rast_put_d_row(d4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(d4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->te2[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 8) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 8);
		}
		Rast_put_d_row(t1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->te2[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 9) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 9);
		}
		Rast_put_d_row(t2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->te2[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 10) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 10);
		}
		Rast_put_d_row(t3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->te2[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 11) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 11);
		}
		Rast_put_d_row(t4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->te2[5]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t5, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 12) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 12);
		}
		Rast_put_d_row(t5, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(t5, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->jux[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(j1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 13) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 13);
		}
		Rast_put_d_row(j1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(j1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->jux[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(j2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 14) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 14);
		}
		Rast_put_d_row(j2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(j2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->edg[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(e1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 15) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 15);
		}
		Rast_put_d_row(e1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(e1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->edg[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(e2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 16) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 16);
		}
		Rast_put_d_row(e2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(e2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}


	G_free(tmp_buf);
    }

    time(&finished_time);
    fprintf(stdout, "\nACTUAL COMPLETION = %s",
	    asctime(localtime(&finished_time)));
    fflush(stdout);

    /* free the memory allocated for the 
       mask and other buffer */

    if (fmask > 0)
	G_free(row_buf);
    for (p = 0; p < nc + 1; p++)
	G_free(buff[p]);
    G_free(buff);

    /* close the raster maps, set the
       color table for the new raster
       map and compress the map */


    if (choice->att[1]) {
	Rast_close(b1);
	set_colors("b1");
	sprintf(cmdbuf, "%s %s", "r.compress", "b1");
	system(cmdbuf);
    }
    if (choice->att[2]) {
	Rast_close(b2);
	set_colors("b2");
	sprintf(cmdbuf, "%s %s", "r.compress", "b2");
	system(cmdbuf);
    }
    if (choice->att[3]) {
	Rast_close(b3);
	set_colors("b3");
	sprintf(cmdbuf, "%s %s", "r.compress", "b3");
	system(cmdbuf);
    }
    if (choice->att[4]) {
	Rast_close(b4);
	set_colors("b4");
	sprintf(cmdbuf, "%s %s", "r.compress", "b4");
	system(cmdbuf);
    }
    if (choice->div[1]) {
	Rast_close(d1);
	set_colors("d1");
	sprintf(cmdbuf, "%s %s", "r.compress", "d1");
	system(cmdbuf);
    }
    if (choice->div[2]) {
	Rast_close(d2);
	set_colors("d2");
	sprintf(cmdbuf, "%s %s", "r.compress", "d2");
	system(cmdbuf);
    }
    if (choice->div[3]) {
	Rast_close(d3);
	set_colors("d3");
	sprintf(cmdbuf, "%s %s", "r.compress", "d3");
	system(cmdbuf);
    }
    if (choice->div[4]) {
	Rast_close(d4);
	set_colors("d4");
	sprintf(cmdbuf, "%s %s", "r.compress", "d4");
	system(cmdbuf);
    }
    if (choice->te2[1]) {
	Rast_close(t1);
	set_colors("t1");
	sprintf(cmdbuf, "%s %s", "r.compress", "t1");
	system(cmdbuf);
    }
    if (choice->te2[2]) {
	Rast_close(t2);
	set_colors("t2");
	sprintf(cmdbuf, "%s %s", "r.compress", "t2");
	system(cmdbuf);
    }
    if (choice->te2[3]) {
	Rast_close(t3);
	set_colors("t3");
	sprintf(cmdbuf, "%s %s", "r.compress", "t3");
	system(cmdbuf);
    }
    if (choice->te2[4]) {
	Rast_close(t4);
	set_colors("t4");
	sprintf(cmdbuf, "%s %s", "r.compress", "t4");
	system(cmdbuf);
    }
    if (choice->te2[5]) {
	Rast_close(t5);
	set_colors("t5");
	sprintf(cmdbuf, "%s %s", "r.compress", "t5");
	system(cmdbuf);
    }
    if (choice->jux[1]) {
	Rast_close(j1);
	set_colors("j1");
	sprintf(cmdbuf, "%s %s", "r.compress", "j1");
	system(cmdbuf);
    }
    if (choice->jux[2]) {
	Rast_close(j2);
	set_colors("j2");
	sprintf(cmdbuf, "%s %s", "r.compress", "j2");
	system(cmdbuf);
    }
    if (choice->edg[1]) {
	Rast_close(e1);
	set_colors("e1");
	sprintf(cmdbuf, "%s %s", "r.compress", "e1");
	system(cmdbuf);
    }
    if (choice->edg[2]) {
	Rast_close(e2);
	set_colors("e2");
	sprintf(cmdbuf, "%s %s", "r.compress", "e2");
	system(cmdbuf);
    }

    Rast_close(fmask);

    return;
}







					/* SET "OUT" RASTER FILE COLOR
					   TABLE TO G-Y-R */

void set_colors(char *name)
{
    struct Colors colors;
    struct FPRange fprange;

    Rast_read_fp_range(name, G_mapset(), &fprange);
    Rast_make_gyr_fp_colors(&colors, fprange.min, fprange.max);
    Rast_write_colors(name, G_mapset(), &colors);
    return;
}






					/* OPEN OUTPUT FILE WITH ERROR TRAP */

FILE *fopen0(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stdout, "\n");
	fprintf(stdout, "   ******************************************\n");
	fprintf(stdout, "    Can't open output file \"%s\"            \n",
		name);
	fprintf(stdout, "    Do you have write permission in r.le.out \n");
	fprintf(stdout, "    subdirectory?                            \n");
	fprintf(stdout, "   ******************************************\n");
	exit(1);
    }
    return fp;
}





					/* OPEN INPUT FILE WITH ERROR TRAP */

FILE *fopen1(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stdout, "\n");
	fprintf(stdout,
		"   ******************************************************\n");
	fprintf(stdout,
		"    You chose a moving window or sampling units analysis \n");
	fprintf(stdout,
		"       but r.le.pixel can't find file \"%s\"             \n",
		name);
	fprintf(stdout,
		"       which defines the moving window or sampling units \n");
	fprintf(stdout,
		"    First use r.le.setup for to setup a moving window or \n");
	fprintf(stdout,
		"       sampling units to make this file                  \n");
	fprintf(stdout,
		"   ******************************************************\n");
	exit(1);
    }
    return fp;
}



					/* OPEN WEIGHT FILE WITH ERROR TRAP */

FILE *fopen2(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stdout, "\n");
	fprintf(stdout,
		"   ***************************************************\n");
	fprintf(stdout,
		"    You chose a juxtaposition measure, but r.le.pixel \n");
	fprintf(stdout,
		"       can't find file \"%s\"             		   \n",
		name);
	fprintf(stdout,
		"       which defines the weights for different edges  \n");
	fprintf(stdout,
		"    First use a text editor to make this file         \n");
	fprintf(stdout,
		"   ***************************************************\n");
	exit(1);
    }
    return fp;
}



					/* OPEN EDGE FILE WITH ERROR TRAP */

FILE *fopen3(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stdout, "\n");
	fprintf(stdout,
		"   ***************************************************\n");
	fprintf(stdout,
		"    You chose an edge measure, but r.le.pixel         \n");
	fprintf(stdout,
		"       can't find file \"%s\"             		   \n",
		name);
	fprintf(stdout,
		"       which defines the types of edges to be counted \n");
	fprintf(stdout,
		"    First use a text editor to make this file         \n");
	fprintf(stdout,
		"   ***************************************************\n");
	exit(1);
    }
    return fp;
}


					/* PERFORMANCE METER - DISPLAYS
					   THE PROGRESS OF THE MOVING WINDOW
					   AS A COUNT AND ESTIMATED COMPLETION
					   TIME WHILE THE PROGRAM RUNS */

void meter2(int n, int i, int div)
{
    long current_time, time_left, elapsed, complete;
    static long start;
    float window_time;
    static int k = 0;
    char done2[30];
    int d;


    if (i <= 1) {
	time(&start);
    }

    if (i < 10)
	d = 1;
    else
	d = div;

    if (k > 2000) {
	if (G_fseek(stdout, 0L, 0))
	    G_fatal_error("Can't reset the \"stdout\", exit.\n");
	k = 0;
    }

    if ((n - i) % d == 0) {
	time(&current_time);
	elapsed = current_time - start;
	window_time = ((float)elapsed) / (i + 1);
	time_left = (long)((n - i) * window_time);
	complete = current_time + time_left;
	strncpy(done2, asctime(localtime(&complete)), 24);
	done2[24] = '\0';
	fprintf(stdout, "WINDOWS LEFT  = %8d     EST. COMPLETION = %s\r",
		(n - i), done2);
	fflush(stdout);
	k++;
    }
    return;
}


					/* READ IN THE MOVING WINDOW
					   PARAMETERS */

void read_mwind(int *uw, int *ul, int *nc, int *nr, int *x0, int *y0,
		float *radius)
{
    FILE *fp;
    int ww, wl;
    char *buf;

    fp = fopen1("r.le.para/move_wind", "r");
    buf = G_malloc(513);

    fgets(buf, 512, fp);
    sscanf(buf, "%d%d", uw, ul);

    fgets(buf, 512, fp);
    sscanf(buf, "%f", radius);

    fgets(buf, 512, fp);
    sscanf(buf, "%d%d", &ww, &wl);

    fgets(buf, 512, fp);
    sscanf(buf, "%d%d", x0, y0);

    *nc = ww - *uw + 1;
    *nr = wl - *ul + 1;

    G_free(buf);
    fclose(fp);

    return;
}


					/* READ IN THE SAMPLING UNIT 
					   PARAMETERS AND RUN R.LE.PIXEL */

void unit_driver()
{
    register int i, j, k, m;
    int top, left, u_w, u_l, nscl, nu, fd, *tmp;
    float *ftmp;
    double *richwhole, *dtmp;
    char *nul_buf;
    char *buf, unitname[10], istr[3];
    int cntwhole = 0;
    FILE *fp;
    CELL **units, *unit_buf;
    float radius = 0.0;
    struct Cell_head wind;


    G_get_set_window(&wind);
    fp = fopen1("r.le.para/units", "r");

    buf = G_malloc(513);

    /* get the number of scales */

    fgets(buf, 512, fp);
    sscanf(buf, "%d", &nscl);


    if (choice->edg[2] || choice->jux[0]) {

	/* dynamically allocate memory for
	   the richness array */

	richwhole = (double *)G_calloc(MAX, sizeof(double));

	/* initialize the richness array
	   elements with the value -999 */

	for (i = 0; i < MAX; i++)
	    richwhole[i] = -999.0;

	switch (data_type) {
	case CELL_TYPE:
	    tmp = Rast_allocate_buf(CELL_TYPE);
	    break;
	case FCELL_TYPE:
	    ftmp = Rast_allocate_buf(FCELL_TYPE);
	    break;
	case DCELL_TYPE:
	    dtmp = Rast_allocate_buf(DCELL_TYPE);
	    break;
	}

	nul_buf = Rast_allocate_null_buf();

	/* go through the search area
	   pixel by pixel */

	for (i = 0; i < wind.rows; i++) {

	    switch (data_type) {
	    case (CELL_TYPE):
		Rast_zero_buf(tmp, CELL_TYPE);
		Rast_get_row(finput, tmp, i, CELL_TYPE);
		break;
	    case (FCELL_TYPE):
		Rast_zero_buf(ftmp, FCELL_TYPE);
		Rast_get_row(finput, ftmp, i, FCELL_TYPE);
		break;
	    case (DCELL_TYPE):
		Rast_zero_buf(dtmp, DCELL_TYPE);
		Rast_get_row(finput, dtmp, i, DCELL_TYPE);
		break;
	    }

	    Rast_get_null_value_row(finput, nul_buf, i);


	    for (j = 0; j < wind.cols; j++) {

		/* if the raster value is not null,
		   call get_rich_whole to tally up the
		   number of different attributes
		   in the search area and fill
		   the richness array with those
		   attributes */
		switch (data_type) {
		case (CELL_TYPE):
		    if ((*(tmp + j) ||
			 *(tmp + j) == 0) && *(nul_buf + j) == 0.0)
			get_rich_whole((double)*(tmp + j), richwhole,
				       &cntwhole);
		    break;
		case (FCELL_TYPE):
		    if ((*(ftmp + j) ||
			 *(ftmp + j) == 0) && *(nul_buf + j) == 0.0)
			get_rich_whole((double)*(ftmp + j), richwhole,
				       &cntwhole);
		    break;
		case (DCELL_TYPE):
		    if ((*(dtmp + j) ||
			 *(dtmp + j) == 0) && *(nul_buf + j) == 0.0)
			get_rich_whole(*(dtmp + j), richwhole, &cntwhole);
		    break;
		}
	    }
	}
	switch (data_type) {
	case (CELL_TYPE):
	    G_free(tmp);
	    break;
	case (FCELL_TYPE):
	    G_free(ftmp);
	    break;
	case (DCELL_TYPE):
	    G_free(dtmp);
	    break;
	}
	G_free(nul_buf);
	G_free(richwhole);
    }

    /* dynamically allocate storage for the
       buffer that will hold the map of the
       sampling units */

    if (choice->units) {
	units = (CELL **) G_calloc(wind.rows + 3, sizeof(CELL *));
	for (i = 0; i < wind.rows + 3; i++)
	    units[i] = (CELL *) G_calloc(wind.cols + 3, sizeof(CELL));
    }

    /* for each scale */

    for (i = 0; i < nscl; i++) {
	g_scale = i + 1;
	fgets(buf, 512, fp);
	sscanf(buf, "%d", &nu);

	/* get the width and length */

	fgets(buf, 512, fp);
	sscanf(buf, "%d%d", &u_w, &u_l);

	/* get the radius to see if sampling
	   units are circles */

	fgets(buf, 512, fp);
	sscanf(buf, "%f", &radius);

	/* if units map was chosen, zero it,
	   then copy the number of the map
	   to the end of the word "units" */

	if (choice->units) {
	    for (k = 0; k < wind.rows + 3; k++) {
		for (m = 0; m < wind.cols + 3; m++)
		    *(*(units + k) + m) = 0;
	    }

	    if (i == 0)
		strcpy(istr, "1");
	    else if (i == 1)
		strcpy(istr, "2");
	    else if (i == 2)
		strcpy(istr, "3");
	    else if (i == 3)
		strcpy(istr, "4");
	    else if (i == 4)
		strcpy(istr, "5");
	    else if (i == 5)
		strcpy(istr, "6");
	    else if (i == 6)
		strcpy(istr, "7");
	    else if (i == 7)
		strcpy(istr, "8");
	    else if (i == 8)
		strcpy(istr, "9");
	    else if (i == 9)
		strcpy(istr, "10");
	    else if (i == 10)
		strcpy(istr, "11");
	    else if (i == 11)
		strcpy(istr, "12");
	    else if (i == 12)
		strcpy(istr, "13");
	    else if (i == 13)
		strcpy(istr, "14");
	    else if (i == 14)
		strcpy(istr, "15");
	    else if (i > 14) {
		fprintf(stdout, "\n");
		fprintf(stdout,
			"   ***************************************************\n");
		fprintf(stdout,
			"    You cannot choose more than 15 scales             \n");
		fprintf(stdout,
			"   ***************************************************\n");
		exit(0);
	    }
	}

	/* for each unit */

	for (j = 0; j < nu; j++) {
	    g_unit = j + 1;
	    fgets(buf, 512, fp);
	    sscanf(buf, "%d%d", &left, &top);

	    /* call cell_clip driver */

	    run_clip(wind.cols, wind.rows, u_w, u_l, left, top, units, j,
		     cntwhole, radius);
	}

	/* if a map of the sampling units
	   was requested */

	if (choice->units) {
	    strcpy(unitname, "units_");
	    strcat(unitname, istr);
	    fd = Rast_open_new(unitname, CELL_TYPE);
	    unit_buf = Rast_allocate_buf(CELL_TYPE);
	    for (k = 1; k < wind.rows + 1; k++) {
		Rast_zero_buf(unit_buf, CELL_TYPE);
		Rast_set_null_value(unit_buf, wind.cols + 1, CELL_TYPE);
		for (m = 1; m < wind.cols + 3; m++) {
		    if (*(*(units + k) + m))
			*(unit_buf + m - 1) = *(*(units + k) + m);
		}
		Rast_put_row(fd, unit_buf, CELL_TYPE);
	    }
	    Rast_close(fd);
	    G_free(unit_buf);
	}
    }

    if (choice->units) {
	for (m = 0; m < wind.rows + 3; m++)
	    G_free(units[m]);
	G_free(units);
    }
    G_free(buf);
    fclose(fp);
    return;
}





					/* CHECK FOR OUT-OF MAP UNIT, THEN
					   CALL CELL CLIP DRIVER */


void run_clip(int ncols, int nrows, int u_w, int u_l, int left, int top,
	      CELL ** units, int id, int cntwhole, float radius)
{
    int i, j;
    double center_row, center_col;
    double dist;

    G_sleep_on_error(0);

    /* check unit */

    if (ncols < left + u_w || nrows < top + u_l) {
	fprintf(stdout, "\n");
	fprintf(stdout,
		"   ******************************************************\n");
	fprintf(stdout,
		"    Sampling units do not fit within the current region. \n");
	fprintf(stdout,
		"    Either correct the region or redo the sampling unit  \n");
	fprintf(stdout,
		"    selection using r.le.setup.  This error message came \n");
	fprintf(stdout,
		"    from an analysis of the r.le.para/units file and the \n");
	fprintf(stdout,
		"    current region setting.                              \n");
	fprintf(stdout,
		"   ******************************************************\n");
	exit(1);
    }

    if (choice->units) {
	if (radius) {
	    center_row = ((double)(top + 1) + ((double)u_l - 1) / 2);
	    center_col = ((double)(left + 1) + ((double)u_w - 1) / 2);

	    for (i = top + 1; i < top + 1 + u_l; i++) {
		for (j = left + 1; j < left + 1 + u_w; j++) {
		    dist =
			sqrt(((double)i - center_row) * ((double)i -
							 center_row) +
			     ((double)j - center_col) * ((double)j -
							 center_col));
		    if (dist < radius) {
			*(*(units + i) + j) = id + 1;
			/*printf("units[%d][%d]=%d id=%d\n",i,j, *(*(units + i) + j),id + 1);  */
		    }
		}
	    }
	}
	else {
	    for (i = top + 1; i < top + 1 + u_l; i++) {
		for (j = left + 1; j < left + 1 + u_w; j++)
		    *(*(units + i) + j) = id + 1;
	    }
	}
    }

    cell_clip_drv(left, top, u_w, u_l, NULL, 0, cntwhole, radius);

    return;
}





					/* CLIP THE REGION, THEN
					   RUN R.LE.PIXEL */


void whole_reg_driver()
{
    register int i, j;
    int *row_buf, regcnt, found, fr, nrows, ncols, cntwhole = 0, *tmp;
    float *ftmp;
    double *richwhole, *dtmp;
    char *nul_buf;
    REGLIST *ptrfirst, *ptrthis, *ptrnew;
    RASTER_MAP_TYPE data_type;

    data_type = Rast_map_type(choice->fn, G_mapset());

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    g_scale = 1;

    if (choice->edg[2] || choice->jux[0]) {

	/* dynamically allocate memory for
	   the richness array */

	richwhole = (double *)G_calloc(MAX, sizeof(double));

	/* initialize the richness array
	   elements with the value -999 */

	for (i = 0; i < MAX; i++)
	    richwhole[i] = -999.0;

	/* allocate memory to store a row of the
	   raster map, depending on the type of
	   input raster map; keep track of the
	   name of the buffer for each raster type */

	switch (data_type) {
	case CELL_TYPE:
	    tmp = Rast_allocate_buf(CELL_TYPE);
	    break;
	case FCELL_TYPE:
	    ftmp = Rast_allocate_buf(FCELL_TYPE);
	    break;
	case DCELL_TYPE:
	    dtmp = Rast_allocate_buf(DCELL_TYPE);
	    break;
	}

	nul_buf = Rast_allocate_null_buf();

	/* go through the search area
	   pixel by pixel */

	for (i = 0; i < nrows; i++) {

	    switch (data_type) {
	    case (CELL_TYPE):
		Rast_zero_buf(tmp, CELL_TYPE);
		Rast_get_row(finput, tmp, i, CELL_TYPE);
		break;
	    case (FCELL_TYPE):
		Rast_zero_buf(ftmp, FCELL_TYPE);
		Rast_get_row(finput, ftmp, i, FCELL_TYPE);
		break;
	    case (DCELL_TYPE):
		Rast_zero_buf(dtmp, DCELL_TYPE);
		Rast_get_row(finput, dtmp, i, DCELL_TYPE);
		break;
	    }

	    Rast_get_null_value_row(finput, nul_buf, i);


	    for (j = 0; j < ncols; j++) {

		/* if the raster value is not null,
		   call get_rich_whole to tally up the
		   number of different attributes
		   in the search area and fill
		   the richness array with those
		   attributes */

		switch (data_type) {
		case (CELL_TYPE):
		    if ((*(tmp + j) ||
			 *(tmp + j) == 0) && *(nul_buf + j) == 0.0)
			get_rich_whole((double)*(tmp + j), richwhole,
				       &cntwhole);
		    break;
		case (FCELL_TYPE):
		    if ((*(ftmp + j) ||
			 *(ftmp + j) == 0.0) && *(nul_buf + j) == 0.0)
			get_rich_whole((double)*(ftmp + j), richwhole,
				       &cntwhole);
		    break;
		case (DCELL_TYPE):
		    if ((*(dtmp + j) ||
			 *(dtmp + j) == 0.0) && *(nul_buf + j) == 0.0)
			get_rich_whole(*(dtmp + j), richwhole, &cntwhole);
		    break;
		}
	    }
	}
	switch (data_type) {
	case (CELL_TYPE):
	    G_free(tmp);
	    break;
	case (FCELL_TYPE):
	    G_free(ftmp);
	    break;
	case (DCELL_TYPE):
	    G_free(dtmp);
	    break;
	}
	G_free(nul_buf);
	G_free(richwhole);
    }

    if (choice->wrum != 'r') {
	cell_clip_drv(0, 0, ncols, nrows, NULL, 0, cntwhole, 0.0);
    }
    else {
	regcnt = 0;
	fr = Rast_open_old(choice->reg, G_mapset());
	row_buf = Rast_allocate_buf(CELL_TYPE);
	for (i = 0; i < nrows; i++) {
	    Rast_zero_buf(row_buf, CELL_TYPE);
	    Rast_get_row(fr, row_buf, i, CELL_TYPE);
	    for (j = 0; j < ncols; j++) {
		if (*(row_buf + j)) {
		    if (regcnt == 0)
			ptrfirst = (REGLIST *) NULL;
		    ptrthis = ptrfirst;
		    found = 0;
		    while (ptrthis) {
			if (*(row_buf + j) == ptrthis->att) {
			    if (j < ptrthis->w)
				ptrthis->w = j;
			    if (j > ptrthis->e)
				ptrthis->e = j;
			    if (i < ptrthis->n)
				ptrthis->n = i;
			    if (i > ptrthis->s)
				ptrthis->s = i;
			    found = 1;
			}
			ptrthis = ptrthis->next;
		    }
		    if (!found) {
			ptrnew = (REGLIST *) G_calloc(1, sizeof(REGLIST));
			if (ptrfirst == (REGLIST *) NULL)
			    ptrfirst = ptrthis = ptrnew;
			else {
			    ptrthis = ptrfirst;
			    while (ptrthis->next != (REGLIST *) NULL)
				ptrthis = ptrthis->next;
			    ptrthis->next = ptrnew;
			    ptrthis = ptrnew;
			}
			ptrthis->att = *(row_buf + j);
			ptrthis->n = i;
			ptrthis->s = i;
			ptrthis->e = j;
			ptrthis->w = j;
			regcnt++;
		    }
		}
	    }
	}
	g_unit = 0;
	ptrthis = ptrfirst;
	while (ptrthis) {
	    g_unit = ptrthis->att;
	    cell_clip_drv(ptrthis->w, ptrthis->n, ptrthis->e - ptrthis->w + 1,
			  ptrthis->s - ptrthis->n + 1, NULL, ptrthis->att,
			  cntwhole, 0.0);
	    ptrthis = ptrthis->next;
	}
	Rast_close(fr);
	G_free(row_buf);
	G_free(ptrnew);
    }

    return;
}






					/* FIND UNCOUNTED ATTRIBUTES, 
					   COUNT THEM UP, AND ADD THEM TO
					   THE RICHNESS ARRAY IN UNSORTED
					   ORDER */

void get_rich_whole(double att, double rich[], int *cnt)
{
    register int i;

    /* if this attribute is already
       in the richness array, then
       return */

    for (i = 0; i < *cnt; i++)
	if (att == rich[i])
	    break;


    /* if this attribute is not already
       in the richness array, then make
       it the "cnt" element of the 
       array, then increment the cnt */

    if (i >= *cnt) {
	rich[*cnt] = att;
	++(*cnt);
	/*printf("rich[%d]=%f\n",*cnt,att);  */
    }

    return;
}
