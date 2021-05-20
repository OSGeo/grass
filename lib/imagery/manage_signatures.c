/*!
   \file lib/imagery/manage_sinatures.c

   \brief Imagery Library - Signature file management functions

   (C) 2021 by Maris Nartiss and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Maris Nartiss
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
 * \brief Remove a signature file
 * 
 * If removal fails, prints a warning and returns 1.
 * It is safe to pass fully qualified names.
 * 
 * \param type SIGFILE_TYPE_ signature type
 * \param name of signature to remove
 * \return 0 on success
 * \return 1 on failure
 */
int I_signatures_remove(int type, const char *name)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char selem[GNAME_MAX];

    G_debug(1, "I_signatures_remove(%d, %s);", type, name);

    /* Remove only if file is in the current mapset */
    if (G_name_is_fully_qualified(name, xname, xmapset) &&
        strcmp(xmapset, G_mapset()) != 0) {
        G_warning(_("%s is not in the current mapset (%s)"), name,
                  G_mapset());
        return 1;
    }
    if (I_find_signature2(type, name, G_mapset())) {
        if (type == SIGFILE_TYPE_SIG)
            sprintf(selem, "signatures%csig", HOST_DIRSEP);
        else if (type == SIGFILE_TYPE_SIGSET) {
            sprintf(selem, "signatures%csigset", HOST_DIRSEP);
        }
        if (G_remove(selem, name) == 1) {
            G_verbose_message(_("%s removed"), name);
            return 0;
        }
        G_warning(_("Unable to remove %s signature"), name);
    }
    else
        G_warning(_("%s is missing"), name);
    return 1;
}
