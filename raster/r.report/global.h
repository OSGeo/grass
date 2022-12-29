#ifndef GLOBAL
# define GLOBAL extern
# define INIT(x)
#else
# define INIT(x) = x
#endif

#include <grass/raster.h>

#define SORT_DEFAULT 0
#define SORT_ASC     1
#define SORT_DESC    2

extern struct Cell_head window;

struct _layer_
{
    const char *name;
    const char *mapset;
    struct Categories labels;
    int nlen;			/* num chars of largest cat when printed */
    int clen;			/* num chars for cat label when printed */
};
#define LAYER struct _layer_
extern LAYER *layers;
extern int nlayers;

struct _gstats_
{
    CELL *cats;
    double area;
    long count;
};
#define GSTATS struct _gstats_
extern GSTATS *Gstats;
extern int nstats;

#define MAX_UNITS 10
struct _units_
{
    double factor;
    int type;
    int len;
    int dp;
    int eformat;
    char *label[2];
};
#define UNITS struct _units_
extern UNITS unit[MAX_UNITS];
extern int nunits;

#define DEFAULT_PAGE_LENGTH "0"
#define DEFAULT_PAGE_WIDTH  "79"

extern int page_width;
extern int page_length;
extern int masking;
extern int use_formfeed;
extern int nlines;
extern int with_headers;
extern int e_format;
extern int no_nulls;
extern int no_nulls_all;
extern int do_sort;

extern char *stats_file;
extern char *no_data_str;
extern int stats_flag;
extern int nsteps, cat_ranges, as_int;
extern int *is_fp;
extern DCELL *DMAX, *DMIN;

extern int maskfd;
extern CELL *mask;
extern CELL NULL_CELL;
extern int (*get_row)();

extern char fs[2];
extern struct Categories *labels;

/* format.c */
int format_parms(double, int *, int *, int *, int);
int scient_format(double, char *, int, int);
int format_double(double, char *, int, int);

/* header.c */
int header(int, int);
int divider(char *);
int trailer(void);
int newline(void);
int lcr(const char *, const char *, const char *, char *, int);

/* label.c */
char *print_label(char *, int, int, int, int);

/* main.c */
int main(int, char *[]);

/* maskinfo.c */
char *maskinfo(void);

/* parse.c */
int parse_command_line(int, char *[]);
int parse_units(char *);
int parse_layer(char *);
int match(char *, char *, int);

/* prt_report.c */
int print_report(int, int);
int construct_val_str(int, CELL *, char *);
char *construct_cat_label(int, CELL);

/* prt_unit.c */
int print_unit(int, int, int);

/* report.c */
int report(void);

/* stats.c */
int get_stats(void);

/* sums.c */
double area_sum(int *, int);
long count_sum(int *, int);
int same_cats(int, int, int);

#define EVERYTHING 0
#define REPORT_ONLY 1
#define STATS_ONLY 2

#define ACRES		1
#define HECTARES	2
#define SQ_MILES	3
#define PERCENT_COVER	4
#define CELL_COUNTS	5
#define SQ_METERS	6
#define SQ_KILOMETERS	7
