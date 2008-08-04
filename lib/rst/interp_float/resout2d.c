
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Summer 1993
 * University of Illinois
 * US Army Construction Engineering Research Lab  
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)   
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995  
 *
 */

#define MULT 100000

#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>

#include <grass/interpf.h>


/* output cell maps for elevation, aspect, slope and curvatures */

int IL_resample_output_2d(struct interp_params *params, double zmin, double zmax,	/* min,max input z-values */
			  double zminac, double zmaxac,	/* min,max interpolated values */
			  double c1min, double c1max, double c2min, double c2max, double gmin, double gmax, double ertot,	/* total interplating func. error */
			  char *input,	/* input file name */
			  double *dnorm, struct Cell_head *outhd,	/* Region with desired resolution */
			  struct Cell_head *winhd,	/* Current region */
			  char *smooth, int n_points)

/*
 * Creates output files as well as history files  and color tables for
 * them.
 */
{
    FCELL *cell1;		/* cell buffer */
    int cf1 = 0, cf2 = 0, cf3 = 0, cf4 = 0, cf5 = 0, cf6 = 0;	/* cell file descriptors */
    int nrows, ncols;		/* current region rows and columns */
    int i;			/* loop counter */
    char *mapset;
    float dat1, dat2;
    struct Colors colors, colors2;
    double value1, value2;
    struct History hist, hist1, hist2, hist3, hist4, hist5;
    struct _Color_Rule_ *rule;
    char *maps, *type;
    int cond1, cond2;

    cond2 = ((params->pcurv != NULL) ||
	     (params->tcurv != NULL) || (params->mcurv != NULL));
    cond1 = ((params->slope != NULL) || (params->aspect != NULL) || cond2);

    /* change region to output cell file region */
    fprintf(stderr,
	    "Temporarily changing the region to desired resolution...\n");
    if (G_set_window(outhd) < 0) {
	fprintf(stderr, "Cannot set region to output region!\n");
	return -1;
    }
    mapset = G_mapset();

    cell1 = G_allocate_f_raster_buf();

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

    nrows = outhd->rows;
    if (nrows != params->nsizr) {
	fprintf(stderr, "first change your rows number(%d) to %d!\n",
		nrows, params->nsizr);
	return -1;
    }

    ncols = outhd->cols;
    if (ncols != params->nsizc) {
	fprintf(stderr, "first change your rows number(%d) to %d!\n",
		ncols, params->nsizc);
	return -1;
    }

    if (params->elev != NULL) {
	fseek(params->Tmp_fd_z, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_z, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0) == -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_z);
	    if (G_put_f_raster_row(cf1, cell1) < 0) {
		fprintf(stderr, "cannot write file\n");
		return -1;
	    }
	}
    }

    if (params->slope != NULL) {
	fseek(params->Tmp_fd_dx, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_dx, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0) == -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_dx);
	    /*
	     * for (ii==0;ii<params->nsizc;ii++) { fprintf(stderr,"ii=%d ",ii);
	     * fprintf(stderr,"%f ",cell1[ii]); }
	     * fprintf(stderr,"params->nsizc=%d \n",params->nsizc);
	     */
	    if (G_put_f_raster_row(cf2, cell1) < 0) {
		fprintf(stderr, "cannot write file\n");
		return -1;
	    }
	}
    }

    if (params->aspect != NULL) {
	fseek(params->Tmp_fd_dy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_dy, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0) == -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_dy);
	    if (G_put_f_raster_row(cf3, cell1) < 0) {
		fprintf(stderr, "cannot write file\n");
		return -1;
	    }
	}
    }

    if (params->pcurv != NULL) {
	fseek(params->Tmp_fd_xx, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_xx, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0) == -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_xx);
	    if (G_put_f_raster_row(cf4, cell1) < 0) {
		fprintf(stderr, "cannot write file\n");
		return -1;
	    }
	}
    }

    if (params->tcurv != NULL) {
	fseek(params->Tmp_fd_yy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_yy, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0) == -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_yy);
	    if (G_put_f_raster_row(cf5, cell1) < 0) {
		fprintf(stderr, "cannot write file\n");
		return -1;
	    }
	}
    }

    if (params->mcurv != NULL) {
	fseek(params->Tmp_fd_xy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    if (fseek(params->Tmp_fd_xy, (long)
		      ((params->nsizr - 1 -
			i) * params->nsizc * sizeof(FCELL)), 0) == -1) {
		fprintf(stderr, "cannot fseek to the right spot\n");
		return -1;
	    }
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_xy);
	    if (G_put_f_raster_row(cf6, cell1) < 0) {
		fprintf(stderr, "cannot write file\n");
		return -1;
	    }
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

    /* write colormaps and history for output cell files */
    /* colortable for elevations */
    maps = G_find_file("cell", input, "");

    if (params->elev != NULL) {
	if (maps == NULL) {
	    fprintf(stderr, "file [%s] not found\n", input);
	    return -1;
	}
	G_init_colors(&colors2);
	/*
	 * G_mark_colors_as_fp(&colors2);
	 */

	if (G_read_colors(input, maps, &colors) >= 0) {
	    if (colors.modular.rules) {
		rule = colors.modular.rules;

		while (rule->next)
		    rule = rule->next;

		for (; rule; rule = rule->prev) {
		    value1 = rule->low.value * params->zmult;
		    value2 = rule->high.value * params->zmult;
		    G_add_modular_d_raster_color_rule(&value1, rule->low.red,
						      rule->low.grn,
						      rule->low.blu, &value2,
						      rule->high.red,
						      rule->high.grn,
						      rule->high.blu,
						      &colors2);
		}
	    }

	    if (colors.fixed.rules) {
		rule = colors.fixed.rules;

		while (rule->next)
		    rule = rule->next;

		for (; rule; rule = rule->prev) {
		    value1 = rule->low.value * params->zmult;
		    value2 = rule->high.value * params->zmult;
		    G_add_d_raster_color_rule(&value1, rule->low.red,
					      rule->low.grn, rule->low.blu,
					      &value2, rule->high.red,
					      rule->high.grn, rule->high.blu,
					      &colors2);
		}
	    }

	    maps = NULL;
	    maps = G_find_file("cell", params->elev, "");
	    if (maps == NULL) {
		fprintf(stderr, "file [%s] not found\n", params->elev);
		return -1;
	    }

	    if (G_write_colors(params->elev, maps, &colors2) < 0) {
		fprintf(stderr, "Cannot write color table\n");
		return -1;
	    }
	    G_quantize_fp_map_range(params->elev, mapset,
				    zminac - 0.5, zmaxac + 0.5,
				    (CELL) (zminac - 0.5),
				    (CELL) (zmaxac + 0.5));
	}
	else
	    fprintf(stderr,
		    "No color table for input file -- will not create color table\n");
    }

    /* colortable for slopes */
    if (cond1 & (!params->deriv)) {
	G_init_colors(&colors);
	G_add_color_rule(0, 255, 255, 255, 2, 255, 255, 0, &colors);
	G_add_color_rule(2, 255, 255, 0, 5, 0, 255, 0, &colors);
	G_add_color_rule(5, 0, 255, 0, 10, 0, 255, 255, &colors);
	G_add_color_rule(10, 0, 255, 255, 15, 0, 0, 255, &colors);
	G_add_color_rule(15, 0, 0, 255, 30, 255, 0, 255, &colors);
	G_add_color_rule(30, 255, 0, 255, 50, 255, 0, 0, &colors);
	G_add_color_rule(50, 255, 0, 0, 90, 0, 0, 0, &colors);

	if (params->slope != NULL) {
	    maps = NULL;
	    maps = G_find_file("cell", params->slope, "");
	    if (maps == NULL) {
		fprintf(stderr, "file [%s] not found\n", params->slope);
		return -1;
	    }
	    G_write_colors(params->slope, maps, &colors);
	    G_quantize_fp_map_range(params->slope, mapset, 0., 90., 0, 90);

	    type = "raster";
	    G_short_history(params->slope, type, &hist1);
	    if (params->elev != NULL)
		sprintf(hist1.edhist[0], "The elevation map is %s",
			params->elev);

	    sprintf(hist1.datsrc_1, "raster map %s", input);
	    hist1.edlinecnt = 1;

	    G_write_history(params->slope, &hist1);
	}

	/* colortable for aspect */
	G_init_colors(&colors);
	G_add_color_rule(0, 255, 255, 255, 0, 255, 255, 255, &colors);
	G_add_color_rule(1, 255, 255, 0, 90, 0, 255, 0, &colors);
	G_add_color_rule(90, 0, 255, 0, 180, 0, 255, 255, &colors);
	G_add_color_rule(180, 0, 255, 255, 270, 255, 0, 0, &colors);
	G_add_color_rule(270, 255, 0, 0, 360, 255, 255, 0, &colors);

	if (params->aspect != NULL) {
	    maps = NULL;
	    maps = G_find_file("cell", params->aspect, "");
	    if (maps == NULL) {
		fprintf(stderr, "file [%s] not found\n", params->aspect);
		return -1;
	    }
	    G_write_colors(params->aspect, maps, &colors);
	    G_quantize_fp_map_range(params->aspect, mapset, 0., 360., 0, 360);

	    type = "raster";
	    G_short_history(params->aspect, type, &hist2);
	    if (params->elev != NULL)
		sprintf(hist2.edhist[0], "The elevation map is %s",
			params->elev);

	    sprintf(hist2.datsrc_1, "raster map %s", input);
	    hist2.edlinecnt = 1;

	    G_write_history(params->aspect, &hist2);
	}

	/* colortable for curvatures */
	if (cond2) {
	    G_init_colors(&colors);

	    dat1 = (FCELL) amin1(c1min, c2min);
	    dat2 = (FCELL) - 0.01;

	    G_add_f_raster_color_rule(&dat1, 50, 0, 155,
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
	    dat2 = (FCELL) 0.00;
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
	    dat2 = (FCELL) amax1(c1max, c2max);
	    G_add_f_raster_color_rule(&dat1, 255, 0, 0,
				      &dat2, 155, 0, 20, &colors);
	    maps = NULL;
	    if (params->pcurv != NULL) {
		maps = G_find_file("cell", params->pcurv, "");
		if (maps == NULL) {
		    fprintf(stderr, "file [%s] not found\n", params->pcurv);
		    return -1;
		}
		G_write_colors(params->pcurv, maps, &colors);

		fprintf(stderr, "color map written\n");

		G_quantize_fp_map_range(params->pcurv, mapset,
					dat1, dat2,
					(CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));
		type = "raster";
		G_short_history(params->pcurv, type, &hist3);
		if (params->elev != NULL)
		    sprintf(hist3.edhist[0], "The elevation map is %s",
			    params->elev);

		sprintf(hist3.datsrc_1, "raster map %s", input);
		hist3.edlinecnt = 1;

		G_write_history(params->pcurv, &hist3);
	    }

	    if (params->tcurv != NULL) {
		maps = NULL;
		maps = G_find_file("cell", params->tcurv, "");
		if (maps == NULL) {
		    fprintf(stderr, "file [%s] not found\n", params->tcurv);
		    return -1;
		}
		G_write_colors(params->tcurv, maps, &colors);
		G_quantize_fp_map_range(params->tcurv, mapset,
					dat1, dat2, (CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));

		type = "raster";
		G_short_history(params->tcurv, type, &hist4);
		if (params->elev != NULL)
		    sprintf(hist4.edhist[0], "The elevation map is %s",
			    params->elev);

		sprintf(hist4.datsrc_1, "raster map %s", input);
		hist4.edlinecnt = 1;

		G_write_history(params->tcurv, &hist4);
	    }

	    if (params->mcurv != NULL) {
		maps = NULL;
		maps = G_find_file("cell", params->mcurv, "");
		if (maps == NULL) {
		    fprintf(stderr, "file [%s] not found\n", params->mcurv);
		    return -1;
		}
		G_write_colors(params->mcurv, maps, &colors);
		G_quantize_fp_map_range(params->mcurv, mapset,
					dat1, dat2,
					(CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));

		type = "raster";
		G_short_history(params->mcurv, type, &hist5);
		if (params->elev != NULL)
		    sprintf(hist5.edhist[0], "The elevation map is %s",
			    params->elev);

		sprintf(hist5.datsrc_1, "raster map %s", input);
		hist5.edlinecnt = 1;

		G_write_history(params->mcurv, &hist5);
	    }
	}
    }

    if (params->elev != NULL) {
	maps = G_find_file("cell", params->elev, "");
	if (maps == NULL) {
	    fprintf(stderr, "file [%s] not found \n", params->elev);
	    return -1;
	}
	G_short_history(params->elev, "raster", &hist);

	if (smooth != NULL)
	    sprintf(hist.edhist[0], "tension=%f, smoothing=%s",
		    params->fi * 1000. / (*dnorm), smooth);
	else
	    sprintf(hist.edhist[0], "tension=%f",
		    params->fi * 1000. / (*dnorm));
	sprintf(hist.edhist[1], "dnorm=%f, zmult=%f", *dnorm, params->zmult);
	sprintf(hist.edhist[2], "KMAX=%d, KMIN=%d, errtotal=%f", params->kmax,
		params->kmin, sqrt(ertot / n_points));
	sprintf(hist.edhist[3], "zmin_data=%f, zmax_data=%f", zmin, zmax);
	sprintf(hist.edhist[4], "zmin_int=%f, zmax_int=%f", zminac, zmaxac);

	sprintf(hist.datsrc_1, "raster map %s", input);

	hist.edlinecnt = 5;

	G_write_history(params->elev, &hist);
    }

    /* change region to initial region */
    fprintf(stderr, "Changing the region back to initial...\n");
    if (G_set_window(winhd) < 0) {
	fprintf(stderr, "Cannot set region to back to initial region!\n");
	return -1;
    }

    return 1;
}
