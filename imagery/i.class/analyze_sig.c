#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"
#include "local_proto.h"

int analyze_sig(void)
{
    int nbands;

    if (!Region.area.completed) {
	G_warning(_("Cannot analyze until region is completed."));
	return (0);
    }

    Menu_msg("");

    /* allocate row buffers and open raster maps */
    Rast_set_window(&Band_cellhd);
    nbands = Refer.nfiles;
    open_band_files();

    /* gather all points which fall within the polygon */
    /* perform a signature of the area */

    signalflag.interrupt = 0;
    if (!outline())
	goto done;
    if (!prepare_signature(nbands)) {
	close_band_files();
	goto done;
    }
    close_band_files();

    show_signature(nbands, 1.5);
    save_signature();

  done:
    /* clean up after analysis */
    if (Region.perimeter) {
	G_free(Region.perimeter);
	Region.perimeter = 0;
    }
    erase_region();
    Menu_msg("");

    return (0);
}
