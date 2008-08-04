#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/config.h>
#include <grass/raster.h>
#include <grass/display.h>

#ifndef GLOBAL
#define GLOBAL extern
#endif
GLOBAL char new_name[GNAME_MAX], current_name[GNAME_MAX],
    orig_name[GNAME_MAX];
GLOBAL char grid_color_name[40];
GLOBAL DCELL max_value, min_value;
GLOBAL int cellsize;
GLOBAL char user_mapset[GMAPSET_MAX], current_mapset[GMAPSET_MAX],
    orig_mapset[GMAPSET_MAX];
GLOBAL struct Cell_head real_window;
GLOBAL char *tempfile;
GLOBAL struct Categories cats;
GLOBAL struct Colors colr;
GLOBAL struct Quant quant;
GLOBAL int grid_color;
GLOBAL int real_nrows, real_ncols;
GLOBAL int colr_ok, cats_ok, quant_ok;
GLOBAL int change_made;
GLOBAL RASTER_MAP_TYPE map_type;


/* keeping names straight:

   new_name    - new name user has input for the cell
   layer to be created in their mapset
   user_mapset - mapset where "new_name" will be created.
   a.k.a. output of G_mapset command
   orig_name   - name of the original file (being edited)
   a.k.a. the layer displayed on the monitor
   when the program starts
   orig_mapset - the mapset where the "orig_name" layer
   and it's support files are found.
   current_name- when editing is taking place, a map is
   displayed on the screen. The first time
   the user is in "edit mode", current_name
   will be same as orig_name. If edit mode
   is exited and then entered again (while 
   still in Dedit), we will have written the
   changes made from the first edit to 
   "new_name" and since we want what is on
   the screen to match the current state of
   edits, we set current_name to the new_name
   and Dcell new_name.
   current_mapset - mapset that jives with where the current_name
   is hanging out.
 */

/* cell.c */
int Dcell(char *, char *, int);

/* draw_grid.c */
int draw_grid(void);

/* edit.c */
int edit(void);
int edit_mouse_info(void);
int edit_mouse_info2(DCELL, DCELL);
int use_mouse(void);

/* main.c */
int do_edit(int, int, double);
int error(int, char[128]);

#ifdef __GNUC_MINOR__
int ext(void) __attribute__ ((__noreturn__));
#else
int ext(void);
#endif
/* menu.c */
int main_menu(void);
int option_menu(void);
int color_menu(char *);
int map_type_menu(void);
int arrow_options(void);
int get_arrow_inputs(void);
int arrow_map(void);

/* mk_new_layer.c */
int make_new_cell_layer(void);

/* mk_tmp_file.c */
int make_temporary_file(void);
