/*
 *
 * provides DateTime functions for timestamp management:
 *
 * Authors: Michael Shapiro & Bill Brown, CERL
 *          grid3 functions by Michael Pelizzari, LMCO
 *
 * G_init_timestamp()
 * G_set_timestamp()
 * G_set_timestamp_range()
 * G_format_timestamp()
 * G_scan_timestamp()
 * G_get_timestamps()
 * G_read_raster_timestamp()
 * G_remove_raster_timestamp()
 * G_read_vector_timestamp()
 * G_remove_vector_timestamp()
 * G_read_grid3_timestamp()
 * G_remove_grid3_timestamp()
 * G_write_raster_timestamp()
 * G_write_vector_timestamp()
 * G_write_grid3_timestamp()
 *
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *
 * The timestamp values must use the format as described in the GRASS
 * datetime library. The source tree for this library should have a
 * description of the format. For convience, the formats as of Feb, 1996
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
 * mintue is 0-59
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
 *
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

void G_init_timestamp(struct TimeStamp *ts)
{
    ts->count = 0;
}

void G_set_timestamp(struct TimeStamp *ts, const DateTime * dt)
{
    datetime_copy(&ts->dt[0], dt);
    ts->count = 1;
}

void G_set_timestamp_range(struct TimeStamp *ts,
			   const DateTime * dt1, const DateTime * dt2)
{
    datetime_copy(&ts->dt[0], dt1);
    datetime_copy(&ts->dt[1], dt2);
    ts->count = 2;
}

int G__read_timestamp(FILE * fd, struct TimeStamp *ts)
{
    char buf[1024];
    char comment[2];

    while (fgets(buf, sizeof(buf), fd)) {
	if (sscanf(buf, "%1s", comment) != 1 || *comment == '#')
	    continue;
	return (G_scan_timestamp(ts, buf) > 0 ? 0 : -1);
    }
    return -2;			/* nothing in the file */
}


/*!
 * \brief output TimeStamp structure to a file as a formatted string
 *
 * A handy fd might be "stdout".
 *
 * Returns:
 *  0 on success
 * -1 error
 *
 *  \param fd    file descriptor
 *  \param ts    TimeStamp struct
 *  \return int  exit value
 */
int G__write_timestamp(FILE * fd, const struct TimeStamp *ts)
{
    char buf[1024];

    if (G_format_timestamp(ts, buf) < 0)
	return -1;
    fprintf(fd, "%s\n", buf);
    return 0;
}


/*!
 * \brief Create text string from TimeStamp structure
 *
 * Fills string *buf with info from TimeStamp structure *ts in a pretty
 * way. The TimeStamp struct is defined in gis.h and populated with e.g.
 * G_read_raster_timestamp().
 *
 * Returns:
 *  1 on success
 * -1 error
 *
 *  \param ts    TimeStamp structure containing time info
 *  \param buf   string to receive formatted timestamp
 *  \return int  exit value
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
 * \brief Fill a TimeStamp structure from a datetime string
 *
 * Populate a TimeStamp structure (defined in gis.h) from a text string.
 * Checks to make sure text string is in valid GRASS datetime format.
 *
 * Returns:
 * 1 on success
 * -1 error
 *
 *  \param ts   TimeStamp structure to be populated
 *  \param buf  String containing formatted time info
 *  \return int exit code
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
 * \brief copy TimeStamp into [two] Datetimes structs
 *
 * Use to copy the TimeStamp information into Datetimes, as the members of
 * struct TimeStamp shouldn't be accessed directly.
 *
 * count=0  means no datetimes were copied
 * count=1  means 1 datetime was copied into dt1
 * count=2  means 2 datetimes were copied
 *
 *  \param ts     source TimeStamp structure
 *  \param dt1    first DateTime struct to be filled
 *  \param dt2    second DateTime struct to be filled
 *  \param count  return code
 *  \return int   always 0
 */
int G_get_timestamps(const struct TimeStamp *ts,
		     DateTime * dt1, DateTime * dt2, int *count)
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

    return 0;
}


/* write timestamp file
 * 1 ok
 * -1 error - can't create timestamp file
 * -2 error - invalid datetime in ts
 */
static int write_timestamp(const char *maptype, const char *dir,
			   const char *name, const struct TimeStamp *ts)
{
    FILE *fd;
    int stat;

    fd = G_fopen_new_misc(dir, "timestamp", name);
    if (fd == NULL) {
	G_warning(_("Can't create timestamp file for %s map %s in mapset %s"),
		  maptype, name, G_mapset());
	return -1;
    }

    stat = G__write_timestamp(fd, ts);
    fclose(fd);
    if (stat == 0)
	return 1;
    G_warning(_("Invalid timestamp specified for %s map %s in mapset %s"),
	      maptype, name, G_mapset());
    return -2;
}

/* read timestamp file
 * 0 no timestamp file
 * 1 ok
 * -1 error - can't open timestamp file
 * -2 error - invalid datetime values in timestamp file
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
	G_warning(_("Can't open timestamp file for %s map %s in mapset %s"),
		  maptype, name, mapset);
	return -1;
    }

    stat = G__read_timestamp(fd, ts);
    fclose(fd);
    if (stat == 0)
	return 1;
    G_warning(_("Invalid timestamp file for %s map %s in mapset %s"),
	      maptype, name, mapset);
    return -2;
}

#define RAST_MISC "cell_misc"
#define VECT_MISC "dig_misc"
#define GRID3	  "grid3"


/*!
 * \brief Read timestamp from raster map
 *
 * Returns:
 * 1 on success
 * 0 or negative on error.
 *
 *  \param name map name
 *  \param mapset mapset the map lives in
 *  \param ts TimeStamp struct to populate
 *  \return int
 */
int G_read_raster_timestamp(const char *name, const char *mapset,
			    struct TimeStamp *ts)
{
    return read_timestamp("raster", RAST_MISC, name, mapset, ts);
}


/*!
 * \brief 
 *
 * Only timestamp files in current mapset can be removed
 * Returns:
 * 0  if no file
 * 1  if successful
 * -1  on fail
 *
 *  \param name
 *  \return int
 */
int G_remove_raster_timestamp(const char *name)
{
    return G_remove_misc(RAST_MISC, "timestamp", name);
}



/*!
 * \brief Read vector timestamp
 *
 * Is this used anymore with the new GRASS 6 vector engine???
 *
 * Returns 1 on success.  0 or negative on error.
 *
 *  \param name
 *  \param mapset
 *  \param ts
 *  \return int
 */
int G_read_vector_timestamp(const char *name, const char *mapset,
			    struct TimeStamp *ts)
{
    return read_timestamp("vector", VECT_MISC, name, mapset, ts);
}



/*!
 * \brief 
 *
 * Is this used anymore with the new GRASS 6 vector engine???
 *
 * Only timestamp files in current mapset can be removed
 * Returns:
 * 0  if no file
 * 1  if successful
 * -1  on fail
 *
 *  \param name
 *  \return int
 */
int G_remove_vector_timestamp(const char *name)
{
    return G_remove_misc(VECT_MISC, "timestamp", name);
}


/*!
 * \brief read grid3 timestamp
 *
 * Returns 1 on success. 0 or
 * negative on error.
 *
 *  \param name
 *  \param mapset
 *  \param ts
 *  \return int
 */
int G_read_grid3_timestamp(const char *name, const char *mapset,
			   struct TimeStamp *ts)
{
    return read_timestamp("grid3", GRID3, name, mapset, ts);
}


/*!
 * \brief remove grid3 timestamp
 *
 * Only timestamp files in current mapset can be removed
 * Returns:
 * 0  if no file
 * 1  if successful
 * -1  on fail
 *
 *  \param name
 *  \return int
 */
int G_remove_grid3_timestamp(const char *name)
{
    return G_remove_misc(GRID3, "timestamp", name);
}



/*!
 * \brief 
 *
 * Returns:
 * 1 on success.
 * -1 error - can't create timestamp file
 * -2 error - invalid datetime in ts
 *
 *  \param name
 *  \param ts
 *  \return int
 */
int G_write_raster_timestamp(const char *name, const struct TimeStamp *ts)
{
    return write_timestamp("raster", RAST_MISC, name, ts);
}



/*!
 * \brief 
 *
 * Returns:
 * 1 on success.
 * -1 error - can't create timestamp file
 * -2 error - invalid datetime in ts
 *
 *  \param name
 *  \param ts
 *  \return int
 */
int G_write_vector_timestamp(const char *name, const struct TimeStamp *ts)
{
    return write_timestamp("vector", VECT_MISC, name, ts);
}


/*!
 * \brief write grid3 timestamp
 *
 * Returns:
 * 1 on success.
 * -1 error - can't create timestamp file
 * -2 error - invalid datetime in ts
 *
 *  \param name
 *  \param ts
 *  \return int
 */
int G_write_grid3_timestamp(const char *name, const struct TimeStamp *ts)
{
    return write_timestamp("grid3", GRID3, name, ts);
}
