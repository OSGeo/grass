#include <stdlib.h>
#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int init_vars (int argc, char *argv[])
{
	SHORT		r, c;
	CELL		*buf, alt_value, wat_value, asp_value;
	int		fd, index;
	char		MASK_flag;

G_gisinit (argv[0]);
ele_flag = wat_flag = asp_flag = pit_flag = run_flag = ril_flag = 0;
ob_flag = bas_flag = seg_flag = haf_flag = arm_flag = dis_flag = 0;
zero = sl_flag = sg_flag = ls_flag = er_flag =  bas_thres = 0;
nxt_avail_pt = 0;
/* dep_flag = 0; */
max_length = dzero = 0.0;
ril_value = -1.0;
/* dep_slope = 0.0; */
sides = 8;
for (r = 1; r < argc; r++) {
    if      (sscanf (argv[r], "el=%[^\n]", ele_name) == 1) ele_flag++;
    else if (sscanf (argv[r], "ac=%[^\n]", wat_name) == 1) wat_flag++;
    else if (sscanf (argv[r], "dr=%[^\n]", asp_name) == 1) asp_flag++;
    else if (sscanf (argv[r], "de=%[^\n]", pit_name) == 1) pit_flag++;
    else if (sscanf (argv[r], "t=%d",   &bas_thres) == 1) ;
    else if (sscanf (argv[r], "ms=%lf", &max_length) == 1) ;
    else if (sscanf (argv[r], "ba=%[^\n]", bas_name) == 1) bas_flag++;
    else if (sscanf (argv[r], "se=%[^\n]", seg_name) == 1) seg_flag++;
    else if (sscanf (argv[r], "ha=%[^\n]", haf_name) == 1) haf_flag++;
    else if (sscanf (argv[r], "ov=%[^\n]", run_name) == 1) run_flag++;
    else if (sscanf (argv[r], "ar=%[^\n]", arm_name) == 1) arm_flag++;
    else if (sscanf (argv[r], "di=%[^\n]", dis_name) == 1) dis_flag++;
    else if (sscanf (argv[r], "sl=%[^\n]", sl_name) == 1) sl_flag++;
    else if (sscanf (argv[r], "S=%[^\n]", sg_name) == 1) sg_flag++;
    else if (sscanf (argv[r], "LS=%[^\n]", ls_name) == 1) ls_flag++;
    else if (sscanf (argv[r], "ob=%[^\n]", ob_name) == 1) ob_flag++;
    else if (sscanf (argv[r], "r=%[^\n]", ril_name) == 1) {
	if (sscanf (ril_name, "%lf", &ril_value) == 0) {
		ril_value = -1.0;
		ril_flag++;
	}
    }
    /* else if (sscanf (argv[r], "sd=%[^\n]", dep_name) == 1) dep_flag++; */
    else if (sscanf (argv[r], "-%d", &sides) == 1) {
       	if (sides != 4) usage (argv[0]);
    }
    else usage (argv[0]);
}
if ((ele_flag != 1) 
    || 
    ((arm_flag == 1) && 
     ((bas_thres <= 0) || ((haf_flag != 1) && (bas_flag != 1)))) 
    || 
    ((bas_thres <= 0) && 
     ((bas_flag == 1) || (seg_flag == 1) || (haf_flag == 1) ||
      (sl_flag == 1) || (sg_flag == 1) || (ls_flag == 1)))
   )
	usage (argv[0]);
tot_parts = 4;
if (ls_flag || sg_flag) tot_parts++;
if (bas_thres > 0) tot_parts++;
G_message(_("SECTION 1a (of %1d): Initiating Memory."), tot_parts);
this_mapset = G_mapset ();
if (asp_flag)	do_legal (asp_name);
if (bas_flag)	do_legal (bas_name);
if (seg_flag)	do_legal (seg_name);
if (haf_flag)	do_legal (haf_name);
if (sl_flag)	do_legal (sl_name);
if (sg_flag)	do_legal (sg_name);
if (ls_flag)	do_legal (ls_name);
if (sl_flag || sg_flag || ls_flag) 
	er_flag = 1;
ele_mapset = do_exist (ele_name);
if (pit_flag) pit_mapset = do_exist (pit_name);
if (ob_flag) ob_mapset = do_exist (ob_name);
if (ril_flag) ril_mapset = do_exist (ril_name);
/* for sd factor
if (dep_flag)	{
	if (sscanf (dep_name, "%lf", &dep_slope) != 1)	{
		dep_mapset = do_exist (dep_name);
		dep_flag = -1;
	}
}
*/
G_get_set_window (&window);
nrows = G_window_rows ();
ncols = G_window_cols ();
total_cells = nrows * ncols;
if (max_length <= dzero)
	max_length = 10 * nrows * window.ns_res +
		10 * ncols * window.ew_res;
if (window.ew_res < window.ns_res) half_res = .5 * window.ew_res;
else half_res = .5 * window.ns_res;
diag = sqrt (window.ew_res * window.ew_res + 
	     window.ns_res * window.ns_res);
if (sides == 4)
	diag *= 0.5;
buf = G_allocate_cell_buf();
alt = (CELL *) G_malloc (sizeof(CELL) * size_array(&alt_seg, nrows, ncols));
r_h = (CELL *) G_malloc (sizeof(CELL) * size_array(&r_h_seg, nrows, ncols));

fd = G_open_cell_old (ele_name, ele_mapset);
if (fd < 0)	{
	G_fatal_error (_("unable to open elevation map layer"));
}

for (r = 0; r < nrows; r++)	{
	G_get_c_raster_row (fd, buf, r);
	for (c = 0; c < ncols; c++)	{
		index = SEG_INDEX(alt_seg, r, c);
		alt[index] = r_h[index] = buf[c];
	}
}
G_close_cell (fd);
wat = (CELL *) G_malloc (sizeof(CELL) * size_array(&wat_seg, nrows, ncols));

if (run_flag) {
	run_mapset = do_exist (run_name);
	fd = G_open_cell_old (run_name, run_mapset);
	if (fd < 0)	{
		G_fatal_error (_("unable to open runoff map layer"));
	}
	for (r = 0; r < nrows; r++)	{
		G_get_c_raster_row (fd, buf, r);
		for (c = 0; c < ncols; c++)	{
			wat[SEG_INDEX(wat_seg, r, c)] = buf[c];
		}
	}
	G_close_cell (fd);
} else {
	for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++)
			wat[SEG_INDEX(wat_seg,r,c)] = 1;
	}
}
asp = (CELL *) G_calloc (size_array(&asp_seg, nrows, ncols), sizeof(CELL));

if (pit_flag) {
	pit_mapset = do_exist (pit_name);
	fd = G_open_cell_old (pit_name, pit_mapset);
	if (fd < 0)	{
		G_fatal_error (_("unable to open depression map layer"));
	}
	for (r = 0; r < nrows; r++)	{
		G_get_c_raster_row (fd, buf, r);
		for (c = 0; c < ncols; c++)	{
			asp[SEG_INDEX(asp_seg, r, c)] = buf[c];
		}
	}
	G_close_cell (fd);
}
swale = flag_create (nrows, ncols);
if (ob_flag) {
	fd = G_open_cell_old (ob_name, ob_mapset);
	if (fd < 0)	{
		G_fatal_error (_("unable to open blocking map layer"));
	}
	for (r = 0; r < nrows; r++)	{
		G_get_c_raster_row (fd, buf, r);
		for (c = 0; c < ncols; c++)	{
			if (buf[c]) FLAG_SET(swale, r, c);
		}
	}
	G_close_cell (fd);
}
if (ril_flag)	{
	ril_fd = G_open_cell_old (ril_name, ril_mapset);
	if (ril_fd < 0)	{
		G_fatal_error (_("unable to open rill map layer"));
	}
}
in_list = flag_create (nrows, ncols);
worked = flag_create (nrows, ncols);
MASK_flag = 0;
do_points = nrows * ncols;
if (NULL != G_find_file ("cell", "MASK", G_mapset())) {
	MASK_flag = 1;
	if ((fd = G_open_cell_old ("MASK", G_mapset())) < 0) {
		G_fatal_error (_("unable to open MASK"));
	} else {
		for (r = 0; r < nrows; r++)	{
			G_get_c_raster_row_nomask (fd, buf, r);
			for (c = 0; c < ncols; c++)	{
				if (!buf[c]) {
					FLAG_SET(worked, r, c);
					FLAG_SET(in_list, r, c);
					do_points--;
				}
			}
		}
		G_close_cell (fd);
	}
}
s_l = (double *)G_malloc(size_array (&s_l_seg, nrows, ncols) * sizeof(double));
/* astar_pts = (POINT *) G_malloc (nrows * ncols * sizeof (POINT)); */
astar_pts = (POINT *) G_malloc (do_points * sizeof (POINT));

if (sg_flag)	{
	s_g = (double *) G_malloc (size_array (&s_g_seg, nrows, ncols) * sizeof(double));
}
if (ls_flag)	{
	l_s = (double *) G_malloc (size_array (&l_s_seg, nrows, ncols) * sizeof(double));
}

G_message(_("SECTION 1b (of %1d): Determining Offmap Flow."), tot_parts);

first_astar = first_cum = -1;
if (MASK_flag) {
	for (r = 0; r < nrows; r++) {
		G_percent (r, nrows, 3);
		for (c = 0; c < ncols; c++) {
			if (FLAG_GET (worked, r, c)) {
			    wat[SEG_INDEX(wat_seg,r,c)] = 0;
			} else {
			    s_l[SEG_INDEX(s_l_seg, r, c)] = half_res;
			    asp_value = asp[SEG_INDEX(asp_seg, r, c)];
			    if (r == 0 || c == 0 || r == nrows - 1 || 
				c == ncols - 1 || asp_value != 0)
			    {
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
				if (r == 0) asp_value = -2;
				else if (c == 0) asp_value = -4;
				else if (r == nrows - 1) asp_value = -6;
				else if (c == ncols - 1) asp_value = -8;
				else asp_value = -1;
				asp[SEG_INDEX(asp_seg,r,c)] = asp_value;
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
			    } else if (FLAG_GET(worked, r-1, c)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -2;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    } else if (FLAG_GET(worked, r+1, c)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -6;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    } else if (FLAG_GET(worked, r, c-1)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -4;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    } else if (FLAG_GET(worked, r, c+1)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -8;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    } else if (sides==8 && FLAG_GET(worked, r-1, c-1)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -3;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    } else if (sides==8 && FLAG_GET(worked, r-1, c+1)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -1;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    } else if (sides==8 && FLAG_GET(worked, r+1, c-1)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -5;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    } else if (sides==8 && FLAG_GET(worked, r+1, c+1)) {
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
				asp[SEG_INDEX(asp_seg,r,c)] = -7;
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0)
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
			    }
			}
		}
	}
} else {
	for (r = 0; r < nrows; r++) {
		G_percent (r, nrows, 3);
		for (c = 0; c < ncols; c++) {
			s_l[SEG_INDEX(s_l_seg, r, c)] = half_res;
			asp_value = asp[SEG_INDEX(asp_seg, r, c)];
			if (r == 0 || c == 0 || r == nrows - 1 || 
				c == ncols - 1 || asp_value != 0)
			{
				wat_value = wat[SEG_INDEX(wat_seg,r,c)];
				if (wat_value > 0) {
					wat[SEG_INDEX(wat_seg,r,c)] = -wat_value;
				}
				if (r == 0) asp_value = -2;
				else if (c == 0) asp_value = -4;
				else if (r == nrows - 1) asp_value = -6;
				else if (c == ncols - 1) asp_value = -8;
				else asp_value = -1;
				asp[SEG_INDEX(asp_seg,r,c)] = asp_value;
				alt_value = alt[SEG_INDEX(alt_seg,r,c)];
				add_pt (r, c, -1, -1, alt_value, alt_value);
			}
		}
	}
}

    G_percent (r, nrows, 3); /* finish it */

    return 0;
}

int 
do_legal (char *file_name)
{
	if (G_legal_filename (file_name) == -1)
		G_fatal_error(_("<%s> is an illegal file name"), file_name);

	return 0;
}

char *
do_exist (char *file_name)
{
	char *file_mapset = G_find_cell2 (file_name, "");

	if (file_mapset == NULL)
		G_fatal_error(_("Raster map <%s> not found"), file_name);

	return (file_mapset);
}
