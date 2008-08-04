/* raster_metadata.c
 *
 * PURPOSE: functions to read and write raster "units" and "vertical datum"
 *              meta-data info
 *
 *   Copyright (C) 2007 by Hamish Bowman, and the GRASS Development Team
 *   Author(s): Hamish Bowman, Dunedin, New Zealand
 *
 *   This program is free software under the GNU General Public
 *   License (>=v2). Read the file COPYING that comes with GRASS
 *   for details.
 */
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/*!
 * \brief Get a raster map's units metadata string
 *
 * Read the raster's units metadata file and put string in str
 *
 * \param name
 * \param mapset
 * \param *str  string to be populated with data
 * \return 0 on success
 * \return -1, EOF (fclose() result) on error
 */
int G_read_raster_units(const char *name, const char *mapset, char *str)
{
    return G__raster_misc_read_line("units", name, mapset, str);
}


/*!
 * \brief Write a string to a raster map's units metadata file
 *
 * Map must exist in the current mapset.
 *
 * \param name
 * \param *str  string containing data to be written
 * \return  0 on success
 * \return -1, EOF (fclose() result) on error
 */
int G_write_raster_units(const char *name, const char *str)
{
    return G__raster_misc_write_line("units", name, str);
}


/*!
 * \brief Get a raster map's vertical datum metadata string
 *
 * Read the raster's vertical datum metadata file and put string in str
 *
 * \param name
 * \param mapset
 * \param *str  string to be populated with data
 * \return  0 on success
 * \return -1, EOF (fclose() result) on error
 */
int G_read_raster_vdatum(const char *name, const char *mapset, char *str)
{
    return G__raster_misc_read_line("vertical_datum", name, mapset, str);
}


/*!
 * \brief Write a string into a raster's vertical datum metadata file
 *
 * Map must exist in the current mapset.
 *
 * \param name
 * \param *str  string containing data to be written
 * \return  0 on success
 * \return -1, EOF (fclose() result) on error
 */
int G_write_raster_vdatum(const char *name, const char *str)
{
    return G__raster_misc_write_line("vertical_datum", name, str);
}



/*!
 * \brief Read the first line of a file in cell_misc/
 *
 * Read the first line of data from a cell_misc/ meta-data file.
 *
 * \param element  metadata component filename
 * \param name
 * \param mapset
 * \param *str  string to be populated with data
 * \return 0 on success
 * \return -1, EOF (fclose() result) on error
 */
int G__raster_misc_read_line(const char *elem, const char *name,
			     const char *mapset, char *str)
{
    FILE *fd;
    char buff[GNAME_MAX];

    buff[0] = '\0';

    if (G_find_file2_misc("cell_misc", elem, name, mapset) == NULL)
	return -1;

    fd = G_fopen_old_misc("cell_misc", elem, name, mapset);
    if (!fd) {
	G_warning(_("Can't read %s for [%s in %s]"), elem, name, mapset);
	return -1;
    }
    if (G_getl2(buff, sizeof(buff) - 1, fd) == 0) {
	/* file is empty */
	return fclose(fd);
    }

    strcpy(str, buff);

    return fclose(fd);
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
 * \return  0 on success
 * \return -1, EOF (fclose() result) on error
 */
int G__raster_misc_write_line(const char *elem, const char *name,
			      const char *str)
{
    FILE *fd;

    fd = G_fopen_new_misc("cell_misc", elem, name);
    if (fd == NULL) {
	G_warning(_("Can't create %s metadata file for [%s in %s]"),
		  elem, name, G_mapset());
	return -1;
    }

    fprintf(fd, "%s", str);

    return fclose(fd);
}
