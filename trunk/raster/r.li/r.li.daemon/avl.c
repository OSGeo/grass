/*
 *      \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "defs.h"
#include "avlDefs.h"
#include "avl.h"


static avl_node *avl_individua(const avl_tree root, const generic_cell k,
			       avl_node ** father, int *direction);
static int avl_height(const avl_tree root);
static avl_node *critical_node(avl_node * added, int *pos1, int *pos2,
			       const avl_node * prec);

void avl_rotation_ll(avl_node * critical);
void avl_rotation_lr(avl_node * critical);
void avl_rotation_rl(avl_node * critical);
void avl_rotation_rr(avl_node * critical);
void printAVL(avl_node * r);


/* define function declared in avl.h */

avl_tree avl_make(const generic_cell k, const long n)
{

    avl_node *root = NULL;	/* tree root pointer */

    /* create root */
    root = G_malloc(sizeof(avl_node));
    if (root == NULL) {
	G_fatal_error("avl.c: avl_make: malloc error");
	return NULL;
    }

    /* inizialize root */
    root->right_child = NULL;
    root->left_child = NULL;
    root->father = NULL;
    root->counter = n;
    root->key = k;

    return root;
}

void avl_destroy(avl_tree root)
{
    struct avl_node *it;
    struct avl_node *save = root;

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

long howManyCell(const avl_tree root, const generic_cell k)
{
    avl_node *nodo = NULL;

    nodo = avl_find(root, k);
    if (nodo == NULL)
	return 0;
    else
	return nodo->counter;
}


avl_node *avl_find(const avl_tree root, const generic_cell k)
{
    avl_node *p = NULL;

    int d = 0;

    if (root == NULL)
	return NULL;

    return avl_individua(root, k, &p, &d);
}


int avl_add(avl_tree * root, const generic_cell k, const long n)
{

    avl_node *p = NULL;
    avl_node *node_temp = NULL;
    avl_node *critical = NULL;

    int d = 0;
    int pos1 = 0, pos2 = 0;
    int rotation = 0;



    if ((root == NULL) || (*root == NULL)) {
	G_fatal_error("\navl.c: avl_add: param NULL");
	return AVL_ERR;
    }


    /* search position where insert the new node */
    node_temp = avl_individua(*root, k, &p, &d);

    if (node_temp != NULL) {
	node_temp->counter = node_temp->counter + n;
	return AVL_PRES;
    }

    node_temp = avl_make(k, n);
    if (node_temp == NULL) {
	G_fatal_error("\navl.c:  avl_add: create node error");
	return AVL_ERR;
    }


    /* link the new node */
    node_temp->father = p;

    if (d == -1) {
	p->left_child = node_temp;
    }
    else {
	if (d == 1) {
	    p->right_child = node_temp;
	}
	else {
	    G_free(node_temp);
	    G_fatal_error("avl.c: avl_add: new node position unknown");
	    return AVL_ERR;
	}
    }

    /* if it's necessary balance the tree */
    critical = critical_node(node_temp, &pos1, &pos2, NULL);
    if (critical == NULL)
	return AVL_ADD;
    rotation = (pos1 * 10) + pos2;

    switch (rotation) {
    case AVL_SS:
	avl_rotation_ll(critical);
	break;
    case AVL_SD:
	avl_rotation_lr(critical);
	break;
    case AVL_DS:
	avl_rotation_rl(critical);
	break;
    case AVL_DD:
	avl_rotation_rr(critical);
	break;
    default:
	G_fatal_error("avl, avl_add: balancing error\n");
	return AVL_ERR;
    }

    /* if after rotation the root is changed modufy the pointer to the root */
    while ((*root)->father != NULL)
	*root = (*root)->father;

    return AVL_ADD;
}




long avl_to_array(avl_node * root, long i, AVL_table a)
{

    if (root != NULL) {
	i = avl_to_array(root->left_child, i, a);
	if (a == NULL)
	    G_fatal_error("avl, avl_to_array: null value");
	else {
	    a[i].k = root->key;
	    a[i].tot = root->counter;
	    i++;
	    i = avl_to_array(root->right_child, i, a);
	}
    }
    return i;
}




static avl_node *avl_individua(const avl_tree root, const generic_cell k,
			       avl_node ** father, int *direction)
{
    int ris = 0;


    if (root == NULL) {
	return NULL;
    }

    ris = equalsGenericCell(root->key, k);


    switch (ris) {
    case GC_EQUAL:
	{
	    return root;
	    break;
	}
    case GC_HIGHER:
	{
	    *father = root;
	    *direction = -1;
	    return avl_individua(root->left_child, k, father, direction);
	}
    case GC_LOWER:
	{
	    *father = root;
	    *direction = 1;
	    return avl_individua(root->right_child, k, father, direction);
	}
    case GC_DIFFERENT_TYPE:
	{
	    G_fatal_error("\avl.c: avl_individua: different type");
	    return NULL;
	}
    default:
	{
	    G_fatal_error("\avl.c: avl_individua: error");
	    return NULL;
	}
    }

}


static int avl_height(const avl_tree root)
{
    if (root == NULL)
	return -1;
    else {
	int tmp1 = avl_height(root->left_child);
	int tmp2 = avl_height(root->right_child);

	return (1 + ((tmp1 > tmp2) ? tmp1 : tmp2));
    }

}


static avl_node *critical_node(avl_node * added, int *pos1, int *pos2,
			       const avl_node * prec)
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


    fdb = abs(avl_height(added->left_child) - avl_height(added->right_child));

    if (fdb > 1)
	return added;
    else {
	prec = added;
	return critical_node(added->father, pos1, pos2, prec);
    }

}


void avl_rotation_ll(avl_node * critical)
{
    avl_node *b = NULL;
    avl_node *r = critical;
    avl_node *s = critical->left_child;

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


void avl_rotation_rr(avl_node * critical)
{
    avl_node *b = NULL;
    avl_node *r = critical;
    avl_node *s = critical->right_child;

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


void avl_rotation_lr(avl_node * critical)
{
    avl_node *b = NULL;
    avl_node *g = NULL;
    avl_node *r = critical;
    avl_node *s = critical->left_child;
    avl_node *t = (critical->left_child)->right_child;

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


void avl_rotation_rl(avl_node * critical)
{
    avl_node *b = NULL;
    avl_node *g = NULL;
    avl_node *r = critical;
    avl_node *s = critical->right_child;
    avl_node *t = (critical->right_child)->left_child;

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
