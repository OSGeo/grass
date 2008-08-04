#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include "local_proto.h"

static int max(int a, int b)
{
    return a > b ? a : b;
}

int print_coor(struct Cell_head *window, double north, double east)
{
    char buffer[200];
    int len_n, len_s, len_e, len_w, t;

    len_n = len_s = len_e = len_w = 0;

    G_limit_north(&north, window->proj);
    G_limit_east(&east, window->proj);

    t = (window->north - north) / window->ns_res;
    north = window->north - (t) * window->ns_res;

    t = (window->east - east) / window->ew_res;
    east = window->east - (t) * window->ew_res;

    strcpy(buffer, "?");
    G_format_northing(north, buffer, window->proj);
    len_n = max(len_n, strlen(buffer));
    fprintf(stderr, "%-*s(N)  ", len_n, buffer);

    strcpy(buffer, "?");
    G_format_easting(east, buffer, window->proj);
    len_e = max(len_e, strlen(buffer));
    fprintf(stderr, "%-*s(E)  ", len_e, buffer);

    fprintf(stderr, "\r");
    fflush(stderr);

    return 1;
}

int print_win(struct Cell_head *window, double north, double south,
	      double east, double west)
{
    char buffer[200];
    int len_n, len_s, len_e, len_w, t;

    len_n = len_s = len_e = len_w = 0;

    G_limit_north(&north, window->proj);
    G_limit_south(&south, window->proj);
    G_limit_east(&east, window->proj);
    G_limit_west(&west, window->proj);

    t = (window->north - north) / window->ns_res;
    north = window->north - (t) * window->ns_res;

    t = (south - window->south) / window->ns_res;
    south = window->south + (t) * window->ns_res;

    t = (window->east - east) / window->ew_res;
    east = window->east - (t) * window->ew_res;

    t = (west - window->west) / window->ew_res;
    west = window->west + (t) * window->ew_res;

    strcpy(buffer, "?");
    G_format_northing(north, buffer, window->proj);
    len_n = max(len_n, strlen(buffer));
    fprintf(stderr, "north: %-*s  ", len_n, buffer);

    strcpy(buffer, "?");
    G_format_northing(south, buffer, window->proj);
    len_s = max(len_s, strlen(buffer));
    fprintf(stderr, "south: %-*s  ", len_s, buffer);

    strcpy(buffer, "?");
    G_format_easting(east, buffer, window->proj);
    len_e = max(len_e, strlen(buffer));
    fprintf(stderr, "east: %-*s  ", len_e, buffer);

    strcpy(buffer, "?");
    G_format_easting(west, buffer, window->proj);
    len_w = max(len_w, strlen(buffer));
    fprintf(stderr, "west: %-*s  ", len_w, buffer);

    fprintf(stderr, "\r");
    fflush(stderr);

    return 1;
}

int print_limit(struct Cell_head *window, struct Cell_head *defwin)
{
    char buffer[1000];
    int limit = 0;

    if (window->north > defwin->north) {
	sprintf(buffer, "North");
	limit = 1;
    }
    if (window->south < defwin->south) {
	if (limit)
	    sprintf(buffer, "%s, south", buffer);
	else
	    sprintf(buffer, "South");
	limit = 1;
    }
    if (window->east > defwin->east) {
	if (limit)
	    sprintf(buffer, "%s, east", buffer);
	else
	    sprintf(buffer, "East");
	limit = 1;
    }
    if (window->west < defwin->west) {
	if (limit)
	    sprintf(buffer, "%s, west", buffer);
	else
	    sprintf(buffer, "West");
	limit = 1;
    }
    if (limit) {
	fprintf(stderr, "%s limit of default region reached.\n", buffer);
    }

    return (limit);
}
