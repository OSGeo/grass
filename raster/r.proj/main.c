/***************************************************************************
*
* MODULE:        r.proj
*
* AUTHOR(S):     Martin Schroeder
*                  University of Heidelberg
*                  Dept. of Geography
*                  emes@geo0.geog.uni-heidelberg.de
*
*                  (With the help of a lot of existing GRASS sources, in
*                  particular v.proj)
*
* PURPOSE:       r.proj converts a map to a new geographic projection. It reads
                 a map from a different location, projects it and write it out
*                to the current location. The projected data is resampled with
*                one of three different methods: nearest neighbor, bilinear and
*                cubic convolution.
*
* COPYRIGHT:     (C) 2001, 2011 by the GRASS Development Team
*
*                This program is free software under the GNU General Public
*                License (>=v2). Read the file COPYING that comes with GRASS
*                for details.
*
* Changes
*                Morten Hulden <morten@untamo.net>, Aug 2000:
*                - aborts if input map is outside current location.
*                - can handle projections (conic, azimuthal etc) where
*                part of the map may fall into areas where south is
*                upward and east is leftward.
*                - avoids passing location edge coordinates to PROJ
*                (they may be invalid in some projections).
*                - output map will be clipped to borders of the current region.
*                - output map cell edges and centers will coincide with those
*                of the current region.
*                - output map resolution (unless changed explicitly) will
*                match (exactly) the resolution of the current region.
*                - if the input map is smaller than the current region, the
*                output map will only cover the overlapping area.
*                - if the input map is larger than the current region, only the
*                 needed amount of memory will be allocated for the projection
*
*                Bugfixes 20050328: added floor() before (int) typecasts to in
*                avoid  asymmetrical rounding errors. Added missing offset
*                outcellhd.ew_res/2 to initial xcoord for each row in main
*                projection loop (we want to  project center of cell, not
*                border).
*
*                Glynn Clements 2006: Use G_interp_* functions, modified
*                version of r.proj which uses a tile cache instead  of loading
*                the entire map into memory.
*                Markus Metz 2010: lanczos and lanczos fallback interpolation
*                methods

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <grass/gjson.h>
#include "r.proj.h"

#ifdef _OPENMP
#include <omp.h>
static inline double rproj_wtime(void)
{
    return omp_get_wtime();
}
#else
static inline double rproj_wtime(void)
{
    return 0.0;
}
#endif

/* modify this table to add new methods */
struct menu menu[] = {
    {p_nearest, "nearest", "nearest neighbor"},
    {p_bilinear, "bilinear", "bilinear interpolation"},
    {p_cubic, "bicubic", "bicubic interpolation"},
    {p_lanczos, "lanczos", "lanczos filter"},
    {p_bilinear_f, "bilinear_f", "bilinear interpolation with fallback"},
    {p_cubic_f, "bicubic_f", "bicubic interpolation with fallback"},
    {p_lanczos_f, "lanczos_f", "lanczos filter with fallback"},
    {NULL, NULL, NULL}};

static char *make_ipol_list(void);
static char *make_ipol_desc(void);

/* Round whole numbers through 32 bit float so every read path
   returns the same values. */
static void quantize_cell_row(void *row, int cols, int cell_type)
{
    int i;

    if (cell_type == CELL_TYPE) {
        CELL *p = row;

        for (i = 0; i < cols; i++)
            if (!Rast_is_c_null_value(&p[i]))
                Rast_set_f_value(&p[i], (FCELL)p[i], CELL_TYPE);
    }
    else if (cell_type == DCELL_TYPE) {
        DCELL *p = row;

        for (i = 0; i < cols; i++)
            if (!Rast_is_d_null_value(&p[i]))
                Rast_set_f_value(&p[i], (FCELL)p[i], DCELL_TYPE);
    }
}

/* Nearest-neighbor read from an in-RAM strip holding input rows [imin, imax].
 * The col_idx and row_idx values are full-map indices and the strip is
 * addressed relative to imin. A sample that lands inside the input map but
 * outside the loaded strip means the band was under-sized, which the guard
 * below catches. */
static void interpolate_strip(void *strip, void *obufptr, int cell_type,
                              double col_idx, double row_idx,
                              struct Cell_head *incellhd, int imin, int imax)
{
    int c = (int)floor(col_idx);
    int r = (int)floor(row_idx);
    int cell_size = Rast_cell_size(cell_type);

    /* A sample outside the input map is a legitimate NULL, like p_nearest. */
    if (r < 0 || r >= incellhd->rows || c < 0 || c >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    /* The band footprint was under-sized when a needed input row lies inside
     * the input map but outside the loaded strip, so it fails loudly rather
     * than emit a wrong NULL. */
    if (r < imin || r > imax)
        G_fatal_error(_("Band strip under-sized: input row %d outside loaded "
                        "range [%d, %d] at column %d"),
                      r, imin, imax, c);

    unsigned char *src =
        (unsigned char *)strip +
        (((size_t)(r - imin) * incellhd->cols + c) * cell_size);
    memcpy(obufptr, src, cell_size);
}

/* Strip kernels in the same order as menu[], so slot i is the strip counterpart
 * of menu[i].method. Slot 0 is nearest above and slots 1 to 6 come from
 * interp_strip.c. */
static const strip_func strip_kernels[] = {
    interpolate_strip, strip_bilinear, strip_cubic,    strip_lanczos,
    strip_bilinear_f,  strip_cubic_f,  strip_lanczos_f};

/* Footprint grid used for comparison and the counters printed at the end. */
static struct footprint_grid *g_fg_boundary = NULL;

/* Serial tile-cache fallback for the oblique and large-halo corner. When even
 * one output row's full-width strip busts the cap, this finishes the run from
 * row obr0 with the classic readcell cache and CVAL kernels, exactly as serial
 * r.proj does. It stays serial because get_block mutates shared cache state,
 * and since the banded path already wrote the earlier rows the result matches
 * a pure serial run. */
static void
fallback_serial_cache(int fdi, int fdo, int cell_type, int method,
                      const struct pj_info *oproj, const struct pj_info *iproj,
                      const struct pj_info *tproj, struct Cell_head *incellhd,
                      struct Cell_head *outcellhd, const double *y_center,
                      int obr0, const char *memory)
{
    struct cache *ibuffer = readcell(fdi, memory);
    func interpolate = menu[method].method;
    void *obuffer = Rast_allocate_output_buf(cell_type);
    int cell_size = Rast_cell_size(cell_type);
    double local_x_start = outcellhd->west + (outcellhd->ew_res / 2);

    for (int row = obr0; row < outcellhd->rows; row++) {
        G_percent(row - obr0, outcellhd->rows - obr0, 5);
        for (int col = 0; col < outcellhd->cols; col++) {
            void *obufptr = (unsigned char *)obuffer + (size_t)col * cell_size;
            double x1 = local_x_start + col * outcellhd->ew_res;
            double y1 = y_center[row];

            if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &x1, &y1, NULL) <
                0) {
                Rast_set_null_value(obufptr, 1, cell_type);
            }
            else {
                double col_idx = (x1 - incellhd->west) / incellhd->ew_res;
                double row_idx = (incellhd->north - y1) / incellhd->ns_res;

                interpolate(ibuffer, obufptr, cell_type, col_idx, row_idx,
                            incellhd);
            }
        }
        Rast_put_row(fdo, obuffer, cell_type);
    }

    release_cache(ibuffer);
    G_free(obuffer);
}

/* Write the deferred band, if any, in order and release it. Shared by the last
 * band and the fallback bails so every path writes the deferred band the same
 * way. */
static void flush_pending_band(int fdo, int cell_type, int cols, int cell_size,
                               void **pending, int r0, int r1)
{
    if (*pending == NULL)
        return;
    for (int wr = r0; wr < r1; wr++)
        Rast_put_row(fdo,
                     (unsigned char *)*pending +
                         (size_t)(wr - r0) * cols * cell_size,
                     cell_type);
    G_free(*pending);
    *pending = NULL;
}

/* Thread count for compute and write overlap, where nprocs above zero
 * overrides OMP_NUM_THREADS. Set before the fit search so the write overlap's
 * two reserved output buffers match the band sizing. */
static int compute_nprocs(struct Option *nprocs)
{
    return G_set_omp_num_threads(nprocs);
}

int main(int argc, char **argv)
{
    char *mapname, /* ptr to name of output layer  */
        *setname,  /* ptr to name of input mapset  */
        *ipolname; /* name of interpolation method */

    int fdi,                       /* input map file descriptor    */
        fdo,                       /* output map file descriptor   */
        method,                    /* position of method in table  */
        permissions,               /* mapset permissions           */
        cell_type,                 /* output celltype              */
        cell_size,                 /* size of a cell in bytes      */
        row, col,                  /* counters                     */
        irows, icols,              /* original rows, cols          */
        orows, ocols, have_colors, /* Input map has a colour table */
        overwrite,                 /* Overwrite                    */
        curr_proj;                 /* output projection (see gis.h) */

    double onorth, osouth, /* save original border coords  */
        oeast, owest, inorth, isouth, ieast, iwest;
    char north_str[30], south_str[30], east_str[30], west_str[30];

    struct Colors colr; /* Input map colour table       */
    struct History history;

    struct pj_info iproj, /* input map proj parameters    */
        oproj,            /* output map proj parameters   */
        tproj;            /* transformation parameters   */

    struct Key_Value *in_proj_info, /* projection information of    */
        *in_unit_info,              /* input and output mapsets     */
        *out_proj_info, *out_unit_info;

    struct GModule *module;

    struct Flag *list,  /* list files in source location */
        *nocrop,        /* don't crop output map        */
        *print_bounds,  /* print output bounds and exit */
        *gprint_bounds; /* same but print shell style   */

    struct Option *imapset, /* name of input mapset         */
        *inmap,             /* name of input layer          */
        *inlocation,        /* name of input location       */
        *outmap,            /* name of output layer         */
        *indbase,           /* name of input database       */
        *interpol,          /* interpolation method         */
        *memory,            /* amount of memory for cache   */
        *nprocs,            /* number of compute threads    */
        *res,               /* resolution of target map     */
        *format;            /* output format                */

    struct Option *pipeline;   /* name of custom PROJ pipeline */
    struct Cell_head incellhd, /* cell header of input map     */
        outcellhd;             /* and output map               */

    enum OutputFormat outputFormat;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("projection"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("import"));
    module->description = _("Re-projects a raster map from given project to "
                            "the current project.");

    inlocation = G_define_standard_option(G_OPT_M_LOCATION);
    inlocation->required = YES;
    inlocation->label = _("Project (location) containing input raster map");
    inlocation->guisection = _("Source");

    imapset = G_define_standard_option(G_OPT_M_MAPSET);
    imapset->label = _("Mapset containing input raster map");
    imapset->description = _("Default: name of current mapset");
    imapset->guisection = _("Source");

    inmap = G_define_standard_option(G_OPT_R_INPUT);
    inmap->description = _("Name of input raster map to re-project");
    inmap->required = NO;
    inmap->guisection = _("Source");

    indbase = G_define_standard_option(G_OPT_M_DBASE);
    indbase->label = _("Path to GRASS database of input project");

    outmap = G_define_standard_option(G_OPT_R_OUTPUT);
    outmap->required = NO;
    outmap->description =
        _("Name for output raster map (default: same as 'input')");
    outmap->guisection = _("Target");

    ipolname = make_ipol_list();

    interpol = G_define_option();
    interpol->key = "method";
    interpol->type = TYPE_STRING;
    interpol->required = NO;
    interpol->answer = "nearest";
    interpol->options = ipolname;
    interpol->description = _("Interpolation method to use");
    interpol->guisection = _("Target");
    interpol->descriptions = make_ipol_desc();

    memory = G_define_standard_option(G_OPT_MEMORYMB);

    nprocs = G_define_standard_option(G_OPT_M_NPROCS);
    nprocs->description = _(
        "Number of threads for parallel computing. 0, the default, uses the "
        "OpenMP default and honors OMP_NUM_THREADS if set. A value above zero "
        "overrides OMP_NUM_THREADS, and a value below zero leaves that many "
        "cores free");

    res = G_define_option();
    res->key = "resolution";
    res->type = TYPE_DOUBLE;
    res->required = NO;
    res->description = _("Resolution of output raster map");
    res->guisection = _("Target");

    format = G_define_standard_option(G_OPT_F_FORMAT);
    format->options = "plain,shell,json";
    format->descriptions = _("plain;Human readable text output;"
                             "shell;shell script style text output;"
                             "json;JSON (JavaScript Object Notation);");
    format->guisection = _("Print");

    pipeline = G_define_option();
    pipeline->key = "pipeline";
    pipeline->type = TYPE_STRING;
    pipeline->required = NO;
    pipeline->description = _("PROJ pipeline for coordinate transformation");

    list = G_define_flag();
    list->key = 'l';
    list->description = _("List raster maps in input mapset and exit");
    list->guisection = _("Print");

    nocrop = G_define_flag();
    nocrop->key = 'n';
    nocrop->description =
        _("Do not perform region cropping optimization. See Notes if working "
          "with a global latitude-longitude projection");

    print_bounds = G_define_flag();
    print_bounds->key = 'p';
    print_bounds->description =
        _("Print input map's bounds in the current projection and exit");
    print_bounds->guisection = _("Print");

    gprint_bounds = G_define_flag();
    gprint_bounds->key = 'g';
    gprint_bounds->label = _("Print input map's bounds in the current "
                             "projection in shell script style [deprecated]");
    gprint_bounds->description =
        _("This flag is deprecated and will "
          "be removed in a future release. Use format=shell instead.");
    gprint_bounds->guisection = _("Print");

    /* The parser checks if the map already exists in current mapset,
       we switch out the check and do it
       in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (strcmp(format->answer, "json") == 0) {
        outputFormat = JSON;
    }
    else if (strcmp(format->answer, "shell") == 0) {
        outputFormat = SHELL;
    }
    else {
        outputFormat = PLAIN;
    }

    if (outputFormat != PLAIN && !print_bounds->answer && !list->answer) {
        G_fatal_error(
            _("The format option can only be used with -%c or -%c flags"),
            print_bounds->key, list->key);
    }

    if (gprint_bounds->answer) {
        G_verbose_message(
            _("Flag 'g' is deprecated and will be removed in a future "
              "release. Please use format=shell instead."));
        outputFormat = SHELL;
    }

    /* get the method */
    for (method = 0; (ipolname = menu[method].name); method++)
        if (strcmp(ipolname, interpol->answer) == 0)
            break;

    if (!ipolname)
        G_fatal_error(_("<%s=%s> unknown %s"), interpol->key, interpol->answer,
                      interpol->key);

    /* Resolve the strip kernel once; menu[] and strip_kernels[] share order. */
    strip_func interp = strip_kernels[method];

    mapname = outmap->answer ? outmap->answer : inmap->answer;
    if (mapname && !list->answer && !overwrite && !print_bounds->answer &&
        outputFormat != SHELL && G_find_raster(mapname, G_mapset()))
        G_fatal_error(_("option <%s>: <%s> exists. To overwrite, use the "
                        "--overwrite flag"),
                      "output", mapname);

    setname = imapset->answer ? imapset->answer : G_store(G_mapset());
    if (strcmp(inlocation->answer, G_location()) == 0 &&
        (!indbase->answer || strcmp(indbase->answer, G_gisdbase()) == 0))
#if 0
        G_fatal_error(_("Input and output locations can not be the same"));
#else
        G_warning(_("Input and output projects are the same"));
#endif
    G_get_window(&outcellhd);

    if (outputFormat == SHELL && !print_bounds->answer)
        print_bounds->answer = 1;
    curr_proj = G_projection();

    /* Get projection info for output mapset */
    if ((out_proj_info = G_get_projinfo()) == NULL)
        G_fatal_error(_("Unable to get projection info of output raster map"));

    if ((out_unit_info = G_get_projunits()) == NULL)
        G_fatal_error(_("Unable to get projection units of output raster map"));

    if (pj_get_kv(&oproj, out_proj_info, out_unit_info) < 0)
        G_fatal_error(
            _("Unable to get projection key values of output raster map"));

    oproj.srid = G_get_projsrid();
    oproj.wkt = G_get_projwkt();

    /* Change the location           */
    G_create_alt_env();
    G_setenv_nogisrc("GISDBASE",
                     indbase->answer ? indbase->answer : G_gisdbase());
    G_setenv_nogisrc("LOCATION_NAME", inlocation->answer);
    G_setenv_nogisrc("MAPSET", setname);

    permissions = G_mapset_permissions(setname);
    if (permissions < 0) /* can't access mapset       */
        G_fatal_error(_("Mapset <%s> in input project <%s> - %s"), setname,
                      inlocation->answer,
                      permissions == 0 ? _("permission denied")
                                       : _("not found"));

    /* if requested, list the raster maps in source location - MN 5/2001 */
    if (list->answer) {
        int i;
        char **srclist;
        G_JSON_Array *maps_array = NULL;
        G_JSON_Value *maps_value = NULL;

        if (outputFormat == JSON) {
            maps_value = G_json_value_init_array();
            if (maps_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON array. Out of memory?"));
            }
            maps_array = G_json_array(maps_value);
        }

        G_verbose_message(_("Checking project <%s> mapset <%s>"),
                          inlocation->answer, setname);
        srclist = G_list(G_ELEMENT_RASTER, G_getenv_nofatal("GISDBASE"),
                         G_getenv_nofatal("LOCATION_NAME"), setname);
        for (i = 0; srclist[i]; i++) {
            switch (outputFormat) {
            case SHELL:
            case PLAIN:
                fprintf(stdout, "%s\n", srclist[i]);
                break;

            case JSON:
                G_json_array_append_string(maps_array, srclist[i]);
                break;
            }
        }
        if (outputFormat == JSON) {
            char *serialized_string = NULL;
            serialized_string = G_json_serialize_to_string_pretty(maps_value);
            if (serialized_string == NULL) {
                G_fatal_error(_("Failed to initialize pretty JSON string."));
            }
            puts(serialized_string);
            G_json_free_serialized_string(serialized_string);
            G_json_value_free(maps_value);
        }
        else {
            fflush(stdout);
        }
        exit(EXIT_SUCCESS); /* leave r.proj after listing */
    }

    if (!inmap->answer)
        G_fatal_error(_("Required parameter <%s> not set"), inmap->key);

    if (!G_find_raster(inmap->answer, setname))
        G_fatal_error(
            _("Raster map <%s> in project <%s> in mapset <%s> not found"),
            inmap->answer, inlocation->answer, setname);

    /* Read input map colour table */
    have_colors = Rast_read_colors(inmap->answer, setname, &colr);

    /* Get projection info for input mapset */
    if ((in_proj_info = G_get_projinfo()) == NULL)
        G_fatal_error(_("Unable to get projection info of input map"));

    /* apparently the +over switch must be set in the input projection,
     * not the output latlon projection
     * TODO: for PROJ 6+, the +over switch must be added to the
     * transformation pipeline if authority:name or WKt are used as
     * crs definition */
    if (curr_proj == PROJECTION_LL)
        G_set_key_value("over", "defined", in_proj_info);

    if ((in_unit_info = G_get_projunits()) == NULL)
        G_fatal_error(_("Unable to get projection units of input map"));

    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
        G_fatal_error(_("Unable to get projection key values of input map"));

    iproj.srid = G_get_projsrid();
    iproj.wkt = G_get_projwkt();

    tproj.pj = NULL;
    tproj.def = NULL;
    if (pipeline->answer) {
        tproj.def = G_store(pipeline->answer);
    }

    G_free_key_value(in_proj_info);
    G_free_key_value(in_unit_info);
    G_free_key_value(out_proj_info);
    G_free_key_value(out_unit_info);
    if (G_verbose() > G_verbose_std())
        pj_print_proj_params(&iproj, &oproj);

    /* this call causes r.proj to read the entire map into memory */
    Rast_get_cellhd(inmap->answer, setname, &incellhd);

    if (G_projection() == PROJECTION_XY)
        G_fatal_error(_("Unable to work with unprojected data (xy project)"));

    /* Save default borders so we can show them later */
    inorth = incellhd.north;
    isouth = incellhd.south;
    ieast = incellhd.east;
    iwest = incellhd.west;
    irows = incellhd.rows;
    icols = incellhd.cols;

    onorth = outcellhd.north;
    osouth = outcellhd.south;
    oeast = outcellhd.east;
    owest = outcellhd.west;
    orows = outcellhd.rows;
    ocols = outcellhd.cols;

    if (print_bounds->answer) {
        G_JSON_Value *root_value = NULL;
        G_JSON_Object *root_object = NULL;

        G_message(_("Input map <%s@%s> in project <%s>:"), inmap->answer,
                  setname, inlocation->answer);

        /* reproject input raster extents from input to output */
        G_set_window(&incellhd);

        G_debug(1, "input window north: %.8f", incellhd.north);
        G_debug(1, "input window south: %.8f", incellhd.south);
        G_debug(1, "input window east: %.8f", incellhd.east);
        G_debug(1, "input window west: %.8f", incellhd.west);

        if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
            G_fatal_error(_("Unable to initialize coordinate transformation"));

        outcellhd.north = -1e9;
        outcellhd.south = 1e9;
        outcellhd.east = -1e9;
        outcellhd.west = 1e9;
        bordwalk_edge(&incellhd, &outcellhd, &iproj, &oproj, &tproj, PJ_FWD);
        inorth = outcellhd.north;
        isouth = outcellhd.south;
        ieast = outcellhd.east;
        iwest = outcellhd.west;

        G_format_northing(inorth, north_str, curr_proj);
        G_format_northing(isouth, south_str, curr_proj);
        G_format_easting(ieast, east_str, curr_proj);
        G_format_easting(iwest, west_str, curr_proj);

        if (outputFormat == JSON) {
            root_value = G_json_value_init_object();
            if (root_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON object. Out of memory?"));
            }
            root_object = G_json_object(root_value);
        }

        switch (outputFormat) {
        case PLAIN:
            fprintf(stdout, "Source cols: %d\n", icols);
            fprintf(stdout, "Source rows: %d\n", irows);
            fprintf(stdout, "Local north: %s\n", north_str);
            fprintf(stdout, "Local south: %s\n", south_str);
            fprintf(stdout, "Local west: %s\n", west_str);
            fprintf(stdout, "Local east: %s\n", east_str);
            break;

        case SHELL:
            fprintf(stdout, "n=%s s=%s w=%s e=%s rows=%d cols=%d\n", north_str,
                    south_str, west_str, east_str, irows, icols);
            break;

        case JSON:
            if (isfinite(inorth)) {
                G_json_object_set_number(root_object, "north", inorth);
            }
            else {
                G_json_object_set_null(root_object, "north");
            }

            if (isfinite(isouth)) {
                G_json_object_set_number(root_object, "south", isouth);
            }
            else {
                G_json_object_set_null(root_object, "south");
            }

            if (isfinite(iwest)) {
                G_json_object_set_number(root_object, "west", iwest);
            }
            else {
                G_json_object_set_null(root_object, "west");
            }

            if (isfinite(ieast)) {
                G_json_object_set_number(root_object, "east", ieast);
            }
            else {
                G_json_object_set_null(root_object, "east");
            }

            G_json_object_set_number(root_object, "rows", irows);
            G_json_object_set_number(root_object, "cols", icols);
            break;
        }

        if (outputFormat == JSON) {
            char *serialized_string = NULL;
            serialized_string = G_json_serialize_to_string_pretty(root_value);
            if (serialized_string == NULL) {
                G_fatal_error(_("Failed to initialize pretty JSON string."));
            }
            puts(serialized_string);
            G_json_free_serialized_string(serialized_string);
            G_json_value_free(root_value);
        }

        exit(EXIT_SUCCESS);
    }

    /* Cut non-overlapping parts of input map */
    if (!nocrop->answer) {
        /* reproject current region from output to input */
        /* switch back to current location,
         * initialize transformation pipeline */
        G_switch_env();
        G_unset_window();
        G_set_window(&outcellhd);
        tproj.def = NULL;
        tproj.pj = NULL;
        if (pipeline->answer) {
            tproj.def = G_store(pipeline->answer);
        }
        if (GPJ_init_transform(&oproj, &iproj, &tproj) < 0)
            G_fatal_error(_("Unable to initialize coordinate transformation"));

        /* switch to input location */
        G_switch_env();

        /* update cellhead of input map */
        bordwalk(&outcellhd, &incellhd, &oproj, &iproj, &tproj, PJ_FWD);
    }

    /* Add 2 cells on each side for bilinear/cubic & future interpolation
     * methods */
    /* (should probably be a factor based on input and output resolution) */
    incellhd.north += 2 * incellhd.ns_res;
    incellhd.east += 2 * incellhd.ew_res;
    incellhd.south -= 2 * incellhd.ns_res;
    incellhd.west -= 2 * incellhd.ew_res;
    if (incellhd.north > inorth)
        incellhd.north = inorth;
    if (incellhd.east > ieast)
        incellhd.east = ieast;
    if (incellhd.south < isouth)
        incellhd.south = isouth;
    if (incellhd.west < iwest)
        incellhd.west = iwest;

    /* switch to current location */

    G_switch_env();

    /* Adjust borders of output map */

    if (!nocrop->answer) {
        /* reproject from input to output */
        /* switch input location,
         * initialize transformation pipeline */
        G_switch_env();
        G_unset_window();
        G_set_window(&incellhd);
        tproj.def = NULL;
        tproj.pj = NULL;
        if (pipeline->answer) {
            tproj.def = G_store(pipeline->answer);
        }
        if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
            G_fatal_error(_("Unable to initialize coordinate transformation"));

        /* switch to output location */
        G_switch_env();

        /* reduce output region */
        bordwalk(&incellhd, &outcellhd, &iproj, &oproj, &tproj, PJ_FWD);
    }

#if 0
    outcellhd.west = outcellhd.south = HUGE_VAL;
    outcellhd.east = outcellhd.north = -HUGE_VAL;
    for (row = 0; row < incellhd.rows; row++) {
        ycoord1 = Rast_row_to_northing((double)(row + 0.5), &incellhd);
        for (col = 0; col < incellhd.cols; col++) {
            xcoord1 = Rast_col_to_easting((double)(col + 0.5), &incellhd);
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD,
                              &xcoord1, &ycoord1, NULL) < 0)
                G_fatal_error(_("Error in %s"), "GPJ_transform()");
            if (xcoord1 > outcellhd.east)
                outcellhd.east = xcoord1;
            if (ycoord1 > outcellhd.north)
                outcellhd.north = ycoord1;
            if (xcoord1 < outcellhd.west)
                outcellhd.west = xcoord1;
            if (ycoord1 < outcellhd.south)
                outcellhd.south = ycoord1;
        }
    }
#endif

    if (res->answer != NULL) /* set user defined resolution */
        outcellhd.ns_res = outcellhd.ew_res = atof(res->answer);

    G_adjust_Cell_head(&outcellhd, 0, 0);
    Rast_set_output_window(&outcellhd);

    G_message(" ");
    G_message(_("Input:"));
    G_message(_("Cols: %d (original: %d)"), incellhd.cols, icols);
    G_message(_("Rows: %d (original: %d)"), incellhd.rows, irows);
    G_message(_("North: %f (original: %f)"), incellhd.north, inorth);
    G_message(_("South: %f (original: %f)"), incellhd.south, isouth);
    G_message(_("West: %f (original: %f)"), incellhd.west, iwest);
    G_message(_("East: %f (original: %f)"), incellhd.east, ieast);
    G_message(_("EW-res: %f"), incellhd.ew_res);
    G_message(_("NS-res: %f"), incellhd.ns_res);
    G_message(" ");

    G_message(_("Output:"));
    G_message(_("Cols: %d (original: %d)"), outcellhd.cols, ocols);
    G_message(_("Rows: %d (original: %d)"), outcellhd.rows, orows);
    G_message(_("North: %f (original: %f)"), outcellhd.north, onorth);
    G_message(_("South: %f (original: %f)"), outcellhd.south, osouth);
    G_message(_("West: %f (original: %f)"), outcellhd.west, owest);
    G_message(_("East: %f (original: %f)"), outcellhd.east, oeast);
    G_message(_("EW-res: %f"), outcellhd.ew_res);
    G_message(_("NS-res: %f"), outcellhd.ns_res);
    G_message(" ");

    /* Open the input map in the input env. Banding loads only per-band input
     * strips rather than the whole map, so fdi stays open across the band
     * loop. */
    G_switch_env();
    Rast_set_input_window(&incellhd);
    fdi = Rast_open_old(inmap->answer, setname);
    cell_type = Rast_get_map_type(fdi);
    if (strcmp(interpol->answer, "nearest") != 0)
        cell_type = FCELL_TYPE;
    cell_size = Rast_cell_size(cell_type);

    /* The read-thread count is decided here in the input env so the mask guard
     * checks the source mapset's mask. Rast_disable_omp_on_mask returns 1 and
     * forces serial under a mask or without OpenMP, and leaves the count
     * untouched otherwise (lib/raster/mask_info.c lines 226-231). When
     * read_nprocs is above one, each thread opens its own fresh fd and fdi
     * serves only the serial fallback. Concurrent fds on the same map across
     * locations follow the r.neighbors in_fd[] precedent, where each fd carries
     * its own cur_row, data, and data_fd. */
    /* Runs before the fit search. See compute_nprocs(). */
    int want_nprocs = compute_nprocs(nprocs);
    int read_nprocs = Rast_disable_omp_on_mask(want_nprocs);
    int *fd_read = NULL;
    if (read_nprocs > 1) {
        fd_read = G_malloc((size_t)read_nprocs * sizeof(int));
        for (int t = 0; t < read_nprocs; t++)
            fd_read[t] = Rast_open_old(inmap->answer, setname);
    }

    /* Back in the output env, set the output window, init the transform, and
     * open the output map. Both fds stay open and their windows survive env
     * switches, so reads and writes use the right window throughout. */
    G_switch_env();
    Rast_set_output_window(&outcellhd);
    G_unset_window();
    G_set_window(&outcellhd);
    tproj.def = NULL;
    tproj.pj = NULL;
    if (pipeline->answer) {
        tproj.def = G_store(pipeline->answer);
    }
    if (GPJ_init_transform(&oproj, &iproj, &tproj) < 0)
        G_fatal_error(_("Unable to initialize coordinate transformation"));

    if (strcmp(interpol->answer, "nearest") == 0)
        fdo = Rast_open_new(mapname, cell_type);
    else
        fdo = Rast_open_fp_new(mapname);

    /* Banding runs a serial band loop of serial strip load, parallel compute
     * into a per-band buffer, and an in-order per-band write. This bounds peak
     * memory by the cap rather than the whole input map. */
    double cap_mb = atof(memory->answer);
    size_t cap_bytes = (size_t)(cap_mb * 1024.0 * 1024.0);
    /* Under write_overlap the overlapped writes run inside the compute region,
     * so their time falls in t_compute. t_write then covers only the
     * non-overlapped writes, which are the last band's flush and every band at
     * one thread. The fallback bail flushes are untimed. */
    double t_size = 0.0, t_fill = 0.0, t_compute = 0.0, t_write = 0.0;
    int n_bands = 0;
    int max_tiles = 1; /* most column tiles used by any single band */

    /* Output-row center northings from the serial recurrence, starting at north
     * minus ns_res/2 and subtracting ns_res per row. The direct form differs by
     * up to one ULP when ns_res is not exactly representable, so the recurrence
     * is kept to stay bitwise identical to serial. The fill loop and the sizing
     * walk share these values. */
    double *y_center = G_malloc((size_t)outcellhd.rows * sizeof(double));
    {
        double yc = outcellhd.north - (outcellhd.ns_res / 2);
        for (int r = 0; r < outcellhd.rows; r++) {
            y_center[r] = yc;
            yc -= outcellhd.ns_res;
        }
    }

    /* Pole footprint fix. A tile that projects onto a geographic pole has an
     * input-row extremum the perimeter walk misses. This happens when the pole
     * lies inside the input map, and also when the pole is outside the input's
     * latitude coverage but still projects into the output frame, as with a
     * pole-centered frame reading an input truncated below the pole, where the
     * highest reachable input latitude is the input's own edge row. So it
     * projects both poles for lat/lon input and folds in the pole's input row
     * clamped to [0, rows-1]. The point-in-rect test keeps this a no-op for
     * frames that image no pole, and a transform failure or non-finite result
     * skips the pole and leaves the under-size guard as the backstop. */
    struct pole_set poles;

    poles.n = 0;
    if (incellhd.proj == PROJECTION_LL) {
        double polelat[2] = {90.0, -90.0};

        for (int p = 0; p < 2; p++) {
            double px = 0.0, py = polelat[p];

            if (GPJ_transform(&oproj, &iproj, &tproj, PJ_INV, &px, &py, NULL) <
                    0 ||
                !isfinite(px) || !isfinite(py))
                continue;
            double ri = (incellhd.north - polelat[p]) / incellhd.ns_res;
            if (ri < 0)
                ri = 0;
            else if (ri > incellhd.rows - 1)
                ri = incellhd.rows - 1;
            poles.ox[poles.n] = px;
            poles.oy[poles.n] = py;
            poles.ri[poles.n] = ri;
            poles.n++;
        }
    }

    /* Build the grid that sizes band heights. */
    g_fg_boundary = fg_build(&outcellhd, &incellhd, &oproj, &iproj, &tproj,
                             y_center, &poles);
    /* The margin covers what the samples can miss between columns. */
    fg_apply_sampling_margin(g_fg_boundary);

    G_important_message(_("Projecting (banded, per-thread PROJ context)..."));

    int used_fallback = 0; /* set when the serial tile-cache fallback runs */
    int force_tilecache = getenv("R_PROJ_FORCE_TILECACHE") != NULL;
    /* Rolling input-strip window. win holds input rows [win_imin, win_imax].
     * win_imax < win_imin marks it empty and forces a full read. win_cap is
     * its allocated byte size, and win is freed once at fallback_done. */
    unsigned char *win = NULL;
    size_t win_cap = 0;
    int win_imin = 0, win_imax = -1;
    /* Output double-buffer predicate. The compute region runs want_nprocs
     * threads rather than the masked read_nprocs, so overlap needs more than
     * one compute thread. out_mult reserves two output bands in the fit search
     * on the same flag the writer uses, so the budget and the writer stay in
     * step. */
    int write_overlap = want_nprocs > 1;
    int out_mult = write_overlap ? 2 : 1;
    /* Previous band's output buffer, written by one thread while the next band
     * computes. It is NULL when nothing is pending and holds rows
     * [pending_r0, pending_r1). */
    void *pending_out = NULL;
    int pending_r0 = 0, pending_r1 = 0;
    int obr0 = 0;
    while (obr0 < outcellhd.rows) {
        /* Size this band. Take the tallest full-width band that fits, and when
         * even one full-width row does not fit split it into whole column
         * blocks and take the widest tile that fits. */
        double ts = rproj_wtime();
        int imin = 0, imax = -1;
        int band_orows =
            force_tilecache
                ? 1
                : fg_band_height(g_fg_boundary, obr0, cap_bytes, out_mult,
                                 cell_size, incellhd.cols);
        int tile_blocks = fg_num_blocks(g_fg_boundary);
        if (band_orows == 1) {
            int worst_block_rows = 0;

            tile_blocks =
                force_tilecache
                    ? 0
                    : fg_tile_blocks(g_fg_boundary, obr0, obr0 + band_orows,
                                     cap_bytes, out_mult, cell_size,
                                     incellhd.cols, &worst_block_rows);
            if (tile_blocks == 0) {
                /* Even the finest tiling busts the cap, so finish from obr0 on
                 * the serial tile-cache path. */
                if (force_tilecache) {
                    G_warning(
                        _("R_PROJ_FORCE_TILECACHE is set: taking the serial "
                          "tile-cache path for all output rows (testing "
                          "override)."));
                }
                else {
                    size_t out1 = (size_t)outcellhd.cols * cell_size;
                    size_t strip_bytes = worst_block_rows > 0
                                             ? (size_t)worst_block_rows *
                                                   incellhd.cols * cell_size
                                             : 0;
                    int needed_mb = (int)ceil((double)(strip_bytes + out1) /
                                              (1024.0 * 1024.0)) +
                                    1;
                    G_warning(
                        _("Memory cap (%.1f MB) is below what one output row "
                          "needs (input footprint %d rows, %.1f MB). Falling "
                          "back to the serial tile-cache path for output rows "
                          "%d-%d; this path is slower. Raise memory= to at "
                          "least %d MB to use the parallel path."),
                        cap_mb, worst_block_rows,
                        (double)(strip_bytes + out1) / (1024.0 * 1024.0), obr0,
                        outcellhd.rows - 1, needed_mb);
                }
                /* Flush the deferred band before the fallback writes from obr0
                 * in order. */
                flush_pending_band(fdo, cell_type, outcellhd.cols, cell_size,
                                   &pending_out, pending_r0, pending_r1);
                fallback_serial_cache(fdi, fdo, cell_type, method, &oproj,
                                      &iproj, &tproj, &incellhd, &outcellhd,
                                      y_center, obr0, memory->answer);
                used_fallback = 1;
                goto fallback_done;
            }
        }
        t_size += rproj_wtime() - ts;

        int obr1 = obr0 + band_orows;
        n_bands++;
        int nb = fg_num_blocks(g_fg_boundary);
        int n_tiles = (nb + tile_blocks - 1) / tile_blocks;
        if (n_tiles > max_tiles)
            max_tiles = n_tiles;

        /* Per-band output buffer at full width, filled tile by tile and written
         * once after all tiles. The row slots are disjoint, so compute is
         * lock-free. */
        void *band_out =
            G_malloc((size_t)band_orows * outcellhd.cols * cell_size);

        /* Column tiles are processed one at a time, so peak strip memory is the
         * worst tile rather than the band's union. A single tile spanning every
         * block is the full-width fast path. */
        for (int tb = 0; tb < nb; tb += tile_blocks) {
            int te = tb + tile_blocks < nb ? tb + tile_blocks : nb;
            int obc0 = fg_block_start(g_fg_boundary, tb);
            int obc1 = fg_block_start(g_fg_boundary, te);

            /* Fill spans come from the grid. The strip is full input width
             * because the raster API reads whole rows, so columns are not
             * cropped. */
            fg_span(g_fg_boundary, obr0, obr1, obc0, obc1, &imin, &imax);
            int strip_rows = imax - imin + 1;

            /* Serial strip load, since a single fd makes get_row unsafe to
             * share. An empty tile with strip_rows at or below zero projects
             * outside the input and is not read, its cells become NULL through
             * interpolate_strip's out-of-map path, and the window is
             * invalidated so the next band re-reads in full. */
            void *strip = NULL;
            if (strip_rows > 0) {
                size_t need = (size_t)strip_rows * incellhd.cols * cell_size;
                size_t row_bytes = (size_t)incellhd.cols * cell_size;
                /* Slide only for a serial-read single-tile band whose rows
                 * advance forward and still overlap the window. Anything else
                 * does a full read, same as before. */
                int can_slide = read_nprocs == 1 && n_tiles == 1 &&
                                win_imax >= 0 && imin >= win_imin &&
                                imin <= win_imax + 1;
                int read_from = imin;
                /* Grow, then memmove, then read the tail. G_realloc may move
                 * the buffer, so it must run before the memmove that
                 * repositions the retained overlap. Realloc preserves the old
                 * rows at their old offsets and the memmove shifts them to the
                 * new imin origin. Rows past a shrunk imax are dropped through
                 * win_imax below and re-read if a later band needs them. */
                if (need > win_cap) {
                    win = G_realloc(win, need);
                    win_cap = need;
                }
                if (can_slide && win_imax >= imin) {
                    memmove(win, win + (size_t)(imin - win_imin) * row_bytes,
                            (size_t)(win_imax - imin + 1) * row_bytes);
                    read_from = win_imax + 1; /* only new rows hit disk */
                }
                strip = win;
                double t0 = rproj_wtime();
                if (read_from <= imax) {
                    G_switch_env(); /* -> input */
                    if (read_nprocs > 1) {
#ifdef _OPENMP
                        /* Parallel read of the full span only, since can_slide
                         * is false here. Each thread reads a contiguous
                         * disjoint block of rows through its own fd into its
                         * own strip slice, so no two threads share an fd or a
                         * row. */
#pragma omp parallel num_threads(read_nprocs)
                        {
                            int t = omp_get_thread_num();
#pragma omp for schedule(static)
                            for (int r = read_from; r <= imax; r++)
                                Rast_get_row(fd_read[t],
                                             (unsigned char *)strip +
                                                 (size_t)(r - imin) *
                                                     incellhd.cols * cell_size,
                                             r, cell_type);
                        }
#endif
                    }
                    else {
                        /* Serial read of the (possibly partial) tail. */
                        for (int r = read_from; r <= imax; r++)
                            Rast_get_row(fdi,
                                         (unsigned char *)strip +
                                             (size_t)(r - imin) *
                                                 incellhd.cols * cell_size,
                                         r, cell_type);
                    }
                    G_switch_env(); /* -> output */
                    for (int r = read_from; r <= imax; r++)
                        quantize_cell_row((unsigned char *)strip +
                                              (size_t)(r - imin) *
                                                  incellhd.cols * cell_size,
                                          incellhd.cols, cell_type);
                }
                t_fill += rproj_wtime() - t0;
                /* Record what the window now holds. A tiled band leaves win
                 * with only its last tile, so invalidate both fields to force
                 * the next band's full read. Setting both keeps validity
                 * independent of the && short-circuit order. */
                if (n_tiles == 1) {
                    win_imin = imin;
                    win_imax = imax;
                }
                else {
                    win_imin = 0;
                    win_imax = -1;
                }
            }
            else {
                win_imin = 0;
                win_imax = -1; /* empty tile, nothing resident */
            }

            double t1 = rproj_wtime();
            /* One parallel region per tile. The omp for divides the band's
             * output rows among this region's threads. The directives are
             * separate so each thread clones its PROJ context before the row
             * loop and destroys it after. */
#pragma omp parallel
            {
                struct gpj_transform_clone tproj_local;
                GPJ_clone_transform(&tproj, &tproj_local);

#pragma omp single nowait
                {
                    /* One thread writes the previous band's rows in order while
                     * the rest compute this band. This is the first tile only,
                     * and pending_out is non-NULL only under write_overlap. */
                    if (obc0 == 0 && pending_out)
                        for (int wr = pending_r0; wr < pending_r1; wr++)
                            Rast_put_row(fdo,
                                         (unsigned char *)pending_out +
                                             (size_t)(wr - pending_r0) *
                                                 outcellhd.cols * cell_size,
                                         cell_type);
                }

#pragma omp for private(row, col) schedule(dynamic)
                for (row = obr0; row < obr1; row++) {
                    void *out_row =
                        (unsigned char *)band_out +
                        (size_t)(row - obr0) * outcellhd.cols * cell_size;
                    double local_y = y_center[row];
                    double local_x_start =
                        outcellhd.west + (outcellhd.ew_res / 2);

                    for (col = obc0; col < obc1; col++) {
                        void *obufptr =
                            (unsigned char *)out_row + (size_t)col * cell_size;
                        double x1 = local_x_start + (col * outcellhd.ew_res);
                        double y1 = local_y;

                        if (GPJ_transform(&oproj, &iproj, &tproj_local.info,
                                          PJ_FWD, &x1, &y1, NULL) < 0) {
                            Rast_set_null_value(obufptr, 1, cell_type);
                        }
                        else {
                            double c_idx =
                                (x1 - incellhd.west) / incellhd.ew_res;
                            double r_idx =
                                (incellhd.north - y1) / incellhd.ns_res;
                            interp(strip, obufptr, cell_type, c_idx, r_idx,
                                   &incellhd, imin, imax);
                        }
                    }
                }

                GPJ_free_transform_clone(&tproj_local);
            }
            t_compute += rproj_wtime() - t1;
            /* strip aliases the persistent window buffer win, so it is not
             * freed per tile. win is freed once at fallback_done. */
        }

        /* Defer this band so the next band's compute region writes it through
         * the omp single above. The previous pending completed at this band's
         * compute barrier, so free it now. Non-overlap bands write and free in
         * order here. */
        if (write_overlap) {
            if (pending_out)
                G_free(pending_out);
            pending_out = band_out;
            pending_r0 = obr0;
            pending_r1 = obr1;
        }
        else {
            double t2 = rproj_wtime();
            for (row = obr0; row < obr1; row++)
                Rast_put_row(fdo,
                             (unsigned char *)band_out + (size_t)(row - obr0) *
                                                             outcellhd.cols *
                                                             cell_size,
                             cell_type);
            t_write += rproj_wtime() - t2;
            G_free(band_out);
        }

        G_percent(obr1, outcellhd.rows, 5);
        obr0 = obr1;
    }

fallback_done:
    /* Flush the last band's deferred write on normal completion, timed into
     * t_write. The fallback bails already flushed, so pending_out is NULL on
     * those paths. */
    {
        double tw = rproj_wtime();
        flush_pending_band(fdo, cell_type, outcellhd.cols, cell_size,
                           &pending_out, pending_r0, pending_r1);
        t_write += rproj_wtime() - tw;
    }
    G_free(y_center);
    if (g_fg_boundary)
        fg_free(g_fg_boundary);
    /* Single free site for the rolling window. Normal completion and both
     * fallback_done bails converge here, so one free covers every path. win is
     * NULL when a bail fired before any band allocated it. */
    if (win)
        G_free(win);

    if (used_fallback)
        G_debug(1, "PHASE_TIMERS fallback=1 fallback_from_row=%d", obr0);
    else
        G_debug(1,
                "PHASE_TIMERS size=%.4f fill=%.4f compute=%.4f write=%.4f "
                "bands=%d tiles=%d",
                t_size, t_fill, t_compute, t_write, n_bands, max_tiles);

    /* Close input map in its own env, then the output map. */
    G_switch_env(); /* -> input */
    Rast_close(fdi);
    if (fd_read) {
        for (int t = 0; t < read_nprocs; t++)
            Rast_close(fd_read[t]);
        G_free(fd_read);
    }
    G_switch_env(); /* -> output */
    Rast_close(fdo);

    if (have_colors > 0) {
        Rast_write_colors(mapname, G_mapset(), &colr);
        Rast_free_colors(&colr);
    }

    Rast_short_history(mapname, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(mapname, &history);

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}

char *make_ipol_list(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
        size += strlen(menu[i].name) + 1;

    buf = G_malloc(size);
    *buf = '\0';

    for (i = 0; menu[i].name; i++) {
        if (i)
            strcat(buf, ",");
        strcat(buf, menu[i].name);
    }

    return buf;
}

char *make_ipol_desc(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
        size += strlen(menu[i].name) + strlen(menu[i].text) + 2;

    buf = G_malloc(size);
    *buf = '\0';

    for (i = 0; menu[i].name; i++) {
        if (i)
            strcat(buf, ";");
        strcat(buf, menu[i].name);
        strcat(buf, ";");
        strcat(buf, menu[i].text);
    }

    return buf;
}
