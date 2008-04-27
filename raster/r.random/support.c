#include <grass/gis.h>
#include <grass/Vect.h>
#include "local_proto.h"


int make_support (struct rr_state *theState, int percent, double percentage)
{
    char title[100];
    struct History hist;
    struct Categories cats;
    struct Colors clr;
    
    /* write categories for output raster */
    if(G_read_raster_cats (theState->inraster, theState->mapset, &cats) >= 0)
    {
	sprintf (title, "Random sites on [%s in %s]", 
                theState->inraster, theState->mapset);
	G_set_cats_title (title, &cats);
	if (theState->use_nulls)
	    G_set_raster_cat (theState->nulls.data.v,
                    theState->nulls.data.v,  
                    "Sites with NULL values in original",
                    &cats,
                    theState->nulls.type);
	G_write_raster_cats (theState->outraster, &cats);
    }

    /* write history for output raster */
    if (G_read_history (theState->outraster, G_mapset(), &hist) >= 0)
    {
        G_short_history(theState->outraster, "raster", &hist);
	sprintf (hist.datsrc_1, "Based on map [%s in %s]", 
                theState->inraster, theState->outraster);
	if (percent)
	    sprintf (hist.datsrc_2, "Random sites over %.2f percent of the base map <%s>",
                    percentage, theState->inraster);
	else
	    sprintf (hist.datsrc_2, "%ld random sites on the base map <%s>", 
                    theState->nRand, theState->inraster);
	G_write_history (theState->outraster, &hist);

    }

    /* write commandline to output vector */
    if (theState->outvector)
    {
        struct Map_info map;

        Vect_open_old (&map, theState->outvector, G_mapset ());
        Vect_hist_command (&map);
        Vect_close (&map);
    }

    /* set colors for output raster */
    if (G_read_colors (theState->inraster, theState->mapset, &clr) >= 0)
    {
        if (theState->use_nulls)
        {
            G_add_raster_color_rule (theState->nulls.data.v, 127, 127, 127, 
                    theState->nulls.data.v, 127, 127, 127, &clr,
                    theState->nulls.type);
        }
        G_write_colors (theState->outraster, G_mapset(), &clr);
    }
        
    return 0;
}
