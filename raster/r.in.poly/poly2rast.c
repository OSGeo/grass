#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "format.h"
#include "local_proto.h"

int poly_to_rast(char *input_file, char *raster_map, char *title, int nrows)
{
    double *x, *y;
    int count;
    long cat;
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

    rfd = G_open_cell_new(raster_map);
    if (rfd < 0)
	G_fatal_error(_("Can't create raster map <%s>"), raster_map);

    if (title == NULL)
	title = "";
    G_strip(title);

    G_init_cats((CELL) 0, title, &labels);

    format = getformat(ifd);
    npasses = begin_rasterization(nrows, format);
    pass = 0;

    do {
	pass++;
	if (npasses > 1)
	    G_message(_("Pass #%d (of %d) ..."), pass, npasses);

	fseek(ifd, 0L, 0);
	while (get_item(ifd, &type, &cat, &x, &y, &count, &labels)) {
	    set_cat(cat);
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

	stat = output_raster(rfd);
    } while (stat == 0);
    /* stat: 0 means repeat
     *       1 means done
     *      -1 means error
     */

    if (stat < 0) {
	G_unopen_cell(rfd);
	return 1;
    }

    G_close_cell(rfd);
    G_write_cats(raster_map, &labels);
    G_short_history(raster_map, "raster", &history);
    G_command_history(&history);
    G_write_history(raster_map, &history);

    return 0;
}
