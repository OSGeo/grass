#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <grass/gis.h>
#include "options.h"
#include "dhist.h"

static char *mk_command(const char *fmt, int nargs, ...)
{
    /* asprintf() would solve this problem better */
    size_t len = strlen(fmt) + 1;
    char *cmd;
    va_list ap;

    va_start(ap, nargs);

    while (nargs--) {
	cmd = va_arg(ap, char *);

	len += strlen(cmd);
    }

    va_end(ap);

    cmd = G_malloc(len);

    va_start(ap, nargs);
    vsprintf(cmd, fmt, ap);

    va_end(ap);

    return cmd;
}

int get_stats(char *mapname, char *mapset, struct stat_list *dist_stats,	/* linked list of stats */
	      int quiet)
{
    char buf[1024];		/* input buffer for reading stats */
    int done = 0;
    char *tempfile;		/* temp file name */
    char *fullname;
    char *cmd;
    FILE *fd;			/* temp file pointer */

    long int cat;		/* a category value */
    long int stat;		/* a category stat value */
    struct stat_node *ptr = NULL;
    int first;

    /* write stats to a temp file */
    tempfile = G_tempfile();
    fullname = G_fully_qualified_name(mapname, mapset);
    is_fp = G_raster_map_is_fp(mapname, mapset);
    if (is_fp) {
	if (cat_ranges) {
	    if (G_read_raster_cats(mapname, mapset, &cats) < 0)
		G_fatal_error("Can't read category file");
	    if (G_number_of_raster_cats(&cats) <= 0) {
		G_warning("There are no labeled cats, using nsteps argument");
		cat_ranges = 0;
	    }
	}
	if (G_read_fp_range(map_name, mapset, &fp_range) <= 0)
	    G_fatal_error("Can't read frange file");
    }
    if (cat_ranges) {
	cmd = mk_command("r.stats -Cr%s%s \"%s\" > \"%s\"\n", 4,
			 type == COUNT ? "c" : "a", quiet ? "q" : "",
			 fullname, tempfile);
    }
    else {
	sprintf(buf, "%d", nsteps);
	cmd = mk_command("r.stats -r%s%s \"%s\" nsteps=%s > \"%s\"\n", 5,
			 type == COUNT ? "c" : "a", quiet ? "q" : "",
			 fullname, buf, tempfile);
    }

    if (system(cmd))
	G_fatal_error("%s: ERROR running r.stats", G_program_name());

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
