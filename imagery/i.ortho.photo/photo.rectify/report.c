#include "global.h"
int 
report (char *name, char *mapset, char *result, long rectify, long compress, int ok)
{
    int minutes, hours;
    long seconds;
    long ncells;

    select_current_env();
    fprintf (stderr, "***********************************************\n");
    fprintf (stderr, "Rectify [%s in %s] (LOCATION %s)\n",
	name, mapset, G_location());
    fprintf (stderr, " into  [%s in ", result);
    select_target_env();
    fprintf (stderr, "%s] (LOCATION %s)\n", G_mapset(), G_location());
    fprintf (stderr, "%s\n", ok?"complete":"failed");
    fprintf (stderr, "-----------------------------------------------\n");
    select_current_env();

    if (!ok)
	return 1;

    seconds = rectify;
    minutes = seconds/60;
    hours = minutes/60;
    minutes -= hours * 60;
    ncells = target_window.rows * target_window.cols;
    fprintf (stderr, " %d rows, %d cols (%ld cells) completed in ",
	target_window.rows, target_window.cols, ncells);
    if (hours)
	fprintf (stderr, "%d:%02d:%02ld\n", hours, minutes, seconds%60);
    else
	fprintf (stderr, "%d:%02ld\n", minutes, seconds%60);
    if (seconds)
	fprintf (stderr, " %.1f cells per minute\n", (60.0*ncells) / ((double) seconds));

    fprintf (stderr, "\n");

    seconds = compress;

    if (seconds <= 0)
	return 1;

    minutes = seconds/60;
    hours = minutes/60;
    minutes -= hours * 60;
    fprintf (stderr, " data compression required an additional ");
    if (hours)
	fprintf (stderr, "%d:%02d:%02ld\n", hours, minutes, seconds%60);
    else
	fprintf (stderr, "%d:%02ld\n", minutes, seconds%60);

    return 0;
}
