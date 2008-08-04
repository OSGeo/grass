
/****************************************************************************
 *
 * MODULE:       r.digit
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Interactive tool used to draw and save vector features
 *               on a graphics monitor using a pointing device (mouse)
 *               and save to a raster map.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#ifndef LOCAL_PROTO_H
#define	LOCAL_PROTO_H

/* bnw_line.c */
int black_and_white_line(int, int, int, int);

/* create_map.c */
int create_map(char *, char *);

/* digitize.c */
int digitize(FILE *);

/* get_en.c */
int get_east_north(int, int, char *, char *);

/* get_point.c */
int get_point(int *, int *, char *, char *);

/* get_type.c */
int get_type(void);

/* instruct.c */
int instructions(int);
int reset_instructions(void);
int left_button(char *);
int middle_button(char *);
int right_button(char *);

/* setup_graph.c */
int move(int, int);
int cont(int, int);
int setup_graphics(void);

#ifdef GRASS_GIS_H
/* get_area.c */
int get_area(FILE *, struct Categories *);

/* get_circle.c */
int get_circle(FILE *, struct Categories *);
int draw_and_record(FILE *, int, int, int, int);

/* get_label.c */
long get_cat(char *);
char *get_label(long, struct Categories *);
int get_category(FILE *, char *, struct Categories *);

/* get_line.c */
int get_line(FILE *, struct Categories *);
#endif

#endif
