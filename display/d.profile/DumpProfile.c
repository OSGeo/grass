#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include "profile.h"

#define UCAT_STR_SIZE 50

char *_fmt_ucat(UCAT *, UCAT *, char *);

int WriteProfile(char *raster, char *mapset,
		 char *fname, char letter, struct Profile *profile)
{
    int proj;
    char *buf = G_calloc(UCAT_STR_SIZE, sizeof(char));
    char *buf2 = G_calloc(UCAT_STR_SIZE, sizeof(char));
    char *outfile = G_calloc(strlen(fname) + 3, sizeof(char));
    char coords[4][80];
    FILE *outFile;
    struct ProfileNode *ptr;

    memset(coords[0], 0, 80);
    memset(coords[1], 0, 80);
    memset(coords[2], 0, 80);
    memset(coords[3], 0, 80);

    /* Something's wrong if all of these are not NULL */
    if (raster == NULL || mapset == NULL || fname == NULL || profile == NULL)
	return -1;

    /* If no profiles were done, do nothing */
    if (profile->ptr == NULL || profile->count == 0)
	return 0;

    /* Each profile has the letter appended to the name */
    sprintf(outfile, "%s.%c", fname, letter);
    outFile = fopen(outfile, "w");
    if (outFile == NULL)
	return -1;

    proj = G_projection();

    /* Do the Header */
    fprintf(outFile, "# Profile %c of %s@%s\n", letter, raster, mapset);
    G_format_easting(profile->e1, coords[0], proj);
    G_format_northing(profile->n1, coords[1], proj);
    G_format_easting(profile->e2, coords[2], proj);
    G_format_northing(profile->n2, coords[3], proj);
    fprintf(outFile, "# From (%s, %s) to (%s, %s)\n",
	    coords[0], coords[1], coords[2], coords[3]);
    fprintf(outFile, "# Stats: Count = %ld, Min = %s, Max = %s\n",
	    profile->count,
	    _fmt_ucat(&profile->MinCat, &profile->MinCat, buf),
	    _fmt_ucat(&profile->MaxCat, &profile->MinCat, buf2));
    fprintf(outFile, "# dist value east north\n");

    /* Now loop through the nodes, one value per line */
    for (ptr = profile->ptr; ptr != NULL; ptr = ptr->next) {
	fprintf(outFile, "%f %s %f %f\n",
		ptr->dist,
		_fmt_ucat(&ptr->cat, &profile->MinCat, buf),
		ptr->east, ptr->north);
    }

    G_free(buf);
    G_free(buf2);
    G_free(outfile);

    fclose(outFile);

    return 0;
}				/* WriteProfile() */


/* Pointer should have space for at least 20 chars.
 * First UCAT is the value we're formatting.
 * Second UCAT is a minimum value (use it if NULL)
 * The char *s in is the same that goes out
 */
char *_fmt_ucat(UCAT * c, UCAT * min, char *s)
{
    memset(s, 0, UCAT_STR_SIZE);
    switch (c->type) {
    case CELL_TYPE:
	if (c->val.c == INT_MIN)
	    sprintf(s, "%d", min->val.c);
	else
	    sprintf(s, "%d", c->val.c);
	break;
    case FCELL_TYPE:
	if (c->val.f == FLT_MIN)
	    sprintf(s, "%f", min->val.f);
	else
	    sprintf(s, "%f", c->val.f);
	break;
    case DCELL_TYPE:
	if (c->val.d == DBL_MIN)
	    sprintf(s, "%lf", min->val.d);
	else
	    sprintf(s, "%lf", c->val.d);
	break;
    default:			/* Shouldn't happen */
	*s = '\0';
    }

    return s;
}


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
