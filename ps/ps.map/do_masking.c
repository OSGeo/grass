/* Function: do_masking
 **
 ** Author: Paul W. Carlson     May 1992
 */

#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "ps_info.h"

extern char *ps_mask_file;

int do_masking(void)
{
    FILE *ps_mask_fp;
    int rows, cols;
    double factor, width;
    char buf[128];

    /* open the temporary mask file */
    if ((ps_mask_fp = fopen(ps_mask_file, "r")) == NULL)
	G_fatal_error(_("Can't open temporary PostScript mask file."));


    /* adjust columns to multiple of 8 */
    rows = Rast_window_rows();
    cols = Rast_window_cols();
    while (cols % 8)
	cols++;
    factor = (double)cols / (double)Rast_window_cols();
    width = factor * PS.map_pix_wide;

    /* write mask to PostScript file, using "no data" color */
    fprintf(PS.fp, "gsave\n");
    fprintf(PS.fp, "/imgstrg %d string def\n", cols / 8);
    fprintf(PS.fp, "/cw %d def /ch %d def\n", cols, rows);
    fprintf(PS.fp, "%.2f %.2f TR\n", PS.map_left, PS.map_bot);
    fprintf(PS.fp, "%d %d scale\n",
	    (int)(width + 0.5), (int)(PS.map_pix_high + 0.5));
    if (PS.mask_color == 1) {
	fprintf(PS.fp, "%.3f %.3f %.3f C\n", PS.mask_r, PS.mask_g, PS.mask_b);
    }
    else {
	fprintf(PS.fp, "%.3f %.3f %.3f C\n", PS.r0, PS.g0, PS.b0);
    }
    fprintf(PS.fp, "cw ch true\n");
    fprintf(PS.fp, "[cw 0 0 ch neg 0 ch]\n");
    fprintf(PS.fp, "{currentfile imgstrg readhexstring pop}\n");
    fprintf(PS.fp, "imagemask\n");
    while (fgets(buf, 128, ps_mask_fp) != NULL)
	fprintf(PS.fp, "%s", buf);
    fprintf(PS.fp, "grestore\n");

    /* close and remove temporary mask file */
    fclose(ps_mask_fp);
    unlink(ps_mask_file);

    return 0;
}
