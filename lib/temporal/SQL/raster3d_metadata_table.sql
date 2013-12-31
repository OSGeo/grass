--#############################################################################
-- This SQL script generates the raster3d metadata table to store 
-- and metadata for SQL queries and temporal GIS support.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################


-- The metadata table reflects most of the raster3d metadata available in grass

CREATE TABLE  raster3d_metadata (
  id VARCHAR NOT NULL,                  -- The id (PK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary key
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
  PRIMARY KEY (id)
);




