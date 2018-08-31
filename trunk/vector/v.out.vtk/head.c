
 /***************************************************************************
 *
 * MODULE:     v.out.vtk  
 * AUTHOR(S):  Soeren Gebbert
 *
 * PURPOSE:    v.out.vtk: writes ASCII VTK file
 *             this module is based on v.out.ascii
 * COPYRIGHT:  (C) 2000 by the GRASS Development Team
 *
 *             This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/vector.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int write_vtk_head(FILE * fp, struct Map_info *Map)
{
    G_debug(3, _("writeVTKHeader: Writing VTK-Header"));
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    fprintf(fp, "GRASS GIS 7 vector map: %s\n", Map->name);
    fprintf(fp, "ASCII\n");
    fprintf(fp, "DATASET POLYDATA\n");	/*We are using polydata. If Volume data is supported, uGrid should be used */
    return (0);
}
