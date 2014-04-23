--#############################################################################
-- This SQL script is to update a space-time raster3d dataset metadata
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_REGISTER_TABLE is a placeholder for specific stds map register table name (SQL compliant)
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset

-- Update the min and max values
UPDATE str3ds_metadata SET min_min = 
       (SELECT min(min) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET min_max = 
       (SELECT max(min) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET max_min = 
       (SELECT min(max) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET max_max = 
       (SELECT max(max) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
-- Update the resolution
UPDATE str3ds_metadata SET nsres_min = 
       (SELECT min(nsres) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET nsres_max = 
       (SELECT max(nsres) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET ewres_min = 
       (SELECT min(ewres) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET ewres_max = 
       (SELECT max(ewres) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET tbres_min = 
       (SELECT min(tbres) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE str3ds_metadata SET tbres_max = 
       (SELECT max(tbres) FROM raster3d_metadata WHERE raster3d_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';

