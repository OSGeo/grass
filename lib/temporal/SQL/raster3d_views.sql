--#############################################################################
-- This SQL script generates two views to access all map specific tables.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################


-- Create the views to access all cols for the absolute and relative time

CREATE VIEW raster3d_view_abs_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator,
            A2.start_time, A2.end_time, A2.timezone,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.datatype, A4.cols, A4.rows, A4.depths,
            A4.nsres, A4.ewres, A4.tbres,
            A4.min, A4.max,
            A4.str3ds_register,
            A4.number_of_cells
            FROM raster3d_base A1, raster3d_absolute_time A2, 
            raster3d_spatial_extent A3, raster3d_metadata A4 
            WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW raster3d_view_rel_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator,
            A2.start_time, A2.end_time,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.datatype, A4.cols, A4.rows, A4.depths,
            A4.nsres, A4.ewres, A4.tbres,
            A4.min, A4.max,
            A4.str3ds_register,
            A4.number_of_cells
            FROM raster3d_base A1, raster3d_relative_time A2, 
            raster3d_spatial_extent A3, raster3d_metadata A4 
            WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;