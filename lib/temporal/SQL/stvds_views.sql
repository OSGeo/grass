--#############################################################################
-- This SQL script generates the space time vector dataset view.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- Create the views to access all columns for absolute or relative time

CREATE VIEW stvds_view_abs_time AS SELECT 
            A1.id, A1.name, A1.mapset, A1.temporal_type,
            A1.semantic_type, 
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
            A2.start_time, A2.end_time, A2.timezone,
            A2.granularity,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.vector_register,
            A4.number_of_maps, 
            A4.title, A4.description, A4.command, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
            FROM stvds_base A1, stvds_absolute_time A2,  
            stvds_spatial_extent A3, stvds_metadata A4 WHERE A1.id = A2.id AND 
            A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW stvds_view_rel_time AS SELECT 
            A1.id, A1.name, A1.mapset, A1.temporal_type,
            A1.semantic_type, 
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
            A2.start_time, A2.end_time, A2.granularity,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.vector_register,
            A4.number_of_maps, 
            A4.title, A4.description, A4.command, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
            FROM stvds_base A1, stvds_relative_time A2,  
            stvds_spatial_extent A3, stvds_metadata A4 WHERE A1.id = A2.id AND 
            A1.id = A3.id AND A1.id = A4.id;
