
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *
 * Date:         july 2008 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The height of a cell is assumed to be constant, and the
 * terrain is viewed as a tesselation of flat cells.  This model is
 * suitable for high resolution rasters; it may not be accurate for
 * low resolution rasters, where it may be better to interpolate the
 * height at a point based on the neighbors, rather than assuming
 * cells are "flat".  The viewshed algorithm is efficient both in
 * terms of CPU operations and I/O operations. It has worst-case
 * complexity O(n lg n) in the RAM model and O(sort(n)) in the
 * I/O-model.  For the algorithm and all the other details see the
 * paper: "Computing Visibility on * Terrains in External Memory" by
 * Herman Haverkort, Laura Toma and Yi Zhuang.
 *
 * COPYRIGHT: (C) 2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/

/*

   A R/B BST. Always call initNILnode() before using the tree.
   Version 0.0.0

   Version 0.0.1
   Rewrote BST Deletion to improve efficiency

   Version 0.0.2
   Bug fixed in deletion.
   CLRS pseudocode forgot to make sure that x is not NIL before
   calling rbDeleteFixup(root,x).

   Version 0.0.3
   Some Cleanup. Separated the public portion and the 
   private porthion of the interface in the header


   =================================
   This is based on BST 1.0.4
   BST change log
   <---------------->
   find max is implemented in this version.
   Version 1.0.2

   Version 1.0.4 
   Major bug fix in deletion (when the node has two children, 
   one of them has a wrong parent pointer after the rotation in the deletion.)
   <----------------->
 */


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "rbbst.h"

#ifdef __GRASS__
extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
}
#endif



TreeNode *NIL;

#define EPSILON 0.0000001


/*public:--------------------------------- */
RBTree *create_tree(TreeValue tv)
{
    init_nil_node();
#ifdef __GRASS__
    RBTree *rbt = (RBTree *) G_malloc(sizeof(RBTree));
    TreeNode *root = (TreeNode *) G_malloc(sizeof(TreeNode));
#else
    RBTree *rbt = (RBTree *) malloc(sizeof(RBTree));
    TreeNode *root = (TreeNode *) malloc(sizeof(TreeNode));
#endif

    rbt->root = root;
    rbt->root->value = tv;
    rbt->root->left = NIL;
    rbt->root->right = NIL;
    rbt->root->parent = NIL;
    rbt->root->color = RB_BLACK;

    return rbt;
}

/*LT: not sure if this is correct */
int is_empty(RBTree * t)
{
    assert(t);
    return (t->root == NIL);
}

void delete_tree(RBTree * t)
{
    destroy_sub_tree(t->root);
    return;
}

void destroy_sub_tree(TreeNode * node)
{
    if (node == NIL)
	return;

    destroy_sub_tree(node->left);
    destroy_sub_tree(node->right);
#ifdef __GRASS__
    G_free(node);
#else
    free(node);
#endif
    return;
}

void insert_into(RBTree * rbt, TreeValue value)
{
    insert_into_tree(&(rbt->root), value);
    return;
}

void delete_from(RBTree * rbt, double key)
{
    delete_from_tree(&(rbt->root), key);
    return;
}

TreeNode *search_for_node_with_key(RBTree * rbt, double key)
{
    return search_for_node(rbt->root, key);
}

/*------------The following is designed for viewshed's algorithm-------*/
double find_max_gradient_within_key(RBTree * rbt, double key)
{
    return find_max_value_within_key(rbt->root, key);
}

/*<--------------------------------->
   //Private below this line */
void init_nil_node()
{
#ifdef __GRASS__
    NIL = (TreeNode *) G_malloc(sizeof(TreeNode));
#else
    NIL = (TreeNode *) malloc(sizeof(TreeNode));
#endif
    NIL->color = RB_BLACK;
    NIL->value.gradient = SMALLEST_GRADIENT;
    NIL->value.maxGradient = SMALLEST_GRADIENT;

    NIL->parent = NULL;
    NIL->left = NULL;
    NIL->right = NULL;
    return;
}

/*you can write change this compare function, depending on your TreeValue struct
   //compare function used by findMaxValue
   //-1: v1 < v2
   //0:  v1 = v2
   //2:  v1 > v2 */
char compare_values(TreeValue * v1, TreeValue * v2)
{
    if (v1->gradient > v2->gradient)
	return 1;
    if (v1->gradient < v2->gradient)
	return -1;

    return 0;
}


/*a function used to compare two doubles */
char compare_double(double a, double b)
{
    if (fabs(a - b) < EPSILON)
	return 0;
    if (a - b < 0)
	return -1;

    return 1;
}



/*create a tree node */
TreeNode *create_tree_node(TreeValue value)
{
    TreeNode *ret;

#ifdef __GRASS__
    ret = (TreeNode *) G_malloc(sizeof(TreeNode));
#else
    ret = (TreeNode *) malloc(sizeof(TreeNode));
#endif

    ret->color = RB_RED;

    ret->left = NIL;
    ret->right = NIL;
    ret->parent = NIL;

    ret->value = value;
    ret->value.maxGradient = SMALLEST_GRADIENT;
    return ret;
}

/*create node with its value set to the value given
   //and insert the node into the tree
   //rbInsertFixup may change the root pointer, so TreeNode** is passed in */
void insert_into_tree(TreeNode ** root, TreeValue value)
{
    TreeNode *curNode;
    TreeNode *nextNode;

    curNode = *root;

    if (compare_double(value.key, curNode->value.key) == -1) {
	nextNode = curNode->left;
    }
    else {
	nextNode = curNode->right;
    }


    while (nextNode != NIL) {
	curNode = nextNode;

	if (compare_double(value.key, curNode->value.key) == -1) {
	    nextNode = curNode->left;
	}
	else {
	    nextNode = curNode->right;
	}
    }

    /*create a new node 
       //and place it at the right place
       //created node is RED by default */
    nextNode = create_tree_node(value);

    nextNode->parent = curNode;

    if (compare_double(value.key, curNode->value.key) == -1) {
	curNode->left = nextNode;
    }
    else {
	curNode->right = nextNode;
    }

    TreeNode *inserted = nextNode;

    /*update augmented maxGradient */
    nextNode->value.maxGradient = nextNode->value.gradient;
    while (nextNode->parent != NIL) {
	if (nextNode->parent->value.maxGradient < nextNode->value.maxGradient)
	    nextNode->parent->value.maxGradient = nextNode->value.maxGradient;

	if (nextNode->parent->value.maxGradient > nextNode->value.maxGradient)
	    break;
	nextNode = nextNode->parent;
    }

    /*fix rb tree after insertion */
    rb_insert_fixup(root, inserted);

    return;
}

void rb_insert_fixup(TreeNode ** root, TreeNode * z)
{
    /*see pseudocode on page 281 in CLRS */
    TreeNode *y;

    while (z->parent->color == RB_RED) {
	if (z->parent == z->parent->parent->left) {
	    y = z->parent->parent->right;
	    if (y->color == RB_RED) {	/*case 1 */
		z->parent->color = RB_BLACK;
		y->color = RB_BLACK;
		z->parent->parent->color = RB_RED;
		z = z->parent->parent;
	    }
	    else {
		if (z == z->parent->right) {	/*case 2 */
		    z = z->parent;
		    left_rotate(root, z);	/*convert case 2 to case 3 */
		}
		z->parent->color = RB_BLACK;	/*case 3 */
		z->parent->parent->color = RB_RED;
		right_rotate(root, z->parent->parent);
	    }

	}
	else {			/*(z->parent == z->parent->parent->right) */
	    y = z->parent->parent->left;
	    if (y->color == RB_RED) {	/*case 1 */
		z->parent->color = RB_BLACK;
		y->color = RB_BLACK;
		z->parent->parent->color = RB_RED;
		z = z->parent->parent;
	    }
	    else {
		if (z == z->parent->left) {	/*case 2 */
		    z = z->parent;
		    right_rotate(root, z);	/*convert case 2 to case 3 */
		}
		z->parent->color = RB_BLACK;	/*case 3 */
		z->parent->parent->color = RB_RED;
		left_rotate(root, z->parent->parent);
	    }
	}
    }
    (*root)->color = RB_BLACK;

    return;
}




/*search for a node with the given key */
TreeNode *search_for_node(TreeNode * root, double key)
{
    TreeNode *curNode = root;

    while (curNode != NIL && compare_double(key, curNode->value.key) != 0) {

	if (compare_double(key, curNode->value.key) == -1) {
	    curNode = curNode->left;
	}
	else {
	    curNode = curNode->right;
	}

    }

    return curNode;
}

/*function used by treeSuccessor */
TreeNode *tree_minimum(TreeNode * x)
{
    while (x->left != NIL)
	x = x->left;

    return x;
}

/*function used by deletion */
TreeNode *tree_successor(TreeNode * x)
{
    if (x->right != NIL)
	return tree_minimum(x->right);
    TreeNode *y = x->parent;

    while (y != NIL && x == y->right) {
	x = y;
	y = y->parent;
    }
    return y;
}


/*delete the node out of the tree */
void delete_from_tree(TreeNode ** root, double key)
{
    double tmpMax;
    TreeNode *z;
    TreeNode *x;
    TreeNode *y;
    TreeNode *toFix;

    z = search_for_node(*root, key);

    if (z == NIL) {
	printf("ATTEMPT to delete key=%f failed\n", key);
	fprintf(stderr, "Node not found. Deletion fails.\n");
	exit(1);
	return;			/*node to delete is not found */
    }

    /*1-3 */
    if (z->left == NIL || z->right == NIL)
	y = z;
    else
	y = tree_successor(z);

    /*4-6 */
    if (y->left != NIL)
	x = y->left;
    else
	x = y->right;

    /*7 */
    x->parent = y->parent;

    /*8-12 */
    if (y->parent == NIL) {
	*root = x;

	toFix = *root;		/*augmentation to be fixed */
    }
    else {
	if (y == y->parent->left)
	    y->parent->left = x;
	else
	    y->parent->right = x;

	toFix = y->parent;	/*augmentation to be fixed */
    }

    /*fix augmentation for removing y */
    TreeNode *curNode = y;
    double left, right;

    while (curNode->parent != NIL) {
	if (curNode->parent->value.maxGradient == y->value.gradient) {
	    left = find_max_value(curNode->parent->left);
	    right = find_max_value(curNode->parent->right);

	    if (left > right)
		curNode->parent->value.maxGradient = left;
	    else
		curNode->parent->value.maxGradient = right;

	    if (curNode->parent->value.gradient >
		curNode->parent->value.maxGradient)
		curNode->parent->value.maxGradient =
		    curNode->parent->value.gradient;
	}
	else {
	    break;
	}
	curNode = curNode->parent;
    }


    /*fix augmentation for x */
    tmpMax =
	toFix->left->value.maxGradient >
	toFix->right->value.maxGradient ? toFix->left->value.
	maxGradient : toFix->right->value.maxGradient;
    if (tmpMax > toFix->value.gradient)
	toFix->value.maxGradient = tmpMax;
    else
	toFix->value.maxGradient = toFix->value.gradient;

    /*13-15 */
    if (y != z) {
	double zGradient = z->value.gradient;

	z->value.key = y->value.key;
	z->value.gradient = y->value.gradient;


	toFix = z;
	/*fix augmentation */
	tmpMax =
	    toFix->left->value.maxGradient >
	    toFix->right->value.maxGradient ? toFix->left->value.
	    maxGradient : toFix->right->value.maxGradient;
	if (tmpMax > toFix->value.gradient)
	    toFix->value.maxGradient = tmpMax;
	else
	    toFix->value.maxGradient = toFix->value.gradient;

	while (z->parent != NIL) {
	    if (z->parent->value.maxGradient == zGradient) {
		if (z->parent->value.gradient != zGradient &&
		    (!(z->parent->left->value.maxGradient == zGradient &&
		       z->parent->right->value.maxGradient == zGradient))) {

		    left = find_max_value(z->parent->left);
		    right = find_max_value(z->parent->right);

		    if (left > right)
			z->parent->value.maxGradient = left;
		    else
			z->parent->value.maxGradient = right;

		    if (z->parent->value.gradient >
			z->parent->value.maxGradient)
			z->parent->value.maxGradient =
			    z->parent->value.gradient;

		}

	    }
	    else {
		if (z->value.maxGradient > z->parent->value.maxGradient)
		    z->parent->value.maxGradient = z->value.maxGradient;
	    }
	    z = z->parent;
	}

    }

    /*16-17 */
    if (y->color == RB_BLACK && x != NIL)
	rb_delete_fixup(root, x);

    /*18 */
#ifdef __GRASS__
    G_free(y);
#else
    free(y);
#endif

    return;
}

/*fix the rb tree after deletion */
void rb_delete_fixup(TreeNode ** root, TreeNode * x)
{
    TreeNode *w;

    while (x != *root && x->color == RB_BLACK) {
	if (x == x->parent->left) {
	    w = x->parent->right;
	    if (w->color == RB_RED) {
		w->color = RB_BLACK;
		x->parent->color = RB_RED;
		left_rotate(root, x->parent);
		w = x->parent->right;
	    }

	    if (w == NIL) {
		x = x->parent;
		continue;
	    }

	    if (w->left->color == RB_BLACK && w->right->color == RB_BLACK) {
		w->color = RB_RED;
		x = x->parent;
	    }
	    else {
		if (w->right->color == RB_BLACK) {
		    w->left->color = RB_BLACK;
		    w->color = RB_RED;
		    right_rotate(root, w);
		    w = x->parent->right;
		}

		w->color = x->parent->color;
		x->parent->color = RB_BLACK;
		w->right->color = RB_BLACK;
		left_rotate(root, x->parent);
		x = *root;
	    }

	}
	else {			/*(x==x->parent->right) */
	    w = x->parent->left;
	    if (w->color == RB_RED) {
		w->color = RB_BLACK;
		x->parent->color = RB_RED;
		right_rotate(root, x->parent);
		w = x->parent->left;
	    }

	    if (w == NIL) {
		x = x->parent;
		continue;
	    }

	    if (w->right->color == RB_BLACK && w->left->color == RB_BLACK) {
		w->color = RB_RED;
		x = x->parent;
	    }
	    else {
		if (w->left->color == RB_BLACK) {
		    w->right->color = RB_BLACK;
		    w->color = RB_RED;
		    left_rotate(root, w);
		    w = x->parent->left;
		}

		w->color = x->parent->color;
		x->parent->color = RB_BLACK;
		w->left->color = RB_BLACK;
		right_rotate(root, x->parent);
		x = *root;
	    }

	}
    }
    x->color = RB_BLACK;

    return;
}

/*find the max value in the given tree
   //you need to provide a compare function to compare the nodes */
double find_max_value(TreeNode * root)
{
    if (!root)
	return SMALLEST_GRADIENT;
    assert(root);
    /*assert(root->value.maxGradient != SMALLEST_GRADIENT);
       //LT: this shoudl be fixed
       //if (root->value.maxGradient != SMALLEST_GRADIENT) */
    return root->value.maxGradient;
}



/*find max within the max key */
double find_max_value_within_key(TreeNode * root, double maxKey)
{
    TreeNode *keyNode = search_for_node(root, maxKey);

    if (keyNode == NIL) {
	/*fprintf(stderr, "key node not found. error occured!\n");
	   //there is no point in the structure with key < maxKey */
	return SMALLEST_GRADIENT;
	exit(1);
    }

    double max = find_max_value(keyNode->left);
    double tmpMax;

    while (keyNode->parent != NIL) {
	if (keyNode == keyNode->parent->right) {	/*its the right node of its parent; */
	    tmpMax = find_max_value(keyNode->parent->left);
	    if (tmpMax > max)
		max = tmpMax;
	    if (keyNode->parent->value.gradient > max)
		max = keyNode->parent->value.gradient;
	}
	keyNode = keyNode->parent;
    }

    return max;
}


void left_rotate(TreeNode ** root, TreeNode * x)
{
    TreeNode *y;

    y = x->right;

    /*maintain augmentation */
    double tmpMax;

    /*fix x */
    tmpMax = x->left->value.maxGradient > y->left->value.maxGradient ?
	x->left->value.maxGradient : y->left->value.maxGradient;

    if (tmpMax > x->value.gradient)
	x->value.maxGradient = tmpMax;
    else
	x->value.maxGradient = x->value.gradient;


    /*fix y */
    tmpMax = x->value.maxGradient > y->right->value.maxGradient ?
	x->value.maxGradient : y->right->value.maxGradient;

    if (tmpMax > y->value.gradient)
	y->value.maxGradient = tmpMax;
    else
	y->value.maxGradient = y->value.gradient;

    /*left rotation
       //see pseudocode on page 278 in CLRS */

    x->right = y->left;		/*turn y's left subtree into x's right subtree */
    y->left->parent = x;

    y->parent = x->parent;	/*link x's parent to y */

    if (x->parent == NIL) {
	*root = y;
    }
    else {
	if (x == x->parent->left)
	    x->parent->left = y;
	else
	    x->parent->right = y;
    }

    y->left = x;
    x->parent = y;

    return;
}

void right_rotate(TreeNode ** root, TreeNode * y)
{
    TreeNode *x;

    x = y->left;

    /*maintain augmentation
       //fix y */
    double tmpMax;

    tmpMax = x->right->value.maxGradient > y->right->value.maxGradient ?
	x->right->value.maxGradient : y->right->value.maxGradient;

    if (tmpMax > y->value.gradient)
	y->value.maxGradient = tmpMax;
    else
	y->value.maxGradient = y->value.gradient;

    /*fix x */
    tmpMax = x->left->value.maxGradient > y->value.maxGradient ?
	x->left->value.maxGradient : y->value.maxGradient;

    if (tmpMax > x->value.gradient)
	x->value.maxGradient = tmpMax;
    else
	x->value.maxGradient = x->value.gradient;

    /*ratation */
    y->left = x->right;
    x->right->parent = y;

    x->parent = y->parent;

    if (y->parent == NIL) {
	*root = x;
    }
    else {
	if (y->parent->left == y)
	    y->parent->left = x;
	else
	    y->parent->right = x;
    }

    x->right = y;
    y->parent = x;

    return;
}
