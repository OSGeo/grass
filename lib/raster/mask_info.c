/**
 * \file lib/raster/mask_info.c
 *
 * \brief Raster Library - Get mask information
 *
 * (C) 1999-2022 by Vaclav Petras and the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author CERL
 * \author Vaclav Petras, NC State University, Center for Geospatial Analytics
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/**
 * @brief Get a printable text with information about raster mask
 *
 * Determines if 2D raster mask is present and returns textual information about the
 * mask suitable for end-user display. The resulting text is translated.
 * Caller is responsible for freeing the memory of the returned string.
 *
 * @return New string with textual information
 */
char *Rast_mask_info(void)
{
    char text[GNAME_MAX + GMAPSET_MAX + 16];
    char name[GNAME_MAX];
    char mapset[GMAPSET_MAX];

    switch (Rast__mask_info(name, mapset)) {
    case 1:
        sprintf(text, _("<%s> in mapset <%s>"), name, mapset);
        break;
    case -1:
        strcpy(text, _("none"));
        break;
    default:
        strcpy(text, _("not known"));
        break;
    }

    return G_store(text);
}

/**
 * @brief Get raster mask status information
 *
 * @param[out] name Name of the raster map used as mask
 * @param[out] mapset Mapset name of the map
 * @param[out] is_mask_reclass True, if MASK raster is a reclass
 *
 * @return true if mask is present, false otherwise
 */
bool Rast_mask_status(char *name, char *mapset, bool *is_mask_reclass)
{
    int present = Rast__mask_info(name, mapset);

    if (is_mask_reclass) {
        if (present) {
            char rname[GNAME_MAX], rmapset[GMAPSET_MAX];

            *is_mask_reclass =
                Rast_is_reclass("MASK", G_mapset(), rname, rmapset) > 0;
        }
        else {
            *is_mask_reclass = false;
        }
    }

    if (present == 1)
        return true;
    else
        return false;
}

/**
 * @brief Get information about the current mask
 *
 * Determines the status of the automatic masking and the name of the 2D
 * raster which forms the mask. Typically, mask is raster called MASK in the
 * current mapset, but when used with r.mask, it is usually a reclassed
 * raster, and so when a MASK raster is present and it is a reclass raster,
 * the name and mapset of the underlying reclassed raster are returned.
 *
 * The name and mapset is written to the parameter which need to be defined
 * with a suffient size, least as `char name[GNAME_MAX], mapset[GMAPSET_MAX]`.
 *
 * When the masking is not active, -1 is returned and name and mapset are
 * undefined. When the masking is active, 1 is returned and name and mapset
 * will hold the name and mapset of the underlying raster.
 *
 * @param[out] name Name of the raster map used as mask
 * @param[out] mapset Mapset name of the map
 *
 * @return 1 if mask is present, -1 otherwise
 */
int Rast__mask_info(char *name, char *mapset)
{
    char rname[GNAME_MAX], rmapset[GMAPSET_MAX];

    strcpy(rname, "MASK");
    strcpy(rmapset, G_mapset());

    if (!G_find_raster(rname, rmapset))
        return -1;

    strcpy(name, rname);
    strcpy(mapset, rmapset);
    if (Rast_is_reclass(name, mapset, rname, rmapset) > 0) {
        strcpy(name, rname);
        strcpy(mapset, rmapset);
    }

    return 1;
}
