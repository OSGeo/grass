/*****************************************************************************
 *
 * MODULE:       GRASS gjson Library
 *
 * AUTHOR:       Nishant Bansal (nishant.bansal.282003@gmail.com)
 *
 * PURPOSE:      Unit tests
 *
 * COPYRIGHT:    (C) 2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#ifndef _TEST_GJSON_LIB_H_
#define _TEST_GJSON_LIB_H_

#include <grass/gjson.h>

#define TEST_OBJECT_KEY     "key"
#define TEST_OBJECT_VALUE   "value"
#define TEST_ARRAY_STRING   "array"
#define TEST_OBJECT_DOT_KEY "key.dot"
#define TEST_NUMBER         123.45
#define TEST_BOOLEAN        1

/* parson wrapper tests */
int unit_test_parson_wrapper(void);

#endif
