
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

    /* TODO: search with t->used[level][which] instead of which,
     * then which = t->used[level][which] */
    which = (pos == t->nb[level][2].pos ? 2 : pos == t->nb[level][1].pos);
    t->nb[level][which].pos = -1;
    t->nb[level][which].dirty = 0;
    
    /* make it lru */
    if (t->used[level][0] == which) {
	t->used[level][0] = t->used[level][1];
	t->used[level][1] = t->used[level][2];
	t->used[level][2] = which; 
    }
    else if (t->used[level][1] == which) {
	t->used[level][1] = t->used[level][2];
	t->used[level][2] = which; 
    }
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
void RTreeGetNode(struct RTree_Node *n, off_t nodepos, int level, struct RTree *t)
{
    int which = 0;

    /* TODO: search with t->used[level][which] instead of which,
     * then which = t->used[level][which] */
    while (t->nb[level][which].pos != nodepos &&
           t->nb[level][which].pos >= 0 && which < 2)
	which++;

    if (t->nb[level][which].pos != nodepos) {
	/* replace least recently used (fastest method of lru, pseudo-lru, mru) */
	which = t->used[level][2];
	/* rewrite node in buffer */
	if (t->nb[level][which].dirty) {
	    assert(t->nb[level][which].pos >= 0);
	    RTreeRewriteNode(&(t->nb[level][which].n), t->nb[level][which].pos, t);
	    t->nb[level][which].dirty = 0;
	}
	RTreeReadNode(&(t->nb[level][which].n), nodepos, t);
	t->nb[level][which].pos = nodepos;
    }
    /* make it mru */
    if (t->used[level][2] == which) {
	t->used[level][2] = t->used[level][1];
	t->used[level][1] = t->used[level][0];
	t->used[level][0] = which; 
    }
    else if (t->used[level][1] == which) {
	t->used[level][1] = t->used[level][0];
	t->used[level][0] = which; 
    }
    /* copy node */
    RTreeCopyNode(n, &(t->nb[level][which].n), t);
    assert(n->level == level);
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

/* update node in buffer */
void RTreePutNode(struct RTree_Node *n, off_t nodepos, struct RTree *t)
{
    int which = 0;

    /* TODO: search with t->used[level][which] instead of which,
     * then which = t->used[level][which] */
    while (t->nb[n->level][which].pos != nodepos && which < 2)
	which++;

    assert(t->nb[n->level][which].pos == nodepos);
    assert(t->nb[n->level][which].n.level == n->level);
    /* copy node */
    RTreeCopyNode(&(t->nb[n->level][which].n), n, t);
    t->nb[n->level][which].dirty = 1;

    /* make it mru */
    if (t->used[n->level][2] == which) {
	t->used[n->level][2] = t->used[n->level][1];
	t->used[n->level][1] = t->used[n->level][0];
	t->used[n->level][0] = which; 
    }
    else if (t->used[n->level][1] == which) {
	t->used[n->level][1] = t->used[n->level][0];
	t->used[n->level][0] = which; 
    }
}

/* update rectangle */
void RTreeUpdateRect(struct RTree_Rect *r, struct RTree_Node *n,
                     off_t nodepos, int b, struct RTree *t)
{
    int i, j;
    int which = 0;
    
    while (t->nb[n->level][which].pos != nodepos && which < 2)
	which++;

    assert(t->nb[n->level][which].n.level == n->level);
    for (i = 0; i < t->ndims_alloc; i++) {
	t->nb[n->level][which].n.branch[b].rect.boundary[i] =
	                  n->branch[b].rect.boundary[i] = r->boundary[i];
	j = i + t->ndims_alloc;
	t->nb[n->level][which].n.branch[b].rect.boundary[j] =
	                  n->branch[b].rect.boundary[j] = r->boundary[j];
    }

    t->nb[n->level][which].dirty = 1;

    /* make it mru */
    if (t->used[n->level][2] == which) {
	t->used[n->level][2] = t->used[n->level][1];
	t->used[n->level][1] = t->used[n->level][0];
	t->used[n->level][0] = which; 
    }
    else if (t->used[n->level][1] == which) {
	t->used[n->level][1] = t->used[n->level][0];
	t->used[n->level][0] = which; 
    }
}

/* flush pending changes to file */
void RTreeFlushBuffer(struct RTree *t)
{
    int i;
    
    for (i = 0; i <= t->rootlevel; i++) {
	if (t->nb[i][0].dirty)
	    RTreeRewriteNode(&(t->nb[i][0].n), t->nb[i][0].pos, t);
	if (t->nb[i][1].dirty)
	    RTreeRewriteNode(&(t->nb[i][1].n), t->nb[i][1].pos, t);
	if (t->nb[i][2].dirty)
	    RTreeRewriteNode(&(t->nb[i][2].n), t->nb[i][2].pos, t);
    }
}
