/* These next defines determine the size of the sub-window that will
 * be held in memory.  Larger values will require
 * more memory (but less i/o) If you increase these values, keep  in
 * mind that although you think the i/o will decrease, system paging
 * (which goes on behind the scenes) may actual increase the i/o.
 */
#include <grass/gis.h>

#define NROWS 64
#define NCOLS 256

/* do not modify past this point */

#ifndef GLOBAL
#define GLOBAL extern
#endif

#include <grass/imagery.h>
#include "rowcol.h"

#define IDX int

GLOBAL ROWCOL row_map[NROWS][NCOLS] ;
GLOBAL ROWCOL col_map[NROWS][NCOLS] ;
GLOBAL ROWCOL row_min[NROWS];
GLOBAL ROWCOL row_max[NROWS];
GLOBAL ROWCOL row_left[NROWS];
GLOBAL ROWCOL row_right[NROWS];
GLOBAL IDX row_idx[NROWS];
GLOBAL int matrix_rows, matrix_cols;

GLOBAL void **cell_buf;
GLOBAL int temp_fd;
GLOBAL RASTER_MAP_TYPE map_type;
GLOBAL char *temp_name;
GLOBAL int *ref_list;
GLOBAL char **new_name;
GLOBAL struct Ref ref;

/* georef coefficients */

GLOBAL double E12[10], N12[10];
GLOBAL double E21[10], N21[10];

/* DELETED WITH CRS MODIFICATIONS
GLOBAL double E12a, E12b, E12c, N12a, N12b, N12c;
GLOBAL double E21a, E21b, E21c, N21a, N21b, N21c;
*/
GLOBAL struct Cell_head target_window;
/* cp.c */
int get_control_points(char *, int);
/* env.c */
int select_current_env(void);
int select_target_env(void);
int show_env(void);
/* exec.c */
int exec_rectify(int, char *);
/* get_wind.c */
int get_target_window(int);
int georef_window(struct Cell_head *, struct Cell_head *, int);
/* matrix.c */
int compute_georef_matrix(struct Cell_head *, struct Cell_head *, int);
/* perform.c */
int perform_georef(int, void *);
/* rectify.c */
int rectify(char *, char *, char *, int);
/* report.c */
int report(char *, char *, char *, long, long, int);
/* target.c */
int get_target(char *);
/* write.c */
int write_matrix(int, int);
int write_map(char *);
