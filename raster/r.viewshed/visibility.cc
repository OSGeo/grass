/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *
 * Date:         july 2008 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The height of a cell is assumed to be constant, and the
 * terrain is viewed as a tesselation of flat cells.  This model is
 * suitable for high resolution rasters; it may not be accurate for
 * low resolution rasters, where it may be better to interpolate the
 * height at a point based on the neighbors, rather than assuming
 * cells are "flat".  The viewshed algorithm is efficient both in
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

#ifdef __GRASS__
extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}
#endif

#include "grid.h"
#include "visibility.h"
#include "grass.h"



/* ------------------------------------------------------------ */
/* viewpoint functions */
void print_viewpoint(Viewpoint vp)
{
    printf("vp=(%d, %d, %.1f) ", vp.row, vp.col, vp.elev);
    return;
}

/* ------------------------------------------------------------ */
void set_viewpoint_coord(Viewpoint * vp, dimensionType row, dimensionType col)
{
    assert(vp);
    vp->row = row;
    vp->col = col;
    return;
}

/* ------------------------------------------------------------ */
void set_viewpoint_elev(Viewpoint * vp, float elev)
{
    assert(vp);
    vp->elev = elev;
    return;
}

/* ------------------------------------------------------------ */
/*copy from b to a */
void copy_viewpoint(Viewpoint * a, Viewpoint b)
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
MemoryVisibilityGrid *create_inmem_visibilitygrid(GridHeader hd, Viewpoint vp){
  
  MemoryVisibilityGrid *visgrid;
#ifdef __GRASS__
  visgrid = (MemoryVisibilityGrid *) G_malloc(sizeof(MemoryVisibilityGrid));
#else
  visgrid = (MemoryVisibilityGrid *) malloc(sizeof(MemoryVisibilityGrid));
#endif
  assert(visgrid);

  
  /* create the grid  */
  visgrid->grid = create_empty_grid(); 
  assert(visgrid->grid);
  
  /* create the header */
#ifdef __GRASS__
    visgrid->grid->hd = (GridHeader *) G_malloc(sizeof(GridHeader));
#else
    visgrid->grid->hd = (GridHeader *) malloc(sizeof(GridHeader));
#endif
    assert(visgrid->grid->hd);

	/* set the header */
    copy_header(visgrid->grid->hd, hd);
	
    /* allocate the  Grid data */
    alloc_grid_data(visgrid->grid);
	
    /*allocate viewpoint */
#ifdef __GRASS__
    visgrid->vp = (Viewpoint *) G_malloc(sizeof(Viewpoint));
#else
    visgrid->vp = (Viewpoint *) malloc(sizeof(Viewpoint));
#endif
    assert(visgrid->vp);
    copy_viewpoint(visgrid->vp, vp);

    return visgrid;
}




/* ------------------------------------------------------------ */
void free_inmem_visibilitygrid(MemoryVisibilityGrid * visgrid)
{

    assert(visgrid);

	if (visgrid->grid) {
	  destroy_grid(visgrid->grid);
    }
#ifdef __GRASS__
	if (visgrid->vp) {
	  G_free(visgrid->vp);
    }
	G_free(visgrid);
	
#else
	if (visgrid->vp) {
	  free(visgrid->vp);
    }
	free(visgrid);
#endif
    return;
}



/* ------------------------------------------------------------ */
/*set all values of visgrid's Grid to the given value*/
void set_inmem_visibilitygrid(MemoryVisibilityGrid * visgrid, float val)
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
/*set the (i,j) value of visgrid's Grid to the given value*/
void add_result_to_inmem_visibilitygrid(MemoryVisibilityGrid * visgrid, 
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
int is_visible(float x) {
  /* if GRASS is on, we cannot guarantee that NODATA is negative; so
	 we need to check */
#ifdef __GRASS__
  int isnull = G_is_f_null_value(&x);
  if (isnull) return 0; 
  else return (x >= 0); 
#else
  return (x >=0);
#endif
}
int is_invisible_not_nodata(float x) {

  return ( (int)x == (int)INVISIBLE);
}

int is_invisible_nodata(float x) {

  return (!is_visible(x)) && (!is_invisible_not_nodata(x));
}

/* ------------------------------------------------------------ */
/* This function is called when the program runs in
   viewOptions.outputMode == OUTPUT_BOOL. */
float booleanVisibilityOutput(float x) {
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
float angleVisibilityOutput(float x) {
  
  return x; 
}






/* ------------------------------------------------------------ */
/* visgrid is the structure that records the visibility information
   after the sweep is done.  Use it to write the visibility output
   grid and then distroy it.
*/
void save_inmem_visibilitygrid(MemoryVisibilityGrid * visgrid,
							   ViewOptions viewOptions, Viewpoint vp) {
 
#ifdef __GRASS__ 
  if (viewOptions.outputMode == OUTPUT_BOOL) 
    save_grid_to_GRASS(visgrid->grid, viewOptions.outputfname, CELL_TYPE, 
					   booleanVisibilityOutput);
  else if (viewOptions.outputMode == OUTPUT_ANGLE) 
	save_grid_to_GRASS(visgrid->grid, viewOptions.outputfname, FCELL_TYPE,
					   angleVisibilityOutput);
  else 
	/* elevation  output */
  	save_vis_elev_to_GRASS(visgrid->grid, viewOptions.inputfname, 
						   viewOptions.outputfname, 
						   vp.elev + viewOptions.obsElev);
#else
  if (viewOptions.outputMode == OUTPUT_BOOL) 
    save_grid_to_arcascii_file(visgrid->grid, viewOptions.outputfname, 
							   booleanVisibilityOutput);
  else if (viewOptions.outputMode == OUTPUT_ANGLE) 
	save_grid_to_arcascii_file(visgrid->grid, viewOptions.outputfname, 
							   angleVisibilityOutput);
  else {
	/* elevation  output */
	printf("Elevation output not implemented in the standalone version."); 
	printf("Output in angle mode\n"); 
  	save_grid_to_arcascii_file(visgrid->grid, viewOptions.outputfname, 
							   angleVisibilityOutput);
	/* if you want elevation output, use grass */
  }
#endif
  
  free_inmem_visibilitygrid(visgrid); 

  return;
}





/* ------------------------------------------------------------ */
/* IOVisibilityGrid functions */
/* ------------------------------------------------------------ */



/* ------------------------------------------------------------ */
/*create grid from given header and viewpoint */
IOVisibilityGrid *init_io_visibilitygrid(GridHeader hd, Viewpoint vp)
{
    IOVisibilityGrid *visgrid;

#ifdef __GRASS__
    visgrid = (IOVisibilityGrid *) G_malloc(sizeof(IOVisibilityGrid));
#else
    visgrid = (IOVisibilityGrid *) malloc(sizeof(IOVisibilityGrid));
#endif
    assert(visgrid);

    /*header */
#ifdef __GRASS__
    visgrid->hd = (GridHeader *) G_malloc(sizeof(GridHeader));
#else
    visgrid->hd = (GridHeader *) malloc(sizeof(GridHeader));
#endif
    assert(visgrid->hd);
    copy_header(visgrid->hd, hd);

    /*viewpoint */
#ifdef __GRASS__
    visgrid->vp = (Viewpoint *) G_malloc(sizeof(Viewpoint));
#else
    visgrid->vp = (Viewpoint *) malloc(sizeof(Viewpoint));
#endif
    assert(visgrid->vp);
    copy_viewpoint(visgrid->vp, vp);

    /*stream */
    visgrid->visStr = new AMI_STREAM < VisCell > ();
    assert(visgrid->visStr);

    return visgrid;
}



/* ------------------------------------------------------------ */
/*free the grid */
void free_io_visibilitygrid(IOVisibilityGrid * grid)
{
    assert(grid);
#ifdef __GRASS__
    if (grid->hd)
	G_free(grid->hd);
    if (grid->vp)
	G_free(grid->vp);
    if (grid->visStr)
	delete grid->visStr;

    G_free(grid);
#else
    if (grid->hd)
	free(grid->hd);
    if (grid->vp)
	free(grid->vp);
    if (grid->visStr)
	delete grid->visStr;

    free(grid);
#endif
    return;
}






/* ------------------------------------------------------------ */
/*write cell to stream */
void add_result_to_io_visibilitygrid(IOVisibilityGrid * visgrid, VisCell * cell)
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
int IJCompare::compare(const VisCell & a, const VisCell & b)
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
void sort_io_visibilitygrid(IOVisibilityGrid * visGrid)
{

    assert(visGrid);
    assert(visGrid->visStr);
    if (visGrid->visStr->stream_len() == 0)
	return;

    AMI_STREAM < VisCell > *sortedStr;
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
void
save_io_visibilitygrid(IOVisibilityGrid * visgrid, 
					   ViewOptions viewOptions, Viewpoint vp) {

#ifdef __GRASS__ 
  if (viewOptions.outputMode == OUTPUT_BOOL)
    save_io_visibilitygrid_to_GRASS(visgrid, viewOptions.outputfname, 
									CELL_TYPE, booleanVisibilityOutput); 
  
  else if (viewOptions.outputMode == OUTPUT_ANGLE) 
	save_io_visibilitygrid_to_GRASS(visgrid, viewOptions.outputfname, 
									FCELL_TYPE, angleVisibilityOutput);
  else 
	/* elevation  output */
  	save_io_vis_and_elev_to_GRASS(visgrid, viewOptions.inputfname, 
								  viewOptions.outputfname, 
								  vp.elev + viewOptions.obsElev);
#else
  if (viewOptions.outputMode == OUTPUT_BOOL) 
    save_io_visibilitygrid_to_arcascii(visgrid, viewOptions.outputfname, 
									   booleanVisibilityOutput);
  else if (viewOptions.outputMode == OUTPUT_ANGLE) 
	save_io_visibilitygrid_to_arcascii(visgrid, viewOptions.outputfname, 
									   angleVisibilityOutput);
  else {
	/* elevation  output */
	printf("Elevation output not implemented in the standalone version."); 
	printf("Output in angle mode\n"); 
  	save_io_visibilitygrid_to_arcascii(visgrid, viewOptions.outputfname, 
											angleVisibilityOutput);
	/* if you want elevation output, use grass */
  }
#endif

  /*free visibiliyty grid */
  free_io_visibilitygrid(visgrid);
  
  return;
}




/* ------------------------------------------------------------ */
/*write visibility grid to arcascii file. assume all cells that are
  not in stream are NOT visible. assume stream is sorted in (i,j)
  order. for each value x it writes to grass fun(x) */
void
save_io_visibilitygrid_to_arcascii(IOVisibilityGrid * visgrid, 
								   char* outputfname,  float(*fun)(float)) {
  
  assert(visgrid && outputfname);
  
  /*open file */
  FILE *fp = fopen(outputfname, "w");
  assert(fp);
  
  /*write header */
  print_grid_header(fp, visgrid->hd);
  
  /*sort the stream in (i,j) order */
  /*sortVisGrid(visgrid); */
 
  /* set up visibility stream */
  AMI_STREAM < VisCell > *vstr = visgrid->visStr;
  off_t streamLen, counter = 0;
  streamLen = vstr->stream_len();
  vstr->seek(0);
  
  
  /*read the first element */ 
  AMI_err ae;
  VisCell *curResult = NULL;
  
  if (streamLen > 0) {
	ae = vstr->read_item(&curResult);
	assert(ae == AMI_ERROR_NO_ERROR);
	counter++;
  }
  for (dimensionType i = 0; i < visgrid->hd->nrows; i++) {
	for (dimensionType j = 0; j < visgrid->hd->ncols; j++) {
	  
	  if (curResult->row == i && curResult->col == j) {
		/* this cell is present in the visibility stream; it must
		   be either visible, or NODATA */
		fprintf(fp, "%.1f ", fun(curResult->angle)); 
		
		/*read next element of stream */
		if (counter < streamLen) {
		  ae = vstr->read_item(&curResult);
		  assert(ae == AMI_ERROR_NO_ERROR);
		  counter++;
		}
	  } 
	  else {
		/*  this cell is not in stream, then it is  invisible */
		fprintf(fp, "%.1f ", fun(INVISIBLE)); 
	  }
	} /* for j */
	fprintf(fp, "\n");
  } /* for i */
  
	/* assert that we read teh entire file, otherwise something is
	   wrong */
  assert(counter == streamLen);
  fclose(fp);
  return;
}


