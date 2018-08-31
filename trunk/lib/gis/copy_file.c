
/****************************************************************************
 *
 * MODULE:       GRASS GIS library - copy_file.c
 * AUTHOR(S):    Paul Kelly
 * PURPOSE:      Function to copy one file to another.
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <grass/gis.h>

/**
 * \brief Copies one file to another
 * 
 * Creates a copy of a file. The destination file will be overwritten if it
 * already exists, so the calling module should check this first if it is
 * important.
 * 
 * \param infile  String containing path to source file
 * \param outfile String containing path to destination file
 * 
 * \return 1 on success; 0 if an error occurred (warning will be printed)
 **/

int G_copy_file(const char *infile, const char *outfile)
{
    FILE *infp, *outfp;
    int inchar, outchar;

    infp = fopen(infile, "r");
    if (infp == NULL) {
	G_warning("Cannot open %s for reading: %s", infile, strerror(errno));
	return 0;
    }

    outfp = fopen(outfile, "w");
    if (outfp == NULL) {
	G_warning("Cannot open %s for writing: %s", outfile, strerror(errno));
	return 0;
    }

    while ((inchar = getc(infp)) != EOF) {
	/* Read a character at a time from infile until EOF
	 * and copy to outfile */
	outchar = putc(inchar, outfp);
	if (outchar != inchar) {
	    G_warning("Error writing to %s", outfile);
	    return 0;
	}
    }
    fflush(outfp);

    fclose(infp);
    fclose(outfp);

    return 1;
}
