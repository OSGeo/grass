--#############################################################################
-- This SQL script generates the space time raster dataset view.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- Create the views to access all cols for absolute or relative time

CREATE VIEW strds_view_abs_time AS SELECT 
            A1.id, A1.name, A1.mapset, A1.temporal_type,
            A1.semantic_type, 
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
            A2.start_time, A2.end_time, A2.timezone, A2.granularity,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.raster_register,
            A4.number_of_maps, 
            A4.nsres_min, A4.ewres_min, 
            A4.nsres_max, A4.ewres_max, 
            A4.min_min, A4.min_max,
            A4.max_min, A4.max_max,
            A4.title, A4.description, A4.command	
            FROM strds_base A1, strds_absolute_time A2,  
            strds_spatial_extent A3, strds_metadata A4 WHERE A1.id = A2.id AND 
            A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW strds_view_rel_time AS SELECT 
            A1.id, A1.name, A1.mapset, A1.temporal_type,
            A1.semantic_type, 
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
            A2.start_time, A2.end_time, A2.granularity,
            A3.north, A3.south, A3.east, A3.west, A3.bottom, A3.top, A3.proj,
            A4.raster_register,
            A4.number_of_maps, 
            A4.nsres_min, A4.ewres_min, 
            A4.nsres_max, A4.ewres_max, 
            A4.min_min, A4.min_max,
            A4.max_min, A4.max_max,
            A4.title, A4.description, A4.command
            FROM strds_base A1, strds_relative_time A2,  
            strds_spatial_extent A3, strds_metadata A4 WHERE A1.id = A2.id AND 
            A1.id = A3.id AND A1.id = A4.id;
