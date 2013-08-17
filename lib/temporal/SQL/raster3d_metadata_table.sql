--#############################################################################
-- This SQL script generates the raster3d metadata table to store 
-- and metadata for SQL queries and temporal GIS support.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

-- The metadata table reflects most of the raster3d metadata available in grass

CREATE TABLE  raster3d_metadata (
  id VARCHAR NOT NULL,                  -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  str3ds_register VARCHAR, -- The name of the table storing all space-time raster3d datasets in which this map is registered
  datatype VARCHAR NOT NULL,
  cols INTEGER NOT NULL,
  rows INTEGER NOT NULL,
  depths INTEGER NOT NULL,
  number_of_cells INTEGER NOT NULL,
  nsres DOUBLE PRECISION NOT NULL,
  ewres DOUBLE PRECISION NOT NULL,
  tbres DOUBLE PRECISION NOT NULL,
  min DOUBLE PRECISION,
  max DOUBLE PRECISION,
  FOREIGN KEY (id) REFERENCES  raster3d_base (id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX raster3d_metadata_index ON raster3d_metadata (id);

