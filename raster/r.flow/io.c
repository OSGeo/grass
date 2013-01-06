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
 **Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 **
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "r.flow.h"
#include "mem.h"


#define OLD 0			/* magic        */
#define NEW 1			/*              */
#define TEMP 2			/*      numbers */

/****************************** Annoyances ******************************/

static const char *tmp_name(const char *fullname)
{
    char element[1024];
    const char *mapset = G_mapset();
    const char *location = G_location_path();
    const char *el = element;

    G__temp_element(element);
    while (*fullname++ == *location++) ;
    while (*fullname++ == *mapset++) ;
    while (*fullname++ == *el++) ;

    return fullname;
}

/********************************* I/O **********************************/

static int open_existing_cell_file(char *fname, struct Cell_head *chd)
{
    const char *mapset = G_find_raster(fname, "");

    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), fname);

    if (chd)
	Rast_get_cellhd(fname, mapset, chd);

    return Rast_open_old(fname, mapset);
}

static int compare_regions(const struct Cell_head *a, const struct Cell_head *b)
{
    return (fabs(a->ew_res - b->ew_res) < 1e-6 * b->ew_res &&
	    fabs(a->ns_res - b->ns_res) < 1e-6 * b->ns_res);
}

void read_input_files(void)
{
    DCELL *barc;
    int fd, row, col;
    struct Cell_head hd;

    fd = open_existing_cell_file(parm.elevin, &hd);
    if (!compare_regions(&region, &hd))
	G_fatal_error(_("Elevation raster map resolution differs from current region resolution"));

    G_important_message(_("Reading input raster map <%s>..."), parm.elevin);
    for (row = 0; row < region.rows; row++) {
        G_percent(row, region.rows, 5);
	Rast_get_d_row(fd, el.buf[row], row);
	if (parm.seg)
	    put_row_seg(el, row);
    }
    G_percent(1, 1, 1);
    if (parm.seg)
	segment_flush(el.seg);
    Rast_close(fd);

    if (parm.aspin) {
	fd = open_existing_cell_file(parm.aspin, &hd);
	if (!compare_regions(&region, &hd))
	    G_fatal_error(_("Resolution of aspect file differs from "
			    "current region resolution"));

        G_important_message(_("Reading input raster map <%s>..."), parm.aspin);
	for (row = 0; row < region.rows; row++) {
            G_percent(row, region.rows, 5);
	    Rast_get_d_row(fd, as.buf[row], row);
	    if (parm.seg)
		put_row_seg(as, row);
	}
        G_percent(1, 1, 1);
	if (parm.seg)
	    segment_flush(as.seg);
	Rast_close(fd);
    }

    if (parm.barin) {
	G_message(_("Reading input files: barrier"));
	barc = Rast_allocate_d_buf();
	fd = open_existing_cell_file(parm.barin, &hd);

	for (row = 0; row < region.rows; row++) {
	    Rast_get_d_row(fd, barc, row);
	    for (col = 0; col < region.cols; col++) {
		BM_set(bitbar, col, row, (barc[col] != 0));
		if (parm.dsout && barc[col] != 0)
		    put(ds, row, col, -1);
	    }
	}
	Rast_close(fd);
    }
}

static int open_segment_file(const char *name, layer l, int new)
{
    int fd;
    const char *mapset;

    if (new == TEMP)
	G__temp_element(string);
    else
	sprintf(string, "cell_misc/%s", parm.elevin);

    if (new || !(mapset = G_find_file2(string, name, ""))) {
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

void open_output_files(void)
{
    if (parm.seg) {
	el.sfd = open_segment_file("elevation.seg", el, OLD);
	as.sfd = open_segment_file("aspect.seg", as, OLD);
	if (parm.dsout)
	    ds.sfd = open_segment_file(tmp_name(G_tempfile()), ds, TEMP);
    }

    if (parm.lgout)
	lgfd = Rast_open_new(parm.lgout, FCELL_TYPE);

    if (parm.flout && (Vect_open_new(&fl, parm.flout, 0) < 0))
	G_fatal_error(_("Unable to create vector map <%s>"), parm.flout);
}

void close_files(void)
{
    if (parm.seg) {
	close(el.sfd);
	close(as.sfd);
	if (parm.dsout)
	    close(ds.sfd);
    }
    /*   if (parm.lgout)
       Rast_close(lgfd); */
    if (parm.flout) {
	Vect_build(&fl);
	Vect_close(&fl);
    }
}

void write_density_file(void)
{
    const char *mapset;
    int dsfd, row, col;
    double dsmax = 0.0;
    struct Colors colors;
    CELL val1, val2;

    Rast_set_output_window(&region);

    G_message(_("Writing output raster map <%s>..."), parm.dsout);
    dsfd = Rast_open_new(parm.dsout, DCELL_TYPE);

    for (row = 0; row < region.rows; row++) {
        G_percent(row, region.rows, 5);
	Rast_put_row(dsfd, get_row(ds, row), DCELL_TYPE);
	for (col = 0; col < region.cols; col++)
	    if (ds.buf[row][col] > dsmax)
		dsmax = ds.buf[row][col];
    }
    G_percent(1, 1, 1);
    Rast_close(dsfd);

    Rast_init_colors(&colors);

    val1 = -1;
    val2 = -1;
    Rast_add_c_color_rule(&val1, 0, 0, 0, &val2, 0, 0, 0, &colors);
    val1 = 0;
    val2 = 5;
    Rast_add_c_color_rule(&val1, 255, 255, 255, &val2, 255, 255, 0, &colors);
    val1 = 5;
    val2 = 30;
    Rast_add_c_color_rule(&val1, 255, 255, 0, &val2, 0, 255, 255, &colors);
    val1 = 30;
    val2 = 100;
    Rast_add_c_color_rule(&val1, 0, 255, 255, &val2, 0, 127, 255, &colors);
    val1 = 100;
    val2 = 1000;
    Rast_add_c_color_rule(&val1, 0, 127, 255, &val2, 0, 0, 255, &colors);
    val1 = 1000;
    val2 = (CELL) dsmax;
    Rast_add_c_color_rule(&val1, 0, 0, 255, &val2, 0, 0, 0, &colors);

    if ((mapset = G_find_file("cell", parm.dsout, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), parm.dsout);

    Rast_write_colors(parm.dsout, mapset, &colors);
    Rast_free_colors(&colors);
}
