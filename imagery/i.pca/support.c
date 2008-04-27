#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/* function prototypes */
static int write_history (int, char *, double **);


int write_support (int bands, char *outname, double **eigmat)
{
    char *mapset = G_mapset ();
    struct Colors colors;
    struct FPRange range;
    DCELL min, max;

    /* make grey scale color table */
    G_read_fp_range (outname, mapset, &range);
    G_get_fp_range_min_max (&range, &min, &max);

    G_make_grey_scale_fp_colors (&colors, min, max);

    if (G_raster_map_is_fp (outname, mapset))
        G_mark_colors_as_fp (&colors);

    if (G_write_colors (outname, mapset, &colors) < 0)
        G_message (_("Cannot write color table of raster map <%s>"), outname);

    return write_history (bands, outname, eigmat);
}


static int
write_history (int bands, char *outname, double **eigmat)
{
    int i, j;
    static int to_std = 0;  /* write to stderr? */
    struct History hist;

    G_short_history (outname, "raster", &hist);
    sprintf (hist.edhist[0], "Eigen vectors:");

    if (!to_std)
        G_message (_("Eigen values:"));

    for (i = 0; i < bands; i++)
    {
        char tmpeigen[80], tmpa[80];

        sprintf (tmpeigen, "( ");
        for (j = 0; j < bands; j++)
        {
            sprintf (tmpa, "%.2f ", eigmat[i][j]);
            G_strcat (tmpeigen, tmpa);
        }
        G_strcat (tmpeigen, ")");

        sprintf (hist.edhist[i+1], tmpeigen);

        /* write eigen values to screen */
        if (!to_std)
            G_message ("%s", tmpeigen);
    }

    hist.edlinecnt = i + 1;
    G_command_history (&hist);

    /* only write to stderr the first time */
    to_std = 1;

    return G_write_history (outname, &hist);
}

