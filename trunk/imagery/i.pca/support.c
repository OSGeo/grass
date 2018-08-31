#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* function prototypes */
static void write_history(int, char *, double **, double *);


void write_support(int bands, char *inname, char *outname, double **eigmat, double *eigval)
{
    const char *mapset = G_mapset();
    struct Colors colors;
    struct FPRange range;
    DCELL min, max;

    if (inname) {
	Rast_read_colors(inname, "", &colors);
    }
    else {
	/* make grey scale color table */
	Rast_read_fp_range(outname, mapset, &range);
	Rast_get_fp_range_min_max(&range, &min, &max);

	Rast_make_grey_scale_fp_colors(&colors, min, max);

    }

    if (Rast_map_is_fp(outname, mapset))
	Rast_mark_colors_as_fp(&colors);

    Rast_write_colors(outname, mapset, &colors);

    write_history(bands, outname, eigmat, eigval);
}


static void write_history(int bands, char *outname, double **eigmat, double *eigval)
{
    int i, j;
    static int first_map = TRUE;     /* write to stderr? */
    struct History hist;
    double eigval_total = 0.0;

    Rast_short_history(outname, "raster", &hist);
    Rast_append_history(&hist, "Eigen values, (vectors), and [percent importance]:");

    if(first_map)
	G_message(_("Eigen values, (vectors), and [percent importance]:"));

    for (i = 0; i < bands; i++)
	eigval_total += eigval[i];

    for (i = 0; i < bands; i++) {
	char tmpeigen[2048], tmpa[80];  /* (bands*8)+30 instead of 2048? */

	sprintf(tmpeigen, "PC%d %9.2f (", i+1, eigval[i]);
	for (j = 0; j < bands; j++) {
	    sprintf(tmpa, "%7.4f", eigmat[i][j]);
	    strcat(tmpeigen, tmpa);
	    if (j < (bands - 1) ){
		sprintf(tmpa, ",");
		strcat(tmpeigen, tmpa);
	    }
	}
	strcat(tmpeigen, ") ");
	
	sprintf(tmpa, "[%5.2f%%]", eigval[i] * 100 / eigval_total);
	strcat(tmpeigen, tmpa);

	Rast_append_history(&hist, tmpeigen);

	/* write eigen values to screen */
	if (first_map)
	    fprintf(stdout, "%s\n", tmpeigen);
    }

    /* only write to stderr the first time (this fn runs for every output map) */
    first_map = FALSE;

    Rast_command_history(&hist);
    Rast_write_history(outname, &hist);
}
