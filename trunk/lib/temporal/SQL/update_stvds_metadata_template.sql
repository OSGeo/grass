--#############################################################################
-- This SQL script is for now a placeholder, till the vector metadata
-- concept is clear
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_REGISTER_TABLE is a placeholder for specific stds map register table name (SQL compliant)
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset

-- Update the vector features and topology 
UPDATE stvds_metadata SET points = 
       (SELECT sum(points) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET lines = 
       (SELECT sum(lines) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET boundaries = 
       (SELECT sum(boundaries) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET centroids = 
       (SELECT sum(centroids) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET faces = 
       (SELECT sum(faces) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET kernels = 
       (SELECT sum(kernels) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET primitives = 
       (SELECT sum(primitives) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET nodes = 
       (SELECT sum(nodes) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET areas = 
       (SELECT sum(areas) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET islands = 
       (SELECT sum(islands) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET holes = 
       (SELECT sum(holes) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
UPDATE stvds_metadata SET volumes = 
       (SELECT sum(volumes) FROM vector_metadata WHERE vector_metadata.id IN 
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) WHERE id = 'SPACETIME_ID';
