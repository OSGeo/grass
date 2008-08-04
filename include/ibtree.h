typedef struct
{
    int key;
    int data;
    int left;
    int right;
} IBTREE_NODE;

typedef struct
{
    IBTREE_NODE *node;		/* tree of values */
    int tlen;			/* allocated tree size */
    int N;			/* number of actual nodes in tree */
    int incr;			/* number of nodes to add at a time */
    int cur;
    int (*cmp) ();		/* routine to compare keys */
} IBTREE;

int ibtree_create(IBTREE *, int (*)(), int);
int ibtree_find(IBTREE *, int, int *);
int ibtree_free(IBTREE *);
int ibtree_next(IBTREE *, int *, int *);
int ibtree_rewind(IBTREE *);
int Btree_init();
int Btree_add(int);
int Btree_report();
int ibtree_update(IBTREE *, int, int);
