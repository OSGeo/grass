/***************************************************************************
 *
 * MODULE:    r.in.pdal
 *
 * AUTHOR(S): Vaclav Petras
 *
 * PURPOSE:   Imports LAS LiDAR point clouds to a raster map using
 *            aggregate statistics.
 *
 * COPYRIGHT: (C) 2019 by Vaclav Petras and the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/

#ifndef GRASSRASTERWRITER_H
#define GRASSRASTERWRITER_H

extern "C" {
#include "lidar.h"
#include "point_binning.h"
}

extern "C" {
#include <grass/gis.h>
#include <grass/raster.h>
}

#include <pdal/Streamable.hpp>
#include <pdal/Writer.hpp>

/* Binning code wrapped as a PDAL Writer class */
#if PDAL_VERSION_MAJOR >= 2 && PDAL_VERSION_MINOR >= 7
class GrassRasterWriter : public pdal::NoFilenameWriter,
                          public pdal::Streamable {
#else
class GrassRasterWriter : public pdal::Writer, public pdal::Streamable {
#endif
public:
    GrassRasterWriter() : n_processed(0) {}

    std::string getName() const { return "writers.grassbinning"; }

    void set_binning(struct Cell_head *region,
                     struct PointBinning *point_binning,
                     struct BinIndex *bin_index_nodes, RASTER_MAP_TYPE rtype,
                     int cols)
    {
        region_ = region;
        point_binning_ = point_binning;
        bin_index_nodes_ = bin_index_nodes;
        rtype_ = rtype;
        cols_ = cols;
    }

    void dim_to_import(pdal::Dimension::Id dim_to_import)
    {
        dim_to_import_ = dim_to_import;
    }

    void set_output_scale(double scale) { scale_ = scale; }

    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
    {
        base_segment_ = base_segment;
        input_region_ = region;
        base_raster_data_type_ = rtype;
    }

    virtual void write(const pdal::PointViewPtr view)
    {
        pdal::PointRef p(*view, 0);
        for (pdal::PointId idx = 0; idx < view->size(); ++idx) {
            p.setPointId(idx);
            processOne(p);
        }
    }

    virtual bool processOne(pdal::PointRef &point)
    {
        using namespace pdal::Dimension;

        double x = point.getFieldAs<double>(Id::X);
        double y = point.getFieldAs<double>(Id::Y);
        double z = point.getFieldAs<double>(dim_to_import_);

        z *= scale_;
        if (base_segment_) {
            double base_z;

            rast_segment_get_value_xy(base_segment_, input_region_,
                                      base_raster_data_type_, x, y, &base_z);
            z -= base_z;
        }

        // TODO: check the bounds and report discrepancies in
        // number of filtered out vs processed to the user
        // (alternatively, change the spatial bounds test to
        // give same results as this, but it might be actually helpful
        // to tell user that they have points right on the border)
        int arr_row = (int)((region_->north - y) / region_->ns_res);
        int arr_col = (int)((x - region_->west) / region_->ew_res);

        if (arr_row >= region_->rows || arr_col >= region_->cols) {
            G_message(_("A point on the edge of computational region detected. "
                        "Ignoring."));
            return false;
        }

        update_value(point_binning_, bin_index_nodes_, cols_, arr_row, arr_col,
                     rtype_, x, y, z);
        n_processed++;
        return true;
    }

    gpoint_count n_processed;

private:
    struct Cell_head *region_;
    struct PointBinning *point_binning_;
    struct BinIndex *bin_index_nodes_;
    RASTER_MAP_TYPE rtype_;
    int cols_;
    double scale_;

    pdal::Dimension::Id dim_to_import_;

    SEGMENT *base_segment_;
    struct Cell_head *input_region_;
    RASTER_MAP_TYPE base_raster_data_type_;
};

#endif // GRASSRASTERWRITER_H
