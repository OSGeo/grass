/*
 *************************************************************************
 *   char *
 *   G_ask_cell_new(prompt, name)) 
 *       asks user to input name of a new cell file
 *
 *   char *
 *   G_ask_cell_old(prompt, name) 
 *       asks user to input name of an existing cell file
 *
 *   char *
 *   G_ask_cell_in_mapset(prompt, name)
 *       asks user to input name of an existing cell file in current mapset
 *
 *   char *
 *   G_ask_cell_any(prompt, name)
 *       asks user to input name of a new or existing cell file in
 *       the current mapset. Warns user about (possible) overwrite
 *       if cell file already exists
 *
 *   parms:
 *      const char *prompt    optional prompt for user
 *      char *name            buffer to hold name of map found
 *
 *   returns:
 *      char *pointer to a string with name of mapset
 *       where file was found, or NULL if not found
 *
 *   note:
 *      rejects all names that begin with .
 **********************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static int lister(char *, char *, char *);


/*!
 * \brief prompt for new raster map
 *
 * Asks the user to enter a name for a raster map which does not
 * exist in the current mapset.
 *
 *  \param prompt
 *  \param name
 *  \return char * 
 */

char *G_ask_cell_new(const char *prompt, char *name)
{

    return G_ask_new_ext(prompt, name, "cell", "raster", _("with titles"),
			 lister);
}


/*!
 * \brief prompt for existing raster map
 *
 * Asks the user to enter the name of an existing raster
 * file in any mapset in the database.
 *
 *  \param prompt
 *  \param name
 *  \return char * 
 */

char *G_ask_cell_old(const char *prompt, char *name)
{
    return G_ask_old_ext(prompt, name, "cell", "raster", _("with titles"),
			 lister);
}


/*!
 * \brief prompt for existing raster map
 *
 * Asks the user to enter the name of an existing raster
 * file in the current mapset.
 *
 *  \param prompt
 *  \param name
 *  \return char * 
 */

char *G_ask_cell_in_mapset(const char *prompt, char *name)
{
    return G_ask_in_mapset_ext(prompt, name, "cell", "raster",
			       _("with titles"), lister);
}

char *G_ask_cell_any(const char *prompt, char *name)
{
    return G_ask_any_ext(prompt, name, "cell", "raster", 1, _("with titles"),
			 lister);
}

static int lister(char *name, char *mapset, char *buf)
{
    char *title;

    *buf = 0;
    if (*name == 0)
	return 0;

    strcpy(buf, title = G_get_cell_title(name, mapset));
    if (*buf == 0)
	strcpy(buf, _("(no title)"));
    G_free(title);

    return 0;
}
