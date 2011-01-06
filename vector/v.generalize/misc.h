#ifndef MISC_H_
#define MISC_H_

#include <grass/vector.h>

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

/* parse filter option and select appropriate lines */
/* return array with selected lines or NULL */
struct varray *parse_filter_options(struct Map_info *Map, int layer, 
                      int mask_type, char *where, char *cats, int *chcat);

/* check topology corruption by boundary modification
 * return 0 on corruption, 1 if modification is ok */
int check_topo(struct Map_info *, int, struct line_pnts *, 
               struct line_pnts *, struct line_cats *);

#endif
