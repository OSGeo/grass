#include <stdio.h>
#include "global.h"

#define F2I(map_type) \
	(map_type == CELL_TYPE ? 0 : (map_type == FCELL_TYPE ? 1 : 2))

static void process_row_ii(int), process_row_if(int), process_row_id(int);
static void process_row_fi(int), process_row_ff(int), process_row_fd(int);
static void process_row_di(int), process_row_df(int), process_row_dd(int);

static void (*process_row_FtypeOtype [3][3])() =
 { {process_row_ii, process_row_if, process_row_id},
   {process_row_fi, process_row_ff, process_row_fd},
   {process_row_di, process_row_df, process_row_dd} };

#define PROCESS_ROW \
     (process_row_FtypeOtype [F2I (in_type)] [F2I (out_type)])

static void *in_rast, *out_rast;
static int nrows, ncols;

int do_recode (void)
{
   struct Cell_head window, cellhd;
   int row, i;
   struct History hist ;

/* set the window from the header for the input file */
   if (align_wind)
   {
	G_get_window (&window);
	if (G_get_cellhd (name, mapset, &cellhd) >= 0)
	{
	    G_align_window (&window, &cellhd);
	    G_set_window (&window);
	}
   }
    
   G_get_set_window (&window);

   nrows = G_window_rows();
   ncols = G_window_cols();

   /* open the input file for reading */
   in_fd = G_open_cell_old (name, mapset);
   if (in_fd < 0) G_fatal_error("Can't open input map");

   out_fd = G_open_raster_new (result, out_type);

   out_rast = G_allocate_raster_buf(out_type);
   in_rast = G_allocate_raster_buf(in_type);

   for (row = 0; row < nrows; row++)
   {
     G_percent (row, nrows, 2);
     PROCESS_ROW(row);
   }
   G_percent (row, nrows, 2);
   G_close_cell (in_fd);
   G_close_cell (out_fd);
   
   /* writing history file */
   G_short_history(result, "raster", &hist);
   sprintf(hist.edhist[0], "recode of raster map %s", name); 
   /* if there are more rules than history lines allocated, write only 
      MAXEDLINES-1 rules , and "...." as a last rule */
       for(i=0; (i< nrules) && (i<MAXEDLINES-2);i++)
      sprintf(hist.edhist[i+1], "%s", rules[i]); 
   if(nrules > MAXEDLINES-1)
   {
      sprintf(hist.edhist[MAXEDLINES-1], "..."); 
      hist.edlinecnt = MAXEDLINES;
   }
   else
      hist.edlinecnt = nrules + 1;
   sprintf(hist.datsrc_1,"raster map %s", name);
   G_write_history (result, &hist);

   return 0;
}

static void process_row_ii(int row)
{
   if(no_mask) 
      G_get_c_raster_row_nomask (in_fd, (CELL *) in_rast, row);
   else
      G_get_c_raster_row(in_fd, (CELL *) in_rast, row);
   G_fpreclass_perform_ii(&rcl_struct, (CELL *) in_rast, (CELL *) out_rast, ncols);
   G_put_raster_row (out_fd, (CELL *) out_rast, CELL_TYPE);
}

static void process_row_if (int row)
{
   if(no_mask) 
      G_get_c_raster_row_nomask (in_fd, (CELL *) in_rast, row);
   else
      G_get_c_raster_row(in_fd, (CELL *) in_rast, row);
   G_fpreclass_perform_if(&rcl_struct, (CELL *) in_rast, (FCELL *) out_rast, ncols);
   G_put_f_raster_row (out_fd, (FCELL *) out_rast);
}

static void process_row_id(int row)
{
   if(no_mask) 
      G_get_c_raster_row_nomask (in_fd, (CELL *) in_rast, row);
   else
      G_get_c_raster_row(in_fd, (CELL *) in_rast, row);
   G_fpreclass_perform_id(&rcl_struct, (CELL *) in_rast, (DCELL *) out_rast, ncols);
   G_put_raster_row (out_fd, (DCELL *) out_rast,DCELL_TYPE);
}

static void process_row_fi(int row)
{
   if(no_mask) 
      G_get_f_raster_row_nomask (in_fd, (FCELL *) in_rast, row);
   else
      G_get_f_raster_row(in_fd, (FCELL *) in_rast, row);
   G_fpreclass_perform_fi(&rcl_struct, (FCELL *) in_rast, (CELL *) out_rast, ncols);
   G_put_raster_row (out_fd, (CELL *) out_rast, CELL_TYPE);
}

static void process_row_ff(int row)
{
   if(no_mask) 
      G_get_f_raster_row_nomask (in_fd, (FCELL *) in_rast, row);
   else
      G_get_f_raster_row(in_fd, (FCELL *) in_rast, row);
   G_fpreclass_perform_ff(&rcl_struct, (FCELL *) in_rast, (FCELL *) out_rast, ncols);
   G_put_f_raster_row (out_fd, (FCELL *) out_rast);
}

static void process_row_fd(int row)
{
   if(no_mask) 
      G_get_f_raster_row_nomask (in_fd, (FCELL *) in_rast, row);
   else
      G_get_f_raster_row(in_fd, (FCELL *) in_rast, row);
   G_fpreclass_perform_fd(&rcl_struct, (FCELL *) in_rast, (DCELL *) out_rast, ncols);
   G_put_raster_row (out_fd, (DCELL *) out_rast,DCELL_TYPE);
}

static void process_row_di(int row)
{
   if(no_mask) 
      G_get_d_raster_row_nomask (in_fd, (DCELL *) in_rast, row);
   else
      G_get_d_raster_row(in_fd, (DCELL *) in_rast, row);
   G_fpreclass_perform_di(&rcl_struct, (DCELL *) in_rast, (CELL *) out_rast, ncols);
   G_put_raster_row (out_fd, (CELL *) out_rast, CELL_TYPE);
}

static void process_row_df(int row)
{
   if(no_mask) 
      G_get_d_raster_row_nomask (in_fd, (DCELL *) in_rast, row);
   else
      G_get_d_raster_row(in_fd, (DCELL *) in_rast, row);
   G_fpreclass_perform_df(&rcl_struct, (DCELL *) in_rast, (FCELL *) out_rast, ncols);
   G_put_f_raster_row (out_fd, (FCELL *) out_rast);
}

static void process_row_dd(int row)
{
   if(no_mask) 
      G_get_d_raster_row_nomask (in_fd, (DCELL *) in_rast, row);
   else
      G_get_d_raster_row(in_fd, (DCELL *) in_rast, row);
   G_fpreclass_perform_dd(&rcl_struct, (DCELL *) in_rast, (DCELL *) out_rast, ncols);
   G_put_raster_row (out_fd, (DCELL *) out_rast,DCELL_TYPE);
}

