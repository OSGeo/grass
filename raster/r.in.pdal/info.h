/*
 * r.in.pdal Functions printing out various information on input LAS files
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
 *
=======
 *  
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
 *
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
 *
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
 *   Copyright 2021 by Maris Nartiss, and The GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef INFO_H
#define INFO_H

#include <pdal/pdal_config.hpp>
<<<<<<< HEAD
<<<<<<< HEAD
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif
=======
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
#include <pdal/PointTable.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Options.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/io/LasHeader.hpp>
<<<<<<< HEAD
<<<<<<< HEAD
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

extern "C" {
=======

<<<<<<< HEAD
extern "C"
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
extern "C" {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

extern "C" {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
#include <grass/gis.h>
#include <grass/glocale.h>
#include "string_list.h"
}

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
void get_extent(struct StringList *, double *, double *, double *, double *,
                double *, double *);
=======
void get_extent(struct StringList *, double *, double *,
                double *, double *, double *, double *);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
void get_extent(struct StringList *, double *, double *, double *, double *,
                double *, double *);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
void get_extent(struct StringList *, double *, double *, double *, double *,
                double *, double *);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
void print_extent(struct StringList *);
void print_lasinfo(struct StringList *);

#endif // INFO_H
