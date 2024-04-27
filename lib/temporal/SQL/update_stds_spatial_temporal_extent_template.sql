--#############################################################################
-- This SQL script is to update the spatial and temporal extent as well as
-- the modification time and revision of a space time dataset. This script
-- should be called when maps inserted or deleted in a space time dataset.
--
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
-- UPDATE FROM syntax: Stefan Blumentrath stefan  <dot>  blumentrath <at> gmx <dot> de
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
UPDATE STDS_absolute_time
   SET
       start_time = new_stats.start_time_new,
       end_time = new_stats.end_time_new
  FROM
       (SELECT
           min(start_time) AS start_time_new,
           max(end_time) AS end_time_new
        FROM
            SPACETIME_REGISTER_TABLE INNER JOIN
            GRASS_MAP_absolute_time ON
            SPACETIME_REGISTER_TABLE.id = GRASS_MAP_absolute_time.id
       ) AS new_stats
 WHERE STDS_absolute_time.id = 'SPACETIME_ID';

UPDATE STDS_relative_time
   SET
       start_time = new_stats.start_time_new,
       end_time = new_stats.end_time_new
  FROM
       (SELECT
           min(start_time) AS start_time_new,
           max(end_time) AS end_time_new
       FROM
           SPACETIME_REGISTER_TABLE INNER JOIN
           GRASS_MAP_relative_time ON
           SPACETIME_REGISTER_TABLE.id = GRASS_MAP_relative_time.id
       ) AS new_stats
 WHERE STDS_relative_time.id = 'SPACETIME_ID';

-- Update the spatial extent
UPDATE STDS_spatial_extent
   SET
       north = new_stats.north_new,
       south = new_stats.south_new,
       east = new_stats.east_new,
       west = new_stats.west_new,
       top = new_stats.top_new,
       bottom = new_stats.bottom_new,
       proj = new_stats.proj_new
  FROM
       (SELECT
           max(north) AS north_new,
           min(south) AS south_new,
           max(east) AS east_new,
           min(west) AS west_new,
           max(top) AS top_new,
           min(bottom) AS bottom_new,
           min(proj) AS proj_new
       FROM
           SPACETIME_REGISTER_TABLE INNER JOIN
           GRASS_MAP_spatial_extent ON
           SPACETIME_REGISTER_TABLE.id = GRASS_MAP_spatial_extent.id
       ) AS new_stats
 WHERE STDS_spatial_extent.id = 'SPACETIME_ID';
