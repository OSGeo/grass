/* ***************************************************************
 * *
 * * MODULE:       v.digit
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               
 * * PURPOSE:      Edit vector
 * *              
 * * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "global.h"
#include "proto.h"

/* Add new command to background commands */
int bg_add(char *cmd)
{
    G_debug(2, "bg_add(): cmd = %s", cmd);
    if (nbgcmd == abgcmd) {
	abgcmd += 10;
	Bgcmd = (BGCMD *) G_realloc(Bgcmd, abgcmd * sizeof(BGCMD));
    }
    Bgcmd[nbgcmd].cmd = G_store(cmd);
    Bgcmd[nbgcmd].on = 1;
    nbgcmd++;

    return nbgcmd;
}
