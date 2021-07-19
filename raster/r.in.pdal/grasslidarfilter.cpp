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
#include "grasslidarfilter.h"

bool GrassLidarFilter::processOne(pdal::PointRef &point)
{
    using pdal::Dimension::Id;

    double x = point.getFieldAs<double>(Id::X);
    double y = point.getFieldAs<double>(Id::Y);
    double z = point.getFieldAs<double>(Id::Z);
=======

#include "grasslidarfilter.h"


bool GrassLidarFilter::processOne(pdal::PointRef & point)
{
    using pdal::Dimension::Id;

    double x = point.getFieldAs < double >(Id::X);
    double y = point.getFieldAs < double >(Id::Y);
    double z = point.getFieldAs < double >(Id::Z);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

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
        double intensity = point.getFieldAs<double>(Id::Intensity);
=======
        double intensity = point.getFieldAs < double >(Id::Intensity);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

        intensity *= iscale_;
        if (intensity < imin_ || intensity > imax_) {
            irange_filtered_++;
            return false;
        }
    }
    if (use_drange_) {
<<<<<<< HEAD
        double value = point.getFieldAs<double>(dim_to_import_);
=======
        double value = point.getFieldAs < double >(dim_to_import_);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

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
            return false; // skip points outside of base raster
=======
            return false;       // skip points outside of base raster
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    }
    if (use_zrange_) {
        if (z < zmin_ || z > zmax_) {
            zrange_filtered_++;
            return false;
        }
    }
    if (use_return_filter_) {
<<<<<<< HEAD
        int return_n = point.getFieldAs<int>(Id::ReturnNumber);
        int n_returns = point.getFieldAs<int>(Id::NumberOfReturns);
=======
        int return_n = point.getFieldAs < int >(Id::ReturnNumber);
        int n_returns = point.getFieldAs < int >(Id::NumberOfReturns);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

        if (return_filter_is_out(&return_filter_, return_n, n_returns)) {
            return_filtered_++;
            return false;
        }
    }
    if (use_class_filter_) {
<<<<<<< HEAD
        int point_class = point.getFieldAs<int>(Id::Classification);
=======
        int point_class = point.getFieldAs < int >(Id::Classification);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))

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
