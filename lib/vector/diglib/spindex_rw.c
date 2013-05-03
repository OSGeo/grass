/*!
   \file diglib/spindex.c

   \brief Vector library - spatial index - read/write (lower level functions)

   Lower level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes
   \author Update to GRASS 5.7 Radim Blazek
   \author Update to GRASS 7 Markus Metz
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/version.h>

/* TODO: only write out actually used sides */
#ifndef NUMSIDES
#define NUMSIDES 6
#endif

/* TODO: merge these two */
struct spidxstack
{
    off_t pos[MAXCARD];		/* file position of child node, object ID on level 0 */
    struct RTree_Node sn;	/* stack node */
    int branch_id;		/* branch no to follow down */
};

struct spidxpstack
{
    off_t pos[MAXCARD];		/* file position of child node, object ID on level 0 */
    struct RTree_Node *sn;	/* stack node pointer */
    int branch_id;		/* branch no to follow down */
};

/*!
   \brief Write spatial index header to file

   \param[in,out] fp pointer to struct gvfile
   \param ptr pointer to Plus_head structure

   \return 0 on success
   \return -1 on error
 */
int dig_Wr_spidx_head(struct gvfile * fp, struct Plus_head *ptr)
{
    unsigned char buf[6];
    long length = 81;		/* header length in bytes */
    struct RTree *t;
    size_t size;

    dig_rewind(fp);
    dig_set_cur_port(&(ptr->spidx_port));

    /* use ptr->off_t_size = 4 if possible */
    if (sizeof(off_t) > 4) {
	size = ptr->Node_spidx->n_nodes * ptr->Node_spidx->nodesize;
	size += ptr->Line_spidx->n_nodes * ptr->Line_spidx->nodesize;
	size += ptr->Area_spidx->n_nodes * ptr->Area_spidx->nodesize;
	size += ptr->Isle_spidx->n_nodes * ptr->Isle_spidx->nodesize;

	if (size < PORT_INT_MAX)
	    ptr->spidx_port.off_t_size = 4;
	else
	    ptr->spidx_port.off_t_size = 8;
    }
    else
	ptr->spidx_port.off_t_size = 4;

    /* bytes 1 - 6 */
    buf[0] = GV_SIDX_VER_MAJOR;
    buf[1] = GV_SIDX_VER_MINOR;
    buf[2] = GV_SIDX_EARLIEST_MAJOR;
    buf[3] = GV_SIDX_EARLIEST_MINOR;
    buf[4] = ptr->spidx_port.byte_order;
    buf[5] = (unsigned char)ptr->spidx_port.off_t_size;
    if (0 >= dig__fwrite_port_C((const char *)buf, 6, fp))
	return (-1);

    /* adjust header size for large files */
    if (ptr->spidx_port.off_t_size == 4) {
	if (ptr->off_t_size == 4)
	    length = 113;
	else if (ptr->off_t_size == 8)
	    length = 117;
	else
            G_fatal_error(_("Topology file must be written before spatial index file"));
    }
    else if (ptr->spidx_port.off_t_size == 8) {
	if (ptr->off_t_size == 4)
	    length = 141;
	else if (ptr->off_t_size == 8)
	    length = 145;
	else
            G_fatal_error(_("Topology file must be written before spatial index file"));
    }

    /* bytes 7 - 10 : header size */
    if (0 >= dig__fwrite_port_L(&length, 1, fp))
	return (0);

    ptr->spidx_head_size = length;

    /* byte 11 : dimension 2D or 3D */
    buf[0] = ptr->spidx_with_z;
    if (0 >= dig__fwrite_port_C((const char *)buf, 1, fp))
	return (-1);

    /* identical for all spatial indices: */
    t = ptr->Node_spidx;
    /* byte 12 : n dimensions */
    if (0 >= dig__fwrite_port_C((const char *)&(t->ndims), 1, fp))
	return (-1);
    /* byte 13 : n sides */
    if (0 >= dig__fwrite_port_C((const char *)&(t->nsides), 1, fp))
	return (-1);
    /* bytes 14 - 17 : nodesize */
    if (0 >= dig__fwrite_port_I(&(t->nodesize), 1, fp))
	return (-1);
    /* bytes 18 - 21 : nodecard */
    if (0 >= dig__fwrite_port_I(&(t->nodecard), 1, fp))
	return (-1);
    /* bytes 22 - 25 : leafcard */
    if (0 >= dig__fwrite_port_I(&(t->leafcard), 1, fp))
	return (-1);
    /* bytes 26 - 29 : min node fill */
    if (0 >= dig__fwrite_port_I(&(t->min_node_fill), 1, fp))
	return (-1);
    /* bytes 30 - 33 : min leaf fill */
    if (0 >= dig__fwrite_port_I(&(t->min_leaf_fill), 1, fp))
	return (-1);

    /* for each spatial index : */

    /* Node spatial index */
    /* bytes 34 - 37 : n nodes */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 38 - 41 : n leafs */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 42 - 45 : n levels */
    if (0 >= dig__fwrite_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 46 - 49 (LFS 53) : root node offset */
    if (0 >=
	dig__fwrite_port_O(&(ptr->Node_spidx_offset), 1, fp,
			   ptr->spidx_port.off_t_size))
	return (-1);

    /* Line spatial index */
    t = ptr->Line_spidx;
    /* bytes 50 - 53 (LFS 54 - 57) : n nodes */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 54 - 57 (LFS 58 - 61) : n leafs */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 58 - 61 (LFS 62 - 65) : n levels */
    if (0 >= dig__fwrite_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 62 - 65 (LFS 66 - 73) : root node offset */
    if (0 >=
	dig__fwrite_port_O(&(ptr->Line_spidx_offset), 1, fp,
			   ptr->spidx_port.off_t_size))
	return (-1);

    /* Area spatial index */
    t = ptr->Area_spidx;
    /* bytes 66 - 69 (LFS 74 - 77) : n nodes */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 70 - 73 (LFS 78 - 81) : n leafs */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 74 - 77 (LFS 82 - 85) : n levels */
    if (0 >= dig__fwrite_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 78 - 81 (LFS 86 - 93) : root node offset */
    if (0 >=
	dig__fwrite_port_O(&(ptr->Area_spidx_offset), 1, fp,
			   ptr->spidx_port.off_t_size))
	return (-1);

    /* Isle spatial index */
    t = ptr->Isle_spidx;
    /* bytes 82 - 85 (LFS 94 - 97) : n nodes */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 86 - 89 (LFS 98 - 101) : n leafs */
    if (0 >= dig__fwrite_port_I((const int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 90 - 93 (LFS 102 - 105) : n levels */
    if (0 >= dig__fwrite_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 94 - 97 (LFS 106 - 113) : root node offset */
    if (0 >=
	dig__fwrite_port_O(&(ptr->Isle_spidx_offset), 1, fp,
			   ptr->spidx_port.off_t_size))
	return (-1);

    /* 3D future : */
    /* Face spatial index */
    /* bytes 98 - 101 (LFS 114 - 121) : root node offset */
    if (0 >=
	dig__fwrite_port_O(&(ptr->Face_spidx_offset), 1, fp,
			   ptr->spidx_port.off_t_size))
	return (-1);
    /* ptr->Face_spidx->rootpos = ptr->Face_spidx_offset; */

    /* Volume spatial index */
    /* bytes 102 - 105 (LFS 122 - 129) : root node offset */
    if (0 >=
	dig__fwrite_port_O(&(ptr->Volume_spidx_offset), 1, fp,
			   ptr->spidx_port.off_t_size))
	return (-1);
    /* ptr->Volume_spidx->rootpos = ptr->Volume_spidx_offset; */

    /* Hole spatial index */
    /* bytes 106 - 109 (LFS 130 - 137) : root node offset */
    if (0 >=
	dig__fwrite_port_O(&(ptr->Hole_spidx_offset), 1, fp,
			   ptr->spidx_port.off_t_size))
	return (-1);
    /* ptr->Hole_spidx->rootpos = ptr->Hole_spidx_offset; */

    G_debug(3, "spidx offset node = %lu line = %lu, area = %lu isle = %lu",
	    (long unsigned)ptr->Node_spidx_offset,
	    (long unsigned)ptr->Line_spidx_offset,
	    (long unsigned)ptr->Area_spidx_offset,
	    (long unsigned)ptr->Isle_spidx_offset);

    /* coor file size : bytes 110 - 113 (117) (LFS: 138 - 141 (145)) */
    if (0 >= dig__fwrite_port_O(&(ptr->coor_size), 1, fp, ptr->off_t_size))
	return (-1);

    length = (long unsigned)dig_ftell(fp);
    G_debug(1, "spidx body offset %lu", length);

    if (ptr->spidx_head_size != length)
	G_fatal_error("wrong sidx head length %ld", ptr->spidx_head_size);

    return (0);
}

/*!
   \brief Read spatial index header from sidx file

   \param fp pointer to struct gvfile
   \param[in,out] ptr pointer to Plus_head structure

   \return 0 on success
   \return -1 on error
 */
int dig_Rd_spidx_head(struct gvfile * fp, struct Plus_head *ptr)
{
    unsigned char buf[6];
    int byte_order;
    struct RTree *t;

    dig_rewind(fp);

    /* bytes 1 - 6 */
    if (0 >= dig__fread_port_C((char *)buf, 6, fp))
	return (-1);
    ptr->version.spidx.major = buf[0];
    ptr->version.spidx.minor = buf[1];
    ptr->version.spidx.back_major = buf[2];
    ptr->version.spidx.back_minor = buf[3];
    byte_order = buf[4];
    ptr->spidx_port.off_t_size = buf[5];

    G_debug(2,
	    "Spidx header: file version %d.%d , supported from GRASS version %d.%d",
	    ptr->version.spidx.major, ptr->version.spidx.minor,
	    ptr->version.spidx.back_major, ptr->version.spidx.back_minor);

    G_debug(2, "  byte order %d", byte_order);

    /* check version numbers */
    if (ptr->version.spidx.major > GV_SIDX_VER_MAJOR ||
	ptr->version.spidx.minor > GV_SIDX_VER_MINOR) {
	/* The file was created by GRASS library with higher version than this one */

	if (ptr->version.spidx.back_major > GV_SIDX_VER_MAJOR ||
	    ptr->version.spidx.back_minor > GV_SIDX_VER_MINOR) {
	    /* This version of GRASS lib is lower than the oldest which can read this format */
	    G_debug(1, "Spatial index format version %d.%d",
		    ptr->version.spidx.major, ptr->version.spidx.minor);
	    G_fatal_error
		(_("This version of GRASS (%d.%d) is too old to read this spatial index format."
		 " Try to rebuild topology or upgrade GRASS to at least version %d."),
		 GRASS_VERSION_MAJOR, GRASS_VERSION_MINOR, GRASS_VERSION_MAJOR + 1);
	    return (-1);
	}

	G_warning(_("Your GRASS version does not fully support "
		    "spatial index format %d.%d of the vector."
		    " Consider to rebuild topology or upgrade GRASS."),
		  ptr->version.spidx.major, ptr->version.spidx.minor);
    }
    if (ptr->version.spidx.major < GV_SIDX_VER_MAJOR ||
	(ptr->version.spidx.major == GV_SIDX_VER_MAJOR &&
	ptr->version.spidx.minor < GV_SIDX_VER_MINOR)) {
	/* The file was created by GRASS library with lower version than this one */
	    G_fatal_error(_("Spatial index format version %d.%d is not "
			    "supported by this release."
			    " Please rebuild topology."),
			  ptr->version.spidx.major, ptr->version.spidx.minor);
	    return (-1);
    }

    /* can this library read the sidx file ? */
    if (ptr->spidx_port.off_t_size > (int)sizeof(off_t)) {
	G_fatal_error("Spatial index was written with LFS but this "
		      "GRASS version does not support LFS. "
		      "Please get a GRASS version with LFS support.");
    }

    dig_init_portable(&(ptr->spidx_port), byte_order);
    dig_set_cur_port(&(ptr->spidx_port));

    /* bytes 7 - 10 : header size */
    if (0 >= dig__fread_port_L(&(ptr->spidx_head_size), 1, fp))
	return (-1);
    G_debug(2, "  header size %ld", ptr->spidx_head_size);

    /* byte 11 : dimension 2D or 3D */
    if (0 >= dig__fread_port_C((char *)buf, 1, fp))
	return (-1);
    ptr->spidx_with_z = buf[0];
    G_debug(2, "  with_z %d", ptr->spidx_with_z);

    /* identical for all spatial indices: */
    t = ptr->Node_spidx;
    /* byte 12 : n dimensions */
    if (0 >= dig__fread_port_C((char *)&(t->ndims), 1, fp))
	return (-1);
    ptr->Line_spidx->ndims = t->ndims;
    ptr->Area_spidx->ndims = t->ndims;
    ptr->Isle_spidx->ndims = t->ndims;

    /* byte 13 : n sides */
    if (0 >= dig__fread_port_C((char *)&(t->nsides), 1, fp))
	return (-1);
    ptr->Line_spidx->nsides = t->nsides;
    ptr->Area_spidx->nsides = t->nsides;
    ptr->Isle_spidx->nsides = t->nsides;

    /* bytes 14 - 17 : nodesize */
    if (0 >= dig__fread_port_I(&(t->nodesize), 1, fp))
	return (-1);
    ptr->Line_spidx->nodesize = t->nodesize;
    ptr->Area_spidx->nodesize = t->nodesize;
    ptr->Isle_spidx->nodesize = t->nodesize;

    /* bytes 18 - 21 : nodecard */
    if (0 >= dig__fread_port_I(&(t->nodecard), 1, fp))
	return (-1);
    ptr->Line_spidx->nodecard = t->nodecard;
    ptr->Area_spidx->nodecard = t->nodecard;
    ptr->Isle_spidx->nodecard = t->nodecard;

    /* bytes 22 - 25 : leafcard */
    if (0 >= dig__fread_port_I(&(t->leafcard), 1, fp))
	return (-1);
    ptr->Line_spidx->leafcard = t->leafcard;
    ptr->Area_spidx->leafcard = t->leafcard;
    ptr->Isle_spidx->leafcard = t->leafcard;

    /* bytes 26 - 29 : min node fill */
    if (0 >= dig__fread_port_I(&(t->min_node_fill), 1, fp))
	return (-1);
    ptr->Line_spidx->min_node_fill = t->min_node_fill;
    ptr->Area_spidx->min_node_fill = t->min_node_fill;
    ptr->Isle_spidx->min_node_fill = t->min_node_fill;

    /* bytes 30 - 33 : min leaf fill */
    if (0 >= dig__fread_port_I(&(t->min_leaf_fill), 1, fp))
	return (-1);
    ptr->Line_spidx->min_leaf_fill = t->min_leaf_fill;
    ptr->Area_spidx->min_leaf_fill = t->min_leaf_fill;
    ptr->Isle_spidx->min_leaf_fill = t->min_leaf_fill;

    /* for each spatial index : */

    /* Node spatial index */
    /* bytes 34 - 37 : n nodes */
    if (0 >= dig__fread_port_I((int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 38 - 41 : n leafs */
    if (0 >= dig__fread_port_I((int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 42 - 45 : n levels */
    if (0 >= dig__fread_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 46 - 49 (LFS 53) : root node offset */
    if (0 >=
	dig__fread_port_O(&(ptr->Node_spidx_offset), 1, fp,
			  ptr->spidx_port.off_t_size))
	return (-1);
    t->rootpos = ptr->Node_spidx_offset;

    /* Line spatial index */
    t = ptr->Line_spidx;
    /* bytes 50 - 53 (LFS 54 - 57) : n nodes */
    if (0 >= dig__fread_port_I((int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 54 - 57 (LFS 58 - 61) : n leafs */
    if (0 >= dig__fread_port_I((int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 58 - 61 (LFS 62 - 65) : n levels */
    if (0 >= dig__fread_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 62 - 65 (LFS 66 - 73) : root node offset */
    if (0 >=
	dig__fread_port_O(&(ptr->Line_spidx_offset), 1, fp,
			  ptr->spidx_port.off_t_size))
	return (-1);
    ptr->Line_spidx->rootpos = ptr->Line_spidx_offset;

    /* Area spatial index */
    t = ptr->Area_spidx;
    /* bytes 66 - 69 (LFS 74 - 77) : n nodes */
    if (0 >= dig__fread_port_I((int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 70 - 73 (LFS 78 - 81) : n leafs */
    if (0 >= dig__fread_port_I((int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 74 - 77 (LFS 82 - 85) : n levels */
    if (0 >= dig__fread_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 78 - 81 (LFS 86 - 93) : root node offset */
    if (0 >=
	dig__fread_port_O(&(ptr->Area_spidx_offset), 1, fp,
			  ptr->spidx_port.off_t_size))
	return (-1);
    ptr->Area_spidx->rootpos = ptr->Area_spidx_offset;

    /* Isle spatial index */
    t = ptr->Isle_spidx;
    /* bytes 82 - 85 (LFS 94 - 97) : n nodes */
    if (0 >= dig__fread_port_I((int *)&(t->n_nodes), 1, fp))
	return (-1);
    /* bytes 86 - 89 (LFS 98 - 101) : n leafs */
    if (0 >= dig__fread_port_I((int *)&(t->n_leafs), 1, fp))
	return (-1);
    /* bytes 90 - 93 (LFS 102 - 105) : n levels */
    if (0 >= dig__fread_port_I(&(t->rootlevel), 1, fp))
	return (-1);
    /* bytes 94 - 97 (LFS 106 - 113) : root node offset */
    if (0 >=
	dig__fread_port_O(&(ptr->Isle_spidx_offset), 1, fp,
			  ptr->spidx_port.off_t_size))
	return (-1);
    ptr->Isle_spidx->rootpos = ptr->Isle_spidx_offset;

    /* 3D future : */
    /* Face spatial index */
    /* bytes 98 - 101 (LFS 114 - 121) : root node offset */
    if (0 >=
	dig__fread_port_O(&(ptr->Face_spidx_offset), 1, fp,
			  ptr->spidx_port.off_t_size))
	return (-1);
    /* ptr->Face_spidx->rootpos = ptr->Face_spidx_offset; */

    /* Volume spatial index */
    /* bytes 102 - 105 (LFS 122 - 129) : root node offset */
    if (0 >=
	dig__fread_port_O(&(ptr->Volume_spidx_offset), 1, fp,
			  ptr->spidx_port.off_t_size))
	return (-1);
    /* ptr->Volume_spidx->rootpos = ptr->Volume_spidx_offset; */

    /* Hole spatial index */
    /* bytes 106 - 109 (LFS 130 - 137) : root node offset */
    if (0 >=
	dig__fread_port_O(&(ptr->Hole_spidx_offset), 1, fp,
			  ptr->spidx_port.off_t_size))
	return (-1);
    /* ptr->Hole_spidx->rootpos = ptr->Hole_spidx_offset; */

    /* coor file size : bytes 110 - 113 (117) (LFS: 138 - 145) */
    if (ptr->off_t_size == -1)
        ptr->off_t_size = ptr->spidx_port.off_t_size;
    if (0 >= dig__fread_port_O(&(ptr->coor_size), 1, fp, ptr->off_t_size))
	return (-1);
    G_debug(2, "  coor size %lu", (long unsigned)ptr->coor_size);

    dig_fseek(fp, ptr->spidx_head_size, SEEK_SET);

    return (0);
}

static int rtree_dump_node(FILE *, struct RTree_Node *n, int);

/*!
   \brief Dump R-tree branch to the file

   \param fp pointer to FILE
   \param b pointer to Branch structure
   \param with_z non-zero value for 3D vector data
   \param level level value

   \return 0
 */
static int rtree_dump_branch(FILE * fp, struct RTree_Branch *b, int with_z,
			     int level)
{
    const struct RTree_Rect *r;

    r = &(b->rect);

    if (level == 0)
	fprintf(fp, "  id = %d ", b->child.id);

    fprintf(fp, " %f %f %f %f %f %f\n", r->boundary[0], r->boundary[1],
	    r->boundary[2], r->boundary[3], r->boundary[4], r->boundary[5]);

    if (level > 0) {
	rtree_dump_node(fp, b->child.ptr, with_z);
    }
    return 0;
}

/*!
   \brief Dump R-tree node to the file

   \param fp pointer to FILE
   \param n pointer to Node structure
   \param with_z non-zero value for 3D vector data

   \return 0
 */
int rtree_dump_node(FILE * fp, struct RTree_Node *n, int with_z)
{
    int i;

    /* recursive nearly-but-a-bit-messy depth-first pre-order traversal
     * potentially filling up memory */
    /* TODO: change to non-recursive depth-first post-order traversal */
    /* left for comparison with GRASS6.x */

    fprintf(fp, "Node level=%d  count=%d\n", n->level, n->count);

    if (n->level > 0)
	for (i = 0; i < NODECARD; i++) {
	    if (n->branch[i].child.ptr) {
		fprintf(fp, "  Branch %d", i);
		rtree_dump_branch(fp, &n->branch[i], with_z, n->level);
	    }
	}
    else
	for (i = 0; i < LEAFCARD; i++) {
	    if (n->branch[i].child.id) {
		fprintf(fp, "  Branch %d", i);
		rtree_dump_branch(fp, &n->branch[i], with_z, n->level);
	    }
	}

    return 0;
}

static int rtree_dump_node_file(FILE *, off_t, int, struct RTree *);

/*!
   \brief Dump R-tree branch from temp file to the file

   \param fp pointer to FILE
   \param b pointer to Branch structure
   \param with_z non-zero value for 3D vector data
   \param level level value

   \return 0
 */
static int rtree_dump_branch_file(FILE * fp, struct RTree_Branch *b, int with_z,
			     int level, struct RTree *t)
{
    const struct RTree_Rect *r;

    r = &(b->rect);

    if (level == 0)
	fprintf(fp, "  id = %d ", b->child.id);

    fprintf(fp, " %f %f %f %f %f %f\n", r->boundary[0], r->boundary[1],
	    r->boundary[2], r->boundary[3], r->boundary[4], r->boundary[5]);

    if (level > 0) {
	rtree_dump_node_file(fp, b->child.pos, with_z, t);
    }
    return 0;
}

/*!
   \brief Dump R-tree node from temp file to the file

   \param fp pointer to FILE
   \param pos position of Node in temp file
   \param with_z non-zero value for 3D vector data
   \param t RTree to dump

   \return 0
 */
int rtree_dump_node_file(FILE * fp, off_t pos, int with_z, struct RTree *t)
{
    int i;
    static struct RTree_Node *n = NULL;
    
    if (!n) {
	n = RTreeAllocNode(t, 1);
    }

    /* recursive nearly-but-a-bit-messy depth-first pre-order traversal
     * potentially filling up memory */
    /* TODO: change to non-recursive depth-first post-order traversal */
    /* left for comparison with GRASS6.x */

    RTreeReadNode(n, pos, t);
    fprintf(fp, "Node level=%d  count=%d\n", n->level, n->count);

    if (n->level > 0)
	for (i = 0; i < NODECARD; i++) {
	    if (n->branch[i].child.pos >= 0) {
		fprintf(fp, "  Branch %d", i);
		rtree_dump_branch_file(fp, &(n->branch[i]), with_z, n->level, t);
	    }
	}
    else
	for (i = 0; i < LEAFCARD; i++) {
	    if (n->branch[i].child.id) {
		fprintf(fp, "  Branch %d", i);
		rtree_dump_branch_file(fp, &(n->branch[i]), with_z, n->level, t);
	    }
	}

    return 0;
}

/*
 * all following methods to transfer spatial indices (rtrees) are based
 * on the same idea
 * do a postorder depth-first non-recursive traversal of the rtree
 * a leaf node is transfered first
 * the root node is transfered last
 * 
 * this applies to all four scenarios
 * - from intermediate file to sidx file
 * - from sidx file to intermediate file
 * - from memory to sidx file
 * - from sidx file to memory
 * 
 * I could not think of one function that's good for all four scenarios, 
 * but that doesn't mean there is none...
 * 
 * maybe something like V2_read_line_array and Write_line_array
 * in Vlib/read.c and Vlib/write.c, at least for transferring from sidx 
 * and transferrring to sidx?
 */


/*!
   \brief Write RTree body from memory to sidx file
   Must be called when new or updated vector is closed

   \param[out] fp pointer to struct gvfile
   \param startpos offset to struct gvfile where to start writing out
   \param t pointer to RTree
   \param off_t_size size of off_t used to write struct gvfile

   \return -1 on error
   \return offset to root node on success
 */

static off_t rtree_write_from_memory(struct gvfile *fp, off_t startpos,
				 struct RTree *t, int off_t_size)
{
    off_t nextfreepos = startpos;
    int sidx_nodesize, sidx_leafsize;
    struct RTree_Node *n;
    int i, j, writeout, maxcard;
    struct spidxpstack *s = G_malloc(MAXLEVEL * sizeof(struct spidxstack));
    int top = 0;

    /* should be foolproof */
    sidx_nodesize =
	(int)(2 * PORT_INT + t->nodecard * (off_t_size + NUMSIDES * PORT_DOUBLE));
    sidx_leafsize =
	(int)(2 * PORT_INT + t->leafcard * (off_t_size + NUMSIDES * PORT_DOUBLE));

    /* stack size of t->rootlevel + 1 would be enough because of
     * depth-first post-order traversal:
     * only one node per level on stack at any given time */

    /* add root node position to stack */
    s[top].branch_id = i = 0;
    s[top].sn = t->root;

    /* depth-first postorder traversal 
     * all children of a node are visitied and written out first
     * when a child is written out, its position in file is stored in pos[] for
     * the parent node and written out with the parent node */
    /* root node is written out last and its position returned */

    while (top >= 0) {
	if (s[top].sn == NULL)
	    G_fatal_error("NULL node ptr at top = %d", top);
	n = s[top].sn;
	writeout = 1;
	/* this is an internal node in the RTree
	 * all its children are processed first,
	 * before it is written out to the sidx file */
	if (s[top].sn->level > 0) {
	    for (i = s[top].branch_id; i < t->nodecard; i++) {
		s[top].pos[i] = 0;
		if (n->branch[i].child.ptr != NULL) {
		    s[top++].branch_id = i + 1;
		    s[top].sn = n->branch[i].child.ptr;
		    s[top].branch_id = 0;
		    writeout = 0;
		    break;
		}
	    }
	    if (writeout) {
		/* nothing else found, ready to write out */
		s[top].branch_id = t->nodecard;
	    }
	}
	if (writeout) {
	    /* write node to sidx file */
	    if (G_ftell(fp->file) != nextfreepos)
		G_fatal_error("write sidx: wrong node position in file");

	    /* write with dig__fwrite_port_* fns */
	    dig__fwrite_port_I(&(s[top].sn->count), 1, fp);
	    dig__fwrite_port_I(&(s[top].sn->level), 1, fp);
	    maxcard = s[top].sn->level ? t->nodecard : t->leafcard;
	    for (j = 0; j < maxcard; j++) {
		dig__fwrite_port_D(s[top].sn->branch[j].rect.boundary,
				   NUMSIDES, fp);
		/* leaf node: vector object IDs are stored in child.id */
		if (s[top].sn->level == 0)
		    s[top].pos[j] = (off_t) s[top].sn->branch[j].child.id;
		dig__fwrite_port_O(&(s[top].pos[j]), 1, fp, off_t_size);
	    }

	    top--;
	    /* update corresponding child position of parent node
	     * this node is only updated if its level is > 0, i.e.
	     * this is an internal node
	     * children of internal nodes do not have an ID, instead
	     * they hold the position in file of the next nodes down the tree */
	    if (top >= 0) {
		s[top].pos[s[top].branch_id - 1] = nextfreepos;
		nextfreepos += (s[top + 1].sn->level ? sidx_nodesize : sidx_leafsize);
	    }
	}
    }
    
    G_free(s);

    return nextfreepos;
}


/*!
   \brief Write RTree body from temporary file to sidx file
   Must be called when new or updated vector is closed

   \param[out] fp pointer to struct gvfile
   \param startpos offset to struct gvfile where to start writing out
   \param t pointer to RTree
   \param off_t_size size of off_t used to write struct gvfile

   \return -1 on error
   \return offset to root node on success
 */

static off_t rtree_write_from_file(struct gvfile *fp, off_t startpos,
				 struct RTree *t, int off_t_size)
{
    off_t nextfreepos = startpos;
    int sidx_nodesize, sidx_leafsize;
    struct RTree_Node *n;
    int i, j, writeout, maxcard;
    static struct spidxstack *s = NULL;
    int top = 0;
    
    if (!s) {
	s = G_malloc(MAXLEVEL * sizeof(struct spidxstack));
	for (i = 0; i < MAXLEVEL; i++) {
	    s[i].sn.branch = G_malloc(MAXCARD * sizeof(struct RTree_Branch));
	    for (j = 0; j < MAXCARD; j++) {
		s[i].sn.branch[j].rect.boundary = G_malloc(6 * sizeof(RectReal));
	    }
	}
    }

    /* write pending changes to file */
    RTreeFlushBuffer(t);

    /* should be foolproof */
    sidx_nodesize =
	(int)(2 * PORT_INT + t->nodecard * (off_t_size + NUMSIDES * PORT_DOUBLE));
    sidx_leafsize =
	(int)(2 * PORT_INT + t->leafcard * (off_t_size + NUMSIDES * PORT_DOUBLE));

    /* stack size of t->rootlevel + 1 would be enough because of
     * depth-first post-order traversal:
     * only one node per level on stack at any given time */

    /* add root node position to stack */
    s[top].branch_id = i = 0;
    RTreeReadNode(&s[top].sn, t->rootpos, t);

    /* depth-first postorder traversal 
     * all children of a node are visitied and written out first
     * when a child is written out, its position in file is stored in pos[] for
     * the parent node and written out with the parent node */
    /* root node is written out last and its position returned */

    while (top >= 0) {
	n = &(s[top].sn);
	writeout = 1;
	/* this is an internal node in the RTree
	 * all its children are processed first,
	 * before it is written out to the sidx file */
	if (s[top].sn.level > 0) {
	    for (i = s[top].branch_id; i < t->nodecard; i++) {
		s[top].pos[i] = -1;
		if (n->branch[i].child.pos >= 0) {
		    s[top++].branch_id = i + 1;
		    RTreeReadNode(&s[top].sn, n->branch[i].child.pos, t);
		    s[top].branch_id = 0;
		    writeout = 0;
		    break;
		}
	    }
	    if (writeout) {
		/* nothing else found, ready to write out */
		s[top].branch_id = t->nodecard;
	    }
	}
	if (writeout) {
	    /* write node to sidx file */
	    if (G_ftell(fp->file) != nextfreepos)
		G_fatal_error(_("Writing sidx: wrong node position in file"));

	    /* write with dig__fwrite_port_* fns */
	    dig__fwrite_port_I(&(s[top].sn.count), 1, fp);
	    dig__fwrite_port_I(&(s[top].sn.level), 1, fp);
	    maxcard = s[top].sn.level ? t->nodecard : t->leafcard;
	    for (j = 0; j < maxcard; j++) {
		dig__fwrite_port_D(s[top].sn.branch[j].rect.boundary,
				   NUMSIDES, fp);
		/* leaf node: vector object IDs are stored in child.id */
		if (s[top].sn.level == 0)
		    s[top].pos[j] = (off_t) s[top].sn.branch[j].child.id;
		dig__fwrite_port_O(&(s[top].pos[j]), 1, fp, off_t_size);
	    }

	    top--;
	    /* update corresponding child position of parent node
	     * this node is only updated if its level is > 0, i.e.
	     * this is an internal node
	     * children of internal nodes do not have an ID, instead
	     * they hold the position in file of the next nodes down the tree */
	    if (top >= 0) {
		s[top].pos[s[top].branch_id - 1] = nextfreepos;
		nextfreepos += (s[top + 1].sn.level ? sidx_nodesize : sidx_leafsize);
	    }
	}
    }
    
    close(t->fd);
    
    return nextfreepos;
}

/* write RTree body to sidx file */
static off_t rtree_write_to_sidx(struct gvfile *fp, off_t startpos,
				 struct RTree *t, int off_t_size)
{
    if (t->fd > -1)
	return rtree_write_from_file(fp, startpos, t, off_t_size);
    else
	return rtree_write_from_memory(fp, startpos, t, off_t_size);
}

/*!
   \brief Load RTree body from sidx file to memory
   Must be called when old vector is opened in update mode

   \param fp pointer to struct gvfile
   \param rootpos position of root node in file
   \param t pointer to RTree
   \param off_t_size size of off_t used to read struct gvfile

   \return pointer to root node on success
 */

static void rtree_load_to_memory(struct gvfile *fp, off_t rootpos,
				  struct RTree *t, int off_t_size)
{
    struct RTree_Node *newnode = NULL;
    int i, j, loadnode, maxcard;
    struct spidxstack *last;
    static struct spidxstack *s = NULL;
    int top = 0;

    if (!s) {
	s = G_malloc(MAXLEVEL * sizeof(struct spidxstack));
	for (i = 0; i < MAXLEVEL; i++) {
	    s[i].sn.branch = G_malloc(MAXCARD * sizeof(struct RTree_Branch));
	    for (j = 0; j < MAXCARD; j++) {
		s[i].sn.branch[j].rect.boundary = G_malloc(6 * sizeof(RectReal));
	    }
	}
    }

    /* stack size of t->rootlevel + 1 would be enough because of
     * depth-first postorder traversal:
     * only one node per level on stack at any given time */

    /* add root node position to stack */
    last = &(s[top]);
    G_fseek(fp->file, rootpos, SEEK_SET);
    /* read with dig__fread_port_* fns */
    dig__fread_port_I(&(s[top].sn.count), 1, fp);
    dig__fread_port_I(&(s[top].sn.level), 1, fp);
    maxcard = s[top].sn.level ? t->nodecard : t->leafcard;
    for (j = 0; j < maxcard; j++) {
	dig__fread_port_D(s[top].sn.branch[j].rect.boundary, NUMSIDES, fp);
	dig__fread_port_O(&(s[top].pos[j]), 1, fp, off_t_size);
	/* leaf node: vector object IDs are stored in child.id */
	if (s[top].sn.level == 0) {
	    s[top].sn.branch[j].child.id = (int)s[top].pos[j];
	}
	else {
	    s[top].sn.branch[j].child.ptr = NULL;
	}
    }

    s[top].branch_id = i = 0;

    /* some sort of postorder traversal */
    /* root node is loaded last and returned */

    while (top >= 0) {
	last = &(s[top]);
	loadnode = 1;
	/* this is an internal node in the RTree
	 * all its children are read first,
	 * before it is transfered to the RTree in memory */
	if (s[top].sn.level > 0) {
	    for (i = s[top].branch_id; i < t->nodecard; i++) {
		if (s[top].pos[i] > 0) {
		    s[top++].branch_id = i + 1;
		    G_fseek(fp->file, last->pos[i], SEEK_SET);
		    /* read with dig__fread_port_* fns */
		    dig__fread_port_I(&(s[top].sn.count), 1, fp);
		    dig__fread_port_I(&(s[top].sn.level), 1, fp);
		    maxcard = s[top].sn.level ? t->nodecard : t->leafcard;
		    for (j = 0; j < maxcard; j++) {
			dig__fread_port_D(s[top].sn.branch[j].rect.boundary,
					  NUMSIDES, fp);
			dig__fread_port_O(&(s[top].pos[j]), 1, fp,
					  off_t_size);
			/* leaf node
			 * vector object IDs are stored in file as
			 * off_t but always fit into an int, see dig_structs.h
			 * vector object IDs are transfered to child.id */
			if (s[top].sn.level == 0) {
			    s[top].sn.branch[j].child.id =
				(int)s[top].pos[j];
			}
			else {
			    s[top].sn.branch[j].child.ptr = NULL;
			}
		    }
		    s[top].branch_id = 0;
		    loadnode = 0;
		    break;
		}
		else if (last->pos[i] < 0)
		    G_fatal_error("corrupt spatial index");
	    }
	    if (loadnode) {
		/* nothing else found, ready to load */
		s[top].branch_id = t->nodecard;
	    }
	}
	if (loadnode) {
	    /* ready to load node to memory */

	    newnode = RTreeAllocNode(t, s[top].sn.level);
	    /* copy from stack node */
	    RTreeCopyNode(newnode, &(s[top].sn), t);

	    top--;
	    /* update child of parent node
	     * this node is only updated if its level is > 0, i.e.
	     * this is an internal node
	     * children of internal nodes do not have an ID, instead
	     * they point to the next nodes down the tree */
	    if (top >= 0) {
		s[top].sn.branch[s[top].branch_id - 1].child.ptr = newnode;
	    }
	}
    }
    
    t->root = newnode;
}

/*!
   \brief Load RTree body from sidx file to temporary file
   Must be called when old vector is opened in update mode

   \param fp pointer to struct gvfile
   \param rootpos position of root node in file
   \param t pointer to RTree
   \param off_t_size size of off_t used to read struct gvfile

   \return offset to root node
 */

static void rtree_load_to_file(struct gvfile *fp, off_t rootpos,
				  struct RTree *t, int off_t_size)
{
    struct RTree_Node newnode;
    off_t newnode_pos = -1;
    int i, j, loadnode, maxcard;
    struct spidxstack *last;
    static struct spidxstack *s = NULL;
    int top = 0;

    if (!s) {
	s = G_malloc(MAXLEVEL * sizeof(struct spidxstack));
	for (i = 0; i < MAXLEVEL; i++) {
	    s[i].sn.branch = G_malloc(MAXCARD * sizeof(struct RTree_Branch));
	    for (j = 0; j < MAXCARD; j++) {
		s[i].sn.branch[j].rect.boundary = G_malloc(6 * sizeof(RectReal));
	    }
	}
    }
	
    /* stack size of t->rootlevel + 1 would be enough because of
     * depth-first postorder traversal:
     * only one node per level on stack at any given time */

    /* add root node position to stack */
    last = &(s[top]);
    G_fseek(fp->file, rootpos, SEEK_SET);
    /* read with dig__fread_port_* fns */
    dig__fread_port_I(&(s[top].sn.count), 1, fp);
    dig__fread_port_I(&(s[top].sn.level), 1, fp);
    maxcard = t->rootlevel ? t->nodecard : t->leafcard;
    for (j = 0; j < maxcard; j++) {
	dig__fread_port_D(s[top].sn.branch[j].rect.boundary, NUMSIDES, fp);
	dig__fread_port_O(&(s[top].pos[j]), 1, fp, off_t_size);
	/* leaf node: vector object IDs are stored in child.id */
	if (s[top].sn.level == 0) {
	    s[top].sn.branch[j].child.id = (int)s[top].pos[j];
	}
	else {
	    s[top].sn.branch[j].child.pos = -1;
	}
    }

    s[top].branch_id = i = 0;

    /* depth-first postorder traversal */
    /* root node is loaded last and returned */

    while (top >= 0) {
	last = &(s[top]);
	loadnode = 1;
	/* this is an internal node in the RTree
	 * all its children are read first,
	 * before it is transfered to the RTree in memory */
	if (s[top].sn.level > 0) {
	    for (i = s[top].branch_id; i < t->nodecard; i++) {
		if (s[top].pos[i] > 0) {
		    s[top++].branch_id = i + 1;
		    G_fseek(fp->file, last->pos[i], SEEK_SET);
		    /* read with dig__fread_port_* fns */
		    dig__fread_port_I(&(s[top].sn.count), 1, fp);
		    dig__fread_port_I(&(s[top].sn.level), 1, fp);
		    maxcard = s[top].sn.level ? t->nodecard : t->leafcard;
		    for (j = 0; j < maxcard; j++) {
			dig__fread_port_D(s[top].sn.branch[j].rect.boundary,
					  NUMSIDES, fp);
			dig__fread_port_O(&(s[top].pos[j]), 1, fp,
					  off_t_size);
			/* leaf node
			 * vector object IDs are stored in file as
			 * off_t but always fit into an int, see dig_structs.h
			 * vector object IDs are transfered to child.id */
			if (s[top].sn.level == 0) {
			    s[top].sn.branch[j].child.id =
				    (int)s[top].pos[j];
			}
			else {
			    s[top].sn.branch[j].child.pos = -1;
			}
		    }
		    s[top].branch_id = 0;
		    loadnode = 0;
		    break;
		}
		else if (last->pos[i] < 0)
		    G_fatal_error("corrupt spatial index");
	    }
	    if (loadnode) {
		/* nothing else found, ready to load */
		s[top].branch_id = t->nodecard;
	    }
	}
	if (loadnode) {
	    /* ready to load node and write to temp file */

	    /* copy from stack node */
	    newnode.level = s[top].sn.level;
	    newnode.count = s[top].sn.count;
	    maxcard = s[top].sn.level ? t->nodecard : t->leafcard;
	    for (j = 0; j < maxcard; j++) {
		newnode.branch[j].rect = s[top].sn.branch[j].rect;
		newnode.branch[j].child = s[top].sn.branch[j].child;
	    }
	    newnode_pos = RTreeGetNodePos(t);
	    RTreeWriteNode(&newnode, t);

	    top--;
	    /* update child of parent node
	     * this node is only updated if its level is > 0, i.e.
	     * this is an internal node
	     * children of internal nodes do not have an ID, instead
	     * they point to the next nodes down the tree */
	    if (top >= 0) {
		s[top].sn.branch[s[top].branch_id - 1].child.pos = newnode_pos;
	    }
	}
    }
    
    t->rootpos = newnode_pos;
}

static void rtree_load_from_sidx(struct gvfile *fp, off_t rootpos,
				  struct RTree *t, int off_t_size)
{
    if (t->fd > -1)
	return rtree_load_to_file(fp, rootpos, t, off_t_size);
    else
	return rtree_load_to_memory(fp, rootpos, t, off_t_size);
}

/*!
   \brief Write spatial index to file

   \param[out] fp pointer to struct gvfile
   \param Plus pointer to Plus_head structure

   \return 0
 */
int dig_Wr_spidx(struct gvfile *fp, struct Plus_head *Plus)
{
    G_debug(1, "dig_Wr_spidx()");

    dig_set_cur_port(&(Plus->spidx_port));
    dig_rewind(fp);

    dig_Wr_spidx_head(fp, Plus);

    /* Nodes */
    Plus->Node_spidx_offset =
	rtree_write_to_sidx(fp, dig_ftell(fp), Plus->Node_spidx,
			    Plus->spidx_port.off_t_size);

    /* Lines */
    Plus->Line_spidx_offset =
	rtree_write_to_sidx(fp, dig_ftell(fp), Plus->Line_spidx,
			    Plus->spidx_port.off_t_size);

    /* Areas */
    Plus->Area_spidx_offset =
	rtree_write_to_sidx(fp, dig_ftell(fp), Plus->Area_spidx,
			    Plus->spidx_port.off_t_size);

    /* Isles */
    Plus->Isle_spidx_offset =
	rtree_write_to_sidx(fp, dig_ftell(fp), Plus->Isle_spidx,
			    Plus->spidx_port.off_t_size);

    /* 3D future : */
    /* Faces */
    /* Volumes */
    /* Holes */

    dig_rewind(fp);
    dig_Wr_spidx_head(fp, Plus);	/* rewrite with offsets */

    dig_fflush(fp);
    return 0;
}

/*!
   \brief Read spatial index from sidx file
   Only needed when old vector is opened in update mode

   \param fp pointer to struct gvfile
   \param[in,out] Plus pointer to Plus_head structure

   \return 0
 */
int dig_Rd_spidx(struct gvfile * fp, struct Plus_head *Plus)
{
    G_debug(1, "dig_read_spindx()");

    /* free old trees, init new trees */
    dig_spidx_free(Plus);
    dig_spidx_init(Plus);

    dig_rewind(fp);
    dig_Rd_spidx_head(fp, Plus);
    dig_set_cur_port(&(Plus->spidx_port));

    /* Nodes */
    rtree_load_from_sidx(fp, Plus->Node_spidx_offset,
			 Plus->Node_spidx, Plus->spidx_port.off_t_size);

    /* Lines */
    rtree_load_from_sidx(fp, Plus->Line_spidx_offset,
			 Plus->Line_spidx, Plus->spidx_port.off_t_size);

    /* Areas */
    rtree_load_from_sidx(fp, Plus->Area_spidx_offset,
			 Plus->Area_spidx, Plus->spidx_port.off_t_size);

    /* Isles */
    rtree_load_from_sidx(fp, Plus->Isle_spidx_offset,
			 Plus->Isle_spidx, Plus->spidx_port.off_t_size);

    /* 3D future : */
    /* Faces */
    /* Volumes */
    /* Holes */

    return 0;
}

/*!
   \brief Dump spatial index

   \param[out] fp pointer to FILE
   \param Plus pointer to Plus_head structure

   \return 0
 */
int dig_dump_spidx(FILE * fp, const struct Plus_head *Plus)
{
    fprintf(fp, "Nodes\n");
    if (Plus->Node_spidx->fd < 0)
	rtree_dump_node(fp, Plus->Node_spidx->root, Plus->with_z);
    else {
	RTreeFlushBuffer(Plus->Node_spidx);
	rtree_dump_node_file(fp, Plus->Node_spidx->rootpos, Plus->with_z,
	                     Plus->Node_spidx);
    }

    fprintf(fp, "Lines\n");
    if (Plus->Line_spidx->fd < 0)
	rtree_dump_node(fp, Plus->Line_spidx->root, Plus->with_z);
    else {
	RTreeFlushBuffer(Plus->Line_spidx);
	rtree_dump_node_file(fp, Plus->Line_spidx->rootpos, Plus->with_z,
	                     Plus->Line_spidx);
    }

    fprintf(fp, "Areas\n");
    if (Plus->Area_spidx->fd < 0)
	rtree_dump_node(fp, Plus->Area_spidx->root, Plus->with_z);
    else {
	RTreeFlushBuffer(Plus->Area_spidx);
	rtree_dump_node_file(fp, Plus->Area_spidx->rootpos, Plus->with_z,
	                     Plus->Area_spidx);
    }

    fprintf(fp, "Isles\n");
    if (Plus->Isle_spidx->fd < 0)
	rtree_dump_node(fp, Plus->Isle_spidx->root, Plus->with_z);
    else {
	RTreeFlushBuffer(Plus->Isle_spidx);
	rtree_dump_node_file(fp, Plus->Isle_spidx->rootpos, Plus->with_z,
	                     Plus->Isle_spidx);
    }

    return 0;
}

/* read node from file */
static void rtree_read_node(struct NodeBuffer *nb,
                              off_t nodepos, struct RTree *t, struct Plus_head *Plus)
{
    int i, maxcard;
    off_t pos;
    struct gvfile *file = &(Plus->spidx_fp);

    dig_fseek(file, nodepos, SEEK_SET);
    /* read with dig__fread_port_* fns */
    dig__fread_port_I(&(nb->n.count), 1, file);
    dig__fread_port_I(&(nb->n.level), 1, file);
    maxcard = nb->n.level ? t->nodecard : t->leafcard;
    for (i = 0; i < maxcard; i++) {
	dig__fread_port_D(nb->n.branch[i].rect.boundary, NUMSIDES,
			  file);
	dig__fread_port_O(&pos, 1, file,
			  Plus->spidx_port.off_t_size);
	/* leaf node: vector object IDs are stored in child.id */
	if (nb->n.level == 0) {
	    nb->n.branch[i].child.id = (int)pos;
	}
	else {
	    nb->n.branch[i].child.pos = pos;
	}
    }
}

/* get node from buffer or file */
static struct RTree_Node *rtree_get_node(off_t nodepos, int level,
                                         struct RTree *t,
					 struct Plus_head *Plus)
{
    int which, i = 0;

    /* check mru first */
    /* t->used[level][i] */
    while (t->nb[level][t->used[level][i]].pos != nodepos &&
	   t->nb[level][t->used[level][i]].pos >= 0 &&
	   i < NODE_BUFFER_SIZE - 1) {
	i++;
    }

    which = t->used[level][i];

    if (t->nb[level][which].pos != nodepos) {
	rtree_read_node(&(t->nb[level][which]), nodepos, t, Plus);
	t->nb[level][which].pos = nodepos;
    }
    assert(t->nb[level][which].n.level == level);


    /* make it mru */
    if (i) { /* t->used[level][0] != which */
#if 0
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

    return &(t->nb[level][which].n);
}


/*!
   \brief Search spatial index file
   Can't use regular RTreeSearch() here because sidx must be read
   with dig__fread_port_*() functions

   \param t pointer to RTree
   \param r search rectangle
   \param shcb user-provided callback
   \param cbarg argument for shcb
   \param Plus pointer to Plus_head structure

   \return number of qualifying rectangles
 */
int rtree_search(struct RTree *t, struct RTree_Rect *r,
                 SearchHitCallback shcb, void *cbarg, struct Plus_head *Plus)
{
    int hitCount = 0, found;
    /* int j, maxcard; */
    int i;
    struct spidxpstack s[MAXLEVEL];
    int top = 0, level;
    off_t lastpos;

    assert(r);
    assert(t);

    /* stack size of t->rootlevel + 1 is enough because of depth first search */
    /* only one node per level on stack at any given time */

    dig_set_cur_port(&(Plus->spidx_port));

    /* add root node position to stack */
    s[top].sn = rtree_get_node(t->rootpos, t->rootlevel, t, Plus);
#if 0
    dig_fseek(&(Plus->spidx_fp), t->rootpos, SEEK_SET);
    /* read with dig__fread_port_* fns */
    dig__fread_port_I(&(s[top].sn.count), 1, &(Plus->spidx_fp));
    dig__fread_port_I(&(s[top].sn.level), 1, &(Plus->spidx_fp));
    maxcard = t->rootlevel ? t->nodecard : t->leafcard;
    for (j = 0; j < maxcard; j++) {
	dig__fread_port_D(s[top].sn.branch[j].rect.boundary, NUMSIDES,
			  &(Plus->spidx_fp));
	dig__fread_port_O(&(s[top].pos[j]), 1, &(Plus->spidx_fp),
			  Plus->spidx_port.off_t_size);
	/* leaf node: vector object IDs are stored in child.id */
	if (s[top].sn.level == 0) {
	    s[top].sn.branch[j].child.id = (int)s[top].pos[j];
	}
	else {
	    s[top].sn.branch[j].child.pos = s[top].pos[j];
	}
    }
#endif

    s[top].branch_id = i = 0;

    while (top >= 0) {
	level = s[top].sn->level;
	if (level > 0) {	/* this is an internal node in the tree */
	    found = 1;
	    for (i = s[top].branch_id; i < t->nodecard; i++) {
		lastpos = s[top].sn->branch[i].child.pos;
		if (lastpos > 0 &&
		    RTreeOverlap(r, &(s[top].sn->branch[i].rect), t)) {
		    s[top++].branch_id = i + 1;
		    s[top].sn = rtree_get_node(lastpos, level - 1, t, Plus);
		    
#if 0
		    dig_fseek(&(Plus->spidx_fp), lastpos, SEEK_SET);
		    /* read with dig__fread_port_* fns */
		    dig__fread_port_I(&(s[top].sn.count), 1,
				      &(Plus->spidx_fp));
		    dig__fread_port_I(&(s[top].sn.level), 1,
				      &(Plus->spidx_fp));
		    maxcard = s[top].sn.level ? t->nodecard : t->leafcard;
		    for (j = 0; j < maxcard; j++) {
			dig__fread_port_D(s[top].sn.branch[j].rect.boundary,
					  NUMSIDES, &(Plus->spidx_fp));
			dig__fread_port_O(&(s[top].pos[j]), 1,
					  &(Plus->spidx_fp),
					  Plus->spidx_port.off_t_size);
			if (s[top].sn.level == 0) {
			    s[top].sn.branch[j].child.id = (int)s[top].pos[j];
			}
			else {
			    s[top].sn.branch[j].child.pos = s[top].pos[j];
			}
		    }
#endif
		    s[top].branch_id = 0;
		    found = 0;
		    break;
		}
	    }
	    if (found) {
		/* nothing else found, go back up */
		s[top].branch_id = t->nodecard;
		top--;
	    }
	}
	else {			/* this is a leaf node */
	    for (i = 0; i < t->leafcard; i++) {
		if (s[top].sn->branch[i].child.id &&
		    RTreeOverlap(r, &(s[top].sn->branch[i].rect), t)) {
		    hitCount++;
		    if (shcb) {	/* call the user-provided callback */
			if (!shcb((int)s[top].sn->branch[i].child.id,
				  &s[top].sn->branch[i].rect, cbarg)) {
			    /* callback wants to terminate search early */
			    return hitCount;
			}
		    }
		}
	    }
	    top--;
	}
    }

    return hitCount;
}
