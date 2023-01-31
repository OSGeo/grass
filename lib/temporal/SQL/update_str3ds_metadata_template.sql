--#############################################################################
-- This SQL script is to update a space-time raster3d dataset metadata
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
-- UPDATE FROM syntax: Stefan Blumentrath stefan  <dot>  blumentrath <at> gmx <dot> de
--#############################################################################

-- SPACETIME_REGISTER_TABLE is a placeholder for specific stds map register table name (SQL compliant)
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset

<<<<<<< HEAD
UPDATE str3ds_metadata
   SET
       -- Update the min and max values
       min_min = new_stats.min_min_new,
       min_max = new_stats.min_max_new,
       max_min = new_stats.max_min_new,
       max_max = new_stats.max_max_new,
       -- Update the resolution
       nsres_min = new_stats.nsres_min_new,
       nsres_max = new_stats.nsres_max_new,
       ewres_min = new_stats.ewres_min_new,
       ewres_max = new_stats.ewres_max_new,
       tbres_min = new_stats.tbres_min_new,
       tbres_max = new_stats.tbres_max_new
  FROM
       (SELECT
           min(min) AS min_min_new,
           max(min) AS min_max_new,
           min(max) AS max_min_new,
           max(max) AS max_max_new,
           min(nsres) AS nsres_min_new,
           max(nsres) AS nsres_max_new,
           min(ewres) AS ewres_min_new,
           max(ewres) AS ewres_max_new,
           min(tbres) AS tbres_min_new,
           max(tbres) AS tbres_max_new
       FROM
           SPACETIME_REGISTER_TABLE INNER JOIN
           raster3d_metadata ON
           SPACETIME_REGISTER_TABLE.id = raster3d_metadata.id
       ) AS new_stats
 WHERE str3ds_metadata.id = 'SPACETIME_ID';
=======
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
<<<<<<< HEAD
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
