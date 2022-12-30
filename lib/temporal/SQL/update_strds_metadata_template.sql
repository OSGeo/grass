--#############################################################################
-- This SQL is to update a space-time raster dataset metadata
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_REGISTER_TABLE is a placeholder for specific stds map register table name (SQL compliant)
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset

-- Update the min and max values
UPDATE strds_metadata SET min_min = 
       (SELECT min(min) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE strds_metadata SET min_max = 
       (SELECT max(min) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE strds_metadata SET max_min = 
       (SELECT min(max) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE strds_metadata SET max_max = 
       (SELECT max(max) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
-- Update the resolution
UPDATE strds_metadata SET nsres_min = 
       (SELECT min(nsres) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE strds_metadata SET nsres_max = 
       (SELECT max(nsres) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE strds_metadata SET ewres_min = 
       (SELECT min(ewres) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE strds_metadata SET ewres_max = 
       (SELECT max(ewres) FROM raster_metadata WHERE raster_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
