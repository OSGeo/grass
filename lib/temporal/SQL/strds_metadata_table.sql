--#############################################################################
-- This SQL script generates the space time raster dataset metadata table.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

CREATE TABLE  strds_metadata (
  id VARCHAR NOT NULL,          -- Id of the space-time dataset, this is the primary foreign key
  raster_register VARCHAR,      -- The id of the table in which the raster maps are registered for this dataset
  number_of_maps INTEGER,       -- The number of registered raster maps
  max_min DOUBLE PRECISION,     -- The minimal maximum of the registered raster maps
  min_min DOUBLE PRECISION,     -- The minimal minimum of the registered raster maps
  max_max DOUBLE PRECISION,     -- The maximal maximum of the registered raster maps
  min_max DOUBLE PRECISION,     -- The maximal minimum of the registered raster maps
  nsres_min DOUBLE PRECISION,   -- The lowest north-south resolution of the registered raster maps
  nsres_max DOUBLE PRECISION,   -- The highest north-south resolution of the registered raster maps
  ewres_min DOUBLE PRECISION,   -- The lowest east-west resolution of the registered raster maps
  ewres_max DOUBLE PRECISION,   -- The highest east-west resolution of the registered raster maps
  title VARCHAR,                -- Title of the space-time raster dataset
  description VARCHAR,          -- Detailed description of the space-time raster dataset
  command VARCHAR,              -- The command that was used to create the space time raster dataset
  FOREIGN KEY (id) REFERENCES  strds_base (id) ON DELETE CASCADE ON UPDATE CASCADE
);
