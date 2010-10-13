/* These next defines determine the size of the sub-window that will
 * be held in memory.  Larger values will require
 * more memory (but less i/o) If you increase these values, keep  in
 * mind that although you think the i/o will decrease, system paging
 * (which goes on behind the scenes) may actual increase the i/o.
 */
#include <grass/gis.h>

#define NROWS 64
#define NCOLS 256

#include <grass/imagery.h>
#include "rowcol.h"

#define IDX int

extern ROWCOL row_map[NROWS][NCOLS];
extern ROWCOL col_map[NROWS][NCOLS];
extern ROWCOL row_min[NROWS];
extern ROWCOL row_max[NROWS];
extern ROWCOL row_left[NROWS];
extern ROWCOL row_right[NROWS];
extern IDX row_idx[NROWS];
extern int matrix_rows, matrix_cols;

extern void **cell_buf;
extern int temp_fd;
extern RASTER_MAP_TYPE map_type;
extern char *temp_name;
extern int *ref_list;
extern char **new_name;
extern struct Ref ref;

/* georef coefficients */

extern double E12[10], N12[10];
extern double E21[10], N21[10];

/* DELETED WITH CRS MODIFICATIONS
   extern double E12a, E12b, E12c, N12a, N12b, N12c;
   extern double E21a, E21b, E21c, N21a, N21b, N21c;
 */
extern struct Cell_head target_window;

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
int georef_window(struct Cell_head *, struct Cell_head *, int, double);

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
