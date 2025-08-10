/*!
 * \file lib/gis/timestamp.c
 *
 * \brief GIS Library - Timestamp management
 *
 * Provides DateTime functions for timestamp management.
 *
 * The timestamp values must use the format as described in the GRASS
 * datetime library. The source tree for this library should have a
 * description of the format. For convenience, the formats as of Feb, 1996
 * are reproduced here:
 *
 * There are two types of datetime values: absolute and relative. Absolute
 * values specify exact dates and/or times. Relative values specify a span
 * of time. Some examples will help clarify:
 *
 * Absolute
 *
 * The general format for absolute values is:
 *
 * day month year [bc] hour:minute:seconds timezone
 *
 * day is 1-31
 * month is jan,feb,...,dec
 * year is 4 digit year
 * [bc] if present, indicates dates is BC
 * hour is 0-23 (24 hour clock)
 * minute is 0-59
 * second is 0-59.9999 (fractions of second allowed)
 * timezone is +hhmm or -hhmm (eg, -0600)
 *
 * parts can be missing
 *
 * 1994 [bc]
 * Jan 1994 [bc]
 * 15 jan 1000 [bc]
 * 15 jan 1994 [bc] 10 [+0000]
 * 15 jan 1994 [bc] 10:00 [+0100]
 * 15 jan 1994 [bc] 10:00:23.34 [-0500]
 *
 * Relative
 *
 * There are two types of relative datetime values, year- month and day-second.
 *
 * The formats are:
 *
 * [-] # years # months
 * [-] # days # hours # minutes # seconds
 *
 * The words years, months, days, hours, minutes, seconds are literal words,
 * and the # are the numeric values.
 *
 * Examples:
 *
 * 2 years
 * 5 months
 * 2 years 5 months
 * 100 days
 * 15 hours 25 minutes 35.34 seconds
 * 100 days 25 minutes
 * 1000 hours 35.34 seconds
 *
 * The following are illegal because it mixes year-month and day-second
 * (because the number of days in a month or in a year vary):
 *
 * 3 months 15 days
 * 3 years 10 days
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Michael Shapiro & Bill Brown, CERL
 * \author raster3d functions by Michael Pelizzari, LMCO
 * \author Soeren Gebbert, vector timestamp implementation update
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/vect/dig_defines.h>
#include <grass/glocale.h>

#define RAST_MISC "cell_misc"
#define GRID3     "grid3"

/*!
   \brief Initialize timestamp structure

   \param ts pointer to TimeStamp structure
 */
void G_init_timestamp(struct TimeStamp *ts)
{
    ts->count = 0;
}

/*!
   \brief Set timestamp (single)

   \param ts pointer to TimeStamp structure
   \param dt pointer to DateTime structure (date/time to be set)
 */
void G_set_timestamp(struct TimeStamp *ts, const DateTime *dt)
{
    datetime_copy(&ts->dt[0], dt);
    ts->count = 1;
}

/*!
   \brief Set timestamp (range)

   \param ts pointer to TimeStamp structure
   \param dt1,dt2 pointer to DateTime structures
 */
void G_set_timestamp_range(struct TimeStamp *ts, const DateTime *dt1,
                           const DateTime *dt2)
{
    datetime_copy(&ts->dt[0], dt1);
    datetime_copy(&ts->dt[1], dt2);
    ts->count = 2;
}

/*!
   \brief Read timestamp

   \param fd file descriptor
   \param[out] ts pointer to TimeStamp structure

   \return -2 EOF
   \return -1 on error
   \return 0 on success
 */
int G__read_timestamp(FILE *fd, struct TimeStamp *ts)
{
    char buf[1024];
    char comment[2];

    while (fgets(buf, sizeof(buf), fd)) {
        if (sscanf(buf, "%1s", comment) != 1 || *comment == '#')
            continue;
        return (G_scan_timestamp(ts, buf) > 0 ? 0 : -1);
    }
    return -2; /* nothing in the file */
}

/*!
   \brief Output TimeStamp structure to a file as a formatted string

   A handy fd might be "stdout".

   \param[in,out] fd file descriptor
   \param ts pointer to TimeStamp structure

   \return 0 on success
   \return -1 on error
 */
int G_write_timestamp(FILE *fd, const struct TimeStamp *ts)
{
    char buf[1024];

    if (G_format_timestamp(ts, buf) < 0)
        return -1;
    fprintf(fd, "%s\n", buf);
    return 0;
}

/*!
   \brief Create text string from TimeStamp structure

   Fills string *buf with info from TimeStamp structure *ts in a
   pretty way. The TimeStamp struct is defined in gis.h and populated
   with e.g. G_read_raster_timestamp().

   \param ts    TimeStamp structure containing time info
   \param buf   string to receive formatted timestamp

   \return 1 on success
   \return -1 error
 */
int G_format_timestamp(const struct TimeStamp *ts, char *buf)
{
    char temp1[128], temp2[128];

    *buf = 0;
    if (ts->count > 0) {
        if (datetime_format(&ts->dt[0], temp1) != 0)
            return -1;
    }
    if (ts->count > 1) {
        if (datetime_format(&ts->dt[1], temp2) != 0)
            return -1;
    }
    if (ts->count == 1)
        strcpy(buf, temp1);
    else if (ts->count == 2)
        sprintf(buf, "%s / %s", temp1, temp2);

    return 1;
}

/*!
   \brief Fill a TimeStamp structure from a datetime string

   Populate a TimeStamp structure (defined in gis.h) from a text
   string. Checks to make sure text string is in valid GRASS datetime
   format.

   \param ts   TimeStamp structure to be populated
   \param buf  string containing formatted time info

   \return 1 on success
   \return -1 error
 */
int G_scan_timestamp(struct TimeStamp *ts, const char *buf)
{
    char temp[1024], *t;
    const char *slash;
    DateTime dt1, dt2;

    G_init_timestamp(ts);
    for (slash = buf; *slash; slash++)
        if (*slash == '/')
            break;
    if (*slash) {
        t = temp;
        while (buf != slash)
            *t++ = *buf++;
        *t = 0;
        buf++;
        if (datetime_scan(&dt1, temp) != 0 || datetime_scan(&dt2, buf) != 0)
            return -1;
        G_set_timestamp_range(ts, &dt1, &dt2);
    }
    else {
        if (datetime_scan(&dt2, buf) != 0)
            return -1;
        G_set_timestamp(ts, &dt2);
    }
    return 1;
}

/*!
   \brief Copy TimeStamp into [two] Datetimes structs

   Use to copy the TimeStamp information into Datetimes, as the members
   of struct TimeStamp shouldn't be accessed directly.

   - count=0  means no datetimes were copied
   - count=1  means 1 datetime was copied into dt1
   - count=2  means 2 datetimes were copied

   \param ts     source TimeStamp structure
   \param[out] dt1    first DateTime struct to be filled
   \param[out] dt2    second DateTime struct to be filled
   \param[out] count  return code
 */
void G_get_timestamps(const struct TimeStamp *ts, DateTime *dt1, DateTime *dt2,
                      int *count)
{
    *count = 0;
    if (ts->count > 0) {
        datetime_copy(dt1, &ts->dt[0]);
        *count = 1;
    }
    if (ts->count > 1) {
        datetime_copy(dt2, &ts->dt[1]);
        *count = 2;
    }
}

/*!
   \brief Write timestamp file

   \param maptype map type
   \param dir directory
   \param name map name
   \param ts pointer to TimeStamp

   \return 1 on success
   \return -1 error - can't create timestamp file
   \return -2 error - invalid datetime in ts
 */
static int write_timestamp(const char *maptype, const char *dir,
                           const char *name, const struct TimeStamp *ts)
{
    FILE *fd;
    int stat;

    fd = G_fopen_new_misc(dir, "timestamp", name);
    if (fd == NULL) {
        G_warning(_("Unable to create timestamp file for %s map <%s@%s>"),
                  maptype, name, G_mapset());
        return -1;
    }

    stat = G_write_timestamp(fd, ts);
    fclose(fd);
    if (stat == 0)
        return 1;
    G_warning(_("Invalid timestamp specified for %s map <%s@%s>"), maptype,
              name, G_mapset());
    return -2;
}

/*!
   \brief Read timestamp file

   \param maptype map type
   \param dir directory
   \param name map name
   \param mapset mapset name
   \param ts pointer to TimeStamp

   \return 0 no timestamp file
   \return 1 on success
   \return -1 error - can't open timestamp file
   \return -2 error - invalid datetime values in timestamp file
 */
static int read_timestamp(const char *maptype, const char *dir,
                          const char *name, const char *mapset,
                          struct TimeStamp *ts)
{
    FILE *fd;
    int stat;

    if (!G_find_file2_misc(dir, "timestamp", name, mapset))
        return 0;
    fd = G_fopen_old_misc(dir, "timestamp", name, mapset);
    if (fd == NULL) {
        G_warning(_("Unable to open timestamp file for %s map <%s@%s>"),
                  maptype, name, mapset);
        return -1;
    }

    stat = G__read_timestamp(fd, ts);
    fclose(fd);
    if (stat == 0)
        return 1;
    G_warning(_("Invalid timestamp file for %s map <%s@%s>"), maptype, name,
              mapset);
    return -2;
}

/*!
   \brief Check if timestamp for raster map exists

   \param name map name
   \param mapset mapset name

   \return 1 on success
   \return 0 no timestamp present
 */
int G_has_raster_timestamp(const char *name, const char *mapset)
{
    if (!G_find_file2_misc(RAST_MISC, "timestamp", name, mapset))
        return 0;

    return 1;
}

/*!
   \brief Read timestamp from raster map

   \param name map name
   \param mapset mapset the map lives in
   \param[out] ts TimeStamp struct to populate

   \return 1 on success
   \return 0 or negative on error
 */
int G_read_raster_timestamp(const char *name, const char *mapset,
                            struct TimeStamp *ts)
{
    return read_timestamp("raster", RAST_MISC, name, mapset, ts);
}

/*!
   \brief Write timestamp of raster map

   \param name map name
   \param[out] ts TimeStamp struct to populate

   \return 1 on success
   \return -1 error - can't create timestamp file
   \return -2 error - invalid datetime in ts

 */
int G_write_raster_timestamp(const char *name, const struct TimeStamp *ts)
{
    return write_timestamp("raster", RAST_MISC, name, ts);
}

/*!
   \brief Remove timestamp from raster map

   Only timestamp files in current mapset can be removed.

   \param name map name

   \return 0 if no file
   \return 1 on success
   \return -1 on error
 */
int G_remove_raster_timestamp(const char *name)
{
    return G_remove_misc(RAST_MISC, "timestamp", name);
}

/*!
   \brief Check if timestamp for vector map exists

   \param name map name
   \param layer The layer names, in case of NULL, layer one is assumed
   \param mapset mapset name

   \return 1 on success
   \return 0 no timestamp present
 */
int G_has_vector_timestamp(const char *name, const char *layer,
                           const char *mapset)
{
    char dir[GPATH_MAX];
    char path[GPATH_MAX + GNAME_MAX];
    char ele[GNAME_MAX];

    if (layer != NULL)
        snprintf(ele, GNAME_MAX, "%s_%s", GV_TIMESTAMP_ELEMENT, layer);
    else
        snprintf(ele, GNAME_MAX, "%s_1", GV_TIMESTAMP_ELEMENT);

    snprintf(dir, GPATH_MAX, "%s/%s", GV_DIRECTORY, name);
    G_file_name(path, dir, ele, mapset);

    G_debug(1, "Check for timestamp <%s>", path);

    if (access(path, R_OK) != 0)
        return 0;

    return 1;
}

/*!
   \brief Read timestamp from vector map

   \param name map name
   \param layer The layer names, in case of NULL, layer one is assumed
   \param mapset mapset name
   \param[out] ts TimeStamp struct to populate

   \return 1 on success
   \return 0 no timestamp present
   \return -1 Unable to open file
   \return -2 invalid time stamp
 */
int G_read_vector_timestamp(const char *name, const char *layer,
                            const char *mapset, struct TimeStamp *ts)
{
    FILE *fd;
    int stat;
    char dir[GPATH_MAX];
    char ele[GNAME_MAX];

    /* In case no timestamp file is present return 0 */
    if (G_has_vector_timestamp(name, layer, mapset) != 1)
        return 0;

    if (layer != NULL)
        snprintf(ele, GNAME_MAX, "%s_%s", GV_TIMESTAMP_ELEMENT, layer);
    else
        snprintf(ele, GNAME_MAX, "%s_1", GV_TIMESTAMP_ELEMENT);

    snprintf(dir, GPATH_MAX, "%s/%s", GV_DIRECTORY, name);

    G_debug(1, "Read timestamp <%s/%s>", dir, ele);

    fd = G_fopen_old(dir, ele, mapset);

    if (fd == NULL) {
        G_warning(_("Unable to open timestamp file for vector map <%s@%s>"),
                  name, G_mapset());
        return -1;
    }

    stat = G__read_timestamp(fd, ts);
    fclose(fd);
    if (stat == 0)
        return 1;
    G_warning(_("Invalid timestamp file for vector map <%s@%s>"), name, mapset);
    return -2;
}

/*!
   \brief Write timestamp of vector map

   \param name map name
   \param layer The layer names, in case of NULL, layer one is assumed
   \param[out] ts TimeStamp struct to populate

   \return 1 on success
   \return -1 error - can't create timestamp file
   \return -2 error - invalid datetime in ts

 */
int G_write_vector_timestamp(const char *name, const char *layer,
                             const struct TimeStamp *ts)
{
    FILE *fd;
    int stat;
    char dir[GPATH_MAX];
    char ele[GNAME_MAX];

    if (layer != NULL)
        snprintf(ele, GNAME_MAX, "%s_%s", GV_TIMESTAMP_ELEMENT, layer);
    else
        snprintf(ele, GNAME_MAX, "%s_1", GV_TIMESTAMP_ELEMENT);

    snprintf(dir, GPATH_MAX, "%s/%s", GV_DIRECTORY, name);

    G_debug(1, "Write timestamp <%s/%s>", dir, ele);

    fd = G_fopen_new(dir, ele);

    if (fd == NULL) {
        G_warning(_("Unable to create timestamp file for vector map <%s@%s>"),
                  name, G_mapset());
        return -1;
    }

    stat = G_write_timestamp(fd, ts);
    fclose(fd);
    if (stat == 0)
        return 1;
    G_warning(_("Invalid timestamp specified for vector map <%s@%s>"), name,
              G_mapset());
    return -2;
}

/*!
   \brief Remove timestamp from vector map

   Only timestamp files in current mapset can be removed.

   \param name map name
   \param layer The layer names, in case of NULL, layer one is assumed

   \return 0 if no file
   \return 1 on success
   \return -1 on failure
 */
int G_remove_vector_timestamp(const char *name, const char *layer)
{
    char dir[GPATH_MAX];
    char ele[GNAME_MAX];

    if (layer)
        snprintf(ele, GNAME_MAX, "%s_%s", GV_TIMESTAMP_ELEMENT, layer);
    else
        snprintf(ele, GNAME_MAX, "%s_1", GV_TIMESTAMP_ELEMENT);

    snprintf(dir, GPATH_MAX, "%s/%s", GV_DIRECTORY, name);
    return G_remove(dir, ele);
}

/*!
   \brief Check if timestamp for 3D raster map exists

   \param name map name
   \param mapset mapset name

   \return 1 on success
   \return 0 no timestamp present
 */
int G_has_raster3d_timestamp(const char *name, const char *mapset)
{
    if (!G_find_file2_misc(GRID3, "timestamp", name, mapset))
        return 0;

    return 1;
}

/*!
   \brief Read timestamp from 3D raster map

   \param name map name
   \param mapset mapset name
   \param[out] ts TimeStamp struct to populate

   \return 1 on success
   \return 0 or negative on error
 */
int G_read_raster3d_timestamp(const char *name, const char *mapset,
                              struct TimeStamp *ts)
{
    return read_timestamp("raster3d", GRID3, name, mapset, ts);
}

/*!
   \brief Write timestamp of 3D raster map

   \param name map name
   \param[out] ts TimeStamp struct to populate

   \return 1 on success
   \return -1 error - can't create timestamp file
   \return -2 error - invalid datetime in ts

 */
int G_write_raster3d_timestamp(const char *name, const struct TimeStamp *ts)
{
    return write_timestamp("raster3d", GRID3, name, ts);
}

/*!
   \brief Remove timestamp from 3D raster map

   Only timestamp files in current mapset can be removed.

   \param name map name

   \return 0 if no file
   \return 1 on success
   \return -1 on failure
 */
int G_remove_raster3d_timestamp(const char *name)
{
    return G_remove_misc(GRID3, "timestamp", name);
}
