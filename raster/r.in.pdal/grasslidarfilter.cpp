<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
/****************************************************************************
=======

 /****************************************************************************
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
/****************************************************************************
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
/****************************************************************************
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
#include "grasslidarfilter.h"

bool GrassLidarFilter::processOne(pdal::PointRef &point)
{
    using pdal::Dimension::Id;

    double x = point.getFieldAs<double>(Id::X);
    double y = point.getFieldAs<double>(Id::Y);
    double z = point.getFieldAs<double>(Id::Z);
<<<<<<< HEAD
=======

=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
#include "grasslidarfilter.h"

bool GrassLidarFilter::processOne(pdal::PointRef &point)
{
    using pdal::Dimension::Id;

<<<<<<< HEAD
    double x = point.getFieldAs < double >(Id::X);
    double y = point.getFieldAs < double >(Id::Y);
    double z = point.getFieldAs < double >(Id::Z);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    double x = point.getFieldAs<double>(Id::X);
    double y = point.getFieldAs<double>(Id::Y);
    double z = point.getFieldAs<double>(Id::Z);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    n_processed_++;

    z *= zscale_;

    if (use_spatial_filter_) {
        if (x < xmin_ || x > xmax_ || y < ymin_ || y > ymax_) {
            n_outside_++;
            return false;
        }
    }
    if (use_irange_) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        double intensity = point.getFieldAs<double>(Id::Intensity);
=======
        double intensity = point.getFieldAs < double >(Id::Intensity);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        double intensity = point.getFieldAs<double>(Id::Intensity);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        double intensity = point.getFieldAs<double>(Id::Intensity);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        intensity *= iscale_;
        if (intensity < imin_ || intensity > imax_) {
            irange_filtered_++;
            return false;
        }
    }
    if (use_drange_) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        double value = point.getFieldAs<double>(dim_to_import_);
=======
        double value = point.getFieldAs < double >(dim_to_import_);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        double value = point.getFieldAs<double>(dim_to_import_);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        double value = point.getFieldAs<double>(dim_to_import_);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        value *= dscale_;
        if (value < dmin_ || value > dmax_) {
            drange_filtered_++;
            return false;
        }
    }
    if (base_segment_) {
        double base_z;

        if (rast_segment_get_value_xy(base_segment_, input_region_,
                                      base_raster_data_type_, x, y, &base_z))
            z -= base_z;
        else
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            return false; // skip points outside of base raster
=======
            return false;       // skip points outside of base raster
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
            return false; // skip points outside of base raster
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            return false; // skip points outside of base raster
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    }
    if (use_zrange_) {
        if (z < zmin_ || z > zmax_) {
            zrange_filtered_++;
            return false;
        }
    }
    if (use_return_filter_) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int return_n = point.getFieldAs<int>(Id::ReturnNumber);
        int n_returns = point.getFieldAs<int>(Id::NumberOfReturns);
=======
        int return_n = point.getFieldAs < int >(Id::ReturnNumber);
        int n_returns = point.getFieldAs < int >(Id::NumberOfReturns);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        int return_n = point.getFieldAs<int>(Id::ReturnNumber);
        int n_returns = point.getFieldAs<int>(Id::NumberOfReturns);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        int return_n = point.getFieldAs<int>(Id::ReturnNumber);
        int n_returns = point.getFieldAs<int>(Id::NumberOfReturns);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        if (return_filter_is_out(&return_filter_, return_n, n_returns)) {
            return_filtered_++;
            return false;
        }
    }
    if (use_class_filter_) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        int point_class = point.getFieldAs<int>(Id::Classification);
=======
        int point_class = point.getFieldAs < int >(Id::Classification);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        int point_class = point.getFieldAs<int>(Id::Classification);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        int point_class = point.getFieldAs<int>(Id::Classification);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        if (class_filter_is_out(&class_filter_, point_class)) {
            n_class_filtered_++;
            return false;
        }
    }
    n_passed_++;

    // PDAL filter streaming function should return false when
    // point is filtered out (and should not be used by next stage)
    // and true otherwise.
    return true;
}
