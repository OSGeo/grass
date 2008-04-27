/****************************************************************************
 *
 * MODULE:       r.watershed/seg - uses the GRASS segmentation library
 * AUTHOR(S):    Charles Ehlschlaeger, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>,
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>
 * PURPOSE:      Watershed determination using the GRASS segmentation lib
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define MAIN
#include <stdlib.h>
#include <unistd.h>
#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>
#undef MAIN


int 
main (int argc, char *argv[])
{
    extern FILE *fopen();

    one = 1;
    zero = 0;
    dzero = 0.0;
    init_vars (argc,argv);
    do_astar ();
    do_cum ();
    if (sg_flag || ls_flag)
    {
	sg_factor();
    }

    if (bas_thres <= 0)
    {
    	G_message(_("SECTION %d beginning: Closing Maps."), tot_parts);
        close_maps ();
    }
    else    
    {
        if (arm_flag)
	{
	    fp = fopen (arm_name, "w");
	}
	cseg_open (&bas, SROW, SCOL, 4);
	cseg_open (&haf, SROW, SCOL, 4);
    	G_message(_("SECTION %d beginning: Watershed determination."), 
			tot_parts - 1);
	find_pourpts ();
    	G_message(_("SECTION %d beginning: Closing Maps."), tot_parts);
	close_array_seg ();
    }

    exit (EXIT_SUCCESS);
}
