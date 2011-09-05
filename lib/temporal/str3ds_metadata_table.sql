--#############################################################################
-- This SQL script generates the space time raster3d dataset metadata table,
-- view and trigger
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

PRAGMA foreign_keys = ON;

CREATE TABLE  str3ds_metadata (
  id VARCHAR NOT NULL,               -- Id of the space-time raster3d dataset, this is the primary foreign key
  raster3d_register VARCHAR,         -- The id of the table in which the raster3d maps are registered for this dataset
  number_of_registered_maps INTEGER, -- The number of registered raster3d maps
  max_min DOUBLE PRECISION,          -- The minimal maximum of the registered raster3d maps
  min_min DOUBLE PRECISION,          -- The minimal minimum of the registered raster3d maps
  max_max DOUBLE PRECISION,          -- The maximal maximum of the registered raster3d maps
  min_max DOUBLE PRECISION,          -- The maximal minimum of the registered raster3d maps
  nsres_min DOUBLE PRECISION,        -- The lowest north-south resolution of the registered raster3d maps
  nsres_max DOUBLE PRECISION,        -- The highest north-south resolution of the registered raster3d maps
  ewres_min DOUBLE PRECISION,        -- The lowest east-west resolution of the registered raster3d maps
  ewres_max DOUBLE PRECISION,        -- The highest east-west resolution of the registered raster3d maps
  tbres_min DOUBLE PRECISION,        -- The lowest top-bottom resolution of the registered raster3d maps
  tbres_max DOUBLE PRECISION,        -- The highest top-bottom resolution of the registered raster3d maps
  title VARCHAR,                     -- Title of the space-time raster3d dataset
  description VARCHAR,               -- Detailed description of the space-time raster3d dataset
  PRIMARY KEY (id),  
  FOREIGN KEY (id) REFERENCES  str3ds_base (id) ON DELETE CASCADE
);
-- Create the views to access all cols for absolute or relative time

CREATE VIEW str3ds_view_abs_time AS SELECT 
            A1.id, A1.temporal_type,
            A1.creator, A1.semantic_type,  
            A1.creation_time, A1.modification_time,
            A1.revision, A2.start_time,
	    A2.end_time, A2.granularity,
	    A3.north, A3.south, A3.east, A3.west,
	    A4.raster3d_register,
	    A4.number_of_registered_maps, 
            A4.nsres_min, A4.ewres_min, 
            A4.nsres_max, A4.ewres_max, 
            A4.tbres_min, A4.tbres_max, 
	    A4.min_min, A4.min_max,
	    A4.max_min, A4.max_max,
            A4.title, A4.description	
	    FROM str3ds_base A1, str3ds_absolute_time A2,  
            str3ds_spatial_extent A3, str3ds_metadata A4 WHERE A1.id = A2.id AND 
	    A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW str3ds_view_rel_time AS SELECT 
            A1.id, A1.temporal_type,
            A1.creator, A1.semantic_type,  
            A1.creation_time, A1.modification_time,
            A1.revision, 
	    A2.interval, A2.granularity,
	    A3.north, A3.south, A3.east, A3.west,
	    A4.raster3d_register,
	    A4.number_of_registered_maps, 
            A4.nsres_min, A4.ewres_min, 
            A4.nsres_max, A4.ewres_max, 
            A4.tbres_min, A4.tbres_max, 
	    A4.min_min, A4.min_max,
	    A4.max_min, A4.max_max,
            A4.title, A4.description	
	    FROM str3ds_base A1, str3ds_relative_time A2,  
            str3ds_spatial_extent A3, str3ds_metadata A4 WHERE A1.id = A2.id AND 
	    A1.id = A3.id AND A1.id = A4.id;


-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 

CREATE TRIGGER update_str3ds_metadata AFTER UPDATE ON str3ds_metadata 
  BEGIN
    UPDATE str3ds_base SET modification_time = datetime("NOW") WHERE id = old.id;
    UPDATE str3ds_base SET revision = (revision + 1) WHERE id = old.id;
  END;

-- Create trigger for automated deletion of dependent rows, this should normally be done using foreign keys 

CREATE TRIGGER delete_str3ds_base AFTER DELETE ON str3ds_base
  BEGIN
    DELETE FROM str3ds_absolute_time WHERE id = old.id;
    DELETE FROM str3ds_relative_time WHERE id = old.id;
    DELETE FROM str3ds_spatial_extent WHERE id = old.id;
    DELETE FROM str3ds_metadata WHERE id = old.id;
  END;

