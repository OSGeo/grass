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

#include "../type.h"
#include "../graph.h"

#include "opt.h"


extern int errno;

/* If the clipper function returns 1 , the node is discarded and
 * the traversing of the graph toward its direction is abandoned.
 * Try to change the return value to 1 and see that the program
 * will not find any more paths to destinations.
 * The clipper receives data relating the network segment being examined.
 * The pvarg is a user pointer registered to dglShortestPath() and
 * passed back to the clipper.
 * As a demo, the main uses the ClipperContext_s structure to store a nodeid
 * that must be discarded during the search. The clipper receives
 * that pointer and checks every node against the one specified there.
 * If the node matches return 1 , otherwise return 0.
 */

typedef struct
{
    dglInt32_t node_to_discard;
} ClipperContext_s;

static int clipper(dglGraph_s * pgraph, dglSPClipInput_s * pIn, dglSPClipOutput_s * pOut, void *pvarg	/* caller's pointer */
    )
{
    ClipperContext_s *pclip = (ClipperContext_s *) pvarg;

#if 0
    dglInt32_t *pnFromXYZ =
	(dglInt32_t *) dglNodeGet_Attr(pgraph, pIn->pnNodeFrom);
    dglInt32_t *pnToXYZ =
	(dglInt32_t *) dglNodeGet_Attr(pgraph, pIn->pnNodeTo);

    printf("clipper called:\n");
    printf("        from node: %ld - attributes x=%ld y=%ld z=%ld\n",
	   dglNodeGet_Id(pgraph, pIn->pnNodeFrom), pnFromXYZ[0], pnFromXYZ[1],
	   pnFromXYZ[2]);
    printf("        to   node: %ld - attributes x=%ld y=%ld z=%ld\n",
	   dglNodeGet_Id(pgraph, pIn->pnNodeTo), pnToXYZ[0], pnToXYZ[1],
	   pnToXYZ[2]);
    printf("        edge     : %ld\n", dglEdgeGet_Id(pgraph, pIn->pnEdge));
#endif

    if (pclip) {
	if (dglNodeGet_Id(pgraph, pIn->pnNodeTo) == pclip->node_to_discard) {
	    /*
	       printf( "        discarder.\n" );
	     */
	    return 1;
	}
    }
    /*
       printf( "        accepted.\n" );
     */
    return 0;
}



int main(int argc, char **argv)
{
    dglGraph_s graph;
    dglInt32_t from, to;

    int fd, nret, version;
    dglSPReport_s *pReport;
    dglInt32_t nDistance;
    dglSPCache_s spCache;
    ClipperContext_s clipctx, *pclipctx;

    /* program options
     */
    char *pszFilein;
    char *pszFrom;
    char *pszTo;
    char *pszDiscard;
    char *pszVersion;
    Boolean fDistance;
    Boolean fUnflatten;

    GNO_BEGIN			/*short       long        default     variable        help */
	GNO_OPTION("g", "graph", NULL, &pszFilein, "graph file to view")
	GNO_OPTION("v", "version", NULL, &pszVersion, "alter graph version")
	GNO_OPTION("f", "from", NULL, &pszFrom, "from-node id")
	GNO_OPTION("t", "to", NULL, &pszTo, "to-node id")
	GNO_OPTION("d", "discard", NULL, &pszDiscard,
		   "node to discard in clipper")
	GNO_SWITCH("D", "distance", False, &fDistance,
		   "Report shortest distance only")
	GNO_SWITCH("U", "unflatten", False, &fUnflatten,
		   "Unflatten the graph before processing")
	GNO_END if (GNO_PARSE(argc, argv) < 0) {
	return 1;
    }
    /* options parsed
     */

    if (pszFilein == NULL || pszFrom == NULL || pszTo == NULL) {
	GNO_HELP("incomplete parameters");
	return 1;
    }

    if (pszDiscard) {
	clipctx.node_to_discard = atol(pszDiscard);
	pclipctx = &clipctx;
    }
    else
	pclipctx = NULL;

    if (pszVersion) {
	version = atoi(pszVersion);
    }

    fd = open(pszFilein, O_RDONLY);
    if (fd < 0) {
	perror("open");
	return 1;
    }

    nret = dglRead(&graph, fd);

    close(fd);

    if (nret < 0) {
	fprintf(stderr, "dglRead error: %s\n", dglStrerror(&graph));
	return 1;
    }

    if (fUnflatten)
	dglUnflatten(&graph);

    if (pszVersion)
	dglSet_Version(&graph, version);

    from = atol(pszFrom);
    to = atol(pszTo);

    printf("shortest path: from-node %ld - to-node %ld\n\n", from, to);

    dglInitializeSPCache(&graph, &spCache);

    if (fDistance == False) {
	nret =
	    dglShortestPath(&graph, &pReport, from, to, clipper, pclipctx,
			    &spCache);

	if (nret == 0) {
	    printf("destination node is unreachable\n\n");
	}
	else if (nret < 0) {
	    fprintf(stderr, "dglShortestPath error: %s\n",
		    dglStrerror(&graph));
	}
	else {
	    int i;

	    printf
		("shortest path report: total edges %ld - total distance %ld\n",
		 pReport->cArc, pReport->nDistance);

	    for (i = 0; i < pReport->cArc; i++) {
		printf("edge[%d]: from %ld to %ld - travel cost %ld - user edgeid %ld - distance from start node %ld\n", i, pReport->pArc[i].nFrom, pReport->pArc[i].nTo, dglEdgeGet_Cost(&graph, pReport->pArc[i].pnEdge),	/* this is the cost from clip() */
		       dglEdgeGet_Id(&graph, pReport->pArc[i].pnEdge),
		       pReport->pArc[i].nDistance);
	    }
	    dglFreeSPReport(&graph, pReport);
	}
    }
    else {
	nret =
	    dglShortestDistance(&graph, &nDistance, from, to, clipper,
				pclipctx, &spCache);

	if (nret == 0) {
	    if (dglErrno(&graph) == 0) {
		printf("destination node is unreachable\n\n");
	    }
	}
	else if (nret < 0) {
	    fprintf(stderr, "dglShortestDistance error: %s\n",
		    dglStrerror(&graph));
	}
	else {
	    printf("shortest distance: %ld\n", nDistance);
	}
    }

    dglReleaseSPCache(&graph, &spCache);
    dglRelease(&graph);

    return 0;

}
