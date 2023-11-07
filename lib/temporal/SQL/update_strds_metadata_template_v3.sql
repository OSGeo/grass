--#############################################################################
-- This SQL is to update a space-time raster dataset metadata (version 3+)
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_REGISTER_TABLE is a placeholder for specific stds map register table name (SQL compliant)
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset

-- Update the number of registered semantic labels
UPDATE strds_metadata SET number_of_semantic_labels =
       (SELECT count(distinct semantic_label) FROM raster_metadata WHERE
       raster_metadata.id IN
               (SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
