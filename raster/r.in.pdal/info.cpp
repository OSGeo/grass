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

#include "info.h"


void get_extent(struct StringList *infiles, double *min_x, double *max_x,
                double *min_y, double *max_y, double *min_z, double *max_z)
{
    pdal::StageFactory factory;
    bool first = 1;

    *min_x = *max_x = *min_y = *max_y = *min_z = *max_z = 0.0 / 0.0;

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
        pdal::LasHeader las_header = las_reader.header();
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


void print_extent(struct StringList *infiles)
{
    double min_x, max_x, min_y, max_y, min_z, max_z;

    get_extent(infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z);
    fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n",
            max_y, min_y, max_x, min_x, min_z, max_z);
}


void print_lasinfo(struct StringList *infiles)
{
    pdal::StageFactory factory;
    pdal::MetadataNode meta_node;

    std::cout << std::endl << "Using PDAL library version '" <<
        pdal::Config::fullVersionString() << "'" << std::endl << std::endl;

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
        pdal::LasHeader las_header = las_reader.header();
        pdal::PointLayoutPtr point_layout = table.layout();
        const pdal::Dimension::IdList & dims = point_layout->dims();

        std::cout << "File: " << infile << std::endl;
        std::cout << las_header;

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
