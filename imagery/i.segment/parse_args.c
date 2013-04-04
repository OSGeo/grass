/* PURPOSE:      Parse and validate the input */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include "iseg.h"

int parse_args(int argc, char *argv[], struct globals *globals)
{
    struct Option *group, *seeds, *bounds, *output,
                  *method, *similarity, *threshold, *min_segment_size,
#ifdef _OR_SHAPE_
		  *shape_weight, *smooth_weight,
#endif
		   *mem;
    struct Flag *diagonal, *weighted;
    struct Option *outband, *endt;

    /* required parameters */
    group = G_define_standard_option(G_OPT_I_GROUP);

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    threshold = G_define_option();
    threshold->key = "threshold";
    threshold->type = TYPE_DOUBLE;
    threshold->required = YES;
    threshold->label = _("Difference threshold between 0 and 1");
    threshold->description = _("Threshold = 0 merges only identical segments; threshold = 1 merges all");

    /* optional parameters */

    method = G_define_option();
    method->key = "method";
    method->type = TYPE_STRING;
    method->required = NO;
    method->answer = "region_growing";
    method->options = "region_growing";
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

    mem = G_define_option();
    mem->key = "memory";
    mem->type = TYPE_INTEGER;
    mem->required = NO;
    mem->answer = "300";
    mem->description = _("Memory in MB");

    /* TODO input for distance function */

    /* debug parameters */
    endt = G_define_option();
    endt->key = "iterations";
    endt->type = TYPE_INTEGER;
    endt->required = NO;
    endt->answer = "20";
    endt->description = _("Maximum number of iterations");
    endt->guisection = _("Settings");

    /* Using raster for seeds
     * Low priority TODO: allow vector points/centroids seed input. */
    seeds = G_define_standard_option(G_OPT_R_INPUT);
    seeds->key = "seeds";
    seeds->required = NO;
    seeds->description = _("Name for input raster map with starting seeds");

    /* Polygon constraints. */
    bounds = G_define_standard_option(G_OPT_R_INPUT);
    bounds->key = "bounds";
    bounds->required = NO;
    bounds->label = _("Name of input bounding/constraining raster map");
    bounds->description =
	_("Must be integer values, each area will be segmented independent of the others");

    outband = G_define_standard_option(G_OPT_R_OUTPUT);
    outband->key = "goodness";
    outband->required = NO;
    outband->description =
	_("Name for output goodness of fit estimate");

    diagonal = G_define_flag();
    diagonal->key = 'd';
    diagonal->description =
	_("Use 8 neighbors (3x3 neighborhood) instead of the default 4 neighbors for each pixel");
    diagonal->guisection = _("Settings");

    weighted = G_define_flag();
    weighted->key = 'w';
    weighted->description =
	_("Weighted input, don't perform the default scaling of input raster maps");
    weighted->guisection = _("Settings");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Check and save parameters */

    globals->image_group = group->answer;

    if (G_legal_filename(output->answer) == TRUE)
	globals->out_name = output->answer;
    else
	G_fatal_error("Invalid output raster name");

    /* Note: this threshold is scaled after we know more at the beginning of create_isegs() */
    globals->alpha = atof(threshold->answer);

    if (globals->alpha <= 0 || globals->alpha >= 1)
	G_fatal_error(_("Threshold should be >= 0 and <= 1"));

    /* segmentation methods:  1 = region growing */
    if (strcmp(method->answer, "region_growing") == 0)
	globals->method = 1;
    else
	G_fatal_error(_("Unable to assign segmentation method"));

    G_debug(1, "segmentation method: %d", globals->method);

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
	if (Rast_map_type(globals->seeds, "") !=
	    CELL_TYPE) {
	    G_fatal_error(_("Seeeds raster map must be CELL type (integers)"));
	}
    }

    if (bounds->answer == NULL) {
	globals->bounds_map = NULL;
    }
    else {
	globals->bounds_map = bounds->answer;
	if ((globals->bounds_mapset = G_find_raster(globals->bounds_map, "")) == NULL) {
	    G_fatal_error(_("Segmentation constraint/boundary raster map not found"));
	}
	if (Rast_map_type(globals->bounds_map, globals->bounds_mapset) !=
	    CELL_TYPE) {
	    G_fatal_error(_("Segmentation constraint raster map must be CELL type (integers)"));
	}
    }

    /* other data */
    globals->nrows = Rast_window_rows();
    globals->ncols = Rast_window_cols();

    /* debug help */
    if (outband->answer == NULL)
	globals->out_band = NULL;
    else {
	if (G_legal_filename(outband->answer) == TRUE)
	    globals->out_band = outband->answer;
	else
	    G_fatal_error(_("Invalid output raster name for goodness of fit"));
    }

    if (endt->answer) {
	if (atoi(endt->answer) > 0)
	    globals->end_t = atoi(endt->answer);
	else {
	    globals->end_t = 100;
	    G_warning(_("Invalid number of iterations, 100 will be used"));
	}
    }
    else
	globals->end_t = 1000;

    if (mem->answer && atoi(mem->answer) > 10)
	globals->mb = atoi(mem->answer);
    else {
	globals->mb = 300;
	G_warning(_("Invalid number of MB, 300 will be used"));
    }

    return TRUE;
}
