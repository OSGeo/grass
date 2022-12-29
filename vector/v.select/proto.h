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
#define OP_OVERLAP    8
#define OP_RELATE     9

struct GParm {
    struct Option *input[2], *output, *type[2], *field[2],
	*operator, *relate;
};
struct GFlag {
    struct Flag *table, *reverse, *cat;
};

/* args.c */
void parse_options(struct GParm *, struct GFlag *);

/* copy_tabs.c */
void copy_tabs(struct Map_info *, struct Map_info *,
	       int, int *, int *, int **);

#ifdef HAVE_GEOS
/* geos.c */
int line_relate_geos(struct Map_info *, const GEOSGeometry *,
		     int, int, const char *);
int area_relate_geos(struct Map_info *, const GEOSGeometry *,
		     int, int, const char *);
#endif

/* select.c */
int select_lines(struct Map_info *, int, int,
                  struct Map_info *, int, int,
                  int, int, const char *, int *, int*, int*);

/* overlap.c */
void add_aarea(struct Map_info *, int, int *, int *);
int line_overlap_area(struct line_pnts *, struct line_pnts *,
                      struct line_pnts **, int);

/* write.c */
void write_lines(struct Map_info *, struct field_info *, int *, int *,
		 struct Map_info *, int, int,
		 int, int *, int *, int **);
#endif /* PROTO_H */
