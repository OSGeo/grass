#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>


void *read_raster(void *buf, const int fd, const RASTER_MAP_TYPE rtype)
{
    void *tmpbuf = buf;
    int rows = Rast_window_rows();
    int i;

    G_message(_("Reading raster map..."));

    for (i = 0; i < rows; i++) {
	G_percent(i + 1, rows, 10);

	Rast_get_row(fd, tmpbuf, i, rtype);
	tmpbuf =
	    G_incr_void_ptr(tmpbuf, Rast_cell_size(rtype) * Rast_window_cols());
    }

    return tmpbuf;
}


void *write_raster(void *buf, const int fd, const RASTER_MAP_TYPE rtype)
{
    void *tmpbuf = buf;
    int rows = Rast_window_rows();
    int i;

    G_message(_("Writing raster map..."));

    for (i = 0; i < rows; i++) {
	G_percent(i, rows, 10);

	Rast_put_row(fd, tmpbuf, rtype);
	tmpbuf =
	    G_incr_void_ptr(tmpbuf, Rast_cell_size(rtype) * Rast_window_cols());
    }

    return tmpbuf;
}
