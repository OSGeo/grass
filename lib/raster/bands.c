/*!
 * \file lib/raster/bands.c
 *
 * \brief Raster Library - Band reference support
 *
 * (C) 2019 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Martin Landa
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static const char *_band_file = "band";

/*!
   \brief Get raster map band reference identifier.

   Note that G_free_key_value() must called by a caller.

   \param name map name
   \param mapset mapset name

   \return band identifier key value pairs
   \return NULL if missing
 */
struct Key_Value *Rast_read_band_reference(const char *name, const char *mapset)
{
    FILE *fp;
    struct Key_Value *key_val;
    const char *filename, *band_ref;

    if (!G_find_file2_misc("cell_misc", _band_file, name, mapset)) {
        G_debug(1, "Band identifier not found for <%s@%s>", name, mapset);
        return NULL;
    }

    fp = G_fopen_old_misc("cell_misc", _band_file, name, mapset);
    if (!fp) {
        G_debug(1, "Unable to read band indentier file for <%s@%s>",
                name, mapset);
        return NULL;
    }

    key_val = G_fread_key_value(fp);
    fclose(fp);
    if (!key_val) {
        G_debug(1, "Unable to parse band indentier key value pairs for <%s@%s>",
                name, mapset);
	return NULL;
    }

    filename = G_find_key_value("file", key_val);
    band_ref = G_find_key_value("identifier", key_val);
    if (!filename || !band_ref) {
        G_debug(1, "Invalid band identifier: unable to parse values for <%s@%s>",
                name, mapset);
        G_free_key_value(key_val);

	return NULL;
    }

    G_debug(1, "Band idenfifier <%s> (%s) detected for <%s@%s>",
            band_ref, filename, name, mapset);

    return key_val;
}

/*!
   \brief Set raster map band reference identifier.

   \param name map name
   \param filename filename JSON reference (NULL to unset)
   \param band_reference band reference identifier (NULL to unset)

   \return 0 success
 */
int Rast_write_band_reference(const char *name,
                              const char *filename, const char *band_reference)
{
    FILE *fp;
    struct Key_Value *key_val;

    if (filename && band_reference) {
        /* set */
        key_val = G_create_key_value();
        G_set_key_value("file", filename, key_val);
        G_set_key_value("identifier", band_reference, key_val);

        fp = G_fopen_new_misc("cell_misc", _band_file, name);
        if (!fp)
            G_fatal_error(_("Unable to create band file for <%s>"), name);

        if (G_fwrite_key_value(fp, key_val) < 0)
            G_fatal_error(_("Error writing band file for <%s>"), name);

        G_free_key_value(key_val);

        fclose(fp);
    }
    else {
        /* unset */
        if (G_find_file2_misc("cell_misc", _band_file, name, G_mapset()))
            G_remove_misc("cell_misc", _band_file, name);
    }

    return 0;
}
