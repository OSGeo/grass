#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "options.h"
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/colors.h>

#include "local_proto.h"

#define CHUNK	128

static int coors_allocated = 0 ;
static double *xarray ;
static double *yarray ;

static double cur_east = 0.0 ;
static double cur_north = 0.0 ;

extern struct Cell_head window ;

int set_text_size (void)
{
    double x, y ;

    x = (D_get_d_east() - D_get_d_west()) * hsize/100.0;
    y = (D_get_d_north() - D_get_d_south()) * vsize/100.0;
    if (x < 0) x = -x;
    if (y < 0) y = -y;

    R_text_size(abs((int)x), abs((int)y)) ;
    return(0) ;
}

int do_draw (char *buf)
{
    double east, north ;

    if (!scan_en (buf, &east, &north, 1))
    {
	bad_line (buf);
	return -1;
    }

    G_plot_line (cur_east, cur_north, east, north);

    cur_east = east ;
    cur_north = north ;

    return(0) ;
}

int do_move (char *buf)
{
    double east, north ;

    if (!scan_en (buf, &east, &north, 1))
    {
	bad_line (buf);
	return -1;
    }

    cur_east = east;
    cur_north = north;

    return(0) ;
}

int do_icon (char *buf)
{
    double east, north ;
    char type ;
    double size ;
    int dsize;
    char where[100];

    if ( 3 != sscanf(buf, "%*s %c %d %[^\n]", &type, &dsize, where))
    {
	bad_line (buf);
        return(-1) ;
    }
    if (!scan_en(where, &east, &north, 0))
    {
	bad_line (buf);
	return -1;
    }

    size = D_d_to_u_col (0.0) - D_d_to_u_col ((double) dsize);
    if (size < 0.0) size = -size;

    switch (type & 0177)
    {
    case 'o':
        G_plot_line(east-size, north-size, east-size, north+size) ;
        G_plot_line(east-size, north+size, east+size, north+size) ;
        G_plot_line(east+size, north+size, east+size, north-size) ;
        G_plot_line(east+size, north-size, east-size, north-size) ;
        break ;
    case 'x':
        G_plot_line(east-size, north-size, east+size, north+size) ;
        G_plot_line(east-size, north+size, east+size, north-size) ;
        break ;
    case '+':
    default:
        G_plot_line(east     , north-size, east     , north+size) ;
        G_plot_line(east-size, north     , east+size, north     ) ;
        break ;
    }
    return(0) ;
}

int do_color (char *buff)
{
    char in_color[64] ;
    int R, G, B, color = 0;

    sscanf(buff, "%*s %s", in_color);

    /* Parse and select color */
    color = G_str_to_color(in_color, &R, &G, &B);
    if(color == 0) {
	G_warning("[%s]: No such color", in_color);
	return(-1);
    }
    if(color == 1) {
	R_RGB_color(R, G, B);
    }
    if(color == 2) {    /* i.e. color=none */
	R = D_translate_color(DEFAULT_BG_COLOR);
	R_standard_color(R);
    }

    return(0);
}

int do_poly (char *buf, int len)
{
    int i,num ;
    char origcmd[64] ;
    double east, north ;
    int more;

    sscanf(buf, "%s", origcmd) ;

    num = 0 ;

    for(;;)
    {
        if (!(more = read_line(buf, len)))
            break ;

        if (! scan_en(buf, &east, &north, 0) )
            break ;

        check_alloc(num+1) ;
        xarray[num] = east;
        yarray[num] = north;
        num++ ;
    }

    if (num)
    {
        if(strcmp(origcmd, "polygon"))
	{
	    for (i = 1; i < num; i++)
		G_plot_line (xarray[i-1], yarray[i-1], xarray[i], yarray[i]);
	    G_plot_line (xarray[num-1], yarray[num-1], xarray[0], yarray[0]);
	}
        else
	    G_plot_polygon (xarray, yarray, num);
    }

    return(more) ;
}

int do_size (char *buf)
{
    if ( 2 != sscanf(buf, "%*s %lf %lf",&hsize, &vsize) )
    {
	bad_line (buf);
        return(-1) ;
    }
    
    set_text_size() ;
    return 0;
}

int do_text (char *buf)
{
    int x, y;
	int x2, y2 ;

/* skip over the word "text" */
    while (*buf && *buf != ' ' && *buf != '\t')
	buf++;

/* skip white space to get to the text */
    while (*buf == ' ' || *buf == '\t')
	buf++;

    G_plot_where_xy(cur_east, cur_north, &x, &y);
    R_move_abs(x, y);
	R_get_text_box(buf, &y2, &y, &x2, &x) ;
    R_text (buf) ;
    G_plot_where_en(x, y, &cur_east, &cur_north);

    return 0;
}

int check_alloc (int num)
{
    int to_alloc ;

    if (num < coors_allocated)
        return 1;
    
    to_alloc = coors_allocated ;
    while (num >= to_alloc)
        to_alloc += CHUNK ;

    if (coors_allocated == 0)
    {
        xarray = (double *) G_malloc(to_alloc * sizeof(double)) ;
        yarray = (double *) G_malloc(to_alloc * sizeof(double)) ;
    }
    else
    {
        xarray = (double *)G_realloc(xarray, to_alloc * sizeof(double));
        yarray = (double *)G_realloc(yarray, to_alloc * sizeof(double));
    }

    coors_allocated = to_alloc ;

    return 0;
}

int scan_en (char *buf, double *east, double *north, int skip)
{
    char ebuf[100], nbuf[100];

    if ( 2 != sscanf(buf, skip ? "%*s %s %s" : "%s %s", ebuf, nbuf) )
	return 0;

    if (!G_scan_easting (ebuf, east, window.proj))
	return 0;
    if (!G_scan_northing (nbuf, north, window.proj))
	return 0;

    return 1;
}
