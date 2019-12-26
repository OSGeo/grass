/*!
 * \file kdtree.h
 *
 * \brief Dynamic balanced k-d tree implementation
 *
 * k-d tree is a multidimensional (k-dimensional) binary search tree for
 * nearest neighbor search and is part of \ref btree2.
 *
 * Copyright and license:
 *
 * (C) 2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Markus Metz
 *
 * \par References
 * Bentley, J. L. (1975). "Multidimensional binary search trees used for 
 * associative searching". Communications of the ACM 18 (9): 509.
 * doi:10.1145/361002.361007
 *
 * \par Features
 * - This k-d tree is a dynamic tree:
 *   elements can be inserted and removed any time.
 * - This k-d tree is balanced:
 *   subtrees have a similar depth (the difference in subtrees' depths is 
 *   not allowed to be larger than the balancing tolerance).
 *
 * Here is a structure of basic usage:
 *
 * Create a new k-d tree:
 *
 *     kdtree_create(...);
 *
 * Insert points into the tree:
 *
 *     kdtree_insert(...);
 *
 * Optionally optimize the tree:
 * 
 *     kdtree_optimize(...);
 *
 * Search k nearest neighbors:
 *
 *     kdtree_knn(...);
 *
 * Search all neighbors in radius:
 *
 *     kdtree_dnn(...);
 *
 * Destroy the tree:
 *
 *     kdtree_destroy(...);
 *
 * \todo
 * Doxygen ignores comment for last parameter after `);`.
 * The parameter description now goes to the end of function description.
 *
 * \todo
 * Include formatting to function descriptions.
 */

/*!
 * \brief Node for k-d tree
 */
struct kdnode
{
    unsigned char dim;          /*!< split dimension of this node */
    unsigned char depth;        /*!< depth at this node */
    unsigned char balance;      /*!< flag to indicate if balancing is needed */
    double *c;                  /*!< coordinates */
    int uid;                    /*!< unique id of this node */
    struct kdnode *child[2];    /*!< link to children: `[0]` for smaller, `[1]` for larger */
};

/*!
 * \brief k-d tree
 */
struct kdtree
{
    unsigned char ndims;        /*!< number of dimensions */
    unsigned char *nextdim;     /*!< split dimension of child nodes */
    int csize;                  /*!< size of coordinates in bytes */
    int btol;                   /*!< balancing tolerance */
    size_t count;               /*!< number of items in the tree */
    struct kdnode *root;        /*!< tree root */
};

/*!
 * \brief k-d tree traversal
 */
struct kdtrav
{
    struct kdtree *tree;        /*!< tree being traversed */
    struct kdnode *curr_node;   /*!< current node */
    struct kdnode *up[256];     /*!< stack of parent nodes */
    int top;                    /*!< index for stack */
    int first;                  /*!< little helper flag */
};

/*! creae a new k-d tree */
struct kdtree *kdtree_create(char ndims,        /*!< number of dimensions */
                             int *btol  /*!< optional balancing tolerance */
    );

/*! destroy a tree */
void kdtree_destroy(struct kdtree *t);

/*! clear a tree, removing all entries */
void kdtree_clear(struct kdtree *t);

/*! insert an item (coordinates c and uid) into the k-d tree */
int kdtree_insert(struct kdtree *t,     /*!< k-d tree */
                  double *c,    /*!< coordinates */
                  int uid,      /*!< the point's unique id */
                  int dc        /*!< allow duplicate coordinates */
    );

/*! remove an item from the k-d tree
 * coordinates c and uid must match */
int kdtree_remove(struct kdtree *t,     /*!< k-d tree */
                  double *c,    /*!< coordinates */
                  int uid       /*!< the point's unique id */
    );

/*! find k nearest neighbors 
 * results are stored in uid (uids) and d (squared distances)
 * optionally an uid to be skipped can be given
 * useful when searching for the nearest neighbors of an item 
 * that is also in the tree */
int kdtree_knn(struct kdtree *t,        /*!< k-d tree */
               double *c,       /*!< coordinates */
               int *uid,        /*!< unique ids of the neighbors */
               double *d,       /*!< squared distances to the neighbors */
               int k,           /*!< number of neighbors to find */
               int *skip        /*!< unique id to skip */
    );

/*! find all nearest neighbors within distance aka radius search
 * results are stored in puid (uids) and pd (squared distances)
 * memory is allocated as needed, the calling fn must free the memory
 * optionally an uid to be skipped can be given */
int kdtree_dnn(struct kdtree *t,        /*!< k-d tree */
               double *c,       /*!< coordinates */
               int **puid,      /*!< unique ids of the neighbors */
               double **pd,     /*!< squared distances to the neighbors */
               double maxdist,  /*!< radius to search around the given coordinates */
               int *skip        /*!< unique id to skip */
    );

/*! find all nearest neighbors within range aka box search
 * the range is specified with min and max for each dimension as
 * (min1, min2, ..., minn, max1, max2, ..., maxn)
 * results are stored in puid (uids) and pd (squared distances)
 * memory is allocated as needed, the calling fn must free the memory
 * optionally an uid to be skipped can be given */
int kdtree_rnn(struct kdtree *t,        /*!< k-d tree */
               double *c,       /*!< coordinates for range */
               int **puid,      /*!< unique ids of the neighbors */
               int *skip        /*!< unique id to skip */
    );

/*! k-d tree optimization, only useful if the tree will be heavily used
 * (more searches than items in the tree)
 * level 0 = a bit, 1 = more, 2 = a lot */
void kdtree_optimize(struct kdtree *t,  /*!< k-d tree */
                     int level  /*!< optimization level */
    );

/*! initialize tree traversal
 * (re-)sets trav structure
 * returns 0
 */
int kdtree_init_trav(struct kdtrav *trav, struct kdtree *tree);

/*! traverse the tree
 * useful to get all items in the tree non-recursively
 * struct kdtrav *trav needs to be initialized first
 * returns 1, 0 when finished
 */
int kdtree_traverse(struct kdtrav *trav, double *c, int *uid);
