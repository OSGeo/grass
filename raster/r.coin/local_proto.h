
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

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* check.c */
int check_report_size(void);

/* cmd.c */
int command_version(int, char *[]);

/* format.c */
__END_DECLS format_double(double, char *, int);

/* inter.c */
__END_DECLS interactive_version(void);

/* main.c */
int main(int, char *[]);

/* make_coin.c */
int make_coin(int);
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

#endif /* __LOCAL_PROTO_H__ */
