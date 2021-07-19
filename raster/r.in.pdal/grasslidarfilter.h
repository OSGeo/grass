<<<<<<< HEAD
/****************************************************************************
=======

 /****************************************************************************
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
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

<<<<<<< HEAD
#ifndef GRASSLIDARFILTER_H
#define GRASSLIDARFILTER_H

extern "C" {
=======

#ifndef GRASSLIDARFILTER_H
#define GRASSLIDARFILTER_H

extern "C"
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
#include "filters.h"
#include "lidar.h"
#include "rast_segment.h"
}

<<<<<<< HEAD
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif
#include <pdal/Filter.hpp>
#include <pdal/Streamable.hpp>
#include <pdal/Dimension.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
=======
#include <pdal/Filter.hpp>
#include <pdal/Streamable.hpp>
#include <pdal/Dimension.hpp>

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

/* All GRASS GIS filters which are similar across multiple modules
 * put together as one PDAL Stage class.
 */
<<<<<<< HEAD
class GrassLidarFilter : public pdal::Filter, public pdal::Streamable {
public:
    GrassLidarFilter()
        : dim_to_import_(pdal::Dimension::Id::Z), use_spatial_filter_(false),
          use_zrange_(false), use_irange_(false), use_drange_(false), xmin_(0),
          xmax_(0), ymin_(0), ymax_(0), zmin_(0), zmax_(0), imin_(0), imax_(0),
          dmin_(0), dmax_(0), n_processed_(0), n_passed_(0), n_outside_(0),
          zrange_filtered_(0), irange_filtered_(0), drange_filtered_(0),
          return_filtered_(0), n_class_filtered_(0), zscale_(1), iscale_(1),
          dscale_(1), use_class_filter_(false), use_return_filter_(false),
          base_segment_(nullptr)
    {
    }
    std::string getName() const { return "filters.grasslidar"; }
=======
class GrassLidarFilter:public pdal::Filter, public pdal::Streamable
{
  public:
    GrassLidarFilter():
        dim_to_import_(pdal::Dimension::Id::Z),
        use_spatial_filter_(false),
        use_zrange_(false),
        use_irange_(false),
        use_drange_(false),
        xmin_(0),
        xmax_(0),
        ymin_(0),
        ymax_(0),
        zmin_(0),
        zmax_(0),
        imin_(0),
        imax_(0),
        dmin_(0),
        dmax_(0),
        n_processed_(0),
        n_passed_(0),
        n_outside_(0),
        zrange_filtered_(0),
        irange_filtered_(0),
        drange_filtered_(0),
        return_filtered_(0),
        n_class_filtered_(0),
        zscale_(1),
        iscale_(1),
        dscale_(1),
        use_class_filter_(false),
        use_return_filter_(false), base_segment_(nullptr)
    {
    }
    std::string getName() const
    {
        return "filters.grasslidar";
    }
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

    void dim_to_import(pdal::Dimension::Id dim_to_import)
    {
        dim_to_import_ = dim_to_import;
    }

<<<<<<< HEAD
    void set_spatial_filter(double xmin, double xmax, double ymin, double ymax)
=======
    void set_spatial_filter(double xmin, double xmax,
                            double ymin, double ymax)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
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
    void set_drange_filter(double min, double max)
    {
        use_drange_ = true;
        dmin_ = min;
        dmax_ = max;
        drange_filtered_ = 0;
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
<<<<<<< HEAD
    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
=======
    void set_base_raster(SEGMENT * base_segment,
                         struct Cell_head *region, RASTER_MAP_TYPE rtype)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    {
        base_segment_ = base_segment;
        input_region_ = region;
        base_raster_data_type_ = rtype;
    }
<<<<<<< HEAD
    void set_z_scale(double scale) { zscale_ = scale; }
    void set_intensity_scale(double scale) { iscale_ = scale; }
    void set_d_scale(double scale) { dscale_ = scale; }

    gpoint_count num_processed() { return n_processed_; }
    gpoint_count num_passed() { return n_passed_; }
    gpoint_count num_return_filtered() { return return_filtered_; }
    gpoint_count num_class_filtered() { return n_class_filtered_; }
    gpoint_count num_zrange_filtered() { return zrange_filtered_; }
    gpoint_count num_irange_filtered() { return irange_filtered_; }
    gpoint_count num_drange_filtered() { return drange_filtered_; }
    gpoint_count num_spatially_filtered() { return n_outside_; }

private:
    virtual void filter(pdal::PointView &view)
=======
    void set_z_scale(double scale)
    {
        zscale_ = scale;
    }
    void set_intensity_scale(double scale)
    {
        iscale_ = scale;
    }
    void set_d_scale(double scale)
    {
        dscale_ = scale;
    }

    gpoint_count num_processed()
    {
        return n_processed_;
    }
    gpoint_count num_passed()
    {
        return n_passed_;
    }
    gpoint_count num_return_filtered()
    {
        return return_filtered_;
    }
    gpoint_count num_class_filtered()
    {
        return n_class_filtered_;
    }
    gpoint_count num_zrange_filtered()
    {
        return zrange_filtered_;
    }
    gpoint_count num_irange_filtered()
    {
        return irange_filtered_;
    }
    gpoint_count num_drange_filtered()
    {
        return drange_filtered_;
    }
    gpoint_count num_spatially_filtered()
    {
        return n_outside_;
    }

  private:
    virtual void filter(pdal::PointView & view)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    {
        pdal::PointRef p(view, 0);
        for (pdal::PointId idx = 0; idx < view.size(); ++idx) {
            p.setPointId(idx);
            processOne(p);
        }
    }
<<<<<<< HEAD
    virtual bool processOne(pdal::PointRef &point);
=======
    virtual bool processOne(pdal::PointRef & point);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

    pdal::Dimension::Id dim_to_import_;

    bool use_spatial_filter_;
    bool use_zrange_;
    bool use_irange_;
    bool use_drange_;
    double xmin_;
    double xmax_;
    double ymin_;
    double ymax_;
    double zmin_;
    double zmax_;
    double imin_;
    double imax_;
    double dmin_;
    double dmax_;
    gpoint_count n_processed_;
    gpoint_count n_passed_;
    gpoint_count n_outside_;
    gpoint_count zrange_filtered_;
    gpoint_count irange_filtered_;
    gpoint_count drange_filtered_;
    gpoint_count return_filtered_;
    gpoint_count n_class_filtered_;

    double zscale_;
    double iscale_;
    double dscale_;

    bool use_class_filter_;
    ClassFilter class_filter_;
    bool use_return_filter_;
    ReturnFilter return_filter_;

    SEGMENT *base_segment_;
    struct Cell_head *input_region_;
    RASTER_MAP_TYPE base_raster_data_type_;

    // not implemented
<<<<<<< HEAD
    GrassLidarFilter &operator=(const GrassLidarFilter &);
    GrassLidarFilter(const GrassLidarFilter &);
};

=======
    GrassLidarFilter & operator=(const GrassLidarFilter &);
    GrassLidarFilter(const GrassLidarFilter &);
};


>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
#endif // GRASSLIDARFILTER_H
