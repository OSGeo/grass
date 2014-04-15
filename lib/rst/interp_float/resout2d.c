
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
#include <grass/raster.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>
#include <grass/interpf.h>
#include <grass/glocale.h>

/* output cell maps for elevation, aspect, slope and curvatures */

static void do_history(const char *name, const char *input,
		       const struct interp_params *params)
{
    struct History hist;

    Rast_short_history(name, "raster", &hist);
    if (params->elev)
	Rast_append_format_history(&hist, "The elevation map is %s",
				   params->elev);

    Rast_format_history(&hist, HIST_DATSRC_1, "raster map %s", input);

    Rast_write_history(name, &hist);

    Rast_free_history(&hist);
}

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
    const char *mapset;
    float dat1, dat2;
    struct Colors colors, colors2;
    double value1, value2;
    struct History hist;
    struct _Color_Rule_ *rule;
    const char *maps;
    int cond1, cond2;
    CELL val1, val2;
    
    cond2 = ((params->pcurv != NULL) ||
	     (params->tcurv != NULL) || (params->mcurv != NULL));
    cond1 = ((params->slope != NULL) || (params->aspect != NULL) || cond2);

    /* change region to output cell file region */
    G_verbose_message(_("Temporarily changing the region to desired resolution..."));
    Rast_set_output_window(outhd);
    mapset = G_mapset();

    cell1 = Rast_allocate_f_output_buf();

    if (params->elev)
	cf1 = Rast_open_fp_new(params->elev);

    if (params->slope)
	cf2 = Rast_open_fp_new(params->slope);

    if (params->aspect)
	cf3 = Rast_open_fp_new(params->aspect);

    if (params->pcurv)
	cf4 = Rast_open_fp_new(params->pcurv);

    if (params->tcurv)
	cf5 = Rast_open_fp_new(params->tcurv);

    if (params->mcurv)
	cf6 = Rast_open_fp_new(params->mcurv);

    nrows = outhd->rows;
    if (nrows != params->nsizr) {
	G_warning(_("First change your rows number(%d) to %d"),
		  nrows, params->nsizr);
	return -1;
    }

    ncols = outhd->cols;
    if (ncols != params->nsizc) {
	G_warning(_("First change your columns number(%d) to %d"),
		  ncols, params->nsizr);
	return -1;
    }

    if (params->elev != NULL) {
	G_fseek(params->Tmp_fd_z, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    G_fseek(params->Tmp_fd_z, (off_t) (params->nsizr - 1 - i) *
		    params->nsizc * sizeof(FCELL), 0);
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_z);
	    Rast_put_f_row(cf1, cell1);
	}
    }

    if (params->slope != NULL) {
	G_fseek(params->Tmp_fd_dx, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    G_fseek(params->Tmp_fd_dx, (off_t) (params->nsizr - 1 - i) *
		    params->nsizc * sizeof(FCELL), 0);
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_dx);
	    /*
	     * for (ii==0;ii<params->nsizc;ii++) { fprintf(stderr,"ii=%d ",ii);
	     * fprintf(stderr,"%f ",cell1[ii]); }
	     * fprintf(stderr,"params->nsizc=%d \n",params->nsizc);
	     */
	    Rast_put_f_row(cf2, cell1);
	}
    }

    if (params->aspect != NULL) {
	G_fseek(params->Tmp_fd_dy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    G_fseek(params->Tmp_fd_dy, (off_t) (params->nsizr - 1 - i) *
		    params->nsizc * sizeof(FCELL), 0);
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_dy);
	    Rast_put_f_row(cf3, cell1);
	}
    }

    if (params->pcurv != NULL) {
	G_fseek(params->Tmp_fd_xx, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    G_fseek(params->Tmp_fd_xx, (off_t) (params->nsizr - 1 - i) *
		    params->nsizc * sizeof(FCELL), 0);
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_xx);
	    Rast_put_f_row(cf4, cell1);
	}
    }

    if (params->tcurv != NULL) {
	G_fseek(params->Tmp_fd_yy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    G_fseek(params->Tmp_fd_yy, (off_t) (params->nsizr - 1 - i) *
		    params->nsizc * sizeof(FCELL), 0);
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_yy);
	    Rast_put_f_row(cf5, cell1);
	}
    }

    if (params->mcurv != NULL) {
	G_fseek(params->Tmp_fd_xy, 0L, 0);	/* seek to the beginning */
	for (i = 0; i < params->nsizr; i++) {
	    /* seek to the right row */
	    G_fseek(params->Tmp_fd_xy, (off_t) (params->nsizr - 1 - i) *
		    params->nsizc * sizeof(FCELL), 0);
	    fread(cell1, sizeof(FCELL), params->nsizc, params->Tmp_fd_xy);
	    Rast_put_f_row(cf6, cell1);
	}
    }

    if (cf1)
	Rast_close(cf1);
    if (cf2)
	Rast_close(cf2);
    if (cf3)
	Rast_close(cf3);
    if (cf4)
	Rast_close(cf4);
    if (cf5)
	Rast_close(cf5);
    if (cf6)
	Rast_close(cf6);

    /* write colormaps and history for output cell files */
    /* colortable for elevations */
    maps = G_find_file("cell", input, "");

    if (params->elev != NULL) {
	if (maps == NULL) {
	    G_warning(_("Raster map <%s> not found"), input);
	    return -1;
	}
	Rast_init_colors(&colors2);
	/*
	 * Rast_mark_colors_as_fp(&colors2);
	 */

	if (Rast_read_colors(input, maps, &colors) >= 0) {
	    if (colors.modular.rules) {
		rule = colors.modular.rules;

		while (rule->next)
		    rule = rule->next;

		for (; rule; rule = rule->prev) {
		    value1 = rule->low.value * params->zmult;
		    value2 = rule->high.value * params->zmult;
		    Rast_add_modular_d_color_rule(&value1, rule->low.red,
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
		    Rast_add_d_color_rule(&value1, rule->low.red,
					      rule->low.grn, rule->low.blu,
					      &value2, rule->high.red,
					      rule->high.grn, rule->high.blu,
					      &colors2);
		}
	    }

	    maps = NULL;
	    maps = G_find_file("cell", params->elev, "");
	    if (maps == NULL) {
		G_warning(_("Raster map <%s> not found"), params->elev);
		return -1;
	    }

	    Rast_write_colors(params->elev, maps, &colors2);
	    Rast_quantize_fp_map_range(params->elev, mapset,
				    zminac - 0.5, zmaxac + 0.5,
				    (CELL) (zminac - 0.5),
				    (CELL) (zmaxac + 0.5));
	}
	else
	    G_warning(_("No color table for input raster map -- will not create color table"));
    }

    /* colortable for slopes */
    if (cond1 & (!params->deriv)) {
	Rast_init_colors(&colors);
	val1 = 0;
	val2 = 2;
	Rast_add_c_color_rule(&val1, 255, 255, 255, &val2, 255, 255, 0, &colors);
	val1 = 2;
	val2 = 5;
	Rast_add_c_color_rule(&val1, 255, 255, 0, &val2, 0, 255, 0, &colors);
	val1 = 5;
	val2 = 10;
	Rast_add_c_color_rule(&val1, 0, 255, 0, &val2, 0, 255, 255, &colors);
	val1 = 10;
	val2 = 15;
	Rast_add_c_color_rule(&val1, 0, 255, 255, &val2, 0, 0, 255, &colors);
	val1 = 15;
	val2 = 30;
	Rast_add_c_color_rule(&val1, 0, 0, 255, &val2, 255, 0, 255, &colors);
	val1 = 30;
	val2 = 50;
	Rast_add_c_color_rule(&val1, 255, 0, 255, &val2, 255, 0, 0, &colors);
	val1 = 50;
	val2 = 90;
	Rast_add_c_color_rule(&val1, 255, 0, 0, &val2, 0, 0, 0, &colors);

	if (params->slope != NULL) {
	    maps = NULL;
	    maps = G_find_file("cell", params->slope, "");
	    if (maps == NULL) {
		G_warning(_("Raster map <%s> not found"), params->slope);
		return -1;
	    }
	    Rast_write_colors(params->slope, maps, &colors);
	    Rast_quantize_fp_map_range(params->slope, mapset, 0., 90., 0, 90);

	    do_history(params->slope, input, params);
	}

	/* colortable for aspect */
	Rast_init_colors(&colors);
	val1 = 0;
	val2 = 0;
	Rast_add_c_color_rule(&val1, 255, 255, 255, &val2, 255, 255, 255, &colors);
	val1 = 1;
	val2 = 90;
	Rast_add_c_color_rule(&val1, 255, 255, 0, &val2, 0, 255, 0, &colors);
	val1 = 90;
	val2 = 180;
	Rast_add_c_color_rule(&val1, 0, 255, 0, &val2, 0, 255, 255, &colors);
	val1 = 180;
	val2 = 270;
	Rast_add_c_color_rule(&val1, 0, 255, 255, &val2, 255, 0, 0, &colors);
	val1 = 270;
	val2 = 360;
	Rast_add_c_color_rule(&val1, 255, 0, 0, &val2, 255, 255, 0, &colors);

	if (params->aspect != NULL) {
	    maps = NULL;
	    maps = G_find_file("cell", params->aspect, "");
	    if (maps == NULL) {
		G_warning(_("Raster map <%s> not found"), params->aspect);
		return -1;
	    }
	    Rast_write_colors(params->aspect, maps, &colors);
	    Rast_quantize_fp_map_range(params->aspect, mapset, 0., 360., 0, 360);

	    do_history(params->aspect, input, params);
	}

	/* colortable for curvatures */
	if (cond2) {
	    Rast_init_colors(&colors);

	    dat1 = (FCELL) amin1(c1min, c2min);
	    dat2 = (FCELL) - 0.01;

	    Rast_add_f_color_rule(&dat1, 50, 0, 155,
				      &dat2, 0, 0, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.001;
	    Rast_add_f_color_rule(&dat1, 0, 0, 255,
				      &dat2, 0, 127, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) - 0.00001;
	    Rast_add_f_color_rule(&dat1, 0, 127, 255,
				      &dat2, 0, 255, 255, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.00;
	    Rast_add_f_color_rule(&dat1, 0, 255, 255,
				      &dat2, 200, 255, 200, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.00001;
	    Rast_add_f_color_rule(&dat1, 200, 255, 200,
				      &dat2, 255, 255, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.001;
	    Rast_add_f_color_rule(&dat1, 255, 255, 0,
				      &dat2, 255, 127, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) 0.01;
	    Rast_add_f_color_rule(&dat1, 255, 127, 0,
				      &dat2, 255, 0, 0, &colors);
	    dat1 = dat2;
	    dat2 = (FCELL) amax1(c1max, c2max);
	    Rast_add_f_color_rule(&dat1, 255, 0, 0,
				      &dat2, 155, 0, 20, &colors);
	    maps = NULL;
	    if (params->pcurv != NULL) {
		maps = G_find_file("cell", params->pcurv, "");
		if (maps == NULL) {
		    G_warning(_("Raster map <%s> not found"), params->pcurv);
		    return -1;
		}
		Rast_write_colors(params->pcurv, maps, &colors);

		fprintf(stderr, "color map written\n");

		Rast_quantize_fp_map_range(params->pcurv, mapset,
					dat1, dat2,
					(CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));
		do_history(params->pcurv, input, params);
	    }

	    if (params->tcurv != NULL) {
		maps = NULL;
		maps = G_find_file("cell", params->tcurv, "");
		if (maps == NULL) {
		    G_warning(_("Raster map <%s> not found"), params->tcurv);
		    return -1;
		}
		Rast_write_colors(params->tcurv, maps, &colors);
		Rast_quantize_fp_map_range(params->tcurv, mapset,
					dat1, dat2, (CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));

		do_history(params->tcurv, input, params);
	    }

	    if (params->mcurv != NULL) {
		maps = NULL;
		maps = G_find_file("cell", params->mcurv, "");
		if (maps == NULL) {
		    G_warning(_("Raster map <%s> not found"), params->mcurv);
		    return -1;
		}
		Rast_write_colors(params->mcurv, maps, &colors);
		Rast_quantize_fp_map_range(params->mcurv, mapset,
					dat1, dat2,
					(CELL) (dat1 * MULT),
					(CELL) (dat2 * MULT));

		do_history(params->mcurv, input, params);
	    }
	}
    }

    if (params->elev != NULL) {
	if (!G_find_file2("cell", params->elev, "")) {
	    G_warning(_("Raster map <%s> not found"), params->elev);
	    return -1;
	}

	Rast_short_history(params->elev, "raster", &hist);

	if (smooth != NULL)
	    Rast_append_format_history(
		&hist, "tension=%f, smoothing=%s",
		params->fi * 1000. / (*dnorm), smooth);
	else
	    Rast_append_format_history(
		&hist, "tension=%f", params->fi * 1000. / (*dnorm));

	Rast_append_format_history(
	    &hist, "dnorm=%f, zmult=%f", *dnorm, params->zmult);
	Rast_append_format_history(
	    &hist, "KMAX=%d, KMIN=%d, errtotal=%f", params->kmax,
	    params->kmin, sqrt(ertot / n_points));
	Rast_append_format_history(
	    &hist, "zmin_data=%f, zmax_data=%f", zmin, zmax);
	Rast_append_format_history(
	    &hist, "zmin_int=%f, zmax_int=%f", zminac, zmaxac);

	Rast_format_history(&hist, HIST_DATSRC_1, "raster map %s", input);

	Rast_write_history(params->elev, &hist);

	Rast_free_history(&hist);
    }

/* change region to initial region */
    G_verbose_message(_("Changing the region back to initial..."));
    Rast_set_output_window(winhd);

    return 1;
}
