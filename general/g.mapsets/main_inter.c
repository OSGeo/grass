/****************************************************************************
 *
 * MODULE:       g.mapsets
 * AUTHOR(S):    Michael Shapiro (CERL), Greg Koerper (ManTech Environmental
 *                    Technology) (original contributors), 
 *               Markus Neteler <neteler itc.it>
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      Allow user to select new mapset path
 * COPYRIGHT:    (C) 1994-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define MAIN
#include "externs.h"
#include "local_proto.h"
#include <grass/gis.h>
int 
main (int argc, char **argv)
{
    G_gisinit (argv[0]);
    G_clear_screen () ;
    fprintf (stdout,"MAPSET SEARCH LIST UTILITY");
    fprintf (stdout,"     LOCATION: %s    MAPSET: %s\n\n", G_location(), G_mapset());

    get_available_mapsets();

    do
    {
        display_mapset_path(1);
        display_available_mapsets(1);
    }
    while (get_mapset_path() < 0);

    set_mapset_path();

    return 0;
}



