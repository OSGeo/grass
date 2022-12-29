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

/*
 * Source best viewed with tabstop=4
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "../type.h"
#include "../graph.h"

#include "opt.h"

extern int errno;


#define NROWS 600
#define NCOLS 100
#define FACTOR 10000
#define BIDIRECTIONAL 1

/*
   #define NROWS 600
   #define NCOLS 400
   #define FACTOR 10000
   #define BIDIRECTIONAL 1
 */


static int _add_edge(dglGraph_s * pgraph,
		     dglInt32_t from, dglInt32_t to, dglInt32_t cost,
		     dglInt32_t arc, char chDirection, int fLast)
{
    int nret;

    dglInt32_t xyz_from[3], xyz_to[3], direction[2];

    if (!fLast) {
	/* setup node attributes with the x y coords in the grid by
	   reversing the calculation done the main loop */
	xyz_from[0] = from % NCOLS;
	xyz_from[1] = from / NCOLS;
	xyz_from[2] = 0;

	xyz_to[0] = to % NCOLS;
	xyz_to[1] = to / NCOLS;
	xyz_to[2] = 0;

	/* chDirection says if the edge direction is 'u'=toward the top 'b'=the bot. 'l'=the left 'r'=the right
	   'o'=toward right-bottom 'O'=toward top-left * Account for this in the edge attributes */
	direction[0] = (dglInt32_t) chDirection;
	direction[1] = 0;


	nret =
	    dglAddEdgeX(pgraph, from, to, cost, arc, xyz_from, xyz_to,
			direction, 0);
	if (nret < 0) {
	    fprintf(stderr, "dglAddEdge error: %s\n", dglStrerror(pgraph));
	    return 1;
	}
    }

    if ((arc % 1024) == 0 || fLast) {
#ifndef DGL_STATS
	printf("add arc %07ld - from %07ld - to %07ld - cost %07ld\r",
	       arc, from, to, cost);
#else
	printf
	    ("add arc %07ld - from %07ld - to %07ld - cost %07ld . Clock: tot %09ld nt %09ld o %09ld\r",
	     arc, from, to, cost, pgraph->clkAddEdge / pgraph->cAddEdge,
	     pgraph->clkNodeTree / pgraph->cAddEdge,
	     (pgraph->clkAddEdge - pgraph->clkNodeTree) / pgraph->cAddEdge);
#endif
	fflush(stdout);
    }
    return 0;
}

int main(int argc, char **argv)
{
    dglGraph_s graph;
    int nret, fd;
    int version;

    int irow, icol, itrow, itcol;
    int irowsave, icolsave;

    dglInt32_t from, to, arc, cost;

    /* node attributes */
    dglInt32_t xyz[3];

    /* edge attributes */
    dglInt32_t direction[2];

    dglInt32_t opaqueset[16] = {
	FACTOR, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /* program options
     */
    char *pszFileout;
    Boolean fInterlaced;
    char *pszVersion;

    GNO_BEGIN			/* short   long                default     variable        help */
	GNO_OPTION("g", "graph", NULL, &pszFileout, "Output Graph file")
	GNO_SWITCH("i", "interlaced", False, &fInterlaced,
		   "Avoid node ids sorting at insertion - default False")
	GNO_OPTION("v", "version", "1", &pszVersion,
		   "Output Graph Version {1,2,3} - default 1")
	GNO_END if (GNO_PARSE(argc, argv) < 0) {
	return 1;
    }

    if (pszFileout == NULL) {
	GNO_HELP("Incomplete parameters");
	return 1;
    }

    /*
     * options parsed
     */
    version = atoi(pszVersion);

    /*
     * new API call
     */
    printf("graph initialize...");
    fflush(stdout);
    dglInitialize(&graph,	/* graph context to initialize */
		  version, sizeof(xyz),	/* node attributes size */
		  sizeof(direction),	/* edge attributes size */
		  opaqueset	/* opaque graph parameters */
	);
    printf("done.\n");

#if 0
    dglSet_Options(&graph, DGL_GO_EdgePrioritize_COST);
#endif

    from = 0;
    to = 0;
    arc = 0;


    printf("add horizontal and vertical edges...\n");
    for (irow = 0; irow < NROWS; irow++) {

	if (fInterlaced == True) {
	    irowsave = irow;
	    if (irow % 2)
		irow = NROWS - irow;
	}

	for (icol = 0; icol < NCOLS; icol++) {

	    if (fInterlaced == True) {
		icolsave = icol;
		if (icol % 2)
		    icol = NCOLS - icol;
	    }

	    itcol = icol + 1;
	    itrow = irow + 1;

	    if (itcol < NCOLS) {
		from = irow * NCOLS + icol;
		to = irow * NCOLS + itcol;
		cost = FACTOR;
		arc++;
		if (_add_edge(&graph, from, to, cost, arc, 'r', 0)) {
		    return 1;
		}
#ifdef BIDIRECTIONAL
		arc++;
		if (_add_edge(&graph, to, from, cost, arc, 'l', 0)) {
		    return 1;
		}
#endif
	    }

	    if (itrow < NROWS) {
		from = irow * NCOLS + icol;
		to = itrow * NCOLS + icol;
		cost = FACTOR;
		arc++;
		if (_add_edge(&graph, from, to, cost, arc, 'b', 0)) {
		    return 1;
		}
#ifdef BIDIRECTIONAL
		arc++;
		if (_add_edge(&graph, to, from, cost, arc, 't', 0)) {
		    return 1;
		}
#endif
	    }

	    if (fInterlaced == True)
		icol = icolsave;
	}

	if (fInterlaced == True)
	    irow = irowsave;
    }
    _add_edge(&graph, to, from, cost, arc, 'x', 1);


#if 1
    printf("\nadd oblique edges...\n");
    for (irow = 0; irow < NROWS; irow++) {
	for (icol = 0; icol < NCOLS; icol++) {
	    itcol = icol + 1;
	    itrow = irow + 1;

	    if (itrow < NROWS && itcol < NCOLS) {
		from = irow * NCOLS + icol;
		to = itrow * NCOLS + itcol;
		cost = (dglInt32_t) (sqrt(2.0) * (double)FACTOR);
		arc++;
		if (_add_edge(&graph, from, to, cost, arc, 'o', 0)) {
		    return 1;
		}
#ifdef BIDIRECTIONAL
		arc++;
		if (_add_edge(&graph, to, from, cost, arc, 'O', 0)) {
		    return 1;
		}
#endif
	    }
	}
    }
    _add_edge(&graph, to, from, cost, arc, 'x', 1);
    printf("\ndone.\n");
#endif


#if 0
    {
	dglEdgeTraverser_s t;
	dglInt32_t *pEdge;

	nret =
	    dglEdge_T_Initialize(&graph, &t, dglGet_EdgePrioritizer(&graph));
	/*nret = dglEdge_T_Initialize(& graph, & t, NULL); */
	if (nret < 0) {
	    fprintf(stderr, "\ndglEdge_T_Initialize error: %s\n",
		    dglStrerror(&graph));
	    return 1;
	}
	for (pEdge = dglEdge_T_First(&t); pEdge; pEdge = dglEdge_T_Next(&t)) {
	    printf("edge: id=%ld cost=%ld\n",
		   dglEdgeGet_Id(&graph, pEdge),
		   dglEdgeGet_Cost(&graph, pEdge));
	}
	dglEdge_T_Release(&t);
    }
#endif


    printf("graph flattening...");
    fflush(stdout);
    nret = dglFlatten(&graph);
    if (nret < 0) {
	fprintf(stderr, "\ndglFlatten error: %s\n", dglStrerror(&graph));
	return 1;
    }
    printf("done.\n");


    printf("graph write...");
    fflush(stdout);
    if ((fd = open(pszFileout, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
	perror("open");
	return 1;
    }
    nret = dglWrite(&graph, fd);
    if (nret < 0) {
	fprintf(stderr, "\ndglWrite error: %s\n", dglStrerror(&graph));
	return 1;
    }
    close(fd);
    printf("done.\n");


    printf("graph release...");
    fflush(stdout);
    dglRelease(&graph);
    printf("program finished.\n");
    return 0;
}
