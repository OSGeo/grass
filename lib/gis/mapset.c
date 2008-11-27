
/**********************************************************************
 *
 *   char *
 *   G_mapset()
 *
 *   returns:    pointer to string containing the one word mapset
 *               name.
 *               NULL if user does not have access to mapset.
 *
 **********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/*!
 * \brief current mapset name
 *
 * Returns the name of the
 * current mapset in the current location. This routine is often used when
 * accessing files in the current mapset. See Mapsets for an
 * explanation of mapsets.
 *
 *  \param void
 *  \return char * 
 */

const char *G_mapset(void)
{
    const char *m = G__mapset();

    if (!m)
	G_fatal_error(_("MAPSET is not set"));

    return m;
}

const char *G__mapset(void)
{
    return G__getenv("MAPSET");
}
