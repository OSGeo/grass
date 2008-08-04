
/* InitProfile.c
 *
 * function defined:
 *
 * InitProfile(profile,window,north1,east1,north2,east2)
 *
 * struct Profile   *profile;   - pointer to profile structure 
 * struct Cell_head window;     - current window definition
 * double           north1,     - coords. of point #1
 *                  east1,
 *                  north2,     - coords. of point #2
 *                  east2;
 *
 * PURPOSE: To initialize a profile structure with profile-line
 * end-points and the current window definition.
 *
 * NOTES: 
 *
 * 1) The profile structure is defined in the file profile.h.
 *
 * 2)
 *
 * Dave Johnson
 * DBA Systems, Inc.
 * 10560 Arrowhead Drive
 * Fairfax, Virginia 22030
 *
 */

#include "profile.h"

int
InitProfile(struct Profile *profile, struct Cell_head window, double north1,
	    double east1, double north2, double east2)
{
    profile->window.north = window.north;
    profile->window.south = window.south;
    profile->window.west = window.west;
    profile->window.east = window.east;
    profile->window.ew_res = window.ew_res;
    profile->window.ns_res = window.ns_res;
    profile->n1 = north1;
    profile->e1 = east1;
    profile->n2 = north2;
    profile->e2 = east2;
    profile->count = 0;
    profile->ptr = NULL;

    return 0;
}
