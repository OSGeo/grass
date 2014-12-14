
struct kdnode
{
    unsigned char dim;		/* split dimension of this node */
    unsigned char depth;	/* depth at this node */
    double *c;			/* coordinates */
    int uid;			/* unique id of this node */
    struct kdnode *child[2];	/* link to children: link[0] for smaller, link[1] for larger */
};

struct kdtree
{
    unsigned char ndims;	/* number of dimensions */
    unsigned char *nextdim;	/* split dimension of child nodes */
    int csize;			/* size of coordinates in bytes */
    int btol;			/* balancing tolerance */
    size_t count;		/* number of items in the tree */
    struct kdnode *root;	/* tree root */
};

struct kdtree *kdtree_create(char, int *);
void kdtree_destroy(struct kdtree *);
void kdtree_clear(struct kdtree *);
int kdtree_insert(struct kdtree *, double *, int, int);
int kdtree_remove(struct kdtree *, double *, int);
int kdtree_knn(struct kdtree *, double *, int *, double *, int, int *);
int kdtree_dnn(struct kdtree *, double *, int **, double **, double, int *);
