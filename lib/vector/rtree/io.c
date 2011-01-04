
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               Markus Metz - file-based and memory-based R*-tree
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <grass/config.h>
#include "index.h"

/* add new free node position for recycling */
void RTreeAddNodePos(off_t pos, int level, struct RTree *t)
{
    int which;
    
    if (t->free_nodes.avail >= t->free_nodes.alloc) {
	size_t size;

	t->free_nodes.alloc += 100;
	size = t->free_nodes.alloc * sizeof(off_t);
	t->free_nodes.pos = (off_t *)realloc((void *)t->free_nodes.pos, size);
	assert(t->free_nodes.pos);
    }
    t->free_nodes.pos[t->free_nodes.avail++] = pos;
    
    which = pos == t->nb[level][1].pos;
    t->nb[level][which].pos = -1;
    t->nb[level][which].dirty = 0;
    t->mru[level] = which == 0;
}

/* looks for free node position, sets file pointer, returns position */
off_t RTreeGetNodePos(struct RTree *t)
{
    if (t->free_nodes.avail > 0) {
	t->free_nodes.avail--;
	return lseek(t->fd, t->free_nodes.pos[t->free_nodes.avail], SEEK_SET);
    }
    else {
	return lseek(t->fd, 0, SEEK_END);
    }
}

/* read node from file */
size_t RTreeReadNode(struct Node *n, off_t nodepos, struct RTree *t)
{
    lseek(t->fd, nodepos, SEEK_SET);
    return read(t->fd, n, t->nodesize);
}

/* get node from buffer or file */
void RTreeGetNode(struct Node *n, off_t nodepos, int level, struct RTree *t)
{
    int which = nodepos == t->nb[level][1].pos;

    if (t->nb[level][which].pos != nodepos) {
	/* which is 0 */
	/* replace least recently used */
	/* least recently used is faster than most recently used */
	if (t->nb[level][which].pos != -1)
	    which = t->mru[level] == 0;
	/* rewrite node in buffer */
	if (t->nb[level][which].dirty) {
	    RTreeRewriteNode(&(t->nb[level][which].n), t->nb[level][which].pos, t);
	    t->nb[level][which].dirty = 0;
	}
	RTreeReadNode(&(t->nb[level][which].n), nodepos, t);
	t->nb[level][which].pos = nodepos;
    }
    t->mru[level] = which;
    *n = t->nb[level][which].n;
}

/* write new node to file */
size_t RTreeWriteNode(struct Node *n, struct RTree *t)
{
    /* file position must be set first with RTreeGetFNodePos() */
    return write(t->fd, n, t->nodesize);
}

/* rewrite updated node to file */
size_t RTreeRewriteNode(struct Node *n, off_t nodepos, struct RTree *t)
{
    lseek(t->fd, nodepos, SEEK_SET);
    return write(t->fd, n, t->nodesize);
}

/* update node in buffer */
void RTreePutNode(struct Node *n, off_t nodepos, struct RTree *t)
{
    int which = nodepos == t->nb[n->level][1].pos;
    
    t->nb[n->level][which].n = *n;
    t->nb[n->level][which].dirty = 1;
    t->mru[n->level] = which;
}

/* update rectangle */
void RTreeUpdateRect(struct Rect *r, struct Node *n, off_t nodepos, int b, struct RTree *t)
{
    int which = nodepos == t->nb[n->level][1].pos;
    
    t->nb[n->level][which].n.branch[b].rect = n->branch[b].rect = *r;
    t->nb[n->level][which].dirty = 1;
    t->mru[n->level] = which;
}

void RTreeFlushBuffer(struct RTree *t)
{
    int i;
    
    for (i = 0; i <= t->rootlevel; i++) {
	if (t->nb[i][0].dirty)
	    RTreeRewriteNode(&(t->nb[i][0].n), t->nb[i][0].pos, t);
	if (t->nb[i][1].dirty)
	    RTreeRewriteNode(&(t->nb[i][1].n), t->nb[i][1].pos, t);
    }
}
