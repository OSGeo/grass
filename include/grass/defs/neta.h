#ifndef GRASS_NETADEFS_H
#define GRASS_NETADEFS_H

/*bridge.c */
int NetA_compute_bridges(dglGraph_s * graph, struct ilist *bridge_list);
int NetA_articulation_points(dglGraph_s * graph,
			     struct ilist *articulation_list);

/*components.c */
int NetA_weakly_connected_components(dglGraph_s * graph, int *component);
int NetA_strongly_connected_components(dglGraph_s * graph, int *component);

/*spanningtree.c */
int NetA_spanning_tree(dglGraph_s * graph, struct ilist *tree_list);

/*neta_flow.c */
int NetA_flow(dglGraph_s * graph, struct ilist *source_list,
	      struct ilist *sink_list, int *flow);
int NetA_min_cut(dglGraph_s * graph, struct ilist *source_list,
		 struct ilist *sink_list, int *flow, struct ilist *cut);
int NetA_split_vertices(dglGraph_s * in, dglGraph_s * out, int *node_costs);

/*utils.c */
void NetA_add_point_on_node(struct Map_info *In, struct Map_info *Out, int node,
			    struct line_cats *Cats);
void NetA_points_to_nodes(struct Map_info *In, struct ilist *point_list);
int NetA_get_node_costs(struct Map_info *In, int layer, char *column,
			int *node_costs);
void NetA_varray_to_nodes(struct Map_info *map, struct varray * varray,
			  struct ilist *nodes, int *nodes_to_features);
int NetA_initialise_varray(struct Map_info *In, int layer, int mask_type,
			   char *where, char *cat, struct varray ** varray);
/*centrality.c */
void NetA_degree_centrality(dglGraph_s * graph, double *degree);
int NetA_eigenvector_centrality(dglGraph_s * graph, int iterations,
				double error, double *eigenvector);
int NetA_betweenness_closeness(dglGraph_s * graph, double *betweenness,
			       double *closeness);

/*path.c */
int NetA_distance_from_points(dglGraph_s * graph, struct ilist *from, int *dst,
			      dglInt32_t ** prev);
int NetA_distance_to_points(dglGraph_s * graph, struct ilist *to, int *dst,
			      dglInt32_t ** nxt);
int NetA_find_path(dglGraph_s * graph, int from, int to, int *edges,
		   struct ilist *list);

/*timetables.c */

/*Structure containing all information about a timetable.
 * Everything in indexed from 0.
 */
typedef struct
{
    int routes;			/*Number of different routes. Two routes are different even if they differ only in time. */
    int *route_length;		/*Length of each route, i.e., number of stops */
    int **route_stops;		/*list of stops on each route in order (time increases) */
    int **route_times;		/*stop arrival times on overy route. Stops are given in the same order as above */
    int stops;			/*number of stops */
    int *stop_length;		/*Number of routes stopping at each stop */
    int **stop_routes;		/*List of routes for each stop. Routes are in increasing order */
    int **stop_times;		/*arrival times of routes for each stop. Routes are given in the same order as above */
    int *walk_length;		/*number of stops with "walking connection" for each stop */
    int **walk_stops;		/*list of stops within walking distance for each stop */
    int **walk_times;		/*walking times between stops as given above */
} neta_timetable;

typedef struct
{
    int **dst;
    int **prev_stop;
    int **prev_route;
    int **prev_conn;
    int rows, routes;
} neta_timetable_result;
int NetA_init_timetable_from_db(struct Map_info *In, int route_layer,
				int walk_layer, char *route_id, char *times,
				char *to_stop, char *walk_length,
				neta_timetable * timetable, int **route_ids,
				int **stop_ids);
int NetA_timetable_shortest_path(neta_timetable * timetable, int from_stop,
				 int to_stop, int start_time, int min_change,
				 int max_changes, int walking_change,
				 neta_timetable_result * result);
int NetA_timetable_get_route_time(neta_timetable * timetable, int stop,
				  int route);
void NetA_timetable_result_release(neta_timetable_result * result);

#endif
