#define TOOL_BREAK    1		/* break at intersection */
#define TOOL_RMDUPL   2		/* remove duplicates */
#define TOOL_RMDANGLE 3		/* remove dangles */
#define TOOL_CHDANGLE 4		/* retype 'boundary' dangles to 'line' */
#define TOOL_SNAP     5		/* snap line to vertex in threshold */
#define TOOL_RMDAC    6		/* remove duplicate area centroids */
#define TOOL_BPOL     7		/* break polygons on shared points with different angles */
#define TOOL_RMBRIDGE 8		/* remove bridges connecting area and island or 2 islands */
#define TOOL_CHBRIDGE 9		/* change the type of bridges connecting area and island or 2 islands from boundary to line */
#define TOOL_PRUNE   10		/* remove vertices in threshold from lines and boundaries */
#define TOOL_RMAREA  11		/* remove small areas */
#define TOOL_RMSA    12		/* remove small angles between lines at nodes */
#define TOOL_RMLINE  13		/* remove all line or boundaries of zero length */

#define SEP \
    "--------------------------------------------------"

int rmdac(struct Map_info *Out, struct Map_info *Err);
void remove_bridges(struct Map_info *Map, struct Map_info *Err);
int prune(struct Map_info *Out, int otype, double thresh,
	  struct Map_info *Err);
int remove_zero_line(struct Map_info *Map, int type, struct Map_info *Err);
int split_lines(struct Map_info *Map, int otype, struct Map_info *Err);
