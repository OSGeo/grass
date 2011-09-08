--#############################################################################
-- This SQL script generates the table in which all registered GRASS_MAP maps 
-- are listed in.
--
-- This table will be created for each STDS.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_NAME is a placeholder for specific stds name (SQL compliant): name_mapset
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset
-- GRASS_MAP is a placeholder for specific map type: raster, raster3d or vector
-- STDS is a placeholder for specific space-time dataset type: strds, str3ds, stvds

PRAGMA foreign_keys = ON;

-- This table stores the.ids of the GRASS_MAP maps registered in the current spacetime GRASS_MAP table
CREATE TABLE  SPACETIME_NAME_GRASS_MAP_register (
  id VARCHAR NOT NULL, -- This colum is a primary foreign key storing the registered GRASS_MAP map.ids
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE
);

CREATE TRIGGER SPACETIME_NAME_GRASS_MAP_register_insert_trigger AFTER INSERT ON SPACETIME_NAME_GRASS_MAP_register 
  BEGIN
    UPDATE STDS_base SET modification_time = datetime("NOW") WHERE id = "SPACETIME_ID";
    UPDATE STDS_base SET revision = (revision + 1) WHERE id = "SPACETIME_ID";
    -- Number of registered maps
    UPDATE STDS_metadata SET number_of_maps = 
           (SELECT count(id) FROM SPACETIME_NAME_GRASS_MAP_register)
           WHERE id = "SPACETIME_ID";
    -- Update the temporal extent
    UPDATE STDS_absolute_time SET start_time = 
           (SELECT min(start_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_absolute_time SET end_time = 
           (SELECT max(end_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    -- Update the spatial extent
    UPDATE STDS_spatial_extent SET north = 
           (SELECT max(north) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET south = 
           (SELECT min(south) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET east = 
           (SELECT max(east) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET west = 
           (SELECT min(west) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET top = 
           (SELECT max(top) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET bottom = 
           (SELECT min(bottom) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET proj = 
           (SELECT min(proj) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
  END;

CREATE TRIGGER SPACETIME_NAME_GRASS_MAP_register_delete_trigger AFTER DELETE ON SPACETIME_NAME_GRASS_MAP_register 
  BEGIN
    UPDATE STDS_base SET modification_time = datetime("NOW") WHERE id = "SPACETIME_ID";
    UPDATE STDS_base SET revision = (revision + 1) WHERE id = "SPACETIME_ID";
    -- Number of registered maps
    UPDATE STDS_metadata SET number_of_maps = 
           (SELECT count(id) FROM  SPACETIME_NAME_GRASS_MAP_register)
           WHERE id = "SPACETIME_ID";
    -- Update the temporal extent
    UPDATE STDS_absolute_time SET start_time = 
           (SELECT min(start_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_absolute_time SET end_time = 
           (SELECT max(end_time) FROM GRASS_MAP_absolute_time WHERE GRASS_MAP_absolute_time.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    -- Update the spatial extent
    UPDATE STDS_spatial_extent SET north = 
           (SELECT max(north) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET south = 
           (SELECT min(south) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET east = 
           (SELECT max(east) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET west = 
           (SELECT min(west) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET top = 
           (SELECT max(top) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET bottom = 
           (SELECT min(bottom) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
    UPDATE STDS_spatial_extent SET proj = 
           (SELECT min(proj) FROM GRASS_MAP_spatial_extent WHERE GRASS_MAP_spatial_extent.id IN 
			(SELECT id FROM SPACETIME_NAME_GRASS_MAP_register)
           ) WHERE id = "SPACETIME_ID";
  END;

