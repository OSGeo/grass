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
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __GRASS__
extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}
#endif

#include "grid.h"


/* ------------------------------------------------------------ */
/*read header from file and return it; */
GridHeader *read_header_from_arcascii_file(char* fname)
{

  assert(fname);
  FILE *fp;
  fp = fopen(fname, "r");
  assert(fp); 
  GridHeader  *hd = read_header_from_arcascii_file(fp); 
  fclose(fp); 
  return hd;
}



/* ------------------------------------------------------------ */
/*read header from file and return it; */
GridHeader *read_header_from_arcascii_file(FILE* fp)
{
    assert(fp);
	rewind(fp); 

    GridHeader *hd = (GridHeader *) malloc(sizeof(GridHeader));
    assert(hd);

    int nrows, ncols;

    fscanf(fp, "%*s%d\n", &ncols);
    fscanf(fp, "%*s%d\n", &nrows);
    /*check that you dont lose precision */
    if (nrows <= maxDimension && ncols <= maxDimension) {
	hd->nrows = (dimensionType) nrows;
	hd->ncols = (dimensionType) ncols;
    }
    else {
	fprintf(stderr, "grid dimension too big for current precision\n");
	printf("change type and re-compile\n");
	exit(1);
    }
    fscanf(fp, "%*s%f\n", &(hd->xllcorner));
    fscanf(fp, "%*s%f\n", &(hd->yllcorner));
    fscanf(fp, "%*s%f\n", &(hd->cellsize));
    fscanf(fp, "%*s%f\n", &(hd->nodata_value));

    return hd;
}



/* ------------------------------------------------------------ */
/*copy from b to a */
void copy_header(GridHeader * a, GridHeader b)
{
    assert(a);
    a->nrows = b.nrows;
    a->ncols = b.ncols;
    a->xllcorner = b.xllcorner;
    a->yllcorner = b.yllcorner;
    a->cellsize = b.cellsize;
    a->nodata_value = b.nodata_value;
    return;
}


/* ------------------------------------------------------------ */
/*print header */
void print_grid_header(GridHeader * hd)
{

    assert(hd);
    print_grid_header(stdout, hd);
    return;
}


/* ------------------------------------------------------------ */
void print_grid_header(FILE * fp, GridHeader * hd)
{
    assert(fp && hd);
    fprintf(fp, "ncols\t%d\n", hd->ncols);
    fprintf(fp, "nrows\t%d\n", hd->nrows);
    fprintf(fp, "xllcorner\t%f\n", hd->xllcorner);
    fprintf(fp, "yllcorner\t%f\n", hd->yllcorner);
    fprintf(fp, "cellsize\t%f\n", hd->cellsize);
    fprintf(fp, "NODATA_value\t%f\n", hd->nodata_value);
    return;
}




/* ------------------------------------------------------------ */
/*returns 1 if value is Nodata, 0 if it is not */
int is_nodata(GridHeader * hd, float value)
{
    assert(hd);
#ifdef __GRASS__
    return G_is_f_null_value(&value);
#else
    if (fabs(value - hd->nodata_value) < 0.000001) {
	return 1;
    }
    return 0;
#endif
}

/* ------------------------------------------------------------ */
/*returns 1 if value is Nodata, 0 if it is not */
int is_nodata(Grid * grid, float value)
{
    assert(grid);
	return is_nodata(grid->hd, value);
}



/* ------------------------------------------------------------ */
/* create an empty grid and return it. The header and the data are set
   to NULL.  */
Grid *create_empty_grid()
{

#ifdef __GRASS__
    Grid *ptr_grid = (Grid *) G_malloc(sizeof(Grid));
#else
    Grid *ptr_grid = (Grid *) malloc(sizeof(Grid));
#endif

    assert(ptr_grid);

    /*initialize structure */
    ptr_grid->hd = NULL;
    ptr_grid->grid_data = NULL;

#ifdef _DEBUG_ON
    printf("**DEBUG: createEmptyGrid \n");
    fflush(stdout);
#endif

    return ptr_grid;
}




/* ------------------------------------------------------------ */
/* allocate memroy for grid_data; grid must have a header that gives
   the dimensions */
void alloc_grid_data(Grid * pgrid)
{
    assert(pgrid);
    assert(pgrid->hd);

#ifdef __GRASS__
    pgrid->grid_data = (float **)G_malloc(pgrid->hd->nrows * sizeof(float *));
#else
    pgrid->grid_data = (float **)malloc(pgrid->hd->nrows * sizeof(float *));
#endif
    assert(pgrid->grid_data);

    dimensionType i;

    for (i = 0; i < pgrid->hd->nrows; i++) {
#ifdef __GRASS__
	  pgrid->grid_data[i] =
	    (float *)G_malloc(pgrid->hd->ncols * sizeof(float));
#else
	  pgrid->grid_data[i] =
	    (float *)malloc(pgrid->hd->ncols * sizeof(float));
#endif

	  assert(pgrid->grid_data[i]);
    }
	
#ifdef _DEBUG_ON
    printf("**DEBUG: allocGridData\n");
    fflush(stdout);
#endif

    return;
}

/* ------------------------------------------------------------ */
/*reads header and data from file */
Grid *read_grid_from_arcascii_file(char *filename)
{

    assert(filename);
    FILE *fp = fopen(filename, "r");

    assert(fp);

    Grid *grid = create_empty_grid();

    grid->hd = read_header_from_arcascii_file(fp);
    alloc_grid_data(grid);

    /*READ DATA */
    dimensionType i, j;
    int first_flag = 1;
    float value = 0;

    for (i = 0; i < grid->hd->nrows; i++) {
	for (j = 0; j < grid->hd->ncols; j++) {
	    fscanf(fp, "%f", &value);
	    grid->grid_data[i][j] = value;

	    if (first_flag) {
		if (is_nodata(grid, value))
		    continue;
		grid->minvalue = grid->maxvalue = value;
		first_flag = 0;
	    }
	    else {
		if (is_nodata(grid, value))
		    continue;
		if (value > grid->maxvalue)
		    grid->maxvalue = value;
		if (value < grid->minvalue)
		    grid->minvalue = value;
	    }
	}
	fscanf(fp, "\n");
    }

    fclose(fp);

#ifdef DEBUG_ON
    printf("**DEBUG: readGridFromArcasciiFile():\n");
    fflush(stdout);
#endif

    return grid;
}





/* ------------------------------------------------------------ */
/*destroy the structure and reclaim all memory allocated */
void destroy_grid(Grid * grid)
{
    assert(grid);

    /*free grid data if its allocated */
#ifdef __GRASS__
    if (grid->grid_data) {
	dimensionType i;

	for (i = 0; i < grid->hd->nrows; i++) {
	    if (!grid->grid_data[i])
		G_free((float *)grid->grid_data[i]);
	}

	G_free((float **)grid->grid_data);
    }

    G_free(grid->hd);
    G_free(grid);


#else
    if (grid->grid_data) {
	dimensionType i;

	for (i = 0; i < grid->hd->nrows; i++) {
	    if (!grid->grid_data[i])
		free((float *)grid->grid_data[i]);
	}

	free((float **)grid->grid_data);
    }

    free(grid->hd);
    free(grid);
#endif

#ifdef _DEBUG_ON
    printf("**DEBUG: grid destroyed.\n");
    fflush(stdout);
#endif

    return;
}







/* ------------------------------------------------------------ */
/*save the grid into an arcascii file.  Loops through all elements x
  in row-column order and writes fun(x) to file */
void
save_grid_to_arcascii_file(Grid * grid, char *filename,
						   float(*fun)(float)) {

  assert(filename && grid);
  printf("saving grid to %s\n", filename);
  fflush(stdout);
  
  FILE *outfile = fopen(filename, "r");
  if (outfile) {		/*outfile already exists */
	printf("The output file already exists. It will be overwritten\n");
	fclose(outfile);
	int ret = remove(filename);	/*delete the existing file */
	
	if (ret != 0) {
	  printf("unable to overwrite the existing output file.\n!");
	  exit(1);
	}
  }
  FILE *fp = fopen(filename, "a");
  assert(fp);
  
  /*print header */
  print_grid_header(fp, grid->hd);
  
  /*print data */
  dimensionType i, j;
  
  for (i = 0; i < grid->hd->nrows; i++) {
	for (j = 0; j < grid->hd->ncols; j++) {
	  
	  /*  call fun() on this element and write it to file  */
	  fprintf(fp, "%.1f ",  fun(grid->grid_data[i][j]) ); 
	  
	}
	fprintf(fp, "\n");
  }
  
#ifdef _DEBUG_ON
  printf("**DEBUG: saveGridToArcasciiFile: saved to %s\n", filename);
  fflush(stdout);
#endif
  
  return;
}
