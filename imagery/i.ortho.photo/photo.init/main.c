/****************************************************************************
 *
 * MODULE:       photo.init
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      creates or modifies entries in a camera initial exposure
 *               station file for imagery group referenced by a sub-block
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define  MAIN  1
#include <stdlib.h>
#include <string.h>
#include "globals.h"


int main (int argc, char *argv[])
{
    int have_old;
    char *location, *mapset, *initial;
    char *name;

    if (argc != 2)
    {
	fprintf (stderr, "usage: %s group\n", argv[0]);
	exit(1);
    }

    initial   = (char *) G_malloc (40*sizeof (char));
    mapset    = (char *) G_malloc (40*sizeof (char));
    location  = (char *) G_malloc (40*sizeof (char));
    name      = (char *) G_malloc (40*sizeof (char));

    G_gisinit (argv[0]);
    location = G_location();
    mapset = G_mapset();
    
    
    /* get group ref */
    name = argv[1];
    strcpy (group.name, name);
    if (!I_find_group (group.name))
    {
	fprintf (stderr, "Group  [%s] not found\n", name);
	exit(1);
    }
#   ifdef DEBUG
    fprintf(stderr, "Found group %s\n", group.name);
#   endif

/*******************
    I_get_Ortho_Image_Group_Ref(group.name, &group.group_ref);
    nfiles = block.block_ref.nfiles;
#   ifdef DEBUG
    fprintf(stderr, "Got group ref \n");
#   endif
*******************/

    
    /* get initial camera exposure infor */ 
    if (I_find_initial(group.name))
    {  have_old = 1;
        I_get_init_info (group.name, &group.camera_exp);
    } 

    /* modifiy infor */
    mod_init_info(have_old, &group.camera_exp);

    /* save info */
    I_put_init_info(group.name, &group.camera_exp);

    exit(0);
}





