/*
 * r.in.lidar projection-related functions
 *
 * Copyright 2011-2015 by Markus Metz, and The GRASS Development Team
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move functions to a file)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#include <string.h>

#include <grass/glocale.h>

#include <liblas/capi/liblas.h>

#include "local_proto.h"


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


int scan_bounds(LASReaderH LAS_reader, int shell_style, int extents, int update,
                double zscale, struct Cell_head *region)
{
    unsigned long line;
    int first;
    double min_x, max_x, min_y, max_y, min_z, max_z;
    double x, y, z;
    LASPointH LAS_point;

    line = 0;
    first = TRUE;

    /* init to nan in case no points are found */
    min_x = max_x = min_y = max_y = min_z = max_z = 0.0 / 0.0;

    G_verbose_message(_("Scanning data ..."));

    LASReader_Seek(LAS_reader, 0);

    while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
        line++;

        /* we don't do any filtering here */

        x = LASPoint_GetX(LAS_point);
        y = LASPoint_GetY(LAS_point);
        z = LASPoint_GetZ(LAS_point);

        if (first) {
            min_x = x;
            max_x = x;
            min_y = y;
            max_y = y;
            min_z = z;
            max_z = z;
            first = FALSE;
        }
        else {
            if (x < min_x)
                min_x = x;
            if (x > max_x)
                max_x = x;
            if (y < min_y)
                min_y = y;
            if (y > max_y)
                max_y = y;
            if (z < min_z)
                min_z = z;
            if (z > max_z)
                max_z = z;
        }
    }

    if (!extents) {
        if (!shell_style) {
            fprintf(stderr, _("Range:     min         max\n"));
            fprintf(stdout, "x: %11f %11f\n", min_x, max_x);
            fprintf(stdout, "y: %11f %11f\n", min_y, max_y);
            fprintf(stdout, "z: %11f %11f\n", min_z * zscale, max_z * zscale);
        }
        else
            fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n",
                    max_y, min_y, max_x, min_x, min_z * zscale,
                    max_z * zscale);

        G_debug(1, "Processed %lu points.", line);
        G_debug(1, "region template: g.region n=%f s=%f e=%f w=%f",
                max_y, min_y, max_x, min_x);
    }
    else if (update) {
        if (min_x < region->west)
            region->west = min_x;
        if (max_x > region->east)
            region->east = max_x;
        if (min_y < region->south)
            region->south = min_y;
        if (max_y > region->north)
            region->north = max_y;
    }
    else {
        region->east = max_x;
        region->west = min_x;
        region->north = max_y;
        region->south = min_y;
    }

    return 0;
}
