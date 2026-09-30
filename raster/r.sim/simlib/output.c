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

#include <grass/simlib.h>

void free_walkers(Simulation *sim, const char *outwalk)
{
    G_free(sim->w);
    G_free(sim->vavg);
    G_free(sim->states);
    if (outwalk != NULL)
        G_free(sim->stack);
}

/* Name of the map written at simulated time tt_minutes: in time series mode
 * the time is appended to the base name, otherwise the base name is used as
 * is. The result is allocated and owned by the caller. */
static char *output_name(const char *base, const Settings *settings,
                         const char *separator, int ndigit, int tt_minutes)
{
    char buf[GNAME_MAX + 10];

    if (!settings->ts)
        return G_store(base);
    snprintf(buf, sizeof(buf), "%s%s%.*d", base, separator, ndigit, tt_minutes);
    return G_store(buf);
}

/* This function was added by Soeren 8. Mar 2011     */
/* It replaces the site walker output implementation */
/* Only the 3d coordinates of the walker are stored. */
static void output_walker_as_vector(const char *name,
                                    struct TimeStamp *timestamp,
                                    const Simulation *sim)
{
    double x, y, z;
    struct Map_info Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int i;

    if (Vect_open_new(&Out, name, WITH_Z) < 0)
        G_fatal_error(_("Unable to create vector map <%s>"), name);
    G_message("Writing %i walker into vector file %s", sim->nstack, name);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    for (i = 0; i < sim->nstack; i++) {
        x = sim->stack[i].x;
        y = sim->stack[i].y;
        z = sim->stack[i].m;

        Vect_reset_line(Points);
        Vect_reset_cats(Cats);

        Vect_cat_set(Cats, 1, i + 1);
        Vect_append_point(Points, x, y, z);
        Vect_write_line(&Out, GV_POINT, Points, Cats);
    }
    Vect_build(&Out);
    /* Close vector file */
    Vect_close(&Out);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    G_write_vector_timestamp(name, "1", timestamp);
}

/* Set a data source history field to the list of the given input map names,
 * skipping inputs which were not given. */
static void set_data_source(struct History *hist, int field,
                            const char *const names[], int count)
{
    char buf[4 * (GNAME_MAX + 1) + 16];
    int i;

    G_strlcpy(buf, "input files:", sizeof(buf));
    for (i = 0; i < count; i++) {
        if (names[i]) {
            G_strlcat(buf, " ", sizeof(buf));
            G_strlcat(buf, names[i], sizeof(buf));
        }
    }
    Rast_set_history(hist, field, buf);
}

/* Record the run summary in the history of a written map, using the same
 * keys as the -p output, and set the map timestamp. */
static void write_history(const char *name, double tt, const Setup *setup,
                          const Settings *settings, const Simulation *sim,
                          const Inputs *inputs, struct TimeStamp *timestamp)
{
    struct History hist;

    Rast_short_history(name, "raster", &hist);

    Rast_append_format_history(
        &hist,
        "walkers_generated=%d, walkers_requested=%d, walkers_remaining=%d",
        sim->nwalk, sim->maxwa, sim->nwalka);
    Rast_append_format_history(&hist, "duration=%d, simulated_time=%f",
                               settings->timesec, tt);
    Rast_append_format_history(&hist, "time_step=%f, mean_velocity=%f",
                               time_step_seconds(setup), setup->vmean);
    if (setup->chmean != 0.0)
        Rast_append_format_history(&hist, "mean_mannings_n=%f",
                                   1.0 / setup->chmean);
    else
        Rast_append_format_history(&hist, "mean_mannings_n=undefined");
    if (inputs->wdepth) {
        const char *const terrain[] = {inputs->wdepth, inputs->dxin,
                                       inputs->dyin};
        const char *const soil[] = {inputs->manin, inputs->detin,
                                    inputs->tranin, inputs->tauin};

        Rast_append_format_history(&hist, "mean_source_rate=%e", setup->si0);
        set_data_source(&hist, HIST_DATSRC_1, terrain, 3);
        set_data_source(&hist, HIST_DATSRC_2, soil, 4);
    }
    else {
        const char *const terrain[] = {inputs->elevin, inputs->dxin,
                                       inputs->dyin};
        const char *const water[] = {inputs->rain, inputs->infil,
                                     inputs->manin};

        Rast_append_format_history(&hist,
                                   "mean_source_rate=%e, mean_infiltration=%e",
                                   setup->si0, setup->infmean);
        set_data_source(&hist, HIST_DATSRC_1, terrain, 3);
        set_data_source(&hist, HIST_DATSRC_2, water, 3);
    }
    Rast_command_history(&hist);
    Rast_write_history(name, &hist);
    G_write_raster_timestamp(name, timestamp);
}

/* conn is the sequential-block extrapolation factor nblock/iblock: scales
 * the cumulative partial sum in gama into an estimator of the eventual total
 * for snapshots taken before all blocks have run. With nblock = 1 (or after
 * the final block) conn = 1.0 and the output formulas are unchanged. err is
 * written from gammas, which already has conn baked into its accumulator. */
int output_data(double tt, double conn, const Setup *setup,
                const Geometry *geometry, const Settings *settings,
                const Simulation *sim, const Inputs *inputs,
                const Outputs *outputs, const Grids *grids, Summary *summary)
{

    FCELL *depth_cell, *disch_cell, *err_cell;
    FCELL *conc_cell, *flux_cell, *erdep_cell;
    int depth_fd, disch_fd, err_fd;
    int conc_fd, flux_fd, erdep_fd;
    int i, iarc, j;
    float gsmax = 0, dismax = 0., gmax = 0., ermax = -1.e+12, ermin = 1.e+12;
    struct Colors colors;
    struct History hist1;
    struct TimeStamp timestamp;
    char *depth0 = NULL, *disch0 = NULL, *err0 = NULL;
    char *conc0 = NULL, *flux0 = NULL;
    char *erdep0 = NULL, *outwalk0 = NULL;
    const char *mapst = NULL;
    char timestamp_buf[15];
    int ndigit;
    int timemin;
    int tt_minutes;
    FCELL dat1, dat2;
    float a1, a2;
    OutputStep step = {0};

    timemin = (int)(settings->timesec / 60. + 0.5);
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
    snprintf(timestamp_buf, sizeof(timestamp_buf), "%d minutes", tt_minutes);
    G_scan_timestamp(&timestamp, timestamp_buf);

    /* Write the output walkers */
    if (outputs->outwalk) {
        outwalk0 =
            output_name(outputs->outwalk, settings, "_", ndigit, tt_minutes);
        output_walker_as_vector(outwalk0, &timestamp, sim);
    }

    /* we write in the same region as we used for reading */

    if (geometry->my != Rast_window_rows())
        G_fatal_error("OOPS: rows changed from %d to %d", geometry->mx,
                      Rast_window_rows());
    if (geometry->mx != Rast_window_cols())
        G_fatal_error("OOPS: cols changed from %d to %d", geometry->my,
                      Rast_window_cols());

    if (outputs->depth) {
        depth_cell = Rast_allocate_f_buf();
        depth0 = output_name(outputs->depth, settings, ".", ndigit, tt_minutes);
        depth_fd = Rast_open_fp_new(depth0);
    }

    if (outputs->disch) {
        disch_cell = Rast_allocate_f_buf();
        disch0 = output_name(outputs->disch, settings, ".", ndigit, tt_minutes);
        disch_fd = Rast_open_fp_new(disch0);
    }

    if (outputs->err) {
        err_cell = Rast_allocate_f_buf();
        err0 = output_name(outputs->err, settings, ".", ndigit, tt_minutes);
        err_fd = Rast_open_fp_new(err0);
    }

    if (outputs->conc) {
        conc_cell = Rast_allocate_f_buf();
        conc0 = output_name(outputs->conc, settings, ".", ndigit, tt_minutes);
        conc_fd = Rast_open_fp_new(conc0);
    }

    if (outputs->flux) {
        flux_cell = Rast_allocate_f_buf();
        flux0 = output_name(outputs->flux, settings, ".", ndigit, tt_minutes);
        flux_fd = Rast_open_fp_new(flux0);
    }

    if (outputs->erdep) {
        erdep_cell = Rast_allocate_f_buf();
        erdep0 = output_name(outputs->erdep, settings, ".", ndigit, tt_minutes);
        erdep_fd = Rast_open_fp_new(erdep0);
    }

    for (iarc = 0; iarc < geometry->my; iarc++) {
        i = geometry->my - iarc - 1;
        if (outputs->depth) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->gama[i][j] == UNDEF)
                    Rast_set_f_null_value(depth_cell + j, 1);
                else {
                    a1 = pow(grids->gama[i][j] * conn, 3. / 5.);
                    depth_cell[j] = (FCELL)a1; /* add conv? */
                    gmax = amax1(gmax, a1);
                }
            }
            Rast_put_f_row(depth_fd, depth_cell);
        }

        if (outputs->disch) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->gama[i][j] == UNDEF ||
                    grids->cchez[i][j] == UNDEF)
                    Rast_set_f_null_value(disch_cell + j, 1);
                else {
                    a2 = geometry->step * grids->gama[i][j] * conn *
                         grids->cchez[i][j];   /* cchez incl. sqrt(sinsl) */
                    disch_cell[j] = (FCELL)a2; /* add conv? */
                    dismax = amax1(dismax, a2);
                }
            }
            Rast_put_f_row(disch_fd, disch_cell);
        }

        if (outputs->err) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->gammas[i][j] == UNDEF)
                    Rast_set_f_null_value(err_cell + j, 1);
                else {
                    err_cell[j] = (FCELL)grids->gammas[i][j];
                    gsmax = amax1(gsmax, grids->gammas[i][j]); /* add conv? */
                }
            }
            Rast_put_f_row(err_fd, err_cell);
        }

        if (outputs->conc) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->gama[i][j] == UNDEF)
                    Rast_set_f_null_value(conc_cell + j, 1);
                else {
                    conc_cell[j] = (FCELL)(grids->gama[i][j] * conn);
                    /*      gsmax = amax1(gsmax, gama[i][j]); */
                }
            }
            Rast_put_f_row(conc_fd, conc_cell);
        }

        if (outputs->flux) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->gama[i][j] == UNDEF ||
                    grids->slope[i][j] == UNDEF)
                    Rast_set_f_null_value(flux_cell + j, 1);
                else {
                    a2 = grids->gama[i][j] * conn * grids->slope[i][j];
                    flux_cell[j] = (FCELL)a2;
                    dismax = amax1(dismax, a2);
                }
            }
            Rast_put_f_row(flux_fd, flux_cell);
        }

        if (outputs->erdep) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->er[i][j] == UNDEF)
                    Rast_set_f_null_value(erdep_cell + j, 1);
                else {
                    erdep_cell[j] = (FCELL)grids->er[i][j];
                    ermax = amax1(ermax, grids->er[i][j]);
                    ermin = amin1(ermin, grids->er[i][j]);
                }
            }
            Rast_put_f_row(erdep_fd, erdep_cell);
        }
    }

    if (outputs->depth)
        Rast_close(depth_fd);
    if (outputs->disch)
        Rast_close(disch_fd);
    if (outputs->err)
        Rast_close(err_fd);
    if (outputs->conc)
        Rast_close(conc_fd);
    if (outputs->flux)
        Rast_close(flux_fd);
    if (outputs->erdep)
        Rast_close(erdep_fd);

    if (outputs->depth) {

        Rast_init_colors(&colors);

        dat1 = (FCELL)0.;
        dat2 = (FCELL)0.001;
        Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.05;
        Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 0, 255, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.1;
        Rast_add_f_color_rule(&dat1, 0, 255, 255, &dat2, 0, 127, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.5;
        Rast_add_f_color_rule(&dat1, 0, 127, 255, &dat2, 0, 0, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)gmax;
        Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 0, 0, &colors);

        if ((mapst = G_find_file("fcell", depth0, "")) == NULL)
            G_fatal_error(_("FP raster map <%s> not found"), depth0);
        Rast_write_colors(depth0, mapst, &colors);
        Rast_quantize_fp_map_range(depth0, mapst, 0., (FCELL)gmax, 0,
                                   (CELL)gmax);
        Rast_free_colors(&colors);
    }

    if (outputs->disch) {

        Rast_init_colors(&colors);

        dat1 = (FCELL)0.;
        dat2 = (FCELL)0.0005;
        Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.005;
        Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 0, 255, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.05;
        Rast_add_f_color_rule(&dat1, 0, 255, 255, &dat2, 0, 127, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.1;
        Rast_add_f_color_rule(&dat1, 0, 127, 255, &dat2, 0, 0, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)dismax;
        Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 0, 0, &colors);

        if ((mapst = G_find_file("cell", disch0, "")) == NULL)
            G_fatal_error(_("Raster map <%s> not found"), disch0);
        Rast_write_colors(disch0, mapst, &colors);
        Rast_quantize_fp_map_range(disch0, mapst, 0., (FCELL)dismax, 0,
                                   (CELL)dismax);
        Rast_free_colors(&colors);
    }

    if (outputs->flux) {

        Rast_init_colors(&colors);

        dat1 = (FCELL)0.;
        dat2 = (FCELL)0.001;
        Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.1;
        Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 255, 127, 0, &colors);
        dat1 = dat2;
        dat2 = (FCELL)1.;
        Rast_add_f_color_rule(&dat1, 255, 127, 0, &dat2, 191, 127, 63, &colors);
        dat1 = dat2;
        dat2 = (FCELL)dismax;
        Rast_add_f_color_rule(&dat1, 191, 127, 63, &dat2, 0, 0, 0, &colors);

        if ((mapst = G_find_file("cell", flux0, "")) == NULL)
            G_fatal_error(_("Raster map <%s> not found"), flux0);
        Rast_write_colors(flux0, mapst, &colors);
        Rast_quantize_fp_map_range(flux0, mapst, 0., (FCELL)dismax, 0,
                                   (CELL)dismax);
        Rast_free_colors(&colors);
    }

    if (outputs->erdep) {

        Rast_init_colors(&colors);

        dat1 = (FCELL)ermax;
        dat2 = (FCELL)0.1;
        Rast_add_f_color_rule(&dat1, 0, 0, 0, &dat2, 0, 0, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.01;
        Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 191, 191, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.0001;
        Rast_add_f_color_rule(&dat1, 0, 191, 191, &dat2, 170, 255, 255,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.;
        Rast_add_f_color_rule(&dat1, 170, 255, 255, &dat2, 255, 255, 255,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.0001;
        Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.01;
        Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 255, 127, 0, &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.1;
        Rast_add_f_color_rule(&dat1, 255, 127, 0, &dat2, 255, 0, 0, &colors);
        dat1 = dat2;
        dat2 = (FCELL)ermin;
        Rast_add_f_color_rule(&dat1, 255, 0, 0, &dat2, 255, 0, 255, &colors);

        if ((mapst = G_find_file("cell", erdep0, "")) == NULL)
            G_fatal_error(_("Raster map <%s> not found"), erdep0);
        Rast_write_colors(erdep0, mapst, &colors);
        Rast_quantize_fp_map_range(erdep0, mapst, (FCELL)ermin, (FCELL)ermax,
                                   (CELL)ermin, (CELL)ermax);
        Rast_free_colors(&colors);

        Rast_short_history(erdep0, "raster", &hist1);
        Rast_append_format_history(&hist1, "The sediment flux file is %s",
                                   flux0);
        Rast_command_history(&hist1);
        Rast_write_history(erdep0, &hist1);
    }

    /* history section */
    if (outputs->depth)
        write_history(depth0, tt, setup, settings, sim, inputs, &timestamp);
    if (outputs->disch)
        write_history(disch0, tt, setup, settings, sim, inputs, &timestamp);
    if (outputs->flux)
        write_history(flux0, tt, setup, settings, sim, inputs, &timestamp);

    /* The step record takes over the allocated names. */
    step.simulated_time = tt;
    step.walkers_remaining = sim->nwalka;
    step.timestamp = G_store(timestamp_buf);
    step.depth = depth0;
    step.disch = disch0;
    step.err = err0;
    step.outwalk = outwalk0;
    step.conc = conc0;
    step.flux = flux0;
    step.erdep = erdep0;
    add_output_step(summary, &step);

    return 1;
}

int output_et(const Geometry *geometry, const Outputs *outputs,
              const Grids *grids)
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

    if (outputs->et) {
        et_cell = Rast_allocate_f_buf();
        /* if (ts == 1) {
           sprintf(buf, "%s.%.*d", et, ndigit, tt);
           et0 = G_store(buf);
           et_fd = Rast_open_fp_new(et0);
           }
           else */
        et_fd = Rast_open_fp_new(outputs->et);
    }

    if (outputs->tc) {
        tc_cell = Rast_allocate_f_buf();
        /*   if (ts == 1) {
           sprintf(buf, "%s.%.*d", tc, ndigit, tt);
           tc0 = G_store(buf);
           tc_fd = Rast_open_fp_new(tc0);
           }
           else */
        tc_fd = Rast_open_fp_new(outputs->tc);
    }

    if (geometry->my != Rast_window_rows())
        G_fatal_error("OOPS: rows changed from %d to %d", geometry->mx,
                      Rast_window_rows());
    if (geometry->mx != Rast_window_cols())
        G_fatal_error("OOPS: cols changed from %d to %d", geometry->my,
                      Rast_window_cols());

    for (iarc = 0; iarc < geometry->my; iarc++) {
        i = geometry->my - iarc - 1;
        if (outputs->et) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->er[i][j] == UNDEF)
                    Rast_set_f_null_value(et_cell + j, 1);
                else {
                    et_cell[j] = (FCELL)grids->er[i][j]; /* add conv? */
                    etmax = amax1(etmax, grids->er[i][j]);
                    etmin = amin1(etmin, grids->er[i][j]);
                }
            }
            Rast_put_f_row(et_fd, et_cell);
        }

        if (outputs->tc) {
            for (j = 0; j < geometry->mx; j++) {
                if (grids->zz[i][j] == UNDEF || grids->sigma[i][j] == UNDEF ||
                    grids->si[i][j] == UNDEF)
                    Rast_set_f_null_value(tc_cell + j, 1);
                else {
                    if (grids->sigma[i][j] == 0.)
                        trc = 0.;
                    else
                        trc = grids->si[i][j] / grids->sigma[i][j];
                    tc_cell[j] = (FCELL)trc;
                    /*  gsmax = amax1(gsmax, trc); */
                }
            }
            Rast_put_f_row(tc_fd, tc_cell);
        }
    }

    if (outputs->tc)
        Rast_close(tc_fd);

    if (outputs->et)
        Rast_close(et_fd);

    if (outputs->et) {

        Rast_init_colors(&colors);

        dat1 = (FCELL)etmax;
        dat2 = (FCELL)0.1;
        Rast_add_f_color_rule(&dat1, 0, 0, 0, &dat2, 0, 0, 255, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.01;
        Rast_add_f_color_rule(&dat1, 0, 0, 255, &dat2, 0, 191, 191, &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.0001;
        Rast_add_f_color_rule(&dat1, 0, 191, 191, &dat2, 170, 255, 255,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)0.;
        Rast_add_f_color_rule(&dat1, 170, 255, 255, &dat2, 255, 255, 255,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.0001;
        Rast_add_f_color_rule(&dat1, 255, 255, 255, &dat2, 255, 255, 0,
                              &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.01;
        Rast_add_f_color_rule(&dat1, 255, 255, 0, &dat2, 255, 127, 0, &colors);
        dat1 = dat2;
        dat2 = (FCELL)-0.1;
        Rast_add_f_color_rule(&dat1, 255, 127, 0, &dat2, 255, 0, 0, &colors);
        dat1 = dat2;
        dat2 = (FCELL)etmin;
        Rast_add_f_color_rule(&dat1, 255, 0, 0, &dat2, 255, 0, 255, &colors);

        /*    if (ts == 1) {
           if ((mapst = G_find_file("cell", et0, "")) == NULL)
           G_fatal_error(_("Raster map <%s> not found"), et0);
           Rast_write_colors(et0, mapst, &colors);
           Rast_quantize_fp_map_range(et0, mapst, (FCELL)etmin, (FCELL)etmax,
           (CELL)etmin, (CELL)etmax); Rast_free_colors(&colors);
           }
           else { */

        if ((mapst = G_find_file("cell", outputs->et, "")) == NULL)
            G_fatal_error(_("Raster map <%s> not found"), outputs->et);
        Rast_write_colors(outputs->et, mapst, &colors);
        Rast_quantize_fp_map_range(outputs->et, mapst, (FCELL)etmin,
                                   (FCELL)etmax, (CELL)etmin, (CELL)etmax);
        Rast_free_colors(&colors);
        /*  } */
    }

    return 1;
}
