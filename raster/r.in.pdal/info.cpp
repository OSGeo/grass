/*
 * r.in.pdal Functions printing out various information on input LAS files
 *
 *   Copyright 2021-2024 by Maris Nartiss, and the GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#include "info.h"
#include <cmath>

#ifdef PDAL_USE_NOSRS
void get_extent(struct StringList *infiles, double *min_x, double *max_x,
                double *min_y, double *max_y, double *min_z, double *max_z,
                bool nosrs)
#else
void get_extent(struct StringList *infiles, double *min_x, double *max_x,
                double *min_y, double *max_y, double *min_z, double *max_z)
#endif
{
    pdal::StageFactory factory;
    bool first = 1;

    *min_x = *max_x = *min_y = *max_y = *min_z = *max_z = NAN;

    for (int i = 0; i < infiles->num_items; i++) {
        const char *infile = infiles->items[i];

        std::string pdal_read_driver = factory.inferReaderDriver(infile);
        if (pdal_read_driver.empty())
            G_fatal_error(_("Cannot determine input file type of <%s>"),
                          infile);

        pdal::PointTable table;
        pdal::Options las_opts;
        pdal::Option las_opt("filename", infile);
        las_opts.add(las_opt);
#ifdef PDAL_USE_NOSRS
        if (nosrs) {
            pdal::Option nosrs_opt("nosrs", true);
            las_opts.add(nosrs_opt);
        }
#endif
        pdal::LasReader las_reader;
        las_reader.setOptions(las_opts);
        try {
            las_reader.prepare(table);
        }
        catch (const std::exception &err) {
            G_fatal_error(_("PDAL error: %s"), err.what());
        }
        const pdal::LasHeader &las_header = las_reader.header();
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

#ifdef PDAL_USE_NOSRS
void print_extent(struct StringList *infiles, bool nosrs)
#else
void print_extent(struct StringList *infiles)
#endif
{
    double min_x, max_x, min_y, max_y, min_z, max_z;

#ifdef PDAL_USE_NOSRS
    get_extent(infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z, nosrs);
#else
    get_extent(infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z);
#endif
    fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n", max_y, min_y, max_x,
            min_x, min_z, max_z);
}

#ifdef PDAL_USE_NOSRS
void print_lasinfo(struct StringList *infiles, bool nosrs)
#else
void print_lasinfo(struct StringList *infiles)
#endif
{
    pdal::StageFactory factory;
    pdal::MetadataNode meta_node;

    std::cout << std::endl
              << "Using PDAL library version '"
              << pdal::Config::fullVersionString() << "'" << std::endl
              << std::endl;

    for (int i = 0; i < infiles->num_items; i++) {
        const char *infile = infiles->items[i];

        std::string pdal_read_driver = factory.inferReaderDriver(infile);
        if (pdal_read_driver.empty())
            G_fatal_error(_("Cannot determine input file type of <%s>"),
                          infile);

        pdal::PointTable table;
        pdal::Options las_opts;
        pdal::Option las_opt("filename", infile);
        las_opts.add(las_opt);
#ifdef PDAL_USE_NOSRS
        if (nosrs) {
            pdal::Option nosrs_opt("nosrs", true);
            las_opts.add(nosrs_opt);
        }
#endif
        pdal::LasReader las_reader;
        las_reader.setOptions(las_opts);
        try {
            las_reader.prepare(table);
        }
        catch (const std::exception &err) {
            G_fatal_error(_("PDAL error: %s"), err.what());
        }
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
        for (size_t k = 0; k < pdal::LasHeader::RETURN_COUNT; ++k)
            std::cout << "Point count by return[" << k + 1 << "]: "
                      << const_cast<pdal::LasHeader &>(h).pointCountByReturn(k)
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
