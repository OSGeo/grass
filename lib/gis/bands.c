/*!
 * \file lib/gis/bands.c
 *
 * \brief GIS Library - Band reference management (internal use only)
 *
 * (C) 2019 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Martin Landa
 */

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
   \brief Read band reference identifier from file (internal use only).

  \param fd file descriptor
  \param[out] key_val key/value pairs (filename, identifier)

  \return 1 on success
  \return -1 error - unable fetch key/value pairs
  \return -2 error - invalid band reference
 */
int G__read_band_reference(FILE *fd, struct Key_Value **key_val)
{
    const char *filename, *band_ref;

    *key_val = G_fread_key_value(fd);
    if (*key_val) {
        G_debug(1, "No band reference detected");
        return -1;
    }

    filename = G_find_key_value("file", *key_val);
    band_ref = G_find_key_value("identifier", *key_val);
    if (!filename || !band_ref) {
        G_debug(1, "Invalid band reference identifier");
        return -2;
    }

    G_debug(1, "Band idenfifier <%s> (%s)", band_ref, filename);

    return 1;
}

/*!
   \brief Write band reference identifier to file (internal use only).

   \param fd file descriptor
   \param filename filename JSON reference
   \param band_reference band reference identifier

   \return 1 on success
   \return -1 error - unable to write key/value pairs into fileo
*/
int G__write_band_reference(FILE *fd,
                            const char *filename, const char *band_reference)
{
    struct Key_Value *key_val;

    key_val = G_create_key_value();
    G_set_key_value("file", filename, key_val);
    G_set_key_value("identifier", band_reference, key_val);

    if (G_fwrite_key_value(fd, key_val) < 0) {
        G_debug(1, "Error writing band reference file");
        G_free_key_value(key_val);
        return -1;
    }
    G_free_key_value(key_val);

    return 1;
}

