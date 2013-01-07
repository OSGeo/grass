#include <grass/gis.h>
#include <grass/vector.h>

struct value
{
    int cat;			/* category */
    int count1, count2;		/* Count of found values; i1: count, coor, sides; i2: sides */
    /* for sides set to 2, if more than 1 area category was found, */
    /* including no category (cat = -1)! */
    int i1, i2;			/* values; i1: query (result int), sides; i2: sides */
    double d1, d2, d3;		/* values (length, area, x/y/z, query) */
    char *str1;			/* string value (query) */
    int *qcat;			/* array query categories */
    int nqcats;			/* number of query cats */
    int aqcats;			/* number of allocated query cats */
    char null;			/* no records selected by query */
};

extern struct value *Values;

struct options
{
    char *name;
    int field;
    char *col[3];
    char *qcol;
    int type;
    int option;
    int print;			/* print only */
    int sql;			/* print only sql statements */
    int total;			/* print totals */
    int units;
    int qfield;			/* query field */
    char fs;
};

extern struct options options;

struct vstat
{
    int rcat;			/* number of categories read from map */
    int select;			/* number of categories selected from DB */
    int exist;			/* number of cats existing in selection from DB */
    int notexist;		/* number of cats not existing in selection from DB */
    int dupl;			/* number of cats with duplicate elements (currently O_COOR only) */
    int update;			/* number of updated rows */
    int error;			/* number of errors */
    int qtype;			/* C type of query column */
};

extern struct vstat vstat;

#define O_CAT		1
#define O_AREA		2
#define O_LENGTH	3
#define O_COUNT		4
#define O_COOR		5	/* Point coordinates */
#define O_QUERY		6	/* Query database records linked by another field (qfield) */
#define O_SIDES         7	/* Left and right area of boundary */
#define O_COMPACT	8	/* Compactness of an area. Circle = 1.0 */
#define O_PERIMETER	9

#define O_START        10	/* line/boundary starting point */
#define O_END          11	/* line/boundary end point */

#define O_SLOPE 	12	/* Line slope */

#define O_FD		13	/* fractal dimension */

#define O_SINUOUS       14	/* sinuousity of a line (length / <distance between end points>) */

#define O_AZIMUTH	15	/* line azimuth */

/* areas.c */
int read_areas(struct Map_info *);

/* calc.c */
double length(register int, register double *, register double *);

/* find.c */
int find_cat(int, int);

/* line.c */
int read_lines(struct Map_info *);

/* parse.c */
int parse_command_line(int, char *[]);

/* query.c */
int query(struct Map_info *);

/* report.c */
int report(void);
int print_stat(void);

/* units.c */
int conv_units(void);

/* update.c */
int update(struct Map_info *);
