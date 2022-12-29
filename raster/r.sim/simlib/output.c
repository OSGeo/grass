/* output.c (simlib), 20.nov.2002, JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include <grass/waterglobs.h>

void free_walkers()
{
    G_free(w);
    G_free(vavg);
    if (outwalk != NULL)
        G_free(stack);
}


static void output_walker_as_vector(int tt_minutes, int ndigit, struct TimeStamp *timestamp);

/* This function was added by Soeren 8. Mar 2011     */
/* It replaces the site walker output implementation */
/* Only the 3d coordinates of the walker are stored. */
void output_walker_as_vector(int tt_minutes, int ndigit, struct TimeStamp *timestamp)
{
    char buf[GNAME_MAX + 10];
    char *outwalk_time = NULL;
    double x, y, z;
    struct Map_info Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int i;

    if (outwalk != NULL) {

	/* In case of time series we extent the output name with the time value */
	if (ts == 1) {
	    G_snprintf(buf, sizeof(buf), "%s_%.*d", outwalk, ndigit, tt_minutes);
	    outwalk_time = G_store(buf);
	    if (Vect_open_new(&Out, outwalk_time, WITH_Z) < 0)
		G_fatal_error(_("Unable to create vector map <%s>"),
				outwalk_time);
	    G_message("Writing %i walker into vector file %s", nstack, outwalk_time);
	}
	else {
	    if (Vect_open_new(&Out, outwalk, WITH_Z) < 0)
		G_fatal_error(_("Unable to create vector map <%s>"), outwalk);
	    G_message("Writing %i walker into vector file %s", nstack, outwalk);
	}

	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();

	for (i = 0; i < nstack; i++) {
	    x = stack[i].x;
	    y = stack[i].y;
	    z = stack[i].m;

	    Vect_reset_line(Points);
	    Vect_reset_cats(Cats);
			    
	    Vect_cat_set(Cats, 1, i + 1);
	    Vect_append_point(Points, x, y, z);
	    Vect_write_line(&Out, GV_POINT, Points, Cats);
	}
	/* Close vector file */
        Vect_close(&Out);
                
        Vect_destroy_line_struct(Points);
        Vect_destroy_cats_struct(Cats);
        if (ts == 1)
            G_write_vector_timestamp(outwalk_time, "1", timestamp);
        else
            G_write_vector_timestamp(outwalk, "1", timestamp);
    }
    
    return;
}

/* Soeren 8. Mar 2011 TODO: 
 * This function needs to be refractured and splittet into smaller parts */
int output_data(int tt, double ft)
{

    FCELL *depth_cell, *disch_cell, *err_cell;
    FCELL *conc_cell, *flux_cell, *erdep_cell;
    int depth_fd, disch_fd, err_fd;
    int conc_fd, flux_fd, erdep_fd;
    int i, iarc, j;
    float gsmax = 0, dismax = 0., gmax = 0., ermax = -1.e+12, ermin = 1.e+12;
    struct Colors colors;
    struct History hist, hist1;	/* hist2, hist3, hist4, hist5 */
    struct TimeStamp timestamp;
    char *depth0 = NULL, *disch0 = NULL, *err0 = NULL;
    char *conc0 = NULL, *flux0 = NULL;
    char *erdep0 = NULL;
    const char *mapst = NULL;
    char *type;
    char buf[GNAME_MAX + 10];
    char timestamp_buf[15];
    int ndigit;
    int timemin;
    int tt_minutes;
    FCELL dat1, dat2;
    float a1, a2;

    timemin = (int)(timesec / 60. + 0.5);
    ndigit = 2;
    /* more compact but harder to read: 
	ndigit = (int)floor(log10(timesec)) + 2 */
    if (timemin >= 100)
	ndigit = 3;
    if (timemin >= 1000)
	ndigit = 4;
    if (timemin >= 10000)
	ndigit = 5;

    /* Convert to minutes */
    tt_minutes = (int)(tt / 60. + 0.5);

    /* Create timestamp */
    sprintf(timestamp_buf, "%d minutes", tt_minutes);
    G_scan_timestamp(&timestamp, timestamp_buf);

    /* Write the output walkers */
    output_walker_as_vector(tt_minutes, ndigit, &timestamp);

    /* we write in the same region as we used for reading */

    if (my != Rast_window_rows())
	G_fatal_error("OOPS: rows changed from %d to %d", mx,
		      Rast_window_rows());
    if (mx != Rast_window_cols())
	G_fatal_error("OOPS: cols changed from %d to %d", my,
		      Rast_window_cols());

    if (depth) {
	depth_cell = Rast_allocate_f_buf();
	if (ts == 1) {
	    G_snprintf(buf, sizeof(buf), "%s.%.*d", depth, ndigit, tt_minutes);
	    depth0 = G_store(buf);
	    depth_fd = Rast_open_fp_new(depth0);
	}
	else
	    depth_fd = Rast_open_fp_new(depth);
    }

    if (disch) {
	disch_cell = Rast_allocate_f_buf();
	if (ts == 1) {
	    G_snprintf(buf, sizeof(buf),"%s.%.*d", disch, ndigit, tt_minutes);
	    disch0 = G_store(buf);
	    disch_fd = Rast_open_fp_new(disch0);
	}
	else
	    disch_fd = Rast_open_fp_new(disch);
    }

    if (err) {
	err_cell = Rast_allocate_f_buf();
	if (ts == 1) {
	    G_snprintf(buf, sizeof(buf), "%s.%.*d", err, ndigit, tt_minutes);
	    err0 = G_store(buf);
	    err_fd = Rast_open_fp_new(err0);
	}
	else
	    err_fd = Rast_open_fp_new(err);
    }

    if (conc) {
	conc_cell = Rast_allocate_f_buf();
	if (ts == 1) {
	    G_snprintf(buf, sizeof(buf), "%s.%.*d", conc, ndigit, tt_minutes);
	    conc0 = G_store(buf);
	    conc_fd = Rast_open_fp_new(conc0);
	}
	else
	    conc_fd = Rast_open_fp_new(conc);
    }

    if (flux) {
	flux_cell = Rast_allocate_f_buf();
	if (ts == 1) {
	    G_snprintf(buf, sizeof(buf), "%s.%.*d", flux, ndigit, tt_minutes);
	    flux0 = G_store(buf);
	    flux_fd = Rast_open_fp_new(flux0);
	}
	else
	    flux_fd = Rast_open_fp_new(flux);
    }

    if (erdep) {
	erdep_cell = Rast_allocate_f_buf();
	if (ts == 1) {
	    G_snprintf(buf, sizeof(buf), "%s.%.*d", erdep, ndigit, tt_minutes);
	    erdep0 = G_store(buf);
	    erdep_fd = Rast_open_fp_new(erdep0);
	}
	else
	    erdep_fd = Rast_open_fp_new(erdep);
    }

    for (iarc = 0; iarc < my; iarc++) {
	i = my - iarc - 1;
	if (depth) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || gama[i][j] == UNDEF)
		    Rast_set_f_null_value(depth_cell + j, 1);
		else {
		    a1 = pow(gama[i][j], 3. / 5.);
		    depth_cell[j] = (FCELL) a1;	/* add conv? */
		    gmax = amax1(gmax, a1);
		}
	    }
	    Rast_put_f_row(depth_fd, depth_cell);
	}

	if (disch) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || gama[i][j] == UNDEF ||
		    cchez[i][j] == UNDEF)
		    Rast_set_f_null_value(disch_cell + j, 1);
		else {
		    a2 = step * gama[i][j] * cchez[i][j];	/* cchez incl. sqrt(sinsl) */
		    disch_cell[j] = (FCELL) a2;	/* add conv? */
		    dismax = amax1(dismax, a2);
		}
	    }
	    Rast_put_f_row(disch_fd, disch_cell);
	}

	if (err) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || gammas[i][j] == UNDEF)
		    Rast_set_f_null_value(err_cell + j, 1);
		else {
		    err_cell[j] = (FCELL) gammas[i][j];
		    gsmax = amax1(gsmax, gammas[i][j]);	/* add conv? */
		}
	    }
	    Rast_put_f_row(err_fd, err_cell);
	}

	if (conc) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || gama[i][j] == UNDEF)
		    Rast_set_f_null_value(conc_cell + j, 1);
		else {
		    conc_cell[j] = (FCELL) gama[i][j];
		    /*      gsmax = amax1(gsmax, gama[i][j]); */
		}
	    }
	    Rast_put_f_row(conc_fd, conc_cell);
	}

	if (flux) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || gama[i][j] == UNDEF ||
		    slope[i][j] == UNDEF)
		    Rast_set_f_null_value(flux_cell + j, 1);
		else {
		    a2 = gama[i][j] * slope[i][j];
		    flux_cell[j] = (FCELL) a2;
		    dismax = amax1(dismax, a2);
		}
	    }
	    Rast_put_f_row(flux_fd, flux_cell);
	}

	if (erdep) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || er[i][j] == UNDEF)
		    Rast_set_f_null_value(erdep_cell + j, 1);
		else {
		    erdep_cell[j] = (FCELL) er[i][j];
		    ermax = amax1(ermax, er[i][j]);
		    ermin = amin1(ermin, er[i][j]);
		}
	    }
	    Rast_put_f_row(erdep_fd, erdep_cell);
	}
    }

    if (depth)
	Rast_close(depth_fd);
    if (disch)
	Rast_close(disch_fd);
    if (err)
	Rast_close(err_fd);
    if (conc)
	Rast_close(conc_fd);
    if (flux)
	Rast_close(flux_fd);
    if (erdep)
	Rast_close(erdep_fd);

    if (depth) {

	Rast_init_colors(&colors);

	dat1 = (FCELL) 0.;
	dat2 = (FCELL) 0.001;
	Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.05;
	Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 0, 255, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.1;
	Rast_add_f_color_rule(&dat1, 0, 255, 255, &dat2, 0, 127, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.5;
	Rast_add_f_color_rule(&dat1, 0, 127, 255, &dat2, 0, 0, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) gmax;
	Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 0, 0, &colors);


	if (ts == 1) {
	    if ((mapst = G_find_file("fcell", depth0, "")) == NULL)
		G_fatal_error(_("FP raster map <%s> not found"), depth0);
	    Rast_write_colors(depth0, mapst, &colors);
	    Rast_quantize_fp_map_range(depth0, mapst, 0., (FCELL) gmax, 0,
				    (CELL) gmax);
	    Rast_free_colors(&colors);
	}
	else {
	    if ((mapst = G_find_file("fcell", depth, "")) == NULL)
		G_fatal_error(_("FP raster map <%s> not found"), depth);
	    Rast_write_colors(depth, mapst, &colors);
	    Rast_quantize_fp_map_range(depth, mapst, 0., (FCELL) gmax, 0,
				    (CELL) gmax);
	    Rast_free_colors(&colors);
	}
    }

    if (disch) {

	Rast_init_colors(&colors);

	dat1 = (FCELL) 0.;
	dat2 = (FCELL) 0.0005;
	Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.005;
	Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 0, 255, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.05;
	Rast_add_f_color_rule(&dat1, 0, 255, 255, &dat2, 0, 127, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.1;
	Rast_add_f_color_rule(&dat1, 0, 127, 255, &dat2, 0, 0, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) dismax;
	Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 0, 0, &colors);

	if (ts == 1) {
	    if ((mapst = G_find_file("cell", disch0, "")) == NULL)
		G_fatal_error(_("Raster map <%s> not found"), disch0);
	    Rast_write_colors(disch0, mapst, &colors);
	    Rast_quantize_fp_map_range(disch0, mapst, 0., (FCELL) dismax, 0,
				    (CELL) dismax);
	    Rast_free_colors(&colors);
	}
	else {

	    if ((mapst = G_find_file("cell", disch, "")) == NULL)
		G_fatal_error(_("Raster map <%s> not found"), disch);
	    Rast_write_colors(disch, mapst, &colors);
	    Rast_quantize_fp_map_range(disch, mapst, 0., (FCELL) dismax, 0,
				    (CELL) dismax);
	    Rast_free_colors(&colors);
	}
    }

    if (flux) {

	Rast_init_colors(&colors);

	dat1 = (FCELL) 0.;
	dat2 = (FCELL) 0.001;
	Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.1;
	Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 255, 127, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 1.;
	Rast_add_f_color_rule(&dat1, 255, 127, 0, &dat2, 191, 127, 63,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) dismax;
	Rast_add_f_color_rule(&dat1, 191, 127, 63, &dat2, 0, 0, 0,
				  &colors);

	if (ts == 1) {
	    if ((mapst = G_find_file("cell", flux0, "")) == NULL)
		G_fatal_error(_("Raster map <%s> not found"), flux0);
	    Rast_write_colors(flux0, mapst, &colors);
	    Rast_quantize_fp_map_range(flux0, mapst, 0., (FCELL) dismax, 0,
				    (CELL) dismax);
	    Rast_free_colors(&colors);
	}
	else {

	    if ((mapst = G_find_file("cell", flux, "")) == NULL)
		G_fatal_error(_("Raster map <%s> not found"), flux);
	    Rast_write_colors(flux, mapst, &colors);
	    Rast_quantize_fp_map_range(flux, mapst, 0., (FCELL) dismax, 0,
				    (CELL) dismax);
	    Rast_free_colors(&colors);
	}
    }

    if (erdep) {

	Rast_init_colors(&colors);

	dat1 = (FCELL) ermax;
	dat2 = (FCELL) 0.1;
	Rast_add_f_color_rule(&dat1, 0, 0, 0, &dat2, 0, 0, 255, &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.01;
	Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 191, 191,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.0001;
	Rast_add_f_color_rule(&dat1, 0, 191, 191, &dat2, 170, 255, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.;
	Rast_add_f_color_rule(&dat1, 170, 255, 255, &dat2, 255, 255, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) - 0.0001;
	Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) - 0.01;
	Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 255, 127, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) - 0.1;
	Rast_add_f_color_rule(&dat1, 255, 127, 0, &dat2, 255, 0, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) ermin;
	Rast_add_f_color_rule(&dat1, 255, 0, 0, &dat2, 255, 0, 255,
				  &colors);

	if (ts == 1) {
	    if ((mapst = G_find_file("cell", erdep0, "")) == NULL)
		G_fatal_error(_("Raster map <%s> not found"), erdep0);
	    Rast_write_colors(erdep0, mapst, &colors);
	    Rast_quantize_fp_map_range(erdep0, mapst, (FCELL) ermin,
				    (FCELL) ermax, (CELL) ermin,
				    (CELL) ermax);
	    Rast_free_colors(&colors);

	    type = "raster";
	    Rast_short_history(erdep0, type, &hist1);
	    Rast_append_format_history(
		&hist1, "The sediment flux file is %s", flux0);
	    Rast_command_history(&hist1);
	    Rast_write_history(erdep0, &hist1);
	}
	else {

	    if ((mapst = G_find_file("cell", erdep, "")) == NULL)
		G_fatal_error(_("Raster map <%s> not found"), erdep);
	    Rast_write_colors(erdep, mapst, &colors);
	    Rast_quantize_fp_map_range(erdep, mapst, (FCELL) ermin,
				    (FCELL) ermax, (CELL) ermin,
				    (CELL) ermax);
	    Rast_free_colors(&colors);

	    type = "raster";
	    Rast_short_history(erdep, type, &hist1);
	    Rast_append_format_history(
		&hist1, "The sediment flux file is %s", flux);
	    Rast_command_history(&hist1);
	    Rast_write_history(erdep, &hist1);
	}
    }

    /* history section */
    if (depth) {
	type = "raster";
	if (ts == 0) {
	    mapst = G_find_file("cell", depth, "");
	    if (mapst == NULL) {
		G_warning(_("Raster map <%s> not found"), depth);
		return -1;
	    }
	    Rast_short_history(depth, type, &hist);
	}
	else
	    Rast_short_history(depth0, type, &hist);

	Rast_append_format_history(
	    &hist, "init.walk=%d, maxwalk=%d, remaining walkers=%d",
	    nwalk, maxwa, nwalka);
	Rast_append_format_history(
	    &hist, "duration (sec.)=%d, time-serie iteration=%d",
	    timesec, tt);
	Rast_append_format_history(
	    &hist, "written deltap=%f, mean vel.=%f",
	    deltap, vmean);
	Rast_append_format_history(
	    &hist, "mean source (si)=%e, mean infil=%e",
	    si0, infmean);

	Rast_format_history(&hist, HIST_DATSRC_1, "input files: %s %s %s",
			    elevin, dxin, dyin);
	Rast_format_history(&hist, HIST_DATSRC_2, "input files: %s %s %s",
			    rain, infil, manin);

	Rast_command_history(&hist);

	if (ts == 1)
	    Rast_write_history(depth0, &hist);
	else
	    Rast_write_history(depth, &hist);

	if (ts == 1)
	    G_write_raster_timestamp(depth0, &timestamp);
	else
	    G_write_raster_timestamp(depth, &timestamp);
    }

    if (disch) {
	type = "raster";
	if (ts == 0) {
	    mapst = G_find_file("cell", disch, "");
	    if (mapst == NULL)
		G_fatal_error(_("Raster map <%s> not found"), disch);
	    Rast_short_history(disch, type, &hist);
	}
	else
	    Rast_short_history(disch0, type, &hist);

	Rast_append_format_history(
	    &hist,"init.walkers=%d, maxwalk=%d, rem. walkers=%d",
	    nwalk, maxwa, nwalka);
	Rast_append_format_history(
	    &hist, "duration (sec.)=%d, time-serie iteration=%d",
	    timesec, tt);
	Rast_append_format_history(
	    &hist, "written deltap=%f, mean vel.=%f",
	    deltap, vmean);
	Rast_append_format_history(
	    &hist, "mean source (si)=%e, mean infil=%e",
	    si0, infmean);

	Rast_format_history(&hist, HIST_DATSRC_1, "input files: %s %s %s",
			    elevin, dxin, dyin);
	Rast_format_history(&hist, HIST_DATSRC_2, "input files: %s %s %s",
			    rain, infil, manin);

	Rast_command_history(&hist);

	if (ts == 1)
	    Rast_write_history(disch0, &hist);
	else
	    Rast_write_history(disch, &hist);

	if (ts == 1)
	    G_write_raster_timestamp(disch0, &timestamp);
	else
	    G_write_raster_timestamp(disch, &timestamp);
    }

    if (flux) {
	type = "raster";
	if (ts == 0) {
	    mapst = G_find_file("cell", flux, "");
	    if (mapst == NULL)
		G_fatal_error(_("Raster map <%s> not found"), flux);
	    Rast_short_history(flux, type, &hist);
	}
	else
	    Rast_short_history(flux0, type, &hist);

	Rast_append_format_history(
	    &hist, "init.walk=%d, maxwalk=%d, remaining walkers=%d",
	    nwalk, maxwa, nwalka);
	Rast_append_format_history(
	    &hist, "duration (sec.)=%d, time-serie iteration=%d",
	    timesec, tt);
	Rast_append_format_history(
	    &hist, "written deltap=%f, mean vel.=%f",
	    deltap, vmean);
	Rast_append_format_history(
	    &hist, "mean source (si)=%f", si0);

	Rast_format_history(&hist, HIST_DATSRC_1, "input files: %s %s %s",
			    wdepth, dxin, dyin);
	Rast_format_history(&hist, HIST_DATSRC_2, "input files: %s %s %s %s",
			    manin, detin, tranin, tauin);

	Rast_command_history(&hist);

	if (ts == 1)
	    Rast_write_history(flux0, &hist);
	else
	    Rast_write_history(flux, &hist);

	if (ts == 1)
	    G_write_raster_timestamp(flux0, &timestamp);
	else
	    G_write_raster_timestamp(flux, &timestamp);
    }

    return 1;
}


int output_et()
{

    FCELL *tc_cell, *et_cell;
    int tc_fd, et_fd;
    int i, iarc, j;
    float etmax = -1.e+12, etmin = 1.e+12;
    float trc;
    struct Colors colors;
    const char *mapst = NULL;

    /*   char buf[GNAME_MAX + 10]; */
    FCELL dat1, dat2;

    /*   float a1,a2; */

    /* we write in the same region as we used for reading */

    if (et) {
	et_cell = Rast_allocate_f_buf();
	/* if (ts == 1) {
	   sprintf(buf, "%s.%.*d", et, ndigit, tt);
	   et0 = G_store(buf);
	   et_fd = Rast_open_fp_new(et0);
	   }
	   else */
	et_fd = Rast_open_fp_new(et);
    }

    if (tc) {
	tc_cell = Rast_allocate_f_buf();
	/*   if (ts == 1) {
	   sprintf(buf, "%s.%.*d", tc, ndigit, tt);
	   tc0 = G_store(buf);
	   tc_fd = Rast_open_fp_new(tc0);
	   }
	   else */
	tc_fd = Rast_open_fp_new(tc);
    }

    if (my != Rast_window_rows())
	G_fatal_error("OOPS: rows changed from %d to %d", mx,
		      Rast_window_rows());
    if (mx != Rast_window_cols())
	G_fatal_error("OOPS: cols changed from %d to %d", my,
		      Rast_window_cols());

    for (iarc = 0; iarc < my; iarc++) {
	i = my - iarc - 1;
	if (et) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || er[i][j] == UNDEF)
		    Rast_set_f_null_value(et_cell + j, 1);
		else {
		    et_cell[j] = (FCELL) er[i][j];	/* add conv? */
		    etmax = amax1(etmax, er[i][j]);
		    etmin = amin1(etmin, er[i][j]);
		}
	    }
	    Rast_put_f_row(et_fd, et_cell);
	}

	if (tc) {
	    for (j = 0; j < mx; j++) {
		if (zz[i][j] == UNDEF || sigma[i][j] == UNDEF ||
		    si[i][j] == UNDEF)
		    Rast_set_f_null_value(tc_cell + j, 1);
		else {
		    if (sigma[i][j] == 0.)
			trc = 0.;
		    else
			trc = si[i][j] / sigma[i][j];
		    tc_cell[j] = (FCELL) trc;
		    /*  gsmax = amax1(gsmax, trc); */
		}
	    }
	    Rast_put_f_row(tc_fd, tc_cell);
	}
    }

    if (tc)
	Rast_close(tc_fd);

    if (et)
	Rast_close(et_fd);

    if (et) {

	Rast_init_colors(&colors);

	dat1 = (FCELL) etmax;
	dat2 = (FCELL) 0.1;
	Rast_add_f_color_rule(&dat1, 0, 0, 0, &dat2, 0, 0, 255, &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.01;
	Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 191, 191,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.0001;
	Rast_add_f_color_rule(&dat1, 0, 191, 191, &dat2, 170, 255, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) 0.;
	Rast_add_f_color_rule(&dat1, 170, 255, 255, &dat2, 255, 255, 255,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) - 0.0001;
	Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) - 0.01;
	Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 255, 127, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) - 0.1;
	Rast_add_f_color_rule(&dat1, 255, 127, 0, &dat2, 255, 0, 0,
				  &colors);
	dat1 = dat2;
	dat2 = (FCELL) etmin;
	Rast_add_f_color_rule(&dat1, 255, 0, 0, &dat2, 255, 0, 255,
				  &colors);

	/*    if (ts == 1) {
	   if ((mapst = G_find_file("cell", et0, "")) == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), et0);
	   Rast_write_colors(et0, mapst, &colors);
	   Rast_quantize_fp_map_range(et0, mapst, (FCELL)etmin, (FCELL)etmax, (CELL)etmin, (CELL)etmax);
	   Rast_free_colors(&colors);
	   }
	   else { */

	if ((mapst = G_find_file("cell", et, "")) == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), et);
	Rast_write_colors(et, mapst, &colors);
	Rast_quantize_fp_map_range(et, mapst, (FCELL) etmin, (FCELL) etmax,
				(CELL) etmin, (CELL) etmax);
	Rast_free_colors(&colors);
	/*  } */
    }

    return 1;
}
