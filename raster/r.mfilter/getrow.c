#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "glob.h"
#include "local_proto.h"

int getmaprow(int fd, void *buf, int row, int len UNUSED)
{
    Rast_get_d_row(fd, (DCELL *)buf, row);
    return 1;
}

int getrow(int fd, void *buf, int row, int len)
{
    if (direction > 0) {
        if (lseek(fd, (off_t)row * len, 0) == -1) {
            int err = errno;
            G_fatal_error(_("File read/write operation failed: %s (%d)"),
                          strerror(err), err);
        }
    }
    else {
        if (lseek(fd, (off_t)(nrows - row - 1) * len, 0) == -1) {
            int err = errno;
            G_fatal_error(_("File read/write operation failed: %s (%d)"),
                          strerror(err), err);
        }
    }
    if (read(fd, (DCELL *)buf, len) != len)
        G_fatal_error(_("Error reading temporary file"));
    return 1;
}
