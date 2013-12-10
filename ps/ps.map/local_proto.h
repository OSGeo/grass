#include <grass/raster.h>

#include "group.h"
#include "ps_info.h"
#include "decorate.h"

/* box.c */
int box_path(double, double, double, double);
int box_clip(double, double, double, double);
int box_fill(double, double, double, double, int);
int box_draw(double, double, double, double);

/* catval.c */
#if defined GRASS_VECTOR_H && defined GRASS_DBMI_H
int load_catval_array_rgb(struct Map_info *, int, dbCatValArray *);
int load_catval_array_rot(struct Map_info *, int, dbCatValArray *);
int load_catval_array_size(struct Map_info *, int, dbCatValArray *);
#endif
/* chk_scale.c */
int check_scale(char *);

/* comment.c */
int read_comment(char *);
int do_comment(void);

/* distance.c */
double distance(double, double);

/* do_grid.c */
int do_grid(void);
int do_grid_cross(void);
int do_grid_numbers(void);

/* do_geogrid */
int do_geogrid(void);
int do_geogrid_numbers(void);

/* do_header.c */
int do_map_header(const char *);
int read_header_file(const char *);

/* do_labels.c */
int do_labels(int);

int do_label(FILE *, int);

/* do_masking.c */
int do_masking(void);

/* do_plt.c */
int do_plt(int);

/* do_psfiles.c */
int do_psfiles(void);

/* do_scalebar */
int do_scalebar(void);

/* do_vectors.c */
int do_vectors(int);
int do_vpoints(int);

/* error.c */
int error(const char *, const char *, const char *);

/* fit_map.c */
int fit_map_to_box(void);

/* get_font.c */
int get_font(char *);

/* getgrid.c */
int getgrid(void);
int getgeogrid(void);

/* get_scalebar.c */
int read_scalebar(void);

/* gprims.c */
int draw_line(double, double, double, double);
int start_line(double, double);
int move_local(int, int);
int cont_local(int, int);
int set_line_width(double);
int set_font_name(char *);
int set_font_size(int);
int show_text(double, double, char *);

/* input.c */
int input(int, char *, char *[]);
int gobble_input(void);

/* key_data.c */
int key_data(char *, char **, char **);

/* main.c */
int usage(int);

/* makeprocs.c */
int make_procs(void);

/* map_info.c */
int map_info(void);

/* map_setup.c */
int map_setup(void);

/* mtextbox.c */
int multi_text_box_path(double, double, int, int, char *, int, float);
int multi_lines(char *);

/* outl_io.c */
int o_io_init(void);
int o_read_row(void *);

#ifdef GRASS_RASTER_H
RASTER_MAP_TYPE o_open_file(char *);
#endif
int o_close_file(void);
char *xmalloc(int, char *);
int xfree(char *, char *);
char *xrealloc(char *, int, char *);

#ifdef GRASS_GIS_H
/* parse_list.c */
int parse_val_list(char *, DCELL **);
#endif
/* ps_clrtbl.c */
int PS_colortable(void);

/* ps_fclrtbl.c */
int PS_fcolortable(void);

/* ps_colors.c */
int get_color_number(char *);
int get_color_rgb(int, float *, float *, float *);
int color_name_is_ok(char *);
char *get_color_name(int);
int set_rgb_color(int);

#ifdef PS_COLOR_H
void unset_color(PSCOLOR *);
void set_color(PSCOLOR *, int, int, int);
void set_color_from_color(PSCOLOR *, int);
int set_ps_color(PSCOLOR *);
int color_none(PSCOLOR *);
#endif
/* ps_header.c */
int write_PS_header(void);
int write_bounding_box(void);

/* ps_map.c */
int ps_map(void);

/* ps_outline.c */
int ps_outline(void);
int read_outline(void);
int draw_outline(void);
int o_alloc_bufs(int, int);
int draw_top(void);
int draw_rite(void);
int draw_left(void);
int draw_bot(void);

/* ps_raster.c */
int PS_make_mask(void);
int PS_raster_plot(void);

#ifdef GRASS_GIS_H
int ps_write_mask_row(register CELL *);
#endif

/* ps_vlegend.c */
int PS_vlegend(void);

#ifdef GRASS_VECTOR_H
/* ps_vpoints.c */
int PS_vpoints_plot(struct Map_info *, int);

/* ps_vlines.c */
int PS_vlines_plot(struct Map_info *, int, int);

/* ps_vareas.c */
int PS_vareas_plot(struct Map_info *, int);

/* vect.c */
int adjust_line(struct line_pnts *);
int construct_path(struct line_pnts *, double, int);
#endif

/* Read definition from script file and store for later use */
/* r_cell.c */
int read_cell(char *, char *);

/* read_cfg.c */
int set_paper(char *);
void reset_map_location(void);
void print_papers(void);

/* r_border.c */
int read_border(void);

/* r_colortable.c */
int read_colortable(void);

/* r_group.c */
int read_group(void);

/* r_header.c */
int read_header(void);

/* r_info.c */
int read_info(void);

/* r_instructions.c */
void read_instructions(int, int);

/* r_labels.c */
int read_labels(char *, char *);

/* r_paper.c */
int read_paper(void);

/* r_plt.c */
int read_point(double, double);
int read_line(double, double, double, double);
int read_rectangle(double, double, double, double);
int read_eps(double, double);
int add_to_plfile(char *);

/* r_rgb.c */
int read_rgb(char *key, char *data);

/* r_text.c */
int read_text(char *, char *, char *);

/* r_vareas.c */
int read_vareas(char *, char *);

/* r_vlegend.c */
int read_vlegend(void);

/* r_vlines.c */
int read_vlines(char *, char *);

/* r_vpoints.c */
int read_vpoints(char *, char *);

/* r_wind.c */
int read_wind(char *, char *);


/* scale.c */
double scale(char *);

/* scan_gis.c */
int scan_gis(char *, char *, char *, char *, char *, char *, int);

/* scan_misc.c */
int scan_easting(char *, double *);
int scan_northing(char *, double *);
int scan_resolution(char *, double *);

/* scan_ref.c */
int scan_ref(char *, int *, int *);
int lowercase(register char *);

/* session.c */
int add_to_session(int, char *);
int accept(void);
int reject(void);

int print_session(FILE *);

/* show_scale.c */
int show_scale(void);

/* symbol.c */
int symbol_draw(char *, double, double, double, double, double);

#ifdef GRASS_SYMBOL_H
#ifdef PS_COLOR_H
int symbol_save(SYMBOL *, PSCOLOR *, PSCOLOR *, char *);
#endif
#endif
/* textbox.c */
int text_box_path(double, double, int, int, char *, float);

/* vector.c */
void vector_init(void);
void vector_alloc(void);

/* yesno.c */
int yesno(char *, char *);

/* eps.c */
int eps_bbox(char *, double *, double *, double *, double *);
int eps_trans(double, double, double, double, double, double, double, double,
	      double *, double *);

int eps_save(FILE *, char *, char *);
int eps_draw_saved(char *, double, double, double, double);
int eps_draw(FILE *, char *, double, double, double, double);
int pat_save(FILE *, char *, char *);
