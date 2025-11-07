/* PURPOSE:      Parse and validate the input */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include "iseg.h"

int parse_args(int argc, char *argv[], struct globals *globals)
{
    struct Option *group, *seeds, *bounds, *output, *method, *similarity,
        *threshold, *min_segment_size, *hs, *hr, *bsuf,
#ifdef _OR_SHAPE_
        *smooth_weight,
#endif
        *mem;
    struct Flag *diagonal, *weighted, *ms_a, *ms_p;
    struct Option *gof, *endt;
    int bands;

    /* required parameters */
    group = G_define_standard_option(G_OPT_R_INPUTS);
    group->key = "group";
    group->description = _("Name of input imagery group or raster maps");

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    bsuf = G_define_standard_option(G_OPT_R_OUTPUT);
    bsuf->key = "band_suffix";
    bsuf->required = NO;
    bsuf->label = _("Suffix for output bands with modified band values");

    threshold = G_define_option();
    threshold->key = "threshold";
    threshold->type = TYPE_DOUBLE;
    threshold->required = YES;
    threshold->label = _("Difference threshold between 0 and 1");
    threshold->description = _("Threshold = 0 merges only identical segments; "
                               "threshold = 1 merges all");

    /* optional parameters */

    hs = G_define_option();
    hs->key = "radius";
    hs->type = TYPE_DOUBLE;
    hs->required = NO;
    hs->answer = "1.5";
    hs->label = _("Spatial radius in number of cells");
    hs->description = _("Must be >= 1, only cells within spatial bandwidth are "
                        "considered for mean shift");

    hr = G_define_option();
    hr->key = "hr";
    hr->type = TYPE_DOUBLE;
    hr->required = NO;
    hr->label = _("Range (spectral) bandwidth [0, 1]");
    hr->description = _("Only cells within range (spectral) bandwidth are "
                        "considered for mean shift. Range bandwidth is used as "
                        "conductance parameter for adaptive bandwidth");

    method = G_define_option();
    method->key = "method";
    method->type = TYPE_STRING;
    method->required = NO;
    method->answer = "region_growing";
    method->options = "region_growing,mean_shift";
    /*
       Watershed method disabled, it's not implemented yet, see
       https://trac.osgeo.org/grass/ticket/3181
       method->options = "region_growing,mean_shift,watershed";
     */
    method->description = _("Segmentation method");
    method->guisection = _("Settings");

    similarity = G_define_option();
    similarity->key = "similarity";
    similarity->type = TYPE_STRING;
    similarity->required = NO;
    similarity->answer = "euclidean";
    similarity->options = "euclidean,manhattan";
    similarity->description = _("Similarity calculation method");
    similarity->guisection = _("Settings");

    min_segment_size = G_define_option();
    min_segment_size->key = "minsize";
    min_segment_size->type = TYPE_INTEGER;
    min_segment_size->required = NO;
    min_segment_size->answer = "1";
    min_segment_size->options = "1-100000";
    min_segment_size->label = _("Minimum number of cells in a segment");
    min_segment_size->description =
        _("The final step will merge small segments with their best neighbor");
    min_segment_size->guisection = _("Settings");

#ifdef _OR_SHAPE_
    radio_weight = G_define_option();
    radio_weight->key = "radio_weight";
    radio_weight->type = TYPE_DOUBLE;
    radio_weight->required = NO;
    radio_weight->answer = "1";
    radio_weight->options = "0-1";
    radio_weight->label =
        _("Importance of radiometric (input raster) values relative to shape");
    radio_weight->guisection = _("Settings");

    smooth_weight = G_define_option();
    smooth_weight->key = "smooth_weight";
    smooth_weight->type = TYPE_DOUBLE;
    smooth_weight->required = NO;
    smooth_weight->answer = "0.5";
    smooth_weight->options = "0-1";
    smooth_weight->label =
        _("Importance of smoothness relative to compactness");
    smooth_weight->guisection = _("Settings");
#endif

    mem = G_define_standard_option(G_OPT_MEMORYMB);

    /* TODO input for distance function */

    /* debug parameters */
    endt = G_define_option();
    endt->key = "iterations";
    endt->type = TYPE_INTEGER;
    endt->required = NO;
    endt->description = _("Maximum number of iterations");
    endt->guisection = _("Settings");

    /* Using raster for seeds
     * Low priority TODO: allow vector points/centroids seed input. */
    seeds = G_define_standard_option(G_OPT_R_INPUT);
    seeds->key = "seeds";
    seeds->required = NO;
    seeds->description = _("Name for input raster map with starting seeds");
    seeds->guisection = _("Settings");

    /* Polygon constraints. */
    bounds = G_define_standard_option(G_OPT_R_INPUT);
    bounds->key = "bounds";
    bounds->required = NO;
    bounds->label = _("Name of input bounding/constraining raster map");
    bounds->description = _("Must be integer values, each area will be "
                            "segmented independent of the others");
    bounds->guisection = _("Settings");

    gof = G_define_standard_option(G_OPT_R_OUTPUT);
    gof->key = "goodness";
    gof->required = NO;
    gof->description = _("Name for output goodness of fit estimate map");
    gof->guisection = _("Settings");

    diagonal = G_define_flag();
    diagonal->key = 'd';
    diagonal->description = _("Use 8 neighbors (3x3 neighborhood) instead of "
                              "the default 4 neighbors for each pixel");
    diagonal->guisection = _("Settings");

    weighted = G_define_flag();
    weighted->key = 'w';
    weighted->description = _("Weighted input, do not perform the default "
                              "scaling of input raster maps");
    weighted->guisection = _("Settings");

    ms_a = G_define_flag();
    ms_a->key = 'a';
    ms_a->label = _("Use adaptive bandwidth for mean shift");
    ms_a->description =
        _("Range (spectral) bandwidth is adapted for each moving window");
    ms_a->guisection = _("Settings");

    ms_p = G_define_flag();
    ms_p->key = 'p';
    ms_p->label = _("Use progressive bandwidth for mean shift");
    ms_p->description = _("Spatial bandwidth is increased, range (spectral) "
                          "bandwidth is decreased in each iteration");
    ms_p->guisection = _("Settings");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* Check and save parameters */

    for (bands = 0; group->answers[bands] != NULL; bands++)
        ;

    I_init_group_ref(&globals->Ref);
    if (bands > 1 || !I_find_group(group->answers[0])) {
        /* create group from input is raster map(s) */
        char name[GNAME_MAX];
        const char *mapset;

        for (bands = 0; group->answers[bands] != NULL; bands++) {
            /* strip @mapset, do not modify opt_in->answers */
            if (G_strlcpy(name, group->answers[bands], sizeof(name)) >=
                sizeof(name)) {
                G_fatal_error(_("Raster map name <%s> is too long"),
                              group->answers[bands]);
            }
            mapset = G_find_raster(name, "");
            if (!mapset)
                G_fatal_error(_("Raster map <%s> not found"),
                              group->answers[bands]);
            /* Add input to group. */
            I_add_file_to_group_ref(name, mapset, &globals->Ref);
        }

        globals->image_group = NULL;
    }
    else {
        /* input is group. Try to read group file */
        if (!I_get_group_ref(group->answers[0], &globals->Ref))
            G_fatal_error(_("Group <%s> not found in the current mapset"),
                          group->answers[0]);

        if (globals->Ref.nfiles <= 0)
            G_fatal_error(_("Group <%s> contains no raster maps"),
                          globals->image_group);

        globals->image_group = group->answers[0];
    }

    if (G_legal_filename(output->answer) == TRUE)
        globals->out_name = output->answer;
    else
        G_fatal_error("Invalid output raster name");

    globals->bsuf = bsuf->answer;

    globals->alpha = atof(threshold->answer);
    if (globals->alpha <= 0 || globals->alpha >= 1)
        G_fatal_error(_("Threshold should be > 0 and < 1"));

    globals->hs = -1;
    if (hs->answer) {
        globals->hs = atof(hs->answer);
        if (globals->hs < 1) {
            G_fatal_error(_("Option '%s' must be >= 1"), hs->key);
        }
    }

    globals->hr = -1;
    if (hr->answer) {
        globals->hr = atof(hr->answer);
        if (globals->hr < 0) {
            G_warning(_("Negative value %s for option '%s': disabling"),
                      hr->answer, hr->key);
            globals->hr = -1;
        }
        if (globals->hr >= 1) {
            G_warning(_("Value %s for option '%s' is >= 1: disabling"),
                      hr->answer, hr->key);
            globals->hr = -1;
        }
    }
    globals->ms_adaptive = ms_a->answer;
    globals->ms_progressive = ms_p->answer;

    /* segmentation methods */
    if (strcmp(method->answer, "region_growing") == 0) {
        globals->method = ORM_RG;
        globals->method_fn = region_growing;
    }
    else if (strcmp(method->answer, "mean_shift") == 0) {
        globals->method = ORM_MS;
        globals->method_fn = mean_shift;
    }
    else if (strcmp(method->answer, "watershed") == 0) {
        globals->method = ORM_WS;
        globals->method_fn = watershed;
    }
    else
        G_fatal_error(_("Unable to assign segmentation method"));

    G_debug(1, "segmentation method: %s (%d)", method->answer, globals->method);

    /* distance methods for similarity measurement */
    if (strcmp(similarity->answer, "euclidean") == 0)
        globals->calculate_similarity = calculate_euclidean_similarity;
    else if (strcmp(similarity->answer, "manhattan") == 0)
        globals->calculate_similarity = calculate_manhattan_similarity;
    else
        G_fatal_error(_("Invalid similarity method"));

#ifdef _OR_SHAPE_
    /* consider shape */
    globals->radio_weight = atof(radio_weight->answer);
    if (globals->radio_weight <= 0)
        G_fatal_error(_("Option '%s' must be > 0"), radio_weight->key);
    if (globals->radio_weight > 1)
        G_fatal_error(_("Option '%s' must be <= 1"), radio_weight->key);
    globals->smooth_weight = atof(smooth_weight->answer);
    if (globals->smooth_weight < 0)
        G_fatal_error(_("Option '%s' must be >= 0"), smooth_weight->key);
    if (globals->smooth_weight > 1)
        G_fatal_error(_("Option '%s' must be <= 1"), smooth_weight->key);
#else
    globals->radio_weight = 1;
    globals->smooth_weight = 0.5;
#endif

    globals->min_segment_size = atoi(min_segment_size->answer);

    if (diagonal->answer == FALSE) {
        globals->find_neighbors = find_four_neighbors;
        globals->nn = 4;
        G_debug(1, "four pixel neighborhood");
    }
    else if (diagonal->answer == TRUE) {
        globals->find_neighbors = find_eight_neighbors;
        globals->nn = 8;
        G_debug(1, "eight (3x3) pixel neighborhood");
    }

    /* default/0 for performing the scaling
     * selected/1 if scaling should be skipped. */
    globals->weighted = weighted->answer;

    globals->seeds = seeds->answer;
    if (globals->seeds) {
        if (G_find_raster(globals->seeds, "") == NULL) {
            G_fatal_error(_("Seeds raster map not found"));
        }
        if (Rast_map_type(globals->seeds, "") != CELL_TYPE) {
            G_fatal_error(_("Seeeds raster map must be CELL type (integers)"));
        }
    }

    if (bounds->answer == NULL) {
        globals->bounds_map = NULL;
    }
    else {
        globals->bounds_map = bounds->answer;
        if ((globals->bounds_mapset = G_find_raster(globals->bounds_map, "")) ==
            NULL) {
            G_fatal_error(
                _("Segmentation constraint/boundary raster map not found"));
        }
        if (Rast_map_type(globals->bounds_map, globals->bounds_mapset) !=
            CELL_TYPE) {
            G_fatal_error(_("Segmentation constraint raster map must be CELL "
                            "type (integers)"));
        }
    }

    /* other data */
    globals->nrows = Rast_window_rows();
    globals->ncols = Rast_window_cols();

    if (sizeof(LARGEINT) < 8) {
        int i;

        LARGEINT intmax;

        intmax = ((LARGEINT)1 << (sizeof(LARGEINT) * 8 - 2)) - 1;
        intmax += ((LARGEINT)1 << (sizeof(LARGEINT) * 8 - 2));

        globals->ncells = globals->ncols;
        for (i = 1; i < globals->nrows; i++) {
            if (globals->ncols > intmax - globals->ncells)
                G_fatal_error(
                    _("Integer overflow: too many cells in current region"));

            globals->ncells += globals->ncols;
        }
    }

    /* debug help */
    if (gof->answer == NULL)
        globals->gof = NULL;
    else {
        if (G_legal_filename(gof->answer) == TRUE)
            globals->gof = gof->answer;
        else
            G_fatal_error(_("Invalid output raster name for goodness of fit"));
    }

    if (!endt->answer) {
        globals->end_t = 50;
        if (globals->method == ORM_MS)
            globals->end_t = 10;
        G_message(_("Maximum number of iterations set to %d"), globals->end_t);
    }
    else {
        if (atoi(endt->answer) > 0)
            globals->end_t = atoi(endt->answer);
        else {
            globals->end_t = 50;
            if (globals->method == ORM_MS)
                globals->end_t = 10;
            G_warning(_("Invalid number of iterations, %d will be used"),
                      globals->end_t);
        }
    }

    if (mem->answer && atoi(mem->answer) > 10)
        globals->mb = atoi(mem->answer);
    else {
        globals->mb = 300;
        G_warning(_("Invalid number of MB, 300 will be used"));
    }

    return TRUE;
}
