/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001, 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdio.h>

#include <grass/vector.h>
#include <grass/glocale.h>

/*!
  \brief Read external vector format file
 
  \param dascii format file (frmt)
  \param[out] finfo pointer to Format_info structure

  \return format code
  \return -1 on error
*/
int dig_read_frmt_ascii(FILE * dascii, struct Format_info *finfo)
{
    char buff[20001], buf1[1024];
    char *ptr;
    int frmt = -1;

    G_debug(3, "dig_read_frmt_ascii()");

    /* read first line which must be FORMAT: */
    if (G_getl2(buff, 2000, dascii)) {
        G_chop(buff);

        if (!(ptr = strchr(buff, ':'))) {
            G_warning(_("Vector format not recognized: %s"), buff);
            return -1;
        }

        strcpy(buf1, buff);
        buf1[ptr - buff] = '\0';

        ptr++;                  /* Search for the start of text */
        while (*ptr == ' ')
            ptr++;

        if (G_strcasecmp(buf1, "FORMAT") == 0) {
#ifdef HAVE_OGR
            if (G_strcasecmp(ptr, "ogr") == 0) {
                frmt = GV_FORMAT_OGR;
            }
#endif
#ifdef HAVE_POSTGRES
            if (G_strcasecmp(ptr, "postgis") == 0) {
                frmt = GV_FORMAT_POSTGIS;
            }
#endif
        }
    }
    if (frmt == -1) {
        G_warning(_("Vector format not recognized: %s"), buff);
        return -1;
    }

    /* init format info values */
#ifdef HAVE_OGR
    G_zero(&(finfo->ogr), sizeof(struct Format_info_ogr));
#else
    if (frmt == GV_FORMAT_OGR) {
        G_warning(_("Vector format '%s' not supported"), ptr);
        return -1;
    }
#endif

#ifdef HAVE_POSTGRES
    G_zero(&(finfo->pg), sizeof(struct Format_info_pg));
#else
    if (frmt == GV_FORMAT_POSTGIS) {
        G_warning(_("Vector format '%s' not supported"), ptr);
        return -1;
    }
#endif

    while (G_getl2(buff, 2000, dascii)) {
        G_chop(buff);

        if (!(ptr = strchr(buff, ':'))) {
            G_warning(_("Format definition is not correct: %s"), buff);
            continue;
        }

        strcpy(buf1, buff);
        buf1[ptr - buff] = '\0';

        ptr++;                  /* Search for the start of text */
        while (*ptr == ' ')
            ptr++;

#ifdef HAVE_OGR
        if (frmt == GV_FORMAT_OGR) {
            if (G_strcasecmp(buf1, "DSN") == 0)
                finfo->ogr.dsn = G_store(ptr);
            if (G_strcasecmp(buf1, "LAYER") == 0)
                finfo->ogr.layer_name = G_store(ptr);
        }
#endif
#ifdef HAVE_POSTGRES
        if (frmt == GV_FORMAT_POSTGIS) {
            if (G_strcasecmp(buf1, "CONNINFO") == 0)
                finfo->pg.conninfo = G_store(ptr);
            if (G_strcasecmp(buf1, "SCHEMA") == 0)
                finfo->pg.schema_name = G_store(ptr);
            if (G_strcasecmp(buf1, "TABLE") == 0)
                finfo->pg.table_name = G_store(ptr);
            if (G_strcasecmp(buf1, "FID") == 0)
                finfo->pg.fid_column = G_store(ptr);
        }
#endif
    }

#ifdef HAVE_POSTGRES
    /* if schema not defined, use 'public' */
    if (frmt == GV_FORMAT_POSTGIS &&
        !finfo->pg.schema_name) {
        finfo->pg.schema_name = G_store("public");
    }

    /* if fid column not defined, use default value */
    if (frmt == GV_FORMAT_POSTGIS &&
        !finfo->pg.fid_column) {
        finfo->pg.fid_column = G_store(GV_PG_FID_COLUMN);
    }
#endif

    return frmt;
}

/* Write vector format, currently does not work
 *  Parse also connection string.
 *
 *  Returns: 0 OK
 *           -1 on error
 */
int dig_write_frmt_ascii(FILE * dascii, struct Format_info *finfo, int format)
{
    G_debug(3, "dig_write_frmt_ascii()");

    G_fatal_error("Format not supported by dig_write_frmt_ascii()");

    return 0;
}
