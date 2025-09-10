/****************************************************************************
 *
 * MODULE:       kdtree test
 * AUTHOR(S):    林永 ynkan
 * PURPOSE:      test the kdtree.c method for logical operations such as
 *               kdtree_create=>kdtree_insert=>kdtree_dnn=>kdtree_remove=>kdtree_destroy
 *               loop executionbalanced tree
 *               See https://github.com/OSGeo/grass/issues/4779
 * COPYRIGHT:    (C) 2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "kdtree.h"

#define MAX_POINTS 5

void generate_random_points(double points[][2], int num_points)
{
    for (int i = 0; i < num_points; i++) {
        points[i][0] = (rand() % 2000) / 100.0;
        points[i][1] = (rand() % 2000) / 100.0;
    }
}

int main(void)
{

    srand(time(NULL));

    while (1) {
        int num = MAX_POINTS;
        double points[MAX_POINTS][2];
        generate_random_points(points, num);

        /* double target[2] = {(rand() % 2000) / 100.0, (rand() % 2000) /
         * 100.0};*/

        struct kdtree *kdt = kdtree_create(2, NULL);

        for (int i = 0; i < num; i++) {
            int result = kdtree_insert(kdt, points[i], i, 0);
            printf("kdtree insert[uid:%d](%.2f, %.2f) :[%d][%s]\r\n", i,
                   points[i][0], points[i][1], result,
                   result ? "success" : "failure");
        }

        for (int i = 0; i < num; i++) {
            int result = kdtree_remove(kdt, points[i], i);
            printf("kdtree remove[uid:%d](%.2f, %.2f) :[%d][%s]\r\n", i,
                   points[i][0], points[i][1], result,
                   result ? "success" : "failure");
        }

        kdtree_destroy(kdt);
    }

    return 0;
}
