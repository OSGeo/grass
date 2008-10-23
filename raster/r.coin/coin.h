
/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef __COIN_H__
#define __COIN_H__

#include <grass/gis.h>

struct stats_table
{
    long count;
    double area;
};

extern struct Cell_head window;

extern char *title1, *title2;

extern double window_cells;
extern double window_area;

extern struct stats_table *table;
extern long *catlist1, *catlist2;
extern int no_data1, no_data2;
extern int Rndex, Cndex;
extern char *dumpname;
extern char *statname;
extern FILE *dumpfile;

extern char map1name[GNAME_MAX], map2name[GNAME_MAX];
extern char *mapset1, *mapset2;
extern int ncat1, ncat2;

extern char *fill, *midline;

/* check.c */
int check_report_size(void);

/* cmd.c */
int command_version(int, char *[]);

/* format.c */
int format_double(double, char *, int);

/* inter.c */
int interactive_version(void);

/* make_coin.c */
int make_coin(void);
int collapse(long *, int);

/* print_coin.c */
int print_coin(int, int, int);

/* print_hdr.c */
int print_coin_hdr(int);

/* prnt_entry.c */
int print_entry(int, long, double);
int print_area(double);
int print_percent(double);

/* totals.c */
int row_total(int, int, long *, double *);
int col_total(int, int, long *, double *);

#endif /* __COIN_H__ */
