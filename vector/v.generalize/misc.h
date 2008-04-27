#ifndef MISC_H_
#define MISC_H_

#include <grass/Vect.h>

/* returns bitmask for all the types specified in type_opt
 * e.g GV_LINE | GV_BOUNDARY
 */
extern int type_mask(struct Option *type_opt);

/* returns the squared distance and the index of the point furthest
 * from the line segment Points[a], Points[b] such that the
 * index of this points is in [a,b]
 */
extern int get_furthest(struct line_pnts *Points, int a, int b, int with_z,
			double *dist);

/* copy attributes of In which appear in Out */
/* returns 1 on success, 0 on failure */
extern int copy_tables_by_cats(struct Map_info *In, struct Map_info *Out);

#endif
