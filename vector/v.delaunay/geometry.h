#ifndef GEOMETRY_H
#define GEOMETRY_H

void divide(struct vertex *sites_sorted[], unsigned int l, unsigned int r,
	    struct edge **l_ccw, struct edge **r_cw);
#endif
