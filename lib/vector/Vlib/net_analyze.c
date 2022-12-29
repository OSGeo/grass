/*!
 * \file lib/vector/Vlib/net_analyze.c
 *
 * \brief Vector library - related fns for vector network analyses
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2009, 2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Stepan Turek stepan.turek seznam.cz (turns support)
 */

#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int From_node;		/* from node set in SP and used by clipper for first arc */

static int clipper(dglGraph_s * pgraph,
		   dglSPClipInput_s * pargIn,
		   dglSPClipOutput_s * pargOut, void *pvarg)
{				/* caller's pointer */
    dglInt32_t cost;
    dglInt32_t from;

    G_debug(3, "Net: clipper()");

    from = dglNodeGet_Id(pgraph, pargIn->pnNodeFrom);

    G_debug(3, "  Edge = %d NodeFrom = %d NodeTo = %d edge cost = %d",
	    (int)dglEdgeGet_Id(pgraph, pargIn->pnEdge),
	    (int)from, (int)dglNodeGet_Id(pgraph, pargIn->pnNodeTo),
	    (int)pargOut->nEdgeCost);

    if (from != From_node) {	/* do not clip first */
	if (dglGet_NodeAttrSize(pgraph) > 0) {
	    memcpy(&cost, dglNodeGet_Attr(pgraph, pargIn->pnNodeFrom),
		   sizeof(cost));
	    if (cost == -1) {	/* closed, cannot go from this node except it is 'from' node */
		G_debug(3, "  closed node");
		return 1;
	    }
	    else {
		G_debug(3, "  EdgeCost += %d (node)", (int)cost);
		pargOut->nEdgeCost += cost;
	    }
	}
    }
    else {
	G_debug(3, "  don't clip first node");
    }

    return 0;
}

/*!
   \brief Converts shortest path result, which is calculated by DGLib on newtwork without turntable, into output format.
 */
static int convert_dgl_shortest_path_result(struct Map_info *Map,
					    dglSPReport_s * pSPReport,
					    struct ilist *List)
{
    int i, line;

    Vect_reset_list(List);

    for (i = 0; i < pSPReport->cArc; i++) {
	line =
	    dglEdgeGet_Id(&(Map->dgraph.graph_s), pSPReport->pArc[i].pnEdge);
	G_debug(2, "From %ld to %ld - cost %ld user %d distance %ld", pSPReport->pArc[i].nFrom, pSPReport->pArc[i].nTo, dglEdgeGet_Cost(&(Map->dgraph.graph_s), pSPReport->pArc[i].pnEdge) / Map->dgraph.cost_multip,	/* this is the cost from clip() */
		line, pSPReport->pArc[i].nDistance);
	Vect_list_append(List, line);
    }

    return 0;
}

/*!
   \brief Converts shortest path result, which is calculated by DGLib on newtwork with turntable, into output format.
 */
static int ttb_convert_dgl_shortest_path_result(struct Map_info *Map,
						dglSPReport_s * pSPReport,
						int tucfield,
						struct ilist *List)
{
    int i, line_id, type, tucfield_idx;
    int line_ucat;

    Vect_reset_list(List);

    tucfield_idx = Vect_cidx_get_field_index(Map, tucfield);

    for (i = 0; i < pSPReport->cArc; i++) {
	dglEdgeGet_Id(&(Map->dgraph.graph_s), pSPReport->pArc[i].pnEdge);

	line_ucat =
	    dglNodeGet_Id(&(Map->dgraph.graph_s),
			  dglEdgeGet_Head(&(Map->dgraph.graph_s),
					  pSPReport->pArc[i].pnEdge));

	/* get standard ucat numbers (DGLib does not like negative node numbers) */
	if (line_ucat % 2 == 1)
	    line_ucat = ((line_ucat - 1) / -2);
	else
	    line_ucat = (line_ucat) / 2;

	/* skip virtual nodes */
	if (Vect_cidx_find_next
	    (Map, tucfield_idx, abs(line_ucat), GV_LINE, 0, &type,
	     &line_id) == -1)
	    continue;

	if (line_ucat < 0)
	    line_id *= -1;

	G_debug(2, "From %ld to %ld - cost %ld user %d distance %ld", pSPReport->pArc[i].nFrom, pSPReport->pArc[i].nTo, dglEdgeGet_Cost(&(Map->dgraph.graph_s), pSPReport->pArc[i].pnEdge) / Map->dgraph.cost_multip,	/* this is the cost from clip() */
		line_ucat, pSPReport->pArc[i].nDistance);

	Vect_list_append(List, line_id);
    }

    return 0;
}

/*!
   \brief Finds shortest path on network using DGLib


   \param Map vector map with build DGLib graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param from from node id in build the network
   \param to to node in build the network
   \param UseTtb the graph is build with/without turntable
   \param tucfield layer with unique cats for turntable (relevant only when UseTtb = 1)
 */
static int find_shortest_path(struct Map_info *Map, int from, int to,
			      struct ilist *List, double *cost, int UseTtb,
			      int tucfield)
{
    int *pclip, cArc, nRet;
    dglSPReport_s *pSPReport;
    dglInt32_t nDistance;
    int use_cache = 1;		/* set to 0 to disable dglib cache */

    G_debug(3, "find_shortest_path(): from = %d, to = %d", from, to);

    /* Note : if from == to dgl goes to nearest node and returns back (dgl feature) => 
     *         check here for from == to */

    if (List != NULL)
	Vect_reset_list(List);

    /* Check if from and to are identical, otherwise dglib returns path to neares node and back! */
    if (from == to) {
	if (cost != NULL)
	    *cost = 0;
	return 0;
    }

    From_node = from;
    pclip = NULL;
    if (List != NULL) {
	if (use_cache) {
	    nRet =
		dglShortestPath(&(Map->dgraph.graph_s), &pSPReport,
				(dglInt32_t) from, (dglInt32_t) to, clipper,
				pclip, &(Map->dgraph.spCache));
	}
	else {
	    nRet =
		dglShortestPath(&(Map->dgraph.graph_s), &pSPReport,
				(dglInt32_t) from, (dglInt32_t) to, clipper,
				pclip, NULL);
	}
    }
    else {
	if (use_cache) {
	    nRet =
		dglShortestDistance(&(Map->dgraph.graph_s), &nDistance,
				    (dglInt32_t) from, (dglInt32_t) to,
				    clipper, pclip, &(Map->dgraph.spCache));
	}
	else {
	    nRet =
		dglShortestDistance(&(Map->dgraph.graph_s), &nDistance,
				    (dglInt32_t) from, (dglInt32_t) to,
				    clipper, pclip, NULL);
	}
    }

    if (nRet == 0) {
	/* G_warning("Destination node %d is unreachable from node %d\n" , to , from); */
	if (cost != NULL)
	    *cost = PORT_DOUBLE_MAX;
	return -1;
    }
    else if (nRet < 0) {
	G_warning(_("dglShortestPath error: %s"),
		  dglStrerror(&(Map->dgraph.graph_s)));
	return -1;
    }

    if (List != NULL) {
	if (UseTtb)
	    ttb_convert_dgl_shortest_path_result(Map, pSPReport, tucfield,
						 List);
	else
	    convert_dgl_shortest_path_result(Map, pSPReport, List);
    }

    if (cost != NULL) {
	if (List != NULL)
	    *cost = (double)pSPReport->nDistance / Map->dgraph.cost_multip;
	else
	    *cost = (double)nDistance / Map->dgraph.cost_multip;
    }

    if (List != NULL) {
	cArc = pSPReport->cArc;
	dglFreeSPReport(&(Map->dgraph.graph_s), pSPReport);
    }
    else
	cArc = 0;

    return cArc;
}

/*!
   \brief Find shortest path on network.

   Costs for 'from' and 'to' nodes are not considered (SP found even if
   'from' or 'to' are 'closed' (costs = -1) and costs of these
   nodes are not added to SP costs result.

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param from start of the path
   \param from_type if 0 - node id (intersection), if 1 - line unique cat 
   \param to end of the path
   \param to_type if 0 - node id (intersection), if 1 - line unique cat
   \param tucfield field with unique categories used in the turntable 
   \param[out] List list of line ids (path)
   \param[out] cost costs value

   \return number of segments
   \return 0 is correct for from = to, or List == NULL ? sum of costs is better return value,
   \return -1 : destination unreachable

 */

int
Vect_net_ttb_shortest_path(struct Map_info *Map, int from, int from_type,
			   int to, int to_type,
			   int tucfield, struct ilist *List, double *cost)
{
    double x, y, z;
    struct bound_box box;
    struct boxlist *box_List;
    struct line_cats *Cats;
    int f, t;
    int i_line, line, type, cfound;

    box_List = Vect_new_boxlist(0);
    Cats = Vect_new_cats_struct();

    if (from_type == 0) {	/* TODO duplicite code with to_type, move into function */
	/* select points at node */
	Vect_get_node_coor(Map, from, &x, &y, &z);
	box.E = box.W = x;
	box.N = box.S = y;
	box.T = box.B = z;
	Vect_select_lines_by_box(Map, &box, GV_POINT, box_List);

	cfound = 0;

	for (i_line = 0; i_line < box_List->n_values; i_line++) {
	    line = box_List->id[i_line];

	    type = Vect_read_line(Map, NULL, Cats, line);
	    if (!(type & GV_POINT))
		continue;
	    if (Vect_cat_get(Cats, tucfield, &f)) {
		++cfound;
		break;
	    }
	}
	if (!cfound)
	    G_fatal_error(_
			  ("Unable to find point with defined unique category for node <%d>."),
			  from);
	else if (cfound > 1)
	    G_warning(_
		      ("There exists more than one point on node <%d> with unique category in field  <%d>.\n"
		       "The unique category layer may not be valid."),
		      tucfield, from);

	G_debug(2, "from node = %d, unique cat = %d ", from, f);
	f = f * 2;
    }
    else {
	if (from < 0)
	    f = from * -2 + 1;
	else
	    f = from * 2;
	G_debug(2, "from edge unique cat = %d", from);
    }

    if (to_type == 0) {
	/* select points at node */
	Vect_get_node_coor(Map, to, &x, &y, &z);
	box.E = box.W = x;
	box.N = box.S = y;
	box.T = box.B = z;
	Vect_select_lines_by_box(Map, &box, GV_POINT, box_List);

	cfound = 0;

	for (i_line = 0; i_line < box_List->n_values; i_line++) {
	    line = box_List->id[i_line];
	    type = Vect_read_line(Map, NULL, Cats, line);
	    if (!(type & GV_POINT))
		continue;
	    if (Vect_cat_get(Cats, tucfield, &t)) {
		cfound = 1;
		break;
	    }
	}
	if (!cfound)
	    G_fatal_error(_
			  ("Unable to find point with defined unique category for node <%d>."),
			  to);
	else if (cfound > 1)
	    G_warning(_
		      ("There exists more than one point on node <%d> with unique category in field  <%d>.\n"
		       "The unique category layer may not be valid."),
		      tucfield, to);

	G_debug(2, "to node = %d, unique cat = %d ", to, t);
	t = t * 2 + 1;
    }
    else {
	if (to < 0)
	    t = to * -2 + 1;
	else
	    t = to * 2;
	G_debug(2, "to edge unique cat = %d", to);
    }

    Vect_destroy_boxlist(box_List);
    Vect_destroy_cats_struct(Cats);

    return find_shortest_path(Map, f, t, List, cost, 1, tucfield);
}

/*!
   \brief Find shortest path.

   Costs for 'from' and 'to' nodes are not considered (SP found even if
   'from' or 'to' are 'closed' (costs = -1) and costs of these
   nodes are not added to SP costs result.

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param from from node
   \param to to node
   \param[out] List list of line ids (path)
   \param[out] cost costs value

   \return number of segments
   \return 0 is correct for from = to, or List == NULL ? sum of costs is better return value,
   \return -1 : destination unreachable

 */
int
Vect_net_shortest_path(struct Map_info *Map, int from, int to,
		       struct ilist *List, double *cost)
{
    return find_shortest_path(Map, from, to, List, cost, 0, -1);
}

/*!
   \brief Get graph structure

   Graph is built by Vect_net_build_graph().

   Returns NULL when graph is not built.

   \param Map pointer to Map_info struct

   \return pointer to dglGraph_s struct or NULL
 */
dglGraph_s *Vect_net_get_graph(struct Map_info * Map)
{
    return &(Map->dgraph.graph_s);
}

/*! 
   \brief Returns in cost for given direction in *cost.

   cost is set to -1 if closed.

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param line line id
   \param direction direction (GV_FORWARD, GV_BACKWARD) 
   \param[out] cost

   \return 1 OK
   \return 0 does not exist (was not inserted)
 */
int
Vect_net_get_line_cost(const struct Map_info *Map, int line, int direction,
		       double *cost)
{
    /* dglInt32_t *pEdge; */

    G_debug(5, "Vect_net_get_line_cost(): line = %d, dir = %d", line,
	    direction);

    if (direction == GV_FORWARD) {
	/* V1 has no index by line-id -> array used */
	/*
	   pEdge = dglGetEdge(&(Map->dgraph.graph_s), line);
	   if (pEdge == NULL)
	   return 0;
	   *cost = (double) dglEdgeGet_Cost(&(Map->dgraph.graph_s), pEdge);
	 */
	if (Map->dgraph.edge_fcosts[line] == -1) {
	    *cost = -1;
	    return 0;
	}
	else
	    *cost = Map->dgraph.edge_fcosts[line];
    }
    else if (direction == GV_BACKWARD) {
	/*
	   pEdge = dglGetEdge(&(Map->dgraph.graph_s), -line);
	   if (pEdge == NULL) 
	   return 0;
	   *cost = (double) dglEdgeGet_Cost(&(Map->dgraph.graph_s), pEdge);
	 */
	if (Map->dgraph.edge_bcosts[line] == -1) {
	    *cost = -1;
	    return 0;
	}
	else
	    *cost = Map->dgraph.edge_bcosts[line];
	G_debug(5, "Vect_net_get_line_cost(): edge_bcosts = %f",
		Map->dgraph.edge_bcosts[line]);
    }
    else {
	G_fatal_error(_("Wrong line direction in Vect_net_get_line_cost()"));
    }

    return 1;
}

/*!
   \brief Get cost of node

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param node node id
   \param[out] cost costs value

   \return 1
 */
int Vect_net_get_node_cost(const struct Map_info *Map, int node, double *cost)
{
    G_debug(3, "Vect_net_get_node_cost(): node = %d", node);

    *cost = Map->dgraph.node_costs[node];

    G_debug(3, "  -> cost = %f", *cost);

    return 1;
}

/*!
   \brief Find nearest node(s) on network. 

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param x,y,z point coordinates (z coordinate NOT USED !)
   \param direction (GV_FORWARD - from point to net, GV_BACKWARD - from net to point)
   \param maxdist maximum distance to the network
   \param[out] node1 pointer where to store the node number (or NULL)
   \param[out] node2 pointer where to store the node number (or NULL)
   \param[out] ln    pointer where to store the nearest line number (or NULL)
   \param[out] costs1 pointer where to store costs on nearest line to node1 (not costs from x,y,z to the line) (or NULL)
   \param[out] costs2 pointer where to store costs on nearest line to node2 (not costs from x,y,z to the line) (or NULL)
   \param[out] Points1 pointer to structure where to store vertices on nearest line to node1 (or NULL)
   \param[out] Points2 pointer to structure where to store vertices on nearest line to node2 (or NULL)
   \param[out] pointer where to distance to the line (or NULL)
   \param[out] distance

   \return number of nodes found (0,1,2)
 */
int Vect_net_nearest_nodes(struct Map_info *Map,
			   double x, double y, double z,
			   int direction, double maxdist,
			   int *node1, int *node2, int *ln, double *costs1,
			   double *costs2, struct line_pnts *Points1,
			   struct line_pnts *Points2, double *distance)
{
    int line, n1, n2, nnodes;
    int npoints;
    int segment;		/* nearest line segment (first is 1) */
    static struct line_pnts *Points = NULL;
    double cx, cy, cz, c1, c2;
    double along;		/* distance along the line to nearest point */
    double length;

    G_debug(3, "Vect_net_nearest_nodes() x = %f y = %f", x, y);

    /* Reset */
    if (node1)
	*node1 = 0;
    if (node2)
	*node2 = 0;
    if (ln)
	*ln = 0;
    if (costs1)
	*costs1 = PORT_DOUBLE_MAX;
    if (costs2)
	*costs2 = PORT_DOUBLE_MAX;
    if (Points1)
	Vect_reset_line(Points1);
    if (Points2)
	Vect_reset_line(Points2);
    if (distance)
	*distance = PORT_DOUBLE_MAX;

    if (!Points)
	Points = Vect_new_line_struct();

    /* Find nearest line */
    line = Vect_find_line(Map, x, y, z, Map->dgraph.line_type, maxdist, 0, 0);

    if (line < 1)
	return 0;

    Vect_read_line(Map, Points, NULL, line);
    npoints = Points->n_points;
    Vect_get_line_nodes(Map, line, &n1, &n2);

    segment =
	Vect_line_distance(Points, x, y, z, 0, &cx, &cy, &cz, distance, NULL,
			   &along);

    G_debug(4, "line = %d n1 = %d n2 = %d segment = %d", line, n1, n2,
	    segment);

    /* Check first or last point and return one node in that case */
    G_debug(4, "cx = %f cy = %f first = %f %f last = %f %f", cx, cy,
	    Points->x[0], Points->y[0], Points->x[npoints - 1],
	    Points->y[npoints - 1]);

    if (Points->x[0] == cx && Points->y[0] == cy) {
	if (node1)
	    *node1 = n1;
	if (ln)
	    *ln = line;
	if (costs1)
	    *costs1 = 0;
	if (Points1) {
	    Vect_append_point(Points1, x, y, z);
	    Vect_append_point(Points1, cx, cy, cz);
	}
	G_debug(3, "first node nearest");
	return 1;
    }
    if (Points->x[npoints - 1] == cx && Points->y[npoints - 1] == cy) {
	if (node1)
	    *node1 = n2;
	if (ln)
	    *ln = line;
	if (costs1)
	    *costs1 = 0;
	if (Points1) {
	    Vect_append_point(Points1, x, y, z);
	    Vect_append_point(Points1, cx, cy, cz);
	}
	G_debug(3, "last node nearest");
	return 1;
    }

    nnodes = 2;

    /* c1 - costs to get from/to the first vertex */
    /* c2 - costs to get from/to the last vertex */
    if (direction == GV_FORWARD) {	/* from point to net */
	Vect_net_get_line_cost(Map, line, GV_BACKWARD, &c1);
	Vect_net_get_line_cost(Map, line, GV_FORWARD, &c2);
    }
    else {
	Vect_net_get_line_cost(Map, line, GV_FORWARD, &c1);
	Vect_net_get_line_cost(Map, line, GV_BACKWARD, &c2);
    }

    if (c1 < 0)
	nnodes--;
    if (c2 < 0)
	nnodes--;
    if (nnodes == 0)
	return 0;		/* both directions closed */

    length = Vect_line_length(Points);

    if (ln)
	*ln = line;

    if (nnodes == 1 && c1 < 0) {	/* first direction is closed, return node2 as node1 */
	if (node1)
	    *node1 = n2;

	if (costs1) {		/* to node 2, i.e. forward */
	    *costs1 = c2 * (length - along) / length;
	}

	if (Points1) {		/* to node 2, i.e. forward */
	    int i;

	    if (direction == GV_FORWARD) {	/* from point to net */
		Vect_append_point(Points1, x, y, z);
		Vect_append_point(Points1, cx, cy, cz);
		for (i = segment; i < npoints; i++)
		    Vect_append_point(Points1, Points->x[i], Points->y[i],
				      Points->z[i]);
	    }
	    else {
		for (i = npoints - 1; i >= segment; i--)
		    Vect_append_point(Points1, Points->x[i], Points->y[i],
				      Points->z[i]);

		Vect_append_point(Points1, cx, cy, cz);
		Vect_append_point(Points1, x, y, z);
	    }
	}
    }
    else {
	if (node1)
	    *node1 = n1;
	if (node2)
	    *node2 = n2;

	if (costs1) {		/* to node 1, i.e. backward */
	    *costs1 = c1 * along / length;
	}

	if (costs2) {		/* to node 2, i.e. forward */
	    *costs2 = c2 * (length - along) / length;
	}

	if (Points1) {		/* to node 1, i.e. backward */
	    int i;

	    if (direction == GV_FORWARD) {	/* from point to net */
		Vect_append_point(Points1, x, y, z);
		Vect_append_point(Points1, cx, cy, cz);
		for (i = segment - 1; i >= 0; i--)
		    Vect_append_point(Points1, Points->x[i], Points->y[i],
				      Points->z[i]);
	    }
	    else {
		for (i = 0; i < segment; i++)
		    Vect_append_point(Points1, Points->x[i], Points->y[i],
				      Points->z[i]);

		Vect_append_point(Points1, cx, cy, cz);
		Vect_append_point(Points1, x, y, z);
	    }
	}

	if (Points2) {		/* to node 2, i.e. forward */
	    int i;

	    if (direction == GV_FORWARD) {	/* from point to net */
		Vect_append_point(Points2, x, y, z);
		Vect_append_point(Points2, cx, cy, cz);
		for (i = segment; i < npoints; i++)
		    Vect_append_point(Points2, Points->x[i], Points->y[i],
				      Points->z[i]);
	    }
	    else {
		for (i = npoints - 1; i >= segment; i--)
		    Vect_append_point(Points2, Points->x[i], Points->y[i],
				      Points->z[i]);

		Vect_append_point(Points2, cx, cy, cz);
		Vect_append_point(Points2, x, y, z);
	    }
	}
    }

    return nnodes;
}

/*!
   \brief Find shortest path on network between 2 points given by coordinates. 

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param fx,fy,fz from point x coordinate (z ignored)
   \param tx,ty,tz to point x coordinate (z ignored)
   \param fmax maximum distance to the network from 'from'
   \param tmax maximum distance to the network from 'to'
   \param UseTtb the graph is build with/without turntable
   \param tucfield field with unique categories used in the turntable 
   \param costs pointer where to store costs on the network (or NULL)
   \param Points pointer to the structure where to store vertices of shortest path (or NULL)
   \param List pointer to the structure where list of lines on the network is stored (or NULL)
   \param NodesList pointer to the structure where list of nodes on the network is stored (or NULL)
   \param FPoints pointer to the structure where to store line from 'from' to first network node (or NULL)
   \param TPoints pointer to the structure where to store line from last network node to 'to' (or NULL)
   \param fdist distance from 'from' to the net (or NULL)
   \param tdist distance from 'to' to the net (or NULL)

   \return 1 OK, 0 not reachable
 */
static int
find_shortest_path_coor(struct Map_info *Map,
			double fx, double fy, double fz, double tx,
			double ty, double tz, double fmax, double tmax,
			int UseTtb, int tucfield,
			double *costs, struct line_pnts *Points,
			struct ilist *List, struct ilist *NodesList,
			struct line_pnts *FPoints,
			struct line_pnts *TPoints, double *fdist,
			double *tdist)
{
    int fnode[2], tnode[2];	/* nearest nodes, *node[1] is 0 if only one was found */
    double fcosts[2], tcosts[2], cur_cst;	/* costs to nearest nodes on the network */
    int nfnodes, ntnodes, fline, tline;
    static struct line_pnts *APoints, *SPoints, *fPoints[2], *tPoints[2];
    static struct ilist *LList;
    static int first = 1;
    int reachable, shortcut;
    int i, j, fn = 0, tn = 0;

    /* from/to_point_node is set if from/to point projected to line 
     *falls exactly on node (shortcut -> fline == tline) */
    int from_point_node = 0;
    int to_point_node = 0;

    G_debug(3, "Vect_net_shortest_path_coor()");

    if (first) {
	APoints = Vect_new_line_struct();
	SPoints = Vect_new_line_struct();
	fPoints[0] = Vect_new_line_struct();
	fPoints[1] = Vect_new_line_struct();
	tPoints[0] = Vect_new_line_struct();
	tPoints[1] = Vect_new_line_struct();
	LList = Vect_new_list();
	first = 0;
    }

    /* Reset */
    if (costs)
	*costs = PORT_DOUBLE_MAX;
    if (Points)
	Vect_reset_line(Points);
    if (fdist)
	*fdist = 0;
    if (tdist)
	*tdist = 0;
    if (List)
	List->n_values = 0;
    if (FPoints)
	Vect_reset_line(FPoints);
    if (TPoints)
	Vect_reset_line(TPoints);
    if (NodesList != NULL)
	Vect_reset_list(NodesList);

    /* Find nearest nodes */
    fnode[0] = fnode[1] = tnode[0] = tnode[1] = 0;

    nfnodes =
	Vect_net_nearest_nodes(Map, fx, fy, fz, GV_FORWARD, fmax, &(fnode[0]),
			       &(fnode[1]), &fline, &(fcosts[0]),
			       &(fcosts[1]), fPoints[0], fPoints[1], fdist);
    if (nfnodes == 0)
	return 0;

    if (nfnodes == 1 && fPoints[0]->n_points < 3) {
	from_point_node = fnode[0];
    }

    ntnodes =
	Vect_net_nearest_nodes(Map, tx, ty, tz, GV_BACKWARD, tmax,
			       &(tnode[0]), &(tnode[1]), &tline, &(tcosts[0]),
			       &(tcosts[1]), tPoints[0], tPoints[1], tdist);
    if (ntnodes == 0)
	return 0;

    if (ntnodes == 1 && tPoints[0]->n_points < 3) {
	to_point_node = tnode[0];
    }


    G_debug(3, "fline = %d tline = %d", fline, tline);

    reachable = shortcut = 0;
    cur_cst = PORT_DOUBLE_MAX;

    /* It may happen, that 2 points are at the same line. */
    /* TODO?: it could also happen that fline != tline but both points are on the same
     * line if they fall on node but a different line was found. This case is correctly
     * handled as normal non shortcut, but it could be added here. In that case 
     * NodesList collection must be changed */
    if (fline == tline && (nfnodes > 1 || ntnodes > 1)) {
	double len, flen, tlen, c, fseg, tseg;
	double fcx, fcy, fcz, tcx, tcy, tcz;

	Vect_read_line(Map, APoints, NULL, fline);
	len = Vect_line_length(APoints);

	/* distance along the line */
	fseg =
	    Vect_line_distance(APoints, fx, fy, fz, 0, &fcx, &fcy, &fcz, NULL,
			       NULL, &flen);
	tseg =
	    Vect_line_distance(APoints, tx, ty, tz, 0, &tcx, &tcy, &tcz, NULL,
			       NULL, &tlen);

	Vect_reset_line(SPoints);
	if (flen == tlen) {
	    cur_cst = 0;

	    Vect_append_point(SPoints, fx, fy, fz);
	    Vect_append_point(SPoints, fcx, fcy, fcz);
	    Vect_append_point(SPoints, tx, ty, tz);

	    reachable = shortcut = 1;
	}
	else if (flen < tlen) {
	    Vect_net_get_line_cost(Map, fline, GV_FORWARD, &c);
	    if (c >= 0) {
		cur_cst = c * (tlen - flen) / len;

		Vect_append_point(SPoints, fx, fy, fz);
		Vect_append_point(SPoints, fcx, fcy, fcz);
		for (i = fseg; i < tseg; i++)
		    Vect_append_point(SPoints, APoints->x[i], APoints->y[i],
				      APoints->z[i]);

		Vect_append_point(SPoints, tcx, tcy, tcz);
		Vect_append_point(SPoints, tx, ty, tz);

		reachable = shortcut = 1;
	    }
	}
	else {			/* flen > tlen */
	    Vect_net_get_line_cost(Map, fline, GV_BACKWARD, &c);
	    if (c >= 0) {
		cur_cst = c * (flen - tlen) / len;

		Vect_append_point(SPoints, fx, fy, fz);
		Vect_append_point(SPoints, fcx, fcy, fcz);
		for (i = fseg - 1; i >= tseg; i--)
		    Vect_append_point(SPoints, APoints->x[i], APoints->y[i],
				      APoints->z[i]);

		Vect_append_point(SPoints, tcx, tcy, tcz);
		Vect_append_point(SPoints, tx, ty, tz);

		reachable = shortcut = 1;
	    }
	}
    }

    /* Find the shortest variant from maximum 4 */
    for (i = 0; i < nfnodes; i++) {
	for (j = 0; j < ntnodes; j++) {
	    double ncst, cst;
	    int ret;

	    G_debug(3, "i = %d fnode = %d j = %d tnode = %d", i, fnode[i], j,
		    tnode[j]);

	    if (UseTtb)
		ret =
		    Vect_net_ttb_shortest_path(Map, fnode[i], 0, tnode[j], 0,
					       tucfield, NULL, &ncst);
	    else
		ret =
		    Vect_net_shortest_path(Map, fnode[i], tnode[j], NULL,
					   &ncst);
	    if (ret == -1)
		continue;	/* not reachable */

	    cst = fcosts[i] + ncst + tcosts[j];
	    if (reachable == 0 || cst < cur_cst) {
		cur_cst = cst;
		fn = i;
		tn = j;
		shortcut = 0;
	    }
	    reachable = 1;
	}
    }

    G_debug(3, "reachable = %d shortcut = %d cur_cst = %f", reachable,
	    shortcut, cur_cst);
    if (reachable) {
	if (shortcut) {
	    if (Points)
		Vect_append_points(Points, SPoints, GV_FORWARD);
	    if (NodesList) {
		/* Check if from/to point projected to line falls on node and 
		 *add it to the list */
		if (from_point_node > 0)
		    Vect_list_append(NodesList, from_point_node);

		if (to_point_node > 0)
		    Vect_list_append(NodesList, to_point_node);
	    }
	}
	else {
	    if (NodesList) {
		/* it can happen that starting point falls on node but SP starts 
		 * form the other node, add it in that case, 
		 * similarly for to point below */
		if (from_point_node > 0 && from_point_node != fnode[fn]) {
		    Vect_list_append(NodesList, from_point_node);
		}

		/* add starting net SP search node */
		Vect_list_append(NodesList, fnode[fn]);
	    }

	    if (UseTtb)
		Vect_net_ttb_shortest_path(Map, fnode[fn], 0, tnode[tn], 0,
					   tucfield, LList, NULL);
	    else
		Vect_net_shortest_path(Map, fnode[fn], tnode[tn], LList,
				       NULL);

	    G_debug(3, "Number of lines %d", LList->n_values);

	    if (Points)
		Vect_append_points(Points, fPoints[fn], GV_FORWARD);

	    if (FPoints)
		Vect_append_points(FPoints, fPoints[fn], GV_FORWARD);

	    for (i = 0; i < LList->n_values; i++) {
		int line;

		line = LList->value[i];
		G_debug(3, "i = %d line = %d", i, line);

		if (Points) {
		    Vect_read_line(Map, APoints, NULL, abs(line));

		    if (line > 0)
			Vect_append_points(Points, APoints, GV_FORWARD);
		    else
			Vect_append_points(Points, APoints, GV_BACKWARD);
		    Points->n_points--;
		}
		if (NodesList) {
		    int node, node1, node2;

		    Vect_get_line_nodes(Map, abs(line), &node1, &node2);
		    /* add the second node, the first of first segmet was alread added */
		    if (line > 0)
			node = node2;
		    else
			node = node1;

		    Vect_list_append(NodesList, node);
		}

		if (List)
		    Vect_list_append(List, line);
	    }

	    if (Points) {
		if (LList->n_values)
		    Points->n_points++;
		Vect_append_points(Points, tPoints[tn], GV_FORWARD);
	    }

	    if (TPoints)
		Vect_append_points(TPoints, tPoints[tn], GV_FORWARD);

	    if (NodesList) {
		if (to_point_node > 0 && to_point_node != tnode[tn]) {
		    Vect_list_append(NodesList, to_point_node);
		}
	    }
	}

	if (costs)
	    *costs = cur_cst;
	if (Points)
	    Vect_line_prune(Points);
    }

    return reachable;
}

/*!
   \brief Find shortest path on network between 2 points given by coordinates. 

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param fx,fy,fz from point x coordinate (z ignored)
   \param tx,ty,tz to point x coordinate (z ignored)
   \param fmax maximum distance to the network from 'from'
   \param tmax maximum distance to the network from 'to'
   \param costs pointer where to store costs on the network (or NULL)
   \param Points pointer to the structure where to store vertices of shortest path (or NULL)
   \param List pointer to the structure where list of lines on the network is stored (or NULL)
   \param NodesList pointer to the structure where list of nodes on the network is stored (or NULL)
   \param FPoints pointer to the structure where to store line from 'from' to first network node (or NULL)
   \param TPoints pointer to the structure where to store line from last network node to 'to' (or NULL)
   \param fdist distance from 'from' to the net (or NULL)
   \param tdist distance from 'to' to the net (or NULL)

   \return 1 OK, 0 not reachable
 */
int
Vect_net_shortest_path_coor(struct Map_info *Map,
			    double fx, double fy, double fz, double tx,
			    double ty, double tz, double fmax, double tmax,
			    double *costs, struct line_pnts *Points,
			    struct ilist *List, struct ilist *NodesList,
			    struct line_pnts *FPoints,
			    struct line_pnts *TPoints, double *fdist,
			    double *tdist)
{
    return find_shortest_path_coor(Map, fx, fy, fz, tx, ty, tz, fmax, tmax, 0,
				   0, costs, Points, List, NodesList,
				   FPoints, TPoints, fdist, tdist);
}

/*!
   \brief Find shortest path on network with turntable between 2 points given by coordinates. 

   \param Map vector map with build graph (see Vect_net_ttb_build_graph and Vect_net_build_graph)
   \param fx,fy,fz from point x coordinate (z ignored)
   \param tx,ty,tz to point x coordinate (z ignored)
   \param fmax maximum distance to the network from 'from'
   \param tmax maximum distance to the network from 'to'
   \param tucfield field with unique categories used in the turntable 
   \param costs pointer where to store costs on the network (or NULL)
   \param Points pointer to the structure where to store vertices of shortest path (or NULL)
   \param List pointer to the structure where list of lines on the network is stored (or NULL)
   \param NodesList pointer to the structure where list of nodes on the network is stored (or NULL)
   \param FPoints pointer to the structure where to store line from 'from' to first network node (or NULL)
   \param TPoints pointer to the structure where to store line from last network node to 'to' (or NULL)
   \param fdist distance from 'from' to the net (or NULL)
   \param tdist distance from 'to' to the net (or NULL)

   \return 1 OK, 0 not reachable
 */
int
Vect_net_ttb_shortest_path_coor(struct Map_info *Map,
				double fx, double fy, double fz, double tx,
				double ty, double tz, double fmax,
				double tmax, int tucfield,
				double *costs, struct line_pnts *Points,
				struct ilist *List, struct ilist *NodesList,
				struct line_pnts *FPoints,
				struct line_pnts *TPoints, double *fdist,
				double *tdist)
{
    return find_shortest_path_coor(Map, fx, fy, fz, tx, ty, tz, fmax, tmax,
				   1, tucfield, costs, Points, List,
				   NodesList, FPoints, TPoints, fdist, tdist);
}
