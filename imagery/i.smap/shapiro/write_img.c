#include <grass/imagery.h>
#include <grass/glocale.h>
#include "bouman.h"


int write_img (
    unsigned char **img, int ncols, int nrows,
    struct SigSet *S,            /* class parameters */ 
    struct parms *parms,         /* parms: command line parameters */
    struct files *files)         /* files: contains file to output */
{
    int row, col;

    if (!parms->quiet)
        G_message(_("Writing [%s] ..."), parms->output_map);

    for (row = 0; row < nrows; row++)
    {
	if (!parms->quiet) G_percent (row, nrows, 2);
	for (col = 0; col < ncols; col++)
	{
	   if(0 && img[row][col] == 0)
		G_set_c_null_value(&files->outbuf[col], 1);
           else
	   {
		int class = (int)img[row][col];
		G_debug(3, "class: [%d] row/col: [%d][%d]", class, row, col);
		files->outbuf[col] = (CELL)S->ClassSig[class].classnum;
           }
	}
	G_put_raster_row (files->output_fd, files->outbuf, CELL_TYPE);
    }
    if (!parms->quiet) G_percent (row, nrows, 2);

    return 0;
}
