/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 * 		soerengebbert <at> gmx <dot> de
 *               
 * PURPOSE:      Unit tests for les solving
 *
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/rtree.h>
#include <grass/vector.h>
#include "test_rtree_lib.h"

/* prototypes */
static int test_basics_1d(void);
static int test_basics_2d(void);
static int test_basics_3d(void);
static int test_basics_4d(void);

/* ************************************************************************* */
/* Performe the solver unit tests ****************************************** */
/* ************************************************************************* */

int unit_test_basics(void)
{
	int sum = 0;

	G_message(_("\n++ Running basic unit tests ++"));

        sum += test_basics_1d();
        sum += test_basics_2d();
        sum += test_basics_3d();
        sum += test_basics_4d();

	if (sum > 0)
            G_warning(_("\n-- Basic rtree unit tests failure --"));
	else
            G_message(_("\n-- Basic rtree unit tests finished successfully --"));

	return sum;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

int test_basics_1d(void)
{
    int sum = 0, num, i;
    
    struct RTree* tree = RTreeCreateTree(-1, 0, 1);
    
    struct ilist *list = G_new_ilist();
    
    for(i = 0; i < 10; i++) {
        
        struct RTree_Rect* rect1 = RTreeAllocRect(tree);
        RTreeSetRect1D(rect1, tree,(i - 2), (i + 2));
        RTreeInsertRect(rect1, i + 1, tree);
        
        struct RTree_Rect* rect2 = RTreeAllocRect(tree);
        RTreeSetRect1D(rect2, tree, 2.0, 7.0);
        
        num = RTreeSearch2(tree, rect2, list);
        printf("Found %i neighbors\n", num);
        
        if(num != i + 1)
            sum++;
        
        RTreeFreeRect(rect1);
        RTreeFreeRect(rect2);
    }
    RTreeDestroyTree(tree);
    G_free_ilist(list);
            
    return sum;
}


/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

int test_basics_2d(void)
{
    int sum = 0, num, i;
    
    struct RTree* tree = RTreeCreateTree(-1, 0, 2);
    
    struct ilist *list = G_new_ilist();
    
    for(i = 0; i < 10; i++) {
        
        struct RTree_Rect* rect1 = RTreeAllocRect(tree);
        RTreeSetRect2D(rect1, tree,(i - 2), (i + 2), (i - 2), (i + 2));
        RTreeInsertRect(rect1, i + 1, tree);
        
        struct RTree_Rect* rect2 = RTreeAllocRect(tree);
        RTreeSetRect2D(rect2, tree, 2.0, 7.0, 2.0, 7.0);
        
        num = RTreeSearch2(tree, rect2, list);
        printf("Found %i neighbors\n", num);

        if(num != i + 1)
            sum++;
        
        RTreeFreeRect(rect1);
        RTreeFreeRect(rect2);
    }
    RTreeDestroyTree(tree);
    G_free_ilist(list);
    
    return sum;
}


/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

int test_basics_3d(void)
{
    int sum = 0, num, i;
    
    struct RTree* tree = RTreeCreateTree(-1, 0, 3);
    
    struct ilist *list = G_new_ilist();
    
    for(i = 0; i < 10; i++) {
        
        struct RTree_Rect* rect1 = RTreeAllocRect(tree);
        RTreeSetRect3D(rect1, tree,(i - 2), (i + 2), (i - 2), (i + 2),
                       (i - 2), (i + 2));
        RTreeInsertRect(rect1, i + 1, tree);
        
        struct RTree_Rect* rect2 = RTreeAllocRect(tree);
        RTreeSetRect3D(rect2, tree, 2.0, 7.0, 2.0,
                       7.0, 2.0, 7.0);
        
        num = RTreeSearch2(tree, rect2, list);
        printf("Found %i neighbors\n", num);
        
        if(num != i + 1)
            sum++;
        
        RTreeFreeRect(rect1);
        RTreeFreeRect(rect2);
    }
    RTreeDestroyTree(tree);
    G_free_ilist(list);
    
    return sum;
}


/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

int test_basics_4d(void)
{
    int sum = 0, num, i;
    
    struct RTree* tree = RTreeCreateTree(-1, 0, 4);
    
    struct ilist *list = G_new_ilist();
    
    for(i = 0; i < 10; i++) {
        
        struct RTree_Rect* rect1 = RTreeAllocRect(tree);
        RTreeSetRect4D(rect1, tree,(i - 2), (i + 2), (i - 2), (i + 2),
                       (i - 2), (i + 2), (i - 2), (i + 2));
        RTreeInsertRect(rect1, i + 1, tree);
        
        struct RTree_Rect* rect2 = RTreeAllocRect(tree);
        RTreeSetRect4D(rect2, tree, 2.0, 7.0, 2.0,
                       7.0, 2.0, 7.0, 2.0, 7.0);
        
        num = RTreeSearch2(tree, rect2, list);
        printf("Found %i neighbors\n", num);
        
        if(num != i + 1)
            sum++;
        
        RTreeFreeRect(rect1);
        RTreeFreeRect(rect2);
    }
    RTreeDestroyTree(tree);
    G_free_ilist(list);
    
    return sum;
}

