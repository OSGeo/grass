
/****************************************************************************
 *
 * MODULE:       r.buffer
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      This program creates distance zones from non-zero
 *               cells in a grid layer. Distances are specified in
 *               meters (on the command-line). Window does not have to
 *               have square cells. Works both for planimetric
 *               (UTM, State Plane) and lat-long.
 *
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* execute.c */
int execute_distance(void);

/* find_dist.c */
int begin_distance(int);
int find_distances(int);
int reset_distances(void);
int find_ll_distance_ncols(int);

/* init.c */
int init_grass(void);

/* parse_dist.c */
int parse_distances(char **, double);

/* process_at.c */
int process_at(int, int, int, int);

/* process_left.c */
int process_left(int, int, int, int);

/* process_rite.c */
int process_right(int, int, int, int);

/* process_row.c */
int process_row(int, int);

/* read_map.c */
int read_input_map(const char *, const char *, int);

/* support.c */
int make_support_files(const char *, const char *);

/* write_map.c */
int write_output_map(const char *, int);

#endif /* __LOCAL_PROTO_H__ */
