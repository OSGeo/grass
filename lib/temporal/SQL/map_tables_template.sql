--#############################################################################
-- This SQL script generates the grass map tables to store time stamps, revision
-- and spatial extent for SQL queries and temporal GIS support.
-- Additionally several triggers are created for convenient functions
-- The grass map metadata is map specific (raster, raster3d and vector maps are 
-- supported)
--
-- The placeholder GRASS_MAP will be replaced by raster, raster3d and vector
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- GRASS_MAP is a placeholder for specific map type: raster, raster3d or vector

--PRAGMA foreign_keys = ON;

CREATE TABLE  GRASS_MAP_base (
  id VARCHAR NOT NULL,                  -- The id (PK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary key
  name VARCHAR NOT NULL,                -- name of the grass map
  mapset VARCHAR NOT NULL,              -- mapset of the grass map
  creator VARCHAR NOT NULL,
  temporal_type VARCHAR,                -- The temporal type of the grass map "absolute" or "relative" or NULL in case no time stamp is available
  creation_time TIMESTAMP NOT NULL,      -- The time of creation of the grass map
-- Uncommented due to performance issues
--  modification_time TIMESTAMP NOT NULL,  -- The time of the last modification of the grass map
--  revision SMALLINT NOT NULL,           -- The revision number
  PRIMARY KEY (id)
);

-- Relative valid time interval with start and end time
CREATE TABLE  GRASS_MAP_relative_time (
  id VARCHAR NOT NULL,          -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  start_time DOUBLE PRECISION,  -- The relative valid start time in [days]
  end_time DOUBLE PRECISION,    -- The relative valid end time in [days]
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE
);

CREATE TABLE  GRASS_MAP_absolute_time (
  id VARCHAR NOT NULL,   -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  start_time TIMESTAMP,  --  Start of the valid time, can be NULL if no time information is available
  end_time TIMESTAMP,    --  End of the valid time, can be NULL if no time information is available or valid time is a single point in time
  timezone SMALLINT,     -- The timezone of the valid time
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE
);

-- The spatial extent of a raster map

CREATE TABLE  GRASS_MAP_spatial_extent (
  id VARCHAR NOT NULL,                  -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreigen key
  -- below is the spatial extent of the map
  north DOUBLE PRECISION NOT NULL,
  south DOUBLE PRECISION NOT NULL,
  east DOUBLE PRECISION NOT NULL,
  west DOUBLE PRECISION NOT NULL,
  top DOUBLE PRECISION NOT NULL,
  bottom DOUBLE PRECISION NOT NULL,
  proj VARCHAR,
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE
);

-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 

--CREATE TRIGGER update_GRASS_MAP_absolute_time AFTER UPDATE ON GRASS_MAP_absolute_time 
--  BEGIN
--    UPDATE GRASS_MAP_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE GRASS_MAP_base SET revision = (revision + 1) WHERE id = old.id;
-- END;

--CREATE TRIGGER update_GRASS_MAP_relative_time AFTER UPDATE ON GRASS_MAP_relative_time 
--  BEGIN
--    UPDATE GRASS_MAP_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE GRASS_MAP_base SET revision = (revision + 1) WHERE id = old.id;
--  END;


--CREATE TRIGGER update_GRASS_MAP_spatial_extent AFTER UPDATE ON GRASS_MAP_spatial_extent 
--  BEGIN
--    UPDATE GRASS_MAP_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE GRASS_MAP_base SET revision = (revision + 1) WHERE id = old.id;
--  END;
