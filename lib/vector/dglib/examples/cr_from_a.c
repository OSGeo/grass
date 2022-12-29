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
#include <string.h>
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



int main(int argc, char **argv)
{
    FILE *fp;
    dglGraph_s graph;
    char sz[1024];
    char c;
    int nret, fd;
    int version, attrsize;
    dglInt32_t nodeid, from, to, cost, user, xyz[3];
    dglInt32_t opaqueset[16] = {
	360000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /* program options
     */
    char *pszFilein;
    char *pszFileout;

    GNO_BEGIN			/* short  long        default     variable        help */
	GNO_OPTION("f", "file", NULL, &pszFilein,
		   "Input Graph definition file")
	GNO_OPTION("g", "graph", NULL, &pszFileout, "Output Graph file")
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

    if ((fp = fopen(pszFilein, "r")) == NULL) {
	perror("fopen");
	return 1;
    }

  reread_first_line:
    if (fgets(sz, sizeof(sz), fp) == NULL) {
	fprintf(stderr, "unexpected EOF\n");
	return 1;
    }

    if (sz[0] == '#' || strlen(sz) == 0)
	goto reread_first_line;

    sscanf(sz, "%d %d", &version, &attrsize);

    /*
     * initialize the graph
     */
    dglInitialize(&graph,	/* graph context to initialize */
		  version,	/* version */
		  sizeof(xyz),	/* node attributes size */
		  0,		/* edge attributes size */
		  opaqueset	/* opaque graph parameters */
	);

    /*
     * generate edge cost prioritizing
     */
    dglSet_Options(&graph, DGL_GO_EdgePrioritize_COST);


    /* add arcs and X/Y/Z node attributes
     */
    while (fgets(sz, sizeof(sz), fp) != NULL) {
	if (sz[0] == '#')
	    continue;

	if (strlen(sz) == 0)
	    continue;

	if (sz[0] == 'A') {	/* add a edge */
	    sscanf(sz, "%c %ld %ld %ld %ld", &c, &from, &to, &cost, &user);

	    nret = dglAddEdge(&graph, from, to, cost, user);


	    if (nret < 0) {
		fprintf(stderr, "dglAddArc error: %s\n", dglStrerror(&graph));
		return 1;
	    }
	}
	else if (sz[0] == 'V') {	/* add a node */
	    sscanf(sz, "%c %ld", &c, &nodeid);

	    printf("add node: %ld\n", nodeid);

	    nret = dglAddNode(&graph, nodeid, NULL, 0);

	    if (nret < 0) {
		fprintf(stderr, "dglAddNode error: %s\n",
			dglStrerror(&graph));
		return 1;
	    }
	}
	else if (sz[0] == 'N') {	/* set attributes for a (already inserted) node */
	    sscanf(sz, "%c %ld %ld %ld %ld", &c, &nodeid, &xyz[0], &xyz[1],
		   &xyz[2]);

	    dglNodeSet_Attr(&graph, dglGetNode(&graph, nodeid), xyz);
	}
    }
    fclose(fp);


#if 0				/* show edges */
    {
	dglEdgeTraverser_s t;
	dglInt32_t *pEdge;

	nret =
	    dglEdge_T_Initialize(&t, &graph, dglGet_EdgePrioritizer(&graph));
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

    /*
     * flatten the graph (make it serializable)
     */
    nret = dglFlatten(&graph);

    if (nret < 0) {
	fprintf(stderr, "dglFlatten error: %s\n", dglStrerror(&graph));
	return 1;
    }

    /*
     * store the graph
     */
    if ((fd = open(pszFileout, O_WRONLY | O_CREAT, 0666)) < 0) {
	perror("open");
	return 1;
    }

    nret = dglWrite(&graph, fd);

    if (nret < 0) {
	fprintf(stderr, "dglWrite error: %s\n", dglStrerror(&graph));
	return 1;
    }

    close(fd);

    /*
     * finish
     */
    dglRelease(&graph);

    return 0;
}
