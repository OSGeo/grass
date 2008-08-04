
#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"

#define FFTWINDOW "fftwindow"


int put_orig_window(struct Cell_head *hd)
{
    char buffer[100];

    /* save the window */
    sprintf(buffer, "cell_misc/%s", Cellmap_real);
    G__put_window(hd, buffer, FFTWINDOW);
    sprintf(buffer, "cell_misc/%s", Cellmap_imag);
    G__put_window(hd, buffer, FFTWINDOW);

    return 0;
}
