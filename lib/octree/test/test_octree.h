/****************************************************************************
 *
 * MODULE:       test.octree.lib
 *
 * AUTHOR(S):    Corey White
 *
 * PURPOSE:      Unit tests for the octree library
 *
 * COPYRIGHT:    (C) 2026 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#ifndef TEST_OCTREE_H
#define TEST_OCTREE_H

#include "octree.h"

/* Unit test function prototypes */
int unit_test_create_node(void);
int unit_test_insert_point(void);
int unit_test_free_octree(void);
int unit_test_query(void);

#endif /* TEST_OCTREE_H */
