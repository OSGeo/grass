
/*****************************************************************************
*
* MODULE:       DBF driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Simple driver for reading and writing dbf files     
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "globals.h"

/* save string to value */
int save_string(VALUE * val, char *c)
{
    int len;

    len = strlen(c) + 1;
    val->c = (char *)G_realloc(val->c, len);

    strcpy(val->c, c);

    return (1);
}
