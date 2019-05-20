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

extern "C"
{
#include "lidar.h"
#include "point_binning.h"
}

extern "C"
{
#include <grass/gis.h>
#include <grass/raster.h>
}

#include <pdal/Streamable.hpp>
#include <pdal/Writer.hpp>


/* Binning code wrapped as a PDAL Writer class */
class GrassRasterWriter : public pdal::Writer, public pdal::Streamable
{
public:
    GrassRasterWriter() : n_processed(0)
    {}

    std::string getName() const
    {
        return "writers.grassbinning";
    }

    void set_binning(struct Cell_head *region,
                     struct PointBinning *point_binning,
                     struct BinIndex *bin_index_nodes,
                     RASTER_MAP_TYPE rtype,
                     int cols)
    {
        region_ = region;
        point_binning_ = point_binning;
        bin_index_nodes_ = bin_index_nodes;
        rtype_ = rtype;
        cols_ = cols;
    }
    void dim_to_use_as_z(pdal::Dimension::Id dim_to_use_as_z)
    {
        dim_to_use_as_z_ = dim_to_use_as_z;
    }

    virtual void write(const pdal::PointViewPtr view)
    {
        pdal::PointRef p(*view, 0);
        for (pdal::PointId idx = 0; idx < view->size(); ++idx)
        {
            p.setPointId(idx);
            processOne(p);
        }
    }
    virtual bool processOne(pdal::PointRef& point)
    {
        using namespace pdal::Dimension;

        double x = point.getFieldAs<double>(Id::X);
        double y = point.getFieldAs<double>(Id::Y);
        double z = point.getFieldAs<double>(dim_to_use_as_z_);

        // TODO: check the bounds and report discrepancies in
        // number of filtered out vs processed to the user
        // (alternativelly, change the spatial bounds test to
        // give same results as this, but it might be actually helpful
        // to tell user that they have points right on the border)
        int arr_row = (int)((region_->north - y) / region_->ns_res);
        int arr_col = (int)((x - region_->west) / region_->ew_res);
        update_value(point_binning_, bin_index_nodes_, cols_,
                     arr_row, arr_col, rtype_, x, y, z);
        n_processed++;
    }
    gpoint_count n_processed;
private:
    struct Cell_head *region_;
    struct PointBinning *point_binning_;
    struct BinIndex *bin_index_nodes_;
    RASTER_MAP_TYPE rtype_;
    int cols_;
    pdal::Dimension::Id dim_to_use_as_z_;
};

#endif // GRASSRASTERWRITER_H
