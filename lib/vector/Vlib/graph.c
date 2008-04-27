/*!
  \file graph.c
  
  \brief Vector library - graph manipulation
  
  Higher level functions for reading/writing/manipulating vectors.

  TODO: Vect_graph_free ( GRAPH *graph )

  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Radim Blazek
  
  \date 2001-2008
*/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

static int From_node;   /* from node set in SP and used by clipper for first arc */  

static int clipper ( dglGraph_s    *pgraph ,
                     dglSPClipInput_s  * pargIn ,
                     dglSPClipOutput_s * pargOut ,
                     void *          pvarg )         /* caller's pointer */
{
    dglInt32_t cost;
    dglInt32_t from;
    
    G_debug ( 3, "Net: clipper()" );
    
    from = dglNodeGet_Id(pgraph, pargIn->pnNodeFrom);
    
    G_debug ( 3, "  Edge = %d NodeFrom = %d NodeTo = %d edge cost = %d", 
	             (int) dglEdgeGet_Id (pgraph, pargIn->pnEdge), 
	             (int) from, (int) dglNodeGet_Id(pgraph, pargIn->pnNodeTo),
		     (int) pargOut->nEdgeCost);

    if ( from != From_node ) { /* do not clip first */
	if ( dglGet_NodeAttrSize(pgraph) > 0 ) {
	    memcpy( &cost, dglNodeGet_Attr(pgraph, pargIn->pnNodeFrom), sizeof(cost) );
	    if ( cost == -1 ) { /* closed, cannot go from this node except it is 'from' node */
	        G_debug ( 3, "  closed node" );
		return 1;
	    } else {
		G_debug ( 3, "  EdgeCost += %d (node)", (int) cost );
		pargOut->nEdgeCost += cost;
	    }
	}
    } else {
        G_debug ( 3, "  don't clip first node" );
    }

    return 0;   
}

/*!
  \brief Initialize graph structure

  \param graph poiter to graph structure
  \param nodes_costs use node costs
  
  \return void
*/
void
Vect_graph_init ( GRAPH *graph, int nodes_costs )
{
    dglInt32_t opaqueset[ 16 ] = { 360000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    G_debug (3, "Vect_graph_init()" ); 

    if ( nodes_costs )
        dglInitialize(graph, (dglByte_t)1, sizeof(dglInt32_t), (dglInt32_t)0, opaqueset);
    else
        dglInitialize(graph, (dglByte_t)1, (dglInt32_t)0, (dglInt32_t)0, opaqueset);
}

/*!
  \brief Build network graph.

  Internal format for edge costs is integer, costs are multiplied
  before conversion to int by 1000. 
  Costs -1 for infinity i.e. arc or node is closed and cannot be traversed.

  \param graph poiter to graph structure

  \return void
*/
void
Vect_graph_build ( GRAPH *graph )
{
    int ret;

    G_debug (3, "Vect_graph_build()" ); 

    ret = dglFlatten ( graph );
    if ( ret < 0 )
      G_fatal_error (_("GngFlatten error"));
}

/*!
  \brief Add edge to graph. 
  
  Internal format for edge costs is integer, costs are multiplied
  before conversion to int by 1000.  Costs -1 for infinity i.e. arc or
  node is closed and cannot be traversed.
  
  \param graph poiter to graph structure
  \param from from node
  \param to to node
  \param costs costs value
 \param id edge id
 
 \return void
*/
void
Vect_graph_add_edge ( GRAPH *graph, int from, int to, double costs, int id  )
{
    int ret;
    dglInt32_t dglcosts;
    
    G_debug (3, "Vect_add_edge() from = %d to = %d, costs = %f, id = %d", from, to, costs, id ); 

    dglcosts = (dglInt32_t) costs * 1000;
	    
    ret = dglAddEdge ( graph, (dglInt32_t)from, (dglInt32_t)to, dglcosts, (dglInt32_t)id );
    if ( ret < 0 )
	G_fatal_error (_("Unable to add network arc"));
}

/*!
  \brief Set node costs
  
  Internal format for edge costs is integer, costs are multiplied
  before conversion to int by 1000.  Costs -1 for infinity i.e. arc or
  node is closed and cannot be traversed.
 
  \param graph poiter to graph structure
  \param node node id
  \param costs costs value

  \return void
*/
void
Vect_graph_set_node_costs ( GRAPH *graph, int node, double costs )
{
    dglInt32_t dglcosts;
    
    /* TODO: Not tested! */
    G_debug (3, "Vect_graph_set_node_costs()" ); 

    dglcosts = (dglInt32_t) costs * 1000;
	    
    dglNodeSet_Attr(graph, dglGetNode(graph, (dglInt32_t)node), &dglcosts); 
}

/*!
  \brief Find shortest path.
  
  Costs for 'from' and 'to' nodes are not considered (SP found even if
  'from' or 'to' are 'closed' (costs = -1) and costs of these
  nodes are not added to SP costs result.
  
  \param graph pointer to graph structure
  \param from from node
  \param to to node
  \param List list of line ids
  \param cost costs value

  \return number of segments
  \return 0 is correct for from = to, or List == NULL ), ? sum of costs is better return value,
  \return -1 destination unreachable
*/
int
Vect_graph_shortest_path ( GRAPH *graph, int from, int to, struct ilist *List, double *cost ) 
{
    int i, line, *pclip, cArc, nRet;
    dglSPReport_s * pSPReport;
    dglInt32_t nDistance;

    G_debug (3, "Vect_graph_shortest_path(): from = %d, to = %d", from, to ); 
    
    /* Note : if from == to dgl goes to nearest node and returns back (dgl feature) => 
    *         check here for from == to */
    
    if ( List != NULL ) Vect_reset_list ( List);

    /* Check if from and to are identical, otherwise dglib returns path to neares node and back! */
    if ( from == to ) {
	if ( cost != NULL ) *cost = 0;
        return 0;
    }

    From_node = from;

    pclip = NULL;
    if ( List != NULL ) {
        nRet = dglShortestPath ( graph, &pSPReport, (dglInt32_t)from, (dglInt32_t)to, clipper, pclip, NULL);
    } else {
        nRet = dglShortestDistance ( graph, &nDistance, (dglInt32_t)from, (dglInt32_t)to, clipper, pclip, NULL);
    }

    if ( nRet == 0 ) {
	if ( cost != NULL )
	   *cost = PORT_DOUBLE_MAX;
	return -1;
    }
    else if ( nRet < 0 ) {
        G_warning(_("dglShortestPath error: %s"), dglStrerror( graph ) );
	return -1;
    }
    
    if ( List != NULL ) {
	for( i = 0 ; i < pSPReport->cArc ; i ++ ) {
	    line = dglEdgeGet_Id(graph, pSPReport->pArc[i].pnEdge);
	    G_debug( 2, "From %ld to %ld - cost %ld user %d distance %ld" ,
			  pSPReport->pArc[i].nFrom,
			  pSPReport->pArc[i].nTo,
	                  /* this is the cost from clip() */
			  dglEdgeGet_Cost(graph, pSPReport->pArc[i].pnEdge) / 1000, 
			  line,
			  pSPReport->pArc[i].nDistance );
	    Vect_list_append ( List, line );
	}
    }

    if ( cost != NULL ) {
        if ( List != NULL ) 
	    *cost = (double) pSPReport->nDistance /  1000;
	else    
	    *cost = (double) nDistance / 1000;
    }
	
    if ( List != NULL ) {
        cArc = pSPReport->cArc;
        dglFreeSPReport( graph, pSPReport );
    } else
	cArc = 0;

    return (cArc);
}
