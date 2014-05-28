--#############################################################################
-- This SQL script is to update the spatial and temporal extent as well as 
-- the modifcation time and revision of a space time dataset. This script
-- should be called when maps inserted or deleted in a space time dataset.
--
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_REGISTER_TABLE is a placeholder for specific stds map register table name (SQL compliant)
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset
-- GRASS_MAP is a placeholder for specific map type: raster, raster3d or vector
-- STDS is a placeholder for specific space-time dataset type: strds, str3ds, stvds

-- UPDATE STDS_base SET modification_time = datetime("NOW") WHERE id = 'SPACETIME_ID';
-- UPDATE STDS_base SET revision = (revision + 1) WHERE id = 'SPACETIME_ID';
-- Number of registered maps
UPDATE STDS_metadata SET number_of_maps = 
       (SELECT count(id) FROM SPACETIME_REGISTER_TABLE)
       WHERE id = 'SPACETIME_ID';
-- Update the temporal extent
UPDATE STDS_absolute_time SET start_time = 
       (SELECT min(start_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_absolute_time SET end_time = 
       (SELECT max(end_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_relative_time SET start_time =
       (SELECT min(start_time) FROM GRASS_MAP_relative_time WHERE GRASS_MAP_relative_time.id IN
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_relative_time SET end_time =
       (SELECT max(end_time) FROM GRASS_MAP_relative_time WHERE GRASS_MAP_relative_time.id IN
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
-- Update the spatial extent
UPDATE STDS_spatial_extent SET north = 
       (SELECT max(north) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_spatial_extent SET south = 
       (SELECT min(south) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_spatial_extent SET east = 
       (SELECT max(east) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_spatial_extent SET west = 
       (SELECT min(west) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_spatial_extent SET top = 
       (SELECT max(top) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_spatial_extent SET bottom = 
       (SELECT min(bottom) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE STDS_spatial_extent SET proj = 
       (SELECT min(proj) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';

