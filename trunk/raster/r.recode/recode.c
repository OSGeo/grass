#include <stdio.h>
#include <grass/raster.h>
#include "global.h"

#define F2I(map_type) \
	(map_type == CELL_TYPE ? 0 : (map_type == FCELL_TYPE ? 1 : 2))

static void process_row_ii(int), process_row_if(int), process_row_id(int);
static void process_row_fi(int), process_row_ff(int), process_row_fd(int);
static void process_row_di(int), process_row_df(int), process_row_dd(int);

static void (*process_row_FtypeOtype[3][3]) () = { {
process_row_ii, process_row_if, process_row_id}, {
process_row_fi, process_row_ff, process_row_fd}, {
process_row_di, process_row_df, process_row_dd}};

#define PROCESS_ROW \
     (process_row_FtypeOtype [F2I (in_type)] [F2I (out_type)])

static void *in_rast, *out_rast;
static int nrows, ncols;

int do_recode(void)
{
    struct Cell_head window, cellhd;
    int row, i;
    struct History hist;

    /* set the window from the header for the input file */
    if (align_wind) {
	G_get_window(&window);
	Rast_get_cellhd(name, "", &cellhd);
	Rast_align_window(&window, &cellhd);
	Rast_set_window(&window);
    }

    G_get_set_window(&window);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* open the input file for reading */
    in_fd = Rast_open_old(name, "");

    out_fd = Rast_open_new(result, out_type);

    out_rast = Rast_allocate_buf(out_type);
    in_rast = Rast_allocate_buf(in_type);

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	PROCESS_ROW(row);
    }
    G_percent(row, nrows, 2);
    Rast_close(in_fd);
    Rast_close(out_fd);

    /* writing history file */
    Rast_short_history(result, "raster", &hist);
    Rast_append_format_history(&hist, "recode of raster map %s", name);
    /* if there are more rules than history lines allocated, write only 
       MAXEDLINES-1 rules , and "...." as a last rule */
    for (i = 0; i < nrules && i < 50; i++)
	Rast_append_history(&hist, rules[i]);
    if (nrules > 50)
	Rast_append_history(&hist, "...");
    Rast_format_history(&hist, HIST_DATSRC_1, "raster map %s", name);

    Rast_command_history(&hist);
    Rast_write_history(result, &hist);

    return 0;
}

static void process_row_ii(int row)
{
    if (no_mask)
	Rast_get_c_row_nomask(in_fd, (CELL *) in_rast, row);
    else
	Rast_get_c_row(in_fd, (CELL *) in_rast, row);
    Rast_fpreclass_perform_ii(&rcl_struct, (CELL *) in_rast, (CELL *) out_rast,
			   ncols);
    Rast_put_row(out_fd, (CELL *) out_rast, CELL_TYPE);
}

static void process_row_if(int row)
{
    if (no_mask)
	Rast_get_c_row_nomask(in_fd, (CELL *) in_rast, row);
    else
	Rast_get_c_row(in_fd, (CELL *) in_rast, row);
    Rast_fpreclass_perform_if(&rcl_struct, (CELL *) in_rast, (FCELL *) out_rast,
			   ncols);
    Rast_put_f_row(out_fd, (FCELL *) out_rast);
}

static void process_row_id(int row)
{
    if (no_mask)
	Rast_get_c_row_nomask(in_fd, (CELL *) in_rast, row);
    else
	Rast_get_c_row(in_fd, (CELL *) in_rast, row);
    Rast_fpreclass_perform_id(&rcl_struct, (CELL *) in_rast, (DCELL *) out_rast,
			   ncols);
    Rast_put_row(out_fd, (DCELL *) out_rast, DCELL_TYPE);
}

static void process_row_fi(int row)
{
    if (no_mask)
	Rast_get_f_row_nomask(in_fd, (FCELL *) in_rast, row);
    else
	Rast_get_f_row(in_fd, (FCELL *) in_rast, row);
    Rast_fpreclass_perform_fi(&rcl_struct, (FCELL *) in_rast, (CELL *) out_rast,
			   ncols);
    Rast_put_row(out_fd, (CELL *) out_rast, CELL_TYPE);
}

static void process_row_ff(int row)
{
    if (no_mask)
	Rast_get_f_row_nomask(in_fd, (FCELL *) in_rast, row);
    else
	Rast_get_f_row(in_fd, (FCELL *) in_rast, row);
    Rast_fpreclass_perform_ff(&rcl_struct, (FCELL *) in_rast, (FCELL *) out_rast,
			   ncols);
    Rast_put_f_row(out_fd, (FCELL *) out_rast);
}

static void process_row_fd(int row)
{
    if (no_mask)
	Rast_get_f_row_nomask(in_fd, (FCELL *) in_rast, row);
    else
	Rast_get_f_row(in_fd, (FCELL *) in_rast, row);
    Rast_fpreclass_perform_fd(&rcl_struct, (FCELL *) in_rast, (DCELL *) out_rast,
			   ncols);
    Rast_put_row(out_fd, (DCELL *) out_rast, DCELL_TYPE);
}

static void process_row_di(int row)
{
    if (no_mask)
	Rast_get_d_row_nomask(in_fd, (DCELL *) in_rast, row);
    else
	Rast_get_d_row(in_fd, (DCELL *) in_rast, row);
    Rast_fpreclass_perform_di(&rcl_struct, (DCELL *) in_rast, (CELL *) out_rast,
			   ncols);
    Rast_put_row(out_fd, (CELL *) out_rast, CELL_TYPE);
}

static void process_row_df(int row)
{
    if (no_mask)
	Rast_get_d_row_nomask(in_fd, (DCELL *) in_rast, row);
    else
	Rast_get_d_row(in_fd, (DCELL *) in_rast, row);
    Rast_fpreclass_perform_df(&rcl_struct, (DCELL *) in_rast, (FCELL *) out_rast,
			   ncols);
    Rast_put_f_row(out_fd, (FCELL *) out_rast);
}

static void process_row_dd(int row)
{
    if (no_mask)
	Rast_get_d_row_nomask(in_fd, (DCELL *) in_rast, row);
    else
	Rast_get_d_row(in_fd, (DCELL *) in_rast, row);
    Rast_fpreclass_perform_dd(&rcl_struct, (DCELL *) in_rast, (DCELL *) out_rast,
			   ncols);
    Rast_put_row(out_fd, (DCELL *) out_rast, DCELL_TYPE);
}
