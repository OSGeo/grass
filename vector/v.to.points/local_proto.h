#ifndef __LOCAL_PROTO_H
#define GV_NODE   1
#define GV_VERTEX 2

void write_point(struct Map_info *, double, double, double,
		 int, double, dbDriver *, struct field_info *);
void write_line(struct Map_info *, struct line_pnts *, int,
		int, int, double, dbDriver *, struct field_info *);
#endif
