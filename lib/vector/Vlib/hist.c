/*!
   \file lib/vector/Vlib/hist.c

   \brief Vector library - history manipulation

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/vector.h>

/*!
   \brief Write command info to history file

   \param Map pointer to Map_info structure

   \return 0 on success
   \return -1 error
 */
int Vect_hist_command(struct Map_info *Map)
{
    char *cmd, buf[2000];

    G_debug(3, "Vect_hist_command()");

    cmd = G_recreate_command();

    Vect_hist_write(Map, "COMMAND: ");
    Vect_hist_write(Map, cmd);
    Vect_hist_write(Map, "\n");

    sprintf(buf, "GISDBASE: %s\n", G_gisdbase());	/* Needed ? */
    Vect_hist_write(Map, buf);

    sprintf(buf, "LOCATION: %s MAPSET: %s USER: %s DATE: %s\n",
	    G_location(), G_mapset(), G_whoami(), G_date());	/* Needed ? */
    Vect_hist_write(Map, buf);

    return 0;
}

/*!
   \brief Write string to history file

   \param Map pointer to Map_info structure
   \param str string to write

   \return the number of characters printed
 */
int Vect_hist_write(struct Map_info *Map, const char *str)
{
    int ret;

    G_debug(5, "Vect_hist_write(): %s", str);
    ret = 0;
    if (Map->hist_fp) {
	ret = fprintf(Map->hist_fp, "%s", str);
	fflush(Map->hist_fp);
    }

    return ret;
}

/*!
   \brief Reads one line from history file without newline character

   \param[out] s buffer, allocated space must be size+1
   \param size maximum number of character
   \param Map vector map

   \return return s on success
   \return NULL on error
   \return EOF end of file
 */
char *Vect_hist_read(char *s, int size, const struct Map_info *Map)
{
    int ret;

    G_debug(5, "Vect_hist_read()");

    if (Map->hist_fp == NULL)
	return NULL;		/* OK for shapefile etc. */

    ret = G_getl2(s, size, Map->hist_fp);

    if (ret == 1)
	return s;

    return NULL;
}

/*!
   \brief Rewind history file

   \param Map vector map

   \return void
 */
void Vect_hist_rewind(struct Map_info *Map)
{
    G_debug(3, "Vect_hist_rewind()");

    if (Map->hist_fp != NULL)
	rewind(Map->hist_fp);
}

/*!
   \brief Copy history from one map to another

   \param In input vector map
   \param[out] Out output vector map

   \return 0 on success
   \return -1 on error
 */
int Vect_hist_copy(const struct Map_info *In, struct Map_info *Out)
{
    size_t red, ret;
    char buf[1000];

    G_debug(3, "Vect_hist_copy()");

    if (In->hist_fp == NULL)
	return 0;		/* This is correct (old hist doesn't exist) */
    if (Out->hist_fp == NULL)
	return -1;

    /* skip empty old hist */
    G_fseek(In->hist_fp, (long)0, SEEK_END);
    if (G_ftell(In->hist_fp) == 0)
	return 0;

    G_fseek(Out->hist_fp, (long)0, SEEK_END);
    rewind(In->hist_fp);

    while ((red = fread(buf, sizeof(char), sizeof(char) * 1000, In->hist_fp))) {
	if (!(ret = fwrite(buf, sizeof(char), red, Out->hist_fp))) {
	    return (-1);
	}
	fflush(Out->hist_fp);
    }

    /* In ends with \n ? */
    G_fseek(In->hist_fp, (long)-1, SEEK_END);
    if (fread(buf, sizeof(char), sizeof(char), In->hist_fp) != 1) {
	return -1;
    }

    if (buf[0] != '\n') {
	Vect_hist_write(Out, "\n");
    }

    /* Separator */
    Vect_hist_write(Out,
		    "---------------------------------------------------------------------------------\n");
    return (0);
}
