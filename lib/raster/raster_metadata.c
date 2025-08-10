/*!
   \file lib/raster/raster_metadata.c

   \brief Raster library - Functions to read and write raster "units",
   "semantic label" and "vertical datum" meta-data info

   (C) 2007-2009, 2021 by Hamish Bowman, Maris Nartiss,
   and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Hamish Bowman
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static char *misc_read_line(const char *, const char *, const char *);
static void misc_write_line(const char *, const char *, const char *);

/*!
 * \brief Get a raster map's units metadata string
 *
 * Read the raster's units metadata file and put string in str
 *
 * \param name raster map name
 * \param mapset mapset name
 *
 * \return  string representing units on success
 * \return  NULL on error
 */
char *Rast_read_units(const char *name, const char *mapset)
{
    return misc_read_line("units", name, mapset);
}

/*!
 * \brief Write a string to a raster map's units metadata file
 *
 * Raster map must exist in the current mapset.
 *
 * \param name raster map name
 * \param str  string containing data to be written
 */
void Rast_write_units(const char *name, const char *str)
{
    misc_write_line("units", name, str);
}

/*!
 * \brief Get a raster map's vertical datum metadata string
 *
 * Read the raster's vertical datum metadata file and put string in str
 *
 * \param name raster map name
 * \param mapset mapset name
 *
 * \return  string representing vertical datum on success
 * \return  NULL on error
 */
char *Rast_read_vdatum(const char *name, const char *mapset)
{
    return misc_read_line("vertical_datum", name, mapset);
}

/*!
 * \brief Write a string into a raster's vertical datum metadata file
 *
 * Raster map must exist in the current mapset.
 *
 * \param name raster map name
 * \param str  string containing data to be written
 */
void Rast_write_vdatum(const char *name, const char *str)
{
    misc_write_line("vertical_datum", name, str);
}

/*!
 * \brief Get a raster map semantic label metadata string
 *
 * Read raster semantic label metadata file and put string in to str
 *
 * \param name raster map name
 * \param mapset mapset name
 *
 * \return  string representing semantic label on success
 * \return  NULL on error
 */
char *Rast_read_semantic_label(const char *name, const char *mapset)
{
    return misc_read_line("semantic_label", name, mapset);
}

/*!
 * \brief Get a raster map semantic label or fall back to its name
 *
 * Use this function if a semantic label is needed but not mandated.
 *
 * \param name raster map name
 * \param mapset mapset name
 *
 * \return  string representing semantic label or map name
 */
char *Rast_get_semantic_label_or_name(const char *name, const char *mapset)
{
    char *buff;

    buff = Rast_read_semantic_label(name, mapset);
    return buff ? buff : G_store(name);
}

/*!
 * \brief Write a string into a rasters semantic label metadata file
 *
 * Raster map must exist in the current mapset.
 *
 * It is up to the caller to validate semantic label string in advance
 * with Rast_legal_semantic_label().
 *
 * \param name raster map name
 * \param str  string containing data to be written
 */
void Rast_write_semantic_label(const char *name, const char *str)
{
    misc_write_line("semantic_label", name, str);
}

/*!
 * \brief Check for legal semantic label
 *
 * Legal semantic label must be a legal GRASS file name.
 * Semantic labels are capped in length to GNAME_MAX.
 *
 * This function will return false if provided semantic label is not
 * considered to be valid.
 * This function does not check if semantic label maps to any entry in
 * metadata files of semantic labels as not all semantic labels have
 * files with extra metadata.
 *
 * The function prints a warning on error.
 *
 * \param semantic label reference to check
 *
 * \return true success
 * \return false failure
 */
bool Rast_legal_semantic_label(const char *semantic_label)
{
    const char *s;

    if (strlen(semantic_label) >= GNAME_MAX) {
        G_warning(_("Semantic label is too long"));
        return false;
    }

    if (G_legal_filename(semantic_label) != 1)
        return false;

    s = semantic_label;
    while (*s) {
        if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') ||
              (*s >= '0' && *s <= '9') || *s == '_' || *s == '-')) {
            G_warning(_("Character '%c' not allowed in a semantic label."), *s);
            return false;
        }
        s++;
    }

    return true;
}

/*!
 * \brief Read the first line of a file in cell_misc/
 *
 * Read the first line of data from a cell_misc/ meta-data file.
 *
 * \param element  metadata component filename
 * \param name
 * \param mapset
 * \return dynamically-allocated string on success
 * \return NULL on error
 */
static char *misc_read_line(const char *elem, const char *name,
                            const char *mapset)
{
    char buff[GNAME_MAX];
    FILE *fp;

    buff[0] = '\0';

    if (G_find_file2_misc("cell_misc", elem, name, mapset) == NULL)
        return NULL;

    fp = G_fopen_old_misc("cell_misc", elem, name, mapset);
    if (!fp) {
        G_warning(_("Unable to read <%s> for raster map <%s@%s>"), elem, name,
                  mapset);
        return NULL;
    }
    if (G_getl2(buff, sizeof(buff) - 1, fp) == 0) {
        /* file is empty */
        *buff = '\0';
    }

    if (fclose(fp) != 0)
        G_fatal_error(
            _("Error closing <%s> metadata file for raster map <%s@%s>"), elem,
            name, mapset);

    return *buff ? G_store(buff) : NULL;
}

/*!
 * \brief Write a line to a raster map metadata file
 *
 * Write (including overwrite) a string into a raster map's metadata file
 * found in in cell_misc/ in the current mapset.
 *
 * \param element  metadata component filename
 * \param name
 * \param *str  string containing data to be written
 */
static void misc_write_line(const char *elem, const char *name, const char *str)
{
    FILE *fp;

    fp = G_fopen_new_misc("cell_misc", elem, name);
    if (!fp) {
        G_fatal_error(
            _("Unable to create <%s> metadata file for raster map <%s@%s>"),
            elem, name, G_mapset());
    } /* This else block is unnecessary but helps to silence static code
         analysis tools */
    else {
        fprintf(fp, "%s\n", str);

        if (fclose(fp) != 0)
            G_fatal_error(
                _("Error closing <%s> metadata file for raster map <%s@%s>"),
                elem, name, G_mapset());
    }
}
