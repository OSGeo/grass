/*!
 * \file lib/raster/band_reference.c
 *
 * \brief Raster Library - Band reference managenent
 *
 * (C) 2019 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Martin Landa (with financial support by mundialis, Bonn, for openEO EU H2020 grant 776242, https://openeo.org)
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static const char *_band_file = "band_reference";

/*!
  \brief Check if band reference for raster map exists

  \param name map name
  \param subproject subproject name

  \return 1 on success
  \return 0 no band reference present
*/
int Rast_has_band_reference(const char *name, const char *subproject)
{
    if (!G_find_file2_misc("cell_misc", _band_file, name, subproject))
	return 0;

    return 1;
}

/*!
   \brief Read raster map band reference identifier.

   Note that output arguments should be freed by the caller using G_free().

   \param name map name
   \param subproject subproject name
   \param[out] filename filename JSON reference
   \param[out] band_reference band reference identifier

  \return 1 on success
  \return 0 band reference not found
  \return negative on error
 */
int Rast_read_band_reference(const char *name, const char *subproject,
                             char **filename, char **band_reference)
{
    int ret;
    FILE *fd;
    struct Key_Value *key_val;

    G_debug(1, "Reading band reference file for raster map <%s@%s>",
            name, subproject);

    if (!Rast_has_band_reference(name, subproject))
        return 0;

    fd = G_fopen_old_misc("cell_misc", _band_file, name, subproject);
    if (!fd) {
        G_debug(1, "Unable to read band identifier file for <%s@%s>",
                name, subproject);
        return -1;
    }

    ret = G__read_band_reference(fd, &key_val);
    *filename = G_store(G_find_key_value("file", key_val));
    *band_reference = G_store(G_find_key_value("identifier", key_val));

    fclose(fd);
    G_free_key_value(key_val);

    return ret;
}

/*!
   \brief Write raster map band reference identifier.

   \param name map name
   \param filename filename JSON reference
   \param band_reference band reference identifier

   \return 1 on success
   \return negative on error
 */
int Rast_write_band_reference(const char *name,
                              const char *filename, const char *band_reference)
{
    int ret;
    FILE *fd;

    fd = G_fopen_new_misc("cell_misc", _band_file, name);
    if (!fd) {
        G_fatal_error(_("Unable to create band file for <%s>"), name);
        return -1;
    }

    ret = G__write_band_reference(fd, filename, band_reference);
    fclose(fd);

    return ret;
}

/*!
  \brief Remove band reference from raster map

  Only band reference files in current subproject can be removed.

  \param name map name

  \return 0 if no file
  \return 1 on success
  \return -1 on error
*/
int Rast_remove_band_reference(const char *name)
{
    return G_remove_misc("cell_misc", _band_file, name);
}
