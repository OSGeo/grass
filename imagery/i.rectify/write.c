#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <grass/raster.h>
#include <grass/glocale.h>

#include "global.h"

int write_matrix(int row, int col)
{
    int n;
    off_t offset;

    select_target_env();
    if (!temp_fd) {
	temp_name = G_tempfile();
	temp_fd = creat(temp_name, 0660);
    }
    for (n = 0; n < matrix_rows; n++) {
	offset =
	    ((off_t) row++ * target_window.cols +
	     col) * Rast_cell_size(map_type);
	lseek(temp_fd, offset, SEEK_SET);

	if (write(temp_fd, cell_buf[n], Rast_cell_size(map_type) * matrix_cols)
	    != Rast_cell_size(map_type) * matrix_cols) {
	    unlink(temp_name);
	    G_fatal_error(_("Error while writing to temp file"));
	}
	/*Rast_put_map_row_random (outfd, cell_buf[n], row++, col, matrix_cols); */
    }
    select_current_env();

    return 0;
}

int write_map(char *name)
{
    int fd, row;
    void *rast;

    Rast_set_window(&target_window);

    rast = Rast_allocate_buf(map_type);
    close(temp_fd);
    temp_fd = open(temp_name, 0);
    fd = Rast_open_raster_new(name, map_type);

    if (fd <= 0)
	G_fatal_error(_("Unable to create raster map <%s>"), name);

    for (row = 0; row < target_window.rows; row++) {
	if (read(temp_fd, rast, target_window.cols * Rast_cell_size(map_type))
	    != target_window.cols * Rast_cell_size(map_type))
	    G_fatal_error(_("Error writing row %d"), row);
	if (Rast_put_raster_row(fd, rast, map_type) < 0) {
	    G_fatal_error(_("Failed writing raster map <%s> row %d"),
			  name, row);
	    unlink(temp_name);
	}
    }
    close(temp_fd);
    unlink(temp_name);
    Rast_close(fd);

    return 0;
}
