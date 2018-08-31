/****************************************************************************
 *
 * MODULE:       r.path
 *               
 * AUTHOR(S):    based on r.drain
 *               Markus Metz
 *               
 * PURPOSE:      Tracing paths from starting points following 
 *               input directions.  
 * COPYRIGHT:    (C) 2017 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <grass/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* for using the "open" statement */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for using the close statement */
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local.h"
#include "pavlrc.h"

/* should probably be updated to a pointer array & malloc/realloc as needed */
#define POINTS_INCREMENT 1024

/* start points */
struct point
{
    int row;
    int col;
    double value;
    struct point *next;
};

/* stack points for bitmask directions */
struct spoint
{
    int row;
    int col;
    int dir;
    double value;
    struct spoint *next;
};

/* output path points */
struct ppoint
{
    int row;
    int col;
    double value;
};

/* managed list of output path points */
struct point_list
{
    struct ppoint *p;
    int n;
    int nalloc;
};

int dir_degree(int dir_fd, int val_fd, struct point *startp, struct Cell_head *window,
               struct Map_info *Out, struct point_list *pl, int out_mode);
int dir_bitmask(int dir_fd, int val_fd, struct point *startp, struct Cell_head *window,
               struct Map_info *Out, struct point_list *pl, int out_mode);
void pl_add(struct point_list *, struct ppoint *);

/* comparing path points with qsort */
int cmp_pp(const void *a, const void *b)
{
    const struct ppoint *ap = (const struct ppoint *)a;
    const struct ppoint *bp = (const struct ppoint *)b;

    /* 1. ascending by row */
    if (ap->row != bp->row)
	return (ap->row - bp->row);
    /* 2. ascending by col */
    if (ap->col != bp->col)
	return (ap->col - bp->col);
    /* 3. descending by value */
    if (ap->value > bp->value)
	return -1;
    return (ap->value < bp->value);
}

int main(int argc, char **argv)
{
    int fd, dir_fd, val_fd;
    int i, j, have_points = 0;
    int nrows, ncols;
    int npoints;
    int out_id, dir_id, dir_format;
    struct FPRange drange;
    DCELL dmin, dmax;
    char map_name[GNAME_MAX], out_name[GNAME_MAX], dir_name[GNAME_MAX];
    char *tempfile1, *tempfile2;
    struct History history;

    struct Cell_head window;
    struct
    {
	struct Option *dir;
	struct Option *format;
	struct Option *val;
	struct Option *coord;
	struct Option *vpoint;
	struct Option *rast;
	struct Option *vect;
    } opt;
    struct
    {
    	struct Flag *copy;
	struct Flag *accum;
	struct Flag *count;
    } flag;
    struct GModule *module;
    void *dir_buf;

    struct point *head_start_pt = NULL;
    struct point *next_start_pt;
    struct point_list pl, *ppl;
    int start_row, start_col, out_mode;
    double east, north;

    struct line_pnts *Points;
    struct line_cats *Cats;
    struct Map_info vout, *pvout;
    char *desc = NULL;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("cost surface"));
    module->description =
	_("Traces paths from starting points following input directions.");

    opt.dir = G_define_standard_option(G_OPT_R_INPUT);
    opt.dir->label = _("Name of input direction");
    opt.dir->description =
	_("Direction in degrees CCW from east, or bitmask encoded");

    opt.format = G_define_option();
    opt.format->type = TYPE_STRING;
    opt.format->key = "format";
    opt.format->label = _("Format of the input direction map");
    opt.format->required = YES;
    opt.format->options = "auto,degree,45degree,bitmask";
    opt.format->answer = "auto";
    G_asprintf(&desc,
           "auto;%s;degree;%s;45degree;%s;bitmask;%s",
           _("auto-detect direction format"),
           _("degrees CCW from East"),
           _("degrees CCW from East divided by 45 (e.g. r.watershed directions)"),
           _("bitmask encoded directions (e.g. r.cost -b)"));
    opt.format->descriptions = desc;

    opt.val = G_define_standard_option(G_OPT_R_INPUT);
    opt.val->key = "values";
    opt.val->label =
	_("Name of input raster values to be used for output");
    opt.val->required = NO;

    opt.rast = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.rast->key = "raster_path";
    opt.rast->required = NO;
    opt.rast->label = _("Name for output raster path map");
    
    opt.vect = G_define_standard_option(G_OPT_V_OUTPUT);
    opt.vect->key = "vector_path";
    opt.vect->required = NO;
    opt.vect->label = _("Name for output vector path map");
    
    opt.coord = G_define_standard_option(G_OPT_M_COORDS);
    opt.coord->key = "start_coordinates";
    opt.coord->multiple = YES;
    opt.coord->description = _("Coordinates of starting point(s) (E,N)");
    opt.coord->guisection = _("Start");

    opt.vpoint = G_define_standard_option(G_OPT_V_INPUTS);
    opt.vpoint->key = "start_points";
    opt.vpoint->required = NO;
    opt.vpoint->label = _("Name of starting vector points map(s)");
    opt.vpoint->guisection = _("Start");

    flag.copy = G_define_flag();
    flag.copy->key = 'c';
    flag.copy->description = _("Copy input cell values on output");
    flag.copy->guisection = _("Path settings");

    flag.accum = G_define_flag();
    flag.accum->key = 'a';
    flag.accum->description = _("Accumulate input values along the path");
    flag.accum->guisection = _("Path settings");

    flag.count = G_define_flag();
    flag.count->key = 'n';
    flag.count->description = _("Count cell numbers along the path");
    flag.count->guisection = _("Path settings");

    G_option_required(opt.rast, opt.vect, NULL);
    G_option_exclusive(flag.copy, flag.accum, flag.count, NULL);
    G_option_requires_all(flag.copy, opt.rast, opt.val, NULL);
    G_option_requires_all(flag.accum, opt.rast, opt.val, NULL);
    G_option_requires_all(flag.count, opt.rast, NULL);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    strcpy(dir_name, opt.dir->answer);
    *map_name = '\0';
    *out_name = '\0';
    if (opt.rast->answer) {
	strcpy(out_name, opt.rast->answer);
	if (opt.val->answer)
	    strcpy(map_name, opt.val->answer);
    }

    pvout = NULL;
    if (opt.vect->answer) {
	if (0 > Vect_open_new(&vout, opt.vect->answer, 0)) {
            G_fatal_error(_("Unable to create vector map <%s>"),
			  opt.vect->answer);
	}
	Vect_hist_command(&vout);
	pvout = &vout;
    }

    if (flag.copy->answer)
	out_mode = OUT_CPY;
    else if (flag.accum->answer)
	out_mode = OUT_ACC;
    else if (flag.count->answer)
	out_mode = OUT_CNT;
    else
	out_mode = OUT_PID;

    /* get the window information  */
    G_get_window(&window);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    npoints = 0;
    /* TODO: use r.cost method to create a list of start points */
    if (opt.coord->answer) {
	for (i = 0; opt.coord->answers[i] != NULL; i += 2) {
	    G_scan_easting(opt.coord->answers[i], &east, G_projection());
	    G_scan_northing(opt.coord->answers[i + 1], &north, G_projection());
	    start_col = (int)Rast_easting_to_col(east, &window);
	    start_row = (int)Rast_northing_to_row(north, &window);

	    if (start_row < 0 || start_row > nrows ||
		start_col < 0 || start_col > ncols) {
		G_warning(_("Starting point %d is outside the current region"),
			  i + 1);
		continue;
	    }
	    npoints++;
	    have_points = 1;

	    next_start_pt =
		(struct point *)(G_malloc(sizeof(struct point)));

	    next_start_pt->row = start_row;
	    next_start_pt->col = start_col;
	    next_start_pt->value = npoints;
	    next_start_pt->next = head_start_pt;
	    head_start_pt = next_start_pt;
	}
    }
    if (opt.vpoint->answers) {
	for (i = 0; opt.vpoint->answers[i] != NULL; i++) {
	    struct Map_info In;
	    struct bound_box box;
	    int cat, type;

	    Points = Vect_new_line_struct();
	    Cats = Vect_new_cats_struct();

	    Vect_set_open_level(1); /* topology not required */

	    if (1 > Vect_open_old(&In, opt.vpoint->answers[i], ""))
		G_fatal_error(_("Unable to open vector map <%s>"), opt.vpoint->answers[i]);

	    G_verbose_message(_("Reading vector map <%s> with start points..."),
                      Vect_get_full_name(&In));
            
	    Vect_rewind(&In);

	    Vect_region_box(&window, &box);
	    box.T = box.B = 0;

	    while (1) {
		/* register line */
		type = Vect_read_next_line(&In, Points, Cats);

		/* Note: check for dead lines is not needed, because they are skipped by V1_read_next_line_nat() */
		if (type == -1) {
		    G_warning(_("Unable to read vector map"));
		    continue;
		}
		else if (type == -2) {
		    break;
		}
		if (!Vect_point_in_box(Points->x[0], Points->y[0], 0, &box))
		    continue;

		start_col = (int)Rast_easting_to_col(Points->x[0], &window);
		start_row = (int)Rast_northing_to_row(Points->y[0], &window);

		/* effectively just a duplicate check to G_site_in_region() ??? */
		if (start_row < 0 || start_row > nrows || start_col < 0 ||
		    start_col > ncols)
		    continue;

		npoints++;
		have_points = 1;

		next_start_pt =
		    (struct point *)(G_malloc(sizeof(struct point)));

		next_start_pt->row = start_row;
		next_start_pt->col = start_col;
		Vect_cat_get(Cats, 1, &cat);
		next_start_pt->value = cat;
		next_start_pt->next = head_start_pt;
		head_start_pt = next_start_pt;
	    }
	    Vect_close(&In);

	    /* only catches maps out of range until something is found, not after */
	    if (!have_points) {
		G_warning(_("Starting vector map <%s> contains no points in the current region"),
			  opt.vpoint->answers[i]);
	    }
	    Vect_destroy_line_struct(Points);
	    Vect_destroy_cats_struct(Cats);
	}
    }
    if (have_points == 0)
	G_fatal_error(_("No start point(s) specified"));

    /* determine the drainage paths */

    /* get some temp files */
    val_fd = -1;
    tempfile1 = NULL;
    if (opt.val->answer && opt.rast->answer) {
	DCELL *map_buf;

	G_verbose_message(_("Reading raster values map <%s> ..."), map_name);

	tempfile1 = G_tempfile();
	val_fd = open(tempfile1, O_RDWR | O_CREAT, 0666);

	map_buf = Rast_allocate_d_buf();

	fd = Rast_open_old(map_name, "");
	/* transfer the values map to a temp file */
	for (i = 0; i < nrows; i++) {
	    Rast_get_d_row(fd, map_buf, i);
	    if (write(val_fd, map_buf, ncols * sizeof(DCELL)) != 
	        ncols * sizeof(DCELL)) {
		G_fatal_error(_("Unable to write to tempfile"));
	    }
	}
	Rast_close(fd);
	G_free(map_buf);
    }

    if (Rast_read_fp_range(dir_name, "", &drange) < 0)
	G_fatal_error(_("Unable to read range file"));
    Rast_get_fp_range_min_max(&drange, &dmin, &dmax);
    if (dmax <= 0)
	G_fatal_error(_("Invalid directions map <%s>"), dir_name);

    dir_format = -1;
    if (strcmp(opt.format->answer, "degree") == 0) {
	if (dmax > 360)
	    G_fatal_error(_("Directional degrees can not be > 360"));
	dir_format = DIR_DEG;
    }
    else if (strcmp(opt.format->answer, "45degree") == 0) {
	if (dmax > 8)
	    G_fatal_error(_("Directional degrees divided by 45 can not be > 8"));
	dir_format = DIR_DEG45;
    }
    else if (strcmp(opt.format->answer, "bitmask") == 0) {
	if (dmax > (1 << 16) - 1)
	    G_fatal_error(_("Bitmask encoded directions can not be > %d"), (1 << 16) - 1);
	dir_format = DIR_BIT;
    }
    else if (strcmp(opt.format->answer, "auto") == 0) {
	if (dmax <= 8) {
	    dir_format = DIR_DEG45;
	    G_important_message(_("Input direction format assumed to be degrees CCW from East divided by 45"));
	}
	else if (dmax <= (1 << 8) - 1) {
	    dir_format = DIR_BIT;
	    G_important_message(_("Input direction format assumed to be bitmask encoded without Knight's move"));
	}
	else if (dmax <= 360) {
	    dir_format = DIR_DEG;
	    G_important_message(_("Input direction format assumed to be degrees CCW from East"));
	}
	else if (dmax <= (1 << 16) - 1) {
	    dir_format = DIR_BIT;
	    G_important_message(_("Input direction format assumed to be bitmask encoded with Knight's move"));
	}
	else
	    G_fatal_error(_("Unable to detect format of input direction map <%s>"), dir_name);
    }
    if (dir_format <= 0)
	G_fatal_error(_("Invalid directions format '%s'"), opt.format->answer);

    G_verbose_message(_("Reading direction map <%s> ..."), dir_name);
    dir_id = Rast_open_old(dir_name, "");
    tempfile2 = G_tempfile();
    dir_fd = open(tempfile2, O_RDWR | O_CREAT, 0666);

    if (dir_format == DIR_BIT) {
	dir_buf = Rast_allocate_c_buf();
	for (i = 0; i < nrows; i++) {
	    Rast_get_c_row(dir_id, dir_buf, i);
	    if (write(dir_fd, dir_buf, ncols * sizeof(CELL)) != 
	        ncols * sizeof(CELL)) {
		G_fatal_error(_("Unable to write to tempfile"));
	    }
	}
    }
    else {
	dir_buf = Rast_allocate_d_buf();
	for (i = 0; i < nrows; i++) {
	    Rast_get_d_row(dir_id, dir_buf, i);
	    if (dir_format == DIR_DEG45) {
		DCELL *dp;

		dp = (DCELL *)dir_buf;
		for (j = 0; j < ncols; j++, dp++)
		    *dp *= 45;
	    }
	    if (write(dir_fd, dir_buf, ncols * sizeof(DCELL)) !=
	        ncols * sizeof(DCELL)) {
		G_fatal_error(_("Unable to write to tempfile"));
	    }
	}
    }
    Rast_close(dir_id);
    G_free(dir_buf);

    ppl = NULL;
    if (opt.rast->answer) {
	/* raster output */
	pl.p = NULL;
	pl.n = 0;
	pl.nalloc = 0;
	ppl = &pl;
    }

    /* determine the drainage paths */

    /* repeat for each starting point */
    G_verbose_message(_("Processing start points..."));
    next_start_pt = head_start_pt;
    while (next_start_pt) {
	/* follow directions from start points to determine paths */
	/* path tracing algorithm selection */
	if (dir_format == DIR_BIT) {
	    struct Map_info Tmp;

	    if (pvout) {
		if (Vect_open_tmp_new(&Tmp, NULL, 0) < 0)
		    G_fatal_error(_("Unable to create temporary vector map"));
		pvout = &Tmp;
	    }

	    if (!dir_bitmask(dir_fd, val_fd, next_start_pt, &window,
                             pvout, ppl, out_mode)) {
		G_warning(_("No path at row %d, col %d"),
		          next_start_pt->row, next_start_pt->col);
	    }
	    if (pvout) {
		Vect_build_partial(&Tmp, GV_BUILD_BASE);
		G_message(_("Breaking lines..."));
		Vect_break_lines(&Tmp, GV_LINE, NULL);
		Vect_copy_map_lines(&Tmp, &vout);
		Vect_set_release_support(&Tmp);
		Vect_close(&Tmp); /* temporary map is deleted automatically */
		pvout = &vout;
	    }
	}
	else {
	    if (!dir_degree(dir_fd, val_fd, next_start_pt, &window,
                            pvout, ppl, out_mode)) {
		G_warning(_("No path at row %d, col %d"),
		          next_start_pt->row, next_start_pt->col);
	    }
	}
	next_start_pt = next_start_pt->next;
    }

    /* raster output */
    if (opt.rast->answer) {
	int row;

	if (pl.n > 1) {
	    /* sort points */
	    qsort(pl.p, pl.n, sizeof(struct ppoint), cmp_pp);


	    /* remove duplicates */
	    j = 1;
	    for (i = 1; i < pl.n; i++) {
		if (pl.p[j - 1].row != pl.p[i].row ||
		    pl.p[j - 1].col != pl.p[i].col) {
		    
		    pl.p[j].row = pl.p[i].row;
		    pl.p[j].col = pl.p[i].col;
		    pl.p[j].value = pl.p[i].value;
		    j++;
		}
	    }
	    pl.n = j;
	}

	if (out_mode == OUT_PID || out_mode == OUT_CNT) {
	    CELL *out_buf;

	    /* Output will be a cell map */
	    /* open a new file and allocate an output buffer */
	    out_id = Rast_open_c_new(out_name);
	    out_buf = Rast_allocate_c_buf();
	    Rast_set_c_null_value(out_buf, window.cols);
	    row = 0;

	    /* build the output map */
	    G_message(_("Writing output raster map..."));
	    for (i = 0; i < pl.n; i++) {
		while (row < pl.p[i].row) {
		    G_percent(row, nrows, 2);
		    Rast_put_c_row(out_id, out_buf);
		    Rast_set_c_null_value(out_buf, window.cols);
		    row++;
		}
		out_buf[pl.p[i].col] = pl.p[i].value;
	    }
	    while (row < window.rows) {
		G_percent(row, nrows, 2);
		Rast_put_c_row(out_id, out_buf);
		Rast_set_c_null_value(out_buf, window.cols);
		row++;
	    }
	    G_percent(1, 1, 1);
	    G_free(out_buf);
	}
	else {			/* mode = OUT_CPY or OUT_ACC */
	    DCELL *out_buf;

	    /* Output could be of the same type as input */
	    /* open a new file and allocate an output buffer */
	    out_id = Rast_open_new(out_name, DCELL_TYPE);
	    out_buf = Rast_allocate_d_buf();
	    Rast_set_d_null_value(out_buf, window.cols);
	    row = 0;

	    /* build the output map */
	    G_message(_("Writing output raster map..."));
	    for (i = 0; i < pl.n; i++) {
		while (row < pl.p[i].row) {
		    G_percent(row, nrows, 2);
		    Rast_put_d_row(out_id, out_buf);
		    Rast_set_d_null_value(out_buf, window.cols);
		    row++;
		}
		out_buf[pl.p[i].col] = pl.p[i].value;
	    }
	    while (row < window.rows) {
		G_percent(row, nrows, 2);
		Rast_put_d_row(out_id, out_buf);
		Rast_set_d_null_value(out_buf, window.cols);
		row++;
	    }
	    G_percent(1, 1, 1);
	    G_free(out_buf);
	}

	Rast_close(out_id);

	Rast_put_cell_title(out_name, "Path trace");

	Rast_short_history(out_name, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out_name, &history);
    }

    /* vector output */
    if (opt.vect->answer) {
	Vect_build(&vout);
	Vect_close(&vout);
    }

    /* close files and free buffers */
    close(dir_fd);
    unlink(tempfile2);

    if (opt.val->answer && opt.rast->answer) {
	close(val_fd);
	unlink(tempfile1);
    }

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}

void pl_add(struct point_list *pl, struct ppoint *p)
{
    if (pl->n == pl->nalloc) {
	pl->nalloc += POINTS_INCREMENT;
	pl->p = (struct ppoint *)G_realloc(pl->p, pl->nalloc * sizeof(struct ppoint));
	if (!pl->p)
	    G_fatal_error(_("Unable to increase point list"));
    }
    pl->p[pl->n] = *p;
    pl->n++;
}


/* Jenson Domingue 1988, r.fill.dir:
 * bitmask encoded directions CW from North
 * value    log2(value)   direction
 *     1	    0		NE
 *     2	    1		E
 *     4	    2		SE
 *     8	    3		S
 *    16	    4		SW
 *    32	    5		W
 *    64	    6		NW
 *   128	    7		N
 *   256	    8		NEN
 *   512	    9		ENE
 *  1024	   10		ESE
 *  2048	   11		SES
 *  4096	   12		SWS
 *  8192	   13		WSW
 * 16384	   14		WNW
 * 32768	   15		NWN
 * 
 *
 * 8 neighbours 
 *  64 128 1
 *  32  x  2
 *  16  8  4
 * 
 * log2(value) CW from North starting at NE
 * expanded for Knight's move
 * 
 *    15     8
 *  14 6  7  0  9
 *     5  X  1
 *  13 4  3  2 10
 *    12    11
 */
int dir_bitmask(int dir_fd, int val_fd, struct point *startp, struct Cell_head *window,
               struct Map_info *Out, struct point_list *pl, int out_mode)
{
    int go = 1, next_row, next_col, next_dir;
    int npoints;
    int dir_row = -1, val_row = -1;
    CELL direction;
    CELL *dir_buf;
    DCELL *val_buf;
    struct spoint *stackp, *newp;
    struct pavlrc_table *visited;
    struct pavlrc ngbr_rc;
    struct ppoint pp;
    int is_stack;
    int cur_dir, i, npaths;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double x, y;
    double value;
    int col_offset[16] = { 1, 1, 1, 0, -1, -1, -1, 0,
	                   1, 2, 2, 1, -1, -2, -2, -1 };
    int row_offset[16] = { -1, 0, 1, 1, 1, 0, -1, -1,
	                   -2, -1, 1, 2, 2, 1, -1, -2 };

    dir_buf = Rast_allocate_c_buf();
    val_buf = NULL;

    stackp = (struct spoint *)G_malloc(sizeof(struct spoint));

    stackp->row = startp->row;
    stackp->col = startp->col;
    stackp->value = startp->value;
    value = startp->value;
    stackp->dir = -1;
    stackp->next = NULL;

    visited = pavlrc_create(NULL);
    ngbr_rc.row = stackp->row;
    ngbr_rc.col = stackp->col;
    pavlrc_insert(visited, &ngbr_rc);

    if (Out) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
	Vect_cat_set(Cats, 1, (int)stackp->value);
    }

    if (pl) {
	if (out_mode == OUT_CNT)
	    value = 1;
	else if (out_mode == OUT_CPY || out_mode == OUT_ACC) {
	    /* read input raster */
	    val_buf = Rast_allocate_d_buf();
	    if (val_row != stackp->row) {
		lseek(val_fd, (off_t) stackp->row * window->cols * sizeof(DCELL),
		      SEEK_SET);
		if (read(val_fd, val_buf, window->cols * sizeof(DCELL)) !=
		    window->cols * sizeof(DCELL)) {
		    G_fatal_error(_("Unable to read from temp file"));
		}
		val_row = stackp->row;
	    }
	    value = val_buf[stackp->col];
	}
	stackp->value = value;
    }

    /* multi-path tracing:
     * multiple paths from the same starting point */

    npoints = 0;
    while (stackp) {
	is_stack = 1;
	stackp->dir++;
	next_row = stackp->row;
	next_col = stackp->col;
	value = stackp->value;

	/* begin loop */
	go = 1;
	while (go) {
	    go = 0;

	    /* find direction from this point */
	    if (dir_row != next_row) {
		lseek(dir_fd, (off_t) next_row * window->cols * sizeof(CELL),
		      SEEK_SET);
		if (read(dir_fd, dir_buf, window->cols * sizeof(CELL)) !=
		    window->cols * sizeof(CELL)) {
		    G_fatal_error(_("Unable to read from temp file"));
		}
		dir_row = next_row;
	    }
	    direction = *(dir_buf + next_col);
	    
	    if (direction <= 0 || Rast_is_c_null_value(&direction)) {
		/* end of current path: write out the result */
		if (Out && Points->n_points > 1) {
		    Vect_write_line(Out, GV_LINE, Points, Cats);
		}

		/* leave this loop */
		if (is_stack) {
		    newp = stackp->next;
		    G_free(stackp);
		    stackp = newp;
		}
		break;
	    }

	    /* start direction */
	    cur_dir = 0;
	    if (is_stack)
		cur_dir = stackp->dir;
	    
	    /* count paths going from current point and 
	     * get next direction as log2(direction) */
	    next_dir = -1;
	    npaths = 0;
	    for (i = cur_dir; i < 16; i++) {
		if ((direction & (1 << i)) != 0) {
		    npaths++;
		    if (next_dir < 0)
			next_dir = i;
		}
	    }

	    if (is_stack) {
		if (npaths > 0) {
		    stackp->dir = next_dir;
		    /* start a new path */
		    if (Out) {
			Vect_reset_line(Points);
			x = window->west + (next_col + 0.5) * window->ew_res;
			y = window->north - (next_row + 0.5) * window->ns_res;
			Vect_append_point(Points, x, y, 0.0);
		    }
		    if (pl) {
			value = stackp->value;
			pp.row = next_row;
			pp.col = next_col;
			pp.value = value;
			pl_add(pl, &pp);
		    }
		    npoints++;
		}
		else {
		    /* stack point without path ? */
		    if (stackp->dir == 0)
			G_warning(_("No path from row %d, col %d"), 
			          stackp->row, stackp->col);

		    /* drop this point from the stack */
		    G_debug(1, "drop point from stack");
		    newp = stackp->next;
		    G_free(stackp);
		    stackp = newp;
		    break;
		}
	    }
	    else { /* not a stack point */
		if (npaths == 0) {
		    /* should not happen */
		    G_fatal_error(_("Invalid direction %d"), direction);
		}
		if (npaths > 1) {
		    /* write out the result */
		    if (Out && Points->n_points > 1) {
			Vect_write_line(Out, GV_LINE, Points, Cats);
		    }

		    ngbr_rc.row = next_row;
		    ngbr_rc.col = next_col;

		    /* add it to the stack and leave this loop */
		    G_debug(1, "add point to stack: row %d, col %d, dir %d",
			    next_row, next_col, next_dir);
		    newp = (struct spoint *)G_malloc(sizeof(struct spoint));
		    newp->row = next_row;
		    newp->col = next_col;
		    newp->dir = next_dir - 1;
		    newp->value = value;
		    newp->next = stackp;
		    stackp = newp;
		    break;
		}
	    }

	    is_stack = 0;

	    /* identify next downstream cell */
	    next_row += row_offset[next_dir];
	    next_col += col_offset[next_dir];

	    G_debug(1, "next cell at row %d, col %d", next_row, next_col);

	    if (next_col >= 0 && next_col < window->cols
		&& next_row >= 0 && next_row < window->rows) {
		
		/* add point */
		if (Out) {
		    x = window->west + (next_col + 0.5) * window->ew_res;
		    y = window->north - (next_row + 0.5) * window->ns_res;
		    Vect_append_point(Points, x, y, 0.0);
		}
		if (pl) {
		    if (out_mode == OUT_CNT)
			value += 1;
		    else if (out_mode == OUT_CPY || out_mode == OUT_ACC) {
			/* read input raster */
			if (val_row != next_row) {
			    lseek(val_fd, (off_t) next_row * window->cols * sizeof(DCELL),
			          SEEK_SET);
			    if (read(val_fd, val_buf, window->cols * sizeof(DCELL)) !=
			        window->cols * sizeof(DCELL)) {
				G_fatal_error(_("Unable to read from temp file"));
			    }
			    val_row = next_row;
			}
			
			if (out_mode == OUT_CPY)
			    value = val_buf[next_col];
			else
			    value += val_buf[next_col];
		    }
		    pp.row = next_row;
		    pp.col = next_col;
		    pp.value = value;
		    pl_add(pl, &pp);
		}

		/* avoid circular paths 
		 * avoid tracing the same path segment several times:
		 * a path can split and later on merge again, 
		 * then split again, merge again
		 * path segments are identical from every merge point 
		 * to the next split point */
		ngbr_rc.row = next_row;
		ngbr_rc.col = next_col;
		if (!pavlrc_insert(visited, &ngbr_rc)) {
		    go = 1;
		    npoints++;
		}
		else {
		    /* reached a merge point */
		    /* write out the result */
		    if (Out && Points->n_points > 1) {
			Vect_write_line(Out, GV_LINE, Points, Cats);
		    }
		}
	    }
	    else {
		G_warning(_("Path is leaving the current region"));
	    }
	}				/* end while */
    }

    pavlrc_destroy(visited);

    G_free(dir_buf);
    if (val_buf)
	G_free(val_buf);

    if (Out) {
	Vect_destroy_line_struct(Points);
	Vect_destroy_cats_struct(Cats);
    }

    return (npoints > 1);
}

int dir_degree(int dir_fd, int val_fd, struct point *startp, struct Cell_head *window,
               struct Map_info *Out, struct point_list *pl, int out_mode)
{
    /*
     * The idea is that each cell of the direction surface has a value representing
     * the direction towards the next cell in the path. The direction is read from 
     * the input raster, and a simple case/switch is used to determine which cell to
     * read next. This is repeated via a while loop until a null direction is found.
     * 
     * directions are degrees CCW from East with East = 360
     */

    int neighbour, next_row, next_col, go = 1;
    int npoints;
    int dir_row = -1, val_row = -1;
    DCELL direction;
    DCELL *dir_buf;
    DCELL *val_buf;
    double x, y;
    double value;
    struct ppoint pp;
    struct line_pnts *Points;
    struct line_cats *Cats;

    dir_buf = Rast_allocate_d_buf();
    val_buf = NULL;

    next_row = startp->row;
    next_col = startp->col;
    value = startp->value;

    if (Out) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
	Vect_cat_set(Cats, 1, (int)value);

	x = window->west + (next_col + 0.5) * window->ew_res;
	y = window->north - (next_row + 0.5) * window->ns_res;
	Vect_append_point(Points, x, y, 0.0);
    }

    if (pl) {
	if (out_mode == OUT_CNT)
	    value = 1;
	else if (out_mode == OUT_CPY || out_mode == OUT_ACC) {
	    /* read input raster */
	    val_buf = Rast_allocate_d_buf();
	    if (val_row != next_row) {
		lseek(val_fd, (off_t) next_row * window->cols * sizeof(DCELL),
		      SEEK_SET);
		if (read(val_fd, val_buf, window->cols * sizeof(DCELL)) !=
		    window->cols * sizeof(DCELL)) {
		    G_fatal_error(_("Unable to read from temp file"));
		}
		val_row = next_row;
	    }

	    value = val_buf[next_col];
	}
	pp.row = next_row;
	pp.col = next_col;
	pp.value = value;
	pl_add(pl, &pp);
    }

    npoints = 1;
    while (go) {
	go = 0;
	/* Directional algorithm
	 * 1) read cell direction               
	 * 2) shift to cell in that direction           
	 */
	/* find the direction recorded at row,col */
	if (dir_row != next_row) {
	    lseek(dir_fd, (off_t) next_row * window->cols * sizeof(DCELL),
	          SEEK_SET);
	    if (read(dir_fd, dir_buf, window->cols * sizeof(DCELL)) !=
	        window->cols * sizeof(DCELL)) {
		G_fatal_error(_("Unable to read from temp file"));
	    }
	    dir_row = next_row;
	}
	direction = *(dir_buf + next_col);
	neighbour = 0;
	if (!Rast_is_d_null_value(&direction)) {
	    neighbour = direction * 10;
	    G_debug(2, "direction read: %lf, neighbour found: %i",
			  direction, neighbour);
	}
	switch (neighbour) {
	case 225: /* ENE */
	    next_row -= 1;
	    next_col += 2;
	    break;
	case 450: /* NE */
	    next_row -= 1;
	    next_col += 1;
	    break;
	case 675: /* NNE */
	    next_row -= 2;
	    next_col += 1;
	    break;
	case 900: /* N */
	    next_row -= 1;
	    break;
	case 1125: /* NNW */
	    next_row -= 2;
	    next_col -= 1;
	    break;
	case 1350: /* NW */
	    next_col -= 1;
	    next_row -= 1;
	    break;
	case 1575: /* WNW */
	    next_col -= 2;
	    next_row -= 1;
	    break;
	case 1800: /* W*/
	    next_col -= 1;
	    break;
	case 2025: /* WSW */
	    next_row += 1;
	    next_col -= 2;
	    break;
	case 2250: /* SW */
	    next_row += 1;
	    next_col -= 1;
	    break;
	case 2475: /* SSW */
	    next_row += 2;
	    next_col -= 1;
	    break;
	case 2700: /* S */
	    next_row += 1;
	    break;
	case 2925: /* SSE */
	    next_row += 2;
	    next_col += 1;
	    break;
	case 3150: /* SE */
	    next_row += 1;
	    next_col += 1;
	    break;
	case 3375: /* ESE */
	    next_row += 1;
	    next_col += 2;
	    break;
	case 3600: /* E */
	    next_col += 1;
	    break;
	default:
	    /* end of path */
	    next_row = -1;
	    next_col = -1;
            break;
	}			/* end switch/case */

	if (next_col >= 0 && next_col < window->cols && next_row >= 0 &&
	    next_row < window->rows) {

	    if (Out) {
		x = window->west + (next_col + 0.5) * window->ew_res;
		y = window->north - (next_row + 0.5) * window->ns_res;
		Vect_append_point(Points, x, y, 0.0);
	    }
	    if (pl) {
		if (out_mode == OUT_CNT)
		    value += 1;
		else if (out_mode == OUT_CPY || out_mode == OUT_ACC) {
		    /* read input raster */
		    if (val_row != next_row) {
			lseek(val_fd, (off_t) next_row * window->cols * sizeof(DCELL),
			      SEEK_SET);
			if (read(val_fd, val_buf, window->cols * sizeof(DCELL)) !=
			    window->cols * sizeof(DCELL)) {
			    G_fatal_error(_("Unable to read from temp file"));
			}
			val_row = next_row;
		    }
		    
		    if (out_mode == OUT_CPY)
			value = val_buf[next_col];
		    else
			value += val_buf[next_col];
		}
		pp.row = next_row;
		pp.col = next_col;
		pp.value = value;
		pl_add(pl, &pp);
	    }

	    go = 1;
	    npoints++;
	}

    }				/* end while */

    if (Out && Points->n_points > 1) {
	Vect_write_line(Out, GV_LINE, Points, Cats);
    }

    G_free(dir_buf);
    if (val_buf)
	G_free(val_buf);

    if (Out) {
	Vect_destroy_line_struct(Points);
	Vect_destroy_cats_struct(Cats);
    }

    return (npoints > 1);
}
