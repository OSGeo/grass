/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *               Tomas Zigo <tomas zigo slovanet sk> (adding the option
 *               to read width, depth values from vector map table columns)
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "enforce.h"


/*
 * update_rast_history - Update a history file.  Some of the digit file
 * information is placed in the history file.
 */
void update_rast_history(struct parms *parm)
{
    struct History hist;

    /* write command line to history */
    Rast_short_history(parm->outrast->answer, "raster", &hist);
    Rast_append_format_history(&hist, "%s version %.2f", G_program_name(), APP_VERSION);
    Rast_append_format_history(&hist, "stream width: %.2f", parm->swidth * 2);
    Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file: %s", parm->inrast->answer);
    Rast_format_history(&hist, HIST_DATSRC_2, "vector stream file: %s", parm->invect->answer);
    Rast_command_history(&hist);
    Rast_write_history(parm->outrast->answer, &hist);
}


void check_mem_alloc(struct ptr *pointer)
/*
 * Function: check_mem_alloc
 * -------------------------
 * Check memory allocation
 *
 * pointer: ptr struct
 *          ptr.type: pointer type
 *          ptr.int|double|char|dbString: pointer
 *          ptr.p_vect_id_cat_map: line id with matching cat (
 *          vect_id_cat_map struct)
 *
 */
{
    switch (pointer->type)
    {
       case P_INT:
           if (!pointer->p_int)
           {
               G_free(pointer->p_int);
               pointer->p_int = NULL;
               G_fatal_error(_("Fail to allocate memory"));
           }
           break;
       case P_DOUBLE:
           if (!pointer->p_double)
           {
               G_free(pointer->p_double);
               pointer->p_double = NULL;
               G_fatal_error(_("Fail to allocate memory"));
           }
           break;
       case P_CHAR:
           if (!pointer->p_char)
           {
               G_free(pointer->p_char);
               pointer->p_char = NULL;
               G_fatal_error(_("Fail to allocate memory"));
           }
           break;
       case P_DBSTRING:
           if (!pointer->p_dbString)
           {
               db_free_string(pointer->p_dbString);
               pointer->p_dbString = NULL;
               G_fatal_error(_("Fail to allocate memory"));
           }
           break;
       case P_VECT_ID_CAT_MAP:
           if (!pointer->p_vect_id_cat_map)
           {
               G_free(pointer->p_vect_id_cat_map);
               pointer->p_vect_id_cat_map = NULL;
               G_fatal_error(_("Fail to allocate memory"));
           }
           break;
     }
}
