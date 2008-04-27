#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"

#define FFTWINDOW "fftwindow"


int 
get_orig_window (struct Cell_head *hd, char *rmapset, char *imapset)
{
  struct Cell_head tmphd;
  char buffer[100];

  /* get the windows for both the real and imaginary parts */
  sprintf(buffer, "cell_misc/%s", Cellmap_real);
  G__get_window(hd, buffer, FFTWINDOW, rmapset);
  sprintf(buffer, "cell_misc/%s", Cellmap_imag);
  G__get_window(&tmphd, buffer, FFTWINDOW, imapset);

  /* compare them */
  if (hd->proj != tmphd.proj ||
      hd->zone != tmphd.zone ||
      hd->north != tmphd.north ||
      hd->south != tmphd.south ||
      hd->east != tmphd.east ||
      hd->west != tmphd.west ||
      hd->ew_res != tmphd.ew_res ||
      hd->ns_res != tmphd.ns_res)
    G_fatal_error(_("The real and imaginary original windows did not match."));

  return 0;
}


