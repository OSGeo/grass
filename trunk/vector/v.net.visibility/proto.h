
/****************************************************************
 * MODULE:     v.path.obstacles
 *
 * AUTHOR(S):  Maximilian Maldacker
 *  
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#ifndef PROTO_H
#define PROTO_H

void load_lines(struct Map_info *map, struct Point **points, int *num_points,
		struct Line **lines, int *num_lines);
void count(struct Map_info *map, int *num_points, int *num_lines);
void process_point(struct line_pnts *sites, struct Point **points,
		   int *index_point, int cat);
void process_line(struct line_pnts *sites, struct Point **points,
		  int *index_point, struct Line **lines, int *index_line,
		  int cat);
void process_boundary(struct line_pnts *sites, struct Point **points,
		      int *index_point, struct Line **lines, int *index_line,
		      int cat);
void add_points(char **coor, struct Point **points, int *num_points);
int count_new(char **coor);


#endif
