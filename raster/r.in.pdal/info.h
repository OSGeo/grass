/*
 * r.in.pdal Functions printing out various information on input LAS files
 *
 *   Copyright 2021 by Maris Nartiss, and the GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef INFO_H
#define INFO_H

#include <pdal/pdal_config.hpp>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif
#include <pdal/PointTable.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Options.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/io/LasHeader.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <pdal/pdal_features.hpp>
#if (PDAL_VERSION_MAJOR >= 2 && PDAL_VERSION_MINOR > 4) || \
    (PDAL_VERSION_MAJOR == 2 && PDAL_VERSION_MINOR == 4 && \
     PDAL_VERSION_PATCH == 3)
#define PDAL_USE_NOSRS 1
#endif

extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
#include "string_list.h"
}

#ifdef PDAL_USE_NOSRS
void get_extent(struct StringList *, double *, double *, double *, double *,
                double *, double *, bool);
void print_extent(struct StringList *, bool);
void print_lasinfo(struct StringList *, bool);
#else
void get_extent(struct StringList *, double *, double *, double *, double *,
                double *, double *);
void print_extent(struct StringList *);
void print_lasinfo(struct StringList *);
#endif

#endif // INFO_H
