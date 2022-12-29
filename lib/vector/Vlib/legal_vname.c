/*!
   \file lib/vector/Vlib/legal_vname.c

   \brief Vector library - Check if map name is legal vector map name

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <string.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief  Check if output is legal vector name.

   Rule:  [A-Za-z][A-Za-z0-9_]*

   Check also for SQL keywords.

   \param s filename to be checked

   \return 1 OK
   \return -1 if name does not start with letter A..Za..z or if name does not continue with A..Za..z0..9_
 */

int Vect_legal_filename(const char *s)
{
    /* full list of SQL keywords available at
       http://www.postgresql.org/docs/8.2/static/sql-keywords-appendix.html
     */
    static const char *keywords[] = { "and", "or", "not", NULL };
    char buf[GNAME_MAX];
    int i;

    sprintf(buf, "%s", s);

    if (*s == '.' || *s == 0) {
	G_warning(_("Illegal vector map name <%s>. May not contain '.' or 'NULL'."),
		  buf);
	return -1;
    }

    /* file name must start with letter */
    if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z'))) {
	G_warning(_("Illegal vector map name <%s>. Must start with a letter."),
		  buf);
	return -1;
    }

    for (s++; *s; s++)
	if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') ||
	      (*s >= '0' && *s <= '9') || *s == '_')) {
	    G_warning(_("Illegal vector map name <%s>. Character '%c' not allowed."),
		      buf, *s);
	    return -1;
	}

    for (i = 0; keywords[i]; i++)
	if (G_strcasecmp(buf, keywords[i]) == 0) {
	    G_warning(_("Illegal vector map name <%s>. SQL keyword cannot be used as vector map name."),
		      buf);
	    return -1;
	}

    return 1;
}

/*!
   \brief Check for input and output vector map name.

   Check
   - output is legal vector name
   - if can find input map
   - if input was found in current mapset, check if input != output

   \param input input name
   \param output output name
   \param error error type G_FATAL_EXIT, G_FATAL_PRINT, G_FATAL_RETURN

   \return 0 OK
   \return 1 error
 */

int Vect_check_input_output_name(const char *input, const char *output,
				 int error)
{
    const char *mapset;
    char inm[GNAME_MAX], ims[GMAPSET_MAX];
    char onm[GNAME_MAX], oms[GMAPSET_MAX];

    /* check for fully-qualified map name */
    if (G_name_is_fully_qualified(output, onm, oms)) {
        if (strcmp(oms, G_mapset()) != 0) {
	    if (error == G_FATAL_EXIT) {
		G_fatal_error(_("Output vector map name <%s> is not in the current mapset (%s)"),
			      output, G_mapset());
	    }
	    else if (error == G_FATAL_PRINT) {
		G_warning(_("Output vector map name <%s> is not in the current mapset (%s)"),
			  output, G_mapset());
		return 1;
	    }
	    else {			/* GV_FATAL_RETURN */
		return 1;
	    }
        }
        output = onm;
    }

    if (Vect_legal_filename(output) == -1) {
	if (error == G_FATAL_EXIT) {
	    G_fatal_error(_("Output vector map name <%s> is not SQL compliant"),
			  output);
	}
	else if (error == G_FATAL_PRINT) {
	    G_warning(_("Output vector map name <%s> is not SQL compliant"),
		      output);
	    return 1;
	}
	else {			/* GV_FATAL_RETURN */
	    return 1;
	}
    }

    if (G_name_is_fully_qualified(input, inm, ims)) {
	if (strcasecmp(ims, "ogr") != 0)
	    mapset = G_find_vector2(input, "");
	else
	    mapset = ims;
    }
    else
	mapset = G_find_vector2(input, "");

    if (mapset == NULL) {
	if (error == G_FATAL_EXIT) {
	    G_fatal_error(_("Vector map <%s> not found"), input);
	}
	else if (error == G_FATAL_PRINT) {
	    G_warning(_("Vector map <%s> not found"), input);
	    return 1;
	}
	else {			/* GV_FATAL_RETURN */
	    return 1;
	}
    }

    if (strcmp(mapset, G_mapset()) == 0) {
	if (G_name_is_fully_qualified(input, inm, ims)) {
	    input = inm;
	}

	if (strcmp(input, output) == 0) {
	    if (error == G_FATAL_EXIT) {
		G_fatal_error(_("Output vector map <%s> is used as input"),
			      output);
	    }
	    else if (error == G_FATAL_PRINT) {
		G_warning(_("Output vector map <%s> is used as input"),
			  output);
		return 1;
	    }
	    else {		/* GV_FATAL_RETURN */
		return 1;
	    }
	}
    }

    return 0;
}
