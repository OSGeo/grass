
/*****************************************************************************/

/***                                                                       ***/

/***                             close_down()                              ***/

/***   	   Closes all input and output raster maps and frees memory.	   ***/

/***               Jo Wood, Project ASSIST, 7th February 1993              ***/

/***                                                                       ***/

/*****************************************************************************/

#include "param.h"


void close_down(void)
{
    struct History history;

    /* Close connection with existing input raster. */
    G_unopen_cell(fd_in);

    /* Write output raster map and close connection. */
    G_close_cell(fd_out);

    G_short_history(rast_out_name, "raster", &history);
    G_command_history(&history);
    G_write_history(rast_out_name, &history);

}
