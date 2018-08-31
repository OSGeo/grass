#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/spawn.h>
#include "options.h"
#include "dhist.h"

static void run_stats(const char *mapname, const char *tempfile)
{
    char buf[32];
    const char *argv[12];
    int argc = 0;

    argv[argc++] = "r.stats";

    argv[argc++] = "-r";
    if (cat_ranges)
	argv[argc++] = "-C";

    argv[argc++] = type == COUNT
	? "-c"
	: "-a";

    argv[argc++] = mapname;

    if (!cat_ranges) {
	sprintf(buf, "nsteps=%d", nsteps);
	argv[argc++] = buf;
    }

    argv[argc++] = SF_REDIRECT_FILE;
    argv[argc++] = SF_STDOUT;
    argv[argc++] = SF_MODE_OUT;
    argv[argc++] = tempfile;

    argv[argc++] = NULL;

    if (G_vspawn_ex(argv[0], argv) != 0)
	G_fatal_error("error running r.stats");
}

int get_stats(const char *mapname, struct stat_list *dist_stats)	/* linked list of stats */
{
    char buf[1024];		/* input buffer for reading stats */
    int done = 0;
    char *tempfile;		/* temp file name */
    FILE *fd;			/* temp file pointer */

    long int cat;		/* a category value */
    long int stat;		/* a category stat value */
    struct stat_node *ptr = NULL;
    int first;

    /* write stats to a temp file */
    tempfile = G_tempfile();
    is_fp = Rast_map_is_fp(mapname, "");
    if (is_fp) {
	if (cat_ranges) {
	    if (Rast_read_cats(mapname, "", &cats) < 0)
		G_fatal_error("Can't read category file");
	    if (Rast_number_of_cats(&cats) <= 0) {
		G_warning("There are no labeled cats, using nsteps argument");
		cat_ranges = 0;
	    }
	}
	if (Rast_read_fp_range(map_name, "", &fp_range) <= 0)
	    G_fatal_error("Can't read frange file");
    }

    run_stats(mapname, tempfile);

    /* open temp file and read the stats into a linked list */
    fd = fopen(tempfile, "r");
    if (fd == NULL) {
	perror("opening r.stats output file");
	G_fatal_error("unable to continue.");
    }
    dist_stats->ptr = NULL;
    dist_stats->count = 0;
    dist_stats->sumstat = 0;

    first = 1;
    while (!done) {
	if (fgets(buf, sizeof(buf), fd) != NULL) {
	    /* WARNING!!!!!!
	     * this will be very wrong if type!=COUNT
	     * since the stat prodcued by r.stats will be a floating point value
	     * possibly less than 1 (shapiro)
	     */
	    if (sscanf(buf, "* %ld", &stat) == 1) {
		dist_stats->null_stat = stat;
		if (stat > dist_stats->maxstat && nodata)
		    dist_stats->maxstat = stat;
		if (stat < dist_stats->minstat && nodata)
		    dist_stats->minstat = stat;
		if (nodata)
		    dist_stats->sumstat += stat;
	    }
	    else if (sscanf(buf, "%ld %ld", &cat, &stat) == 2) {
		/* count stats */
		dist_stats->count++;

		/* sum stats */
		dist_stats->sumstat += stat;

		/* a max or a min stat? */
		if (first) {
		    dist_stats->maxstat = stat;
		    dist_stats->minstat = stat;
		    dist_stats->maxcat = cat;
		    dist_stats->mincat = cat;
		    first = 0;
		}
		if (stat > dist_stats->maxstat)
		    dist_stats->maxstat = stat;
		if (stat < dist_stats->minstat)
		    dist_stats->minstat = stat;

		/* a max or a min cat? */
		if (cat > dist_stats->maxcat)
		    dist_stats->maxcat = cat;
		if (cat < dist_stats->mincat)
		    dist_stats->mincat = cat;

		/* put it in the list */
		if (dist_stats->ptr == NULL) {
		    /* first in list */
		    dist_stats->ptr = (struct stat_node *)
			G_malloc(sizeof(struct stat_node));
		    dist_stats->ptr->cat = cat;
		    dist_stats->ptr->stat = stat;
		    dist_stats->ptr->next = NULL;
		    ptr = dist_stats->ptr;
		}
		else {
		    ptr->next = (struct stat_node *)
			G_malloc(sizeof(struct stat_node));
		    ptr->next->cat = cat;
		    ptr->next->stat = stat;
		    ptr->next->next = NULL;	/* mod: shapiro */
		    ptr = ptr->next;
		}
	    }
	}
	else
	    done = 1;
    }
    fclose(fd);
    unlink(tempfile);

    return 0;
}
