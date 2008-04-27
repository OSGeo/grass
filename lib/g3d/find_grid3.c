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
 *  \param char *name       file name to look for
 *  \param char *mapset     mapset to search. if mapset is ""
 *                         will search in mapset search list
 *  \return char *  pointer to a string with name of mapset
 *              where cell file was found, or NULL if not found
 */

char *
G_find_grid3  (char *cell, char *mset)

{
    char name[256], mapset[256], element[512];

    if (cell == NULL || *cell == 0)
        return 0;

    if(G__name_is_fully_qualified (cell, name, mapset))
        sprintf (element, "grid3/%s", name);
    else
        sprintf (element, "grid3/%s", cell);

    return (G_find_file (element, "cell", mset));
    /* actually looks for the data, not the directory */

}
