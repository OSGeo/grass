#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/* function prototypes */
static int write_history(int, char *, double **, double *);


int write_support(int bands, char *outname, double **eigmat, double *eigval)
{
    const char *mapset = G_mapset();
    struct Colors colors;
    struct FPRange range;
    DCELL min, max;

    /* make grey scale color table */
    G_read_fp_range(outname, mapset, &range);
    G_get_fp_range_min_max(&range, &min, &max);

    G_make_grey_scale_fp_colors(&colors, min, max);

    if (G_raster_map_is_fp(outname, mapset))
	G_mark_colors_as_fp(&colors);

    if (G_write_colors(outname, mapset, &colors) < 0)
	G_message(_("Unable to write color table for raster map <%s>"), outname);

    return write_history(bands, outname, eigmat, eigval);
}


static int write_history(int bands, char *outname, double **eigmat, double *eigval)
{
    int i, j;
    static int first_map = TRUE;     /* write to stderr? */
    struct History hist;
    double eigval_total = 0.0;

    G_short_history(outname, "raster", &hist);
    sprintf(hist.edhist[0], "Eigen values, (vectors), and [percent importance]:");

    if(first_map)
	G_message(_("Eigen values, (vectors), and [percent importance]:"));

    for (i = 0; i < bands; i++)
	eigval_total += eigval[i];

    for (i = 0; i < bands; i++) {
	char tmpeigen[256], tmpa[80];

	sprintf(tmpeigen, "PC%d %9.2f ( ", i+1, eigval[i]);
	for (j = 0; j < bands; j++) {
	    sprintf(tmpa, "%5.2f ", eigmat[i][j]);
	    strcat(tmpeigen, tmpa);
	}

	strcat(tmpeigen, ") ");
	
	sprintf(tmpa, "[ %5.2f%% ]", eigval[i] * 100/eigval_total);
	strcat(tmpeigen, tmpa);

	sprintf(hist.edhist[i + 1], tmpeigen);

	/* write eigen values to screen */
	if(first_map)
	    G_message("%s", tmpeigen);
    }

    hist.edlinecnt = i + 1;
    G_command_history(&hist);

    /* only write to stderr the first time (this fn runs for every output map) */
    first_map = FALSE;

    return G_write_history(outname, &hist);
}
