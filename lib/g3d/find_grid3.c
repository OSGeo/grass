#include <grass/gis.h>

/*     
 * \brief
 *      searches for a grid3 file from the mapset search list
 *      or in a specified mapset.
 *      returns the mapset name where the cell file was found.
 *
 *  note:
 *      rejects all names that begin with '.'
 *      if name is of the form nnn in ppp then
 *      name = nnn and mapset = ppp
 *
 *  \param const char *name       file name to look for
 *  \param const char *mapset     mapset to search. if mapset is ""
 *                                will search in mapset search list
 *  \return char *  pointer to a string with name of mapset
 *              where cell file was found, or NULL if not found
 */

const char *G_find_grid3(const char *name, const char *mapset)
{
    return G_find_file2_misc("grid3", "cell", name, mapset);
}
