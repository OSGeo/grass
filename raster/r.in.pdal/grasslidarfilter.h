<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
/****************************************************************************
=======
>>>>>>> osgeo-main

 /****************************************************************************
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
/****************************************************************************
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/****************************************************************************
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
#ifndef GRASSLIDARFILTER_H
#define GRASSLIDARFILTER_H

extern "C" {
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

#ifndef GRASSLIDARFILTER_H
#define GRASSLIDARFILTER_H

extern "C"
{
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
#ifndef GRASSLIDARFILTER_H
#define GRASSLIDARFILTER_H

extern "C" {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
#include "filters.h"
#include "lidar.h"
#include "rast_segment.h"
}

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
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
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
#include <pdal/Filter.hpp>
#include <pdal/Streamable.hpp>
#include <pdal/Dimension.hpp>

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/* All GRASS GIS filters which are similar across multiple modules
 * put together as one PDAL Stage class.
 */
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

/* All GRASS GIS filters which are similar across multiple modules
 * put together as one PDAL Stage class.
 */
<<<<<<< HEAD
=======
/* All GRASS GIS filters which are similar across multiple modules
 * put together as one PDAL Stage class.
 */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/* All GRASS GIS filters which are similar across multiple modules
 * put together as one PDAL Stage class.
 */
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    {
    }
    std::string getName() const { return "filters.grasslidar"; }
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    {
    }
    std::string getName() const { return "filters.grasslidar"; }
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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
=======
    {
    }
    std::string getName() const { return "filters.grasslidar"; }
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    {
    }
    std::string getName() const { return "filters.grasslidar"; }
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

    void dim_to_import(pdal::Dimension::Id dim_to_import)
    {
        dim_to_import_ = dim_to_import;
    }

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    void set_spatial_filter(double xmin, double xmax, double ymin, double ymax)
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    void set_spatial_filter(double xmin, double xmax, double ymin, double ymax)
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
    void set_spatial_filter(double xmin, double xmax, double ymin, double ymax)
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
    void set_spatial_filter(double xmin, double xmax, double ymin, double ymax)
=======
>>>>>>> osgeo-main
    void set_spatial_filter(double xmin, double xmax,
                            double ymin, double ymax)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    void set_spatial_filter(double xmin, double xmax, double ymin, double ymax)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    void set_spatial_filter(double xmin, double xmax, double ymin, double ymax)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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
<<<<<<< HEAD
<<<<<<< HEAD
    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
=======
>>>>>>> osgeo-main
    void set_base_raster(SEGMENT * base_segment,
                         struct Cell_head *region, RASTER_MAP_TYPE rtype)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    void set_base_raster(SEGMENT *base_segment, struct Cell_head *region,
                         RASTER_MAP_TYPE rtype)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    {
        base_segment_ = base_segment;
        input_region_ = region;
        base_raster_data_type_ = rtype;
    }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
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
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
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
=======
    void set_z_scale(double scale) { zscale_ = scale; }
    void set_intensity_scale(double scale) { iscale_ = scale; }
    void set_d_scale(double scale) { dscale_ = scale; }
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

    gpoint_count num_processed() { return n_processed_; }
    gpoint_count num_passed() { return n_passed_; }
    gpoint_count num_return_filtered() { return return_filtered_; }
    gpoint_count num_class_filtered() { return n_class_filtered_; }
    gpoint_count num_zrange_filtered() { return zrange_filtered_; }
    gpoint_count num_irange_filtered() { return irange_filtered_; }
    gpoint_count num_drange_filtered() { return drange_filtered_; }
    gpoint_count num_spatially_filtered() { return n_outside_; }

<<<<<<< HEAD
  private:
    virtual void filter(pdal::PointView & view)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
private:
    virtual void filter(pdal::PointView &view)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    {
        pdal::PointRef p(view, 0);
        for (pdal::PointId idx = 0; idx < view.size(); ++idx) {
            p.setPointId(idx);
            processOne(p);
        }
    }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    virtual bool processOne(pdal::PointRef &point);
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    virtual bool processOne(pdal::PointRef &point);
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
    virtual bool processOne(pdal::PointRef &point);
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
    virtual bool processOne(pdal::PointRef &point);
=======
>>>>>>> osgeo-main
    virtual bool processOne(pdal::PointRef & point);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    virtual bool processOne(pdal::PointRef &point);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    virtual bool processOne(pdal::PointRef &point);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

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
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
    GrassLidarFilter &operator=(const GrassLidarFilter &);
    GrassLidarFilter(const GrassLidarFilter &);
};

<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    GrassLidarFilter & operator=(const GrassLidarFilter &);
    GrassLidarFilter(const GrassLidarFilter &);
};


>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    GrassLidarFilter &operator=(const GrassLidarFilter &);
    GrassLidarFilter(const GrassLidarFilter &);
};

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
#endif // GRASSLIDARFILTER_H
