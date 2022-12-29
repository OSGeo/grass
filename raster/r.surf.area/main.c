/* main.c - r.surf.area */

/* Copyright Notice
 * ---------------- 
 * Written by Bill Brown, USACERL December 21, 1994 
 * Copyright 1994, Bill Brown, USACERL
 * brown@gis.uiuc.edu
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


/* Calculates area of regular 3D triangulated points 
 * (centers of cells) in current region by
 * adding areas of triangles.  Therefore, area of a flat surface 
 * will be reported as (rows + cols -1)*(area of cell) less than area of 
 * flat region due to a half row and half column missing around the 
 * perimeter.
 * NOTE:  This calculation is heavily dependent on data resolution
 *     (think of it as a fractal shoreline problem, the more resolution
 *     the more detail, the more area, etc).  This program uses the
 *     CURRENT GRASS REGION, not the resolution of the map.
 * This version actually calculates area twice for each triangle pair,
 * keeping a running minimum and maximum area depending on the
 * direction of the diagonal used.
 * Reported totals are: 
 *      1) "plan" area within calculation region (rows-1 * cols-1 * cellarea)
 *      2) avg of min & max calculated 3d triangle area within this region
 *      3) "plan" area within current GRASS region (rows * cols * cellarea)
 *      4) scaling of calculated area to current GRASS region 
 */

/* Modified by Eric G. Miller to work with FP rasters and to handle
 * NULL value cells.  I'm not too sure how bad the surface area
 * calculation will get if there are a lot of NULL cells mixed around.
 * Added function prototypes and removed a couple unnecessary typedefs
 * Added the add_null_area() function.
 * 2000-10-17
 */

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
	struct Option *surf, *vscale, *units;
    } opt;
    DCELL *cell_buf[2];
    int row;
    struct Cell_head w;
    int cellfile, units;
    double minarea, maxarea, sz, nullarea;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("area estimation"));
    module->description = _("Prints estimation of surface area for raster map.");

    opt.surf = G_define_standard_option(G_OPT_R_MAP);
    
    opt.vscale = G_define_option();
    opt.vscale->key = "vscale";
    opt.vscale->type = TYPE_DOUBLE;
    opt.vscale->required = NO;
    opt.vscale->multiple = NO;
    opt.vscale->description = _("Vertical scale");
    opt.vscale->answer = "1.0";
    
    opt.units = G_define_standard_option(G_OPT_M_UNITS);
    opt.units->label = _("Output units");
    opt.units->description = _("Default: square map units");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sz = atof(opt.vscale->answer);
    if (opt.units->answer) {
	units = G_units(opt.units->answer);
	G_verbose_message(_("Output in '%s'"), G_get_units_name(units, TRUE, TRUE));
    }
    else {
	units = U_UNDEFINED;
	G_verbose_message(_("Output in 'square map units'"));
    }
    
    G_get_set_window(&w);

    /* open raster map for reading */
    cellfile = Rast_open_old(opt.surf->answer, "");

    cell_buf[0] = (DCELL *) G_malloc(w.cols * Rast_cell_size(DCELL_TYPE));
    cell_buf[1] = (DCELL *) G_malloc(w.cols * Rast_cell_size(DCELL_TYPE));

    {
	DCELL *top, *bottom;

	minarea = maxarea = nullarea = 0.0;
	for (row = 0; row < w.rows - 1; row++) {
	    if (!row) {
		Rast_get_row(cellfile, cell_buf[1], 0, DCELL_TYPE);
		top = cell_buf[1];
	    }
	    Rast_get_row(cellfile, cell_buf[row % 2], row + 1,
			     DCELL_TYPE);
	    bottom = cell_buf[row % 2];
	    add_row_area(top, bottom, sz, &w, &minarea, &maxarea);
	    add_null_area(top, &w, &nullarea);
	    top = bottom;
	    G_percent(row, w.rows, 10);
	}

	/* Get last null row area */
	if (w.rows > 1)
	    add_null_area(top, &w, &nullarea);
    }


    G_free(cell_buf[0]);
    G_free(cell_buf[1]);
    Rast_close(cellfile);

    {	/* report */
	double reg_area, flat_area, estavg;

	flat_area = (w.cols - 1) * (w.rows - 1) * w.ns_res * w.ew_res;
	reg_area = w.cols * w.rows * w.ns_res * w.ew_res;
	estavg = (minarea + maxarea) / 2.0;

	fprintf(stdout, "%s %f\n",
		_("Null value area ignored in calculation:"),
		conv_value(nullarea, units));
	fprintf(stdout, "%s %f\n", _("Plan area used in calculation:"),
		conv_value(flat_area, units));
	fprintf(stdout, "%s\n\t%f %f %f\n",
		_("Surface area calculation(low, high, avg):"),
		conv_value(minarea, units),
		conv_value(maxarea, units),
		conv_value(estavg, units));

	fprintf(stdout, "%s %f\n", _("Current region plan area:"),
		conv_value(reg_area, units));
	fprintf(stdout, "%s %f\n", _("Estimated region Surface Area:"),
		flat_area > 0 ?
			conv_value(reg_area * estavg / flat_area, units) :
			0);
    }

    exit(EXIT_SUCCESS);
}
