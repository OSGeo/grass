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

/* Nearest read from an in-RAM input STRIP holding input rows [imin, imax].
 * col_idx/row_idx are full-map input indices; the strip is addressed relative
 * to imin. A sample inside the full input map but outside the loaded strip
 * means the band footprint was under-sized: this is the stop-on-divergence
 * trip (must never fire if band_input_row_span is correct). Lock-free: reads
 * only, disjoint output slots per thread. */
static void interpolate_strip(void *strip, void *obufptr, int cell_type,
                              double col_idx, double row_idx,
                              struct Cell_head *incellhd, int imin, int imax)
{
    int c = (int)floor(col_idx);
    int r = (int)floor(row_idx);
    int cell_size = Rast_cell_size(cell_type);

    /* Outside the full input map: legitimate NULL (same as p_nearest). */
    if (r < 0 || r >= incellhd->rows || c < 0 || c >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    /* This input row is inside the input map (the check above already handled
     * coordinates that fall outside it), but it is not among the rows we
     * preloaded into this band's strip. That cannot happen if the band's
     * footprint estimate was right, so it means the estimate was wrong: a bug
     * in band sizing, not a normal case. Fail loudly rather than write a NULL
     * and silently produce wrong output. */
    if (r < imin || r > imax)
        G_fatal_error(_("Band strip under-sized: input row %d outside loaded "
                        "range [%d, %d] at column %d"),
                      r, imin, imax, c);

    unsigned char *src =
        (unsigned char *)strip +
        (((size_t)(r - imin) * incellhd->cols + c) * cell_size);
    memcpy(obufptr, src, cell_size);
}

/* Dense edge-walk of an output band's rectangle [obr0, obr1) projected into
 * input space; returns the min/max INPUT ROW touched, plus a 2-cell margin,
 * clamped to the input map. Samples the band's top and bottom rows across all
 * columns and its left and right columns across all band rows (bordwalk-style),
 * so a curved transform's interior-edge extremum is caught -- corner-only
 * sampling can under-size the strip. Called serially, before the parallel
 * region, so the shared tproj is safe here. Returns imax < imin for a band
 * that projects entirely outside the input. */
static void band_input_row_span(const struct Cell_head *ohd,
                                const struct Cell_head *ihd,
                                const struct pj_info *oproj,
                                const struct pj_info *iproj,
                                const struct pj_info *tproj, int obr0, int obr1,
                                int *imin, int *imax)
{
    double rmin = 1e300, rmax = -1e300;
    int e, r, c;

    /* top edge (row obr0) and bottom edge (row obr1-1), all columns */
    for (e = 0; e < 2; e++) {
        int orow = (e == 0) ? obr0 : (obr1 - 1);
        double y = ohd->north - (orow + 0.5) * ohd->ns_res;
        for (c = 0; c < ohd->cols; c++) {
            double x = ohd->west + (c + 0.5) * ohd->ew_res;
            double xx = x, yy = y;
            if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &xx, &yy, NULL) < 0)
                continue;
            double ri = (ihd->north - yy) / ihd->ns_res;
            if (ri < rmin)
                rmin = ri;
            if (ri > rmax)
                rmax = ri;
        }
    }
    /* left edge (col 0) and right edge (col cols-1), all band rows */
    for (e = 0; e < 2; e++) {
        int ocol = (e == 0) ? 0 : (ohd->cols - 1);
        double x = ohd->west + (ocol + 0.5) * ohd->ew_res;
        for (r = obr0; r < obr1; r++) {
            double y = ohd->north - (r + 0.5) * ohd->ns_res;
            double xx = x, yy = y;
            if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &xx, &yy, NULL) < 0)
                continue;
            double ri = (ihd->north - yy) / ihd->ns_res;
            if (ri < rmin)
                rmin = ri;
            if (ri > rmax)
                rmax = ri;
        }
    }

    if (rmax < rmin) { /* band projects entirely outside the input */
        *imin = 0;
        *imax = -1;
        return;
    }

    int lo = (int)floor(rmin) - 2; /* 2-cell margin for interp stencils */
    int hi = (int)floor(rmax) + 2;
    if (lo < 0)
        lo = 0;
    if (hi > ihd->rows - 1)
        hi = ihd->rows - 1;
    *imin = lo;
    *imax = hi;
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

    /* Open the input map (input location env). Banding loads only per-band
     * input strips, not the whole map, so fdi stays open across the band loop.
     */
    G_switch_env();
    Rast_set_input_window(&incellhd);
    fdi = Rast_open_old(inmap->answer, setname);
    cell_type = Rast_get_map_type(fdi);
    if (strcmp(interpol->answer, "nearest") != 0)
        cell_type = FCELL_TYPE;
    cell_size = Rast_cell_size(cell_type);

    /* Parallel input reads: decide the read-thread count here, in the INPUT
     * env, so the mask guard checks the source mapset's mask (the mask that
     * would apply to Rast_get_row on fdi). Rast_disable_omp_on_mask returns 1
     * (serial) if a mask is present or without OpenMP, and does NOT touch the
     * thread count when no mask exists (lib/raster/mask_info.c:226-231), so the
     * compute region's threads are unperturbed in the common case. When
     * read_nprocs > 1 we open that many FRESH read fds (one per thread); fdi is
     * used only by the serial fallback.
     * INFERRED-safe (not yet runtime-verified; the gate converts it):
     * concurrent Rast_open_old fds on the same map across locations is sound
     * from the r.neighbors same-location precedent (in_fd[t]) plus the Stage 1
     * fcb analysis (each fd carries its own cur_row/data/data_fd; reads depend
     * only on the fcb and R__.rd_window). */
#ifdef _OPENMP
    int want_nprocs = omp_get_max_threads();
#else
    int want_nprocs = 1;
#endif
    int read_nprocs = Rast_disable_omp_on_mask(want_nprocs);
    int *fd_read = NULL;
    if (read_nprocs > 1) {
        fd_read = G_malloc((size_t)read_nprocs * sizeof(int));
        for (int t = 0; t < read_nprocs; t++)
            fd_read[t] = Rast_open_old(inmap->answer, setname);
    }

    /* Back to the output location: set output window, init transform, open
     * output map. Both fds now stay open; rd_window/wr_window are set and
     * survive env switches, so reads/writes use the right windows throughout.
     */
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

    /* Banding (r.neighbors two-level structure): outer serial band loop ->
     * serial strip load -> parallel compute into a per-band buffer -> serial
     * in-order per-band write -> next band. Bounds peak memory by the cap
     * instead of the whole input map (Path A). */
    double cap_mb = atof(memory->answer);
    size_t cap_bytes = (size_t)(cap_mb * 1024.0 * 1024.0);
    double t_size = 0.0, t_fill = 0.0, t_compute = 0.0, t_write = 0.0;
    int n_bands = 0;

    G_important_message(_("Projecting (banded, per-thread PROJ context)..."));

    int obr0 = 0;
    while (obr0 < outcellhd.rows) {
        /* Band height: start from all remaining rows, halve until the input
         * strip plus the band's output buffer fit the cap. band_input_row_span
         * is RE-RUN for every candidate height -- the previous band's span is
         * never reused. */
        double ts = rproj_wtime();
        int band_orows = outcellhd.rows - obr0;
        int imin = 0, imax = -1;
        for (;;) {
            band_input_row_span(&outcellhd, &incellhd, &oproj, &iproj, &tproj,
                                obr0, obr0 + band_orows, &imin, &imax);
            int strip_rows = imax - imin + 1;
            size_t strip_bytes =
                strip_rows > 0 ? (size_t)strip_rows * incellhd.cols * cell_size
                               : 0;
            size_t out_bytes = (size_t)band_orows * outcellhd.cols * cell_size;
            if (strip_bytes + out_bytes <= cap_bytes)
                break;
            if (band_orows == 1)
                G_fatal_error(
                    _("A single output row needs %.1f MB (input footprint %d "
                      "rows), exceeding the memory cap (%.1f MB). This "
                      "large-halo/oblique case needs the tile-cache path, "
                      "which is not implemented."),
                    (double)(strip_bytes + out_bytes) / (1024.0 * 1024.0),
                    strip_rows, cap_mb);
            band_orows = (band_orows + 1) / 2; /* halve (round up), re-sample */
        }
        t_size += rproj_wtime() - ts;

        int obr1 = obr0 + band_orows;
        int strip_rows = imax - imin + 1;
        n_bands++;

        /* Serial strip load (single fd -> get_row not thread-safe). Reads in
         * the INPUT env (matching the serial code's invariant), then back to
         * OUTPUT for compute+write. EMPTY BAND: strip_rows <= 0 means the band
         * projects entirely outside the input -> no malloc, no read; its cells
         * become NULL via interpolate_strip's out-of-map path. */
        void *strip = NULL;
        if (strip_rows > 0) {
            strip = G_malloc((size_t)strip_rows * incellhd.cols * cell_size);
            double t0 = rproj_wtime();
            G_switch_env(); /* -> input */
            if (read_nprocs > 1) {
#ifdef _OPENMP
                /* Parallel read: each thread reads a contiguous, disjoint
                 * block of strip rows (schedule(static)) through its OWN fd
                 * into its own disjoint strip slice (slice = row r - imin).
                 * No two threads share an fd or a strip row. */
#pragma omp parallel num_threads(read_nprocs)
                {
                    int t = omp_get_thread_num();
#pragma omp for schedule(static)
                    for (int r = imin; r <= imax; r++)
                        Rast_get_row(fd_read[t],
                                     (unsigned char *)strip +
                                         (size_t)(r - imin) * incellhd.cols *
                                             cell_size,
                                     r, cell_type);
                }
#endif
            }
            else {
                /* Serial fallback (nprocs==1, mask present, or no OpenMP):
                 * original loop, unchanged, through fdi. */
                for (int r = imin; r <= imax; r++)
                    Rast_get_row(fdi,
                                 (unsigned char *)strip + (size_t)(r - imin) *
                                                              incellhd.cols *
                                                              cell_size,
                                 r, cell_type);
            }
            G_switch_env(); /* -> output */
            t_fill += rproj_wtime() - t0;
        }

        /* Per-band output buffer, lock-free disjoint row slots (band-relative
         * index), mirroring r.neighbors' outputs[i].buf. */
        void *band_out =
            G_malloc((size_t)band_orows * outcellhd.cols * cell_size);

        double t1 = rproj_wtime();
        /* Each band runs one parallel region. This is not nested parallelism:
         * the "omp for" below does not create a second thread team, it only
         * divides the band's output rows among the threads that this "omp
         * parallel" created. The two directives are kept separate instead of a
         * combined "omp parallel for" because every thread must clone its own
         * PROJ context before the row loop starts and destroy it after the loop
         * ends, and that per-thread setup has to sit inside the parallel region
         * but outside the for. */
#pragma omp parallel
        {
            /* Per-thread PROJ context + private transform clone (KEEP: this is
             * the bit-exact-verified, banding-agnostic part). oproj/iproj are
             * read-only shared; the static METERS_in/out race is benign
             * (constant CRS per run). */
            struct gpj_transform_clone tproj_local;
            GPJ_clone_transform(&tproj, &tproj_local);

#pragma omp for private(row, col) schedule(dynamic)
            for (row = obr0; row < obr1; row++) {
                void *out_row =
                    (unsigned char *)band_out +
                    (size_t)(row - obr0) * outcellhd.cols * cell_size;
                double local_y = outcellhd.north - (outcellhd.ns_res / 2) -
                                 (row * outcellhd.ns_res);
                double local_x_start = outcellhd.west + (outcellhd.ew_res / 2);

                for (col = 0; col < outcellhd.cols; col++) {
                    void *obufptr =
                        (unsigned char *)out_row + (size_t)col * cell_size;
                    double x1 = local_x_start + (col * outcellhd.ew_res);
                    double y1 = local_y;

                    if (GPJ_transform(&oproj, &iproj, &tproj_local.info, PJ_FWD,
                                      &x1, &y1, NULL) < 0) {
                        Rast_set_null_value(obufptr, 1, cell_type);
                    }
                    else {
                        double c_idx = (x1 - incellhd.west) / incellhd.ew_res;
                        double r_idx = (incellhd.north - y1) / incellhd.ns_res;
                        interpolate_strip(strip, obufptr, cell_type, c_idx,
                                          r_idx, &incellhd, imin, imax);
                    }
                }
            }

            GPJ_free_transform_clone(&tproj_local);
        }
        t_compute += rproj_wtime() - t1;

        /* Serial in-order write of the band's rows (Rast_put_row sequential).
         */
        double t2 = rproj_wtime();
        for (row = obr0; row < obr1; row++)
            Rast_put_row(fdo,
                         (unsigned char *)band_out +
                             (size_t)(row - obr0) * outcellhd.cols * cell_size,
                         cell_type);
        t_write += rproj_wtime() - t2;

        G_percent(obr1, outcellhd.rows, 5);

        if (strip)
            G_free(strip);
        G_free(band_out);
        obr0 = obr1;
    }

    G_debug(1,
            "PHASE_TIMERS size=%.4f fill=%.4f compute=%.4f write=%.4f bands=%d",
            t_size, t_fill, t_compute, t_write, n_bands);

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
