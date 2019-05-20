 /****************************************************************************
 *
 * MODULE:    r.in.pdal
 *
 * AUTHOR(S): Vaclav Petras
 *
 * PURPOSE:   Imports LAS LiDAR point clouds to a raster map using
 *            aggregate statistics.
 *
 * COPYRIGHT: (C) 2011-2019 by Vaclav Petras and the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/


#ifndef GRASSLIDARFILTER_H
#define GRASSLIDARFILTER_H

extern "C"
{
#include "filters.h"
#include "lidar.h"
#include "rast_segment.h"
}

#include <pdal/Filter.hpp>
#include <pdal/Streamable.hpp>
#include <pdal/Dimension.hpp>


/* All GRASS GIS filters which are similar across multiple modules
 * put together as one PDAL Stage class.
 */
class GrassLidarFilter : public pdal::Filter, public pdal::Streamable
{
public:
    GrassLidarFilter() :
        n_processed_(0), n_passed_(0),
        n_outside_(0), zrange_filtered_(0), irange_filtered_(0),
        return_filtered_(0), n_class_filtered_(0),
        use_spatial_filter_(false),
        use_zrange_ (false), use_irange_(false),
        use_return_filter_(false), use_class_filter_(false),
        base_segment_(nullptr),
        zscale_(1), iscale_(1),
        dim_to_use_as_z_(pdal::Dimension::Id::Z),
        dim_to_use_as_i_(pdal::Dimension::Id::Intensity)
    {}
    std::string getName() const
    {
        return "filters.grasslidar";
    }

    void set_spatial_filter(double xmin, double xmax,
                            double ymin, double ymax)
    {
        use_spatial_filter_ = true;
        xmin_ = xmin;
        xmax_ = xmax;
        ymin_ = ymin;
        ymax_ = ymax;
        n_outside_ = 0;
    }
    void set_zrange_filter(double min, double max)
    {
        use_zrange_ = true;
        zmin_ = min;
        zmax_ = max;
        zrange_filtered_ = 0;
    }
    void set_irange_filter(double min, double max)
    {
        use_irange_ = true;
        imin_ = min;
        imax_ = max;
        irange_filtered_ = 0;
    }
    void set_return_filter(ReturnFilter return_filter)
    {
        use_return_filter_ = true;
        return_filter_ = return_filter;
        return_filtered_ = 0;
    }
    void set_class_filter(ClassFilter class_filter)
    {
        use_class_filter_ = true;
        class_filter_ = class_filter;
        n_class_filtered_ = 0;
    }
    void set_base_raster(SEGMENT *base_segment,
                         struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
    {
        base_segment_ = base_segment;
        input_region_ =  region;
        base_raster_data_type_ = rtype;
    }
    void set_z_scale(double scale) { zscale_ = scale; }
    void set_intensity_scale(double scale) { iscale_ = scale; }

    gpoint_count num_processed() { return n_processed_; }
    gpoint_count num_passed() { return n_passed_; }
    gpoint_count num_return_filtered() { return return_filtered_; }
    gpoint_count num_class_filtered() { return n_class_filtered_; }
    gpoint_count num_zrange_filtered() { return zrange_filtered_; }
    gpoint_count num_irange_filtered() { return irange_filtered_; }
    gpoint_count num_spatially_filtered() { return n_outside_; }

private:
    virtual void filter(pdal::PointView& view)
    {
        pdal::PointRef p(view, 0);
        for (pdal::PointId idx = 0; idx < view.size(); ++idx)
        {
            p.setPointId(idx);
            processOne(p);
        }
    }
    virtual bool processOne(pdal::PointRef& point);

    // TODO: define the dimension!
    pdal::Dimension::Id dim_to_use_as_z_;
    pdal::Dimension::Id dim_to_use_as_i_;

    bool use_spatial_filter_;
    bool use_zrange_;
    bool use_irange_;
    double xmin_;
    double xmax_;
    double ymin_;
    double ymax_;
    double zmin_;
    double zmax_;
    double imin_;
    double imax_;
    gpoint_count n_processed_;
    gpoint_count n_passed_;
    gpoint_count n_outside_;
    gpoint_count zrange_filtered_;
    gpoint_count irange_filtered_;
    gpoint_count return_filtered_;
    gpoint_count n_class_filtered_;

    double zscale_;
    double iscale_;

    bool use_class_filter_;
    ClassFilter class_filter_;
    bool use_return_filter_;
    ReturnFilter return_filter_;

    SEGMENT *base_segment_;
    struct Cell_head *input_region_;
    RASTER_MAP_TYPE base_raster_data_type_;

    // not implemented
    GrassLidarFilter& operator=(const GrassLidarFilter&);
    GrassLidarFilter(const GrassLidarFilter&);
};


#endif // GRASSLIDARFILTER_H
