#include <unistd.h>
#include <string.h>
#include <grass/imagery.h>
#include "globals.h"
#include "local_proto.h"

/* read the target for the group and cast it into the alternate GRASS env */

static int which_env;

int get_target(void)
{
    char location[40];
    char mapset[40];
    char buf[1024];
    int stat;
    char tl[100];
    char math_exp[100];
    char units[100];
    char nd[100];

    elev_layer = (char *)G_malloc(40 * sizeof(char));
    mapset_elev = (char *)G_malloc(40 * sizeof(char));

    /*fprintf (stderr,"Looking for target location\n"); */

    if (!I_get_target(group.name, location, mapset)) {
	sprintf(buf, "Target information missing for group [%s]\n",
		group.name);
	goto error;
    }

/***
fprintf (stderr,"target location: %s\n", location);
fprintf (stderr,"target mapset: %s\n", mapset);
fprintf (stderr,"Checking target access\n");
G_sleep (3);
****/
    sprintf(buf, "%s/%s", G_gisdbase(), location);
    if (access(buf, 0) != 0) {
	sprintf(buf, "Target location [%s] not found\n", location);
	goto error;
    }
    G__create_alt_env();
    G__setenv("LOCATION_NAME", location);
    stat = G__mapset_permissions(mapset);
    if (stat > 0) {
	G__setenv("MAPSET", mapset);
	G__create_alt_search_path();
	G__switch_env();
	G__switch_search_path();
	which_env = 0;

	/* get the block elevation layer raster map  in target location */

/***
fprintf (stderr,"Looking for elevation file in block: %s\n", block.name);
G_sleep (3);
***/
	/* Return the elev name from the group file ELEVATION */
	if (!I_get_group_elev(group.name, elev_layer, mapset_elev,
			      tl, math_exp, units, nd)) {
	    sprintf(buf, "Elevation information missing.for group [%s] \n",
		    group.name);
	    goto error;
	}

/**
fprintf (stderr,"Block elevation: %s in %s\n", elev_layer, mapset_elev);
G_sleep (3);
**/
	return 1;
    }
    sprintf(buf, "Mapset [%s] in target location [%s] - ", mapset, location);
    strcat(buf, stat == 0 ? "permission denied\n" : "not found\n");


  error:

/****
    strcat (buf, "Please run i.target for block ");
    strcat (buf, block.name);
****/
    G_fatal_error(buf);
}

int select_current_env(void)
{
    if (which_env != 0) {
	G__switch_env();
	G__switch_search_path();
	which_env = 0;
    }

    return 0;
}

int select_target_env(void)
{
    if (which_env != 1) {
	G__switch_env();
	G__switch_search_path();
	which_env = 1;
    }

    return 0;
}
