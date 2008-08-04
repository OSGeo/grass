#define TOOL_NODES   0
#define TOOL_CONNECT 1
#define TOOL_REPORT  2
#define TOOL_NREPORT 3

/* connect.c */
int connect_arcs(struct Map_info *In, struct Map_info *Pnts,
		 struct Map_info *Out, int nfield, double thresh);

/* nodes.c */
int nodes(struct Map_info *In, struct Map_info *Out, int add_cats,
	  int nfield);

/* report.c */
int report(struct Map_info *In, int afield, int nfield, int action);
