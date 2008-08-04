#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int WindowRange(char *name, char *mapset, long *min, long *max)
{
    char inbuf[512];		/* input buffer for reading stats */
    int done = 0;
    char stats_cmd[512];	/* string for r.stats command */
    char *temp_fname;		/* temp file name */
    FILE *temp_file;		/* temp file pointer */
    long int cat;		/* a category value */
    long int stat;		/* a category stat value */
    int first;

    /* write stats to a temp file */
    temp_fname = G_tempfile();
    sprintf(stats_cmd, "r.stats -ci %s > %s\n", name, temp_fname);
    system(stats_cmd);

    /* open temp file and read the stats into a linked list */
    temp_file = fopen(temp_fname, "r");

    first = 1;
    while (!done) {
	if (fgets(inbuf, 1024, temp_file) != NULL) {
	    if (sscanf(inbuf, "%ld %ld", &cat, &stat) == 2) {
		if (first) {
		    *max = cat;
		    *min = cat;
		    first = 0;
		}
		else {
		    if (cat > *max)
			*max = cat;
		    if (cat < *min)
			*min = cat;
		}
	    }
	    else
		done = 1;
	}
	else
	    done = 1;
    }

    return 0;
}

int quick_range(char *name, char *mapset, long *min, long *max)
{
    struct Range range;
    struct FPRange fprange;
    CELL xmin, xmax;
    DCELL fpxmin, fpxmax;

    switch (G_raster_map_type(name, mapset)) {
    case CELL_TYPE:
	if (G_read_range(name, mapset, &range) <= 0)
	    return 0;
	G_get_range_min_max(&range, &xmin, &xmax);
	*max = xmax;
	*min = xmin;
	break;
    default:
	if (G_read_fp_range(name, mapset, &fprange) <= 0)
	    return 0;
	G_get_fp_range_min_max(&fprange, &fpxmin, &fpxmax);
	*max = (long)fpxmax;
	*min = (long)fpxmin;
	break;
    }
    return 1;
}

int slow_range(char *name, char *mapset, long *min, long *max)
{
    FILE *fd;
    int first;
    long n;
    int ok;
    char buf[512];

    *min = *max = 0;

    G_message(_("one moment ..."));
    sprintf(buf, "Gdescribe -r -1 '%s in %s'", name, mapset);
    fd = popen(buf, "r");
    if (fd == NULL)
	return 0;
    ok = 1;
    first = 1;
    while (ok && fgets(buf, sizeof(buf), fd)) {
	ok = (sscanf(buf, "%ld", &n) == 1);
	if (!ok)
	    break;
	if (n == 0)
	    continue;
	*max = n;
	if (first)
	    *min = n;
	first = 0;
    }
    pclose(fd);
    if (!ok)
	*min = *max = 0;
    return ok;
}
