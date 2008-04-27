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

#ifndef GLOBAL
#define GLOBAL extern
#endif

struct stats_table
{
    long count;
    double area;
} ;

GLOBAL struct Cell_head window;

GLOBAL char *title1, *title2;

GLOBAL double window_cells;
GLOBAL double window_area;

GLOBAL struct stats_table *table;
GLOBAL long     *catlist1, *catlist2;
GLOBAL int      no_data1, no_data2; 
GLOBAL int      Rndex,Cndex;
GLOBAL char	*dumpname;
GLOBAL char     *statname;
GLOBAL FILE	*dumpfile;

GLOBAL char	map1name[GNAME_MAX], map2name[GNAME_MAX];
GLOBAL char    *mapset1,     *mapset2;
GLOBAL int      ncat1, ncat2;

GLOBAL char *fill, *midline;
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
