--#############################################################################
-- This SQL script generates the raster metadata table to store 
-- and metadata for SQL queries and temporal GIS support. 
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

-- The metadata table reflects most of the raster metadata available in grass

CREATE TABLE  raster_metadata (
  id VARCHAR NOT NULL,               -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  strds_register VARCHAR,            -- The name of the table storing all space-time raster datasets in which this map is registered
  datatype VARCHAR NOT NULL,
  cols INTEGER NOT NULL,
  rows INTEGER NOT NULL,
  number_of_cells INTEGER NOT NULL,
  nsres DOUBLE PRECISION NOT NULL,
  ewres DOUBLE PRECISION NOT NULL,
  min DOUBLE PRECISION,
  max DOUBLE PRECISION,
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  raster_base (id) ON DELETE CASCADE
);

-- Create a trigger to update the modification time and revision number in case the metadata have been updated 
-- Uncommented due to performance issues
--CREATE TRIGGER update_raster_metadata AFTER UPDATE ON raster_metadata 
--  BEGIN
--    UPDATE raster_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE raster_base SET revision = (revision + 1) WHERE id = old.id;
--  END;

