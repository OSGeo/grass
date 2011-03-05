#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

static int blank_line();

static char *error_prefix;
static int first_read, last_read;
static char cell_name[256];
static int in_file_d;
static int raster_size, row_length, row_count, n_rows;
static RASTER_MAP_TYPE map_type;


int o_io_init(void)
{
    error_prefix = "ps.map";
    n_rows = PS.w.rows;
    row_length = PS.w.cols;

    return 0;
}

int o_read_row(void *buf)
{
    void *ptr;

    ptr = buf;
    if (last_read)
	return (0);
    if (first_read) {
	blank_line(buf);
	first_read = 0;
    }
    else {
	if (row_count >= n_rows) {
	    last_read = 1;
	    blank_line(buf);
	}
	else {
	    Rast_set_null_value(ptr, 1, map_type);
	    ptr = G_incr_void_ptr(ptr, raster_size);

	    Rast_get_row(in_file_d, ptr, row_count++, map_type);

	    ptr = G_incr_void_ptr(ptr, raster_size * (row_length + 1));
	    Rast_set_null_value(ptr, 1, map_type);
	}
    }
    return (row_length + 2);
}

static int blank_line(void *buf)
{
    Rast_set_null_value(buf, row_length + 2, map_type);

    return 0;
}

RASTER_MAP_TYPE o_open_file(char *cell)
{
    /* open raster map */
    sscanf(cell, "%s", cell_name);
    in_file_d = Rast_open_old(cell_name, "");

    map_type = Rast_get_map_type(in_file_d);
    raster_size = Rast_cell_size(map_type);
    first_read = 1;
    last_read = 0;
    row_count = 0;
    o_alloc_bufs(row_length + 2, raster_size);
    return (map_type);
}

int o_close_file(void)
{
    Rast_close(in_file_d);

    return 0;
}

#ifdef DEBUG
char *xmalloc(int size, char *label)
{
    char *addr, *G_malloc();

    addr = G_malloc(size);
    fprintf(stdout, "MALLOC:   %8d   %7d          %s\n", addr, size, label);
    return (addr);
}

int xfree(char *addr, char *label)
{
    fprintf(stdout, "FREE:     %8d                %s\n", addr, label);
    G_free(addr);

    return 0;
}

char *xrealloc(char *addr, int size, char *label)
{
    char *addr2, *G_realloc();

    addr2 = G_realloc(addr, size);
    fprintf(stdout, "REALLOC:  %8d   %7d  (%8d)   %s\n", addr2, size, addr,
	    label);
    return (addr2);
}
#endif
