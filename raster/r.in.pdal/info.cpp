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

#include "info.h"
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
#include <cmath>
=======
=======
#include <cmath>
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

<<<<<<< HEAD
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
void get_extent(struct StringList *infiles, double *min_x, double *max_x,
                double *min_y, double *max_y, double *min_z, double *max_z)
{
    pdal::StageFactory factory;
    bool first = 1;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    *min_x = *max_x = *min_y = *max_y = *min_z = *max_z = NAN;
=======
    *min_x = *max_x = *min_y = *max_y = *min_z = *max_z = 0.0 / 0.0;
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    *min_x = *max_x = *min_y = *max_y = *min_z = *max_z = 0.0 / 0.0;
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    *min_x = *max_x = *min_y = *max_y = *min_z = *max_z = NAN;
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

    for (int i = 0; i < infiles->num_items; i++) {
        const char *infile = infiles->items[i];

        std::string pdal_read_driver = factory.inferReaderDriver(infile);
        if (pdal_read_driver.empty())
            G_fatal_error("Cannot determine input file type of <%s>", infile);

        pdal::PointTable table;
        pdal::Options las_opts;
        pdal::Option las_opt("filename", infile);
        las_opts.add(las_opt);
        pdal::LasReader las_reader;
        las_reader.setOptions(las_opts);
        las_reader.prepare(table);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        const pdal::LasHeader &las_header = las_reader.header();
=======
        pdal::LasHeader las_header = las_reader.header();
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        const pdal::LasHeader &las_header = las_reader.header();
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        const pdal::LasHeader &las_header = las_reader.header();
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        if (first) {
            *min_x = las_header.minX();
            *min_y = las_header.minY();
            *min_z = las_header.minZ();
            *max_x = las_header.maxX();
            *max_y = las_header.maxY();
            *max_z = las_header.maxZ();

            first = 0;
        }
        else {
            if (*min_x > las_header.minX())
                *min_x = las_header.minX();
            if (*min_y > las_header.minY())
                *min_y = las_header.minY();
            if (*min_z > las_header.minZ())
                *min_z = las_header.minZ();
            if (*max_x < las_header.maxX())
                *max_x = las_header.maxX();
            if (*max_y < las_header.maxY())
                *max_y = las_header.maxY();
            if (*max_z < las_header.maxZ())
                *max_z = las_header.maxZ();
        }
    }
}

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
void print_extent(struct StringList *infiles)
{
    double min_x, max_x, min_y, max_y, min_z, max_z;

    get_extent(infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n", max_y, min_y, max_x,
            min_x, min_z, max_z);
}

<<<<<<< HEAD
=======
    fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n",
            max_y, min_y, max_x, min_x, min_z, max_z);
}


>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n", max_y, min_y, max_x,
            min_x, min_z, max_z);
}

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
void print_lasinfo(struct StringList *infiles)
{
    pdal::StageFactory factory;
    pdal::MetadataNode meta_node;

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    std::cout << std::endl
              << "Using PDAL library version '"
              << pdal::Config::fullVersionString() << "'" << std::endl
              << std::endl;
<<<<<<< HEAD
<<<<<<< HEAD
=======
    std::cout << std::endl << "Using PDAL library version '" <<
        pdal::Config::fullVersionString() << "'" << std::endl << std::endl;
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    for (int i = 0; i < infiles->num_items; i++) {
        const char *infile = infiles->items[i];

        std::string pdal_read_driver = factory.inferReaderDriver(infile);
        if (pdal_read_driver.empty())
            G_fatal_error("Cannot determine input file type of <%s>", infile);

        pdal::PointTable table;
        pdal::Options las_opts;
        pdal::Option las_opt("filename", infile);
        las_opts.add(las_opt);
        pdal::LasReader las_reader;
        las_reader.setOptions(las_opts);
        las_reader.prepare(table);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        const pdal::LasHeader &h = las_reader.header();
        pdal::PointLayoutPtr point_layout = table.layout();
        const pdal::Dimension::IdList &dims = point_layout->dims();

        std::cout << "File: " << infile << std::endl;
        std::cout << "File version = "
                  << "1." << (int)h.versionMinor() << "\n";
        std::cout << "File signature: " << h.fileSignature() << "\n";
        std::cout << "File source ID: " << h.fileSourceId() << "\n";
        std::cout << "Global encoding: " << h.globalEncoding() << "\n";
        std::cout << "Project UUID: " << h.projectId() << "\n";
        std::cout << "System ID: " << h.getSystemIdentifier() << "\n";
        std::cout << "Software ID: " << h.softwareId() << "\n";
        std::cout << "Creation DOY: " << h.creationDOY() << "\n";
        std::cout << "Creation Year: " << h.creationYear() << "\n";
        std::cout << "VLR offset (header size): " << h.vlrOffset() << "\n";
        std::cout << "VLR Count: " << h.vlrCount() << "\n";
        std::cout << "Point format: " << (int)h.pointFormat() << "\n";
        std::cout << "Point offset: " << h.pointOffset() << "\n";
        std::cout << "Point count: " << h.pointCount() << "\n";
        for (size_t i = 0; i < pdal::LasHeader::RETURN_COUNT; ++i)
            std::cout << "Point count by return[" << i << "]: "
                      << const_cast<pdal::LasHeader &>(h).pointCountByReturn(i)
                      << "\n";
        std::cout << "Scales X/Y/Z: " << h.scaleX() << "/" << h.scaleY() << "/"
                  << h.scaleZ() << "\n";
        std::cout << "Offsets X/Y/Z: " << h.offsetX() << "/" << h.offsetY()
                  << "/" << h.offsetZ() << "\n";
        std::cout << "Max X/Y/Z: " << h.maxX() << "/" << h.maxY() << "/"
                  << h.maxZ() << "\n";
        std::cout << "Min X/Y/Z: " << h.minX() << "/" << h.minY() << "/"
                  << h.minZ() << "\n";
        if (h.versionAtLeast(1, 4)) {
            std::cout << "Ext. VLR offset: " << h.eVlrOffset() << "\n";
            std::cout << "Ext. VLR count: " << h.eVlrCount() << "\n";
        }
        std::cout << "Compressed: " << (h.compressed() ? "true" : "false")
                  << "\n";
<<<<<<< HEAD
=======
        pdal::LasHeader las_header = las_reader.header();
=======
        const pdal::LasHeader &h = las_reader.header();
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
        pdal::PointLayoutPtr point_layout = table.layout();
        const pdal::Dimension::IdList &dims = point_layout->dims();

        std::cout << "File: " << infile << std::endl;
<<<<<<< HEAD
        std::cout << las_header;
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        std::cout << "File version = "
                  << "1." << (int)h.versionMinor() << "\n";
        std::cout << "File signature: " << h.fileSignature() << "\n";
        std::cout << "File source ID: " << h.fileSourceId() << "\n";
        std::cout << "Global encoding: " << h.globalEncoding() << "\n";
        std::cout << "Project UUID: " << h.projectId() << "\n";
        std::cout << "System ID: " << h.getSystemIdentifier() << "\n";
        std::cout << "Software ID: " << h.softwareId() << "\n";
        std::cout << "Creation DOY: " << h.creationDOY() << "\n";
        std::cout << "Creation Year: " << h.creationYear() << "\n";
        std::cout << "VLR offset (header size): " << h.vlrOffset() << "\n";
        std::cout << "VLR Count: " << h.vlrCount() << "\n";
        std::cout << "Point format: " << (int)h.pointFormat() << "\n";
        std::cout << "Point offset: " << h.pointOffset() << "\n";
        std::cout << "Point count: " << h.pointCount() << "\n";
        for (size_t i = 0; i < pdal::LasHeader::RETURN_COUNT; ++i)
            std::cout << "Point count by return[" << i << "]: "
                      << const_cast<pdal::LasHeader &>(h).pointCountByReturn(i)
                      << "\n";
        std::cout << "Scales X/Y/Z: " << h.scaleX() << "/" << h.scaleY() << "/"
                  << h.scaleZ() << "\n";
        std::cout << "Offsets X/Y/Z: " << h.offsetX() << "/" << h.offsetY()
                  << "/" << h.offsetZ() << "\n";
        std::cout << "Max X/Y/Z: " << h.maxX() << "/" << h.maxY() << "/"
                  << h.maxZ() << "\n";
        std::cout << "Min X/Y/Z: " << h.minX() << "/" << h.minY() << "/"
                  << h.minZ() << "\n";
        if (h.versionAtLeast(1, 4)) {
            std::cout << "Ext. VLR offset: " << h.eVlrOffset() << "\n";
            std::cout << "Ext. VLR count: " << h.eVlrCount() << "\n";
        }
        std::cout << "Compressed: " << (h.compressed() ? "true" : "false")
                  << "\n";
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        bool first = 1;

        for (auto di = dims.begin(); di != dims.end(); ++di) {
            pdal::Dimension::Id d = *di;

            if (first) {
                std::cout << "Dimensions: " << point_layout->dimName(d);
                first = 0;
            }
            else {
                std::cout << ", " << point_layout->dimName(d);
            }
        }
        std::cout << std::endl << std::endl;
    }
}
