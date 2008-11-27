
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Summer 1992
 * Copyright 1992, H. Mitasova 
 * I. Kosinovsky,  and D.Gerdes    
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995  
 * modified by Mitasova in August 1999 (fix for elev color)
 * modified by Brown in September 1999 (fix for Timestamps)
 * Modified by Mitasova in Nov. 1999 (write given tension into hist)
 * Last modification: 2006-12-13
 *
 */

#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>

#include <grass/interpf.h>


#define MULT 100000


int IL_output_2d(struct interp_params *params, struct Cell_head *cellhd,	/* current region */
		 double zmin, double zmax,	/* min,max input z-values */
		 double zminac, double zmaxac, double c1min, double c1max,	/* min,max interpolated values */
		 double c2min, double c2max, double gmin, double gmax, double ertot,	/* total interplating func. error */
		 char *input,	/* input file name */
		 double dnorm, int dtens, int vect, int n_points)

/*
 * Creates output files as well as history files  and color tables for
 * them.
 */
{
    FCELL *cell1;
    int cf1 = 0, cf2 = 0, cf3 = 0, cf4 = 0, cf5 = 0, cf6 = 0;
    int nrows, ncols;
    int i, ii;
    double zstep;
    FCELL data1, data2;
    struct Colors colors;
    struct History hist, hist1, hist2, hist3, hist4, hist5;
    char *type;
    const char *mapset = NULL;
    int cond1, cond2;
    FCELL dat1, dat2;

    cond2 = ((params->pcurv != NULL) || (params->tcurv != NULL)
	     || (params->mcurv != NULL));
    cond1 = ((params->slope != NULL) || (params->aspect != NULL) || cond2);

    cell1 = G_allocate_f_raster_buf();

    /*
     * G_set_embedded_null_value_mode(1);
     */
    if (params->elev != NULL) {
	cf1 = G_open_fp_cell_new(params->elev);
	if (cf1 < 0) {
	    fprintf(stderr, "unable to create raster map %s\n", params->elev);
	    return -1;
	}
    }

    if (params->slope != NULL) {
	cf2 = G_open_fp_cell_new(params->slope);
	if (cf2 < 0) {
	    fprintf(stderr, "unable to create raster map %s\n",
		    params->slope);
	    return -1;
	}
    }

    if (params->aspect != NULL) {
	cf3 = G_open_fp_cell_new(params->aspect);
	if (cf3 < 0) {
	    fprintf(stderr, "unable to create raster map %s\n",
		    params->aspect);
	    return -1;
	}
    }

    if (params->pcurv != NULL) {
	cf4 = G_open_fp_cell_new(params->pcurv);
	if (cf4 < 0) {
	    fprintf(stderr, "unable to create raster map %s\n",
		    params->pcurv);
	    return -1;
	}
    }

    if (params->tcurv != NULL) {
	cf5 = G_open_fp_cell_new(params->tcurv);
	if (cf5 < 0) {
	    fprintf(stderr, "unable to create raster map %s\n",
		    params->tcurv);
	    return -1;
	}
    }

    if (params->mcurv != NULL) {
	cf6 = G_open_fp_cell_new(params->mcurv);
	if (cf6 < 0) {
	    fprintf(stderr, "unable to create raster map %s\n",
		    params->mcurv);
	    return -1;
	}
    }

    nrows = cellhd->rows;
    if (nrows != params->nsizr) {
	fprintf(stderr, "first change your rows number to nsizr! %d %d\n",
		nrows, params->nsizr);
	return -1;
    }

    ncols = cellhd->cols;
    if (ncols != params->nsizc) {
	fprintf(stderr, "first change your cols number to nsizc! %d %d\n",
		ncols, params->nsizc);
	return -1;
    }

    if (G_set_window(cellhd) < 0)
	return -1;

    if (nrows != G_window_rows()) {
	fprintf(stderr, "OOPS: rows changed from %d to %d\n", nrows,
		G_window_rows());
	return -1;
    }

    if (ncols != G_window_cols()) {
	fprintf(stderr, "OOPS: cols changed from %d to %d\n", ncols,
		G_window_cols());
	return -1;
    }

    if (params->elev != NULL) {
	fseek(params->Tmp_fd_z, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_z, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0)
		== -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    ii = fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_z);
	    /*
	     * for(j=0;j<params->nsizc;j++) fprintf(stderr,"%f ",cell1[j]);
	     * fprintf(stderr,"\n");
	     */
	    G_put_f_raster_row(cf1, cell1);

	}
    }

    if (params->slope != NULL) {
	fseek(params->Tmp_fd_dx, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_dx, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0)
		== -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_dx);
	    G_put_f_raster_row(cf2, cell1);
	}
    }

    if (params->aspect != NULL) {
	fseek(params->Tmp_fd_dy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_dy, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0)
		== -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_dy);
	    G_put_f_raster_row(cf3, cell1);
	}
    }

    if (params->pcurv != NULL) {
	fseek(params->Tmp_fd_xx, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_xx, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0)
		== -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_xx);
	    G_put_f_raster_row(cf4, cell1);
	}
    }

    if (params->tcurv != NULL) {
	fseek(params->Tmp_fd_yy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_yy, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0)
		== -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_yy);
	    G_put_f_raster_row(cf5, cell1);
	}
    }

    if (params->mcurv != NULL) {
	fseek(params->Tmp_fd_xy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_xy, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0)
		== -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_xy);
	    G_put_f_raster_row(cf6, cell1);
	}
    }

    if (cf1)
	G_close_cell(cf1);
    if (cf2)
	G_close_cell(cf2);
    if (cf3)
	G_close_cell(cf3);
    if (cf4)
	G_close_cell(cf4);
    if (cf5)
	G_close_cell(cf5);
    if (cf6)
	G_close_cell(cf6);


    /* colortable for elevations */
    G_init_colors(&colors);
    zstep = (FCELL) (zmaxac - zminac) / 5.;
    for (i = 1; i <= 5; i++) {
	data1 = (FCELL) (zminac + (i - 1) * zstep);
	data2 = (FCELL) (zminac + i * zstep);
	switch (i) {
	case 1:
	    G_add_f_raster_color_rule(&data1, 0, 191, 191,
				      &data2, 0, 255, 0, &colors);
	    break;
	case 2:
	    G_add_f_raster_color_rule(&data1, 0, 255, 0,
				      &data2, 255, 255, 0, &colors);
	    break;
	case 3:
	    G_add_f_raster_color_rule(&data1, 255, 255, 0,
				      &data2, 255, 127, 0, &colors);
	    break;
	case 4:
	    G_add_f_raster_color_rule(&data1, 255, 127, 0,
				      &data2, 191, 127, 63, &colors);
	    break;
	case 5:
	    G_add_f_raster_color_rule(&data1, 191, 127, 63,
				      &data2, 20, 20, 20, &colors);
	    break;
	}
    }

    if (params->elev != NULL) {
	mapset = G_find_file("cell", params->elev, "");
	if (mapset == NULL) {
	    fprintf(stderr, "file [%s] not found\n", params->elev);
	    return -1;
	}
	G_write_colors(params->elev, mapset, &colors);
	G_quantize_fp_map_range(params->elev, mapset,
				(DCELL) zminac - 0.5, (DCELL) zmaxac + 0.5,
				(CELL) (zminac - 0.5), (CELL) (zmaxac + 0.5));
    }

    /* colortable for slopes */
    if (cond1) {
	if (!params->deriv) {
	    /*
	     * smin = (CELL) ((int)(gmin*scig)); smax = (CELL) gmax; fprintf
	     * (stderr, "min %d max %d \n", smin,smax); G_make_rainbow_colors
	     * (&colors,smin,smax);
	     */
	    G_init_colors(&colors);
	    G_add_color_rule(0, 255, 255, 255, 2, 255, 255, 0, &colors);
	    G_add_color_rule(2, 255, 255, 0, 5, 0, 255, 0, &colors);
	    G_add_color_rule(5, 0, 255, 0, 10, 0, 255, 255, &colors);
	    G_add_color_rule(10, 0, 255, 255, 15, 0, 0, 255, &colors);
	    G_add_color_rule(15, 0, 0, 255, 30, 255, 0, 255, &colors);
	    G_add_color_rule(30, 255, 0, 255, 50, 255, 0, 0, &colors);
	    G_add_color_rule(50, 255, 0, 0, 90, 0, 0, 0, &colors);
	}
	else {
	    G_init_colors(&colors);
	    dat1 = (FCELL) - 5.0;	/* replace by min dx, amin1 (c1min,
					 * c2min); */
	    dat2 = (FCELL) - 0.1;
	    G_add_f_raster_color_rule(&dat1, 127, 0, 255,
				      &dat2, 0, 0, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.01;
	    G_add_f_raster_color_rule(&dat1, 0, 0, 255,
				      &dat2, 0, 127, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.001;
	    G_add_f_raster_color_rule(&dat1, 0, 127, 255,
				      &dat2, 0, 255, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.0;
	    G_add_f_raster_color_rule(&dat1, 0, 255, 255,
				      &dat2, 200, 255, 200, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.001;
	    G_add_f_raster_color_rule(&dat1, 200, 255, 200,
				      &dat2, 255, 255, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.01;
	    G_add_f_raster_color_rule(&dat1, 255, 255, 0,
				      &dat2, 255, 127, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.1;
	    G_add_f_raster_color_rule(&dat1, 255, 127, 0,
				      &dat2, 255, 0, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 5.0;	/* replace by max dx, amax1 (c1max,
				 * c2max); */
	    G_add_f_raster_color_rule(&dat1, 255, 0, 0,
				      &dat2, 255, 0, 200, &colors);
	}

	if (params->slope != NULL) {
	    mapset = G_find_file("cell", params->slope, "");
	    if (mapset == NULL) {
		fprintf(stderr, "file [%s] not found\n", params->slope);
		return -1;
	    }
	    G_write_colors(params->slope, mapset, &colors);
	    G_quantize_fp_map_range(params->slope, mapset, 0., 90., 0, 90);

	    type = "raster";
	    G_short_history(params->slope, type, &hist1);
	    if (params->elev != NULL)
		sprintf(hist1.edhist[0], "The elevation map is %s",
			params->elev);
	    if (vect)
		sprintf(hist1.datsrc_1, "vector map %s", input);
	    else
		sprintf(hist1.datsrc_1, "site file %s", input);
	    hist1.edlinecnt = 1;

	    G_command_history(&hist1);
	    G_write_history(params->slope, &hist1);
	    if (params->ts)
		G_write_raster_timestamp(params->slope, params->ts);

	}

	/* colortable for aspect */
	if (!params->deriv) {
	    G_init_colors(&colors);
	    G_add_color_rule(0, 255, 255, 255, 0, 255, 255, 255, &colors);
	    G_add_color_rule(1, 255, 255, 0, 90, 0, 255, 0, &colors);
	    G_add_color_rule(90, 0, 255, 0, 180, 0, 255, 255, &colors);
	    G_add_color_rule(180, 0, 255, 255, 270, 255, 0, 0, &colors);
	    G_add_color_rule(270, 255, 0, 0, 360, 255, 255, 0, &colors);
	}
	else {
	    G_init_colors(&colors);
	    dat1 = (FCELL) - 5.0;	/* replace by min dy, amin1 (c1min,
					 * c2min); */
	    dat2 = (FCELL) - 0.1;
	    G_add_f_raster_color_rule(&dat1, 127, 0, 255,
				      &dat2, 0, 0, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.01;
	    G_add_f_raster_color_rule(&dat1, 0, 0, 255,
				      &dat2, 0, 127, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.001;
	    G_add_f_raster_color_rule(&dat1, 0, 127, 255,
				      &dat2, 0, 255, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.0;
	    G_add_f_raster_color_rule(&dat1, 0, 255, 255,
				      &dat2, 200, 255, 200, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.001;
	    G_add_f_raster_color_rule(&dat1, 200, 255, 200,
				      &dat2, 255, 255, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.01;
	    G_add_f_raster_color_rule(&dat1, 255, 255, 0,
				      &dat2, 255, 127, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.1;
	    G_add_f_raster_color_rule(&dat1, 255, 127, 0,
				      &dat2, 255, 0, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 5.0;	/* replace by max dy, amax1 (c1max,
				 * c2max); */
	    G_add_f_raster_color_rule(&dat1, 255, 0, 0,
				      &dat2, 255, 0, 200, &colors);
	}

	if (params->aspect != NULL) {
	    mapset = G_find_file("cell", params->aspect, "");
	    if (mapset == NULL) {
		fprintf(stderr, "file [%s] not found\n", params->aspect);
		return -1;
	    }
	    G_write_colors(params->aspect, mapset, &colors);
	    G_quantize_fp_map_range(params->aspect, mapset, 0., 360., 0, 360);

	    type = "raster";
	    G_short_history(params->aspect, type, &hist2);
	    if (params->elev != NULL)
		sprintf(hist2.edhist[0], "The elevation map is %s",
			params->elev);
	    if (vect)
		sprintf(hist2.datsrc_1, "vector map %s", input);
	    else
		sprintf(hist2.datsrc_1, "site file %s", input);
	    hist2.edlinecnt = 1;

	    G_command_history(&hist2);
	    G_write_history(params->aspect, &hist2);
	    if (params->ts)
		G_write_raster_timestamp(params->aspect, params->ts);
	}

	/* colortable for curvatures */
	if (cond2) {
	    G_init_colors(&colors);
	    dat1 = (FCELL) amin1(c1min, c2min);	/* for derivatives use min
						 * dxx,dyy,dxy */
	    dat2 = (FCELL) - 0.01;
	    G_add_f_raster_color_rule(&dat1, 127, 0, 255,
				      &dat2, 0, 0, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.001;
	    G_add_f_raster_color_rule(&dat1, 0, 0, 255,
				      &dat2, 0, 127, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.00001;
	    G_add_f_raster_color_rule(&dat1, 0, 127, 255,
				      &dat2, 0, 255, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.0;
	    G_add_f_raster_color_rule(&dat1, 0, 255, 255,
				      &dat2, 200, 255, 200, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.00001;
	    G_add_f_raster_color_rule(&dat1, 200, 255, 200,
				      &dat2, 255, 255, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.001;
	    G_add_f_raster_color_rule(&dat1, 255, 255, 0,
				      &dat2, 255, 127, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.01;
	    G_add_f_raster_color_rule(&dat1, 255, 127, 0,
				      &dat2, 255, 0, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) amax1(c1max, c2max);	/* for derivatives use max
						 * dxx,dyy,dxy */
	    G_add_f_raster_color_rule(&dat1, 255, 0, 0,
				      &dat2, 255, 0, 200, &colors);

	    if (params->pcurv != NULL) {
		mapset = G_find_file("cell", params->pcurv, "");
		if (mapset == NULL) {
		    fprintf(stderr, "file [%s] not found\n", params->pcurv);
		    return -1;
		}
		G_write_colors(params->pcurv, mapset, &colors);
		G_quantize_fp_map_range(params->pcurv, mapset, dat1, dat2,
					(CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));

		type = "raster";
		G_short_history(params->pcurv, type, &hist3);
		if (params->elev != NULL)
		    sprintf(hist3.edhist[0], "The elevation map is %s",
			    params->elev);
		if (vect)
		    sprintf(hist3.datsrc_1, "vector map %s", input);
		else
		    sprintf(hist3.datsrc_1, "site file %s", input);
		hist3.edlinecnt = 1;

		G_command_history(&hist3);
		G_write_history(params->pcurv, &hist3);
		if (params->ts)
		    G_write_raster_timestamp(params->pcurv, params->ts);
	    }

	    if (params->tcurv != NULL) {
		mapset = G_find_file("cell", params->tcurv, "");
		if (mapset == NULL) {
		    fprintf(stderr, "file [%s] not found\n", params->tcurv);
		    return -1;
		}
		G_write_colors(params->tcurv, mapset, &colors);
		G_quantize_fp_map_range(params->tcurv, mapset, dat1, dat2,
					(CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));

		type = "raster";
		G_short_history(params->tcurv, type, &hist4);
		if (params->elev != NULL)
		    sprintf(hist4.edhist[0], "The elevation map is %s",
			    params->elev);
		if (vect)
		    sprintf(hist4.datsrc_1, "vector map %s", input);
		else
		    sprintf(hist4.datsrc_1, "site file %s", input);
		hist4.edlinecnt = 1;

		G_command_history(&hist4);
		G_write_history(params->tcurv, &hist4);
		if (params->ts)
		    G_write_raster_timestamp(params->tcurv, params->ts);
	    }

	    if (params->mcurv != NULL) {
		mapset = G_find_file("cell", params->mcurv, "");
		if (mapset == NULL) {
		    fprintf(stderr, "file [%s] not found\n", params->mcurv);
		    return -1;
		}
		G_write_colors(params->mcurv, mapset, &colors);
		G_quantize_fp_map_range(params->mcurv, mapset, dat1, dat2,
					(CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));

		type = "raster";
		G_short_history(params->mcurv, type, &hist5);
		if (params->elev != NULL)
		    sprintf(hist5.edhist[0], "The elevation map is %s",
			    params->elev);
		if (vect)
		    sprintf(hist5.datsrc_1, "vector map %s", input);
		else
		    sprintf(hist5.datsrc_1, "site file %s", input);
		hist5.edlinecnt = 1;

		G_command_history(&hist5);
		G_write_history(params->mcurv, &hist5);
		if (params->ts)
		    G_write_raster_timestamp(params->mcurv, params->ts);
	    }
	}
    }

    if (params->elev != NULL) {
	mapset = G_find_file("cell", params->elev, "");
	if (mapset == NULL) {
	    fprintf(stderr, "file [%s] not found\n", params->elev);
	    return -1;
	}
	type = "raster";
	G_short_history(params->elev, type, &hist);

	params->dmin = sqrt(params->dmin);
	fprintf(stdout, "history initiated\n");
	fflush(stdout);
	/*
	 * sprintf (hist.edhist[0], "tension=%f, smoothing=%f", params->fi *
	 * dnorm / 1000., params->rsm);
	 */

	if (dtens) {
	    if (params->rsm == -1)
		sprintf(hist.edhist[0], "giventension=%f, smoothing att=%d",
			params->fi * 1000. / dnorm, params->smatt);
	    else
		sprintf(hist.edhist[0], "giventension=%f, smoothing=%f",
			params->fi * 1000. / dnorm, params->rsm);
	}
	else {
	    if (params->rsm == -1)
		sprintf(hist.edhist[0], "tension=%f, smoothing att=%d",
			params->fi * 1000. / dnorm, params->smatt);
	    else
		sprintf(hist.edhist[0], "tension=%f, smoothing=%f",
			params->fi, params->rsm);
	}

	sprintf(hist.edhist[1], "dnorm=%f, dmin=%f, zmult=%f",
		dnorm, params->dmin, params->zmult);
	/*
	 * sprintf(hist.edhist[2], "segmax=%d, npmin=%d, errtotal=%f",
	 * params->kmax,params->kmin,ertot);
	 */
	/*
	 * sprintf (hist.edhist[2], "segmax=%d, npmin=%d, errtotal =%f",
	 * params->kmax, params->kmin, sqrt (ertot) / n_points);
	 */

	sprintf(hist.edhist[2], "segmax=%d, npmin=%d, rmsdevi=%f",
		params->kmax, params->kmin, sqrt(ertot / n_points));

	sprintf(hist.edhist[3], "zmin_data=%f, zmax_data=%f", zmin, zmax);
	sprintf(hist.edhist[4], "zmin_int=%f, zmax_int=%f", zminac, zmaxac);

	if ((params->theta) && (params->scalex)) {
	    sprintf(hist.edhist[5], "theta=%f, scalex=%f", params->theta,
		    params->scalex);
	    hist.edlinecnt = 6;
	}
	else
	    hist.edlinecnt = 5;

	if (vect)
	    sprintf(hist.datsrc_1, "vector map %s", input);
	else
	    sprintf(hist.datsrc_1, "site file %s", input);

	G_command_history(&hist);
	G_write_history(params->elev, &hist);
	if (params->ts)
	    G_write_raster_timestamp(params->elev, params->ts);
    }

    /*
     * if (title) G_put_cell_title (output, title);
     */
    return 1;
}
