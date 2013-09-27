#define TOOL_NODES   0
#define TOOL_CONNECT 1
#define TOOL_REPORT  2
#define TOOL_NREPORT 3
#define TOOL_ARCS    4

struct opt {
    struct Option *input, *points;
    struct Option *output;
    struct Option *action;
    struct Option *afield_opt, *nfield_opt, *thresh_opt;
    struct Option *file;
    struct Flag *cats_flag, *snap_flag;
};

/* arcs.c */
int create_arcs(FILE *, struct Map_info *,
		struct Map_info *, int, int);

/* argc.c */
void define_options(struct opt *);
void parse_arguments(const struct opt *,
		     int *, int *, double *, int *);

/* connect.c */
int connect_arcs(struct Map_info *, struct Map_info *,
		 struct Map_info *, int, int, double, int);

/* nodes.c */
int nodes(struct Map_info *, struct Map_info *, int,
	  int);

/* report.c */
int report(struct Map_info *, int, int,
	   int);
