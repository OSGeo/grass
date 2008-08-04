
/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include "coin.h"


/* computes the approximate number of lines/pages an 80 column
 * report would take, and asks the user if this is ok
 */
int check_report_size(void)
{
    long nlines;
    long npages;
    long npanels;
    char buf[100];


    npanels = ncat1 / 3;
    if (ncat1 % 3)
	npanels++;

    nlines = (12 + ncat2) * npanels + 11 + ncat2;
    npages = (nlines + 65) / 66;

    while (1) {
	fprintf(stdout, "\nThe coincidence table is %d rows by %d columns\n",
		ncat2, ncat1);
	fprintf(stdout,
		"The report will require approximately %ld lines (%ld pages)\n",
		nlines, npages);
	fprintf(stdout, "Do you want to continue? ");
	while (1) {
	    fprintf(stdout, "(y/n) ");
	    if (!G_gets(buf))
		break;
	    if (*buf == 'y' || *buf == 'Y')
		return 1;
	    if (*buf == 'n' || *buf == 'N')
		exit(0);
	}
    }
}
