
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
#include <unistd.h>
#include "coin.h"

struct Cell_head window;

char *title1, *title2;

double window_cells;
double window_area;

struct stats_table *table;
long *catlist1, *catlist2;
int no_data1, no_data2;
int Rndex, Cndex;
char *dumpname;
char *statname;
FILE *dumpfile;

char map1name[GNAME_MAX], map2name[GNAME_MAX];
char *mapset1, *mapset2;
int ncat1, ncat2;

char *fill, *midline;

int main(int argc, char *argv[])
{
    fill =
	"                                                                                                                                       ";
    midline =
	"------------------------------------------------------------------------------------------------------------------------------------";

    G_gisinit(argv[0]);
    G_get_window(&window);
    /* now make a temorary region with the same boundaries only 1 x 1 */
    window.rows = 1;
    window.cols = 1;
    G_adjust_Cell_head(&window, 1, 1);
    G_set_window(&window);

    G_begin_cell_area_calculations();
    window_area = G_area_of_cell_at_row(0);

    /* restore region back to the original */
    G_get_window(&window);
    G_set_window(&window);

    dumpname = G_tempfile();
    statname = G_tempfile();

    window_cells = G_window_rows() * G_window_cols();


    command_version(argc, argv);

    unlink(dumpname);
    unlink(statname);

    exit(0);
}
