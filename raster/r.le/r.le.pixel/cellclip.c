/*
 ************************************************************
 * MODULE: r.le.pixel/cellclip.c                            *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze pixel-scale landscape properties     *
 *         cellclip.c clips the area that is being analyzed *
 *         out of the original raster map                   *
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
#include "local_proto.h"


extern struct CHOICE *choice;
extern int finput;


					/* CELL CLIP DRIVER */

void cell_clip_drv(int col0, int row0, int ncols, int nrows, double **value,
		   int index, int cntwhole, float radius)
{

    register int i, j;
    int cnt = 0, p;
    double *rich, *richtmp;
    char *name, *mapset;
    DCELL **buf;
    DCELL **null_buf;
    RASTER_MAP_TYPE data_type;

    /*
       col0 = starting column for area to be clipped
       row0 = starting row for area to be clipped
       ncols = number of columns in area to be clipped
       nrows = number of rows in area to be clipped
       value =
       index = number of the region to be clipped, if there's a region map
       buf = pointer to array containing the clipped area, a smaller area
       than the original raster map to be read from finput                            printf("h2\n");

       pat = pointer to array containing the map of patch numbers
       cor = pointer to array containing the map of interior area
     */

    name = choice->fn;
    mapset = G_mapset();
    data_type = Rast_map_type(name, mapset);

    /* dynamically allocate storage for the
       buffer that will hold the contents of
       the window */

    buf = (DCELL **) G_calloc(nrows + 3, sizeof(DCELL *));
    for (i = 0; i < nrows + 3; i++) {
	buf[i] = (DCELL *) G_calloc(ncols + 3, sizeof(DCELL));
    }

    /* dynamically allocate storage for the
       buffer that will hold the null values for
       the clipped area */

    null_buf = (DCELL **) G_calloc(nrows + 3, sizeof(DCELL *));
    for (i = 0; i < nrows + 3; i++)
	null_buf[i] = (DCELL *) G_calloc(ncols + 3, sizeof(DCELL));

    /* call the cell_clip routine */

    cell_clip(buf, null_buf, row0, col0, nrows, ncols, index, radius);

    /* dynamically allocate memory for
       the richness array */

    richtmp = (double *)G_calloc(MAX, sizeof(double));

    /* go through the sampling area
       pixel by pixel */

    for (i = 1; i < nrows + 1; i++) {
	for (j = 1; j < ncols + 1; j++) {

	    /* if buf[i][j] is not a null value,
	       call get_rich to tally up the
	       number of different attributes
	       in the sampling area and fill
	       the richness array with those
	       attributes */

	    if ((buf[i][j] || buf[i][j] == 0.0) && null_buf[i][j] == 0.0) {
		/*printf("buf[%d][%d] = %f\n",i,j,buf[i][j]); */

		get_rich(buf[i][j], richtmp, &cnt);
	    }
	}
    }

    if (cnt) {

	rich = (double *)G_calloc(cnt, sizeof(double));
	for (i = 0; i < cnt; i++) {
	    rich[i] = richtmp[i];
	}
	G_free(richtmp);

	/* call ANSI C runtime library
	   function qsort to sort the 
	   richness array into ascending
	   order */

	qsort(rich, cnt, sizeof(double), compar);

	/* moving window */

	if (choice->wrum == 'm') {
	    if (is_not_empty_buffer(buf, null_buf, nrows + 1, ncols + 1)) {
		if (center_is_not_null(buf, null_buf, nrows, ncols))
		    mv_texture(nrows, ncols, buf, null_buf, value, index,
			       rich, cnt, cntwhole);
		else {
		    for (p = 0; p < 17; p++)
			*(*(value + index) + p) = -BIG;
		}
	    }
	}

	/* whole map, units, or regions */

	else if (is_not_empty_buffer(buf, null_buf, nrows + 1, ncols + 1))
	    df_texture(nrows, ncols, buf, null_buf, rich, cnt, cntwhole);

	for (i = 0; i < nrows + 3; i++)
	    G_free(*(buf + i));
	G_free(buf);

	/* free memory allocated for null buffer */

	for (i = 0; i < nrows + 3; i++)
	    G_free(null_buf[i]);
	G_free(null_buf);

	G_free(rich);
    }
    else
	G_free(richtmp);

    return;
}






					/* CHECK BUFFER; RETURN 1 IF BUFFER
					   IS NOT EMPTY, 0 IF EMPTY */

int is_not_empty_buffer(DCELL ** buf, DCELL ** null_buf, int rows, int cols)
{

    register int i, j;

    /* if value in raster is positive or 
       negative, then it is not null; if
       value in raster is zero, and the 
       null value is 0.0, then the zero
       raster value is not null */

    for (i = 1; i < rows + 1; i++)
	for (j = 1; j < cols + 1; j++) {
	    if (buf[i][j])
		return (1);
	    else if (!buf[i][j] && null_buf[i][j] == 0.0)
		return (1);
	}
    return (0);

}



				/* CHECK TO SEE IF THE CENTER PIXEL
				   IN THE BUFFER IS NULL.  RETURN 1
				   IF IT IS NOT NULL, 0 IF IT IS NULL */

int center_is_not_null(DCELL ** buf, DCELL ** null_buf, int rows, int cols)
{

    /* if value in raster is positive or 
       negative, then it is not null; if
       value in raster is zero, and the 
       null value is 0.0, then the zero
       raster value is not null */

    if (buf[(rows / 2) + 1][(cols / 2) + 1] > -BIG) {
	return (1);
    }
    else if (!buf[(rows / 2) + 1][(cols / 2) + 1] &&
	     null_buf[(rows / 2) + 1][(cols / 2) + 1] == 0.0) {
	return (1);
    }
    return (0);

}




				/* OPEN THE RASTER FILE TO BE CLIPPED,
				   AND DO THE CLIPPING */

void cell_clip(DCELL ** buf, DCELL ** null_buf, int row0, int col0, int nrows,
	       int ncols, int index, float radius)
{
    CELL *tmp, *tmp1;
    FCELL *ftmp;
    DCELL *dtmp;
    char *tmpname, *nulltmp;
    int fr;
    register int i, j;
    double center_row = 0.0, center_col = 0.0;
    double dist;
    RASTER_MAP_TYPE data_type;

    /*
       Variables:
       IN:
       buf        = pointer to array containing only the pixels inside the area 
       that was specified to be clipped, so a smaller array than the
       original raster map
       null_buf   = pointer to array containing the corresponding null values
       row0       = starting row for the area to be clipped out of the raster map
       col0       = starting col for the area to be clipped out of the raster map
       nrows      = total number of rows in the area to be clipped
       ncols      = total number of cols in the area to be clipped
       index      = number of the region to be clipped, if there's a region map
       INTERNAL:
       tmp        = pointer to a temporary array to store a row of the raster map
       tmp1       = pointer to a temporary array to store a row of the region map
       fr         = return value from attempting to open the region map
       i, j       = indices to rows and cols of the arrays
     */

    data_type = Rast_map_type(choice->fn, G_mapset());

    /* if sampling by region was chosen, check
       for the region map and make sure it is
       an integer (CELL_TYPE) map */

    if (choice->wrum == 'r') {
	if (0 > (fr = Rast_open_old(choice->reg, G_mapset()))) {
	    fprintf(stderr, "\n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    fprintf(stderr,
		    "    You specified sam=r to request sampling by region,    \n");
	    fprintf(stderr,
		    "    but the region map specified with the 'reg=' parameter\n");
	    fprintf(stderr,
		    "    cannot be found in the current mapset.                \n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    exit(1);
	}
	if (Rast_map_type(choice->reg, G_mapset()) > 0) {
	    fprintf(stderr, "\n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    fprintf(stderr,
		    "    You specified sam=r to request sampling by region,    \n");
	    fprintf(stderr,
		    "    but the region map specified with the 'reg=' parameter\n");
	    fprintf(stderr,
		    "    must be an integer map, and it is floating point or   \n");
	    fprintf(stderr,
		    "    double instead.                                       \n");
	    fprintf(stderr,
		    "   *******************************************************\n");
	    exit(1);
	}
	tmp1 = Rast_allocate_buf(CELL_TYPE);
	Rast_zero_buf(tmp1, CELL_TYPE);
	fprintf(stderr, "Analyzing region number %d...\n", index);
    }

    /* allocate memory to store a row of the
       raster map, depending on the type of
       input raster map; keep track of the
       name of the buffer for each raster type */

    switch (data_type) {
    case CELL_TYPE:
	tmp = Rast_allocate_buf(CELL_TYPE);
	tmpname = "tmp";
	break;
    case FCELL_TYPE:
	ftmp = Rast_allocate_buf(FCELL_TYPE);
	tmpname = "ftmp";
	break;
    case DCELL_TYPE:
	dtmp = Rast_allocate_buf(DCELL_TYPE);
	tmpname = "dtmp";
	break;
    }

    /* allocate memory to store a row of the
       null values corresponding to the raster
       map */

    nulltmp = Rast_allocate_null_buf();

    /* if circles are used for sampling, then
       calculate the center of the area to be
       clipped, in pixels */

    if ((int)radius) {
	center_row = ((double)row0 + ((double)nrows - 1) / 2);
	center_col = ((double)col0 + ((double)ncols - 1) / 2);
    }

    /* for each row of the area to be clipped */

    for (i = row0; i < row0 + nrows; i++) {

	/* if region, read in the corresponding
	   map row in the region file */

	if (choice->wrum == 'r')
	    Rast_get_row_nomask(fr, tmp1, i, CELL_TYPE);

	/* initialize each element of the
	   row buffer to 0; this row buffer
	   will hold one row of the clipped
	   raster map.  Then read row i of the
	   map and the corresponding null values
	   into tmp and nulltmp buffers */

	switch (data_type) {
	case CELL_TYPE:
	    Rast_zero_buf(tmp, data_type);
	    Rast_get_row(finput, tmp, i, CELL_TYPE);
	    break;
	case FCELL_TYPE:
	    Rast_zero_buf(ftmp, data_type);
	    Rast_get_row(finput, ftmp, i, FCELL_TYPE);
	    break;
	case DCELL_TYPE:
	    Rast_zero_buf(dtmp, data_type);
	    Rast_get_row(finput, dtmp, i, DCELL_TYPE);
	    break;
	}

	Rast_get_null_value_row(finput, nulltmp, i);

	/* for all the columns one by one */

	for (j = col0; j < col0 + ncols; j++) {

	    /* if circles are used for sampling */

	    if ((int)radius) {
		dist = sqrt(((double)i - center_row) *
			    ((double)i - center_row) +
			    ((double)j - center_col) *
			    ((double)j - center_col));

		/* copy the contents of tmp into the
		   appropriate cell in buf */

		if (dist < radius) {
		    switch (data_type) {
		    case CELL_TYPE:
			*(*(buf + i + 1 - row0) + j + 1 - col0) = *(tmp + j);
			break;
		    case FCELL_TYPE:
			*(*(buf + i + 1 - row0) + j + 1 - col0) = *(ftmp + j);
			break;
		    case DCELL_TYPE:
			*(*(buf + i + 1 - row0) + j + 1 - col0) = *(dtmp + j);
			break;
		    }
		    *(*(null_buf + i + 1 - row0) + j + 1 - col0) =
			*(nulltmp + j);
		}
	    }

	    /* if circles are not used and
	       if the choice is not "by region" or
	       if this column is in region "index" */

	    else if (choice->wrum != 'r' || *(tmp1 + j) == index) {

		/* copy the contents of the correct tmp
		   into the appropriate cell in the buf
		   and the corresponding null values into
		   the appropriate cell in null_buf */

		switch (data_type) {
		case CELL_TYPE:
		    *(*(buf + i + 1 - row0) + j + 1 - col0) = *(tmp + j);
		    break;
		case FCELL_TYPE:
		    *(*(buf + i + 1 - row0) + j + 1 - col0) = *(ftmp + j);
		    break;
		case DCELL_TYPE:
		    *(*(buf + i + 1 - row0) + j + 1 - col0) = *(dtmp + j);
		    break;
		}
		*(*(null_buf + i + 1 - row0) + j + 1 - col0) = *(nulltmp + j);
	    }
	}
    }

    switch (data_type) {
    case CELL_TYPE:
	G_free(tmp);
	break;
    case FCELL_TYPE:
	G_free(ftmp);
	break;
    case DCELL_TYPE:
	G_free(dtmp);
	break;
    }
    if (choice->wrum == 'r') {
	G_free(tmp1);
	Rast_close(fr);
    }
    G_free(nulltmp);
    return;
}





					/* FIND UNCOUNTED ATTRIBUTES,
					   COUNT THEM UP, AND ADD THEM TO
					   THE RICHNESS ARRAY IN UNSORTED
					   ORDER */

void get_rich(double att, double rich[], int *cnt)
{
    register int i;

    /* if this attribute is already
       in the richness array, then
       return */

    for (i = 0; i < *cnt; i++) {
	if (att == rich[i]) {
	    break;
	}
    }
    /* if this attribute is not already
       in the richness array, then make
       it the "cnt" element of the
       array, then increment the cnt */

    if (i >= *cnt) {
	rich[*cnt] = att;
	/* fprintf(stderr, "cnt=%d i=%d att=%f\n",*cnt,i,att); */
	++(*cnt);
    }
    return;
}





					/* COMPARE */

int compar(int *i, int *j)
{
    return (*i - *j);
}
