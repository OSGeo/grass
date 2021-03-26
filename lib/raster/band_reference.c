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
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static const char *_band_file = "band_reference";
static const char *_band_dir = "band_meta";

/*!
   \brief Read band reference identifier from file (internal use only).

  \param fd file descriptor
  \param[out] key_val key/value pairs (filename, identifier)

  \return 1 on success
  \return -1 error - unable fetch key/value pairs
  \return -2 error - invalid band reference
*/
int Rast__read_band_reference(FILE *fd, struct Key_Value **key_val)
{
    const char *filename, *band_id;

    *key_val = G_fread_key_value(fd);
    if (!*key_val) {
        G_debug(1, "No band reference detected");
        return -1;
    }

    filename = G_find_key_value("file", *key_val);
    band_id = G_find_key_value("identifier", *key_val);
    if (!filename || !band_id) {
        G_debug(1, "Invalid band reference identifier");
        return -2;
    }

    G_debug(1, "Band reference <%s> (%s)", band_id, filename);

    return 1;
}

/*!
   \brief Write band reference identifier to file (internal use only).

   \param fd file descriptor
   \param filename filename JSON reference
   \param band_id band reference identifier

   \return 1 on success
   \return -1 error - unable to write key/value pairs into fileo
*/
int Rast__write_band_reference(FILE *fd,
                            const char *filename, const char *band_id)
{
    struct Key_Value *key_val;

    key_val = G_create_key_value();
    G_set_key_value("file", filename, key_val);
    G_set_key_value("identifier", band_id, key_val);

    if (G_fwrite_key_value(fd, key_val) < 0) {
        G_debug(1, "Error writing band reference file");
        G_free_key_value(key_val);
        return -1;
    }
    G_free_key_value(key_val);

    return 1;
}

/*!
  \brief Check if band reference for raster map exists

  \param name map name
  \param mapset mapset name

  \return 1 on success
  \return 0 no band reference present
*/
int Rast_has_band_reference(const char *name, const char *mapset)
{
    if (!G_find_file2_misc("cell_misc", _band_file, name, mapset))
	return 0;

    return 1;
}

/*!
   \brief Read raster map band reference identifier.

   Note that output arguments should be freed by the caller using G_free().

   \param name map name
   \param mapset mapset name
   \param[out] filename filename JSON reference
   \param[out] band_id band reference identifier

  \return 1 on success
  \return 0 band reference not found
  \return negative on error
 */
int Rast_read_band_reference(const char *name, const char *mapset,
                             char **filename, char **band_id)
{
    int ret;
    FILE *fd;
    struct Key_Value *key_val;

    G_debug(3, "Reading band reference file for raster map <%s@%s>",
            name, mapset);

    if (!Rast_has_band_reference(name, mapset))
        return 0;

    fd = G_fopen_old_misc("cell_misc", _band_file, name, mapset);
    if (!fd) {
        G_debug(1, "Unable to read band identifier file for <%s@%s>",
                name, mapset);
        return -1;
    }

    ret = Rast__read_band_reference(fd, &key_val);
    *filename = G_store(G_find_key_value("file", key_val));
    *band_id = G_store(G_find_key_value("identifier", key_val));
    fclose(fd);
    G_free_key_value(key_val);

    return ret;
}

/*!
   \brief Write raster map band reference identifier.

   \param name map name
   \param filename filename JSON reference
   \param band_id band reference identifier

   \return 1 on success
   \return negative on error
 */
int Rast_write_band_reference(const char *name,
                              const char *filename, const char *band_id)
{
    int ret;
    FILE *fd;

    G_debug(3, "Writing band reference file for raster map <%s>", name);

    fd = G_fopen_new_misc("cell_misc", _band_file, name);
    if (!fd) {
        G_fatal_error(_("Unable to create band file for <%s>"), name);
        return -1;
    }

    ret = Rast__write_band_reference(fd, filename, band_id);
    fclose(fd);

    return ret;
}

/*!
  \brief Remove band reference from raster map

  Only band reference files in current mapset can be removed.

  \param name map name

  \return 0 if no file
  \return 1 on success
  \return -1 on error
*/
int Rast_remove_band_reference(const char *name)
{
    return G_remove_misc("cell_misc", _band_file, name);
}

/*!
  \brief Check for legal band name.

  Legal band names must be legal GRASS file names.
  They are in format <shortcut>_<band>.
  This function will return -1 if provided band id is not considered
  to be valid.
  This function does not check if band ID maps to any entry in band
  metadata files as not all band IDs have files with extra metadata.

  The function prints a warning on error.

  \param band_id band name to check

  \return 1 success
  \return -1 failure
*/
int Rast_legal_band_id(const char *band_id)
{
    char **tokens;
    int ntok, i;

    if (G_legal_filename(band_id) != 1)
        return -1;

    tokens = G_tokenize(band_id, "_");
    ntok = G_number_of_tokens(tokens);
    if (ntok < 2) {
        G_warning(_("Illegal band name <%s>"), band_id);
        G_free_tokens(tokens);
        return -1;
    }

    if (strlen(tokens[1]) < 1) {
        G_free_tokens(tokens);
        return -1;
    }

    G_free_tokens(tokens);
    return 1;
}

/*!
   \brief Searches for band metadata file

   Searches in both GRASS built-in and also user managed band metadata files

   \param band_filename name of band metadata file

   \return pointer to a string with full path to metadata file,
           or NULL if not found
*/
char *Rast_find_band_filename(const char *filename) {
    char out_path[GPATH_MAX];

    G_snprintf(out_path, GPATH_MAX, "%s/etc/%s/%s", G_gisbase(), _band_dir, filename);
    if (access(out_path, 0) == 0)
        return G_store(out_path);

    G_snprintf(out_path, GPATH_MAX, "%s/%s/%s", G_config_path(), _band_dir, filename);
    if (access(out_path, 0) == 0)
        return G_store(out_path);

    return NULL;
}
