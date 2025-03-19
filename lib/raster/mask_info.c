/**
 * \file lib/raster/mask_info.c
 *
 * \brief Raster Library - Get mask information
 *
 * (C) 1999-2024 by Vaclav Petras and the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author CERL
 * \author Vaclav Petras, NC State University, Center for Geospatial Analytics
 */

#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/**
 * @brief Get a printable text with information about raster mask
 *
 * Determines if 2D raster mask is present and returns textual information about
 * the mask suitable for end-user display. The resulting text is translated.
 * Caller is responsible for freeing the memory of the returned string.
 *
 * @return New string with textual information
 *
 * @see Rast_mask_status()
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
 * @brief Retrieves the name of the raster mask to use.
 *
 * The returned raster map name is fully qualified, i.e., in the form
 * "name@mapset". The mask name is returned whether the mask is present or not.
 *
 * This function checks if an environment variable "GRASS_MASK" is set.
 * If it is set, the value of the environment variable is returned
 * as the mask name. If it is not set, the function will default to the
 * mask name "MASK@<mapset>", where <mapset> is the current mapset.
 *
 * The memory for the returned mask name is dynamically allocated using
 * G_store(). It is the caller's responsibility to free the memory with
 * G_free() when it is no longer needed.
 *
 * @returns A dynamically allocated string containing the mask name.
 */
char *Rast_mask_name(void)
{
    // First, see if the environment variable is defined.
    const char *env_variable = getenv("GRASS_MASK");
    if (env_variable != NULL && strcmp(env_variable, "") != 0) {
        // Variable exists and is not empty.
        // While the function does not document that, the provided mapset
        // is a fallback, so we don't have to parse the name to find out
        // ourselves what to do.
        return G_fully_qualified_name(env_variable, G_mapset());
    }

    // Mask name defaults to "MASK@<current mapset>".
    return G_fully_qualified_name("MASK", G_mapset());
}

/**
 * @brief Get name of a mask if it is present
 *
 * Unlike, Rast__mask_info() this always returns name of the mask
 * if it is present regardless of the mask being a reclass or not.
 *
 * @param[out] name Name of the raster map used as mask
 * @param[out] mapset Name of the map's mapset
 *
 * @return true if mask is present, false otherwise
 */
static bool Rast__get_present_mask(char *name, char *mapset)
{
    char rname[GNAME_MAX], rmapset[GMAPSET_MAX];
    char *full_name = Rast_mask_name();
    if (!G_find_raster2(full_name, ""))
        return false;
    G_unqualified_name(full_name, "", rname, rmapset);
    strncpy(name, rname, GMAPSET_MAX);
    strncpy(mapset, rmapset, GMAPSET_MAX);
    G_free(full_name);
    return true;
}

/**
 * @brief Get raster mask status information
 *
 * _is_mask_reclass_ is a pointer to a bool variable which
 * will be set to true if mask raster is a reclass and false otherwise.
 *
 * If you are not interested in the underlying reclassified raster map,
 * pass NULL pointers for the three reclass parameters:
 *
 * ```
 * Rast_mask_status(name, mapset, NULL, NULL, NULL);
 * ```
 *
 * @param[out] name Name of the raster map used as mask
 * @param[out] mapset Name of the mapset the raster is in
 * @param[out] is_mask_reclass Will be set to true if mask raster is a reclass
 * @param[out] reclass_name Name of the underlying reclassified raster map
 * @param[out] reclass_mapset Name of the mapset the reclassified raster is in
 *
 * @return true if mask is present, false otherwise
 */
bool Rast_mask_status(char *name, char *mapset, bool *is_mask_reclass,
                      char *reclass_name, char *reclass_mapset)
{
    bool present = Rast__get_present_mask(name, mapset);

    if (is_mask_reclass && reclass_name && reclass_mapset) {
        if (present) {
            *is_mask_reclass =
                Rast_is_reclass(name, mapset, reclass_name, reclass_mapset) > 0;
        }
        else {
            *is_mask_reclass = false;
        }
    }
    return present;
}

/**
 * @brief Get information about the current mask
 *
 * Determines the status of the automatic masking and the name of the 2D
 * raster which forms the mask. Typically, mask is raster called MASK in the
 * current mapset, but when used with r.mask, it is usually a reclassed
 * raster, and so when a mask raster is present and it is a reclass raster,
 * the name and mapset of the underlying reclassed raster are returned.
 *
 * The name and mapset is written to the parameter which need to be defined
 * with a sufficient size, least as `char name[GNAME_MAX], mapset[GMAPSET_MAX]`.
 *
 * When the masking is not active, -1 is returned and name and mapset are
 * undefined. When the masking is active, 1 is returned and name and mapset
 * will hold the name and mapset of the underlying raster.
 *
 * @param[out] name Name of the raster map used as mask
 * @param[out] mapset Name of the map's mapset
 *
 * @return 1 if mask is present, -1 otherwise
 *
 * @see Rast_mask_status(), Rast_mask_name()
 */
int Rast__mask_info(char *name, char *mapset)
{
    char rname[GNAME_MAX], rmapset[GMAPSET_MAX];
    bool present = Rast__get_present_mask(name, mapset);
    if (!present)
        return -1;

    if (Rast_is_reclass(name, mapset, rname, rmapset) > 0) {
        strcpy(name, rname);
        strcpy(mapset, rmapset);
    }
    return 1;
}

/**
 * @brief Check presence of 2D raster mask
 *
 * @return true if mask is present, false otherwise
 */
bool Rast_mask_is_present(void)
{
    char *name = Rast_mask_name();
    bool present = G_find_raster2(name, "") != NULL;
    return present;
}
