#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"


int make_support(struct rr_state *theState, int percent, double percentage)
{
    char title[100];
    struct History hist;
    struct Categories cats;
    struct Colors clr;
    char *inraster;
    struct RASTER_MAP_PTR nulls;

    /* write categories for output raster 
       use values from input or cover map
     */
    if (theState->docover == 1) {
	inraster = theState->inrcover;
	nulls = theState->cnulls;
    }
    else {
	inraster = theState->inraster;
	nulls = theState->nulls;
    }
    if (Rast_read_cats(inraster, "", &cats) >= 0) {
	sprintf(title, "Random points on <%s>", inraster);
	Rast_set_cats_title(title, &cats);
	if (theState->use_nulls)
	    Rast_set_cat(nulls.data.v,
			     nulls.data.v,
			     "Points with NULL values in original",
			     &cats, nulls.type);
	Rast_write_cats(theState->outraster, &cats);
    }

    /* write history for output raster */
    if (Rast_read_history(theState->outraster, G_mapset(), &hist) >= 0) {
	Rast_short_history(theState->outraster, "raster", &hist);
	Rast_format_history(&hist, HIST_DATSRC_1, "Based on map <%s>", inraster);
	if (percent)
	    Rast_format_history(
		&hist, HIST_DATSRC_2,
		"Random points over %.2f percent of the base map <%s>",
		percentage, inraster);
	else
	    Rast_format_history(
		&hist, HIST_DATSRC_2,
#ifdef HAVE_LONG_LONG_INT
		"%llu random points on the base map <%s>",
#else
		"%lu random points on the base map <%s>",
#endif
		theState->nRand, theState->inraster);

	Rast_command_history(&hist);
	Rast_write_history(theState->outraster, &hist);
    }

    /* write commandline to output vector */
    if (theState->outvector) {
	struct Map_info map;

	if (Vect_open_old(&map, theState->outvector, G_mapset()) < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"),
			    theState->outvector);
	Vect_hist_command(&map);
	Vect_close(&map);
    }

    /* set colors for output raster */
    if (Rast_read_colors(inraster, "", &clr) >= 0) {
	if (theState->use_nulls) {
	    Rast_add_color_rule(nulls.data.v, 127, 127, 127,
				nulls.data.v, 127, 127, 127, &clr,
				nulls.type);
	}
	Rast_write_colors(theState->outraster, G_mapset(), &clr);
    }

    return 0;
}
