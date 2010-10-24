#ifndef PROTO_H
#define PROTO_H

#define OP_EQUALS     0
#define OP_DISJOINT   1
#define OP_INTERSECTS 2
#define OP_TOUCHES    3
#define OP_CROSSES    4
#define OP_WITHIN     5
#define OP_CONTAINS   6
#define OP_OVERLAPS   7
#define OP_RELATE     8

struct GParm {
    struct Option *input[2], *output, *type[2], *field[2],
	*operator, *relate;
};
struct GFlag {
    struct Flag *table, *reverse, *geos, *cat;
};

/* args.c */
void parse_options(struct GParm *, struct GFlag *);

#ifdef HAVE_GEOS
/* geos.c */
int line_relate_geos(struct Map_info *, const GEOSGeometry *,
		     int, int, const char *);
int area_relate_geos(struct Map_info *, const GEOSGeometry *,
		     int, int, const char *);
#endif

/* overlap.c */
void add_aarea(struct Map_info *, int, int *);
int line_overlap_area(struct Map_info *, int, struct Map_info *, int);

#endif /* PROTO_H */
