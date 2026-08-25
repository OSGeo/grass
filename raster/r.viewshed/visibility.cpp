/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *               Markus Metz: surface interpolation
 *
 * Date:         april 2011
 *
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The terrain is NOT viewed as a tessellation of flat cells,
 * i.e. if the line-of-sight does not pass through the cell center,
 * elevation is determined using bilinear interpolation.
 * The viewshed algorithm is efficient both in
 * terms of CPU operations and I/O operations. It has worst-case
 * complexity O(n lg n) in the RAM model and O(sort(n)) in the
 * I/O-model.  For the algorithm and all the other details see the
 * paper: "Computing Visibility on * Terrains in External Memory" by
 * Herman Haverkort, Laura Toma and Yi Zhuang.
 *
 * COPYRIGHT: (C) 2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/
#include <stdio.h>

extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "grid.h"
#include "visibility.h"
#include "grass.h"

/* ------------------------------------------------------------ */
/* viewpoint functions */
void print_viewpoint(Viewpoint vp)
{
    G_debug(3, "vp=(%d, %d, %.1f) ", vp.row, vp.col, vp.elev);
    return;
}

/* ------------------------------------------------------------ */
void set_viewpoint_coord(Viewpoint *vp, dimensionType row, dimensionType col)
{
    assert(vp);
    vp->row = row;
    vp->col = col;
    return;
}

/* ------------------------------------------------------------ */
void set_viewpoint_elev(Viewpoint *vp, float elev)
{
    assert(vp);
    vp->elev = elev;
    return;
}

/* ------------------------------------------------------------ */
/*copy from b to a */
void copy_viewpoint(Viewpoint *a, Viewpoint b)
{
    assert(a);
    a->row = b.row;
    a->col = b.col;
    a->elev = b.elev;
    return;
}

/* ------------------------------------------------------------ */
/* MemoryVisibilityGrid functions */

/* create and return a grid of the sizes specified in the header */
MemoryVisibilityGrid *create_inmem_visibilitygrid(const GridHeader &hd,
                                                  Viewpoint vp)
{

    MemoryVisibilityGrid *visgrid;

    visgrid = (MemoryVisibilityGrid *)G_malloc(sizeof(MemoryVisibilityGrid));

    assert(visgrid);

    /* create the grid  */
    visgrid->grid = create_empty_grid();
    assert(visgrid->grid);

    /* create the header */
    visgrid->grid->hd = (GridHeader *)G_malloc(sizeof(GridHeader));

    assert(visgrid->grid->hd);

    /* set the header */
    copy_header(visgrid->grid->hd, hd);

    /* allocate the  Grid data */
    alloc_grid_data(visgrid->grid);

    /*allocate viewpoint */
    visgrid->vp = (Viewpoint *)G_malloc(sizeof(Viewpoint));

    assert(visgrid->vp);
    copy_viewpoint(visgrid->vp, vp);

    return visgrid;
}

/* ------------------------------------------------------------ */
void free_inmem_visibilitygrid(MemoryVisibilityGrid *visgrid)
{

    assert(visgrid);

    if (visgrid->grid) {
        destroy_grid(visgrid->grid);
    }
    if (visgrid->vp) {
        G_free(visgrid->vp);
    }
    G_free(visgrid);

    return;
}

/* ------------------------------------------------------------ */
/*set all values of visgrid's Grid to the given value */
void set_inmem_visibilitygrid(MemoryVisibilityGrid *visgrid, float val)
{

    assert(visgrid && visgrid->grid && visgrid->grid->hd &&
           visgrid->grid->grid_data);

    dimensionType i, j;

    for (i = 0; i < visgrid->grid->hd->nrows; i++) {
        assert(visgrid->grid->grid_data[i]);
        for (j = 0; j < visgrid->grid->hd->ncols; j++) {
            visgrid->grid->grid_data[i][j] = val;
        }
    }
    return;
}

/* ------------------------------------------------------------ */
/*set the (i,j) value of visgrid's Grid to the given value */
void add_result_to_inmem_visibilitygrid(MemoryVisibilityGrid *visgrid,
                                        dimensionType i, dimensionType j,
                                        float val)
{

    assert(visgrid && visgrid->grid && visgrid->grid->hd &&
           visgrid->grid->grid_data);
    assert(i < visgrid->grid->hd->nrows);
    assert(j < visgrid->grid->hd->ncols);
    assert(visgrid->grid->grid_data[i]);

    visgrid->grid->grid_data[i][j] = val;

    return;
}

/* ------------------------------------------------------------ */
/*  The following functions are used to convert the visibility results
   recorded during the viewshed computation into the output grid into
   tehe output required by the user.

   x is assumed to be the visibility value computed for a cell during the
   viewshed computation.

   The value computed during the viewshed is the following:

   x is NODATA if the cell is NODATA;


   x is INVISIBLE if the cell is invisible;

   x is the vertical angle of the cell wrt the viewpoint if the cell is
   visible---the angle is a value in (0,180).
 */
int is_visible(float x)
{
    /* if GRASS is on, we cannot guarantee that NODATA is negative; so
       we need to check */
    int isnull = Rast_is_null_value(&x, G_SURFACE_TYPE);

    if (isnull)
        return 0;
    else
        return (x >= 0);
}
int is_invisible_not_nodata(float x)
{

    return ((int)x == (int)INVISIBLE);
}

int is_invisible_nodata(float x)
{

    return (!is_visible(x)) && (!is_invisible_not_nodata(x));
}

/* ------------------------------------------------------------ */
/* This function is called when the program runs in
   viewOptions.outputMode == OUTPUT_BOOL. */
float booleanVisibilityOutput(float x)
{
    /* NODATA and INVISIBLE are both negative values */
    if (is_visible(x))
        return BOOL_VISIBLE;
    else
        return BOOL_INVISIBLE;
}

/* ------------------------------------------------------------ */
/* This function is called when the program runs in
   viewOptions.outputMode == OUTPUT_ANGLE. In this case x represents
   the right value.  */
float angleVisibilityOutput(float x)
{

    return x;
}

/* ------------------------------------------------------------ */
/* visgrid is the structure that records the visibility information
   after the sweep is done.  Use it to write the visibility output
   grid and then destroy it.
 */
void save_inmem_visibilitygrid(MemoryVisibilityGrid *visgrid,
                               ViewOptions viewOptions, Viewpoint vp)
{

    if (viewOptions.outputMode == OUTPUT_BOOL)
        save_grid_to_GRASS(visgrid->grid, viewOptions.outputfname, CELL_TYPE,
                           OUTPUT_BOOL);
    else if (viewOptions.outputMode == OUTPUT_ANGLE)
        save_grid_to_GRASS(visgrid->grid, viewOptions.outputfname, FCELL_TYPE,
                           OUTPUT_ANGLE);
    else
        /* elevation  output */
        save_vis_elev_to_GRASS(visgrid->grid, viewOptions.inputfname,
                               viewOptions.outputfname,
                               vp.elev + viewOptions.obsElev);

    free_inmem_visibilitygrid(visgrid);

    return;
}

/* ------------------------------------------------------------ */
/* IOVisibilityGrid functions */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*create grid from given header and viewpoint */
IOVisibilityGrid *init_io_visibilitygrid(const GridHeader &hd, Viewpoint vp)
{
    IOVisibilityGrid *visgrid;

    visgrid = (IOVisibilityGrid *)G_malloc(sizeof(IOVisibilityGrid));

    assert(visgrid);

    /*header */
    visgrid->hd = (GridHeader *)G_malloc(sizeof(GridHeader));

    assert(visgrid->hd);
    copy_header(visgrid->hd, hd);

    /*viewpoint */
    visgrid->vp = (Viewpoint *)G_malloc(sizeof(Viewpoint));

    assert(visgrid->vp);
    copy_viewpoint(visgrid->vp, vp);

    /*stream */
    visgrid->visStr = new AMI_STREAM<VisCell>();
    assert(visgrid->visStr);

    return visgrid;
}

/* ------------------------------------------------------------ */
/*free the grid */
void free_io_visibilitygrid(IOVisibilityGrid *grid)
{
    assert(grid);

    if (grid->hd)
        G_free(grid->hd);
    if (grid->vp)
        G_free(grid->vp);
    if (grid->visStr)
        delete grid->visStr;

    G_free(grid);

    return;
}

/* ------------------------------------------------------------ */
/*write cell to stream */
void add_result_to_io_visibilitygrid(IOVisibilityGrid *visgrid, VisCell *cell)
{

    assert(visgrid && cell);

    AMI_err ae;

    assert(visgrid->visStr);
    ae = visgrid->visStr->write_item(*cell);
    assert(ae == AMI_ERROR_NO_ERROR);
    return;
}

/* ------------------------------------------------------------ */
/*compare function, (i,j) grid order */
int IJCompare::compare(const VisCell &a, const VisCell &b)
{
    if (a.row > b.row)
        return 1;
    if (a.row < b.row)
        return -1;

    /*a.row==b.row */
    if (a.col > b.col)
        return 1;
    if (a.col < b.col)
        return -1;
    /*all equal */
    return 0;
}

/* ------------------------------------------------------------ */
/*sort stream in grid order */
void sort_io_visibilitygrid(IOVisibilityGrid *visGrid)
{

    assert(visGrid);
    assert(visGrid->visStr);
    if (visGrid->visStr->stream_len() == 0)
        return;

    AMI_STREAM<VisCell> *sortedStr;
    AMI_err ae;
    IJCompare cmpObj;

    ae = AMI_sort(visGrid->visStr, &sortedStr, &cmpObj, 1);
    assert(ae == AMI_ERROR_NO_ERROR);
    assert(sortedStr);
    sortedStr->seek(0);

    visGrid->visStr = sortedStr;
    return;
}

/* ------------------------------------------------------------ */
void save_io_visibilitygrid(IOVisibilityGrid *visgrid, ViewOptions viewOptions,
                            Viewpoint vp)
{

    if (viewOptions.outputMode == OUTPUT_BOOL)
        save_io_visibilitygrid_to_GRASS(visgrid, viewOptions.outputfname,
                                        CELL_TYPE, booleanVisibilityOutput,
                                        OUTPUT_BOOL);

    else if (viewOptions.outputMode == OUTPUT_ANGLE)
        save_io_visibilitygrid_to_GRASS(visgrid, viewOptions.outputfname,
                                        FCELL_TYPE, angleVisibilityOutput,
                                        OUTPUT_ANGLE);
    else
        /* elevation  output */
        save_io_vis_and_elev_to_GRASS(visgrid, viewOptions.inputfname,
                                      viewOptions.outputfname,
                                      vp.elev + viewOptions.obsElev);

    /*free visibiliyty grid */
    free_io_visibilitygrid(visgrid);

    return;
}
