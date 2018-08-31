
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
 * terrain. The terrain is NOT viewed as a tesselation of flat cells, 
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

#ifndef visibility_h
#define visibility_h

#include <grass/config.h>
#include <grass/iostream/ami.h>


#include "grid.h"



/*  default max distance */
#define  INFINITY_DISTANCE  -1

/* File/directory name lengths for GRASS compatibility */
#define GNAME_MAX 256
#define GPATH_MAX 4096


typedef struct viewpoint_
{
    dimensionType row, col;
    float elev;
    float target_offset;
} Viewpoint;


typedef enum
{
    VISIBLE = 1,
    INVISIBLE = -1,

    /*boolean values for output */
    BOOL_VISIBLE = 1,
    BOOL_INVISIBLE = 0
} VisMode;


typedef struct visCell_
{
    dimensionType row;
    dimensionType col;
    /*   VisMode vis; */
    float angle;
} VisCell;



typedef enum outputMode_
{
    OUTPUT_ANGLE = 0,
    OUTPUT_BOOL = 1,
    OUTPUT_ELEV = 2
} OutputMode;


typedef struct viewOptions_
{

    /* the name of the input raster */
    char inputfname[GNAME_MAX];

    /* the name of the output raster */
    char outputfname[GNAME_MAX];

    float obsElev;
    /* observer elevation above the terrain */

    float tgtElev;
    /* target elevation offset above the terrain */

    float maxDist;
    /* points that are farther than this distance from the viewpoint are
       not visible  */

    OutputMode outputMode;
    /* The mode the viewshed is output; 
       - in angle mode, the values recorded are   {NODATA, INVISIBLE, angle}
       - in boolean mode, the values recorded are {BOOL_INVISIBLE, BOOL_VISIBLE}
       - in elev mode, the values recorded are    {NODATA, INVISIBLE, elevation}
     */

    int doCurv;
    /*determines if the curvature of the earth should be considered
       when calculating.  Only implemented for GRASS version. */

    int doRefr;
    double refr_coef;
    /*determines if atmospheric refraction should be considered
       when calculating.  Only implemented for GRASS version. */

    double ellps_a;		/* the parameter of the ellipsoid */
    float cellsize;		/* the cell resolution */
    char streamdir[GPATH_MAX];	/* directory for tmp files */
} ViewOptions;




/*memory visibility grid */
typedef struct memory_visibility_grid_
{
    Grid *grid;
    Viewpoint *vp;
} MemoryVisibilityGrid;


/*io-efficient visibility grid */
typedef struct IOvisibility_grid_
{
    GridHeader *hd;
    Viewpoint *vp;
      AMI_STREAM < VisCell > *visStr;
} IOVisibilityGrid;




/* ------------------------------------------------------------ */
/* visibility output functions */

/*  The following functions are used to convert the visibility results
   recorded during the viewshed computation into the output grid into
   the format required by the user.  x is assumed to be the
   visibility angle computed for a cell during the viewshed
   computation. 

   The value passed to this function is the following: x is NODATA if the
   cell is NODATA; x is INVISIBLE if the cell is invisible; x is the
   vertical angle of the cell wrt the viewpoint if the cell is
   visible---the angle is a value in (0,180).
 */
/* these functions assume that x is a value computed during the
   viewshed computation; right now x represents the vertical angle of a
   visible point wrt to the viewpoint; INVISIBLE if invisible; NODATA if
   nodata. They return true if x is visible, invisible but nodata,
   andnodata, respectively  */
int is_visible(float x);
int is_invisible_not_nodata(float x);
int is_invisible_nodata(float x);

/* This function is called when the program runs in
   viewOptions.outputMode == OUTPUT_BOOL. */
float booleanVisibilityOutput(float x);

/* This function is called when the program runs in
   viewOptions.outputMode == OUTPUT_ANGLE.   */
float angleVisibilityOutput(float x);





/* ------------------------------------------------------------ */
/* viewpoint functions */

void print_viewpoint(Viewpoint vp);

/*copy from b to a */
void copy_viewpoint(Viewpoint * a, Viewpoint b);

void
set_viewpoint_coord(Viewpoint * vp, dimensionType row, dimensionType col);

void set_viewpoint_elev(Viewpoint * vp, float elev);



/* ------------------------------------------------------------ */
/* MemoryVisibilityGrid functions */

MemoryVisibilityGrid *create_inmem_visibilitygrid(GridHeader hd,
						  Viewpoint vp);

void free_inmem_visibilitygrid(MemoryVisibilityGrid * visgrid);

void set_inmem_visibilitygrid(MemoryVisibilityGrid * visgrid, float val);

void add_result_to_inmem_visibilitygrid(MemoryVisibilityGrid * visgrid,
					dimensionType i, dimensionType j,
					float val);

void save_inmem_visibilitygrid(MemoryVisibilityGrid * vigrid,
			       ViewOptions viewopt, Viewpoint vp);


/* ------------------------------------------------------------ */
/* IOVisibilityGrid functions */

/*create grid from given header and viewpoint */
IOVisibilityGrid *init_io_visibilitygrid(GridHeader hd, Viewpoint vp);

/*frees a visibility grid */
void free_io_visibilitygrid(IOVisibilityGrid * grid);

/*write cell to stream */
void add_result_to_io_visibilitygrid(IOVisibilityGrid * visgrid,
				     VisCell * cell);

/*void
   addResult(IOVisibilityGrid* visgrid, DimensionType row, DimensionType col, 
   VisMode vis);
 */


/* write visibility grid. assume all cells that are not in stream are
   NOT visible.  assume stream is sorted.  */
void
save_io_visibilitygrid(IOVisibilityGrid * visgrid,
		       ViewOptions viewoptions, Viewpoint vp);


/*sort stream in grid (i,j) order */
void sort_io_visibilitygrid(IOVisibilityGrid * visGrid);

class IJCompare
{
  public:
    int compare(const VisCell &, const VisCell &);
};



#endif
