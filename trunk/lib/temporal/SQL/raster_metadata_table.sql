--#############################################################################
-- This SQL script generates the raster metadata table to store 
-- and metadata for SQL queries and temporal GIS support. 
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

-- The metadata table reflects most of the raster metadata available in grass

CREATE TABLE  raster_metadata (
  id VARCHAR NOT NULL,               -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary key
  datatype VARCHAR NOT NULL,
  cols INTEGER NOT NULL,
  rows INTEGER NOT NULL,
  number_of_cells INTEGER NOT NULL,
  nsres DOUBLE PRECISION NOT NULL,
  ewres DOUBLE PRECISION NOT NULL,
  min DOUBLE PRECISION,
  max DOUBLE PRECISION,
  PRIMARY KEY (id)
);


