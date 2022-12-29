
typedef struct
{
    int cat;			/* category number */
    int node;			/* node number */
} CENTER;

typedef struct
{
    int center;			/* neares center, initially -1 */
    double cost;		/* costs from this center, initially not defined */
    int edge;			/* edge to follow from this node */
} NODE;

int alloc_from_centers_loop_tt(struct Map_info *Map, NODE *Nodes,
                               CENTER *Centers, int ncenters,
                               int tucfield);
int alloc_to_centers_loop_tt(struct Map_info *Map, NODE *Nodes,
                               CENTER *Centers, int ncenters,
                               int tucfield);

int alloc_from_centers(dglGraph_s *graph, NODE *Nodes, CENTER *Centers, int ncenters);
int alloc_to_centers(dglGraph_s *graph, NODE *Nodes, CENTER *Centers, int ncenters);
