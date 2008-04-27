#ifndef GLOBAL
# define GLOBAL extern
# define INIT(x)
#else
# define INIT(x) = x
#endif

#include <grass/gis.h>
#include <grass/glocale.h>

GLOBAL struct Cell_head window ;

#define LAYER struct _layer_
GLOBAL LAYER
{
    char *name;
    char *mapset;
    struct Categories labels;
    int nlen;               /* num chars of largest cat when printed */
    int clen;               /* num chars for cat label when printed */
} *layers INIT(NULL);
GLOBAL int nlayers INIT(0);

#define GSTATS struct _gstats_
GLOBAL GSTATS
{
    CELL *cats;
    double area;
    long count;
} *Gstats INIT(NULL);
GLOBAL int nstats INIT(0);

#define MAX_UNITS 10
#define UNITS struct _units_
GLOBAL UNITS
{
    double factor;
    int type;
    int len;
    int dp;
    int eformat;
    char *label[2];
}unit[MAX_UNITS];
GLOBAL int nunits INIT(0);

#define DEFAULT_PAGE_LENGTH 0
#define DEFAULT_PAGE_WIDTH  79
GLOBAL int page_width INIT(DEFAULT_PAGE_WIDTH);
GLOBAL int page_length INIT(DEFAULT_PAGE_LENGTH);
GLOBAL int masking INIT(1);
GLOBAL int use_formfeed INIT(0);
GLOBAL int nlines INIT(0);
GLOBAL int with_headers INIT(1);
GLOBAL int verbose INIT(1);
GLOBAL int e_format INIT(0);
GLOBAL int no_nulls INIT(0);
GLOBAL int no_nulls_all INIT(0);

GLOBAL char *stats_file;
GLOBAL char *no_data_str;
GLOBAL int stats_flag INIT(0);
GLOBAL int nsteps, cat_ranges, as_int;
GLOBAL int *is_fp INIT(NULL);
GLOBAL DCELL *DMAX INIT(NULL), *DMIN INIT(NULL);

GLOBAL int maskfd;
GLOBAL CELL *mask;
GLOBAL CELL NULL_CELL;
GLOBAL int (*get_row)();
/* format.c */
int format_parms(double, int *, int *, int *, int);
int scient_format(double, char *, int, int);
int format_double(double, char *, int, int);
/* header.c */
int header(int, int);
int divider(char *);
int trailer(void);
int newline(void);
int lcr(char *, char *, char *, char *, int);
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

GLOBAL char fs[2];
GLOBAL struct Categories *labels INIT(NULL) ;
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
