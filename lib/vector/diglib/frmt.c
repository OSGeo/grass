/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <grass/vector.h>

/*!
  \brief Read external vector format file
 
  \param dascii format file (frmt)
  \param[out] finfo pointer to Format_info structure

  \return format code
  \return -1 on error
*/
int dig_read_frmt_ascii(FILE * dascii, struct Format_info *finfo)
{
    char buff[20001], buf1[1024];
    char *ptr;
    int frmt = -1;

    G_debug(3, "dig_read_frmt_ascii()");

    /* read first line which must be FORMAT: */
    if (G_getl2(buff, 2000, dascii)) {
	G_chop(buff);

	if (!(ptr = strchr(buff, ':'))) {
	    G_warning("Vector format not recognized: %s", buff);
	    return (-1);
	}

	strcpy(buf1, buff);
	buf1[ptr - buff] = '\0';

	ptr++;			/* Search for the start of text */
	while (*ptr == ' ')
	    ptr++;

	if (strcmp(buf1, "FORMAT") == 0) {
	    if (G_strcasecmp(ptr, "ogr") == 0) {
		frmt = GV_FORMAT_OGR;
	    }
	}
    }
    if (frmt == -1) {
	G_warning("Vector format not recognized: %s", buff);
	return (-1);
    }

    /* init format info values */
#ifdef HAVE_OGR
    finfo->ogr.dsn = NULL;
    finfo->ogr.layer_name = NULL;
#endif

    while (G_getl2(buff, 2000, dascii)) {
	G_chop(buff);

	if (!(ptr = strchr(buff, ':'))) {
	    G_warning("Format definition is not correct: %s", buff);
	    continue;
	}

	strcpy(buf1, buff);
	buf1[ptr - buff] = '\0';

	ptr++;			/* Search for the start of text */
	while (*ptr == ' ')
	    ptr++;

#ifdef HAVE_OGR
	if (strcmp(buf1, "DSN") == 0)
	    finfo->ogr.dsn = G_store(ptr);
	if (strcmp(buf1, "LAYER") == 0)
	    finfo->ogr.layer_name = G_store(ptr);
#endif
    }

    return frmt;
}

/* Write vector format, currently does not work
 *  Parse also connection string.
 *
 *  Returns: 0 OK
 *           -1 on error
 */
int dig_write_frmt_ascii(FILE * dascii, struct Format_info *finfo, int format)
{
    G_debug(3, "dig_write_frmt_ascii()");

    G_fatal_error("Format not supported by dig_write_frmt_ascii()");

    return 0;
}
