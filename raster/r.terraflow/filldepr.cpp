/****************************************************************************
 *
 *  MODULE:        r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#include <assert.h>

#include "filldepr.h"
#include "unionFind.h"
#include "common.h"

#ifdef _MSC_VER
#pragma warning(default : 4716)
#endif
#define FLOOD_DEBUG if (0)

/************************************************************/
/* INPUT: stream containing the edgelist of watershed adjacency graph
E={(u,v,h) | 0 <= u,v <= W-1}; W is the maximum number of watersheds
(also counting the outside watershed 0)
elevation_type*

h is the smallest height on the boundary between watershed u and
watershed v;

the outside face is assumed to be watershed number 0

E contains the edges between the watersheds on the boundary and the
outside watershed 0;

E is sorted increasingly by (h,u,v)

OUTPUT: allocate and returns an array raise[1..W-1], raise[i] is the
height to which the watershed i must be raised in order to have a
valid flow path to the outside watershed (raise[0] is 0) */
/************************************************************/

elevation_type *fill_depression(AMI_STREAM<boundaryType> *boundaryStr,
                                cclabel_type maxWatersheds)
{

    if (stats) {
        stats->comment("----------", opt->verbose);
        stats->comment("flooding depressions");
    }

    /* find available memory */
    size_t mem_avail = getAvailableMemory();
    if (opt->verbose)
        MM_manager.print();

    /* find how much memory filling depression uses */
    size_t mem_usage = inmemory_fill_depression_mmusage(maxWatersheds);

    /* decide whether to run it in memory or not */
    if (mem_avail > mem_usage) {
        return inmemory_fill_depression(boundaryStr, maxWatersheds);
    }
    else {
        return ext_fill_depression(boundaryStr, maxWatersheds);
    }
}

/************************************************************/
elevation_type *
ext_fill_depression(AMI_STREAM<boundaryType> *boundaryStr UNUSED,
                    cclabel_type maxWatersheds UNUSED)
{
    G_fatal_error(
        _("Fill_depressions do not fit in memory. Not implemented yet"));
}

/************************************************************/
/* inside the function memory allocation is done with malloc/calloc
and not with new; memory check should be done prior to this function
to decide whether there's enough available memory to run it*/
/************************************************************/
elevation_type *inmemory_fill_depression(AMI_STREAM<boundaryType> *boundaryStr,
                                         cclabel_type maxWatersheds)
{

    assert(boundaryStr && maxWatersheds >= 0);

    /*__________________________________________________________*/
    /* initialize */
    /*__________________________________________________________*/

    /* allocate the raised-elevation array */
    elevation_type *raise = new elevation_type[maxWatersheds];
    assert(raise);

    /*allocate and initialize done; done[i] = true iff watershed i has
      found a flow path to the outside; initially only outside watershed
      is done */
    int *done = (int *)calloc(maxWatersheds, sizeof(int));
    assert(done);
    done[LABEL_BOUNDARY] = 1;

    /*allocate and initialize an union find structure; insert all
      watersheds except for the outside watershed; the outside watershed
      is not in the unionfind structure; */
    unionFind<cclabel_type> unionf;
    FLOOD_DEBUG printf("nb watersheds %d, bstream length %ld\n",
                       (int)maxWatersheds, (long)boundaryStr->stream_len());

    for (cclabel_type i = 1; i < maxWatersheds; i++) {
        FLOOD_DEBUG printf("makeset %d\n", i);
        unionf.makeSet(i);
    }

    /*__________________________________________________________*/
    /*SCAN THE EDGES; invariant---watersheds adjacent to a 'done' watershed
      become done */
    /*__________________________________________________________*/
    AMI_err ae;
    boundaryType *nextedge;
    elevation_type h;
    cclabel_type u, v, ur, vr;
    off_t nitems = boundaryStr->stream_len();
    boundaryStr->seek(0);
    for (size_t i = 0; i < nitems; i++) {

        /*read next edge*/
        ae = boundaryStr->read_item(&nextedge);
        assert(ae == AMI_ERROR_NO_ERROR);
        u = nextedge->getLabel1();
        v = nextedge->getLabel2();
        h = nextedge->getElevation();
        FLOOD_DEBUG
        {
            printf("\nreading edge ((%d,%d),h=%d)\n", (int)u, (int)v, (int)h);
        }

        /*find representatives;  LABEL_BOUNDARY means the outside watershed*/
        (u == LABEL_BOUNDARY) ? ur = LABEL_BOUNDARY : ur = unionf.findSet(u);
        (v == LABEL_BOUNDARY) ? vr = LABEL_BOUNDARY : vr = unionf.findSet(v);
        FLOOD_DEBUG printf("%d is %d, %d is %d\n", u, ur, v, vr);

        /*watersheds are done; just ignore it*/
        if ((ur == vr) || (done[ur] && done[vr])) {
            continue;
        }

        /*union and raise colliding watersheds*/

        /* if one of the watersheds is done, then raise the other one,
        mark it as done too but do not union them; this handles also the
        case of boundary watersheds; */
        if (done[ur] || done[vr]) {
            if (done[ur]) {
                FLOOD_DEBUG printf("%d is done, %d raised to %f and done\n", ur,
                                   vr, (double)h);
                done[vr] = 1;
                raise[vr] = h;
            }
            else {
                assert(done[vr]);
                FLOOD_DEBUG printf("%d is done, %d raised to %f and done\n", vr,
                                   ur, (double)h);
                done[ur] = 1;
                raise[ur] = h;
            }
            continue;
        }

        /* if none of the  watersheds is done: union and raise them */
        assert(!done[ur] && !done[vr] && ur > 0 && vr > 0);
        FLOOD_DEBUG printf("union %d and %d,  raised to %f\n", ur, vr,
                           (double)h);
        raise[ur] = raise[vr] = h;
        unionf.makeUnion(ur, vr);
    }

#ifndef NDEBUG
    for (cclabel_type i = 1; i < maxWatersheds; i++) {
        /* assert(done[unionf.findSet(i)]); sometimes this fails! */
        if (!done[unionf.findSet(i)]) {
            G_warning(_("Watershed %d (R=%d) not done"), i, unionf.findSet(i));
        }
    }
#endif
    /* for each watershed find its raised elevation */
    for (cclabel_type i = 1; i < maxWatersheds; i++) {
        raise[i] = raise[unionf.findSet(i)];
    }
    raise[LABEL_BOUNDARY] = 0;
    /*__________________________________________________________*/
    /*cleanup*/
    /*__________________________________________________________*/
    free(done);

    return raise;
}

/************************************************************/
/* returns the amount of mmemory allocated by
   inmemory_fill_depression() */
size_t inmemory_fill_depression_mmusage(cclabel_type maxWatersheds)
{

    size_t mmusage = 0;

    /*account for done array*/
    mmusage += sizeof(int) * maxWatersheds;

    /* account for raise array  */
    mmusage += sizeof(elevation_type) * maxWatersheds;

    /*account for unionFind structure*/
    unionFind<cclabel_type> foo;
    mmusage += foo.mmusage(maxWatersheds);

    return mmusage;
}

/************************************************************/
/* produce a new stream where each elevation e inside watershed i is
   replaced with max(raise[i], e) */
/************************************************************/
void commit_fill(AMI_STREAM<labelElevType> *labeledGrid, elevation_type *raise,
                 cclabel_type maxWatersheds,
                 AMI_STREAM<elevation_type> *filledGrid)
{

    labelElevType *pt;
    elevation_type h;

    labeledGrid->seek(0);
    while (labeledGrid->read_item(&pt) == AMI_ERROR_NO_ERROR) {
        h = pt->getElevation();
        if (is_nodata(h) || pt->getLabel() == LABEL_UNDEF) {
            /*h = nodataType::ELEVATION_NODATA;        ..unhack... XXX*/
        }
        else {
            assert(pt->getLabel() < maxWatersheds);
            h = (pt->getElevation() < raise[pt->getLabel()])
                    ? raise[pt->getLabel()]
                    : pt->getElevation();
        }
        filledGrid->write_item(h);
    }
    /* cout << "filled " << filledGrid->stream_len() << " points\n"; */
}
