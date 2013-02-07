/* PURPOSE:      Parse and validate the input */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include "iseg.h"

int parse_args(int argc, char *argv[], struct files *files,
	       struct functions *functions)
{
    struct Option *group, *seeds, *bounds, *output, *method, *similarity, *threshold, *min_segment_size, *endt;	/* Establish an Option pointer for each option */
    struct Option *radio_weight, *smooth_weight;
    struct Flag *estimate_threshold, *diagonal, *weighted, *limited, *final;	/* Establish a Flag pointer for each option */
    struct Option *outband;	/* optional saving of segment data, until a seperate module is written */

#ifdef VCLOSE
    struct Option *very_close;
#endif

    /* required parameters */
    /* enhancement: consider giving the option to process just one
     * raster directly, without creating an image group. */
    group = G_define_standard_option(G_OPT_I_GROUP);

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    threshold = G_define_option();
    threshold->key = "threshold";
    threshold->type = TYPE_DOUBLE;
    threshold->required = YES;
    threshold->description = _("Similarity threshold.");

    method = G_define_option();
    method->key = "method";
    method->type = TYPE_STRING;
    method->required = YES;
    method->answer = "region_growing";
    method->options = "region_growing";
    method->description = _("Segmentation method.");

    similarity = G_define_option();
    similarity->key = "similarity";
    similarity->type = TYPE_STRING;
    similarity->required = YES;
    similarity->answer = "euclidean";
    similarity->options = "euclidean, manhattan";
    similarity->description = _("Similarity calculation method.");

    min_segment_size = G_define_option();
    min_segment_size->key = "minsize";
    min_segment_size->type = TYPE_INTEGER;
    min_segment_size->required = YES;
    min_segment_size->answer = "1";	/* default: no merges, a minimum of 1 pixel is allowed in a segment. */
    min_segment_size->options = "1-100000";
    min_segment_size->label = _("Minimum number of cells in a segment.");
    min_segment_size->description =
	_("The final iteration will ignore the threshold for any segments with fewer pixels.");

#ifdef VCLOSE
    very_close = G_define_option();
    very_close->key = "very_close";
    very_close->type = TYPE_DOUBLE;
    very_close->required = YES;
    very_close->answer = "0";
    very_close->options = "0-1";
    very_close->label = _("Fraction of the threshold.");
    very_close->description =
	_("Neighbors similarity lower then this fraction of the threshold will be merged without regard to any other processing rules.");
#endif

    /* for the weights of bands values vs. shape parameters, and weight of smoothness vs. compactness */

    radio_weight = G_define_option();
    radio_weight->key = "radioweight";
    radio_weight->type = TYPE_DOUBLE;
    radio_weight->required = YES;
    radio_weight->answer = "0.9";
    radio_weight->options = "0-1";
    radio_weight->label =
	_("Importance of radiometric (input raseters) values relative to shape");

    smooth_weight = G_define_option();
    smooth_weight->key = "smoothweight";
    smooth_weight->type = TYPE_DOUBLE;
    smooth_weight->required = YES;
    smooth_weight->answer = "0.5";
    smooth_weight->options = "0-1";
    smooth_weight->label =
	_("Importance of smoothness relative to compactness");

    /* optional parameters */

    estimate_threshold = G_define_flag();
    estimate_threshold->key = 't';
    estimate_threshold->description =
	_("Estimate a threshold based on input image group and exit.");

    diagonal = G_define_flag();
    diagonal->key = 'd';
    diagonal->description =
	_("Use 8 neighbors (3x3 neighborhood) instead of the default 4 neighbors for each pixel.");

    weighted = G_define_flag();
    weighted->key = 'w';
    weighted->description =
	_("Weighted input, don't perform the default scaling of input maps.");

    final = G_define_flag();
    final->key = 'f';
    final->description =
	_("Final forced merge only (skip the growing portion of the algorithm.");

    /* Raster for initial segment seeds */
    /* future enhancement: allow vector points/centroids for seed input. */
    seeds = G_define_standard_option(G_OPT_R_INPUT);
    seeds->key = "seeds";
    seeds->required = NO;
    seeds->label = _("Optional raster map with starting seeds.");
    seeds->description =
	_("Pixel values with positive integers are used as starting seeds.");

    /* Polygon constraints. */
    bounds = G_define_standard_option(G_OPT_R_INPUT);
    bounds->key = "bounds";
    bounds->required = NO;
    bounds->label = _("Optional bounding/constraining raster map");
    bounds->description =
	_("Pixels with the same integer value will be segmented independent of the others.");

    /* other parameters */
    endt = G_define_option();
    endt->key = "endt";
    endt->type = TYPE_INTEGER;
    endt->required = NO;
    endt->answer = "1000";
    endt->description =
	_("Maximum number of passes (time steps) to complete.");

    /* Leaving path flag out of user options, will hardcode TRUE for 
       this option.  This does change the resulting segments, but I 
       don't see any reason why one version is more valid.  It 
       reduced the processing time by 50% when segmenting the ortho 
       image. Code in the segmenation algorithm remains, in case more 
       validation of this option should be done. */
    /*
       path = G_define_flag();
       path->key = 'p';
       path->description =
       _("temporary option, pathflag, select to use Rk as next Ri if not mutually best neighbors.");
     */
    limited = G_define_flag();
    limited->key = 'l';
    limited->description =
	_("segments are limited to be included in only one merge per pass");

    outband = G_define_standard_option(G_OPT_R_OUTPUT);
    outband->key = "final_mean";
    outband->required = NO;
    outband->description =
	_("Save the final mean values for the first band in the imagery group.");
    /* enhancement: save mean values for all bands.  Multiple rasters or switch to polygons? */


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Validation */

    /* Check and save parameters */

    files->image_group = group->answer;

    functions->estimate_threshold = estimate_threshold->answer;

    /* if we are just estimating a threshold, skip remaining input validation */
    if (functions->estimate_threshold == TRUE)
	return TRUE;

    if (G_legal_filename(output->answer) == TRUE)
	files->out_name = output->answer;	/* name of output (segment ID) raster map */
    else
	G_fatal_error(_("Invalid output raster name."));

    functions->threshold = atof(threshold->answer);	/* Note: this threshold is scaled after we know more at the beginning of create_isegs() */

    if (weighted->answer == FALSE &&
	(functions->threshold <= 0 || functions->threshold >= 1))
	G_fatal_error(_("threshold should be >= 0 and <= 1"));

    /* segmentation methods: 1 = region growing */
    if (strncmp(method->answer, "region_growing", 10) == 0)
	functions->method = 1;
    else
	G_fatal_error(_("Couldn't assign segmentation method."));	/*shouldn't be able to get here */

    /* distance methods for similarity measurement */
    if (strncmp(similarity->answer, "euclidean", 5) == 0)
	functions->calculate_similarity = &calculate_euclidean_similarity;
    else if (strncmp(similarity->answer, "manhattan", 5) == 0)
	functions->calculate_similarity = &calculate_manhattan_similarity;
    else
	G_fatal_error(_("Couldn't assign similarity method."));	/*shouldn't be able to get here */

#ifdef VCLOSE
    functions->very_close = atof(very_close->answer);
#endif

    functions->min_segment_size = atoi(min_segment_size->answer);

    functions->radio_weight = atof(radio_weight->answer);
    functions->smooth_weight = atof(smooth_weight->answer);

    if (diagonal->answer == FALSE) {
	functions->find_pixel_neighbors = &find_four_pixel_neighbors;
	functions->num_pn = 4;
    }
    else if (diagonal->answer == TRUE) {
	functions->find_pixel_neighbors = &find_eight_pixel_neighbors;
	functions->num_pn = 8;
    }
    /* speed enhancement: Check if function pointer or IF statement is faster */

    /* default/0 for performing the scaling, but selected/1 if 
     * user has weighted values so scaling should be skipped. */
    files->weighted = weighted->answer;

	functions->final_merge_only = final->answer;

    /* check if optional seeds map was given, if not, use all pixels as starting seeds. */
    if (seeds->answer == NULL) {
	files->seeds_map = NULL;
    }
    else {			/* seeds provided, check if valid map */
	files->seeds_map = seeds->answer;
	if ((files->seeds_mapset =
	     G_find_raster2(files->seeds_map, "")) == NULL) {
	    G_fatal_error(_("Starting seeds map not found."));
	}
	if (Rast_map_type(files->seeds_map, files->seeds_mapset) != CELL_TYPE) {
	    G_fatal_error(_("Starting seeds map must be CELL type (integers)"));
	}
    }

    /* check if optional processing boundaries were provided */
    if (bounds->answer == NULL) {	/*no processing constraints */
	files->bounds_map = NULL;
    }
    else {			/* processing constraints given, check if valid map */
	files->bounds_map = bounds->answer;
	if ((files->bounds_mapset =
	     G_find_raster2(files->bounds_map, "")) == NULL) {
	    G_fatal_error(_("Segmentation constraint/boundary map not found."));
	}
	if (Rast_map_type(files->bounds_map, files->bounds_mapset) !=
	    CELL_TYPE) {
	    G_fatal_error(_("Segmentation constraint map must be CELL type (integers)"));
	}
    }

    /* other data */
    files->nrows = Rast_window_rows();
    files->ncols = Rast_window_cols();

    if (endt->answer != NULL && atoi(endt->answer) >= 0)
	functions->end_t = atoi(endt->answer);
    else {
	functions->end_t = 1000;
	G_warning(_("invalid number of iterations, 1000 will be used."));
    }

    /* other parameters */

    /* default/0 for no pathflag, but selected/1 to use 
     * Rk as next Ri if not mutually best neighbors. */
    /* functions->path = path->answer; */
    functions->path = TRUE;
    /* see notes above about pathflag. */

    functions->limited = limited->answer;

    if (outband->answer == NULL)
	files->out_band = NULL;
    else {
	if (G_legal_filename(outband->answer) == TRUE)
	    files->out_band = outband->answer;	/* name of current means */
	else
	    G_fatal_error(_("Invalid output raster name for means."));
    }

    return TRUE;
}
