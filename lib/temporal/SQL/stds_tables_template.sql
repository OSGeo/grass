--#############################################################################
-- This SQL script generates the space time dataset tables to store time 
-- stamps and revision for SQL queries and temporal GIS support.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- STDS is a placeholder for specific space-time dataset type: strds, str3ds, stvds

CREATE TABLE  STDS_base (
  id VARCHAR NOT NULL,                 -- Id of the space-time dataset, name@mapset this is the primary key
  name VARCHAR NOT NULL,               -- name of the space-time dataset
  mapset VARCHAR NOT NULL,             -- mapset of the space-time dataset
  creator VARCHAR NOT NULL,            -- Name of the creator
  temporal_type VARCHAR NOT NULL,      -- The temporal type of the dataset "absolute" or "relative" 
  semantic_type VARCHAR NOT NULL,      -- The semantic data description used for aggregation/decomposition algorithm selection: min, max, mean or sum
  creation_time TIMESTAMP NOT NULL,    -- The time of creation of the space-time dataset
  modification_time TIMESTAMP NOT NULL,-- The time of the last modification of the space time dataset
  PRIMARY KEY (id)
);

CREATE TABLE  STDS_relative_time (
  id VARCHAR NOT NULL,            -- Id of the space-time dataset, this is the primary key
  start_time INTEGER,             -- The relative valid start time 
  end_time INTEGER,               -- The relative valid end time 
  granularity INTEGER,            -- The granularity 
  unit VARCHAR,                   -- The relative time unit, available are "years, months, days, minutes, seconds"
  map_time VARCHAR,               -- The temporal type of the registered maps, may be interval, point or mixed
  PRIMARY KEY (id)
);

CREATE TABLE  STDS_absolute_time (
  id VARCHAR NOT NULL,            -- Id of the space-time dataset, this is the primary key
  start_time TIMESTAMP,           -- Start of the valid time, can be NULL if no map is registered
  end_time TIMESTAMP,             -- End of the valid time, can be NULL if no map is registered
  granularity VARCHAR,            -- The granularity "NNN seconds, NNN minutes, NNN hours, NNN days, NNN months, NNN years"
  map_time VARCHAR,               -- The temporal type of the registered maps, may be interval, point or mixed
  PRIMARY KEY (id)
);

CREATE TABLE  STDS_spatial_extent (
  id VARCHAR NOT NULL,      -- Id of the space-time dataset, this is the primary key
  north DOUBLE PRECISION,   -- The spatial north extent, derived from the registered maps
  south DOUBLE PRECISION,   -- The spatial south extent, derived from the registered maps
  east DOUBLE PRECISION,    -- The spatial east extent, derived from the registered maps
  west DOUBLE PRECISION,    -- The spatial west extent, derived from the registered maps
  top DOUBLE PRECISION,     -- The spatial top extent, derived from the registered maps
  bottom DOUBLE PRECISION,  -- The spatial bottom extent, derived from the registered maps
  proj VARCHAR,      -- The projection of the space time dataset (XY of LL)
  PRIMARY KEY (id)
);



