
/****************************************************************************
 *
 * MODULE:       r.watershed/seg - uses the GRASS segmentation library
 * AUTHOR(S):    Charles Ehlschlaeger, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>,
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Markus Metz <markus.metz.giswork gmail.com>
 * PURPOSE:      Hydrological analysis using the GRASS segmentation lib
 * COPYRIGHT:    (C) 1999-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>

struct Cell_head window;

int mfd, c_fac, abs_acc, ele_scale;
SSEG heap_index;
int heap_size;
int first_astar, first_cum, nxt_avail_pt, total_cells, do_points;
SHORT nrows, ncols;
double half_res, diag, max_length, dep_slope;
int bas_thres, tot_parts;
SSEG astar_pts;
BSEG worked, in_list, s_b, swale;
CSEG dis, alt, asp, bas, haf, r_h, dep;
DSEG wat;
DSEG slp, s_l, s_g, l_s, ril;
CELL one, zero;
double ril_value, d_zero, d_one;
SHORT sides;
SHORT drain[3][3] = { {7, 6, 5}, {8, 0, 4}, {1, 2, 3} };
SHORT updrain[3][3] = { {3, 2, 1}, {4, 0, 8}, {5, 6, 7} };
SHORT nextdr[8] = { 1, -1, 0, 0, -1, 1, 1, -1 };
SHORT nextdc[8] = { 0, 0, -1, 1, 1, -1, 1, -1 };
char ele_name[GNAME_MAX], pit_name[GNAME_MAX];
char run_name[GNAME_MAX], ob_name[GNAME_MAX];
char ril_name[GNAME_MAX], dep_name[GNAME_MAX];
const char *this_mapset;
char seg_name[GNAME_MAX], bas_name[GNAME_MAX], haf_name[GNAME_MAX],
    thr_name[8];
char ls_name[GNAME_MAX], st_name[GNAME_MAX], sl_name[GNAME_MAX],
    sg_name[GNAME_MAX];
char wat_name[GNAME_MAX], asp_name[GNAME_MAX], arm_name[GNAME_MAX],
    dis_name[GNAME_MAX];
char ele_flag, pit_flag, run_flag, dis_flag, ob_flag;
char wat_flag, asp_flag, arm_flag, ril_flag, dep_flag;
char bas_flag, seg_flag, haf_flag, er_flag;
char st_flag, sb_flag, sg_flag, sl_flag, ls_flag;
FILE *fp;



int main(int argc, char *argv[])
{
    one = 1;
    zero = 0;
    d_zero = 0.0;
    d_one = 1.0;
    init_vars(argc, argv);
    do_astar();
    if (mfd) {
	do_cum_mfd();
    }
    else {
	do_cum();
    }
    if (sg_flag || ls_flag) {
	sg_factor();
    }

    if (bas_thres <= 0) {
	G_message(_("SECTION %d: Closing Maps."), tot_parts);
	close_maps();
    }
    else {
	if (arm_flag) {
	    fp = fopen(arm_name, "w");
	}
	cseg_open(&bas, SROW, SCOL, 4);
	cseg_open(&haf, SROW, SCOL, 4);
	G_message(_("SECTION %d: Watershed determination."), tot_parts - 1);
	find_pourpts();
	G_message(_("SECTION %d: Closing Maps."), tot_parts);
	close_array_seg();
    }

    exit(EXIT_SUCCESS);
}
