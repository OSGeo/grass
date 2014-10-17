/*
 * \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

#ifndef AVL_H
#define AVL_H

#include "GenericCell.h"

typedef struct avl_node
{
    generic_cell key;		/* key field is a CELL or a DCELL or a FCELL */
    long counter;		/* data */
    struct avl_node *father;
    struct avl_node *right_child;
    struct avl_node *left_child;
} avl_node;

typedef avl_node *avl_tree;

/*table */
typedef struct AVL_tableRow
{
    generic_cell k;
    long tot;
} AVL_tableRow;

typedef AVL_tableRow *AVL_table;

/* prototype of functions */
avl_tree avl_make(const generic_cell k, const long n);
void avl_destroy(avl_tree root);
avl_node *avl_find(const avl_tree root, const generic_cell k);
int avl_add(avl_tree * root, const generic_cell k, const long n);
long avl_to_array(avl_node * root, long i, AVL_table a);
long howManyCell(const avl_tree root, const generic_cell k);


#endif
