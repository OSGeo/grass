#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"


void openfiles(char *r_name, char *g_name, char *b_name,
	       char *h_name, char *i_name, char *s_name,
	       int fd_input[3], int fd_output[3], CELL * rowbuf[3])
{
    char *mapset;

    if ((mapset = G_find_cell(r_name, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), r_name);
    if ((mapset = G_find_cell(g_name, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), g_name);
    if ((mapset = G_find_cell(b_name, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), b_name);

    if ((fd_input[0] = G_open_cell_old(r_name, mapset)) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), r_name);
    if ((fd_input[1] = G_open_cell_old(g_name, mapset)) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), g_name);
    if ((fd_input[2] = G_open_cell_old(b_name, mapset)) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), b_name);

    /* open output files */
    if ((fd_output[0] = G_open_cell_new(h_name)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), h_name);
    if ((fd_output[1] = G_open_cell_new(i_name)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), i_name);
    if ((fd_output[2] = G_open_cell_new(s_name)) < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), s_name);

    /* allocate the cell row buffer */
    if ((rowbuf[0] = G_allocate_cell_buf()) == NULL)
	G_fatal_error(_("Unable to allocate the input row buffer"));
    if ((rowbuf[1] = G_allocate_cell_buf()) == NULL)
	G_fatal_error(_("Unable to allocate the input row buffer"));
    if ((rowbuf[2] = G_allocate_cell_buf()) == NULL)
	G_fatal_error(_("Unable to allocate the input row buffer"));

    return;
}
