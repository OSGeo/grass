
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

/* #define USAGE_SWAP */

/* add new free node position for recycling */
void RTreeAddNodePos(off_t pos, int level, struct RTree *t)
{
    int which, i;

    if (t->free_nodes.avail >= t->free_nodes.alloc) {
	size_t size;

	t->free_nodes.alloc += 100;
	size = t->free_nodes.alloc * sizeof(off_t);
	t->free_nodes.pos = (off_t *)realloc((void *)t->free_nodes.pos, size);
	assert(t->free_nodes.pos);
    }
    t->free_nodes.pos[t->free_nodes.avail++] = pos;

    /* check mru first */
    i = 0;
    while (t->nb[level][t->used[level][i]].pos != pos &&
           i < NODE_BUFFER_SIZE)
	i++;

    /* is it possible that this node is not in the buffer? */
    assert(i < NODE_BUFFER_SIZE);
    which = t->used[level][i];
    t->nb[level][which].pos = -1;
    t->nb[level][which].dirty = 0;
    
    /* make it lru */
    if (i < NODE_BUFFER_SIZE - 1) { /* which != t->used[level][NODE_BUFFER_SIZE - 1] */
	/* simple swap does not work here */
	while (i < NODE_BUFFER_SIZE - 1 &&
	       t->nb[level][t->used[level][i + 1]].pos != -1) {
	    t->used[level][i] = t->used[level][i + 1];
	    i++;
	}
	assert(i < NODE_BUFFER_SIZE);
	t->used[level][i] = which;
    }
}

/* look for free node position, set file pointer, return position */
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

/* read branch from file */
size_t RTreeReadBranch(struct RTree_Branch *b, struct RTree *t)
{
    size_t size = 0;

    size += read(t->fd, b->rect.boundary, t->rectsize);
    size += read(t->fd, &(b->child), sizeof(union RTree_Child));

    return size;
}

/* read node from file */
size_t RTreeReadNode(struct RTree_Node *n, off_t nodepos, struct RTree *t)
{
    int i;
    size_t size = 0;

    lseek(t->fd, nodepos, SEEK_SET);
    size += read(t->fd, &(n->count), sizeof(int));
    size += read(t->fd, &(n->level), sizeof(int));

    for (i = 0; i < MAXCARD; i++) {
	size += RTreeReadBranch(&(n->branch[i]), t);
    }

    return size;
}

/* get node from buffer or file */
struct RTree_Node *RTreeGetNode(off_t nodepos, int level, struct RTree *t)
{
    int which, i = 0;

    /* check mru first */
    while (t->nb[level][t->used[level][i]].pos != nodepos &&
           t->nb[level][t->used[level][i]].pos >= 0 &&
	   i < NODE_BUFFER_SIZE - 1)
	i++;

    which = t->used[level][i];

    if (t->nb[level][which].pos != nodepos) {
	/* rewrite node in buffer */
	if (t->nb[level][which].dirty) {
	    RTreeRewriteNode(&(t->nb[level][which].n),
	                     t->nb[level][which].pos, t);
	    t->nb[level][which].dirty = 0;
	}
	RTreeReadNode(&(t->nb[level][which].n), nodepos, t);
	t->nb[level][which].pos = nodepos;
    }
    /* make it mru */
    if (i) { /* t->used[level][0] != which */
#ifdef USAGE_SWAP
	t->used[level][i] = t->used[level][0];
	t->used[level][0] = which;
#else
	while (i) {
	    t->used[level][i] = t->used[level][i - 1];
	    i--;
	}
	t->used[level][0] = which;
#endif
    }

    /* RTreeCopyNode(n, &(t->nb[level][which].n), t); */
    
    return &(t->nb[level][which].n);
}

/* write branch to file */
size_t RTreeWriteBranch(struct RTree_Branch *b, struct RTree *t)
{
    size_t size = 0;

    size += write(t->fd, b->rect.boundary, t->rectsize);
    size += write(t->fd, &(b->child), sizeof(union RTree_Child));

    return size;
}

/* write new node to file */
size_t RTreeWriteNode(struct RTree_Node *n, struct RTree *t)
{
    int i;
    size_t size = 0;

    /* file position must be set first with RTreeGetFNodePos() */
    size += write(t->fd, &(n->count), sizeof(int));
    size += write(t->fd, &(n->level), sizeof(int));

    for (i = 0; i < MAXCARD; i++) {
	size += RTreeWriteBranch(&(n->branch[i]), t);
    }

    return size;
}

/* rewrite updated node to file */
size_t RTreeRewriteNode(struct RTree_Node *n, off_t nodepos, struct RTree *t)
{
    lseek(t->fd, nodepos, SEEK_SET);

    return RTreeWriteNode(n, t);
}

/* mark node in buffer as changed */
void RTreeNodeChanged(struct RTree_Node *n, off_t nodepos, struct RTree *t)
{
    int which, i = 0;

    /* check mru first */
    while (t->nb[n->level][t->used[n->level][i]].pos != nodepos &&
           i < NODE_BUFFER_SIZE)
	i++;

    assert(i < NODE_BUFFER_SIZE);
    /* as it is used, it should always be mru */
    assert(i == 0);
    which = t->used[n->level][i];

    t->nb[n->level][which].dirty = 1;

    /* make it mru */
    if (i) { /* t->used[level][0] != which */
#ifdef USAGE_SWAP
	t->used[n->level][i] = t->used[n->level][0];
	t->used[n->level][0] = which;
#else
	while (i) {
	    t->used[n->level][i] = t->used[n->level][i - 1];
	    i--;
	}
	t->used[n->level][0] = which;
#endif
    }
}

/* flush pending changes to file */
void RTreeFlushBuffer(struct RTree *t)
{
    int i, j;
    
    for (i = 0; i <= t->rootlevel; i++) {
	for (j = 0; j < NODE_BUFFER_SIZE; j++) {
	    if (t->nb[i][j].dirty) {
		RTreeRewriteNode(&(t->nb[i][j].n), t->nb[i][j].pos, t);
		t->nb[i][j].dirty = 0;
	    }
	}
    }
}
