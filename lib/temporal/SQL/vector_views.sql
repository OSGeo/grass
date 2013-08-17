--#############################################################################
-- This SQL script generates two views to access all map specific tables.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- Create the views to access all columns for the absolute and relative time

CREATE VIEW vector_view_abs_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.layer, A1.temporal_type,
            A1.creation_time, 
            A1.creator, 
            A2.start_time, A2.end_time, A2.timezone,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.stvds_register, A4.is_3d, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
            FROM vector_base A1, vector_absolute_time A2, 
            vector_spatial_extent A3, vector_metadata A4 
            WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW vector_view_rel_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.layer, A1.temporal_type,
            A1.creation_time,
            A1.creator, 
            A2.start_time, A2.end_time,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.stvds_register, A4.is_3d, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
            FROM vector_base A1, vector_relative_time A2, 
            vector_spatial_extent A3, vector_metadata A4 
            WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;
