/*!
   \file cluster/c_distinct.c

   \brief Cluster library - Distinct value

   (C) 2001-2009 by the GRASS Development Team

    SPDX-License-Identifier: GPL-2.0-or-later
\author Original author CERL
 */

#include <grass/cluster.h>

/*!
   \brief Get distinct value

   \param C pointer to Cluster structure
   \param separation separation value

   \return distinction value
 */
int I_cluster_distinct(struct Cluster *C, double separation)
{
    int class1, class2;
    int distinct;
    double dmin;
    double dsep;

    /* compute sum of squares for each class */
    I_cluster_sum2(C);

    /* find closest classes */
    distinct = 1;
    dmin = separation;
    for (class1 = 0; class1 < (C->nclasses - 1); class1++) {
        if (C->count[class1] < 2)
            continue;
        for (class2 = class1 + 1; class2 < C->nclasses; class2++) {
            if (C->count[class2] < 2)
                continue;
            dsep = I_cluster_separation(C, class1, class2);

            if (dsep >= 0.0 && dsep < dmin) {
                distinct = 0;
                C->merge1 = class1;
                C->merge2 = class2;
                dmin = dsep;
            }
        }
    }

    return distinct;
}
