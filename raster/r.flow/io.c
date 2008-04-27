/*
**  Original Algorithm:    H. Mitasova, L. Mitas, J. Hofierka, M. Zlocha 
**  GRASS Implementation:  J. Caplan, M. Ruesink  1995
**
**  US Army Construction Engineering Research Lab, University of Illinois 
**
**  Copyright  J. Caplan, H. Mitasova, L. Mitas, J. Hofierka, 
**      M. Zlocha
**
**This program is free software; you can redistribute it and/or
**modify it under the terms of the GNU General Public License
**as published by the Free Software Foundation; either version 2
**of the License, or (at your option) any later version.
**
**This program is distributed in the hope that it will be useful,
**but WITHOUT ANY WARRANTY; without even the implied warranty of
**MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**GNU General Public License for more details.
**
**You should have received a copy of the GNU General Public License
**along with this program; if not, write to the Free Software
**Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
**
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/glocale.h>
#include "r.flow.h"
#include "mem.h"


#define OLD 0			/* magic	*/
#define NEW 1			/*		*/
#define TEMP 2			/*	numbers	*/

/****************************** Annoyances ******************************/

static char *
tmp_name (char *fullname)
{
    char *mapset = G_mapset();
    char *location = G_location_path();
    char element[1024];
    char *el = element;

    G__temp_element(element);
    while (*fullname++ == *location++);
    while (*fullname++ == *mapset++);
    while (*fullname++ == *el++);

    return fullname;
}

/********************************* I/O **********************************/

static int
open_existing_cell_file (char *fname, struct Cell_head *chd)
{
    char   *mapset = G_find_cell(fname, "");

    if (mapset == NULL)
        G_fatal_error(_("Raster map <%s> not found"), fname);

    if (chd && (G_get_cellhd(fname, mapset, chd) < 0))
        G_fatal_error(_("Unable to get header for %s"), fname);

    return G_open_cell_old(fname, mapset);
}

void
read_input_files (void)
{
    DCELL   *barc;
    int     fd, row, col;
    struct  Cell_head hd;

    G_message(_("Reading input files: elevation"));

    fd = open_existing_cell_file(parm.elevin, &hd);
    if (!((region.ew_res == hd.ew_res)
	  && (region.ns_res == hd.ns_res)))
	G_fatal_error(_("Elevation file's resolution differs from current region resolution"));

    for (row = 0; row < region.rows; row++)
    {
	G_get_d_raster_row(fd, el.buf[row], row);
	if (parm.seg)
	    put_row_seg(el, row);
    }
    if (parm.seg)
	segment_flush(el.seg);
    G_close_cell(fd);

    if (parm.aspin)
    {
        G_message(_("Reading input files: aspect"));
	fd = open_existing_cell_file(parm.aspin, &hd);
	if (!((region.ew_res == hd.ew_res)
	      && (region.ns_res == hd.ns_res)))
	    G_fatal_error(_("Resolution of aspect file differs from "
                            "current region resolution"));

	for (row = 0; row < region.rows; row++)
	{
	    G_get_d_raster_row(fd, as.buf[row], row);
	    if (parm.seg)
		put_row_seg(as, row);
	}
	if (parm.seg)
	    segment_flush(as.seg);
	G_close_cell(fd);
    }

    if (parm.barin)
    {
        G_message(_("Reading input files: barrier"));
	barc = G_allocate_d_raster_buf();
	fd = open_existing_cell_file(parm.barin, &hd);

	for (row = 0; row < region.rows; row++)
	{
	    G_get_d_raster_row(fd, barc, row);
	    for (col = 0; col < region.cols; col++)
	    {
		BM_set(bitbar, col, row, (barc[col] != 0));
		if (barc[col] != 0)
		    put(ds, row, col, -1);
	    }
	}
	G_close_cell(fd);
    }
}

static int
open_segment_file (char *name, layer l, int new)
{
    int fd;
    char *mapset;

    if (new == TEMP)
	G__temp_element(string);
    else
	sprintf(string, "cell_misc/%s", parm.elevin);

    if (new || !(mapset = G_find_file(string, name, "")))
    {
	if ((fd = G_open_new(string, name)) < 0)
	    G_fatal_error(_("Cannot create segment file %s"), name);

	if (segment_format(fd, region.rows + l.row_offset * 2, 
		       region.cols + l.col_offset * 2, SEGROWS, SEGCOLS,
		       sizeof(DCELL)) < 1)
	    G_fatal_error(_("Cannot format segment file %s"), name);
	close(fd);
	mapset = G_mapset();
    }

    if ((fd = G_open_update(string, name)) < 0)
	G_fatal_error(_("Cannot open segment file %s"), name);

    return fd;
}

void
open_output_files (void)
{
    if (parm.seg)
    {
	el.sfd = open_segment_file("elevation.seg", el, OLD);
	as.sfd = open_segment_file("aspect.seg", as, OLD);
	if (parm.dsout)
	    ds.sfd = open_segment_file(tmp_name(G_tempfile()), ds, TEMP);
    }

    if (parm.lgout && ((lgfd = G_open_raster_new(parm.lgout, FCELL_TYPE)) < 0))
	G_fatal_error(_("Unable to create raster map <%s>"), parm.lgout);

    if (parm.flout && (Vect_open_new(&fl, parm.flout, 0) < 0))
	G_fatal_error(_("Unable to create vector map <%s>"),	parm.flout);
}

void
close_files (void)
{
    if (parm.seg)
    {
	close(el.sfd);
	close(as.sfd);
	if (parm.dsout)
	    close(ds.sfd);
    }
 /*   if (parm.lgout)
	G_close_cell(lgfd);*/
    if (parm.flout){
	Vect_build (&fl, stderr);
	Vect_close(&fl);
    }
}

void
write_density_file (void)
{
    char   *mapset;
    int     dsfd, row, col;
    double  dsmax = 0.0;
    struct  Colors colors;

    if (G_set_window(&region) < 0)
	G_fatal_error(_("Cannot reset current region"));

    G_message(_("Writing density file"));
    dsfd = G_open_raster_new(parm.dsout, DCELL_TYPE);
    if (dsfd < 0)
	G_fatal_error(_("Unable to create raster map <%s>"),	parm.dsout);

    for (row = 0; row < region.rows; row++)
    {
	G_put_raster_row(dsfd, get_row(ds, row), DCELL_TYPE);
	for (col = 0; col < region.cols; col++)
	    if (ds.buf[row][col] > dsmax)
		dsmax = ds.buf[row][col];	
    }
    G_close_cell(dsfd);
    
    G_init_colors(&colors);
    
    G_add_color_rule(-1,   0,0,0,       -1,           0,0,0,	 &colors);
    G_add_color_rule(0,    255,255,255, 5,            255,255,0, &colors);
    G_add_color_rule(5,    255,255,0,   30,           0,255,255, &colors);
    G_add_color_rule(30,   0,255,255,   100,          0,127,255, &colors);
    G_add_color_rule(100,  0,127,255,   1000,         0,0,255,	 &colors);
    G_add_color_rule(1000, 0,0,255,     (CELL) dsmax, 0,0,0,	 &colors);
    
    if ((mapset = G_find_file("cell", parm.dsout, "")) == NULL)
	G_fatal_error(_("Unable to find file %s"), parm.dsout);

    G_write_colors(parm.dsout, mapset, &colors);
    G_free_colors(&colors);
}
