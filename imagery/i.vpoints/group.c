#include <string.h>
#include <stdlib.h>
#include "globals.h"

static int cmp(const void *, const void *);

int prepare_group_list(void)
{
    FILE *fd;
    int *idx;
    int n;
    int len, len1, len2;

    /* open file to store group file names */
    fd = fopen(group_list, "w");
    if (fd == NULL)
	G_fatal_error("Can't open any tempfiles");

    /*
     * build sorted index into group files
     * so that all raster maps for a mapset to appear together
     */
    idx = (int *)G_calloc(group.ref.nfiles, sizeof(int));
    for (n = 0; n < group.ref.nfiles; n++)
	idx[n] = n;
    qsort(idx, group.ref.nfiles, sizeof(int), cmp);

    /* determine length of longest mapset name, and longest raster map name */
    len1 = len2 = 0;
    for (n = 0; n < group.ref.nfiles; n++) {
	len = strlen(group.ref.file[n].name);
	if (len > len1)
	    len1 = len;
	len = strlen(group.ref.file[n].mapset);
	if (len > len2)
	    len2 = len;
    }

    /* write lengths, names to file */
    fwrite(&len1, sizeof(len1), 1, fd);
    fwrite(&len2, sizeof(len2), 1, fd);
    for (n = 0; n < group.ref.nfiles; n++)
	fprintf(fd, "%s %s\n", group.ref.file[idx[n]].name,
		group.ref.file[idx[n]].mapset);
    fclose(fd);

    G_free(idx);

    return 0;
}

static int cmp(const void *aa, const void *bb)
{
    const int *a = aa, *b = bb;
    int n;

    if (n = strcmp(group.ref.file[*a].mapset, group.ref.file[*b].mapset))
	return n;
    return strcmp(group.ref.file[*a].name, group.ref.file[*b].name);
}

/* ask the user to pick a file */
int choose_groupfile(char *name, char *mapset)
{
    int stat;

    stat = ask_gis_files("raster", group_list, name, mapset, -1);
    return (stat);
}
