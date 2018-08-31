/*
 * \AUTHOR: Serena Pallecchi student of Computer Science University of Pisa (Italy)
 *                      Commission from Faunalia Pontedera (PI) www.faunalia.it
 *
 *   This program is free software under the GPL (>=v2)
 *   Read the COPYING file that comes with GRASS for details.
 *       
 */

typedef struct avlID_node
{
    long id;			/*key field */
    long counter;		/*data */
    struct avlID_node *father;
    struct avlID_node *right_child;
    struct avlID_node *left_child;
} avlID_node;

typedef avlID_node *avlID_tree;

/*table */
typedef struct avlID_tableRow
{
    long k;
    long tot;
} avlID_tableRow;

typedef avlID_tableRow *avlID_table;

/* prototype of functions */
avlID_tree avlID_make(const long k, const long n);
void avlID_destroy(avlID_tree root);
avlID_node *avlID_find(const avlID_tree root, const long k);
int avlID_add(avlID_tree * root, const long k, const long n);
long avlID_to_array(avlID_node * root, long i, avlID_table * a);
long howManyID(const avlID_tree root, const long k);
long avlID_sub(avlID_tree * root, const long k);
