#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

#include "bouman.h"

int write_img(unsigned char **img, float **goodness, int ncols, int nrows,
              struct SigSet *S,	/* class parameters */
	      struct parms *parms,	/* parms: command line parameters */
	      struct files *files)
{				/* files: contains file to output */
    int row, col;
    FCELL *fcellbuf = NULL;

    G_message(_("Writing raster map <%s>..."), parms->output_map);

    /* write goodness of fit */
    if (parms->goodness_map)
	fcellbuf = Rast_allocate_f_buf();

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	for (col = 0; col < ncols; col++) {
	    int class = (int)img[row][col];
		
	    G_debug(3, "class: [%d] row/col: [%d][%d]", class, row, col);
	    files->outbuf[col] = (CELL) S->ClassSig[class].classnum;
	    
	    if (parms->goodness_map)
		fcellbuf[col] = goodness[row][col];
	}
	Rast_put_row(files->output_fd, files->outbuf, CELL_TYPE);
	if (parms->goodness_map)
	    Rast_put_row(files->goodness_fd, fcellbuf, FCELL_TYPE);
    }
    G_percent(nrows, nrows, 2);

    return 0;
}
