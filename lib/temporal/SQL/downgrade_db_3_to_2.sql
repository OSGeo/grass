--#############################################################################
-- This SQL script downgrades TGIS DB from version 3 to version 2.
--
-- Author: Markus Metz
--#############################################################################

-- raster_metadata_table.sql
CREATE TEMPORARY TABLE raster_metadata_backup(
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
INSERT INTO raster_metadata_backup SELECT id,datatype,cols,rows,number_of_cells,nsres,ewres,min,max FROM raster_metadata;
DROP TABLE raster_metadata;
CREATE TABLE raster_metadata(
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
INSERT INTO raster_metadata SELECT id,datatype,cols,rows,number_of_cells,nsres,ewres,min,max FROM raster_metadata_backup;
DROP TABLE raster_metadata_backup;


-- strds_metadata_table.sql
CREATE TEMPORARY TABLE strds_metadata_backup(
  id VARCHAR NOT NULL,          -- Id of the space-time dataset, this is the primary key
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
  aggregation_type VARCHAR,     -- The aggregation type of the dataset (mean, min, max, ...) set by aggregation modules
  title VARCHAR,                -- Title of the space-time raster dataset
  description VARCHAR,          -- Detailed description of the space-time raster dataset
  command VARCHAR,              -- The command that was used to create the space time raster dataset
  PRIMARY KEY (id)
);
INSERT INTO strds_metadata_backup SELECT id,raster_register,number_of_maps,max_min,min_min,max_max,min_max,nsres_min,nsres_max,ewres_min,ewres_max,aggregation_type,title,description,command FROM strds_metadata;
DROP TABLE strds_metadata;
CREATE TABLE strds_metadata(
  id VARCHAR NOT NULL,          -- Id of the space-time dataset, this is the primary key
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
  aggregation_type VARCHAR,     -- The aggregation type of the dataset (mean, min, max, ...) set by aggregation modules
  title VARCHAR,                -- Title of the space-time raster dataset
  description VARCHAR,          -- Detailed description of the space-time raster dataset
  command VARCHAR,              -- The command that was used to create the space time raster dataset
  PRIMARY KEY (id)
);
INSERT INTO strds_metadata SELECT id,raster_register,number_of_maps,max_min,min_min,max_max,min_max,nsres_min,nsres_max,ewres_min,ewres_max,aggregation_type,title,description,command FROM strds_metadata_backup;
DROP TABLE strds_metadata_backup;

-- tgis_metadata
UPDATE tgis_metadata
  SET value = '2'
  WHERE key = 'tgis_db_version';
