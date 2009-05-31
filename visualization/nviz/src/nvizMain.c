
/****************************************************************************
 *
 * MODULE:       nviz
 * AUTHOR(S):    Bill Brown, Terry Baker, Mark Astley and David Gerdes
 *               (CERL and UIUC, original contributors)
 *               Bob Covill, Tekmap Consulting
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Huidae Cho <grass4u gmail.com>,
 *               Philip Warner <pjw rhyme.com.au>
 * PURPOSE:      main app for nviz visualization and animation tool
 * COPYRIGHT:    (C) 2002-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************//*
 * This is basically tkAppInit.c from the tk4.0 distribution except
 * that we define Tcl_AppInit in tkAppInit.c.
 */

#include <stdlib.h>
#include <string.h>
#include <tk.h>
#include <grass/gis.h>
#include "interface.h"

extern int NVIZ_AppInit(Tcl_Interp *);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      This is the main program for the application.
 *
 * Results:
 *      None: Tk_Main never returns here, so this procedure never
 *      returns either.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int main(int argc,		/* Number of command-line arguments. */
	 char **argv)		/* Values of command-line arguments. */
{
    Tcl_FindExecutable(argv[0]);

    if (argc > 1) {
	if (strstr(argv[argc - 1], "-h") != NULL)
	    sprintf(argv[argc - 1], "--h");
    }

    Tk_Main(argc, argv, NVIZ_AppInit);

    exit(EXIT_SUCCESS);
}
