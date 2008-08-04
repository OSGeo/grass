
/****************************************************************************
 *
 * MODULE:       edit library functions
 * AUTHOR(S):    Originally part of gis lib dir in CERL GRASS code
 *               Subsequent (post-CVS) contributors:
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Eric G. Miller <egm2 jps.net>,
 *               Markus Neteler <neteler itc.it>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Bernhard Reiter <bernhard intevation.de>
 * PURPOSE:      libraries for interactively editing raster support data
 * COPYRIGHT:    (C) 1996-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
 **********************************************************************
 *
 *   E_edit_history (hist)
 *      struct History *hist 
 *
 *   Interactively prompts the user for history information.
 *   Uses screen oriented prompting through the visual_ask library.
 *   Programs using this must be compiled with the GRASS library $(VASKLIB)
 *   and include $(CURSES) in the compile line
 *
 *   Returns: 1 ok
 *           -1 user canceled edit
 *
 **********************************************************************/

#include <grass/gis.h>
#include <grass/vask.h>
#include <grass/edit.h>

int E_edit_history(struct History *phist)
{
    int len;
    int i;

    if (!*phist->mapid)
	sprintf(phist->mapid, "%s : %s", G_date(), "filename");
    if (!*phist->title)
	sprintf(phist->title, "%s", "filename");
    if (!*phist->mapset)
	sprintf(phist->mapset, "%s", G_mapset());
    if (!*phist->creator)
	sprintf(phist->creator, "%s", G_whoami());
    if (!*phist->maptype)
	sprintf(phist->maptype, "raster");

    V_clear();
    V_line(0, "		 ENTER/CORRECT FILE HISTORY INFORMATION");
    V_line(2, "Map ID ...");
    V_line(4, "Title ....");
    V_line(6, "Project ..");
    V_line(8, "Creator ..");
    V_line(10, "Maptype ..");
    V_line(12, "Data source");
    V_line(16, "Data Description");
    V_line(19, "For history comments see next page");

    len = RECORD_LEN - 1;
    if (len > 65)
	len = 65;
    phist->mapid[len] = 0;
    phist->title[len] = 0;
    phist->mapset[len] = 0;
    phist->creator[len] = 0;
    phist->maptype[len] = 0;
    len--;

    V_const(phist->mapid, 's', 2, 11, len);
    V_ques(phist->title, 's', 4, 11, len);
    V_const(phist->mapset, 's', 6, 11, len);
    V_const(phist->creator, 's', 8, 11, len);
    V_ques(phist->maptype, 's', 10, 11, len);

    phist->datsrc_1[len] = 0;
    phist->datsrc_2[len] = 0;
    phist->keywrd[len] = 0;
    len--;

    V_ques(phist->datsrc_1, 's', 13, 0, len);
    V_ques(phist->datsrc_2, 's', 14, 0, len);
    V_ques(phist->keywrd, 's', 17, 0, len);

    V_intrpt_ok();
    if (!V_call())
	return -1;

    G_strip(phist->title);
    G_strip(phist->maptype);
    G_strip(phist->datsrc_1);
    G_strip(phist->datsrc_2);
    G_strip(phist->keywrd);

    V_clear();
    V_line(0, "		 ENTER/CORRECT FILE HISTORY COMMENTS");
    V_ques(phist->edhist[0], 's', 2, 0, len);
    V_ques(phist->edhist[1], 's', 3, 0, len);
    V_ques(phist->edhist[2], 's', 4, 0, len);
    V_ques(phist->edhist[3], 's', 5, 0, len);
    V_ques(phist->edhist[4], 's', 6, 0, len);
    V_ques(phist->edhist[5], 's', 7, 0, len);
    V_ques(phist->edhist[6], 's', 8, 0, len);
    V_ques(phist->edhist[7], 's', 9, 0, len);
    V_ques(phist->edhist[8], 's', 10, 0, len);
    V_ques(phist->edhist[9], 's', 11, 0, len);
    V_ques(phist->edhist[10], 's', 12, 0, len);
    V_ques(phist->edhist[11], 's', 13, 0, len);
    V_ques(phist->edhist[12], 's', 14, 0, len);
    V_ques(phist->edhist[13], 's', 15, 0, len);
    V_ques(phist->edhist[14], 's', 16, 0, len);
    V_ques(phist->edhist[15], 's', 17, 0, len);
    V_ques(phist->edhist[16], 's', 18, 0, len);
    V_ques(phist->edhist[17], 's', 19, 0, len);
    V_ques(phist->edhist[18], 's', 20, 0, len);
    V_ques(phist->edhist[19], 's', 21, 0, 65);

    len++;
    for (i = 0; i <= 19; i++)
	phist->edhist[i][len] = 0;

    V_intrpt_ok();
    if (!V_call())
	return -1;

    for (i = 0; i <= 19; i++)
	G_strip(phist->edhist[i]);

    for (phist->edlinecnt = 19; phist->edlinecnt > 0; phist->edlinecnt--) {
	if (*phist->edhist[phist->edlinecnt] != 0)
	    break;
    }
    phist->edlinecnt++;
    V_clear();

    return (1);
}
