/*!
 * \file lib/vector/Vlib/net.c
 *
 * \brief Vector library - net releated fns
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
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
   \brief Build network graph.

   Internal format for edge costs is integer, costs are multiplied
   before conversion to int by 1000 and for lenghts LL without geo flag by 1000000.
   The same multiplication factor is used for nodes.
   Costs in database column may be 'integer' or 'double precision' number >= 0
   or -1 for infinity i.e. arc or node is closed and cannot be traversed
   If record in table is not found for arcs, arc is skip.
   If record in table is not found for node, costs for node are set to 0.

   \param Map vector map
   \param ltype line type for arcs
   \param afield arc costs field (if 0, use length)
   \param nfield node costs field (if 0, do not use node costs)
   \param afcol column with forward costs for arc
   \param abcol column with backward costs for arc (if NULL, back costs = forward costs), 
   \param ncol column with costs for nodes (if NULL, do not use node costs), 
   \param geo use geodesic calculation for length (LL), 
   \param algorithm not used (in future code for algorithm)

   \return 0 on success, 1 on error
 */
int
Vect_net_build_graph(struct Map_info *Map,
		     int ltype,
		     int afield,
		     int nfield,
		     const char *afcol,
		     const char *abcol,
		     const char *ncol, int geo, int algorithm)
{
    int i, j, from, to, line, nlines, nnodes, ret, type, cat, skipped, cfound;
    int dofw, dobw;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double dcost, bdcost, ll;
    int cost, bcost;
    dglGraph_s *gr;
    dglInt32_t opaqueset[16] =
	{ 360000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    struct field_info *Fi;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString stmt;
    dbColumn *Column;
    dbCatValArray fvarr, bvarr;
    int fctype = 0, bctype = 0, nrec;

    /* TODO int costs -> double (waiting for dglib) */
    G_debug(1, "Vect_build_graph(): ltype = %d, afield = %d, nfield = %d",
	    ltype, afield, nfield);
    G_debug(1, "    afcol = %s, abcol = %s, ncol = %s", afcol, abcol, ncol);

    G_message(_("Building graph..."));

    Map->dgraph.line_type = ltype;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    ll = 0;
    if (G_projection() == 3)
	ll = 1;			/* LL */

    if (afcol == NULL && ll && !geo)
	Map->dgraph.cost_multip = 1000000;
    else
	Map->dgraph.cost_multip = 1000;

    nlines = Vect_get_num_lines(Map);
    nnodes = Vect_get_num_nodes(Map);

    gr = &(Map->dgraph.graph_s);

    /* Allocate space for costs, later replace by functions reading costs from graph */
    Map->dgraph.edge_fcosts = (double *)G_malloc((nlines + 1) * sizeof(double));
    Map->dgraph.edge_bcosts = (double *)G_malloc((nlines + 1) * sizeof(double));
    Map->dgraph.node_costs = (double *)G_malloc((nnodes + 1) * sizeof(double));
    /* Set to -1 initially */
    for (i = 1; i <= nlines; i++) {
	Map->dgraph.edge_fcosts[i] = -1;	/* forward */
	Map->dgraph.edge_bcosts[i] = -1;	/* backward */
    }
    for (i = 1; i <= nnodes; i++) {
	Map->dgraph.node_costs[i] = 0;
    }

    if (ncol != NULL)
	dglInitialize(gr, (dglByte_t) 1, sizeof(dglInt32_t), (dglInt32_t) 0,
		      opaqueset);
    else
	dglInitialize(gr, (dglByte_t) 1, (dglInt32_t) 0, (dglInt32_t) 0,
		      opaqueset);

    if (gr == NULL)
	G_fatal_error(_("Unable to build network graph"));

    db_init_handle(&handle);
    db_init_string(&stmt);

    if (abcol != NULL && afcol == NULL)
	G_fatal_error(_("Forward costs column not specified"));

    /* --- Add arcs --- */
    /* Open db connection */
    if (afcol != NULL) {
	/* Get field info */
	if (afield < 1)
	    G_fatal_error(_("Arc field < 1"));
	Fi = Vect_get_field(Map, afield);
	if (Fi == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  afield);

	/* Open database */
	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);

	/* Load costs to array */
	if (db_get_column(driver, Fi->table, afcol, &Column) != DB_OK)
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  afcol, Fi->table);

	fctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));

	if (fctype != DB_C_TYPE_INT && fctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Data type of column <%s> not supported (must be numeric)"),
			  afcol);

	db_CatValArray_init(&fvarr);
	nrec =
	    db_select_CatValArray(driver, Fi->table, Fi->key, afcol, NULL,
				  &fvarr);
	G_debug(1, "forward costs: nrec = %d", nrec);

	if (abcol != NULL) {
	    if (db_get_column(driver, Fi->table, abcol, &Column) != DB_OK)
		G_fatal_error(_("Column <%s> not found in table <%s>"),
			      abcol, Fi->table);

	    bctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));

	    if (bctype != DB_C_TYPE_INT && bctype != DB_C_TYPE_DOUBLE)
		G_fatal_error(_("Data type of column <%s> not supported (must be numeric)"),
			      abcol);

	    db_CatValArray_init(&bvarr);
	    nrec =
		db_select_CatValArray(driver, Fi->table, Fi->key, abcol, NULL,
				      &bvarr);
	    G_debug(1, "backward costs: nrec = %d", nrec);
	}
    }

    skipped = 0;

    G_message(_("Registering arcs..."));

    for (i = 1; i <= nlines; i++) {
	G_percent(i, nlines, 1);	/* must be before any continue */
	dofw = dobw = 1;
	type = Vect_read_line(Map, Points, Cats, i);
	if (!(type & ltype & (GV_LINE | GV_BOUNDARY)))
	    continue;

	Vect_get_line_nodes(Map, i, &from, &to);

	if (afcol != NULL) {
	    if (!(Vect_cat_get(Cats, afield, &cat))) {
		G_debug(2,
			"Category of field %d not attached to the line %d -> line skipped",
			afield, i);
		skipped += 2;	/* Both directions */
		continue;
	    }
	    else {
		if (fctype == DB_C_TYPE_INT) {
		    ret = db_CatValArray_get_value_int(&fvarr, cat, &cost);
		    dcost = cost;
		}
		else {		/* DB_C_TYPE_DOUBLE */
		    ret =
			db_CatValArray_get_value_double(&fvarr, cat, &dcost);
		}
		if (ret != DB_OK) {
		    G_warning(_("Database record for line %d (cat = %d, "
				"forward/both direction(s)) not found "
				"(forward/both direction(s) of line skipped)"),
			      i, cat);
		    dofw = 0;
		}

		if (abcol != NULL) {
		    if (bctype == DB_C_TYPE_INT) {
			ret =
			    db_CatValArray_get_value_int(&bvarr, cat, &bcost);
			bdcost = bcost;
		    }
		    else {	/* DB_C_TYPE_DOUBLE */
			ret =
			    db_CatValArray_get_value_double(&bvarr, cat,
							    &bdcost);
		    }
		    if (ret != DB_OK) {
			G_warning(_("Database record for line %d (cat = %d, "
				    "backword direction) not found"
				    "(direction of line skipped)"), i, cat);
			dobw = 0;
		    }
		}
		else {
		    if (dofw)
			bdcost = dcost;
		    else
			dobw = 0;
		}
	    }
	}
	else {
	    if (ll) {
		if (geo)
		    dcost = Vect_line_geodesic_length(Points);
		else
		    dcost = Vect_line_length(Points);
	    }
	    else
		dcost = Vect_line_length(Points);

	    bdcost = dcost;
	}
	if (dofw && dcost != -1) {
	    cost = (dglInt32_t) Map->dgraph.cost_multip * dcost;
	    G_debug(5, "Add arc %d from %d to %d cost = %d", i, from, to,
		    cost);
	    ret =
		dglAddEdge(gr, (dglInt32_t) from, (dglInt32_t) to,
			   (dglInt32_t) cost, (dglInt32_t) i);
	    Map->dgraph.edge_fcosts[i] = dcost;
	    if (ret < 0)
		G_fatal_error("Cannot add network arc");
	}

	G_debug(5, "bdcost = %f edge_bcosts = %f", bdcost,
		Map->dgraph.edge_bcosts[i]);
	if (dobw && bdcost != -1) {
	    bcost = (dglInt32_t) Map->dgraph.cost_multip * bdcost;
	    G_debug(5, "Add arc %d from %d to %d bcost = %d", -i, to, from,
		    bcost);
	    ret =
		dglAddEdge(gr, (dglInt32_t) to, (dglInt32_t) from,
			   (dglInt32_t) bcost, (dglInt32_t) - i);
	    Map->dgraph.edge_bcosts[i] = bdcost;
	    if (ret < 0)
		G_fatal_error(_("Cannot add network arc"));
	}
    }

    if (afcol != NULL && skipped > 0)
	G_debug(2, "%d lines missing category of field %d skipped", skipped,
		afield);

    if (afcol != NULL) {
	db_close_database_shutdown_driver(driver);
	db_CatValArray_free(&fvarr);

	if (abcol != NULL) {
	    db_CatValArray_free(&bvarr);
	}
    }

    /* Set node attributes */
    G_debug(2, "Register nodes");
    if (ncol != NULL) {
	double x, y, z;
	struct bound_box box;
	struct boxlist *List;
	
	List = Vect_new_boxlist(0);

	G_debug(2, "Set nodes' costs");
	if (nfield < 1)
	    G_fatal_error("Node field < 1");

	G_message(_("Setting node costs..."));

	Fi = Vect_get_field(Map, nfield);
	if (Fi == NULL)
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  nfield);

	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);

	/* Load costs to array */
	if (db_get_column(driver, Fi->table, ncol, &Column) != DB_OK)
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  ncol, Fi->table);

	fctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));

	if (fctype != DB_C_TYPE_INT && fctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Data type of column <%s> not supported (must be numeric)"),
			  ncol);

	db_CatValArray_init(&fvarr);
	nrec =
	    db_select_CatValArray(driver, Fi->table, Fi->key, ncol, NULL,
				  &fvarr);
	G_debug(1, "node costs: nrec = %d", nrec);

	for (i = 1; i <= nnodes; i++) {
	    /* TODO: what happens if we set attributes of not existing node (skipped lines,
	     *       nodes without lines) */

	    /* select points at node */
	    Vect_get_node_coor(Map, i, &x, &y, &z);
	    box.E = box.W = x;
	    box.N = box.S = y;
	    box.T = box.B = z;
	    Vect_select_lines_by_box(Map, &box, GV_POINT, List);

	    G_debug(2, "  node = %d nlines = %d", i, List->n_values);
	    cfound = 0;
	    dcost = 0;

	    for (j = 0; j < List->n_values; j++) {
		line = List->id[j];
		G_debug(2, "  line (%d) = %d", j, line);
		type = Vect_read_line(Map, NULL, Cats, line);
		if (!(type & GV_POINT))
		    continue;
		if (Vect_cat_get(Cats, nfield, &cat)) {	/* point with category of field found */
		    /* Set costs */
		    if (fctype == DB_C_TYPE_INT) {
			ret =
			    db_CatValArray_get_value_int(&fvarr, cat, &cost);
			dcost = cost;
		    }
		    else {	/* DB_C_TYPE_DOUBLE */
			ret =
			    db_CatValArray_get_value_double(&fvarr, cat,
							    &dcost);
		    }
		    if (ret != DB_OK) {
			G_warning(_("Database record for node %d (cat = %d) not found "
				   "(cost set to 0)"), i, cat);
		    }
		    cfound = 1;
		    break;
		}
	    }
	    if (!cfound) {
		G_debug(2,
			"Category of field %d not attached to any points in node %d"
			"(costs set to 0)", nfield, i);
	    }
	    if (dcost == -1) {	/* closed */
		cost = -1;
	    }
	    else {
		cost = (dglInt32_t) Map->dgraph.cost_multip * dcost;
	    }
	    G_debug(3, "Set node's cost to %d", cost);
	    dglNodeSet_Attr(gr, dglGetNode(gr, (dglInt32_t) i),
			    (dglInt32_t *) (dglInt32_t) & cost);
	    Map->dgraph.node_costs[i] = dcost;
	}
	db_close_database_shutdown_driver(driver);
	db_CatValArray_free(&fvarr);
	
	Vect_destroy_boxlist(List);
    }

    G_message(_("Flattening the graph..."));
    ret = dglFlatten(gr);
    if (ret < 0)
	G_fatal_error(_("GngFlatten error"));

    /* init SP cache */
    /* disable to debug dglib cache */
    dglInitializeSPCache(gr, &(Map->dgraph.spCache));

    G_message(_("Graph was built"));

    return 0;
}


/*!
   \brief Find shortest path.

   Costs for 'from' and 'to' nodes are not considered (SP found even if
   'from' or 'to' are 'closed' (costs = -1) and costs of these
   nodes are not added to SP costs result.

   \param Map vector map
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
    int i, line, *pclip, cArc, nRet;
    dglSPReport_s *pSPReport;
    dglInt32_t nDistance;
    int use_cache = 1;		/* set to 0 to disable dglib cache */

    G_debug(3, "Vect_net_shortest_path(): from = %d, to = %d", from, to);

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
		dglShortestPath(&(Map->dgraph.graph_s), &pSPReport, (dglInt32_t) from,
				(dglInt32_t) to, clipper, pclip, &(Map->dgraph.spCache));
	}
	else {
	    nRet =
		dglShortestPath(&(Map->dgraph.graph_s), &pSPReport, (dglInt32_t) from,
				(dglInt32_t) to, clipper, pclip, NULL);
	}
    }
    else {
	if (use_cache) {
	    nRet =
		dglShortestDistance(&(Map->dgraph.graph_s), &nDistance, (dglInt32_t) from,
				    (dglInt32_t) to, clipper, pclip, &(Map->dgraph.spCache));
	}
	else {
	    nRet =
		dglShortestDistance(&(Map->dgraph.graph_s), &nDistance, (dglInt32_t) from,
				    (dglInt32_t) to, clipper, pclip, NULL);
	}
    }

    if (nRet == 0) {
	/* G_warning("Destination node %d is unreachable from node %d\n" , to , from); */
	if (cost != NULL)
	    *cost = PORT_DOUBLE_MAX;
	return -1;
    }
    else if (nRet < 0) {
	G_warning(_("dglShortestPath error: %s"), dglStrerror(&(Map->dgraph.graph_s)));
	return -1;
    }

    if (List != NULL) {
	for (i = 0; i < pSPReport->cArc; i++) {
	    line = dglEdgeGet_Id(&(Map->dgraph.graph_s), pSPReport->pArc[i].pnEdge);
	    G_debug(2, "From %ld to %ld - cost %ld user %d distance %ld", pSPReport->pArc[i].nFrom, pSPReport->pArc[i].nTo, dglEdgeGet_Cost(&(Map->dgraph.graph_s), pSPReport->pArc[i].pnEdge) / Map->dgraph.cost_multip,	/* this is the cost from clip() */
		    line, pSPReport->pArc[i].nDistance);
	    Vect_list_append(List, line);
	}
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

    return (cArc);
}

/*!
  \brief Get graph structure
  
  Graph is built by Vect_net_build_graph().
  
  Returns NULL when graph is not built.
  
  \param Map pointer to Map_info struct

  \return pointer to dglGraph_s struct or NULL
*/
dglGraph_s *Vect_net_get_graph(struct Map_info *Map)
{
    return &(Map->dgraph.graph_s);
}

/*! 
   \brief Returns in cost for given direction in *cost.

   cost is set to -1 if closed.

   \param Map vector map
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

   \param Map vector map
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

   \param Map vetor map
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

   \param Map vector map
   \param fx,fy,fz from point x coordinate (z ignored)
   \param tx,ty,tz to point x coordinate (z ignored)
   \param fmax maximum distance to the network from 'from'
   \param tmax maximum distance to the network from 'to'
   \param[out] costs pointer where to store costs on the network (or NULL)
   \param[out] Points pointer to the structure where to store vertices of shortest path (or NULL)
   \param[out] List pointer to the structure where list of lines on the network is stored (or NULL)
   \param[out] FPoints pointer to the structure where to store line from 'from' to first network node (or NULL)
   \param[out] TPoints pointer to the structure where to store line from last network node to 'to' (or NULL)
   \param[out] fdist distance from 'from' to the net (or NULL)
   \param[out] tdist distance from 'to' to the net (or NULL)

   \return 1 OK
   \return 0 not reachable
 */
int
Vect_net_shortest_path_coor(struct Map_info *Map,
			    double fx, double fy, double fz, double tx,
			    double ty, double tz, double fmax, double tmax,
			    double *costs, struct line_pnts *Points,
			    struct ilist *List, struct line_pnts *FPoints,
			    struct line_pnts *TPoints, double *fdist,
			    double *tdist)
{
  return Vect_net_shortest_path_coor2(Map, fx, fy, fz, tx, ty, tz, fmax, tmax, 
            costs, Points, List, NULL, FPoints, TPoints, fdist, tdist);
}

/*!
   \brief Find shortest path on network between 2 points given by coordinates. 

   \param Map vector map
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
Vect_net_shortest_path_coor2(struct Map_info *Map,
			    double fx, double fy, double fz, double tx,
			    double ty, double tz, double fmax, double tmax,
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

	    ret =
		Vect_net_shortest_path(Map, fnode[i], tnode[j], NULL, &ncst);
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

	    if (Points)
		Vect_append_points(Points, tPoints[tn], GV_FORWARD);

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
    }

    return reachable;
}
