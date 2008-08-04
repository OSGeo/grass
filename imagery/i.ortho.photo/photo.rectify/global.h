/* These next defines determine the size of the sub-window that will
 * be held in memory.  Larger values will require
 * more memory (but less i/o) If you increase these values, keep  in
 * mind that although you think the i/o will decrease, system paging
 * (which goes on behind the scenes) may actual increase the i/o.
 */

#include <grass/imagery.h>
#include <grass/ortholib.h>
#include "defs.h"

/* the large, the worse the results in mountaneous regions!! 128 is MAX! 
 * but: the large, the slower - wants a dynamic implementation - TODO
 * possible solution: ratio local elevation range/camera height = 0.003
 */

#define TIE_ROW_DIST 128
#define TIE_COL_DIST 128

#define NROWS 128
#define NCOLS 128

/* do not modify past this point */

#ifndef GLOBAL
#define GLOBAL extern
#endif

#define IDX int

/* activate debug in Gmakefile */
#ifdef  DEBUG3
GLOBAL FILE *Bugsr;
#endif

GLOBAL ROWCOL row_map[NROWS][NCOLS];
GLOBAL ROWCOL col_map[NROWS][NCOLS];
GLOBAL ROWCOL row_min[NROWS];
GLOBAL ROWCOL row_max[NROWS];
GLOBAL ROWCOL row_left[NROWS];
GLOBAL ROWCOL row_right[NROWS];
GLOBAL IDX row_idx[NROWS];
GLOBAL int matrix_rows, matrix_cols;

GLOBAL int temp_fd;
GLOBAL RASTER_MAP_TYPE map_type;
GLOBAL CELL **cell_buf;
GLOBAL char *temp_name;

GLOBAL int *ref_list;
GLOBAL char **new_name;


GLOBAL struct Ortho_Image_Group group;
GLOBAL struct Ortho_Photo_Points cp;
GLOBAL struct Ortho_Control_Points cpz;
GLOBAL struct Ortho_Control_Points temp_points;
GLOBAL struct Ortho_Camera_File_Ref cam_info;

GLOBAL struct Cell_head elevhd;
GLOBAL DCELL *elevbuf;
GLOBAL int elevfd;
GLOBAL char *elev_layer;
GLOBAL char *mapset_elev;


/* georef coefficients */
GLOBAL double E12[3], N12[3], Z12[3];
GLOBAL double E21[3], N21[3], Z21[3];
GLOBAL double E12a, E12b, E12c, N12a, N12b, N12c;
GLOBAL double E21a, E21b, E21c, N21a, N21b, N21c;

GLOBAL struct Cell_head target_window;

GLOBAL Tie_Point **T_Point;

#include "local_proto.h"
