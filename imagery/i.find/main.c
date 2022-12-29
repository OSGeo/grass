
/****************************************************************************
 *
 * MODULE:       i.find
 * AUTHOR(S):    Markus Neteler <neteler itc.it> 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Paul Kelly <paul-grass stjohnspoint.co.uk>
 * PURPOSE:      produces a file containing the names of files of type
 *               element (cell, dig, etc) in the search path for the mapset 
 *               in location
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/************************************************************************
 * usage: i.find location mapset element file [element2 file2]...
 *
 * produces a file containing the names of files of type
 * element (cell, dig, etc) in the search path for the mapset in location
 *
 * the output file is in the format used by i.ask, which does a popup menu
 * of the files, and lets the user select one using the mouse
 *
 * at present this routine requires that both the current location/mapset
 * and the specified location/mapset be valid for the user.
 * I hope to remove this requirement some time.
 *
 * note: the list is created in other file, and when complete it is moved
 *       to the one specified on the command line. This allows programs
 *       to run this command in background and check for completion by
 *       looking for the file.
 *
 *       if there are no files, the list file is not created
 *
 ***********************************************************************/

#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/* function prototypes */
static int find(FILE * fd, char *element);


int main(int argc, char *argv[])
{
    char *tempfile;
    int n;

    if (argc < 5 || argc % 2 == 0)
	G_fatal_error(_("usage: %s location mapset element file."), argv[0]);

    G_gisinit(argv[0]);

    /*
     * this code assumes that the SEARCH PATH is not read
     * until we call G__mapset_name() in find()
     */
    tempfile = G_tempfile();

    G_setenv_nogisrc("LOCATION_NAME", argv[1]);
    G_setenv_nogisrc("MAPSET", argv[2]);

    for (n = 3; n < argc; n += 2) {
	FILE *fd;
	int ok;

	/* get this list into a temp file first */
	fd = fopen(tempfile, "w");
	if (fd == NULL)
	    G_fatal_error(_("Unable to open temp file."));

	remove(argv[n + 1]);
	ok = find(fd, argv[n]);
	fclose(fd);

	/* move the temp file to the real file
	 * this allows programs to run i.find in the background
	 * and check for completion by looking for the file
	 */
	if (ok)
	    G_rename_file(tempfile, argv[n + 1]);

	remove(tempfile);
    }
    G_free(tempfile);

    return 0;
}


static int find(FILE * fd, char *element)
{
    int len1 = 0, len2 = 0;
    const char *mapset;
    int n;

    G_fseek(fd, 0L, SEEK_SET);
    fwrite(&len1, sizeof(len1), 1L, fd);
    fwrite(&len2, sizeof(len2), 1L, fd);

    for (n = 0; ((mapset = G_get_mapset_name(n)) != NULL); n++) {
	int len;
	char dir[1024];
	struct dirent *dp;
	DIR *dfd;

	G_file_name(dir, element, "", mapset);
	if ((dfd = opendir(dir)) == NULL)
	    continue;

	len = strlen(mapset);
	if (len > len2)
	    len2 = len;

	while ((dp = readdir(dfd)) != NULL) {
	    if (dp->d_name[0] != '.') {
		fprintf(fd, "%s %s\n", dp->d_name, mapset);
		len = strlen(dp->d_name);
		if (len > len1)
		    len1 = len;
	    }
	}

	closedir(dfd);
    }

    if (len1 == 0 || len2 == 0)
	return 0;

    fflush(fd);
    G_fseek(fd, 0L, SEEK_SET);
    fwrite(&len1, sizeof(len1), 1L, fd);
    fwrite(&len2, sizeof(len2), 1L, fd);

    return 1;
}
