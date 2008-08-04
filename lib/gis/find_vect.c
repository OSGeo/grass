/*
 **********************************************************************
 *  char *
 *  G_find_vector (name, mapset)
 *        char *name       file name to look for
 *        char *mapset     mapset to search. if mapset is ""
 *                         will search in mapset search list
 *
 *      searches for a vector map from the mapset search list
 *      or in a specified mapset.
 *      returns the mapset name where the vector map was found.
 *
 * NOTE: If the user specifies a fully qualified vector map which exists,
 *       then <i>G_find_vector()</i> modifies <b>name</b> by removing the
 *      "@<i>mapset</i>" part.
 *
 *  returns:
 *      char *  pointer to a string with name of mapset
 *              where vector map was found, or NULL if not found
 *  note:
 *      rejects all names that begin with .
 *
 *      if name is of the form nnn in ppp then 
 *      name = nnn and mapset = ppp
 **********************************************************************/

#include <grass/gis.h>
#include <grass/vect/dig_defines.h>


/* \brief searches for a vector map
 *
 *      searches for a vector map from the mapset search list
 *      or in a specified mapset.
 *      returns the mapset name where the vector map was found.
 *
 *  returns:
 *      char *  pointer to a string with name of mapset
 *              where vector map was found, or NULL if not found
 *  NOTES:
 *      If the user specifies a fully qualified vector map which exists,
 *      then <i>G_find_vector()</i> modifies <b>name</b> by removing the
 *      "@<i>mapset</i>" part.
 *
 *      Rejects all names that begin with "."
 *
 *      If name is of the form nnn in ppp then 
 *      name = nnn and mapset = ppp
 *
 *  \param name
 *  \param mapset
 *  \return char *
 *
 */
char *G_find_vector(char *name, const char *mapset)
{
    return G_find_file(GRASS_VECT_DIRECTORY, name, mapset);
}



/*!
 * \brief find a vector map (look but don't touch)
 *
 * The same as G_find_vector() but doesn't remove the "@<i>mapset</i>"
 * qualification from <b>name</b>, if present.
 *
 * Returns NULL if the map wasn't found, or the mapset the vector was
 * found in if it was.
 *
 *  \param name
 *  \param mapset
 *  \return char *
 */
char *G_find_vector2(const char *name, const char *mapset)
{
    return G_find_file2(GRASS_VECT_DIRECTORY, name, mapset);
}
