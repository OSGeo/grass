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


CREATE TABLE  GRASS_MAP_base (
  id VARCHAR NOT NULL,                  -- The id (PK) is the unique identifier for all tables, it is based on name (layer) and mapset (name(:layer)@mapset) and is used as primary key
  name VARCHAR NOT NULL,                -- name of the grass map
  mapset VARCHAR NOT NULL,              -- mapset of the grass map
  layer VARCHAR,                        -- The layer id of the map, this is currently only in use by vector maps
  creator VARCHAR NOT NULL,
  temporal_type VARCHAR,                -- The temporal type of the grass map "absolute" or "relative" or NULL in case no time stamp is available
  creation_time TIMESTAMP NOT NULL,      -- The time of creation of the grass map
  PRIMARY KEY (id)
);

-- Relative valid time interval with start and end time
CREATE TABLE  GRASS_MAP_relative_time (
  id VARCHAR NOT NULL,          -- The id (PK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  start_time INTEGER,  -- The relative valid start time in 
  end_time INTEGER,    -- The relative valid end time in 
  unit VARCHAR,                 -- The relative time unit, available are "years, months, days, minutes, seconds"
  PRIMARY KEY (id)
);

CREATE TABLE  GRASS_MAP_absolute_time (
  id VARCHAR NOT NULL,   -- The id (PK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary key
  start_time TIMESTAMP,  --  Start of the valid time, can be NULL if no time information is available
  end_time TIMESTAMP,    --  End of the valid time, can be NULL if no time information is available or valid time is a single point in time
  PRIMARY KEY (id)
);

-- The spatial extent of a raster map

CREATE TABLE  GRASS_MAP_spatial_extent (
  id VARCHAR NOT NULL,                  -- The id (PK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary key
  -- below is the spatial extent of the map
  north DOUBLE PRECISION NOT NULL,
  south DOUBLE PRECISION NOT NULL,
  east DOUBLE PRECISION NOT NULL,
  west DOUBLE PRECISION NOT NULL,
  top DOUBLE PRECISION NOT NULL,
  bottom DOUBLE PRECISION NOT NULL,
  proj VARCHAR,
  PRIMARY KEY (id)
);

-- We have a specific table that stores the space time dataset ids in which the maps a registered

CREATE TABLE  GRASS_MAP_stds_register (
  id VARCHAR NOT NULL,                  -- The id (PK) is the unique identifier for all tables, it is based on name (layer) and mapset (name(:layer)@mapset) and is used as primary key
  registered_stds VARCHAR,              -- This column stores the names of all space time datasets in which a specific map is registered
  PRIMARY KEY (id)
);
