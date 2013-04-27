--#############################################################################
-- This SQL script generates the space time 3D raster dataset metadata table.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

CREATE TABLE  str3ds_metadata (
  id VARCHAR NOT NULL,          -- Id of the space time 3D raster dataset, this is the primary foreign key
  raster3d_register VARCHAR,    -- The id of the table in which the 3D raster maps are registered for this dataset
  number_of_maps INTEGER,       -- The number of registered 3D raster maps
  max_min DOUBLE PRECISION,     -- The minimal maximum of the registered 3D raster maps
  min_min DOUBLE PRECISION,     -- The minimal minimum of the registered 3D raster maps
  max_max DOUBLE PRECISION,     -- The maximal maximum of the registered 3D raster maps
  min_max DOUBLE PRECISION,     -- The maximal minimum of the registered 3D raster maps
  nsres_min DOUBLE PRECISION,   -- The lowest north-south resolution of the registered 3D raster maps
  nsres_max DOUBLE PRECISION,   -- The highest north-south resolution of the registered 3D raster maps
  ewres_min DOUBLE PRECISION,   -- The lowest east-west resolution of the registered 3D raster maps
  ewres_max DOUBLE PRECISION,   -- The highest east-west resolution of the registered 3D raster maps
  tbres_min DOUBLE PRECISION,   -- The lowest top-bottom resolution of the registered 3D raster maps
  tbres_max DOUBLE PRECISION,   -- The highest top-bottom resolution of the registered 3D raster maps
  title VARCHAR,                -- Title of the space time 3D raster dataset
  description VARCHAR,          -- Detailed description of the space time 3D raster dataset
  command VARCHAR,              -- The command that was used to create the space time 3D raster dataset
  PRIMARY KEY (id),  
  FOREIGN KEY (id) REFERENCES  str3ds_base (id) ON DELETE CASCADE
);

-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 
-- Uncommented due to performance issues
--CREATE TRIGGER update_str3ds_metadata AFTER UPDATE ON str3ds_metadata 
--  BEGIN
--    UPDATE str3ds_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE str3ds_base SET revision = (revision + 1) WHERE id = old.id;
--  END;
