#include <grass/raster.h>
#include "clr.h"

/* Font sizes */
#define PS_FONT_MAX_SIZE      50
#define PS_FONT_MIN_SIZE       1
#define PS_FONT_DEFAULT_SIZE   8

#define DELTA_Y                0.05
#define MAX_PSFILES       20

#define PAGE_PORTRAIT 1
#define PAGE_LANDSCAPE 2

/* Following XCONV, YCONV were commented because of using G_plot_where_xy()
 * and uncommented again because G_adjust_easting 
 * in it is not best for each case,  RB Jan 2000 
 */
#define XCONV(E_COORD)	(PS.map_left + PS.ew_to_x * ((E_COORD) - PS.w.west))
#define YCONV(N_COORD)	(PS.map_bot  + PS.ns_to_y * ((N_COORD) - PS.w.south))


struct PS_data
{
    struct Cell_head w;
    struct Colors colors;
    struct Categories cats;
    CELL min_color, max_color;
    const char *cell_mapset;
    char *cell_name;
    char *plfile;
    char *commentfile;
    char *grid_font, *geogrid_font;
    char *psfiles[MAX_PSFILES];
    char scaletext[100];
    char celltitle[100];
    int level;
    int grey;
    int mask_needed;
    int do_header;
    int do_raster;
    int do_colortable;
    int do_border;
    int do_scalebar;
    int num_psfiles;
    int grid, grid_numbers, grid_fontsize;
    PSCOLOR grid_color, grid_numbers_color;
    float grid_cross;
    char geogridunit[64];
    int geogrid, geogrid_numbers, geogrid_fontsize;
    PSCOLOR geogrid_color, geogrid_numbers_color;
    double grid_width, geogrid_width;
    int do_outline;
    PSCOLOR outline_color;
    int cell_fd;
    int row_delta, col_delta;
    int cells_wide, cells_high;
    int num_panels, startpanel, endpanel;
    int res;
    double page_width, page_height;
    double left_marg, right_marg, top_marg, bot_marg;
    double map_x_orig, map_y_orig, map_y_loc, min_y, set_y;
    /* map_y_loc is set by 'maploc' as distance from top
     * map_x_orig is from left, map_y_orig is from bottom */
    double map_pix_wide, map_pix_high;
    double map_width, map_height;
    double map_top, map_bot, map_left, map_right;
    double ew_res, ns_res;
    double ew_to_x, ns_to_y;
    double r0, g0, b0;
    int mask_color;
    double mask_r, mask_g, mask_b;
    double outline_width;
    FILE *fp;
};

#ifdef WHITE
#undef WHITE
#endif
#ifdef BLACK
#undef BLACK
#endif
#ifdef GREY
#undef GREY
#endif

extern struct PS_data PS;
extern int WHITE, BLACK, GREY, sec_draw;
