/*
 * r.in.pdal Functions printing out various information on input LAS files
 *  
 *   Copyright 2020 by Maris Nartiss, and The GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef INFO_H
#define INFO_H

#include <pdal/PointTable.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Options.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/io/LasHeader.hpp>

extern "C"
{
#include <grass/gis.h>
#include <grass/glocale.h>
#include "string_list.h"
}

void get_extent(struct StringList *, double *, double *,
                double *, double *, double *, double *);
void print_extent(struct StringList *);

#endif // INFO_H
