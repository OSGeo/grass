/*  cmcpy.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
void cmcpy(Cpx * a, Cpx * b, int n)
{
    int i;

    for (i = 0; i < n; ++i)
	*a++ = *b++;
}
