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
#include <cmath>
#include <stdbool.h>

#include <pdal/filters/ReprojectionFilter.hpp>
#include <pdal/io/BufferReader.hpp>

#include "info.h"

extern "C" {
#include "projection.h"
}

#ifdef R_IN_PDAL_USE_NOSRS
void get_extent(struct StringList *infiles, double *min_x, double *max_x,
                double *min_y, double *max_y, double *min_z, double *max_z,
                bool nosrs)
#else
void get_extent(struct StringList *infiles, double *min_x, double *max_x,
                double *min_y, double *max_y, double *min_z, double *max_z)
#endif
{
    pdal::StageFactory factory;
    pdal::SpatialReference spatial_reference;
    bool first = true;
    double min_x_, max_x_, min_y_, max_y_, min_z_, max_z_;
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
#ifdef R_IN_PDAL_USE_NOSRS
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
        spatial_reference = las_reader.getSpatialReference();
        std::string dataset_wkt = spatial_reference.getWKT();
        bool proj_match = is_wkt_projection_same_as_loc(dataset_wkt.c_str());

        const pdal::LasHeader &las_header = las_reader.header();

        min_x_ = las_header.minX();
        min_y_ = las_header.minY();
        min_z_ = las_header.minZ();
        max_x_ = las_header.maxX();
        max_y_ = las_header.maxY();
        max_z_ = las_header.maxZ();
#ifdef R_IN_PDAL_USE_NOSRS
        bool need_to_reproject = !nosrs && !proj_match &&
                                 spatial_reference.valid() &&
                                 !spatial_reference.empty();
#else
        bool need_to_reproject = !proj_match && spatial_reference.valid() &&
                                 !spatial_reference.empty();
#endif

        if (need_to_reproject) {
            get_reprojected_extent(spatial_reference, &min_x_, &max_x_, &min_y_,
                                   &max_y_, &min_z_, &max_z_);
        }
        if (first) {
            *min_x = min_x_;
            *min_y = min_y_;
            *min_z = min_z_;
            *max_x = max_x_;
            *max_y = max_y_;
            *max_z = max_z_;

            first = false;
        }
        else {
            if (*min_x > min_x_)
                *min_x = min_x_;
            if (*min_y > min_y_)
                *min_y = min_y_;
            if (*min_z > min_z_)
                *min_z = min_z_;
            if (*max_x < max_x_)
                *max_x = max_x_;
            if (*max_y < max_y_)
                *max_y = max_y_;
            if (*max_z < max_z_)
                *max_z = max_z_;
        }
    }
}
/* Reproject 4 points representing bounding box using PDAL pipeline */
void get_reprojected_extent(pdal::SpatialReference &spatial_reference,
                            double *min_x, double *max_x, double *min_y,
                            double *max_y, double *min_z, double *max_z)
{
    pdal::PointTable table;
    table.layout()->registerDim(pdal::Dimension::Id::X);
    table.layout()->registerDim(pdal::Dimension::Id::Y);
    table.layout()->registerDim(pdal::Dimension::Id::Z);

    pdal::PointViewPtr view(new pdal::PointView(table));

    view->setField(pdal::Dimension::Id::X, 0, *min_x);
    view->setField(pdal::Dimension::Id::Y, 0, *min_y);
    view->setField(pdal::Dimension::Id::Z, 0, *min_z);
    view->setField(pdal::Dimension::Id::X, 1, *min_x);
    view->setField(pdal::Dimension::Id::Y, 1, *max_y);
    view->setField(pdal::Dimension::Id::Z, 1, *min_z);
    view->setField(pdal::Dimension::Id::X, 2, *max_x);
    view->setField(pdal::Dimension::Id::Y, 2, *min_y);
    view->setField(pdal::Dimension::Id::Z, 2, *max_z);
    view->setField(pdal::Dimension::Id::X, 3, *max_x);
    view->setField(pdal::Dimension::Id::Y, 3, *max_y);
    view->setField(pdal::Dimension::Id::Z, 3, *max_z);

    pdal::BufferReader reader;
    reader.addView(view);

    pdal::StageFactory factory;
    pdal::Stage *reproject = factory.createStage("filters.reprojection");
    pdal::Options reproject_options;
    reproject_options.add("in_srs", spatial_reference.getWKT());
    reproject_options.add("out_srs", location_projection_as_wkt(false));
    reproject->setOptions(reproject_options);
    reproject->setInput(reader);
    reproject->prepare(table);
    reproject->execute(table);

    for (pdal::PointId i = 0; i < view->size(); ++i) {

        double x = view->getFieldAs<double>(pdal::Dimension::Id::X, i);
        double y = view->getFieldAs<double>(pdal::Dimension::Id::Y, i);
        double z = view->getFieldAs<double>(pdal::Dimension::Id::Z, i);
        if (i == 0) {
            *min_x = *max_x = x;
            *min_y = *max_y = y;
            *min_z = *max_z = z;
        }
        else {
            if (*min_x > x)
                *min_x = x;
            if (*min_y > y)
                *min_y = y;
            if (*min_z > z)
                *min_z = z;
            if (*max_x < x)
                *max_x = x;
            if (*max_y < y)
                *max_y = y;
            if (*max_z < z)
                *max_z = z;
        }
    }
}

#ifdef R_IN_PDAL_USE_NOSRS
void print_extent(struct StringList *infiles, bool nosrs)
#else
void print_extent(struct StringList *infiles)
#endif
{
    double min_x, max_x, min_y, max_y, min_z, max_z;

#ifdef R_IN_PDAL_USE_NOSRS
    get_extent(infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z, nosrs);
#else
    get_extent(infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z);
#endif
    fprintf(stdout, "n=%f s=%f e=%f w=%f b=%f t=%f\n", max_y, min_y, max_x,
            min_x, min_z, max_z);
}

#ifdef R_IN_PDAL_USE_NOSRS
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
#ifdef R_IN_PDAL_USE_NOSRS
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
