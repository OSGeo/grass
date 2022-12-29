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

#include <grass/config.h>
#include <stdio.h>
#include <regex.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "opt.h"
#include "../type.h"
#include "../graph.h"

static void _regmtostring(char *pszOut, int cszOut, char *pszIn,
			  regmatch_t * pregm)
{
    int i, iout;

    for (iout = 0, i = pregm->rm_so; i < pregm->rm_eo && iout < cszOut - 1;
	 i++) {
	if (i >= 0)
	    pszOut[iout++] = pszIn[i];
    }
    pszOut[iout] = 0;
}

static int _sztoattr(unsigned char *pbNodeAttr, int cbNodeAttr, char *szw)
{
    int i, ib;

    for (ib = 0, i = 0; szw[i] && ib < cbNodeAttr; i++) {
	if (szw[i] == ' ')
	    continue;
	pbNodeAttr[ib] = (szw[i] >= '0' &&
			  szw[i] <= '9') ? (szw[i] - '0') * 16 : (szw[i] >=
								  'A' &&
								  szw[i] <=
								  'F') ? (10 +
									  (szw
									   [i]
									   -
									   'A'))
	    * 16 : (szw[i] >= 'a' &&
		    szw[i] <= 'f') ? (10 + (szw[i] - 'a')) * 16 : 0;
	i++;
	if (szw[i]) {
	    pbNodeAttr[ib] += (szw[i] >= '0' &&
			       szw[i] <= '9') ? (szw[i] - '0') : (szw[i] >=
								  'A' &&
								  szw[i] <=
								  'F') ? (10 +
									  (szw
									   [i]
									   -
									   'A'))
		: (szw[i] >= 'a' &&
		   szw[i] <= 'f') ? (10 + (szw[i] - 'a')) : 0;
	}
	ib++;
    }
    return ib;
}

int main(int argc, char **argv)
{
    FILE *fp;
    char sz[1024];
    char szw[1024];
    int nret;
    regmatch_t aregm[64];
    dglInt32_t nVersion;
    dglInt32_t nNodeAttrSize;
    dglInt32_t nEdgeAttrSize;
    dglInt32_t anOpaque[16];
    int i, fd, cOut;

    regex_t reVersion;
    regex_t reByteOrder;
    regex_t reNodeAttrSize;
    regex_t reEdgeAttrSize;
    regex_t reCounters;
    regex_t reOpaque;
    regex_t reNodeFrom;
    regex_t reNodeAttr;
    regex_t reEdge;
    regex_t reToNodeAttr;
    regex_t reEdgeAttr;


    dglInt32_t nNodeFrom, nNodeTo, nUser, nCost;

    int fInOpaque;
    int fInBody;

    unsigned char *pbNodeAttr, *pbEdgeAttr, *pbToNodeAttr;

    struct stat statdata;

    dglGraph_s graphOut;

    /* program options
     */
    char *pszFilein;
    char *pszGraphout;

    GNO_BEGIN			/* short  long        default     variable        help */
	GNO_OPTION("i", "input", NULL, &pszFilein, "Input text file")
	GNO_OPTION("o", "output", NULL, &pszGraphout, "Output graph file")
	GNO_END if (GNO_PARSE(argc, argv) < 0) {
	return 1;
    }
    /*
     * options parsed
     */

    if (pszFilein == NULL) {
	GNO_HELP("... usage");
	return 1;
    }

    /*
     * compile header expressions
     */
    printf("Compile header expressions...");
    fflush(stdout);
    i = 0;
    if (regcomp(&reVersion, "^Version:[ ]+([0-9]+)", REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    if (regcomp(&reByteOrder, "^Byte Order:[ ]+(.+)", REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    if (regcomp
	(&reNodeAttrSize, "^Node Attribute Size:[ ]+([0-9]+)",
	 REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    if (regcomp
	(&reEdgeAttrSize, "^Edge Attribute Size:[ ]+([0-9]+)",
	 REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    if (regcomp(&reCounters, "^Counters:[ ]+.*", REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    if (regcomp(&reOpaque, "^Opaque Settings:", REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    printf("done.\n");

    /*
     * compile body expressions
     */

    printf("Compile body expressions...");
    fflush(stdout);
    if (regcomp(&reNodeFrom, "^HEAD ([0-9]+)[ ]*- [HT/']+", REG_EXTENDED) !=
	0)
	goto regc_error;
    i++;
    if (regcomp(&reNodeAttr, ".*HEAD ATTR [[]([0-9a-fA-F ]+)]", REG_EXTENDED)
	!= 0)
	goto regc_error;
    i++;

    if (regcomp
	(&reEdge,
	 "^EDGE #([0-9]+)[ ]*: TAIL ([0-9]+)[ ]*- [HT/']+[ ]+- COST ([0-9]+)[ ]*- ID ([0-9]+)",
	 REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    if (regcomp
	(&reToNodeAttr, ".*TAIL ATTR [[]([0-9a-fA-F ]+)]", REG_EXTENDED) != 0)
	goto regc_error;
    i++;
    if (regcomp(&reEdgeAttr, ".*EDGE ATTR [[]([0-9a-fA-F ]+)]", REG_EXTENDED)
	!= 0)
	goto regc_error;
    i++;

    printf("done.\n");

    goto regc_ok;




  regc_error:
    fprintf(stderr, "regex compilation error %d\n", i);
    exit(1);





  regc_ok:

    if ((fp = fopen(pszFilein, "r")) == NULL) {
	perror("fopen");
	return 1;
    }

    fstat(fileno(fp), &statdata);

    fInOpaque = 0;
    fInBody = 0;

    nNodeAttrSize = 0;
    nEdgeAttrSize = 0;
    pbNodeAttr = NULL;
    pbToNodeAttr = NULL;
    pbEdgeAttr = NULL;

    cOut = 0;

    while (fgets(sz, sizeof(sz), fp)) {
#ifndef VERBOSE
	if (!(cOut++ % 512) || G_ftell(fp) == statdata.st_size)
	    printf("Parse input file ... status: %ld/%ld\r", G_ftell(fp),
		   statdata.st_size);
	fflush(stdout);
#endif

#ifdef VERYVERBOSE
	printf("<<<%s>>>\n", sz);
#endif
	if (fInOpaque == 0 && fInBody == 0) {
	    if (regexec(&reVersion, sz, 64, aregm, 0) == 0) {
		_regmtostring(szw, sizeof(szw), sz, &aregm[1]);
		nVersion = atoi(szw);
#ifdef VERYVERBOSE
		printf("-- version %d\n", nVersion);
#endif
	    }
	    else if (regexec(&reByteOrder, sz, 64, aregm, 0) == 0) {
	    }
	    else if (regexec(&reNodeAttrSize, sz, 64, aregm, 0) == 0) {
		_regmtostring(szw, sizeof(szw), sz, &aregm[1]);
		nNodeAttrSize = atoi(szw);
		if (nNodeAttrSize) {
		    pbNodeAttr = (unsigned char *)malloc(nNodeAttrSize);
		    if (pbNodeAttr == NULL) {
			fprintf(stderr, "Memory Exhausted\n");
			exit(1);
		    }
		    pbToNodeAttr = (unsigned char *)malloc(nNodeAttrSize);
		    if (pbToNodeAttr == NULL) {
			fprintf(stderr, "Memory Exhausted\n");
			exit(1);
		    }
		}
#ifdef VERYVERBOSE
		printf("-- node attr size %d\n", nNodeAttrSize);
#endif
	    }
	    else if (regexec(&reEdgeAttrSize, sz, 64, aregm, 0) == 0) {
		_regmtostring(szw, sizeof(szw), sz, &aregm[1]);
		nEdgeAttrSize = atoi(szw);
		if (nEdgeAttrSize > 0) {
		    pbEdgeAttr = (unsigned char *)malloc(nEdgeAttrSize);
		    if (pbEdgeAttr == NULL) {
			fprintf(stderr, "Memory Exhausted\n");
			exit(1);
		    }
		}
#ifdef VERYVERBOSE
		printf("-- edge attr size %d\n", nEdgeAttrSize);
#endif
	    }
	    else if (regexec(&reOpaque, sz, 64, aregm, 0) == 0) {
#ifdef VERYVERBOSE
		printf("-- opaque...\n");
#endif
		fInOpaque = 1;
	    }
	    else if (strncmp(sz, "--", 2) == 0) {
		nret = dglInitialize(&graphOut,
				     nVersion,
				     nNodeAttrSize, nEdgeAttrSize, anOpaque);
		if (nret < 0) {
		    fprintf(stderr, "dglInitialize error %s\n",
			    dglStrerror(&graphOut));
		    exit(1);
		}
#ifdef VERBOSE
		printf("Initialize: Version=%ld NodeAttr=%ld EdgeAttr=%ld\n",
		       nVersion, nNodeAttrSize, nEdgeAttrSize);
#endif
		fInBody = 1;
	    }
	}
	else if (fInOpaque > 0 && fInBody == 0) {
	    if (fInOpaque == 1) {
		sscanf(sz, "%ld %ld %ld %ld",
		       &anOpaque[0],
		       &anOpaque[1], &anOpaque[2], &anOpaque[3]);
		fInOpaque++;
#ifdef VERYVERBOSE
		printf("opaque 1: %ld %ld %ld %ld\n",
		       anOpaque[0], anOpaque[1], anOpaque[2], anOpaque[3]);
#endif

	    }
	    else if (fInOpaque == 2) {
		sscanf(sz, "%ld %ld %ld %ld",
		       &anOpaque[4],
		       &anOpaque[5], &anOpaque[6], &anOpaque[7]);
#ifdef VERYVERBOSE
		printf("opaque 2: %ld %ld %ld %ld\n",
		       anOpaque[4], anOpaque[5], anOpaque[6], anOpaque[7]);
#endif
		fInOpaque++;
	    }
	    else if (fInOpaque == 3) {
		sscanf(sz, "%ld %ld %ld %ld",
		       &anOpaque[8],
		       &anOpaque[9], &anOpaque[10], &anOpaque[11]);
#ifdef VERYVERBOSE
		printf("opaque 3: %ld %ld %ld %ld\n",
		       anOpaque[8], anOpaque[9], anOpaque[10], anOpaque[11]);
#endif
		fInOpaque++;
	    }
	    else if (fInOpaque == 4) {
		sscanf(sz, "%ld %ld %ld %ld",
		       &anOpaque[12],
		       &anOpaque[13], &anOpaque[14], &anOpaque[15]);
#ifdef VERYVERBOSE
		printf("opaque 4: %ld %ld %ld %ld\n",
		       anOpaque[12],
		       anOpaque[13], anOpaque[14], anOpaque[15]);
#endif
		fInOpaque = 0;
	    }
	}
	else if (fInBody == 1) {
	    if (regexec(&reNodeFrom, sz, 64, aregm, 0) == 0) {
		_regmtostring(szw, sizeof(szw), sz, &aregm[1]);
#ifdef VERYVERBOSE
		printf("node from snippet = %s\n", szw);
#endif
		nNodeFrom = atol(szw);
		if (nNodeAttrSize > 0) {
		    if (regexec(&reNodeAttr, sz, 64, aregm, 0) == 0) {
			_regmtostring(szw, sizeof(szw), sz, &aregm[1]);
			if (_sztoattr(pbNodeAttr, nNodeAttrSize, szw) !=
			    nNodeAttrSize) {
			    fprintf(stderr, "node attr size mismatch\n");
			}
#ifdef VERYVERBOSE
			{
			    int k;

			    for (k = 0; k < nNodeAttrSize; k++) {
				printf("%02x", pbNodeAttr[k]);
			    }
			    printf("\n");
			}
#endif
		    }
		}
	    }
	    else if (regexec(&reEdge, sz, 64, aregm, 0) == 0) {
		_regmtostring(szw, sizeof(szw), sz, &aregm[2]);
		nNodeTo = atol(szw);
		_regmtostring(szw, sizeof(szw), sz, &aregm[3]);
		nCost = atol(szw);
		_regmtostring(szw, sizeof(szw), sz, &aregm[4]);
		nUser = atol(szw);
		if (nEdgeAttrSize > 0) {
		    if (regexec(&reEdgeAttr, sz, 64, aregm, 0) == 0) {
			_regmtostring(szw, sizeof(szw), sz, &aregm[1]);
			if (_sztoattr(pbEdgeAttr, nEdgeAttrSize, szw) !=
			    nEdgeAttrSize) {
			    fprintf(stderr, "edge attr size mismatch\n");
			}
#ifdef VERYVERBOSE
			{
			    int k;

			    for (k = 0; k < nEdgeAttrSize; k++) {
				printf("%02x", pbEdgeAttr[k]);
			    }
			    printf("\n");
			}
#endif
		    }
		}
		if (nNodeAttrSize > 0) {
		    if (regexec(&reToNodeAttr, sz, 64, aregm, 0) == 0) {
			_regmtostring(szw, sizeof(szw), sz, &aregm[1]);
			if (_sztoattr(pbToNodeAttr, nNodeAttrSize, szw) !=
			    nNodeAttrSize) {
			    fprintf(stderr, "to node attr size mismatch\n");
			}
#ifdef VERYVERBOSE
			{
			    int k;

			    for (k = 0; k < nNodeAttrSize; k++) {
				printf("%02x", pbToNodeAttr[k]);
			    }
			    printf("\n");
			}
#endif
		    }
		}
		nret = dglAddEdgeX(&graphOut,
				   nNodeFrom,
				   nNodeTo,
				   nCost,
				   nUser,
				   pbNodeAttr, pbToNodeAttr, pbEdgeAttr, 0);

		if (nret < 0) {
		    fprintf(stderr, "dglAddEdge error %s\n",
			    dglStrerror(&graphOut));
		    exit(1);
		}
#ifdef VERBOSE
		printf("AddEdge: from=%ld to=%ld cost=%ld user=%ld\n",
		       nNodeFrom, nNodeTo, nCost, nUser);
#endif
	    }
	}
    }
#ifndef VERBOSE
    printf("\ndone.\n");
#endif

    fclose(fp);

    regfree(&reVersion);
    regfree(&reByteOrder);
    regfree(&reNodeAttrSize);
    regfree(&reEdgeAttrSize);
    regfree(&reCounters);
    regfree(&reOpaque);
    regfree(&reNodeFrom);
    regfree(&reNodeAttr);
    regfree(&reEdge);
    regfree(&reToNodeAttr);
    regfree(&reEdgeAttr);

    if (pbNodeAttr)
	free(pbNodeAttr);
    if (pbToNodeAttr)
	free(pbToNodeAttr);
    if (pbEdgeAttr)
	free(pbEdgeAttr);


    printf("Flatten...");
    fflush(stdout);
    nret = dglFlatten(&graphOut);
    if (nret < 0) {
	fprintf(stderr, "dglFlatten error %s\n", dglStrerror(&graphOut));
	exit(1);
    }
    printf("done.\n");

    if (pszGraphout) {
	fd = open(pszGraphout, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
	    perror("open");
	    exit(1);
	}

	printf("Write <%s>...", pszGraphout);
	fflush(stdout);
	nret = dglWrite(&graphOut, fd);
	if (nret < 0) {
	    fprintf(stderr, "dglWrite error %s\n", dglStrerror(&graphOut));
	    exit(1);
	}
	printf("done.\n");
	close(fd);
    }

    printf("Release...");
    fflush(stdout);
    dglRelease(&graphOut);
    printf("done.\n");

    return 0;
}
