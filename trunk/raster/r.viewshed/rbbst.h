
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *               Markus Metz: surface interpolation
 *
 * Date:         april 2011 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The terrain is NOT viewed as a tesselation of flat cells, 
 * i.e. if the line-of-sight does not pass through the cell center, 
 * elevation is determined using bilinear interpolation.
 * The viewshed algorithm is efficient both in
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

#ifndef __RB_BINARY_SEARCH_TREE__
#define __RB_BINARY_SEARCH_TREE__

#define SMALLEST_GRADIENT (- 9999999999999999999999.0)
/*this value is returned by findMaxValueWithinDist() is there is no
   key within that distance.  The largest double value is 1.7 E 308 */



#define RB_RED (0)
#define RB_BLACK (1)

/*<===========================================>
   //public:
   //The value that's stored in the tree
   //Change this structure to avoid type casting at run time */
typedef struct tree_value_
{
    /*this field is mandatory and cannot be removed.
       //the tree is indexed by this "key". */
    double key;

    /*anything else below this line is optional */
    double gradient[3];
    double angle[3];
    double maxGradient;
} TreeValue;


/*The node of a tree */
typedef struct tree_node_
{
    TreeValue value;

    char color;

    struct tree_node_ *left;
    struct tree_node_ *right;
    struct tree_node_ *parent;

} TreeNode;

typedef struct rbtree_
{
    TreeNode *root;
} RBTree;



RBTree *create_tree(TreeValue tv);
void delete_tree(RBTree * t);
void destroy_sub_tree(TreeNode * node);
void insert_into(RBTree * rbt, TreeValue value);
void delete_from(RBTree * rbt, double key);
TreeNode *search_for_node_with_key(RBTree * rbt, double key);


/*------------The following is designed for kreveld's algorithm-------*/
double find_max_gradient_within_key(RBTree * rbt, double key, double angle, double gradient);

/*LT: not sure if this is correct */
int is_empty(RBTree * t);





/*<================================================>
   //private:
   //The below are private functions you should not 
   //call directly when using the Tree

   //<--------------------------------->
   //for RB tree only

   //in RB TREE, used to replace NULL */
void init_nil_node();


/*Left and Right Rotation
   //the root of the tree may be modified during the rotations
   //so TreeNode** is passed into the functions */
void left_rotate(TreeNode ** root, TreeNode * x);
void right_rotate(TreeNode ** root, TreeNode * y);
void rb_insert_fixup(TreeNode ** root, TreeNode * z);
void rb_delete_fixup(TreeNode ** root, TreeNode * x);

/*<------------------------------------> */


/*compare function used by findMaxValue
   //-1: v1 < v2
   //0:  v1 = v2
   //2:  v1 > v2 */
char compare_values(TreeValue * v1, TreeValue * v2);

/*a function used to compare two doubles */
char compare_double(double a, double b);

/*create a tree node */
TreeNode *create_tree_node(TreeValue value);

/*create node with its value set to the value given
   //and insert the node into the tree */
void insert_into_tree(TreeNode ** root, TreeValue value);

/*delete the node out of the tree */
void delete_from_tree(TreeNode ** root, double key);

/*search for a node with the given key */
TreeNode *search_for_node(TreeNode * root, double key);

/*find the min value in the given tree value */
double find_value_min_value(TreeValue * v);

/*find the max value in the given tree
   //you need to provide a compare function to compare the nodes */
double find_max_value(TreeNode * root);

/*find max within the max key */
double find_max_value_within_key(TreeNode * root, double maxKey, double angle, double gradient);

#endif
