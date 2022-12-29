#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "globals.h"


void openfiles(char *r_name, char *g_name, char *b_name,
	       char *h_name, char *i_name, char *s_name,
	       int fd_input[3], int fd_output[3], CELL * rowbuf[3])
{
    fd_input[0] = Rast_open_old(r_name, "");
    fd_input[1] = Rast_open_old(g_name, "");
    fd_input[2] = Rast_open_old(b_name, "");

    /* open output files */
    fd_output[0] = Rast_open_c_new(h_name);
    fd_output[1] = Rast_open_c_new(i_name);
    fd_output[2] = Rast_open_c_new(s_name);

    /* allocate the cell row buffer */
    rowbuf[0] = Rast_allocate_c_buf();
    rowbuf[1] = Rast_allocate_c_buf();
    rowbuf[2] = Rast_allocate_c_buf();
}
