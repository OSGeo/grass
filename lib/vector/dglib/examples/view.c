/* LIBDGL -- a Directed Graph Library implementation
 * Copyright (C) 2002 Roberto Micarelli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* best view tabstop=4
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#include "../type.h"
#include "../graph.h"

#include "opt.h"


extern int errno;

#define _EDGESET_OFFSET(pg,pl) ((int)(pl) - (int)(pg)->pEdgeBuffer)

static int _print_node(dglGraph_s * pgraph, dglInt32_t * pnode, void *pvarg)
{
    FILE *f = (FILE *) pvarg;
    dglInt32_t *pedgeset;
    dglInt32_t *pedge;
    dglInt32_t *ptonode;
    dglInt32_t *pnattr;
    int iAttr, cAttr;
    int role;
    int i;
    dglEdgesetTraverser_s edgeaT;

    role = 0;
    if (dglNodeGet_Status(pgraph, pnode) & DGL_NS_HEAD) {
	role |= 1;
    }
    if (dglNodeGet_Status(pgraph, pnode) & DGL_NS_TAIL) {
	role |= 2;
    }

    fprintf(f, "HEAD %-8ld - %-s",
	    dglNodeGet_Id(pgraph, pnode),
	    (role > 2) ? "'H/T'" : (role == 2) ? "'T  '" : (role ==
							    1) ? "'H  '" :
	    "'A  '");

    if ((cAttr = dglGet_NodeAttrSize(pgraph)) > 0) {
	pnattr = dglNodeGet_Attr(pgraph, pnode);
	fprintf(f, " - HEAD ATTR [");
	for (iAttr = 0; iAttr < cAttr; iAttr++) {
	    if (iAttr && !(iAttr % 4))
		fprintf(f, " ");
	    fprintf(f, "%02x", ((unsigned char *)pnattr)[iAttr]);
	}
	fprintf(f, "]\n");
    }
    else {
	fprintf(f, "\n");
    }

    if (role & 1) {
	pedgeset = dglNodeGet_OutEdgeset(pgraph, pnode);

	dglEdgeset_T_Initialize(&edgeaT, pgraph, pedgeset);
	for (i = 0, pedge = dglEdgeset_T_First(&edgeaT);
	     pedge; i++, pedge = dglEdgeset_T_Next(&edgeaT)
	    ) {
	    ptonode = dglEdgeGet_Tail(pgraph, pedge);

	    if (ptonode) {
		role = 0;
		if (dglNodeGet_Status(pgraph, ptonode) & DGL_NS_HEAD) {
		    role |= 1;
		}
		if (dglNodeGet_Status(pgraph, ptonode) & DGL_NS_TAIL) {
		    role |= 2;
		}

		fprintf(f,
			"EDGE #%-8d: TAIL %-8ld - %-s - COST %-8ld - ID %-8ld",
			i, dglNodeGet_Id(pgraph, ptonode),
			(role > 2) ? "'H/T'" : (role ==
						2) ? "'T  '" : (role ==
								1) ? "'H  '" :
			"'A  '", dglEdgeGet_Cost(pgraph, pedge),
			dglEdgeGet_Id(pgraph, pedge)
		    );

		if ((cAttr = dglGet_NodeAttrSize(pgraph)) > 0) {
		    pnattr = dglNodeGet_Attr(pgraph, ptonode);
		    fprintf(f, " - TAIL ATTR [");
		    for (iAttr = 0; iAttr < cAttr; iAttr++) {
			if (iAttr && !(iAttr % 4))
			    fprintf(f, " ");
			fprintf(f, "%02x", ((unsigned char *)pnattr)[iAttr]);
		    }
		    fprintf(f, "]");
		}

		if ((cAttr = dglGet_EdgeAttrSize(pgraph)) > 0) {
		    pnattr = dglEdgeGet_Attr(pgraph, pedge);
		    fprintf(f, " - EDGE ATTR [");
		    for (iAttr = 0; iAttr < cAttr; iAttr++) {
			if (iAttr && !(iAttr % 4))
			    fprintf(f, " ");
			fprintf(f, "%02x", ((unsigned char *)pnattr)[iAttr]);
		    }
		    fprintf(f, "]\n");
		}
		else {
		    fprintf(f, "\n");
		}
	    }
	}
	dglEdgeset_T_Release(&edgeaT);
    }
    return 0;
}

int main(int argc, char **argv)
{
    dglGraph_s graph;
    int fd;
    int nret;

    /* program options
     */
    char *pszFilein;

    GNO_BEGIN			/* short  long        default     variable        help */
	GNO_OPTION("g", "graph", NULL, &pszFilein, "Graph file to view")
	GNO_END if (GNO_PARSE(argc, argv) < 0) {
	return 1;
    }
    /*
     * options parsed
     */

    if (pszFilein == NULL) {
	GNO_HELP("Incomplete parameters");
	return 1;
    }

    fd = open(pszFilein, O_RDONLY);
    if (fd < 0) {
	perror("open");
	return 1;
    }

    nret = dglRead(&graph, fd);

    if (nret < 0) {
	fprintf(stderr, "dglRead error: %s\n", dglStrerror(&graph));
	return 1;
    }

    close(fd);

    /* print the header
     */
    fprintf(stdout, "Version: %d\n", graph.Version);
    fprintf(stdout, "Byte Order: %s\n",
	    (graph.Endian ==
	     DGL_ENDIAN_LITTLE) ? "Little Endian" : "Big Endian");
    fprintf(stdout, "Node Attribute Size:  %ld\n", graph.NodeAttrSize);
    fprintf(stdout, "Edge Attribute Size:  %ld\n", graph.EdgeAttrSize);
    fprintf(stdout,
	    "Counters:  %ld Edges - %ld Nodes: %ld HEAD / %ld TAIL / %ld ALONE\n",
	    graph.cEdge, graph.cNode, graph.cHead, graph.cTail, graph.cAlone);
    fprintf(stdout, "Opaque Settings:\n");
    fprintf(stdout, "%10ld %10ld %10ld %10ld\n",
	    graph.aOpaqueSet[0], graph.aOpaqueSet[1],
	    graph.aOpaqueSet[2], graph.aOpaqueSet[3]);
    fprintf(stdout, "%10ld %10ld %10ld %10ld\n",
	    graph.aOpaqueSet[4], graph.aOpaqueSet[5],
	    graph.aOpaqueSet[6], graph.aOpaqueSet[7]);
    fprintf(stdout, "%10ld %10ld %10ld %10ld\n",
	    graph.aOpaqueSet[8], graph.aOpaqueSet[9],
	    graph.aOpaqueSet[10], graph.aOpaqueSet[11]);
    fprintf(stdout, "%10ld %10ld %10ld %10ld\n",
	    graph.aOpaqueSet[12], graph.aOpaqueSet[13],
	    graph.aOpaqueSet[14], graph.aOpaqueSet[15]);
    fprintf(stdout, "Total Cost: %lld\n", graph.nnCost);
    fprintf(stdout, "--\n");


    {
	dglInt32_t *pnode;
	dglNodeTraverser_s traverser;

	dglNode_T_Initialize(&traverser, &graph);
	for (pnode = dglNode_T_First(&traverser); pnode;
	     pnode = dglNode_T_Next(&traverser)) {
	    _print_node(&graph, pnode, stdout);
	}
	dglNode_T_Release(&traverser);
    }

    printf("\n");
    dglRelease(&graph);
    return 0;

}
