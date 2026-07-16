/***************************************************************************
 *
 * MODULE:     v.out.vtk
 * AUTHOR(S):  Soeren Gebbert
 *
 * PURPOSE:    v.out.vtk: writes ASCII VTK file
 *             this module is based on v.out.ascii
 * SPDX-FileCopyrightText: 2000 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/vector.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int write_vtk_head(FILE *fp, struct Map_info *Map)
{
    G_debug(3, _("writeVTKHeader: Writing VTK-Header"));
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    /* The header line describes the data. */
    fprintf(fp, "GRASS GIS %d vector map: %s\n", GRASS_VERSION_MAJOR,
            Map->name);
    fprintf(fp, "ASCII\n");
    fprintf(fp, "DATASET POLYDATA\n"); /*We are using polydata. If Volume data
                                          is supported, uGrid should be used */
    return (0);
}

