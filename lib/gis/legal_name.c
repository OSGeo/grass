
/**
 * \file legal_name.c
 *
 * \brief GIS Library - Functions to handle file name legality.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/**
 * \brief Check for legal database file name.
 *
 * Legal file names will <b>not</b> begin with '.' or NULL and must 
 * not contain the characters, ' ' (space), '/', '"'. '\'' (single 
 * quote), '@', ',', '=', '*', and all other non-alphanumeric
 * characters within.
 *
 * \param[in] s file name to check
 * \return 1 success
 * \return -1 failure
 */

int G_legal_filename(const char *s)
{
    if (*s == '.' || *s == 0) {
	fprintf(stderr, _("Illegal filename.  Cannot be '.' or 'NULL'\n"));
	return -1;
    }

    for (; *s; s++)
	if (*s == '/' || *s == '"' || *s == '\'' || *s <= ' ' ||
	    *s == '@' || *s == ',' || *s == '=' || *s == '*' || *s > 0176) {
	    fprintf(stderr,
		    _("Illegal filename. Character <%c> not allowed.\n"), *s);
	    return -1;
	}

    return 1;
}


/**
 * \brief Check input and output file names.
 *
 * Check: 1) output is legal map name, 2) if can find input map, and 3) 
 * if input was found in current mapset, check if input != output.
 *
 * \param[in] input name
 * \param[out] output name
 * \param[in] error error type: GR_FATAL_EXIT, GR_FATAL_PRINT, GR_FATAL_RETURN
 * \return 0 OK
 * \return 1 error
 */

int G_check_input_output_name(const char *input, const char *output,
			      int error)
{
    char *mapset;

    if (output == NULL)
	return 0;		/* don't die on undefined parameters */
    if (G_legal_filename(output) == -1) {
	if (error == GR_FATAL_EXIT) {
	    G_fatal_error(_("Output raster map name <%s> is not valid map name"),
			  output);
	}
	else if (error == GR_FATAL_PRINT) {
	    G_warning(_("Output raster map name <%s> is not valid map name"),
		      output);
	    return 1;
	}
	else {			/* GR_FATAL_RETURN */
	    return 1;
	}
    }

    mapset = G_find_cell2(input, "");

    if (mapset == NULL) {
	if (error == GR_FATAL_EXIT) {
	    G_fatal_error(_("Raster map <%s> not found"), input);
	}
	else if (error == GR_FATAL_PRINT) {
	    G_warning(_("Raster map <%s> not found"), input);
	    return 1;
	}
	else {			/* GR_FATAL_RETURN */
	    return 1;
	}
    }

    if (strcmp(mapset, G_mapset()) == 0) {
	char nm[1000], ms[1000];
	const char *in;

	if (G__name_is_fully_qualified(input, nm, ms)) {
	    in = nm;
	}
	else {
	    in = input;
	}

	if (strcmp(in, output) == 0) {
	    if (error == GR_FATAL_EXIT) {
		G_fatal_error(_("Output raster map <%s> is used as input"),
			      output);
	    }
	    else if (error == GR_FATAL_PRINT) {
		G_warning(_("Output raster map <%s> is used as input"),
			  output);
		return 1;
	    }
	    else {		/* GR_FATAL_RETURN */
		return 1;
	    }
	}
    }

    return 0;
}
