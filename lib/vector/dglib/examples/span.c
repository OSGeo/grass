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

static int _clipper(dglGraph_s * pgraphIn,
		    dglGraph_s * pgraphOut,
		    dglSpanClipInput_s * pArgIn,
		    dglSpanClipOutput_s * pArgOut, void *pvArg)
{
    return 0;
}

int main(int argc, char **argv)
{
    dglGraph_s graph, graphOut;
    dglInt32_t nVertex;
    int nret, fd;

    /* program options
     */
    char *pszGraph;
    char *pszGraphOut;
    char *pszVertex;

    GNO_BEGIN			/* short   long                default     variable        help */
	GNO_OPTION("g", "graph", NULL, &pszGraph, "Input Graph file")
	GNO_OPTION("o", "graphout", NULL, &pszGraphOut, "Output Graph file")
	GNO_OPTION("v", "vertex", NULL, &pszVertex, "Vertex Node Id")
	GNO_END if (GNO_PARSE(argc, argv) < 0) {
	return 1;
    }
    /*
     * options parsed
     */

    if (pszVertex == NULL) {
	GNO_HELP("span usage");
	return 1;
    }
    nVertex = atol(pszVertex);

    printf("Graph read:\n");
    if ((fd = open(pszGraph, O_RDONLY)) < 0) {
	perror("open");
	return 1;
    }
    nret = dglRead(&graph, fd);
    if (nret < 0) {
	fprintf(stderr, "dglRead error: %s\n", dglStrerror(&graph));
	return 1;
    }
    close(fd);
    printf("Done.\n");

    printf("Graph depth spanning:\n");
    nret = dglDepthSpanning(&graph, &graphOut, nVertex, _clipper, NULL);
    if (nret < 0) {
	fprintf(stderr, "dglDepthSpanning error: %s\n", dglStrerror(&graph));
	return 1;
    }
    printf("Done.\n");


    printf("Graph flatten:\n");
    nret = dglFlatten(&graphOut);
    printf("Done.\n");

    if (dglGet_EdgeCount(&graphOut) > 0) {


	if (pszGraphOut) {
	    printf("Graph write:\n");
	    if ((fd =
		 open(pszGraphOut, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
		perror("open");
		return 1;
	    }
	    dglWrite(&graphOut, fd);
	    if (nret < 0) {
		fprintf(stderr, "dglWrite error: %s\n",
			dglStrerror(&graphOut));
		return 1;
	    }
	    close(fd);
	    printf("Done.\n");
	}
    }
    else {
	printf("Empty span. No output produced.\n");
    }

    dglRelease(&graph);
    dglRelease(&graphOut);
    return 0;
}
