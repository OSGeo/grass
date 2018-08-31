#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "format.h"
#include "local_proto.h"

int poly_to_rast(char *input_file, char *raster_map, char *title, int nrows, int raster_type, int *null)
{
    double *x, *y;
    int count;
    int cat_int;
    double cat_double;
    int type;
    struct Categories labels;
    FILE *ifd;			/* for input file */
    int rfd;			/* for raster map */
    int format;
    int stat;
    int pass, npasses;
    struct History history;


   /* open input file */
    if (strcmp("-", input_file) == 0)
	ifd = stdin;
    else
	ifd = fopen(input_file, "r");

    if (ifd == NULL) {
	perror(input_file);
	exit(EXIT_FAILURE);
    }

    rfd = Rast_open_new(raster_map, raster_type);

    if (title == NULL)
	title = "";
    G_strip(title);

    Rast_init_cats(title, &labels);

    format = getformat(ifd, raster_type, null);
    
    /* ?? otherwise get complaints about window changes */
    G_suppress_warnings(TRUE);
    npasses = begin_rasterization(nrows, format);
    G_suppress_warnings(FALSE);

    pass = 0;

    do {
	pass++;
	if (npasses > 1)
	    G_message(_("Pass #%d (of %d) ..."), pass, npasses);

	G_fseek(ifd, 0L, 0);
	while (get_item(ifd, format, &type, &cat_int, &cat_double, &x, &y, &count, &labels)) {
	    if (format == USE_FCELL || format == USE_DCELL)
		set_cat_double(cat_double);
	    else
		set_cat_int(cat_int);
	    switch (type) {
	    case 'A':
		G_plot_polygon(x, y, count);
		break;
	    case 'L':
		while (--count > 0) {
		    G_plot_line2(x[0], y[0], x[1], y[1]);
		    x++;
		    y++;
		}
		break;
	    case 'P':
		G_plot_point(x[0], y[0]);
		break;
	    }
	}

	G_message(_("Writing raster map..."));

	stat = output_raster(rfd, null);
    } while (stat == 0);
    /* stat: 0 means repeat
     *       1 means done
     *      -1 means error
     */

    if (stat < 0) {
	Rast_unopen(rfd);
	return 1;
    }

    Rast_close(rfd);
    Rast_write_cats(raster_map, &labels);
    Rast_short_history(raster_map, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(raster_map, &history);

    return 0;
}
