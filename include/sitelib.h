#ifndef GRASS_SITELIB_H
#define GRASS_SITELIB_H
/* From src/sites/Lib */
#ifndef GRASS_SITE_H
#include <grass/site.h>
#endif
/* announce.c */
int announce(char *);

/* ask_quad.c */
int ask_quad(int *);

/* center.c */
int center(char *, int);

/* chain.c */
int chain(char *);

/* copy_sites.c */
int copy_sites(SITE_LIST *, SITE_LIST *, int);

/* copyfile.c */
int copyfile(char *, char *);

/* count_site.c */
int count_sites(SITE_LIST *, int);

/* counter.c */
int counter_reset(char *, int);
int counter(int);

/* die.c */
int die(char *);

/* eq_sites.c */
int equal_sites(SITE_LIST *, SITE_LIST *);

/* execute.c */
int execute(char *);

/* hitreturn.c */
int hitreturn(void);

/* maximum.c */
int maximum(register int *, int);

/* memcopy.c */
int memcopy(char *, char *, int);

#ifdef MENU
/* menu_hndlr.c */
int menu_handler(MENU, char *);
#endif

/* meta_reprt.c */
int meta_report(char *, char *, char *, int, int);

/* parse.c */
int parse(char *, char *[], int, char *);

/* read_sites.c */
int read_site_list(SITE_LIST *, FILE *);
int get_site_list(SITE_LIST *, char *);

/* region.c */
char *format_res(double, char *, int);
char *format_east(double, char *, int);
char *format_north(double, char *, int);
int scan_north(char *, double *);
int scan_east(char *, double *);
int scan_res(char *, double *);

#ifdef REPORT
/* report.c */
REPORT *report_open(char *);
REPORT *report_open_ref(char *, REPORT *);
int report_close(REPORT *);
int report_read(REPORT *);
int report_record(REPORT *, char *);
int report_read_record(REPORT *, char *);
int report_scan(REPORT *);
int report_matrix(REPORT *);

/* rprt_finds.c */
rprt_finds.c:int report_find_layer(REPORT *, int);
rprt_finds.c:int report_find_cat(REPORT *, int, int);
rprt_finds.c:int report_find_point(REPORT *, int);
rprt_finds.c:int report_find_data(REPORT *, int, int);

/* rprt_seeks.c */
rprt_seeks.c:int report_seek_layers(REPORT *);
rprt_seeks.c:int report_seek_points(REPORT *);
rprt_seeks.c:int report_seek_cats(REPORT *);
rprt_seeks.c:int report_seek_data(REPORT *);
#endif

/* rpt_screen.c */
int new_report_screen(void);

/* scan_int.c */
int scan_int(char *, int *);

/* scn_double.c */
int scan_double(char *, double *);

/* scopy.c */
int scopy(char *, char *, int);

/* site.c */
int add_site(SITE_LIST *, double, double, char *);
int initialize_site_list(SITE_LIST *);
int rewind_site_list(SITE_LIST *);
int next_site(SITE_LIST *, double *, double *, char **);
int free_site_list(SITE_LIST *);

/* sort_int.c */
int sort_int(int[], int, int);

/* trace.c */
int trace(int);

/* ut_to_cell.c */
float northing_to_row(double, struct Cell_head *);
float easting_to_col(double, struct Cell_head *);

/* within_wnd.c */
int within_window(double, double, struct Cell_head *);

/* write_site.c */
int put_site_list(SITE_LIST *, char *, int, int);
int write_site_list(SITE_LIST *, FILE *, int, int);

/* yes.c */
int yes(char *);

#endif
