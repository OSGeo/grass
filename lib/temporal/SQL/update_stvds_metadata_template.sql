--#############################################################################
-- This SQL script is for now a placeholder, till the vector metadata
-- concept is clear
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
-- UPDATE FROM syntax: Stefan Blumentrath stefan  <dot>  blumentrath <at> gmx <dot> de
--#############################################################################

-- SPACETIME_REGISTER_TABLE is a placeholder for specific stds map register table name (SQL compliant)
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset

-- Update the vector features and topology
UPDATE stvds_metadata
   SET
       stvds_metadata.points = new_stats.points_new,
       stvds_metadata.lines = new_stats.lines_new,
       stvds_metadata.boundaries = new_stats.boundaries_new,
       stvds_metadata.centroids = new_stats.centroids_new,
       stvds_metadata.faces = new_stats.faces_new,
       stvds_metadata.kernels = new_stats.kernels_new,
       stvds_metadata.primitives = new_stats.primitives_new,
       stvds_metadata.nodes = new_stats.nodes_new,
       stvds_metadata.areas = new_stats.areas_new,
       stvds_metadata.islands = new_stats.islands_new,
       stvds_metadata.holes = new_stats.holes_new,
       stvds_metadata.volumes = new_stats.volumes_new
  FROM
       (SELECT
           sum(points) AS points_new,
           sum(lines) AS lines_new,
           sum(boundaries) AS boundaries_new,
           sum(centroids) AS centroids_new,
           sum(faces) AS faces_new,
           sum(kernels) AS kernels_new,
           sum(primitives) AS primitives_new,
           sum(nodes) AS nodes_new,
           sum(areas) AS areas_new,
           sum(islands) AS islands_new,
           sum(holes) AS holes_new,
           sum(volumes) volumes = volumes_new
       FROM vector_metadata WHERE vector_metadata.id IN
    		(SELECT id FROM SPACETIME_REGISTER_TABLE)
       ) AS new_stats
 WHERE stvds_metadata.id = 'SPACETIME_ID';
