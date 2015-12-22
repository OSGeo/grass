/*
 * v.in.lidar projection-related functions
 *
 * Copyright 2011-2015 by Markus Metz, and The GRASS Development Team
 * Authors:
 *  Markus Metz (v.in.lidar)
 *  Vaclav Petras (move functions to a file)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>

#include "info.h"

void print_lasinfo(LASHeaderH LAS_header, LASSRSH LAS_srs)
{
    char *las_srs_proj4 = LASSRS_GetProj4(LAS_srs);
    int las_point_format = LASHeader_GetDataFormatId(LAS_header);

    fprintf(stdout, "\nUsing LAS Library Version '%s'\n\n",
            LAS_GetFullVersion());
    fprintf(stdout, "LAS File Version:                  %d.%d\n",
            LASHeader_GetVersionMajor(LAS_header),
            LASHeader_GetVersionMinor(LAS_header));
    fprintf(stdout, "System ID:                         '%s'\n",
            LASHeader_GetSystemId(LAS_header));
    fprintf(stdout, "Generating Software:               '%s'\n",
            LASHeader_GetSoftwareId(LAS_header));
    fprintf(stdout, "File Creation Day/Year:            %d/%d\n",
            LASHeader_GetCreationDOY(LAS_header),
            LASHeader_GetCreationYear(LAS_header));
    fprintf(stdout, "Point Data Format:                 %d\n",
            las_point_format);
    fprintf(stdout, "Number of Point Records:           %d\n",
            LASHeader_GetPointRecordsCount(LAS_header));
    fprintf(stdout, "Number of Points by Return:        %d %d %d %d %d\n",
            LASHeader_GetPointRecordsByReturnCount(LAS_header, 0),
            LASHeader_GetPointRecordsByReturnCount(LAS_header, 1),
            LASHeader_GetPointRecordsByReturnCount(LAS_header, 2),
            LASHeader_GetPointRecordsByReturnCount(LAS_header, 3),
            LASHeader_GetPointRecordsByReturnCount(LAS_header, 4));
    fprintf(stdout, "Scale Factor X Y Z:                %g %g %g\n",
            LASHeader_GetScaleX(LAS_header),
            LASHeader_GetScaleY(LAS_header), LASHeader_GetScaleZ(LAS_header));
    fprintf(stdout, "Offset X Y Z:                      %g %g %g\n",
            LASHeader_GetOffsetX(LAS_header),
            LASHeader_GetOffsetY(LAS_header),
            LASHeader_GetOffsetZ(LAS_header));
    fprintf(stdout, "Min X Y Z:                         %g %g %g\n",
            LASHeader_GetMinX(LAS_header),
            LASHeader_GetMinY(LAS_header), LASHeader_GetMinZ(LAS_header));
    fprintf(stdout, "Max X Y Z:                         %g %g %g\n",
            LASHeader_GetMaxX(LAS_header),
            LASHeader_GetMaxY(LAS_header), LASHeader_GetMaxZ(LAS_header));
    if (las_srs_proj4 && strlen(las_srs_proj4) > 0) {
        fprintf(stdout, "Spatial Reference:\n");
        fprintf(stdout, "%s\n", las_srs_proj4);
    }
    else {
        fprintf(stdout, "Spatial Reference:                 None\n");
    }

    fprintf(stdout, "\nData Fields:\n");
    fprintf(stdout,
            "  'X'\n  'Y'\n  'Z'\n  'Intensity'\n  'Return Number'\n");
    fprintf(stdout, "  'Number of Returns'\n  'Scan Direction'\n");
    fprintf(stdout,
            "  'Flighline Edge'\n  'Classification'\n  'Scan Angle Rank'\n");
    fprintf(stdout, "  'User Data'\n  'Point Source ID'\n");
    if (las_point_format == 1 || las_point_format == 3 ||
        las_point_format == 4 || las_point_format == 5) {
        fprintf(stdout, "  'GPS Time'\n");
    }
    if (las_point_format == 2 || las_point_format == 3 ||
        las_point_format == 5) {
        fprintf(stdout, "  'Red'\n  'Green'\n  'Blue'\n");
    }
    fprintf(stdout, "\n");
    fflush(stdout);

    return;
}
