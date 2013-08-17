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
  id VARCHAR NOT NULL,                  -- The id (PK) is the unique identifier for all tables, it is based on name (layer) and mapset (name(:layer)@mapset) and is used as primary key
  name VARCHAR NOT NULL,                -- name of the grass map
  mapset VARCHAR NOT NULL,              -- mapset of the grass map
  layer VARCHAR,                        -- The layer id of the map, this is currently only in use by vector maps
  creator VARCHAR NOT NULL,
  temporal_type VARCHAR,                -- The temporal type of the grass map "absolute" or "relative" or NULL in case no time stamp is available
  creation_time TIMESTAMP NOT NULL,      -- The time of creation of the grass map
-- Uncommented due to performance issues
--  modification_time TIMESTAMP NOT NULL,  -- The time of the last modification of the grass map
--  revision SMALLINT NOT NULL,           -- The revision number
  PRIMARY KEY (id)
);

CREATE INDEX GRASS_MAP_base_index ON GRASS_MAP_base (id);

-- Relative valid time interval with start and end time
CREATE TABLE  GRASS_MAP_relative_time (
  id VARCHAR NOT NULL,          -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  start_time INTEGER,  -- The relative valid start time in 
  end_time INTEGER,    -- The relative valid end time in 
  unit VARCHAR,                 -- The relative time unit, available are "years, months, days, minutes, seconds"
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX GRASS_MAP_relative_time_index ON GRASS_MAP_relative_time (id, start_time, end_time);

CREATE TABLE  GRASS_MAP_absolute_time (
  id VARCHAR NOT NULL,   -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  start_time TIMESTAMP,  --  Start of the valid time, can be NULL if no time information is available
  end_time TIMESTAMP,    --  End of the valid time, can be NULL if no time information is available or valid time is a single point in time
  timezone VARCHAR,      -- The timezone of the valid time stored as string. This is currently not in use. Instead the timezone is set in the datetime strings 
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX GRASS_MAP_absolute_time_index ON GRASS_MAP_absolute_time (id, start_time, end_time);

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
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX GRASS_MAP_spatial_extent_index ON GRASS_MAP_spatial_extent (id);
