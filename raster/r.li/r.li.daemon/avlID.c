/*
 *      \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "defs.h"
#include "avlDefs.h"
#include "avlID.h"

static avlID_node *avlID_individua(const avlID_tree root, const long k,
				   avlID_node ** father, int *direction);
static int avlID_height(const avlID_tree root);
static avlID_node *critical_node(avlID_node * added, int *pos1, int *pos2,
				 const avlID_node * prec);

void avlID_rotation_ll(avlID_node * critical);
void avlID_rotation_lr(avlID_node * critical);
void avlID_rotation_rl(avlID_node * critical);
void avlID_rotation_rr(avlID_node * critical);


avlID_tree avlID_make(const long k, const long n)
{
    avlID_node *root = NULL;	/* root pointer */

    /* create root */
    root = G_malloc(sizeof(avlID_node));
    if (root == NULL)
	return NULL;

    /* initialize root  */
    root->right_child = NULL;
    root->left_child = NULL;
    root->father = NULL;
    root->counter = n;
    root->id = k;

    return root;
}

void avlID_destroy(avlID_tree root)
{
    struct avlID_node *it;
    struct avlID_node *save = root;

    /*
    Rotate away the left links so that
    we can treat this like the destruction
    of a linked list
    */
    while((it = save) != NULL) {
	if (it->left_child == NULL) {
	    /* No left links, just kill the node and move on */
	    save = it->right_child;
	    G_free(it);
	    it = NULL;
	}
	else {
	    /* Rotate away the left link and check again */
	    save = it->left_child;
	    it->left_child = save->right_child;
	    save->right_child = it;
	}
    }
}

long howManyID(const avlID_tree root, const long k)
{
    avlID_node *nodo = NULL;

    nodo = avlID_find(root, k);
    if (nodo == NULL)
	return 0;
    else
	return nodo->counter;
}


avlID_node *avlID_find(const avlID_tree root, const long k)
{
    avlID_node *p = NULL;
    int d = 0;

    if (root == NULL)
	return NULL;

    return avlID_individua(root, k, &p, &d);
}

long avlID_sub(avlID_tree * root, const long k)
{
    long ris = 0;
    avlID_node *nodo = NULL;

    nodo = avlID_find(*root, k);
    if (nodo != NULL) {
	ris = nodo->counter;
	nodo->counter = 0;
    }
    return ris;
}

int avlID_add(avlID_tree * root, const long k, const long n)
{
    int d = 0;			/* -1 if the new node is the left child
				   1 if the new node is the right child */
    int pos1 = 0, pos2 = 0;
    int rotation = 0;		/* rotation type to balance tree */

    avlID_node *node_temp = NULL;
    avlID_node *critical = NULL;
    avlID_node *p = NULL;	/* father pointer of new node */


    if ((root == NULL) || (*root == NULL))
	return AVL_ERR;

    /* find where insert the new node */
    node_temp = avlID_individua(*root, k, &p, &d);
    if (node_temp != NULL) {
	node_temp->counter = node_temp->counter + n;
	return AVL_PRES;	/* key already exists in the tree, only update the counter */
    }

    /* create the new node */
    node_temp = avlID_make(k, n);
    if (node_temp == NULL)
	return AVL_ERR;

    /* insert the new node */
    node_temp->father = p;
    if (d == -1)
	p->left_child = node_temp;
    else {
	if (d == 1)
	    p->right_child = node_temp;
	else {
	    G_free(node_temp);
	    return AVL_ERR;
	}
    }

    /* if necessary balance the tree */
    critical = critical_node(node_temp, &pos1, &pos2, NULL);
    if (critical == NULL)
	return AVL_ADD;		/* not necessary */

    /* balance */
    rotation = (pos1 * 10) + pos2;

    switch (rotation) {		/* rotate */
    case AVL_SS:
	avlID_rotation_ll(critical);
	break;
    case AVL_SD:
	avlID_rotation_lr(critical);
	break;
    case AVL_DS:
	avlID_rotation_rl(critical);
	break;
    case AVL_DD:
	avlID_rotation_rr(critical);
	break;
    default:
	G_fatal_error("avl, avlID_add: balancing error\n");
	return AVL_ERR;
    }

    /* if after rotation there is a new root update the root pointer */
    while ((*root)->father != NULL)
	*root = (*root)->father;

    return AVL_ADD;
}




long avlID_to_array(avlID_node * root, long i, avlID_table * a)
{

    if (root != NULL) {
	i = avlID_to_array(root->left_child, i, a);
	if (a == NULL)
	    G_fatal_error("avl, avlID_to_array: null value");
	else {
	    a[i] = G_malloc(sizeof(avlID_tableRow));
	    a[i]->k = root->id;
	    a[i]->tot = root->counter;
	    i++;
	    i = avlID_to_array(root->right_child, i, a);
	}
    }
    return i;
}




/* private functions */

static avlID_node *avlID_individua(const avlID_tree root, const long k,
				   avlID_node ** father, int *direction)
{

    if (root == NULL)
	return NULL;
    if (root->id == k)
	return root;
    else {
	if (root->id > k) {
	    *father = root;
	    *direction = -1;
	    return avlID_individua(root->left_child, k, father, direction);
	}
	else {			/* key < k */

	    *father = root;
	    *direction = 1;
	    return avlID_individua(root->right_child, k, father, direction);
	}
    }
}


static int avlID_height(const avlID_tree root)
{
    if (root == NULL)
	return -1;
    else {
	int tmp1 = avlID_height(root->left_child);
	int tmp2 = avlID_height(root->right_child);

	return (1 + ((tmp1 > tmp2) ? tmp1 : tmp2));
    }

}


static avlID_node *critical_node(avlID_node * added, int *pos1, int *pos2,
				 const avlID_node * prec)
{
    int fdb = 0;

    if (added == NULL)
	return NULL;

    if (prec == NULL)
	*pos1 = *pos2 = 0;
    else {
	*pos2 = *pos1;
	if (prec == added->left_child)
	    *pos1 = AVL_S;
	else
	    *pos1 = AVL_D;	/* prec == added->right_child */
    }


    fdb =
	abs(avlID_height(added->left_child) -
	    avlID_height(added->right_child));

    if (fdb > 1)
	return added;
    else {
	prec = added;
	return critical_node(added->father, pos1, pos2, prec);
    }

}


void avlID_rotation_ll(avlID_node * critical)
{
    avlID_node *b = NULL;
    avlID_node *r = critical;
    avlID_node *s = critical->left_child;

    s->father = r->father;

    if (r->father != NULL) {
	if ((r->father)->left_child == r)
	    (r->father)->left_child = s;
	else
	    (r->father)->right_child = s;
    }

    b = s->right_child;
    s->right_child = r;
    r->father = s;
    r->left_child = b;

    if (b != NULL)
	b->father = r;
}


void avlID_rotation_rr(avlID_node * critical)
{
    avlID_node *b = NULL;
    avlID_node *r = critical;
    avlID_node *s = critical->right_child;

    s->father = r->father;

    if (r->father != NULL) {
	if ((r->father)->left_child == r)
	    (r->father)->left_child = s;
	else
	    (r->father)->right_child = s;
    }

    b = s->left_child;
    s->left_child = r;
    r->father = s;
    r->right_child = b;

    if (b != NULL)
	b->father = r;
}


void avlID_rotation_lr(avlID_node * critical)
{
    avlID_node *b = NULL;
    avlID_node *g = NULL;
    avlID_node *r = critical;
    avlID_node *s = critical->left_child;
    avlID_node *t = (critical->left_child)->right_child;

    t->father = r->father;

    if (r->father != NULL) {
	if ((r->father)->left_child == r)
	    (r->father)->left_child = t;
	else
	    (r->father)->right_child = t;
    }

    b = t->left_child;
    g = t->right_child;

    t->left_child = s;
    t->right_child = r;
    r->father = t;
    s->father = t;

    s->right_child = b;
    r->left_child = g;

    if (b != NULL)
	b->father = s;
    if (g != NULL)
	g->father = r;
}


void avlID_rotation_rl(avlID_node * critical)
{
    avlID_node *b = NULL;
    avlID_node *g = NULL;
    avlID_node *r = critical;
    avlID_node *s = critical->right_child;
    avlID_node *t = (critical->right_child)->left_child;

    t->father = r->father;

    if (r->father != NULL) {
	if ((r->father)->left_child == r)
	    (r->father)->left_child = t;
	else
	    (r->father)->right_child = t;
    }

    b = t->left_child;
    g = t->right_child;

    t->left_child = r;
    t->right_child = s;
    r->father = t;
    s->father = t;

    r->right_child = b;
    s->left_child = g;

    if (b != NULL)
	b->father = r;
    if (g != NULL)
	g->father = s;
}
